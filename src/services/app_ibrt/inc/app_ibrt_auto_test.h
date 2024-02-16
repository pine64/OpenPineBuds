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
#ifndef __APP_IBRT_AUTO_TEST_H__
#define __APP_IBRT_AUTO_TEST_H__
#include <stdint.h>

typedef struct
{
    uint8_t     local_volume;
    uint8_t     a2dp_volume;
    uint8_t     hfp_volume;
    uint8_t     a2dp_streamming;
    uint8_t     a2dp_codec_type;
    uint8_t     a2dp_sample_rate;
    uint8_t     a2dp_sample_bit;
    uint8_t     sco_streaming;
    uint8_t     sco_codec_type;
    uint8_t     call_status;
} __attribute__((__packed__)) AUTO_TEST_BT_STREAM_STATE_T;

typedef struct
{
    uint16_t media_active;
    uint16_t curr_active_media; // low 8 bits are out direciton, while high 8 bits are in direction
    uint8_t  promt_exist; // low 8 bits are out direciton, while high 8 bits are in direction
} __attribute__((__packed__)) AUTO_TEST_MEDIA_STATE_T;

typedef struct
{
    uint32_t    current_ms;
    uint8_t     cpu_freq;
    uint8_t     super_state;
    uint8_t     active_event;
    uint8_t     ibrt_sm_running;
    uint8_t     ui_state;
    uint8_t     mobile_link_bt_role;
    uint8_t     mobile_link_bt_mode;
    uint32_t    mobile_constate;
    uint8_t     tws_role;
    uint8_t     tws_link_bt_role;
    uint8_t     tws_link_bt_mode;
    uint32_t    tws_constate;
    uint8_t     role_switch_state;    // 1:ongoing, 0:idle
    uint8_t     ble_state;
    uint8_t     ble_operation;
    uint8_t     ble_connection_state; // bits are in the order of connection index
} __attribute__((__packed__)) AUTO_TEST_UI_STATE_T;


typedef struct
{
    uint8_t head[3];
    uint8_t length;
    AUTO_TEST_BT_STREAM_STATE_T bt_stream_state;
    AUTO_TEST_MEDIA_STATE_T bt_media_state;
    AUTO_TEST_UI_STATE_T ui_state;
} __attribute__((__packed__)) AUTO_TEST_STATE_T;


extern AUTO_TEST_STATE_T auto_test_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void app_tws_ibrt_disconnect_mobile(void);
uint16_t app_ibrt_auto_test_get_tws_page_timeout_value(void);
void app_ibrt_auto_test_init(void);

#ifdef __cplusplus
}
#endif

#endif

