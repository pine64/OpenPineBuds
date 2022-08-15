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
#ifndef IIR_RESAMPLE_H
#define IIR_RESAMPLE_H

#include <stdint.h>

enum IIR_RESAMPLE_MODE
{
    IIR_RESAMPLE_MODE_1TO2 = 0,
    IIR_RESAMPLE_MODE_1TO3,
    IIR_RESAMPLE_MODE_1TO6,
    IIR_RESAMPLE_MODE_2TO1,
    IIR_RESAMPLE_MODE_3TO1,
    IIR_RESAMPLE_MODE_6TO1,
    IIR_RESAMPLE_MODE_NUM,
};

struct IirResampleState_;

typedef struct IirResampleState_ IirResampleState;

#ifdef __cplusplus
extern "C" {
#endif

IirResampleState *iir_resample_init(int frame_size, enum IIR_RESAMPLE_MODE mode);

void iir_resample_destroy(IirResampleState *st);

int iir_resample_needed_out_count(IirResampleState *st, int in_size);

// frame_size is input buffer frame_size
void iir_resample_process(IirResampleState *st, int16_t *in, int16_t *out, int in_size);

enum IIR_RESAMPLE_MODE iir_resample_choose_mode(int sr_in, int sr_out);

#ifdef __cplusplus
}
#endif

#endif