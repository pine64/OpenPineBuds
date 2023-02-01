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

#include "tota_stream_data_transfer.h"
#include "app_spp_tota.h"
#include "app_spp_tota_general_service.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "app_tota_conn.h"
#include "app_tota_data_handler.h"
#include "app_utils.h"
#include "apps.h"
#include "cmsis_os.h"
#include "crc32.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "stdbool.h"
#include "string.h"
#include "tota_buffer_manager.h"

/*
**  stream control strcut
**  stream packet format:
**  # header + body #
**  > header: 2   bytes
**  > body  : 664 bytes
**
**  note: stream will never be encrypted!
*/
typedef struct {
  uint32_t flush_bytes;
  bool is_streaming;
  uint8_t module;

  osMutexId mutex;
  osSemaphoreId sem;
} stream_control_t;

stream_control_t stream_control;

osMutexDef(tota_tx_buf_mutex);
osMutexId tota_tx_buf_mutex_id;

#define SPP_BUFFER_NUM 5

static uint8_t spp_tx_buffer[MAX_SPP_PACKET_SIZE * SPP_BUFFER_NUM];
static uint8_t tx_buf_index = 0;

/* static function */
static bool _tota_send_stream_packet(uint8_t *pdata, uint32_t dataLen);
static void _tota_stream_data_init();
static void _update_tx_buf();
static uint8_t *_get_tx_buf_ptr();

// Semaphore
osSemaphoreDef(app_tota_send_data_sem);

// define stream data transfer thread
#define TOTA_STREAM_DATA_STACK_SIZE 2048
static void tota_stream_data_transfer_thread(void const *argument);
osThreadId tota_stream_thread_tid;
osThreadDef(tota_stream_data_transfer_thread, osPriorityHigh, 1,
            TOTA_STREAM_DATA_STACK_SIZE, "TOTA_STREAM_DATA_THREAD");

static void tota_stream_data_transfer_thread(void const *argument) {
  // TODO: max speed
  uint8_t buf[MAX_SPP_PACKET_SIZE];
  while (true) {
    app_sysfreq_req(APP_SYSFREQ_USER_OTA, APP_SYSFREQ_32K);
    osSignalWait(0x0001, osWaitForever);
    app_sysfreq_req(APP_SYSFREQ_USER_OTA, APP_SYSFREQ_208M);
    while (tota_stream_buffer_read(buf + STREAM_HEADER_SIZE,
                                   MAX_SPP_PACKET_SIZE - STREAM_HEADER_SIZE)) {
      _tota_send_stream_packet(buf, MAX_SPP_PACKET_SIZE);
    }
  }
}

void app_tota_stream_data_transfer_init() {
  stream_control.sem =
      osSemaphoreCreate(osSemaphore(app_tota_send_data_sem), SPP_BUFFER_NUM);
  tota_stream_thread_tid =
      osThreadCreate(osThread(tota_stream_data_transfer_thread), NULL);
  _tota_stream_data_init();
}

// stream start
void app_tota_stream_data_start(uint16_t set_module) {
  stream_control.module = set_module;
  stream_control.is_streaming = true;
}

// stream end
void app_tota_stream_data_end() { stream_control.is_streaming = false; }

// stream send data while stream is start
bool app_tota_send_stream_data(uint8_t *pdata, uint32_t dataLen) {
  if (!stream_control.is_streaming) {
    TOTA_LOG_DBG(0, "error: data stream not start.");
    return false;
  }
  if (tota_stream_buffer_write(pdata, dataLen)) {
    TOTA_LOG_DBG(0, "send to stream buffer ok.");
    return true;
  } else {
    TOTA_LOG_DBG(0, "send to stream buffer error.");
    return false;
  }
}

// stream flush
void app_tota_stream_data_flush() { tota_stream_buffer_flush(); }

// stream clean
void app_tota_stream_data_clean() { tota_stream_buffer_clean(); }

bool app_tota_send_data_via_spp(uint8_t *pdata, uint32_t dataLen) {
  osSemaphoreWait(stream_control.sem, osWaitForever);
  osMutexRelease(tota_tx_buf_mutex_id);
  uint8_t *pbuf = pdata;
  uint32_t bufLen = dataLen;
#if TOTA_ENCODE
  if (((uint16_t *)pdata)[0] == OP_TOTA_STRING) {
    TOTA_LOG_DBG(0, "yeah! This is a string. Do not encrypt");
  } else if (is_tota_connected()) {
    pbuf = tota_encrypt_packet(pdata, dataLen, &bufLen);
  }
#endif
  memcpy(_get_tx_buf_ptr(), pbuf, bufLen);

  if (app_spp_tota_send_data(_get_tx_buf_ptr(), bufLen)) {
    _update_tx_buf();
    osMutexRelease(tota_tx_buf_mutex_id);
    return true;
  } else {
    osMutexRelease(tota_tx_buf_mutex_id);
    osSemaphoreRelease(stream_control.sem);
    return false;
  }
}

void app_tota_tx_done_callback() { osSemaphoreRelease(stream_control.sem); }

bool is_stream_data_running() { return stream_control.is_streaming; }

// send stream packet with header. packet size:666
static bool _tota_send_stream_packet(uint8_t *pdata, uint32_t dataLen) {
  osSemaphoreWait(stream_control.sem, osWaitForever);
  osMutexRelease(tota_tx_buf_mutex_id);
  ((uint16_t *)pdata)[0] = (OP_TOTA_STREAM_DATA - stream_control.module);
  memcpy(_get_tx_buf_ptr(), pdata, dataLen);
  if (app_spp_tota_send_data(_get_tx_buf_ptr(), dataLen)) {
    _update_tx_buf();
    osMutexRelease(tota_tx_buf_mutex_id);
    return true;
  } else {
    osMutexRelease(tota_tx_buf_mutex_id);
    osSemaphoreRelease(stream_control.sem);
    return false;
  }
}

static void _tota_stream_data_init() {
  tota_stream_buffer_init(tota_stream_thread_tid);
  stream_control.module = 0;
  stream_control.is_streaming = false;
}

static void _update_tx_buf() { tx_buf_index = (tx_buf_index + 1) % 5; }

static uint8_t *_get_tx_buf_ptr() {
  return (spp_tx_buffer + tx_buf_index * MAX_SPP_PACKET_SIZE);
}
