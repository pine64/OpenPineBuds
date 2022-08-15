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
#ifndef __SENSORMIC_DENOISE_H__
#define __SENSORMIC_DENOISE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t     bypass;
	int32_t     blend_en;
    // float       left_gain;              // MIC Left增益补偿
    // float       right_gain;             // MIC Right增益补偿
    int32_t     delay_tapsM;            // MIC L/R delay samples. 0: 适用于麦克距离为<2cm; 1: 适用于麦克距离为2cm左右; 2: 适用于麦克距离为4cm左右
    int32_t     delay_tapsS;
    // int32_t     delay_tapsC;
    // float       coefH[2][5];            // {{a0,a1,a2,a3,a4},{b0,b1,b2,b3,b4}}
    // float       coefL[2][5];            // {{a0,a1,a2,a3,a4},{b0,b1,b2,b3,b4}}
    int32_t     crossover_freq;
    float       *ff_fb_coeff;
    int         ff_fb_coeff_len;
} SensorMicDenoiseConfig;

struct SensorMicDenoiseState_;

typedef struct SensorMicDenoiseState_ SensorMicDenoiseState;

//SensorMicDenoiseState *sensormic_denoise_create(int32_t sample_rate, int32_t frame_size, const SensorMicDenoiseConfig *cfg);
SensorMicDenoiseState *sensormic_denoise_create(int32_t sample_rate, int32_t frame_size, int32_t fft_size,int32_t ps_size,const SensorMicDenoiseConfig *cfg);

int32_t sensormic_denoise_trace(SensorMicDenoiseState *st);

int32_t sensormic_denoise_destroy(SensorMicDenoiseState *st);

int32_t sensormic_denoise_set_config(SensorMicDenoiseState *st, const SensorMicDenoiseConfig *cfg);

int32_t sensormic_denoise_process(SensorMicDenoiseState *st, short *pcm_buf, int32_t pcm_len, short *out_buf);

bool sensormic_denoise_get_vad(SensorMicDenoiseState *st);

void sensormic_denoise_set_anc_status(SensorMicDenoiseState *st, bool anc_enable);

float sensormic_denoise_get_required_mips(SensorMicDenoiseState *st);

#ifdef __cplusplus
}
#endif

#endif
