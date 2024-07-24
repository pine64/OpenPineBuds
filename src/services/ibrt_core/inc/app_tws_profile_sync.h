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
#ifndef __APP_TWS_PROFILE_SYNC__
#define __APP_TWS_PROFILE_SYNC__

#include "spp_api.h"

#define COMMON_DATA_ARRAY_LEN           (164)
#define RFC_MUX_DATA_ARRAY_LEN          (196)
#define HFP_DATA_ARRAY_LEN              (284)
#define A2DP_DATA_ARRAY_LEN             (196)
#define A2DP_CONTINUE_DATA_ARRAY_LEN    (564)
#define AVRCP_DATA_ARRAY_LEN            (612)
#define MAP_DATA_ARRAY_LEN              (256)
#define HID_DATA_ARRAY_LEN              (256)
#define SPP_DATA_ARRAY_LEN              (256)


typedef struct
{
    uint8_t common_data[COMMON_DATA_ARRAY_LEN];
    uint8_t rfc_mux_data[RFC_MUX_DATA_ARRAY_LEN];
    uint8_t hfp_data[HFP_DATA_ARRAY_LEN];
    uint8_t a2dp_data[A2DP_DATA_ARRAY_LEN];
    uint8_t a2dp_continue_data[A2DP_CONTINUE_DATA_ARRAY_LEN];
    uint8_t avrcp_data[AVRCP_DATA_ARRAY_LEN];
    uint8_t map_data[MAP_DATA_ARRAY_LEN];
    uint8_t hid_data[HID_DATA_ARRAY_LEN];
    uint8_t spp_data[SPP_DEVICE_NUM][SPP_DATA_ARRAY_LEN];

    uint16_t common_data_len;
    uint16_t rfc_mux_data_len;
    uint16_t hfp_data_len;
    uint16_t a2dp_data_len;
    uint16_t a2dp_continue_data_len;
    uint16_t avrcp_data_len;
    uint16_t map_data_len;
    uint16_t hid_data_len;
    uint16_t spp_data_len[SPP_DEVICE_NUM];
    uint32_t app_id[SPP_DEVICE_NUM];
    uint8_t  spp_amount;
    uint8_t  spp_num;
} data_store_mem_t;


/*
 * Sync tws master's mobile link profile to tws slave
 */
#define APP_TWS_NUM_BT_DEVICES  2
#define APP_TWS_NUM_KNOWN_DEVICES  10


void app_tws_profile_data_sync(uint8_t *p_buff, uint16_t length);
void app_tws_parse_profile_data(uint8_t *p_buff, uint32_t length);
void app_bt_update_bt_profile_manager(void);
void app_tws_profile_sync_complete_timer_trigger(void);
uint32_t app_tws_profile_data_tx(uint8_t flag,uint8_t *buf);
uint32_t app_tws_profile_data_rx(uint8_t flag,uint8_t *buf,uint32_t length);
void app_tws_profile_data_save_temporarily(uint8_t *p_buff, uint32_t length);
bool app_tws_profile_data_rx_needed(uint8_t profile_flag);
bool app_tws_profile_data_rx_completed(uint8_t final_flag);
void app_tws_profile_rx_parse(void);
uint8_t app_tws_profile_data_tx_filter(uint32_t profile_mask,uint8_t data_array[][2]);
bool app_tws_profile_data_tx_allowed(void);
bool app_tws_profile_connecting(void);
uint32_t app_tws_profile_mapping_data_fragment(uint8_t flag);
void ibrt_app_wait_data_send_to_peer_timeout_handler(void const *param);
void app_tws_profile_resume_a2dp_hfp(void);

#endif

