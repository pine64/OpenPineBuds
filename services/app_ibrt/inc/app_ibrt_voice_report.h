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
#ifndef __APP_IBRT_VOICE_REPORT_H__
#define __APP_IBRT_VOICE_REPORT_H__

typedef struct
{
    uint32_t aud_id;
    uint32_t aud_pram;
    uint32_t tg_tick;
    uint32_t vol;
} __attribute__((packed)) app_ibrt_voice_report_info_t;

typedef struct
{
    uint32_t aud_id;
    uint32_t aud_pram;
    uint32_t vol;
} __attribute__((packed)) app_ibrt_voice_report_request_t;

#ifdef __cplusplus
extern "C" {
#endif

int app_ibrt_voice_report_trigger_checker(void);
int app_ibrt_voice_report_trigger_init(uint32_t aud_id, uint32_t aud_pram);
int app_ibrt_voice_report_trigger_deinit(void);
void app_ibrt_send_voice_report_request_req(uint8_t *p_buff, uint16_t length);
void app_ibrt_voice_report_request_req_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_voice_report_start_req(uint8_t *p_buff, uint16_t length);
void app_ibrt_voice_report_request_start_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
int app_ibrt_if_voice_report_handler(uint32_t aud_id, uint16_t aud_pram);
bool app_ibrt_voice_report_is_me(uint32_t voice_chnlsel);

#ifdef __cplusplus
}
#endif

#endif
