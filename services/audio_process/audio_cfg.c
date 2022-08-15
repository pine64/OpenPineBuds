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
#include "audio_cfg.h"
#include "iir_process.h"
#include "drc.h"
#include "limiter.h"
#include "aud_section.h"
#include "string.h"
#include "hal_trace.h"


#define TOOL_SUPPORT_MAX_IIR_EQ_BAND_NUM    (20)

struct AUDIO_CFG_T_{
    IIR_CFG_T       iir_eq;       // Now just support one type
    IIR_PARAM_T     reserved[TOOL_SUPPORT_MAX_IIR_EQ_BAND_NUM - IIR_PARAM_NUM];
    DrcConfig       drc;
    LimiterConfig   limiter;
};

typedef struct {
    uint8_t         reserved[AUDIO_SECTION_CFG_RESERVED_LEN];
    AUDIO_CFG_T     cfg;
} AUDIO_SECTION_AUDIO_CFG_T;

static AUDIO_SECTION_AUDIO_CFG_T audio_section_audio_cfg;

// Can not use sizeof(AUDIO_CFG_T), so define this function
int sizeof_audio_cfg(void)
{
    return sizeof(AUDIO_CFG_T);
}

int store_audio_cfg_into_audio_section(AUDIO_CFG_T *cfg)
{
    int res = 0;

    memcpy(&audio_section_audio_cfg.cfg, cfg, sizeof(AUDIO_CFG_T));

    res = audio_section_store_cfg(AUDIO_SECTION_DEVICE_AUDIO,
                                (uint8_t *)&audio_section_audio_cfg,
                                sizeof(AUDIO_SECTION_AUDIO_CFG_T));


    if(res)
    {
        TRACE(2,"[%s] ERROR: res = %d", __func__, res);
    }
    else
    {
        TRACE(1,"[%s] Store audio cfg into audio section!!!", __func__); 
    }

    return res;
}

void *load_audio_cfg_from_audio_section(enum AUDIO_PROCESS_TYPE_T type)
{
    ASSERT(type < AUDIO_PROCESS_TYPE_NUM, "[%s] type(%d) is invalid", __func__, type);
    int res = 0;
    res = audio_section_load_cfg(AUDIO_SECTION_DEVICE_AUDIO,
                                (uint8_t *)&audio_section_audio_cfg,
                                sizeof(AUDIO_SECTION_AUDIO_CFG_T));

    void *res_ptr = NULL;

    if (res)
    {
        TRACE(2,"[%s] ERROR: res = %d", __func__, res);
        res_ptr = NULL;  
    }
    else
    {
        if (type == AUDIO_PROCESS_TYPE_IIR_EQ)
        {
            TRACE(1,"[%s] Load iir_eq from audio section!!!", __func__);
            res_ptr = (void *)&audio_section_audio_cfg.cfg.iir_eq;
        }
        else if (type == AUDIO_PROCESS_TYPE_DRC)
        {
            TRACE(1,"[%s] Load drc from audio section!!!", __func__);
            res_ptr = (void *)&audio_section_audio_cfg.cfg.drc;
        }
        else if (type == AUDIO_PROCESS_TYPE_LIMITER)
        {
            TRACE(1,"[%s] Load limiter from audio section!!!", __func__);
            res_ptr = (void *)&audio_section_audio_cfg.cfg.limiter;
        }
        else 
        {
            TRACE(2,"[%s] ERROR: Invalid type(%d)", __func__, type);
            res_ptr = NULL;    
        }
    }

    return res_ptr;
}
