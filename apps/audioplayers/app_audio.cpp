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
#include "app_bt_trace.h"
#include "cmsis_os.h"
#include "string.h"

#include "app_audio.h"
#include "app_media_player.h"
#include "app_ring_merge.h"
#include "app_thread.h"
#include "cqueue.h"
#include "hal_aud.h"
#include "list.h"
#include "nvrecord.h"
#include "resources.h"
#include <assert.h>

#include "a2dp_api.h"

#include "app_bt_media_manager.h"
#include "btapp.h"

extern uint8_t __StackLimit[];
extern uint8_t __HeapLimit[];

static const size_t heap_size = __StackLimit - __HeapLimit - 512;

static bool app_audio_init = false;

static uint32_t capture_audio_buff_size_used;
// from the bottom of the system available memory pool, size is
// APP_CAPTURE_AUDIO_BUFFER_SIZE can be overlayed with the sco used audio buffer
static uint8_t *capture_audio_buffer =
    __HeapLimit + heap_size - APP_CAPTURE_AUDIO_BUFFER_SIZE;

int app_capture_audio_mempool_init(void) {
  TRACE_AUD_HDL_I("[CAPMEM][INIT] size %d.", APP_CAPTURE_AUDIO_BUFFER_SIZE);
  capture_audio_buff_size_used = 0;
  memset((uint8_t *)capture_audio_buffer, 0, APP_CAPTURE_AUDIO_BUFFER_SIZE);

  return 0;
}

uint32_t app_capture_audio_mempool_free_buff_size() {
  return APP_CAPTURE_AUDIO_BUFFER_SIZE - capture_audio_buff_size_used;
}

int app_capture_audio_mempool_get_buff(uint8_t **buff, uint32_t size) {
  uint32_t buff_size_free;
  uint8_t *capture_buf_addr = (uint8_t *)capture_audio_buffer;
  buff_size_free = app_capture_audio_mempool_free_buff_size();

  if (size % 4) {
    size = size + (4 - size % 4);
  }

  TRACE_AUD_HDL_I("[CAPMEM][GET] current free %d to allocate %d",
                  buff_size_free, size);

  ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__,
         size, buff_size_free);

  *buff = capture_buf_addr + capture_audio_buff_size_used;

  capture_audio_buff_size_used += size;
  TRACE_AUD_HDL_I("[CAPMEM][GET] %d, now used %d left %d", size,
                  capture_audio_buff_size_used,
                  app_capture_audio_mempool_free_buff_size());

  return 0;
}

osPoolDef(app_audio_status_mempool, 20, APP_AUDIO_STATUS);
osPoolId app_audio_status_mempool = NULL;

// control queue access
osMutexId g_app_audio_queue_mutex_id = NULL;
osMutexDef(g_app_audio_queue_mutex);

// control pcmbuff access
static CQueue app_audio_pcm_queue;
static osMutexId app_audio_pcmbuff_mutex_id = NULL;
osMutexDef(app_audio_pcmbuff_mutex);

#ifdef __AUDIO_QUEUE_SUPPORT__

typedef struct {
  list_t *audio_list;
} APP_AUDIO_CONFIG;

APP_AUDIO_CONFIG app_audio_conifg = {.audio_list = NULL};

#endif

void LOCK_APP_AUDIO_QUEUE() {
  osMutexWait(g_app_audio_queue_mutex_id, osWaitForever);
}

void UNLOCK_APP_AUDIO_QUEUE() { osMutexRelease(g_app_audio_queue_mutex_id); }

uint32_t app_audio_lr_balance(uint8_t *buf, uint32_t len, int8_t balance) {
  short *balance_buf = (short *)buf;
  uint32_t balance_len = len / 2;
  float factor;

  ASSERT((balance >= -100) && (balance <= 100), "balance = %d is invalid!",
         balance);

  if (balance > 0) {
    // reduce L channel
    factor = 1 - 0.01 * balance;
    for (uint32_t i = 0; i < balance_len; i += 2) {
      balance_buf[i] = (short)(factor * balance_buf[i]);
    }
  } else if (balance < 0) {
    // reduce R channel
    factor = 1 + 0.01 * balance;
    for (uint32_t i = 0; i < balance_len; i += 2) {
      balance_buf[i + 1] = (short)(factor * balance_buf[i + 1]);
    }
  }
  return 0;
}

