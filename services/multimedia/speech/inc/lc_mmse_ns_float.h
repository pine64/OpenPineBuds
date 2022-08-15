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
#ifndef __LC_MMSE_NS_FLOAT_H__
#define __LC_MMSE_NS_FLOAT_H__

// #define LC_MMSE_NS_ENABLE_RES_ECHO

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t     bypass;
    float       denoise_dB;
    int32_t     banks;
} SpeechNs2FloatConfig;

struct SpeechNs2FloatState_;

typedef struct SpeechNs2FloatState_ SpeechNs2FloatState;

SpeechNs2FloatState *speech_ns2float_create(int32_t sample_rate, int32_t frame_size, bool cp, const SpeechNs2FloatConfig *cfg);

int32_t speech_ns2float_destroy(SpeechNs2FloatState *st);

int32_t speech_ns2float_set_config(SpeechNs2FloatState *st, const SpeechNs2FloatConfig *cfg, bool cp);

int32_t speech_ns2float_process(SpeechNs2FloatState *st, int16_t *pcm_buf, int32_t pcm_len);

int32_t speech_ns2float_set_echo_state(SpeechNs2FloatState *st, void *echo_state);

int32_t speech_ns2float_set_echo_suppress(SpeechNs2FloatState *st, int32_t echo_suppress);

void speech_ns2float_set_external_vad(SpeechNs2FloatState *st, bool vad);

float speech_ns2float_get_required_mips(SpeechNs2FloatState *st);

#ifdef __cplusplus
}
#endif

#endif
