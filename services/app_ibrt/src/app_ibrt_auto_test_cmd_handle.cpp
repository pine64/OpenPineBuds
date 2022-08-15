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
#include "apps.h"
#include "app_battery.h"
#include "app_anc.h"
#include "app_key.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui_test.h"
#include "app_ibrt_auto_test.h"
#include "app_ibrt_auto_test_cmd_handle.h"
#include "app_ibrt_ui_test_cmd_if.h"
#include "app_ibrt_peripheral_manager.h"
#include "a2dp_decoder.h"
#include "app_ibrt_keyboard.h"
#include "nvrecord_env.h"
#include "nvrecord_ble.h"
#include "norflash_api.h"
#include "app_tws_if.h"
#include "besbt.h"
#include "app_bt.h"
#include "app_ai_if.h"
#include "app_ai_manager_api.h"
#include "app_ble_uart.h"
#if defined(BISTO_ENABLED)
#include "gsound_custom_actions.h"
#include "gsound_custom_bt.h"
#include "gsound_custom_ble.h"
    
#endif
#ifdef __IAG_BLE_INCLUDE__
#include "app.h"
#include "app_ble_mode_switch.h"
#endif
    
#ifdef BES_OTA
#include "ota_control.h"
#endif

#if defined(IBRT)
#ifdef BES_AUTOMATE_TEST

/*****************************************************************************
 Prototype    : app_ibrt_ui_reboot_test
 Description  : system reboot test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/5/8
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_reboot_test(void)
{
    app_ibrt_peripheral_auto_test_stop();
    app_reset();
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_factory_reset_test
 Description  : Factory reset test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/5/8
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_factory_reset_test(void)
{
    app_ibrt_peripheral_auto_test_stop();
    nv_record_rebuild();
    app_reset();
}

/*****************************************************************************
 Prototype    : app_tws_ibrt_disconnect_tws
 Description  : disconnect tws
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/24
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_disconnect_tws(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    btif_remote_device_t *remdev = NULL;

    if(app_tws_ibrt_tws_link_connected())
    {
        remdev = btif_me_get_remote_device_by_handle(p_ibrt_ctrl->tws_conhandle);

        if (remdev != NULL)
        {
            app_tws_ibrt_disconnect_connection(remdev);
        }
    }

}

/*****************************************************************************
 Prototype    : app_ibrt_ui_auto_test_voice_promt_handle
 Description  : handle voice promt test cmd handle
 Input        : uint8_t operation_code
              : uint8_t *param
              : uint8_t param_len
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2020/3/3
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
static void app_ibrt_ui_auto_test_voice_promt_handle(uint8_t operation_code,
                                                         uint8_t *param,
                                                         uint8_t param_len)
{
    TRACE(2, "%s op_code 0x%x", __func__, operation_code);
    switch (operation_code)
    {
        default:
        break;
    }
}

/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_a2dp_handle
Description  : handle a2dp test cmd handle
Input        : uint8_t operation_code
             : uint8_t *param
             : uint8_t param_len
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/3/3
Author       : bestechnic
Modification : Created function

*****************************************************************************/
static void app_ibrt_ui_auto_test_a2dp_handle(uint8_t operation_code,
                                                uint8_t *param,
                                                uint8_t param_len)
{
    TRACE(2, "%s op_code 0x%x", __func__, operation_code);

    switch (operation_code)
    {
        case A2DP_AUTO_TEST_AUDIO_PLAY:
            app_ibrt_ui_audio_play_test();
        break;
        case A2DP_AUTO_TEST_AUDIO_PAUSE:
            app_ibrt_ui_audio_pause_test();
        break;
        case A2DP_AUTO_TEST_AUDIO_FORWARD:
            app_ibrt_ui_audio_forward_test();
        break;
        case A2DP_AUTO_TEST_AUDIO_BACKWARD:
            app_ibrt_ui_audio_backward_test();
        break;
        case A2DP_AUTO_TEST_AVRCP_VOL_UP:
            app_bt_volumeup();
        break;
        case A2DP_AUTO_TEST_AVRCP_VOL_DOWN:
            app_bt_volumedown();
        break;
        default:
        break;
    }
}

/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_hfp_handle
Description  : handle hfp test cmd handle
Input        : uint8_t operation_code
             : uint8_t *param
             : uint8_t param_len
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/3/3
Author       : bestechnic
Modification : Created function

