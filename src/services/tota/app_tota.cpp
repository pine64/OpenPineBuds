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
#include "app_audio.h"
#include "app_ble_rx_handler.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "app_spp_tota.h"
#include "app_thread.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "app_tota_data_handler.h"
#include "app_utils.h"
#include "apps.h"
#include "bt_drv_reg_op.h"
#include "btapp.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include "hal_aud.h"
#include "hal_location.h"
#include "hal_norflash.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include "rwapp_config.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#if defined(IBRT)
#include "app_tws_ibrt.h"
#endif
#include "aes.h"
#include "app_battery.h"
#include "app_ibrt_rssi.h"
#include "app_spp_tota_general_service.h"
#include "app_tota_anc.h"
#include "app_tota_audio_dump.h"
#include "app_tota_conn.h"
#include "app_tota_custom.h"
#include "app_tota_flash_program.h"
#include "app_tota_general.h"
#include "app_tota_mic.h"
#include "cmsis.h"
#include "crc32.h"
#include "factory_section.h"
#include "tota_stream_data_transfer.h"

typedef struct {
  uint8_t connectedType;
  APP_TOTA_TRANSMISSION_PATH_E dataPath;
} APP_TOTA_ENV_T;

static APP_TOTA_ENV_T app_tota_env = {
    0,
};

bool app_is_in_tota_mode(void) { return app_tota_env.connectedType; }

void app_tota_init(void) {
  TOTA_LOG_DBG(0, "Init application test over the air.");
  app_spp_tota_init();
  app_spp_tota_gen_init();
  app_tota_cmd_handler_init();

  app_tota_stream_data_transfer_init();
  /* register callback modules */
  app_tota_mic_init();
  app_tota_anc_init();
  app_tota_audio_dump_init();
  app_tota_general_init();
  app_tota_custom_init();
  app_tota_flash_init();

  /* set module to access spp callback */
  // tota_callback_module_set(APP_TOTA_AUDIO_DUMP);
  tota_callback_module_set(APP_TOTA_ANC);

#if (BLE_APP_TOTA)
  app_ble_rx_handler_init();
#endif
}

void app_tota_connected(uint8_t connType) {
  TOTA_LOG_DBG(0, "Tota is connected.");
  app_tota_env.connectedType |= connType;
}

void app_tota_disconnected(uint8_t disconnType) {
  TOTA_LOG_DBG(0, "Tota is disconnected.");
  app_tota_env.connectedType &= disconnType;
}

void app_tota_general_connected(uint8_t connType) {
  TOTA_LOG_DBG(0, "Tota gen is connected.");
  app_tota_env.connectedType |= connType;
}

void app_tota_update_datapath(APP_TOTA_TRANSMISSION_PATH_E dataPath) {
  app_tota_env.dataPath = dataPath;
}

APP_TOTA_TRANSMISSION_PATH_E app_tota_get_datapath(void) {
  return app_tota_env.dataPath;
}

/*---------------------------------------------------------------------------------------------------------------------------*/

static bool _is_tota_connect = false;

/**/
void tota_connected_handle() { _is_tota_connect = true; }

/**/
void tota_disconnected_handle() { _is_tota_connect = false; }

bool is_tota_connected() { return _is_tota_connect; }

/*
**  @func:  encrypt && decrypt
*/
static uint8_t encrypt_data[MAX_SPP_PACKET_SIZE];
static uint8_t decrypt_data[MAX_SPP_PACKET_SIZE];

