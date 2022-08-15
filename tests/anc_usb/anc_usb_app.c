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
#include "anc_process.h"
#include "anc_usb_app.h"
#include "audioflinger.h"
#include "cmsis.h"
#include "hal_codec.h"
#include "hal_key.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "pmu.h"
#include "usb_audio_app.h"
#include "tgt_hardware.h"

#ifdef ANC_SWITCH_GPADC_CHAN
#include "hal_gpadc.h"
#endif

#define ANC_KEY_CPU_WAKE_USER           HAL_CPU_WAKE_LOCK_USER_4
#define ANC_STATE_CPU_WAKE_USER         HAL_CPU_WAKE_LOCK_USER_5

#define ANC_FADE_IN_OUT
#define ANC_FADE_GAIN_STEP              1
#define ANC_MODE_SWITCH_WITHOUT_FADE

#ifndef ANC_INIT_ON_TIMEOUT_MS
#ifdef CHIP_BEST1000
#define ANC_INIT_ON_TIMEOUT_MS          1200
#else
#define ANC_INIT_ON_TIMEOUT_MS          200
#endif
#endif
#ifndef ANC_SHUTDOWN_TIMEOUT_MS
#define ANC_SHUTDOWN_TIMEOUT_MS         5000
#endif

#ifndef ANC_SWITCH_CHECK_INTERVAL
#define ANC_SWITCH_CHECK_INTERVAL       MS_TO_TICKS(500)
#endif

#ifdef ANC_COEF_NUM
#if (ANC_COEF_NUM < 1)
#error "Invalid ANC_COEF_NUM configuration"
#endif
#else
#define ANC_COEF_NUM                    (1)
#endif

#define KEY_PROCESS_TIMER_INTERVAL      (MS_TO_TICKS(1) / 2)
#define FADE_TIMER_INTERVAL             (MS_TO_TICKS(1) / 2)

enum ANC_STATUS_T {
    ANC_STATUS_NULL = 0,
    ANC_STATUS_INIT,
    ANC_STATUS_FADEIN,
    ANC_STATUS_ENABLE,
    ANC_STATUS_FADEOUT,
    ANC_STATUS_DISABLE,
    ANC_STATUS_INIT_CLOSE,
};

enum ANC_KEY_STATE_T {
    ANC_KEY_STATE_NULL = 0,
    ANC_KEY_STATE_CLOSE,
    ANC_KEY_STATE_OPEN,
    ANC_KEY_STATE_DEBOUNCE,
};

enum ANC_CTRL_SM_T {
    ANC_CTRL_SM_OFF = 0,
    ANC_CTRL_SM_COEF_0,
    ANC_CTRL_SM_COEF_N = ANC_CTRL_SM_COEF_0 + ANC_COEF_NUM - 1,

    ANC_CTRL_SM_QTY
};

static enum ANC_STATUS_T anc_status = ANC_STATUS_NULL;
static enum ANC_KEY_STATE_T prev_key_state = ANC_KEY_STATE_NULL;
static enum HAL_KEY_EVENT_T anc_key_event = HAL_KEY_EVENT_NONE;
static enum ANC_CTRL_SM_T anc_ctrl_sm = ANC_CTRL_SM_OFF;
#ifdef ANC_TALK_THROUGH
static bool talk_through = false;
#endif

static uint32_t anc_init_time;
static uint32_t anc_close_time;

static enum ANC_INDEX cur_coef_index = ANC_INDEX_0;

static enum AUD_SAMPRATE_T anc_sample_rate[AUD_STREAM_NUM];

static bool anc_running = false;

#ifdef ANC_SWITCH_GPIO_PIN
static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_anc = {ANC_SWITCH_GPIO_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE};
static bool gpio_pending;
static bool gpio_irq_en;
static enum HAL_GPIO_IRQ_POLARITY_T gpio_irq_polarity;
#endif

// ANC_SWITCH_GPADC_CHAN
static HWTIMER_ID gpadc_timer;

// ANC_FADE_IN_OUT
static uint8_t fadein_cnt = 0;
static uint8_t fadeout_cnt = 0;
static uint32_t prev_fade_time = 0;

