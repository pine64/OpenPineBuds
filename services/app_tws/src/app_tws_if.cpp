/***************************************************************************
 *
 *Copyright 2015-2019 BES.
 *All rights reserved. All unpublished rights reserved.
 *
 *No part of this work may be used or reproduced in any form or by any
 *means, or stored in a database or retrieval system, without prior written
 *permission of BES.
 *
 *Use of this work is governed by a license granted by BES.
 *This work contains confidential and proprietary information of
 *BES. which is protected by copyright, trade secret,
 *trademark and other intellectual property rights.
 *
 ****************************************************************************/

/*****************************header include********************************/
#include "app_tws_if.h"
#include "app.h"
#include "app_ai_if.h"
#include "app_ai_manager_api.h"
#include "app_ai_tws.h"
#include "app_bt.h"
#include "app_bt_func.h"
#include "app_bt_stream.h"
#include "app_dip.h"
#include "app_ibrt_ui.h"
#include "app_sec.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "besbt.h"
#include "btapp.h"
#include "cmsis.h"
#include "hal_trace.h"
#include "me_api.h"
#include "nvrecord_ble.h"
#include "nvrecord_env.h"
#include "nvrecord_extension.h"
#include "tgt_hardware.h"
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef IBRT
#include "app_ibrt_customif_cmd.h"
#include "app_ibrt_customif_ui.h"
#include "app_ibrt_if.h"
#endif

#ifdef BLE_ENABLE
#include "app_ble_include.h"
#endif

#ifdef OTA_ENABLED
#include "ota_common.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#include "gsound_custom_tws.h"
#endif

#ifdef __AI_VOICE__
#include "ai_thread.h"
#endif

#ifdef IBRT_OTA
#include "ota_control.h"
#endif

#ifdef IS_MULTI_AI_ENABLED
#include "ai_manager.h"
#endif

#ifdef GFPS_ENABLED
#include "app_gfps.h"
#include "nvrecord_fp_account_key.h"
#endif

#ifdef __GMA_VOICE__
#include "app_gma_ota.h"
#endif

/************************private macro defination***************************/

/************************private struct defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/
static TWS_ENV_T twsEnv;
static TWS_SYNC_DATA_T twsSyncBuf;

osMutexId twsSyncBufMutexId = NULL;
osMutexDef(twsSyncBufMutex);

#define POST_ROLESWITCH_STATE_STAYING_MS 15000
osTimerId app_post_roleswitch_handling_timer_id = NULL;
static int app_post_roleswitch_handling_timer_handler(void const *param);
osTimerDef(APP_POST_ROLESWITCH_HANDLING_TIMER,
           (void (*)(void const *))
               app_post_roleswitch_handling_timer_handler); // define timers

/****************************function defination****************************/
extern int hfp_volume_get(enum BT_DEVICE_ID_T id);
extern int a2dp_volume_get(enum BT_DEVICE_ID_T id);
static const char *tws_sync_user2str(TWS_SYNC_USER_E user) {
  const char *str = NULL;
#define CASES(user)                                                            \
  case user:                                                                   \
    str = "[" #user "]";                                                       \
    break

  switch (user) {
    CASES(TWS_SYNC_USER_BLE_INFO);
    CASES(TWS_SYNC_USER_OTA);
    CASES(TWS_SYNC_USER_AI_CONNECTION);
    CASES(TWS_SYNC_USER_GFPS_INFO);
    CASES(TWS_SYNC_USER_AI_INFO);
    CASES(TWS_SYNC_USER_AI_MANAGER);
    CASES(TWS_SYNC_USER_DIP);

  default:
    str = "[INVALID]";
    break;
  }

  return str;
}

void app_tws_if_init(void) {
  // reset the environment
  memset(&twsEnv, 0, sizeof(twsEnv));
  memset(&twsSyncBuf, 0, TWS_SYNC_BUF_SIZE);

  // init the tws sync buffer mutex
  if (NULL == twsSyncBufMutexId) {
    twsSyncBufMutexId = osMutexCreate(osMutex(twsSyncBufMutex));
  }

  // register all users
#ifdef BLE_ENABLE
  // init ble tws interface
  app_ble_mode_tws_sync_init();
#endif

#ifdef OTA_ENABLED
  ota_common_tws_sync_init();
#endif

#ifdef BISTO_ENABLED
  // init gsound tws interface
  app_ai_tws_gsound_sync_init();
#endif

#ifdef GFPS_ENABLED
  app_gfps_tws_sync_init();
#endif

#ifdef __AI_VOICE__
  app_ai_tws_sync_init();
#endif

#ifdef IS_MULTI_AI_ENABLED
  ai_manager_sync_init();
#endif

#ifdef BTIF_DIP_DEVICE
  app_dip_sync_init();
#endif
}