static __unused uint8_t key[ENCRYPT_KEY_SIZE] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static __unused uint8_t iv[ENCRYPT_KEY_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

void tota_set_encrypt_key_from_hash_key(uint8_t *hash_key) {
  for (uint8_t i = 0; i < ENCRYPT_KEY_SIZE; i++) {
    key[i] = hash_key[2 * i];
  }
  TOTA_LOG_DBG(0, "aes key:");
  DUMP8("%02x ", key, ENCRYPT_KEY_SIZE);
}

void test_aes_encode_decode() {
  uint8_t w[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  uint8_t z[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t x[16] = {1, 1, 1, 1, 1, 1, 1, 1};

  uint8_t dw[16] = {0x69, 0x0a, 0x6c, 0x5e, 0xd6, 0x66, 0x51, 0x25,
                    0x97, 0xfa, 0x1e, 0x6e, 0xcc, 0xfc, 0x1b, 0xdb};
  uint8_t dz[16] = {0x50, 0xfe, 0x67, 0xcc, 0x99, 0x6d, 0x32, 0xb6,
                    0xda, 0x09, 0x37, 0xe9, 0x9b, 0xaf, 0xec, 0x60};
  uint32_t length = 0;
  tota_encrypt_packet(w, 16, &length);
  osDelay(50);
  tota_encrypt_packet(z, 16, &length);
  osDelay(50);
  tota_decrypt_packet(dw, 16, &length);
  osDelay(50);
  tota_decrypt_packet(dz, 16, &length);

  tota_encrypt_packet(x, 8, &length);
  osDelay(50);
}

uint8_t *tota_encrypt_packet(uint8_t *in, uint32_t inLen, uint32_t *poutLen) {
  uint32_t morebytes = inLen % ENCRYPT_KEY_SIZE ? ENCRYPT_KEY_SIZE : 0;
  uint32_t outLen = (inLen / ENCRYPT_KEY_SIZE) * ENCRYPT_KEY_SIZE + morebytes;
  TOTA_LOG_DBG(0, "raw data:");
  DUMP8("0x%02x, ", in, inLen);
#if defined(TOTA)
  AES128_CBC_encrypt_buffer(encrypt_data, in, inLen, key, iv);
#endif
  *poutLen = outLen;
  TOTA_LOG_DBG(2, "encrypt data: %u -> %u", inLen, outLen);
  DUMP8("0x%02x, ", encrypt_data, *poutLen);
  return encrypt_data;
}

/*
**  inLen must 16 times
*/
uint8_t *tota_decrypt_packet(uint8_t *in, uint32_t inLen, uint32_t *poutLen) {
  uint32_t morebytes = inLen % ENCRYPT_KEY_SIZE ? ENCRYPT_KEY_SIZE : 0;
  uint32_t outLen = (inLen / ENCRYPT_KEY_SIZE) * ENCRYPT_KEY_SIZE + morebytes;
  TOTA_LOG_DBG(0, "raw data:");
  DUMP8("0x%02x, ", in, inLen);
#if defined(TOTA)
  AES128_CBC_decrypt_buffer(decrypt_data, in, inLen, key, iv);
#endif
  *poutLen = outLen;
  TOTA_LOG_DBG(2, "decrypt data: %u -> %u", inLen, outLen);
  DUMP8("0x%02x, ", decrypt_data, *poutLen);
  return decrypt_data;
}

// TODO:
static char strBuf[MAX_SPP_PACKET_SIZE - 4];
void tota_printf(const char *format, ...) {
  va_list vlist;
  va_start(vlist, format);
  vsprintf(strBuf, format, vlist);
  va_end(vlist);
  app_tota_send_command(OP_TOTA_STRING, (uint8_t *)strBuf, strlen(strBuf),
                        app_tota_get_datapath());
}

/*---------------------------------------------------------------------------------------------------------------------------*/
static void app_tota_demo_cmd_handler(APP_TOTA_CMD_CODE_E funcCode,
                                      uint8_t *ptrParam, uint32_t paramLen) {
  TOTA_LOG_DBG(2, "Func code 0x%x, param len %d", funcCode, paramLen);
  TOTA_LOG_DBG(0, "Param content:");
  DUMP8("%02x ", ptrParam, paramLen);
}
TOTA_COMMAND_TO_ADD(OP_TOTA_DEMO_CMD, app_tota_demo_cmd_handler, false, 0,
                    NULL);