#ifdef CFG_HW_ANC_LED_PIN
const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_anc_led[] = {
    {CFG_HW_ANC_LED_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
};
#endif
#ifdef CFG_HW_ANC_LED_PIN2
const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_anc_led2[] = {
    {CFG_HW_ANC_LED_PIN2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
};
#endif

static void anc_full_open(void);
static void anc_full_close(void);

// ANC_FADE_IN_OUT
int anc_usb_app_fadein(enum ANC_TYPE_T anc_type)
{


    int32_t gain0_curr, gain1_curr;
    int32_t gain0_tg, gain1_tg;

    anc_get_gain(&gain0_curr, &gain1_curr,anc_type);
    anc_get_cfg_gain(&gain0_tg, &gain1_tg,anc_type);

/*
    anc_set_gain(gain0_tg, gain1_tg,anc_type);

    return 0;
*/
    if (gain0_tg > 0) {
        if (gain0_curr < gain0_tg) {
            if (gain0_curr + ANC_FADE_GAIN_STEP < gain0_tg) {
                gain0_curr += ANC_FADE_GAIN_STEP;
            } else {
                gain0_curr = gain0_tg;
            }
        }
    } else {
        if (gain0_curr > gain0_tg) {
            if (gain0_curr - ANC_FADE_GAIN_STEP > gain0_tg) {
                gain0_curr -= ANC_FADE_GAIN_STEP;
            } else {
                gain0_curr = gain0_tg;
            }
        }
    }

    if (gain1_tg > 0) {
        if (gain1_curr < gain1_tg) {
            if (gain1_curr + ANC_FADE_GAIN_STEP < gain1_tg) {
                gain1_curr += ANC_FADE_GAIN_STEP;
            } else {
                gain1_curr = gain1_tg;
            }
        }
    } else {
        if (gain1_curr > gain1_tg) {
            if (gain1_curr - ANC_FADE_GAIN_STEP > gain1_tg) {
                gain1_curr -= ANC_FADE_GAIN_STEP;
            } else {
                gain1_curr = gain1_tg;
            }
        }
    }

    //TRACE(3,"[%s] cur gain: %d %d", __func__, gain0_curr, gain1_curr);
    anc_set_gain(gain0_curr, gain1_curr,anc_type);

    if ((gain0_curr == gain0_tg) && (gain1_curr == gain1_tg)) {
        return 0;
    }

    return 1;
}

int anc_usb_app_fadeout(enum ANC_TYPE_T anc_type)
{

/*
    anc_set_gain(0, 0,anc_type);

    return 0;
*/

    int32_t gain0_curr, gain1_curr;

    anc_get_gain(&gain0_curr, &gain1_curr,anc_type);

    if (gain0_curr > 0) {
        if (gain0_curr > ANC_FADE_GAIN_STEP) {
            gain0_curr -= ANC_FADE_GAIN_STEP;
        } else {
            gain0_curr = 0;
        }
    } else if (gain0_curr < 0) {
        if (gain0_curr < -ANC_FADE_GAIN_STEP) {
            gain0_curr += ANC_FADE_GAIN_STEP;
        } else {
            gain0_curr = 0;
        }
    }

    if (gain1_curr > 0) {
        if (gain1_curr > ANC_FADE_GAIN_STEP) {
            gain1_curr -= ANC_FADE_GAIN_STEP;
        } else {
            gain1_curr = 0;
        }
    } else if (gain1_curr < 0) {
        if (gain1_curr < -ANC_FADE_GAIN_STEP) {
            gain1_curr += ANC_FADE_GAIN_STEP;
        } else {
            gain1_curr = 0;
        }
    }

    // TRACE(3,"[%s] gain: %d, %d", __func__, gain0_curr, gain1_curr);
    anc_set_gain(gain0_curr, gain1_curr,anc_type);

    if ((gain0_curr == 0) && (gain1_curr == 0)) {
        return 0;
    }

    return 1;
}

void anc_usb_app(bool on)
{
    TRACE(2,"%s: on=%d", __func__, on);

    if (anc_running==on)
        return;
    else
        anc_running=on;

    if (on) {
        anc_enable();
    } else {
        anc_disable();
    }

#ifdef CFG_HW_ANC_LED_PIN
    if (on) {
        hal_gpio_pin_set(CFG_HW_ANC_LED_PIN);
    } else {
        hal_gpio_pin_clr(CFG_HW_ANC_LED_PIN);
    }
#endif
#ifdef CFG_HW_ANC_LED_PIN2
    if (on) {
        hal_gpio_pin_set(CFG_HW_ANC_LED_PIN2);
    } else {
        hal_gpio_pin_clr(CFG_HW_ANC_LED_PIN2);
    }
#endif
}

bool anc_usb_app_get_status()
{
    return anc_running;
}

#ifdef ANC_SWITCH_GPIO_PIN
static void anc_key_gpio_handler(enum HAL_GPIO_PIN_T pin)
{
    struct HAL_GPIO_IRQ_CFG_T gpiocfg;

    gpiocfg.irq_enable = false;
    hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)pinmux_anc.pin, &gpiocfg);

    gpio_irq_en = false;
    gpio_pending = true;

    hal_cpu_wake_lock(ANC_KEY_CPU_WAKE_USER);
}

