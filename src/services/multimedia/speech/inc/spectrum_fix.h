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
#ifndef SPECTRUM_FIX_H
#define SPECTRUM_FIX_H

#include <stdint.h>

#define MAX_FREQ_NUM (10)

typedef struct
{
    int freq_num;
    int freq_list[MAX_FREQ_NUM];
} SpectrumFixConfig;

struct SpectrumFixState_;

typedef struct SpectrumFixState_ SpectrumFixState;

#ifdef __cplusplus
extern "C" {
#endif

SpectrumFixState *spectrum_fix_init(int sample_rate, int frame_size, const SpectrumFixConfig *config);

void spectrum_fix_destroy(SpectrumFixState *st);

/* Update input buffer, must be called every block */
void spectrum_fix_analysis(SpectrumFixState *st, const int16_t *x);

/* Calculate spectrum, should be called when needed */
void spectrum_fix_process(SpectrumFixState *st, int16_t *spectrum, int spectrum_num);

#ifdef __cplusplus
}
#endif

#endif