*****************************************************************************/
static void app_ibrt_ui_auto_test_hfp_handle(uint8_t operation_code,
                                               uint8_t *param,
                                               uint8_t param_len)
{
    TRACE(2, "%s op_code 0x%x", __func__, operation_code);

    switch (operation_code)
    {
        case HFP_AUTO_TEST_SCO_CREATE:
            app_ibrt_ui_hfsco_create_test();
        break;
        case HFP_AUTO_TEST_SCO_DISC:
            app_ibrt_ui_hfsco_disc_test();
        break;
        case HFP_AUTO_TEST_CALL_REDIAL:
            app_ibrt_ui_call_redial_test();
        break;
        case HFP_AUTO_TEST_CALL_ANSWER:
            app_ibrt_ui_call_answer_test();
        break;
        case HFP_AUTO_TEST_CALL_HANGUP:
            app_ibrt_ui_call_hangup_test();
        break;
        case HFP_AUTO_TEST_VOLUME_UP:
            app_ibrt_ui_local_volume_up_test();
        break;
        case HFP_AUTO_TEST_VOLUME_DOWN:
            app_ibrt_ui_local_volume_down_test();
        break;
        default:
        break;
    }
}
/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_ui_handle
Description  : handle UI test cmd handle
Input        : uint8_t operation_code
             : uint8_t *param
             : uint8_t param_len
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/3/3
Author       : bestechnic
Modification : Created function

*****************************************************************************/
static void app_ibrt_ui_auto_test_ui_handle(uint8_t operation_code,
                                             uint8_t *param,
                                             uint8_t param_len)
{
    TRACE(2, "%s op_code 0x%x", __func__, operation_code);

    switch (operation_code)
    {
        case UI_AUTO_TEST_OPEN_BOX:
            app_ibrt_ui_open_box_event_test();
        break;
        case UI_AUTO_TEST_CLOSE_BOX:
            app_ibrt_ui_close_box_event_test();
        break;
        case UI_AUTO_TEST_FETCH_OUT_BOX:
            app_ibrt_ui_fetch_out_box_event_test();
        break;
        case UI_AUTO_TEST_PUT_IN_BOX:
            app_ibrt_ui_put_in_box_event_test();
        break;
        case UI_AUTO_TEST_WEAR_UP:
            app_ibrt_ui_ware_up_event_test();
        break;
        case UI_AUTO_TEST_WEAR_DOWN:
            app_ibrt_ui_ware_down_event_test();
        break;
        case UI_AUTO_TEST_ROLE_SWITCH:
            app_tws_if_trigger_role_switch();
        break;
        case UI_AUTO_TEST_PHONE_CONN_EVENT:
            app_ibrt_ui_phone_connect_event_test();
        break;
        case UI_AUTO_TEST_RECONN_EVENT:
            app_ibrt_ui_reconnect_event_test();
        break;
        case UI_AUTO_TEST_CONN_SECOND_MOBILE:
            app_ibrt_ui_choice_connect_second_mobile();
        break;
        case UI_AUTO_TEST_MOBILE_TWS_DISC:
            app_ibrt_if_disconnect_mobile_tws_link();
        break;
        case UI_AUTO_TEST_PAIRING_MODE:
            app_ibrt_ui_pairing_mode_test();
        break;
        case UI_AUTO_TEST_FREEMAN_MODE:
            app_ibrt_ui_freeman_pairing_mode_test();
        break;
        case UI_AUTO_TEST_SUSPEND_IBRT:
            app_ibrt_ui_suspend_ibrt_test();
        break;
        case UI_AUTO_TEST_RESUME_IBRT:
            app_ibrt_ui_resume_ibrt_test();
        break;
        case UI_AUTO_TEST_SHUT_DOWN:
            app_ibrt_ui_shut_down_test();
        break;
        case UI_AUTO_TEST_REBOOT:
            app_ibrt_ui_reboot_test();
        break;
        case UI_AUTO_TEST_FACTORY_RESET:
            app_ibrt_ui_factory_reset_test();
        break;
        case UI_AUTO_TEST_ASSERT:
            app_ibrt_peripheral_auto_test_stop();
            ASSERT(false, "Force Panic!!!");
        break;
        case UI_AUTO_TEST_CONNECT_MOBILE:
            app_ibrt_if_choice_mobile_connect(0);
        break;
        case UI_AUTO_TEST_DISCONNECT_MOBILE:
            app_tws_ibrt_disconnect_mobile();
        break;
        case UI_AUTO_TEST_CONNECT_TWS:
            app_tws_ibrt_create_tws_connection(app_ibrt_auto_test_get_tws_page_timeout_value());
        break;
        case UI_AUTO_TEST_DISCONNECT_TWS:
            app_ibrt_ui_disconnect_tws();
        break;
        case UI_AUTO_TEST_ENABLE_TPORTS:
            app_ibrt_enable_tports_test();
        break;
        default:
        break;
    }
}

