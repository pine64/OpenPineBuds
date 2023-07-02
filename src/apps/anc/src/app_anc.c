/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "app_anc.h"
#include "anc_assist.h"
#include "anc_process.h"
#include "anc_wnr.h"
#include "app_ibrt_keyboard.h"
#include "app_ibrt_ui.h"
#include "app_thread.h"
#include "apps.h"
#include "aud_section.h"
#include "audioflinger.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_bootmode.h"
#include "hal_codec.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "plat_types.h"
#include "pmu.h"
#include "tgt_hardware.h"

#include "app_status_ind.h"
#ifdef __SIMPLE_INTERNAL_PLAYER_SUPPORT__
#include "simple_internal_player.h"
#endif
#include "hal_aud.h"
#ifdef IBRT
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt.h"
#include "app_tws_ibrt_cmd_handler.h"
#endif

#include "co_math.h"
//#define ANC_MODE_SWITCH_WITHOUT_FADE //Comment this line if you need fade
// function between anc mode

#ifndef _ANC_FADE_STACK_SIZE
#define _ANC_FADE_STACK_SIZE (1 * 1024)
#endif

static osThreadId anc_fade_thread_tid;

#define FADE_IN 0x0001
#define FADE_OUT 0x0002
#define CHANGE_FROM_ANC_TO_TT_DIRECTLY 0x0003

static void anc_fade_thread(void const *argument);
osThreadDef(anc_fade_thread, osPriorityBelowNormal, 1, _ANC_FADE_STACK_SIZE,
            "anc_fade_thread");

extern uint8_t app_poweroff_flag;
uint32_t app_anc_get_anc_status(void);

void app_anc_disable(void);
void app_anc_close_anc(void);
osStatus osDelay(uint32_t millisec);
void app_anc_timer_set(uint32_t request, uint32_t delay);
void app_anc_timer_close(void);
extern bool app_mode_is_usbaudio(void); // 96k 384k
extern bool app_mode_is_i2s(void);      // 96k 384k
void app_anc_init_timer(void);
extern void i2s_player_send_stop(void);
extern void fb_anti_howl_start(void);
extern void fb_anti_howl_stop(void);
extern void hal_codec_reconfig_pll_freq(enum AUD_SAMPRATE_T dac_rate,
                                        enum AUD_SAMPRATE_T adc_rate);
extern void anc_status_sync(void);
extern void anc_status_sync_init(void);

extern uint8_t is_sbc_mode(void);
extern uint8_t is_sco_mode(void);

void app_anc_switch_set_edge(uint8_t down_edge);

#ifdef __ANC_STICK_SWITCH_USE_GPIO__
typedef void (*ANC_KEY_CALLBACK)(uint8_t status);
static ANC_KEY_CALLBACK app_switch_callback;
#endif

extern int app_shutdown(void);
extern int app_reset(void);

extern void analog_aud_codec_speaker_enable(bool en);

#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
extern bool anc_single_mode_is_on(void);
#endif
enum {
  ANC_STATUS_OFF = 0,
  ANC_STATUS_ON,
  ANC_STATUS_WAITING_ON,
  ANC_STATUS_WAITING_OFF,
  ANC_STATUS_INIT_ON,
  ANC_STATUS_NONE
};

enum {
  ANC_EVENT_INIT = 0,
  ANC_EVENT_OPEN,
  ANC_EVENT_CLOSE,
  ANC_EVENT_FADE_IN,
  ANC_EVENT_FADE_OUT,
  ANC_EVENT_CHANGE_SAMPLERATE,
  ANC_EVENT_CHANGE_STATUS,
  ANC_EVENT_HOWL_PROCESS,
  ANC_EVENT_SYNC_STATUS,
  ANC_EVENT_SYNC_INIT,
  ANC_EVENT_PWR_KEY_MONITOR,
  ANC_EVENT_PWR_KEY_MONITOR_REBOOT,
  ANC_EVENT_SWITCH_KEY_DEBONCE,
  SIMPLE_PLAYER_CLOSE_CODEC_EVT,
  SIMPLE_PLAYER_DELAY_STOP_EVT,
  ANC_EVENT_NONE
};

static uint32_t anc_work_status = ANC_STATUS_OFF;
static uint32_t anc_timer_request = ANC_EVENT_NONE;

static enum AUD_SAMPRATE_T anc_sample_rate[AUD_STREAM_NUM];

static HWTIMER_ID anc_timerid = NULL;
#define anc_gain_adjust_gap (60)

#define anc_init_switch_off_time (MS_TO_TICKS(1000 * 60 * 2))
#define anc_auto_power_off_time (MS_TO_TICKS(1000 * 60 * 60))
#define anc_switch_on_time (MS_TO_TICKS(600))
#define anc_close_delay_time (MS_TO_TICKS(1000 * 20))
#define anc_pwr_key_monitor_time (MS_TO_TICKS(1500))
#define anc_switch_key_debonce_time (MS_TO_TICKS(40))

#if 0 // def ANC_FB_CHECK
uint32_t app_fb_check_threshold_vale=0x4000000;
enum ANC_TYPE_T app_anc_check_type = ANC_FEEDBACK;
uint32_t app_fb_check_delay_time=((MS_TO_TICKS(5000)));
#endif

#ifdef ANC_FB_CHECK
uint32_t app_fb_check_threshold_vale = 0x4000000;
enum ANC_TYPE_T app_anc_check_type = ANC_FEEDFORWARD;
uint32_t app_fb_check_decrease_resume_delay_time = (MS_TO_TICKS(400));
uint8_t app_fb_check_decrease_value = 18;
uint32_t app_fb_check_delay_time = (MS_TO_TICKS(3000));
uint8_t app_fb_check_delay_time_flag = 0;
uint32_t app_fb_check_delay_time2 = (MS_TO_TICKS(3000));
uint32_t app_fb_check_threshold_count = 20;
uint8_t app_gain_resume_status = 1;
#endif

enum {
  APP_ANC_IDLE = 0,
  APP_ANC_FADE_IN,
  APP_ANC_FADE_OUT,
};

uint32_t app_anc_fade_status = APP_ANC_IDLE;

static bool app_init_done = false;
#ifdef ANC_SWITCH_PIN
static bool app_anc_shutdown = false;
#endif
bool anc_set_dac_pa_delay = false;
static enum ANC_INDEX anc_coef_idx = 0;

extern void analog_aud_enable_dac_pa(uint8_t dac);

bool app_anc_is_on(void) { return (ANC_STATUS_ON == anc_work_status); }

uint32_t app_anc_get_sample_rate(void) {
  return (uint32_t)anc_sample_rate[AUD_STREAM_CAPTURE];
}

void app_anc_set_coef_idx(uint8_t idx) { anc_coef_idx = idx; }

uint8_t app_anc_get_coef_idx(void) { return anc_coef_idx; }

#if defined(IBRT)

static enum ANC_INDEX anc_peer_coef_idx = 0;
static bool anc_status_sync_flag = false;

void app_anc_set_peer_coef_idx(uint8_t idx) { anc_peer_coef_idx = idx; }

uint8_t app_anc_get_peer_coef_idx(void) { return anc_peer_coef_idx; }

void app_anc_set_status_sync_flag(bool status) {
  anc_status_sync_flag = status;
}

bool app_anc_get_status_sync_flag(void) { return anc_status_sync_flag; }

#endif

void app_anc_set_init_done(void) { app_init_done = true; }

void app_anc_switch_turnled(bool on);

#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__

static ANC_KEY_CALLBACK app_pwr_key_monitor_callback;

