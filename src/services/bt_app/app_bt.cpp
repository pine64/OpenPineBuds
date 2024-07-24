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

#include "app_bt.h"
#include "a2dp_api.h"
#include "app_a2dp.h"
#include "app_ai_manager_api.h"
#include "app_bt.h"
#include "app_bt_func.h"
#include "app_dip.h"
#include "app_hfp.h"
#include "app_status_ind.h"
#include "app_thread.h"
#include "apps.h"
#include "besbt.h"
#include "besbt_cfg.h"
#include "bluetooth.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "btapp.h"
#include "dip_api.h"
#include "hal_aud.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "hci_api.h"
#include "hfp_api.h"
#include "l2cap_api.h"
#include "me_api.h"
#include "mei_api.h"
#include "nvrecord.h"
#include "os_api.h"

#if defined(IBRT)
#include "app_ibrt_a2dp.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui.h"
#include "app_tws_ibrt.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#include "app_ble_mode_switch.h"

#ifdef GFPS_ENABLED
#include "app_gfps.h"
#endif

#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "ai_spp.h"
#include "ai_thread.h"
#endif

#ifdef __INTERCONNECTION__
#include "app_interconnection.h"
#include "app_interconnection_ble.h"
#include "app_interconnection_ccmp.h"
#include "app_interconnection_spp.h"
#include "app_spp.h"
#include "spp_api.h"
#endif

#ifdef __INTERACTION__
#include "app_interaction.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom_bt.h"
#endif

#if defined(__EARPHONE_STAY_BCR_SLAVE__) && defined(__BT_ONE_BRING_TWO__)
#error can not defined at the same time.
#endif

#ifdef GFPS_ENABLED
#include "app_gfps.h"
#endif

#ifdef TILE_DATAPATH
#include "rwip_config.h"
#include "tile_target_ble.h"
#endif

#if (A2DP_DECODER_VER >= 2)
#include "a2dp_decoder.h"
#endif
extern "C" {
#include "ddbif.h"
}
extern struct BT_DEVICE_T app_bt_device;
extern "C" bool app_anc_work_status(void);
extern uint8_t avrcp_get_media_status(void);
void avrcp_set_media_status(uint8_t status);

#ifdef GFPS_ENABLED
extern "C" void
app_gfps_handling_on_mobile_link_disconnection(btif_remote_device_t *pRemDev);
#endif

static btif_remote_device_t *connectedMobile = NULL;
static btif_remote_device_t *sppOpenMobile = NULL;

U16 bt_accessory_feature_feature = BTIF_HF_CUSTOM_FEATURE_SUPPORT;

#ifdef __BT_ONE_BRING_TWO__
btif_device_record_t record2_copy;
uint8_t record2_avalible;
#endif

enum bt_profile_reconnect_mode {
  bt_profile_reconnect_null,
  bt_profile_reconnect_openreconnecting,
  bt_profile_reconnect_reconnecting,
  bt_profile_reconnect_reconnect_pending,
};

enum bt_profile_connect_status {
  bt_profile_connect_status_unknow,
  bt_profile_connect_status_success,
  bt_profile_connect_status_failure,
};

struct app_bt_profile_manager {
  bool has_connected;
  enum bt_profile_connect_status hfp_connect;
  enum bt_profile_connect_status hsp_connect;
  enum bt_profile_connect_status a2dp_connect;
  bt_bdaddr_t rmt_addr;
  bt_profile_reconnect_mode reconnect_mode;
  bt_profile_reconnect_mode saved_reconnect_mode;
  a2dp_stream_t *stream;
  // HfChannel *chan;
  hf_chan_handle_t chan;
#if defined(__HSP_ENABLE__)
  HsChannel *hs_chan;
#endif
  uint16_t reconnect_cnt;
  osTimerId connect_timer;
  void (*connect_timer_cb)(void const *);

  APP_BT_CONNECTING_STATE_E connectingState;
};

// reconnect = (INTERVAL+PAGETO)*CNT = (3000ms+5000ms)*15 = 120s
#define APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS (3000)
#define APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT (2)
#define APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT (15)
#define APP_BT_PROFILE_CONNECT_RETRY_MS (10000)

static struct app_bt_profile_manager bt_profile_manager[BT_DEVICE_NUM];

static int8_t app_bt_profile_reconnect_pending_process(void);
void app_bt_connectable_mode_stop_reconnecting(void);

btif_accessible_mode_t g_bt_access_mode = BTIF_BAM_NOT_ACCESSIBLE;

#define APP_BT_PROFILE_BOTH_SCAN_MS (11000)
#define APP_BT_PROFILE_PAGE_SCAN_MS (4000)

osTimerId app_bt_accessmode_timer = NULL;
btif_accessible_mode_t app_bt_accessmode_timer_argument =
    BTIF_BAM_NOT_ACCESSIBLE;
static int app_bt_accessmode_timehandler(void const *param);
osTimerDef(APP_BT_ACCESSMODE_TIMER,
           (void (*)(void const *))
               app_bt_accessmode_timehandler); // define timers

#define A2DP_CONN_CLOSED 10

#ifdef __IAG_BLE_INCLUDE__
#define APP_FAST_BLE_ADV_TIMEOUT_IN_MS 30000
osTimerId app_fast_ble_adv_timeout_timer = NULL;
static int app_fast_ble_adv_timeout_timehandler(void const *param);
osTimerDef(APP_FAST_BLE_ADV_TIMEOUT_TIMER,
           (void (*)(void const *))
               app_fast_ble_adv_timeout_timehandler); // define timers

/*---------------------------------------------------------------------------
 *            app_start_fast_connectable_ble_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start fast connectable BLE adv
 *
 * Parameters:
 *    advInterval - adv interval
 *
 * Return:
 *    void
 */
static void app_start_fast_connectable_ble_adv(uint16_t advInterval);
#endif

#if defined(__INTERCONNECTION__)
btif_accessible_mode_t app_bt_get_current_access_mode(void) {
  return g_bt_access_mode;
}

bool app_bt_is_connected() {
  uint8_t i = 0;
  bool connceted_value = false;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (bt_profile_manager[i].has_connected) {
      connceted_value = true;
      break;
    }
  }

  TRACE(1, "bt_is_connected is %d", connceted_value);
  return connceted_value;
}
#endif

bool app_device_bt_is_connected() {
  uint8_t i = 0;
  bool connceted_value = false;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (bt_profile_manager[i].has_connected) {
      connceted_value = true;
      break;
    }
  }

  //   TRACE("bt_is_connected : %d, a2dp : %d, hfp : %d",
  //   connceted_value,bt_profile_manager[i].a2dp_connect,bt_profile_manager[i].hfp_connect);
  return connceted_value;
}

static void app_bt_precheck_before_starting_connecting(uint8_t isBtConnected);

static void app_bt_accessmode_handler(btif_accessible_mode_t accMode) {
  const btif_access_mode_info_t info = {
      BTIF_BT_DEFAULT_INQ_SCAN_INTERVAL, BTIF_BT_DEFAULT_INQ_SCAN_WINDOW,
      BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL, BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW};

  osapi_lock_stack();
  if (accMode == BTIF_BAM_CONNECTABLE_ONLY) {
    app_bt_accessmode_timer_argument = BTIF_BAM_GENERAL_ACCESSIBLE;
    osTimerStart(app_bt_accessmode_timer, APP_BT_PROFILE_PAGE_SCAN_MS);
  } else if (accMode == BTIF_BAM_GENERAL_ACCESSIBLE) {
    app_bt_accessmode_timer_argument = BTIF_BAM_CONNECTABLE_ONLY;
    osTimerStart(app_bt_accessmode_timer, APP_BT_PROFILE_BOTH_SCAN_MS);
  }
  app_bt_ME_SetAccessibleMode(accMode, &info);
  TRACE(1, "app_bt_accessmode_timehandler accMode=%x", accMode);
  osapi_unlock_stack();
}

static int app_bt_accessmode_timehandler(void const *param) {
  btif_accessible_mode_t accMode = *(btif_accessible_mode_t *)(param);
  app_bt_start_custom_function_in_bt_thread(
      (uint32_t)accMode, 0, (uint32_t)app_bt_accessmode_handler);
  return 0;
}

void app_bt_accessmode_set(btif_accessible_mode_t mode) {
  const btif_access_mode_info_t info = {
      BTIF_BT_DEFAULT_INQ_SCAN_INTERVAL, BTIF_BT_DEFAULT_INQ_SCAN_WINDOW,
      BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL, BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW};
#if defined(IBRT)
  return;
#endif
  TRACE(2, "%s %d", __func__, mode);
  osapi_lock_stack();
  g_bt_access_mode = mode;
  if (g_bt_access_mode == BTIF_BAM_GENERAL_ACCESSIBLE) {
    app_bt_accessmode_timehandler(&g_bt_access_mode);
  } else {
    osTimerStop(app_bt_accessmode_timer);
    app_bt_ME_SetAccessibleMode(g_bt_access_mode, &info);
    TRACE(1, "app_bt_accessmode_set access_mode=%x", g_bt_access_mode);
  }
  osapi_unlock_stack();
}

extern "C" uint8_t app_bt_get_act_cons(void) {
  int activeCons;

  osapi_lock_stack();
  activeCons = btif_me_get_activeCons();
  osapi_unlock_stack();
  TRACE(2, "%s %d", __func__, activeCons);
  return activeCons;
}
enum {
  INITIATE_PAIRING_NONE = 0,
  INITIATE_PAIRING_RUN = 1,
};
static uint8_t initiate_pairing = INITIATE_PAIRING_NONE;
void app_bt_connectable_state_set(uint8_t set) { initiate_pairing = set; }
bool is_app_bt_pairing_running(void) {
  return (initiate_pairing == INITIATE_PAIRING_RUN) ? (true) : (false);
}
#define APP_DISABLE_PAGE_SCAN_AFTER_CONN
#ifdef APP_DISABLE_PAGE_SCAN_AFTER_CONN
osTimerId disable_page_scan_check_timer = NULL;
static void disable_page_scan_check_timer_handler(void const *param);
osTimerDef(DISABLE_PAGE_SCAN_CHECK_TIMER,
           (void (*)(void const *))
               disable_page_scan_check_timer_handler); // define timers
static void disable_page_scan_check_timer_handler(void const *param) {
#ifdef __BT_ONE_BRING_TWO__
  if ((btif_me_get_activeCons() > 1) &&
      (initiate_pairing == INITIATE_PAIRING_NONE)) {
#else
  if ((btif_me_get_activeCons() > 0) &&
      (initiate_pairing == INITIATE_PAIRING_NONE)) {
#endif
    app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
  }
}

static void disable_page_scan_check_timer_start(void) {
  if (disable_page_scan_check_timer == NULL) {
    disable_page_scan_check_timer = osTimerCreate(
        osTimer(DISABLE_PAGE_SCAN_CHECK_TIMER), osTimerOnce, NULL);
  }
  osTimerStart(disable_page_scan_check_timer, 4000);
}

#endif
void PairingTransferToConnectable(void) {
  int activeCons;
  osapi_lock_stack();
  activeCons = btif_me_get_activeCons();
  osapi_unlock_stack();
  TRACE(1, "%s", __func__);

  app_bt_connectable_state_set(INITIATE_PAIRING_NONE);
  if (activeCons == 0) {
    TRACE(0, "!!!PairingTransferToConnectable  BAM_CONNECTABLE_ONLY\n");
    app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
  }
}
int app_bt_get_audio_up_id(void) {
  uint8_t i;
  btif_remote_device_t *remDev = NULL;
  btif_cmgr_handler_t *cmgrHandler;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    remDev = btif_me_enumerate_remote_devices(i);
    if (remDev != NULL) {
      cmgrHandler = btif_cmgr_get_acl_handler(remDev);
      if (btif_cmgr_is_audio_up(cmgrHandler) == true)
        break;
    }
  }

  return i;
}
#define HFP_DEBUG
void hfp_call_state_checker(void);

#if defined(IBRT)
int app_bt_ibrt_profile_checker(const char *str, btif_remote_device_t *remDev,
                                btif_cmgr_handler_t *cmgrHandler,
                                a2dp_stream_t *a2dp_stream,
                                hf_chan_handle_t hf_channel) {
  if (remDev && cmgrHandler) {
    TRACE(6, "checker: %s remDev state:%d mode:%d role:%d sniffInterval:%d/%d",
          str, btif_me_get_remote_device_state(remDev),
          btif_me_get_remote_device_mode(remDev),
          btif_me_get_remote_device_role(remDev),
          btif_cmgr_get_cmgrhandler_sniff_Interval(cmgrHandler),
          btif_cmgr_get_cmgrhandler_sniff_info(cmgrHandler)->maxInterval);

    TRACE(2, "checker: %s remDev:%p remote_dev_address:", str, remDev);
    DUMP8("0x%02x ", btif_me_get_remote_device_bdaddr(remDev)->address,
          BTIF_BD_ADDR_SIZE);
  } else {
    TRACE(3, "checker: %s remDev:%p cmgrHandler:%p", str, remDev, cmgrHandler);
  }

  if (a2dp_stream) {
    TRACE(1, "a2dp State:%d", btif_a2dp_get_stream_state(a2dp_stream));
    if (btif_a2dp_get_stream_state(a2dp_stream) > BTIF_AVDTP_STRM_STATE_IDLE) {
      TRACE(1, "a2dp cmgrHandler remDev:%p",
            btif_a2dp_get_stream_devic_cmgrHandler_remdev(a2dp_stream));
    }
  } else {
    TRACE(1, "%s a2dp stream NULL", str);
  }
  if (hf_channel) {
    TRACE(4, "hf_channel Connected:%d IsAudioUp:%d/%d remDev:%p",
          btif_hf_is_acl_connected(hf_channel), app_bt_device.hf_audio_state[0],
          btif_hf_check_AudioConnect_status(hf_channel),
          btif_hf_cmgr_get_remote_device(hf_channel));
  } else {
    TRACE(1, "%s hf_channel NULL", str);
  }
  return 0;
}
#endif

int app_bt_state_checker(void) {
  btif_remote_device_t *remDev = NULL;
  btif_cmgr_handler_t *cmgrHandler;

  osapi_lock_stack();

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (app_tws_ibrt_mobile_link_connected()) {
    TRACE(1, "checker: IBRT_MASTER activeCons:%d", btif_me_get_activeCons());
    remDev = p_ibrt_ctrl->p_tws_remote_dev;
    if (remDev != NULL) {
      cmgrHandler = btif_cmgr_get_acl_handler(remDev);
      if (cmgrHandler) {
        app_bt_ibrt_profile_checker("tws peers", remDev, cmgrHandler, NULL,
                                    NULL);
      } else {
        TRACE(0, "checker: cmgrhandler not handle p_tws_remote_dev!");
      }
    } else {
      TRACE(0, "checker: tws_remote_dev is NULL!");
    }

    remDev = p_ibrt_ctrl->p_mobile_remote_dev;
    if (remDev != NULL) {
      cmgrHandler = btif_cmgr_get_acl_handler(remDev);
      if (cmgrHandler) {
        app_bt_ibrt_profile_checker(
            "master mobile", remDev, cmgrHandler,
            app_bt_device.a2dp_connected_stream[0],
            app_bt_device.hf_conn_flag[0] ? app_bt_device.hf_channel[0] : NULL);
      } else {
        TRACE(0, "checker: cmgrhandler not handle mobile_remote_dev");
      }
    } else {
      TRACE(0, "checker: mobile_remote_dev is NULL!");
    }
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    TRACE(1, "checker: IBRT_SLAVE activeCons:%d", btif_me_get_activeCons());
    remDev = p_ibrt_ctrl->p_tws_remote_dev;
    if (remDev != NULL) {
      cmgrHandler = btif_cmgr_get_acl_handler(remDev);
      if (cmgrHandler) {
        app_bt_ibrt_profile_checker("tws peers", remDev, cmgrHandler, NULL,
                                    NULL);
      } else {
        TRACE(0, "checker: cmgrhandler not handle p_tws_remote_dev!");
      }
    } else {
      TRACE(0, "checker: tws_remote_dev is NULL!");
    }
    if (app_ibrt_ui_is_profile_exchanged()) {
      app_bt_ibrt_profile_checker(
          "ibrt mobile", NULL, NULL, app_bt_device.a2dp_connected_stream[0],
          app_bt_device.hf_conn_flag[0] ? app_bt_device.hf_channel[0] : NULL);
    }
  } else {
    TRACE(1, "checker: IBRT_UNKNOW activeCons:%d", btif_me_get_activeCons());
  }
  app_ibrt_if_ctx_checker();
#if defined(ENHANCED_STACK)
  btif_me_cobuf_state_dump();
  btif_me_hcibuff_state_dump();
#endif
  // BT controller state checker
  ASSERT(bt_drv_reg_op_check_bt_controller_state(), "BT controller dead!");
#else
  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    remDev = btif_me_enumerate_remote_devices(i);
    if (remDev != NULL) {
      cmgrHandler = btif_cmgr_get_acl_handler(remDev);
      if (cmgrHandler) {
        TRACE(8,
              "checker: id:%d state:%d mode:%d role:%d cmghdl:%p "
              "sniffInterva:%d/%d IsAudioUp:%d",
              i, btif_me_get_remote_device_state(remDev),
              btif_me_get_remote_device_mode(remDev),
              btif_me_get_remote_device_role(remDev), cmgrHandler,
              btif_cmgr_get_cmgrhandler_sniff_Interval(cmgrHandler),
              btif_cmgr_get_cmgrhandler_sniff_info(cmgrHandler)->maxInterval,
              app_bt_device.hf_audio_state[i]);
        DUMP8("0x%02x ", btif_me_get_remote_device_bdaddr(remDev)->address,
              BTIF_BD_ADDR_SIZE);
        TRACE(4, "remDev:%p a2dp State:%d hf_channel Connected:%d remDev:%p ",
              remDev,
              app_bt_device.a2dp_connected_stream[i]
                  ? btif_a2dp_get_stream_state(
                        app_bt_device.a2dp_connected_stream[i])
                  : BTIF_A2DP_STREAM_STATE_CLOSED,
              app_bt_device.hf_conn_flag[i],
              app_bt_device.hf_conn_flag[i]
                  ? (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(
                        app_bt_device.hf_channel[i])
                  : NULL);
      }
    } else {
      TRACE(1, "checker: id:%d remDev is NULL!", i);
    }

#ifdef __AI_VOICE__
    TRACE(1, "ai_setup_complete %d", app_ai_is_setup_complete());
#endif
#ifdef IS_MULTI_AI_ENABLED
    TRACE(1, "current_spec %d", app_ai_manager_get_current_spec());
#endif
#if defined(__HSP_ENABLE__)
    TRACE(2, "hs_channel Connected:%d remDev:%p ",
          app_bt_device.hs_conn_flag[i],
          app_bt_device.hs_channel[i].cmgrHandler.remDev);
#endif
    TRACE(2, "%s btif_me_get_activeCons = %d", __func__,
          btif_me_get_activeCons());
#ifdef HFP_DEBUG
    hfp_call_state_checker();
#endif
  }
#if defined(ENHANCED_STACK)
  btif_me_cobuf_state_dump();
  btif_me_hcibuff_state_dump();
#endif
#endif
  osapi_unlock_stack();

  return 0;
}

static uint8_t app_bt_get_devId_from_RemDev(btif_remote_device_t *remDev) {
  uint8_t connectedDevId = 0;
  for (uint8_t devId = 0; devId < BT_DEVICE_NUM; devId++) {
    if (btif_me_enumerate_remote_devices(devId) == remDev) {
      TRACE(2, "%s %d", __func__, devId);
      connectedDevId = devId;
      break;
    }
  }

  return connectedDevId;
}
void app_bt_accessible_manager_process(const btif_event_t *Event) {
#if defined(IBRT)
  // IBRT device's access mode will be controlled by UI
  return;
#else
  btif_event_type_t etype = btif_me_get_callback_event_type(Event);
#ifdef __BT_ONE_BRING_TWO__
  static uint8_t opening_reconnect_cnf_cnt = 0;
  // uint8_t disconnectedDevId =
  // app_bt_get_devId_from_RemDev(btif_me_get_callback_event_rem_dev( Event));

  if (app_bt_profile_connect_openreconnecting(NULL)) {
    if (etype == BTIF_BTEVENT_LINK_CONNECT_CNF) {
      opening_reconnect_cnf_cnt++;
    }
    if (record2_avalible) {
      if (opening_reconnect_cnf_cnt < 2) {
        return;
      }
    }
  }
#endif
  switch (etype) {
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
  case BTIF_BTEVENT_LINK_CONNECT_IND:
    TRACE(1, "BTEVENT_LINK_CONNECT_IND/CNF activeCons:%d",
          btif_me_get_activeCons());
#if defined(__BTIF_EARPHONE__)
    app_stop_10_second_timer(APP_PAIR_TIMER_ID);
#endif
#ifdef __BT_ONE_BRING_TWO__
    if (btif_me_get_activeCons() == 0) {
#ifdef __EARPHONE_STAY_BOTH_SCAN__
      app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#else
      app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif
    } else if (btif_me_get_activeCons() == 1) {
      app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
    } else if (btif_me_get_activeCons() >= 2) {
      app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
    }
#else
    if (btif_me_get_activeCons() == 0) {
#ifdef __EARPHONE_STAY_BOTH_SCAN__
      app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#else
      app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif
    } else if (btif_me_get_activeCons() >= 1) {
      app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
    }
#endif
    break;
  case BTIF_BTEVENT_LINK_DISCONNECT:
    TRACE(1, "DISCONNECT activeCons:%d", btif_me_get_activeCons());
#ifdef __EARPHONE_STAY_BOTH_SCAN__
#ifdef __BT_ONE_BRING_TWO__
    if (btif_me_get_activeCons() == 0) {
      app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
    } else if (btif_me_get_activeCons() == 1) {
      app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
    } else if (btif_me_get_activeCons() >= 2) {
      app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
    }
#else
    app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#endif
#else
    app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif
    break;
#ifdef __BT_ONE_BRING_TWO__
  case BTIF_BTEVENT_SCO_CONNECT_IND:
  case BTIF_BTEVENT_SCO_CONNECT_CNF:
    if (btif_me_get_activeCons() == 1) {
      app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
    }
    break;
  case BTIF_BTEVENT_SCO_DISCONNECT:
    if (btif_me_get_activeCons() == 1) {
      app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
    }
    break;
#endif
  default:
    break;
  }
#endif
}

#define APP_BT_SWITCHROLE_LIMIT (2)
// #define __SET_OUR_AS_MASTER__

