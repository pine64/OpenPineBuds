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
#include "app_tota_general.h"
#include "app_hfp.h"
#include "app_key.h"
#include "app_spp_tota.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "bluetooth.h"
#include "cmsis.h"
#include "crc32.h"
#include "hal_cmu.h"
#include "nvrecord_ble.h"
/*
** general info struct
**  ->  bt  name
**  ->  ble name
**  ->  bt  local/peer addr
**  ->  ble local/peer addr
**  ->  ibrt role
**  ->  crystal freq
**  ->  xtal fcap
**  ->  bat volt/level/status
**  ->  fw version
**  ->  ear location
**  ->  rssi info
*/

/*------------------------------------------------------------------------------------------------------------------------*/

/*
**  general info
*/
static general_info_t general_info;

/*
**  get general info
*/
static void __get_general_info();

/*
**  handle general cmd
*/
static void __tota_general_cmd_handle(APP_TOTA_CMD_CODE_E funcCode,
                                      uint8_t *ptrParam, uint32_t paramLen);

/*------------------------------------------------------------------------------------------------------------------------*/
static void _tota_spp_connected(void);
static void _tota_spp_disconnected(void);
static void _tota_spp_tx_done(void);
static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len);

static tota_callback_func_t s_func = {_tota_spp_connected,
                                      _tota_spp_disconnected, _tota_spp_tx_done,
                                      _tota_spp_data_receive_handle};

static APP_TOTA_MODULE_E s_module = APP_TOTA_GENERAL;

void app_tota_general_init() {
  tota_callback_module_register(s_module, s_func);
}

static void _tota_spp_connected(void) { ; }

static void _tota_spp_disconnected(void) { ; }

static void _tota_spp_tx_done(void) { ; }

static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len) { ; }

/*------------------------------------------------------------------------------------------------------------------------*/

static void __tota_general_cmd_handle(APP_TOTA_CMD_CODE_E funcCode,
                                      uint8_t *ptrParam, uint32_t paramLen) {
  TOTA_LOG_DBG(2, "Func code 0x%x, param len %d", funcCode, paramLen);
  TOTA_LOG_DBG(0, "Param content:");
  DUMP8("%02x ", ptrParam, paramLen);
  uint8_t resData[48] = {0};
  uint32_t resLen = 1;
  uint8_t volume_level;
  switch (funcCode) {
  case OP_TOTA_GENERAL_INFO_CMD:
    __get_general_info();
    app_tota_send_response_to_command(
        funcCode, TOTA_NO_ERROR, (uint8_t *)&general_info,
        sizeof(general_info_t), app_tota_get_datapath());
    // for test
    // app_tota_send_response_to_command(funcCode,TOTA_NO_ERROR,(uint8_t
    // *)&resLen, sizeof(uint32_t),app_tota_get_datapath());
    return;
  case OP_TOTA_MERIDIAN_EFFECT_CMD:
    resData[0] = app_meridian_eq(ptrParam[0]);
    break;
  case OP_TOTA_EQ_SELECT_CMD:
    break;
  case OP_TOTA_VOLUME_PLUS_CMD:
    app_bt_volumeup();
    volume_level = app_bt_stream_local_volume_get();
    resData[0] = volume_level;
    TRACE(1, "volume = %d", volume_level);
    break;
  case OP_TOTA_VOLUME_DEC_CMD:
    app_bt_volumedown();
    volume_level = app_bt_stream_local_volume_get();
    resData[0] = volume_level;
    resLen = 1;
    TRACE(1, "volume = %d", volume_level);
    break;
  case OP_TOTA_VOLUME_SET_CMD:
    // uint8_t scolevel = ptrParam[0];
    // uint8_t a2dplevel = ptrParam[1];
    app_bt_set_volume(APP_BT_STREAM_HFP_PCM, ptrParam[0]);
    app_bt_set_volume(APP_BT_STREAM_A2DP_SBC, ptrParam[1]);
    btapp_hfp_report_speak_gain();
    btapp_a2dp_report_speak_gain();
    break;
  case OP_TOTA_VOLUME_GET_CMD:
    resData[0] = app_bt_stream_hfpvolume_get();
    resData[1] = app_bt_stream_a2dpvolume_get();
    resLen = 2;
    break;
  case OP_TOTA_EQ_SET_CMD:
    // int eq_index = ptrParam[0];
    if (ptrParam[0] == 3)
      resData[0] = app_meridian_eq(true);
    else
      resData[0] = app_audio_set_eq(ptrParam[0]);
    resLen = 1;
    break;
  case OP_TOTA_EQ_GET_CMD:
    resData[0] = app_audio_get_eq();
    resLen = 1;
    break;
  case OP_TOTA_RAW_DATA_SET_CMD:
    app_ibrt_debug_parse(ptrParam, paramLen);
    break;
  default:
    TRACE(1, "wrong cmd 0x%x", funcCode);
    resData[0] = -1;
    return;
  }
  app_tota_send_response_to_command(funcCode, TOTA_NO_ERROR, resData, resLen,
                                    app_tota_get_datapath());
}

