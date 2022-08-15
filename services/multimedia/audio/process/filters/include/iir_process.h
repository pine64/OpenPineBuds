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
#ifndef __IIR_PROCESS_H__
#define __IIR_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "hal_aud.h"

#ifdef USB_EQ_TUNING

#ifdef __SW_IIR_EQ_PROCESS__
#define SW_IIR_PARAM_NUM        (2)
#else
#define SW_IIR_PARAM_NUM        (0)
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
#define HW_DAC_IIR_PARAM_NUM    (8)
#else
#define HW_DAC_IIR_PARAM_NUM    (0)
#endif

#define IIR_PARAM_NUM  (SW_IIR_PARAM_NUM + HW_DAC_IIR_PARAM_NUM)

#else

#define IIR_PARAM_NUM           (20)

#endif

typedef enum {
    IIR_TYPE_LOW_SHELF = 0,
    IIR_TYPE_PEAK,
    IIR_TYPE_HIGH_SHELF,
    IIR_TYPE_LOW_PASS,
    IIR_TYPE_HIGH_PASS,
    IIR_TYPE_NUM
} IIR_TYPE_T;

typedef struct {
    IIR_TYPE_T  type;
    float       gain;
    float       fc;
    float       Q;
} IIR_PARAM_T;

typedef struct {
    float   gain0;
    float   gain1;
    int     num;
    IIR_PARAM_T param[IIR_PARAM_NUM];
} IIR_CFG_T;

int iir_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits,enum AUD_CHANNEL_NUM_T ch_num);
int iir_set_cfg(const IIR_CFG_T *cfg);
int iir_set_cfg_ch(const IIR_CFG_T *cfg, enum AUD_CHANNEL_NUM_T ch);
int iir_run(uint8_t *buf, uint32_t len);
int iir_close(void);

int iir_set_gain_and_num(float gain0, float gain1, int num);
int iir_set_param(uint8_t index, const IIR_PARAM_T *param);

void iir_update_cfg_tbl(uint8_t *buf, uint32_t  len);

#ifdef __cplusplus
}
#endif

#endif
