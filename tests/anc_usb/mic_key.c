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
#ifdef CFG_MIC_KEY

#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "hal_iomux.h"
#include "hal_gpadc.h"
#include "analog.h"
#include "usb_audio.h"
#include "tgt_hardware.h"

#define MIC_KEY_CHK_TIMES       20
#define MIC_KEY_SKIP_CNT        8
#define MIC_KEY_CHK_INTERVAL    MS_TO_TICKS(1)  // in ticks

static struct {
    HWTIMER_ID  chk_timer;

    uint16_t    chk_val[MIC_KEY_CHK_TIMES-MIC_KEY_SKIP_CNT];

    uint8_t     chk_times;
    uint8_t     gpio_dn;
    uint8_t     key_dn[MIC_KEY_NUM];
} s_mic_key_ctx;

extern void mic_key_gpio_irq_set (void);

void mic_key_irq_hdl (uint16_t irq_val, HAL_GPADC_MV_T volt)
{
    hal_gpadc_close(mic_key_gpadc_chan);

    if (++s_mic_key_ctx.chk_times >= MIC_KEY_CHK_TIMES) {

        uint8_t i, j;
        uint32_t mean;
        
        for (i = 0; i < MIC_KEY_NUM; i++) {
            mean = 0;

            for (j = 0; j < MIC_KEY_CHK_TIMES-MIC_KEY_SKIP_CNT; j++) {
                mean += (uint32_t)s_mic_key_ctx.chk_val[j];
            }
            mean /= MIC_KEY_CHK_TIMES-MIC_KEY_SKIP_CNT;

            if ((mean > mic_key_cfg_lst[i].ref_vol_low) && (mean <= mic_key_cfg_lst[i].ref_vol_high)) 
                break;
        }

        if (i < MIC_KEY_NUM) {
            TRACE(2,"ana_key: key[%d] matched at [%dmv]", i, mean);
            usb_audio_hid_set_event(mic_key_cfg_lst[i].hid_evt, 1);
            s_mic_key_ctx.key_dn[i] = 1;
        } else {
            TRACE(1,"ana_key: no key matched [%dmv]", mean);
        }

        return;
    }

    if (s_mic_key_ctx.chk_times > MIC_KEY_SKIP_CNT) {
        s_mic_key_ctx.chk_val[s_mic_key_ctx.chk_times - MIC_KEY_SKIP_CNT - 1] = volt;
    }

    hwtimer_start(s_mic_key_ctx.chk_timer, MIC_KEY_CHK_INTERVAL);
}

void mic_key_gpio_det_irq_hdl (enum HAL_GPIO_PIN_T pin)
{
    if (pin != mic_key_det_gpio_pin)
        return;

    TRACE(1,"GPIO detected at status[%d]", s_mic_key_ctx.gpio_dn);
    if (s_mic_key_ctx.gpio_dn) {

        uint8_t i;

        for (i = 0; i < MIC_KEY_NUM; i++) {
            if (s_mic_key_ctx.key_dn[i]) {
                break;
            }
        }
        if (i < MIC_KEY_NUM) {
            usb_audio_hid_set_event(mic_key_cfg_lst[i].hid_evt, 0);
            s_mic_key_ctx.key_dn[i] = 0;
        }

        s_mic_key_ctx.gpio_dn = 0;
    } else {
        s_mic_key_ctx.chk_times = 0;
        s_mic_key_ctx.gpio_dn = 1;
        hal_gpadc_open(mic_key_gpadc_chan, HAL_GPADC_ATP_ONESHOT, mic_key_irq_hdl);
    }

    mic_key_gpio_irq_set();
}


void mic_key_periodic_check (void* param)
{
    hal_gpadc_open(mic_key_gpadc_chan, HAL_GPADC_ATP_ONESHOT, mic_key_irq_hdl);
}

void mic_key_gpio_irq_set (void)
{
    struct HAL_GPIO_IRQ_CFG_T cfg;

    cfg.irq_debounce = 1;
    cfg.irq_enable = 1;
    cfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
    if (s_mic_key_ctx.gpio_dn) {
        cfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
    } else {
        cfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    }
    cfg.irq_handler = mic_key_gpio_det_irq_hdl;

    hal_gpio_setup_irq(mic_key_det_gpio_pin, &cfg);
}

void mic_key_open (void)
{
    struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_key_det[] = {
        {0, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_MEM, HAL_IOMUX_PIN_NOPULL},
    };

    TRACE(1,"%s", __func__);

    pinmux_key_det[0].pin = mic_key_det_gpio_pin;
    hal_iomux_init(pinmux_key_det, ARRAY_SIZE(pinmux_key_det));

    analog_aud_mickey_enable(true);

    s_mic_key_ctx.chk_timer = hwtimer_alloc(mic_key_periodic_check, NULL);
    hal_gpio_pin_set_dir(mic_key_det_gpio_pin, HAL_GPIO_DIR_IN, 0);
    mic_key_gpio_irq_set();
}


#endif
