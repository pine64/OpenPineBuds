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

#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_data_handler.h"
#include "app_tota_flash_program.h"
#include "app_utils.h"
#include "cmsis_os.h"
#include "crc32.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "tota_buffer_manager.h"
#include "tota_stream_data_transfer.h"
#include <stdint.h>

uint8_t flash_wbuf[] = "hello";

static void tota_test_cmd(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                          uint32_t paramLen) {
  tota_printf("this is cmd test\r\n");
  tota_printf("hello1\r\n");
  tota_printf("hello2\r\n");
  tota_printf("hello3\r\n");
  tota_printf("hello4\r\n");
  tota_printf("will write data to flash");
  test_aes_encode_decode();
  tota_write_flash_test(TOTA_FLASH_TEST_ADDR, flash_wbuf,
                        strlen((char *)flash_wbuf));
}

static void tota_echo_test_cmd(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                               uint32_t paramLen) {
  tota_printf("echo: \r\n");
  app_tota_send_data_via_spp(ptrParam, paramLen);
}

TOTA_COMMAND_TO_ADD(OP_TOTA_ECHO_TEST_CMD, tota_echo_test_cmd, false, 0, NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_TEST_CMD, tota_test_cmd, false, 0, NULL);
