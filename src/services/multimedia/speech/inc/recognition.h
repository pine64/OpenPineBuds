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
#ifndef __RECOGNITION_H__
#define __RECOGNITION_H__

#include "vad.h"
#include "mfcc.h"

#define VOICE_RECOGNITION_NN (120)
#define VOICE_RECOGNITION_OVLP (60)
#define VOICE_RECOGNITION_FFTFRAME (240)
#define VOICE_RECOGNITION_MODE_NUM_MAX (8)

typedef struct{
    vad_t vad;    
    mfcc_t mfcc;
    mfcc_vct_t mfcc_vct;
    int tag;   
    short in_tmp[VOICE_RECOGNITION_NN * 2 + VOICE_RECOGNITION_OVLP];
    int dis_thr;
    mfcc_vct_t *mode[VOICE_RECOGNITION_MODE_NUM_MAX];
    unsigned char mode_num;
} mfcc_extract_t;

int speech_recognition_process(mfcc_extract_t *mfcc_exec, short *buf, unsigned int len);

#endif // __RECOGNITION_H__

