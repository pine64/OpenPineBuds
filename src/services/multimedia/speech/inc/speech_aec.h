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
#ifndef __SPEECH_AEC_H__
#define __SPEECH_AEC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPEECH_AEC_GET_LIB_ST          (0)

/*
60
16383
4
*/
typedef struct {
    int32_t     bypass;
    int32_t     delay;
    int32_t     leak_estimate;
    int32_t     leak_estimate_shift;
} SpeechAecConfig;

struct SpeechAecState_;

typedef struct SpeechAecState_ SpeechAecState;

// Creat a instance from speech_aec module/class
// Common value include: sample rate, frame size and so on. 
SpeechAecState *speech_aec_create(int32_t sample_rate, int32_t frame_size, const SpeechAecConfig *cfg);

// Destory a speech aec instance
int32_t speech_aec_destroy(SpeechAecState *st);

// Just use modify instance configure
int32_t speech_aec_set_config(SpeechAecState *st, const SpeechAecConfig *cfg);

// Get/set some value or enable/disable some function
int32_t speech_aec_ctl(SpeechAecState *st, int32_t ctl, void *ptr);

// Process speech stream
int32_t speech_aec_process(SpeechAecState *st, int16_t *pcm_in, int16_t *pcm_ref, int32_t pcm_len, int16_t *pcm_out);

// Debug speech_aec instance
int32_t speech_aec_dump(SpeechAecState *st);

#ifdef __cplusplus
}
#endif

#endif
