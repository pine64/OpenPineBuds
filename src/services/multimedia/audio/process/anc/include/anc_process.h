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
#ifndef __ANC_PROCESS_H__
#define __ANC_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "aud_section.h"
#include "hal_aud.h"

typedef enum {
    ANC_NO_ERR=0,
    ANC_TYPE_ERR,
    ANC_GAIN_ERR,
    ANC_OTHER_ERR,
    ANC_ERR_TOTAL,
}ANC_ERROR;

typedef enum {
    ANC_GAIN_NO_DELAY=0,
    ANC_GAIN_DELAY,
    ANC_GAIN_TOTAL,
}ANC_GAIN_TIME;


typedef struct _aud_fir_item
{
    int32_t    fir_bypass_flag;
    int32_t    fir_len;
    int32_t    fir_coef[AUD_COEF_LEN];
} aud_fir_item;

typedef struct _struct_anc_fir_cfg
{
    aud_fir_item anc_fir_cfg_ff_l;
    aud_fir_item anc_fir_cfg_ff_r;
    aud_fir_item anc_fir_cfg_fb_l;
    aud_fir_item anc_fir_cfg_fb_r;
    aud_fir_item anc_fir_cfg_tt_l;
    aud_fir_item anc_fir_cfg_tt_r;
    aud_fir_item anc_fir_cfg_mc_l;
    aud_fir_item anc_fir_cfg_mc_r;
} struct_anc_fir_cfg;

int anc_set_fir_cfg( struct_anc_fir_cfg * cfg, enum ANC_TYPE_T anc_type);

int anc_opened(enum ANC_TYPE_T anc_type);
int anc_open(enum ANC_TYPE_T anc_type);
void anc_close(enum ANC_TYPE_T anc_type);

int anc_disable(void);
int anc_enable(void);

int anc_set_cfg(const struct_anc_cfg * cfg,enum ANC_TYPE_T anc_type,ANC_GAIN_TIME anc_gain_delay);
int anc_set_gain(int32_t gain_ch_l, int32_t gain_ch_r,enum ANC_TYPE_T anc_type);
int anc_get_gain(int32_t *gain_ch_l, int32_t *gain_ch_r,enum ANC_TYPE_T anc_type);
int anc_get_cfg_gain(int32_t *gain_ch_l, int32_t *gain_ch_r,enum ANC_TYPE_T anc_type);




//void AncPduHardwarCh0(const short Coef[], short CoefLen);
//void AncPduHardwarCh1(const short Coef[], short CoefLen);
void AncPduHardwarCh0(const short Coef[], short CoefLen,short gain);
void AncPduHardwarCh1(const short Coef[], short CoefLen,short gain);

int anc_select_coef(enum AUD_SAMPRATE_T rate,enum ANC_INDEX index,enum ANC_TYPE_T anc_type,ANC_GAIN_TIME anc_gain_delay);
int anc_load_cfg(void);
int anc_cmd_receve_process(uint8_t *buf,uint32_t len);
int anc_cmd_send_process(uint8_t **pbuf,uint16_t *len);

enum ANC_INDEX anc_get_current_coef_index(void);
enum AUD_SAMPRATE_T anc_get_current_coef_samplerate(void);
int anc_mc_run_stereo(uint8_t *buf, int len,float left_gain,float right_gain,enum AUD_BITS_T sample_bit);
int anc_mc_run_mono(uint8_t *buf, int len,float left_gain,enum AUD_BITS_T sample_bit);

void anc_mc_run_init(enum AUD_SAMPRATE_T rate);
void anc_mc_run_setup(enum AUD_SAMPRATE_T rate);

void anc_disable_gain_updated_when_pass0(uint8_t on);
void  anc_set_ch_map( int32_t ch_map );
void  anc_fb_check_set(int32_t flag );
void anc_fb_check_irq_handler(uint32_t status);
void anc_fb_check_param(void);
void anc_fb_check_stop_timer(void);

#ifdef __cplusplus
}
#endif

#endif
