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


#ifndef __AF_ANC_H__
#define __AF_ANC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_aud.h"

//#define AF_ANC_DUMP_DATA


#if defined(_24BITS_ENABLE)
#define _SAMPLE_BITS        (24)
typedef int   ASSIST_PCM_T;
#else
#define _SAMPLE_BITS        (16)
typedef short   ASSIST_PCM_T;
#endif


typedef enum{
    AF_ANC_OFF = 0,
    AF_ANC_ON,
}AF_ANC_STATUS_T;

typedef enum{
    AF_ANC_MUSIC = 0,
    AF_ANC_STANDALONE,
}AF_ANC_MODE_T;


typedef struct LeakageDetectionState_ LeakageDetectionState;
LeakageDetectionState *LeakageDetection_create(int frame_size,int delay0);
void LeakageDetection_destroy(LeakageDetectionState *st);
int LeakageDetection_process(LeakageDetectionState *leak_st,AF_ANC_STATUS_T adapflag,ASSIST_PCM_T *fb_buf,ASSIST_PCM_T *ref_buf,int pcm_len);
void LeakageDetection_adjust_delay(LeakageDetectionState *st, int delay0);

void get_pilot_data(uint8_t *buf,int len);





typedef struct ANCAssistMulti_ ANCAssistMultiState;

ANCAssistMultiState * ANCAssistMulti_create(int sample_rate, int frame_size,int fftsize);
void ANCAssistMulti_process(ANCAssistMultiState *st, ASSIST_PCM_T *inF, ASSIST_PCM_T *inB,ASSIST_PCM_T *inR,int frame_len);
void ANCAssistMulti_destroy(ANCAssistMultiState * st);




//need to implement
void anc_assist_change_curve(int curve_id);
// void audio_adpt_status_set_anc_mode(uint8_t mode, bool init);
// void audio_engine_set_anc_gain(int32_t gain_ch_l, int32_t gain_ch_r, int type);
// int af_anc_sync_wind_status(uint8_t status);
bool audio_engine_tt_is_on();
// bool anc_usb_app_get_status();
// void audio_adpt_status_set_wind_status(int state);
void anc_assist_set_anc_gain(float gain_ch_l, float gain_ch_r,enum ANC_TYPE_T anc_type);


extern bool app_anc_work_status();
#ifdef __cplusplus
}
#endif

#endif