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
// Standard C Included Files
#include "a2dp_decoder.h"
#include "a2dp_decoder_internal.h"
#include "app_audio.h"
#include "app_bt.h"
#include "app_bt_media_manager.h"
#include "audioflinger.h"
#include "avdtp_api.h"
#include "bt_drv_reg_op.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "codec_sbc.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "heap_api.h"
#include "plat_types.h"
#include <string.h>
#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_audio_analysis.h"
#include "app_tws_ibrt_audio_sync.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "bt_drv_interface.h"
#include "btapp.h"
#endif
#include "audio_prompt_sbc.h"
#include "crc32.h"

#ifndef A2DP_AUDIO_MEMPOOL_SIZE
#if defined(A2DP_LDAC_ON)
#define A2DP_AUDIO_LDAC_MEMPOOL_SIZE (76 * 1024)
#endif
#define A2DP_AUDIO_MEMPOOL_SIZE (72 * 1024)
#endif
#define A2DP_AUDIO_WAIT_TIMEOUT_MS (500)
#define A2DP_AUDIO_MUTE_FRAME_CNT_AFTER_NO_CACHE (25)
#define A2DP_AUDIO_SKIP_FRAME_LIMIT_AFTER_NO_CACHE (50)

#define A2DP_AUDIO_REFILL_AFTER_NO_CACHE (1)

#ifdef __A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__
#define A2DP_AUDIO_SYNC_INTERVAL (25)
#else
#define A2DP_AUDIO_SYNC_INTERVAL (1000)
#endif

#define A2DP_AUDIO_LATENCY_LOW_FACTOR (1.0f)
#define A2DP_AUDIO_LATENCY_HIGH_FACTOR (1.6f)
#define A2DP_AUDIO_LATENCY_MORE_FACTOR (1.2f)

#define A2DP_AUDIO_SYNC_FACTOR_REFERENCE (1.0f)
#define A2DP_AUDIO_SYNC_FACTOR_FAST_LIMIT (0.00015f)
#define A2DP_AUDIO_SYNC_FACTOR_SLOW_LIMIT (-0.00035f)
#define A2DP_AUDIO_SYNC_FACTOR_NEED_FAST_CACHE (-0.001f)

#define A2DP_AUDIO_UNDERFLOW_CAUSE_AUDIO_RETRIGGER (1)

extern A2DP_AUDIO_DECODER_T a2dp_audio_sbc_decoder_config;
#if defined(A2DP_AAC_ON)
extern A2DP_AUDIO_DECODER_T a2dp_audio_aac_lc_decoder_config;
#endif
#if defined(A2DP_LHDC_ON)
extern A2DP_AUDIO_DECODER_T a2dp_audio_lhdc_decoder_config;
#endif

#if defined(A2DP_LDAC_ON)
extern A2DP_AUDIO_DECODER_T a2dp_audio_ldac_decoder_config;
#endif

#if defined(A2DP_SCALABLE_ON)
extern A2DP_AUDIO_DECODER_T a2dp_audio_scalable_decoder_config;
#endif

osSemaphoreDef(audio_buffer_semaphore);
osMutexDef(audio_buffer_mutex);
osMutexDef(audio_status_mutex);
osMutexDef(audio_stop_mutex);

#ifdef __A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__
//#define A2DP_AUDIO_SYNC_FIX_DIFF_INTERVA_PRINT_FLOAT (1)

#define A2DP_AUDIO_SYNC_FIX_DIFF_INTERVAL (640)
#define A2DP_AUDIO_SYNC_FIX_DIFF_FAST_LIMIT (0.003f)
#define A2DP_AUDIO_SYNC_FIX_DIFF_SLOW_LIMIT (-0.003f)

extern uint32_t app_bt_stream_get_dma_buffer_samples(void);
static int a2dp_audio_sync_fix_diff_proc(uint32_t tick);
static int a2dp_audio_sync_fix_diff_stop(uint32_t tick);
static int32_t a2dp_audio_sync_fix_diff_reset(void);
static int32_t a2dp_audio_sync_fix_diff_start(uint32_t tick);

typedef enum {
  A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_IDLE,
  A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_START,
  A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_STOP,
} A2DP_AUDIO_SYNC_FIX_DIFF_STATUS;

typedef struct {
  int32_t tick;
  A2DP_AUDIO_SYNC_FIX_DIFF_STATUS status;
} A2DP_AUDIO_SYNC_FIX_DIFF_T;

static A2DP_AUDIO_SYNC_FIX_DIFF_T a2dp_audio_sync_fix_diff;
#endif

A2DP_AUDIO_CONTEXT_T a2dp_audio_context;

static heap_handle_t a2dp_audio_heap;

static A2DP_AUDIO_LASTFRAME_INFO_T a2dp_audio_lastframe_info;

static A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK
    a2dp_audio_detect_next_packet_callback = NULL;
static A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK a2dp_audio_store_packet_callback =
    NULL;

static float a2dp_audio_latency_factor = A2DP_AUDIO_LATENCY_LOW_FACTOR;
static float sync_tune_dest_ratio = 1.0f;

static uint32_t store_packet_history_loctime = 0;

static uint32_t check_sum_seed = 0;

static int a2dp_audio_internal_lastframe_info_ptr_get(
    A2DP_AUDIO_LASTFRAME_INFO_T **lastframe_info);

float a2dp_audio_get_sample_reference(void);
int8_t a2dp_audio_get_current_buf_size(void);

void a2dp_audio_heap_init(void *begin_addr, uint32_t size) {
  a2dp_audio_heap = heap_register(begin_addr, size);
}

void *a2dp_audio_heap_malloc(uint32_t size) {
  void *ptr = heap_malloc(a2dp_audio_heap, size);
  ASSERT_A2DP_DECODER(ptr, "%s size:%d", __func__, size);
  return ptr;
}

void *a2dp_audio_heap_cmalloc(uint32_t size) {
  void *ptr = heap_malloc(a2dp_audio_heap, size);
  ASSERT_A2DP_DECODER(ptr, "%s size:%d", __func__, size);
  memset(ptr, 0, size);
  return ptr;
}

void *a2dp_audio_heap_realloc(void *rmem, uint32_t newsize) {
  void *ptr = heap_realloc(a2dp_audio_heap, rmem, newsize);
  ASSERT_A2DP_DECODER(ptr, "%s rmem:%p size:%d", __func__, rmem, newsize);
  return ptr;
}

void a2dp_audio_heap_free(void *rmem) {
  ASSERT_A2DP_DECODER(rmem, "%s rmem:%p", __func__, rmem);
  heap_free(a2dp_audio_heap, rmem);
}

void a2dp_audio_heap_info(size_t *total, size_t *used, size_t *max_used) {
  multi_heap_info_t info;
  heap_get_info(a2dp_audio_heap, &info);

  if (total != NULL)
    *total = info.total_bytes;

  if (used != NULL)
    *used = info.total_allocated_bytes;

  if (max_used != NULL)
    *max_used = info.total_bytes - info.minimum_free_bytes;
}

int inline a2dp_audio_semaphore_init(void) {
  if (a2dp_audio_context.audio_semaphore.semaphore == NULL) {
    a2dp_audio_context.audio_semaphore.semaphore =
        osSemaphoreCreate(osSemaphore(audio_buffer_semaphore), 0);
  }
  a2dp_audio_context.audio_semaphore.enalbe = false;
  return 0;
}

int inline a2dp_audio_buffer_mutex_init(void) {
  if (a2dp_audio_context.audio_buffer_mutex == NULL) {
    a2dp_audio_context.audio_buffer_mutex =
        osMutexCreate((osMutex(audio_buffer_mutex)));
  }
  return 0;
}

int inline a2dp_audio_buffer_mutex_lock(void) {
  osMutexWait((osMutexId)a2dp_audio_context.audio_buffer_mutex, osWaitForever);
  return 0;
}

int inline a2dp_audio_buffer_mutex_unlock(void) {
  osMutexRelease((osMutexId)a2dp_audio_context.audio_buffer_mutex);
  return 0;
}

int inline a2dp_audio_status_mutex_init(void) {
  if (a2dp_audio_context.audio_status_mutex == NULL) {
    a2dp_audio_context.audio_status_mutex =
        osMutexCreate((osMutex(audio_status_mutex)));
  }
  return 0;
}

int inline a2dp_audio_status_mutex_lock(void) {
  osMutexWait((osMutexId)a2dp_audio_context.audio_status_mutex, osWaitForever);
  return 0;
}

int inline a2dp_audio_status_mutex_unlock(void) {
  osMutexRelease((osMutexId)a2dp_audio_context.audio_status_mutex);
  return 0;
}

int inline a2dp_audio_semaphore_wait(uint32_t timeout_ms) {
  osSemaphoreId semaphore_id =
      (osSemaphoreId)a2dp_audio_context.audio_semaphore.semaphore;

  a2dp_audio_buffer_mutex_lock();
  a2dp_audio_context.audio_semaphore.enalbe = true;
  a2dp_audio_buffer_mutex_unlock();

  int32_t nRet = osSemaphoreWait(semaphore_id, timeout_ms);
  if ((0 == nRet) || (-1 == nRet)) {
    TRACE_A2DP_DECODER_W("%s wait timerout", __func__);
    return -1;
  }
  return 0;
}

int inline a2dp_audio_semaphore_release(void) {
  bool enalbe = false;

  a2dp_audio_buffer_mutex_lock();
  if (a2dp_audio_context.audio_semaphore.enalbe) {
    a2dp_audio_context.audio_semaphore.enalbe = false;
    enalbe = true;
  }
  a2dp_audio_buffer_mutex_unlock();

  if (enalbe) {
    osSemaphoreId semaphore_id =
        (osSemaphoreId)a2dp_audio_context.audio_semaphore.semaphore;
    osSemaphoreRelease(semaphore_id);
  }
  return 0;
}

list_node_t *a2dp_audio_list_begin(const list_t *list) {
  a2dp_audio_buffer_mutex_lock();
  list_node_t *node = list_begin(list);
  a2dp_audio_buffer_mutex_unlock();
  return node;
}

list_node_t *a2dp_audio_list_end(const list_t *list) {
  a2dp_audio_buffer_mutex_lock();
  list_node_t *node = list_end(list);
  a2dp_audio_buffer_mutex_unlock();
  return node;
}

uint32_t a2dp_audio_list_length(const list_t *list) {
  a2dp_audio_buffer_mutex_lock();
  uint32_t length = list_length(list);
  a2dp_audio_buffer_mutex_unlock();
  return length;
}

void *a2dp_audio_list_node(const list_node_t *node) {
  a2dp_audio_buffer_mutex_lock();
  void *data = list_node(node);
  a2dp_audio_buffer_mutex_unlock();
  return data;
}