/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_ai_simulate_button_action
Description  : simulate ai button action
Input        : enum APP_KEY_EVENT_T eventCode
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/5/8
Author       : bestechnic
Modification : Created function

*****************************************************************************/
void app_ibrt_ui_auto_test_ai_simulate_button_action(enum APP_KEY_EVENT_T eventCode)
{
    APP_KEY_STATUS keyEvent;
    keyEvent.code = APP_KEY_CODE_GOOGLE;
    keyEvent.event = eventCode;
    app_ibrt_ui_test_voice_assistant_key(&keyEvent, NULL);
}

/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_ai_disconnect
Description  : disconnect AI connection
Input        : None
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/5/8
Author       : bestechnic
Modification : Created function

*****************************************************************************/
void app_ibrt_ui_auto_test_ai_disconnect(void)
{
#ifdef __AI_VOICE__
    //if (app_ai_ble_is_connected() && AI_TRANSPORT_BLE != app_ai_get_transport_type())
    //{
        //app_ai_disconnect_ble();
    //}
    //if (app_ai_spp_is_connected() && AI_TRANSPORT_SPP != app_ai_get_transport_type())
    //{
        //app_ai_spp_disconnlink();
    //}
#endif

#ifdef BISTO_ENABLED
    //gsound_custom_ble_disconnect_ble_link();
    //gsound_custom_bt_disconnect_all_channel();
#endif
}

/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_ai_handle
Description  : handle AI test cmd handle
Input        : uint8_t operation_code
             : uint8_t *param
             : uint8_t param_len
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/3/3
Author       : bestechnic
Modification : Created function

*****************************************************************************/
static void app_ibrt_ui_auto_test_ai_handle(uint8_t operation_code,
                                             uint8_t *param,
                                             uint8_t param_len)
{
    TRACE(2, "%s op_code 0x%x", __func__, operation_code);
    switch (operation_code)
    {
        case AI_AUTO_TEST_PTT_BUTTON_ACTION:
            app_ibrt_ui_auto_test_ai_simulate_button_action((enum APP_KEY_EVENT_T)*param);
        break;
        case AI_AUTO_TEST_DISCONNECT_AI:
            app_ibrt_ui_auto_test_ai_disconnect();
        break;
        default:
        break;
    }
}

typedef struct
{
    uint8_t  scanFilterPolicy;
    uint16_t scanWindowInMs;
    uint16_t scanIntervalInMs;
} APP_IBRT_UI_AUTO_TEST_BLE_SCAN_PARAM_T;

typedef struct
{
    uint8_t  conidx;
    uint32_t min_interval_in_ms;
    uint32_t max_interval_in_ms;
    uint32_t supervision_timeout_in_ms;
    uint8_t  slaveLantency;
} APP_IBRT_UI_AUTO_TEST_BLE_UPDATED_CONN_PARAM_T;

/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_ble_handle
Description  : handle ble test cmd handle
Input        : uint8_t operation_code
             : uint8_t *param
             : uint8_t param_len
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/3/3
Author       : bestechnic
Modification : Created function