void app_tws_if_role_switch_started_handler(void) {
  TRACE(1, "[%s]+++", __func__);
  if (NULL == app_post_roleswitch_handling_timer_id) {
    app_post_roleswitch_handling_timer_id = osTimerCreate(
        osTimer(APP_POST_ROLESWITCH_HANDLING_TIMER), osTimerOnce, NULL);
  }

  osTimerStop(app_post_roleswitch_handling_timer_id);

  app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_ROLE_SWITCH,
                           UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);

  TRACE(1, "[%s]---", __func__);
}

static int app_post_roleswitch_handling_timer_handler(void const *param) {
  app_bt_start_custom_function_in_bt_thread(ACTIVE_MODE_KEEPER_ROLE_SWITCH,
                                            UPDATE_ACTIVE_MODE_FOR_ALL_LINKS,
                                            (uint32_t)app_bt_active_mode_clear);
  return 0;
}

static void app_tws_if_post_roleswitch_handler(void) {
  if (NULL == app_post_roleswitch_handling_timer_id) {
    app_post_roleswitch_handling_timer_id = osTimerCreate(
        osTimer(APP_POST_ROLESWITCH_HANDLING_TIMER), osTimerOnce, NULL);
  }

  osTimerStart(app_post_roleswitch_handling_timer_id,
               POST_ROLESWITCH_STATE_STAYING_MS);
  // keep bt link in active mode right after role switch
  app_bt_active_mode_set(ACTIVE_MODE_KEEPER_ROLE_SWITCH,
                         UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
}

void app_tws_if_tws_role_switch_complete_handler(uint8_t newRole) {
  TRACE(2, "[%s]+++ role %d", __func__, newRole);

#ifdef BISTO_ENABLED
  gsound_on_system_role_switch_done(newRole);
#endif

  // update BLE info saved in flash after role switch complete
  nv_record_tws_exchange_ble_info();

#ifdef BLE_USE_RPA
  appm_refresh_ble_irk();
#else
  btif_me_set_ble_bd_address(bt_get_ble_local_address());
#endif

#ifdef __AI_VOICE__
  app_ai_tws_role_switch_complete();
#endif

#ifdef BLE_ENABLE
  ble_callback_evnet_t event;
  event.evt_type = BLE_CALLBACK_RS_COMPLETE;
  event.p.rs_complete_handled.newRole = newRole;
  app_ble_core_global_callback_event(&event, NULL);
#endif

  app_tws_if_post_roleswitch_handler();

  TRACE(1, "[%s]---", __func__);

#ifdef IBRT_OTA
  ota_control_send_role_switch_complete();
#endif
}

void app_tws_if_register_sync_user(uint8_t id, TWS_SYNC_USER_T *user) {
  memcpy(&twsEnv.syncUser[id], user, sizeof(TWS_SYNC_USER_T));
}

void app_tws_if_deregister_sync_user(uint8_t id) {
  memset(&twsEnv.syncUser[id], 0, sizeof(TWS_SYNC_USER_T));
}

void app_tws_if_sync_info(TWS_SYNC_USER_E id) {
  TRACE(1, "[%s]+++", __func__);
  TRACE(1, "user:%s", tws_sync_user2str(id));

  osMutexWait(twsSyncBufMutexId, osWaitForever);
  if (twsEnv.syncUser[id].sync_info_prepare_handler) {
    uint16_t length = 0;
    twsEnv.syncUser[id].sync_info_prepare_handler(twsSyncBuf.info, &length);

    if (length) {
      twsSyncBuf.userId = id;
      length++;

      tws_ctrl_send_cmd(APP_TWS_CMD_SHARE_COMMON_INFO, (uint8_t *)&twsSyncBuf,
                        length);
    } else {
      TRACE(0, "no info to sync");
    }
  } else {
    TRACE(0, "prepare handler is null pointer");
  }
  osMutexRelease(twsSyncBufMutexId);

  TRACE(1, "[%s]---", __func__);
}

void app_tws_if_sync_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                           uint16_t length) {
  TRACE(1, "[%s]+++", __func__);

  TWS_SYNC_DATA_T *pInfo = (TWS_SYNC_DATA_T *)p_buff;
  TRACE(1, "user:%s", tws_sync_user2str(pInfo->userId));

  // handle received data
  if (twsEnv.syncUser[pInfo->userId].sync_info_received_handler) {
    twsEnv.syncUser[pInfo->userId].sync_info_received_handler(pInfo->info,
                                                              (length - 1));
  }

  // send response to peer device
  uint16_t len = 0;
  osMutexWait(twsSyncBufMutexId, osWaitForever);

  if (twsEnv.syncUser[pInfo->userId].sync_info_prepare_rsp_handler) {
    twsEnv.syncUser[pInfo->userId].sync_info_prepare_rsp_handler(
        twsSyncBuf.info, &len);
  }

  twsSyncBuf.userId = pInfo->userId;
  len++;

  tws_ctrl_send_rsp(APP_TWS_CMD_SHARE_COMMON_INFO, rsp_seq,
                    (uint8_t *)&twsSyncBuf, len);
  osMutexRelease(twsSyncBufMutexId);

  TRACE(1, "[%s]---", __func__);
}