list_node_t *a2dp_audio_list_next(const list_node_t *node) {
  a2dp_audio_buffer_mutex_lock();
  list_node_t *next = list_next(node);
  a2dp_audio_buffer_mutex_unlock();
  return next;
}

bool a2dp_audio_list_remove(list_t *list, void *data) {
  a2dp_audio_buffer_mutex_lock();
  bool nRet = list_remove(list, data);
  a2dp_audio_buffer_mutex_unlock();
  return nRet;
}

bool a2dp_audio_list_append(list_t *list, void *data) {
  a2dp_audio_buffer_mutex_lock();
  bool nRet = list_append(list, data);
  a2dp_audio_buffer_mutex_unlock();
  return nRet;
}

void a2dp_audio_list_clear(list_t *list) {
  a2dp_audio_buffer_mutex_lock();
  list_clear(list);
  a2dp_audio_buffer_mutex_unlock();
}

void a2dp_audio_list_free(list_t *list) {
  a2dp_audio_buffer_mutex_lock();
  list_free(list);
  a2dp_audio_buffer_mutex_unlock();
}

list_t *a2dp_audio_list_new(list_free_cb callback, list_mempool_zmalloc zmalloc,
                            list_mempool_free free) {
  a2dp_audio_buffer_mutex_lock();
  list_t *list = list_new(callback, zmalloc, free);
  a2dp_audio_buffer_mutex_unlock();
  return list;
}

uint32_t a2dp_audio_get_passed(uint32_t curr_ticks, uint32_t prev_ticks,
                               uint32_t max_ticks) {
  if (curr_ticks < prev_ticks)
    return ((max_ticks - prev_ticks + 1) + curr_ticks);
  else
    return (curr_ticks - prev_ticks);
}

#ifdef A2DP_CP_ACCEL
extern "C" uint32_t get_in_cp_frame_cnt(void);
extern "C" uint32_t get_in_cp_frame_delay(void);
#else
static uint32_t get_in_cp_frame_cnt(void) { return 0; }

static uint32_t get_in_cp_frame_delay(void) { return 0; }
#endif

int inline a2dp_audio_set_status(
    enum A2DP_AUDIO_DECODER_STATUS decoder_status) {
  a2dp_audio_status_mutex_lock();
  a2dp_audio_context.audio_decoder_status = decoder_status;
  a2dp_audio_status_mutex_unlock();

  return 0;
}

enum A2DP_AUDIO_DECODER_STATUS inline a2dp_audio_get_status(void) {
  enum A2DP_AUDIO_DECODER_STATUS decoder_status;
  a2dp_audio_status_mutex_lock();
  decoder_status = a2dp_audio_context.audio_decoder_status;
  a2dp_audio_status_mutex_unlock();

  return decoder_status;
}

int inline a2dp_audio_set_store_packet_status(
    enum A2DP_AUDIO_DECODER_STORE_PACKET_STATUS store_packet_status) {
  a2dp_audio_status_mutex_lock();
  a2dp_audio_context.store_packet_status = store_packet_status;
  a2dp_audio_status_mutex_unlock();

  return 0;
}

enum A2DP_AUDIO_DECODER_STORE_PACKET_STATUS inline a2dp_audio_get_store_packet_status(
    void) {
  enum A2DP_AUDIO_DECODER_STORE_PACKET_STATUS store_packet_status;
  a2dp_audio_status_mutex_lock();
  store_packet_status = a2dp_audio_context.store_packet_status;
  a2dp_audio_status_mutex_unlock();

  return store_packet_status;
}

int inline a2dp_audio_set_playback_status(
    enum A2DP_AUDIO_DECODER_PLAYBACK_STATUS playback_status) {
  a2dp_audio_status_mutex_lock();
  a2dp_audio_context.playback_status = playback_status;
  a2dp_audio_status_mutex_unlock();

  return 0;
}

enum A2DP_AUDIO_DECODER_PLAYBACK_STATUS inline a2dp_audio_get_playback_status(
    void) {
  enum A2DP_AUDIO_DECODER_PLAYBACK_STATUS playback_status;
  a2dp_audio_status_mutex_lock();
  playback_status = a2dp_audio_context.playback_status;
  a2dp_audio_status_mutex_unlock();

  return playback_status;
}

static void a2dp_decoder_info_checker(void) {
  //    app_bt_start_custom_function_in_app_thread(0, 0,
  //    (uint32_t)bt_drv_reg_op_bt_info_checker);
}

int a2dp_audio_sync_pid_config(void) {
  A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;
  A2DP_AUDIO_SYNC_PID_T *pid = &audio_sync->pid;

  pid->proportiongain = 0.4f;
  pid->integralgain = 0.1f;
  pid->derivativegain = 0.6f;

  return 0;
}

int a2dp_audio_sync_reset_data(void) {
  A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;

  a2dp_audio_status_mutex_lock();
  audio_sync->tick = 0;
  audio_sync->cnt = 0;
  a2dp_audio_sync_pid_config();
#ifdef __A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__
  a2dp_audio_sync_fix_diff_reset();
#endif
  a2dp_audio_status_mutex_unlock();

  TRACE_A2DP_DECODER_I("[SYNC]reset_data");
  return 0;
}

int a2dp_audio_sync_init(double ratio) {
#ifdef __A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__
  a2dp_audio_sync_fix_diff_reset();
#endif
  a2dp_audio_sync_reset_data();
  a2dp_audio_sync_tune_sample_rate(ratio);
  sync_tune_dest_ratio = (float)ratio;
  return 0;
}

int a2dp_audio_sync_tune_sample_rate(double ratio) {
  float resample_rate_ratio;
  bool need_tune = false;

  a2dp_audio_status_mutex_lock();
  if (a2dp_audio_context.output_cfg.factor_reference != (float)ratio) {
    a2dp_audio_context.output_cfg.factor_reference = (float)ratio;
    resample_rate_ratio = (float)(ratio - (double)1.0);
    need_tune = true;
  }
  // a2dp_audio_status_mutex_unlock();

  if (need_tune) {
    app_audio_manager_tune_samplerate_ratio(AUD_STREAM_PLAYBACK,
                                            resample_rate_ratio);
    TRACE_A2DP_DECODER_I("[SYNC] ppb:%d ratio:%08d",
                         (int32_t)(resample_rate_ratio * 10000000),
                         (int32_t)(ratio * 10000000));
  }
  a2dp_audio_status_mutex_unlock();

  return 0;
}

int a2dp_audio_sync_direct_tune_sample_rate(double ratio) {
  float resample_rate_ratio;
  bool need_tune = false;

  a2dp_audio_status_mutex_lock();
  if (a2dp_audio_context.output_cfg.factor_reference != (float)ratio) {
    a2dp_audio_context.output_cfg.factor_reference = (float)ratio;
    resample_rate_ratio = (float)(ratio - (double)1.0);
    need_tune = true;
  }

  if (need_tune) {
    af_codec_direct_tune(AUD_STREAM_PLAYBACK, resample_rate_ratio);

    TRACE_A2DP_DECODER_I("[SYNC] ppb:%d ratio:%08d",
                         (int32_t)(resample_rate_ratio * 10000000),
                         (int32_t)(ratio * 10000000));
  }
  a2dp_audio_status_mutex_unlock();

  return 0;
}

bool a2dp_audio_sync_tune_onprocess(void) {
  bool nRet = false;
  ;
  if (a2dp_audio_context.output_cfg.factor_reference != sync_tune_dest_ratio) {
    nRet = true;
  }
  return nRet;
}

int a2dp_audio_sync_tune(float ratio) {
  int nRet = 0;
  if (sync_tune_dest_ratio == ratio) {
    goto exit;
  }
  sync_tune_dest_ratio = ratio;
#if defined(IBRT)
  if (app_ibrt_ui_is_profile_exchanged()) {
    if (app_tws_ibrt_mobile_link_connected()) {
      APP_TWS_IBRT_AUDIO_SYNC_TUNE_T sync_tune;
      sync_tune.factor_reference = ratio;
      if (!app_tws_ibrt_audio_sync_tune_need_skip()) {
        tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_TUNE, (uint8_t *)&sync_tune,
                          sizeof(APP_TWS_IBRT_AUDIO_SYNC_TUNE_T));
      } else {
        a2dp_audio_sync_tune_cancel();
        nRet = -1;
      }
    }
  } else {
    a2dp_audio_sync_tune_sample_rate(ratio);
  }
#else
  a2dp_audio_sync_tune_sample_rate(ratio);
#endif
exit:
  return nRet;
}

int a2dp_audio_sync_tune_cancel(void) {
  sync_tune_dest_ratio = a2dp_audio_context.output_cfg.factor_reference;
  return 0;
}

float a2dp_audio_sync_pid_calc(A2DP_AUDIO_SYNC_PID_T *pid, float diff) {
  float increment;
  float pError, dError, iError;

  pid->error[0] = diff;
  pError = pid->error[0] - pid->error[1];
  iError = pid->error[0];
  dError = pid->error[0] - 2 * pid->error[1] + pid->error[2];

  increment = pid->proportiongain * pError + pid->integralgain * iError +
              pid->derivativegain * dError;

  pid->error[2] = pid->error[1];
  pid->error[1] = pid->error[0];

  pid->result += increment;

  return pid->result;
}

#ifdef __A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__
static int a2dp_audio_sync_fix_diff_proc(uint32_t tick) {
  if (a2dp_audio_sync_fix_diff.status ==
      A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_START) {
    if (a2dp_audio_sync_fix_diff.tick > 0) {
      a2dp_audio_sync_fix_diff.tick--;
    } else {
      a2dp_audio_sync_fix_diff_stop(0);
    }
  }
  return 0;
}

static int32_t a2dp_audio_sync_fix_diff_start(uint32_t tick) {
  TRACE_A2DP_DECODER_I("[SYNC] fix diff start");

  a2dp_audio_sync_fix_diff.status = A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_START;
  a2dp_audio_sync_fix_diff.tick = tick;
  return 0;
}

static int a2dp_audio_sync_fix_diff_stop(uint32_t tick) {
  TRACE_A2DP_DECODER_I("[SYNC] fix diff stop");

  a2dp_audio_sync_fix_diff.status = A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_STOP;
  a2dp_audio_sync_fix_diff.tick = 0;

  return 0;
}

static int32_t a2dp_audio_sync_fix_diff_reset(void) {
  TRACE_A2DP_DECODER_I("[SYNC] fix diff reset");

  a2dp_audio_sync_fix_diff.status = A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_IDLE;
  a2dp_audio_sync_fix_diff.tick = 0;

  return 0;
}

