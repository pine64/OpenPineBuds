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
#ifndef __AUDIO_PROCESS_H__
#define __AUDIO_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "iir_process.h"
#include "fir_process.h"

typedef enum {
    AUDIO_EQ_TYPE_SW_IIR = 0,
    AUDIO_EQ_TYPE_HW_FIR,
    AUDIO_EQ_TYPE_HW_DAC_IIR,
    AUDIO_EQ_TYPE_HW_IIR,
} AUDIO_EQ_TYPE_T;

int audio_process_init(void);
int audio_process_open(enum AUD_SAMPRATE_T sample_rate, enum AUD_BITS_T sample_bits,enum AUD_CHANNEL_NUM_T sw_ch_num, enum AUD_CHANNEL_NUM_T hw_ch_num,int32_t frame_size, void *eq_buf, uint32_t len);
int audio_process_run(uint8_t *buf, uint32_t len);
int audio_process_close(void);

int audio_eq_set_cfg(const FIR_CFG_T *fir_cfg,const IIR_CFG_T *iir_cfg,AUDIO_EQ_TYPE_T audio_eq_type);

#ifdef USB_EQ_TUNING
void audio_eq_usb_eq_update (void);
#endif

#ifdef __cplusplus
}
#endif

#endif
