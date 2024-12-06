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
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h" // SW configuration
#if (BLE_APP_PRESENT)
#include <string.h>

#include "app.h"       // Application Definition
#include "app_task.h"  // Application task Definition
#include "gap.h"       // GAP Definition
#include "gapc_task.h" // GAP Controller Task API
#include "gapm.h"
#include "gapm_task.h" // GAP Manager Task API
#include "tgt_hardware.h"

#include "gattc_task.h"

#include "co_bt.h"   // Common BT Definition
#include "co_math.h" // Common Maths Definition
#include "l2cc.h"
#include "l2cc_pdu.h"

#include "nvrecord_ble.h"
#include "prf.h"

#include "nvrecord_env.h"
#ifndef _BLE_NVDS_
#include "tgt_hardware.h"
#endif
#ifdef __AI_VOICE__
#include "app_ai_ble.h" // AI Voice Application Definitions
#endif                  //(__AI_VOICE__)

#if (BLE_APP_SEC)
#include "app_sec.h" // Application security Definition
#endif               // (BLE_APP_SEC)

#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h" // Data Path Server Application Definitions
#endif                           //(BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_DIS)
#include "app_dis.h" // Device Information Service Application Definitions
#endif               //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h" // Battery Application Definitions
#endif                //(BLE_APP_DIS)

#if (BLE_APP_HID)
#include "app_hid.h" // HID Application Definitions
#endif               //(BLE_APP_HID)

#if (BLE_APP_VOICEPATH)
#include "app_voicepath_ble.h" // Voice Path Application Definitions
#endif                         //(BLE_APP_VOICEPATH)

#if (BLE_APP_OTA)
#include "app_ota.h" // OTA Application Definitions
#endif               //(BLE_APP_OTA)

#if (BLE_APP_TOTA)
#include "app_tota_ble.h" // OTA Application Definitions
#endif                    //(BLE_APP_TOTA)

#if (BLE_APP_ANCC)
#include "app_ancc.h" // ANCC Application Definitions
#endif                //(BLE_APP_ANCC)

#if (BLE_APP_AMS)
#include "app_amsc.h" // AMS Module Definition
#endif                // (BLE_APP_AMS)

#if (BLE_APP_GFPS)
#include "app_gfps.h" // Google Fast Pair Service Definitions
#endif
#if (DISPLAY_SUPPORT)
#include "app_display.h" // Application Display Definition
#endif                   //(DISPLAY_SUPPORT)

#ifdef BLE_APP_AM0
#include "am0_app.h" // Audio Mode 0 Application
#endif               // defined(BLE_APP_AM0)

#if (NVDS_SUPPORT)
#include "nvds.h" // NVDS Definitions
#endif            //(NVDS_SUPPORT)

#include "app_ble_mode_switch.h"
#include "app_bt.h"
#include "apps.h"
#include "besbt.h"
#include "ble_app_dbg.h"
#include "cmsis_os.h"
#include "crc16.h"
#include "ke_timer.h"
#include "nvrecord.h"

#ifdef BISTO_ENABLED
#include "gsound_service.h"
#endif

#if defined(__INTERCONNECTION__)
#include "app_battery.h"
#include "app_bt.h"
#endif

#if (BLE_APP_TILE)
#include "tile_service.h"
#include "tile_target_ble.h"
#endif

#if defined(IBRT)
#include "app_tws_ibrt.h"
#endif

#ifdef __GATT_OVER_BR_EDR__
#include "btgatt_api.h"
#endif

/*
 * DEFINES
 ****************************************************************************************
 */
/// Default Device Name if no value can be found in NVDS
#define APP_DFLT_DEVICE_NAME ("BES_BLE")
#define APP_DFLT_DEVICE_NAME_LEN (sizeof(APP_DFLT_DEVICE_NAME))

#if (BLE_APP_HID)
// HID Mouse
#define DEVICE_NAME "Hid Mouse"
#else
#define DEVICE_NAME "RW DEVICE"
#endif

#define DEVICE_NAME_SIZE sizeof(DEVICE_NAME)

/**
 * UUID List part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x03 - Complete list of 16-bit UUIDs available
 * x09\x18 - Health Thermometer Service UUID
 *   or
 * x12\x18 - HID Service UUID
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_UUID "\x03\x03\x09\x18"
#define APP_HT_ADV_DATA_UUID_LEN (4)
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_UUID "\x03\x03\x12\x18"
#define APP_HID_ADV_DATA_UUID_LEN (4)
#endif //(BLE_APP_HID)

#if (BLE_APP_DATAPATH_SERVER)
/*
 * x11 - Length
 * x07 - Complete list of 16-bit UUIDs available
 * .... the 128 bit UUIDs
 */
#define APP_DATAPATH_SERVER_ADV_DATA_UUID                                      \
  "\x11\x07\x9e\x34\x9B\x5F\x80\x00\x00\x80\x00\x10\x00\x00\x00\x01\x00\x01"
#define APP_DATAPATH_SERVER_ADV_DATA_UUID_LEN (18)
#endif //(BLE_APP_HT)

/**
 * Appearance part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x19 - Appearance
 * x03\x00 - Generic Thermometer
 *   or
 * xC2\x04 - HID Mouse
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_APPEARANCE "\x03\x19\x00\x03"
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_APPEARANCE "\x03\x19\xC2\x03"
#endif //(BLE_APP_HID)

#define APP_ADV_DATA_APPEARANCE_LEN (4)

/**
 * Advertising Parameters
 */