static void anc_key_gpio_irq_setup(enum HAL_GPIO_IRQ_POLARITY_T polarity)
{
    struct HAL_GPIO_IRQ_CFG_T gpiocfg;

    if (gpio_irq_en && gpio_irq_polarity == polarity) {
        return;
    }

    gpio_irq_polarity = polarity;
    gpio_irq_en = true;

    gpiocfg.irq_enable = true;
    gpiocfg.irq_debounce = true;
    gpiocfg.irq_polarity = polarity;
    gpiocfg.irq_handler = anc_key_gpio_handler;
    gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE;

    hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)pinmux_anc.pin, &gpiocfg);
}
#endif

static enum ANC_KEY_STATE_T anc_key_level_to_state(bool level)
{
    enum ANC_KEY_STATE_T key_state;

#ifdef ANC_SWITCH_GPIO_PIN
    anc_key_gpio_irq_setup(level ? HAL_GPIO_IRQ_POLARITY_LOW_FALLING : HAL_GPIO_IRQ_POLARITY_HIGH_RISING);
#endif

    key_state = level ? ANC_KEY_STATE_OPEN : ANC_KEY_STATE_CLOSE;

    return key_state;
}

enum ANC_KEY_STATE_T anc_key_get_state(bool init_state)
{
    enum ANC_KEY_STATE_T key_state = ANC_KEY_STATE_NULL;
    uint8_t level;

#if defined(ANC_SWITCH_GPIO_PIN)
    level = hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)pinmux_anc.pin);

    level = level ? true : false;

#elif defined(ANC_SWITCH_GPADC_CHAN)
    HAL_GPADC_MV_T volt = 0;

    if (hal_gpadc_get_volt(ANC_SWITCH_GPADC_CHAN, &volt)) {
        level = volt > ANC_SWITCH_VOLTAGE_THRESHOLD ? ANC_SWITCH_LEVEL_HIGH : ANC_SWITCH_LEVEL_LOW;
        // TRACE(3,"[%s] level = %d, volt = %d", __func__, level, volt);
    } else {
        // TRACE(1,"[%s] else...", __func__);
        return ANC_KEY_STATE_NULL;
    }
#else
    return key_state;
#endif

    static uint32_t s_time;
    static bool key_trigger = false;
    static bool key_level = false;

    if (init_state) {
        key_level = level;
        return anc_key_level_to_state(level);
    }

    if (key_trigger) {
        if (key_level == level) {
            key_trigger = false;
        } else {
            uint32_t diff_time_ms;

            diff_time_ms = TICKS_TO_MS(hal_sys_timer_get() - s_time);

            if(diff_time_ms >= 200) {
                key_level = level;
                key_trigger = false;
            }
        }
    } else {
        if (key_level != level) {
            s_time = hal_sys_timer_get();
            key_trigger = true;
        }
    }

    if (key_trigger) {
        key_state = ANC_KEY_STATE_DEBOUNCE;
    } else {
        key_state = anc_key_level_to_state(level);
    }

    return key_state;
}

static void anc_key_proc_open(bool from_key)
{
    int POSSIBLY_UNUSED ret;
    uint32_t POSSIBLY_UNUSED time;

    time = hal_sys_timer_get();

    if (anc_status == ANC_STATUS_INIT_CLOSE) {
        TRACE(1,"[ANC KEY PROC] ANC INIT_CLOSE => INIT T:%u", time);

        anc_status = ANC_STATUS_INIT;
    } else if (anc_status == ANC_STATUS_NULL) {
        TRACE(1,"[ANC KEY PROC] ANC NULL => INIT T:%u", time);

        anc_full_open();
#ifdef ANC_MODE_SWITCH_WITHOUT_FADE
    } else if (anc_status == ANC_STATUS_ENABLE) {
#ifdef ANC_FF_ENABLED
        ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index,ANC_FEEDFORWARD,ANC_GAIN_NO_DELAY);
        TRACE(2,"[ANC KEY PROC] updata coefs ff %d: ret=%d", cur_coef_index, ret);
#endif
#ifdef ANC_FB_ENABLED
	 ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index,ANC_FEEDBACK,ANC_GAIN_NO_DELAY);
        TRACE(2,"[ANC KEY PROC] updata coefs fb %d: ret=%d", cur_coef_index, ret);
#endif
#if defined(AUDIO_ANC_FB_MC_HW)
        ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index, ANC_MUSICCANCLE, ANC_GAIN_NO_DELAY);
        TRACE(2,"[ANC KEY PROC] anc_select_coef mc %d: ret=%d", cur_coef_index, ret);