void app_bt_role_manager_process(const btif_event_t *Event) {
#if defined(IBRT) || defined(APP_LINEIN_A2DP_SOURCE) ||                        \
    defined(APP_I2S_A2DP_SOURCE) || defined(__APP_A2DP_SOURCE__)
  return;
#else
  static btif_remote_device_t *opRemDev = NULL;
  static uint8_t switchrole_cnt = 0;
  btif_remote_device_t *remDev = NULL;
  btif_event_type_t etype = btif_me_get_callback_event_type(Event);
  // on phone connecting
  switch (etype) {
  case BTIF_BTEVENT_LINK_CONNECT_IND:
    if (btif_me_get_callback_event_err_code(Event) == BTIF_BEC_NO_ERROR) {
      if (btif_me_get_activeCons() == 1) {
        switch (btif_me_get_callback_event_rem_dev_role(Event)) {
#if defined(__SET_OUR_AS_MASTER__)
        case BTIF_BCR_SLAVE:
        case BTIF_BCR_PSLAVE:
#else
        case BTIF_BCR_MASTER:
        case BTIF_BCR_PMASTER:
#endif
          TRACE(1, "CONNECT_IND try to role %p\n",
                btif_me_get_callback_event_rem_dev(Event));
          // curr connectrot try to role
          opRemDev = btif_me_get_callback_event_rem_dev(Event);
          switchrole_cnt = 0;
          app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                  BTIF_BLP_MASTER_SLAVE_SWITCH |
                                      BTIF_BLP_SNIFF_MODE);
          break;
#if defined(__SET_OUR_AS_MASTER__)
        case BTIF_BCR_MASTER:
        case BTIF_BCR_PMASTER:
#else
        case BTIF_BCR_SLAVE:
        case BTIF_BCR_PSLAVE:
#endif
        case BTIF_BCR_ANY:
        case BTIF_BCR_UNKNOWN:
        default:
          TRACE(1, "CONNECT_IND disable role %p\n",
                btif_me_get_callback_event_rem_dev(Event));
          // disable roleswitch when 1 connect
          app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                  BTIF_BLP_SNIFF_MODE);
          break;
        }
        // set next connector to master
        app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
      } else if (btif_me_get_activeCons() > 1) {
        switch (btif_me_get_callback_event_rem_dev_role(Event)) {
        case BTIF_BCR_MASTER:
        case BTIF_BCR_PMASTER:
          TRACE(1, "CONNECT_IND disable role %p\n",
                btif_me_get_callback_event_rem_dev(Event));
          // disable roleswitch
          app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                  BTIF_BLP_SNIFF_MODE);
          break;
        case BTIF_BCR_SLAVE:
        case BTIF_BCR_PSLAVE:
        case BTIF_BCR_ANY:
        case BTIF_BCR_UNKNOWN:
        default:
          // disconnect slave
          TRACE(1, "CONNECT_IND disconnect slave %p\n",
                btif_me_get_callback_event_rem_dev(Event));
          app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev(Event));
          break;
        }
        // set next connector to master
        app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
      }
    }
    break;
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
    if (btif_me_get_activeCons() == 1) {
      switch (btif_me_get_callback_event_rem_dev_role(Event)) {
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_SLAVE:
      case BTIF_BCR_PSLAVE:
#else
      case BTIF_BCR_MASTER:
      case BTIF_BCR_PMASTER:
#endif
        TRACE(1, "CONNECT_CNF try to role %p\n",
              btif_me_get_callback_event_rem_dev(Event));
        // curr connectrot try to role
        opRemDev = btif_me_get_callback_event_rem_dev(Event);
        switchrole_cnt = 0;
        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                BTIF_BLP_MASTER_SLAVE_SWITCH |
                                    BTIF_BLP_SNIFF_MODE);
        app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev(Event));
        break;
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_MASTER:
      case BTIF_BCR_PMASTER:
#else
      case BTIF_BCR_SLAVE:
      case BTIF_BCR_PSLAVE:
#endif
      case BTIF_BCR_ANY:
      case BTIF_BCR_UNKNOWN:
      default:
        TRACE(1, "CONNECT_CNF disable role %p\n",
              btif_me_get_callback_event_rem_dev(Event));
        // disable roleswitch
        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                BTIF_BLP_SNIFF_MODE);
        break;
      }
      // set next connector to master
      app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
    } else if (btif_me_get_activeCons() > 1) {
      switch (btif_me_get_callback_event_rem_dev_role(Event)) {
      case BTIF_BCR_MASTER:
      case BTIF_BCR_PMASTER:
        TRACE(1, "CONNECT_CNF disable role %p\n",
              btif_me_get_callback_event_rem_dev(Event));
        // disable roleswitch
        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                BTIF_BLP_SNIFF_MODE);
        break;
      case BTIF_BCR_SLAVE:
      case BTIF_BCR_ANY:
      case BTIF_BCR_UNKNOWN:
      default:
        // disconnect slave
        TRACE(1, "CONNECT_CNF disconnect slave %p\n",
              btif_me_get_callback_event_rem_dev(Event));
        app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev(Event));
        break;
      }
      // set next connector to master
      app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
    }
    break;
  case BTIF_BTEVENT_LINK_DISCONNECT:
    if (opRemDev == btif_me_get_callback_event_rem_dev(Event)) {
      opRemDev = NULL;
      switchrole_cnt = 0;
    }
    if (btif_me_get_activeCons() == 0) {
      for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
        if (app_bt_device.a2dp_connected_stream[i])
          app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_connected_stream[i],
                                    FALSE);
        app_bt_HF_SetMasterRole(app_bt_device.hf_channel[i], FALSE);
      }
      app_bt_ME_SetConnectionRole(BTIF_BCR_ANY);
    } else if (btif_me_get_activeCons() == 1) {
      // set next connector to master
      app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
    }
    break;
  case BTIF_BTEVENT_ROLE_CHANGE:
    if (opRemDev == btif_me_get_callback_event_rem_dev(Event)) {
      switch (btif_me_get_callback_event_role_change_new_role(Event)) {
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_SLAVE:
#else
      case BTIF_BCR_MASTER:
#endif
        if (++switchrole_cnt <= APP_BT_SWITCHROLE_LIMIT) {
          app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev(Event));
        } else {
#if defined(__SET_OUR_AS_MASTER__)
          TRACE(2, "ROLE TO MASTER FAILED remDev %p cnt:%d\n",
                btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#else
          TRACE(2, "ROLE TO SLAVE FAILED remDev %p cnt:%d\n",
                btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#endif
          opRemDev = NULL;
          switchrole_cnt = 0;
        }
        break;
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_MASTER:
        TRACE(2, "ROLE TO MASTER SUCCESS remDev %p cnt:%d\n",
              btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#else
      case BTIF_BCR_SLAVE:
        TRACE(2, "ROLE TO SLAVE SUCCESS remDev %p cnt:%d\n",
              btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#endif
        opRemDev = NULL;
        switchrole_cnt = 0;
        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                BTIF_BLP_SNIFF_MODE);
        break;
      case BTIF_BCR_ANY:
        break;
      case BTIF_BCR_UNKNOWN:
        break;
      default:
        break;
      }
    }

    if (btif_me_get_activeCons() > 1) {
      uint8_t slave_cnt = 0;
      for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
        remDev = btif_me_enumerate_remote_devices(i);
        if (btif_me_get_current_role(remDev) == BTIF_BCR_SLAVE) {
          slave_cnt++;
        }
      }
      if (slave_cnt > 1) {
        TRACE(1, "ROLE_CHANGE disconnect slave %p\n",
              btif_me_get_callback_event_rem_dev(Event));
        app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev(Event));
      }
    }
    break;
  default:
    break;
  }
#endif
}

void app_bt_role_manager_process_dual_slave(const btif_event_t *Event) {
#if defined(IBRT) || defined(APP_LINEIN_A2DP_SOURCE) ||                        \
    defined(APP_I2S_A2DP_SOURCE) || defined(__APP_A2DP_SOURCE__)
  return;
#else
  static btif_remote_device_t *opRemDev = NULL;
  static uint8_t switchrole_cnt = 0;
  // btif_remote_device_t *remDev = NULL;
  // on phone connecting
  switch (btif_me_get_callback_event_type(Event)) {
  case BTIF_BTEVENT_LINK_CONNECT_IND:
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
    if (btif_me_get_callback_event_err_code(Event) == BTIF_BEC_NO_ERROR) {
      switch (btif_me_get_callback_event_rem_dev_role(Event)) {
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_SLAVE:
      case BTIF_BCR_PSLAVE:
#else
      case BTIF_BCR_MASTER:
      case BTIF_BCR_PMASTER:
#endif
        TRACE(1, "CONNECT_IND/CNF try to role %p\n",
              btif_me_get_callback_event_rem_dev(Event));
        opRemDev = btif_me_get_callback_event_rem_dev(Event);
        switchrole_cnt = 0;
        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                BTIF_BLP_MASTER_SLAVE_SWITCH |
                                    BTIF_BLP_SNIFF_MODE);
        app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev(Event));
        break;
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_MASTER:
      case BTIF_BCR_PMASTER:
#else
      case BTIF_BCR_SLAVE:
      case BTIF_BCR_PSLAVE:
#endif
      case BTIF_BCR_ANY:
      case BTIF_BCR_UNKNOWN:
      default:
        TRACE(1, "CONNECT_IND disable role %p\n",
              btif_me_get_callback_event_rem_dev(Event));
        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                BTIF_BLP_SNIFF_MODE);
        break;
      }
      app_bt_ME_SetConnectionRole(BTIF_BCR_SLAVE);
    }
    break;
  case BTIF_BTEVENT_LINK_DISCONNECT:
    if (opRemDev == btif_me_get_callback_event_rem_dev(Event)) {
      opRemDev = NULL;
      switchrole_cnt = 0;
    }
    if (btif_me_get_activeCons() == 0) {
      for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
        if (app_bt_device.a2dp_connected_stream[i])
          app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_connected_stream[i],
                                    FALSE);
        app_bt_HF_SetMasterRole(app_bt_device.hf_channel[i], FALSE);
      }
      app_bt_ME_SetConnectionRole(BTIF_BCR_ANY);
    } else if (btif_me_get_activeCons() == 1) {
      app_bt_ME_SetConnectionRole(BTIF_BCR_SLAVE);
    }
    break;
  case BTIF_BTEVENT_ROLE_CHANGE:
    if (opRemDev == btif_me_get_callback_event_rem_dev(Event)) {
      switch (btif_me_get_callback_event_role_change_new_role(Event)) {
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_SLAVE:
#else
      case BTIF_BCR_MASTER:
#endif
        if (++switchrole_cnt <= APP_BT_SWITCHROLE_LIMIT) {
          TRACE(1, "ROLE_CHANGE try to role again: %d", switchrole_cnt);
          app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                                  BTIF_BLP_MASTER_SLAVE_SWITCH |
                                      BTIF_BLP_SNIFF_MODE);
          app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev(Event));
        } else {
#if defined(__SET_OUR_AS_MASTER__)
          TRACE(2, "ROLE TO MASTER FAILED remDev %p cnt:%d\n",
                btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#else
          TRACE(2, "ROLE TO SLAVE FAILED remDev %p cnt:%d\n",
                btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#endif
          opRemDev = NULL;
          switchrole_cnt = 0;
        }
        break;
#if defined(__SET_OUR_AS_MASTER__)
      case BTIF_BCR_MASTER:
        TRACE(2, "ROLE TO MASTER SUCCESS remDev %p cnt:%d\n",
              btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#else
      case BTIF_BCR_SLAVE:
        TRACE(2, "ROLE TO SLAVE SUCCESS remDev %p cnt:%d\n",
              btif_me_get_callback_event_rem_dev(Event), switchrole_cnt);
#endif
        opRemDev = NULL;
        switchrole_cnt = 0;
        // workaround for power reset opening reconnect sometime unsuccessfully
        // in sniff mode, only after authentication completes, enable sniff
        // mode.
        opRemDev = btif_me_get_callback_event_rem_dev(Event);
        if (btif_me_get_remote_device_auth_state(opRemDev) ==
            BTIF_BAS_AUTHENTICATED) {
          app_bt_Me_SetLinkPolicy(opRemDev, BTIF_BLP_SNIFF_MODE);
        } else {
          app_bt_Me_SetLinkPolicy(opRemDev, BTIF_BLP_DISABLE_ALL);
        }
        break;
      case BTIF_BCR_ANY:
        break;
      case BTIF_BCR_UNKNOWN:
        break;
      default:
        break;
      }
    }
    break;
  }
#endif
}

static int app_bt_sniff_manager_init(void) {
  btif_sniff_info_t sniffInfo;
  btif_remote_device_t *remDev = NULL;

  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    remDev = btif_me_enumerate_remote_devices(i);
    sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
    sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
    sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
    sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
    app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(&sniffInfo, remDev);
    app_bt_HF_EnableSniffMode(app_bt_device.hf_channel[i], FALSE);
#if defined(__HSP_ENABLE__)
    app_bt_HS_EnableSniffMode(&app_bt_device.hs_channel[i], FALSE);
#endif
  }

  return 0;
}

void app_bt_sniff_config(btif_remote_device_t *remDev) {
  btif_sniff_info_t sniffInfo;
  sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
  sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
  sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
  sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
  app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(&sniffInfo, remDev);
#if !defined(IBRT)
  if (btif_me_get_activeCons() > 1) {
    btif_remote_device_t *tmpRemDev = NULL;
    btif_cmgr_handler_t *currbtif_cmgr_handler_t = NULL;
    btif_cmgr_handler_t *otherbtif_cmgr_handler_t = NULL;
    currbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(remDev);
    for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
      tmpRemDev = btif_me_enumerate_remote_devices(i);
      if (remDev != tmpRemDev && tmpRemDev != NULL) {
        otherbtif_cmgr_handler_t = btif_cmgr_get_acl_handler(tmpRemDev);
        if (otherbtif_cmgr_handler_t && currbtif_cmgr_handler_t) {
          if (btif_cmgr_get_cmgrhandler_sniff_info(otherbtif_cmgr_handler_t)
                  ->maxInterval ==
              btif_cmgr_get_cmgrhandler_sniff_info(currbtif_cmgr_handler_t)
                  ->maxInterval) {
            sniffInfo.maxInterval =
                btif_cmgr_get_cmgrhandler_sniff_info(otherbtif_cmgr_handler_t)
                    ->maxInterval -
                20;
            sniffInfo.minInterval =
                btif_cmgr_get_cmgrhandler_sniff_info(otherbtif_cmgr_handler_t)
                    ->minInterval -
                20;
            sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
            sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
            app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(&sniffInfo, remDev);
          }
        }
        break;
      } else {
        TRACE(3,
              "%s:enumerate i:%d remDev is NULL, param remDev:%p, this may "
              "cause error!",
              __func__, i, remDev);
      }
    }
  }
#endif
}

void app_bt_sniff_manager_process(const btif_event_t *Event) {
  static btif_remote_device_t *opRemDev = NULL;
  btif_remote_device_t *remDev = NULL;
  btif_cmgr_handler_t *currbtif_cmgr_handler_t = NULL;
  btif_cmgr_handler_t *otherbtif_cmgr_handler_t = NULL;

  btif_sniff_info_t sniffInfo;

  if (!besbt_cfg.sniff)
    return;

  switch (btif_me_get_callback_event_type(Event)) {
  case BTIF_BTEVENT_LINK_CONNECT_IND:
    break;
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
    break;
  case BTIF_BTEVENT_LINK_DISCONNECT:
    if (opRemDev == btif_me_get_callback_event_rem_dev(Event)) {
      opRemDev = NULL;
    }
    sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
    sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
    sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
    sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
    app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(
        &sniffInfo, btif_me_get_callback_event_rem_dev(Event));
    break;
  case BTIF_BTEVENT_MODE_CHANGE:

    /*
    if(Event->p.modeChange.curMode == BLM_SNIFF_MODE){
        currbtif_cmgr_handler_t =
    btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev( Event)); if
    (Event->p.modeChange.interval > CMGR_SNIFF_MAX_INTERVAL){ if (!opRemDev){
                    opRemDev = currbtif_cmgr_handler_t->remDev;
                }
                currbtif_cmgr_handler_t->sniffInfo.maxInterval =
    CMGR_SNIFF_MAX_INTERVAL; currbtif_cmgr_handler_t->sniffInfo.minInterval =
    CMGR_SNIFF_MIN_INTERVAL; currbtif_cmgr_handler_t->sniffInfo.attempt =
    CMGR_SNIFF_ATTEMPT; currbtif_cmgr_handler_t->sniffInfo.timeout =
    CMGR_SNIFF_TIMEOUT;
                app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(&currbtif_cmgr_handler_t->sniffInfo,btif_me_get_callback_event_rem_dev(
    Event)); app_bt_ME_StopSniff(currbtif_cmgr_handler_t->remDev); }else{ if
    (currbtif_cmgr_handler_t){ currbtif_cmgr_handler_t->sniffInfo.maxInterval =
    Event->p.modeChange.interval; currbtif_cmgr_handler_t->sniffInfo.minInterval
    = Event->p.modeChange.interval; currbtif_cmgr_handler_t->sniffInfo.attempt =
    CMGR_SNIFF_ATTEMPT; currbtif_cmgr_handler_t->sniffInfo.timeout =
    CMGR_SNIFF_TIMEOUT;
                app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(&currbtif_cmgr_handler_t->sniffInfo,btif_me_get_callback_event_rem_dev(
    Event));
            }
            if (btif_me_get_activeCons() > 1){
                for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                    remDev = btif_me_enumerate_remote_devices(i);
                    if (btif_me_get_callback_event_rem_dev( Event) != remDev){
                        otherbtif_cmgr_handler_t =
    btif_cmgr_get_acl_handler(remDev); if (otherbtif_cmgr_handler_t){ if
    (otherbtif_cmgr_handler_t->sniffInfo.maxInterval ==
    currbtif_cmgr_handler_t->sniffInfo.maxInterval){ if
    (btif_me_get_current_mode(remDev) == BLM_ACTIVE_MODE){
                                    otherbtif_cmgr_handler_t->sniffInfo.maxInterval
    -= 20; otherbtif_cmgr_handler_t->sniffInfo.minInterval -= 20;
                                    otherbtif_cmgr_handler_t->sniffInfo.attempt
    = CMGR_SNIFF_ATTEMPT; otherbtif_cmgr_handler_t->sniffInfo.timeout =
    CMGR_SNIFF_TIMEOUT;
                                    app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(&otherbtif_cmgr_handler_t->sniffInfo,
    remDev); TRACE(1,"reconfig sniff other RemDev:%x\n", remDev); }else if
    (btif_me_get_current_mode(remDev) == BLM_SNIFF_MODE){ need_reconfig = true;
                                }
                            }
                        }
                        break;
                    }
                }
            }
            if (need_reconfig){
                opRemDev = remDev;
                if (currbtif_cmgr_handler_t){
                    currbtif_cmgr_handler_t->sniffInfo.maxInterval -= 20;
                    currbtif_cmgr_handler_t->sniffInfo.minInterval -= 20;
                    currbtif_cmgr_handler_t->sniffInfo.attempt =
    CMGR_SNIFF_ATTEMPT; currbtif_cmgr_handler_t->sniffInfo.timeout =
    CMGR_SNIFF_TIMEOUT;
                    app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(&currbtif_cmgr_handler_t->sniffInfo,
    currbtif_cmgr_handler_t->remDev);
                }
                app_bt_ME_StopSniff(currbtif_cmgr_handler_t->remDev);
                TRACE(1,"reconfig sniff setup op opRemDev:%x\n", opRemDev);
            }
        }
    }
    if (Event->p.modeChange.curMode == BLM_ACTIVE_MODE){
        if (opRemDev ==btif_me_get_callback_event_rem_dev( Event)){
            TRACE(1,"reconfig sniff op opRemDev:%x\n", opRemDev);
            opRemDev = NULL;
            currbtif_cmgr_handler_t =
    btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev( Event)); if
    (currbtif_cmgr_handler_t){
                app_bt_CMGR_SetSniffTimer(currbtif_cmgr_handler_t, NULL,
    CMGR_SNIFF_TIMER);
            }
        }
    }
    */
    break;
  case BTIF_BTEVENT_ACL_DATA_ACTIVE:
    btif_cmgr_handler_t *cmgrHandler;
    /* Start the sniff timer */
    cmgrHandler =
        btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev(Event));
    if (cmgrHandler)
      app_bt_CMGR_SetSniffTimer(cmgrHandler, NULL, BTIF_CMGR_SNIFF_TIMER);
    break;
  case BTIF_BTEVENT_SCO_CONNECT_IND:
  case BTIF_BTEVENT_SCO_CONNECT_CNF:
    TRACE(1, "BTEVENT_SCO_CONNECT_IND/CNF cur_remDev = %p",
          btif_me_get_callback_event_rem_dev(Event));
    currbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(
        btif_me_get_callback_event_rem_dev(Event));
    app_bt_Me_SetLinkPolicy(
        btif_me_get_callback_event_sco_connect_rem_dev(Event),
        BTIF_BLP_DISABLE_ALL);
    if (btif_me_get_activeCons() > 1) {
      for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
        remDev = btif_me_enumerate_remote_devices(i);
        TRACE(1, "other_remDev = %p", remDev);
        if (btif_me_get_callback_event_rem_dev(Event) == remDev) {
          continue;
        }

        otherbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(remDev);
        if (otherbtif_cmgr_handler_t) {
          if (btif_cmgr_is_link_up(otherbtif_cmgr_handler_t)) {
            if (btif_me_get_current_mode(remDev) == BTIF_BLM_ACTIVE_MODE) {
              TRACE(0, "other dev disable sniff");
              app_bt_Me_SetLinkPolicy(remDev, BTIF_BLP_DISABLE_ALL);
            } else if (btif_me_get_current_mode(remDev) ==
                       BTIF_BLM_SNIFF_MODE) {
              TRACE(0, " ohter dev exit & disable sniff");
              app_bt_ME_StopSniff(remDev);
              app_bt_Me_SetLinkPolicy(remDev, BTIF_BLP_DISABLE_ALL);
            }
          }
        }

#if defined(HFP_NO_PRERMPT)
        TRACE(2, "cur_audio = %d other_audio = %d",
              btif_cmgr_is_audio_up(currbtif_cmgr_handler_t),
              btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t));
        if ((btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t) == true) &&
            (btif_cmgr_is_audio_up(currbtif_cmgr_handler_t) == true)
            /*(btapp_hfp_get_call_active()!=0)*/) {
          btif_cmgr_remove_audio_link(currbtif_cmgr_handler_t);
          app_bt_Me_switch_sco(btif_cmgr_get_sco_connect_sco_Hcihandler(
              otherbtif_cmgr_handler_t));
        }
#endif
      }
    }
    break;
  case BTIF_BTEVENT_SCO_DISCONNECT:
    app_bt_profile_reconnect_pending_process();
    if (a2dp_is_music_ongoing()) {
      break;
    }
    if (btif_me_get_activeCons() == 1) {
      app_bt_Me_SetLinkPolicy(
          btif_me_get_callback_event_sco_connect_rem_dev(Event),
          BTIF_BLP_SNIFF_MODE);
    } else {
      uint8_t i;
      for (i = 0; i < BT_DEVICE_NUM; i++) {
        remDev = btif_me_enumerate_remote_devices(i);
        if (btif_me_get_callback_event_rem_dev(Event) == remDev) {
          break;
        }
      }
      /*
                      if(i==0)
                          remDev = btif_me_enumerate_remote_devices(1);
                      else if(i==1)
                          remDev = btif_me_enumerate_remote_devices(0);
                      else
                          ASSERT(0,"error other remotedevice!!!");     */
      otherbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(remDev);
      currbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(
          btif_me_get_callback_event_rem_dev(Event));

      TRACE(4, "SCO_DISCONNECT:%d/%d %p/%p\n",
            btif_cmgr_is_audio_up(currbtif_cmgr_handler_t),
            btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t),
            btif_cmgr_get_cmgrhandler_remdev(currbtif_cmgr_handler_t),
            btif_me_get_callback_event_rem_dev(Event));
      if (otherbtif_cmgr_handler_t) {
        if (!btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t)) {
          TRACE(0, "enable sniff to all\n");
          app_bt_Me_SetLinkPolicy(
              btif_me_get_callback_event_sco_connect_rem_dev(Event),
              BTIF_BLP_SNIFF_MODE);
          app_bt_Me_SetLinkPolicy(
              btif_cmgr_get_cmgrhandler_remdev(otherbtif_cmgr_handler_t),
              BTIF_BLP_SNIFF_MODE);
        }
      } else {
        app_bt_Me_SetLinkPolicy(
            btif_me_get_callback_event_sco_connect_rem_dev(Event),
            BTIF_BLP_SNIFF_MODE);
      }
    }
    break;
  default:
    break;
  }
}

APP_BT_GOLBAL_HANDLE_HOOK_HANDLER
app_bt_global_handle_hook_handler[APP_BT_GOLBAL_HANDLE_HOOK_USER_QTY] = {0};
void app_bt_global_handle_hook(const btif_event_t *Event) {
  uint8_t i;
  for (i = 0; i < APP_BT_GOLBAL_HANDLE_HOOK_USER_QTY; i++) {
    if (app_bt_global_handle_hook_handler[i])
      app_bt_global_handle_hook_handler[i](Event);
  }
}

int app_bt_global_handle_hook_set(enum APP_BT_GOLBAL_HANDLE_HOOK_USER_T user,
                                  APP_BT_GOLBAL_HANDLE_HOOK_HANDLER handler) {
  app_bt_global_handle_hook_handler[user] = handler;
  return 0;
}