#if (BLE_APP_HID)
/// Default Advertising duration - 30s (in multiple of 10ms)
#define APP_DFLT_ADV_DURATION (3000)
#endif //(BLE_APP_HID)
/// Advertising channel map - 37, 38, 39
#define APP_ADV_CHMAP (0x07)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database
enum appm_svc_list {
#if (BLE_APP_HT)
  APPM_SVC_HTS,
#endif //(BLE_APP_HT)
#if (BLE_APP_DIS)
  APPM_SVC_DIS,
#endif //(BLE_APP_DIS)
#if (BLE_APP_BATT)
  APPM_SVC_BATT,
#endif //(BLE_APP_BATT)
#if (BLE_APP_HID)
  APPM_SVC_HIDS,
#endif //(BLE_APP_HID)
#ifdef BLE_APP_AM0
  APPM_SVC_AM0_HAS,
#endif // defined(BLE_APP_AM0)
#if (BLE_APP_HR)
  APPM_SVC_HRP,
#endif
#if (BLE_APP_DATAPATH_SERVER)
  APPM_SVC_DATAPATH_SERVER,
#endif //(BLE_APP_DATAPATH_SERVER)
#if (BLE_APP_VOICEPATH)
  APPM_SVC_VOICEPATH,
#ifdef BISTO_ENABLED
  APPM_SVC_BMS,
#endif
#endif //(BLE_APP_VOICEPATH)
#if (ANCS_PROXY_ENABLE)
  APPM_SVC_ANCSP,
  APPM_SVC_AMSP,
#endif
#if (BLE_APP_ANCC)
  APPM_SVC_ANCC,
#endif //(BLE_APP_ANCC)
#if (BLE_APP_AMS)
  APPM_SVC_AMSC,
#endif //(BLE_APP_AMS)
#if (BLE_APP_OTA)
  APPM_SVC_OTA,
#endif //(BLE_APP_OTA)
#if (BLE_APP_GFPS)
  APPM_SVC_GFPS,
#endif //(BLE_APP_GFPS)
#if (BLE_AI_VOICE)
  APPM_AI_SMARTVOICE,
#endif //(BLE_AI_VOICE)
#if (BLE_APP_TOTA)
  APPM_SVC_TOTA,
#endif //(BLE_APP_TOTA)

#if (BLE_APP_TILE)
  APPM_SVC_TILE,
#endif //(BLE_APP_TILE)

  APPM_SVC_LIST_STOP,
};

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
// gattc_msg_handler_tab
// #define KE_MSG_HANDLER_TAB(task)   __STATIC const struct ke_msg_handler
// task##_msg_handler_tab[] =
/// Application Task Descriptor