#endif
#if defined(AUDIO_ANC_TT_HW)
        ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index, ANC_TALKTHRU, ANC_GAIN_NO_DELAY);
        TRACE(2,"[ANC KEY PROC] anc_select_coef tt %d: ret=%d", cur_coef_index, ret);
#endif

#endif
    }else{
        if (from_key && anc_status == ANC_STATUS_INIT) {
            TRACE(1,"[ANC KEY PROC] ANC ON2 T:%u", time);
            // Let state machine enable ANC
            return;
        }

        TRACE(1,"[ANC KEY PROC] ANC ON T:%u", time);

#ifdef ANC_FF_ENABLED
        ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index,ANC_FEEDFORWARD,ANC_GAIN_DELAY);
        TRACE(2,"[ANC KEY PROC] anc_select_coef ff %d: ret=%d", cur_coef_index, ret);
#endif
#ifdef ANC_FB_ENABLED
	 ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index,ANC_FEEDBACK,ANC_GAIN_DELAY);
        TRACE(2,"[ANC KEY PROC] anc_select_coef fb %d: ret=%d", cur_coef_index, ret);
#endif
#if defined(AUDIO_ANC_FB_MC_HW)
        ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index, ANC_MUSICCANCLE, ANC_GAIN_DELAY);
        TRACE(2,"[ANC KEY PROC] anc_select_coef mc %d: ret=%d", cur_coef_index, ret);
#endif
#if defined(AUDIO_ANC_TT_HW)
        ret = anc_select_coef(anc_sample_rate[AUD_STREAM_PLAYBACK],cur_coef_index, ANC_TALKTHRU, ANC_GAIN_DELAY);
        TRACE(2,"[ANC KEY PROC] anc_select_coef tt %d: ret=%d", cur_coef_index, ret);
#endif

        anc_usb_app(true);
#ifdef ANC_FADE_IN_OUT
        fadein_cnt = 0;
        anc_status = ANC_STATUS_FADEIN;
#else
		{
			int32_t gain_ch_l, gain_ch_r;
#if defined(AUDIO_ANC_TT_HW)
			anc_get_cfg_gain(&gain_ch_l, &gain_ch_r,ANC_TALKTHRU);
			anc_set_gain(gain_ch_l,gain_ch_r,ANC_TALKTHRU);
#endif
#ifdef ANC_FF_ENABLED
			anc_get_cfg_gain(&gain_ch_l, &gain_ch_r,ANC_FEEDFORWARD);
			anc_set_gain(gain_ch_l,gain_ch_r,ANC_FEEDFORWARD);
#endif
#ifdef ANC_FB_ENABLED
			anc_get_cfg_gain(&gain_ch_l, &gain_ch_r,ANC_FEEDBACK);
			anc_set_gain(gain_ch_l,gain_ch_r,ANC_FEEDBACK);
#endif
#ifdef AUDIO_ANC_FB_MC_HW
			anc_get_cfg_gain(&gain_ch_l, &gain_ch_r,ANC_MUSICCANCLE);
			anc_set_gain(gain_ch_l,gain_ch_r,ANC_MUSICCANCLE);
#endif
		}
        anc_status = ANC_STATUS_ENABLE;
#endif
    }
}

static void anc_key_proc_close(bool from_key)
{
    uint32_t time;

    time = hal_sys_timer_get();
    anc_close_time = time;

    if (anc_status == ANC_STATUS_INIT) {
        TRACE(1,"[ANC KEY PROC] ANC INIT => INIT_CLOSE T:%u", time);

        anc_status = ANC_STATUS_INIT_CLOSE;
    } else if (anc_status == ANC_STATUS_INIT_CLOSE || anc_status == ANC_STATUS_DISABLE) {
        if (from_key) {
            TRACE(1,"[ANC KEY PROC] ANC OFF2 T:%u", time);
            // Let state machine to shutdown ANC
            return;
        }

        TRACE(1,"[ANC KEY PROC] ANC CLOSE => NULL T:%u", time);

        anc_full_close();
    } else {
        TRACE(1,"[ANC KEY PROC] ANC OFF T:%u", time);

#ifdef ANC_FADE_IN_OUT
        fadeout_cnt = 0;
        anc_status = ANC_STATUS_FADEOUT;
#else
        anc_usb_app(false);
        anc_status = ANC_STATUS_DISABLE;
#endif
    }
}

void anc_switch_on_off_ctrl(void)
{
    uint32_t lock;
    enum ANC_KEY_STATE_T key_state = ANC_KEY_STATE_NULL;

    lock = int_lock();
    // Check if there is a state change
    if (anc_key_event != HAL_KEY_EVENT_NONE) {
        key_state = prev_key_state;
        anc_key_event = HAL_KEY_EVENT_NONE;
    }
    int_unlock(lock);

    if(key_state == ANC_KEY_STATE_OPEN) {
        anc_key_proc_open(true);
    } else if(key_state == ANC_KEY_STATE_CLOSE) {
        anc_key_proc_close(true);
    }
}

