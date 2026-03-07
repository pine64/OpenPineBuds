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

#include "tota_buffer_manager.h"
#include "app_tota_cmd_code.h"
#include "cmsis_os.h"
#include "string.h"

osMutexDef(stream_buf_mutex);
osMutexId stream_buf_mutex_id;

static stream_buf_t stream_buf;
static osThreadId father_thread_tid;

void tota_stream_buffer_init(osThreadId tid) {
  stream_buf.dataSize = 0;
  stream_buf.availableSpace = TOTA_STREAM_BUF_SIZE;
  stream_buf.readPos = 0;
  stream_buf.writePos = 0;
  stream_buf.flushBytes = 0;

  stream_buf_mutex_id = osMutexCreate(osMutex(stream_buf_mutex));
  father_thread_tid = tid;
}

bool tota_stream_buffer_write(uint8_t *buf, uint32_t bufLen) {
  osMutexWait(stream_buf_mutex_id, osWaitForever);
  if (stream_buf.availableSpace > bufLen) {
    // write to stream buf
    if (stream_buf.writePos + bufLen <= TOTA_STREAM_BUF_SIZE) {
      memcpy(stream_buf.buf + stream_buf.writePos, buf, bufLen);

      stream_buf.writePos += bufLen;
      if (stream_buf.writePos == TOTA_STREAM_BUF_SIZE)
        stream_buf.writePos = 0;
    } else {
      uint32_t part1 = TOTA_STREAM_BUF_SIZE - stream_buf.writePos;
      memcpy(stream_buf.buf + stream_buf.writePos, buf, part1);
      memcpy(stream_buf.buf, buf + part1, bufLen - part1);
      stream_buf.writePos = bufLen - part1;
    }

    stream_buf.dataSize += bufLen;
    stream_buf.availableSpace -= bufLen;

    TOTA_LOG_DBG(1, "buffer> after write: %5u bytes", bufLen);
    TOTA_LOG_DBG(4, "buffer> size:%5u available:%5u writepos:%5u readpos:%5u:",
                 stream_buf.dataSize, stream_buf.availableSpace,
                 stream_buf.writePos, stream_buf.readPos);

    osMutexRelease(stream_buf_mutex_id);
    /* set signal to father thread, has data */
    osSignalSet(father_thread_tid, 0x0001);

    return true;
  } else {
    osMutexRelease(stream_buf_mutex_id);
    return false;
  }
}

bool tota_stream_buffer_read(uint8_t *rbuf, uint32_t readSize,
                             uint32_t *flushbytes) {
  osMutexWait(stream_buf_mutex_id, osWaitForever);
  if (readSize < stream_buf.dataSize) {
    if (stream_buf.readPos + readSize <= TOTA_STREAM_BUF_SIZE) {
      memcpy(rbuf, stream_buf.buf + stream_buf.readPos, readSize);
      stream_buf.readPos += readSize;
      if (stream_buf.readPos == TOTA_STREAM_BUF_SIZE)
        stream_buf.readPos = 0;
    } else {
      uint32_t part1 = TOTA_STREAM_BUF_SIZE - stream_buf.readPos;
      memcpy(rbuf, stream_buf.buf + stream_buf.readPos, part1);
      memcpy(rbuf + part1, stream_buf.buf, readSize - part1);
      stream_buf.readPos = readSize - part1;
    }
    stream_buf.dataSize -= readSize;
    stream_buf.availableSpace += readSize;
    /* debug */
    TOTA_LOG_DBG(1, "buffer> after read: %5u bytes", readSize);
    TOTA_LOG_DBG(4, "buffer> size:%5u available:%5u writepos:%5u readpos:%5u:",
                 stream_buf.dataSize, stream_buf.availableSpace,
                 stream_buf.writePos, stream_buf.readPos);
    if (flushbytes != NULL) {
      *flushbytes = stream_buf.flushBytes;
      stream_buf.flushBytes = 0;
    }
    osMutexRelease(stream_buf_mutex_id);
    return true;
  } else {
    osMutexRelease(stream_buf_mutex_id);
    return false;
  }
}

void tota_stream_buffer_flush(void) {
  osMutexWait(stream_buf_mutex_id, osWaitForever);
  stream_buf.flushBytes = stream_buf.dataSize;
  stream_buf.availableSpace = TOTA_STREAM_BUF_SIZE;
  stream_buf.dataSize = 0;
  stream_buf.readPos = 0;
  stream_buf.writePos = 0;
  osMutexRelease(stream_buf_mutex_id);
}

void tota_stream_buffer_clean(void) {
  osMutexWait(stream_buf_mutex_id, osWaitForever);
  stream_buf.flushBytes = 0;
  stream_buf.availableSpace = TOTA_STREAM_BUF_SIZE;
  stream_buf.dataSize = 0;
  stream_buf.readPos = 0;
  stream_buf.writePos = 0;
  osMutexRelease(stream_buf_mutex_id);
}