// static const struct ke_task_desc TASK_DESC_APP = {&appm_default_handler,
//                                                  appm_state, APPM_STATE_MAX,
//                                                  APP_IDX_MAX};
extern const struct ke_task_desc TASK_DESC_APP;

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] = {
#if (BLE_APP_HT)
    (appm_add_svc_func_t)app_ht_add_hts,
#endif //(BLE_APP_HT)
#if (BLE_APP_DIS)
    (appm_add_svc_func_t)app_dis_add_dis,
#endif //(BLE_APP_DIS)
#if (BLE_APP_BATT)
    (appm_add_svc_func_t)app_batt_add_bas,
#endif //(BLE_APP_BATT)
#if (BLE_APP_HID)
    (appm_add_svc_func_t)app_hid_add_hids,
#endif //(BLE_APP_HID)
#ifdef BLE_APP_AM0
    (appm_add_svc_func_t)am0_app_add_has,
#endif // defined(BLE_APP_AM0)
#if (BLE_APP_HR)
    (appm_add_svc_func_t)app_hrps_add_profile,
#endif
#if (BLE_APP_DATAPATH_SERVER)
    (appm_add_svc_func_t)app_datapath_add_datapathps,
#endif //(BLE_APP_DATAPATH_SERVER)
#if (BLE_APP_VOICEPATH)
    (appm_add_svc_func_t)app_ble_voicepath_add_svc,
#ifdef BISTO_ENABLED
    (appm_add_svc_func_t)app_ble_bms_add_svc,
#endif
#endif //(BLE_APP_VOICEPATH)
#if (ANCS_PROXY_ENABLE)
    (appm_add_svc_func_t)app_ble_ancsp_add_svc,
    (appm_add_svc_func_t)app_ble_amsp_add_svc,
#endif
#if (BLE_APP_ANCC)
    (appm_add_svc_func_t)app_ancc_add_ancc,
#endif
#if (BLE_APP_AMS)
    (appm_add_svc_func_t)app_amsc_add_amsc,
#endif
#if (BLE_APP_OTA)
    (appm_add_svc_func_t)app_ota_add_ota,
#endif //(BLE_APP_OTA)
#if (BLE_APP_GFPS)
    (appm_add_svc_func_t)app_gfps_add_gfps,
#endif
#if (BLE_APP_AI_VOICE)
    (appm_add_svc_func_t)app_ai_add_ai,
#endif //(BLE_APP_AI_VOICE)
#if (BLE_APP_TOTA)
    (appm_add_svc_func_t)app_tota_add_tota,
#endif //(BLE_APP_TOTA)
#if (BLE_APP_TILE)
    (appm_add_svc_func_t)app_ble_tile_add_svc,
#endif
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

static APP_BLE_NEGOTIATED_CONN_PARAM_T
    negotiatedBleConnParam[BLE_CONNECTION_MAX];

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void appm_refresh_ble_irk(void) {
  nv_record_blerec_get_local_irk(app_env.loc_irk);
  LOG_I("local irk:");
  DUMP8("0x%02x ", app_env.loc_irk, 16);
  gapm_update_irk(app_env.loc_irk);
}

uint8_t *appm_get_current_ble_irk(void) { return app_env.loc_irk; }

void appm_init() {
  BLE_APP_FUNC_ENTER();

  // Reset the application manager environment
  memset(&app_env, 0, sizeof(app_env));

  // Create APP task
  ke_task_create(TASK_APP, &TASK_DESC_APP);

  // Initialize Task state
  ke_state_set(TASK_APP, APPM_INIT);

  // Load the device name from NVDS

  // Get the Device Name to add in the Advertising Data (Default one or NVDS
  // one)
#ifdef _BLE_NVDS_
  const char *ble_name_in_nv = nvrec_dev_get_ble_name();
#else
  const char *ble_name_in_nv = BLE_DEFAULT_NAME;
#endif
  uint32_t nameLen = strlen(ble_name_in_nv) + 1;
  if (nameLen > APP_DEVICE_NAME_MAX_LEN) {
    nameLen = APP_DEVICE_NAME_MAX_LEN;
  }
  // Get default Device Name (No name if not enough space)
  memcpy(app_env.dev_name, ble_name_in_nv, nameLen);
  app_env.dev_name[nameLen - 1] = '\0';
  app_env.dev_name_len = nameLen;
  LOG_I("device ble name:%s", app_env.dev_name);
#ifdef _BLE_NVDS_
  nv_record_blerec_init();

  nv_record_blerec_get_local_irk(app_env.loc_irk);
#else
  uint8_t counter;

  // avoid ble irk collision low probability
  uint32_t generatedSeed = hal_sys_timer_get();
  for (uint8_t index = 0; index < sizeof(bt_addr); index++) {
    generatedSeed ^=
        (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get() & 0xF));
  }
  srand(generatedSeed);

  // generate a new IRK
  for (counter = 0; counter < KEY_LEN; counter++) {
    app_env.loc_irk[counter] = (uint8_t)co_rand_word();
  }
#endif

  /*------------------------------------------------------
   * INITIALIZE ALL MODULES
   *------------------------------------------------------*/

  // load device information:

#if (DISPLAY_SUPPORT)
  app_display_init();
#endif //(DISPLAY_SUPPORT)

#if (BLE_APP_SEC)
  // Security Module
  app_sec_init();
#endif // (BLE_APP_SEC)

#if (BLE_APP_HT)
  // Health Thermometer Module
  app_ht_init();
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
  // Device Information Module
  app_dis_init();
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
  // HID Module
  app_hid_init();
#endif //(BLE_APP_HID)

#if (BLE_APP_BATT)
  // Battery Module
  app_batt_init();
#endif //(BLE_APP_BATT)

#ifdef BLE_APP_AM0
  // Audio Mode 0 Module
  am0_app_init();
#endif // defined(BLE_APP_AM0)

#if (BLE_APP_VOICEPATH)
  // Voice Path Module
  app_ble_voicepath_init();
#endif //(BLE_APP_VOICEPATH)

#if (BLE_APP_TILE)
  app_ble_tile_init();
#endif

#if (BLE_APP_DATAPATH_SERVER)
  // Data Path Server Module
  app_datapath_server_init();
#endif //(BLE_APP_DATAPATH_SERVER)
#if (BLE_APP_AI_VOICE)
  // AI Voice Module
  app_ai_init();
#endif //(BLE_APP_AI_VOICE)

#if (BLE_APP_OTA)
  // OTA Module
  app_ota_init();
#endif //(BLE_APP_OTA)

#if (BLE_APP_TOTA)
  // TOTA Module
  app_tota_ble_init();
#endif //(BLE_APP_TOTA)

#if (BLE_APP_GFPS)
  //
  app_gfps_init();
#endif

#if (BLE_APP_TILE)
  app_tile_init();
#endif

  BLE_APP_FUNC_LEAVE();
}

bool appm_add_svc(void) {
  // Indicate if more services need to be added in the database
  bool more_svc = false;

  // Check if another should be added in the database
  if (app_env.next_svc != APPM_SVC_LIST_STOP) {
    ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL,
                app_env.next_svc, 1);

    BLE_APP_DBG("appm_add_svc adds service");

    // Call the function used to add the required service
    appm_add_svc_func_list[app_env.next_svc]();

    // Select following service to add
    app_env.next_svc++;
    more_svc = true;
  } else {
    BLE_APP_DBG("appm_add_svc doesn't execute, next svc is %d",
                app_env.next_svc);
  }

  return more_svc;
}

uint16_t appm_get_conhdl_from_conidx(uint8_t conidx) {
  return app_env.context[conidx].conhdl;
}

