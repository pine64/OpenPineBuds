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
#ifndef __SPEECH_IIR_H__
#define __SPEECH_IIR_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPEECH_IIR_COEFF_LEN    (5)

typedef float SPEECH_RX_EQ_PCOEFFS_T[SPEECH_IIR_COEFF_LEN];

typedef struct {
    int32_t numStages;

    // {
    //     {b10, b11, b12, a11, a12},
    //     {b20, b21, b22, a21, a22},
    //     ...
    // }    
    // const float   (*pCoeffs)[SPEECH_IIR_COEFF_LEN];
    SPEECH_RX_EQ_PCOEFFS_T *pCoeffs;
} SPEECH_IIR_CFG_T;

typedef void SPEECH_IIR_STATE_T;

// Initial speech_iir module/class
int speech_iir_init(void);

// Deinitial speech_iir module/calss
int speech_iir_deinit(void);

// Creat a instance from speech_iir module/class
// Common value include: sample rate, frame size and so on. 
SPEECH_IIR_STATE_T *speech_iir_create(const SPEECH_IIR_CFG_T *cfg);

// Destory a speech iir instance
int speech_iir_destroy(SPEECH_IIR_STATE_T *inst);

// Just use modify instance configure
int speech_iir_set_config(SPEECH_IIR_STATE_T *inst, const SPEECH_IIR_CFG_T *cfg);

// Process speech stream
int speech_iir_process(SPEECH_IIR_STATE_T *inst, int16_t *pcm_buf, int32_t pcm_len);

// Debug speech_iir instance
int speech_iir_dump(SPEECH_IIR_STATE_T *inst);

#ifdef __cplusplus
}
#endif

#endif
