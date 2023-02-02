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
#include "app_ibrt_ui_test.h"
#include "a2dp_decoder.h"
#include "app.h"
#include "app_ai_if.h"
#include "app_ai_manager_api.h"
#include "app_anc.h"
#include "app_battery.h"
#include "app_ble_mode_switch.h"
#include "app_bt.h"
#include "app_ibrt_auto_test.h"
#include "app_ibrt_if.h"
#include "app_ibrt_keyboard.h"
#include "app_ibrt_peripheral_manager.h"
#include "app_ibrt_ui_test_cmd_if.h"
#include "app_key.h"
#include "app_tws_ibrt_trace.h"
#include "app_tws_if.h"
#include "apps.h"
#include "besbt.h"
#include "btapp.h"
#include "factory_section.h"
#include "norflash_api.h"
#include "nvrecord_ble.h"
#include "nvrecord_env.h"
#include "nvrecord_extension.h"
#include <string.h>
#if defined(BISTO_ENABLED)
#include "gsound_custom_actions.h"
#include "gsound_custom_ble.h"
#include "gsound_custom_bt.h"
#endif

#ifdef __AI_VOICE__
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_ble.h"
#endif

#ifdef IBRT_OTA
#include "ota_control.h"
#endif

void app_voice_assistant_key(APP_KEY_STATUS *status, void *param);
extern void app_bt_volumedown();
extern void app_bt_volumeup();
#ifdef IBRT_OTA
extern uint8_t ota_role_switch_flag;
extern uint8_t avoid_even_packets_protect_flag;
#endif
#if defined(IBRT)
#include "btapp.h"
extern struct BT_DEVICE_T app_bt_device;

bt_bdaddr_t master_ble_addr = {0x76, 0x33, 0x33, 0x22, 0x11, 0x11};
bt_bdaddr_t slave_ble_addr = {0x77, 0x33, 0x33, 0x22, 0x11, 0x11};
bt_bdaddr_t box_ble_addr = {0x78, 0x33, 0x33, 0x22, 0x11, 0x11};

#ifdef IBRT_SEARCH_UI
void app_ibrt_battery_callback(APP_BATTERY_MV_T currvolt, uint8_t currlevel,
                               enum APP_BATTERY_STATUS_T curstatus,
                               uint32_t status,
                               union APP_BATTERY_MSG_PRAMS prams);
void app_ibrt_simulate_charger_plug_in_test(void) {
  union APP_BATTERY_MSG_PRAMS msg_prams;
  msg_prams.charger = APP_BATTERY_CHARGER_PLUGIN;
  app_ibrt_battery_callback(0, 0, APP_BATTERY_STATUS_CHARGING, 1, msg_prams);
}
void app_ibrt_simulate_charger_plug_out_test(void) {
  union APP_BATTERY_MSG_PRAMS msg_prams;
  msg_prams.charger = APP_BATTERY_CHARGER_PLUGOUT;
  app_ibrt_battery_callback(0, 0, APP_BATTERY_STATUS_CHARGING, 1, msg_prams);
}
void app_ibrt_simulate_charger_plug_box_test(void) {
  static int count = 0;
  if (count++ % 2 == 0) {
    app_ibrt_simulate_charger_plug_in_test();
  } else {
    app_ibrt_simulate_charger_plug_out_test();
  }
}
extern void app_ibrt_sync_volume_info();

void app_ibrt_search_ui_gpio_key_handle(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s,event:%d,code:%d", __func__, status->event, status->code);
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (IBRT_SLAVE == p_ibrt_ctrl->current_role &&
      status->event == APP_KEY_EVENT_DOUBLECLICK) {
    app_ibrt_if_keyboard_notify(status, param);
  } else {
    switch (status->event) {
    case APP_KEY_EVENT_CLICK:
      /*if (status->code == APP_KEY_CODE_FN1)
      {
          app_ibrt_simulate_charger_plug_in_test();
      }
      else if(status->code == APP_KEY_CODE_FN2)
      {
          app_ibrt_simulate_charger_plug_out_test();
      }
      else
      {

      }*/
      break;
    case APP_KEY_EVENT_DOUBLECLICK:
      if (status->code == APP_KEY_CODE_FN1) {
        app_bt_volumeup();
        if (IBRT_MASTER == p_ibrt_ctrl->current_role) {
          TRACE(0, "ibrt master sync volume up to slave !");
          app_ibrt_sync_volume_info();
        }

      } else if (status->code == APP_KEY_CODE_FN2) {
        app_bt_volumedown();
        if (IBRT_MASTER == p_ibrt_ctrl->current_role) {
          TRACE(0, "ibrt master sync volume up to slave !");
          app_ibrt_sync_volume_info();
        }
      } else {
      }
      break;
    case APP_KEY_EVENT_LONGPRESS:
      if (status->code == APP_KEY_CODE_FN1) {
#ifdef TPORTS_KEY_COEXIST
        app_ibrt_simulate_charger_plug_out_test();
#else
        app_ibrt_ui_tws_switch();
#endif
      } else if (status->code == APP_KEY_CODE_FN2) {

      } else {
      }
      break;
    default:
      break;
    }
  }
}
#endif

