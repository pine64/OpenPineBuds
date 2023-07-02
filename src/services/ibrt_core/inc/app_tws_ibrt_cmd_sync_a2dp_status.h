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
#ifndef __APP_TWS_IBRT_CMD_SYNC_A2DP_STATUS_H__
#define __APP_TWS_IBRT_CMD_SYNC_A2DP_STATUS_H__
#include "app_tws_ibrt.h"
#include <stdint.h>

typedef struct {
  ibrt_codec_t codec;
  uint8_t volume;
  btif_avdtp_stream_state_t state;
  float latency_factor;
  uint32_t session;
} __attribute__((packed)) ibrt_a2dp_status_t;

bool app_ibrt_sync_a2dp_status_onporcess(void);
void app_ibrt_sync_a2dp_status_set(bool status);

void app_ibrt_sync_a2dp_status(void);
void app_ibrt_sync_a2dp_send_status(uint8_t *p_buff, uint16_t length);
void app_ibrt_sync_a2dp_send_status_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                            uint16_t length);
void app_ibrt_sync_a2dp_send_status_rsp_timeout_handler(uint16_t rsp_seq,
                                                        uint8_t *p_buff,
                                                        uint16_t length);
void app_ibrt_sync_a2dp_send_status_rsp_handler(uint16_t rsp_seq,
                                                uint8_t *p_buff,
                                                uint16_t length);

#endif
