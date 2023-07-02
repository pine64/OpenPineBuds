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
#ifndef __APP_TWS_IBRT_CMD_AUDIO_ANALYSIS_H__
#define __APP_TWS_IBRT_CMD_AUDIO_ANALYSIS_H__

void app_ibrt_cmd_send_playback_info(uint8_t *p_buff, uint16_t length);
void app_ibrt_cmd_send_playback_info_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);
void app_tws_ibrt_audio_analysis_send_info_done(uint16_t cmdcode, uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);

void app_ibrt_cmd_set_trigger(uint8_t *p_buff, uint16_t length);
void app_ibrt_cmd_set_trigger_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);

void app_ibrt_cmd_need_retrigger(uint8_t *p_buff, uint16_t length);
void app_ibrt_cmd_need_retrigger_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ibrt_cmd_audio_sync_tune(uint8_t *p_buff, uint16_t length);
void app_ibrt_cmd_audio_sync_tune_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);

void app_ibrt_cmd_audio_sync_set_latencyfactor(uint8_t *p_buff, uint16_t length);
void app_ibrt_cmd_audio_sync_set_latencyfactor_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);

#endif
