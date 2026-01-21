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
#ifndef __APP_TWS_IBRT_AUDIO_SYNC_H__
#define __APP_TWS_IBRT_AUDIO_SYNC_H__

#define TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE (1.0f - 80*1e-6)
//#define TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE     (1.0f)
#define TWS_IBRT_AUDIO_SYNC_FACTOR_FAST_LIMIT    ( 0.0005f)
#define TWS_IBRT_AUDIO_SYNC_FACTOR_SLOW_LIMIT    (-0.0005f)

#define TWS_IBRT_AUDIO_SYNC_DEAD_ZONE_US     (5)

#define APP_TWS_IBRT_AUDIO_SYNC_MISMATCH_FLAG_NULL          (0)
#define APP_TWS_IBRT_AUDIO_SYNC_MISMATCH_FLAG_NEEDRETRIGGER (1<<1)

#define APP_TWS_IBRT_AUDIO_SYNC_SAMPLEING_FLAG_NULL     (0)
#define APP_TWS_IBRT_AUDIO_SYNC_SAMPLEING_FLAG_DETECTER (1<<1)

#ifdef __A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__
#define APP_TWS_IBRT_AUDIO_SYNC_TUNE_SKIP_NEXT_CNT      (50)
#else
#define APP_TWS_IBRT_AUDIO_SYNC_TUNE_SKIP_NEXT_CNT      (200)
#endif

typedef enum {
    APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC,
    APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_RESUME,
    APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_LOCAL,
}APP_TWS_IBRT_AUDIO_TRIGGER_TYPE;

typedef struct {    
    uint32_t timestamp;
    uint16_t sequenceNumber;
    uint8_t curSubSequenceNumber;
    uint8_t totalSubSequenceNumber;
    uint32_t frame_samples;
} APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_PLAYBACK_INFO_T;

typedef struct
{
    uint32_t trigger_time;
    uint8_t trigger_skip_frame;
    APP_TWS_IBRT_AUDIO_TRIGGER_TYPE trigger_type;
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_PLAYBACK_INFO_T audio_info;
    uint16_t sequenceNumberStart;
    float factor_reference;
    uint8_t a2dp_session;
    uint32_t handler_cnt;
    uint32_t trigger_bt_clk;
    uint16_t trigger_bt_cnt;
#ifdef __SW_TRIG__
    uint16_t tg_bitcnt;
#endif
}APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T;

typedef struct
{
    uint32_t reseved;
}APP_TWS_IBRT_AUDIO_NEED_RETRIGGER_T;

typedef struct
{
    float factor_reference;
    uint32_t handler_cnt;
}APP_TWS_IBRT_AUDIO_SYNC_TUNE_T;

typedef struct
{
    APP_TWS_IBRT_AUDIO_SYNC_TUNE_T sync_tune;
    uint32_t new_reference_flag;
}APP_TWS_IBRT_AUDIO_SYNC_TUNE_REQ_T;

typedef enum{
    APP_TWS_IBRT_AUDIO_SYNC_MISMATCH_RESUME_REQ_DISABLE,
    APP_TWS_IBRT_AUDIO_SYNC_MISMATCH_RESUME_REQ_NULL,
    APP_TWS_IBRT_AUDIO_SYNC_MISMATCH_RESUME_REQ_REQUEST,
    APP_TWS_IBRT_AUDIO_SYNC_MISMATCH_RESUME_REQ_WAITRESUME,
}APP_TWS_IBRT_AUDIO_SYNC_MISMATCH_RESUME_REQ;

typedef struct{
    float factor_reference;
    float factor_fast_limit;
    float factor_slow_limit;
    uint32_t dead_zone_us;  
}APP_TWS_IBRT_AUDIO_SYNC_CFG_T;

float app_tws_ibrt_audio_sync_config_factor_reference_get(void);

int app_tws_ibrt_audio_sync_sampleing_set_flag(uint32_t flag);

int app_tws_ibrt_audio_sync_sampleing_get_flag(uint32_t *flag);

int app_tws_ibrt_audio_sync_new_reference(float factor_reference);

int app_tws_ibrt_audio_sync_reconfig(APP_TWS_IBRT_AUDIO_SYNC_CFG_T *config);

bool app_tws_ibrt_audio_sync_playback_info_initdone(void);

int app_tws_ibrt_audio_sync_init(void);

int app_tws_ibrt_audio_sync_start(void);

int app_tws_ibrt_audio_sync_restart(void);

int app_tws_ibrt_audio_sync_stop(void);

int app_tws_ibrt_audio_sync_tick(uint32_t tick);

bool app_tws_ibrt_audio_sync_sampleing(float sample);

int app_tws_ibrt_audio_sync_mismatch_tick(bool mismatch);

int app_tws_ibrt_audio_sync_mismatch_resume_handle(uint32_t tick);

int app_tws_ibrt_audio_sync_mismatch_resume_notify(void);

int app_tws_ibrt_audio_sync_tune_request(APP_TWS_IBRT_AUDIO_SYNC_TUNE_REQ_T *sync_tune);

bool app_tws_ibrt_audio_sync_tune_onprocess(void);

int app_tws_ibrt_audio_sync_tune_handle(uint32_t tick);

int app_tws_ibrt_audio_sync_tune_cancel(void);

int app_tws_ibrt_audio_sync_tune_skip_next_cnt_proc(void);

int app_tws_ibrt_audio_sync_tune_need_skip(void);

void app_tws_ibrt_audio_sync_tune_skip_next_cnt(int32_t bypass_next_cnt);

APP_TWS_IBRT_AUDIO_SYNC_CFG_T *app_tws_ibrt_audio_sync_config_default_get(void);

#endif
