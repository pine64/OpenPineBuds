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
#include "bt_if.h"
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
#include "me_api.h"
#include "app_tws_if.h"
#include "besbt.h"
#include "btapp.h"
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

#define IBRT_ENHANCED_STACK_PTS
static bt_bdaddr_t pts_bt_addr = {{
#if 0
        0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00
#else
        0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00
#endif
    }
};
void btif_pts_hf_create_link_with_pts(void)
{
    btif_pts_hf_create_service_link(&pts_bt_addr);
}
void btif_pts_av_create_channel_with_pts(void)
{
    btif_pts_av_create_channel(&pts_bt_addr);
}
void btif_pts_ar_connect_with_pts(void)
{
    btif_pts_ar_connect(&pts_bt_addr);
}

void app_ibrt_ui_audio_play_test(void)
{
    uint8_t action[] = {IBRT_ACTION_PLAY};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_audio_pause_test(void)
{
    uint8_t action[] = {IBRT_ACTION_PAUSE};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_audio_forward_test(void)
{
    uint8_t action[] = {IBRT_ACTION_FORWARD};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_audio_backward_test(void)
{
    uint8_t action[] = {IBRT_ACTION_BACKWARD};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_avrcp_volume_up_test(void)
{
    uint8_t action[] = {IBRT_ACTION_AVRCP_VOLUP};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_avrcp_volume_down_test(void)
{
    uint8_t action[] = {IBRT_ACTION_AVRCP_VOLDN};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_hfsco_create_test(void)
{
    uint8_t action[] = {IBRT_ACTION_HFSCO_CREATE};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_hfsco_disc_test(void)
{
    uint8_t action[] = {IBRT_ACTION_HFSCO_DISC};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_call_redial_test(void)
{
    uint8_t action[] = {IBRT_ACTION_REDIAL};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_call_answer_test(void)
{
    uint8_t action[] = {IBRT_ACTION_ANSWER};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_call_hangup_test(void)
{
    uint8_t action[] = {IBRT_ACTION_HANGUP};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_local_volume_up_test(void)
{
    uint8_t action[] = {IBRT_ACTION_LOCAL_VOLUP};
    app_ibrt_if_start_user_action(action, sizeof(action));
}
void app_ibrt_ui_local_volume_down_test(void)
{
    uint8_t action[] = {IBRT_ACTION_LOCAL_VOLDN};
    app_ibrt_if_start_user_action(action, sizeof(action));
}

void app_ibrt_ui_get_tws_conn_state_test(void)
{
    if(app_tws_ibrt_tws_link_connected())
    {
        TRACE(0,"ibrt_ui_log:TWS CONNECTED");
    }
    else
    {
        TRACE(0,"ibrt_ui_log:TWS DISCONNECTED");
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_soft_reset_test
 Description  : soft reset device
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/7/7
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_soft_reset_test(void)
{
    app_reset();
}


extern void bt_change_to_iic(APP_KEY_STATUS *status, void *param);
void app_ibrt_ui_iic_uart_switch_test(void)
{
    bt_change_to_iic(NULL,NULL);
}

void app_ibrt_ui_get_a2dp_state_test(void)
{
   const char* a2dp_state_strings[] = {"IDLE", "CODEC_CONFIGURED", "OPEN","STREAMING","CLOSED","ABORTING"};
   AppIbrtA2dpState a2dp_state;

   AppIbrtStatus  status = app_ibrt_if_get_a2dp_state(&a2dp_state);
   if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE(0,"ibrt_ui_log:a2dp_state=%s",a2dp_state_strings[a2dp_state]);
    }
    else
    {
        TRACE(0,"ibrt_ui_log:get a2dp state error");
    }
}

void app_ibrt_ui_get_avrcp_state_test(void)
{
    const char* avrcp_state_strings[] = {"DISCONNECTED",  "CONNECTED", "PLAYING","PAUSED", "VOLUME_UPDATED"};
    AppIbrtAvrcpState avrcp_state;

    AppIbrtStatus status = app_ibrt_if_get_avrcp_state(&avrcp_state);
    if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE(0,"ibrt_ui_log:avrcp_state=%s",avrcp_state_strings[avrcp_state]);
    }
    else
    {
        TRACE(0,"ibrt_ui_log:get avrcp state error");
    }
}

void app_ibrt_ui_get_hfp_state_test(void)
{
    const char* hfp_state_strings[] = {"SLC_DISCONNECTED",  "CLOSED", "SCO_CLOSED","PENDING","SLC_OPEN", \
                            "NEGOTIATE","CODEC_CONFIGURED","SCO_OPEN","INCOMING_CALL","OUTGOING_CALL","RING_INDICATION"};
    AppIbrtHfpState hfp_state;
    AppIbrtStatus status = app_ibrt_if_get_hfp_state(&hfp_state);

    if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE(0,"ibrt_ui_log:hfp_state=%s",hfp_state_strings[hfp_state]);
    }
    else
    {
        TRACE(0,"ibrt_ui_log:get hfp state error");
    }
}

void app_ibrt_ui_get_call_status_test()
{
    const char* call_status_strings[] = {"NO_CALL","CALL_ACTIVE","HOLD","INCOMMING","OUTGOING","ALERT"};
    AppIbrtCallStatus call_status;

    AppIbrtStatus status = app_ibrt_if_get_hfp_call_status(&call_status);
    if(APP_IBRT_IF_STATUS_SUCCESS == status)
    {
        TRACE(0,"ibrt_ui_log:call_status=%s",call_status_strings[call_status]);
    }
    else
    {
        TRACE(0,"ibrt_ui_log:get call status error");
    }
}

void app_ibrt_ui_get_ibrt_role_test(void)
{
    ibrt_role_e role = app_ibrt_if_get_ibrt_role();

    if(IBRT_MASTER == role)
    {
        TRACE(0,"ibrt_ui_log:ibrt role is MASTER");
    }
    else if(IBRT_SLAVE == role)
    {
        TRACE(0,"ibrt_ui_log:ibrt role is SLAVE");
    }
    else
    {
        TRACE(0,"ibrt_ui_log:ibrt role is UNKNOW");
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_open_box_event_test
 Description  : app ibrt ui open box test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_open_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_OPEN_BOX_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_fetch_out_box_event_test
 Description  : fetch out box test function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_fetch_out_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_FETCH_OUT_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_put_in_box_event_test
 Description  : app ibrt put in box test function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_put_in_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_PUT_IN_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_close_box_event_test
 Description  : app ibrt close box test function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_close_box_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_CLOSE_BOX_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_reconnect_event_test
 Description  : app ibrt reconnect function test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_reconnect_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_RECONNECT_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_ware_up_event_test
 Description  : app ibrt wear up function test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_ware_up_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_WEAR_UP_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_ware_down_event_test
 Description  : app ibrt wear down function test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_ware_down_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_WEAR_DOWN_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_phone_connect_event_test
 Description  : app ibrt ui phone connect event
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/3
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_phone_connect_event_test(void)
{
    app_ibrt_if_event_entry(IBRT_PHONE_CONNECT_EVENT);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_shut_down_test
 Description  : shut down test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/10
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
extern "C" int app_shutdown(void);
void app_ibrt_ui_shut_down_test(void)
{
    app_ibrt_peripheral_auto_test_stop();
    app_shutdown();
}

/*****************************************************************************
 Prototype    : app_ibrt_enable_tports_test
 Description  : enable BT tports
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/10
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
extern void bt_enable_tports(void);
void app_ibrt_enable_tports_test(void)
{
    bt_enable_tports();
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_tws_swtich_test
 Description  : tws switch test
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/10
 Author       : bestechnic
 Modification : Created function
 
 *****************************************************************************/
 void app_ibrt_ui_tws_swtich_test(void)
 {
     ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
 
     if (p_ibrt_ctrl->current_role == IBRT_MASTER)
     {
         btif_me_ibrt_role_switch(p_ibrt_ctrl->mobile_conhandle);
     }
     else
     {
         TRACE(0,"ibrt_ui_log:local role is ibrt_slave, pls send tws switch in another dev");
     }
 }
 
 /*****************************************************************************
  Prototype    : app_ibrt_ui_suspend_ibrt_test
  Description  : suspend ibrt fastack
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
 void app_ibrt_ui_suspend_ibrt_test(void)
 {
     ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
 
     if (p_ibrt_ctrl->current_role== IBRT_MASTER)
     {
         btif_me_suspend_ibrt();
     }
     else
     {
         TRACE(0,"ibrt_ui_log:local role is ibrt_slave, suspend ibrt failed");
     }
 }
 /*****************************************************************************
  Prototype    : app_ibrt_ui_resume_ibrt_test
  Description  : resume ibrt test
  Input        : void
  Output       : None
  Return Value :
  Calls        :
  Called By    :
 
  History        :
  Date         : 2019/4/27
  Author       : bestechnic
  Modification : Created function
 
 *****************************************************************************/
 void app_ibrt_ui_resume_ibrt_test(void)
 {
     btif_me_resume_ibrt(1);
 }
 /*****************************************************************************
  Prototype    : app_ibrt_ui_pairing_mode_test
  Description  : pairing mode test
  Input        : void
  Output       : None
  Return Value :
  Calls        :
  Called By    :
 
  History        :
  Date         : 2019/11/20
  Author       : bestechnic
  Modification : Created function
 
 *****************************************************************************/
 void app_ibrt_ui_pairing_mode_test(void)
 {
     app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
 }
 /*****************************************************************************
  Prototype    : app_ibrt_ui_freeman_pairing_mode_test
  Description  : ibrt freeman pairing mode test
  Input        : void
  Output       : None
  Return Value :
  Calls        :
  Called By    :
 
  History        :
  Date         : 2019/11/21
  Author       : bestechnic
  Modification : Created function
 
 *****************************************************************************/
 void app_ibrt_ui_freeman_pairing_mode_test(void)
 {
     app_ibrt_ui_event_entry(IBRT_FREEMAN_PAIRING_EVENT);
 }
 
 void app_ibrt_inquiry_start_test(void)
 {
     app_bt_start_search();
 }

void app_ibrt_role_switch_test(void)
{
    app_tws_if_trigger_role_switch();
}