static A2DP_AUDIO_SYNC_FIX_DIFF_STATUS
a2dp_audio_sync_fix_diff_status_get(void) {
  return a2dp_audio_sync_fix_diff.status;
}

int a2dp_audio_sync_handler(uint8_t *buffer, uint32_t buffer_bytes) {
  A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info = NULL;
  A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;
  float diff_mtu = 0;
  bool need_tune = false;
  bool force_slow = false;

#if defined(IBRT)
  if (!app_tws_ibrt_mobile_link_connected()) {
    return -1;
  }
#endif
  if (a2dp_audio_internal_lastframe_info_ptr_get(&lastframe_info) < 0) {
    return -1;
  }

  a2dp_audio_sync_fix_diff_proc(audio_sync->tick);
  if (a2dp_audio_sync_fix_diff_status_get() ==
      A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_STOP) {
#if defined(IBRT)
    if (!app_tws_ibrt_audio_sync_tune_onprocess() &&
        !a2dp_audio_sync_tune_onprocess()
#else
    if (!a2dp_audio_sync_tune_onprocess()
#endif
    ) {
      int sync_tune_result = 0;
      if (a2dp_audio_context.output_cfg.factor_reference !=
          a2dp_audio_context.init_factor_reference) {
        sync_tune_result =
            a2dp_audio_sync_tune(a2dp_audio_context.init_factor_reference);
      }
      if (!sync_tune_result) {
        a2dp_audio_sync_fix_diff_reset();
#ifdef A2DP_AUDIO_SYNC_FIX_DIFF_INTERVA_PRINT_FLOAT
        TRACE_A2DP_DECODER_I("[SYNC] tune ratio normal %f mut:%5.3f->%d",
                             (double)a2dp_audio_context.init_factor_reference,
                             (double)a2dp_audio_context.average_packet_mut,
                             a2dp_audio_context.dest_packet_mut);
#else
        TRACE_A2DP_DECODER_I(
            "[SYNC] tune ratio normal %d mut:%d->%d",
            (int32_t)(a2dp_audio_context.init_factor_reference * 10000000),
            (int32_t)(a2dp_audio_context.average_packet_mut + 0.5f),
            a2dp_audio_context.dest_packet_mut);
#endif
      } else {
        TRACE_A2DP_DECODER_I("[SYNC] tune ratio normal busy");
      }

    } else {
#ifdef A2DP_AUDIO_SYNC_FIX_DIFF_INTERVA_PRINT_FLOAT
      TRACE_A2DP_DECODER_I("[SYNC] tune ratio busy %f mut:%5.3f->%d",
                           (double)a2dp_audio_context.init_factor_reference,
                           (double)a2dp_audio_context.average_packet_mut,
                           a2dp_audio_context.dest_packet_mut);
#else
      TRACE_A2DP_DECODER_I(
          "[SYNC] tune ratio busy %d mut:%d->%d",
          (int32_t)(a2dp_audio_context.init_factor_reference * 10000000),
          (int32_t)(a2dp_audio_context.average_packet_mut + 0.5f),
          a2dp_audio_context.dest_packet_mut);
#endif
    }
    goto exit;
  }

  if (audio_sync->tick % A2DP_AUDIO_SYNC_INTERVAL == 0) {
    diff_mtu = a2dp_audio_context.average_packet_mut -
               (float)a2dp_audio_context.dest_packet_mut;
    if (ABS(diff_mtu) < 0.6f) {
#ifdef A2DP_AUDIO_SYNC_FIX_DIFF_INTERVA_PRINT_FLOAT
      TRACE_A2DP_DECODER_I("[SYNC] skip mut:%5.3f", (double)diff_mtu);
#else
      TRACE_A2DP_DECODER_I("[SYNC] skip mut:0.%d", (int32_t)(diff_mtu * 10));
#endif
      goto exit;
    } else if (diff_mtu / a2dp_audio_context.dest_packet_mut < -0.2f) {
      float curr_ratio = a2dp_audio_context.output_cfg.factor_reference;
      float ref_ratio = a2dp_audio_context.init_factor_reference;
      if (curr_ratio != (ref_ratio + A2DP_AUDIO_SYNC_FIX_DIFF_SLOW_LIMIT)) {
#ifdef A2DP_AUDIO_SYNC_FIX_DIFF_INTERVA_PRINT_FLOAT
        TRACE_A2DP_DECODER_I("[SYNC] force slow mut:%5.3f", (double)diff_mtu);
#else
        TRACE_A2DP_DECODER_I("[SYNC] force slow mut:0.%d",
                             (int32_t)(diff_mtu * 10));
#endif
        force_slow = true;
      }
    }
    if (a2dp_audio_sync_fix_diff_status_get() ==
            A2DP_AUDIO_SYNC_FIX_DIFF_STATUS_IDLE ||
        force_slow) {
      uint32_t dma_buffer_samples, samples, dma_interval;
      double ratio = 1.0, limit_ratio = 1.0;
      float sampleRate, ref_ratio, curr_ratio;
      float ref_us = 0;
      float dest_us = 0;
      float sample_us = 0;

      dma_buffer_samples = app_bt_stream_get_dma_buffer_samples() / 2;
      dma_interval = A2DP_AUDIO_SYNC_FIX_DIFF_INTERVAL;

      ref_ratio = a2dp_audio_context.init_factor_reference;
      curr_ratio = a2dp_audio_context.output_cfg.factor_reference;

      samples = dma_interval * dma_buffer_samples;
      sampleRate = lastframe_info->stream_info.sample_rate * ref_ratio;

      sample_us = 1e6 / sampleRate;
      ref_us = sample_us * (float)samples;
      dest_us = sample_us * (float)(lastframe_info->frame_samples * diff_mtu);
      ratio = ref_us / (ref_us - dest_us) * curr_ratio;

      if (ratio > (double)(ref_ratio + A2DP_AUDIO_SYNC_FIX_DIFF_FAST_LIMIT)) {
        limit_ratio = ref_ratio + A2DP_AUDIO_SYNC_FIX_DIFF_FAST_LIMIT;
      } else if (ratio <
                 double(ref_ratio + A2DP_AUDIO_SYNC_FIX_DIFF_SLOW_LIMIT)) {
        limit_ratio = ref_ratio + A2DP_AUDIO_SYNC_FIX_DIFF_SLOW_LIMIT;
      } else {
        limit_ratio = ratio;
      }
      TRACE_A2DP_DECODER_I(
          "[SYNC] sampleRate:%d ref_ratio:%d samples:%d %d->%d",
          (int32_t)sampleRate, (int32_t)(ref_ratio * 10000000), samples,
          (int32_t)ref_us, (int32_t)dest_us);
      need_tune = true;
#if defined(IBRT)
      if (!app_tws_ibrt_audio_sync_tune_onprocess() &&
          !a2dp_audio_sync_tune_onprocess() &&
#else
      if (!a2dp_audio_sync_tune_onprocess() &&
#endif
          need_tune) {
        if (!a2dp_audio_sync_tune((float)limit_ratio)) {
          a2dp_audio_sync_fix_diff_start(dma_interval);
#ifdef A2DP_AUDIO_SYNC_FIX_DIFF_INTERVA_PRINT_FLOAT
          TRACE_A2DP_DECODER_I("[SYNC] tune ratio %f  mut:%5.3f->%d",
                               limit_ratio,
                               (double)a2dp_audio_context.average_packet_mut,
                               a2dp_audio_context.dest_packet_mut);
#else
          TRACE_A2DP_DECODER_I(
              "[SYNC] tune ratio %d  mut:%d->%d",
              (int32_t)(limit_ratio * 10000000),
              (int32_t)(a2dp_audio_context.average_packet_mut + 0.5f),
              a2dp_audio_context.dest_packet_mut);
#endif
        } else {
          TRACE_A2DP_DECODER_I("[SYNC] tune ratio busy");
        }
      }
    } else {
      TRACE_A2DP_DECODER_I(
          "[SYNC] avg_mut:%d dest_mtu:%d",
          (int32_t)(a2dp_audio_context.average_packet_mut + 0.5f),
          a2dp_audio_context.dest_packet_mut);
    }
  } else {
  }
exit:
  audio_sync->tick++;
  return 0;
}
#else
int a2dp_audio_sync_handler(uint8_t *buffer, uint32_t buffer_bytes) {
  A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info = NULL;
  A2DP_AUDIO_SYNC_T *audio_sync = &a2dp_audio_context.audio_sync;
  float dest_pid_result = .0f;
  float diff_mtu = 0;
  int32_t frame_mtu = 0;
  int32_t total_mtu = 0;
  float diff_factor = 0;

#if defined(IBRT)
  if (!app_tws_ibrt_mobile_link_connected()) {
    return -1;
  }
#endif
  if (a2dp_audio_internal_lastframe_info_ptr_get(&lastframe_info) < 0) {
    return -1;
  }

  if (audio_sync->tick++ % A2DP_AUDIO_SYNC_INTERVAL == 0) {
    list_t *list = a2dp_audio_context.audio_datapath.input_raw_packet_list;
    A2DP_AUDIO_SYNC_PID_T *pid = &audio_sync->pid;
    // valid limter 0x80000
    if (audio_sync->cnt < 0x80000) {
      audio_sync->cnt += A2DP_AUDIO_SYNC_INTERVAL;
    }
    frame_mtu = lastframe_info->stream_info.frame_samples /
                lastframe_info->frame_samples;
    total_mtu = audio_sync->cnt * frame_mtu;
    diff_mtu = a2dp_audio_context.average_packet_mut -
               (float)a2dp_audio_context.dest_packet_mut;
#if 1
    TRACE_A2DP_DECODER_I(
        "[SYNC] sample:%d/%d diff:%d/%d/%d/%d curr:%d",
        lastframe_info->frame_samples,
        lastframe_info->stream_info.frame_samples, (int32_t)(diff_mtu + 0.5f),
        (int32_t)(a2dp_audio_context.average_packet_mut + 0.5f),
        a2dp_audio_context.dest_packet_mut, total_mtu,
        a2dp_audio_list_length(list) + get_in_cp_frame_cnt());
#else
    TRACE_A2DP_DECODER_I(
        "[SYNC] diff:%10.9f/%10.9f frame_mut:%d dest:%d total:%d curr:%d",
        (double)diff_mtu, (double)a2dp_audio_context.average_packet_mut,
        frame_mtu, a2dp_audio_context.dest_packet_mut, total_mtu,
        a2dp_audio_list_length(list));
    TRACE_A2DP_DECODER_I("[SYNC] try tune:%d, %10.9f %10.9f",
                         diff_mtu != 0.f && audio_sync->tick != 1
                             ? ABS(diff_mtu) > ((float)frame_mtu * 0.2f)
                             : 0,
                         (double)ABS(diff_mtu),
                         (double)((float)frame_mtu * 0.2f));
#endif
    // TRACE(2,"audio_sync tune %d/%d tick",
    // app_tws_ibrt_audio_sync_tune_onprocess(),
    // a2dp_audio_sync_tune_onprocess(), audio_sync->tick);

#if defined(IBRT)
    if ((!app_tws_ibrt_audio_sync_tune_onprocess() &&
         !a2dp_audio_sync_tune_onprocess()) &&
#else
    if (!a2dp_audio_sync_tune_onprocess() &&
#endif
        diff_mtu != 0.f && audio_sync->tick != 1 &&
        ((ABS(diff_mtu) > ((float)frame_mtu * 0.25f) && diff_mtu > 0) ||
         (ABS(diff_mtu) > ((float)frame_mtu * 0.1f) && diff_mtu < 0))) {
      diff_factor = diff_mtu / a2dp_audio_context.average_packet_mut;
      if (a2dp_audio_sync_pid_calc(pid, diff_factor)) {
        dest_pid_result =
            a2dp_audio_context.output_cfg.factor_reference + pid->result;

        if (dest_pid_result > (A2DP_AUDIO_SYNC_FACTOR_REFERENCE +
                               A2DP_AUDIO_SYNC_FACTOR_FAST_LIMIT)) {
          dest_pid_result = A2DP_AUDIO_SYNC_FACTOR_REFERENCE +
                            A2DP_AUDIO_SYNC_FACTOR_FAST_LIMIT;
        } else if (dest_pid_result < (A2DP_AUDIO_SYNC_FACTOR_REFERENCE +
                                      A2DP_AUDIO_SYNC_FACTOR_SLOW_LIMIT)) {
          dest_pid_result = A2DP_AUDIO_SYNC_FACTOR_REFERENCE +
                            A2DP_AUDIO_SYNC_FACTOR_SLOW_LIMIT;
        }

        if (a2dp_audio_context.output_cfg.factor_reference != dest_pid_result) {
          if (!a2dp_audio_sync_tune(dest_pid_result)) {
            audio_sync->cnt = 0;
          }
          TRACE_A2DP_DECODER_I(
              "[SYNC] tune diff_factor:%10.9f pid:%10.9f tune:%10.9f",
              (double)diff_factor, (double)pid->result,
              (double)dest_pid_result);
        } else {
          TRACE_A2DP_DECODER_I("[SYNC] tune skip same");
        }
      }
    } else {
      /*
                  TRACE_A2DP_DECODER_I("[SYNC] tune busy skip proc:%d/%d mtu:%d
         tick:%d >0:%d <0:%d tick:%d", app_tws_ibrt_audio_sync_tune_onprocess(),
                                                      a2dp_audio_sync_tune_onprocess(),
                                                      diff_mtu != 0.f,
                                                      audio_sync->tick != 1,
                                                      (ABS(diff_mtu) >
         ((float)frame_mtu * 0.25f) && diff_mtu > 0), (ABS(diff_mtu) >
         ((float)frame_mtu * 0.1f) && diff_mtu < 0), audio_sync->tick);
      */
    }
  } else {
    bool need_tune = false;
    if (lastframe_info->undecode_min_frames * 10 <=
        a2dp_audio_context.dest_packet_mut * 10 / 3) {
      dest_pid_result = a2dp_audio_context.init_factor_reference +
                        A2DP_AUDIO_SYNC_FACTOR_NEED_FAST_CACHE;
      need_tune = true;
    } else if (lastframe_info->undecode_min_frames * 10 <=
               a2dp_audio_context.dest_packet_mut * 20 / 3) {
      dest_pid_result = a2dp_audio_context.init_factor_reference +
                        A2DP_AUDIO_SYNC_FACTOR_SLOW_LIMIT;
      need_tune = true;
    }
#if defined(IBRT)
    if (!app_tws_ibrt_audio_sync_tune_onprocess() &&
        !a2dp_audio_sync_tune_onprocess() &&
#else
    if (!a2dp_audio_sync_tune_onprocess() &&
#endif

        need_tune) {
      if (a2dp_audio_context.output_cfg.factor_reference != dest_pid_result) {
        a2dp_audio_sync_reset_data();
        a2dp_audio_sync_tune(dest_pid_result);
        TRACE_A2DP_DECODER_I("[SYNC] tune ratio force slow %d/%d->%d",
                             lastframe_info->undecode_min_frames,
                             lastframe_info->undecode_max_frames,
                             a2dp_audio_context.dest_packet_mut);
      }
    }
  }
  return 0;
}
#endif

#if A2DP_DECODER_HISTORY_SEQ_SAVE
static int a2dp_audio_reset_history_seq(void) {
  a2dp_audio_status_mutex_lock();
  for (uint8_t i = 0; i < A2DP_DECODER_HISTORY_SEQ_SAVE; i++) {
    a2dp_audio_context.historySeq[i] = 0;
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
    a2dp_audio_context.historyLoctime[i] = 0;
#endif
#ifdef A2DP_DECODER_HISTORY_CHECK_SUM_SAVE
    a2dp_audio_context.historyChecksum[i] = 0;
#endif
  }
  a2dp_audio_context.historySeq_idx = 0;
  a2dp_audio_status_mutex_unlock();

  return 0;
}

static int a2dp_audio_save_history_seq(btif_media_header_t *header,
                                       unsigned char *buf, unsigned int len) {
  uint16_t historySeqPre = 0;
  uint8_t historySeq_idx = 0;

  a2dp_audio_status_mutex_lock();
  historySeq_idx = a2dp_audio_context.historySeq_idx;
  if (historySeq_idx) {
    historySeq_idx = (historySeq_idx - 1) % A2DP_DECODER_HISTORY_SEQ_SAVE;
    historySeqPre = a2dp_audio_context.historySeq[historySeq_idx];
    if ((header->sequenceNumber - historySeqPre) != 1) {
      TRACE_A2DP_DECODER_W("[INPUT] SEQ ERR %d/%d", historySeqPre,
                           header->sequenceNumber);
      a2dp_audio_show_history_seq();
    }
  }
  historySeq_idx =
      a2dp_audio_context.historySeq_idx % A2DP_DECODER_HISTORY_SEQ_SAVE;
  a2dp_audio_context.historySeq[historySeq_idx] = header->sequenceNumber;
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
  a2dp_audio_context.historyLoctime[historySeq_idx] = hal_fast_sys_timer_get();
#endif
#ifdef A2DP_DECODER_HISTORY_CHECK_SUM_SAVE
  a2dp_audio_context.historyChecksum[historySeq_idx] = crc32(0, buf, len);
#endif
  a2dp_audio_context.historySeq_idx++;
  a2dp_audio_status_mutex_unlock();

  return 0;
}

int a2dp_audio_show_history_seq(void) {
  uint8_t i = 0, j = 1;
  uint16_t reordHistorySeq[A2DP_DECODER_HISTORY_SEQ_SAVE];
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
  int32_t diff_max_idx = 0;
  int32_t diff_max_ms = 0;
  int64_t diff_avg_ms = 0;
  int32_t diff_avg_cnt = 0;
  uint32_t historyLoctime[A2DP_DECODER_HISTORY_SEQ_SAVE];
#endif
#ifdef A2DP_DECODER_HISTORY_CHECK_SUM_SAVE
  uint32_t historyChecksum[A2DP_DECODER_HISTORY_SEQ_SAVE];
#endif

  a2dp_audio_status_mutex_lock();
  for (i = 0; i < A2DP_DECODER_HISTORY_SEQ_SAVE; i++) {
    reordHistorySeq[i] = a2dp_audio_context.historySeq[i];
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
    historyLoctime[i] = a2dp_audio_context.historyLoctime[i];
#endif
#ifdef A2DP_DECODER_HISTORY_CHECK_SUM_SAVE
    historyChecksum[i] = a2dp_audio_context.historyChecksum[i];
#endif
  }

  for (i = 0; i < A2DP_DECODER_HISTORY_SEQ_SAVE - 1; i++) {
    for (j = 0; j < A2DP_DECODER_HISTORY_SEQ_SAVE - 1 - i; j++) {
      if (reordHistorySeq[j] > reordHistorySeq[j + 1]) {
        uint16_t temp_seq = reordHistorySeq[j];
        reordHistorySeq[j] = reordHistorySeq[j + 1];
        reordHistorySeq[j + 1] = temp_seq;
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
        uint32_t temp_Loctime = historyLoctime[j];
        historyLoctime[j] = historyLoctime[j + 1];
        historyLoctime[j + 1] = temp_Loctime;
#endif
#ifdef A2DP_DECODER_HISTORY_CHECK_SUM_SAVE
        uint32_t temp_Checksum = historyChecksum[j];
        historyChecksum[j] = historyChecksum[j + 1];
        historyChecksum[j + 1] = temp_Checksum;
#endif
      }
    }
  }
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
  for (i = 0, j = 1; i < A2DP_DECODER_HISTORY_SEQ_SAVE - 1; i++, j++) {
    int32_t tmp_ms = historyLoctime[j] - historyLoctime[i];
    diff_avg_ms += tmp_ms;
    diff_avg_cnt++;
    if (tmp_ms > diff_max_ms) {
      diff_max_ms = tmp_ms;
      diff_max_idx = i;
    }
    if (tmp_ms > (int32_t)MS_TO_FAST_TICKS(50)) {
      TRACE_A2DP_DECODER_I("[INPUT] > 30ms seq:%d diff:%d :%d / %d",
                           reordHistorySeq[i], FAST_TICKS_TO_MS(tmp_ms),
                           historyLoctime[i], historyLoctime[j]);
    }
  }
  diff_avg_ms /= diff_avg_cnt;
  TRACE_A2DP_DECODER_I("[INPUT] max_diff:%dms idx:%d avg:%dus",
                       FAST_TICKS_TO_MS(diff_max_ms), diff_max_idx,
                       FAST_TICKS_TO_US(diff_avg_ms));
#endif
//    DUMP16("%d ", reordHistorySeq, A2DP_DECODER_HISTORY_SEQ_SAVE);
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
  DUMP32("%x ", historyLoctime, A2DP_DECODER_HISTORY_SEQ_SAVE);
#endif
#ifdef A2DP_DECODER_HISTORY_CHECK_SUM_SAVE
  DUMP32("%x ", historyChecksum, A2DP_DECODER_HISTORY_SEQ_SAVE);
#endif
  a2dp_audio_status_mutex_unlock();

  return 0;
}

#endif

/*
1,  2^1
3,  2^2
7,  2^3
15, 2^4
31, 2^5
*/
#define AUDIO_ALPHA_PRAMS_1 (3)
#define AUDIO_ALPHA_PRAMS_2 (4)

static inline float a2dp_audio_alpha_filter(float y, float x) {
  if (y) {
    y = ((AUDIO_ALPHA_PRAMS_1 * y) + x) / AUDIO_ALPHA_PRAMS_2;
  } else {
    y = x;
  }
  return y;
}

static void inline a2dp_audio_convert_16bit_to_24bit(int32_t *out, int16_t *in,
                                                     int len) {
  for (int i = len - 1; i >= 0; i--) {
    out[i] = ((int32_t)in[i] << 8);
  }
}

static void inline a2dp_audio_channel_select(
    A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel, uint8_t *buffer,
    uint32_t buffer_bytes) {
  uint32_t samples;
  uint32_t i;

  ASSERT_A2DP_DECODER(a2dp_audio_context.output_cfg.num_channels == 2,
                      "%s num_channels:%d", __func__,
                      a2dp_audio_context.output_cfg.num_channels);

  if (a2dp_audio_context.output_cfg.bits_depth == 24) {
    int32_t *buf_l_p = (int32_t *)buffer;
    int32_t *buf_r_p = (int32_t *)buffer + 1;

    samples = buffer_bytes / 4 / 2;
    switch (chnl_sel) {
    case A2DP_AUDIO_CHANNEL_SELECT_LRMERGE:
      for (i = 0; i < samples; i++, buf_l_p += 2, buf_r_p += 2) {
        int32_t tmp_sample = (*buf_l_p + *buf_r_p) >> 1;
        *buf_l_p = tmp_sample;
        *buf_r_p = tmp_sample;
      }
      break;
    case A2DP_AUDIO_CHANNEL_SELECT_LCHNL:
      for (i = 0; i < samples; i++, buf_l_p += 2, buf_r_p += 2) {
        *buf_r_p = *buf_l_p;
      }
      break;
    case A2DP_AUDIO_CHANNEL_SELECT_RCHNL:
      for (i = 0; i < samples; i++, buf_l_p += 2, buf_r_p += 2) {
        *buf_l_p = *buf_r_p;
      }
      break;
    case A2DP_AUDIO_CHANNEL_SELECT_STEREO:
    default:
      break;
    }
  } else {
    int16_t *buf_l_p = (int16_t *)buffer;
    int16_t *buf_r_p = (int16_t *)buffer + 1;

    samples = buffer_bytes / 2 / 2;
    switch (chnl_sel) {
    case A2DP_AUDIO_CHANNEL_SELECT_LRMERGE:
      for (i = 0; i < samples; i++, buf_l_p += 2, buf_r_p += 2) {
        int16_t tmp_sample = ((int32_t)*buf_l_p + (int32_t)*buf_r_p) >> 1;
        *buf_l_p = tmp_sample;
        *buf_r_p = tmp_sample;
      }
      break;
    case A2DP_AUDIO_CHANNEL_SELECT_LCHNL:
      for (i = 0; i < samples; i++, buf_l_p += 2, buf_r_p += 2) {
        *buf_r_p = *buf_l_p;
      }
      break;
    case A2DP_AUDIO_CHANNEL_SELECT_RCHNL:
      for (i = 0; i < samples; i++, buf_l_p += 2, buf_r_p += 2) {
        *buf_l_p = *buf_r_p;
      }
      break;
    case A2DP_AUDIO_CHANNEL_SELECT_STEREO:
    default:
      break;
    }
  }
}

#ifdef A2DP_CP_ACCEL
extern "C" bool is_cp_need_reset(void);
extern uint32_t app_bt_stream_get_dma_buffer_samples(void);

static uint32_t get_cp_frame_mtus(A2DP_AUDIO_LASTFRAME_INFO_T *info) {
  uint32_t cp_frame_mtus = app_bt_stream_get_dma_buffer_samples() / 2;
  if (cp_frame_mtus % (info->frame_samples)) {
    cp_frame_mtus = cp_frame_mtus / (info->frame_samples) + 1;
  } else {
    cp_frame_mtus = cp_frame_mtus / (info->frame_samples);
  }

  TRACE_A2DP_DECODER_I("[CP] cp_frame_mtus:%d", cp_frame_mtus);

  return cp_frame_mtus;
}

#else
bool is_cp_need_reset(void) { return false; }

#endif

#define A2DP_AUDIO_SYSFREQ_BOOST_RESUME_CNT (20)
uint32_t a2dp_audio_sysfreq_cnt = UINT32_MAX;
uint32_t a2dp_audio_sysfreq_dest_boost_cnt = 0;
APP_SYSFREQ_FREQ_T a2dp_audio_sysfreq_normalfreq = APP_SYSFREQ_52M;

int a2dp_audio_sysfreq_boost_init(uint32_t normalfreq) {
  a2dp_audio_sysfreq_cnt = UINT32_MAX;
  a2dp_audio_sysfreq_dest_boost_cnt = 0;
  a2dp_audio_sysfreq_normalfreq = (APP_SYSFREQ_FREQ_T)normalfreq;
  TRACE_A2DP_DECODER_I("[BOOST] freq:%d", normalfreq);
  return 0;
}

int a2dp_audio_sysfreq_boost_start(uint32_t boost_cnt) {
  enum APP_SYSFREQ_FREQ_T sysfreq = APP_SYSFREQ_104M;

  a2dp_audio_sysfreq_cnt = 0;
  a2dp_audio_sysfreq_dest_boost_cnt = boost_cnt;
  if (a2dp_audio_sysfreq_normalfreq >= APP_SYSFREQ_52M) {
    sysfreq = APP_SYSFREQ_104M;
  } else {
    sysfreq = APP_SYSFREQ_52M;
  }
  TRACE_A2DP_DECODER_I("[BOOST] freq:%d cnt:%d", sysfreq, boost_cnt);
  app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, sysfreq);
  return 0;
}

static int a2dp_audio_sysfreq_boost_porc(void) {
  if (a2dp_audio_sysfreq_cnt == UINT32_MAX) {
    // do nothing
  } else if (a2dp_audio_sysfreq_cnt >= a2dp_audio_sysfreq_dest_boost_cnt) {
    a2dp_audio_sysfreq_cnt = UINT32_MAX;
    TRACE_A2DP_DECODER_I("[BOOST] freq:%d", a2dp_audio_sysfreq_normalfreq);
    app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, a2dp_audio_sysfreq_normalfreq);
  } else {
    a2dp_audio_sysfreq_cnt++;
  }
  return 0;
}

int a2dp_audio_sysfreq_boost_running(void) {
  return a2dp_audio_sysfreq_cnt == UINT32_MAX ? 0 : 1;
}

int a2dp_audio_store_packet_checker_start(void) {
  store_packet_history_loctime = 0;
  return 0;
}

int a2dp_audio_store_packet_checker(btif_media_header_t *header) {
  bool show_info = false;
  uint32_t fast_sys_tick = hal_fast_sys_timer_get();
  int32_t tmp_ms = 0;

  if (store_packet_history_loctime) {
    tmp_ms = fast_sys_tick - store_packet_history_loctime;
    if (tmp_ms > (int32_t)MS_TO_FAST_TICKS(50)) {
      show_info = true;
    }
  }

  if (show_info) {
    // TRACE_A2DP_DECODER_I("[INPUT] >50ms seq:%d diff:%d",
    // header->sequenceNumber, FAST_TICKS_TO_MS(tmp_ms));
    a2dp_decoder_info_checker();
  }

  store_packet_history_loctime = fast_sys_tick;

  if (a2dp_audio_context.historySeq_idx &&
      ((a2dp_audio_context.historySeq_idx - 1) %
           A2DP_DECODER_HISTORY_SEQ_SAVE ==
       0)) {
    a2dp_audio_show_history_seq();
  }

  return 0;
}
//#define DECODER_ERR_TEST
int a2dp_audio_store_packet(btif_media_header_t *header, unsigned char *buf,
                            unsigned int len) {
  int nRet = A2DP_DECODER_NO_ERROR;

  if (a2dp_audio_get_status() ==
      A2DP_AUDIO_DECODER_STATUS_NULL) { // if mem deinit , drop data
    return nRet;
  }
  a2dp_audio_status_mutex_lock();

#ifdef DECODER_ERR_TEST
  static uint16_t cnt = 0;
  static uint16_t limit = 500;
  cnt++;
  if (cnt % limit == 0) {
    cnt = 0;
    limit = rand() % 500;
    uint16_t len2 = rand() % len;
    memset(buf + len - len2, 0, len2);
    TRACE_A2DP_DECODER_I("[INPUT] Fill Err!!!! seq:%d", header->sequenceNumber);
  }
#endif
#if A2DP_DECODER_HISTORY_SEQ_SAVE
  a2dp_audio_save_history_seq(header, buf, len);
#endif
  a2dp_audio_set_store_packet_status(
      A2DP_AUDIO_DECODER_STORE_PACKET_STATUS_BUSY);
  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_store_packet_checker(header);
    if (a2dp_audio_context.need_detect_first_packet) {
      a2dp_audio_context.need_detect_first_packet = false;
      a2dp_audio_context.audio_decoder.audio_decoder_preparse_packet(header,
                                                                     buf, len);
    }

    if (a2dp_audio_detect_next_packet_callback) {
      a2dp_audio_detect_next_packet_callback(header, buf, len);
    }

    nRet = a2dp_audio_context.audio_decoder.audio_decoder_store_packet(
        header, buf, len);
#if defined(IBRT)
    if (is_cp_need_reset()) {
      TRACE_A2DP_DECODER_I("[CP] find cp error need restart");
      app_ibrt_if_force_audio_retrigger();
    }
    if (nRet == A2DP_DECODER_MTU_LIMTER_ERROR) {
      if (app_tws_ibrt_mobile_link_connected()) {
        // try again
        // a2dp_audio_semaphore_wait(A2DP_AUDIO_WAIT_TIMEOUT_MS);
        nRet = a2dp_audio_context.audio_decoder.audio_decoder_store_packet(
            header, buf, len);
      }
      if (nRet == A2DP_DECODER_MTU_LIMTER_ERROR) {
        int dest_discards_samples = 0;
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        if (p_ibrt_ctrl == NULL) {
          return A2DP_DECODER_NOT_SUPPORT;
        }
        if (app_tws_ibrt_mobile_link_connected()) {
          bt_syn_trig_checker(p_ibrt_ctrl->mobile_conhandle);
        } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
          bt_syn_trig_checker(p_ibrt_ctrl->ibrt_conhandle);
        }
        dest_discards_samples = app_bt_stream_get_dma_buffer_samples() / 2;
        a2dp_audio_discards_samples(dest_discards_samples * 2);
        a2dp_audio_context.audio_decoder.audio_decoder_store_packet(header, buf,
                                                                    len);
        TRACE_A2DP_DECODER_I("[INPUT] MTU_LIMTER so discards_packet");
      }
    }
#else
    if (is_cp_need_reset()) {
      TRACE_A2DP_DECODER_I("[CP] find cp error need restart");
      app_audio_decode_err_force_trigger();
    }

    if (nRet == A2DP_DECODER_MTU_LIMTER_ERROR) {
      a2dp_audio_synchronize_dest_packet_mut(
          a2dp_audio_context.dest_packet_mut);
      a2dp_audio_context.audio_decoder.audio_decoder_store_packet(header, buf,
                                                                  len);
      TRACE_A2DP_DECODER_W("[INPUT] MTU_LIMTER so discards_packet");
    }
#endif
  } else {
    TRACE(2, "[INPUT] skip packet status:%d", a2dp_audio_get_status());
  }

  if (a2dp_audio_store_packet_callback) {
    a2dp_audio_store_packet_callback(header, buf, len);
  }

  a2dp_audio_set_store_packet_status(
      A2DP_AUDIO_DECODER_STORE_PACKET_STATUS_IDLE);
  a2dp_audio_status_mutex_unlock();
  return 0;
}