APP_BT_GOLBAL_HANDLE_HOOK_HANDLER
app_bt_global_handle_hook_get(enum APP_BT_GOLBAL_HANDLE_HOOK_USER_T user) {
  return app_bt_global_handle_hook_handler[user];
}

extern uint8_t once_event_case;
extern bool IsMobileLinkLossing;
extern void startonce_event(int ms, uint8_t once_event_pram);

extern void a2dp_update_music_link(void);
/////There is a device connected, so stop PAIR_TIMER and POWEROFF_TIMER of
/// earphone.
btif_handler app_bt_handler;
void app_bt_global_handle(const btif_event_t *Event) {
  switch (btif_me_get_callback_event_type(Event)) {
  case BTIF_BTEVENT_HCI_INITIALIZED:
    break;
#if defined(IBRT)
  case BTIF_BTEVENT_HCI_COMMAND_SENT:
    return;
#else
  case BTIF_BTEVENT_HCI_COMMAND_SENT:
  case BTIF_BTEVENT_ACL_DATA_NOT_ACTIVE:
    return;
  case BTIF_BTEVENT_ACL_DATA_ACTIVE:
    btif_cmgr_handler_t *cmgrHandler;
    /* Start the sniff timer */
    cmgrHandler =
        btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev(Event));
    if (cmgrHandler)
      app_bt_CMGR_SetSniffTimer(cmgrHandler, NULL, BTIF_CMGR_SNIFF_TIMER);
    return;
#endif
  case BTIF_BTEVENT_AUTHENTICATED:
    TRACE(1, "[BTEVENT] HANDER AUTH error=%x",
          btif_me_get_callback_event_err_code(Event));
    // after authentication completes, re-enable sniff mode.
    if (btif_me_get_callback_event_err_code(Event) == BTIF_BEC_NO_ERROR) {
      app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev(Event),
                              BTIF_BLP_SNIFF_MODE);
    } else if (btif_me_get_callback_event_err_code(Event) ==
               BTIF_BEC_AUTHENTICATE_FAILURE) {
      // auth failed should clear nv record link key
      bt_bdaddr_t *bd_ddr = btif_me_get_callback_event_rem_dev_bd_addr(Event);
      btif_device_record_t record;

      if (ddbif_find_record(bd_ddr, &record) == BT_STS_SUCCESS) {
        ddbif_delete_record(&record.bdAddr);
#if defined(IBRT)
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        if (!p_ibrt_ctrl->mobile_pair_canceled)
#endif
        {
          memset(&record, 0, sizeof(record));
          record.bdAddr = *bd_ddr;
          ddbif_add_record(&record);
        }
        nv_record_flash_flush();
      }
    }
    break;
  }
  // trace filter
  switch (btif_me_get_callback_event_type(Event)) {
  case BTIF_BTEVENT_HCI_COMMAND_SENT:
  case BTIF_BTEVENT_ACL_DATA_NOT_ACTIVE:
  case BTIF_BTEVENT_ACL_DATA_ACTIVE:
    break;
  default:
    TRACE(1, "[BTEVENT] evt = %d", btif_me_get_callback_event_type(Event));
    break;
  }

  switch (btif_me_get_callback_event_type(Event)) {
  case BTIF_BTEVENT_LINK_CONNECT_IND:
    hfp_reconnecting_timer_stop_callback(Event);
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
#ifdef __BT_ONE_BRING_TWO__
    if (bt_drv_reg_op_get_reconnecting_flag()) {
      bt_drv_reg_op_clear_reconnecting_flag();
      if (a2dp_is_music_ongoing())
        a2dp_update_music_link();
    }
#endif

    if (BTIF_BEC_NO_ERROR == btif_me_get_callback_event_err_code(Event)) {
      connectedMobile = btif_me_get_callback_event_rem_dev(Event);
      uint8_t connectedDevId = app_bt_get_devId_from_RemDev(connectedMobile);
      app_bt_set_connecting_profiles_state(connectedDevId);
      TRACE(1, "MEC(pendCons) is %d", btif_me_get_pendCons());

      app_bt_stay_active_rem_dev(btif_me_get_callback_event_rem_dev(Event));
#ifdef __BT_ONE_BRING_TWO__
      btif_remote_device_t *remote_dev =
          btif_me_get_callback_event_rem_dev(Event);
      uint16_t conn_handle = btif_me_get_remote_device_hci_handle(remote_dev);
      btif_me_qos_set_up(conn_handle);
#endif

#if (defined(__AI_VOICE__) || defined(BISTO_ENABLED))
      app_ai_if_mobile_connect_handle(
          btif_me_get_callback_event_rem_dev_bd_addr(Event));
#endif
    }

    TRACE(4,
          "[BTEVENT] CONNECT_IND/CNF evt:%d errCode:0x%0x newRole:%d "
          "activeCons:%d",
          btif_me_get_callback_event_type(Event),
          btif_me_get_callback_event_err_code(Event),
          btif_me_get_callback_event_rem_dev_role(Event),
          btif_me_get_activeCons());
    DUMP8("%02x ", btif_me_get_callback_event_rem_dev_bd_addr(Event),
          BTIF_BD_ADDR_SIZE);

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
    if (btif_me_get_activeCons() == 0) {
      app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
    } else {
      app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
    }
#endif
#if defined(__BT_ONE_BRING_TWO__) || defined(IBRT)
    if (btif_me_get_activeCons() > 2) {
      TRACE(1, "CONNECT_IND/CNF activeCons:%d so disconnect it",
            btif_me_get_activeCons());
      app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev(Event));
    }
#else
    if (btif_me_get_activeCons() > 1) {
      TRACE(1, "CONNECT_IND/CNF activeCons:%d so disconnect it",
            btif_me_get_activeCons());
      app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev(Event));
    }
#endif
    break;
  case BTIF_BTEVENT_LINK_DISCONNECT: {
    connectedMobile = btif_me_get_callback_event_rem_dev(Event);
    uint8_t disconnectedDevId = app_bt_get_devId_from_RemDev(connectedMobile);
    connectedMobile = NULL;
    app_bt_clear_connecting_profiles_state(disconnectedDevId);

    btif_remote_device_t *remote_dev =
        btif_me_get_callback_event_disconnect_rem_dev(Event);
    if (remote_dev) {
      uint16_t conhdl = btif_me_get_remote_device_hci_handle(remote_dev);
      bt_drv_reg_op_acl_tx_silence_clear(conhdl);
      bt_drv_hwspi_select(conhdl - 0x80, 0);
    }

    TRACE(5,
          "[BTEVENT] DISCONNECT evt = %d encryptState:%d reason:0x%02x/0x%02x "
          "activeCons:%d",
          btif_me_get_callback_event_type(Event),
          btif_me_get_remote_sevice_encrypt_state(
              btif_me_get_callback_event_rem_dev(Event)),
          btif_me_get_remote_device_disc_reason_saved(
              btif_me_get_callback_event_rem_dev(Event)),
          btif_me_get_remote_device_disc_reason(
              btif_me_get_callback_event_rem_dev(Event)),
          btif_me_get_activeCons());
    DUMP8("%02x ", btif_me_get_callback_event_rem_dev_bd_addr(Event),
          BTIF_BD_ADDR_SIZE);
#ifdef CHIP_BEST2000
    bt_drv_patch_force_disconnect_ack();
#endif
    // disconnect from reconnect connection, and the HF don't connect successful
    // once (whitch will release the saved_reconnect_mode ). so we are reconnect
    // fail with remote link key loss.
    // goto pairing.
    // reason 07 maybe from the controller's error .
    // 05  auth error
    // 16  io cap reject.

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
    if (btif_me_get_activeCons() == 0) {
      app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
    }
#endif

#if defined(BISTO_ENABLED)
    gsound_custom_bt_link_disconnected_handler(
        btif_me_get_callback_event_rem_dev_bd_addr(Event)->address);
#endif
#if defined(__AI_VOICE__)
    app_ai_mobile_disconnect_handle(
        btif_me_get_callback_event_rem_dev_bd_addr(Event));
#endif
#ifdef __IAG_BLE_INCLUDE__
    // start BLE adv
    app_ble_force_switch_adv(BLE_SWITCH_USER_BT_CONNECT, true);
#endif

#ifdef BTIF_DIP_DEVICE
    btif_dip_clear(remote_dev);
#endif

    app_bt_active_mode_reset(disconnectedDevId);

#ifdef GFPS_ENABLED
    app_gfps_handling_on_mobile_link_disconnection(
        btif_me_get_callback_event_rem_dev(Event));
#endif
#if !defined(IBRT) && defined(BT_XTAL_SYNC_NO_RESET)
    bt_term_xtal_sync_default();
#endif

    break;
  }
  case BTIF_BTEVENT_ROLE_CHANGE:
    TRACE(3,
          "[BTEVENT] ROLE_CHANGE eType:0x%x errCode:0x%x newRole:%d "
          "activeCons:%d",
          btif_me_get_callback_event_type(Event),
          btif_me_get_callback_event_err_code(Event),
          btif_me_get_callback_event_role_change_new_role(Event),
          btif_me_get_activeCons());
    break;
  case BTIF_BTEVENT_MODE_CHANGE:
    TRACE(4,
          "[BTEVENT] MODE_CHANGE evt:%d errCode:0x%0x curMode=0x%0x, "
          "interval=%d ",
          btif_me_get_callback_event_type(Event),
          btif_me_get_callback_event_err_code(Event),
          btif_me_get_callback_event_mode_change_curMode(Event),
          btif_me_get_callback_event_mode_change_interval(Event));
    DUMP8("%02x ", btif_me_get_callback_event_rem_dev_bd_addr(Event),
          BTIF_BD_ADDR_SIZE);
    break;
  case BTIF_BTEVENT_ACCESSIBLE_CHANGE:
    TRACE(3, "[BTEVENT] ACCESSIBLE_CHANGE evt:%d errCode:0x%0x aMode=0x%0x",
          btif_me_get_callback_event_type(Event),
          btif_me_get_callback_event_err_code(Event),
          btif_me_get_callback_event_a_mode(Event));
#if !defined(IBRT)
    if (app_is_access_mode_set_pending()) {
      app_set_pending_access_mode();
    } else {
      if (BTIF_BEC_NO_ERROR != btif_me_get_callback_event_err_code(Event)) {
        app_retry_setting_access_mode();
      }
    }
#endif
    break;
  case BTIF_BTEVENT_LINK_POLICY_CHANGED: {
    BT_SET_LINKPOLICY_REQ_T *pReq = app_bt_pop_pending_set_linkpolicy();
    if (NULL != pReq) {
      app_bt_Me_SetLinkPolicy(pReq->remDev, pReq->policy);
    }
    break;
  }
  case BTIF_BTEVENT_DEFAULT_LINK_POLICY_CHANGED: {
    TRACE(0, "[BTEVENT] DEFAULT_LINK_POLICY_CHANGED-->BT_STACK_INITIALIZED");
    app_notify_stack_ready(STACK_READY_BT);
    break;
  }
  case BTIF_BTEVENT_NAME_RESULT: {
    uint8_t *ptrName;
    uint8_t nameLen;
    nameLen = btif_me_get_callback_event_remote_dev_name(Event, &ptrName);
    TRACE(1, "[BTEVENT] NAME_RESULT name len %d", nameLen);
    if (nameLen > 0) {
      TRACE(1, "remote dev name: %s", ptrName);
    }
    // return;
  }
  default:
    break;
  }

#ifdef MULTIPOINT_DUAL_SLAVE
  app_bt_role_manager_process_dual_slave(Event);
#else
  app_bt_role_manager_process(Event);
#endif
  app_bt_accessible_manager_process(Event);
#if !defined(IBRT)
  app_bt_sniff_manager_process(Event);
#endif
  app_bt_global_handle_hook(Event);
#if defined(IBRT)
  app_tws_ibrt_global_callback(Event);
#endif
}

#include "app_bt_media_manager.h"
osTimerId bt_sco_recov_timer = NULL;
static void bt_sco_recov_timer_handler(void const *param);
osTimerDef(BT_SCO_RECOV_TIMER,
           (void (*)(void const *))bt_sco_recov_timer_handler); // define timers
void hfp_reconnect_sco(uint8_t flag);
static void bt_sco_recov_timer_handler(void const *param) {
  TRACE(1, "%s", __func__);
  hfp_reconnect_sco(0);
}
static void bt_sco_recov_timer_start() {
  osTimerStop(bt_sco_recov_timer);
  osTimerStart(bt_sco_recov_timer, 2500);
}

enum {
  SCO_DISCONNECT_RECONN_START,
  SCO_DISCONNECT_RECONN_RUN,
  SCO_DISCONNECT_RECONN_NONE,
};

static uint8_t sco_reconnect_status = SCO_DISCONNECT_RECONN_NONE;

void hfp_reconnect_sco(uint8_t set) {
  TRACE(3, "%s cur_chl_id=%d reconnect_status =%d", __func__,
        app_bt_device.curr_hf_channel_id, sco_reconnect_status);
  if (set == 1) {
    sco_reconnect_status = SCO_DISCONNECT_RECONN_START;
  }
  if (sco_reconnect_status == SCO_DISCONNECT_RECONN_START) {
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                  app_bt_device.curr_hf_channel_id,
                                  MAX_RECORD_NUM);
    app_bt_HF_DisconnectAudioLink(
        app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id]);
    sco_reconnect_status = SCO_DISCONNECT_RECONN_RUN;
    bt_sco_recov_timer_start();
  } else if (sco_reconnect_status == SCO_DISCONNECT_RECONN_RUN) {
    app_bt_HF_CreateAudioLink(
        app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id]);
    sco_reconnect_status = SCO_DISCONNECT_RECONN_NONE;
  }
}

void app_bt_global_handle_init(void) {
  btif_event_mask_t mask = BTIF_BEM_NO_EVENTS;
  btif_me_init_handler(&app_bt_handler);
  app_bt_handler.callback = app_bt_global_handle;
  btif_me_register_global_handler(&app_bt_handler);
#if defined(IBRT)
  btif_me_register_accept_handler(&app_bt_handler);
#endif
#ifdef IBRT_SEARCH_UI
  app_bt_global_handle_hook_set(APP_BT_GOLBAL_HANDLE_HOOK_USER_0,
                                app_bt_manager_ibrt_role_process);
#endif

  mask |= BTIF_BEM_ROLE_CHANGE | BTIF_BEM_SCO_CONNECT_CNF |
          BTIF_BEM_SCO_DISCONNECT | BTIF_BEM_SCO_CONNECT_IND;
  mask |= BTIF_BEM_AUTHENTICATED;
  mask |= BTIF_BEM_LINK_CONNECT_IND;
  mask |= BTIF_BEM_LINK_DISCONNECT;
  mask |= BTIF_BEM_LINK_CONNECT_CNF;
  mask |= BTIF_BEM_ACCESSIBLE_CHANGE;
  mask |= BTIF_BEM_ENCRYPTION_CHANGE;
  mask |= BTIF_BEM_SIMPLE_PAIRING_COMPLETE;
#if (defined(__BT_ONE_BRING_TWO__) || defined(IBRT))
  mask |= BTIF_BEM_MODE_CHANGE;
#endif
  mask |= BTIF_BEM_LINK_POLICY_CHANGED;

  app_bt_ME_SetConnectionRole(BTIF_BCR_ANY);
  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_stream[i]->a2dp_stream, FALSE);
#if defined(A2DP_LHDC_ON)
    app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_lhdc_stream[i]->a2dp_stream,
                              FALSE);
#endif
#if defined(A2DP_AAC_ON)
    app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_aac_stream[i]->a2dp_stream,
                              FALSE);
#endif
#if defined(A2DP_SCALABLE_ON)
    app_bt_A2DP_SetMasterRole(
        app_bt_device.a2dp_scalable_stream[i]->a2dp_stream, FALSE);
#endif
#if defined(A2DP_LDAC_ON)
    app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_ldac_stream[i]->a2dp_stream,
                              FALSE);
#endif

    app_bt_HF_SetMasterRole(app_bt_device.hf_channel[i], FALSE);
#if defined(__HSP_ENABLE__)
    HS_SetMasterRole(&app_bt_device.hs_channel[i], FALSE);
#endif
  }
  btif_me_set_event_mask(&app_bt_handler, mask);
  app_bt_sniff_manager_init();
  app_bt_accessmode_timer =
      osTimerCreate(osTimer(APP_BT_ACCESSMODE_TIMER), osTimerOnce,
                    &app_bt_accessmode_timer_argument);
  bt_sco_recov_timer =
      osTimerCreate(osTimer(BT_SCO_RECOV_TIMER), osTimerOnce, NULL);
}

void app_bt_send_request(uint32_t message_id, uint32_t param0, uint32_t param1,
                         uint32_t ptr) {
  APP_MESSAGE_BLOCK msg;

  msg.mod_id = APP_MODUAL_BT;
  msg.msg_body.message_id = message_id;
  msg.msg_body.message_Param0 = param0;
  msg.msg_body.message_Param1 = param1;
  msg.msg_body.message_ptr = ptr;
  app_mailbox_put(&msg);
}

extern void app_start_10_second_timer(uint8_t timer_id);

static int app_bt_handle_process(APP_MESSAGE_BODY *msg_body) {
  btif_accessible_mode_t old_access_mode;

  switch (msg_body->message_id) {
  case APP_BT_REQ_ACCESS_MODE_SET:
    old_access_mode = g_bt_access_mode;
    app_bt_accessmode_set(msg_body->message_Param0);
    if (msg_body->message_Param0 == BTIF_BAM_GENERAL_ACCESSIBLE &&
        old_access_mode != BTIF_BAM_GENERAL_ACCESSIBLE) {
      // app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
#ifdef MEDIA_PLAYER_SUPPORT
      app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
#endif
      app_start_10_second_timer(APP_PAIR_TIMER_ID);
    } else {
      // app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
    }
    break;
  default:
    break;
  }

  return 0;
}

void *app_bt_profile_active_store_ptr_get(uint8_t *bdAddr) {
  static btdevice_profile device_profile = {true, false, true, 0};
  btdevice_profile *ptr;

  nvrec_btdevicerecord *record = NULL;
  if (!nv_record_btdevicerecord_find((bt_bdaddr_t *)bdAddr, &record)) {
    uint32_t lock = nv_record_pre_write_operation();
    ptr = &(record->device_plf);
    DUMP8("0x%02x ", bdAddr, BTIF_BD_ADDR_SIZE);
    TRACE(5, "%s hfp_act:%d hsp_act:%d a2dp_act:0x%x codec_type=%x", __func__,
          ptr->hfp_act, ptr->hsp_act, ptr->a2dp_act, ptr->a2dp_codectype);
    /* always need connect a2dp and hfp */
    ptr->hfp_act = true;
    ptr->a2dp_act = true;
    nv_record_post_write_operation(lock);
  } else {
    ptr = &device_profile;
    TRACE(1, "%s default", __func__);
  }
  return (void *)ptr;
}

static void app_bt_profile_reconnect_timehandler(void const *param);
extern void startonce_delay_event_Timer_(int ms);
osTimerDef(BT_PROFILE_CONNECT_TIMER0,
           app_bt_profile_reconnect_timehandler); // define timers
#ifdef __BT_ONE_BRING_TWO__
osTimerDef(BT_PROFILE_CONNECT_TIMER1, app_bt_profile_reconnect_timehandler);
#endif

