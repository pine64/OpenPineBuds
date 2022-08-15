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
#ifndef __DSD_PROCESS_H__
#define __DSD_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "hal_aud.h"

#define FIR_COEF_NUM                        384

typedef struct {
    int8_t  gain0;
    int8_t  gain1;
    int16_t len;
    int32_t coef[FIR_COEF_NUM];
} DSD_CFG_T;

int dsd_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits,enum AUD_CHANNEL_NUM_T ch_num, void *eq_buf, uint32_t len);
int dsd_process(uint8_t *buf, uint32_t len);
int dsd_close(void);

#ifdef __cplusplus
}
#endif

#endif
