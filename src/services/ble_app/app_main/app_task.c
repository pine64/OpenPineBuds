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
/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h" // SW configuration

#if (BLE_APP_PRESENT)

#include "../l2cm/l2cm_int.h"
#include "app.h" // Application Manager Definition
#include "app_ble_include.h"
#include "app_task.h"  // Application Manager Task API
#include "arch.h"      // Platform Definitions
#include "gapc_task.h" // GAP Controller Task API
#include "gapm_task.h" // GAP Manager Task API
#include "gattc_task.h"
#include "ke_timer.h" // Kernel timer
#include <string.h>

#include "co_utils.h"
#if (BLE_APP_SEC)
#include "app_sec.h" // Security Module Definition
#endif               //(BLE_APP_SEC)

#if (BLE_APP_HT)
#include "app_ht.h" // Health Thermometer Module Definition
#include "htpt_task.h"
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h" // Device Information Module Definition
#include "diss_task.h"
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h" // Battery Module Definition
#include "bass_task.h"
#endif //(BLE_APP_BATT)

#if (BLE_APP_HID)
#include "app_hid.h" // HID Module Definition
#include "hogpd_task.h"
#endif //(BLE_APP_HID)

#if (BLE_APP_HR)
#include "app_hrps.h"
#endif

#if (BLE_APP_VOICEPATH)
#include "app_voicepath_ble.h" // Voice Path Module Definition
#endif                         // (BLE_APP_VOICEPATH)

#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h" // Data Path Server Module Definition
#include "datapathps_task.h"
#endif // (BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_AI_VOICE)
#include "app_ai_ble.h" // ama Voice Module Definition
#endif                  // (BLE_APP_AI_VOICE)

#if (BLE_APP_OTA)
#include "app_ota.h" // OTA Module Definition
#include "ota_task.h"
#endif // (BLE_APP_OTA)

#if (BLE_APP_TOTA)
#include "app_tota_ble.h" // TOTA Module Definition
#include "tota_task.h"
#endif // (BLE_APP_TOTA)

#if (BLE_APP_ANCC)
#include "ancc_task.h"
#include "app_ancc.h" // ANC Module Definition
#include "app_ancc_task.h"
#endif // (BLE_APP_ANCC)

#if (BLE_APP_AMS)
#include "amsc_task.h"
#include "app_amsc.h" // AMS Module Definition
#include "app_amsc_task.h"
#endif // (BLE_APP_AMS)
#if (BLE_APP_GFPS)
#include "app_gfps.h" // google fast pair service provider
#include "gfps_provider_task.h"
#endif // (BLE_APP_GFPS)
#ifdef BLE_APP_AM0
#include "am0_app.h" // Audio Mode 0 Application
#endif               // defined(BLE_APP_AM0)

#if (DISPLAY_SUPPORT)
#include "app_display.h" // Application Display Definition
#endif                   //(DISPLAY_SUPPORT)

#include "app_fp_rfcomm.h"
#include "ble_app_dbg.h"
#include "bt_drv_interface.h"
#include "nvrecord_ble.h"

#if (BLE_APP_TILE)
#include "tile_gatt_server.h"
#include "tile_target_ble.h"
#endif

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#define APP_CONN_PARAM_INTERVEL_MIN (20)

uint8_t ble_stack_ready = 0;

extern bool app_factorymode_get(void);