*****************************************************************************/
static void app_ibrt_ui_auto_test_ble_handle(uint8_t operation_code,
                                               uint8_t *param,
                                               uint8_t param_len)
{
    TRACE(2, "%s op_code 0x%x", __func__, operation_code);
#ifdef __IAG_BLE_INCLUDE__
    switch (operation_code)
    {
        case BLE_AUTO_TEST_START_ADV:
        {
            uint16_t advIntervalMs = *(uint16_t *)param;
            app_ble_start_connectable_adv(advIntervalMs);
            break;
        }
        case BLE_AUTO_TEST_STOP_ADV:
        {
            app_ble_stop_activities();
            break;
        }
        case BLE_AUTO_TEST_DISCONNECT:
        {
            app_ble_disconnect_all();
            break;
        }
        case BLE_AUTO_TEST_START_CONNECT:
        {
            // six bytes ble address as the parameter
            app_ble_start_connect(param);
            break;
        }
        case BLE_AUTO_TEST_STOP_CONNECT:
        {
            app_ble_stop_activities();
            break;
        }
        case BLE_AUTO_TEST_START_SCAN:
        {
            APP_IBRT_UI_AUTO_TEST_BLE_SCAN_PARAM_T* pScanParam =
                (APP_IBRT_UI_AUTO_TEST_BLE_SCAN_PARAM_T *)param;
            app_ble_start_scan((enum BLE_SCAN_FILTER_POLICY)(pScanParam->scanFilterPolicy),
                pScanParam->scanWindowInMs, pScanParam->scanIntervalInMs);
            break;
        }
        case BLE_AUTO_TEST_STOP_SCAN:
        {
            app_ble_stop_activities();
            break;
        }
        case BLE_AUTO_TEST_UPDATE_CONN_PARAM:
        {
        #ifdef __IAG_BLE_INCLUDE__
            APP_IBRT_UI_AUTO_TEST_BLE_UPDATED_CONN_PARAM_T* pConnParam
             = (APP_IBRT_UI_AUTO_TEST_BLE_UPDATED_CONN_PARAM_T *)param;
            if (app_ble_is_connection_on(pConnParam->conidx))
            {
                l2cap_update_param(pConnParam->conidx,
                    pConnParam->min_interval_in_ms,
                    pConnParam->max_interval_in_ms,
                    pConnParam->supervision_timeout_in_ms,
                    pConnParam->slaveLantency);
            }
        #endif
            break;
        }
        default:
        break;
    }
#endif
}
/*****************************************************************************
Prototype    : app_ibrt_ui_auto_test_flash_handle
Description  : handle flash test cmd handle
Input        : uint8_t operation_code
             : uint8_t *param
             : uint8_t param_len
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/3/3
Author       : bestechnic
Modification : Created function

*****************************************************************************/
static void app_ibrt_ui_auto_test_flash_handle(uint8_t operation_code,
                                                 uint8_t *param,
                                                 uint8_t param_len)
{
    TRACE(2, "%s op_code 0x%x", __func__, operation_code);
    switch (operation_code)
    {
        case FLASH_AUTO_TEST_PROGRAM:
            break;
        case FLASH_AUTO_TEST_ERASE:
            break;
        case FLASH_AUTO_TEST_FLUSH_NV:
        {
            nv_record_flash_flush();
            norflash_flush_all_pending_op();
            break;
        }
        default:
        break;
    }
}

/*****************************************************************************
Prototype    : app_ibrt_ui_automate_test_cmd_handler
Description  : ibrt ui automate test cmd handler
Input        : uint8_t group_code
: uint8_t operation_code
: uint8_t *param
: uint8_t param_len
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2020/3/3
Author       : bestechnic
Modification : Created function

*****************************************************************************/
extern "C" void app_ibrt_ui_automate_test_cmd_handler(uint8_t group_code,
                                                             uint8_t operation_code,
                                                             uint8_t *param,
                                                             uint8_t param_len)
{
    switch (group_code)
    {
        case AUTO_TEST_VOICE_PROMPT:
            app_ibrt_ui_auto_test_voice_promt_handle(operation_code, param, param_len);
            break;
        case AUTO_TEST_A2DP:
            app_ibrt_ui_auto_test_a2dp_handle(operation_code, param, param_len);
            break;
        case AUTO_TEST_HFP:
            app_ibrt_ui_auto_test_hfp_handle(operation_code, param, param_len);
            break;
        case AUTO_TEST_UI:
            app_ibrt_ui_auto_test_ui_handle(operation_code, param, param_len);
            break;
        case AUTO_TEST_AI:
            app_ibrt_ui_auto_test_ai_handle(operation_code, param, param_len);
            break;
        case AUTO_TEST_BLE:
            app_ibrt_ui_auto_test_ble_handle(operation_code, param, param_len);
            break;
        case AUTO_TEST_FLASH:
            app_ibrt_ui_auto_test_flash_handle(operation_code, param, param_len);
            break;
        case AUTO_TEST_UNUSE:
            break;
        default:
            break;
    }
}

#endif
#endif

