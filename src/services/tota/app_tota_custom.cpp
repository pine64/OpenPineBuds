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

#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_ibrt_nvrecord.h"
#include "app_spp_tota.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "apps.h"
#include "cmsis_os.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "tota_stream_data_transfer.h"

/**/
static void _custom_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                               uint32_t paramLen);

/*-----------------------------------------------------------------------------*/
static void _tota_spp_connected(void);
static void _tota_spp_disconnected(void);
static void _tota_spp_tx_done(void);
static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len);

static tota_callback_func_t s_func = {_tota_spp_connected,
                                      _tota_spp_disconnected, _tota_spp_tx_done,
                                      _tota_spp_data_receive_handle};

static APP_TOTA_MODULE_E s_module = APP_TOTA_CUSTOM;

void app_tota_custom_init() { tota_callback_module_register(s_module, s_func); }

static void _tota_spp_connected(void) { ; }

static void _tota_spp_disconnected(void) { ; }

static void _tota_spp_tx_done(void) { ; }

static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len) { ; }
/*-----------------------------------------------------------------------------*/

static void _custom_cmd_handle(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                               uint32_t paramLen) {
  TOTA_LOG_DBG(2, "Func code 0x%x, param len %d", funcCode, paramLen);
  TOTA_LOG_DBG(0, "Param content:");
  DUMP8("%02x ", ptrParam, paramLen);
  APP_TOTA_CMD_RET_STATUS_E rsp_status = TOTA_NO_ERROR;
  app_tota_send_response_to_command(funcCode, rsp_status, NULL, 0,
                                    app_tota_get_datapath());
  switch (funcCode) {
  case OP_TOTA_FACTORY_RESET:
    TRACE(0, "##### custom: factory reset");
    app_reset();
    break;
  case OP_TOTA_CLEAR_PAIRING_INFO:
    TRACE(0, "##### custom: clear pairing info");
    app_ibrt_nvrecord_delete_all_mobile_record();
    break;
  case OP_TOTA_SHUTDOWM:
    TRACE(0, "##### custom: shutdown");
    app_shutdown();
    break;
  case OP_TOTA_REBOOT:
    TRACE(0, "##### custom: reboot");
    // TODO:
    break;
  default:;
  }
}

TOTA_COMMAND_TO_ADD(OP_TOTA_FACTORY_RESET, _custom_cmd_handle, false, 0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_CLEAR_PAIRING_INFO, _custom_cmd_handle, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_SHUTDOWM, _custom_cmd_handle, false, 0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_REBOOT, _custom_cmd_handle, false, 0, NULL);