static uint8_t appm_get_handler(const struct ke_state_handler *handler_list,
                                ke_msg_id_t msgid, void *param,
                                ke_task_id_t src_id) {
  // Counter
  uint8_t counter;

  // Get the message handler function by parsing the message table
  for (counter = handler_list->msg_cnt; 0 < counter; counter--) {
    struct ke_msg_handler handler =
        (struct ke_msg_handler)(*(handler_list->msg_table + counter - 1));

    if ((handler.id == msgid) || (handler.id == KE_MSG_DEFAULT_HANDLER)) {
      // If handler is NULL, message should not have been received in this state
      ASSERT_ERR(handler.func);

      return (uint8_t)(handler.func(msgid, param, TASK_APP, src_id));
    }
  }

  // If we are here no handler has been found, drop the message
  return (KE_MSG_CONSUMED);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_adv_timeout_handler(ke_msg_id_t const msgid, void const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id) {
#if (BLE_APP_HID)
#else
  // Stop advertising
  appm_stop_advertising();
#endif

  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP. - Reset the stack
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_device_ready_ind_handler(ke_msg_id_t const msgid,
                                         void const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id) {
  if (ble_stack_ready) {
    return KE_MSG_CONSUMED;
  }

#ifdef __FACTORY_MODE_SUPPORT__
  if (app_factorymode_get()) {
    return (KE_MSG_CONSUMED);
  }
#endif

  BLE_APP_FUNC_ENTER();

  // Application has not been initialized
  ASSERT_ERR(ke_state_get(dest_id) == APPM_INIT);

  ble_stack_ready = 1;

  // Reset the stack
  struct gapm_reset_cmd *cmd =
      KE_MSG_ALLOC(GAPM_RESET_CMD, TASK_GAPM, TASK_APP, gapm_reset_cmd);

  cmd->operation = GAPM_RESET;

  ke_msg_send(cmd);

  BLE_APP_FUNC_LEAVE();

  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id) {
  BLE_APP_FUNC_ENTER();
  BLE_APP_DBG("param->operation: %d status is %d app_env.next_svc is %d",
              param->operation, param->status, app_env.next_svc);

  switch (param->operation) {
  // Reset completed
  case (GAPM_RESET): {
    if (param->status == GAP_ERR_NO_ERROR) {
#if (BLE_APP_HID)
      app_hid_start_mouse();
#endif //(BLE_APP_HID)

      // Set Device configuration
      struct gapm_set_dev_config_cmd *cmd =
          KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD, TASK_GAPM, TASK_APP,
                       gapm_set_dev_config_cmd);
      // Set the operation
      cmd->operation = GAPM_SET_DEV_CONFIG;
      // Set the device role - Peripheral
      cmd->role = GAP_ROLE_ALL;
      // Set Data length parameters
      cmd->sugg_max_tx_octets = APP_MAX_TX_OCTETS;
      cmd->sugg_max_tx_time = APP_MAX_TX_TIME;
      cmd->pairing_mode = GAPM_PAIRING_LEGACY;
#ifdef CFG_SEC_CON
      cmd->pairing_mode |= GAPM_PAIRING_SEC_CON;
#endif
      cmd->max_mtu = 512;
      cmd->att_cfg = 0;
      cmd->att_cfg |= GAPM_MASK_ATT_SVC_CHG_EN;
#if (BLE_APP_HID)
      // Enable Slave Preferred Connection Parameters present
      cmd->att_cfg |= GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN;
#endif //(BLE_APP_HID)

#ifdef BLE_APP_AM0
      cmd->addr_type = GAPM_CFG_ADDR_HOST_PRIVACY;
      cmd->audio_cfg = GAPM_MASK_AUDIO_AM0_SUP;
#endif // BLE_APP_AM0

      // load IRK
      memcpy(cmd->irk.key, app_env.loc_irk, KEY_LEN);

      // Send message
      ke_msg_send(cmd);
    } else {
      ASSERT_ERR(0);
    }
  } break;
  case (GAPM_PROFILE_TASK_ADD): {
    // ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation,
    // param->status); Add the next requested service
    if (!appm_add_svc()) {
      // Go to the ready state
      ke_state_set(TASK_APP, APPM_READY);

      // No more service to add
      app_ble_system_ready();
    }
  } break;
  // Device Configuration updated
  case (GAPM_SET_DEV_CONFIG): {
    ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation,
                param->status);

    // Go to the create db state
    ke_state_set(TASK_APP, APPM_CREATE_DB);

    // Add the first required service in the database
    // and wait for the PROFILE_ADDED_IND
    appm_add_svc();
  } break;

  case (GAPM_ADV_NON_CONN):
  case (GAPM_ADV_UNDIRECT):
#if !(BLE_APP_HID)
  case (GAPM_ADV_DIRECT):
#endif // !(BLE_APP_HID)
  case (GAPM_ADV_DIRECT_LDC): {
    LOG_I("adv evt cmp status 0x%x", param->status);
    ASSERT(GAP_ERR_ADV_DATA_INVALID != param->status,
           "The BLE adv data or scan rsp data is invalid! Better check their "
           "length.");

    if (GAP_ERR_CANCELED == param->status) {
      app_advertising_stopped();
    } else if (GAP_ERR_NO_ERROR == param->status) {
      if (ke_state_get(TASK_APP) == APPM_ADVERTISING) {
        app_advertising_started();
      } else {
        app_advertising_stopped();
      }
    } else {
      app_advertising_starting_failed();
    }

    break;
  }
  case GAPM_UPDATE_ADVERTISE_DATA: {
    app_adv_data_updated();
    break;
  }
  case GAPM_SCAN_ACTIVE:
  case GAPM_SCAN_PASSIVE: {
    LOG_I("scan evt cmp status %d", param->status);
    if (GAP_ERR_CANCELED == param->status) {
      app_scanning_stopped();
    } else if (GAP_ERR_NO_ERROR == param->status) {
      app_scanning_started();
    } else {
      app_scanning_starting_failed();
    }
  } break;
  case GAPM_CONNECTION_DIRECT:
  case GAPM_CONNECTION_AUTO:
  case GAPM_CONNECTION_SELECTIVE:
  case GAPM_CONNECTION_NAME_REQUEST:
  case GAPM_CONNECTION_GENERAL: {
    BLE_GAP_DBG("connecting cmp status %d", param->status);
    if (GAP_ERR_CANCELED == param->status) {
      app_connecting_stopped();
    } else if (GAP_ERR_NO_ERROR == param->status) {
      app_connecting_started();
    } else {
      app_connecting_failed();
    }
  } break;

#if (BLE_APP_HID)
  case (GAPM_ADV_DIRECT): {
    if (param->status == GAP_ERR_TIMEOUT) {
      ke_state_set(TASK_APP, APPM_READY);
    }
  } break;
#endif //(BLE_APP_HID)
  case GAPM_RESOLV_ADDR: {
    LOG_I("Resolve result %d", param->status);
    if (GAP_ERR_NOT_FOUND == param->status) {
      appm_random_ble_addr_solved(false, NULL);
    }
    break;
  }
  default: {
    // Drop the message
  } break;
  }

  BLE_APP_FUNC_LEAVE();

  return (KE_MSG_CONSUMED);
}

