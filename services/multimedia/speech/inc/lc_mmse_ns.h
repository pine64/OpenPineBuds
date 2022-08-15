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
#ifndef __LC_MMSE_NS_H__
#define __LC_MMSE_NS_H__

#include <stdint.h>

#define LC_MMSE_NS_ENABLE_RES_ECHO

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t     bypass;
    float       denoise_dB;
} SpeechNs2Config;

struct SpeechNs2State_;

typedef struct SpeechNs2State_ SpeechNs2State;

SpeechNs2State *speech_ns2_create(int32_t sample_rate, int32_t frame_size, const SpeechNs2Config *cfg);

int32_t speech_ns2_destroy(SpeechNs2State *st);

int32_t speech_ns2_set_config(SpeechNs2State *st, const SpeechNs2Config *cfg);

int32_t speech_ns2_process(SpeechNs2State *st, int16_t *pcm_buf, int32_t pcm_len);

int32_t speech_ns2_set_echo_state(SpeechNs2State *st, void *echo_state);

int32_t speech_ns2_set_echo_suppress(SpeechNs2State *st, int32_t echo_suppress);

#ifdef __cplusplus
}
#endif

#endif