uint32_t a2dp_audio_playback_handler(uint8_t *buffer, uint32_t buffer_bytes) {
  uint32_t len = buffer_bytes;
  int nRet = A2DP_DECODER_NO_ERROR;
  A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info = NULL;
  list_t *list = a2dp_audio_context.audio_datapath.input_raw_packet_list;

  a2dp_audio_set_playback_status(A2DP_AUDIO_DECODER_PLAYBACK_STATUS_BUSY);
  if (a2dp_audio_get_status() != A2DP_AUDIO_DECODER_STATUS_START) {
    TRACE_A2DP_DECODER_W("[PLAYBACK] skip handler status:%d",
                         a2dp_audio_get_status());
    goto exit;
  }

  a2dp_audio_sysfreq_boost_porc();
  if (a2dp_audio_context.average_packet_mut == 0) {
    A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
    a2dp_audio_decoder_headframe_info_get(&headframe_info);
    a2dp_audio_context.average_packet_mut = a2dp_audio_list_length(list);
    TRACE_A2DP_DECODER_I(
        "[PLAYBACK] init average_packet_mut:%d seq:%d",
        (uint16_t)(a2dp_audio_context.average_packet_mut + 0.5f),
        headframe_info.sequenceNumber);
  } else {
    if (!a2dp_audio_refill_packet()) {
      uint16_t packet_mut = 0;
      if (!a2dp_audio_internal_lastframe_info_ptr_get(&lastframe_info)) {
        packet_mut = a2dp_audio_list_length(list) + get_in_cp_frame_cnt() +
                     get_in_cp_frame_delay() * (lastframe_info->frame_samples /
                                                lastframe_info->list_samples);
        a2dp_audio_context.average_packet_mut = a2dp_audio_alpha_filter(
            (float)a2dp_audio_context.average_packet_mut, (float)packet_mut);
        a2dp_audio_sync_handler(buffer, buffer_bytes);
      }
    }
  }
#if defined(A2DP_AUDIO_REFILL_AFTER_NO_CACHE)
  if (a2dp_audio_context.skip_frame_cnt_after_no_cache) {
#if defined(A2DP_CP_ACCEL)
    uint32_t cp_delay_mtus = get_in_cp_frame_delay();
    cp_delay_mtus *= get_cp_frame_mtus(&a2dp_audio_lastframe_info);
    if (a2dp_audio_list_length(list) >=
        (a2dp_audio_context.dest_packet_mut - cp_delay_mtus)) {
      a2dp_audio_context.skip_frame_cnt_after_no_cache = 0;
    }
#else
    if (a2dp_audio_list_length(list) >= a2dp_audio_context.dest_packet_mut) {
      a2dp_audio_context.skip_frame_cnt_after_no_cache = 0;
    }
#endif

    memset(buffer, 0, buffer_bytes);
    TRACE_A2DP_DECODER_I("[PLAYBACK] decode refill skip_cnt:%d, list:%d",
                         a2dp_audio_context.skip_frame_cnt_after_no_cache,
                         a2dp_audio_list_length(list));
    a2dp_decoder_info_checker();
    if (a2dp_audio_context.skip_frame_cnt_after_no_cache > 0) {
      a2dp_audio_context.skip_frame_cnt_after_no_cache--;
    } else {
      a2dp_audio_context.mute_frame_cnt_after_no_cache =
          A2DP_AUDIO_MUTE_FRAME_CNT_AFTER_NO_CACHE;
    }
  } else
#endif
  {
    if (a2dp_audio_context.output_cfg.bits_depth == 24 &&
        16 == bt_sbc_player_get_sample_bit()) {

      len = len / (sizeof(int32_t) / sizeof(int16_t));

      nRet = a2dp_audio_context.audio_decoder.audio_decoder_decode_frame(buffer,
                                                                         len);
      if (nRet < 0 || a2dp_audio_context.mute_frame_cnt_after_no_cache) {
        TRACE_A2DP_DECODER_I("[PLAYBACK] decode failed nRet=%d mute_cnt:%d",
                             nRet,
                             a2dp_audio_context.mute_frame_cnt_after_no_cache);
        a2dp_decoder_info_checker();
        // mute frame
        memset(buffer, 0, len);
      }
      a2dp_audio_convert_16bit_to_24bit((int32_t *)buffer, (int16_t *)buffer,
                                        len / sizeof(int16_t));

      // Restore len to 24-bit sample buffer length
      len = len * (sizeof(int32_t) / sizeof(int16_t));

    } else if (a2dp_audio_context.output_cfg.bits_depth ==
               a2dp_audio_context.audio_decoder.stream_info.bits_depth) {

      nRet = a2dp_audio_context.audio_decoder.audio_decoder_decode_frame(buffer,
                                                                         len);
      if (nRet < 0 || a2dp_audio_context.mute_frame_cnt_after_no_cache) {
        // mute frame
        TRACE_A2DP_DECODER_I("[PLAYBACK] decode failed nRet=%d mute_cnt:%d",
                             nRet,
                             a2dp_audio_context.mute_frame_cnt_after_no_cache);
        a2dp_decoder_info_checker();
        memset(buffer, 0, len);
      }
    }
    a2dp_audio_channel_select(a2dp_audio_context.chnl_sel, buffer,
                              buffer_bytes);
  }
  a2dp_audio_semaphore_release();

  if (nRet == A2DP_DECODER_CACHE_UNDERFLOW_ERROR) {
    if (a2dp_audio_internal_lastframe_info_ptr_get(&lastframe_info) < 0) {
      goto exit;
    }
    TRACE(2, "CACHE_UNDERFLOW lastseq:%d ftick:%d",
          lastframe_info->sequenceNumber, hal_fast_sys_timer_get());
    a2dp_audio_show_history_seq();
    uint32_t mute_frames = A2DP_AUDIO_MUTE_FRAME_CNT_AFTER_NO_CACHE;
    uint32_t skip_frames =
        A2DP_AUDIO_SKIP_FRAME_LIMIT_AFTER_NO_CACHE - get_in_cp_frame_delay();
    a2dp_audio_context.mute_frame_cnt_after_no_cache =
        (uint32_t)((float)mute_frames * a2dp_audio_latency_factor_get());
    a2dp_audio_context.skip_frame_cnt_after_no_cache =
        (uint32_t)((float)skip_frames * a2dp_audio_latency_factor_get());

    a2dp_audio_context.average_packet_mut = 0;
    a2dp_audio_sync_reset_data();
#if defined(IBRT)

    a2dp_audio_sync_tune_sample_rate(
        app_tws_ibrt_audio_sync_config_factor_reference_get());
#else
    a2dp_audio_sync_tune_sample_rate(a2dp_audio_context.init_factor_reference);
#endif
    a2dp_decoder_info_checker();
  } else {
    if (a2dp_audio_context.mute_frame_cnt_after_no_cache > 0) {
      a2dp_audio_context.mute_frame_cnt_after_no_cache--;
      a2dp_audio_context.average_packet_mut = 0;
      if (a2dp_audio_context.mute_frame_cnt_after_no_cache >= 1) {
        a2dp_audio_synchronize_dest_packet_mut(
            a2dp_audio_context.dest_packet_mut);
      }
    }
  }
  if (!a2dp_audio_internal_lastframe_info_ptr_get(&lastframe_info)) {
    lastframe_info->stream_info.factor_reference =
        a2dp_audio_context.output_cfg.factor_reference;
    lastframe_info->average_frames =
        (uint32_t)(a2dp_audio_context.average_packet_mut + 0.5f);
  }
exit:
  a2dp_audio_set_playback_status(A2DP_AUDIO_DECODER_PLAYBACK_STATUS_IDLE);
#if defined(IBRT)
  if (nRet == A2DP_DECODER_CACHE_UNDERFLOW_ERROR) {
#if defined(A2DP_AUDIO_UNDERFLOW_CAUSE_AUDIO_RETRIGGER)
    bool force_audio_retrigger = true;
#else
    bool force_audio_retrigger = false;
#endif
    if (a2dp_audio_latency_factor_get() == A2DP_AUDIO_LATENCY_LOW_FACTOR &&
        app_tws_ibrt_mobile_link_connected()) {
      a2dp_audio_latency_factor_sethigh();
      if (app_tws_ibrt_tws_link_connected() &&
          app_ibrt_ui_is_profile_exchanged()) {
        float latency_factor = a2dp_audio_latency_factor_get();
        tws_ctrl_send_cmd(APP_TWS_CMD_SET_LATENCYFACTOR,
                          (uint8_t *)&latency_factor, sizeof(latency_factor));
        force_audio_retrigger = true;
      }
    }
    if (force_audio_retrigger && !a2dp_audio_context.underflow_onporcess) {
      a2dp_audio_context.underflow_onporcess = true;
      app_ibrt_if_force_audio_retrigger();
    }
  }
#else
  if (nRet == A2DP_DECODER_CACHE_UNDERFLOW_ERROR) {
    app_audio_decode_err_force_trigger();
  }
#endif
  return len;
}

