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
#include <string.h>
#include "app_tws_ibrt_trace.h"
#include "factory_section.h"
#include "hal_timer.h"
#include "apps.h"
#include "app_battery.h"
#include "app_anc.h"
#include "app_a2dp.h"
#include "app_key.h"
#include "app_ibrt_auto_test.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui.h"
#include "app_ibrt_ui_test.h"
#include "app_ibrt_peripheral_manager.h"
#include "a2dp_decoder.h"
#include "app_ibrt_keyboard.h"
#include "nvrecord_env.h"
#include "nvrecord_ble.h"
#include "app_tws_if.h"
#include "besbt.h"
#include "app_bt.h"
#include "app_ai_if.h"
#include "app_ai_manager_api.h"
#include "app_bt_media_manager.h"
#include "app_audio.h"
#include "app_ble_mode_switch.h"

#if defined(IBRT)
#include "btapp.h"
extern struct BT_DEVICE_T  app_bt_device;
extern int hfp_volume_get(enum BT_DEVICE_ID_T id);
int bt_sco_player_get_codetype(void);

AUTO_TEST_STATE_T auto_test_state_t = {0};


uint16_t app_ibrt_auto_test_get_tws_page_timeout_value(void)
{
    return app_ibrt_ui_get_tws_page_timeout_value();
}

#ifdef BES_AUTOMATE_TEST
void app_ibrt_auto_test_print_earphone_state(void const *n)
{
    app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    auto_test_state_t.head[0] = 0xE1;
    auto_test_state_t.head[1] = 0x38;
    auto_test_state_t.head[2] = 0x7C;
    auto_test_state_t.length = sizeof(auto_test_state_t);

    //a2dp & hfp state
    auto_test_state_t.bt_stream_state.local_volume = app_bt_stream_local_volume_get();
    auto_test_state_t.bt_stream_state.a2dp_volume = a2dp_volume_get(BT_DEVICE_ID_1);
    auto_test_state_t.bt_stream_state.hfp_volume = hfp_volume_get(BT_DEVICE_ID_1);
    auto_test_state_t.bt_stream_state.a2dp_streamming = a2dp_is_music_ongoing();
    auto_test_state_t.bt_stream_state.a2dp_codec_type = app_bt_device.codec_type[BT_DEVICE_ID_1];
    auto_test_state_t.bt_stream_state.a2dp_sample_rate = app_bt_device.sample_rate[BT_DEVICE_ID_1];
    auto_test_state_t.bt_stream_state.a2dp_sample_bit = app_bt_device.sample_bit[BT_DEVICE_ID_1];

    auto_test_state_t.bt_stream_state.sco_streaming = is_sco_mode();
    auto_test_state_t.bt_stream_state.sco_codec_type = (uint8_t)bt_sco_player_get_codetype();
    app_ibrt_if_get_hfp_call_status((AppIbrtCallStatus *)&auto_test_state_t.bt_stream_state.call_status);

    //media state
    auto_test_state_t.bt_media_state.media_active = bt_media_get_media_active(BT_DEVICE_ID_1);
    auto_test_state_t.bt_media_state.curr_active_media = bt_media_get_current_media();
    auto_test_state_t.bt_media_state.promt_exist = app_audio_list_playback_exist();

    // ui state
    auto_test_state_t.ui_state.current_ms = TICKS_TO_MS(hal_sys_timer_get());
    auto_test_state_t.ui_state.cpu_freq = hal_sysfreq_get();
    auto_test_state_t.ui_state.super_state = p_ibrt_ui->super_state;
    auto_test_state_t.ui_state.active_event = p_ibrt_ui->active_event;
    auto_test_state_t.ui_state.ibrt_sm_running = p_ibrt_ui->ibrt_sm_running;
    auto_test_state_t.ui_state.ui_state = p_ibrt_ui->box_state;

    if (IBRT_SLAVE != p_ibrt_ctrl->current_role)
    {
        auto_test_state_t.ui_state.mobile_link_bt_role = app_tws_ibrt_get_local_mobile_role();
        auto_test_state_t.ui_state.mobile_link_bt_mode = p_ibrt_ctrl->mobile_mode;
        auto_test_state_t.ui_state.mobile_constate = p_ibrt_ctrl->mobile_constate;
        //auto_test_state_t.ui_state.mobile_connect = app_tws_ibrt_mobile_link_connected();
    }
    auto_test_state_t.ui_state.tws_role = p_ibrt_ctrl->current_role;
    auto_test_state_t.ui_state.tws_link_bt_role = app_tws_ibrt_get_local_tws_role();
    auto_test_state_t.ui_state.tws_link_bt_mode = p_ibrt_ctrl->tws_mode;
    auto_test_state_t.ui_state.tws_constate = p_ibrt_ctrl->tws_constate;
    auto_test_state_t.ui_state.role_switch_state = 
                (p_ibrt_ctrl->slave_tws_switch_pending ||
                p_ibrt_ctrl->master_tws_switch_pending);
#ifdef __IAG_BLE_INCLUDE__
    auto_test_state_t.ui_state.ble_state =  app_ble_get_current_state();
    auto_test_state_t.ui_state.ble_operation = app_ble_get_current_operation();
    auto_test_state_t.ui_state.ble_connection_state = 0;
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (app_ble_is_connection_on(i))
        {
            auto_test_state_t.ui_state.ble_connection_state |= 
                (1 << i);
        }
    }
#endif

    //TRACE(1, "earphone len %d state :", auto_test_state_t.length);
    DUMP8("%02x", (uint8_t *)&auto_test_state_t, auto_test_state_t.length);
}

#define APP_AUTO_TEST_PRINT_STATE_TIME_IN_MS 500
osTimerDef (APP_AUTO_TEST_PRINT_STATE_TIMEOUT, app_ibrt_auto_test_print_earphone_state);
osTimerId   app_auto_test_print_state_timeout_timer_id = NULL;
#endif

void app_ibrt_auto_test_init(void)
{
#ifdef BES_AUTOMATE_TEST
    memset((uint8_t *)&auto_test_state_t, 0, sizeof(auto_test_state_t));
    if (app_auto_test_print_state_timeout_timer_id == NULL)
    {
        app_auto_test_print_state_timeout_timer_id = osTimerCreate(osTimer(APP_AUTO_TEST_PRINT_STATE_TIMEOUT), osTimerPeriodic, NULL);
        osTimerStart(app_auto_test_print_state_timeout_timer_id, APP_AUTO_TEST_PRINT_STATE_TIME_IN_MS);
    }
#endif
}

void app_ibrt_auto_test_inform_cmd_received(uint8_t group_code,
        uint8_t operation_code)
{
    AUTO_TEST_TRACE(2, "AUTO_TEST_CMD received:%d:%d:", group_code, operation_code);
}

#endif