void app_tws_if_sync_info_rsp_received_handler(uint16_t rsp_seq,
                                               uint8_t *p_buff,
                                               uint16_t length) {
  TRACE(1, "[%s]+++", __func__);

  TWS_SYNC_DATA_T *pInfo = (TWS_SYNC_DATA_T *)p_buff;

  // handle received data
  if (twsEnv.syncUser[pInfo->userId].sync_info_rsp_received_handler) {
    twsEnv.syncUser[pInfo->userId].sync_info_rsp_received_handler(pInfo->info,
                                                                  (length - 1));
  }

  TRACE(1, "[%s]---", __func__);
}

void app_tws_if_sync_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                              uint16_t length) {
  TRACE(1, "[%s]+++", __func__);

  TWS_SYNC_DATA_T *pInfo = (TWS_SYNC_DATA_T *)p_buff;

  if (twsEnv.syncUser[pInfo->userId].sync_info_rsp_timeout_handler) {
    twsEnv.syncUser[pInfo->userId].sync_info_rsp_timeout_handler(pInfo->info,
                                                                 (length - 1));
  }

  TRACE(1, "[%s]---", __func__);
}

void app_tws_if_tws_role_updated_handler(uint8_t newRole) {
#ifdef BLE_ENABLE
  ble_callback_evnet_t event;
  event.evt_type = BLE_CALLBACK_ROLE_UPDATE;
  event.p.role_update_handled.newRole = newRole;
  app_ble_core_global_callback_event(&event, NULL);
#endif

#if defined(BISTO_ENABLED) && defined(IBRT)
  gsound_on_tws_role_updated(newRole);
#endif
}

void app_tws_if_tws_connected_sync_info(void) {
  TRACE(2, "[%s]+++ role %d", __func__, app_tws_ibrt_role_get_callback(NULL));

  if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL)) {
    // inform peer to update the common info
    app_tws_if_sync_info(TWS_SYNC_USER_BLE_INFO);
    app_tws_if_sync_info(TWS_SYNC_USER_OTA);
    // app_tws_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
    app_tws_if_sync_info(TWS_SYNC_USER_GFPS_INFO);
    app_tws_if_sync_info(TWS_SYNC_USER_AI_INFO);
    app_tws_if_sync_info(TWS_SYNC_USER_AI_MANAGER);
    app_tws_if_sync_info(TWS_SYNC_USER_DIP);

#ifdef GFPS_ENABLED
    if (!app_tws_ibrt_mobile_link_connected()) {
      app_enter_fastpairing_mode();
    }
#endif
  }

  TRACE(1, "[%s]---", __func__);
}

void app_tws_if_tws_connected_handler(void) {
  TRACE(1, "twsif_tws_connected, role %d",
        app_tws_ibrt_role_get_callback(NULL));
}

void app_tws_if_tws_disconnected_handler(void) {
  TRACE(0, "twsif_tws_disconnected.");

#ifdef IBRT
  if (IBRT_SLAVE == app_tws_ibrt_role_get_callback(NULL)) {
#ifdef BISTO_ENABLED
    gsound_set_ble_connect_state(GSOUND_CHANNEL_AUDIO, false);
    gsound_set_ble_connect_state(GSOUND_CHANNEL_CONTROL, false);
#endif
  }
#ifdef __GMA_VOICE__
  app_gma_ota_exception_reboot(); // shuailong
#endif
#endif
}

void app_tws_if_mobile_connected_handler(uint8_t *addr) {
  TRACE(0, "twsif_mobile_connected");

#ifdef IBRT
  if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
#endif
  {
    // inform peer to update the common info
    // app_tws_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
    app_tws_if_sync_info(TWS_SYNC_USER_GFPS_INFO);
  }
}

void app_tws_if_mobile_disconnected_handler(uint8_t *addr) {
  TRACE(0, "twsif_mobile_disconnected");
}

