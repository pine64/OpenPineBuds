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
#include "app_tota_audio_dump.h"
#include "app_spp_tota.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "cmsis_os.h"
#include "tota_stream_data_transfer.h"

// #define _TOTA_AUDIO_DUMP_DEBUG

static void _tota_audio_dump_connected(void);
static void _tota_audio_dump_disconnected(void);
static void _tota_audio_dump_tx_done(void);
static void _tota_audio_dump_receive_handle(uint8_t *buf, uint32_t len);

/**/
static void _audio_dump_control(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                                uint32_t paramLen);

#ifdef _TOTA_AUDIO_DUMP_DEBUG
/* only for test */
static void test_init();
#endif

/**/
static tota_callback_func_t s_func = {
    _tota_audio_dump_connected, _tota_audio_dump_disconnected,
    _tota_audio_dump_tx_done, _tota_audio_dump_receive_handle};

static APP_TOTA_MODULE_E s_module = APP_TOTA_AUDIO_DUMP;

void app_tota_audio_dump_init() {
  TOTA_LOG_DBG(1, "[%s] ...", __func__);

  tota_callback_module_register(s_module, s_func);
}

static void _tota_audio_dump_connected(void) {
  TOTA_LOG_DBG(1, "[%s] ...", __func__);

#ifdef _TOTA_AUDIO_DUMP_DEBUG
  test_init();
#endif
}

static void _tota_audio_dump_disconnected(void) {
  TOTA_LOG_DBG(1, "[%s] ...", __func__);
}

static void _tota_audio_dump_tx_done(void) { ; }

static void _tota_audio_dump_receive_handle(uint8_t *buf, uint32_t len) { ; }

/*-----------------------------------------------------------------------------*/
void app_tota_audio_dump_start() {
  TOTA_LOG_DBG(1, "[%s] ...", __func__);

  app_tota_stream_data_start(s_module);
  app_tota_audio_dump_flush();
}

void app_tota_audio_dump_stop() {
  TOTA_LOG_DBG(1, "[%s] ...", __func__);

  app_tota_stream_data_end();
}

void app_tota_audio_dump_flush() {
  TOTA_LOG_DBG(1, "[%s] ...", __func__);

  app_tota_stream_data_flush();
}

bool app_tota_audio_dump_send(uint8_t *pdata, uint32_t dataLen) {
  return app_tota_send_stream_data(pdata, dataLen);
}

/*-----------------------------------------------------------------------------*/
static void _audio_dump_control(APP_TOTA_CMD_CODE_E funcCode, uint8_t *ptrParam,
                                uint32_t paramLen) {
  switch (funcCode) {
  case OP_TOTA_AUDIO_DUMP_START:
    TOTA_LOG_DBG(0, "tota_audio_dump start");
    app_tota_audio_dump_start();
    break;

  case OP_TOTA_AUDIO_DUMP_STOP:
    TOTA_LOG_DBG(0, "tota_audio_dump stop");
    app_tota_audio_dump_stop();
    break;

  case OP_TOTA_AUDIO_DUMP_CONTROL:
    TOTA_LOG_DBG(0, "tota_audio_dump contorl");
    // TODO: Can get or set info
    app_tota_send_response_to_command(funcCode, TOTA_NO_ERROR,
                                      (uint8_t *)".pcm", sizeof(".pcm"),
                                      app_tota_get_datapath());
    break;

  default:
    break;
  }
}

#ifdef _TOTA_AUDIO_DUMP_DEBUG
/*-----------------------------------------------------------------------------*/
/*----------------------------------TEST---------------------------------------*/
/*-----------------------------------------------------------------------------*/
#define TOTA_TEST_STACK_SIZE (1024 * 2)
static void tota_audio_dump_test_thread(void const *argument);
osThreadId tota_audio_dump_test_thread_tid;
osThreadDef(tota_audio_dump_test_thread, osPriorityNormal, 1,
            TOTA_TEST_STACK_SIZE, "TOTA_TEST_THREAD"); // osPriorityHigh

static uint16_t audio_cnt = 0;

uint16_t audioBuf[512];

static void tota_audio_dump_test_thread(void const *argument) {
  app_tota_audio_dump_flush();
  while (true) {
    osDelay(1000);
    for (uint16_t i = 0; i < sizeof(audioBuf) / sizeof(uint16_t); i++) {
      audioBuf[i] = audio_cnt++;
      // audioBuf[i] = 0x5555;
    }

    app_tota_audio_dump_send((uint8_t *)audioBuf, sizeof(audioBuf));
  }
}

static void test_init() {
  tota_audio_dump_test_thread_tid =
      osThreadCreate(osThread(tota_audio_dump_test_thread), NULL);
}
#endif

/*-----------------------------------------------------------------------------*/
/*------------------------------ADD USR
 * FUNCTION-------------------------------*/
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/*------------------------------ADD USR
 * COMMAND--------------------------------*/
/*-----------------------------------------------------------------------------*/
TOTA_COMMAND_TO_ADD(OP_TOTA_AUDIO_DUMP_START, _audio_dump_control, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_AUDIO_DUMP_STOP, _audio_dump_control, false, 0,
                    NULL);
TOTA_COMMAND_TO_ADD(OP_TOTA_AUDIO_DUMP_CONTROL, _audio_dump_control, false, 0,
                    NULL);
