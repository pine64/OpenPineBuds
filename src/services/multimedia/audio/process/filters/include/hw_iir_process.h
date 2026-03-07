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
#ifndef __HW_IIR_PROCESS_H__
#define __HW_IIR_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "hal_aud.h"
#include "iir_process.h"

#define AUD_IIR_NUM_EQ                        (8)

typedef enum {
    HW_IIR_NO_ERR=0,
    HW_IIR_TYPE_ERR,
    HW_IIR_SAMPLERATE_ERR,
    HW_IIR_COUNTER_ERR,
    HW_IIR_OTHER_ERR,
    HW_IIR_ERR_TOTAL,
}HW_IIR_ERROR;

typedef struct _hw_iir_coefs_t
{
    int32_t coef_b[3];
    int32_t coef_a[3];
}HW_IIR_COEFS_T;

typedef struct _hw_iir_filters_t
{
    uint16_t iir_counter;
    HW_IIR_COEFS_T iir_coef[AUD_IIR_NUM_EQ];
} HW_IIR_FILTERS_T;

typedef struct _hw_iir_cfg_t
{
    HW_IIR_FILTERS_T iir_filtes_l;
    HW_IIR_FILTERS_T iir_filtes_r;
} HW_IIR_CFG_T;

int hw_iir_needed_size(enum AUD_BITS_T sample_bits, int32_t frame_size);
int hw_iir_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits,enum AUD_CHANNEL_NUM_T ch_num, void *eq_buf, uint32_t len);
int hw_iir_set_cfg(HW_IIR_CFG_T *cfg);
int hw_iir_run(uint8_t *buf, uint32_t len);
int hw_iir_close(void);
HW_IIR_CFG_T *hw_iir_get_cfg(enum AUD_SAMPRATE_T sample_rate, const IIR_CFG_T* ext_cfg);

#ifdef __cplusplus
}
#endif

#endif