#ifdef __AUTO_CONNECT_OTHER_PROFILE__
static void app_bt_profile_connect_hf_retry_handler(void) {
  struct app_bt_profile_manager *bt_profile_manager_p =
      (struct app_bt_profile_manager *)param;
  if (MEC(pendCons) > 0) {
    TRACE(1, "Former link is not down yet, reset the timer %s.", __FUNCTION__);
    osTimerStart(bt_profile_manager_p->connect_timer,
                 APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
  } else {
    app_bt_precheck_before_starting_connecting(
        bt_profile_manager_p->has_connected);
    if (bt_profile_manager_p->hfp_connect !=
        bt_profile_connect_status_success) {
      app_bt_HF_CreateServiceLink(bt_profile_manager_p->chan,
                                  &bt_profile_manager_p->rmt_addr);
    }
  }
}

static void app_bt_profile_connect_hf_retry_timehandler(void const *param) {
  app_bt_start_custom_function_in_bt_thread(
      0, 0, (uint32_t)app_bt_profile_connect_hf_retry_handler);
}

#if defined(__HSP_ENABLE__)
static void app_bt_profile_connect_hs_retry_timehandler(void const *param) {
  struct app_bt_profile_manager *bt_profile_manager_p =
      (struct app_bt_profile_manager *)param;
  if (MEC(pendCons) > 0) {
    if (bt_profile_manager_p->reconnect_cnt <
        APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT) {
      bt_profile_manager_p->reconnect_cnt++;
    }
    TRACE(1, "Former link is not down yet, reset the timer %s.", __FUNCTION__);
    osTimerStart(bt_profile_manager_p->connect_timer,
                 BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS +
                     APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
  } else {
    if (bt_profile_manager_p->hsp_connect !=
        bt_profile_connect_status_success) {
      app_bt_HS_CreateServiceLink(bt_profile_manager_p->hs_chan,
                                  &bt_profile_manager_p->rmt_addr);
    }
  }
}
#endif

static bool app_bt_profile_manager_connect_a2dp_filter_connected_a2dp_stream(
    BT_BD_ADDR bd_addr) {
  uint8_t i = 0;
  BtRemoteDevice *StrmRemDev;
  A2dpStream *connected_stream;

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if ((app_bt_device.a2dp_stream[i].stream.state ==
             AVDTP_STRM_STATE_STREAMING ||
         app_bt_device.a2dp_stream[i].stream.state == AVDTP_STRM_STATE_OPEN)) {
      connected_stream = &app_bt_device.a2dp_stream[i];
      StrmRemDev = A2DP_GetRemoteDevice(connected_stream);
      if (memcmp(StrmRemDev->bdAddr.addr, bd_addr.addr, BD_ADDR_SIZE) == 0) {
        return true;
      }
    }
  }
  return false;
}

static void app_bt_profile_connect_a2dp_retry_handler(void) {
  struct app_bt_profile_manager *bt_profile_manager_p =
      (struct app_bt_profile_manager *)param;
  TRACE(1, "%s reconnect_cnt = %d", __func__,
        bt_profile_manager_p->reconnect_cnt);

  if (MEC(pendCons) > 0) {
    if (bt_profile_manager_p->reconnect_cnt <
        APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT) {
      bt_profile_manager_p->reconnect_cnt++;
    }
    TRACE(1, "Former link is not down yet, reset the timer %s.", __FUNCTION__);
    osTimerStart(bt_profile_manager_p->connect_timer,
                 BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS +
                     APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
  } else {
    if (app_bt_profile_manager_connect_a2dp_filter_connected_a2dp_stream(
            bt_profile_manager_p->rmt_addr) == true) {
      TRACE(0, "has been connected , no need to init connect again");
      return;
    }
    app_bt_precheck_before_starting_connecting(
        bt_profile_manager_p->has_connected);
    if (bt_profile_manager_p->a2dp_connect !=
        bt_profile_connect_status_success) {
      app_bt_A2DP_OpenStream(bt_profile_manager_p->stream,
                             &bt_profile_manager_p->rmt_addr);
    }
  }
}

static void app_bt_profile_connect_a2dp_retry_timehandler(void const *param) {
  app_bt_start_custom_function_in_bt_thread(
      0, 0, (uint32_t)app_bt_profile_connect_a2dp_retry_handler);
}
#endif

void app_bt_reset_reconnect_timer(bt_bdaddr_t *pBdAddr) {
  uint8_t devId = 0;
  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    if (pBdAddr == &(bt_profile_manager[i].rmt_addr)) {
      devId = i;
      break;
    }
  }

  TRACE(1, "Resart the reconnecting timer of dev %d", devId);
  osTimerStart(bt_profile_manager[devId].connect_timer,
               BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS +
                   APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
}

static void app_bt_profile_reconnect_handler(void const *param) {
#if !defined(IBRT)
  struct app_bt_profile_manager *bt_profile_manager_p =
      (struct app_bt_profile_manager *)param;
  TRACE(1, "%s reconnect_cnt = %d", __FUNCTION__,
        bt_profile_manager_p->reconnect_cnt);

  if (btif_me_get_pendCons() > 0) {
    if (bt_profile_manager_p->reconnect_cnt <
        APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT) {
      bt_profile_manager_p->reconnect_cnt++;
    }
    TRACE(1, "Former link is not down yet, reset the timer %s.", __FUNCTION__);
    osTimerStart(bt_profile_manager_p->connect_timer,
                 BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS +
                     APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
  } else {
    btdevice_profile *btdevice_plf_p =
        (btdevice_profile *)app_bt_profile_active_store_ptr_get(
            bt_profile_manager_p->rmt_addr.address);
#ifdef __BT_ONE_BRING_TWO__
    if (a2dp_is_music_ongoing() &&
        (bt_profile_manager_p->has_connected == false)) {
      bt_drv_reg_op_set_reconnecting_flag();
      a2dp_update_music_link();
    }
#endif

    if (bt_profile_manager_p->connect_timer_cb) {
      bt_profile_manager_p->connect_timer_cb(param);
      bt_profile_manager_p->connect_timer_cb = NULL;
    } else {
      if ((btdevice_plf_p->hfp_act) && (bt_profile_manager_p->hfp_connect !=
                                        bt_profile_connect_status_success)) {
        TRACE(0, "try connect hf");
        app_bt_precheck_before_starting_connecting(
            bt_profile_manager_p->has_connected);
        app_bt_HF_CreateServiceLink(
            bt_profile_manager_p->chan,
            (bt_bdaddr_t *)&bt_profile_manager_p->rmt_addr);
      }
#if defined(__HSP_ENABLE__)
      else if (btdevice_plf_p->hsp_act)
                &&(bt_profile_manager_p->hsp_connect != bt_profile_connect_status_success))
            {
                  TRACE(0, "try connect hs");
                  app_bt_precheck_before_starting_connecting(
                      bt_profile_manager_p->has_connected);
                  app_bt_HS_CreateServiceLink(bt_profile_manager_p->hs_chan,
                                              &bt_profile_manager_p->rmt_addr);
                }
#endif
      else if ((btdevice_plf_p->a2dp_act) &&
               (bt_profile_manager_p->a2dp_connect !=
                bt_profile_connect_status_success)) {
                TRACE(0, "try connect a2dp");
                app_bt_precheck_before_starting_connecting(
                    bt_profile_manager_p->has_connected);
                app_bt_A2DP_OpenStream(bt_profile_manager_p->stream,
                                       &bt_profile_manager_p->rmt_addr);
      }
    }
  }
#else
  TRACE(0, "ibrt_ui_log:app_bt_profile_reconnect_timehandler called");
#endif
}

static void app_bt_profile_reconnect_timehandler(void const *param) {
  app_bt_start_custom_function_in_bt_thread(
      (uint32_t)param, 0, (uint32_t)app_bt_profile_reconnect_handler);
}

bool app_bt_is_in_connecting_profiles_state(void) {
  for (uint8_t devId = 0; devId < BT_DEVICE_NUM; devId++) {
    if (APP_BT_IN_CONNECTING_PROFILES_STATE ==
        bt_profile_manager[devId].connectingState) {
      return true;
    }
  }

  return false;
}

void app_bt_clear_connecting_profiles_state(uint8_t devId) {
  TRACE(1, "Dev %d exists connecting profiles state", devId);

  bt_profile_manager[devId].connectingState = APP_BT_IDLE_STATE;
  if (!app_bt_is_in_connecting_profiles_state()) {
#ifdef __IAG_BLE_INCLUDE__
    app_start_fast_connectable_ble_adv(BLE_FAST_ADVERTISING_INTERVAL);
#endif
  }
}

void app_bt_set_connecting_profiles_state(uint8_t devId) {
  TRACE(1, "Dev %d enters connecting profiles state", devId);

  bt_profile_manager[devId].connectingState =
      APP_BT_IN_CONNECTING_PROFILES_STATE;
}

void app_bt_profile_connect_manager_open(void) {
  uint8_t i = 0;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    bt_profile_manager[i].has_connected = false;
    bt_profile_manager[i].hfp_connect = bt_profile_connect_status_unknow;
    bt_profile_manager[i].hsp_connect = bt_profile_connect_status_unknow;
    bt_profile_manager[i].a2dp_connect = bt_profile_connect_status_unknow;
    memset(bt_profile_manager[i].rmt_addr.address, 0, BTIF_BD_ADDR_SIZE);
    bt_profile_manager[i].reconnect_mode = bt_profile_reconnect_null;
    bt_profile_manager[i].saved_reconnect_mode = bt_profile_reconnect_null;
    bt_profile_manager[i].stream = NULL;
    bt_profile_manager[i].chan = NULL;
#if defined(__HSP_ENABLE__)
    bt_profile_manager[i].hs_chan = NULL;
#endif
    bt_profile_manager[i].reconnect_cnt = 0;
    bt_profile_manager[i].connect_timer_cb = NULL;
    bt_profile_manager[i].connectingState = APP_BT_IDLE_STATE;
  }

  bt_profile_manager[BT_DEVICE_ID_1].connect_timer =
      osTimerCreate(osTimer(BT_PROFILE_CONNECT_TIMER0), osTimerOnce,
                    &bt_profile_manager[BT_DEVICE_ID_1]);
#ifdef __BT_ONE_BRING_TWO__
  bt_profile_manager[BT_DEVICE_ID_2].connect_timer =
      osTimerCreate(osTimer(BT_PROFILE_CONNECT_TIMER1), osTimerOnce,
                    &bt_profile_manager[BT_DEVICE_ID_2]);
#endif
}

BOOL app_bt_profile_connect_openreconnecting(void *ptr) {
  bool nRet = false;
  uint8_t i;

  /*
   * If launched from peer device,stop reconnecting and accept connection
   */
  if ((ptr != NULL) && (btif_me_get_remote_device_initiator(
                            (btif_remote_device_t *)ptr) == FALSE)) {
    // Peer device launch reconnet,then we give up reconnect procedure
    TRACE(0, "give up reconnecting");
    app_bt_connectable_mode_stop_reconnecting();
    return false;
  }

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    nRet |= bt_profile_manager[i].reconnect_mode ==
                    bt_profile_reconnect_openreconnecting
                ? true
                : false;
    if (nRet) {
      TRACE(2, "io cap rj [%d]: %d", i, bt_profile_manager[i].reconnect_mode);
    }
  }

  return nRet;
}

bool app_bt_is_in_reconnecting(void) {
  uint8_t devId;
  for (devId = 0; devId < BT_DEVICE_NUM; devId++) {
    if (bt_profile_reconnect_null != bt_profile_manager[devId].reconnect_mode) {
      return true;
    }
  }

  return false;
}

void app_bt_profile_connect_manager_opening_reconnect(void) {
  int ret;
  btif_device_record_t record1;
  btif_device_record_t record2;
  btdevice_profile *btdevice_plf_p;
  int find_invalid_record_cnt;
#if defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)
  if (app_bt_device.src_or_snk == BT_DEVICE_SRC) {
    return;
  }
#endif
  osapi_lock_stack();

#ifndef __BT_ONE_BRING_TWO__
  if (btif_me_get_activeCons() != 0) {
    osapi_unlock_stack();
    TRACE(0, "bt link disconnect not complete,ignore this time reconnect");
    return;
  }
#endif

  do {
    find_invalid_record_cnt = 0;
    ret = nv_record_enum_latest_two_paired_dev(&record1, &record2);
    if (ret == 1) {
      btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          record1.bdAddr.address);
      if (!(btdevice_plf_p->hfp_act) && !(btdevice_plf_p->a2dp_act)) {
                nv_record_ddbrec_delete((bt_bdaddr_t *)&record1.bdAddr);
                find_invalid_record_cnt++;
      }
    } else if (ret == 2) {
      btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          record1.bdAddr.address);
      if (!(btdevice_plf_p->hfp_act) && !(btdevice_plf_p->a2dp_act)) {
                nv_record_ddbrec_delete((bt_bdaddr_t *)&record1.bdAddr);
                find_invalid_record_cnt++;
      }
      btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          record2.bdAddr.address);
      if (!(btdevice_plf_p->hfp_act) && !(btdevice_plf_p->a2dp_act)) {
                nv_record_ddbrec_delete((bt_bdaddr_t *)&record2.bdAddr);
                find_invalid_record_cnt++;
      }
    }
  } while (find_invalid_record_cnt);

  TRACE(0, "!!!app_bt_opening_reconnect:\n");
  DUMP8("%02x ", &record1.bdAddr, 6);
  DUMP8("%02x ", &record2.bdAddr, 6);

  if (ret > 0) {
    TRACE(0, "!!!start reconnect first device\n");

    if (btif_me_get_pendCons() == 0) {
      bt_profile_manager[BT_DEVICE_ID_1].reconnect_mode =
          bt_profile_reconnect_openreconnecting;
      bt_profile_manager[BT_DEVICE_ID_1].reconnect_cnt = 0;
      memcpy(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address,
             record1.bdAddr.address, BTIF_BD_ADDR_SIZE);
      btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address);

#if defined(A2DP_LHDC_ON)
      if (btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                bt_profile_manager[BT_DEVICE_ID_1].stream =
                    app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream;
      else
#endif
#if defined(A2DP_AAC_ON)
          if (btdevice_plf_p->a2dp_codectype ==
              BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
                bt_profile_manager[BT_DEVICE_ID_1].stream =
                    app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;
      else
#endif
#if defined(A2DP_LDAC_ON) // workaround for mate10 no a2dp issue when link back
                if (btdevice_plf_p->a2dp_codectype ==
                    BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
                  // bt_profile_manager[BT_DEVICE_ID_1].stream =
                  // app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;
                  // btdevice_plf_p->a2dp_codectype =
                  // BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
                  bt_profile_manager[BT_DEVICE_ID_1].stream =
                      app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]
                          ->a2dp_stream;
                  btdevice_plf_p->a2dp_codectype =
                      BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
                } else
#endif

#if defined(A2DP_SCALABLE_ON)
                    if (btdevice_plf_p->a2dp_codectype ==
                        BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                  bt_profile_manager[BT_DEVICE_ID_1].stream =
                      app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]
                          ->a2dp_stream;
                else
#endif
                {
                  bt_profile_manager[BT_DEVICE_ID_1].stream =
                      app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
                }

      btif_a2dp_reset_stream_state(bt_profile_manager[BT_DEVICE_ID_1].stream);

      bt_profile_manager[BT_DEVICE_ID_1].chan =
          app_bt_device.hf_channel[BT_DEVICE_ID_1];
#if defined(__HSP_ENABLE__)
      bt_profile_manager[BT_DEVICE_ID_1].hs_chan =
          &app_bt_device.hs_channel[BT_DEVICE_ID_1];
#endif
      if (btdevice_plf_p->hfp_act) {
                TRACE(0, "try connect hf");
                app_bt_precheck_before_starting_connecting(
                    bt_profile_manager[BT_DEVICE_ID_1].has_connected);
                app_bt_HF_CreateServiceLink(
                    bt_profile_manager[BT_DEVICE_ID_1].chan,
                    (bt_bdaddr_t *)&bt_profile_manager[BT_DEVICE_ID_1]
                        .rmt_addr);
      } else if (btdevice_plf_p->a2dp_act) {
                TRACE(0, "try connect a2dp");
                app_bt_precheck_before_starting_connecting(
                    bt_profile_manager[BT_DEVICE_ID_1].has_connected);
                app_bt_A2DP_OpenStream(
                    bt_profile_manager[BT_DEVICE_ID_1].stream,
                    &bt_profile_manager[BT_DEVICE_ID_1].rmt_addr);
      }

#if defined(__HSP_ENABLE__)
      else if (btdevice_plf_p->hsp_act) {
                TRACE(0, "try connect hs");
                app_bt_precheck_before_starting_connecting(
                    bt_profile_manager[BT_DEVICE_ID_1].has_connected);
                app_bt_HS_CreateServiceLink(
                    bt_profile_manager[BT_DEVICE_ID_1].hs_chan,
                    &bt_profile_manager[BT_DEVICE_ID_1].rmt_addr);
      }
#endif
    }
#ifdef __BT_ONE_BRING_TWO__
    if (ret > 1) {
      TRACE(0, "!!!need reconnect second device\n");
      bt_profile_manager[BT_DEVICE_ID_2].reconnect_mode =
          bt_profile_reconnect_openreconnecting;
      bt_profile_manager[BT_DEVICE_ID_2].reconnect_cnt = 0;
      memcpy(bt_profile_manager[BT_DEVICE_ID_2].rmt_addr.address,
             record2.bdAddr.address, BTIF_BD_ADDR_SIZE);
      btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          bt_profile_manager[BT_DEVICE_ID_2].rmt_addr.address);

#if defined(A2DP_LHDC_ON)
      if (btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                bt_profile_manager[BT_DEVICE_ID_2].stream =
                    app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream;
      else
#endif
#if defined(A2DP_AAC_ON)
          if (btdevice_plf_p->a2dp_codectype ==
              BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
                bt_profile_manager[BT_DEVICE_ID_2].stream =
                    app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream;
      else
#endif
#if defined(A2DP_SCALABLE_ON)
          if (btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                bt_profile_manager[BT_DEVICE_ID_2].stream =
                    app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]
                        ->a2dp_stream;
      else
#endif
      {
                bt_profile_manager[BT_DEVICE_ID_2].stream =
                    app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream;
      }

      btif_a2dp_reset_stream_state(bt_profile_manager[BT_DEVICE_ID_2].stream);

      bt_profile_manager[BT_DEVICE_ID_2].chan =
          app_bt_device.hf_channel[BT_DEVICE_ID_2];
#if defined(__HSP_ENABLE__)
      bt_profile_manager[BT_DEVICE_ID_2].hs_chan =
          &app_bt_device.hs_channel[BT_DEVICE_ID_2];
#endif
    }
#endif
  }

  else {
    TRACE(0, "!!!go to pairing\n");
#ifdef __EARPHONE_STAY_BOTH_SCAN__
    app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#else
    app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif
  }
  osapi_unlock_stack();
}

void app_bt_resume_sniff_mode(uint8_t deviceId) {
  if (bt_profile_connect_status_success ==
          bt_profile_manager[deviceId].a2dp_connect ||
      bt_profile_connect_status_success ==
          bt_profile_manager[deviceId].hfp_connect ||
      bt_profile_connect_status_success ==
          bt_profile_manager[deviceId].hsp_connect) {
    app_bt_allow_sniff(deviceId);
    btif_remote_device_t *currentRemDev = app_bt_get_remoteDev(deviceId);
    app_bt_sniff_config(currentRemDev);
  }
}
#if !defined(IBRT)
static int8_t app_bt_profile_reconnect_pending(enum BT_DEVICE_ID_T id) {
  if (btapp_hfp_is_dev_call_active(id) == true) {
    bt_profile_manager[id].reconnect_mode =
        bt_profile_reconnect_reconnect_pending;
    return 0;
  }
  return -1;
}
#endif
static int8_t app_bt_profile_reconnect_pending_process(void) {
  uint8_t i = BT_DEVICE_NUM;

  btif_remote_device_t *remDev = NULL;
  btif_cmgr_handler_t *cmgrHandler;

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    remDev = btif_me_enumerate_remote_devices(i);
    if (remDev != NULL) {
      cmgrHandler = btif_cmgr_get_acl_handler(remDev);
      if (btif_cmgr_is_audio_up(cmgrHandler) == 1)
                return -1;
    }
  }
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (bt_profile_manager[i].reconnect_mode ==
        bt_profile_reconnect_reconnect_pending)
      break;
  }

  if (i == BT_DEVICE_NUM)
    return -1;

  bt_profile_manager[i].reconnect_mode = bt_profile_reconnect_reconnecting;
#ifdef __IAG_BLE_INCLUDE__
  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
  osTimerStart(bt_profile_manager[i].connect_timer,
               APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
  return 0;
}

uint8_t app_bt_get_num_of_connected_dev(void) {
  uint8_t num_of_connected_dev = 0;
  uint8_t deviceId;

  for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++) {
    if (bt_profile_manager[deviceId].has_connected) {
      num_of_connected_dev++;
    }
  }

  return num_of_connected_dev;
}

static uint8_t recorded_latest_connected_service_device_id = BT_DEVICE_ID_1;
void app_bt_record_latest_connected_service_device_id(uint8_t device_id) {
  recorded_latest_connected_service_device_id = device_id;
}

uint8_t app_bt_get_recorded_latest_connected_service_device_id(void) {
  return recorded_latest_connected_service_device_id;
}

static void app_bt_precheck_before_starting_connecting(uint8_t isBtConnected) {
#ifdef __IAG_BLE_INCLUDE__
  if (!isBtConnected) {
    app_ble_force_switch_adv(BLE_SWITCH_USER_BT_CONNECT, false);
  }
#endif
}

static void app_bt_restore_reconnecting_idle_mode(uint8_t deviceId) {
  bt_profile_manager[deviceId].reconnect_mode = bt_profile_reconnect_null;
#ifdef __IAG_BLE_INCLUDE__
  app_start_fast_connectable_ble_adv(BLE_FAST_ADVERTISING_INTERVAL);
#endif
}

#ifdef __BT_ONE_BRING_TWO__
static void app_bt_update_connectable_mode_after_connection_management(void) {
  uint8_t deviceId;
  bool isEnterConnetableOnlyState = true;
  for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++) {
    // assure none of the device is in reconnecting mode
    if (bt_profile_manager[deviceId].reconnect_mode !=
        bt_profile_reconnect_null) {
      isEnterConnetableOnlyState = false;
      break;
    }
  }

  if (isEnterConnetableOnlyState) {
    for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++) {
      if (!bt_profile_manager[deviceId].has_connected) {
                app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                return;
      }
    }
  }
}
#endif

static void app_bt_connectable_mode_stop_reconnecting_handler(void) {
  uint8_t deviceId;
  btif_remote_device_t *remDev;
  btif_cmgr_handler_t *cmgrHandler;
  for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++) {
    if (bt_profile_manager[deviceId].reconnect_mode !=
        bt_profile_reconnect_null) {
      bt_profile_manager[deviceId].hfp_connect =
          bt_profile_connect_status_failure;
      bt_profile_manager[deviceId].reconnect_mode = bt_profile_reconnect_null;
      bt_profile_manager[deviceId].saved_reconnect_mode =
          bt_profile_reconnect_null;
      bt_profile_manager[deviceId].reconnect_cnt = 0;
      if (bt_profile_manager[deviceId].connect_timer != NULL)
                osTimerStop(bt_profile_manager[deviceId].connect_timer);
      remDev = btif_me_enumerate_remote_devices(deviceId);
      if (remDev != NULL) {
                cmgrHandler = btif_cmgr_get_acl_handler(remDev);
                btif_me_cancel_create_link(
                    btif_cmgr_get_cmgrhandler_remdev_bthandle(cmgrHandler),
                    remDev);
      }
    }
  }
}

void app_bt_connectable_mode_stop_reconnecting(void) {
  app_bt_start_custom_function_in_bt_thread(
      0, 0, (uint32_t)app_bt_connectable_mode_stop_reconnecting_handler);
}

#if defined(__HSP_ENABLE__)
void app_bt_profile_connect_manager_hs(enum BT_DEVICE_ID_T id, HsChannel *Chan,
                                       HsCallbackParms *Info) {
  btdevice_profile *btdevice_plf_p =
      (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          (uint8_t *)Info->p.remDev->bdAddr.address);

  osTimerStop(bt_profile_manager[id].connect_timer);
  bt_profile_manager[id].connect_timer_cb = NULL;
  bool profile_reconnect_enable = false;

  if (Chan && Info) {
    switch (Info->event) {
    case HF_EVENT_SERVICE_CONNECTED:
      TRACE(1, "%s HS_EVENT_SERVICE_CONNECTED", __func__);
      nv_record_btdevicerecord_set_hsp_profile_active_state(btdevice_plf_p,
                                                            true);
      nv_record_touch_cause_flush();
      bt_profile_manager[id].hsp_connect = bt_profile_connect_status_success;
      bt_profile_manager[id].reconnect_cnt = 0;
      bt_profile_manager[id].hs_chan = &app_bt_device.hs_channel[id];
      memcpy(bt_profile_manager[id].rmt_addr.address,
             Info->p.remDev->bdAddr.address, BTIF_BD_ADDR_SIZE);
      if (false == bt_profile_manager[id].has_connected) {
                app_bt_resume_sniff_mode(id);
      }
      if (bt_profile_manager[id].reconnect_mode ==
          bt_profile_reconnect_openreconnecting) {
                // do nothing
      } else if (bt_profile_manager[id].reconnect_mode ==
                 bt_profile_reconnect_reconnecting) {
                if (btdevice_plf_p->a2dp_act &&
                    bt_profile_manager[id].a2dp_connect !=
                        bt_profile_connect_status_success) {
                  TRACE(0, "!!!continue connect a2dp\n");
                  app_bt_precheck_before_starting_connecting(
                      bt_profile_manager[id].has_connected);
                  app_bt_A2DP_OpenStream(bt_profile_manager[id].stream,
                                         &bt_profile_manager[id].rmt_address);
                }
      }
#ifdef __AUTO_CONNECT_OTHER_PROFILE__
      else {
                if (btdevice_plf_p->a2dp_act &&
                    bt_profile_manager[id].a2dp_connect !=
                        bt_profile_connect_status_success) {
                  bt_profile_manager[id].connect_timer_cb =
                      app_bt_profile_connect_a2dp_retry_timehandler;
                  app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                  osTimerStart(bt_profile_manager[id].connect_timer,
                               APP_BT_PROFILE_CONNECT_RETRY_MS);
                }
      }
#endif
      break;
    case HF_EVENT_SERVICE_DISCONNECTED:
      TRACE(2, "%s HS_EVENT_SERVICE_DISCONNECTED discReason:%d", __func__,
            Info->p.remDev->discReason);
      bt_profile_manager[id].hsp_connect = bt_profile_connect_status_failure;
      if (bt_profile_manager[id].reconnect_mode ==
          bt_profile_reconnect_openreconnecting) {
                if (++bt_profile_manager[id].reconnect_cnt <
                    APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT) {
                  app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                  profile_reconnect_enable = true;
                  bt_profile_manager[id].hfp_connect =
                      bt_profile_connect_status_unknow;
                }
      } else if (bt_profile_manager[id].reconnect_mode ==
                 bt_profile_reconnect_reconnecting) {
                if (++bt_profile_manager[id].reconnect_cnt <
                    APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT) {
                  app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                  profile_reconnect_enable = true
                } else {
                  app_bt_restore_reconnecting_idle_mode(id);
                  // bt_profile_manager[id].reconnect_mode =
                  // bt_profile_reconnect_null;
                }
                TRACE(2, "%s try to reconnect cnt:%d", __func__,
                      bt_profile_manager[id].reconnect_cnt);
#if !defined(IBRT)
      } else if (Info->p.remDev->discReason == 0x8) {
                bt_profile_manager[id].reconnect_mode =
                    bt_profile_reconnect_reconnecting;
                app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                TRACE(1, "%s try to reconnect", __func__);
                if (app_bt_profile_reconnect_pending(id) != 0) {
                  profile_reconnect_enable = true;
                }
#endif
      } else {
                bt_profile_manager[id].hsp_connect =
                    bt_profile_connect_status_unknow;
      }

      if (profile_reconnect_enable) {
#ifdef __IAG_BLE_INCLUDE__
                app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
                osTimerStart(bt_profile_manager[id].connect_timer,
                             APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
      }
      break;
    default:
      break;
    }
  }

  if (bt_profile_manager[id].reconnect_mode ==
      bt_profile_reconnect_reconnecting) {
    bool reconnect_hsp_proc_final = true;
    bool reconnect_a2dp_proc_final = true;
    if (bt_profile_manager[id].hsp_connect ==
        bt_profile_connect_status_failure) {
      reconnect_hsp_proc_final = false;
    }
    if (bt_profile_manager[id].a2dp_connect ==
        bt_profile_connect_status_failure) {
      reconnect_a2dp_proc_final = false;
    }
    if (reconnect_hsp_proc_final && reconnect_a2dp_proc_final) {
      TRACE(3, "!!!reconnect success %d/%d/%d\n",
            bt_profile_manager[id].hfp_connect,
            bt_profile_manager[id].hsp_connect,
            bt_profile_manager[id].a2dp_connect);
      app_bt_restore_reconnecting_idle_mode(id);
      // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
    }
  } else if (bt_profile_manager[id].reconnect_mode ==
             bt_profile_reconnect_openreconnecting) {
    bool opening_hsp_proc_final = false;
    bool opening_a2dp_proc_final = false;

    if (btdevice_plf_p->hsp_act && bt_profile_manager[id].hsp_connect ==
                                       bt_profile_connect_status_unknow) {
      opening_hsp_proc_final = false;
    } else {
      opening_hsp_proc_final = true;
    }

    if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect ==
                                        bt_profile_connect_status_unknow) {
      opening_a2dp_proc_final = false;
    } else {
      opening_a2dp_proc_final = true;
    }

    if ((opening_hsp_proc_final && opening_a2dp_proc_final) ||
        (bt_profile_manager[id].hsp_connect ==
         bt_profile_connect_status_failure)) {
      TRACE(3, "!!!reconnect success %d/%d/%d\n",
            bt_profile_manager[id].hfp_connect,
            bt_profile_manager[id].hsp_connect,
            bt_profile_manager[id].a2dp_connect);
      app_bt_restore_reconnecting_idle_mode(id);
      // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
    }

    if (btdevice_plf_p->hsp_act && bt_profile_manager[id].hsp_connect ==
                                       bt_profile_connect_status_success) {
      if (btdevice_plf_p->a2dp_act && !opening_a2dp_proc_final) {
                TRACE(0, "!!!continue connect a2dp\n");
                app_bt_precheck_before_starting_connecting(
                    bt_profile_manager[id].has_connected);
                app_bt_A2DP_OpenStream(bt_profile_manager[id].stream,
                                       &bt_profile_manager[id].rmt_addr);
      }
    }

    if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_null) {
      for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
                if (bt_profile_manager[i].reconnect_mode ==
                    bt_profile_reconnect_openreconnecting) {
                  TRACE(0, "!!!hs->start reconnect second device\n");
                  if ((btdevice_plf_p->hfp_act) &&
                      (!bt_profile_manager[i].hfp_connect)) {
                    TRACE(0, "try connect hf");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[i].has_connected);
                    app_bt_HF_CreateServiceLink(
                        bt_profile_manager[i].chan,
                        &bt_profile_manager[i].rmt_addr);
                  } else if ((btdevice_plf_p->hsp_act) &&
                             (!bt_profile_manager[i].hsp_connect)) {
                    TRACE(0, "try connect hs");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[i].has_connected);
                    app_bt_HS_CreateServiceLink(
                        bt_profile_manager[i].hs_chan,
                        &bt_profile_manager[i].rmt_addr);

                  } else if ((btdevice_plf_p->a2dp_act) &&
                             (!bt_profile_manager[i].a2dp_connect)) {
                    TRACE(0, "try connect a2dp");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[i].has_connected);
                    app_bt_A2DP_OpenStream(bt_profile_manager[i].stream,
                                           &bt_profile_manager[i].rmt_addr);
                  }
                  break;
                }
      }
    }
  }