const app_uart_handle_t app_ibrt_uart_test_handle[] = {
    {"factoryreset_test", app_ibrt_nvrecord_rebuild},
    {"roleswitch_test", app_ibrt_role_switch_test},
    {"inquiry_start_test", app_ibrt_inquiry_start_test},
    {"open_box_event_test", app_ibrt_ui_open_box_event_test},
    {"fetch_out_box_event_test", app_ibrt_ui_fetch_out_box_event_test},
    {"put_in_box_event_test", app_ibrt_ui_put_in_box_event_test},
    {"close_box_event_test", app_ibrt_ui_close_box_event_test},
    {"reconnect_event_test", app_ibrt_ui_reconnect_event_test},
    {"wear_up_event_test", app_ibrt_ui_ware_up_event_test},
    {"wear_down_event_test", app_ibrt_ui_ware_down_event_test},
    {"shut_down_test", app_ibrt_ui_shut_down_test},
    {"phone_connect_event_test", app_ibrt_ui_phone_connect_event_test},
    {"switch_ibrt_test", app_ibrt_ui_tws_swtich_test},
    {"suspend_ibrt_test", app_ibrt_ui_suspend_ibrt_test},
    {"resume_ibrt_test", app_ibrt_ui_resume_ibrt_test},
    {"conn_second_mobile_test", app_ibrt_ui_choice_connect_second_mobile},
    {"mobile_tws_disc_test", app_ibrt_if_disconnect_mobile_tws_link},
    {"pairing_mode_test", app_ibrt_ui_pairing_mode_test},
    {"freeman_mode_test", app_ibrt_ui_freeman_pairing_mode_test},
    {"audio_play", app_ibrt_ui_audio_play_test},
    {"audio_pause", app_ibrt_ui_audio_pause_test},
    {"audio_forward", app_ibrt_ui_audio_forward_test},
    {"audio_backward", app_ibrt_ui_audio_backward_test},
    {"avrcp_volup", app_ibrt_ui_avrcp_volume_up_test},
    {"avrcp_voldn", app_ibrt_ui_avrcp_volume_down_test},
    {"hfsco_create", app_ibrt_ui_hfsco_create_test},
    {"hfsco_disc", app_ibrt_ui_hfsco_disc_test},
    {"call_redial", app_ibrt_ui_call_redial_test},
    {"call_answer", app_ibrt_ui_call_answer_test},
    {"call_hangup", app_ibrt_ui_call_hangup_test},
    {"volume_up", app_ibrt_ui_local_volume_up_test},
    {"volume_down", app_ibrt_ui_local_volume_down_test},
    {"get_a2dp_state", app_ibrt_ui_get_a2dp_state_test},
    {"get_avrcp_state", app_ibrt_ui_get_avrcp_state_test},
    {"get_hfp_state", app_ibrt_ui_get_hfp_state_test},
    {"get_call_status", app_ibrt_ui_get_call_status_test},
    {"get_ibrt_role", app_ibrt_ui_get_ibrt_role_test},
    {"get_tws_state", app_ibrt_ui_get_tws_conn_state_test},
    {"iic_switch", app_ibrt_ui_iic_uart_switch_test},
    {"soft_reset", app_ibrt_ui_soft_reset_test},
#ifdef IBRT_SEARCH_UI
    {"plug_in_test", app_ibrt_simulate_charger_plug_in_test},
    {"plug_out_test", app_ibrt_simulate_charger_plug_out_test},
    {"plug_box_test", app_ibrt_simulate_charger_plug_box_test},
#endif
#ifdef IBRT_ENHANCED_STACK_PTS
    {"hf_create_service_link", btif_pts_hf_create_link_with_pts},
    {"hf_disc_service_link", btif_pts_hf_disc_service_link},
    {"hf_create_audio_link", btif_pts_hf_create_audio_link},
    {"hf_disc_audio_link", btif_pts_hf_disc_audio_link},
    {"hf_answer_call", btif_pts_hf_answer_call},
    {"hf_hangup_call", btif_pts_hf_hangup_call},
    {"rfc_register", btif_pts_rfc_register_channel},
    {"rfc_close", btif_pts_rfc_close},
    {"av_create_channel", btif_pts_av_create_channel_with_pts},
    {"av_disc_channel", btif_pts_av_disc_channel},
    {"ar_connect", btif_pts_ar_connect_with_pts},
    {"ar_disconnect", btif_pts_ar_disconnect},
    {"ar_panel_play", btif_pts_ar_panel_play},
    {"ar_panel_pause", btif_pts_ar_panel_pause},
    {"ar_panel_stop", btif_pts_ar_panel_stop},
    {"ar_panel_forward", btif_pts_ar_panel_forward},
    {"ar_panel_backward", btif_pts_ar_panel_backward},
    {"ar_volume_up", btif_pts_ar_volume_up},
    {"ar_volume_down", btif_pts_ar_volume_down},
    {"ar_volume_notify", btif_pts_ar_volume_notify},
    {"ar_volume_change", btif_pts_ar_volume_change},
    {"ar_set_absolute_volume", btif_pts_ar_set_absolute_volume},
#endif
};

