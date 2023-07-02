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
#ifndef __AUDIO_DRC2_H__
#define __AUDIO_DRC2_H__

#include <stdint.h>

#define MAX_LOOK_AHEAD_TIME (10)

typedef struct {
    int knee;
    int look_ahead_time;
    int threshold;
    float makeup_gain;
    int ratio;
    int attack_time;
    int release_time;
} DRC2_CFG_T;

#ifdef __cplusplus
extern "C" {
#endif

void audio_drc2_open(int sample_rate, int sample_bits, int ch_num);
void audio_drc2_close(void);
void audio_drc2_process(uint8_t *buf_p_l, uint8_t *buf_p_r, uint32_t pcm_len);
void audio_drc2_set_cfg(const DRC2_CFG_T *cfg);


#ifdef __cplusplus
}
#endif

#endif
