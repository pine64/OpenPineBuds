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
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h" // SW configuration

#if (BLE_APP_GFPS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app.h" // Application Manager Definitions
#include "app_ble_mode_switch.h"
#include "app_bt.h"
#include "app_gfps.h" // Device Information Service Application Definitions
#include "apps.h"
#include "bt_if.h"
#include "cmsis_os.h"
#include "gap.h"
#include "gapm.h"
#include "gapm_task.h" // GAP Manager Task API
#include "gfps_crypto.h"
#include "gfps_provider.h"
#include "gfps_provider_errors.h"
#include "gfps_provider_task.h" // Device Information Profile Functions
#include "me_api.h"
#include "nvrecord.h"
#include "nvrecord_fp_account_key.h"
#include "prf_types.h" // Profile Common Types Definitions
#include <string.h>

#ifdef IBRT
#include "app_tws_ibrt.h"
#include "app_tws_if.h"
#endif

/************************private macro defination***************************/
#define USE_BLE_ADDR_AS_SALT 0
#define USE_RANDOM_NUM_AS_SALT 1
#define GFPS_ACCOUNTKEY_SALT_TYPE USE_BLE_ADDR_AS_SALT

#define FP_SERVICE_LEN 0x06
#define FP_SERVICE_UUID 0x2CFE
#define FP_DEVICE_MODEL_ID 0x2B677D

#define GFPS_INITIAL_ADV_RAND_SALT 0xFF

#define BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL (160)
#define BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL (48)

/************************private type defination****************************/

/************************extern function declearation***********************/
extern void AES128_ECB_decrypt(uint8_t *input, const uint8_t *key,
                               uint8_t *output);

/**********************private function declearation************************/
/*---------------------------------------------------------------------------
 *            gfps_ble_data_fill_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    BLE advertisement and scan response data fill handler for Google fast pair
 *
 * Parameters:
 *    param - pointer of BLE parameter to be configure
 *
 * Return:
 *    void
 */
static void gfps_ble_data_fill_handler(void *param);

/************************private variable defination************************/
struct app_gfps_env_tag app_gfps_env;

static char app_gfps_power_uuid[APP_GFPS_ADV_POWER_UUID_LEN] =
    APP_GFPS_ADV_POWER_UUID;

/****************************function defination****************************/
static void app_gfps_init_env(void) {
  memset((uint8_t *)&app_gfps_env, 0, sizeof(struct app_gfps_env_tag));
  app_gfps_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
  app_gfps_env.batteryDataType = HIDE_UI_INDICATION;
  app_gfps_env.advRandSalt = GFPS_INITIAL_ADV_RAND_SALT;
  app_ble_register_data_fill_handle(
      USER_GFPS, (BLE_DATA_FILL_FUNC_T)gfps_ble_data_fill_handler, false);
}