/*****************************************************************************
 Prototype    : app_ibrt_ui_find_uart_handle
 Description  : find the test cmd handle
 Input        : uint8_t* buf
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
app_uart_test_function_handle app_ibrt_ui_find_uart_handle(unsigned char *buf) {
  app_uart_test_function_handle p = NULL;
  for (uint8_t i = 0;
       i < sizeof(app_ibrt_uart_test_handle) / sizeof(app_uart_handle_t); i++) {
    if (strncmp((char *)buf, app_ibrt_uart_test_handle[i].string,
                strlen(app_ibrt_uart_test_handle[i].string)) == 0 ||
        strstr(app_ibrt_uart_test_handle[i].string, (char *)buf)) {
      p = app_ibrt_uart_test_handle[i].function;
      break;
    }
  }
  return p;
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_test_cmd_handler
 Description  : ibrt ui test cmd handler
 Input        : uint8_t *buf
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
extern "C" int app_ibrt_ui_test_cmd_handler(unsigned char *buf,
                                            unsigned int length) {
  int ret = 0;

  if (buf[length - 2] == 0x0d || buf[length - 2] == 0x0a) {
    buf[length - 2] = 0;
  }

  app_uart_test_function_handle handl_function =
      app_ibrt_ui_find_uart_handle(buf);
  if (handl_function) {
    handl_function();
  } else {
    ret = -1;
    TRACE(0, "can not find handle function");
  }
  return ret;
}
#ifdef BES_AUDIO_DEV_Main_Board_9v0
void app_ibrt_key1(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 0;
  msg.msg_body.message_ptr = (uint32_t)NULL;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key2(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 1;
  msg.msg_body.message_ptr = (uint32_t)NULL;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key3(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 2;
  msg.msg_body.message_ptr = (uint32_t)NULL;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key4(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 3;
  msg.msg_body.message_ptr = (uint32_t)NULL;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key5(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 4;
  msg.msg_body.message_ptr = (uint32_t)NULL;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_key6(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 5;
  msg.msg_body.message_ptr = (uint32_t)NULL;
  app_ibrt_peripheral_mailbox_put(&msg);
}
#endif

void app_bt_sleep(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  // app_ibrt_ui_event_entry(IBRT_CLOSE_BOX_EVENT);
  bt_key_handle_func_click();
}

void app_wakeup_sleep(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  // app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
  a2dp_handleKey(AVRCP_KEY_PLAY);
}

void app_test_key(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  // app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
  a2dp_handleKey(AVRCP_KEY_PLAY);
}

#if defined(__BT_DEBUG_TPORTS__) && !defined(TPORTS_KEY_COEXIST)

void app_ibrt_ui_test_key(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  {
#ifdef IBRT_SEARCH_UI
    app_ibrt_search_ui_handle_key(status, param);
#else
    app_ibrt_normal_ui_handle_key(status, param);
#endif
  }
}

#else

void app_ibrt_ui_test_key(APP_KEY_STATUS *status, void *param) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
#ifdef TILE_DATAPATH
  uint8_t shutdown_key = HAL_KEY_EVENT_TRIPLECLICK;
#else
  uint8_t shutdown_key = HAL_KEY_EVENT_LONGLONGPRESS;
#endif
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

#if 0 // def IBRT_OTA
	if ((status->code == HAL_KEY_CODE_PWR)&&\
        (status->event == APP_KEY_EVENT_CLICK)&&\
        (app_check_user_can_role_switch_in_ota()))
	{
	    TRACE(0, "[OTA] role switch in progress!");
        return ;
	}
#endif

  if (IBRT_SLAVE == p_ibrt_ctrl->current_role &&
      status->event != shutdown_key) {
    /*       if(((status->code == HAL_KEY_CODE_PWR)&&(status->event ==
       APP_KEY_EVENT_CLICK)) &&\
                      ((p_ibrt_ctrl->master_tws_switch_pending != false) || \
                       (p_ibrt_ctrl->slave_tws_switch_pending != false) ||  \
                       (ota_role_switch_flag != 0)||\
                       (avoid_even_packets_protect_flag != 0)))
           {
               TRACE(0, "[OTA] role switch in progress!");
                           return ;
           }
   */
    app_ibrt_if_keyboard_notify(status, param);
  }

  else {
#ifdef IBRT_SEARCH_UI
    app_ibrt_search_ui_handle_key(status, param);
#else
    app_ibrt_normal_ui_handle_key(status, param);
#endif
  }
}
#endif