static void app_pwr_key_monitor_handler(enum HAL_GPIO_PIN_T pin) {
  uint8_t gpio_val = hal_gpio_pin_get_val(pin);

  TRACE(2, " %s :%d ", __func__, gpio_val);
  if (app_pwr_key_monitor_callback)
    app_pwr_key_monitor_callback(gpio_val);
}

bool app_pwr_key_monitor_get_val(void) {
  return (bool)hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)PWR_KEY_MONITOR_PIN);
}

void app_pwr_key_monitor_set_int_edge(uint8_t down_edge) {
  struct HAL_GPIO_IRQ_CFG_T gpiocfg;
  gpiocfg.irq_enable = true;
  gpiocfg.irq_debounce = true;
  gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
  gpiocfg.irq_polarity = (down_edge == 1) ? HAL_GPIO_IRQ_POLARITY_LOW_FALLING
                                          : HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
  gpiocfg.irq_handler = app_pwr_key_monitor_handler;
  TRACE(2, " %s :%d ", __func__, down_edge);
  hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)PWR_KEY_MONITOR_PIN, &gpiocfg);
}

void app_pwr_key_monitor_init(ANC_KEY_CALLBACK callback) {
  app_pwr_key_monitor_callback = callback;
}
#endif

#ifdef ANC_LED_PIN
void app_anc_switch_turnled(bool on) {
  TRACE(2, " %s on %d ", __func__, on);

  if (cfg_anc_led.pin != HAL_IOMUX_PIN_NUM) {
    if (on) {
      TRACE(0, "on\n");
      hal_gpio_pin_set((enum HAL_GPIO_PIN_T)cfg_anc_led.pin);
    } else {
      TRACE(0, "off\n");
      hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)cfg_anc_led.pin);
    }
  }
}
#endif

#ifdef __ANC_STICK_SWITCH_USE_GPIO__

static void app_anc_switch_int_handler(enum HAL_GPIO_PIN_T pin) {
  // uint8_t gpio_val;
  uint8_t gpio_val = hal_gpio_pin_get_val(pin);
  TRACE(3, " %s ,pin %d,status %d", __func__, pin, gpio_val);
#ifdef ANC_LED_PIN
  if (gpio_val)
    app_anc_switch_turnled(true);
  else
    app_anc_switch_turnled(false);
#endif
  app_anc_switch_set_edge(gpio_val);

  app_anc_timer_set(ANC_EVENT_SWITCH_KEY_DEBONCE, anc_switch_key_debonce_time);
  // if (app_switch_callback)
  //    app_switch_callback(gpio_val);
  // app_anc_key_handler();
}

void app_anc_switch_init(ANC_KEY_CALLBACK callback) {
  app_switch_callback = callback;
}
#endif

#ifdef ANC_SWITCH_PIN
bool app_anc_set_reboot(void) {
  return (app_anc_shutdown &&
          hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)ANC_SWITCH_PIN));
}

bool app_anc_switch_get_val(void) {
  return (bool)hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)ANC_SWITCH_PIN);
}

void app_anc_switch_set_edge(uint8_t down_edge) {
  struct HAL_GPIO_IRQ_CFG_T gpiocfg;
  gpiocfg.irq_enable = true;
  gpiocfg.irq_debounce = true;
  gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
  gpiocfg.irq_polarity = (down_edge == 1) ? HAL_GPIO_IRQ_POLARITY_LOW_FALLING
                                          : HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
  gpiocfg.irq_handler = app_anc_switch_int_handler;
  TRACE(2, " %s :%d ", __func__, down_edge);
  hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)ANC_SWITCH_PIN, &gpiocfg);
}
#endif

int __anc_usb_app_fadein(enum ANC_TYPE_T type) {
  int32_t gain0_curr, gain1_curr;
  int32_t gain0_tg, gain1_tg;

  anc_get_gain(&gain0_curr, &gain1_curr, type);
  anc_get_cfg_gain(&gain0_tg, &gain1_tg, type);

  if (gain0_tg > 0) {
    if (gain0_curr < gain0_tg) {
      gain0_curr++;
    }
  } else {
    if (gain0_curr > gain0_tg) {
      gain0_curr--;
    }
  }

  if (gain1_tg > 0) {
    if (gain1_curr < gain1_tg) {
      gain1_curr++;
    }
  } else {
    if (gain1_curr > gain1_tg) {
      gain1_curr--;
    }
  }
  hal_sys_timer_delay_us(25);

  anc_set_gain(gain0_curr, gain1_curr, type);

  if ((gain0_curr == gain0_tg) && (gain1_curr == gain1_tg)) {
    TRACE(3, "[%s] cur gain: %d %d", __func__, gain0_curr, gain1_curr);
    return 0;
  }

  return 1;
}

int __anc_usb_app_fadeout(enum ANC_TYPE_T type) {
  int32_t gain0_curr, gain1_curr;

  anc_get_gain(&gain0_curr, &gain1_curr, type);

  if (gain0_curr > 0) {
    gain0_curr--;
  } else if (gain0_curr < 0) {
    gain0_curr++;
  }

  if (gain1_curr > 0) {
    gain1_curr--;
  } else if (gain1_curr < 0) {
    gain1_curr++;
  }

  //  gain0_curr = gain1_curr = 0 ;
  hal_sys_timer_delay_us(25);

  anc_set_gain(gain0_curr, gain1_curr, type);

  if ((gain0_curr == 0) && (gain1_curr == 0)) {
    TRACE(3, "[%s] gain: %d, %d", __func__, gain0_curr, gain1_curr);
    return 0;
  }

  return 1;
}