void appm_disconnect(uint8_t conidx) {
  if (BLE_DISCONNECTED != app_env.context[conidx].connectStatus) {
    struct gapc_disconnect_cmd *cmd =
        KE_MSG_ALLOC(GAPC_DISCONNECT_CMD, KE_BUILD_ID(TASK_GAPC, conidx),
                     TASK_APP, gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
  }
}

// TODO: freddie modify this part
#if (BLE_APP_GFPS)
#define UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED 1
#else
#define UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED 0
#endif

void gapm_update_ble_adv_data_right_before_started(uint8_t *pAdvData,
                                                   uint8_t *advDataLen,
                                                   uint8_t *pScanRspData,
                                                   uint8_t *scanRspDataLen,
                                                   void *adv_cmd) {
  // for the user case that the BLE adv data or scan rsp data need to be
  // generated from the run-time BLE address. Just enable macro
  // UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED, and call
  // appm_get_current_ble_addr to get the run-time used BLE address For fastpair
  // GFPS_ACCOUNTKEY_SALT_TYPE==USE_BLE_ADDR_AS_SALT case, this macro must be
  // enabled

  // #if UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED
  //     appm_fill_ble_adv_scan_rsp_data(pAdvData, advDataLen, pScanRspData,
  //     scanRspDataLen, adv_cmd);
  // #endif
  // TODO: freddie implement this API
}

/**
 ****************************************************************************************
 * Advertising Functions
 ****************************************************************************************
 */
void appm_start_advertising(void *param) {
  BLE_APP_FUNC_ENTER();

  LOG_I("%s state: %d", __func__, ke_state_get(TASK_APP));

  // Prepare the GAPM_START_ADVERTISE_CMD message
  struct gapm_start_advertise_cmd *cmd = KE_MSG_ALLOC(
      GAPM_START_ADVERTISE_CMD, TASK_GAPM, TASK_APP, gapm_start_advertise_cmd);

  BLE_ADV_PARAM_T *advParam = (BLE_ADV_PARAM_T *)param;

#ifdef BLE_USE_RPA
  cmd->op.addr_src = GAPM_GEN_RSLV_ADDR;
  cmd->op.randomAddrRenewIntervalInSecond = (uint16_t)(60 * 15);
#else
  cmd->op.addr_src = GAPM_STATIC_ADDR;

  // To use non-resolvable address
  // cmd->op.addr_src    = GAPM_GEN_NON_RSLV_ADDR;
  // cmd->op.randomAddrRenewIntervalInSecond = (uint16_t)(60*15);
#endif

  cmd->channel_map = APP_ADV_CHMAP;
  cmd->intv_min = advParam->advInterval * 8 / 5;
  cmd->intv_max = advParam->advInterval * 8 / 5;
  cmd->op.code = advParam->advType;
  cmd->info.host.mode = GAP_GEN_DISCOVERABLE;

  cmd->isIncludedFlags = advParam->withFlag;
  cmd->info.host.flags = GAP_LE_GEN_DISCOVERABLE_FLG | GAP_BR_EDR_NOT_SUPPORTED;

  cmd->info.host.adv_data_len = advParam->advDataLen;
  memcpy(cmd->info.host.adv_data, advParam->advData,
         cmd->info.host.adv_data_len);
  cmd->info.host.scan_rsp_data_len = advParam->scanRspDataLen;
  memcpy(cmd->info.host.scan_rsp_data, advParam->scanRspData,
         cmd->info.host.scan_rsp_data_len);

  if (advParam->withFlag) {
    ASSERT(cmd->info.host.adv_data_len <= BLE_ADV_DATA_WITH_FLAG_LEN,
           "adv data exceed");
  } else {
    ASSERT(cmd->info.host.adv_data_len <= BLE_ADV_DATA_WITHOUT_FLAG_LEN,
           "adv data exceed");
  }
  ASSERT(cmd->info.host.scan_rsp_data_len <= SCAN_RSP_DATA_LEN,
         "scan rsp data exceed");
  LOG_I("[ADDR_TYPE]:%d", cmd->op.addr_src);
  if (GAPM_GEN_RSLV_ADDR == cmd->op.addr_src) {
    LOG_I("[IRK]:");
    DUMP8("0x%02x ", appm_get_current_ble_irk(), KEY_LEN);
  } else if (GAPM_STATIC_ADDR == cmd->op.addr_src) {
    LOG_I("[ADDR]:");
    DUMP8("0x%02x ", bt_get_ble_local_address(), 6);
  }

  LOG_I("[ADV_TYPE]:%d", cmd->op.code);
  LOG_I("[ADV_INTERVAL]:%d", cmd->intv_min);

  // Send the message
  ke_msg_send(cmd);

  // Set the state of the task to APPM_ADVERTISING
  ke_state_set(TASK_APP, APPM_ADVERTISING);

  BLE_APP_FUNC_LEAVE();
}

void appm_stop_advertising(void) {

#if (BLE_APP_HID)
  // Stop the advertising timer if needed
  if (ke_timer_active(APP_ADV_TIMEOUT_TIMER, TASK_APP)) {
    ke_timer_clear(APP_ADV_TIMEOUT_TIMER, TASK_APP);
  }
#endif //(BLE_APP_HID)

  // Go in ready state
  ke_state_set(TASK_APP, APPM_READY);

  // Prepare the GAPM_CANCEL_CMD message
  struct gapm_cancel_cmd *cmd =
      KE_MSG_ALLOC(GAPM_CANCEL_CMD, TASK_GAPM, TASK_APP, gapm_cancel_cmd);

  cmd->operation = GAPM_CANCEL;

  // Send the message
  ke_msg_send(cmd);

#if (DISPLAY_SUPPORT)
  // Update advertising state screen
  app_display_set_adv(false);
#endif //(DISPLAY_SUPPORT)
}
void appm_start_scanning(uint16_t intervalInMs, uint16_t windowInMs,
                         uint32_t filtPolicy) {
  struct gapm_start_scan_cmd *cmd = KE_MSG_ALLOC(GAPM_START_SCAN_CMD, TASK_GAPM,
                                                 TASK_APP, gapm_start_scan_cmd);

  cmd->op.code = GAPM_SCAN_PASSIVE;
  cmd->op.addr_src = GAPM_STATIC_ADDR;

  cmd->interval = intervalInMs * 1000 / 625;
  cmd->window = windowInMs * 1000 / 625;
  cmd->mode = GAP_OBSERVER_MODE; // GAP_GEN_DISCOVERY;
  cmd->filt_policy = filtPolicy;
  cmd->filter_duplic = SCAN_FILT_DUPLIC_DIS;

  ke_state_set(TASK_APP, APPM_SCANNING);

  ke_msg_send(cmd);
}

void appm_stop_scanning(void) {
  // if (ke_state_get(TASK_APP) == APPM_SCANNING)
  {
    // Go in ready state
    ke_state_set(TASK_APP, APPM_READY);

    // Prepare the GAPM_CANCEL_CMD message
    struct gapm_cancel_cmd *cmd =
        KE_MSG_ALLOC(GAPM_CANCEL_CMD, TASK_GAPM, TASK_APP, gapm_cancel_cmd);

    cmd->operation = GAPM_CANCEL;

    // Send the message
    ke_msg_send(cmd);
  }
}

void appm_add_dev_into_whitelist(struct gap_bdaddr *ptBdAddr) {
  struct gapm_white_list_mgt_cmd *cmd =
      KE_MSG_ALLOC_DYN(GAPM_WHITE_LIST_MGT_CMD, TASK_GAPM, TASK_APP,
                       gapm_white_list_mgt_cmd, sizeof(struct gap_bdaddr));
  cmd->operation = GAPM_ADD_DEV_IN_WLIST;
  cmd->nb = 1;

  memcpy(cmd->devices, ptBdAddr, sizeof(struct gap_bdaddr));

  ke_msg_send(cmd);
}

struct gap_bdaddr BLE_BdAddr; //{{0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00}, 0};
void pts_ble_addr_init(void) {
  uint8_t addr[6] = {0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00};
  memcpy(BLE_BdAddr.addr.addr, addr, 6);
  BLE_BdAddr.addr_type = 0;
}

void appm_start_connecting(struct gap_bdaddr *ptBdAddr) {
  struct gapm_start_connection_cmd *cmd =
      KE_MSG_ALLOC_DYN(GAPM_START_CONNECTION_CMD, TASK_GAPM, TASK_APP,
                       gapm_start_connection_cmd, sizeof(struct gap_bdaddr));
  cmd->ce_len_max = 4;
  cmd->ce_len_min = 0;
  cmd->con_intv_max = 6; // in the unit of 1.25ms
  cmd->con_intv_min = 6; // in the unit of 1.25ms
  cmd->con_latency = 0;
  cmd->superv_to = 1000; // in the unit of 10ms
  cmd->op.code = GAPM_CONNECTION_DIRECT;
  cmd->op.addr_src = GAPM_STATIC_ADDR;
  cmd->nb_peers = 1;
  cmd->scan_interval = ((60) * 1000 / 625);
  cmd->scan_window = ((30) * 1000 / 625);

  memcpy(cmd->peers, ptBdAddr, sizeof(struct gap_bdaddr));

  ke_state_set(TASK_APP, APPM_CONNECTING);

  ke_msg_send(cmd);
}

void appm_stop_connecting(void) {
  // Go in ready state
  ke_state_set(TASK_APP, APPM_READY);

  // Prepare the GAPM_CANCEL_CMD message
  struct gapm_cancel_cmd *cmd =
      KE_MSG_ALLOC(GAPM_CANCEL_CMD, TASK_GAPM, TASK_APP, gapm_cancel_cmd);

  cmd->operation = GAPM_CANCEL;

  // Send the message
  ke_msg_send(cmd);
}

void appm_update_param(uint8_t conidx, struct gapc_conn_param *conn_param) {
  // Prepare the GAPC_PARAM_UPDATE_CMD message
  struct gapc_param_update_cmd *cmd =
      KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD, KE_BUILD_ID(TASK_GAPC, conidx),
                   TASK_APP, gapc_param_update_cmd);

  cmd->operation = GAPC_UPDATE_PARAMS;
  cmd->intv_min = conn_param->intv_min;
  cmd->intv_max = conn_param->intv_max;
  cmd->latency = conn_param->latency;
  cmd->time_out = conn_param->time_out;

  // not used by a slave device
  cmd->ce_len_min = 0xFFFF;
  cmd->ce_len_max = 0xFFFF;

  // Send the message
  ke_msg_send(cmd);
}