static int gapc_get_dev_info_req_ind_handler(
    ke_msg_id_t const msgid, struct gapc_get_dev_info_req_ind const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  switch (param->req) {
  case GAPC_DEV_NAME: {
    struct gapc_get_dev_info_cfm *cfm =
        KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM, src_id, dest_id,
                         gapc_get_dev_info_cfm, APP_DEVICE_NAME_MAX_LEN);
    cfm->req = param->req;
    cfm->info.name.length = appm_get_dev_name(cfm->info.name.value);

    // Send message
    ke_msg_send(cfm);
  } break;

  case GAPC_DEV_APPEARANCE: {
    // Allocate message
    struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(
        GAPC_GET_DEV_INFO_CFM, src_id, dest_id, gapc_get_dev_info_cfm);
    cfm->req = param->req;
// Set the device appearance
#if (BLE_APP_HT)
    // Generic Thermometer - TODO: Use a flag
    cfm->info.appearance = 728;
#elif (BLE_APP_HID)
    // HID Mouse
    cfm->info.appearance = 962;
#else
    // No appearance
    cfm->info.appearance = 0;
#endif

    // Send message
    ke_msg_send(cfm);
  } break;

  case GAPC_DEV_SLV_PREF_PARAMS: {
    // Allocate message
    struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(
        GAPC_GET_DEV_INFO_CFM, src_id, dest_id, gapc_get_dev_info_cfm);
    cfm->req = param->req;
    // Slave preferred Connection interval Min
    cfm->info.slv_params.con_intv_min = 8;
    // Slave preferred Connection interval Max
    cfm->info.slv_params.con_intv_max = 10;
    // Slave preferred Connection latency
    cfm->info.slv_params.slave_latency = 0;
    // Slave preferred Link supervision timeout
    cfm->info.slv_params.conn_timeout = 200; // 2s (500*10ms)

    // Send message
    ke_msg_send(cfm);
  } break;

  default: /* Do Nothing */
    break;
  }

  return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles GAPC_SET_DEV_INFO_REQ_IND message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_set_dev_info_req_ind_handler(
    ke_msg_id_t const msgid, struct gapc_set_dev_info_req_ind const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  // Set Device configuration
  struct gapc_set_dev_info_cfm *cfm = KE_MSG_ALLOC(
      GAPC_SET_DEV_INFO_CFM, src_id, dest_id, gapc_set_dev_info_cfm);
  // Reject to change parameters
  cfm->status = GAP_ERR_REJECTED;
  cfm->req = param->req;
  // Send message
  ke_msg_send(cfm);

  return (KE_MSG_CONSUMED);
}