#if defined(ANC_FF_ENABLED) && defined(ANC_FB_ENABLED)
int __anc_usb_app_fadein_ff_fb(void) {
  int32_t gain0_curr, gain1_curr;
  int32_t gain2_curr, gain3_curr;

  int32_t gain0_tg, gain1_tg;
  int32_t gain2_tg, gain3_tg;

  anc_get_gain(&gain0_curr, &gain1_curr, ANC_FEEDFORWARD);
  anc_get_gain(&gain2_curr, &gain3_curr, ANC_FEEDBACK);
  anc_get_cfg_gain(&gain0_tg, &gain1_tg, ANC_FEEDFORWARD);
  anc_get_cfg_gain(&gain2_tg, &gain3_tg, ANC_FEEDBACK);

  uint32_t random_factor = rand();
  random_factor %= 5;
  if (random_factor == 0)
    random_factor++;

  uint32_t random_factor_inverse = 5 - random_factor;
  if (random_factor_inverse == 0)
    random_factor_inverse++;

  random_factor_inverse = 1;

  if (is_sbc_mode() || is_sco_mode()) {
    if (gain0_tg > 0) {
      if (gain0_curr < gain0_tg) {
        gain0_curr = MIN(gain0_curr + random_factor_inverse, gain0_tg);
      }
    } else {
      if (gain0_curr > gain0_tg) {
        gain0_curr = MAX(gain0_curr - random_factor_inverse, gain0_tg);
      }
    }

    if (gain1_tg > 0) {
      if (gain1_curr < gain1_tg) {
        gain1_curr = MIN(gain1_curr + random_factor_inverse, gain1_tg);
      }
    } else {
      if (gain1_curr > gain1_tg) {
        gain1_curr = MAX(gain1_curr - random_factor_inverse, gain1_tg);
      }
    }

    if (gain2_tg > 0) {
      if (gain2_curr < gain2_tg) {
        gain2_curr = MIN(gain2_curr + random_factor_inverse, gain2_tg);
      }
    } else {
      if (gain2_curr > gain2_tg) {
        gain2_curr = MAX(gain2_curr - random_factor_inverse, gain2_tg);
      }
    }

    if (gain3_tg > 0) {
      if (gain3_curr < gain3_tg) {
        gain3_curr = MIN(gain3_curr + random_factor_inverse, gain3_tg);
      }
    } else {
      if (gain3_curr > gain3_tg) {
        gain3_curr = MAX(gain3_curr - random_factor_inverse, gain3_tg);
      }
    }
  } else {
    if (gain0_tg > 0) {
      if (gain0_curr < gain0_tg) {
        gain0_curr = MIN(gain0_curr + random_factor_inverse, gain0_tg);
      }
    } else {
      if (gain0_curr > gain0_tg) {
        gain0_curr = MAX(gain0_curr - random_factor_inverse, gain0_tg);
      }
    }

    if (gain1_tg > 0) {
      if (gain1_curr < gain1_tg) {
        gain1_curr = MIN(gain1_curr + random_factor_inverse, gain1_tg);
      }
    } else {
      if (gain1_curr > gain1_tg) {
        gain1_curr = MAX(gain1_curr - random_factor_inverse, gain1_tg);
      }
    }

    if (gain2_tg > 0) {
      if (gain2_curr < gain2_tg) {
        gain2_curr = MIN(gain2_curr + random_factor_inverse, gain2_tg);
      }
    } else {
      if (gain2_curr > gain2_tg) {
        gain2_curr = MAX(gain2_curr - random_factor_inverse, gain2_tg);
      }
    }

    if (gain3_tg > 0) {
      if (gain3_curr < gain3_tg) {
        gain3_curr = MIN(gain3_curr + random_factor_inverse, gain3_tg);
      }
    } else {
      if (gain3_curr > gain3_tg) {
        gain3_curr = MAX(gain3_curr - random_factor_inverse, gain3_tg);
      }
    }
  }

  anc_set_gain(gain0_curr, gain1_curr, ANC_FEEDFORWARD);
  anc_set_gain(gain2_curr, gain3_curr, ANC_FEEDBACK);

  // TRACE(5,"[%s] cur gain: %d %d %d %d", __func__, gain0_curr, gain1_curr,
  // gain2_curr, gain3_curr);

  // osDelay(random_factor);

  hal_sys_timer_delay_us(40);
  if (gain0_curr % 40 == 0) {
    // osDelay(random_factor);
  }

  if ((gain0_curr == gain0_tg) && (gain1_curr == gain1_tg) &&
      (gain2_curr == gain2_tg) && (gain3_curr == gain3_tg)) {
    anc_disable_gain_updated_when_pass0(1);

    anc_set_gain(gain0_curr, gain1_curr, ANC_FEEDFORWARD);
    anc_set_gain(gain2_curr, gain3_curr, ANC_FEEDBACK);
    TRACE(5, "[%s] end cur gain: %d, %d, %d, %d", __func__, gain0_curr,
          gain1_curr, gain2_curr, gain3_curr);
    return 0;
  }
  return 1;
}

int __anc_usb_app_fadeout_ff_fb(void) {
  int32_t gain0_curr, gain1_curr;
  int32_t gain2_curr, gain3_curr;
  anc_get_gain(&gain0_curr, &gain1_curr, ANC_FEEDFORWARD);
  anc_get_gain(&gain2_curr, &gain3_curr, ANC_FEEDBACK);

  uint32_t random_factor = rand();
  random_factor = random_factor % 5;
  if (random_factor == 0)
    random_factor++;

  uint32_t random_factor_inverse = 5 - random_factor;
  if (random_factor_inverse == 0)
    random_factor_inverse++;

  random_factor_inverse = 1;

  if (is_sbc_mode() || is_sco_mode()) {
    if (gain0_curr > 0) {
      gain0_curr = MAX(gain0_curr - random_factor_inverse, 0);
    } else if (gain0_curr < 0) {
      gain0_curr = MIN(gain0_curr + random_factor_inverse, 0);
    }

    if (gain1_curr > 0) {
      gain1_curr = MAX(gain1_curr - random_factor_inverse, 0);
    } else if (gain1_curr < 0) {
      gain1_curr = MIN(gain1_curr + random_factor_inverse, 0);
    }

    if (gain2_curr > 0) {
      gain2_curr = MAX(gain2_curr - random_factor_inverse, 0);
    } else if (gain2_curr < 0) {
      gain2_curr = MIN(gain2_curr + random_factor_inverse, 0);
    }

    if (gain3_curr > 0) {
      gain3_curr = MAX(gain3_curr - random_factor_inverse, 0);
    } else if (gain3_curr < 0) {
      gain3_curr = MIN(gain3_curr + random_factor_inverse, 0);
    }
  } else {
    if (gain0_curr > 0) {
      gain0_curr = MAX(gain0_curr - random_factor_inverse, 0);
    } else if (gain0_curr < 0) {
      gain0_curr = MIN(gain0_curr + random_factor_inverse, 0);
    }

    if (gain1_curr > 0) {
      gain1_curr = MAX(gain1_curr - random_factor_inverse, 0);
    } else if (gain1_curr < 0) {
      gain1_curr = MIN(gain1_curr + random_factor_inverse, 0);
    }

    if (gain2_curr > 0) {
      gain2_curr = MAX(gain2_curr - random_factor_inverse, 0);
    } else if (gain2_curr < 0) {
      gain2_curr = MIN(gain2_curr + random_factor_inverse, 0);
    }

    if (gain3_curr > 0) {
      gain3_curr = MAX(gain3_curr - random_factor_inverse, 0);
    } else if (gain3_curr < 0) {
      gain3_curr = MIN(gain3_curr + random_factor_inverse, 0);
    }
  }

  anc_set_gain(gain0_curr, gain1_curr, ANC_FEEDFORWARD);
  anc_set_gain(gain2_curr, gain3_curr, ANC_FEEDBACK);
  // TRACE(5,"[%s] cur gain: %d %d %d %d", __func__, gain0_curr, gain1_curr,
  // gain2_curr, gain3_curr);

  // osDelay(random_factor);

  hal_sys_timer_delay_us(40);
  if (gain0_curr % 40 == 0) {
    // osDelay(random_factor);
  }

  if ((gain0_curr == 0) && (gain1_curr == 0) && (gain2_curr == 0) &&
      (gain3_curr == 0)) {
    anc_disable_gain_updated_when_pass0(1);

    anc_set_gain(gain0_curr, gain1_curr, ANC_FEEDFORWARD);
    anc_set_gain(gain2_curr, gain3_curr, ANC_FEEDBACK);
    TRACE(5, "[%s] end cur gain: %d, %d, %d, %d", __func__, gain0_curr,
          gain1_curr, gain2_curr, gain3_curr);
    return 0;
  }

  return 1;
}
#endif

void anc_gain_fade_handle(void) {
  TRACE(2, " %s %d ", __func__, app_anc_fade_status);
  if (app_anc_fade_status == APP_ANC_FADE_OUT) {
    osSignalSet(anc_fade_thread_tid, FADE_OUT);
  }

  if (app_anc_fade_status == APP_ANC_FADE_IN) {
#ifdef ANC_LED_PIN
    app_anc_switch_turnled(true);
#endif

    osSignalSet(anc_fade_thread_tid, FADE_IN);

    if (anc_set_dac_pa_delay) {
      anc_set_dac_pa_delay = false;
      //          osDelay(600);
      //          analog_aud_enable_dac_pa(48);
    }
  }
}