void l2cap_update_param(uint8_t conidx, uint32_t min_interval_in_ms,
                        uint32_t max_interval_in_ms,
                        uint32_t supervision_timeout_in_ms,
                        uint8_t slaveLantency) {
  struct l2cc_update_param_req *req =
      L2CC_SIG_PDU_ALLOC(conidx, L2C_CODE_CONN_PARAM_UPD_REQ,
                         KE_BUILD_ID(TASK_GAPC, conidx), l2cc_update_param_req);

  // generate packet identifier
  uint8_t pkt_id = co_rand_word() & 0xFF;
  if (pkt_id == 0) {
    pkt_id = 1;
  }

  /* fill up the parameters */
  req->intv_max = (uint16_t)(max_interval_in_ms / 1.25);
  req->intv_min = (uint16_t)(min_interval_in_ms / 1.25);
  req->latency = slaveLantency;
  req->timeout = supervision_timeout_in_ms / 10;
  req->pkt_id = pkt_id;
  LOG_I("%s val: 0x%x 0x%x 0x%x 0x%x 0x%x", __func__, req->intv_max,
        req->intv_min, req->latency, req->timeout, req->pkt_id);
  l2cc_pdu_send(req);
}

uint8_t appm_get_dev_name(uint8_t *name) {
  // copy name to provided pointer
  memcpy(name, app_env.dev_name, app_env.dev_name_len);
  // return name length
  return app_env.dev_name_len;
}

void appm_exchange_mtu(uint8_t conidx) {
  struct gattc_exc_mtu_cmd *cmd =
      KE_MSG_ALLOC(GATTC_EXC_MTU_CMD, KE_BUILD_ID(TASK_GATTC, conidx), TASK_APP,
                   gattc_exc_mtu_cmd);

  cmd->operation = GATTC_MTU_EXCH;
  cmd->seq_num = 0;

  ke_msg_send(cmd);
}