void app_audio_mempool_init_with_specific_size(uint32_t size) {
  syspool_init_specific_size(size);
}

int app_audio_pcmbuff_init(uint8_t *buff, uint16_t len) {
  if (app_audio_pcmbuff_mutex_id == NULL)
    app_audio_pcmbuff_mutex_id =
        osMutexCreate((osMutex(app_audio_pcmbuff_mutex)));

  if ((buff == NULL) || (app_audio_pcmbuff_mutex_id == NULL))
    return -1;

  osMutexWait(app_audio_pcmbuff_mutex_id, osWaitForever);
  InitCQueue(&app_audio_pcm_queue, len, buff);
  memset(buff, 0x00, len);
  osMutexRelease(app_audio_pcmbuff_mutex_id);

  return 0;
}

int app_audio_pcmbuff_length(void) {
  int len;

  osMutexWait(app_audio_pcmbuff_mutex_id, osWaitForever);
  len = LengthOfCQueue(&app_audio_pcm_queue);
  osMutexRelease(app_audio_pcmbuff_mutex_id);

  return len;
}

int app_audio_pcmbuff_put(uint8_t *buff, uint16_t len) {
  int status;

  osMutexWait(app_audio_pcmbuff_mutex_id, osWaitForever);
  status = EnCQueue(&app_audio_pcm_queue, buff, len);
  osMutexRelease(app_audio_pcmbuff_mutex_id);

  return status;
}

int app_audio_pcmbuff_get(uint8_t *buff, uint16_t len) {
  unsigned char *e1 = NULL, *e2 = NULL;
  unsigned int len1 = 0, len2 = 0;
  int status;

  osMutexWait(app_audio_pcmbuff_mutex_id, osWaitForever);
  status = PeekCQueue(&app_audio_pcm_queue, len, &e1, &len1, &e2, &len2);
  if (len == (len1 + len2)) {
    memcpy(buff, e1, len1);
    memcpy(buff + len1, e2, len2);
    DeCQueue(&app_audio_pcm_queue, 0, len1);
    DeCQueue(&app_audio_pcm_queue, 0, len2);
  } else {
    memset(buff, 0x00, len);
    status = -1;
  }
  osMutexRelease(app_audio_pcmbuff_mutex_id);

  return status;
}

int app_audio_pcmbuff_discard(uint16_t len) {
  int status;

  osMutexWait(app_audio_pcmbuff_mutex_id, osWaitForever);
  status = DeCQueue(&app_audio_pcm_queue, 0, len);
  osMutexRelease(app_audio_pcmbuff_mutex_id);

  return status;
}

void __attribute__((section(".fast_text_sram")))
app_audio_memcpy_16bit(int16_t *des, int16_t *src, int len) {
  // Check input
  for (int i = 0; i < len; i++) {
    des[i] = src[i];
  }
}

void __attribute__((section(".fast_text_sram")))
app_audio_memset_16bit(int16_t *des, int16_t val, int len) {
  // Check input
  for (int i = 0; i < len; i++) {
    des[i] = val;
  }
}

#ifdef __AUDIO_QUEUE_SUPPORT__
int app_audio_sendrequest_param(uint16_t id, uint8_t status, uint32_t ptr,
                                uint32_t param) {
  uint32_t audevt;
  APP_MESSAGE_BLOCK msg;

  if (app_audio_init == false)
    return -1;

  msg.mod_id = APP_MODUAL_AUDIO;
  APP_AUDIO_SET_MESSAGE(audevt, id, status);
  msg.msg_body.message_id = audevt;
  msg.msg_body.message_ptr = ptr;
  msg.msg_body.message_Param0 = param;
  app_mailbox_put(&msg);

  return 0;
}

int app_audio_sendrequest(uint16_t id, uint8_t status, uint32_t ptr) {
  return app_audio_sendrequest_param(id, status, ptr, 0);
}

extern bool app_audio_list_playback_exist(void);
#ifdef MEDIA_PLAYER_SUPPORT
static uint8_t app_audio_get_list_playback_num(void) {
  APP_AUDIO_STATUS *audio_handle = NULL;
  list_node_t *node = NULL;
  uint8_t num = 0;
  for (node = list_begin(app_audio_conifg.audio_list);
       node != list_end(app_audio_conifg.audio_list); node = list_next(node)) {
    audio_handle = (APP_AUDIO_STATUS *)list_node(node);
    if (audio_handle == NULL) {
      return num;
    }
    if (audio_handle->id == APP_PLAY_BACK_AUDIO)
      num++;
  }
  return num;
}
#endif
#endif