void app_ibrt_ui_test_key_io_event(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  switch (status->event) {
  case APP_KEY_EVENT_CLICK:
    if (status->code == APP_KEY_CODE_FN1) {
      app_ibrt_if_event_entry(IBRT_OPEN_BOX_EVENT);
    } else if (status->code == APP_KEY_CODE_FN2) {
      app_ibrt_if_event_entry(IBRT_FETCH_OUT_EVENT);
    } else {
      app_ibrt_if_event_entry(IBRT_WEAR_UP_EVENT);
    }
    break;

  case APP_KEY_EVENT_DOUBLECLICK:
    if (status->code == APP_KEY_CODE_FN1) {
      app_ibrt_if_event_entry(IBRT_CLOSE_BOX_EVENT);
    } else if (status->code == APP_KEY_CODE_FN2) {
      app_ibrt_if_event_entry(IBRT_PUT_IN_EVENT);
    } else {
      app_ibrt_if_event_entry(IBRT_WEAR_DOWN_EVENT);
    }
    break;

  case APP_KEY_EVENT_LONGPRESS:
    break;

  case APP_KEY_EVENT_TRIPLECLICK:
    break;

  case HAL_KEY_EVENT_LONGLONGPRESS:
    break;

  case APP_KEY_EVENT_ULTRACLICK:
    break;

  case APP_KEY_EVENT_RAMPAGECLICK:
    break;
  }
}

void app_ibrt_ui_test_key_custom_event(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  switch (status->event) {
  case APP_KEY_EVENT_CLICK:
    break;

  case APP_KEY_EVENT_DOUBLECLICK:
    break;

  case APP_KEY_EVENT_LONGPRESS:
    break;

  case APP_KEY_EVENT_TRIPLECLICK:
    break;

  case HAL_KEY_EVENT_LONGLONGPRESS:
    break;

  case APP_KEY_EVENT_ULTRACLICK:
    break;

  case APP_KEY_EVENT_RAMPAGECLICK:
    break;
  }
}

void app_ibrt_ui_test_voice_assistant_key(APP_KEY_STATUS *status, void *param) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  TRACE(3, "%s code 0x%x event %d", __func__, status->code, status->event);

  if (APP_KEY_CODE_GOOGLE != status->code) {
    return;
  } else {
    ibrt_ctrl_t *pIbrtCtrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (IBRT_ACTIVE_MODE != pIbrtCtrl->tws_mode) {
      app_tws_ibrt_exit_sniff_with_mobile();
    }
  }

  if (p_ibrt_ctrl->current_role != IBRT_MASTER) {
    app_ibrt_if_keyboard_notify(status, param);
    TRACE(2, "%s isn't master %d", __func__, p_ibrt_ctrl->current_role);
    return;
  }