#ifdef __IAG_BLE_INCLUDE__
  if (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success &&
      bt_profile_manager[id].hsp_connect == bt_profile_connect_status_success &&
      bt_profile_manager[id].a2dp_connect ==
          bt_profile_connect_status_success) {
    app_ble_force_switch_adv(BLE_SWITCH_USER_BT_CONNECT, true);
  }
#endif

  if (!bt_profile_manager[id].has_connected &&
      (bt_profile_manager[id].hfp_connect ==
           bt_profile_connect_status_success ||
       bt_profile_manager[id].hsp_connect ==
           bt_profile_connect_status_success ||
       bt_profile_manager[id].a2dp_connect ==
           bt_profile_connect_status_success)) {

    bt_profile_manager[id].has_connected = true;
    TRACE(0, "BT connected!!!");

#ifndef IBRT
    btif_me_get_remote_device_name(&(ctx->remote_dev_bdaddr),
                                   app_bt_global_handle);
#endif
#if defined(MEDIA_PLAYER_SUPPORT) && !defined(IBRT)
    app_voice_report(APP_STATUS_INDICATION_CONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
    app_interconnection_start_disappear_adv(
        INTERCONNECTION_BLE_ADVERTISING_INTERVAL,
        APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);

    if (btif_me_get_activeCons() <= 2) {
      app_interconnection_spp_open(btif_me_enumerate_remote_devices(id));
    }
#endif
#ifdef __INTERACTION__
    // app_interaction_spp_open();
#endif
  }

  if (bt_profile_manager[id].has_connected &&
      (bt_profile_manager[id].hfp_connect !=
           bt_profile_connect_status_success &&
       bt_profile_manager[id].hsp_connect !=
           bt_profile_connect_status_success &&
       bt_profile_manager[id].a2dp_connect !=
           bt_profile_connect_status_success)) {

    bt_profile_manager[id].has_connected = false;
    TRACE(0, "BT disconnected!!!");

#ifdef GFPS_ENABLED
    if (app_gfps_is_last_response_pending()) {
      app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
    }
#endif

#if defined(MEDIA_PLAYER_SUPPORT) && !defined(IBRT)
    app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
    app_interconnection_disconnected_callback();
#endif

    app_set_disconnecting_all_bt_connections(false);
  }

#ifdef __BT_ONE_BRING_TWO__
  app_bt_update_connectable_mode_after_connection_management();
#endif
}
#endif

void hfp_reconnecting_timer_stop_callback(const btif_event_t *event) {
  uint8_t i = 0;
  uint8_t id = BT_DEVICE_NUM;
  bt_bdaddr_t *remote = NULL;
  bt_bdaddr_t *hfp_remote = NULL;
  remote = btif_me_get_callback_event_rem_dev_bd_addr(event);
  if (remote != NULL) {
    for (i = 0; i < BT_DEVICE_NUM; i++) {
      hfp_remote = &bt_profile_manager[i].rmt_addr;
      if (!strcmp((char *)hfp_remote, (char *)remote)) {
                id = i;
                TRACE(2, "%s: find bt device num = %d", __func__, id);
                break;
      }
    }
  }
  if (i < BT_DEVICE_NUM) {
    TRACE(3, "%s: hfp_connect=%d,reconnect_mode=%d,reconnect_cnt=%d", __func__,
          bt_profile_manager[id].hfp_connect,
          bt_profile_manager[id].reconnect_mode,
          bt_profile_manager[id].reconnect_cnt);
    if ((bt_profile_manager[id].reconnect_mode != bt_profile_reconnect_null) &&
        bt_profile_manager[id].reconnect_cnt != 0) {
      bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
      bt_profile_manager[id].saved_reconnect_mode = bt_profile_reconnect_null;
      bt_profile_manager[id].reconnect_cnt = 0;
      if (bt_profile_manager[id].connect_timer != NULL)
                osTimerStop(bt_profile_manager[id].connect_timer);

      TRACE(1, "%s: stop success", __func__);
    }
  } else {
    TRACE(1, "%s: not find bt device", __func__);
  }
}

void app_audio_switch_flash_flush_req(void);

extern uint8_t once_event_case;
extern bool IsMobileLinkLossing;
// void app_bt_profile_connect_manager_hf(enum BT_DEVICE_ID_T id, HfChannel
// *Chan, HfCallbackParms *Info)
void app_bt_profile_connect_manager_hf(enum BT_DEVICE_ID_T id,
                                       hf_chan_handle_t Chan,
                                       struct hfp_context *ctx) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
  // btdevice_profile *btdevice_plf_p = (btdevice_profile
  // *)app_bt_profile_active_store_ptr_get((uint8_t
  // *)Info->p.remDev->bdAddr.address);
  btdevice_profile *btdevice_plf_p =
      (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          (uint8_t *)ctx->remote_dev_bdaddr.address);
  bool profile_reconnect_enable = false;

  osTimerStop(bt_profile_manager[id].connect_timer);
  bt_profile_manager[id].connect_timer_cb = NULL;
  // if (Chan&&Info){
  if (Chan) {
    switch (ctx->event) {
    case BTIF_HF_EVENT_SERVICE_CONNECTED:
      TRACE(1, "%s HF_EVENT_SERVICE_CONNECTED", __func__);
      nv_record_btdevicerecord_set_hfp_profile_active_state(btdevice_plf_p,
                                                            true);
      nv_record_touch_cause_flush();
      bt_profile_manager[id].hfp_connect = bt_profile_connect_status_success;
      bt_profile_manager[id].saved_reconnect_mode = bt_profile_reconnect_null;
      bt_profile_manager[id].reconnect_cnt = 0;
      bt_profile_manager[id].chan = app_bt_device.hf_channel[id];
      memcpy(bt_profile_manager[id].rmt_addr.address,
             ctx->remote_dev_bdaddr.address, BTIF_BD_ADDR_SIZE);
      if (false == bt_profile_manager[id].has_connected) {
                app_bt_resume_sniff_mode(id);
      }

#ifdef BTIF_DIP_DEVICE
      btif_dip_get_remote_info(app_bt_get_remoteDev(id));
#endif

      if (bt_profile_manager[id].reconnect_mode ==
          bt_profile_reconnect_openreconnecting) {
                // do nothing
      }
#if defined(IBRT)
      else if (app_bt_ibrt_reconnect_mobile_profile_flag_get()) {
                app_bt_ibrt_reconnect_mobile_profile_flag_clear();
#else
      else if (bt_profile_manager[id].reconnect_mode ==
               bt_profile_reconnect_reconnecting) {
#endif
                TRACE(2, "app_bt: a2dp_act in NV =%d,a2dp_connect=%d",
                      btdevice_plf_p->a2dp_act,
                      bt_profile_manager[id].a2dp_connect);
                if (btdevice_plf_p->a2dp_act &&
                    bt_profile_manager[id].a2dp_connect !=
                        bt_profile_connect_status_success) {
                  TRACE(0, "!!!continue connect a2dp\n");
                  app_bt_precheck_before_starting_connecting(
                      bt_profile_manager[id].has_connected);
                  app_bt_A2DP_OpenStream(bt_profile_manager[id].stream,
                                         &bt_profile_manager[id].rmt_addr);
                }
      }
#ifdef __AUTO_CONNECT_OTHER_PROFILE__
      else {
                // befor auto connect a2dp profile, check whether a2dp is
                // supported
                if (btdevice_plf_p->a2dp_act &&
                    bt_profile_manager[id].a2dp_connect !=
                        bt_profile_connect_status_success) {
                  bt_profile_manager[id].connect_timer_cb =
                      app_bt_profile_connect_a2dp_retry_timehandler;
                  app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
                  osTimerStart(bt_profile_manager[id].connect_timer,
                               APP_BT_PROFILE_CONNECT_RETRY_MS);
                }
      }
#endif
      break;
    case BTIF_HF_EVENT_SERVICE_DISCONNECTED:
      if (((ctx->disc_reason == 0) || (ctx->disc_reason == 19)) &&
          (p_ibrt_ctrl->current_role != IBRT_SLAVE)) {
                once_event_case = 2;
                startonce_delay_event_Timer_(1000);
      }
      // TRACE(3,"%s HF_EVENT_SERVICE_DISCONNECTED discReason:%d/%d",__func__,
      // Info->p.remDev->discReason, Info->p.remDev->discReason_saved);
      TRACE(3, "%s HF_EVENT_SERVICE_DISCONNECTED discReason:%d/%d", __func__,
            ctx->disc_reason, ctx->disc_reason_saved);
      bt_profile_manager[id].hfp_connect = bt_profile_connect_status_failure;
      if (bt_profile_manager[id].reconnect_mode ==
          bt_profile_reconnect_openreconnecting) {
                if (++bt_profile_manager[id].reconnect_cnt <
                    APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT) {
                  app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                  profile_reconnect_enable = true;
                  bt_profile_manager[id].hfp_connect =
                      bt_profile_connect_status_unknow;
                }
      } else if (bt_profile_manager[id].reconnect_mode ==
                 bt_profile_reconnect_reconnecting) {
                if (++bt_profile_manager[id].reconnect_cnt <
                    APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT) {
                  app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                  profile_reconnect_enable = true;
                } else {
                  app_bt_restore_reconnecting_idle_mode(id);
                  // bt_profile_manager[id].reconnect_mode =
                  // bt_profile_reconnect_null;
                }
                TRACE(2, "%s try to reconnect cnt:%d", __func__,
                      bt_profile_manager[id].reconnect_cnt);
                /*
                }else if ((Info->p.remDev->discReason == 0x8)||
                      (Info->p.remDev->discReason_saved == 0x8)){
                      */
      }
#if !defined(IBRT)
#if defined(ENHANCED_STACK)
      else if ((ctx->disc_reason == 0x8) || (ctx->disc_reason_saved == 0x8) ||
               (ctx->disc_reason == 0x4) || (ctx->disc_reason_saved == 0x4))
#else
      else if ((ctx->disc_reason == 0x8) || (ctx->disc_reason_saved == 0x8) ||
               (ctx->disc_reason == 0x0) || (ctx->disc_reason_saved == 0x0))
#endif
      {
                bt_profile_manager[id].reconnect_mode =
                    bt_profile_reconnect_reconnecting;
                app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                TRACE(2, "%s try to reconnect reason =%d", __func__,
                      ctx->disc_reason);
                if (app_bt_profile_reconnect_pending(id) != 0) {
                  profile_reconnect_enable = true;
                }
      }
#endif
      else {
                bt_profile_manager[id].hfp_connect =
                    bt_profile_connect_status_unknow;
      }

      if (profile_reconnect_enable) {
#ifdef __IAG_BLE_INCLUDE__
                app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
                osTimerStart(bt_profile_manager[id].connect_timer,
                             APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
      }
      break;
    default:
      break;
    }
  }
  DUMP8("%02x ", &bt_profile_manager[id].rmt_addr.address, 6);
  btdevice_profile *btdevice_plf_p1 =
      (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          (uint8_t *)&bt_profile_manager[id].rmt_addr.address);

  if (bt_profile_manager[id].reconnect_mode ==
      bt_profile_reconnect_reconnecting) {
    bool reconnect_hfp_proc_final = false;
    bool reconnect_a2dp_proc_final = false;

    if (bt_profile_manager[id].hfp_connect !=
        bt_profile_connect_status_success) {
      reconnect_hfp_proc_final = false;
    } else {
      reconnect_hfp_proc_final = true;
    }
    if (bt_profile_manager[id].a2dp_connect !=
        bt_profile_connect_status_success) {
      if (btdevice_plf_p1->hfp_act && btdevice_plf_p1->a2dp_act) {
                reconnect_a2dp_proc_final = false;
      } else {
                reconnect_a2dp_proc_final = true;
      }
    }
    if (reconnect_hfp_proc_final && reconnect_a2dp_proc_final) {
      TRACE(3, "!!!reconnect success %d/%d/%d\n",
            bt_profile_manager[id].hfp_connect,
            bt_profile_manager[id].hsp_connect,
            bt_profile_manager[id].a2dp_connect);
      app_bt_restore_reconnecting_idle_mode(id);
      // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
    }
  } else if (bt_profile_manager[id].reconnect_mode ==
             bt_profile_reconnect_openreconnecting) {
    bool opening_hfp_proc_final = false;
    bool opening_a2dp_proc_final = false;

    if (btdevice_plf_p1->hfp_act && bt_profile_manager[id].hfp_connect ==
                                        bt_profile_connect_status_unknow) {
      opening_hfp_proc_final = false;
    } else {
      opening_hfp_proc_final = true;
    }

    if (btdevice_plf_p1->a2dp_act && bt_profile_manager[id].a2dp_connect ==
                                         bt_profile_connect_status_unknow) {
      opening_a2dp_proc_final = false;
    } else {
      opening_a2dp_proc_final = true;
    }

    if (opening_hfp_proc_final && opening_a2dp_proc_final) {
      TRACE(3, "!!!reconnect success %d/%d/%d\n",
            bt_profile_manager[id].hfp_connect,
            bt_profile_manager[id].hsp_connect,
            bt_profile_manager[id].a2dp_connect);
      bt_profile_manager[id].saved_reconnect_mode =
          bt_profile_reconnect_openreconnecting;
      app_bt_restore_reconnecting_idle_mode(id);
    } else if (bt_profile_manager[id].hfp_connect ==
               bt_profile_connect_status_failure) {
      TRACE(3, "reconnect_mode888:%d", bt_profile_manager[0].reconnect_mode);
      TRACE(3, "!!!reconnect success %d/%d/%d\n",
            bt_profile_manager[id].hfp_connect,
            bt_profile_manager[id].hsp_connect,
            bt_profile_manager[id].a2dp_connect);
      bt_profile_manager[id].saved_reconnect_mode =
          bt_profile_reconnect_openreconnecting;
      if ((bt_profile_manager[id].reconnect_mode ==
           bt_profile_reconnect_openreconnecting) &&
          (bt_profile_manager[id].reconnect_cnt >=
           APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT)) {
                app_bt_restore_reconnecting_idle_mode(id);
      }
    }

    if (btdevice_plf_p1->hfp_act && bt_profile_manager[id].hfp_connect ==
                                        bt_profile_connect_status_success) {
      if (btdevice_plf_p1->a2dp_act && !opening_a2dp_proc_final) {
                TRACE(1, "!!!continue connect a2dp %p\n",
                      bt_profile_manager[id].stream);
                app_bt_precheck_before_starting_connecting(
                    bt_profile_manager[id].has_connected);
                app_bt_A2DP_OpenStream(bt_profile_manager[id].stream,
                                       &bt_profile_manager[id].rmt_addr);
      }
    }

    if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_null) {
      for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
                btdevice_profile *btdevice_plf_p_temp =
                    (btdevice_profile *)app_bt_profile_active_store_ptr_get(
                        (uint8_t *)bt_profile_manager[i].rmt_addr.address);
                TRACE(3, "reconnect_mode:%d",
                      bt_profile_manager[i].reconnect_mode);
                if (bt_profile_manager[i].reconnect_mode ==
                    bt_profile_reconnect_openreconnecting) {
                  TRACE(0, "!!!hf->start reconnect second device\n");
                  if ((btdevice_plf_p_temp->hfp_act) &&
                      (!bt_profile_manager[i].hfp_connect)) {
                    TRACE(0, "try connect hf");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[id].has_connected);
                    app_bt_HF_CreateServiceLink(
                        bt_profile_manager[i].chan,
                        (bt_bdaddr_t *)&bt_profile_manager[i].rmt_addr);
                  }
#if defined(__HSP_ENABLE__)
                  else if ((btdevice_plf_p_temp->hsp_act) &&
                           (!bt_profile_manager[i].hsp_connect)) {
                    TRACE(0, "try connect hs");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[id].has_connected);
                    app_bt_HS_CreateServiceLink(
                        bt_profile_manager[i].hs_chan,
                        &bt_profile_manager[i].rmt_addr);
                  }
#endif
                  else if ((btdevice_plf_p_temp->a2dp_act) &&
                           (!bt_profile_manager[i].a2dp_connect)) {
                    TRACE(0, "try connect a2dp");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[id].has_connected);
                    app_bt_A2DP_OpenStream(bt_profile_manager[i].stream,
                                           &bt_profile_manager[i].rmt_addr);
                  }
                  break;
                }
      }
    }
  }
#ifdef __INTERCONNECTION__
  if (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success &&
      bt_profile_manager[id].a2dp_connect ==
          bt_profile_connect_status_success) {
    app_interconnection_start_disappear_adv(
        INTERCONNECTION_BLE_ADVERTISING_INTERVAL,
        APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);

    if (btif_me_get_activeCons() <= 2) {
      app_interconnection_spp_open(btif_me_enumerate_remote_devices(id));
    }
  }
#endif

#ifdef __IAG_BLE_INCLUDE__
  TRACE(3, "%s hfp %d a2dp %d", __func__, bt_profile_manager[id].hfp_connect,
        bt_profile_manager[id].a2dp_connect);
  if (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success &&
#ifdef __HSP_ENABLE__
      bt_profile_manager[id].hsp_connect == bt_profile_connect_status_success &&
#endif
      bt_profile_manager[id].a2dp_connect ==
          bt_profile_connect_status_success) {
    app_ble_force_switch_adv(BLE_SWITCH_USER_BT_CONNECT, true);
  }
#endif

  if (!bt_profile_manager[id].has_connected &&
      (bt_profile_manager[id].hfp_connect ==
           bt_profile_connect_status_success ||
#ifdef __HSP_ENABLE__
       bt_profile_manager[id].hsp_connect ==
           bt_profile_connect_status_success ||
#endif
       bt_profile_manager[id].a2dp_connect ==
           bt_profile_connect_status_success)) {

    bt_profile_manager[id].has_connected = true;
    TRACE(0, "BT connected!!!");
    once_event_case = 1;
    /*if(IsMobileLinkLossing){
        startonce_delay_event_Timer_(3000);
    }
    else{
        startonce_delay_event_Timer_(1500);
    }*/
    app_voice_report(APP_STATUS_INDICATION_CONNECTED, 0);
    IsMobileLinkLossing = FALSE;
#ifndef IBRT
    btif_me_get_remote_device_name(&(ctx->remote_dev_bdaddr),
                                   app_bt_global_handle);
#endif
#if defined(MEDIA_PLAYER_SUPPORT) && !defined(IBRT)
    app_voice_report(APP_STATUS_INDICATION_CONNECTED, id);
#endif

#if 0 // #ifdef __INTERCONNECTION__
        app_interconnection_start_disappear_adv(BLE_ADVERTISING_INTERVAL, APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);
        app_interconnection_spp_open();
#endif

#ifdef __INTERACTION__
    //    app_interaction_spp_open();
#endif
  }

  if (bt_profile_manager[id].has_connected &&
      (bt_profile_manager[id].hfp_connect !=
           bt_profile_connect_status_success &&
#ifdef __HSP_ENABLE__
       bt_profile_manager[id].hsp_connect !=
           bt_profile_connect_status_success &&
#endif
       bt_profile_manager[id].a2dp_connect !=
           bt_profile_connect_status_success)) {

    bt_profile_manager[id].has_connected = false;
    TRACE(0, "BT disconnected!!!");

#ifdef GFPS_ENABLED
    if (app_gfps_is_last_response_pending()) {
      app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
    }
#endif

#if defined(MEDIA_PLAYER_SUPPORT) && !defined(IBRT)
    app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
    app_interconnection_disconnected_callback();
#endif

    app_set_disconnecting_all_bt_connections(false);
  }

#ifdef __BT_ONE_BRING_TWO__
  app_bt_update_connectable_mode_after_connection_management();
#endif
}