void anc_double_click_on_off(void)
{
    uint32_t lock;
    enum HAL_KEY_EVENT_T event;

    lock = int_lock();
    event = anc_key_event;
    anc_key_event = HAL_KEY_EVENT_NONE;
    hal_cpu_wake_unlock(ANC_KEY_CPU_WAKE_USER);
    int_unlock(lock);

    if (event == HAL_KEY_EVENT_NONE) {
        return;
    } else if (event == HAL_KEY_EVENT_DOUBLECLICK) {
        prev_key_state = (prev_key_state == ANC_KEY_STATE_OPEN) ? ANC_KEY_STATE_CLOSE : ANC_KEY_STATE_OPEN;
        if (prev_key_state == ANC_KEY_STATE_OPEN) {
            anc_key_proc_open(true);
        } else {
            anc_key_proc_close(true);
        }
    }
}

void anc_click_on_off(void)
{
    enum ANC_STATE_UPDATE_T {
        ANC_STATE_UPDATE_NULL = 0,
        ANC_STATE_UPDATE_TALK_THROUGH,
        ANC_STATE_UPDATE_COEF,
    };

    enum HAL_KEY_EVENT_T event;
    enum ANC_STATE_UPDATE_T state_update = ANC_STATE_UPDATE_NULL;
    uint8_t click_cnt = 0;
    uint32_t lock;

    lock = int_lock();
    event = anc_key_event;
    anc_key_event = HAL_KEY_EVENT_NONE;
    hal_cpu_wake_unlock(ANC_KEY_CPU_WAKE_USER);
    int_unlock(lock);

    if (event == HAL_KEY_EVENT_NONE) {
        return;
#ifdef ANC_TALK_THROUGH
    } else if (event == HAL_KEY_EVENT_LONGPRESS) {
        talk_through = !talk_through;
        if (talk_through) {
            state_update = ANC_STATE_UPDATE_TALK_THROUGH;
            cur_coef_index = ANC_COEF_NUM;
            TRACE(0,"[ANC KEY PROC] anc_talk_through on");
        } else {
            state_update = ANC_STATE_UPDATE_COEF;
            TRACE(0,"[ANC KEY PROC] anc_talk_through off");
        }
#endif
    } else if (event == HAL_KEY_EVENT_CLICK) {
        click_cnt = 1;
    } if (event == HAL_KEY_EVENT_DOUBLECLICK) {
        click_cnt = 2;
    } if (event == HAL_KEY_EVENT_TRIPLECLICK) {
        click_cnt = 3;
    }

    if (click_cnt > 0) {
        anc_ctrl_sm = (anc_ctrl_sm + click_cnt) % ANC_CTRL_SM_QTY;
        state_update = ANC_STATE_UPDATE_COEF;
#ifdef ANC_TALK_THROUGH
        talk_through = false;
#endif
    }

    if (state_update == ANC_STATE_UPDATE_COEF) {
        if (anc_ctrl_sm >= ANC_CTRL_SM_COEF_0 && anc_ctrl_sm <= ANC_CTRL_SM_COEF_N) {
            cur_coef_index = anc_ctrl_sm - ANC_CTRL_SM_COEF_0;
        }
    }

    if (state_update != ANC_STATE_UPDATE_NULL) {
        if (anc_ctrl_sm == ANC_CTRL_SM_OFF && state_update != ANC_STATE_UPDATE_TALK_THROUGH) {
            anc_key_proc_close(true);
        } else {
            anc_key_proc_open(true);
        }
    }
}

static void anc_key_process(void)
{
#if defined(ANC_SWITCH_GPIO_PIN) || defined(ANC_SWITCH_GPADC_CHAN)

    anc_switch_on_off_ctrl();

#elif defined(ANC_KEY_DOUBLE_CLICK_ON_OFF)

    anc_double_click_on_off();

#else

    anc_click_on_off();

#endif
}