static void a2dp_audio_packet_free(void *packet) {
  if (a2dp_audio_context.audio_decoder.audio_decoder_packet_free) {
    a2dp_audio_context.audio_decoder.audio_decoder_packet_free(packet);
  } else {
    a2dp_audio_heap_free(packet);
  }
}

void a2dp_audio_clear_input_raw_packet_list(void) {
  // just clean the packet list to start receive ai data again
  if (a2dp_audio_context.audio_datapath.input_raw_packet_list)
    a2dp_audio_list_clear(
        a2dp_audio_context.audio_datapath.input_raw_packet_list);
}

int a2dp_audio_init(uint32_t sysfreq, A2DP_AUDIO_CODEC_TYPE codec_type,
                    A2DP_AUDIO_OUTPUT_CONFIG_T *config,
                    A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel,
                    uint16_t dest_packet_mut) {
  uint8_t *heap_buff = NULL;
  uint32_t heap_size = 0;
  double ratio = 0;

  A2DP_AUDIO_OUTPUT_CONFIG_T decoder_output_config;
  TRACE_A2DP_DECODER_I("[INIT] freq:%d codec:%d chnl:%d", sysfreq, codec_type,
                       chnl_sel);
  TRACE_A2DP_DECODER_I("[INIT] out:%d-%d-%d smp:%d dest:%d",
                       config->sample_rate, config->num_channels,
                       config->bits_depth, config->frame_samples,
                       dest_packet_mut);
  a2dp_audio_sysfreq_boost_init(sysfreq);
  a2dp_audio_sysfreq_boost_start(A2DP_AUDIO_SYSFREQ_BOOST_RESUME_CNT);
  a2dp_audio_semaphore_init();
  a2dp_audio_buffer_mutex_init();
  a2dp_audio_status_mutex_init();

  a2dp_audio_status_mutex_lock();

  a2dp_audio_detect_next_packet_callback_register(NULL);
  a2dp_audio_detect_store_packet_callback_register(NULL);

#if defined(A2DP_LDAC_ON)
  if (bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
    heap_size = A2DP_AUDIO_LDAC_MEMPOOL_SIZE;
  } else
#endif
  {
    heap_size = A2DP_AUDIO_MEMPOOL_SIZE;
  }
  app_audio_mempool_get_buff(&heap_buff, heap_size);
  ASSERT_A2DP_DECODER(heap_buff, "%s size:%d", __func__, heap_size);
  a2dp_audio_heap_init(heap_buff, heap_size);

  memset(&a2dp_audio_lastframe_info, 0, sizeof(A2DP_AUDIO_LASTFRAME_INFO_T));

  a2dp_audio_context.audio_datapath.input_raw_packet_list = a2dp_audio_list_new(
      a2dp_audio_packet_free, (list_mempool_zmalloc)a2dp_audio_heap_cmalloc,
      (list_mempool_free)a2dp_audio_heap_free);

  a2dp_audio_context.audio_datapath.output_pcm_packet_list =
      a2dp_audio_list_new(a2dp_audio_packet_free,
                          (list_mempool_zmalloc)a2dp_audio_heap_cmalloc,
                          (list_mempool_free)a2dp_audio_heap_free);

  memcpy(&(a2dp_audio_context.output_cfg), config,
         sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));
  ratio = a2dp_audio_context.output_cfg.factor_reference;
  a2dp_audio_context.output_cfg.factor_reference = 0;

  a2dp_audio_context.init_factor_reference = config->factor_reference;
  a2dp_audio_context.chnl_sel = chnl_sel;
  a2dp_audio_context.dest_packet_mut = dest_packet_mut;
  a2dp_audio_context.average_packet_mut = 0;

  switch (codec_type) {
  case A2DP_AUDIO_CODEC_TYPE_SBC:
    decoder_output_config.sample_rate = config->sample_rate;
    decoder_output_config.num_channels = 2;
    decoder_output_config.bits_depth = 16;
    decoder_output_config.frame_samples = config->frame_samples;
    decoder_output_config.factor_reference = 1.0f;
    memcpy(&(a2dp_audio_context.audio_decoder), &a2dp_audio_sbc_decoder_config,
           sizeof(A2DP_AUDIO_DECODER_T));
    break;
#if defined(A2DP_AAC_ON)
  case A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC:
    decoder_output_config.sample_rate = config->sample_rate;
    decoder_output_config.num_channels = 2;
    decoder_output_config.bits_depth = 16;
    decoder_output_config.frame_samples = config->frame_samples;
    decoder_output_config.factor_reference = 1.0f;
    memcpy(&(a2dp_audio_context.audio_decoder),
           &a2dp_audio_aac_lc_decoder_config, sizeof(A2DP_AUDIO_DECODER_T));
    break;
#endif
#if defined(A2DP_SCALABLE_ON)
  case A2DP_AUDIO_CODEC_TYPE_SCALABL:
    decoder_output_config.sample_rate = config->sample_rate;
    decoder_output_config.num_channels = 2;
    decoder_output_config.bits_depth = config->curr_bits;
    decoder_output_config.frame_samples = config->frame_samples;
    decoder_output_config.factor_reference = 1.0f;
    memcpy(&(a2dp_audio_context.audio_decoder),
           &a2dp_audio_scalable_decoder_config, sizeof(A2DP_AUDIO_DECODER_T));
    break;
#endif
#if defined(A2DP_LHDC_ON)
  case A2DP_AUDIO_CODEC_TYPE_LHDC:
    decoder_output_config.sample_rate = config->sample_rate;
    decoder_output_config.num_channels = 2;
    decoder_output_config.bits_depth = config->curr_bits;
    decoder_output_config.frame_samples = config->frame_samples;
    decoder_output_config.factor_reference = 1.0f;
    memcpy(&(a2dp_audio_context.audio_decoder), &a2dp_audio_lhdc_decoder_config,
           sizeof(A2DP_AUDIO_DECODER_T));
    break;
#endif
#if defined(A2DP_LDAC_ON)
  case A2DP_AUDIO_CODEC_TYPE_LDAC:
    decoder_output_config.sample_rate = config->sample_rate;
    decoder_output_config.num_channels = 2;
    decoder_output_config.bits_depth = config->curr_bits;
    decoder_output_config.frame_samples = config->frame_samples;
    decoder_output_config.factor_reference = 1.0f;
    memcpy(&(a2dp_audio_context.audio_decoder), &a2dp_audio_ldac_decoder_config,
           sizeof(A2DP_AUDIO_DECODER_T));
    break;
#endif
  default:
    ASSERT_A2DP_DECODER(0, "%s invalid codec_type:%d", __func__, codec_type);
    break;
  }

  a2dp_audio_context.audio_decoder.audio_decoder_init(
      &decoder_output_config, (void *)&a2dp_audio_context);
  a2dp_audio_context.need_detect_first_packet = true;
  a2dp_audio_context.underflow_onporcess = false;
  a2dp_audio_context.skip_frame_cnt_after_no_cache = 0;
  a2dp_audio_context.mute_frame_cnt_after_no_cache = 0;

  a2dp_audio_context.audio_decoder_status = A2DP_AUDIO_DECODER_STATUS_READY;

  a2dp_audio_context.store_packet_status =
      A2DP_AUDIO_DECODER_STORE_PACKET_STATUS_IDLE;
  a2dp_audio_context.playback_status = A2DP_AUDIO_DECODER_PLAYBACK_STATUS_IDLE;

  a2dp_audio_sync_init(ratio);

