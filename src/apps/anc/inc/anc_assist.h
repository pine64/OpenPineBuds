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



#ifndef __ANC_ASSIT_H__
#define __ANC_ASSIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

//#define AF_ANC_DUMP_DATA


#if defined(_24BITS_ENABLE)
#define _SAMPLE_BITS        (24)
typedef int   ASSIST_PCM_T;
#else
#define _SAMPLE_BITS        (16)
typedef short   ASSIST_PCM_T;
#endif


#define ANC_ASSIST_FF1_MIC (1<<2)
#define ANC_ASSIST_FF2_MIC (1<<1)
#define ANC_ASSIST_FB_MIC (1<<0)





typedef enum{
    ANC_ASSIST_STANDALONE = 0,
    ANC_ASSIST_MUSIC,
    ANC_ASSIST_PHONE_8K,
    ANC_ASSIST_PHONE_16K,

    ANC_ASSIST_MODE_QTY

}ANC_ASSIST_MODE_T;


void anc_assist_open(ANC_ASSIST_MODE_T mode);

void anc_assist_process(uint8_t * buf, int len);
void anc_assist_close();



#ifdef __cplusplus
}
#endif

#endif