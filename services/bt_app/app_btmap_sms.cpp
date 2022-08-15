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
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_audio.h"
#include "app_battery.h"
#include "app_status_ind.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif
#include "hfp_api.h"
#include "besbt.h"
#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "bt_if.h"
#include "app_hfp.h"
#include "app_fp_rfcomm.h"
#include "os_api.h"
#include "app_bt_func.h"
#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#include "bt_drv_interface.h"

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif
#include "app.h"
#ifdef __AI_VOICE__
#include "ai_control.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_sync_hfp_status.h"
#include "app_ibrt_hf.h"
#include "app_tws_ibrt.h"
#endif

#if defined(__BTMAP_ENABLE__)
#include "obex_api.h"
#include "map_api.h"
#include "app_btmap_sms.h"

extern struct BT_DEVICE_T app_bt_device;
struct SMS_MSG_T app_sms_msg;

#define CHACK_BTMAP_STATE_STAYING_MS    2000
osTimerId app_btmap_state_check_handling_timer_id = NULL;
static int app_btmap_state_check_handling_handler(void const *param);
osTimerDef (APP_BTMAP_STATE_CHECK_HANDLING_TIMER, (void (*)(void const *))app_btmap_state_check_handling_handler);


static int btmap_sms_session_event_handler(btif_map_session_handle_t handle,
    btif_map_session_event_t event, btif_map_session_cb_param_t *param)
{
    TRACE(1,"%s", __func__);
    btif_map_session_function_param_t func_param;
    switch (event) {
        case BTIF_MAP_SESSION_EVENT_OPEN:
        func_param.p.SetFolder.folder[0] = "root";
        func_param.p.SetFolder.folder[1] = "telecom";
        func_param.p.SetFolder.folder[2] = "msg";
        func_param.p.SetFolder.folder[3] = NULL;
        func_param.p.SetFolder.up_level = 0;
        btif_map_set_folder(handle, &func_param);
        break;
        case BTIF_MAP_SESSION_EVENT_CLOSE:
        break;
        default:
        break;
    }
    return 0;
}

void app_btmap_connected_callback(void* param, void* map_session)
{
    TRACE(1,"%s", __func__);
#if defined(IBRT)
    app_tws_ibrt_profile_callback(BTIF_APP_MAP_PROFILE_ID,(void *)param,(void *)map_session);
#endif
}

void app_btmap_sms_init(void)
{
    TRACE(1,"%s", __func__);
    int i = 0;
    btif_map_initialize();
    btif_map_callback_register(app_btmap_connected_callback);
    // only map mas client supported now
    for (i = 0; i < BT_DEVICE_NUM; ++i) {
        app_bt_device.map_session_handle[i] = btif_map_create_session();
    }
}

void app_btmap_sms_open(BT_DEVICE_ID_T id, bt_bdaddr_t *remote)
{
    TRACE(1,"%s", __func__);
    btif_map_session_config_t config;
    btif_map_session_handle_t handle = NULL;
    handle = app_bt_device.map_session_handle[id];

    if (handle == NULL) {
        TRACE(2,"%s:handle is NULL, id=%d", __func__, id);
        return;
    }

    TRACE(2,"btmap_sms_open:id=%d,remote=0x%p", id, remote);
    DUMP8("0x%02x-", remote, 6);

    config.type = BTIF_MAP_SESSION_TYPE_MAS;
    config.obex_role = BTIF_OBEX_SESSION_ROLE_CLIENT;
    btif_map_session_open(handle, remote, &config, btmap_sms_session_event_handler);
}

void app_btmap_sms_close(BT_DEVICE_ID_T id)
{
    TRACE(1,"%s", __func__);
    btif_map_session_handle_t handle = NULL;
    handle = app_bt_device.map_session_handle[id];

    if (handle == NULL) {
        TRACE(2,"%s:handle is NULL, id=%d", __func__, id);
        return;
    }

    btif_map_session_close(handle);
}

void app_btmap_sms_send(BT_DEVICE_ID_T id, char* telNum, char* msg)
{
    btif_map_session_handle_t handle = NULL;
    handle = app_bt_device.map_session_handle[id];

    if (handle == NULL) {
        TRACE(2,"%s:handle is NULL, id=%d", __func__, id);
        return;
    }

    btif_map_send_sms(handle, telNum, msg);
}

bool app_btmap_check_is_connected(BT_DEVICE_ID_T id)
{
    TRACE(1,"%s", __func__);
    btif_map_session_handle_t handle = NULL;
    handle = app_bt_device.map_session_handle[id];

    return btif_map_check_is_connected(handle);
}

bool app_btmap_check_is_idle(BT_DEVICE_ID_T id)
{
    TRACE(1,"%s", __func__);
    btif_map_session_handle_t handle = NULL;
    handle = app_bt_device.map_session_handle[id];

    return btif_map_check_is_idle(handle);
}


void app_btmap_sms_save(BT_DEVICE_ID_T id, char* telNum, char* msg)
{
    TRACE(1,"%s", __func__);
    app_sms_msg.telNum = telNum;
    app_sms_msg.msg = msg;
    app_sms_msg.telNumLen = strlen(telNum);
    app_sms_msg.msgLen = strlen(msg);
    
    if (NULL == app_btmap_state_check_handling_timer_id)
    {
        app_btmap_state_check_handling_timer_id = 
        osTimerCreate(osTimer(APP_BTMAP_STATE_CHECK_HANDLING_TIMER), osTimerOnce, NULL);
    }

    osTimerStart(app_btmap_state_check_handling_timer_id, CHACK_BTMAP_STATE_STAYING_MS);
}

void app_btmap_sms_resend(void)
{
    TRACE(1,"%s", __func__);

    app_btmap_sms_send(BT_DEVICE_ID_1, app_sms_msg.telNum, app_sms_msg.msg);
    memset((void *)app_sms_msg.telNum, 0, app_sms_msg.telNumLen);
    memset((void *)app_sms_msg.msg, 0, app_sms_msg.msgLen);
}
static uint8_t resendTime = 0;
static int app_btmap_state_check_handling_handler(void const *param)
{
    TRACE(1,"%s", __func__);
    resendTime++;
    if(resendTime >= 6)
    {
        resendTime = 0;
        return -1;
    }

    if(true == app_btmap_check_is_connected(BT_DEVICE_ID_1))
    {
        app_btmap_sms_resend();
    }
    else
    {
        osTimerStart(app_btmap_state_check_handling_timer_id, CHACK_BTMAP_STATE_STAYING_MS);
    }
    return 0;
}

#endif /* __BTMAP_ENABLE__ */