void app_bt_profile_connect_manager_a2dp(enum BT_DEVICE_ID_T id,
                                         a2dp_stream_t *Stream,
                                         const a2dp_callback_parms_t *info) {
  btdevice_profile *btdevice_plf_p = NULL;
  btif_remote_device_t *remDev = NULL;
  btif_a2dp_callback_parms_t *Info = (btif_a2dp_callback_parms_t *)info;
  osTimerStop(bt_profile_manager[id].connect_timer);
  bt_profile_manager[id].connect_timer_cb = NULL;
  bool profile_reconnect_enable = false;

  remDev = btif_a2dp_get_stream_conn_remDev(Stream);
  if (remDev) {
    btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
        btif_me_get_remote_device_bdaddr(remDev)->address);
  } else {
    btdevice_plf_p =
        (btdevice_profile *)app_bt_profile_active_store_ptr_get(NULL);
  }

  if (Stream && Info) {

    switch (Info->event) {
    case BTIF_A2DP_EVENT_STREAM_OPEN:
      TRACE(4, "%s A2DP_EVENT_STREAM_OPEN,codec type=%x a2dp:%d mode:%d",
            __func__, Info->p.configReq->codec.codecType,
            bt_profile_manager[id].a2dp_connect,
            bt_profile_manager[id].reconnect_mode);

      nv_record_btdevicerecord_set_a2dp_profile_active_state(btdevice_plf_p,
                                                             true);
      nv_record_btdevicerecord_set_a2dp_profile_codec(
          btdevice_plf_p, Info->p.configReq->codec.codecType);
      nv_record_touch_cause_flush();
      if (bt_profile_manager[id].a2dp_connect ==
          bt_profile_connect_status_success) {
                TRACE(0, "!!!a2dp has opened   force return ");
                return;
      }
      bt_profile_manager[id].a2dp_connect = bt_profile_connect_status_success;
      bt_profile_manager[id].reconnect_cnt = 0;
      bt_profile_manager[id].stream = app_bt_device.a2dp_connected_stream[id];
      memcpy(bt_profile_manager[id].rmt_addr.address,
             btif_me_get_remote_device_bdaddr(
                 btif_a2dp_get_stream_conn_remDev(Stream))
                 ->address,
             BTIF_BD_ADDR_SIZE);
      app_bt_record_latest_connected_service_device_id(id);
      if (false == bt_profile_manager[id].has_connected) {
                app_bt_resume_sniff_mode(id);
      }

#ifdef BTIF_DIP_DEVICE
      btif_dip_get_remote_info(remDev);
#endif

      if (bt_profile_manager[id].reconnect_mode ==
          bt_profile_reconnect_openreconnecting) {
                // do nothing
      }

#if defined(IBRT)
      else if (app_bt_ibrt_reconnect_mobile_profile_flag_get()) {
                app_bt_ibrt_reconnect_mobile_profile_flag_clear();
#else
      else if (bt_profile_manager[id].reconnect_mode ==
               bt_profile_reconnect_reconnecting) {
#endif
                TRACE(2, "app_bt: hfp_act in NV =%d,a2dp_connect=%d",
                      btdevice_plf_p->hfp_act,
                      bt_profile_manager[id].hfp_connect);
                if (btdevice_plf_p->hfp_act &&
                    bt_profile_manager[id].hfp_connect !=
                        bt_profile_connect_status_success) {
                  if (btif_hf_check_rfcomm_l2cap_channel_is_creating(
                          &bt_profile_manager[id].rmt_addr)) {
                    TRACE(0,
                          "!!!remote is creating hfp after a2dp connected\n");
                  } else {
                    TRACE(0, "!!!continue connect hfp\n");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[id].has_connected);
                    app_bt_HF_CreateServiceLink(
                        bt_profile_manager[id].chan,
                        (bt_bdaddr_t *)&bt_profile_manager[id].rmt_addr);
                  }
                }
#if defined(__HSP_ENABLE__)
                else if (btdevice_plf_p->hsp_act &&
                         bt_profile_manager[id].hsp_connect !=
                             bt_profile_connect_status_success) {
                  TRACE(0, "!!!continue connect hsp\n");
                  app_bt_precheck_before_starting_connecting(
                      bt_profile_manager[id].has_connected);
                  app_bt_HS_CreateServiceLink(bt_profile_manager[id].hs_chan,
                                              &bt_profile_manager[id].rmt_addr);
                }
#endif
      }
#ifdef __AUTO_CONNECT_OTHER_PROFILE__
      else {
                if (btdevice_plf_p->hfp_act &&
                    bt_profile_manager[id].hfp_connect !=
                        bt_profile_connect_status_success) {
                  bt_profile_manager[id].connect_timer_cb =
                      app_bt_profile_connect_hf_retry_timehandler;
                  app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
                  osTimerStart(bt_profile_manager[id].connect_timer,
                               APP_BT_PROFILE_CONNECT_RETRY_MS);
                }
#if defined(__HSP_ENABLE__)
                else if (btdevice_plf_p->hsp_act &&
                         bt_profile_manager[id].hsp_connect !=
                             bt_profile_connect_status_success) {
                  bt_profile_manager[id].connect_timer_cb =
                      app_bt_profile_connect_hs_retry_timehandler;
                  app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
                  osTimerStart(bt_profile_manager[id].connect_timer,
                               APP_BT_PROFILE_CONNECT_RETRY_MS);
                }
#endif
      }
#endif
#ifdef APP_DISABLE_PAGE_SCAN_AFTER_CONN
      disable_page_scan_check_timer_start();
#endif
      break;
    case BTIF_A2DP_EVENT_STREAM_CLOSED:

      TRACE(2, "%s A2DP_EVENT_STREAM_CLOSED discReason1:%d", __func__,
            Info->discReason);

      if (Info->subevt != A2DP_CONN_CLOSED) {
                TRACE(0, "do not need set access mode");
                return;
      }

      if (Stream != NULL) {
                if (btif_a2dp_get_remote_device(Stream) != NULL)
                  TRACE(2, "%s A2DP_EVENT_STREAM_CLOSED discReason2:%d",
                        __func__,
                        btif_me_get_remote_device_disc_reason_saved(
                            btif_a2dp_get_remote_device(Stream)));
      }

#if defined(IBRT)
      if (app_bt_ibrt_reconnect_mobile_profile_flag_get()) {
                app_bt_HF_CreateServiceLink(
                    bt_profile_manager[id].chan,
                    (bt_bdaddr_t *)&bt_profile_manager[id].rmt_addr);
      }
#endif

      bt_profile_manager[id].a2dp_connect = bt_profile_connect_status_failure;

      if (bt_profile_manager[id].reconnect_mode ==
          bt_profile_reconnect_openreconnecting) {
                if (++bt_profile_manager[id].reconnect_cnt <
                    APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT) {
                  app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                  profile_reconnect_enable = true;
                  bt_profile_manager[id].a2dp_connect =
                      bt_profile_connect_status_unknow;
                }
      } else if (bt_profile_manager[id].reconnect_mode ==
                 bt_profile_reconnect_reconnecting) {
                if (++bt_profile_manager[id].reconnect_cnt <
                    APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT) {
                  app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                  profile_reconnect_enable = true;
                } else {
                  app_bt_restore_reconnecting_idle_mode(id);
                  // bt_profile_manager[id].reconnect_mode =
                  // bt_profile_reconnect_null;
                }
                TRACE(2, "%s try to reconnect cnt:%d", __func__,
                      bt_profile_manager[id].reconnect_cnt);
      }
#if !defined(IBRT)
#if defined(ENHANCED_STACK)
      else if (((Info->discReason == 0x08) || (Info->discReason == 0x04)) &&
#else
      else if (((Info->discReason == 0x8) || (Info->discReason_saved == 0x8) ||
                (Info->discReason_saved == 0x0)) &&
#endif
               (btdevice_plf_p->a2dp_act) && (!btdevice_plf_p->hfp_act) &&
               (!btdevice_plf_p->hsp_act)) {
                bt_profile_manager[id].reconnect_mode =
                    bt_profile_reconnect_reconnecting;
                TRACE(2, "%s try to reconnect cnt:%d", __func__,
                      bt_profile_manager[id].reconnect_cnt);
                app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                if (app_bt_profile_reconnect_pending(id) != 0) {
                  profile_reconnect_enable = true;
                }
      }
#endif
      else {
                bt_profile_manager[id].a2dp_connect =
                    bt_profile_connect_status_unknow;
      }

      if (profile_reconnect_enable) {
#ifdef __IAG_BLE_INCLUDE__
                app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
                osTimerStart(bt_profile_manager[id].connect_timer,
                             APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
      }
      break;
    default:
      break;
    }
  }

  if (bt_profile_manager[id].reconnect_mode ==
      bt_profile_reconnect_reconnecting) {
    bool reconnect_hfp_proc_final = true;
    bool reconnect_a2dp_proc_final = true;
    if (bt_profile_manager[id].hfp_connect ==
        bt_profile_connect_status_failure) {
      reconnect_hfp_proc_final = false;
    }
#if defined(__HSP_ENABLE__)
    if (btdevice_plf_p->hsp_act != 0) // has HSP
    {
      reconnect_hfp_proc_final = true;
      if (bt_profile_manager[id].hsp_connect ==
          bt_profile_connect_status_failure) {
                reconnect_hfp_proc_final = false;
      }
    }
#endif
    if (bt_profile_manager[id].a2dp_connect ==
        bt_profile_connect_status_failure) {
      reconnect_a2dp_proc_final = false;
    }
    if (reconnect_hfp_proc_final && reconnect_a2dp_proc_final) {
      TRACE(3, "!!!reconnect success %d/%d/%d\n",
            bt_profile_manager[id].hfp_connect,
            bt_profile_manager[id].hsp_connect,
            bt_profile_manager[id].a2dp_connect);
      app_bt_restore_reconnecting_idle_mode(id);
      // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
    }
  } else if (bt_profile_manager[id].reconnect_mode ==
             bt_profile_reconnect_openreconnecting) {
    bool opening_hfp_proc_final = false;
    bool opening_a2dp_proc_final = false;

    if (btdevice_plf_p->hfp_act && bt_profile_manager[id].hfp_connect ==
                                       bt_profile_connect_status_unknow) {
      opening_hfp_proc_final = false;
    } else {
      opening_hfp_proc_final = true;
    }

    if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect ==
                                        bt_profile_connect_status_unknow) {
      opening_a2dp_proc_final = false;
    } else {
      opening_a2dp_proc_final = true;
    }

    if ((opening_hfp_proc_final && opening_a2dp_proc_final) ||
        (bt_profile_manager[id].a2dp_connect ==
         bt_profile_connect_status_failure)) {
      TRACE(3, "!!!reconnect success %d/%d/%d\n",
            bt_profile_manager[id].hfp_connect,
            bt_profile_manager[id].hsp_connect,
            bt_profile_manager[id].a2dp_connect);
      app_bt_restore_reconnecting_idle_mode(id);
      // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
    }

    if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect ==
                                        bt_profile_connect_status_success) {
      if (btdevice_plf_p->hfp_act && !opening_hfp_proc_final) {
                if (btif_hf_check_rfcomm_l2cap_channel_is_creating(
                        &bt_profile_manager[id].rmt_addr)) {
                  TRACE(0, "!!!remote is creating hf after a2dp connected\n");
                } else {
                  TRACE(0, "!!!continue connect hf\n");
                  app_bt_precheck_before_starting_connecting(
                      bt_profile_manager[id].has_connected);
                  app_bt_HF_CreateServiceLink(
                      bt_profile_manager[id].chan,
                      (bt_bdaddr_t *)&bt_profile_manager[id].rmt_addr);
                }
      }
#if defined(__HSP_ENABLE)
      else if (btdevice_plf_p->hsp_act && !opening_hfp_hsp_proc_final) {
                TRACE(0, "!!!continue connect hs\n");
                app_bt_precheck_before_starting_connecting(
                    bt_profile_manager[id].has_connected);
                app_bt_HS_CreateServiceLink(bt_profile_manager[id].hs_chan,
                                            &bt_profile_manager[id].rmt_addr);
      }
#endif
    }

    if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_null) {
      for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
                if (bt_profile_manager[i].reconnect_mode ==
                    bt_profile_reconnect_openreconnecting) {
                  TRACE(1, "!!!a2dp->start reconnect device %d\n", i);
                  if ((btdevice_plf_p->hfp_act) &&
                      (!bt_profile_manager[i].hfp_connect)) {
                    TRACE(0, "try connect hf");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[i].has_connected);
                    app_bt_HF_CreateServiceLink(
                        bt_profile_manager[i].chan,
                        (bt_bdaddr_t *)&bt_profile_manager[i].rmt_addr);
                  }
#if defined(__HSP_ENABLE__)
                  else if ((btdevice_plf_p->hsp_act) &&
                           (!bt_profile_manager[i].hsp_connect)) {
                    TRACE(0, "try connect hs");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[i].has_connected);
                    app_bt_HS_CreateServiceLink(
                        bt_profile_manager[i].hs_chan,
                        &bt_profile_manager[i].rmt_addr);
                  }
#endif
                  else if ((btdevice_plf_p->a2dp_act) &&
                           (!bt_profile_manager[i].a2dp_connect)) {
                    TRACE(0, "try connect a2dp");
                    app_bt_precheck_before_starting_connecting(
                        bt_profile_manager[i].has_connected);
                    app_bt_A2DP_OpenStream(bt_profile_manager[i].stream,
                                           &bt_profile_manager[i].rmt_addr);
                  }
                  break;
                }
      }
    }
  }

#ifdef __INTERCONNECTION__
  if (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success &&
      bt_profile_manager[id].a2dp_connect ==
          bt_profile_connect_status_success) {
    app_interconnection_start_disappear_adv(
        INTERCONNECTION_BLE_ADVERTISING_INTERVAL,
        APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);

    if (btif_me_get_activeCons() <= 2) {
      app_interconnection_spp_open(remDev);
    }
  }
#endif

#ifdef __IAG_BLE_INCLUDE__
  TRACE(3, "%s hfp %d a2dp %d", __func__, bt_profile_manager[id].hfp_connect,
        bt_profile_manager[id].a2dp_connect);
  if (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success &&
#ifdef __HSP_ENABLE__
      bt_profile_manager[id].hsp_connect == bt_profile_connect_status_success &&
#endif
      bt_profile_manager[id].a2dp_connect ==
          bt_profile_connect_status_success) {
    app_ble_force_switch_adv(BLE_SWITCH_USER_BT_CONNECT, true);
  }
#endif

  if (!bt_profile_manager[id].has_connected &&
      (bt_profile_manager[id].hfp_connect ==
           bt_profile_connect_status_success ||
#ifdef __HSP_ENABLE__
       bt_profile_manager[id].hsp_connect ==
           bt_profile_connect_status_success ||
#endif
       bt_profile_manager[id].a2dp_connect ==
           bt_profile_connect_status_success)) {

    bt_profile_manager[id].has_connected = true;
    TRACE(0, "BT connected!!!");
    IsMobileLinkLossing = FALSE;
#ifndef IBRT
    btif_me_get_remote_device_name(&(bt_profile_manager[id].rmt_addr),
                                   app_bt_global_handle);
#endif
#if defined(MEDIA_PLAYER_SUPPORT) //&& !defined(IBRT)
    app_voice_report(APP_STATUS_INDICATION_CONNECTED, id);
#endif

#if 0 // #ifdef __INTERCONNECTION__
        app_interconnection_start_disappear_adv(BLE_ADVERTISING_INTERVAL, APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);
        app_interconnection_spp_open();
#endif

#ifdef __INTERACTION__
    //    app_interaction_spp_open();
#endif
  }

  if (bt_profile_manager[id].has_connected &&
      (bt_profile_manager[id].hfp_connect !=
           bt_profile_connect_status_success &&
#ifdef __HSP_ENABLE__
       bt_profile_manager[id].hsp_connect !=
           bt_profile_connect_status_success &&
#endif
       bt_profile_manager[id].a2dp_connect !=
           bt_profile_connect_status_success)) {

    bt_profile_manager[id].has_connected = false;
    TRACE(0, "BT disconnected!!!");

#ifdef GFPS_ENABLED
    if (app_gfps_is_last_response_pending()) {
      app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
    }
#endif

#if defined(MEDIA_PLAYER_SUPPORT) && !defined(IBRT)
    app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
    app_interconnection_disconnected_callback();
#endif

    app_set_disconnecting_all_bt_connections(false);
  }

#ifdef __BT_ONE_BRING_TWO__
  app_bt_update_connectable_mode_after_connection_management();
#endif
}

#ifdef BTIF_HID_DEVICE
void hid_exit_shutter_mode(void);
#endif

static bool isDisconnectAllBtConnections = false;

bool app_is_disconnecting_all_bt_connections(void) {
  return isDisconnectAllBtConnections;
}

void app_set_disconnecting_all_bt_connections(bool isEnable) {
  isDisconnectAllBtConnections = isEnable;
}

bt_status_t LinkDisconnectDirectly(bool PowerOffFlag) {
  app_set_disconnecting_all_bt_connections(true);
  // TRACE(1,"osapi_lock_is_exist:%d",osapi_lock_is_exist());
  if (osapi_lock_is_exist())
    osapi_lock_stack();
#ifdef __IAG_BLE_INCLUDE__
  TRACE(1, "ble_connected_state:%d", app_ble_is_any_connection_exist());
#endif

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (true == PowerOffFlag)
    p_ibrt_ctrl->ibrt_in_poweroff = true;

  if (p_ibrt_ctrl->init_done) {
    if (IBRT_MASTER == p_ibrt_ctrl->current_role) {
      if (app_tws_ibrt_mobile_link_connected()) {
                // should check return status
                app_tws_ibrt_disconnect_connection(
                    btif_me_get_remote_device_by_handle(
                        p_ibrt_ctrl->mobile_conhandle));
      }
    }
    if (app_tws_ibrt_tws_link_connected()) {
      app_tws_ibrt_disconnect_connection(
          btif_me_get_remote_device_by_handle(p_ibrt_ctrl->tws_conhandle));
    }
  }

  if (osapi_lock_is_exist())
    osapi_unlock_stack();

  osDelay(500);

  return BT_STS_SUCCESS;
#endif

  TRACE(1, "activeCons:%d", btif_me_get_activeCons());

  uint8_t Tmp_activeCons = btif_me_get_activeCons();

  if (Tmp_activeCons) {
    // TRACE(3,"%s id1 hf:%d a2dp:%d",__func__,
    // app_bt_device.hf_channel[BT_DEVICE_ID_1].state,
    // btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream));
    TRACE(3, "%s id1 hf:%d a2dp:%d", __func__,
          btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]),
          btif_a2dp_get_stream_state(
              app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream));
#ifdef BTIF_HID_DEVICE
    hid_exit_shutter_mode();
#endif
    if (btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]) ==
        BTIF_HF_STATE_OPEN) {
      app_bt_HF_DisconnectServiceLink(app_bt_device.hf_channel[BT_DEVICE_ID_1]);
    }
#if defined(__HSP_ENABLE__)
    if (app_bt_device.hs_channel[BT_DEVICE_ID_1].state == HS_STATE_OPEN) {
      app_bt_HS_DisconnectServiceLink(
          &app_bt_device.hs_channel[BT_DEVICE_ID_1]);
    }
#endif // btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[device_id]->a2dp_stream)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream);
    }
#if defined(A2DP_LHDC_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream);
    }
#endif
#if defined(A2DP_LDAC_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream);
    }
#endif

#if defined(A2DP_AAC_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream);
    }
#endif
#if defined(A2DP_SCALABLE_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream);
    }
#endif
    if (btif_avrcp_get_remote_device(
            app_bt_device.avrcp_channel[BT_DEVICE_ID_1]
                ->avrcp_channel_handle)) {
      btif_avrcp_disconnect(
          app_bt_device.avrcp_channel[BT_DEVICE_ID_1]->avrcp_channel_handle);
    }
#ifdef __BT_ONE_BRING_TWO__
    TRACE(3, "%s id2 hf:%d a2dp:%d", __func__,
          btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_2]),
          btif_a2dp_get_stream_state(
              app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream));
    // if(app_bt_device.hf_channel[BT_DEVICE_ID_2].state == HF_STATE_OPEN){
    if (btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_2]) ==
        BTIF_HF_STATE_OPEN) {
      app_bt_HF_DisconnectServiceLink(app_bt_device.hf_channel[BT_DEVICE_ID_2]);
    }

#if defined(__HSP_ENABLE__)
    if (app_bt_device.hs_channel[BT_DEVICE_ID_2].state == HS_STATE_OPEN) {
      app_bt_HS_DisconnectServiceLink(
          &app_bt_device.hs_channel[BT_DEVICE_ID_2]);
    }
#endif // __HSP_ENABLE__

    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream);
    }
#if defined(A2DP_LHDC_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream);
    }
#endif // A2DP_LHDC_ON
#if defined(A2DP_AAC_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream);
    }
#endif // A2DP_AAC_ON
#if defined(A2DP_SCALABLE_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(
            app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]->a2dp_stream) ==
            BTIF_AVDTP_STRM_STATE_OPEN) {
      app_bt_A2DP_CloseStream(
          app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]->a2dp_stream);
    }
#endif // A2DP_SCALABLE_ON
    if (btif_avrcp_get_remote_device(
            app_bt_device.avrcp_channel[BT_DEVICE_ID_2]
                ->avrcp_channel_handle)) {
      btif_avrcp_disconnect(
          app_bt_device.avrcp_channel[BT_DEVICE_ID_2]->avrcp_channel_handle);
    }
#endif //__BT_ONE_BRING_TWO__

#ifdef BISTO_ENABLED
    gsound_custom_bt_disconnect_all_channel();
#endif
  }

#ifdef __IAG_BLE_INCLUDE__
  if (app_ble_is_any_connection_exist()) {
#ifdef GFPS_ENABLED
    if (!app_gfps_is_last_response_pending())
#endif
      app_ble_disconnect_all();
  }
#endif

  if (osapi_lock_is_exist())
    osapi_unlock_stack();

  osDelay(500);

  if (Tmp_activeCons) {
    btif_remote_device_t *remDev = app_bt_get_remoteDev(BT_DEVICE_ID_1);
    if (NULL != remDev) {
      app_bt_MeDisconnectLink(remDev);
    }

#ifdef __BT_ONE_BRING_TWO__
    remDev = app_bt_get_remoteDev(BT_DEVICE_ID_2);
    if (NULL != remDev) {
      osDelay(200);
      app_bt_MeDisconnectLink(remDev);
    }
#endif
  }
  return BT_STS_SUCCESS;
}

void app_disconnect_all_bt_connections(void) { LinkDisconnectDirectly(false); }

static int app_custom_function_process(APP_MESSAGE_BODY *msg_body) {
  APP_APPTHREAD_REQ_CUSTOMER_CALL_FN_T customer_call =
      (APP_APPTHREAD_REQ_CUSTOMER_CALL_FN_T)(msg_body->message_ptr);
  TRACE(4, "func:0x%08x,param0:0x%08x, param1:0x%08x", msg_body->message_ptr,
        msg_body->message_Param0, msg_body->message_Param1);
  if (customer_call) {
    customer_call((void *)msg_body->message_Param0,
                  (void *)msg_body->message_Param1);
  }
  return 0;
}

int app_bt_start_custom_function_in_app_thread(uint32_t param0, uint32_t param1,
                                               uint32_t funcPtr) {
  APP_MESSAGE_BLOCK msg;

  msg.mod_id = APP_MODUAL_CUSTOM_FUNCTION;
  msg.msg_body.message_id = 0;
  msg.msg_body.message_ptr = funcPtr;
  msg.msg_body.message_Param0 = param0;
  msg.msg_body.message_Param1 = param1;

  app_mailbox_put(&msg);
  return 0;
}

void app_bt_init(void) {
  app_bt_mail_init();
  app_set_threadhandle(APP_MODUAL_BT, app_bt_handle_process);
  btif_me_sec_set_io_cap_rsp_reject_ext(
      app_bt_profile_connect_openreconnecting);
  app_bt_active_mode_manager_init();
  app_set_threadhandle(APP_MODUAL_CUSTOM_FUNCTION, app_custom_function_process);
}

extern "C" bool app_bt_has_connectivitys(void) {
  int activeCons;
  osapi_lock_stack();
  activeCons = btif_me_get_activeCons();
  osapi_unlock_stack();

  if (activeCons > 0)
    return true;

  return false;
#if 0
    if(app_bt_device.hf_channel[BT_DEVICE_ID_1].cmgrHandler.remDev)
        return true;
    if(app_bt_device.a2dp_stream[BT_DEVICE_ID_1].device->cmgrHandler.remDev)
        return true;
#ifdef __BT_ONE_BRING_TWO__
    if(app_bt_device.hf_channel[BT_DEVICE_ID_2].cmgrHandler.remDev)
        return true;
    if(app_bt_device.a2dp_stream[BT_DEVICE_ID_2].device->cmgrHandler.remDev)
        return true;
#endif
    return false;
#endif
}