static bool need_flush_flash_switch_audio = false;
void app_audio_switch_flash_flush_req(void) {
  uint32_t lock;

  lock = int_lock();
  need_flush_flash_switch_audio = true;
  int_unlock(lock);
}

static void app_audio_switch_flash_proc(void) {
  // no need to do this across the audio switch,
  // will use suspend flash erase to assure that no audio
  // irq is missing for handling caused by long time global irq disabling
  // during flash erase.
  // Just flash the nvrecord flash periodically
  return;

  uint32_t lock;
  bool need_flush_flash = false;

  lock = int_lock();
  if (need_flush_flash_switch_audio) {
    need_flush_flash_switch_audio = false;
    need_flush_flash = true;
  }
  int_unlock(lock);

  if (need_flush_flash) {
    nv_record_flash_flush();
  }
}

#ifdef VOICE_DATAPATH
static bool app_audio_handle_pre_processing(APP_MESSAGE_BODY *msg_body) {
  uint16_t stream_type;
  APP_AUDIO_GET_AUD_ID(msg_body->message_ptr, stream_type);

  bool isToResetCaptureStream = false;
  if ((BT_STREAM_SBC == stream_type) || (BT_STREAM_MEDIA == stream_type)) {
    if (app_audio_manager_capture_is_active()) {
      isToResetCaptureStream = true;
    }
  }

  if (isToResetCaptureStream) {
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_CAPTURE,
                                  0, 0);

    APP_MESSAGE_BLOCK msg;
    msg.msg_body = *msg_body;
    msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
    app_mailbox_put(&msg);
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                                  BT_STREAM_CAPTURE, 0, 0);

    return false;
  } else {
    return true;
  }
}
#endif

static int app_audio_handle_process(APP_MESSAGE_BODY *msg_body) {
  int nRet = -1;

  APP_AUDIO_STATUS aud_status;

  if (app_audio_init == false)
    return -1;

#ifdef VOICE_DATAPATH
  bool isContinue = app_audio_handle_pre_processing(msg_body);
  if (!isContinue) {
    return -1;
  }
#endif

  APP_AUDIO_GET_ID(msg_body->message_id, aud_status.id);
  APP_AUDIO_GET_STATUS(msg_body->message_id, aud_status.status);
  APP_AUDIO_GET_AUD_ID(msg_body->message_ptr, aud_status.aud_id);
  APP_AUDIO_GET_FREQ(msg_body->message_Param0, aud_status.freq);

  switch (aud_status.status) {
  case APP_BT_SETTING_OPEN:
#ifdef __AUDIO_QUEUE_SUPPORT__
    TRACE_AUD_HDL_I("[OPEN] before status_id: 0x%x%s, aud_id: %d, len = %d",
                    aud_status.id, player2str(aud_status.id), aud_status.aud_id,
                    list_length(app_audio_conifg.audio_list));

    if (app_audio_list_append(&aud_status)) {
      app_bt_stream_open(&aud_status);
      TRACE_AUD_HDL_I("[OPEN] after status_id: 0x%x%s, len = %d", aud_status.id,
                      player2str(aud_status.id),
                      list_length(app_audio_conifg.audio_list));
    }
#else
    app_bt_stream_open(&aud_status);
#endif

    break;
  case APP_BT_SETTING_CLOSE:
    app_audio_switch_flash_proc();
#ifdef __AUDIO_QUEUE_SUPPORT__
    APP_AUDIO_STATUS next_status;
    TRACE_AUD_HDL_I("[CLOSE] current id: 0x%x%s", aud_status.id,
                    player2str(aud_status.id));
    app_bt_stream_close(aud_status.id);
    app_audio_switch_flash_proc();
#ifdef MEDIA_PLAYER_SUPPORT
    if (aud_status.id == APP_PLAY_BACK_AUDIO) {
      TRACE_AUD_HDL_I("[CLOSE] list: %d", app_audio_get_list_playback_num());
      if (app_audio_get_list_playback_num() == 1) {
        TRACE_AUD_HDL_I("=======>APP_BT_SETTING_CLOSE MEDIA");
        bt_media_stop(BT_STREAM_MEDIA, BT_DEVICE_ID_1);
      }
    }
#endif
    if (app_audio_list_rmv_callback(&aud_status, &next_status,
                                    APP_BT_SETTING_Q_POS_HEAD, false)) {
      TRACE_AUD_HDL_I("[CLOSE] %p, next id: 0x%x%s, status %d", &next_status,
                      next_status.id, player2str(next_status.id),
                      next_status.status);
      app_bt_stream_open(&next_status);
    }
#else
    app_bt_stream_close(aud_status.id);
    app_audio_switch_flash_proc();
#endif
    break;
  case APP_BT_SETTING_SETUP:
    app_bt_stream_setup(aud_status.id, msg_body->message_ptr);
    break;
  case APP_BT_SETTING_RESTART:
    app_bt_stream_restart(&aud_status);
    break;
  case APP_BT_SETTING_CLOSEALL:
    app_bt_stream_closeall();
#ifdef __AUDIO_QUEUE_SUPPORT__
    app_audio_list_clear();
#endif
    app_audio_switch_flash_proc();
    break;
  default:
    break;
  }

  return nRet;
}