static void POSSIBLY_UNUSED gapc_refresh_remote_dev_feature(uint8_t conidx) {
  // Send a GAPC_GET_INFO_CMD in order to read the device name characteristic
  // value
  struct gapc_get_info_cmd *p_cmd =
      KE_MSG_ALLOC(GAPC_GET_INFO_CMD, KE_BUILD_ID(TASK_GAPC, conidx), TASK_GAPM,
                   gapc_get_info_cmd);

  // request peer device name.
  p_cmd->operation = GAPC_GET_PEER_FEATURES;

  // send command
  ke_msg_send(p_cmd);
}

static void POSSIBLY_UNUSED gpac_exchange_data_packet_length(uint8_t conidx) {
#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P)
  return;
#endif

  struct gapc_set_le_pkt_size_cmd *set_le_pakt_size_req =
      KE_MSG_ALLOC(GAPC_SET_LE_PKT_SIZE_CMD, KE_BUILD_ID(TASK_GAPC, conidx),
                   TASK_APP, gapc_set_le_pkt_size_cmd);

  set_le_pakt_size_req->operation = GAPC_SET_LE_PKT_SIZE;
  set_le_pakt_size_req->tx_octets = APP_MAX_TX_OCTETS;
  set_le_pakt_size_req->tx_time = APP_MAX_TX_TIME;
  // Send message
  ke_msg_send(set_le_pakt_size_req);
}

static int gapc_peer_features_ind_handler(ke_msg_id_t const msgid,
                                          struct gapc_peer_features_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id) {
  LOG_I("Peer dev feature is:");
  DUMP8("0x%02x ", param->features, GAP_LE_FEATS_LEN);
  uint8_t conidx = KE_IDX_GET(src_id);

  if (param->features[0] & GAPC_EXT_DATA_LEN_MASK) {
    gpac_exchange_data_packet_length(conidx);
  }
  return (KE_MSG_CONSUMED);
}

