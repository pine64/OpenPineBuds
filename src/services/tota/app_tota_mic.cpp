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

#include "app_tota_mic.h"
#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_spp_tota.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "cmsis_os.h"
#include "tota_stream_data_transfer.h"

static void _tota_spp_connected(void);
static void _tota_spp_disconnected(void);
static void _tota_spp_tx_done(void);
static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len);
/**/
static void _mic_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                            uint32_t paramLen);

/**/
static tota_callback_func_t s_func = {_tota_spp_connected,
                                      _tota_spp_disconnected, _tota_spp_tx_done,
                                      _tota_spp_data_receive_handle};

static APP_TOTA_MODULE_E s_module = APP_TOTA_MIC;

void app_tota_mic_init() { tota_callback_module_register(s_module, s_func); }

static void _tota_spp_connected(void) { ; }

static void _tota_spp_disconnected(void) { ; }

static void _tota_spp_tx_done(void) { ; }

static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len) { ; }

/*-----------------------------------------------------------------------------*/
static void _mic_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                            uint32_t paramLen) {
  TOTA_LOG_DBG(2, "Func code 0x%x, param len %d", funcCode, paramLen);
  TOTA_LOG_DBG(0, "Param content:");
  DUMP8("%02x ", ptrParam, paramLen);
  APP_TOTA_CMD_RET_STATUS_E rsp_status = TOTA_NO_ERROR;
  uint8_t resData[48] = {0};
  uint32_t resLen = 1;
  switch (funcCode) {
  case OP_TOTA_MIC_TEST_ON:
    TRACE(0, "#####mic test on:");
#ifdef __FACTORY_MODE_SUPPORT__
    app_audio_sendrequest(APP_BT_STREAM_INVALID,
                          (uint8_t)APP_BT_SETTING_CLOSEALL, 0);
    app_audio_sendrequest(APP_FACTORYMODE_AUDIO_LOOP,
                          (uint8_t)APP_BT_SETTING_OPEN, 0);
#endif
    break;
  case OP_TOTA_MIC_TEST_OFF:
    TRACE(0, "#####mic test off:");
#ifdef __FACTORY_MODE_SUPPORT__
    app_audio_sendrequest(APP_FACTORYMODE_AUDIO_LOOP,
                          (uint8_t)APP_BT_SETTING_CLOSE, 0);
#endif
    break;
  case OP_TOTA_MIC_SWITCH:
    TRACE(0, "####mic switch test, do role switch");
    if (!app_ibrt_ui_tws_switch())
      rsp_status = TOTA_MIC_SWITCH_FAILED;
    break;
  default:;
  }
  app_tota_send_response_to_command(funcCode, rsp_status, resData, resLen,
                                    app_tota_get_datapath());
}

TOTA_COMMAND_TO_ADD(OP_TOTA_MIC_TEST_ON, _mic_cmd_handle, false, 0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_MIC_TEST_OFF, _mic_cmd_handle, false, 0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_MIC_SWITCH, _mic_cmd_handle, false, 0, NULL);