void anc_fade_thread(void const *argument) {

  osEvent evt;
  evt.status = 0;
  uint32_t singles = 0;

  while (1) {
    evt = osSignalWait(0, osWaitForever);

    singles = evt.value.signals;
    TRACE(2, "anc_fade_thread. %d", singles);

    if (evt.status == osEventSignal) {

      switch (singles) {

      case FADE_IN:
#if defined(ANC_FF_ENABLED) && defined(ANC_FB_ENABLED)
        anc_disable_gain_updated_when_pass0(0);
        while (__anc_usb_app_fadein_ff_fb())
          ;
#else
#ifdef ANC_FF_ENABLED
        while (__anc_usb_app_fadein(ANC_FEEDFORWARD))
          ;
#endif
#ifdef ANC_FB_ENABLED
        while (__anc_usb_app_fadein(ANC_FEEDBACK))
          ;
#endif
#endif
        app_anc_fade_status = APP_ANC_IDLE;

        break;
      case FADE_OUT:

#if defined(ANC_FF_ENABLED) && defined(ANC_FB_ENABLED)
        anc_disable_gain_updated_when_pass0(0);
        while (__anc_usb_app_fadeout_ff_fb())
          ;
#else
#ifdef ANC_FF_ENABLED
        while (__anc_usb_app_fadeout(ANC_FEEDFORWARD))
          ;
#endif
#ifdef ANC_FB_ENABLED
        while (__anc_usb_app_fadeout(ANC_FEEDBACK))
          ;
#endif
#endif

        app_anc_fade_status = APP_ANC_IDLE;

#ifdef ANC_FB_CHECK
        hal_codec_anc_fb_check_set_irq_handler(anc_fb_check_irq_handler);

        anc_fb_check_param();
#endif
        break;
      case CHANGE_FROM_ANC_TO_TT_DIRECTLY:
#if defined(ANC_FF_ENABLED) && defined(ANC_FB_ENABLED)
        anc_disable_gain_updated_when_pass0(0);
        while (__anc_usb_app_fadeout_ff_fb())
          ;
#else
#ifdef ANC_FF_ENABLED
        while (__anc_usb_app_fadeout(ANC_FEEDFORWARD))
          ;
#endif

#ifdef ANC_FB_ENABLED
        while (__anc_usb_app_fadeout(ANC_FEEDBACK))
          ;
#endif
#endif

#if defined(ANC_FF_ENABLED) && defined(ANC_FB_ENABLED)
        anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                        ANC_FEEDFORWARD, ANC_GAIN_DELAY);
        anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                        ANC_FEEDBACK, ANC_GAIN_DELAY);
#ifdef AUDIO_ANC_FB_MC_HW
        anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                        ANC_MUSICCANCLE, ANC_GAIN_DELAY);
#endif
#else
#ifdef ANC_FF_ENABLED
        anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                        ANC_FEEDFORWARD, ANC_GAIN_DELAY);
#endif

#ifdef ANC_FB_ENABLED
        anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                        ANC_FEEDBACK, ANC_GAIN_DELAY);
#endif
#endif

#if defined(ANC_FF_ENABLED) && defined(ANC_FB_ENABLED)
        anc_disable_gain_updated_when_pass0(0);
        while (__anc_usb_app_fadein_ff_fb())
          ;
#else
#ifdef ANC_FF_ENABLED
        while (__anc_usb_app_fadein(ANC_FEEDFORWARD))
          ;
#endif

#ifdef ANC_FB_ENABLED
        while (__anc_usb_app_fadein(ANC_FEEDBACK))
          ;
#endif
#endif

        app_anc_fade_status = APP_ANC_IDLE;

        // recommand to play "ANC SWITCH" prompt here...

        break;
      default:
        break;
      }

    } else {
      TRACE(2, "anc fade thread evt error");
      continue;
    }
  }
}

void app_anc_gain_fadein(void) {
  APP_MESSAGE_BLOCK msg;
  TRACE(1, " %s ", __func__);
  if (app_poweroff_flag)
    return;

  app_anc_fade_status = APP_ANC_FADE_IN;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_FADE_IN;
  app_mailbox_put(&msg);
}

void app_anc_post_anc_codec_close(void) {
  APP_MESSAGE_BLOCK msg;
  TRACE(1, " %s ", __func__);
  if (app_poweroff_flag)
    return;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = SIMPLE_PLAYER_CLOSE_CODEC_EVT;
  app_mailbox_put(&msg);
}

void app_anc_gain_fadeout(void) {
  APP_MESSAGE_BLOCK msg;
  TRACE(1, " %s ", __func__);
  if (app_poweroff_flag)
    return;

  app_anc_fade_status = APP_ANC_FADE_OUT;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_FADE_OUT;
  app_mailbox_put(&msg);
}

void app_anc_status_post(uint8_t status) {
  APP_MESSAGE_BLOCK msg;
  TRACE(2, " %s status %d", __func__, status);
  if (app_poweroff_flag)
    return;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_CHANGE_STATUS;
  msg.msg_body.message_Param0 = status;
  app_mailbox_put(&msg);
}

void app_anc_status_sync(uint8_t status) {
  APP_MESSAGE_BLOCK msg;
  TRACE(2, " %s status %d", __func__, status);
  if (app_poweroff_flag)
    return;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_SYNC_STATUS;
  msg.msg_body.message_Param0 = status;
  app_mailbox_put(&msg);
}

void app_anc_status_sync_init(void) {
  APP_MESSAGE_BLOCK msg;
  TRACE(1, " %s ", __func__);
  if (app_poweroff_flag)
    return;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_SYNC_INIT;
  app_mailbox_put(&msg);
}

void app_anc_do_init(void) {
  APP_MESSAGE_BLOCK msg;
  TRACE(1, " %s DO INIT", __func__);
  if (app_poweroff_flag)
    return;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_INIT;
  app_mailbox_put(&msg);
}

void app_anc_send_howl_evt(uint32_t howl) {
  APP_MESSAGE_BLOCK msg;
  TRACE(2, " %s %d", __func__, howl);
  if (app_poweroff_flag)
    return;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_HOWL_PROCESS;
  msg.msg_body.message_Param0 = howl;
  app_mailbox_put(&msg);
}

void app_anc_send_pwr_key_monitor_evt(uint8_t level) {
  APP_MESSAGE_BLOCK msg;
  TRACE(2, " %s %d", __func__, level);
  if (app_poweroff_flag)
    return;

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = ANC_EVENT_PWR_KEY_MONITOR;
  msg.msg_body.message_Param0 = level;
  app_mailbox_put(&msg);
}

extern void simple_player_delay_stop(void);

void app_anc_post_simplayer_stop_evt(void) {
  APP_MESSAGE_BLOCK msg;
  TRACE(1, " %s d", __func__);

  msg.mod_id = APP_MODUAL_ANC;
  msg.msg_body.message_id = SIMPLE_PLAYER_DELAY_STOP_EVT;
  msg.msg_body.message_Param0 = 0;
  app_mailbox_put(&msg);
}

bool anc_enabled(void) { return (anc_work_status == ANC_STATUS_ON); }

void app_anc_resample(uint32_t res_ratio, uint32_t *in, uint32_t *out,
                      uint32_t samples) {
  uint32_t flag = int_lock();
  for (int i = samples; i > 0; i--) {
    for (int j = 0; j < res_ratio; j++) {
      *(out + (i - 1) * res_ratio + j) = *(in + i - 1);
    }
  }
  int_unlock(flag);
}

void app_anc_select_coef(void) {
  TRACE(2, "enter %s %d\n", __FUNCTION__, __LINE__);

#ifdef ANC_FF_ENABLED
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_FEEDFORWARD, ANC_GAIN_DELAY);
#endif