#ifdef __AUDIO_QUEUE_SUPPORT__
static void app_audio_handle_free(void *data) {
#ifdef MEDIA_PLAYER_SUPPORT
  APP_AUDIO_STATUS *status = (APP_AUDIO_STATUS *)data;

  if (status->id == APP_PLAY_BACK_AUDIO) {
    TRACE_AUD_HDL_I("[HANDLE_FREE] , aud_id: 0x%x, type = 0x%x", status->aud_id,
                    status->aud_type);
  }
#endif
  osPoolFree(app_audio_status_mempool, data);
}

void app_audio_list_create() {
  if (app_audio_conifg.audio_list == NULL) {
    app_audio_conifg.audio_list = list_new(app_audio_handle_free, NULL, NULL);
  }
}

bool app_audio_list_stream_exist() {
  APP_AUDIO_STATUS *audio_handle = NULL;
  list_node_t *node = NULL;

  for (node = list_begin(app_audio_conifg.audio_list);
       node != list_end(app_audio_conifg.audio_list); node = list_next(node)) {

    audio_handle = (APP_AUDIO_STATUS *)list_node(node);
    if (audio_handle == NULL) {
      return false;
    }
    if (audio_handle->id == APP_BT_STREAM_HFP_PCM ||
        audio_handle->id == APP_BT_STREAM_HFP_CVSD ||
        audio_handle->id == APP_BT_STREAM_HFP_VENDOR ||
        audio_handle->id == APP_BT_STREAM_A2DP_SBC ||
        audio_handle->id == APP_BT_STREAM_A2DP_AAC ||
#ifdef VOICE_DATAPATH
        audio_handle->id == APP_BT_STREAM_VOICEPATH ||
#endif
#ifdef __AI_VOICE__
        audio_handle->id == APP_BT_STREAM_AI_VOICE ||
#endif
#ifdef __THIRDPARTY
        audio_handle->id == APP_BT_STREAM_THIRDPARTY_VOICE ||
#endif
        audio_handle->id == APP_BT_STREAM_A2DP_VENDOR) {
      TRACE_AUD_HDL_I("[STREAM_LIST][EXIST] id 0x%x", audio_handle->id);
      return true;
    }
  }

  return false;
}

bool app_audio_list_filter_exist(APP_AUDIO_STATUS *aud_status) {
#ifdef MEDIA_PLAYER_SUPPORT
  APP_AUDIO_STATUS *audio_handle = NULL;
  list_node_t *node = NULL;
  uint8_t cnt = 0;

  if (aud_status->id == APP_PLAY_BACK_AUDIO) {
    if (aud_status->aud_id == AUD_ID_BT_CALL_INCOMING_CALL) {
      for (node = list_begin(app_audio_conifg.audio_list);
           node != list_end(app_audio_conifg.audio_list);
           node = list_next(node)) {
        audio_handle = (APP_AUDIO_STATUS *)list_node(node);
        if (audio_handle == NULL) {
          return false;
        }
        if (audio_handle->id == APP_PLAY_BACK_AUDIO &&
            audio_handle->aud_id == AUD_ID_BT_CALL_INCOMING_CALL) {
          TRACE_AUD_HDL_I("[STREAM_LIST][FILTER] id 0x%x", audio_handle->id);
          return true;
        }
      }
    } else {
      for (node = list_begin(app_audio_conifg.audio_list);
           node != list_end(app_audio_conifg.audio_list);
           node = list_next(node)) {
        audio_handle = (APP_AUDIO_STATUS *)list_node(node);
        if (cnt++ > 1) {
          TRACE_AUD_HDL_I("[STREAM_LIST][FILTER] cnt %d", cnt);
          return true;
        }
      }
    }
  }
#endif
  return false;
}

