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
#ifndef __PMU_BEST2300P_H__
#define __PMU_BEST2300P_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_cmu.h"

enum PMU_EFUSE_PAGE_T {
    PMU_EFUSE_PAGE_SECURITY     = 0,
    PMU_EFUSE_PAGE_BOOT         = 1,
    PMU_EFUSE_PAGE_FEATURE      = 2,
    PMU_EFUSE_PAGE_BATTER_LV    = 3,

    PMU_EFUSE_PAGE_BATTER_HV    = 4,
    PMU_EFUSE_PAGE_SW_CFG       = 5,
    PMU_EFUSE_PAGE_PROD_TEST    = 6,
    PMU_EFUSE_PAGE_RESERVED_7   = 7,

    PMU_EFUSE_PAGE_BT_POWER     = 8,
    PMU_EFUSE_PAGE_DCCALIB2_L   = 9,
    PMU_EFUSE_PAGE_DCCALIB2_R   = 10,
    PMU_EFUSE_PAGE_DCCALIB_L    = 11,

    PMU_EFUSE_PAGE_DCCALIB_R    = 12,
    PMU_EFUSE_PAGE_RESERVED_13  = 13,
    PMU_EFUSE_PAGE_MODEL        = 14,
    PMU_EFUSE_PAGE_RESERVED_15  = 15,

    PMU_EFUSE_PAGE_BOOT_SIG1    = 16,
    PMU_EFUSE_PAGE_BOOT_SIG2    = 17,
    PMU_EFUSE_PAGE_BOOT_SIG3    = 18,
    PMU_EFUSE_PAGE_RESERVED_19  = 19,

    PMU_EFUSE_PAGE_RESERVED_20  = 20,
    PMU_EFUSE_PAGE_RESERVED_21  = 21,
    PMU_EFUSE_PAGE_RESERVED_22  = 22,
    PMU_EFUSE_PAGE_RESERVED_23  = 23,
};

enum PMU_IRQ_TYPE_T {
    PMU_IRQ_TYPE_PWRKEY,
    PMU_IRQ_TYPE_GPADC,
    PMU_IRQ_TYPE_RTC,
    PMU_IRQ_TYPE_CHARGER,

    PMU_IRQ_TYPE_QTY
};

enum PMU_PLL_DIV_TYPE_T {
    PMU_PLL_DIV_DIG,
    PMU_PLL_DIV_CODEC,
};

void pmu_codec_hppa_enable(int enable);

void pmu_codec_mic_bias_enable(uint32_t map);

void pmu_codec_mic_bias_lowpower_mode(uint32_t map);

int pmu_codec_volt_ramp_up(void);

int pmu_codec_volt_ramp_down(void);

void pmu_codec_vad_save_power(void);

void pmu_codec_vad_restore_power(void);

void pmu_lbrt_config(int enable);

void pmu_pll_div_reset_set(enum HAL_CMU_PLL_T pll);

void pmu_pll_div_reset_clear(enum HAL_CMU_PLL_T pll);

void pmu_pll_div_set(enum HAL_CMU_PLL_T pll, enum PMU_PLL_DIV_TYPE_T type, uint32_t div);

void pmu_led_set_hiz(enum HAL_GPIO_PIN_T pin);

#ifdef __cplusplus
}
#endif

#endif