#ifdef __TWS_CHARGER_BOX__

extern "C" {
bt_status_t ME_Ble_Clear_Whitelist(void);
bt_status_t ME_Ble_Set_Private_Address(BT_BD_ADDR *addr);
bt_status_t ME_Ble_Add_Dev_To_Whitelist(U8 addr_type, BT_BD_ADDR *addr);
bt_status_t ME_Ble_SetAdv_data(U8 len, U8 *data);
bt_status_t ME_Ble_SetScanRsp_data(U8 len, U8 *data);
bt_status_t ME_Ble_SetAdv_parameters(adv_para_struct *para);
bt_status_t ME_Ble_SetAdv_en(U8 en);
bt_status_t ME_Ble_Setscan_parameter(scan_para_struct *para);
bt_status_t ME_Ble_Setscan_en(U8 scan_en, U8 filter_duplicate);
}

int8_t power_level = 0;
#define TWS_BOX_OPEN 1
#define TWS_BOX_CLOSE 0
void app_tws_box_set_slave_adv_data(uint8_t power_level, uint8_t box_status) {
  uint8_t adv_data[] = {
      0x02, 0xfe, 0x00, 0x02, 0xfd, 0x00 // manufacturer data
  };

  adv_data[2] = power_level;

  adv_data[5] = box_status;
  ME_Ble_SetAdv_data(sizeof(adv_data), adv_data);
}

void app_tws_box_set_slave_adv_para(void) {
  uint8_t peer_addr[BTIF_BD_ADDR_SIZE] = {0};
  adv_para_struct para;

  para.interval_min = 0x0040; // 20ms
  para.interval_max = 0x0040; // 20ms
  para.adv_type = 0x03;
  para.own_addr_type = 0x01;
  para.peer_addr_type = 0x01;
  para.adv_chanmap = 0x07;
  para.adv_filter_policy = 0x00;
  memcpy(para.bd_addr.addr, peer_addr, BTIF_BD_ADDR_SIZE);

  ME_Ble_SetAdv_parameters(&para);
}

extern uint8_t bt_addr[6];
void app_tws_start_chargerbox_adv(void) {
  app_tws_box_set_slave_adv_data(power_level, TWS_BOX_OPEN);
  ME_Ble_Set_Private_Address((BT_BD_ADDR *)bt_addr);
  app_tws_box_set_slave_adv_para();
  ME_Ble_SetAdv_en(1);
}

#endif

bool app_is_hfp_service_connected(void) {
  return (bt_profile_manager[BT_DEVICE_ID_1].hfp_connect ==
          bt_profile_connect_status_success);
}

btif_remote_device_t *app_bt_get_remoteDev(uint8_t deviceId) {
  btif_remote_device_t *currentRemDev = NULL;

  if (btif_a2dp_get_stream_state(
          app_bt_device.a2dp_stream[deviceId]->a2dp_stream) ==
          BTIF_AVDTP_STRM_STATE_STREAMING ||
      btif_a2dp_get_stream_state(
          app_bt_device.a2dp_stream[deviceId]->a2dp_stream) ==
          BTIF_AVDTP_STRM_STATE_OPEN) {
    currentRemDev = btif_a2dp_get_stream_conn_remDev(
        app_bt_device.a2dp_stream[deviceId]->a2dp_stream);
  } else if (btif_get_hf_chan_state(app_bt_device.hf_channel[deviceId]) ==
             BTIF_HF_STATE_OPEN) {
    currentRemDev = (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(
        app_bt_device.hf_channel[deviceId]);
  }

  TRACE(2, "%s get current Remdev %p", __FUNCTION__, currentRemDev);

  return currentRemDev;
}

void app_bt_stay_active_rem_dev(btif_remote_device_t *pRemDev) {
  if (pRemDev) {
    btif_cmgr_handler_t *cmgrHandler;
    /* Clear the sniff timer */
    cmgrHandler = btif_cmgr_get_acl_handler(pRemDev);
    btif_cmgr_clear_sniff_timer(cmgrHandler);
    btif_cmgr_disable_sniff_timer(cmgrHandler);
    app_bt_Me_SetLinkPolicy(pRemDev, BTIF_BLP_MASTER_SLAVE_SWITCH);
  }
}

void app_bt_stay_active(uint8_t deviceId) {
  btif_remote_device_t *currentRemDev = app_bt_get_remoteDev(deviceId);
  app_bt_stay_active_rem_dev(currentRemDev);
}

void app_bt_allow_sniff_rem_dev(btif_remote_device_t *pRemDev) {
  if (pRemDev &&
      (BTIF_BDS_CONNECTED == btif_me_get_remote_device_state(pRemDev))) {
    btif_cmgr_handler_t *cmgrHandler;
    /* Enable the sniff timer */
    cmgrHandler = btif_cmgr_get_acl_handler(pRemDev);

    /* Start the sniff timer */
    btif_sniff_info_t sniffInfo;
    sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
    sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
    sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
    sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
    if (cmgrHandler) {
      btif_cmgr_set_sniff_timer(cmgrHandler, &sniffInfo, BTIF_CMGR_SNIFF_TIMER);
    }
    app_bt_Me_SetLinkPolicy(pRemDev,
                            BTIF_BLP_MASTER_SLAVE_SWITCH | BTIF_BLP_SNIFF_MODE);
  }
}

extern "C" uint8_t is_sco_mode(void);
void app_bt_allow_sniff(uint8_t deviceId) {
  if (a2dp_is_music_ongoing() || is_sco_mode()) {
    return;
  }
  btif_remote_device_t *currentRemDev = app_bt_get_remoteDev(deviceId);
  app_bt_allow_sniff_rem_dev(currentRemDev);
}

void app_bt_stop_sniff(uint8_t deviceId) {
  btif_remote_device_t *currentRemDev = app_bt_get_remoteDev(deviceId);

  if (currentRemDev &&
      (btif_me_get_remote_device_state(currentRemDev) == BTIF_BDS_CONNECTED)) {
    if (btif_me_get_current_mode(currentRemDev) == BTIF_BLM_SNIFF_MODE) {
      TRACE(1, "!!! stop sniff currmode:%d\n",
            btif_me_get_current_mode(currentRemDev));
      app_bt_ME_StopSniff(currentRemDev);
    }
  }
}

bool app_bt_is_device_connected(uint8_t deviceId) {
  if (deviceId < BT_DEVICE_NUM) {
    return bt_profile_manager[deviceId].has_connected;
  } else {
    // Indicate no connection is user passes invalid deviceId
    return false;
  }
}

#if defined(__BT_SELECT_PROF_DEVICE_ID__)
int8_t app_bt_a2dp_is_same_stream(a2dp_stream_t *src_Stream,
                                  a2dp_stream_t *dst_Stream) {
  return btif_a2dp_is_register_codec_same(src_Stream, dst_Stream);
}
void app_bt_a2dp_find_same_unused_stream(a2dp_stream_t *in_Stream,
                                         a2dp_stream_t **out_Stream,
                                         uint32_t device_id) {
  *out_Stream = NULL;
  if (app_bt_a2dp_is_same_stream(
          app_bt_device.a2dp_stream[device_id]->a2dp_stream, in_Stream))
    *out_Stream = app_bt_device.a2dp_stream[device_id]->a2dp_stream;
#if defined(A2DP_LHDC_ON)
  else if (app_bt_a2dp_is_same_stream(
               app_bt_device.a2dp_lhdc_stream[device_id]->a2dp_stream,
               in_Stream))
    *out_Stream = app_bt_device.a2dp_lhdc_stream[device_id]->a2dp_stream;
#endif
#if defined(A2DP_LDAC_ON)
  else if (app_bt_a2dp_is_same_stream(
               app_bt_device.a2dp_ldac_stream[device_id]->a2dp_stream,
               in_Stream))
    *out_Stream = app_bt_device.a2dp_ldac_stream[device_id]->a2dp_stream;
#endif
#if defined(A2DP_AAC_ON)
  else if (app_bt_a2dp_is_same_stream(
               app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream,
               in_Stream))
    *out_Stream = app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream;
#endif
#if defined(A2DP_SCALABLE_ON)
  else if (app_bt_a2dp_is_same_stream(
               app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream,
               in_Stream))
    *out_Stream = app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream;
#endif
}
int8_t app_bt_a2dp_is_stream_on_device_id(a2dp_stream_t *in_Stream,
                                          uint32_t device_id) {
  if (app_bt_device.a2dp_stream[device_id]->a2dp_stream == in_Stream)
    return 1;
#if defined(A2DP_LHDC_ON)
  else if (app_bt_device.a2dp_lhdc_stream[device_id]->a2dp_stream == in_Stream)
    return 1;
#endif
#if defined(A2DP_LDAC_ON)
  else if (app_bt_device.a2dp_ldac_stream[device_id]->a2dp_stream == in_Stream)
    return 1;
#endif
#if defined(A2DP_AAC_ON)
  else if (app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream == in_Stream)
    return 1;
#endif
#if defined(A2DP_SCALABLE_ON)
  else if (app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream ==
           in_Stream)
    return 1;
#endif
  return 0;
}
int8_t app_bt_hfp_is_chan_on_device_id(hf_chan_handle_t chan,
                                       uint32_t device_id) {
  if (app_bt_device.hf_channel[device_id] == chan)
    return 1;
  return 0;
}
int8_t app_bt_is_any_profile_connected(uint32_t device_id) {
  // TODO avrcp?spp?hid?bisto?ama?dma?rfcomm?
  if ((bt_profile_manager[device_id].hfp_connect ==
       bt_profile_connect_status_success) ||
      (bt_profile_manager[device_id].hsp_connect ==
       bt_profile_connect_status_success) ||
      (bt_profile_manager[device_id].a2dp_connect ==
       bt_profile_connect_status_success)) {
    return 1;
  }

  return 0;
}
int8_t app_bt_is_a2dp_connected(uint32_t device_id) {
  if (bt_profile_manager[device_id].a2dp_connect ==
      bt_profile_connect_status_success) {
    return 1;
  }

  return 0;
}
btif_remote_device_t *app_bt_get_connected_profile_remdev(uint32_t device_id) {
  if (bt_profile_manager[device_id].a2dp_connect ==
      bt_profile_connect_status_success) {
    return (btif_remote_device_t *)btif_a2dp_get_remote_device(
        app_bt_device.a2dp_connected_stream[device_id]);
  } else if (bt_profile_manager[device_id].hfp_connect ==
             bt_profile_connect_status_success) {
    return (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(
        app_bt_device.hf_channel[device_id]);
  }
#if defined(__HSP_ENABLE__)
  else if (bt_profile_manager[device_id].hsp_connect ==
           bt_profile_connect_status_success) {
    // TODO hsp support
    // return (btif_remote_device_t
    // *)btif_hs_cmgr_get_remote_device(app_bt_device.hs_channel[i]);
  }
#endif

  return NULL;
}
#endif

bool app_bt_get_device_bdaddr(uint8_t deviceId, uint8_t *btAddr) {
  bool ret = false;

  if (app_bt_is_device_connected(deviceId)) {
    btif_remote_device_t *currentRemDev = app_bt_get_remoteDev(deviceId);

    if (currentRemDev) {
      memcpy(btAddr, btif_me_get_remote_device_bdaddr(currentRemDev)->address,
             BTIF_BD_ADDR_SIZE);
      ret = true;
    }
  }

  return ret;
}

void fast_pair_enter_pairing_mode_handler(void) {
#if defined(IBRT)
  app_ibrt_ui_judge_scan_type(IBRT_FASTPAIR_TRIGGER, MOBILE_LINK, 0);
#else
  app_bt_accessmode_set(BTIF_BAM_GENERAL_ACCESSIBLE);
#endif

#ifdef __INTERCONNECTION__
  clear_discoverable_adv_timeout_flag();
  app_interceonnection_start_discoverable_adv(
      INTERCONNECTION_BLE_FAST_ADVERTISING_INTERVAL,
      APP_INTERCONNECTION_FAST_ADV_TIMEOUT_IN_MS);
#endif
}

bool app_bt_is_hfp_audio_on(void) {
  bool hfp_audio_is_on = false;
  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    if (BTIF_HF_AUDIO_CON == app_bt_device.hf_audio_state[i]) {
      hfp_audio_is_on = true;
      break;
    }
  }
  return hfp_audio_is_on;
}

btif_remote_device_t *app_bt_get_connected_mobile_device_ptr(void) {
  return connectedMobile;
}
void app_bt_set_spp_device_ptr(btif_remote_device_t *device) {
  TRACE(2, "%s set sppOpenMobile is %p", __func__, device);
  sppOpenMobile = device;
  return;
}

btif_remote_device_t *app_bt_get_spp_device_ptr(void) {
  TRACE(2, "%s sppOpenMobile %p", __func__, sppOpenMobile);
  ASSERT((sppOpenMobile != NULL), "sppOpenMobile is NULL!!!!!!!!!!!!");
  return sppOpenMobile;
}

#ifdef BT_USB_AUDIO_DUAL_MODE
#include "a2dp_api.h"
extern "C" a2dp_stream_t *app_bt_get_steam(enum BT_DEVICE_ID_T id) {
  a2dp_stream_t *stream;

  stream = (a2dp_stream_t *)bt_profile_manager[id].stream;
  return stream;
}

extern "C" int app_bt_get_bt_addr(enum BT_DEVICE_ID_T id, bt_bdaddr_t *bdaddr) {
  memcpy(bdaddr, &bt_profile_manager[id].rmt_addr, sizeof(bt_bdaddr_t));
  return 0;
}

extern "C" bool app_bt_a2dp_service_is_connected(void) {
  return (bt_profile_manager[BT_DEVICE_ID_1].a2dp_connect ==
          bt_profile_connect_status_success);
}
#endif

struct app_bt_search_t {
  bool search_start;
  bool inquiry_pending;
  bool device_searched;
  bt_bdaddr_t address;
};

static bool app_bt_search_device_match(const bt_bdaddr_t *addr,
                                       const char *name) {
  TRACE(7,
        "app_bt_search_callback found device %02x:%02x:%02x:%02x:%02x:%02x "
        "'%s'\n",
        addr->address[0], addr->address[1], addr->address[2], addr->address[3],
        addr->address[4], addr->address[5], name);

#if defined(HFP_MOBILE_AG_ROLE)
  bt_bdaddr_t test_device1 = {{0xd2, 0x53, 0x86, 0x42, 0x71, 0x31}};
  bt_bdaddr_t test_device2 = {{0xd3, 0x53, 0x86, 0x42, 0x71, 0x31}};
  return (memcmp(addr, test_device1.address, sizeof(bt_bdaddr_t)) == 0 ||
          memcmp(addr, test_device2.address, sizeof(bt_bdaddr_t)) == 0);
#else
  return false;
#endif
}

static struct app_bt_search_t g_bt_search;
static void app_bt_search_callback(const btif_event_t *event) {
  TRACE(2, "%s event %d\n", __func__, btif_me_get_callback_event_type(event));

  switch (btif_me_get_callback_event_type(event)) {
  case BTIF_BTEVENT_INQUIRY_RESULT: {
    bt_bdaddr_t *addr = btif_me_get_callback_event_inq_result_bd_addr(event);
    uint8_t mode = btif_me_get_callback_event_inq_result_inq_mode(event);
    const int NAME_MAX_LEN = 255;
    char device_name[NAME_MAX_LEN + 1] = {0};
    int device_name_len = 0;
    uint8_t *eir = NULL;

    if ((mode == BTIF_INQ_MODE_EXTENDED) &&
        (eir = btif_me_get_callback_event_inq_result_ext_inq_resp(event))) {
      device_name_len = btif_me_get_ext_inq_data(
          eir, 0x09, (uint8_t *)device_name, NAME_MAX_LEN);
    }

    if (app_bt_search_device_match(addr,
                                   device_name_len > 0 ? device_name : "")) {
      g_bt_search.address = *addr;
      g_bt_search.device_searched = true;
      btif_me_cancel_inquiry();
    }
  } break;
  case BTIF_BTEVENT_INQUIRY_COMPLETE:
  case BTIF_BTEVENT_INQUIRY_CANCELED:
    btif_me_unregister_globa_handler((btif_handler *)btif_me_get_bt_handler());
    g_bt_search.search_start = false;
    g_bt_search.inquiry_pending = false;
    if (g_bt_search.device_searched) {
#if defined(HFP_MOBILE_AG_ROLE)
      bt_profile_manager[BT_DEVICE_ID_1].reconnect_mode =
          bt_profile_reconnect_null;
      bt_profile_manager[BT_DEVICE_ID_1].rmt_addr = g_bt_search.address;
      bt_profile_manager[BT_DEVICE_ID_1].chan =
          app_bt_device.hf_channel[BT_DEVICE_ID_1];
      app_bt_precheck_before_starting_connecting(
          bt_profile_manager[BT_DEVICE_ID_1].has_connected);
      app_bt_HF_CreateServiceLink(bt_profile_manager[BT_DEVICE_ID_1].chan,
                                  &bt_profile_manager[BT_DEVICE_ID_1].rmt_addr);
#endif
    } else {
      TRACE(1, "%s no device matched\n", __func__);
#if 0
                /* continue to search ??? */
                app_bt_start_search();
#endif
    }
    break;
  default:
    break;
  }
}

void app_bt_start_search(void) {
  uint8_t max_search_time = 10; /* 12.8s */

  if (g_bt_search.search_start) {
    TRACE(1, "%s already started\n", __func__);
    return;
  }

  btif_me_set_handler(btif_me_get_bt_handler(), app_bt_search_callback);

  btif_me_set_event_mask(
      btif_me_get_bt_handler(),
      BTIF_BEM_INQUIRY_RESULT | BTIF_BEM_INQUIRY_COMPLETE |
          BTIF_BEM_INQUIRY_CANCELED | BTIF_BEM_LINK_CONNECT_IND |
          BTIF_BEM_LINK_CONNECT_CNF | BTIF_BEM_LINK_DISCONNECT |
          BTIF_BEM_ROLE_CHANGE | BTIF_BEM_MODE_CHANGE);

  btif_me_register_global_handler(btif_me_get_bt_handler());

  g_bt_search.search_start = true;
  g_bt_search.device_searched = false;
  g_bt_search.inquiry_pending = false;

  if (BT_STS_PENDING != btif_me_inquiry(BTIF_BT_IAC_GIAC, max_search_time, 0)) {
    TRACE(1, "%s start inquiry failed\n", __func__);
    g_bt_search.inquiry_pending = true;
  }
}

uint8_t app_bt_avrcp_get_notify_trans_id(void) {
  return btif_a2dp_get_avrcpadvancedpdu_trans_id(
      app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1]);
}

void app_bt_avrcp_set_notify_trans_id(uint8_t trans_id) {
  TRACE(3, "%s %d %p\n", __func__, trans_id,
        app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1]);
  btif_a2dp_set_avrcpadvancedpdu_trans_id(
      app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1], trans_id);
}

uint8_t app_bt_avrcp_get_ctl_trans_id(void) {
  return btif_avrcp_get_ctl_trans_id(
      app_bt_device.avrcp_channel[BT_DEVICE_ID_1]);
}

void app_bt_avrcp_set_ctl_trans_id(uint8_t trans_id) {
  TRACE(3, "%s %d %p\n", __func__, trans_id,
        app_bt_device.avrcp_channel[BT_DEVICE_ID_1]);
  btif_avrcp_set_ctl_trans_id(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],
                              trans_id);
}

void app_bt_set_mobile_a2dp_stream(uint32_t deviceId, a2dp_stream_t *stream) {
  bt_profile_manager[deviceId].stream = stream;
}

#if defined(IBRT)
#if defined(ENHANCED_STACK)
uint32_t app_bt_save_spp_app_ctx(uint32_t app_id, btif_remote_device_t *rem_dev,
                                 uint8_t *buf, uint32_t buf_len) {
  bt_bdaddr_t *remote = NULL;
  uint32_t offset = 0;
  struct spp_device *device = (struct spp_device *)btif_spp_get_device(app_id);
  ASSERT(device, "%s NULL spp device app_id=0x%x", __func__, app_id);

  // save app_id
  buf[offset++] = app_id & 0xFF;
  buf[offset++] = (app_id >> 8) & 0xFF;
  buf[offset++] = (app_id >> 16) & 0xFF;
  buf[offset++] = (app_id >> 24) & 0xFF;

  // save port type
  buf[offset++] = device->portType;

  // save remote address
  remote = btif_me_get_remote_device_bdaddr(rem_dev);
  memcpy(buf + offset, remote, sizeof(bt_bdaddr_t));
  offset += sizeof(bt_bdaddr_t);

  // TRACE(7,"%s:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
  //    __func__, remote->addr[5], remote->addr[4], remote->addr[3],
  //               remote->addr[2], remote->addr[1], remote->addr[0]);

  // spp device
  buf[offset++] = device->spp_connected_flag;

  return offset;
}

uint32_t app_bt_restore_spp_app_ctx(uint8_t *buf, uint32_t buf_len,
                                    uint32_t app_id) {
  bt_bdaddr_t remote;
  uint32_t offset = 0;
  struct spp_device *device = NULL;
  uint8_t i = 0;
  uint8_t port_type = 0;
  uint32_t app_id_restore = 0;

  // restore app_id
  for (i = 0; i < 4; i++) {
    app_id_restore += (buf[offset + i] << (8 * i));
  }
  offset += 4;

  port_type = buf[offset++];

  ASSERT(app_id_restore == app_id, "%s,spp app id mismatch=%x,%x", __func__,
         app_id_restore, app_id);

  // restore remote address
  memcpy(&remote, buf + offset, sizeof(remote));
  offset += sizeof(remote);

  device = (struct spp_device *)btif_spp_get_device(app_id);

#ifdef __INTERCONNECTION__
  TRACE(1, "%s,%x,%x,%x", __func__, app_id, BTIF_APP_SPP_CLIENT_CCMP_ID,
        BTIF_APP_SPP_CLIENT_RED_ID);
#endif

  if (device == NULL) {
    /*
     * SPP client device may not be created in bt host initialized stage,so IBRT
     * SLAVE will restore it
     */
    if (port_type == BTIF_SPP_CLIENT_PORT) {
      TRACE(1, "%s,spp client device null", __func__);
      switch (app_id) {
#ifdef __INTERCONNECTION__
      case BTIF_APP_SPP_CLIENT_CCMP_ID:
                app_ccmp_client_open((uint8_t *)SppServiceSearchReq,
                                     app_interconnection_get_length(), 0, 1);
                device = (struct spp_device *)btif_spp_get_device(
                    BTIF_APP_SPP_CLIENT_CCMP_ID);
                device->spp_callback = ccmp_callback;
                // device->_channel = chnl; //restore in
                // btif_spp_profile_restore_ctx
                device->sppUsedFlag = 1;
                break;

      case BTIF_APP_SPP_CLIENT_RED_ID:
                app_spp_client_open((uint8_t *)SppServiceSearchReq,
                                    app_interconnection_get_length(), 1);
                device = (struct spp_device *)btif_spp_get_device(
                    BTIF_APP_SPP_CLIENT_RED_ID);
                device->spp_callback = spp_client_callback;
                // device->_channel = chnl; //restore in
                // btif_spp_profile_restore_ctx
                device->sppUsedFlag = 1;
                break;
#endif

      default:
                ASSERT(device, "%s NULL spp client device app_id=0x%x",
                       __func__, app_id);
                break;
      }
    } else {
      ASSERT(device, "%s NULL spp server device app_id=0x%x", __func__, app_id);
    }
  }

#ifdef __INTERCONNECTION__
  if (app_id == BTIF_APP_SPP_CLIENT_RED_ID) {
    // btm_conn will NULL if only SPP profile since btm_conn restore later
    app_bt_set_spp_device_ptr(
        (btif_remote_device_t *)btif_me_get_remote_device_by_bdaddr(&remote));
  }
#endif

  // restore spp device
  device->portType = port_type;
  device->spp_connected_flag = buf[offset++];

  return offset;
}