void anc_state_transition(void)
{
    uint32_t t_time;
#ifdef ANC_FADE_IN_OUT
    int res_ff = 0, res_fb = 0;
#ifdef AUDIO_ANC_TT_HW
    int res_tt = 0;
#endif
#ifdef AUDIO_ANC_FB_MC_HW
    int  res_mc = 0;
#endif
#endif

    t_time = hal_sys_timer_get();

    if (anc_status == ANC_STATUS_INIT) {
        if (t_time - anc_init_time >= MS_TO_TICKS(ANC_INIT_ON_TIMEOUT_MS)) {
            //TRACE(2,"[%s] anc init on T:%u", __func__, t_time);
            anc_key_proc_open(false);
            // fadein or open
        }
    }

#ifdef ANC_FADE_IN_OUT
    if(anc_status == ANC_STATUS_FADEIN)
    {
        // process
        if(fadein_cnt == 0)
        {
            TRACE(2,"[%s] anc fadein started T:%u", __func__, t_time);
            prev_fade_time = t_time;
            fadein_cnt++;
        }
        else if(fadein_cnt == 1)
        {
            // delay 60 ticks
            if(t_time - prev_fade_time >= 60)
            {
                fadein_cnt++;
                prev_fade_time = t_time;
            }
        }
        else
        {
            // delay 1 ticks
            if(t_time > prev_fade_time)
            {
                //TRACE(2,"[%s] anc_usb_app_fadein T:%u", __func__, t_time);
#ifdef AUDIO_ANC_TT_HW
                res_tt = anc_usb_app_fadein(ANC_TALKTHRU);
#endif
#ifdef ANC_FF_ENABLED
                res_ff = anc_usb_app_fadein(ANC_FEEDFORWARD);
#endif
#ifdef ANC_FB_ENABLED
                res_fb = anc_usb_app_fadein(ANC_FEEDBACK);
#endif
#ifdef AUDIO_ANC_FB_MC_HW
                res_mc = anc_usb_app_fadein(ANC_MUSICCANCLE);
#endif
                if(res_ff==0&&res_fb==0
#ifdef AUDIO_ANC_TT_HW
                    &&res_tt==0
#endif
#ifdef AUDIO_ANC_FB_MC_HW
                    &&res_mc==0
#endif
                    )
                {
                    anc_status = ANC_STATUS_ENABLE;
                    TRACE(2,"[%s] anc fadein done T:%u", __func__, t_time);
                }
                else
                {
                    prev_fade_time = t_time;
                }
            }
        }
    }
    else if(anc_status == ANC_STATUS_FADEOUT)
    {
        // process
        if(fadeout_cnt == 0)
        {
            TRACE(2,"[%s] anc fadeout started T:%u", __func__, t_time);
            prev_fade_time = t_time;
            fadeout_cnt++;
        }
        else if(fadeout_cnt == 1)
        {
            // delay 1 ticks
            if(t_time > prev_fade_time)
            {
                //TRACE(2,"[%s] anc_usb_app_fadeout T:%u", __func__, t_time);
#ifdef AUDIO_ANC_TT_HW
                res_tt = anc_usb_app_fadeout(ANC_TALKTHRU);
#endif
#ifdef ANC_FF_ENABLED
                res_ff = anc_usb_app_fadeout(ANC_FEEDFORWARD);
#endif
#ifdef ANC_FB_ENABLED
		  res_fb = anc_usb_app_fadeout(ANC_FEEDBACK);
#endif
#ifdef AUDIO_ANC_FB_MC_HW
                res_mc = anc_usb_app_fadein(ANC_MUSICCANCLE);
#endif
                if(res_ff==0&&res_fb==0
#ifdef AUDIO_ANC_TT_HW
                    &&res_tt==0
#endif
#ifdef AUDIO_ANC_FB_MC_HW
                    &&res_mc==0
#endif
                    )
                {
                    fadeout_cnt++;
                }

                prev_fade_time = t_time;
            }
        }
        else
        {
            anc_usb_app(false);
            anc_status = ANC_STATUS_DISABLE;
            TRACE(2,"[%s] anc fadeout done T:%u", __func__, t_time);
        }
    }
#endif

    if (anc_status == ANC_STATUS_INIT_CLOSE || anc_status == ANC_STATUS_DISABLE) {
        if(t_time - anc_close_time >= MS_TO_TICKS(ANC_SHUTDOWN_TIMEOUT_MS)) {
            //TRACE(2,"[%s] anc shutdown T:%u", __func__, t_time);
            anc_key_proc_close(false);
        }
    }

    if (anc_status == ANC_STATUS_NULL || anc_status == ANC_STATUS_ENABLE) {
        hal_cpu_wake_unlock(ANC_STATE_CPU_WAKE_USER);
    } else {
        hal_cpu_wake_lock(ANC_STATE_CPU_WAKE_USER);
    }
}