void app_tws_if_ibrt_connected_handler(void) {
  TRACE(0, "twsif_ibrt_connected");

#ifdef IBRT
  if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
#endif
  {
    // inform peer to update the common info
    app_tws_if_sync_info(TWS_SYNC_USER_AI_INFO);
    app_tws_if_sync_info(TWS_SYNC_USER_AI_MANAGER);
  }
}

void app_tws_if_ibrt_disconnected_handler(void) {
  TRACE(0, "twsif_ibrt_disconnected");
}

void app_tws_if_ble_connected_handler(void) {
  TRACE(0, "twsif_ble_connected");

#ifdef IBRT
  if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
#endif
  {
    // inform peer to update the common info
    // app_tws_if_sync_info(TWS_SYNC_USER_BLE_INFO);
    app_tws_if_sync_info(TWS_SYNC_USER_OTA);
    // app_tws_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
    app_tws_if_sync_info(TWS_SYNC_USER_GFPS_INFO);
  }
}

void app_tws_if_ble_disconnected_handler(void) {
  TRACE(0, "twsif_ble_disconnected");

#ifdef IBRT
  if (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL))
#endif
  {
    // inform peer to update the common info
    // app_tws_if_sync_info(TWS_SYNC_USER_AI_CONNECTION);
  }
}

void app_tws_if_trigger_role_switch(void) {
  if (false == app_ibrt_ui_can_tws_switch()) {
    TRACE(1, "%s can't switch", __func__);
    return;
  }

  app_ibrt_customif_ui_tws_switch();
}

void app_tws_if_handle_click(void) {
  TRACE(1, "%s", __func__);

#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
  app_tws_if_trigger_role_switch();
#else
  bt_key_handle_func_click();
#endif
}

#ifdef IBRT
void app_tws_if_ai_send_cmd_to_peer(uint8_t *p_buff, uint16_t length) {
  TRACE(1, "[%s]", __func__);

  tws_ctrl_send_cmd(APP_TWS_CMD_AI_SEND_CMD_TO_PEER, p_buff, length);
}

void app_tws_if_ai_rev_peer_cmd_hanlder(uint16_t rsp_seq, uint8_t *p_buff,
                                        uint16_t length) {
  TRACE(1, "[%s]", __func__);

#ifdef __AI_VOICE__
  app_ai_tws_rev_peer_cmd_hanlder(rsp_seq, p_buff, length);
#endif
}

void app_tws_if_ai_send_cmd_with_rsp_to_peer(uint8_t *p_buff, uint16_t length) {
  TRACE(1, "[%s]", __func__);

  tws_ctrl_send_cmd(APP_TWS_CMD_AI_SEND_CMD_TO_PEER_WITH_RSP, p_buff, length);
}

void app_tws_if_ai_send_cmd_rsp_to_peer(uint8_t *p_buff, uint16_t rsp_seq,
                                        uint16_t length) {
  TRACE(1, "[%s]", __func__);

  tws_ctrl_send_rsp(APP_TWS_CMD_AI_SEND_CMD_TO_PEER_WITH_RSP, rsp_seq, p_buff,
                    length);
}

void app_tws_if_ai_rev_peer_cmd_with_rsp_hanlder(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length) {
  TRACE(1, "[%s]", __func__);

#ifdef __AI_VOICE__
  app_ai_tws_rev_peer_cmd_with_rsp_hanlder(rsp_seq, p_buff, length);
#endif
}

void app_tws_if_ai_rev_cmd_rsp_from_peer_hanlder(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length) {
  TRACE(1, "[%s]", __func__);

#ifdef __AI_VOICE__
  app_ai_tws_rev_cmd_rsp_from_peer_hanlder(rsp_seq, p_buff, length);
#endif
}

void app_tws_if_ai_rev_cmd_rsp_timeout_hanlder(uint16_t rsp_seq,
                                               uint8_t *p_buff,
                                               uint16_t length) {
  TRACE(1, "[%s]", __func__);

#ifdef __AI_VOICE__
  app_ai_tws_rev_cmd_rsp_timeout_hanlder(rsp_seq, p_buff, length);
#endif
}

void app_tws_if_master_prepare_rs(uint8_t *p_buff, uint16_t length) {
#ifdef BLE_ENABLE
  ble_callback_evnet_t event;
  event.evt_type = BLE_CALLBACK_RS_START;
  app_ble_core_global_callback_event(&event, NULL);
#endif
#ifdef BISTO_ENABLED
  if (*p_buff == AI_SPEC_GSOUND) {
    gsound_tws_update_roleswitch_initiator(IBRT_SLAVE);
    gsound_tws_request_roleswitch();
  }
#endif
#ifdef __AI_VOICE__
  if (*p_buff != AI_SPEC_GSOUND) {
    app_ai_tws_master_role_switch_prepare();
  }
#endif
}