#ifdef ANC_FB_ENABLED
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_FEEDBACK, ANC_GAIN_DELAY);
#endif

#ifdef AUDIO_ANC_FB_MC_HW
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_MUSICCANCLE, ANC_GAIN_NO_DELAY);
#endif

  TRACE(2, "exit %s %d\n", __FUNCTION__, __LINE__);
}

void app_anc_enable(void) {
  TRACE(2, "enter %s %d\n", __FUNCTION__, __LINE__);
// anc_active_codec();
#ifndef __SIMPLE_INTERNAL_PLAYER_SUPPORT__
  analog_aud_codec_speaker_enable(true);
#endif
#ifdef ANC_FF_ENABLED
  anc_open(ANC_FEEDFORWARD);
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_FEEDFORWARD, ANC_GAIN_DELAY);
#endif

#ifdef ANC_FB_ENABLED
  anc_open(ANC_FEEDBACK);
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_FEEDBACK, ANC_GAIN_DELAY);
#endif

#ifdef AUDIO_ANC_FB_MC_HW
  anc_open(ANC_MUSICCANCLE);
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_MUSICCANCLE, ANC_GAIN_NO_DELAY);
#endif

#ifdef AUDIO_ANC_FB_MC_HW
  int32_t gain_ch_l = 0;
  int32_t gain_ch_r = 0;

  anc_get_cfg_gain(&gain_ch_l, &gain_ch_r, ANC_MUSICCANCLE);
  anc_set_gain(gain_ch_l, gain_ch_r, ANC_MUSICCANCLE);
#endif

#if defined(ANC_WNR_ENABLED)
  if (is_sco_mode()) {
    anc_wnr_open(ANC_WNR_OPEN_MODE_CONFIGURE);
  } else {
    anc_wnr_open(ANC_WNR_OPEN_MODE_STANDALONE);
  }
#endif

#if defined(ANC_ASSIST_ENABLED)
  anc_assist_open(ANC_ASSIST_STANDALONE);
#endif

  TRACE(2, "exit %s %d\n", __FUNCTION__, __LINE__);
}

void app_anc_disable(void) {
  TRACE(2, "enter %s %d\n", __FUNCTION__, __LINE__);

#if defined(ANC_WNR_ENABLED)
  anc_wnr_close();
#endif

#if defined(ANC_ASSIST_ENABLED)
  anc_assist_close();
#endif

#ifdef AUDIO_ANC_FB_MC_HW
  anc_close(ANC_MUSICCANCLE);
#endif

#ifdef ANC_FF_ENABLED
  anc_close(ANC_FEEDFORWARD);
#endif

#ifdef ANC_FB_ENABLED
  anc_close(ANC_FEEDBACK);
#endif

  TRACE(2, "exit %s %d\n", __FUNCTION__, __LINE__);
}

static void anc_sample_rate_change(enum AUD_STREAM_T stream,
                                   enum AUD_SAMPRATE_T rate,
                                   enum AUD_SAMPRATE_T *new_play,
                                   enum AUD_SAMPRATE_T *new_cap) {
  enum AUD_SAMPRATE_T play_rate, cap_rate;

  if (anc_sample_rate[stream] != rate) {
#ifdef CHIP_BEST1000
    if (stream == AUD_STREAM_PLAYBACK) {
      play_rate = rate;
      cap_rate = rate * (anc_sample_rate[AUD_STREAM_CAPTURE] /
                         anc_sample_rate[AUD_STREAM_PLAYBACK]);
    } else {
      play_rate = rate / (anc_sample_rate[AUD_STREAM_CAPTURE] /
                          anc_sample_rate[AUD_STREAM_PLAYBACK]);
      cap_rate = rate;
    }
#else
    play_rate = rate;
    cap_rate = rate;
#ifdef ANC_FF_ENABLED
    anc_select_coef(play_rate, anc_coef_idx, ANC_FEEDFORWARD,
                    ANC_GAIN_NO_DELAY);
#endif

#ifdef ANC_FB_ENABLED
    anc_select_coef(play_rate, anc_coef_idx, ANC_FEEDBACK, ANC_GAIN_NO_DELAY);
#endif

#ifdef AUDIO_ANC_FB_MC_HW
    anc_select_coef(play_rate, anc_coef_idx, ANC_MUSICCANCLE,
                    ANC_GAIN_NO_DELAY);
#endif

#endif // CHIP_BEST1000

    TRACE(5, "%s: Update anc sample rate from %u/%u to %u/%u", __func__,
          anc_sample_rate[AUD_STREAM_PLAYBACK],
          anc_sample_rate[AUD_STREAM_CAPTURE], play_rate, cap_rate);

    if (new_play) {
      *new_play = play_rate;
    }
    if (new_cap) {
      *new_cap = cap_rate;
    }

    anc_sample_rate[AUD_STREAM_PLAYBACK] = play_rate;
    anc_sample_rate[AUD_STREAM_CAPTURE] = cap_rate;
  }
}

void app_anc_open_anc(void) {
  enum AUD_SAMPRATE_T playback_rate;
  enum AUD_SAMPRATE_T capture_rate;
  AF_ANC_HANDLER handler;

  TRACE(2, "enter %s %d\n", __FUNCTION__, __LINE__);

  handler = anc_sample_rate_change;

#ifdef __SIMPLE_INTERNAL_PLAYER_SUPPORT__
  app_start_player_by_anc();
#endif

  if (anc_sample_rate[AUD_STREAM_PLAYBACK] == AUD_SAMPRATE_NULL) {
#ifdef CHIP_BEST1000
    anc_sample_rate[AUD_STREAM_PLAYBACK] = AUD_SAMPRATE_96000;
    anc_sample_rate[AUD_STREAM_CAPTURE] = AUD_SAMPRATE_384000;
#else // !CHIP_BEST1000
    anc_sample_rate[AUD_STREAM_PLAYBACK] = AUD_SAMPRATE_48000;
    anc_sample_rate[AUD_STREAM_CAPTURE] = AUD_SAMPRATE_48000;
#endif
    anc_sample_rate[AUD_STREAM_PLAYBACK] =
        hal_codec_anc_convert_rate(anc_sample_rate[AUD_STREAM_PLAYBACK]);
    anc_sample_rate[AUD_STREAM_CAPTURE] =
        hal_codec_anc_convert_rate(anc_sample_rate[AUD_STREAM_CAPTURE]);
  }

  playback_rate = anc_sample_rate[AUD_STREAM_PLAYBACK];
  capture_rate = anc_sample_rate[AUD_STREAM_CAPTURE];

  pmu_anc_config(1);

#ifdef ANC_FF_ENABLED
  af_anc_open(ANC_FEEDFORWARD, playback_rate, capture_rate, handler);
  anc_open(ANC_FEEDFORWARD);
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_FEEDFORWARD, ANC_GAIN_DELAY);
#endif

#ifdef ANC_FB_ENABLED
  af_anc_open(ANC_FEEDBACK, playback_rate, capture_rate, handler);
  anc_open(ANC_FEEDBACK);
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_FEEDBACK, ANC_GAIN_DELAY);
#endif

#ifdef AUDIO_ANC_FB_MC_HW
  anc_open(ANC_MUSICCANCLE);
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_MUSICCANCLE, ANC_GAIN_NO_DELAY);
#endif

#if defined(AUDIO_ANC_TT_HW)
  anc_open(ANC_TALKTHRU);
  anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                  ANC_TALKTHRU, ANC_GAIN_NO_DELAY);
#endif

