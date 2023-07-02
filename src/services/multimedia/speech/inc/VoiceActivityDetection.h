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
#ifndef __VOICEACTIVITYDETECTION_H__
#define __VOICEACTIVITYDETECTION_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float     snrthd;
	float     energythd;
} VADConfig;

struct VADState_;

typedef struct VADState_ VADState;

VADState *VAD_process_state_init(int32_t sample_rate, int32_t frame_size, const VADConfig *cfg);

int32_t VAD_set_config(VADState *st, const VADConfig *cfg);

short VAD_process_run(VADState *st, short *in);

int32_t VAD_destroy(VADState *st);

#ifdef __cplusplus
}
#endif

#endif