#ifdef IS_MULTI_AI_ENABLED
  if (app_ai_manager_spec_get_status_is_in_invalid()) {
    TRACE(0, "AI feature has been diabled");
    return;
  }

  if (app_ai_manager_is_need_reboot()) {
    TRACE(1, "%s ai need to reboot", __func__);
    return;
  }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
  if (app_ai_manager_get_spec_update_flag()) {
    TRACE(0, "device reboot is ongoing...");
    return;
  }
#endif

  if (app_ai_manager_voicekey_is_enable()) {
    if (AI_SPEC_GSOUND == app_ai_manager_get_current_spec()) {
#ifdef BISTO_ENABLED
      gsound_custom_actions_handle_key(status, param);
#endif
    } else if (AI_SPEC_INIT != app_ai_manager_get_current_spec()) {
      app_ai_key_event_handle(status, 0);
    }
  }
#else
#ifdef __AI_VOICE__
  app_ai_key_event_handle(status, 0);
#endif
#ifdef BISTO_ENABLED
  gsound_custom_actions_handle_key(status, param);
#endif
#endif
}

const APP_KEY_HANDLE app_ibrt_ui_test_key_cfg[] = {

#if defined(ANC_APP)

#endif

#if defined(__BT_ANC_KEY__) && defined(ANC_APP)
    //{{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt anc key",app_anc_key, NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGPRESS},
     "app_ibrt_ui_test_key",
     app_anc_key,
     NULL},
#else
//{{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"app_ibrt_ui_test_key",
// app_ibrt_ui_test_key, NULL},
#endif

    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
     "app_ibrt_ui_test_key",
     app_bt_sleep,
     NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_DOUBLECLICK},
     "app_ibrt_ui_test_key",
     app_wakeup_sleep,
     NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_TRIPLECLICK},
     "app_ibrt_ui_test_key",
     app_ibrt_ui_test_key,
     NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_ULTRACLICK},
     "app_ibrt_ui_test_key",
     app_ibrt_ui_test_key,
     NULL},
};

/*
 * customer addr config here
 */