#ifdef AUDIO_ANC_FB_MC_HW
  int32_t gain_ch_l = 0;
  int32_t gain_ch_r = 0;

  anc_get_cfg_gain(&gain_ch_l, &gain_ch_r, ANC_MUSICCANCLE);
  anc_set_gain(gain_ch_l, gain_ch_r, ANC_MUSICCANCLE);
#endif

#if defined(ANC_WNR_ENABLED)
  if (is_sco_mode()) {
    anc_wnr_open(ANC_WNR_OPEN_MODE_CONFIGURE);
  } else {
    anc_wnr_open(ANC_WNR_OPEN_MODE_STANDALONE);
  }
#endif

#if defined(ANC_ASSIST_ENABLED)
  anc_assist_open(ANC_ASSIST_STANDALONE);
#endif

  TRACE(2, "exit %s %d\n", __FUNCTION__, __LINE__);
}

void app_anc_close_anc(void) {
  TRACE(2, "enter %s %d\n", __FUNCTION__, __LINE__);
  anc_set_dac_pa_delay = false;
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
  if (anc_single_mode_is_on()) {
    analog_aud_codec_speaker_enable(false);
  }
#endif

#ifdef ANC_WNR_ENABLED
  anc_wnr_close();
#endif

#if defined(ANC_ASSIST_ENABLED)
  anc_assist_close();
#endif

#if defined(AUDIO_ANC_TT_HW)
  anc_close(ANC_TALKTHRU);
#endif

#ifdef ANC_FF_ENABLED
  anc_close(ANC_FEEDFORWARD);
  af_anc_close(ANC_FEEDFORWARD);
#endif

#ifdef ANC_FB_ENABLED
#if defined(AUDIO_ANC_FB_MC_HW)
  anc_close(ANC_MUSICCANCLE);
#endif
  anc_close(ANC_FEEDBACK);
  af_anc_close(ANC_FEEDBACK);
#endif

  pmu_anc_config(0);
#ifdef __SIMPLE_INTERNAL_PLAYER_SUPPORT__
  app_stop_player_by_anc();
#endif
  TRACE(2, "exit %s %d\n", __FUNCTION__, __LINE__);
}

void app_anc_bitrate_reopen(void) {
  TRACE(2, " %s status %d", __func__, anc_work_status);
  if (anc_work_status == ANC_STATUS_INIT_ON) {
    //        hal_codec_reconfig_pll_freq(playback_rate, capture_rate);
  } else if (anc_work_status == ANC_STATUS_ON) {
    //    hal_codec_reconfig_pll_freq(playback_rate, capture_rate);
  } else if (anc_work_status == ANC_STATUS_WAITING_ON) {
  } else {
    TRACE(0, "no deal situation.");
  }
}

void app_anc_status_change(void) {
#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (p_ibrt_ctrl->current_role == IBRT_SLAVE) {
    anc_coef_idx = anc_peer_coef_idx;
  }
#endif

  TRACE(3, "%s anc_work_status: %d anc_coef_idx: %d", __func__, anc_work_status,
        anc_coef_idx);

  switch (anc_work_status) {
  case ANC_STATUS_OFF:
    anc_work_status = ANC_STATUS_WAITING_ON;
    app_anc_timer_set(ANC_EVENT_OPEN, anc_switch_on_time);
    app_anc_open_anc();
    break;
  case ANC_STATUS_ON:

#if (ANC_COEF_LIST_NUM > 1)

#ifdef __BT_ANC_KEY__
    if (anc_status_sync_flag == false) {
      anc_coef_idx++;
    }
#endif

    if (anc_coef_idx < ANC_COEF_LIST_NUM) {
      TRACE(2, " %s set coef idx: %d ", __func__, anc_coef_idx);

#ifdef ANC_MODE_SWITCH_WITHOUT_FADE

#ifdef ANC_FF_ENABLED
      anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                      ANC_FEEDFORWARD, ANC_GAIN_NO_DELAY);
#endif
#ifdef ANC_FB_ENABLED
      anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                      ANC_FEEDBACK, ANC_GAIN_NO_DELAY);
#endif
#ifdef AUDIO_ANC_FB_MC_HW
      anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                      ANC_MUSICCANCLE, ANC_GAIN_NO_DELAY);
#endif

      // recommand to play "ANC SWITCH" prompt here...

#else
      osSignalSet(anc_fade_thread_tid, CHANGE_FROM_ANC_TO_TT_DIRECTLY);
#endif
    } else {
      anc_coef_idx = 0;
      app_anc_timer_set(ANC_EVENT_CLOSE, anc_close_delay_time);
      app_anc_gain_fadeout();
      anc_work_status = ANC_STATUS_WAITING_OFF;
    }
#else
    anc_coef_idx = 0;
    app_anc_timer_set(ANC_EVENT_CLOSE, anc_close_delay_time);
    app_anc_gain_fadeout();
    anc_work_status = ANC_STATUS_WAITING_OFF;
#endif
    break;
  case ANC_STATUS_INIT_ON:
    app_anc_select_coef();
    app_anc_gain_fadein();
    anc_work_status = ANC_STATUS_WAITING_ON;
    app_anc_timer_close();
    break;
  default:
    break;
  }
  if (anc_status_sync_flag == true)
    anc_status_sync_flag = false;
}