void appm_check_and_resolve_ble_address(uint8_t conidx) {
  APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

#ifdef __GATT_OVER_BR_EDR__
  if (conidx == btif_btgatt_get_connection_index()) {
    pContext->isGotSolvedBdAddr = true;
    pContext->isBdAddrResolvingInProgress = false;
    btif_btgatt_get_device_address(pContext->solvedBdAddr);
  }
#endif

  // solved already, return
  if (pContext->isGotSolvedBdAddr) {
    LOG_I("Already get solved bd addr.");
    return;
  }
  // not solved yet and the solving is in progress, return and wait
  else if (app_is_resolving_ble_bd_addr()) {
    LOG_I("Random bd addr solving on going.");
    return;
  }

  if (BLE_RANDOM_ADDR == pContext->peerAddrType) {
    memset(pContext->solvedBdAddr, 0, BD_ADDR_LEN);
    bool isSuccessful =
        appm_resolve_random_ble_addr_from_nv(conidx, pContext->bdAddr);
    LOG_I("%s isSuccessful %d", __func__, isSuccessful);
    if (isSuccessful) {
      pContext->isBdAddrResolvingInProgress = true;
    } else {
      pContext->isGotSolvedBdAddr = true;
      pContext->isBdAddrResolvingInProgress = false;
    }
  } else {
    pContext->isGotSolvedBdAddr = true;
    pContext->isBdAddrResolvingInProgress = false;
    memcpy(pContext->solvedBdAddr, pContext->bdAddr, BD_ADDR_LEN);
  }
}

bool appm_resolve_random_ble_addr_from_nv(uint8_t conidx, uint8_t *randomAddr) {
#ifdef _BLE_NVDS_
  struct gapm_resolv_addr_cmd *cmd = KE_MSG_ALLOC_DYN(
      GAPM_RESOLV_ADDR_CMD, KE_BUILD_ID(TASK_GAPM, conidx), TASK_APP,
      gapm_resolv_addr_cmd, BLE_RECORD_NUM * GAP_KEY_LEN);

  uint8_t irkeyNum = nv_record_ble_fill_irk((uint8_t *)(cmd->irk));
  if (0 == irkeyNum) {
    LOG_I("No history irk, cannot solve bd addr.");
    KE_MSG_FREE(cmd);
    return false;
  }

  LOG_I("Start random bd addr solving.");

  cmd->operation = GAPM_RESOLV_ADDR;
  cmd->nb_key = irkeyNum;
  memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
  ke_msg_send(cmd);
  return true;
#else
  return false;
#endif
}

void appm_resolve_random_ble_addr_with_sepcific_irk(uint8_t conidx,
                                                    uint8_t *randomAddr,
                                                    uint8_t *pIrk) {
  struct gapm_resolv_addr_cmd *cmd =
      KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD, KE_BUILD_ID(TASK_GAPM, conidx),
                       TASK_APP, gapm_resolv_addr_cmd, GAP_KEY_LEN);
  cmd->operation = GAPM_RESOLV_ADDR;
  cmd->nb_key = 1;
  memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
  memcpy(cmd->irk, pIrk, GAP_KEY_LEN);
  ke_msg_send(cmd);
}

void appm_random_ble_addr_solved(bool isSolvedSuccessfully,
                                 uint8_t *irkUsedForSolving) {
  APP_BLE_CONN_CONTEXT_T *pContext;
  uint32_t conidx;
  for (conidx = 0; conidx < BLE_CONNECTION_MAX; conidx++) {
    pContext = &(app_env.context[conidx]);
    if (pContext->isBdAddrResolvingInProgress) {
      break;
    }
  }

  if (conidx < BLE_CONNECTION_MAX) {
    pContext->isBdAddrResolvingInProgress = false;
    pContext->isGotSolvedBdAddr = true;

    LOG_I("%s conidx %d isSolvedSuccessfully %d", __func__, conidx,
          isSolvedSuccessfully);
    if (isSolvedSuccessfully) {
#ifdef _BLE_NVDS_
      bool isSuccessful = nv_record_blerec_get_bd_addr_from_irk(
          app_env.context[conidx].solvedBdAddr, irkUsedForSolving);
      if (isSuccessful) {
        LOG_I("[CONNECT]Connected random address's original addr is:");
        DUMP8("%02x ", app_env.context[conidx].solvedBdAddr, GAP_BD_ADDR_LEN);
      } else
#endif
      {
        LOG_I("[CONNECT]Resolving of the connected BLE random addr failed.");
      }
    } else {
      LOG_I("[CONNECT]random resolving failed.");
    }
  }

#if defined(CFG_VOICEPATH)
  ke_task_msg_retrieve(prf_get_task_from_id(TASK_ID_VOICEPATH));
#endif
  ke_task_msg_retrieve(TASK_GAPC);
  ke_task_msg_retrieve(TASK_APP);

  app_ble_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
}

uint8_t app_ble_connection_count(void) { return app_env.conn_cnt; }

bool app_is_arrive_at_max_ble_connections(void) {
  LOG_I("connection count %d", app_env.conn_cnt);

#if defined(GFPS_ENABLED) && (BLE_APP_GFPS_VER == FAST_PAIR_REV_2_0)
  return (app_env.conn_cnt >= BLE_CONNECTION_MAX);
#else
  return (app_env.conn_cnt >= 1);
#endif
}

bool app_is_resolving_ble_bd_addr(void) {
  for (uint32_t index = 0; index < BLE_CONNECTION_MAX; index++) {
    if (app_env.context[index].isBdAddrResolvingInProgress) {
      return true;
    }
  }

  return false;
}

void app_trigger_ble_service_discovery(uint8_t conidx, uint16_t shl,
                                       uint16_t ehl) {
  struct gattc_send_svc_changed_cmd *cmd =
      KE_MSG_ALLOC(GATTC_SEND_SVC_CHANGED_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
                   TASK_APP, gattc_send_svc_changed_cmd);
  cmd->operation = GATTC_SVC_CHANGED;
  cmd->svc_shdl = shl;
  cmd->svc_ehdl = ehl;
  ke_msg_send(cmd);
}