void app_exchange_remote_feature(uint8_t conidx) {
  APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

  LOG_I("connectStatus:%d, isFeatureExchanged:%d, isGotSolvedBdAddr:%d",
        pContext->connectStatus, pContext->isFeatureExchanged,
        pContext->isGotSolvedBdAddr);

  if ((BLE_CONNECTED == pContext->connectStatus) &&
      !pContext->isFeatureExchanged) {
    if (pContext->isGotSolvedBdAddr) {
      gapc_refresh_remote_dev_feature(conidx);
      pContext->isFeatureExchanged = true;
    }
  }
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required
 *profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_connection_req_ind_handler(
    ke_msg_id_t const msgid, struct gapc_connection_req_ind *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  uint8_t conidx = KE_IDX_GET(src_id);

  APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

  pContext->connectStatus = BLE_CONNECTED;

  APP_BLE_NEGOTIATED_CONN_PARAM_T connParam;
  connParam.con_interval = param->con_interval;
  connParam.sup_to = param->sup_to;
  connParam.con_latency = param->con_latency;
  app_ble_save_negotiated_conn_param(conidx, &connParam);

  ke_state_set(dest_id, APPM_CONNECTED);

  app_env.conn_cnt++;

  if (app_is_resolving_ble_bd_addr()) {
    LOG_I("A ongoing ble addr solving is in progress, refuse the new "
          "connection.");
    appm_disconnect(conidx);
    return KE_MSG_CONSUMED;
  }

  LOG_I("[CONNECT]device info:");
  LOG_I("peer addr:");
  DUMP8("%02x ", param->peer_addr.addr, 6);
  LOG_I("peer addr type:%d", param->peer_addr_type);
  LOG_I("connection index:%d, isGotSolvedBdAddr:%d", conidx,
        pContext->isGotSolvedBdAddr);
  LOG_I("conn interval:%d, timeout:%d", param->con_interval, param->sup_to);

  BLE_APP_FUNC_ENTER();

  // Check if the received Connection Handle was valid
  if (conidx != GAP_INVALID_CONIDX) {
    pContext->peerAddrType = param->peer_addr_type;
    memcpy(pContext->bdAddr, param->peer_addr.addr, BD_ADDR_LEN);

    // Retrieve the connection info from the parameters
    pContext->conhdl = param->conhdl;

    if (BLE_RANDOM_ADDR == pContext->peerAddrType) {
      pContext->isGotSolvedBdAddr = false;
    } else {
      pContext->isGotSolvedBdAddr = true;
    }

    // Clear the advertising timeout timer
    if (ke_timer_active(APP_ADV_TIMEOUT_TIMER, TASK_APP)) {
      ke_timer_clear(APP_ADV_TIMEOUT_TIMER, TASK_APP);
    }

#if (BLE_APP_SEC)
    app_sec_reset_env_on_connection();
#endif

    // Send connection confirmation
    struct gapc_connection_cfm *cfm =
        KE_MSG_ALLOC(GAPC_CONNECTION_CFM, KE_BUILD_ID(TASK_GAPC, conidx),
                     TASK_APP, gapc_connection_cfm);

#if (BLE_APP_SEC)
    cfm->auth = app_sec_get_bond_status() ? BLE_AUTHENTICATION_LEVEL
                                          : GAP_AUTH_REQ_NO_MITM_NO_BOND;
#else  // !(BLE_APP_SEC)
    cfm->auth = GAP_AUTH_REQ_NO_MITM_NO_BOND;
#endif // (BLE_APP_SEC)
    // Send the message
    ke_msg_send(cfm);

    // We are now in connected State
    ke_state_set(dest_id, APPM_CONNECTED);
    app_exchange_remote_feature(conidx);

    app_ble_connected_evt_handler(conidx, param->peer_addr.addr);
  }

#if (BLE_APP_TILE)
  app_tile_connected_evt_handler(conidx, param);
#endif

  app_env.context[conidx].connInterval = param->con_interval;

  app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_DEFAULT, true);
  BLE_APP_FUNC_LEAVE();

  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id) {
  switch (param->operation) {
  case (GAPC_UPDATE_PARAMS): {
    if (param->status != GAP_ERR_NO_ERROR) {
      //                appm_disconnect();
    }
  } break;

  default: {
  } break;
  }

  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                       struct gapc_disconnect_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id) {
  LOG_I("[DISCONNECT] device info:");
  uint8_t conidx = KE_IDX_GET(src_id);
  LOG_I("connection index:%d, reason:0x%x", conidx, param->reason);

  app_env.context[conidx].isBdAddrResolvingInProgress = false;
  app_env.context[conidx].isGotSolvedBdAddr = false;

  app_env.context[conidx].connectStatus = BLE_DISCONNECTED;
  app_env.context[conidx].isFeatureExchanged = false;
  app_env.context[conidx].connInterval = 0;
  l2cm_buffer_reset(conidx);

  // Go to the ready state
  ke_state_set(TASK_APP, APPM_READY);

  app_env.conn_cnt--;

#if (BLE_VOICEPATH)
  app_voicepath_disconnected_evt_handler(conidx);
#endif

#if (BLE_DATAPATH_SERVER)
  app_datapath_server_disconnected_evt_handler(conidx);
#endif

#if (BLE_OTA)
  app_ota_disconnected_evt_handler(conidx);
#endif

#if (BLE_APP_TOTA)
  app_tota_disconnected_evt_handler(conidx);
#endif

#if (BLE_APP_GFPS)
  app_gfps_disconnected_evt_handler(conidx);
#endif

#if (BLE_AI_VOICE)
  app_ai_disconnected_evt_handler(conidx);
#endif

#if (BLE_APP_TILE)
  app_tile_disconnected_evt_handler(conidx);
#endif

  app_ble_disconnected_evt_handler(conidx);

  app_ble_reset_conn_param_mode(conidx);
  return (KE_MSG_CONSUMED);
}

