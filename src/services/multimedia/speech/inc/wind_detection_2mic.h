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
#ifndef __WIND_DETECTION_H__
#define __WIND_DETECTION_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t     bypass;
	float		power_thd;
} WindDetection2MicConfig;

struct WindDetection2MicState_;

typedef struct WindDetection2MicState_ WindDetection2MicState;


WindDetection2MicState *WindDetection2Mic_create(int sample_rate, int sample_bits, int frame_size, const WindDetection2MicConfig *cfg);

void WindDetection2Mic_destroy(WindDetection2MicState *st);

int32_t WindDetection2Mic_set_config(WindDetection2MicState *st, const WindDetection2MicConfig *cfg);

uint8_t wind_state_detect(uint8_t pre_wind_st, float wind_indictor, float *windindicator_cache, float *windthd, uint8_t *cur_wind_st);

float WindDetection2Mic_process_16bit(WindDetection2MicState *st, short *inF, short *inR, uint32_t frame_len, float *wind_power);

float WindDetection2Mic_process_24bit(WindDetection2MicState *st, int *inF, int *inR, uint32_t frame_len, float *wind_power);

#ifdef __cplusplus
}
#endif

#endif