#if A2DP_DECODER_HISTORY_SEQ_SAVE
  a2dp_audio_reset_history_seq();
#endif
  a2dp_audio_store_packet_checker_start();

  a2dp_audio_status_mutex_unlock();

  return 0;
}

int a2dp_audio_deinit(void) {
  TRACE_A2DP_DECODER_I("[DEINIT]");

  a2dp_audio_status_mutex_lock();

  a2dp_audio_detect_next_packet_callback_register(NULL);
  a2dp_audio_detect_store_packet_callback_register(NULL);

  a2dp_audio_context.audio_decoder.audio_decoder_deinit();
  memset(&(a2dp_audio_context.audio_decoder), 0, sizeof(A2DP_AUDIO_DECODER_T));
  memset(&(a2dp_audio_context.output_cfg), 0,
         sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));
  a2dp_audio_list_clear(
      a2dp_audio_context.audio_datapath.input_raw_packet_list);
  a2dp_audio_list_free(a2dp_audio_context.audio_datapath.input_raw_packet_list);
  a2dp_audio_context.audio_datapath.input_raw_packet_list = NULL;
  a2dp_audio_list_clear(
      a2dp_audio_context.audio_datapath.output_pcm_packet_list);
  a2dp_audio_list_free(
      a2dp_audio_context.audio_datapath.output_pcm_packet_list);
  a2dp_audio_context.audio_datapath.output_pcm_packet_list = NULL;

  size_t total = 0, used = 0, max_used = 0;
  a2dp_audio_heap_info(&total, &used, &max_used);
  TRACE_A2DP_DECODER_I(
      "[DEINIT] heap info: total - %d, used - %d, max_used - %d.", total, used,
      max_used);
  // ASSERT_A2DP_DECODER(used == 0, "[%s] used != 0", __func__);

  a2dp_audio_set_store_packet_status(
      A2DP_AUDIO_DECODER_STORE_PACKET_STATUS_IDLE);
  a2dp_audio_set_playback_status(A2DP_AUDIO_DECODER_PLAYBACK_STATUS_IDLE);
  a2dp_audio_set_status(A2DP_AUDIO_DECODER_STATUS_NULL);
