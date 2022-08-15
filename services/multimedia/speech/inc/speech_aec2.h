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
#ifndef __SPEECH_AEC2_H__
#define __SPEECH_AEC2_H__

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int32_t     bypass;
    int32_t     enEAecEnable;
    int32_t     enHpfEnable;
    int32_t     enAfEnable;
    int32_t     enNsEnable;
    int32_t     enNlpEnable;
    int32_t     enCngEnable;
    int32_t     shwDelayLength;
    float       shwNlpBandSortIdx;
    float       shwNlpBandSortIdxLow;
    float       shwNlpTargetSupp;
    float       shwNlpMinOvrd;
} SpeechAec2Config;

struct SpeechAec2State_;

typedef struct SpeechAec2State_ SpeechAec2State;

SpeechAec2State *speech_aec2_create(int32_t sample_rate, int32_t frame_size, const SpeechAec2Config *cfg);

int32_t speech_aec2_destroy(SpeechAec2State *inst);

int32_t speech_aec2_set_config(SpeechAec2State *st, const SpeechAec2Config *cfg);

int32_t speech_aec2_process(SpeechAec2State *inst, int16_t *pcm_in, int16_t *pcm_ref, int32_t pcm_len);


#ifdef __cplusplus
}
#endif

#endif//__BT_SCO_CHAIN_WEBRTC_H__