ibrt_pairing_info_t g_ibrt_pairing_info[] = {
    {{0x51, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x50, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x53, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x52, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*LJH*/
    {{0x61, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x60, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x67, 0x66, 0x66, 0x22, 0x11, 0x11},
     {0x66, 0x66, 0x66, 0x22, 0x11, 0x11}}, /*bisto*/
    {{0x71, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x70, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x81, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x80, 0x33, 0x33, 0x22, 0x11, 0x11}},
    {{0x91, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x90, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*Customer use*/
    {{0x05, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x04, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*Rui*/
    {{0x07, 0x33, 0x33, 0x22, 0x11, 0x11},
     {0x06, 0x33, 0x33, 0x22, 0x11, 0x11}}, /*zsl*/
    {{0x88, 0xaa, 0x33, 0x22, 0x11, 0x11},
     {0x87, 0xaa, 0x33, 0x22, 0x11, 0x11}}, /*Lufang*/
    {{0x77, 0x22, 0x66, 0x22, 0x11, 0x11},
     {0x77, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*xiao*/
    {{0xAA, 0x22, 0x66, 0x22, 0x11, 0x11},
     {0xBB, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*LUOBIN*/
    {{0x08, 0x33, 0x66, 0x22, 0x11, 0x11},
     {0x07, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Yangbin1*/
    {{0x0B, 0x33, 0x66, 0x22, 0x11, 0x11},
     {0x0A, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Yangbin2*/
    {{0x35, 0x33, 0x66, 0x22, 0x11, 0x11},
     {0x34, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*Lulu*/
    {{0xF8, 0x33, 0x66, 0x22, 0x11, 0x11},
     {0xF7, 0x33, 0x66, 0x22, 0x11, 0x11}}, /*jtx*/
    {{0xd3, 0x53, 0x86, 0x42, 0x71, 0x31},
     {0xd2, 0x53, 0x86, 0x42, 0x71, 0x31}}, /*shhx*/
    {{0xcc, 0xaa, 0x99, 0x88, 0x77, 0x66},
     {0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66}}, /*mql*/
    {{0x95, 0x33, 0x69, 0x22, 0x11, 0x11},
     {0x94, 0x33, 0x69, 0x22, 0x11, 0x11}}, /*wyl*/
    {{0x82, 0x35, 0x68, 0x24, 0x19, 0x17},
     {0x81, 0x35, 0x68, 0x24, 0x19, 0x17}}, /*hy*/
    {{0x66, 0x66, 0x88, 0x66, 0x66, 0x88},
     {0x65, 0x66, 0x88, 0x66, 0x66, 0x88}}, /*xdl*/
    {{0x61, 0x66, 0x66, 0x66, 0x66, 0x81},
     {0x16, 0x66, 0x66, 0x66, 0x66, 0x18}}, /*test1*/
    {{0x62, 0x66, 0x66, 0x66, 0x66, 0x82},
     {0x26, 0x66, 0x66, 0x66, 0x66, 0x28}}, /*test2*/
    {{0x63, 0x66, 0x66, 0x66, 0x66, 0x83},
     {0x36, 0x66, 0x66, 0x66, 0x66, 0x38}}, /*test3*/
    {{0x64, 0x66, 0x66, 0x66, 0x66, 0x84},
     {0x46, 0x66, 0x66, 0x66, 0x66, 0x48}}, /*test4*/
    {{0x65, 0x66, 0x66, 0x66, 0x66, 0x85},
     {0x56, 0x66, 0x66, 0x66, 0x66, 0x58}}, /*test5*/
    {{0xaa, 0x66, 0x66, 0x66, 0x66, 0x86},
     {0xaa, 0x66, 0x66, 0x66, 0x66, 0x68}}, /*test6*/
    {{0x67, 0x66, 0x66, 0x66, 0x66, 0x87},
     {0x76, 0x66, 0x66, 0x66, 0x66, 0x78}}, /*test7*/
    {{0x68, 0x66, 0x66, 0x66, 0x66, 0xa8},
     {0x86, 0x66, 0x66, 0x66, 0x66, 0x8a}}, /*test8*/
    {{0x69, 0x66, 0x66, 0x66, 0x66, 0x89},
     {0x86, 0x66, 0x66, 0x66, 0x66, 0x18}}, /*test9*/
    {{0x93, 0x33, 0x33, 0x33, 0x33, 0x33},
     {0x92, 0x33, 0x33, 0x33, 0x33, 0x33}}, /*gxl*/
    {{0xae, 0x28, 0x00, 0xe9, 0xc6, 0x5c},
     {0xd8, 0x29, 0x00, 0xe9, 0xc6, 0x5c}}, /*lsk*/
    {{0x07, 0x13, 0x66, 0x22, 0x11, 0x11},
     {0x06, 0x13, 0x66, 0x22, 0x11, 0x11}}, /*yangguo*/
    {{0x02, 0x15, 0x66, 0x22, 0x11, 0x11},
     {0x01, 0x15, 0x66, 0x22, 0x11, 0x11}}, /*mql fpga*/

};

// /******************************pwrkey_det_timer*********************************************************/
// osTimerId pwrkey_detid = NULL;
// void startpwrkey_det(int ms);
// void stoppwrkey_det(void);
// static void pwrkey_detfun(const void *);
// osTimerDef(defpwrkey_det,pwrkey_detfun);
// void pwrkey_detinit(void)
// {
// 	TRACE(3,"%s",__func__);
// 	pwrkey_detid = osTimerCreate(osTimer(defpwrkey_det),osTimerOnce,(void
// *)0);
// }

// extern void app_ibrt_customif_test1_cmd_send(uint8_t *p_buff, uint16_t
// length); static void pwrkey_detfun(const void *)
// {

// 	static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
// 	static bool last_pwrkey = false;
// 	bool curr_pwrkey_sta;
// 	curr_pwrkey_sta = hal_pwrkey_pressed();
// 	APP_KEY_STATUS inear_status[] = {APP_KEY_CODE_FN3,HAL_KEY_EVENT_CLICK};
// 	APP_KEY_STATUS outear_status[] = {APP_KEY_CODE_FN4,HAL_KEY_EVENT_CLICK};
// 	//TRACE(3,"pwrkey = %d",curr_pwrkey_sta);
// 	if(curr_pwrkey_sta != last_pwrkey){
// 		if(curr_pwrkey_sta == true){
// 			//app_wakeup_sleep(NULL,NULL);
// 		    	TRACE(3,"%s PLAY",__func__);
//                 	app_bt_accessmode_set(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
// 					app_voice_report(APP_STATUS_INDICATION_BOTHSCAN,0);

// 			if (IBRT_SLAVE == p_ibrt_ctrl->current_role){
// 				app_ibrt_customif_test1_cmd_send((uint8_t
// *)inear_status, sizeof(APP_KEY_STATUS)); 			}else{
// a2dp_handleKey(AVRCP_KEY_PLAY);
// 			}
// 		}else{
// 			//app_bt_sleep(NULL,NULL);
// 			TRACE(3,"%s PAUSE",__func__);
// 			//a2dp_handleKey(AVRCP_KEY_PAUSE);
// 			if (IBRT_SLAVE == p_ibrt_ctrl->current_role){
// 				app_ibrt_customif_test1_cmd_send((uint8_t
// *)outear_status, sizeof(APP_KEY_STATUS)); 			}else{
// 				a2dp_handleKey(AVRCP_KEY_PAUSE);
// 			}
// 		}
// 		last_pwrkey = curr_pwrkey_sta;
// 	}
// 	startpwrkey_det(200);
// }

// void startpwrkey_det(int ms)
// {
// 	//TRACE(3,"\n\n !!!!!!!!!!start %s\n\n",__func__);
// 	osTimerStart(pwrkey_detid,ms);
// }

// void stoppwrkey_det(void)
// {
// 	//TRACE("\n\n!!!!!!!!!!  stop %s\n\n",__func__);
// 	osTimerStop(pwrkey_detid);
// }

/********************************pwrkey_det_timer*******************************************************/

int app_ibrt_ui_test_config_load(void *config) {
  ibrt_pairing_info_t *ibrt_pairing_info_lst = g_ibrt_pairing_info;
  uint32_t lst_size = sizeof(g_ibrt_pairing_info) / sizeof(ibrt_pairing_info_t);
  ibrt_config_t *ibrt_config = (ibrt_config_t *)config;
  struct nvrecord_env_t *nvrecord_env;
  uint8_t ble_address[6] = {0, 0, 0, 0, 0, 0};

  nv_record_env_get(&nvrecord_env);
  if (nvrecord_env->ibrt_mode.tws_connect_success == 0) {
    app_ibrt_ui_clear_tws_connect_success_last();
  } else {
    app_ibrt_ui_set_tws_connect_success_last();
  }

  if (memcmp(nv_record_tws_get_self_ble_info(), bt_get_ble_local_address(),
             BD_ADDR_LEN) &&
      memcmp(nv_record_tws_get_self_ble_info(), ble_address, BD_ADDR_LEN)) {
    nv_record_tws_exchange_ble_info();
  }

  factory_section_original_btaddr_get(ibrt_config->local_addr.address);
  for (uint32_t i = 0; i < lst_size; i++) {
    if (!memcmp(ibrt_pairing_info_lst[i].master_bdaddr.address,
                ibrt_config->local_addr.address, BD_ADDR_LEN)) {
      ibrt_config->nv_role = IBRT_MASTER;
      ibrt_config->audio_chnl_sel = AUDIO_CHANNEL_SELECT_RCHNL;
      memcpy(ibrt_config->peer_addr.address,
             ibrt_pairing_info_lst[i].slave_bdaddr.address, BD_ADDR_LEN);
      return 0;
    } else if (!memcmp(ibrt_pairing_info_lst[i].slave_bdaddr.address,
                       ibrt_config->local_addr.address, BD_ADDR_LEN)) {
      ibrt_config->nv_role = IBRT_SLAVE;
      ibrt_config->audio_chnl_sel = AUDIO_CHANNEL_SELECT_LCHNL;
      memcpy(ibrt_config->peer_addr.address,
             ibrt_pairing_info_lst[i].master_bdaddr.address, BD_ADDR_LEN);
      return 0;
    }
  }
  return -1;
}

void app_ibrt_ui_test_key_init(void) {
  app_key_handle_clear();
  for (uint8_t i = 0; i < ARRAY_SIZE(app_ibrt_ui_test_key_cfg); i++) {
    app_key_handle_registration(&app_ibrt_ui_test_key_cfg[i]);
  }
}

void app_ibrt_ui_test_init(void) {
  TRACE(1, "%s", __func__);

  app_ibrt_ui_box_init(&box_ble_addr);
  app_ibrt_auto_test_init();
}

void app_ibrt_ui_sync_status(uint8_t status) {
#ifdef ANC_APP
  app_anc_status_post(status);
#endif
}

#endif