/* get general info */
static void __get_general_info() {
  /* get bt-ble name */
  uint8_t *factory_name_ptr = factory_section_get_bt_name();
  if (factory_name_ptr != NULL) {
    uint16_t valid_len =
        strlen((char *)factory_name_ptr) > BT_BLE_LOCAL_NAME_LEN
            ? BT_BLE_LOCAL_NAME_LEN
            : strlen((char *)factory_name_ptr);
    memcpy(general_info.btName, factory_name_ptr, valid_len);
  }

  factory_name_ptr = factory_section_get_ble_name();
  if (factory_name_ptr != NULL) {
    uint16_t valid_len =
        strlen((char *)factory_name_ptr) > BT_BLE_LOCAL_NAME_LEN
            ? BT_BLE_LOCAL_NAME_LEN
            : strlen((char *)factory_name_ptr);
    memcpy(general_info.bleName, factory_name_ptr, valid_len);
  }

  /* get bt-ble peer addr */
  ibrt_config_t addrInfo;
  app_ibrt_ui_test_config_load(&addrInfo);
  general_info.ibrtRole = addrInfo.nv_role;
  memcpy(general_info.btLocalAddr.address, addrInfo.local_addr.address, 6);
  memcpy(general_info.btPeerAddr.address, addrInfo.peer_addr.address, 6);

#ifdef BLE
  memcpy(general_info.bleLocalAddr.address, bt_get_ble_local_address(), 6);
  memcpy(general_info.blePeerAddr.address, nv_record_tws_get_peer_ble_addr(),
         6);
#endif

  /* get crystal info */
  general_info.crystal_freq = hal_cmu_get_crystal_freq();

  /* factory_section_xtal_fcap_get */
  factory_section_xtal_fcap_get(&general_info.xtal_fcap);

  /* get battery info (volt level)*/
  app_battery_get_info(&general_info.battery_volt, &general_info.battery_level,
                       &general_info.battery_status);

  /* get firmware version */
#ifdef FIRMWARE_REV
  system_get_info(&general_info.fw_version[0], &general_info.fw_version[1],
                  &general_info.fw_version[2], &general_info.fw_version[3]);
  TRACE(4, "firmware version = %d.%d.%d.%d", general_info.fw_version[0],
        general_info.fw_version[1], general_info.fw_version[2],
        general_info.fw_version[3]);
#endif

  /* get ear location info */
  if (app_tws_is_right_side())
    general_info.ear_location = EAR_SIDE_RIGHT;
  else if (app_tws_is_left_side())
    general_info.ear_location = EAR_SIDE_LEFT;
  else
    general_info.ear_location = EAR_SIDE_UNKNOWN;

  app_ibrt_rssi_get_stutter(general_info.rssi, &general_info.rssi_len);
}

/* general command */
TOTA_COMMAND_TO_ADD(OP_TOTA_GENERAL_INFO_CMD, __tota_general_cmd_handle, false,
                    0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_MERIDIAN_EFFECT_CMD, __tota_general_cmd_handle,
                    false, 0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_SELECT_CMD, __tota_general_cmd_handle, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_PLUS_CMD, __tota_general_cmd_handle, false,
                    0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_DEC_CMD, __tota_general_cmd_handle, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_SET_CMD, __tota_general_cmd_handle, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_GET_CMD, __tota_general_cmd_handle, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_SET_CMD, __tota_general_cmd_handle, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_GET_CMD, __tota_general_cmd_handle, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_RAW_DATA_SET_CMD, __tota_general_cmd_handle, false,
                    0, NULL);