bool app_audio_list_playback_exist(void) {
#ifdef MEDIA_PLAYER_SUPPORT
  APP_AUDIO_STATUS *audio_handle = NULL;
  list_node_t *node = NULL;

  for (node = list_begin(app_audio_conifg.audio_list);
       node != list_end(app_audio_conifg.audio_list); node = list_next(node)) {
    audio_handle = (APP_AUDIO_STATUS *)list_node(node);
    if (audio_handle == NULL) {
      return false;
    }
    if (audio_handle->id == APP_PLAY_BACK_AUDIO) {
      TRACE_AUD_HDL_I("[STREAM_LIST][PLAYBACK_EXIST]");
      return true;
    }
  }
#endif
  return false;
}

void app_audio_list_playback_clear(void) {
#ifdef MEDIA_PLAYER_SUPPORT
  APP_AUDIO_STATUS *audio_handle = NULL;
  list_node_t *node = NULL;

  for (node = list_begin(app_audio_conifg.audio_list);
       node != list_end(app_audio_conifg.audio_list); node = list_next(node)) {
    audio_handle = (APP_AUDIO_STATUS *)list_node(node);
    if (audio_handle == NULL) {
      TRACE_AUD_HDL_I("[STREAM_LIST][CLR] find null\n ");
      return;
    }
    if (audio_handle->id == APP_PLAY_BACK_AUDIO) {
      list_remove(app_audio_conifg.audio_list, list_node(node));
    }
  }
#endif
}

bool app_audio_list_append(APP_AUDIO_STATUS *aud_status) {
  APP_AUDIO_STATUS *data_to_append = NULL;
  bool add_data_to_head_of_list = false;
  bool ret = true;
  TRACE_AUD_HDL_I("[STREAM_LIST][APPEND] id 0x%x%s", aud_status->id,
                  player2str(aud_status->id));
#ifdef MEDIA_PLAYER_SUPPORT
  if (aud_status->id == APP_PLAY_BACK_AUDIO) {
    // ignore redundant ring ind from hfp...
    if (app_audio_list_filter_exist(aud_status)) {
      return false;
    }

    if (app_audio_list_playback_exist()) {
      if (list_length(app_audio_conifg.audio_list) >= MAX_AUDIO_BUF_LIST) {
        if (app_audio_list_stream_exist()) {
          data_to_append =
              (APP_AUDIO_STATUS *)osPoolCAlloc(app_audio_status_mempool);
          if (data_to_append == NULL) {
            return false;
          }
          memcpy(data_to_append,
                 (const void *)list_front(app_audio_conifg.audio_list),
                 sizeof(APP_AUDIO_STATUS));
          add_data_to_head_of_list = true;
        }

        app_audio_list_clear();
        TRACE_AUD_HDL_E("[STREAM_LIST][APPEND] FIXME!!!!\n ");
      } else {
        ret = false;
      }
    }
  } else
#endif
  {
    add_data_to_head_of_list = true;
  }

  if (data_to_append == NULL) {
    data_to_append = (APP_AUDIO_STATUS *)osPoolCAlloc(app_audio_status_mempool);
    if (data_to_append == NULL) {
      return false;
    }
    memcpy(data_to_append, aud_status, sizeof(APP_AUDIO_STATUS));
  }

  if (add_data_to_head_of_list) {
    list_prepend(app_audio_conifg.audio_list, (void *)data_to_append);
  } else {
    list_append(app_audio_conifg.audio_list, (void *)data_to_append);
  }

  TRACE_AUD_HDL_I("[STREAM_LIST][APPEND] id 0x%x%s status %d len %d ret %d",
                  data_to_append->id, player2str(data_to_append->id),
                  data_to_append->status,
                  list_length(app_audio_conifg.audio_list), ret);
  return ret;
}

