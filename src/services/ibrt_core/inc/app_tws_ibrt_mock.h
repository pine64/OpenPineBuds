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
#ifndef __APP_TWS_IBRT_MOCK_H__
#define __APP_TWS_IBRT_MOCK_H__

typedef struct {
    uint16_t curr_pkt_sequenceNumber;
    uint32_t curr_pkt_timestamp;
    uint32_t rcv_pkt_cnt;
    uint16_t first_pkt_sequenceNumber;
    uint32_t first_pkt_timestamp;
    uint32_t bt_clk;
    uint32_t bt_cnt;
    uint32_t mobile_clk;
    uint32_t mobile_cnt;
} A2DP_STREAM_MEDIA_INFO_T;

int a2dp_stream_media_info_get(A2DP_STREAM_MEDIA_INFO_T *media_info);
int a2dp_stream_media_info_set(A2DP_STREAM_MEDIA_INFO_T *media_info);

#endif