void appm_update_adv_data(uint8_t *pAdvData, uint32_t advDataLen,
                          uint8_t *pScanRspData, uint32_t scanRspDataLen) {
  LOG_I("%s", __func__);
  ASSERT(advDataLen <= BLE_DATA_LEN, "adv data exceed");
  ASSERT(scanRspDataLen <= SCAN_RSP_DATA_LEN, "scan rsp data exceed");

  struct gapm_update_advertise_data_cmd *cmd =
      KE_MSG_ALLOC(GAPM_UPDATE_ADVERTISE_DATA_CMD, TASK_GAPM, TASK_APP,
                   gapm_update_advertise_data_cmd);
  cmd->operation = GAPM_UPDATE_ADVERTISE_DATA;
  memcpy(cmd->adv_data, pAdvData, advDataLen);
  cmd->adv_data_len = advDataLen;

  memcpy(cmd->scan_rsp_data, pScanRspData, scanRspDataLen);
  cmd->scan_rsp_data_len = scanRspDataLen;

  ke_msg_send(cmd);
}

__attribute__((weak)) void
app_ble_connected_evt_handler(uint8_t conidx, const uint8_t *pPeerBdAddress) {}

__attribute__((weak)) void app_ble_disconnected_evt_handler(uint8_t conidx) {}

__attribute__((weak)) void app_advertising_stopped(void) {}

__attribute__((weak)) void app_advertising_started(void) {}

__attribute__((weak)) void app_connecting_started(void) {}

__attribute__((weak)) void app_scanning_stopped(void) {}

__attribute__((weak)) void app_connecting_stopped(void) {}

__attribute__((weak)) void app_scanning_started(void) {}

__attribute__((weak)) void app_ble_system_ready(void) {}

__attribute__((weak)) void
app_adv_reported_scanned(struct gapm_adv_report_ind *ptInd) {}

uint8_t *appm_get_current_ble_addr(void) {
#ifdef BLE_USE_RPA
  return (uint8_t *)gapm_get_bdaddr();
#else
  return ble_addr;
#endif
}

#define IS_WORKAROUND_SAME_BT_BLE_ADDR_FOR_RPA_CASEx
uint8_t *appm_get_local_identity_ble_addr(void) {
#if defined(BLE_USE_RPA) && defined(IS_WORKAROUND_SAME_BT_BLE_ADDR_FOR_RPA_CASE)
  return (uint8_t *)gapm_get_bdaddr();
#else
  return ble_addr;
#endif
}

// bit mask of the existing conn param modes
static uint32_t existingBleConnParamModes[BLE_CONNECTION_MAX] = {0};

static BLE_CONN_PARAM_CONFIG_T ble_conn_param_config[] = {
    // default value: for the case of BLE just connected and the BT idle state
    {BLE_CONN_PARAM_MODE_DEFAULT, BLE_CONN_PARAM_PRIORITY_NORMAL, 24},

    {BLE_CONN_PARAM_MODE_AI_STREAM_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL1,
     24},

    //{BLE_CONN_PARAM_MODE_A2DP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL0, 36},

    {BLE_CONN_PARAM_MODE_HFP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL2, 36},

    {BLE_CONN_PARAM_MODE_OTA, BLE_CONN_PARAM_PRIORITY_HIGH, 12},

    {BLE_CONN_PARAM_MODE_OTA_SLOWER, BLE_CONN_PARAM_PRIORITY_HIGH, 20},

    {BLE_CONN_PARAM_MODE_SNOOP_EXCHANGE, BLE_CONN_PARAM_PRIORITY_HIGH, 8},

    // TODO: add mode cases if needed
};

void app_ble_reset_conn_param_mode(uint8_t conidx) {
  uint32_t lock = int_lock_global();
  existingBleConnParamModes[conidx] = 0;
  int_unlock_global(lock);
}

void app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_E mode, bool isEnable) {
  for (uint8_t index = 0; index < BLE_CONNECTION_MAX; index++) {
    if (BLE_CONNECTED == app_env.context[index].connectStatus) {
      app_ble_update_conn_param_mode_of_specific_connection(index, mode,
                                                            isEnable);
    }
  }
}

bool app_ble_is_parameter_mode_enabled(uint8_t conidx,
                                       BLE_CONN_PARAM_MODE_E mode) {
  bool isEnabled = false;
  uint32_t lock = int_lock_global();
  isEnabled = existingBleConnParamModes[conidx] & (1 << mode);
  int_unlock_global(lock);

  return isEnabled;
}

void app_ble_parameter_mode_clear(uint8_t conidx, BLE_CONN_PARAM_MODE_E mode) {
  uint32_t lock = int_lock_global();
  existingBleConnParamModes[conidx] &= (~(1 << mode));
  int_unlock_global(lock);
}

