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
#ifndef __SubBandAec_H__
#define __SubBandAec_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int bypass;
    int filter_size;
} SubBandAecConfig;

struct SubBandAecState_;

typedef struct SubBandAecState_ SubBandAecState;

SubBandAecState *SubBandAec_init(int sample_rate, int frame_size, const SubBandAecConfig *cfg);

void SubBandAec_destroy(SubBandAecState *st);

int32_t SubBandAec_process(SubBandAecState *st, short *datain, short *echoref, short *dataout, short Len);

float SubBandAec_get_required_mips(SubBandAecState *st);

#ifdef __cplusplus
}
#endif

#endif