#if A2DP_DECODER_HISTORY_SEQ_SAVE
  a2dp_audio_reset_history_seq();
#endif
  a2dp_audio_status_mutex_unlock();

  return 0;
}

int a2dp_audio_stop(void) {
  TRACE_A2DP_DECODER_I("[STOP]");
  int cnt = 0;

  a2dp_audio_set_status(A2DP_AUDIO_DECODER_STATUS_STOP);
  a2dp_audio_semaphore_release();

  cnt = 50;
  do {
    if (a2dp_audio_get_playback_status() ==
        A2DP_AUDIO_DECODER_PLAYBACK_STATUS_IDLE) {
      TRACE_A2DP_DECODER_I("[DEINIT]PLAYBACK_STATUS_IDLE cnt:%d", cnt);
      break;
    } else {
      osThreadYield();
    }
  } while (--cnt > 0);
  cnt = 50;
  do {
    if (a2dp_audio_get_store_packet_status() ==
        A2DP_AUDIO_DECODER_STORE_PACKET_STATUS_IDLE) {
      TRACE_A2DP_DECODER_I("[DEINIT] STORE_PACKET_STATUS_IDLE cnt:%d", cnt);
      break;
    } else {
      osThreadYield();
    }
  } while (--cnt > 0);

  return 0;
}

int a2dp_audio_start(void) {
  TRACE_A2DP_DECODER_I("[START]");
  a2dp_audio_status_mutex_lock();
  a2dp_audio_set_status(A2DP_AUDIO_DECODER_STATUS_START);
  a2dp_audio_status_mutex_unlock();
  return 0;
}

int a2dp_audio_detect_next_packet_callback_register(
    A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK callback) {
  a2dp_audio_status_mutex_lock();
  a2dp_audio_detect_next_packet_callback = callback;
  a2dp_audio_status_mutex_unlock();

  return 0;
}