static void big_little_switch(const uint8_t *in, uint8_t *out, uint8_t len) {
  if (len < 1)
    return;
  for (int i = 0; i < len; i++) {
    out[i] = in[len - i - 1];
  }
  return;
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
extern uint8_t bt_addr[6];

void app_gfps_connected_evt_handler(uint8_t conidx) {
  app_gfps_env.connectionIndex = conidx;
  TRACE(0, "local LE addr: ");

  bd_addr_t *pBdAddr = gapm_get_connected_bdaddr(conidx);
  big_little_switch(pBdAddr->addr, &app_gfps_env.local_le_addr.addr[0], 6);
  DUMP8("0x%02x ", pBdAddr->addr, 6);
#if !defined(IBRT)
  big_little_switch((&bt_addr[0]), &app_gfps_env.local_bt_addr.addr[0], 6);
  TRACE(0, "local bt addr: ");
  DUMP8("0x%02x ", &bt_addr[0], 6);
#else
  big_little_switch(app_tws_ibrt_get_bt_ctrl_ctx()->local_addr.address,
                    &app_gfps_env.local_bt_addr.addr[0], 6);
  TRACE(0, "local bt addr: ");
  DUMP8("0x%02x ", app_tws_ibrt_get_bt_ctrl_ctx()->local_addr.address, 6);
#endif
}

void app_gfps_disconnected_evt_handler(uint8_t conidx) {
  if (conidx == app_gfps_env.connectionIndex) {
    // recover classic bt iocap
    if (app_gfps_env.bt_set_iocap != NULL) {
      app_gfps_env.bt_set_iocap(app_gfps_env.bt_iocap);
    }
    if (app_gfps_env.bt_set_authrequirements != NULL) {
      app_gfps_env.bt_set_authrequirements(app_gfps_env.bt_authrequirements);
    }

    app_gfps_env.isKeyBasedPairingNotificationEnabled = false;
    app_gfps_env.isPassKeyNotificationEnabled = false;
    app_gfps_env.isPendingForWritingNameReq = false;
    app_gfps_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
  }
}

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static int gfpsp_value_req_ind_handler(ke_msg_id_t const msgid,
                                       struct gfpsp_value_req_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id) {
  // Initialize length
  uint8_t len = 0;
  // Pointer to the data
  uint8_t *data = NULL;
  TRACE(1, "val %d", param->value);
  // Check requested value
  switch (param->value) {
  case GFPSP_MANUFACTURER_NAME_CHAR:
  case GFPSP_MODEL_NB_STR_CHAR:
  case GFPSP_SYSTEM_ID_CHAR:
  case GFPSP_PNP_ID_CHAR:
  case GFPSP_SERIAL_NB_STR_CHAR:
  case GFPSP_HARD_REV_STR_CHAR:
  case GFPSP_FIRM_REV_STR_CHAR:
  case GFPSP_SW_REV_STR_CHAR:
  case GFPSP_IEEE_CHAR: {
    // Set information
    len = APP_GFPS_IEEE_LEN;
    data = (uint8_t *)APP_GFPS_IEEE;
  } break;

  default:
    ASSERT_ERR(0);
    break;
  }

  // Allocate confirmation to send the value
  struct gfpsp_value_cfm *cfm_value =
      KE_MSG_ALLOC_DYN(GFPSP_VALUE_CFM, src_id, dest_id, gfpsp_value_cfm, len);

  // Set parameters
  cfm_value->value = param->value;
  cfm_value->length = len;
  if (len) {
    // Copy data
    memcpy(&cfm_value->data[0], data, len);
  }
  // Send message
  ke_msg_send(cfm_value);

  return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
// MSB->LSB
const uint8_t bes_demo_Public_anti_spoofing_key[64] = {
    0x3E, 0x08, 0x3B, 0x0A, 0x5C, 0x04, 0x78, 0x84, 0xBE, 0x41, 0xBE,
    0x7E, 0x52, 0xD1, 0x0C, 0x68, 0x64, 0x6C, 0x4D, 0xB6, 0xD9, 0x20,
    0x95, 0xA7, 0x32, 0xE9, 0x42, 0x40, 0xAC, 0x02, 0x54, 0x48, 0x99,
    0x49, 0xDA, 0xE1, 0x0D, 0x9C, 0xF5, 0xEB, 0x29, 0x35, 0x7F, 0xB1,
    0x70, 0x55, 0xCB, 0x8C, 0x8F, 0xBF, 0xEB, 0x17, 0x15, 0x3F, 0xA0,
    0xAA, 0xA5, 0xA2, 0xC4, 0x3C, 0x1B, 0x48, 0x60, 0xDA};

// MSB->LSB
const uint8_t bes_demo_private_anti_spoofing_key[32] = {
    0xCD, 0xF8, 0xAA, 0xC0, 0xDF, 0x4C, 0x93, 0x63, 0x2F, 0x48, 0x20,
    0xA6, 0xD8, 0xAB, 0x22, 0xF3, 0x3A, 0x94, 0xBF, 0x8E, 0x4C, 0x90,
    0x25, 0xB3, 0x44, 0xD2, 0x2E, 0xDE, 0x0F, 0xB7, 0x22, 0x1F};

extern void fast_pair_enter_pairing_mode_handler(void);

void app_gfps_init(void) {
  nv_record_fp_account_key_init();
  app_gfps_init_env();
  app_gfps_env.enter_pairing_mode = NULL;
  app_gfps_env.bt_set_iocap = NULL;
  app_gfps_env.bt_set_authrequirements = NULL;
  gfps_crypto_init();

  app_bt_get_fast_pair_info();
#ifndef IS_USE_CUSTOM_FP_INFO
  gfps_crypto_set_p256_key(bes_demo_Public_anti_spoofing_key,
                           bes_demo_private_anti_spoofing_key);
#else
  gfps_crypto_set_p256_key(app_bt_get_fast_pair_public_key(),
                           app_bt_get_fast_pair_private_key());
#endif
  app_gfps_set_bt_access_mode(fast_pair_enter_pairing_mode_handler);
  app_gfps_set_io_cap((gfps_bt_io_cap_set)btif_sec_set_io_capabilities);

  app_gfps_set_authrequirements(
      (gfps_bt_io_authrequirements_set)btif_sec_set_authrequirements);

  app_gfps_enable_battery_info(true);
}

void app_gfps_set_bt_access_mode(gfps_enter_pairing_mode cb) {
  app_gfps_env.enter_pairing_mode = cb;
}

void app_gfps_set_io_cap(gfps_bt_io_cap_set cb) {
  app_gfps_env.bt_set_iocap = cb;
}

void app_gfps_set_authrequirements(gfps_bt_io_authrequirements_set cb) {
  app_gfps_env.bt_set_authrequirements = cb;
}

void app_gfps_set_battery_info_acquire_handler(
    gfps_get_battery_info_handler cb) {
  app_gfps_env.get_battery_info_handler = cb;
}

void app_gfps_add_gfps(void) {
  struct gfpsp_db_cfg *db_cfg;
  // Allocate the DISS_CREATE_DB_REQ
  struct gapm_profile_task_add_cmd *req =
      KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD, TASK_GAPM, TASK_APP,
                       gapm_profile_task_add_cmd, sizeof(struct gfpsp_db_cfg));
  // Fill message
  req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
  req->sec_lvl = PERM(SVC_AUTH, ENABLE) | PERM(SVC_MI, ENABLE);
#else
  req->sec_lvl = PERM(SVC_AUTH, ENABLE);
#endif

  req->prf_task_id = TASK_ID_GFPSP;
  req->app_task = TASK_APP;
  req->start_hdl = 0;

  // Set parameters
  db_cfg = (struct gfpsp_db_cfg *)req->param;
  db_cfg->features = APP_GFPS_FEATURES;

  // Send the message
  ke_msg_send(req);
}

void app_gfps_send_keybase_pairing_via_notification(uint8_t *ptrData,
                                                    uint32_t length) {
  struct gfpsp_send_data_req_t *req =
      KE_MSG_ALLOC_DYN(GFPSP_KEY_BASED_PAIRING_WRITE_NOTIFY,
                       KE_BUILD_ID(prf_get_task_from_id(TASK_ID_GFPSP),
                                   app_gfps_env.connectionIndex),
                       TASK_APP, gfpsp_send_data_req_t, length);

  req->connecionIndex = app_gfps_env.connectionIndex;
  req->length = length;
  memcpy(req->value, ptrData, length);

  ke_msg_send(req);
}

int app_gfps_send_passkey_via_notification(uint8_t *ptrData, uint32_t length) {
  struct gfpsp_send_data_req_t *req =
      KE_MSG_ALLOC_DYN(GFPSP_KEY_PASS_KEY_WRITE_NOTIFY,
                       KE_BUILD_ID(prf_get_task_from_id(TASK_ID_GFPSP),
                                   app_gfps_env.connectionIndex),
                       TASK_APP, gfpsp_send_data_req_t, length);

  req->connecionIndex = app_gfps_env.connectionIndex;
  req->length = length;
  memcpy(req->value, ptrData, length);

  ke_msg_send(req);

  return (KE_MSG_CONSUMED);
}

static void app_gfps_send_naming_packet_via_notification(uint8_t *ptrData,
                                                         uint32_t length) {
  struct gfpsp_send_data_req_t *req =
      KE_MSG_ALLOC_DYN(GFPSP_NAME_NOTIFY,
                       KE_BUILD_ID(prf_get_task_from_id(TASK_ID_GFPSP),
                                   app_gfps_env.connectionIndex),
                       TASK_APP, gfpsp_send_data_req_t, length);

  req->connecionIndex = app_gfps_env.connectionIndex;
  req->length = length;
  memcpy(req->value, ptrData, length);

  ke_msg_send(req);
}

void app_gfps_handling_on_mobile_link_disconnection(
    btif_remote_device_t *pRemDev) {
  bool isDisconnectedWithMobile = false;
#ifdef IBRT

  ibrt_link_type_e link_type = app_tws_ibrt_get_remote_link_type(pRemDev);
  if (MOBILE_LINK == link_type) {
    isDisconnectedWithMobile = true;
  }
#else
  isDisconnectedWithMobile = true;
#endif

  if (isDisconnectedWithMobile) {
    if (app_gfps_is_last_response_pending()) {
      app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
    }
  }
}

void app_gfps_enter_connectable_mode_req_handler(uint8_t *response) {
  TRACE(2, "%s isLastResponsePending:%d", __func__,
        app_gfps_env.isLastResponsePending);
  TRACE(0, "response data:");
  DUMP8("%02x ", response, GFPSP_ENCRYPTED_RSP_LEN);

#ifdef __BT_ONE_BRING_TWO__
  app_gfps_send_keybase_pairing_via_notification(response,
                                                 GFPSP_ENCRYPTED_RSP_LEN);
#else

#ifndef IBRT
  if (btif_me_get_activeCons() > 0)
#else
  if (app_tws_ibrt_mobile_link_connected())
#endif
  {
    memcpy(app_gfps_env.pendingLastResponse, response, GFPSP_ENCRYPTED_RSP_LEN);
    app_gfps_env.isLastResponsePending = true;
#ifndef IBRT
    app_disconnect_all_bt_connections();
#else
    app_tws_ibrt_disconnect_mobile();
#endif
  } else {
    app_gfps_env.isLastResponsePending = false;
    app_gfps_send_keybase_pairing_via_notification(response,
                                                   GFPSP_ENCRYPTED_RSP_LEN);
    TRACE(0, "wait for pair req maybe classic or ble");
    app_gfps_env.isWaitingForFpToConnect = true;
    if (app_gfps_env.enter_pairing_mode != NULL) {
      app_gfps_env.enter_pairing_mode();
    }

    if (app_gfps_env.bt_set_iocap != NULL) {
      TRACE(0, "SET IOC");
      app_gfps_env.bt_iocap = app_gfps_env.bt_set_iocap(1); // IO_display_yesno
    }

    if (app_gfps_env.bt_set_authrequirements != NULL) {
      TRACE(0, "SET authrequirements");
      app_gfps_env.bt_authrequirements = app_gfps_env.bt_set_authrequirements(
          1); // Man in the Middle protection req
    }
  }
#endif
}

uint8_t *app_gfps_get_last_response(void) {
  return app_gfps_env.pendingLastResponse;
}

bool app_gfps_is_last_response_pending(void) {
  return app_gfps_env.isLastResponsePending;
}

static uint8_t
app_gfps_handle_decrypted_keybase_pairing_request(gfpsp_req_resp *raw_req,
                                                  uint8_t *out_key) {
  gfpsp_encrypted_resp en_rsp;
  gfpsp_raw_resp raw_rsp;
  memcpy(app_gfps_env.keybase_pair_key, out_key, 16);
  memcpy(&app_gfps_env.seeker_bt_addr.addr[0],
         &raw_req->rx_tx.key_based_pairing_req.seeker_addr[0], 6);
  if (raw_req->rx_tx.key_based_pairing_req.flags_discoverability ==
      RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_EN) {
    TRACE(0, "TODO discoverable 10S");
    // TODO start a timer keep discoverable 10S...
    // TODO make sure there is no ble ADV with the MODEL ID data
  }
  raw_rsp.message_type = KEY_BASED_PAIRING_RSP; // Key-based Pairing Response
  memcpy(raw_rsp.provider_addr, app_gfps_env.local_bt_addr.addr, 6);

  TRACE(0, "raw_rsp.provider_addr:");
  DUMP8("%02x ", raw_rsp.provider_addr, 6);

  for (uint8_t index = 0; index < 9; index++) {
    raw_rsp.salt[index] = (uint8_t)rand();
  }

  gfps_crypto_encrypt((const uint8_t *)(&raw_rsp.message_type), sizeof(raw_rsp),
                      app_gfps_env.keybase_pair_key, en_rsp.uint128_array);

  TRACE(1, "message type is 0x%x", raw_req->rx_tx.raw_req.message_type);
  TRACE(4, "bit 0: %d, bit 1: %d, bit 2: %d, bit 3: %d",
        raw_req->rx_tx.key_based_pairing_req.flags_discoverability,
        raw_req->rx_tx.key_based_pairing_req.flags_bonding_addr,
        raw_req->rx_tx.key_based_pairing_req.flags_get_existing_name,
        raw_req->rx_tx.key_based_pairing_req
            .flags_retroactively_write_account_key);

  bool isReturnName =
      raw_req->rx_tx.key_based_pairing_req.flags_get_existing_name;

  if (raw_req->rx_tx.key_based_pairing_req.flags_bonding_addr ==
      RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_EN) {
    TRACE(0, "try connect to remote BR/EDR addr");
    // TODO:
    app_gfps_send_keybase_pairing_via_notification(
        (uint8_t *)en_rsp.uint128_array, sizeof(en_rsp));
  } else if (raw_req->rx_tx.key_based_pairing_req
                 .flags_retroactively_write_account_key) {
    // check whether the seeker's bd address is the same as already connected
    // mobile
    uint8_t swapedBtAddr[6];
    big_little_switch(app_gfps_env.seeker_bt_addr.addr, swapedBtAddr,
                      sizeof(swapedBtAddr));

    uint8_t isMatchMobileAddr = false;
    for (uint32_t devId = 0; devId < btif_me_get_activeCons(); devId++) {
      uint8_t connectedAddr[6];
      app_bt_get_device_bdaddr(devId, connectedAddr);
      if (!memcmp(connectedAddr, swapedBtAddr, 6)) {
        isMatchMobileAddr = true;
        break;
      }
    }

    if (isMatchMobileAddr) {
      app_gfps_send_keybase_pairing_via_notification(
          (uint8_t *)en_rsp.uint128_array, sizeof(en_rsp));
    } else {
      // reject the write request
      return ATT_ERR_WRITE_NOT_PERMITTED;
    }
  } else if (raw_req->rx_tx.key_based_pairing_req.flags_bonding_addr ==
             RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_DIS) {
    app_gfps_enter_connectable_mode_req_handler(
        (uint8_t *)en_rsp.uint128_array);
  } else {
    app_gfps_send_keybase_pairing_via_notification(
        (uint8_t *)en_rsp.uint128_array, sizeof(en_rsp));
  }

  if (isReturnName) {
    app_gfps_env.isPendingForWritingNameReq = true;
    TRACE(0, "get existing name.");
    uint8_t response[16 + FP_MAX_NAME_LEN];
    uint8_t *ptrRawName;
    uint32_t rawNameLen;
    ptrRawName = nv_record_fp_get_name_ptr(&rawNameLen);

    gfps_encrypt_name(app_gfps_env.keybase_pair_key, ptrRawName, rawNameLen,
                      &response[16], response, &response[8]);

    app_gfps_send_naming_packet_via_notification(response, 16 + rawNameLen);
  } else {
    TRACE(0, "Unusable bit.");
  }

  return GAP_ERR_NO_ERROR;
}

static void app_gfps_update_local_bt_name(void) {
  uint8_t *ptrRawName;
  uint32_t rawNameLen;
  // name has been updated to fp nv record
  ptrRawName = nv_record_fp_get_name_ptr(&rawNameLen);
  if (rawNameLen > 0) {
    bt_set_local_dev_name((const unsigned char *)(ptrRawName),
                          strlen((char *)(ptrRawName)) + 1);
    btif_update_bt_name((const unsigned char *)(ptrRawName),
                        strlen((char *)(ptrRawName)) + 1);
  }
}

static bool app_gfps_decrypt_keybase_pairing_request(uint8_t *pairing_req,
                                                     uint8_t *output) {
  uint8_t keyCount = nv_record_fp_account_key_count();
  if (0 == keyCount) {
    return false;
  }

  gfpsp_req_resp raw_req;
  uint8_t accountKey[FP_ACCOUNT_KEY_SIZE];
  for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++) {
    nv_record_fp_account_key_get_by_index(keyIndex, accountKey);

    AES128_ECB_decrypt(pairing_req, (const uint8_t *)accountKey,
                       (uint8_t *)&raw_req);
    TRACE(0, "Decrypted keybase pairing req result:");
    DUMP8("0x%02x ", (uint8_t *)&raw_req, 16);

    if ((memcmp(raw_req.rx_tx.key_based_pairing_req.provider_addr,
                app_gfps_env.local_bt_addr.addr, 6) == 0) ||
        (memcmp(raw_req.rx_tx.key_based_pairing_req.provider_addr,
                app_gfps_env.local_le_addr.addr, 6) == 0)) {
      memcpy(output, accountKey, FP_ACCOUNT_KEY_SIZE);
      TRACE(1, "fp message type 0x%02x.", raw_req.rx_tx.raw_req.message_type);
      if (KEY_BASED_PAIRING_REQ == raw_req.rx_tx.raw_req.message_type) {
        app_gfps_handle_decrypted_keybase_pairing_request(&raw_req, accountKey);
        return true;
      } else if (ACTION_REQUEST == raw_req.rx_tx.raw_req.message_type) {
        memcpy(app_gfps_env.keybase_pair_key, accountKey, 16);
        memcpy(&app_gfps_env.seeker_bt_addr.addr[0],
               &(raw_req.rx_tx.key_based_pairing_req.seeker_addr[0]), 6);
        gfpsp_encrypted_resp en_rsp;
        gfpsp_raw_resp raw_rsp;

        raw_rsp.message_type =
            KEY_BASED_PAIRING_RSP; // Key-based Pairing Response
        memcpy(raw_rsp.provider_addr, app_gfps_env.local_bt_addr.addr, 6);

        TRACE(0, "raw_rsp.provider_addr:");
        DUMP8("%02x ", raw_rsp.provider_addr, 6);

        for (uint8_t index = 0; index < 9; index++) {
          raw_rsp.salt[index] = (uint8_t)rand();
        }

        gfps_crypto_encrypt((const uint8_t *)(&raw_rsp.message_type),
                            sizeof(raw_rsp), app_gfps_env.keybase_pair_key,
                            en_rsp.uint128_array);

        app_gfps_send_keybase_pairing_via_notification(
            (uint8_t *)en_rsp.uint128_array, sizeof(en_rsp));

        if (raw_req.rx_tx.action_req.isDeviceAction) {
          // TODO: device action via BLE
        } else if (raw_req.rx_tx.action_req.isFollowedByAdditionalDataCh) {
          // write name request will be received
          TRACE(0, "FP write name request will be received.");
          app_gfps_env.isPendingForWritingNameReq = true;
        }
        return true;
      }
    }
  }

  return false;
}

