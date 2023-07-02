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
#ifndef __AUDIO_RESAMPLER_EX_H__
#define __AUDIO_RESAMPLER_EX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_aud.h"

enum RESAMPLE_STATUS_T {
    RESAMPLE_STATUS_OK = 0,
    RESAMPLE_STATUS_ERROR,
    RESAMPLE_STATUS_NO_COEF,
    RESAMPLE_STATUS_NO_COEF_GROUP,
    RESAMPLE_STATUS_BAD_COEF_NUM,
    RESAMPLE_STATUS_BAD_FACTOR,
    RESAMPLE_STATUS_NO_BUF,
    RESAMPLE_STATUS_BUF_MISALIGN,
    RESAMPLE_STATUS_BUF_TOO_SMALL,
    RESAMPLE_STATUS_BAD_ID,

    RESAMPLE_STATUS_OUT_FULL,
    RESAMPLE_STATUS_IN_EMPTY,
    RESAMPLE_STATUS_DONE,
};

struct RESAMPLE_COEF_T {
    uint16_t upsample_factor;
    uint16_t downsample_factor;
    uint8_t phase_coef_num;
    uint16_t total_coef_num;
    const int16_t *coef_group;
};

struct RESAMPLE_CFG_T {
    enum AUD_CHANNEL_NUM_T chans;
    enum AUD_BITS_T bits;
    float ratio_step;
    const struct RESAMPLE_COEF_T *coef;
    void *buf;
    uint32_t size;
};

struct RESAMPLE_IO_BUF_T {
    const void *in;
    uint32_t in_size;
    void *out;
    uint32_t out_size;
    // For cyclic output buffer
    void *out_cyclic_start;
    void *out_cyclic_end;
};

typedef void *RESAMPLE_ID;

uint32_t audio_resample_ex_get_buffer_size(enum AUD_CHANNEL_NUM_T chans, enum AUD_BITS_T bits, uint8_t phase_coef_num);
enum RESAMPLE_STATUS_T audio_resample_ex_open(const struct RESAMPLE_CFG_T *cfg, RESAMPLE_ID *id_ptr);
enum RESAMPLE_STATUS_T audio_resample_ex_run(RESAMPLE_ID id, const struct RESAMPLE_IO_BUF_T *io, uint32_t *in_size_ptr, uint32_t *out_size_ptr);
void audio_resample_ex_close(RESAMPLE_ID id);
void audio_resample_ex_flush(RESAMPLE_ID id);

enum RESAMPLE_STATUS_T audio_resample_ex_set_ratio_step(RESAMPLE_ID id, float ratio_step);
enum RESAMPLE_STATUS_T audio_resample_ex_get_ratio_step(RESAMPLE_ID id, float *ratio_step);

#ifdef __cplusplus
}
#endif

#endif
