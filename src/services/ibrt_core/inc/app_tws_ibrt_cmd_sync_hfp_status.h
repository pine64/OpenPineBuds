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
#ifndef __APP_TWS_IBRT_CMD_SYNC_HFP_STATUS_H__
#define __APP_TWS_IBRT_CMD_SYNC_HFP_STATUS_H__

typedef struct
{
    uint8_t audio_state;
    uint8_t volume;
    uint8_t lmp_sco_hdl;
} __attribute__((packed)) ibrt_hfp_status_t;

void app_ibrt_sync_hfp_status(void);

void app_ibrt_sync_hfp_send_status(uint8_t *p_buff, uint16_t length);

void app_ibrt_sync_hfp_send_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ibrt_sync_hfp_send_status_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ibrt_sync_hfp_send_status_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ibrt_sync_hfp_status_done(uint16_t cmdcode, uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);

#endif
