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
#ifndef _SPEECH_NOISE_GATE_H_
#define _SPEECH_NOISE_GATE_H_

#include <stdint.h>

typedef struct {
    int32_t     bypass;
    int32_t     data_threshold;
    int32_t     data_shift;
    float       factor_up;
    float       factor_down;
} NoisegateConfig;

struct NoisegateState_;

typedef struct NoisegateState_ NoisegateState;

#ifdef __cplusplus
extern "C" {
#endif

NoisegateState* speech_noise_gate_create(int32_t sample_rate, int32_t frame_size, const NoisegateConfig *cfg);
int32_t speech_noise_gate_destroy(NoisegateState *st);
int32_t speech_noise_gate_set_config(NoisegateState *st, const NoisegateConfig *cfg);

int32_t speech_noise_gate_process(NoisegateState *st, int16_t *pcm_buf, int32_t pcm_len);

#ifdef __cplusplus
}
#endif

#endif