void app_tws_if_slave_continue_rs(uint8_t *p_buff, uint16_t length) {
  TRACE(2, "%s len %d", __func__, length);
  DUMP8("%x ", p_buff, length);

#ifdef __AI_VOICE__
  if (*p_buff != AI_SPEC_GSOUND) {
    app_ai_tws_role_switch_prepare_done();
  } else
#endif
  {
    app_ibrt_if_tws_switch_prepare_done_in_bt_thread(IBRT_ROLE_SWITCH_USER_AI,
                                                     (uint32_t)*p_buff);
  }
}
#endif

#if defined(GFPS_ENABLED) && defined(IBRT)
void app_ibrt_share_fastpair_info(uint8_t *p_buff, uint16_t length) {
  app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_SHARE_FASTPAIR_INFO, p_buff,
                                length);
}

void app_tws_send_fastpair_info_to_slave(void) {
  TRACE(0, "Send fastpair info to secondary device.");
  NV_FP_ACCOUNT_KEY_RECORD_T *pFpData = nv_record_get_fp_data_structure_info();
  app_ibrt_share_fastpair_info((uint8_t *)pFpData,
                               sizeof(NV_FP_ACCOUNT_KEY_RECORD_T));
}

void app_ibrt_shared_fastpair_info_received_handler(uint16_t rsp_seq,
                                                    uint8_t *p_buff,
                                                    uint16_t length) {
  NV_FP_ACCOUNT_KEY_RECORD_T *pFpData = (NV_FP_ACCOUNT_KEY_RECORD_T *)p_buff;
  nv_record_update_fp_data_structure(pFpData);
}
#endif

#ifdef IBRT
bool app_tws_is_master_mode(void) {
  return (IBRT_MASTER == app_tws_ibrt_role_get_callback(NULL)) ? true : false;
}

bool app_tws_is_slave_mode(void) {
  return (IBRT_SLAVE == app_tws_ibrt_role_get_callback(NULL)) ? true : false;
}

bool app_tws_is_freeman_mode(void) {
  return (IBRT_UNKNOW == app_tws_ibrt_role_get_callback(NULL)) ? true : false;
}

bool app_tws_nv_is_master_role(void) {
  TRACE(1, "[ZYH]%s -> %d", __func__, app_tws_ibrt_nv_role_get_callback(NULL));
  return (IBRT_MASTER == app_tws_ibrt_nv_role_get_callback(NULL)) ? true
                                                                  : false;
}

void app_ibrt_sync_volume_info(void) {
  if (app_tws_ibrt_tws_link_connected()) {
    ibrt_volume_info_t ibrt_volume_req;
    ibrt_volume_req.a2dp_volume = a2dp_volume_get(BT_DEVICE_ID_1);
    ibrt_volume_req.hfp_volume = hfp_volume_get(BT_DEVICE_ID_1);
    tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_VOLUME_INFO, (uint8_t *)&ibrt_volume_req,
                      sizeof(ibrt_volume_req));
  }
}

static APP_TWS_SIDE_T app_tws_side = EAR_SIDE_UNKNOWN;

void app_tws_set_side(APP_TWS_SIDE_T side) {
  ASSERT((EAR_SIDE_LEFT == side) || (EAR_SIDE_RIGHT == side),
         "Error: setting invalid side");
  TRACE(1, "Set TWS side to %d", side);
  app_tws_side = side;
}

bool app_tws_is_left_side(void) { return (app_tws_side == EAR_SIDE_LEFT); }

bool app_tws_is_right_side(void) { return (app_tws_side == EAR_SIDE_RIGHT); }

bool app_tws_is_unknown_side(void) {
  return (app_tws_side == EAR_SIDE_UNKNOWN);
}

// void app_tws_set_side_from_addr(uint8_t *addr)
// {
//     ASSERT(addr, "Error: address invalid");
//     if (addr[0] & 0x1) {
//         app_tws_set_side(EAR_SIDE_RIGHT);
//         TRACE(0, "Right earbud");
//     } else {
//         app_tws_set_side(EAR_SIDE_LEFT);
//         TRACE(0, "Left earbud");
//     }
// }

void app_tws_set_side_from_gpio(void) {
  if (tgt_tws_get_channel_is_right()) {
    app_tws_set_side(EAR_SIDE_RIGHT);
  } else {
    app_tws_set_side(EAR_SIDE_LEFT);
  }
}
#endif
