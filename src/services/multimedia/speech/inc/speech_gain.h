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
#ifndef __SPEECH_GAIN_H__
#define __SPEECH_GAIN_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPEECH_GAIN_SET_AAA          (0)
#define SPEECH_GAIN_GET_AAA          (1)

typedef struct {
    int32_t     bypass;
    float       gain_dB;
} SpeechGainConfig;

struct SpeechGainState_;

typedef struct SpeechGainState_ SpeechGainState;

// Creat a instance from speech_gain module/class
// Common value include: sample rate, frame size and so on. 
SpeechGainState *speech_gain_create(int32_t sample_rate, int32_t frame_size, const SpeechGainConfig *cfg);

// Destory a speech gain instance
int32_t speech_gain_destroy(SpeechGainState *st);

// Just use modify instance configure
int32_t speech_gain_set_config(SpeechGainState *st, const SpeechGainConfig *cfg);

// Get/set some value or enable/disable some function
// int32_t speech_gain_ctl(SpeechGainState *st, int32_t ctl, void *ptr);

// Process speech stream
int32_t speech_gain_process(SpeechGainState *st, int16_t *pcm_buf, int32_t pcm_len);

// Debug speech_gain instance
int32_t speech_gain_dump(SpeechGainState *st);

float speech_gain_get_required_mips(SpeechGainState *st);

#ifdef __cplusplus
}
#endif

#endif
