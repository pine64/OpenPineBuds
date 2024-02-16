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
#include "apps.h"
#include "cmsis_os.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "rwapp_config.h"
#include "stdbool.h"
#include "string.h"

#include "app_ble_cmd_handler.h"
#include "app_ble_custom_cmd.h"
#include "retention_ram.h"

#if (BTIF_BLE_APP_DATAPATH_SERVER)

extern void app_datapath_server_send_data_via_notification(uint8_t *ptrData,
                                                           uint32_t length);

void BLE_dummy_handler(uint32_t funcCode, uint8_t *ptrParam,
                       uint32_t paramLen) {}

void BLE_test_no_response_print_handler(uint32_t funcCode, uint8_t *ptrParam,
                                        uint32_t paramLen) {
  TRACE(1, "%s Get OP_TEST_NO_RESPONSE_PRINT command!!!", __FUNCTION__);
}

void BLE_test_with_response_print_handler(uint32_t funcCode, uint8_t *ptrParam,
                                          uint32_t paramLen) {
  TRACE(1, "%s Get OP_TEST_WITH_RESPONSE_PRINT command!!!", __FUNCTION__);

  uint32_t currentTicks = GET_CURRENT_TICKS();
  BLE_send_response_to_command(funcCode, NO_ERROR, (uint8_t *)&currentTicks,
                               sizeof(currentTicks),
                               TRANSMISSION_VIA_NOTIFICATION);
}

void BLE_test_with_response_print_rsp_handler(
    BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t *ptrParam,
    uint32_t paramLen) {
  if (NO_ERROR == retStatus) {
    TRACE(1, "%s Get the response of OP_TEST_WITH_RESPONSE_PRINT command!!!",
          __FUNCTION__);
  } else if (TIMEOUT_WAITING_RESPONSE == retStatus) {
    TRACE(1,
          "%s Timeout happens, doesn't get the response of "
          "OP_TEST_WITH_RESPONSE_PRINT command!!!",
          __FUNCTION__);
  }
}

static void app_otaMode_enter(void) {
  TRACE(1, "%s", __func__);
  hal_norflash_disable_protection(HAL_NORFLASH_ID_0);
  hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
  /*hal_cmu_reset_set(HAL_CMU_MOD_P_GLOBAL);*/
}

void BLE_enter_OTA_mode_handler(uint32_t funcCode, uint8_t *ptrParam,
                                uint32_t paramLen) {
  fillBleBdAddrForOta(ptrParam);
  app_otaMode_enter();
}

#ifdef __SW_IIR_EQ_PROCESS__
int audio_config_eq_iir_via_config_structure(uint8_t *buf, uint32_t len);
void BLE_iir_eq_handler(uint32_t funcCode, uint8_t *ptrParam,
                        uint32_t paramLen) {

  audio_config_eq_iir_via_config_structure(
      BLE_custom_command_raw_data_buffer_pointer(),
      BLE_custom_command_received_raw_data_size());
}

#endif

extern uint8_t bt_addr[6];
void BLE_get_bt_address_handler(uint32_t funcCode, uint8_t *ptrParam,
                                uint32_t paramLen) {
  app_datapath_server_send_data_via_notification((uint8_t *)&bt_addr,
                                                 sizeof(bt_addr));
}

CUSTOM_COMMAND_TO_ADD(OP_RESPONSE_TO_CMD, BLE_get_response_handler, false, 0,
                      NULL);
CUSTOM_COMMAND_TO_ADD(OP_START_RAW_DATA_XFER, BLE_raw_data_xfer_control_handler,
                      true, 2000, BLE_start_raw_data_xfer_control_rsp_handler);
CUSTOM_COMMAND_TO_ADD(OP_STOP_RAW_DATA_XFER, BLE_raw_data_xfer_control_handler,
                      true, 2000, BLE_stop_raw_data_xfer_control_rsp_handler);
CUSTOM_COMMAND_TO_ADD(OP_TEST_NO_RESPONSE_PRINT,
                      BLE_test_no_response_print_handler, false, 0, NULL);
CUSTOM_COMMAND_TO_ADD(OP_TEST_WITH_RESPONSE_PRINT,
                      BLE_test_with_response_print_handler, true, 5000,
                      BLE_test_with_response_print_rsp_handler);
CUSTOM_COMMAND_TO_ADD(OP_ENTER_OTA_MODE, BLE_enter_OTA_mode_handler, false, 0,
                      NULL);
#ifdef __SW_IIR_EQ_PROCESS__
CUSTOM_COMMAND_TO_ADD(OP_SW_IIR_EQ, BLE_iir_eq_handler, false, 0, NULL);
#endif
CUSTOM_COMMAND_TO_ADD(OP_GET_BT_ADDRESS, BLE_get_bt_address_handler, false, 0,
                      NULL);

#if 0
/** \brief  Instances list of BLE custom command handler, should be in the order of the
 *			command code definition in BLE_CUSTOM_CMD_CODE_E
 */
BLE_CUSTOM_CMD_INSTANCE_T     customCommandArray[] =
{
	// { command handler,  wait for response or not,  time out of waiting response in ms,  callback when the response is received }
	// non-touchable instances
	{ BLE_get_response_handler,  			false,  0,  	NULL 										},
	{ BLE_raw_data_xfer_control_handler,  	true,  	2000,  	BLE_start_raw_data_xfer_control_rsp_handler },
	{ BLE_raw_data_xfer_control_handler,  	true,  	2000,  	BLE_stop_raw_data_xfer_control_rsp_handler 	},

	// TO ADD: the new custom command instance
	{ BLE_test_no_response_print_handler, 	false,	0,		NULL 										},
	{ BLE_test_with_response_print_handler,	true,	5000, 	BLE_test_with_response_print_rsp_handler 	},

	{ BLE_enter_OTA_mode_handler,			false,	0, 		NULL 	},

#ifdef __SW_IIR_EQ_PROCESS__
	{ BLE_iir_eq_handler,					false,	0, 		NULL	},
#else
	{ BLE_dummy_handler,					false,	0, 		NULL	},
#endif

	{ BLE_get_bt_address_handler,			false,	0,		NULL	},
};
#endif
#endif