static int app_anc_handle_process(APP_MESSAGE_BODY *msg_body) {
  uint32_t evt = msg_body->message_id;
  uint32_t arg0 = msg_body->message_Param0;

  TRACE(4, " %s evt: %d, arg0: %d , anc status :%d", __func__, evt, arg0,
        anc_work_status);

  switch (evt) {
  case ANC_EVENT_INIT:
    // init anc_timer
    app_anc_init_timer();
    // init anc&open codec
#ifdef __AUDIO_SECTION_SUPPT__
    anc_load_cfg();
#endif

#ifdef __ANC_INIT_ON__
    app_anc_open_anc();
    anc_work_status = ANC_STATUS_INIT_ON;
    app_anc_timer_set(ANC_EVENT_CLOSE, anc_init_switch_off_time);
#else
    anc_work_status = ANC_STATUS_OFF;
#endif
#ifdef __ANC_STICK_SWITCH_USE_GPIO__
    if (app_anc_switch_get_val() == ANC_SWITCH_ON_LEVEL) {
      app_anc_switch_set_edge(ANC_SWITCH_ON_LEVEL);
      app_anc_status_post(ANC_SWITCH_ON_LEVEL);
    } else {
      app_anc_switch_set_edge(!(bool)ANC_SWITCH_ON_LEVEL);
    }
#endif
    // anc_coef_idx = 0;
    anc_fade_thread_tid = osThreadCreate(osThread(anc_fade_thread), NULL);
    break;
  case ANC_EVENT_FADE_IN:
  case ANC_EVENT_FADE_OUT:
    anc_gain_fade_handle();
    if (evt == ANC_EVENT_FADE_IN) {
      anc_work_status = ANC_STATUS_ON;
      // recommand to play "ANC ON" prompt here...
      app_voice_report(APP_STATUS_INDICATION_ALEXA_START,
                       0); // close latlatency mode
    }
    if (evt == ANC_EVENT_FADE_OUT) {
      anc_work_status = ANC_STATUS_INIT_ON;
      // recommand to play "ANC OFF" prompt here...
      app_voice_report(APP_STATUS_INDICATION_ALEXA_STOP,
                       0); // close latlatency mode
    }
    break;
  case ANC_EVENT_CHANGE_SAMPLERATE:
    app_anc_bitrate_reopen();
    break;
  case ANC_EVENT_CHANGE_STATUS:

#ifdef __ANC_STICK_SWITCH_USE_GPIO__
    osDelay(10);
    if (app_anc_switch_get_val() != (bool)arg0) { // debonce
      TRACE(0, "io level not equeue exit");
      break;
    }

#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
    if (anc_single_mode_is_on()) {
      if (arg0 != ANC_SWITCH_ON_LEVEL) {
        TRACE(1, " Anc ON but exit %d ", app_init_done);
        if (app_init_done) {
          app_anc_shutdown = true;
          app_shutdown();
        } else {
          app_anc_timer_set(ANC_EVENT_SWITCH_KEY_DEBONCE,
                            anc_switch_key_debonce_time);
        }
        return 0;
      }
    }
#endif
    if (arg0 != ANC_SWITCH_ON_LEVEL)
      app_anc_switch_set_edge((!(bool)ANC_SWITCH_ON_LEVEL));
    else
      app_anc_switch_set_edge(ANC_SWITCH_ON_LEVEL);

    if (((arg0 == ANC_SWITCH_ON_LEVEL) && (anc_work_status == ANC_STATUS_ON)) ||
        ((arg0 != ANC_SWITCH_ON_LEVEL) &&
         (anc_work_status == ANC_STATUS_OFF))) {
      // status same, no handle
      TRACE(0, " Anc NOT ON, exit");
      return 0;
    }
#endif

#if defined(IBRT)
    if (((BOOL)arg0 == false) && (app_anc_work_status() == false)) {
      anc_coef_idx = 0;
      anc_peer_coef_idx = 0;
      anc_status_sync_flag = 0;
      return 0;
    }

    if (((BOOL)arg0 == true) && (app_anc_work_status() == true) &&
        (anc_coef_idx == anc_peer_coef_idx)) {
      anc_status_sync_flag = 0;
      return 0;
    }
#endif
    app_anc_status_change();

    break;
  case ANC_EVENT_SYNC_STATUS:
#if defined(IBRT)
    TRACE(4, "%s anc_work_status %d--->%d anc_coef_idx:%d ", __func__,
          anc_work_status, arg0, anc_coef_idx);

    if (ANC_STATUS_ON == arg0) {
      arg0 = 1;
    } else {
      arg0 = 0;
    }

    uint8_t buf[3] = {0};
    buf[0] = (uint8_t)arg0;
    buf[1] = (uint8_t)anc_coef_idx;
    buf[2] = false; // set peer anc_status_sync_flag = false
    tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_ANC_STATUS, buf, 3);
#endif
    break;
  case ANC_EVENT_SYNC_INIT:
    break;
  case ANC_EVENT_HOWL_PROCESS:
    // disable anc,set max gain,enable anc
    {
      // uint32_t howl = arg0;
      if (anc_work_status == ANC_STATUS_ON) {
        //__anc_usb_app_howl_set_gain(howl);
      }
    }
    break;
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
  case ANC_EVENT_PWR_KEY_MONITOR:
    osDelay(10);

    if (app_pwr_key_monitor_get_val() != (bool)arg0) // debonce
      return 0;

    if ((bool)arg0) {
      app_anc_timer_set(ANC_EVENT_PWR_KEY_MONITOR_REBOOT,
                        anc_pwr_key_monitor_time);
      app_pwr_key_monitor_set_int_edge(1);
    } else {
      hwtimer_stop(anc_timerid);
      app_pwr_key_monitor_set_int_edge(0);
    }

    break;
#endif
  case SIMPLE_PLAYER_DELAY_STOP_EVT:
#ifdef __SIMPLE_INTERNAL_PLAYER_SUPPORT__
    simple_player_delay_stop();
#endif
    break;
  case SIMPLE_PLAYER_CLOSE_CODEC_EVT:
    app_anc_close_anc();
    anc_work_status = ANC_STATUS_OFF;
    break;
  default:
    break;
  }

  return 0;
}

static void anc_gain_fade_timer_handler(void *p) {
  TRACE(2, " %s , request %d ", __func__, anc_timer_request);
  switch (anc_timer_request) {
  case ANC_EVENT_OPEN:
    app_anc_gain_fadein();
    break;
  case ANC_EVENT_CLOSE:
    // real close
    // app_anc_close_anc();
    // anc_work_status = ANC_STATUS_OFF;
    app_anc_post_anc_codec_close();

    break;
  case ANC_EVENT_PWR_KEY_MONITOR_REBOOT:
    TRACE(0, " reboot !");
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT_ANC_ON);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_BT_ON);
    app_reset();
    break;
  case ANC_EVENT_SWITCH_KEY_DEBONCE:
    TRACE(0, " ANC SWITCH KEY ");
#ifdef __ANC_STICK_SWITCH_USE_GPIO__
    if (app_switch_callback)
      app_switch_callback(
          hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)ANC_SWITCH_PIN));
#endif
    break;
  default:
    break;
  }
  anc_timer_request = ANC_EVENT_NONE;
  hwtimer_stop(anc_timerid);
}

void app_anc_init_timer(void) {
  if (anc_timerid == NULL)
    anc_timerid = hwtimer_alloc((HWTIMER_CALLBACK_T)anc_gain_fade_timer_handler,
                                &anc_timer_request);
}

void app_anc_timer_set(uint32_t request, uint32_t delay) {
  TRACE(3, " %s request %d , delay %ds ", __func__, request,
        (TICKS_TO_MS(delay) / 1000));
  if (anc_timerid == NULL)
    return;
  anc_timer_request = request;
  hwtimer_stop(anc_timerid);
  hwtimer_start(anc_timerid, delay);
}

void app_anc_timer_close(void) {
  TRACE(3, " %s enter...", __func__);
  if (anc_timerid == NULL) {
    return;
  }

  anc_timer_request = ANC_EVENT_NONE;
  hwtimer_stop(anc_timerid);
}

#if defined(IBRT)
void app_anc_cmd_receive_process(uint8_t *buf, uint16_t len) {
  TRACE(1, "[%s] enter...", __func__);

  if (buf[0] == IBRT_ACTION_ANC_NOTIRY_MASTER_EXCHANGE_COEF) {
    switch (buf[1]) {
#ifdef __BT_ANC_KEY__
    case 0:
      app_anc_key(NULL, NULL);
      break; // slave invoke app_anc_key();
#endif
#ifndef __BT_ANC_KEY__
    case 1:
      app_anc_start();
      break; // slave invoke app_anc_start();
    case 2:
      app_anc_switch_coef(buf[2]);
      break; // slave invoke app_anc_switch_coef();
    case 3:
      app_anc_stop();
      break; // slave invoke app_anc_stop();
#endif
    default:
      TRACE(1, "[%s] cmd invalid...", __func__);
      break;
    }
  }
}

static void app_anc_notify_master_to_exchange_coef(uint8_t arg0, uint8_t arg1) {
  uint8_t buf[3] = {0};

  TRACE(3, "[%s] arg0:%d arg1:%d", __func__, arg0, arg1);

  buf[0] = IBRT_ACTION_ANC_NOTIRY_MASTER_EXCHANGE_COEF;
  buf[1] = arg0;
  buf[2] = arg1;

  app_ibrt_ui_send_user_action(buf, 3);
}
#endif

#ifdef __BT_ANC_KEY__
void app_anc_key(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s anc_work_st:%d , timer:%d", __func__, anc_work_status,
        anc_timer_request);
  bool flag = app_anc_work_status();

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  TRACE(2, "[%s] current_role: %d", __func__, p_ibrt_ctrl->current_role);
  if (p_ibrt_ctrl->current_role == IBRT_SLAVE) {
    app_anc_notify_master_to_exchange_coef(0, 0);
    return;
  }
  app_anc_status_sync(!flag);