int anc_usb_app_key(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event)
{
#if !defined(ANC_SWITCH_GPIO_PIN) && !defined(ANC_SWITCH_GPADC_CHAN)

#ifdef ANC_KEY_DOUBLE_CLICK_ON_OFF

    if (code == ANC_FUNCTION_KEY) {
        if (event == HAL_KEY_EVENT_DOUBLECLICK) {
            anc_key_event = event;
            hal_cpu_wake_lock(ANC_KEY_CPU_WAKE_USER);
            // The key event has been processed
            return 0;
        }
    }

#else

    if (code == ANC_FUNCTION_KEY) {
        if (event == HAL_KEY_EVENT_CLICK || event == HAL_KEY_EVENT_DOUBLECLICK ||
                event == HAL_KEY_EVENT_TRIPLECLICK || event == HAL_KEY_EVENT_LONGPRESS) {
            anc_key_event = event;
            hal_cpu_wake_lock(ANC_KEY_CPU_WAKE_USER);
        }
        // The key event has been processed
        return 0;
    }

#endif

#endif

    // Let other applications check the key event
    return 1;
}

void anc_key_check(void)
{
    enum ANC_KEY_STATE_T key_state;

#ifdef ANC_SWITCH_GPIO_PIN
    uint32_t lock;

    gpio_pending = false;
#endif

    key_state = anc_key_get_state(false);

    if (key_state == ANC_KEY_STATE_NULL) {
        return;
    } else if (key_state == ANC_KEY_STATE_DEBOUNCE) {
        hal_cpu_wake_lock(ANC_KEY_CPU_WAKE_USER);
    } else if (key_state != prev_key_state) {
        prev_key_state = key_state;
        // Reuse click event to tag a state change
        anc_key_event = HAL_KEY_EVENT_CLICK;
        hal_cpu_wake_lock(ANC_KEY_CPU_WAKE_USER);
    } else if (anc_key_event == HAL_KEY_EVENT_NONE) {
#ifdef ANC_SWITCH_GPIO_PIN
        lock = int_lock();
        if (!gpio_pending) {
            hal_cpu_wake_unlock(ANC_KEY_CPU_WAKE_USER);
        }
        int_unlock(lock);
#else
        hal_cpu_wake_unlock(ANC_KEY_CPU_WAKE_USER);
#endif
    }
}

void anc_key_gpadc_timer_handler(void *param)
{
    hwtimer_start(gpadc_timer, ANC_SWITCH_CHECK_INTERVAL);
}

static void anc_key_init(void)
{
#if defined(ANC_SWITCH_GPIO_PIN)
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pinmux_anc, 1);
    hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)pinmux_anc.pin, HAL_GPIO_DIR_IN, 0);
    // Make sure gpio value is ready
    hal_sys_timer_delay(3);
#elif defined(ANC_SWITCH_GPADC_CHAN)
    hal_gpadc_open(ANC_SWITCH_GPADC_CHAN, HAL_GPADC_ATP_20MS, NULL);
    // Make sure gpadc channel data is ready
    hal_sys_timer_delay(6);
    gpadc_timer = hwtimer_alloc(anc_key_gpadc_timer_handler, NULL);
    ASSERT(gpadc_timer, "Failed to alloc gpadc timer");
    hwtimer_start(gpadc_timer, ANC_SWITCH_CHECK_INTERVAL);
#endif

    // Add other key initialize
}

#if defined(USB_AUDIO_DYN_CFG) && !defined(__AUDIO_RESAMPLE__)
static void anc_sample_rate_change(enum AUD_STREAM_T stream, enum AUD_SAMPRATE_T rate, enum AUD_SAMPRATE_T *new_play, enum AUD_SAMPRATE_T *new_cap)
{
    enum AUD_SAMPRATE_T play_rate, cap_rate;

    if (anc_sample_rate[stream] != rate) {
#ifdef CHIP_BEST1000
        if (stream == AUD_STREAM_PLAYBACK) {
            play_rate = rate;
            cap_rate = rate * (anc_sample_rate[AUD_STREAM_CAPTURE] / anc_sample_rate[AUD_STREAM_PLAYBACK]);
        } else {
            play_rate = rate / (anc_sample_rate[AUD_STREAM_CAPTURE] / anc_sample_rate[AUD_STREAM_PLAYBACK]);
            cap_rate = rate;
        }
#else
        play_rate = rate;
        cap_rate = rate;
#ifdef ANC_FF_ENABLED
        anc_select_coef(play_rate,cur_coef_index, ANC_FEEDFORWARD, ANC_GAIN_NO_DELAY);
#endif

#ifdef ANC_FB_ENABLED
        anc_select_coef(play_rate,cur_coef_index, ANC_FEEDBACK, ANC_GAIN_NO_DELAY);
#endif

#if defined(AUDIO_ANC_FB_MC_HW)
      anc_select_coef(play_rate,cur_coef_index, ANC_MUSICCANCLE, ANC_GAIN_NO_DELAY);
#endif

#if defined(AUDIO_ANC_TT_HW)
      anc_select_coef(play_rate,cur_coef_index, ANC_TALKTHRU, ANC_GAIN_NO_DELAY);
#endif

#endif

        TRACE(5,"%s: Update anc sample rate from %u/%u to %u/%u", __func__,
            anc_sample_rate[AUD_STREAM_PLAYBACK], anc_sample_rate[AUD_STREAM_CAPTURE], play_rate, cap_rate);

        if (new_play) {
            *new_play= play_rate;
        }
        if (new_cap) {
            *new_cap = cap_rate;
        }

        anc_sample_rate[AUD_STREAM_PLAYBACK] = play_rate;
        anc_sample_rate[AUD_STREAM_CAPTURE] = cap_rate;
    }
}
#endif

