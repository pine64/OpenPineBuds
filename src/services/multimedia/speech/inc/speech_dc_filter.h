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
#ifndef __SPEECH_DC_FILTER_H__
#define __SPEECH_DC_FILTER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPEECH_DC_FILTER_GAIN_Q   (10)

#define SPEECH_DC_FILTER_SET_CHANNEL_NUM            (0)
#define SPEECH_DC_FILTER_SET_DATA_SEPARATION        (1)

typedef struct {
    int32_t     bypass;
    float       gain;
} SpeechDcFilterConfig;

struct SpeechDcFilterState_;

typedef struct SpeechDcFilterState_ SpeechDcFilterState;

// Creat a instance from speech_dc_filter module/class
// Common value include: sample rate, frame size and so on. 
SpeechDcFilterState *speech_dc_filter_create(int32_t sample_rate, const SpeechDcFilterConfig *cfg);

// Destory a speech dc_filter instance
int32_t speech_dc_filter_destroy(SpeechDcFilterState *st);

// Just use modify instance configure
int32_t speech_dc_filter_set_config(SpeechDcFilterState *st, const SpeechDcFilterConfig *cfg);

// Get/set some value or enable/disable some function
int32_t speech_dc_filter_ctl(SpeechDcFilterState *st, int32_t ctl, void *ptr);

// Process speech stream
int32_t speech_dc_filter_process(SpeechDcFilterState *st, int16_t *pcm_buf, int32_t pcm_len);

int32_t speech_dc_filter_process_int24(SpeechDcFilterState *st, int32_t *pcm_buf, int32_t pcm_len);

// Debug speech_dc_filter instance
int32_t speech_dc_filter_dump(SpeechDcFilterState *st);

float speech_dc_filter_get_required_mips(SpeechDcFilterState *st);

#ifdef __cplusplus
}
#endif

#endif