bool app_audio_list_rmv_callback(APP_AUDIO_STATUS *status_close,
                                 APP_AUDIO_STATUS *status_next,
                                 enum APP_BT_AUDIO_Q_POS pos, bool pop_next) {
  void *data_to_remove = NULL;
  bool ret = false;

  // for status: first bt_a2dp->APP_BT_SETTING_CLOSE,then ring->
  // APP_BT_SETTING_CLOSE
  TRACE_AUD_HDL_I("[STREAM_LIST][RMV] audio list len %d close_id 0x%x%s",
                  list_length(app_audio_conifg.audio_list), status_close->id,
                  player2str(status_close->id));
  if (list_length(app_audio_conifg.audio_list) == 0) {
    return false;
  }

#ifdef MEDIA_PLAYER_SUPPORT
  APP_AUDIO_STATUS *audio_handle = NULL;
  list_node_t *node = NULL;
  if (status_close->id == APP_PLAY_BACK_AUDIO) {
    for (node = list_begin(app_audio_conifg.audio_list);
         node != list_end(app_audio_conifg.audio_list);
         node = list_next(node)) {
      audio_handle = (APP_AUDIO_STATUS *)list_node(node);
      if (audio_handle == NULL) {
        return ret;
      }
      if (audio_handle->id == APP_PLAY_BACK_AUDIO) {
        list_node_t *nod_next = list_next(node);
        data_to_remove = list_node(node);

        if (pop_next) {
          memcpy(status_next, list_node(node), sizeof(APP_AUDIO_STATUS));
          ret = true;
          break;
        }
        if (nod_next != NULL) {
          memcpy(status_next, list_node(nod_next), sizeof(APP_AUDIO_STATUS));
          ASSERT(status_next->id == APP_PLAY_BACK_AUDIO,
                 "[%s] 111ERROR: status_next->id != APP_PLAY_BACK_AUDIO",
                 __func__);

          ret = true;
        } else if (app_audio_list_stream_exist()) {
          void *indata = list_front(app_audio_conifg.audio_list);
          if (indata == NULL) {
            return ret;
          }
          memcpy(status_next, indata, sizeof(APP_AUDIO_STATUS));
          ASSERT(status_next->id != APP_PLAY_BACK_AUDIO,
                 "[%s] 222ERROR: status_next->id != APP_PLAY_BACK_AUDIO",
                 __func__);

          ret = true;
        }
        break;
      }
    }
  } else // maybe...a2dp send >> APP_BT_SETTING_CLOSE,  when ring
#endif
  {
    if (app_audio_list_stream_exist()) {
      if (pos == APP_BT_SETTING_Q_POS_HEAD) {
        data_to_remove = list_front(app_audio_conifg.audio_list);
      } else if (pos == APP_BT_SETTING_Q_POS_TAIL) {
        data_to_remove = list_back(app_audio_conifg.audio_list);
      }
    }
  }

  if (data_to_remove) {
    list_remove(app_audio_conifg.audio_list, data_to_remove);
  }
  TRACE_AUD_HDL_I("[STREAM_LIST][RMV] end len:%d ret %d data %p",
                  list_length(app_audio_conifg.audio_list), ret,
                  data_to_remove);
  return ret;
}

void app_audio_list_clear() { list_clear(app_audio_conifg.audio_list); }
#endif

void app_audio_open(void) {
  if (app_audio_init) {
    return;
  }
  if (g_app_audio_queue_mutex_id == NULL) {
    g_app_audio_queue_mutex_id =
        osMutexCreate((osMutex(g_app_audio_queue_mutex)));
  } else {
    ASSERT(0, "[%s] ERROR: g_app_audio_queue_mutex_id != NULL", __func__);
  }

  if (app_audio_status_mempool == NULL)
    app_audio_status_mempool = osPoolCreate(osPool(app_audio_status_mempool));
  ASSERT(app_audio_status_mempool,
         "[%s] ERROR: app_audio_status_mempool != NULL", __func__);

#ifdef __AUDIO_QUEUE_SUPPORT__
  app_audio_list_create();
#endif
  app_ring_merge_init();

  app_set_threadhandle(APP_MODUAL_AUDIO, app_audio_handle_process);

  app_bt_stream_init();

  app_audio_init = true;
}

void app_audio_close(void) {
  app_set_threadhandle(APP_MODUAL_AUDIO, NULL);
  app_audio_init = false;
}