void app_ble_update_conn_param_mode_of_specific_connection(
    uint8_t conidx, BLE_CONN_PARAM_MODE_E mode, bool isEnable) {
  ASSERT(mode < BLE_CONN_PARAM_MODE_NUM, "Wrong ble conn param mode %d!", mode);

  uint32_t lock = int_lock_global();

  // locate the conn param mode
  BLE_CONN_PARAM_CONFIG_T *pConfig = NULL;
  uint8_t index;

  for (index = 0;
       index < sizeof(ble_conn_param_config) / sizeof(BLE_CONN_PARAM_CONFIG_T);
       index++) {
    if (mode == ble_conn_param_config[index].ble_conn_param_mode) {
      pConfig = &ble_conn_param_config[index];
      break;
    }
  }

  if (NULL == pConfig) {
    int_unlock_global(lock);
    LOG_W("conn param mode %d not defined!", mode);
    return;
  }

  if (isEnable) {
    if (0 == existingBleConnParamModes[conidx]) {
      // no other params existing, just configure this one
      existingBleConnParamModes[conidx] = 1 << mode;
    } else {
      // already existing, directly return
      if (existingBleConnParamModes[conidx] & (1 << mode)) {
        int_unlock_global(lock);
        return;
      } else {
        // update the bit-mask
        existingBleConnParamModes[conidx] |= (1 << mode);

        // not existing yet, need to go throuth the existing params to see
        // whether we need to update the param
        for (index = 0; index < sizeof(ble_conn_param_config) /
                                    sizeof(BLE_CONN_PARAM_CONFIG_T);
             index++) {
          if (((uint32_t)1
               << (uint8_t)ble_conn_param_config[index].ble_conn_param_mode) &
              existingBleConnParamModes[conidx]) {
            if (ble_conn_param_config[index].priority > pConfig->priority) {
              // one of the exiting param has higher priority than this one,
              // so do nothing but update the bit-mask
              int_unlock_global(lock);
              return;
            }
          }
        }

        // no higher priority conn param existing, so we need to apply this one
      }
    }
  } else {
    if (0 == existingBleConnParamModes[conidx]) {
      // no other params existing, just return
      int_unlock_global(lock);
      return;
    } else {
      // doesn't exist, directly return
      if (!(existingBleConnParamModes[conidx] & (1 << mode))) {
        int_unlock_global(lock);
        return;
      } else {
        // update the bit-mask
        existingBleConnParamModes[conidx] &= (~(1 << mode));

        if (0 == existingBleConnParamModes[conidx]) {
          int_unlock_global(lock);
          return;
        }

        pConfig = NULL;

        // existing, need to apply for the highest priority conn param
        for (index = 0; index < sizeof(ble_conn_param_config) /
                                    sizeof(BLE_CONN_PARAM_CONFIG_T);
             index++) {
          if (((uint32_t)1
               << (uint8_t)ble_conn_param_config[index].ble_conn_param_mode) &
              existingBleConnParamModes[conidx]) {
            if (NULL != pConfig) {
              if (ble_conn_param_config[index].priority > pConfig->priority) {
                pConfig = &ble_conn_param_config[index];
              }
            } else {
              pConfig = &ble_conn_param_config[index];
            }
          }
        }
      }
    }
  }

  int_unlock_global(lock);

  // if we can arrive here, it means we have got one config to apply
  ASSERT(NULL != pConfig, "It's strange that config pointer is still NULL.");

  APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

  if (pContext->connInterval != pConfig->conn_param_interval) {
    l2cap_update_param(conidx, pConfig->conn_param_interval * 10 / 8,
                       pConfig->conn_param_interval * 10 / 8,
                       BLE_CONN_PARAM_SUPERVISE_TIMEOUT_MS,
                       BLE_CONN_PARAM_SLAVE_LATENCY_CNT);

    LOG_I("try to update conn interval to %d", pConfig->conn_param_interval);
  }

  LOG_I("conn param mode of conidx %d switched to:0x%x", conidx,
        existingBleConnParamModes[conidx]);
}

void app_ble_save_negotiated_conn_param(
    uint8_t conidx, APP_BLE_NEGOTIATED_CONN_PARAM_T *pConnParam) {
  if (conidx < BLE_CONNECTION_MAX) {
    negotiatedBleConnParam[conidx] = *pConnParam;
  }
}

bool app_ble_get_connection_interval(
    uint8_t conidx, APP_BLE_NEGOTIATED_CONN_PARAM_T *pConnParam) {
  if ((conidx < BLE_CONNECTION_MAX) &&
      (BLE_CONNECTED == app_env.context[conidx].connectStatus)) {
    *pConnParam = negotiatedBleConnParam[conidx];
    return true;
  } else {
    return false;
  }
}

#if GFPS_ENABLED
uint8_t delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
#define FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE (10000)
osTimerId fp_update_ble_param_timer = NULL;
static void fp_update_ble_connect_param_timer_handler(void const *param);
osTimerDef(FP_UPDATE_BLE_CONNECT_PARAM_TIMER,
           (void (*)(void const *))fp_update_ble_connect_param_timer_handler);
extern uint8_t is_sco_mode(void);
static void fp_update_ble_connect_param_timer_handler(void const *param) {
  LOG_I("fp_update_ble_connect_param_timer_handler");
  for (uint8_t index = 0; index < BLE_CONNECTION_MAX; index++) {
    if ((BLE_CONNECTED == app_env.context[index].connectStatus) &&
        (index == delay_update_conidx)) {
      LOG_I("update connection interval of conidx %d", delay_update_conidx);

      if (is_sco_mode()) {
        app_ble_update_conn_param_mode_of_specific_connection(
            delay_update_conidx, BLE_CONN_PARAM_MODE_HFP_ON, true);
      } else {
        app_ble_update_conn_param_mode_of_specific_connection(
            delay_update_conidx, BLE_CONN_PARAM_MODE_DEFAULT, true);
      }
      break;
    }
  }
  delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
}

void fp_update_ble_connect_param_start(uint8_t ble_conidx) {
  if (fp_update_ble_param_timer == NULL) {
    fp_update_ble_param_timer = osTimerCreate(
        osTimer(FP_UPDATE_BLE_CONNECT_PARAM_TIMER), osTimerOnce, NULL);
    return;
  }

  delay_update_conidx = ble_conidx;
  if (fp_update_ble_param_timer)
    osTimerStart(fp_update_ble_param_timer,
                 FP_DELAY_UPDATE_BLE_CONN_PARAM_TIMER_VALUE);
}

void fp_update_ble_connect_param_stop(uint8_t ble_conidx) {
  if (delay_update_conidx == ble_conidx) {
    if (fp_update_ble_param_timer)
      osTimerStop(fp_update_ble_param_timer);
    delay_update_conidx = BLE_INVALID_CONNECTION_INDEX;
  }
}
#endif

#endif //(BLE_APP_PRESENT)
/// @} APP