#endif
  app_anc_status_post(!flag);
}
#endif

#ifndef __BT_ANC_KEY__
int app_anc_start(void) {
  TRACE(1, "[%s] enter...", __func__);
  bool flag = app_anc_work_status();

  if (flag == true) {
    TRACE(1, "[%s] anc has been on...", __func__);
    return -1;
  }

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  TRACE(2, "[%s] current_role: %d", __func__, p_ibrt_ctrl->current_role);
  if (p_ibrt_ctrl->current_role == IBRT_SLAVE) {
    app_anc_notify_master_to_exchange_coef(1, 0);
    return -1;
  }
  anc_coef_idx = 0;
  app_anc_status_sync(!flag);
#endif
  anc_coef_idx = 0;
  app_anc_status_post(!flag);

  return 0;
}

int app_anc_switch_coef(uint8_t index) {
  TRACE(1, "[%s] enter...", __func__);
  bool flag = app_anc_work_status();

  if (flag == false) {
    TRACE(1, "[%s] anc has been off...", __func__);
    return -1;
  }

  if (index == anc_coef_idx) {
    TRACE(1, "[%s] anc coef has been load...", __func__);
    return -1;
  }

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  TRACE(2, "[%s] current_role: %d", __func__, p_ibrt_ctrl->current_role);
  if (p_ibrt_ctrl->current_role == IBRT_SLAVE) {
    app_anc_notify_master_to_exchange_coef(2, index);
    return -1;
  }
  anc_coef_idx = index;
  app_anc_status_sync(!flag);
#endif
  anc_coef_idx = index;
  app_anc_status_post(!flag);

  return 0;
}

int app_anc_stop(void) {
  TRACE(1, "[%s] enter...", __func__);
  bool flag = app_anc_work_status();

  if (flag == false) {
    TRACE(1, "[%s] anc has been off...", __func__);
    return -1;
  }

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  TRACE(2, "[%s] current_role: %d", __func__, p_ibrt_ctrl->current_role);
  if (p_ibrt_ctrl->current_role == IBRT_SLAVE) {
    app_anc_notify_master_to_exchange_coef(3, 0);
    return -1;
  }
  anc_coef_idx = ANC_COEF_NUM;
  app_anc_status_sync(!flag);
#endif
  anc_coef_idx = ANC_COEF_NUM;
  app_anc_status_post(!flag);

  return 0;
}
#endif

void app_anc_ios_init(void) {
#ifdef __ANC_STICK_SWITCH_USE_GPIO__
  if (cfg_anc_switch.pin != HAL_IOMUX_PIN_NUM) {
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_anc_switch, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_anc_switch.pin,
                         HAL_GPIO_DIR_IN, 0);
  }
#endif
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
  if (cfg_pwr_key_monitor.pin != HAL_IOMUX_PIN_NUM) {
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_pwr_key_monitor,
                   1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_pwr_key_monitor.pin,
                         HAL_GPIO_DIR_IN, 0);
  }
  if (cfg_anc_led.pin != HAL_IOMUX_PIN_NUM) {
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_anc_led, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_anc_led.pin, HAL_GPIO_DIR_OUT,
                         0);
  }

#endif
#ifdef ANC_SWITCH_PIN
  uint8_t gpio_val = hal_gpio_pin_get_val(ANC_SWITCH_PIN);
  if (gpio_val) {
    app_anc_switch_turnled(true);
    TRACE(1, " %s turn on led", __func__);
  } else {
    app_anc_switch_turnled(false);
    TRACE(1, " %s turn off led", __func__);
  }
#endif
}

int app_anc_open_module(void) {
  TRACE(1, "%s ", __func__);
  // set app module
  app_set_threadhandle(APP_MODUAL_ANC, app_anc_handle_process);
  app_anc_do_init();
#ifdef __ANC_STICK_SWITCH_USE_GPIO__
  app_anc_switch_init(app_anc_status_post);
  app_anc_switch_set_edge(app_anc_switch_get_val());
#endif
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
  if (anc_single_mode_is_on()) {
    // app_pwr_key_monitor_init(app_anc_send_pwr_key_monitor_evt);
    //  app_pwr_key_monitor_set_int_edge(app_pwr_key_monitor_get_val());
  }
#endif

  return 0;
}

int app_anc_close_module(void) {
  TRACE(1, " %s ", __func__);

  anc_timer_request = ANC_EVENT_NONE;
  if (anc_timerid) {
    hwtimer_stop(anc_timerid);
    hwtimer_free(anc_timerid);
    anc_timerid = NULL;
  }
  if (app_anc_get_anc_status() != ANC_STATUS_OFF) {
    anc_work_status = ANC_STATUS_OFF;
    app_anc_disable();
    app_anc_close_anc();
  }
  app_set_threadhandle(APP_MODUAL_ANC, NULL);
  return 0;
}

enum AUD_SAMPRATE_T app_anc_get_play_rate(void) {
  return anc_sample_rate[AUD_STREAM_PLAYBACK];
}

bool app_anc_work_status(void) {
  // TRACE(2," %s st %d", __func__, anc_work_status);
  return (anc_work_status == ANC_STATUS_ON ||
          anc_work_status == ANC_STATUS_WAITING_ON);
}

uint32_t app_anc_get_anc_status(void) { return anc_work_status; }

void test_anc(void) {
  app_anc_open_anc();
  app_anc_enable();
}

void test_anc_switch_coef(void) {
  anc_coef_idx++;

  TRACE(2, " %s set coef idx: %d ", __func__, anc_coef_idx);

  if (ANC_STATUS_OFF == anc_work_status ||
      ANC_STATUS_INIT_ON == anc_work_status) {
    app_anc_status_change();
    anc_coef_idx = 0;
    return;
  }
  if (anc_coef_idx < (ANC_COEF_LIST_NUM)) {
#ifdef ANC_FF_ENABLED
    while (__anc_usb_app_fadeout(ANC_FEEDFORWARD))
      ;
#endif
#ifdef ANC_FB_ENABLED
    while (__anc_usb_app_fadeout(ANC_FEEDBACK))
      ;
#endif

    app_anc_disable();
    anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK], anc_coef_idx,
                    ANC_FEEDFORWARD, ANC_GAIN_DELAY);
    app_anc_enable();

#ifdef ANC_FF_ENABLED
    while (__anc_usb_app_fadein(ANC_FEEDFORWARD))
      ;
#endif
#ifdef ANC_FB_ENABLED
    while (__anc_usb_app_fadein(ANC_FEEDBACK))
      ;
#endif
  } else {
    anc_coef_idx = 0;
    app_anc_timer_set(ANC_EVENT_CLOSE, anc_close_delay_time);
    app_anc_gain_fadeout();
    anc_work_status = ANC_STATUS_WAITING_OFF;
  }
}

#if defined(IBRT)
void app_anc_sync_status(void) {
  uint8_t buf[3] = {0};

  bool flag = app_anc_work_status();

  buf[0] = (uint8_t)flag;

  if (flag == true) {
    buf[1] = (uint8_t)anc_coef_idx;
  } else {
    buf[1] = (uint8_t)ANC_COEF_NUM;
  }

  buf[2] = true; // set peer anc_status_sync_flag = true

  anc_peer_coef_idx = 0;

  TRACE(4, "%s anc_work_status:%d, anc_coef_idx:%d, anc_sync_status_flag:%d",
        __func__, buf[0], buf[1], buf[2]);
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (p_ibrt_ctrl->current_role == IBRT_MASTER) {
    tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_ANC_STATUS, buf, 3);
  }
}
#endif