static int gapm_profile_added_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_profile_added_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id) {
  LOG_I("prf_task_id %d is added.", param->prf_task_id);

  return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of all messages sent from the lower layers to the
 *application
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int appm_msg_handler(ke_msg_id_t const msgid, void *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id) {
  // Retrieve identifier of the task from received message
  ke_task_id_t src_task_id = MSG_T(msgid);
  // Message policy
  uint8_t msg_pol = KE_MSG_CONSUMED;

  switch (src_task_id) {
  case (TASK_ID_GAPC): {
#if (BLE_APP_SEC)
    if ((msgid >= GAPC_BOND_CMD) && (msgid <= GAPC_SECURITY_IND)) {
      // Call the Security Module
      msg_pol = appm_get_handler(&app_sec_table_handler, msgid, param, src_id);
    }
#endif //(BLE_APP_SEC)
       // else drop the message
  } break;

  case (TASK_ID_GATTC): {
    // Service Changed - Drop
  } break;

#if (BLE_APP_HT)
  case (TASK_ID_HTPT): {
    // Call the Health Thermometer Module
    msg_pol = appm_get_handler(&app_ht_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
  case (TASK_ID_DISS): {
    // Call the Device Information Module
    msg_pol = appm_get_handler(&app_dis_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
  case (TASK_ID_HOGPD): {
    // Call the HID Module
    msg_pol = appm_get_handler(&app_hid_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_HID)

#if (BLE_APP_BATT)
  case (TASK_ID_BASS): {
    // Call the Battery Module
    msg_pol = appm_get_handler(&app_batt_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_BATT)

#if defined(BLE_APP_AM0)
  case (TASK_ID_AM0): {
    // Call the Audio Mode 0 Module
    msg_pol = appm_get_handler(&am0_app_table_handler, msgid, param, src_id);
  } break;
  case (TASK_ID_AM0_HAS): {
    // Call the Audio Mode 0 Module
    msg_pol =
        appm_get_handler(&am0_app_has_table_handler, msgid, param, src_id);
  } break;
#endif // defined(BLE_APP_AM0)

#if (BLE_APP_HR)
  case (TASK_ID_HRPS): {
    // Call the HRPS Module
    msg_pol = appm_get_handler(&app_hrps_table_handler, msgid, param, src_id);
  } break;
#endif

#if (BLE_APP_VOICEPATH)
  case (TASK_ID_VOICEPATH): {
    // Call the Voice Path Module
    msg_pol = appm_get_handler(app_voicepath_ble_get_msg_handler_table(), msgid,
                               param, src_id);
  } break;
#endif //(BLE_APP_VOICEPATH)

#if (BLE_APP_DATAPATH_SERVER)
  case (TASK_ID_DATAPATHPS): {
    // Call the Data Path Module
    msg_pol = appm_get_handler(&app_datapath_server_table_handler, msgid, param,
                               src_id);
  } break;
#endif //(BLE_APP_DATAPATH_SERVER)
#if (BLE_APP_TILE)
  case (TASK_ID_TILE): {
    // Call the TILE Module
    msg_pol = appm_get_handler(&app_tile_table_handler, msgid, param, src_id);
  } break;
#endif

#if (BLE_APP_AI_VOICE)
  case (TASK_ID_AI): {
    // Call the AI Voice
    msg_pol = appm_get_handler(app_ai_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_AI_VOICE)

#if (BLE_APP_OTA)
  case (TASK_ID_OTA): {
    // Call the OTA
    msg_pol = appm_get_handler(&app_ota_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_OTA)

#if (BLE_APP_TOTA)
  case (TASK_ID_TOTA): {
    // Call the TOTA
    msg_pol = appm_get_handler(&app_tota_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_TOTA)

#if (BLE_APP_ANCC)
  case (TASK_ID_ANCC): {
    // Call the ANCC
    msg_pol = appm_get_handler(&app_ancc_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_ANCC)

#if (BLE_APP_AMS)
  case (TASK_ID_AMSC): {
    // Call the AMS
    msg_pol = appm_get_handler(&app_amsc_table_handler, msgid, param, src_id);
  } break;
#endif //(BLE_APP_AMS)
#if (BLE_APP_GFPS)
  case (TASK_ID_GFPSP): {
    msg_pol = appm_get_handler(&app_gfps_table_handler, msgid, param, src_id);
  } break;
#endif

  default: {
#if (BLE_APP_HT)
    if (msgid == APP_HT_MEAS_INTV_TIMER) {
      msg_pol = appm_get_handler(&app_ht_table_handler, msgid, param, src_id);
    }
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
    if (msgid == APP_HID_MOUSE_TIMEOUT_TIMER) {
      msg_pol = appm_get_handler(&app_hid_table_handler, msgid, param, src_id);
    }
#endif //(BLE_APP_HID)
  } break;
  }

  return (msg_pol);
}

static int gapm_adv_report_ind_handler(ke_msg_id_t const msgid,
                                       struct gapm_adv_report_ind *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id) {
  app_adv_reported_scanned(param);
  return KE_MSG_CONSUMED;
}

static int gapm_addr_solved_ind_handler(ke_msg_id_t const msgid,
                                        struct gapm_addr_solved_ind *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id) {
  /// Indicate that resolvable random address has been solved
  appm_random_ble_addr_solved(true, param->irk.key);
  return KE_MSG_CONSUMED;
}

__STATIC int gattc_mtu_changed_ind_handler(
    ke_msg_id_t const msgid, struct gattc_mtu_changed_ind const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  uint8_t conidx = KE_IDX_GET(src_id);

  LOG_I("MTU has been negotiated as %d conidx %d", param->mtu, conidx);

#if (BLE_APP_DATAPATH_SERVER)
  app_datapath_server_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_VOICEPATH)
  app_voicepath_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_OTA)
  app_ota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_APP_TOTA)
  app_tota_mtu_exchanged_handler(conidx, param->mtu);
#endif

#if (BLE_AI_VOICE)
  app_ai_mtu_exchanged_handler(conidx, param->mtu);
#endif

  return (KE_MSG_CONSUMED);
}

#define APP_CONN_PARAM_INTERVEL_MAX (30)
__STATIC int gapc_conn_param_update_req_ind_handler(
    ke_msg_id_t const msgid, struct gapc_param_update_req_ind const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  bool accept = true;
  ble_evnet_t event;

  LOG_I("Receive the conn param update request: min %d max %d latency %d "
        "timeout %d",
        param->intv_min, param->intv_max, param->latency, param->time_out);

  struct gapc_param_update_cfm *cfm = KE_MSG_ALLOC(
      GAPC_PARAM_UPDATE_CFM, src_id, dest_id, gapc_param_update_cfm);

  event.evt_type = BLE_CONN_PARAM_UPDATE_REQ_EVENT;
  event.p.conn_param_update_req_handled.intv_min = param->intv_min;
  event.p.conn_param_update_req_handled.intv_max = param->intv_max;
  event.p.conn_param_update_req_handled.latency = param->latency;
  event.p.conn_param_update_req_handled.time_out = param->time_out;

  app_ble_core_global_handle(&event, &accept);
  LOG_I("%s ret %d ", __func__, accept);

  cfm->accept = accept;

#ifdef GFPS_ENABLED
  // if fastpair doesn't have the requirement of finishing
  // pairing in a really short period, just comment out this
  // code block to avoid audio glitch if this event happens during music
  // playback and interval is smaller than 15ms
  if (param->intv_min < (uint16_t)(15 / 1.25)) {
    LOG_I("accept");
    cfm->accept = true;
    fp_update_ble_connect_param_start(KE_IDX_GET(src_id));
  } else {
    fp_update_ble_connect_param_stop(KE_IDX_GET(src_id));
  }
#endif

  ke_msg_send(cfm);

  return (KE_MSG_CONSUMED);
}

static int gapc_conn_param_updated_handler(ke_msg_id_t const msgid,
                                           struct gapc_param_updated_ind *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id) {
  uint8_t conidx = KE_IDX_GET(src_id);
  LOG_I("Conidx %d conn parameter is updated as interval %d timeout %d", conidx,
        param->con_interval, param->sup_to);

  APP_BLE_NEGOTIATED_CONN_PARAM_T connParam;
  connParam.con_interval = param->con_interval;
  connParam.sup_to = param->sup_to;
  connParam.con_latency = param->con_latency;
  app_ble_save_negotiated_conn_param(conidx, &connParam);

#if (BLE_VOICEPATH)
  app_voicepath_ble_conn_parameter_updated(conidx, param->con_interval,
                                           param->con_latency);
#endif
#if BLE_APP_TILE
  app_tile_ble_conn_parameter_updated(conidx, param);
#endif

  app_env.context[conidx].connInterval = param->con_interval;

  if (param->con_interval >= 32) {
    if (app_ble_is_parameter_mode_enabled(conidx, BLE_CONN_PARAM_MODE_OTA)) {
      app_ble_parameter_mode_clear(conidx, BLE_CONN_PARAM_MODE_OTA);
      app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_OTA_SLOWER, true);
    } else if (app_ble_is_parameter_mode_enabled(
                   conidx, BLE_CONN_PARAM_MODE_AI_STREAM_ON)) {
      app_ble_parameter_mode_clear(conidx, BLE_CONN_PARAM_MODE_AI_STREAM_ON);
      app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_AI_STREAM_ON, true);
    }
  }

  return (KE_MSG_CONSUMED);
}

static int gapm_dev_addr_ind_handler(ke_msg_id_t const msgid,
                                     struct gapm_dev_bdaddr_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id) {
  ble_evnet_t event;

  // Indicate that a new random BD address set in lower layers
  LOG_I("New dev addr:");
  DUMP8("%02x ", param->addr.addr.addr, 6);

#ifdef GFPS_ENABLED
  app_fp_msg_send_updated_ble_addr();
  app_gfps_update_random_salt();
#endif

  event.evt_type = BLE_SET_RANDOM_BD_ADDR_EVENT;
  event.p.set_random_bd_addr_handled.new_bdaddr = param->addr.addr.addr;
  app_ble_core_global_handle(&event, NULL);

  return KE_MSG_CONSUMED;
}

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */
/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(appm){
    // Note: first message is latest message checked by kernel so default is put
    // on top.
    {KE_MSG_DEFAULT_HANDLER, (ke_msg_func_t)appm_msg_handler},

    {APP_ADV_TIMEOUT_TIMER, (ke_msg_func_t)app_adv_timeout_handler},

    {GAPM_DEVICE_READY_IND, (ke_msg_func_t)gapm_device_ready_ind_handler},
    {GAPM_CMP_EVT, (ke_msg_func_t)gapm_cmp_evt_handler},
    {GAPC_GET_DEV_INFO_REQ_IND,
     (ke_msg_func_t)gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND,
     (ke_msg_func_t)gapc_set_dev_info_req_ind_handler},
    {GAPC_CONNECTION_REQ_IND, (ke_msg_func_t)gapc_connection_req_ind_handler},
    {GAPC_CMP_EVT, (ke_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_DISCONNECT_IND, (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GAPM_PROFILE_ADDED_IND, (ke_msg_func_t)gapm_profile_added_ind_handler},
    {GATTC_MTU_CHANGED_IND, (ke_msg_func_t)gattc_mtu_changed_ind_handler},
    {GAPC_PARAM_UPDATE_REQ_IND,
     (ke_msg_func_t)gapc_conn_param_update_req_ind_handler},
    {GAPC_PARAM_UPDATED_IND, (ke_msg_func_t)gapc_conn_param_updated_handler},
    {GAPM_ADV_REPORT_IND, (ke_msg_func_t)gapm_adv_report_ind_handler},
    {GAPC_PEER_FEATURES_IND, (ke_msg_func_t)gapc_peer_features_ind_handler},
    {GAPM_ADDR_SOLVED_IND, (ke_msg_func_t)gapm_addr_solved_ind_handler},
    {GAPM_DEV_BDADDR_IND, (ke_msg_func_t)gapm_dev_addr_ind_handler},
};

/* Defines the place holder for the states of all the task instances. */
ke_state_t appm_state[APP_IDX_MAX];

const struct ke_task_desc TASK_DESC_APP = {appm_msg_handler_tab, appm_state,
                                           APP_IDX_MAX,
                                           ARRAY_LEN(appm_msg_handler_tab)};

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