static void anc_full_open(void)
{
    AF_ANC_HANDLER POSSIBLY_UNUSED handler;

#if defined(USB_AUDIO_DYN_CFG) && !defined(__AUDIO_RESAMPLE__)
    handler = anc_sample_rate_change;
#else
    handler = NULL;
#endif

#ifdef USB_AUDIO_APP
    usb_audio_keep_streams_running(true);
#endif

    pmu_anc_config(1);

#ifdef ANC_FF_ENABLED
    af_anc_open(ANC_FEEDFORWARD, anc_sample_rate[AUD_STREAM_PLAYBACK], anc_sample_rate[AUD_STREAM_CAPTURE], handler);
    anc_open(ANC_FEEDFORWARD);
#endif

#ifdef ANC_FB_ENABLED
    af_anc_open(ANC_FEEDBACK, anc_sample_rate[AUD_STREAM_PLAYBACK], anc_sample_rate[AUD_STREAM_CAPTURE], handler);
    anc_open(ANC_FEEDBACK);
#if defined(AUDIO_ANC_FB_MC_HW)
    anc_open(ANC_MUSICCANCLE);
#endif
#endif

#if defined(AUDIO_ANC_TT_HW)
    anc_open(ANC_TALKTHRU);
#endif


    anc_init_time = hal_sys_timer_get();
    anc_status = ANC_STATUS_INIT;
}

static void anc_full_close(void)
{

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

    anc_status = ANC_STATUS_NULL;

#ifdef USB_AUDIO_APP
    usb_audio_keep_streams_running(false);
#endif
}

void anc_usb_app_loop(void)
{
#if defined(ANC_SWITCH_GPIO_PIN) || defined(ANC_SWITCH_GPADC_CHAN)
    anc_key_check();
#endif
    anc_key_process();
    anc_state_transition();
}

void anc_usb_app_init(enum AUD_IO_PATH_T input_path, enum AUD_SAMPRATE_T playback_rate, enum AUD_SAMPRATE_T capture_rate)
{
    enum ANC_KEY_STATE_T key_state;

#ifdef CFG_HW_ANC_LED_PIN
    hal_iomux_init(pinmux_anc_led, ARRAY_SIZE(pinmux_anc_led));
    hal_gpio_pin_set_dir(CFG_HW_ANC_LED_PIN, HAL_GPIO_DIR_OUT, 0);
#endif
#ifdef CFG_HW_ANC_LED_PIN2
    hal_iomux_init(pinmux_anc_led2, ARRAY_SIZE(pinmux_anc_led2));
    hal_gpio_pin_set_dir(CFG_HW_ANC_LED_PIN2, HAL_GPIO_DIR_OUT, 0);
#endif

    anc_key_init();

#ifdef __AUDIO_SECTION_SUPPT__
    anc_load_cfg();
#endif

    anc_sample_rate[AUD_STREAM_PLAYBACK] = hal_codec_anc_convert_rate(playback_rate);
    anc_sample_rate[AUD_STREAM_CAPTURE] = hal_codec_anc_convert_rate(capture_rate);

#if defined(ANC_SWITCH_GPIO_PIN) || defined(ANC_SWITCH_GPADC_CHAN)
    key_state = anc_key_get_state(true);
    prev_key_state = key_state;
#elif defined(ANC_KEY_DOUBLE_CLICK_ON_OFF)
    key_state = ANC_KEY_STATE_OPEN;
    prev_key_state = key_state;
#else
    key_state = ANC_KEY_STATE_OPEN;
    anc_ctrl_sm = ANC_CTRL_SM_COEF_0 + cur_coef_index;
#endif

#ifdef ANC_INIT_OFF
    key_state = ANC_KEY_STATE_CLOSE;
    prev_key_state = key_state;
    anc_ctrl_sm = ANC_CTRL_SM_OFF;
#endif

    if (key_state == ANC_KEY_STATE_OPEN) {
        anc_full_open();
    }
}

void anc_usb_app_term(void)
{
    anc_full_close();
}

