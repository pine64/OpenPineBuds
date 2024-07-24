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
#include "hal_key.h"
#include "cmsis_nvic.h"
#include "hal_chipid.h"
#include "hal_iomux.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "pmu.h"
#include "string.h"
#include "tgt_hardware.h"

#ifdef KEY_DEBUG
#define HAL_KEY_TRACE(n, s, ...)                                               \
  TRACE(n, "[%u]" s, TICKS_TO_MS(hal_sys_timer_get()), ##__VA_ARGS__)
#else
#define HAL_KEY_TRACE(n, s, ...) TRACE_DUMMY(n, s, ##__VA_ARGS__)
#endif

#ifdef CHIP_BEST2000
#define GPIO_MAP_64BIT
#endif

#ifdef GPIO_MAP_64BIT
typedef uint64_t GPIO_MAP_T;
#else
typedef uint32_t GPIO_MAP_T;
#endif

// #ifndef APP_TEST_MODE
// #define CHECK_PWRKEY_AT_BOOT
// #endif
#ifdef NO_PWRKEY
#undef CHECK_PWRKEY_AT_BOOT
#endif
#ifdef NO_GPIOKEY
#undef CFG_HW_GPIOKEY_NUM
#define CFG_HW_GPIOKEY_NUM 0
#endif
#ifdef NO_ADCKEY
#undef CFG_HW_ADCKEY_NUMBER
#define CFG_HW_ADCKEY_NUMBER 0
#endif

#ifndef CFG_SW_KEY_LLPRESS_THRESH_MS
#define CFG_SW_KEY_LLPRESS_THRESH_MS 5000
#endif
#ifndef CFG_SW_KEY_LPRESS_THRESH_MS
#define CFG_SW_KEY_LPRESS_THRESH_MS 1500
#endif
#ifndef CFG_SW_KEY_REPEAT_THRESH_MS
#define CFG_SW_KEY_REPEAT_THRESH_MS 500
#endif
#ifndef CFG_SW_KEY_DBLCLICK_THRESH_MS
#define CFG_SW_KEY_DBLCLICK_THRESH_MS 400
#endif
#ifndef CFG_SW_KEY_INIT_DOWN_THRESH_MS
#define CFG_SW_KEY_INIT_DOWN_THRESH_MS 200
#endif
#ifndef CFG_SW_KEY_INIT_LPRESS_THRESH_MS
#define CFG_SW_KEY_INIT_LPRESS_THRESH_MS 3000
#endif
#ifndef CFG_SW_KEY_INIT_LLPRESS_THRESH_MS
#define CFG_SW_KEY_INIT_LLPRESS_THRESH_MS 10000
#endif
#ifndef CFG_SW_KEY_CHECK_INTERVAL_MS
#define CFG_SW_KEY_CHECK_INTERVAL_MS 40
#endif

// common key define
#define KEY_LONGLONGPRESS_THRESHOLD MS_TO_TICKS(CFG_SW_KEY_LLPRESS_THRESH_MS)
#define KEY_LONGPRESS_THRESHOLD MS_TO_TICKS(CFG_SW_KEY_LPRESS_THRESH_MS)
#define KEY_DOUBLECLICK_THRESHOLD MS_TO_TICKS(CFG_SW_KEY_DBLCLICK_THRESH_MS)
#define KEY_LONGPRESS_REPEAT_THRESHOLD MS_TO_TICKS(CFG_SW_KEY_REPEAT_THRESH_MS)

#define KEY_INIT_DOWN_THRESHOLD MS_TO_TICKS(CFG_SW_KEY_INIT_DOWN_THRESH_MS)
#define KEY_INIT_LONGPRESS_THRESHOLD                                           \
  MS_TO_TICKS(CFG_SW_KEY_INIT_LPRESS_THRESH_MS)
#define KEY_INIT_LONGLONGPRESS_THRESHOLD                                       \
  MS_TO_TICKS(CFG_SW_KEY_INIT_LLPRESS_THRESH_MS)

#define KEY_CHECKER_INTERVAL MS_TO_TICKS(CFG_SW_KEY_CHECK_INTERVAL_MS)

#define KEY_DEBOUNCE_INTERVAL (KEY_CHECKER_INTERVAL * 2)
#define KEY_DITHER_INTERVAL (KEY_CHECKER_INTERVAL * 1)

#define MAX_KEY_CLICK_COUNT (HAL_KEY_EVENT_RAMPAGECLICK - HAL_KEY_EVENT_CLICK)

struct HAL_KEY_ADCKEY_T {
  bool debounce;
  bool dither;
  enum HAL_KEY_CODE_T code_debounce;
  enum HAL_KEY_CODE_T code_down;
  uint32_t time;
};

struct HAL_KEY_GPIOKEY_T {
  GPIO_MAP_T pin_debounce;
  GPIO_MAP_T pin_dither;
  GPIO_MAP_T pin_down;
  uint32_t time_debounce;
  uint32_t time_dither;
};

struct HAL_KEY_PWRKEY_T {
  bool debounce;
  bool dither;
  bool pressed;
  uint32_t time;
};

struct HAL_KEY_STATUS_T {
  enum HAL_KEY_CODE_T code_down;
  enum HAL_KEY_CODE_T code_ready;
  enum HAL_KEY_CODE_T code_click;
  enum HAL_KEY_EVENT_T event;
  uint32_t time_updown;
  uint32_t time_click;
  uint8_t cnt_repeat;
  uint8_t cnt_click;
};

static int (*key_detected_callback)(uint32_t, uint8_t) = NULL;
static HWTIMER_ID debounce_timer = NULL;
static bool timer_active = false;
static struct HAL_KEY_STATUS_T key_status;

static void hal_key_disable_allint(void);
static void hal_key_enable_allint(void);

static int send_key_event(enum HAL_KEY_CODE_T code,
                          enum HAL_KEY_EVENT_T event) {
  if (key_detected_callback) {
    return key_detected_callback(code, event);
  }
  return 0;
}

static void hal_key_debounce_timer_restart(void) {
  uint32_t lock;
  bool set = false;

  lock = int_lock();
  if (!timer_active) {
    timer_active = true;
    set = true;
  }
  int_unlock(lock);

  if (set) {
    hwtimer_stop(debounce_timer);
    hwtimer_start(debounce_timer, KEY_CHECKER_INTERVAL);
    // hal_sys_wake_lock(HAL_SYS_WAKE_LOCK_USER_KEY);
  }
}

#if (CFG_HW_ADCKEY_NUMBER > 0)
static uint16_t adckey_volt_table[CFG_HW_ADCKEY_NUMBER];
struct HAL_KEY_ADCKEY_T adc_key;

static inline POSSIBLY_UNUSED void hal_adckey_enable_press_int(void) {
  hal_adckey_set_irq(HAL_ADCKEY_IRQ_PRESSED);
}

static inline POSSIBLY_UNUSED void hal_adckey_enable_release_int(void) {
  hal_adckey_set_irq(HAL_ADCKEY_IRQ_RELEASED);
}

static inline POSSIBLY_UNUSED void hal_adckey_enable_adc_int(void) {
  hal_gpadc_open(HAL_GPADC_CHAN_ADCKEY, HAL_GPADC_ATP_NULL, NULL);
}

static inline POSSIBLY_UNUSED void hal_adckey_disable_adc_int(void) {
  hal_gpadc_close(HAL_GPADC_CHAN_ADCKEY);
}

static inline POSSIBLY_UNUSED void hal_adckey_disable_allint(void) {
  hal_gpadc_close(HAL_GPADC_CHAN_ADCKEY);
  hal_adckey_set_irq(HAL_ADCKEY_IRQ_NONE);
}

static inline POSSIBLY_UNUSED void hal_adckey_reset(void) {
  memset(&adc_key, 0, sizeof(adc_key));
}

static enum HAL_KEY_CODE_T hal_adckey_findkey(uint16_t volt) {
  int index = 0;

#if 0
    if (volt == HAL_GPADC_BAD_VALUE) {
        return HAL_KEY_CODE_NONE;
    }
#endif

  if (CFG_HW_ADCKEY_ADC_KEYVOLT_BASE < volt &&
      volt < CFG_HW_ADCKEY_ADC_MAXVOLT) {
    for (index = 0; index < CFG_HW_ADCKEY_NUMBER; index++) {
      if (volt <= adckey_volt_table[index]) {
        return CFG_HW_ADCKEY_MAP_TABLE[index];
      }
    }
  }

  return HAL_KEY_CODE_NONE;
}

static void hal_adckey_irqhandler(enum HAL_ADCKEY_IRQ_STATUS_T irq_status,
                                  HAL_GPADC_MV_T val) {
  enum HAL_KEY_CODE_T code;

#ifdef NO_GROUPKEY
  hal_key_disable_allint();
#else
  hal_adckey_disable_allint();
#endif

  if (irq_status & (HAL_ADCKEY_ERR0 | HAL_ADCKEY_ERR1)) {
    HAL_KEY_TRACE(1, "irq,adckey err 0x%04X", irq_status);
    adc_key.debounce = true;
    adc_key.code_debounce = HAL_KEY_CODE_NONE;
    goto _debounce;
  }
  if (irq_status & HAL_ADCKEY_PRESSED) {
    HAL_KEY_TRACE(0, "irq,adckey press");
    adc_key.debounce = true;
    adc_key.code_debounce = HAL_KEY_CODE_NONE;
    adc_key.time = hal_sys_timer_get();
    hal_adckey_enable_adc_int();
    goto _exit;
  }
  if (irq_status & HAL_ADCKEY_RELEASED) {
    HAL_KEY_TRACE(0, "irq,adckey release");
    adc_key.code_debounce = HAL_KEY_CODE_NONE;
    goto _debounce;
  }
  if (irq_status & HAL_ADCKEY_ADC_VALID) {
    code = hal_adckey_findkey(val);
    HAL_KEY_TRACE(3, "irq,adckey cur:0x%X pre:0x%X volt:%d", code,
                  adc_key.code_debounce, val);

    if (adc_key.code_debounce == HAL_KEY_CODE_NONE) {
      adc_key.code_debounce = code;
    } else if (adc_key.code_debounce != code) {
      adc_key.code_debounce = HAL_KEY_CODE_NONE;
    }
  }

_debounce:
  hal_key_debounce_timer_restart();
_exit:
  return;
}

static void hal_adckey_open(void) {
  uint16_t i;
  uint32_t basevolt;

  HAL_KEY_TRACE(1, "%s\n", __func__);

  hal_adckey_reset();

  basevolt = (CFG_HW_ADCKEY_ADC_MAXVOLT - CFG_HW_ADCKEY_ADC_MINVOLT) /
             (CFG_HW_ADCKEY_NUMBER + 2);

  adckey_volt_table[0] = CFG_HW_ADCKEY_ADC_KEYVOLT_BASE + basevolt;

  for (i = 1; i < CFG_HW_ADCKEY_NUMBER - 1; i++) {
    adckey_volt_table[i] = adckey_volt_table[i - 1] + basevolt;
  }
  adckey_volt_table[CFG_HW_ADCKEY_NUMBER - 1] = CFG_HW_ADCKEY_ADC_MAXVOLT;

  hal_adckey_set_irq_handler(hal_adckey_irqhandler);
}

static void hal_adckey_close(void) {
  HAL_KEY_TRACE(1, "%s\n", __func__);

  hal_adckey_reset();

  hal_adckey_disable_allint();
}
#endif // (CFG_HW_ADCKEY_NUMBER > 0)

#ifndef NO_PWRKEY
struct HAL_KEY_PWRKEY_T pwr_key;

static inline POSSIBLY_UNUSED void hal_pwrkey_enable_riseedge_int(void) {
  hal_pwrkey_set_irq(HAL_PWRKEY_IRQ_RISING_EDGE);
}

static inline POSSIBLY_UNUSED void hal_pwrkey_enable_falledge_int(void) {
  hal_pwrkey_set_irq(HAL_PWRKEY_IRQ_FALLING_EDGE);
}

static inline POSSIBLY_UNUSED void hal_pwrkey_enable_bothedge_int(void) {
  hal_pwrkey_set_irq(HAL_PWRKEY_IRQ_BOTH_EDGE);
}

static inline void hal_pwrkey_enable_int(void) {
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
  hal_pwrkey_enable_riseedge_int();
#else
  hal_pwrkey_enable_falledge_int();
#endif
}

static inline void hal_pwrkey_disable_int(void) {
  hal_pwrkey_set_irq(HAL_PWRKEY_IRQ_NONE);
}

static inline void hal_pwrkey_reset(void) {
  memset(&pwr_key, 0, sizeof(pwr_key));
}

static inline bool hal_pwrkey_get_status(void) {
#ifdef CHIP_BEST1000
  if (hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_2) {
    return pwr_key.pressed;
  } else
#endif
  {
    return hal_pwrkey_pressed();
  }
}

static void hal_pwrkey_handle_irq_state(enum HAL_PWRKEY_IRQ_T state) {
  //    uint32_t time = hal_sys_timer_get();

#ifdef NO_GROUPKEY
  hal_key_disable_allint();
#else
  hal_pwrkey_disable_int();
#endif

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__

  if (state & HAL_PWRKEY_IRQ_RISING_EDGE) {
    HAL_KEY_TRACE(0, "pwr_key irq up");
    pwr_key.debounce = true;
  }

#else // !__POWERKEY_CTRL_ONOFF_ONLY__

  if (state & HAL_PWRKEY_IRQ_FALLING_EDGE) {
    HAL_KEY_TRACE(0, "pwr_key irq down");
#ifdef CHIP_BEST1000
    if (hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_2) {
      pwr_key.debounce = true;
      pwr_key.pressed = true;
      hal_pwrkey_enable_riseedge_int();
    } else
#endif
    {
      pwr_key.pressed = hal_pwrkey_pressed();
      if (pwr_key.pressed) {
        pwr_key.debounce = true;
      } else {
        pwr_key.dither = true;
      }
    }
    // pwr_key.time = time;
  }

#ifdef CHIP_BEST1000
  if (state & HAL_PWRKEY_IRQ_RISING_EDGE) {
    if (hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_2) {
      HAL_KEY_TRACE(0, "pwr_key irq up");
      pwr_key.pressed = false;
      hal_pwrkey_enable_falledge_int();
    }
  }
#endif

#endif // !__POWERKEY_CTRL_ONOFF_ONLY__

  hal_key_debounce_timer_restart();
}

#ifdef CHIP_HAS_EXT_PMU
#define PWRKEY_IRQ_HDLR_PARAM uint16_t irq_status
#else
#define PWRKEY_IRQ_HDLR_PARAM void
#endif
static void hal_pwrkey_irqhandler(PWRKEY_IRQ_HDLR_PARAM) {
  enum HAL_PWRKEY_IRQ_T state;

#ifdef CHIP_HAS_EXT_PMU
  state = pmu_pwrkey_irq_value_to_state(irq_status);
#else
  state = hal_pwrkey_get_irq_state();
#endif

  HAL_KEY_TRACE(2, "%s: %08x", __func__, state);

  hal_pwrkey_handle_irq_state(state);
}

static void hal_pwrkey_open(void) {
  hal_pwrkey_reset();

#ifdef CHIP_HAS_EXT_PMU
  pmu_set_irq_unified_handler(PMU_IRQ_TYPE_PWRKEY, hal_pwrkey_irqhandler);
#else
  NVIC_SetVector(PWRKEY_IRQn, (uint32_t)hal_pwrkey_irqhandler);
  NVIC_SetPriority(PWRKEY_IRQn, IRQ_PRIORITY_NORMAL);
  NVIC_ClearPendingIRQ(PWRKEY_IRQn);
  NVIC_EnableIRQ(PWRKEY_IRQn);
#endif
}

static void hal_pwrkey_close(void) {
  hal_pwrkey_reset();

  hal_pwrkey_disable_int();

#ifdef CHIP_HAS_EXT_PMU
  pmu_set_irq_unified_handler(PMU_IRQ_TYPE_PWRKEY, NULL);
#else
  NVIC_SetVector(PWRKEY_IRQn, (uint32_t)NULL);
  NVIC_DisableIRQ(PWRKEY_IRQn);
#endif
}
#endif // !NO_PWRKEY

#if (CFG_HW_GPIOKEY_NUM > 0)
struct HAL_KEY_GPIOKEY_T gpio_key;

static void hal_gpiokey_disable_irq(enum HAL_GPIO_PIN_T pin);

static inline void hal_gpiokey_reset(void) {
  memset(&gpio_key, 0, sizeof(gpio_key));
}

static int hal_gpiokey_find_index(enum HAL_GPIO_PIN_T pin) {
  int i;

  for (i = 0; i < CFG_HW_GPIOKEY_NUM; i++) {
    if (cfg_hw_gpio_key_cfg[i].key_config.pin == (enum HAL_IOMUX_PIN_T)pin) {
      return i;
    }
  }

  ASSERT(i < CFG_HW_GPIOKEY_NUM, "GPIOKEY IRQ: Invalid pin=%d", pin);
  return i;
}

static bool hal_gpiokey_pressed(enum HAL_GPIO_PIN_T pin) {
  int i = hal_gpiokey_find_index(pin);
  return (hal_gpio_pin_get_val(pin) == cfg_hw_gpio_key_cfg[i].key_down);
}

static void hal_gpiokey_irqhandler(enum HAL_GPIO_PIN_T pin) {
  bool pressed;
  uint32_t lock;
  uint32_t time;

#ifdef NO_GROUPKEY
  hal_key_disable_allint();
#else
  hal_gpiokey_disable_irq(pin);
#endif

  pressed = hal_gpiokey_pressed(pin);
  HAL_KEY_TRACE(2, "gpio_key trig=%d pressed=%d", pin, pressed);

  time = hal_sys_timer_get();

  lock = int_lock();
  if (pressed) {
    gpio_key.pin_debounce |= ((GPIO_MAP_T)1 << pin);
    gpio_key.time_debounce = time;
  } else {
    gpio_key.pin_dither |= ((GPIO_MAP_T)1 << pin);
    gpio_key.time_dither = time;
  }
  int_unlock(lock);

  hal_key_debounce_timer_restart();
}

static void hal_gpiokey_enable_irq(enum HAL_GPIO_PIN_T pin,
                                   enum HAL_GPIO_IRQ_POLARITY_T polarity) {
  struct HAL_GPIO_IRQ_CFG_T gpiocfg;

  hal_gpio_pin_set_dir(pin, HAL_GPIO_DIR_IN, 0);

  gpiocfg.irq_enable = true;
  gpiocfg.irq_debounce = true;
  gpiocfg.irq_polarity = polarity;
  gpiocfg.irq_handler = hal_gpiokey_irqhandler;
  gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;

  hal_gpio_setup_irq(pin, &gpiocfg);
}

static void hal_gpiokey_disable_irq(enum HAL_GPIO_PIN_T pin) {
  static const struct HAL_GPIO_IRQ_CFG_T gpiocfg = {
      .irq_enable = false,
      .irq_debounce = false,
      .irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING,
      .irq_handler = NULL,
      .irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE,
  };

  hal_gpio_setup_irq(pin, &gpiocfg);
}

static inline void hal_gpiokey_enable_allint(void) {
  uint8_t i;
  enum HAL_GPIO_IRQ_POLARITY_T polarity;

  for (i = 0; i < CFG_HW_GPIOKEY_NUM; i++) {
    if (cfg_hw_gpio_key_cfg[i].key_code == HAL_KEY_CODE_NONE) {
      continue;
    }

    if (cfg_hw_gpio_key_cfg[i].key_down == HAL_KEY_GPIOKEY_VAL_LOW) {
      polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    } else {
      polarity = HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
    }
    hal_gpiokey_enable_irq(
        (enum HAL_GPIO_PIN_T)cfg_hw_gpio_key_cfg[i].key_config.pin, polarity);
  }
}

static inline void hal_gpiokey_disable_allint(void) {
  uint8_t i;

  for (i = 0; i < CFG_HW_GPIOKEY_NUM; i++) {
    if (cfg_hw_gpio_key_cfg[i].key_code == HAL_KEY_CODE_NONE) {
      continue;
    }

    hal_gpiokey_disable_irq(
        (enum HAL_GPIO_PIN_T)cfg_hw_gpio_key_cfg[i].key_config.pin);
  }
}

static void hal_gpiokey_open(void) {
  uint8_t i;
  HAL_KEY_TRACE(1, "%s\n", __func__);

  hal_gpiokey_reset();

  for (i = 0; i < CFG_HW_GPIOKEY_NUM; i++) {
    if (cfg_hw_gpio_key_cfg[i].key_code == HAL_KEY_CODE_NONE) {
      continue;
    }

    hal_iomux_init(&cfg_hw_gpio_key_cfg[i].key_config, 1);
  }
}

static void hal_gpiokey_close(void) {
  HAL_KEY_TRACE(1, "%s\n", __func__);

  hal_gpiokey_reset();

  hal_gpiokey_disable_allint();
}
#endif // (CFG_HW_GPIOKEY_NUM > 0)

enum HAL_KEY_EVENT_T hal_key_read_status(enum HAL_KEY_CODE_T code) {
  uint8_t gpio_val;
  int i;

  if (code == HAL_KEY_CODE_PWR) {
    if (hal_pwrkey_pressed())
      return HAL_KEY_EVENT_DOWN;
    else
      return HAL_KEY_EVENT_UP;
  } else {
    for (i = 0; i < CFG_HW_GPIOKEY_NUM; i++) {
      if (cfg_hw_gpio_key_cfg[i].key_code == code) {
        gpio_val = hal_gpio_pin_get_val(
            (enum HAL_GPIO_PIN_T)cfg_hw_gpio_key_cfg[i].key_config.pin);
        if (gpio_val == cfg_hw_gpio_key_cfg[i].key_down) {
          return HAL_KEY_EVENT_DOWN;
        } else {
          return HAL_KEY_EVENT_UP;
        }
      }
    }
  }
  return HAL_KEY_EVENT_NONE;
}

static void hal_key_disable_allint(void) {
#ifndef NO_PWRKEY
  hal_pwrkey_disable_int();
#endif

#if (CFG_HW_ADCKEY_NUMBER > 0)
  hal_adckey_disable_allint();
#endif

#if (CFG_HW_GPIOKEY_NUM > 0)
  hal_gpiokey_disable_allint();
#endif
}

static void hal_key_enable_allint(void) {
#ifndef NO_PWRKEY
  hal_pwrkey_enable_int();
#endif

#if (CFG_HW_ADCKEY_NUMBER > 0)
  hal_adckey_enable_press_int();
#endif

#if (CFG_HW_GPIOKEY_NUM > 0)
  hal_gpiokey_enable_allint();
#endif
}

static void hal_key_debounce_handler(void *param) {
  uint32_t time;
  enum HAL_KEY_CODE_T code_down = HAL_KEY_CODE_NONE;
  int index;
  bool need_timer = false;

  timer_active = false;

  time = hal_sys_timer_get();

#ifndef NO_PWRKEY
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
  if (pwr_key.debounce) {
    pwr_key.debounce = false;
    code_down |= HAL_KEY_CODE_PWR;
#ifdef NO_GROUPKEY
    hal_key_enable_allint();
#else
    hal_pwrkey_enable_int();
#endif
  }
#else
  if (pwr_key.debounce || pwr_key.dither || pwr_key.pressed) {
    bool pressed = hal_pwrkey_get_status();

    // HAL_KEY_TRACE(4,"keyDbnPwr: dbn=%d dither=%d pressed=%d/%d",
    // pwr_key.debounce, pwr_key.dither, pwr_key.pressed, pressed);

    if (pwr_key.debounce) {
      pwr_key.pressed = pressed;
      if (pressed) {
        pwr_key.dither = false;
        if (time - pwr_key.time >= KEY_DEBOUNCE_INTERVAL) {
          pwr_key.debounce = false;
          pwr_key.dither = false;
          code_down |= HAL_KEY_CODE_PWR;
        }
      } else {
        pwr_key.debounce = false;
        pwr_key.dither = true;
        pwr_key.time = time;
      }
    } else if (pwr_key.dither) {
      if (time - pwr_key.time >= KEY_DITHER_INTERVAL) {
        pwr_key.dither = false;
        pwr_key.pressed = false;
#ifdef NO_GROUPKEY
        hal_key_enable_allint();
#else
        hal_pwrkey_enable_int();
#endif
      }
    } else if (pwr_key.pressed) {
      if (pressed) {
        code_down |= HAL_KEY_CODE_PWR;
      } else {
        pwr_key.pressed = false;
#ifdef NO_GROUPKEY
        hal_key_enable_allint();
#else
        hal_pwrkey_enable_int();
#endif
      }
    }
  }
  if (pwr_key.debounce || pwr_key.dither || pwr_key.pressed) {
    need_timer = true;
  }
#endif
#endif

#if (CFG_HW_ADCKEY_NUMBER > 0)
  if (adc_key.debounce || adc_key.dither ||
      adc_key.code_down != HAL_KEY_CODE_NONE) {
    bool skip_check = false;

    // HAL_KEY_TRACE(4,"keyDbnAdc: dbn=%d dither=%d code_dbn=0x%X
    // code_down=0x%X", adc_key.debounce, adc_key.dither, adc_key.code_debounce,
    // adc_key.code_down);

    if (adc_key.debounce) {
      if (adc_key.code_debounce == HAL_KEY_CODE_NONE) {
        adc_key.debounce = false;
        adc_key.dither = true;
        adc_key.time = time;
      } else {
        if (time - adc_key.time >= KEY_DEBOUNCE_INTERVAL) {
          adc_key.debounce = false;
          adc_key.dither = false;
          adc_key.code_down = adc_key.code_debounce;
          adc_key.code_debounce = HAL_KEY_CODE_NONE;
          code_down |= adc_key.code_down;
        }
      }
    } else if (adc_key.dither) {
      if (time - adc_key.time >= KEY_DITHER_INTERVAL) {
        adc_key.dither = false;
        adc_key.code_debounce = HAL_KEY_CODE_NONE;
        adc_key.code_down = HAL_KEY_CODE_NONE;
#ifdef NO_GROUPKEY
        hal_key_enable_allint();
#else
        hal_adckey_enable_press_int();
#endif
      }
      skip_check = true;
    } else if (adc_key.code_down != HAL_KEY_CODE_NONE) {
      if (adc_key.code_debounce == adc_key.code_down) {
        code_down |= adc_key.code_down;
      } else {
        adc_key.code_down = HAL_KEY_CODE_NONE;
#ifdef NO_GROUPKEY
        hal_key_enable_allint();
#else
        hal_adckey_enable_press_int();
#endif
        skip_check = true;
      }
    }

    if (!skip_check) {
      hal_adckey_enable_adc_int();
    }
  }
  if (adc_key.debounce || adc_key.dither ||
      adc_key.code_down != HAL_KEY_CODE_NONE) {
    need_timer = true;
  }
#endif

#if (CFG_HW_GPIOKEY_NUM > 0)
  enum HAL_GPIO_PIN_T gpio;
  GPIO_MAP_T pin;
  uint32_t lock;

#ifdef GPIO_MAP_64BIT
  ASSERT((gpio_key.pin_debounce & gpio_key.pin_dither) == 0 &&
             (gpio_key.pin_debounce & gpio_key.pin_dither) == 0 &&
             (gpio_key.pin_debounce & gpio_key.pin_dither) == 0,
         "Bad gpio_key pin map: dbn=0x%X-%X dither=0x%X-%X down=0x%X-%X",
         (uint32_t)(gpio_key.pin_debounce >> 32),
         (uint32_t)(gpio_key.pin_debounce),
         (uint32_t)(gpio_key.pin_dither >> 32), (uint32_t)(gpio_key.pin_dither),
         (uint32_t)(gpio_key.pin_down >> 32), (uint32_t)(gpio_key.pin_down));
#if 0
        HAL_KEY_TRACE(6,"keyDbnGpio: pin_dbn=0x%X-%X pin_dither=0x%X-%X pin_down=0x%X-%X",
            (uint32_t)(gpio_key.pin_debounce >> 32), (uint32_t)(gpio_key.pin_debounce),
            (uint32_t)(gpio_key.pin_dither >> 32), (uint32_t)(gpio_key.pin_dither),
            (uint32_t)(gpio_key.pin_down >> 32), (uint32_t)(gpio_key.pin_down));
#endif
#else // !GPIO_MAP_64BIT
  ASSERT((gpio_key.pin_debounce & gpio_key.pin_dither) == 0 &&
             (gpio_key.pin_debounce & gpio_key.pin_dither) == 0 &&
             (gpio_key.pin_debounce & gpio_key.pin_dither) == 0,
         "Bad gpio_key pin map: dbn=0x%X dither=0x%X down=0x%X",
         (uint32_t)gpio_key.pin_debounce, (uint32_t)gpio_key.pin_dither,
         (uint32_t)gpio_key.pin_down);
#if 0
        HAL_KEY_TRACE(3,"keyDbnGpio: pin_dbn=0x%X pin_dither=0x%X pin_down=0x%X",
            (uint32_t)gpio_key.pin_debounce, (uint32_t)gpio_key.pin_dither, (uint32_t)gpio_key.pin_down);
#endif
#endif // !GPIO_MAP_64BIT

  if (gpio_key.pin_dither) {
    if (time - gpio_key.time_dither >= KEY_DITHER_INTERVAL) {
      pin = gpio_key.pin_dither;

      lock = int_lock();
      gpio_key.pin_dither &= ~pin;
      int_unlock(lock);

#ifdef NO_GROUPKEY
      hal_key_enable_allint();
#else
      gpio = HAL_GPIO_PIN_P0_0;
      while (pin) {
        if (pin & ((GPIO_MAP_T)1 << gpio)) {
          pin &= ~((GPIO_MAP_T)1 << gpio);
          index = hal_gpiokey_find_index(gpio);
          hal_gpiokey_enable_irq(gpio, (cfg_hw_gpio_key_cfg[index].key_down ==
                                        HAL_KEY_GPIOKEY_VAL_LOW)
                                           ? HAL_GPIO_IRQ_POLARITY_LOW_FALLING
                                           : HAL_GPIO_IRQ_POLARITY_HIGH_RISING);
        }
        gpio++;
      }
#endif
    }
  }
  if (gpio_key.pin_down) {
    pin = gpio_key.pin_down;

    gpio = HAL_GPIO_PIN_P0_0;
    while (pin) {
      if (pin & ((GPIO_MAP_T)1 << gpio)) {
        pin &= ~((GPIO_MAP_T)1 << gpio);
        index = hal_gpiokey_find_index(gpio);
        if (hal_gpio_pin_get_val(gpio) == cfg_hw_gpio_key_cfg[index].key_down) {
          code_down |= cfg_hw_gpio_key_cfg[index].key_code;
        } else {
          gpio_key.pin_down &= ~((GPIO_MAP_T)1 << gpio);
#ifdef NO_GROUPKEY
          hal_key_enable_allint();
#else
          hal_gpiokey_enable_irq(gpio, (cfg_hw_gpio_key_cfg[index].key_down ==
                                        HAL_KEY_GPIOKEY_VAL_LOW)
                                           ? HAL_GPIO_IRQ_POLARITY_LOW_FALLING
                                           : HAL_GPIO_IRQ_POLARITY_HIGH_RISING);
#endif
        }
      }
      gpio++;
    }
  }
  if (gpio_key.pin_debounce) {
    GPIO_MAP_T down_added = 0;
    GPIO_MAP_T dither_added = 0;

    pin = gpio_key.pin_debounce;

    gpio = HAL_GPIO_PIN_P0_0;
    while (pin) {
      if (pin & ((GPIO_MAP_T)1 << gpio)) {
        pin &= ~((GPIO_MAP_T)1 << gpio);
        index = hal_gpiokey_find_index(gpio);
        if (hal_gpio_pin_get_val(gpio) == cfg_hw_gpio_key_cfg[index].key_down) {
          if (time - gpio_key.time_debounce >= KEY_DEBOUNCE_INTERVAL) {
            down_added |= ((GPIO_MAP_T)1 << gpio);
            code_down |= cfg_hw_gpio_key_cfg[index].key_code;
            gpio_key.pin_down |= ((GPIO_MAP_T)1 << gpio);
          }
        } else {
          dither_added |= ((GPIO_MAP_T)1 << gpio);
        }
      }
      gpio++;
    }

    lock = int_lock();
    gpio_key.pin_debounce &= ~(down_added | dither_added);
    gpio_key.pin_dither |= dither_added;
    int_unlock(lock);
  }
  if (gpio_key.pin_dither || gpio_key.pin_down || gpio_key.pin_debounce) {
    need_timer = true;
  }
#endif

  enum HAL_KEY_CODE_T down_new;
  enum HAL_KEY_CODE_T up_new;
  enum HAL_KEY_CODE_T map;

  down_new = code_down & ~key_status.code_down;
  up_new = ~code_down & key_status.code_down;

  // HAL_KEY_TRACE(5,"keyDbn: code_down=0x%X/0x%X down_new=0x%X up_new=0x%X
  // event=%d", key_status.code_down, code_down, down_new, up_new,
  // key_status.event);

  // Check newly up keys
  map = up_new;
  index = 0;
  while (map) {
    if (map & (1 << index)) {
      map &= ~(1 << index);
      send_key_event((1 << index), HAL_KEY_EVENT_UP);
      if (key_status.event == HAL_KEY_EVENT_LONGPRESS ||
          key_status.event == HAL_KEY_EVENT_LONGLONGPRESS) {
        send_key_event((1 << index), HAL_KEY_EVENT_UP_AFTER_LONGPRESS);
      }
      key_status.time_updown = time;
    }
    index++;
  }

  if (up_new) {
    if (key_status.event == HAL_KEY_EVENT_LONGPRESS ||
        key_status.event == HAL_KEY_EVENT_LONGLONGPRESS) {
      // LongPress is finished when all of the LongPress keys are released
      if ((code_down & key_status.code_ready) == 0) {
        key_status.event = HAL_KEY_EVENT_NONE;
      }
    } else if (key_status.event == HAL_KEY_EVENT_DOWN) {
      // Enter click handling if not in LongPress
      key_status.event = HAL_KEY_EVENT_UP;
    }
  }

  if (key_status.event == HAL_KEY_EVENT_UP) {
    // ASSERT(key_status.code_ready != HAL_KEY_CODE_NONE, "Bad code_ready");

    if (key_status.code_click == HAL_KEY_CODE_NONE ||
        key_status.code_click != key_status.code_ready) {
      if (key_status.code_click != HAL_KEY_CODE_NONE) {
        send_key_event(key_status.code_click,
                       HAL_KEY_EVENT_CLICK + key_status.cnt_click);
      }
      key_status.code_click = key_status.code_ready;
      key_status.cnt_click = 0;
      key_status.time_click = time;
    } else if (up_new &&
               (up_new | key_status.code_down) == key_status.code_click) {
      key_status.cnt_click++;
      key_status.time_click = time;
    }
    if (time - key_status.time_click >= KEY_DOUBLECLICK_THRESHOLD ||
        key_status.cnt_click >= MAX_KEY_CLICK_COUNT) {
      send_key_event(key_status.code_click,
                     HAL_KEY_EVENT_CLICK + key_status.cnt_click);
      key_status.code_click = HAL_KEY_CODE_NONE;
      key_status.cnt_click = 0;
      key_status.event = HAL_KEY_EVENT_NONE;
    }
  }

  // Update key_status.code_down
  key_status.code_down = code_down;

  // Check newly down keys and update key_status.code_ready
  map = down_new;
  index = 0;
  while (map) {
    if (map & (1 << index)) {
      map &= ~(1 << index);
      send_key_event((1 << index), HAL_KEY_EVENT_DOWN);
      if (key_status.event == HAL_KEY_EVENT_NONE) {
        send_key_event((1 << index), HAL_KEY_EVENT_FIRST_DOWN);
      } else {
        send_key_event((1 << index), HAL_KEY_EVENT_CONTINUED_DOWN);
      }
      if (key_status.event == HAL_KEY_EVENT_NONE ||
          key_status.event == HAL_KEY_EVENT_DOWN ||
          key_status.event == HAL_KEY_EVENT_UP) {
        key_status.code_ready = code_down;
      }
      key_status.time_updown = time;
    }
    index++;
  }

  if (down_new) {
    if (key_status.event == HAL_KEY_EVENT_NONE ||
        key_status.event == HAL_KEY_EVENT_UP) {
      key_status.event = HAL_KEY_EVENT_DOWN;
    }
  }

  // LongPress should be stopped if any key is released
  if ((code_down & key_status.code_ready) == key_status.code_ready) {
    if (key_status.event == HAL_KEY_EVENT_DOWN) {
      if (time - key_status.time_updown >= KEY_LONGPRESS_THRESHOLD) {
        key_status.cnt_repeat = 0;
        key_status.event = HAL_KEY_EVENT_LONGPRESS;
        send_key_event(key_status.code_ready, key_status.event);
      }
    } else if (key_status.event == HAL_KEY_EVENT_LONGPRESS ||
               key_status.event == HAL_KEY_EVENT_LONGLONGPRESS) {
      key_status.cnt_repeat++;
      if (key_status.cnt_repeat ==
          KEY_LONGPRESS_REPEAT_THRESHOLD / KEY_CHECKER_INTERVAL) {
        key_status.cnt_repeat = 0;
        send_key_event(key_status.code_ready, HAL_KEY_EVENT_REPEAT);
      }
      if (key_status.event == HAL_KEY_EVENT_LONGPRESS) {
        if (time - key_status.time_updown >= KEY_LONGLONGPRESS_THRESHOLD) {
          key_status.event = HAL_KEY_EVENT_LONGLONGPRESS;
          send_key_event(key_status.code_ready, key_status.event);
        }
      }
    }
  }

  if (key_status.event != HAL_KEY_EVENT_NONE) {
    need_timer = true;
  }

  if (need_timer) {
    hal_key_debounce_timer_restart();
  } else {
    // hal_sys_wake_unlock(HAL_SYS_WAKE_LOCK_USER_KEY);
  }
}

#if 0 // def CHECK_PWRKEY_AT_BOOT
static void hal_key_boot_handler(void *param)
{
#ifndef NO_PWRKEY
    uint32_t time;

    timer_active = false;

    time = hal_sys_timer_get();

    if (pwr_key.debounce || pwr_key.dither || pwr_key.pressed) {
        bool pressed = hal_pwrkey_get_status();

        //HAL_KEY_TRACE(5,"keyBoot: dbn=%d dither=%d pressed=%d/%d event=%d", pwr_key.debounce, pwr_key.dither, pwr_key.pressed, pressed, key_status.event);

        if (pwr_key.debounce) {
            pwr_key.pressed = pressed;
            if (pressed) {
                pwr_key.dither = false;
                if (time - pwr_key.time >= KEY_DEBOUNCE_INTERVAL) {
                    pwr_key.debounce = false;
                    key_status.time_updown = time;
                }
            } else {
                pwr_key.debounce = false;
                pwr_key.dither = true;
                pwr_key.time = time;
            }
        } else if (pwr_key.dither) {
            if (time - pwr_key.time >= KEY_DITHER_INTERVAL) {
                pwr_key.dither = false;
                pwr_key.pressed = false;
            }
        } else if (pwr_key.pressed) {
            if (!pressed) {
                pwr_key.pressed = false;
            }
        }
    }
    if (pwr_key.debounce || pwr_key.dither || pwr_key.pressed) {
        if (pwr_key.pressed) {
            if (key_status.event == HAL_KEY_EVENT_NONE) {
                if (time - key_status.time_updown >= KEY_INIT_DOWN_THRESHOLD) {
                    key_status.event = HAL_KEY_EVENT_INITDOWN;
                    send_key_event(HAL_KEY_CODE_PWR, key_status.event);
                }
            } else if (key_status.event == HAL_KEY_EVENT_INITDOWN) {
                if (time - key_status.time_updown >= KEY_INIT_LONGPRESS_THRESHOLD) {
                    key_status.cnt_repeat = 0;
                    key_status.event = HAL_KEY_EVENT_INITLONGPRESS;
                    send_key_event(HAL_KEY_CODE_PWR, key_status.event);
                }
            } else if (key_status.event == HAL_KEY_EVENT_INITLONGPRESS) {
                if (time - key_status.time_updown >= KEY_INIT_LONGLONGPRESS_THRESHOLD) {
                    key_status.event = HAL_KEY_EVENT_INITLONGLONGPRESS;
                    send_key_event(HAL_KEY_CODE_PWR, key_status.event);
                }
            }
        }
        hal_key_debounce_timer_restart();
    } else {
        if (key_status.event == HAL_KEY_EVENT_NONE || key_status.event == HAL_KEY_EVENT_INITDOWN) {
            send_key_event(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITUP);
        }
        send_key_event(HAL_KEY_CODE_PWR, HAL_KEY_EVENT_INITFINISHED);

        hwtimer_update(debounce_timer, hal_key_debounce_handler, NULL);
        //hal_sys_wake_unlock(HAL_SYS_WAKE_LOCK_USER_KEY);

        memset(&key_status, 0, sizeof(key_status));
        hal_pwrkey_reset();
        hal_key_enable_allint();
    }
#endif
}
#endif // CHECK_PWRKEY_AT_BOOT

int hal_key_open(int checkPwrKey, int (*cb)(uint32_t, uint8_t)) {
  int nRet = 0;
  uint32_t lock;

  key_detected_callback = cb;

  memset(&key_status, 0, sizeof(key_status));

  lock = int_lock();

#ifdef CHECK_PWRKEY_AT_BOOT
  if (checkPwrKey) {
    int cnt;
    int i = 0;

    cnt = 10;
    do {
      hal_sys_timer_delay(MS_TO_TICKS(150));
      if (!hal_pwrkey_startup_pressed()) {
        HAL_KEY_TRACE(0, "pwr_key init DITHERING");
        nRet = -1;
        goto _exit;
      }
    } while (++i < cnt);
  }
#endif

#ifndef NO_PWRKEY
  hal_pwrkey_open();
#endif
#if (CFG_HW_ADCKEY_NUMBER > 0)
  hal_adckey_open();
#endif
#if (CFG_HW_GPIOKEY_NUM > 0)
  hal_gpiokey_open();
#endif

#ifdef CHECK_PWRKEY_AT_BOOT
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
  if (checkPwrKey) {
    debounce_timer = hwtimer_alloc(hal_key_boot_handler, NULL);
    hal_pwrkey_handle_irq_state(HAL_PWRKEY_IRQ_FALLING_EDGE);
  } else
#endif
#endif
  {
    debounce_timer = hwtimer_alloc(hal_key_debounce_handler, NULL);
    hal_key_enable_allint();
  }

  ASSERT(debounce_timer, "Failed to alloc key debounce timer");

  goto _exit; // Avoid compiler warnings

_exit:
  int_unlock(lock);

  return nRet;
}

int hal_key_close(void) {
  hal_key_disable_allint();

#ifndef NO_PWRKEY
  hal_pwrkey_close();
#endif
#if (CFG_HW_ADCKEY_NUMBER > 0)
  hal_adckey_close();
#endif
#if (CFG_HW_GPIOKEY_NUM > 0)
  hal_gpiokey_close();
#endif

  if (debounce_timer) {
    hwtimer_stop(debounce_timer);
    hwtimer_free(debounce_timer);
    debounce_timer = NULL;
  }
  timer_active = false;
  key_detected_callback = NULL;

  // hal_sys_wake_unlock(HAL_SYS_WAKE_LOCK_USER_KEY);

  return 0;
}
