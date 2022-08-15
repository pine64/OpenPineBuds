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
#ifndef __SPEECH_PEAK_DETECTOR_H__
#define __SPEECH_PEAK_DETECTOR_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// return_type
#define SPEECH_PEAK_DETECTOR_RETURN_MEAN            (0)
#define SPEECH_PEAK_DETECTOR_RETURN_ARRAY           (1)

typedef struct {
    int     return_type;
    float   factor_up;
    float   factor_down;
} SPEECH_PEAK_DETECTOR_CFG_T;

typedef void *SPEECH_PEAK_DETECTOR_STATE_T;

// Creat a instance from speech_peak_detector module/class
// Common value include: sample rate, frame size and so on. 
SPEECH_PEAK_DETECTOR_STATE_T *speech_peak_detector_create(SPEECH_PEAK_DETECTOR_CFG_T *cfg, int frame_size, int sample_rate);

// Destory a speech peak_detector instance
int speech_peak_detector_destroy(SPEECH_PEAK_DETECTOR_STATE_T *inst);

// Just use modify instance configure
int speech_peak_detector_set_config(SPEECH_PEAK_DETECTOR_STATE_T *inst, SPEECH_PEAK_DETECTOR_CFG_T *cfg);

// Enable some function
int speech_peak_detector_ctl(SPEECH_PEAK_DETECTOR_STATE_T *inst, int ctl, void *ptr);

// Process speech stream
int speech_peak_detector_process(SPEECH_PEAK_DETECTOR_STATE_T *inst, int16_t *pcm_buf, int32_t pcm_len, float *out);

// Debug speech_peak_detector instance
int speech_peak_detector_dump(SPEECH_PEAK_DETECTOR_STATE_T *inst);

#ifdef __cplusplus
}
#endif

#endif