int a2dp_audio_detect_store_packet_callback_register(
    A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK callback) {
  a2dp_audio_status_mutex_lock();
  a2dp_audio_store_packet_callback = callback;
  a2dp_audio_status_mutex_unlock();

  return 0;
}

int a2dp_audio_detect_first_packet(void) {
  a2dp_audio_status_mutex_lock();
  a2dp_audio_context.need_detect_first_packet = true;
  a2dp_audio_status_mutex_unlock();
  return 0;
}

int a2dp_audio_detect_first_packet_clear(void) {
  a2dp_audio_status_mutex_lock();
  a2dp_audio_context.need_detect_first_packet = false;
  a2dp_audio_status_mutex_unlock();
  return 0;
}

int a2dp_audio_discards_packet(uint32_t packets) {
  int nRet = 0;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_status_mutex_lock();
    nRet =
        a2dp_audio_context.audio_decoder.audio_decoder_discards_packet(packets);
    a2dp_audio_status_mutex_unlock();
  } else {
    nRet = -1;
  }

  return nRet;
}

int a2dp_audio_synchronize_dest_packet_mut(uint32_t mtu) {
  int nRet = 0;
  int cnt = 50;
  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    do {
      if (a2dp_audio_get_playback_status() ==
          A2DP_AUDIO_DECODER_PLAYBACK_STATUS_IDLE) {
        nRet = a2dp_audio_context.audio_decoder
                   .audio_decoder_synchronize_dest_packet_mut(mtu);
        break;
      } else {
        osThreadYield();
      }
    } while (--cnt > 0);
  } else {
    nRet = -1;
  }

  return nRet;
}

int a2dp_audio_discards_samples(uint32_t samples) {
  return a2dp_audio_context.audio_decoder.a2dp_audio_discards_samples(samples);
}

int a2dp_audio_convert_list_to_samples(uint32_t *samples) {
  return a2dp_audio_context.audio_decoder.a2dp_audio_convert_list_to_samples(
      samples);
}

int a2dp_audio_get_packet_samples(void) {
  A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
  uint32_t packet_samples = 0;
  uint16_t totalSubSequenceNumber = 1;

  a2dp_audio_lastframe_info_get(&lastframe_info);

  if (lastframe_info.totalSubSequenceNumber) {
    totalSubSequenceNumber = lastframe_info.totalSubSequenceNumber;
  }

  packet_samples = totalSubSequenceNumber * lastframe_info.frame_samples;

  return packet_samples;
}

static int a2dp_audio_internal_lastframe_info_ptr_get(
    A2DP_AUDIO_LASTFRAME_INFO_T **lastframe_info) {
  int nRet = 0;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    *lastframe_info = &a2dp_audio_lastframe_info;
  } else {
    *lastframe_info = NULL;
    nRet = -1;
  }

  return nRet;
}

int a2dp_audio_lastframe_info_get(A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info) {
  int nRet = 0;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_buffer_mutex_lock();
    memcpy(lastframe_info, &a2dp_audio_lastframe_info,
           sizeof(A2DP_AUDIO_LASTFRAME_INFO_T));
    a2dp_audio_buffer_mutex_unlock();
  } else {
    memset(lastframe_info, 0, sizeof(A2DP_AUDIO_LASTFRAME_INFO_T));
    nRet = -1;
  }

  return nRet;
}

int a2dp_audio_decoder_internal_check_sum_reset(void) {
  check_sum_seed = 0;
  return 0;
}

uint32_t a2dp_audio_decoder_internal_check_sum_generate(const uint8_t *buf,
                                                        uint32_t len) {
#if A2DP_DECODER_CHECKER
  check_sum_seed = crc32(check_sum_seed, buf, len);
#else
  check_sum_seed = 0;
#endif
  return check_sum_seed;
}

int a2dp_audio_lastframe_info_reset_undecodeframe(void) {
  int nRet = 0;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_buffer_mutex_lock();
    a2dp_audio_lastframe_info.undecode_frames = 0;
    a2dp_audio_lastframe_info.undecode_max_frames = 0;
    a2dp_audio_lastframe_info.undecode_min_frames = 0xffff;
    a2dp_audio_decoder_internal_check_sum_reset();
    a2dp_audio_lastframe_info.check_sum = 0;
    a2dp_audio_buffer_mutex_unlock();
  } else {
    nRet = -1;
  }

  return nRet;
}

int a2dp_audio_decoder_internal_lastframe_info_set(
    A2DP_AUDIO_DECODER_LASTFRAME_INFO_T *lastframe_info) {
  a2dp_audio_buffer_mutex_lock();
  a2dp_audio_lastframe_info.sequenceNumber = lastframe_info->sequenceNumber;
  a2dp_audio_lastframe_info.timestamp = lastframe_info->timestamp;
  a2dp_audio_lastframe_info.curSubSequenceNumber =
      lastframe_info->curSubSequenceNumber;
  a2dp_audio_lastframe_info.totalSubSequenceNumber =
      lastframe_info->totalSubSequenceNumber;
  a2dp_audio_lastframe_info.frame_samples = lastframe_info->frame_samples;
  a2dp_audio_lastframe_info.list_samples = lastframe_info->list_samples;
  a2dp_audio_lastframe_info.decoded_frames = lastframe_info->decoded_frames;
  a2dp_audio_lastframe_info.undecode_frames = lastframe_info->undecode_frames;

  a2dp_audio_lastframe_info.undecode_max_frames =
      MAX(a2dp_audio_lastframe_info.undecode_frames,
          a2dp_audio_lastframe_info.undecode_max_frames);
  a2dp_audio_lastframe_info.undecode_min_frames =
      MIN(a2dp_audio_lastframe_info.undecode_frames,
          a2dp_audio_lastframe_info.undecode_min_frames);

  a2dp_audio_lastframe_info.stream_info = lastframe_info->stream_info;
  a2dp_audio_lastframe_info.check_sum = lastframe_info->check_sum;
  a2dp_audio_buffer_mutex_unlock();

  return 0;
}

int a2dp_audio_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info,
                                  uint32_t mask) {
  int nRet = 0;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_status_mutex_lock();
    nRet = a2dp_audio_context.audio_decoder.audio_decoder_synchronize_packet(
        sync_info, mask);
    if (nRet == A2DP_DECODER_NOT_SUPPORT) {
      // can't support synchronize packet, so return fake val;
      nRet = A2DP_DECODER_NO_ERROR;
    }
    a2dp_audio_status_mutex_unlock();
  } else {
    nRet = -1;
  }

  return nRet;
}

int a2dp_audio_decoder_headframe_info_get(
    A2DP_AUDIO_HEADFRAME_INFO_T *headframe_info) {
  int nRet = 0;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    nRet = a2dp_audio_context.audio_decoder.audio_decoder_headframe_info_get(
        headframe_info);
  } else {
    memset(headframe_info, 0, sizeof(A2DP_AUDIO_HEADFRAME_INFO_T));
    nRet = -1;
  }

  return nRet;
}

int a2dp_audio_refill_packet(void) {
  int refill_cnt = 0;

#if defined(A2DP_AUDIO_REFILL_AFTER_NO_CACHE)
  refill_cnt += a2dp_audio_context.skip_frame_cnt_after_no_cache;
#endif
  refill_cnt += a2dp_audio_context.mute_frame_cnt_after_no_cache;

  return refill_cnt;
}

bool a2dp_audio_auto_synchronize_support(void) {
  bool nRet = 0;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_buffer_mutex_lock();
    nRet = a2dp_audio_context.audio_decoder.auto_synchronize_support > 0
               ? true
               : false;
    a2dp_audio_buffer_mutex_unlock();
  } else {
    nRet = 0;
  }

  return nRet;
}

A2DP_AUDIO_OUTPUT_CONFIG_T *a2dp_audio_get_output_config(void) {
  A2DP_AUDIO_OUTPUT_CONFIG_T *output_config = NULL;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_buffer_mutex_lock();
    output_config = &a2dp_audio_context.output_cfg;
    a2dp_audio_buffer_mutex_unlock();
  } else {
    output_config = NULL;
  }

  return output_config;
}

int a2dp_audio_latency_factor_setlow(void) {
  a2dp_audio_latency_factor = A2DP_AUDIO_LATENCY_LOW_FACTOR;
  return 0;
}

int a2dp_audio_latency_factor_sethigh(void) {
  a2dp_audio_latency_factor = A2DP_AUDIO_LATENCY_HIGH_FACTOR;
  return 0;
}

float a2dp_audio_latency_factor_get(void) { return a2dp_audio_latency_factor; }

int a2dp_audio_latency_factor_set(float factor) {
  a2dp_audio_latency_factor = factor;
  return 0;
}

int a2dp_audio_latency_factor_status_get(
    A2DP_AUDIO_LATENCY_STATUS_E *latency_status, float *more_latency_factor) {
  if (a2dp_audio_latency_factor == A2DP_AUDIO_LATENCY_HIGH_FACTOR) {
    *latency_status = A2DP_AUDIO_LATENCY_STATUS_HIGH;
    *more_latency_factor = A2DP_AUDIO_LATENCY_MORE_FACTOR;
  } else {
    *latency_status = A2DP_AUDIO_LATENCY_STATUS_LOW;
    *more_latency_factor = 1.0f;
  }
  return 0;
}

int a2dp_audio_frame_delay_get(void) { return get_in_cp_frame_delay(); }

int a2dp_audio_dest_packet_mut_get(void) {
  return a2dp_audio_context.dest_packet_mut;
}

int a2dp_audio_set_channel_select(A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel) {
  int nRet = A2DP_DECODER_NO_ERROR;

  if (a2dp_audio_get_status() == A2DP_AUDIO_DECODER_STATUS_START) {
    a2dp_audio_context.chnl_sel = chnl_sel;
    if (a2dp_audio_context.audio_decoder.audio_decoder_channel_select) {
      nRet = a2dp_audio_context.audio_decoder.audio_decoder_channel_select(
          chnl_sel);
    }
  }

  return nRet;
}

float a2dp_audio_get_sample_reference(void) {
  TRACE(1, "a2dp_audio_get_sample_reference:%d",
        (int32_t)(a2dp_audio_context.output_cfg.factor_reference * 10000000));
  return a2dp_audio_context.output_cfg.factor_reference;
}

int8_t a2dp_audio_get_current_buf_size(void) {
  TRACE(1, "a2dp_audio_get_current_buf_size:%d",
        (int8_t)(a2dp_audio_context.average_packet_mut + 0.5f));
  return (int8_t)(a2dp_audio_context.average_packet_mut + 0.5f);
}
