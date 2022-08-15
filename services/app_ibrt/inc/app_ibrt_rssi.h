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
#ifndef __APP_IBRT_RSSI_H__
#define __APP_IBRT_RSSI_H__
#if defined(IBRT)

typedef struct
{
    int8_t    fa_gain;
    float     sample_ref;
    int8_t    cur_buf_size_l;
}tota_stutter_t;

void app_ibrt_rssi_get_stutter(uint8_t * data,uint32_t * data_len);
void app_ibrt_debug_parse(uint8_t *data, uint32_t data_len);

#ifdef __cplusplus
extern "C" {
#endif
void app_ibrt_ui_rssi_reset(void);
void app_ibrt_ui_rssi_process(void);
void app_ibrt_ui_check_roleswitch_timer_cb(void const *current_evt);
void app_ibrt_get_peer_mobile_rssi(uint8_t *p_buff, uint16_t length);
void app_ibrt_get_peer_mobile_rssi_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_get_peer_mobile_rssi_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#ifdef __cplusplus
    }
#endif

#endif
#endif
