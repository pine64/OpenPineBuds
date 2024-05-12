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
#ifndef __SPEECH_NS_H__
#define __SPEECH_NS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPEECH_NS_SET_AEC_STATE         (0)
#define SPEECH_NS_SET_AEC_SUPPRESS      (1)

typedef struct {
    int32_t     bypass;
    float       denoise_dB;
} SpeechNsConfig;

struct SpeechNsState_;

typedef struct SpeechNsState_ SpeechNsState;

// Creat a instance from speech_ns module/class
// Common value include: sample rate, frame size and so on. 
SpeechNsState *speech_ns_create(int32_t sample_rate, int32_t frame_size, const SpeechNsConfig *cfg);

// Destory a speech ns instance
int32_t speech_ns_destroy(SpeechNsState *st);

// Just use modify instance configure
int32_t speech_ns_set_config(SpeechNsState *st, const SpeechNsConfig *cfg);

// Get/set some value or enable/disable some function
int32_t speech_ns_ctl(SpeechNsState *st, int32_t ctl, void *ptr);

// Process speech stream
int32_t speech_ns_process(SpeechNsState *st, int16_t *pcm_buf, int32_t pcm_len);

// Debug speech_ns instance
int32_t speech_ns_dump(SpeechNsState *st);

#ifdef __cplusplus
}
#endif

#endif
