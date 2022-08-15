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
#ifndef __HW_CODEC_IIR_PROCESS_H__
#define __HW_CODEC_IIR_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "hal_aud.h"

#if defined(CHIP_BEST1402) ||defined(CHIP_BEST2001) ||defined(CHIP_BEST2300A)
#define AUD_DAC_IIR_NUM_EQ                        (20)
#else
#define AUD_DAC_IIR_NUM_EQ                        (8)
#endif

typedef enum {
    HW_CODEC_IIR_NO_ERR=0,
    HW_CODEC_IIR_TYPE_ERR,
    HW_CODEC_IIR_SAMPLERATE_ERR,
    HW_CODEC_IIR_COUNTER_ERR,
    HW_CODEC_IIR_OTHER_ERR,
    HW_CODEC_IIR_ERR_TOTAL,
}HW_CODEC_IIR_ERROR;

typedef enum  {
    HW_CODEC_IIR_NOTYPE = 0,
    HW_CODEC_IIR_DAC,
    HW_CODEC_IIR_ADC,
}HW_CODEC_IIR_TYPE_T;

typedef struct _hw_codec_iir_coefs
{
    int32_t coef_b[3];
    int32_t coef_a[3];
}hw_codec_iir_coefs;

typedef struct _hw_codec_iir_coefs_f
{
    float coef_b[3];
    float coef_a[3];
}hw_codec_iir_coefs_f;

typedef struct _hw_codec_iir_filters
{
    uint16_t iir_bypass_flag;
    uint16_t iir_counter;
    hw_codec_iir_coefs iir_coef[AUD_DAC_IIR_NUM_EQ];
} HW_CODEC_IIR_FILTERS_T;

typedef struct _hw_codec_iir_filters_f
{
    uint16_t iir_bypass_flag;
    uint16_t iir_counter;
    hw_codec_iir_coefs_f iir_coef[AUD_DAC_IIR_NUM_EQ];
} HW_CODEC_IIR_FILTERS_F;

typedef struct _hw_codec_iir_cfg_t
{
    HW_CODEC_IIR_FILTERS_T iir_filtes_l;
    HW_CODEC_IIR_FILTERS_T iir_filtes_r;
} HW_CODEC_IIR_CFG_T;

typedef struct _hw_codec_iir_cfg_f
{
    HW_CODEC_IIR_FILTERS_F iir_filtes_l;
    HW_CODEC_IIR_FILTERS_F iir_filtes_r;
} HW_CODEC_IIR_CFG_F;

int hw_codec_iir_open(enum AUD_SAMPRATE_T sample_rate,  HW_CODEC_IIR_TYPE_T hw_iir_type, int32_t ch_map);
int hw_codec_iir_set_cfg( HW_CODEC_IIR_CFG_T *cfg, enum AUD_SAMPRATE_T sample_rate, HW_CODEC_IIR_TYPE_T hw_iir_type);
int hw_codec_iir_close( HW_CODEC_IIR_TYPE_T hw_iir_type);
HW_CODEC_IIR_CFG_T *hw_codec_iir_get_cfg(enum AUD_SAMPRATE_T sample_rate,const IIR_CFG_T* ext_cfg);
int hw_codec_iir_set_coefs(HW_CODEC_IIR_CFG_F *cfg, HW_CODEC_IIR_TYPE_T hw_iir_type);

int hal_codec_iir_dump(HW_CODEC_IIR_CFG_T *cfg);

#ifdef __cplusplus
}
#endif

#endif