int app_gfps_write_key_based_pairing_ind_hander(
    ke_msg_id_t const msgid, struct gfpsp_write_ind_t const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {

  gfpsp_Key_based_Pairing_req en_req;
  gfpsp_req_resp *ptr_raw_req;

  en_req.en_req = (gfpsp_encrypted_req_uint128 *)&(param->data[0]);
  en_req.pub_key = (gfpsp_64B_public_key *)&(param->data[16]);
  uint8_t out_key[16] = {0};
  uint8_t decryptdata[16] = {0};
  uint8_t write_rsp_status = GAP_ERR_NO_ERROR;

  TRACE(3, "length = %d value = 0x%x  0x%x", param->length, param->data[0],
        param->data[1]);
  DUMP8("%02x ", param->data, 80);

  if (param->length == GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITH_PUBLIC_KEY) {
    memset(app_gfps_env.keybase_pair_key, 0, 6);
    uint32_t gfps_state = gfps_crypto_get_secret_decrypt(
        (const uint8_t *)en_req.en_req, (const uint8_t *)en_req.pub_key,
        out_key, decryptdata);
    if (gfps_state == GFPS_SUCCESS) {
      memcpy(app_gfps_env.aesKeyFromECDH, out_key, 16);
      app_gfps_env.isInitialPairing = true;
      ptr_raw_req = (gfpsp_req_resp *)decryptdata;
      TRACE(0, "raw req provider's addr:");
      DUMP8("%02x ", ptr_raw_req->rx_tx.key_based_pairing_req.provider_addr, 6);
      TRACE(0, "raw req seeker's addr:");
      DUMP8("%02x ", ptr_raw_req->rx_tx.key_based_pairing_req.seeker_addr, 6);
      TRACE(1, "fp message type 0x%02x.",
            ptr_raw_req->rx_tx.raw_req.message_type);
      if ((KEY_BASED_PAIRING_REQ == ptr_raw_req->rx_tx.raw_req.message_type) &&
          ((memcmp(ptr_raw_req->rx_tx.key_based_pairing_req.provider_addr,
                   app_gfps_env.local_bt_addr.addr, 6) == 0) ||
           (memcmp(ptr_raw_req->rx_tx.key_based_pairing_req.provider_addr,
                   app_gfps_env.local_le_addr.addr, 6) == 0))) {
        write_rsp_status = app_gfps_handle_decrypted_keybase_pairing_request(
            ptr_raw_req, out_key);
      } else {
        TRACE(0, "decrypt false..ingore");
      }

    } else {
      TRACE(1, "error = %x", gfps_state);
    }

  } else if (param->length ==
             GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITHOUT_PUBLIC_KEY) {

    app_gfps_env.isInitialPairing = false;
    bool isDecryptedSuccessful = app_gfps_decrypt_keybase_pairing_request(
        (uint8_t *)en_req.en_req, out_key);
    TRACE(1, "Decrypt keybase pairing req without public key result: %d",
          isDecryptedSuccessful);
  } else {
    TRACE(0, "who you are??");
  }

  struct gfpsp_send_write_rsp_t *response = KE_MSG_ALLOC(
      GFPSP_SEND_WRITE_RESPONSE, src_id, dest_id, gfpsp_send_write_rsp_t);
  *response = param->pendingWriteRsp;
  response->status = write_rsp_status;
  ke_msg_send(response);

  return (KE_MSG_CONSUMED);
}

int app_gfps_write_passkey_ind_hander(ke_msg_id_t const msgid,
                                      struct gfpsp_write_ind_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id) {
  gfpsp_raw_pass_key_resp raw_rsp;
  gfpsp_encrypted_resp en_rsp;
  uint8_t decryptdata[16] = {0};
  TRACE(1, "length = %d value = 0x", param->length);
  DUMP8("%02X, ", param->data, 16);
  gfps_crypto_decrypt(param->data, 16, app_gfps_env.keybase_pair_key,
                      decryptdata);
  TRACE(0, "decrypt data =0x");
  TRACE(0, "===============================");
  DUMP8("%02X", decryptdata, 16);
  TRACE(0, "===============================");

  TRACE(0, "pass key = 1-3 bytes");

  raw_rsp.message_type = 0x03; // Provider's passkey
  raw_rsp.passkey[0] = decryptdata[1];
  raw_rsp.passkey[1] = decryptdata[2];
  raw_rsp.passkey[2] = decryptdata[3];
  raw_rsp.reserved[0] = 0x38; // my magic num  temp test
  raw_rsp.reserved[1] = 0x30;
  raw_rsp.reserved[2] = 0x23;
  raw_rsp.reserved[3] = 0x30;
  raw_rsp.reserved[4] = 0x06;
  raw_rsp.reserved[5] = 0x10;
  raw_rsp.reserved[6] = 0x05;
  raw_rsp.reserved[7] = 0x13;
  raw_rsp.reserved[8] = 0x06;
  raw_rsp.reserved[9] = 0x12;
  raw_rsp.reserved[10] = 0x12;
  raw_rsp.reserved[11] = 0x01;
  gfps_crypto_encrypt((const uint8_t *)(&raw_rsp.message_type), sizeof(raw_rsp),
                      app_gfps_env.keybase_pair_key, en_rsp.uint128_array);
  app_gfps_send_passkey_via_notification((uint8_t *)en_rsp.uint128_array,
                                         sizeof(en_rsp));

  return (KE_MSG_CONSUMED);
}

static int app_gfps_write_name_ind_hander(ke_msg_id_t const msgid,
                                          struct gfpsp_write_ind_t const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id) {
  bool isSuccessful = false;

  if (!app_gfps_env.isPendingForWritingNameReq) {
    TRACE(0, "Pre fp write name request is not received.");
  } else {
    uint8_t rawName[FP_MAX_NAME_LEN];

    struct gfpsp_write_ind_t *pWriteInd = (struct gfpsp_write_ind_t *)param;
    if (app_gfps_env.isInitialPairing) {
      isSuccessful = gfps_decrypt_name(
          app_gfps_env.aesKeyFromECDH, pWriteInd->data, &(pWriteInd->data[8]),
          &(pWriteInd->data[16]), rawName, pWriteInd->length - 16);
    } else {
      isSuccessful = gfps_decrypt_name(
          app_gfps_env.keybase_pair_key, pWriteInd->data, &(pWriteInd->data[8]),
          &(pWriteInd->data[16]), rawName, pWriteInd->length - 16);
    }

    TRACE(1, "write name successful flag %d", isSuccessful);

    if (isSuccessful) {
      nv_record_fp_update_name(rawName, pWriteInd->length - 16);
      TRACE(1, "Rename BT name: [%s]", rawName);
      app_gfps_update_local_bt_name();
#ifdef IBRT
      app_tws_send_fastpair_info_to_slave();
#endif
    }

    app_gfps_env.isPendingForWritingNameReq = false;
  }

  struct gfpsp_send_write_rsp_t *response = KE_MSG_ALLOC(
      GFPSP_SEND_WRITE_RESPONSE, src_id, dest_id, gfpsp_send_write_rsp_t);
  *response = param->pendingWriteRsp;
  if (isSuccessful) {
    response->status = ATT_ERR_NO_ERROR;
  } else {
    response->status = ATT_ERR_WRITE_NOT_PERMITTED;
  }
  ke_msg_send(response);

  return (KE_MSG_CONSUMED);
}

static int app_gfps_write_accountkey_ind_hander(
    ke_msg_id_t const msgid, struct gfpsp_write_ind_t const *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  NV_FP_ACCOUNT_KEY_ENTRY_T accountkey;

  TRACE(1, "length = %d value = 0x", param->length);
  DUMP8("%02X, ", param->data, FP_ACCOUNT_KEY_SIZE);
  gfps_crypto_decrypt(param->data, FP_ACCOUNT_KEY_SIZE,
                      app_gfps_env.keybase_pair_key, accountkey.key);
  TRACE(0, "decrypt account key:");
  // TRACE(0,"===============================");
  DUMP8("%02X", accountkey.key, FP_ACCOUNT_KEY_SIZE);
  // TRACE(0,"===============================");

  nv_record_fp_account_key_add(&accountkey);

#ifdef IBRT
  app_tws_send_fastpair_info_to_slave();
#endif

  // update the BLE ADV as account key has been added
  if (!app_is_in_fastpairing_mode()) {
    // restart the BLE adv if it's retro-active pairing
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
  }
  return (KE_MSG_CONSUMED);
}

void app_gfps_set_battery_datatype(GFPS_BATTERY_DATA_TYPE_E batteryDataType) {
  if (app_gfps_env.batteryDataType != batteryDataType) {
    app_gfps_env.batteryDataType = batteryDataType;
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
  }
}

GFPS_BATTERY_DATA_TYPE_E app_gfps_get_battery_datatype(void) {
  return app_gfps_env.batteryDataType;
}

void app_gfps_enable_battery_info(bool isEnable) {
  app_gfps_env.isBatteryInfoIncluded = isEnable;
  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void app_gfps_get_battery_levels(uint8_t *pCount, uint8_t *pBatteryLevel) {
  *pCount = 0;
  if (app_gfps_env.get_battery_info_handler) {
    app_gfps_env.get_battery_info_handler(pCount, pBatteryLevel);
  }
}

void app_gfps_update_random_salt(void) {
  app_gfps_env.advRandSalt = (uint8_t)rand();
}

uint8_t app_gfps_generate_accountkey_data(uint8_t *outputData) {
  uint8_t keyCount = nv_record_fp_account_key_count();
  if (0 == keyCount) {
    outputData[0] = 0;
    outputData[1] = 0;
    return 2;
  }

  uint8_t accountKeyData[32];
  accountKeyData[0] = 0;

  uint8_t accountKeyDataLen = 2;
  uint8_t hash256Result[32];

  uint8_t sizeOfFilter = (((uint8_t)((float)1.2 * keyCount)) + 3);
  uint8_t FArray[2 * FP_ACCOUNT_KEY_RECORD_NUM + 3];
  memset(FArray, 0, sizeof(FArray));

#if GFPS_ACCOUNTKEY_SALT_TYPE == USE_BLE_ADDR_AS_SALT
  uint8_t VArray[FP_ACCOUNT_KEY_SIZE + 6 + 4];
#else
  uint8_t VArray[FP_ACCOUNT_KEY_SIZE + 1 + 4];
  uint8_t randomSalt;
  if (GFPS_INITIAL_ADV_RAND_SALT != app_gfps_env.advRandSalt) {
    randomSalt = app_gfps_env.advRandSalt;
  } else {
    randomSalt = (uint8_t)rand();
  }
#endif

  uint8_t index;

  uint8_t batteryFollowingData[1 + GFPS_BATTERY_VALUE_MAX_COUNT];
  uint8_t batteryFollowingDataLen = 0;

  for (uint8_t keyIndex = 0; keyIndex < keyCount; keyIndex++) {
    uint8_t offsetOfVArray;

    nv_record_fp_account_key_get_by_index(keyIndex, VArray);

#if GFPS_ACCOUNTKEY_SALT_TYPE == USE_BLE_ADDR_AS_SALT
    uint8_t *currentBleAddr = appm_get_current_ble_addr();
    for (index = 0; index < 6; index++) {
      VArray[FP_ACCOUNT_KEY_SIZE + index] = currentBleAddr[5 - index];
    }
    offsetOfVArray = FP_ACCOUNT_KEY_SIZE + 6;
#else
    VArray[FP_ACCOUNT_KEY_SIZE] = randomSalt;
    offsetOfVArray = FP_ACCOUNT_KEY_SIZE + 1;
#endif

    if (app_gfps_env.isBatteryInfoIncluded) {
      uint8_t batteryLevelCount = 0;
      uint8_t batteryLevel[GFPS_BATTERY_VALUE_MAX_COUNT];
      app_gfps_get_battery_levels(&batteryLevelCount, batteryLevel);

      uint8_t startOffsetOfBatteryInfo = offsetOfVArray;

      VArray[offsetOfVArray++] =
          app_gfps_env.batteryDataType | (batteryLevelCount << 4);
      for (index = 0; index < batteryLevelCount; index++) {
        VArray[offsetOfVArray++] = batteryLevel[index];
      }

      batteryFollowingDataLen = offsetOfVArray - startOffsetOfBatteryInfo;
      memcpy(batteryFollowingData, &VArray[startOffsetOfBatteryInfo],
             batteryFollowingDataLen);
    }

    TRACE(0, "To hash256 on:");
    DUMP8("%02x ", VArray, offsetOfVArray);

    gfps_SHA256_hash(VArray, offsetOfVArray, hash256Result);

    // K = Xi % (s * 8)
    // F[K/8] = F[K/8] | (1 << (K % 8))
    uint32_t pX[8];
    for (index = 0; index < 8; index++) {
      pX[index] = (((uint32_t)(hash256Result[index * 4])) << 24) |
                  (((uint32_t)(hash256Result[index * 4 + 1])) << 16) |
                  (((uint32_t)(hash256Result[index * 4 + 2])) << 8) |
                  (((uint32_t)(hash256Result[index * 4 + 3])) << 0);
    }

    for (index = 0; index < 8; index++) {
      uint32_t K = pX[index] % (sizeOfFilter * 8);
      FArray[K / 8] = FArray[K / 8] | (1 << (K % 8));
    }
  }

  memcpy(&accountKeyData[2], FArray, sizeOfFilter);

  accountKeyDataLen += sizeOfFilter;

  accountKeyData[1] = (sizeOfFilter << 4);

#if GFPS_ACCOUNTKEY_SALT_TYPE == USE_RANDOM_NUM_AS_SALT
  accountKeyData[2 + sizeOfFilter] = 0x11;
  accountKeyData[2 + sizeOfFilter + 1] = randomSalt;

  accountKeyDataLen += 2;
#endif

  TRACE(1, "Generated accountkey data len:%d", accountKeyDataLen);
  DUMP8("%02x ", accountKeyData, accountKeyDataLen);

  memcpy(outputData, accountKeyData, accountKeyDataLen);

  memcpy(outputData + accountKeyDataLen, batteryFollowingData,
         batteryFollowingDataLen);

  return (accountKeyDataLen + batteryFollowingDataLen);
}

static void gfpsp_update_connection_state(uint8_t conidx) {
  if (BLE_INVALID_CONNECTION_INDEX == app_gfps_env.connectionIndex) {
    app_gfps_connected_evt_handler(conidx);
  }
}

static int app_gfpsp_key_based_pairing_ntf_handler(
    ke_msg_id_t const msgid, struct app_gfps_key_based_notif_config_t *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  app_gfps_env.isKeyBasedPairingNotificationEnabled =
      param->isNotificationEnabled;
  if (app_gfps_env.isKeyBasedPairingNotificationEnabled) {
    uint8_t conidx = KE_IDX_GET(src_id);
    gfpsp_update_connection_state(conidx);
  }

  return (KE_MSG_CONSUMED);
}

static int app_gfpsp_pass_key_ntf_handler(
    ke_msg_id_t const msgid, struct app_gfps_pass_key_notif_config_t *param,
    ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  app_gfps_env.isPassKeyNotificationEnabled = param->isNotificationEnabled;
  if (app_gfps_env.isPassKeyNotificationEnabled) {
    uint8_t conidx = KE_IDX_GET(src_id);
    gfpsp_update_connection_state(conidx);
  }

  return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_gfps_msg_handler_list[] = {
    {GFPSP_VALUE_REQ_IND, (ke_msg_func_t)gfpsp_value_req_ind_handler},
    {GFPSP_KEY_BASED_PAIRING_WRITE_IND,
     (ke_msg_func_t)app_gfps_write_key_based_pairing_ind_hander},
    {GFPSP_KEY_PASS_KEY_WRITE_IND,
     (ke_msg_func_t)app_gfps_write_passkey_ind_hander},
    {GFPSP_KEY_ACCOUNT_KEY_WRITE_IND,
     (ke_msg_func_t)app_gfps_write_accountkey_ind_hander},
    {GFPSP_KEY_BASED_PAIRING_NTF_CFG,
     (ke_msg_func_t)app_gfpsp_key_based_pairing_ntf_handler},
    {GFPSP_KEY_PASS_KEY_NTF_CFG, (ke_msg_func_t)app_gfpsp_pass_key_ntf_handler},
    {GFPSP_NAME_WRITE_IND, (ke_msg_func_t)app_gfps_write_name_ind_hander},

};

const struct ke_state_handler app_gfps_table_handler = {
    &app_gfps_msg_handler_list[0],
    (sizeof(app_gfps_msg_handler_list) / sizeof(struct ke_msg_handler)),
};

static uint8_t is_in_fastpairing_mode = false;

bool app_is_in_fastpairing_mode(void) { return is_in_fastpairing_mode; }

void app_set_in_fastpairing_mode_flag(bool isEnabled) {
  is_in_fastpairing_mode = isEnabled;
  TRACE(1, "[FP]mode is set to %d", is_in_fastpairing_mode);
}

void app_exit_fastpairing_mode(void) {

  if (app_is_in_fastpairing_mode()) {
    TRACE(0, "[FP]exit fast pair mode");
    app_stop_10_second_timer(APP_FASTPAIR_LASTING_TIMER_ID);

    app_set_in_fastpairing_mode_flag(false);

    // reset ble adv
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
  }
}

void app_fast_pairing_timeout_timehandler(void) { app_exit_fastpairing_mode(); }

void app_enter_fastpairing_mode(void) {
  TRACE(0, "[FP] enter fast pair mode");
  app_set_in_fastpairing_mode_flag(true);

  app_ble_start_connectable_adv(BLE_FAST_ADVERTISING_INTERVAL);
  app_start_10_second_timer(APP_FASTPAIR_LASTING_TIMER_ID);
}

/*---------------------------------------------------------------------------
 *            gfps_ble_data_fill_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    BLE advertisement and scan response data fill handler for Google fast pair
 *
 * Parameters:
 *    param - pointer of BLE parameter to be configure
 *
 * Return:
 *    void
 */
static void gfps_ble_data_fill_handler(void *param) {
  TRACE(1, "[%s]+++", __func__);
  ASSERT(param, "invalid param");

  bool adv_enable = false;
  BLE_ADV_PARAM_T *advInfo = (BLE_ADV_PARAM_T *)param;
  TRACE(2, "adv data offset:%d, scan response data offset:%d",
        advInfo->advDataLen, advInfo->scanRspDataLen);

#ifdef IBRT
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  TRACE(1, "current role:%s", app_tws_ibrt_role2str(p_ibrt_ctrl->current_role));

  if (IBRT_SLAVE != p_ibrt_ctrl->current_role && p_ibrt_ctrl->init_done)
#endif
  {
    TRACE(0, "GFPS data will add in adv data");

    adv_enable = true;
    if (app_is_in_fastpairing_mode()) {
      TRACE(0, "fast pair mode");
      advInfo->advInterval = BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL;

      advInfo->advData[advInfo->advDataLen++] = FP_SERVICE_LEN;
      advInfo->advData[advInfo->advDataLen++] = BLE_ADV_SVC_FLAG;
      advInfo->advData[advInfo->advDataLen++] = (FP_SERVICE_UUID >> 8) & 0xFF;
      advInfo->advData[advInfo->advDataLen++] = (FP_SERVICE_UUID >> 0) & 0xFF;
      uint32_t modelId;
#ifndef IS_USE_CUSTOM_FP_INFO
      modelId = FP_DEVICE_MODEL_ID;
#else
      modelId = app_bt_get_model_id();
#endif
      advInfo->advData[advInfo->advDataLen++] = (modelId >> 16) & 0xFF;
      advInfo->advData[advInfo->advDataLen++] = (modelId >> 8) & 0xFF;
      advInfo->advData[advInfo->advDataLen++] = (modelId >> 0) & 0xFF;

#ifndef IS_USE_CUSTOM_FP_INFO
      memcpy(&advInfo->advData[advInfo->advDataLen], APP_GFPS_ADV_POWER_UUID,
             APP_GFPS_ADV_POWER_UUID_LEN);
#else
      memcpy(&advInfo->advData[advInfo->advDataLen], app_gfps_power_uuid,
             APP_GFPS_ADV_POWER_UUID_LEN);
#endif
      advInfo->advDataLen += APP_GFPS_ADV_POWER_UUID_LEN;
    } else {
      TRACE(0, "not in fast pair mode");
      advInfo->advInterval = BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL;

#if BLE_APP_GFPS_VER == FAST_PAIR_REV_2_0
      uint8_t serviceData[32];

      // service UUID part
      serviceData[0] = 0x03; // original length of service length
      serviceData[1] = BLE_ADV_SVC_FLAG;
      serviceData[2] = (FP_SERVICE_UUID >> 8) & 0xFF;
      ;
      serviceData[3] = (FP_SERVICE_UUID >> 0) & 0xFF;

      // account key part
      uint8_t dataLen = app_gfps_generate_accountkey_data(&serviceData[4]);
      serviceData[0] += dataLen;
      memcpy(&advInfo->advData[advInfo->advDataLen], serviceData,
             serviceData[0] + 1);
      advInfo->advDataLen += (serviceData[0] + 1);

      // power part
      memcpy(&advInfo->advData[advInfo->advDataLen], APP_GFPS_ADV_POWER_UUID,
             APP_GFPS_ADV_POWER_UUID_LEN);
      advInfo->advDataLen += APP_GFPS_ADV_POWER_UUID_LEN;
#endif
    }
  }

  app_ble_data_fill_enable(USER_GFPS, adv_enable);
  TRACE(1, "[%s]---", __func__);
}

void gfps_info_prepare_handler(uint8_t *buf, uint16_t *length) {
  *length = sizeof(NV_FP_ACCOUNT_KEY_RECORD_T);

  NV_FP_ACCOUNT_KEY_RECORD_T *info = nv_record_get_fp_data_structure_info();
  memcpy(buf, info, *length);
}

void gfps_info_received_handler(uint8_t *buf, uint16_t length) {
  NV_FP_ACCOUNT_KEY_RECORD_T *pInfo = (NV_FP_ACCOUNT_KEY_RECORD_T *)buf;
  nv_record_fp_update_all((uint8_t *)pInfo);
}

void app_gfps_tws_sync_init(void) {
#ifdef IBRT
  // TODO: freddie move to isolated ota file
  TWS_SYNC_USER_T userGfps = {
      gfps_info_prepare_handler, gfps_info_received_handler, NULL, NULL, NULL,
  };

  app_tws_if_register_sync_user(TWS_SYNC_USER_GFPS_INFO, &userGfps);
#endif
}
static FastPairInfo g_fast_pair_info;
uint32_t app_bt_get_model_id(void) { return g_fast_pair_info.model_id; }

extern uint32_t Get_ModelId();
void app_bt_get_fast_pair_info(void) {
  g_fast_pair_info.model_id = Get_ModelId();
  switch (g_fast_pair_info.model_id) {
  // default model id(bes moddel id)
  case FP_DEVICE_MODEL_ID: {
    memcpy(g_fast_pair_info.public_anti_spoofing_key,
           bes_demo_Public_anti_spoofing_key,
           sizeof(bes_demo_Public_anti_spoofing_key));
    memcpy(g_fast_pair_info.private_anti_spoofing_key,
           bes_demo_private_anti_spoofing_key,
           sizeof(bes_demo_private_anti_spoofing_key));
  } break;

    // customer add customer model id here;

  default: {
    g_fast_pair_info.model_id = FP_DEVICE_MODEL_ID;
    memcpy(g_fast_pair_info.public_anti_spoofing_key,
           bes_demo_Public_anti_spoofing_key,
           sizeof(bes_demo_Public_anti_spoofing_key));
    memcpy(g_fast_pair_info.private_anti_spoofing_key,
           bes_demo_private_anti_spoofing_key,
           sizeof(bes_demo_private_anti_spoofing_key));
  }
  }
}

void app_bt_set_fast_pair_info(FastPairInfo fast_pair_info) {
  memcpy(&g_fast_pair_info, &fast_pair_info, sizeof(fast_pair_info));
}

void app_gfps_set_tx_power_in_adv(char rssi) {
  app_gfps_power_uuid[APP_GFPS_ADV_POWER_UUID_LEN - 1] = rssi;
}

void app_bt_set_fast_pair_tx_power(int8_t tx_power) {
  app_gfps_set_tx_power_in_adv(tx_power);
}

const uint8_t *app_bt_get_fast_pair_public_key(void) {
  return g_fast_pair_info.public_anti_spoofing_key;
}

const uint8_t *app_bt_get_fast_pair_private_key(void) {
  return g_fast_pair_info.private_anti_spoofing_key;
}

#endif // BLE_APP_GFPS

/// @} APP