uint32_t app_bt_save_hfp_app_ctx(btif_remote_device_t *rem_dev, uint8_t *buf,
                                 uint32_t buf_len) {
  BTIF_CTX_INIT(buf);

  BTIF_CTX_STR_BUF(btif_me_get_remote_device_bdaddr(rem_dev),
                   BTIF_BD_ADDR_SIZE);

  BTIF_CTX_STR_VAL8(app_bt_device.hfchan_call[BT_DEVICE_ID_1]);
  BTIF_CTX_STR_VAL8(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]);
  BTIF_CTX_STR_VAL8(app_bt_device.hf_callheld[BT_DEVICE_ID_1]);

  BTIF_CTX_SAVE_UPDATE_DATA_LEN();
  return BTIF_CTX_GET_TOTAL_LEN();
}

uint32_t app_bt_restore_hfp_app_ctx(uint8_t *buf, uint32_t buf_len) {
  bt_bdaddr_t remote;
  uint8_t call, callsetup, callheld;
  BTIF_CTX_INIT(buf);

  BTIF_CTX_LDR_BUF(&remote, BTIF_BD_ADDR_SIZE);

  BTIF_CTX_LDR_VAL8(call);
  BTIF_CTX_LDR_VAL8(callsetup);
  BTIF_CTX_LDR_VAL8(callheld);

  app_bt_device.hfchan_call[BT_DEVICE_ID_1] = call;
  app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] = callsetup;
  app_bt_device.hf_callheld[BT_DEVICE_ID_1] = callheld;

  TRACE(4, "%s call %d callsetup %d callheld %d", __func__, call, callsetup,
        callheld);

  return BTIF_CTX_GET_TOTAL_LEN();
}
uint32_t app_bt_save_a2dp_app_ctx(btif_remote_device_t *rem_dev, uint8_t *buf,
                                  uint32_t buf_len) {
  uint32_t offset = 0;
  unsigned char stream_enc = 0;
  uint32_t factor = 0;

  // TODO
  // more codecs, BT_DEVICE_ID_2
  if (bt_profile_manager[BT_DEVICE_ID_1].stream ==
      app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream) {
    stream_enc = 0;
  }
#if defined(A2DP_AAC_ON)
  else if (bt_profile_manager[BT_DEVICE_ID_1].stream ==
           app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream) {
    stream_enc = 1;
  }
#endif
#if defined(A2DP_LHDC_ON)
  else if (bt_profile_manager[BT_DEVICE_ID_1].stream ==
           app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream) {
    stream_enc = 2;
  }
#endif
#if defined(A2DP_LDAC_ON)
  else if (bt_profile_manager[BT_DEVICE_ID_1].stream ==
           app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream) {
    stream_enc = 2;
  }
#endif

  buf[offset++] = stream_enc;
  memcpy(buf + offset, btif_me_get_remote_device_bdaddr(rem_dev),
         BTIF_BD_ADDR_SIZE);
  offset += BTIF_BD_ADDR_SIZE;

  buf[offset++] = app_bt_device.a2dp_state[BT_DEVICE_ID_1];
  buf[offset++] = app_bt_device.a2dp_play_pause_flag;
  buf[offset++] = avrcp_get_media_status();

  // codec
  buf[offset++] = app_bt_device.codec_type[BT_DEVICE_ID_1];
  buf[offset++] = app_bt_device.sample_rate[BT_DEVICE_ID_1];
  buf[offset++] = app_bt_device.sample_bit[BT_DEVICE_ID_1];
#if defined(A2DP_LHDC_ON)
  buf[offset++] = app_bt_device.a2dp_lhdc_llc[BT_DEVICE_ID_1];
#endif

#if defined(__A2DP_AVDTP_CP__)
  buf[offset++] = app_bt_device.avdtp_cp[BT_DEVICE_ID_1];
#endif

  // volume
  buf[offset++] = (uint8_t)a2dp_volume_get(BT_DEVICE_ID_1);

  // latency factor
  factor = (uint32_t)a2dp_audio_latency_factor_get();
  buf[offset++] = factor & 0xFF;
  buf[offset++] = (factor >> 8) & 0xFF;
  buf[offset++] = (factor >> 16) & 0xFF;
  buf[offset++] = (factor >> 24) & 0xFF;

  // a2dp session
  buf[offset++] = a2dp_ibrt_session_get() & 0xFF;
  buf[offset++] = (a2dp_ibrt_session_get() >> 8) & 0xFF;
  buf[offset++] = (a2dp_ibrt_session_get() >> 16) & 0xFF;
  buf[offset++] = (a2dp_ibrt_session_get() >> 24) & 0xFF;

  return offset;
}

uint32_t app_bt_restore_a2dp_app_ctx(uint8_t *buf, uint32_t buf_len) {
  uint32_t offset = 0;
  bt_bdaddr_t remote;
  unsigned char stream_enc = 0;

  stream_enc = buf[offset++];

  memcpy(&remote, buf + offset, BTIF_BD_ADDR_SIZE);
  offset += BTIF_BD_ADDR_SIZE;

  app_bt_device.a2dp_state[BT_DEVICE_ID_1] = buf[offset++];
  app_bt_device.a2dp_play_pause_flag = buf[offset++];
  avrcp_set_media_status(buf[offset++]);

  // codec info
  app_bt_device.codec_type[BT_DEVICE_ID_1] = buf[offset++];
  app_bt_device.sample_rate[BT_DEVICE_ID_1] = buf[offset++];
  app_bt_device.sample_bit[BT_DEVICE_ID_1] = buf[offset++];
#if defined(A2DP_LHDC_ON)
  app_bt_device.a2dp_lhdc_llc[BT_DEVICE_ID_1] = buf[offset++];
#endif

#if defined(__A2DP_AVDTP_CP__)
  app_bt_device.avdtp_cp[BT_DEVICE_ID_1] = buf[offset++];
#endif

  // volume
  a2dp_volume_set(BT_DEVICE_ID_1, buf[offset++]);

  // latency factor
  a2dp_audio_latency_factor_set((float)(buf[offset] + (buf[offset + 1] << 8) +
                                        (buf[offset + 2] << 16) +
                                        (buf[offset + 3] << 24)));
  offset += 4;

  // a2dp session
  a2dp_ibrt_session_set((buf[offset] + (buf[offset + 1] << 8) +
                         (buf[offset + 2] << 16) + (buf[offset + 3] << 24)));
  offset += 4;

  // TODO
  // more codecs, BT_DEVICE_ID_2
  if (stream_enc == 0) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
  }
#if defined(A2DP_AAC_ON)
  else if (stream_enc == 1) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;
  }
#endif
#if defined(A2DP_LHDC_ON)
  else if (stream_enc == 2) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream;
  }
#endif
#if defined(A2DP_LDAC_ON)
  else if (stream_enc == 2) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream;
  }
#endif

  memcpy(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address, &remote,
         BTIF_BD_ADDR_SIZE);
  bt_profile_manager[BT_DEVICE_ID_1].a2dp_connect =
      bt_profile_connect_status_success;
  bt_profile_manager[BT_DEVICE_ID_1].hfp_connect =
      bt_profile_connect_status_success;
  bt_profile_manager[BT_DEVICE_ID_1].has_connected = true;

  return offset;
}

uint32_t app_bt_save_avrcp_app_ctx(btif_remote_device_t *rem_dev, uint8_t *buf,
                                   uint32_t buf_len) {
  uint32_t offset = 0;

  buf[offset++] = app_bt_device.avrcp_state[BT_DEVICE_ID_1];
  buf[offset++] = app_bt_device.volume_report[BT_DEVICE_ID_1];

  if (app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1]) {
    buf[offset++] = true;
    buf[offset++] = app_bt_avrcp_get_notify_trans_id();
  } else {
    buf[offset++] = false;
    buf[offset++] = 0;
  }

  return offset;
}

uint32_t app_bt_restore_avrcp_app_ctx(uint8_t *buf, uint32_t buf_len) {
  uint32_t offset = 0;
  uint8_t notify_rsp_exist = 0;
  uint8_t trans_id = 0;

  app_bt_device.avrcp_state[BT_DEVICE_ID_1] = buf[offset++];
  app_bt_device.volume_report[BT_DEVICE_ID_1] = buf[offset++];
  notify_rsp_exist = buf[offset++];
  trans_id = buf[offset++];

  TRACE(4, "app_bt_restore_avrcp_app_ctx state %d report %d notify %d %d\n",
        app_bt_device.avrcp_state[BT_DEVICE_ID_1],
        app_bt_device.volume_report[BT_DEVICE_ID_1], notify_rsp_exist,
        trans_id);

  if (notify_rsp_exist &&
      app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1] == NULL) {
    btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(
        &app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1]);
  }

  app_bt_avrcp_set_notify_trans_id(trans_id);

  return offset;
}

#ifdef __BTMAP_ENABLE__
uint32_t app_bt_save_map_app_ctx(btif_remote_device_t *rem_dev, uint8_t *buf,
                                 uint32_t buf_len) {
  // struct bdaddr_t *remote = NULL;
  uint32_t offset = 0;

  memcpy((void *)buf, (void *)app_bt_device.map_session_handle,
         sizeof(app_bt_device.map_session_handle));
  offset += sizeof(app_bt_device.map_session_handle);

  return offset;
}

uint32_t app_bt_restore_map_app_ctx(uint8_t *buf, uint32_t buf_len) {
  uint32_t offset = 0;

  memcpy((void *)app_bt_device.map_session_handle, (void *)buf,
         sizeof(btif_map_session_handle_t));
  offset += sizeof(btif_map_session_handle_t);

  return offset;
}
#endif

#if BTIF_HID_DEVICE
uint32_t app_bt_save_hid_app_ctx(uint8_t *buf) {
  uint32_t offset = 0;

  if (app_bt_device.hid_channel == NULL) {
    TRACE(0, "app_bt_save_hid_app_ctx app_bt_device.hid_channel is NULL");
    return offset;
  }

  memcpy((void *)buf, (void *)app_bt_device.hid_channel,
         sizeof(app_bt_device.hid_channel));
  offset += sizeof(app_bt_device.hid_channel);

  return offset;
}

uint32_t app_bt_restore_hid_app_ctx(uint8_t *buf) {
  uint32_t offset = 0;

  memcpy((void *)app_bt_device.hid_channel, (void *)buf, sizeof(hid_channel_t));
  offset += sizeof(hid_channel_t);

  return offset;
}
#else
uint32_t app_bt_restore_hid_app_ctx(uint8_t *buf) { return 4; }
uint32_t app_bt_save_hid_app_ctx(uint8_t *buf) {
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  return 4;
}

#endif
#endif /* ENHANCED_STACK */

a2dp_stream_t *app_bt_get_mobile_a2dp_stream(uint32_t deviceId) {
  return bt_profile_manager[deviceId].stream;
}

void app_bt_update_bt_profile_manager(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  memcpy(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address,
         p_ibrt_ctrl->mobile_addr.address, BTIF_BD_ADDR_SIZE);

#if defined(A2DP_AAC_ON)
  if (p_ibrt_ctrl->a2dp_codec.codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;

  } else
#endif
#if defined(A2DP_SCALABLE_ON)
      if (p_ibrt_ctrl->a2dp_codec.codec_type ==
          BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else
#endif
#if defined(A2DP_LHDC_ON)
      if (p_ibrt_ctrl->a2dp_codec.codec_type ==
          BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else
#endif
#if defined(A2DP_LDAC_ON)
      if (p_ibrt_ctrl->a2dp_codec.codec_type ==
          BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else
#endif
      if (p_ibrt_ctrl->a2dp_codec.codec_type == BTIF_AVDTP_CODEC_TYPE_SBC) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else {
    ASSERT(0, "%s err codec_type:%d ", __func__,
           p_ibrt_ctrl->a2dp_codec.codec_type);
  }

  bt_profile_manager[BT_DEVICE_ID_1].a2dp_connect =
      bt_profile_connect_status_success;
  bt_profile_manager[BT_DEVICE_ID_1].hfp_connect =
      bt_profile_connect_status_success;
  bt_profile_manager[BT_DEVICE_ID_1].has_connected = true;

  TRACE(3, "%s codec_type:%x if_a2dp_stream:%p", __func__,
        p_ibrt_ctrl->a2dp_codec.codec_type,
        bt_profile_manager[BT_DEVICE_ID_1].stream);
  DUMP8("%02x ", bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address,
        BTIF_BD_ADDR_SIZE);
}

void app_bt_update_bt_profile_manager_codec_type(uint8_t codec_type) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  p_ibrt_ctrl->a2dp_codec.codec_type = codec_type;

#if defined(A2DP_AAC_ON)
  if (p_ibrt_ctrl->a2dp_codec.codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;

  } else
#endif
#if defined(A2DP_SCALABLE_ON)
      if (p_ibrt_ctrl->a2dp_codec.codec_type ==
          BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else
#endif
#if defined(A2DP_LHDC_ON)
      if (p_ibrt_ctrl->a2dp_codec.codec_type ==
          BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else
#endif
#if defined(A2DP_LDAC_ON)
      if (p_ibrt_ctrl->a2dp_codec.codec_type ==
          BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else
#endif
      if (p_ibrt_ctrl->a2dp_codec.codec_type == BTIF_AVDTP_CODEC_TYPE_SBC) {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
  } else {
    ASSERT(0, "%s err codec_type:%d ", __func__,
           p_ibrt_ctrl->a2dp_codec.codec_type);
  }

  bt_profile_manager[BT_DEVICE_ID_1].a2dp_connect =
      bt_profile_connect_status_success;
  bt_profile_manager[BT_DEVICE_ID_1].hfp_connect =
      bt_profile_connect_status_success;
  bt_profile_manager[BT_DEVICE_ID_1].has_connected = true;

  TRACE(3, "%s codec_type:%x if_a2dp_stream:%p", __func__,
        p_ibrt_ctrl->a2dp_codec.codec_type,
        bt_profile_manager[BT_DEVICE_ID_1].stream);
}

static bool ibrt_reconnect_mobile_profile_flag = false;
void app_bt_ibrt_reconnect_mobile_profile_flag_set(void) {
  ibrt_reconnect_mobile_profile_flag = true;
}

void app_bt_ibrt_reconnect_mobile_profile_flag_clear(void) {
  ibrt_reconnect_mobile_profile_flag = false;
}

bool app_bt_ibrt_reconnect_mobile_profile_flag_get(void) {
  return ibrt_reconnect_mobile_profile_flag;
}

void app_bt_ibrt_reconnect_mobile_profile(bt_bdaddr_t mobile_addr) {
  nvrec_btdevicerecord *mobile_record = NULL;

  bt_profile_manager[BT_DEVICE_ID_1].reconnect_mode = bt_profile_reconnect_null;
  bt_profile_manager[BT_DEVICE_ID_1].rmt_addr = mobile_addr;
  bt_profile_manager[BT_DEVICE_ID_1].chan =
      app_bt_device.hf_channel[BT_DEVICE_ID_1];

  if (!nv_record_btdevicerecord_find(&mobile_addr, &mobile_record)) {
#if defined(A2DP_AAC_ON)
    if (mobile_record->device_plf.a2dp_codectype ==
        BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
      bt_profile_manager[BT_DEVICE_ID_1].stream =
          app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;
    } else
#endif
#if defined(A2DP_SCALABLE_ON)
        if (mobile_record->device_plf.a2dp_codectype ==
            BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
      bt_profile_manager[BT_DEVICE_ID_1].stream =
          app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream;
    } else
#endif
#if defined(A2DP_LHDC_ON)
        if (mobile_record->device_plf.a2dp_codectype ==
            BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
      bt_profile_manager[BT_DEVICE_ID_1].stream =
          app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream;
    } else
#endif
#if defined(A2DP_LDAC_ON)
        if (mobile_record->device_plf.a2dp_codectype ==
            BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
      bt_profile_manager[BT_DEVICE_ID_1].stream =
          app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream;
    } else
#endif
    {
      bt_profile_manager[BT_DEVICE_ID_1].stream =
          app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
    }
  } else {
    bt_profile_manager[BT_DEVICE_ID_1].stream =
        app_bt_device.a2dp_stream[BT_DEVICE_ID_1]
            ->a2dp_stream; // default using SBC
  }

  btif_a2dp_reset_stream_state(bt_profile_manager[BT_DEVICE_ID_1].stream);

  TRACE(0, "ibrt_ui_log:start reconnect mobile, addr below:");
  DUMP8("0x%02x ", &(mobile_addr.address[0]), BTIF_BD_ADDR_SIZE);
  app_bt_ibrt_reconnect_mobile_profile_flag_set();
  app_bt_precheck_before_starting_connecting(
      bt_profile_manager[BT_DEVICE_ID_1].has_connected);

  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
  if (p_ibrt_ui->config.profile_concurrency_supported) {
    app_bt_A2DP_OpenStream(bt_profile_manager[BT_DEVICE_ID_1].stream,
                           &(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr));
    app_bt_HF_CreateServiceLink(bt_profile_manager[BT_DEVICE_ID_1].chan,
                                &(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr));
  } else {
    app_bt_A2DP_OpenStream(bt_profile_manager[BT_DEVICE_ID_1].stream,
                           &(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr));
    // app_bt_HF_CreateServiceLink(bt_profile_manager[BT_DEVICE_ID_1].chan,
    // &(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr));
  }
  // osTimerStart(bt_profile_manager[BT_DEVICE_ID_1].connect_timer,
  // APP_IBRT_RECONNECT_TIMEOUT_MS);
}
#endif

#ifdef __IAG_BLE_INCLUDE__
static void app_start_fast_connectable_ble_adv(uint16_t advInterval) {
  bool ret = FALSE;

  if (NULL == app_fast_ble_adv_timeout_timer) {
    app_fast_ble_adv_timeout_timer = osTimerCreate(
        osTimer(APP_FAST_BLE_ADV_TIMEOUT_TIMER), osTimerOnce, NULL);
  }

  osTimerStart(app_fast_ble_adv_timeout_timer, APP_FAST_BLE_ADV_TIMEOUT_IN_MS);

#ifdef IBRT
  ret = app_ibrt_ui_get_snoop_via_ble_enable();
#endif

  if (FALSE == ret) {
    app_ble_start_connectable_adv(advInterval);
  }
}

static int app_fast_ble_adv_timeout_timehandler(void const *param) {
  bool ret = FALSE;

#ifdef IBRT
  ret = app_ibrt_ui_get_snoop_via_ble_enable();
#endif

  if (FALSE == ret) {
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
  }

  return 0;
}

void app_stop_fast_connectable_ble_adv_timer(void) {
  if (NULL != app_fast_ble_adv_timeout_timer) {
    osTimerStop(app_fast_ble_adv_timeout_timer);
  }
}
#endif

static uint32_t bt_link_active_mode_bits[MAX_ACTIVE_MODE_MANAGED_LINKS];

void app_bt_active_mode_manager_init(void) {
  memset(bt_link_active_mode_bits, 0, sizeof(bt_link_active_mode_bits));
}

void app_bt_active_mode_reset(uint32_t linkIndex) {
  bt_link_active_mode_bits[linkIndex] = 0;
}

void app_bt_active_mode_set(BT_LINK_ACTIVE_MODE_KEEPER_USER_E user,
                            uint32_t linkIndex) {
#if defined(IBRT)
  return;
#endif

  bool isAlreadyInActiveMode = false;
  if (linkIndex < MAX_ACTIVE_MODE_MANAGED_LINKS) {
    uint32_t lock = int_lock_global();
    if (bt_link_active_mode_bits[linkIndex] > 0) {
      isAlreadyInActiveMode = true;
    } else {
      isAlreadyInActiveMode = false;
    }
    bt_link_active_mode_bits[linkIndex] |= (1 << user);
    int_unlock_global(lock);

    if (!isAlreadyInActiveMode) {
      app_bt_stop_sniff(linkIndex);
      app_bt_stay_active(linkIndex);
    }

  } else if (MAX_ACTIVE_MODE_MANAGED_LINKS == linkIndex) {
    for (uint8_t devId = 0; devId < BT_DEVICE_NUM; devId++) {
      uint32_t lock = int_lock_global();
      if (bt_link_active_mode_bits[devId] > 0) {
                isAlreadyInActiveMode = true;
      } else {
                isAlreadyInActiveMode = false;
      }
      bt_link_active_mode_bits[devId] |= (1 << user);
      int_unlock_global(lock);

      if (!isAlreadyInActiveMode) {
                app_bt_stop_sniff(devId);
                app_bt_stay_active(devId);
      }
    }
  }

  TRACE(2, "set active mode for user %d, link %d, now state:", user, linkIndex);
  DUMP32("%08x", bt_link_active_mode_bits, MAX_ACTIVE_MODE_MANAGED_LINKS);
}

void app_bt_active_mode_clear(BT_LINK_ACTIVE_MODE_KEEPER_USER_E user,
                              uint32_t linkIndex) {
#if defined(IBRT)
  return;
#endif

  bool isAlreadyAllowSniff = false;
  if (linkIndex < MAX_ACTIVE_MODE_MANAGED_LINKS) {
    uint32_t lock = int_lock_global();

    if (0 == bt_link_active_mode_bits[linkIndex]) {
      isAlreadyAllowSniff = true;
    } else {
      isAlreadyAllowSniff = false;
    }

    bt_link_active_mode_bits[linkIndex] &= (~(1 << user));

    int_unlock_global(lock);

    if (!isAlreadyAllowSniff) {
      app_bt_allow_sniff(linkIndex);
    }
  } else if (MAX_ACTIVE_MODE_MANAGED_LINKS == linkIndex) {
    for (uint8_t devId = 0; devId < BT_DEVICE_NUM; devId++) {
      uint32_t lock = int_lock_global();
      if (0 == bt_link_active_mode_bits[devId]) {
                isAlreadyAllowSniff = true;
      } else {
                isAlreadyAllowSniff = false;
      }
      bt_link_active_mode_bits[devId] &= (~(1 << user));
      int_unlock_global(lock);

      if (!isAlreadyAllowSniff) {
                app_bt_allow_sniff(devId);
      }
    }
  }

  TRACE(2, "clear active mode for user %d, link %d, now state:", user,
        linkIndex);
  DUMP32("%08x ", bt_link_active_mode_bits, MAX_ACTIVE_MODE_MANAGED_LINKS);
}

int8_t app_bt_get_rssi(void) {
  int8_t rssi = 127;
  uint8_t i;
  btif_remote_device_t *remDev = NULL;
  rx_agc_t tws_agc = {0};

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    remDev = btif_me_enumerate_remote_devices(i);
    if (remDev) {
      if (btif_me_get_remote_device_hci_handle(remDev)) {
                rssi = bt_drv_reg_op_read_rssi_in_dbm(
                    btif_me_get_remote_device_hci_handle(remDev), &tws_agc);
                rssi = bt_drv_reg_op_rssi_correction(rssi);
                TRACE(1, " headset to mobile RSSI:%d dBm", rssi);
      }
    }
  }
  return rssi;
}

#ifdef TILE_DATAPATH
int8_t app_tile_get_ble_rssi(void) {
  int8_t rssi = 127;
  uint8_t i;
  btif_remote_device_t *remDev = NULL;
  rx_agc_t tws_agc = {0};

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    remDev = btif_me_enumerate_remote_devices(i);
    if (remDev) {
      if (app_tile_ble_get_connection_index() != BLE_INVALID_CONNECTION_INDEX) {
                rssi = bt_drv_reg_op_read_ble_rssi_in_dbm(
                    app_tile_ble_get_connection_index(), &tws_agc);
                rssi = bt_drv_reg_op_rssi_correction(rssi);
                TRACE(1, " headset to mobile RSSI:%d dBm", rssi);
      }
    }
  }
  return rssi;
}
#endif

#ifdef __GMA_VOICE__
void app_bt_prepare_for_ota(void) {
  app_ibrt_ui_t *p_ui_ctrl = app_ibrt_ui_get_ctx();

  p_ui_ctrl->config.disable_tws_switch = true; // disable role switch

  app_key_close();

  if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL)) {
    btif_hf_disconnect_service_link(app_bt_device.hf_channel[BT_DEVICE_ID_1]);
    btif_a2dp_suspend_stream(
        app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream);
  }
  TRACE(0, "app_bt_prepare_for_ota");
}
#endif
