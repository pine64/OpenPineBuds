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
#if (A2DP_DECODER_VER < 2)

#ifdef MBED
#include "mbed.h"
#include "rtos.h"
#endif
// Standard C Included Files
#include "tgt_hardware.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MBED
#include "SDFileSystem.h"
#endif
#include "analog.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "audioflinger.h"
#include "cqueue.h"
#include "hal_codec.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif

extern "C" {
#if defined(A2DP_LHDC_ON)
#include "hal_sysfreq.h"
#include "lhdcUtil.h"
#endif
}

extern "C" {
#if defined(A2DP_LDAC_ON)
#include "hal_sysfreq.h"
// #include "speech_memory.h"
#include "ldacBT.h"
#define MED_MEM_HEAP_SIZE (1024 * 20)
HANDLE_LDAC_BT hLdacData = NULL;
#endif
}

#include "a2dp_api.h"

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
#include "audio_resample_ex.h"
#include "hal_chipid.h"
#include "hal_sysfreq.h"
#endif

#include "btapp.h"
#include "cmsis.h"
#include "hal_location.h"

#define TEXT_A2DP_LOC_A(n, l) __attribute__((section(#n "." #l)))
#define TEXT_A2DP_LOC(n, l) TEXT_A2DP_LOC_A(n, l)

#define TEXT_SBC_LOC TEXT_A2DP_LOC(.overlay_a2dp_sbc, __LINE__)
#define TEXT_AAC_LOC TEXT_A2DP_LOC(.overlay_a2dp_aac, __LINE__)
#define TEXT_SSC_LOC TEXT_A2DP_LOC(.overlay_a2dp_ssc, __LINE__)
#define TEXT_LDAC_LOC TEXT_A2DP_LOC(.overlay_a2dp_ldac, __LINE__)
#define TEXT_LHDC_LOC TEXT_A2DP_LOC(.overlay_a2dp_lhdc, __LINE__)

// #define A2DP_AUDIO_SYNC_WITH_LOCAL (1)

#define A2DP_AUDIO_SYNC_TRACE(s, ...)
// TRACE(s, ##__VA_ARGS__)

#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
#define A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_DEFAULT (0)
#define A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_INC (2)
#define A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_DEC (-2)
#define A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_STEP (0.00005f)
#define A2DPPLAY_SYNC_STATUS_SET (0x01)
#define A2DPPLAY_SYNC_STATUS_RESET (0x02)
#define A2DPPLAY_SYNC_STATUS_PROC (0x04)

enum A2DP_AUDIO_SYNC_STATUS {
  A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEC,
  A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_INC,
  A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEFAULT,
};
#endif

#define A2DPPLAY_CACHE_OK_THRESHOLD (sbc_frame_size << 6)

enum A2DPPLAY_STRTEAM_T {
  A2DPPLAY_STRTEAM_PUT = 0,
  A2DPPLAY_STRTEAM_GET,
  A2DPPLAY_STRTEAM_QTY,
};

/* sbc queue */
#define SBC_TEMP_BUFFER_SIZE 128
#define SBC_QUEUE_SIZE_DEFAULT (SBC_TEMP_BUFFER_SIZE * 64)
#define SBC_QUEUE_SIZE (1024 * 11)

/* sbc decoder */
static bool need_init_decoder = true;
static btif_sbc_decoder_t *sbc_decoder = NULL;

static float sbc_eq_band_gain[CFG_HW_AUD_EQ_NUM_BANDS];

CQueue sbc_queue;

static uint32_t g_sbc_queue_size = SBC_QUEUE_SIZE_DEFAULT;
static uint16_t sbc_frame_size = SBC_TEMP_BUFFER_SIZE;
static uint16_t sbc_frame_rev_len = 0;
static uint16_t sbc_frame_num = 0;
static uint32_t assumed_mtu_interval = 0;

static uint32_t dec_start_time;
static uint32_t last_dec_time;
#ifdef A2DP_TRACE_DEC_TIME
static const bool dec_trace_time = true;
#else
static bool dec_trace_time;
#endif
static bool dec_reset_queue;

static enum APP_AUDIO_CACHE_T a2dp_cache_status = APP_AUDIO_CACHE_QTY;

#define A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_CNT (5)
#define A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_MS (20)
static osThreadId a2dp_put_thread_tid = NULL;
static bool a2dp_get_need_sync = false;

extern enum AUD_SAMPRATE_T a2dp_sample_rate;
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
#ifdef CHIP_BEST1000
static bool allow_resample = false;
#else
static const bool allow_resample = true;
#endif
#endif

extern int a2dp_timestamp_parser_needsync(void);

#define A2DP_SYNC_WITH_GET_MUTUX_ALLOC()                                       \
  do {                                                                         \
    if (a2dp_put_thread_tid == NULL) {                                         \
      a2dp_put_thread_tid = osThreadGetId();                                   \
    }                                                                          \
  } while (0)

#define A2DP_SYNC_WITH_GET_MUTUX_FREE()                                        \
  do {                                                                         \
    a2dp_put_thread_tid = NULL;                                                \
  } while (0)

#define A2DP_SYNC_WITH_GET_MUTUX_WAIT()                                        \
  do {                                                                         \
    a2dp_get_need_sync = true;                                                 \
    if (a2dp_put_thread_tid) {                                                 \
      osSignalClear(a2dp_put_thread_tid, 0x80);                                \
      osSignalWait(0x80, A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_MS);                \
    }                                                                          \
  } while (0)

#define A2DP_SYNC_WITH_GET_MUTUX_SET()                                         \
  do {                                                                         \
    if (a2dp_get_need_sync) {                                                  \
      a2dp_get_need_sync = false;                                              \
      if (a2dp_put_thread_tid) {                                               \
        osSignalSet(a2dp_put_thread_tid, 0x80);                                \
      }                                                                        \
    }                                                                          \
  } while (0)

// #define A2DP_SYNC_WITH_PUT_MUTUX (1)
#define A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_CNT (1)
#define A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_MS (3)
static osThreadId a2dp_get_thread_tid = NULL;
static bool a2dp_get_put_sync = false;

#define A2DP_SYNC_WITH_PUT_MUTUX_ALLOC()                                       \
  do {                                                                         \
    if (a2dp_get_thread_tid == NULL) {                                         \
      a2dp_get_thread_tid = osThreadGetId();                                   \
    }                                                                          \
  } while (0)

#define A2DP_SYNC_WITH_PUT_MUTUX_FREE()                                        \
  do {                                                                         \
    a2dp_get_thread_tid = NULL;                                                \
  } while (0)

#define A2DP_SYNC_WITH_PUT_MUTUX_WAIT()                                        \
  do {                                                                         \
    a2dp_get_put_sync = true;                                                  \
    if (a2dp_get_thread_tid) {                                                 \
      osSignalClear(a2dp_get_thread_tid, 0x80);                                \
      osSignalWait(0x5, A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_MS);                 \
    }                                                                          \
  } while (0)

#define A2DP_SYNC_WITH_PUT_MUTUX_SET()                                         \
  do {                                                                         \
    if (a2dp_get_put_sync) {                                                   \
      a2dp_get_put_sync = false;                                               \
      if (a2dp_get_thread_tid) {                                               \
        osSignalSet(a2dp_get_thread_tid, 0x80);                                \
      }                                                                        \
    }                                                                          \
  } while (0)

int a2dp_audio_sbc_set_frame_info(int rcv_len, int frame_num) {
  if ((!rcv_len) || (!frame_num)) {
    return 0;
  }

  if (sbc_frame_rev_len != rcv_len || sbc_frame_num != frame_num) {
    sbc_frame_rev_len = rcv_len;
    sbc_frame_num = frame_num;
    sbc_frame_size = rcv_len / frame_num;
  }

  return 0;
}

extern struct BT_DEVICE_T app_bt_device;
extern uint8_t a2dp_channel_num[BT_DEVICE_NUM];
void expand_1_channel_to_2_channels_16bits(unsigned char *in,
                                           unsigned int in_len) {
  int cnt = 0;
  int len = in_len;
  short *ptr = (short *)in + in_len / 2;
  short *ptr_out = (short *)in + in_len;

  while (len > 0) {
    *(ptr_out - 2 * cnt - 1) = *(ptr - cnt);
    *(ptr_out - 2 * cnt - 2) = *(ptr - cnt);
    cnt += 1;
    len -= 2;
  }
}

#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
static enum A2DP_AUDIO_SYNC_STATUS sync_status =
    A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEFAULT;

static int a2dp_audio_sync_proc(uint8_t status, int shift) {
#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
  struct AF_STREAM_CONFIG_T *cfg;

  bool need_shift = false;
  static int cur_shift = 0;
  static int dest_shift = 0;

  LOCK_APP_AUDIO_QUEUE();
  if (status & A2DPPLAY_SYNC_STATUS_RESET) {
    sync_status = A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEFAULT;
    cur_shift = 0;
    dest_shift = 0;
  }
  if (status & A2DPPLAY_SYNC_STATUS_SET) {
    dest_shift = shift;
  }
  if (cur_shift > dest_shift) {
    cur_shift--;
    need_shift = true;
  }
  if (cur_shift < dest_shift) {
    cur_shift++;
    need_shift = true;
  }
  if (need_shift) {
    A2DP_AUDIO_SYNC_TRACE(1, "a2dp_audio_sync_proc shift:%d\n", cur_shift);
    uint32_t ret =
        af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &cfg, false);
    if (0 == ret) {
      af_codec_tune(AUD_STREAM_PLAYBACK,
                    A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_STEP * cur_shift);
    }
  }
  UNLOCK_APP_AUDIO_QUEUE();
#endif
  return 0;
}
#endif

bool a2dp_audio_isrunning(enum A2DPPLAY_STRTEAM_T stream, bool run) {
  static bool stream_running[A2DPPLAY_STRTEAM_QTY] = {false, false};

  if (stream >= A2DPPLAY_STRTEAM_QTY)
    return false;

  stream_running[stream] = run;

  if (stream_running[A2DPPLAY_STRTEAM_PUT] &&
      stream_running[A2DPPLAY_STRTEAM_GET])
    return true;
  else
    return false;
}

#if defined(A2DP_AAC_ON)
#define AAC_READBUF_SIZE (2048)

uint32_t aac_maxreadBytes = AAC_READBUF_SIZE;
uint32_t aac_frame_mute = 0;

int decode_aac_frame(unsigned char *pcm_buffer, unsigned int pcm_len);
#endif

#if defined(A2DP_LHDC_ON)
#include "hal_timer.h"
#define A2DP_LHDC_DEFAULT_LATENCY 1
uint8_t latencyIndex = 0;
uint8_t latencyUpdated = 0;

uint16_t latencyTable[] = {50, 150, 300};

uint8_t magicTag[] = {'l', 'h', 'd', 'c'};
typedef struct {
  uint8_t tag[4];
  uint32_t packetLen;
  uint8_t dptr[0];
} LHDC_HEADER;

typedef struct {
  uint16_t frame_len;
  bool isSplit;
  bool isLeft;
} LHDC_FRAME_HDR;

#define LHDC_READBUF_SIZE                                                      \
  1024 * 4 /* pick something big enough to hold a bunch of frames */
uint8_t lhdcTempBuf[LHDC_READBUF_SIZE];
uint8_t lhdc_input_mid_buf[LHDC_READBUF_SIZE];
#define L2CAP_MTU 672
#define PACKET_MTU_SIZE (L2CAP_MTU - 12)

void initial_lhdc_assemble_packet(bool splitFlg);
/**
 * get lhdc frame header
 */
bool get_lhdc_header(uint8_t *in, LHDC_FRAME_HDR *h);

uint32_t get_lhdc_frame(uint8_t *frame_q, size_t q_available_size,
                        uint8_t *out_buf, uint32_t *out_size,
                        uint32_t *out_frames);
/**
 *  Grabe lhdc data from queue
 */
int get_lhdc_data(unsigned char *frame, unsigned int len);
int assemble_lhdc_packet(uint8_t *input, uint32_t input_len, uint8_t **pLout,
                         uint32_t *pLlen, uint8_t **pRout, uint32_t *pRlen);

#endif
#if defined(A2DP_LDAC_ON)
#define LDAC_FRAME_LEN_INDEX_106 106
#define LDAC_FRAME_LEN_INDEX_128 128
#define LDAC_FRAME_LEN_INDEX_160 160
#define LDAC_FRAME_LEN_INDEX_216 216
#define LDAC_FRAME_LEN_INDEX_326 326

uint8_t get_ldac_frame_num(uint16_t frame_length_index) {
  uint8_t frame_num = 0;
  switch (frame_length_index) {
  case LDAC_FRAME_LEN_INDEX_106:
    frame_num = 6;
    break;
  case LDAC_FRAME_LEN_INDEX_128:
    frame_num = 5;
    break;
  case LDAC_FRAME_LEN_INDEX_160:
    frame_num = 4;
    break;
  case LDAC_FRAME_LEN_INDEX_216:
    frame_num = 3;
    break;
  case LDAC_FRAME_LEN_INDEX_326:
    frame_num = 2;
    break;
  default:
    ASSERT(0, "Unknown ldac frame format: %d !!!!!", frame_length_index);
    break;
  }
  return frame_num;
}
#endif

#define DELAY_MTU_LIMIT 8

static volatile int delay_mtu_limit = DELAY_MTU_LIMIT;
static volatile int mtu_count = 0;
static volatile int delay_mtu_count = 0;
bool app_bt_stream_trigger_onprocess(void);
void app_bt_stream_trigger_start(uint8_t offset);

void a2dp_audio_set_mtu_limit(uint8_t mut) { delay_mtu_limit = mut; }

static uint32_t old_t = 0;
static uint32_t ssb_err = 0;
static uint32_t ssb_err_max = 0;
static uint32_t ssb_err_min = 0xffffffff;
static uint32_t lagest_mtu_in1s = 0;
static uint32_t least_mtu_in1s = 0xffffffff;
static uint32_t check_interval = 50;

int store_sbc_buffer(unsigned char *buf, unsigned int len) {
  int POSSIBLY_UNUSED size;
  int cnt = 0;
  int nRet = 0;
#if defined(A2DP_LHDC_ON)
  uint32_t newLen = 0;
  bool mtu_plus = false;
#endif
#if defined(A2DP_LDAC_ON)
  uint16_t frame_length_index = 0;
#endif
  uint32_t now_t = TICKS_TO_MS(hal_sys_timer_get());
  ssb_err = now_t - old_t;
  old_t = now_t;

  if (ssb_err < 500) {
    if (ssb_err_max < ssb_err)
      ssb_err_max = ssb_err;
    if (ssb_err_min > ssb_err)
      ssb_err_min = ssb_err;
  }

  if (!a2dp_audio_isrunning(A2DPPLAY_STRTEAM_PUT, true)) {
    TRACE(3, "%s not ready:%d cache_status:%d", __func__, len,
          a2dp_cache_status);
  }
  uint8_t overlay_id = app_get_current_overlay();

  if (overlay_id == APP_OVERLAY_A2DP) {
    // TRACE(8,"sbc %d %x %x %x %x mtu_count:%d
    // sbcqueue:%d,sbc_frame_rev_len=%d", len, buf[0],
    //    buf[1],buf[2],buf[3],mtu_count,APP_AUDIO_LengthOfCQueue(&sbc_queue),sbc_frame_rev_len);
  }
#if defined(A2DP_LDAC_ON)
  else if (overlay_id == APP_OVERLAY_A2DP_LDAC) {
    uint16_t data1 = (uint16_t)(buf[1] & 0x07);
    uint16_t data2 = (uint16_t)buf[2];
    frame_length_index = ((data1 << 6 & 0xFFFF) | (data2 >> 2 & 0xFFFF));
    sbc_frame_num = get_ldac_frame_num(frame_length_index);
    // TRACE(10,"ldac %d %d %x %x %x %x mtu_count:%d
    // sbcqueue:%d,sbc_frame_rev_len=%d,sbc_frame_num %d", len,
    // sbc_frame_size,buf[0],
    //    buf[1],buf[2],buf[3],mtu_count,APP_AUDIO_LengthOfCQueue(&sbc_queue),sbc_frame_rev_len,sbc_frame_num);
  }
#endif
  else {
    // TRACE(8,"AAC %d %x %x %x %x mtu_count:%d
    // sbcqueue:%d,sbc_frame_rev_len=%d", len, buf[0],
    //    buf[1],buf[2],buf[3],mtu_count,APP_AUDIO_LengthOfCQueue(&sbc_queue),sbc_frame_rev_len);
  }

  switch (a2dp_cache_status) {
  case APP_AUDIO_CACHE_CACHEING: {
#if defined(A2DP_LHDC_ON)
    newLen = 0;
    if (overlay_id == APP_OVERLAY_A2DP_LHDC) {
      uint32_t lSize = 0, rSize = 0;
      uint8_t *lPTR = NULL, *rPTR = NULL;
      // TRACE(2,"%s:APP_AUDIO_CACHE_CACHEING Enter len = %d",__func__, len);
      // TRACE(2,"%s: input len(%d)",__func__, len);
      if (assemble_lhdc_packet(buf, len, &lPTR, &lSize, &rPTR, &rSize) > 0) {
        if (lPTR != NULL && lSize != 0) {
          newLen = lSize + sizeof(LHDC_HEADER);
          memcpy(lhdcTempBuf + sizeof(LHDC_HEADER), lPTR, lSize);
          memcpy(lhdcTempBuf, magicTag, sizeof(magicTag));
          memcpy(lhdcTempBuf + sizeof(magicTag), &lSize, sizeof(unsigned int));
          mtu_plus = true;
        }
      } else {
        nRet = 0;
      }
    }
#endif

    LOCK_APP_AUDIO_QUEUE();
#if defined(A2DP_LHDC_ON)
    if (overlay_id == APP_OVERLAY_A2DP_LHDC) {
      nRet = APP_AUDIO_EnCQueue(&sbc_queue, lhdcTempBuf, newLen);
      size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
    } else
#endif

#if defined(A2DP_LDAC_ON)
        if (overlay_id == APP_OVERLAY_A2DP_LDAC) {
      // if(bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
      nRet = APP_AUDIO_EnCQueue(&sbc_queue, buf, len);
      size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
    } else
#endif

#if defined(A2DP_AAC_ON)
        if (overlay_id == APP_OVERLAY_A2DP_AAC) {
      nRet = AvailableOfCQueue(&sbc_queue);
      if (nRet >= (int)(len + 2)) {
        nRet = APP_AUDIO_EnCQueue(&sbc_queue, (uint8_t *)&len, 2);
        nRet = APP_AUDIO_EnCQueue(&sbc_queue, buf, len);
      } else
        nRet = CQ_ERR;
    } else
#endif

    {
      nRet = APP_AUDIO_EnCQueue(&sbc_queue, buf, len);
    }
    size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
#if defined(A2DP_LHDC_ON)
    if (overlay_id == APP_OVERLAY_A2DP_LHDC) {
      if (mtu_plus) {
        if (delay_mtu_count == 0)
          mtu_count = 0;
        delay_mtu_count++;
        mtu_count++;
        mtu_plus = false;
      }
    } else {
#endif
      if (overlay_id == APP_OVERLAY_A2DP
#if defined(A2DP_LDAC_ON)
          || overlay_id == APP_OVERLAY_A2DP_LDAC
#endif
      ) {
        if (delay_mtu_count == 0)
          mtu_count = 0;
        delay_mtu_count += sbc_frame_num;
        mtu_count += sbc_frame_num;
        //					TRACE(1,"+mtu_count =
        //%d",mtu_count);
      } else {
        if (delay_mtu_count == 0)
          mtu_count = 0;
        delay_mtu_count++;
        mtu_count++;
      }
#if defined(A2DP_LHDC_ON)
    }
#endif
    UNLOCK_APP_AUDIO_QUEUE();
    bool flag = 0;
#if defined(A2DP_AAC_ON)
    if (overlay_id == APP_OVERLAY_A2DP_AAC)
      flag = 1;
#endif
#if defined(A2DP_SCALABLE_ON)
    if (overlay_id == APP_OVERLAY_A2DP_SCALABLE)
      flag = 1;
#endif
#if defined(A2DP_LHDC_ON)
    if (overlay_id == APP_OVERLAY_A2DP_LHDC)
      flag = 1;
#endif
#if defined(A2DP_LDAC_ON)
    if (overlay_id == APP_OVERLAY_A2DP_LDAC) {
      flag = 1;
      TRACE(0, "y# ldac cache");
    }
#endif

    if (flag) {
#ifdef __A2DP_PLAYER_USE_BT_TRIGGER__
      if (app_bt_stream_trigger_onprocess() && mtu_count) {
        TRACE(0, "cache ok use dma trigger\n");
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
        app_bt_stream_trigger_start(0);
      }
#else
      if (delay_mtu_count >= delay_mtu_limit) {
        TRACE(2, "aac cache ok:%d,mtu_count=%d\n", size, mtu_count);
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
      }
#endif
    } else {
#ifdef __A2DP_PLAYER_USE_BT_TRIGGER__
      if (app_bt_stream_trigger_onprocess() && mtu_count) {
        TRACE(0, "cache ok use dma trigger\n");
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
        app_bt_stream_trigger_start(0);
      }
#else
      if (delay_mtu_count >= delay_mtu_limit) {
        TRACE(4, "cache ok:%d,%d,%d,mtu_count=%d\n", size, len, sbc_frame_size,
              mtu_count);
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
      } else if (sbc_frame_size && (size >= A2DPPLAY_CACHE_OK_THRESHOLD)) {
        TRACE(2, "cache ok:%d,mtu_count=%d\n", size, mtu_count);
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
      } else {
        TRACE(1, "cache add:%d\n", len);
      }
#endif
    }

    if (nRet == CQ_ERR) {
      TRACE(0, "cache add overflow\n");
      a2dp_cache_status = APP_AUDIO_CACHE_OK;
    }
    if (a2dp_cache_status == APP_AUDIO_CACHE_OK) {
      old_t = 0;
      ssb_err = 0;
      ssb_err_max = 0;
      ssb_err_min = 0xffffffff;
      lagest_mtu_in1s = 0;
      least_mtu_in1s = 0xffffffff;
      check_interval = 50;
#ifdef __LOCK_AUDIO_THREAD__
      af_unlock_thread();
#endif
      A2DP_SYNC_WITH_GET_MUTUX_ALLOC();
      A2DP_SYNC_WITH_GET_MUTUX_WAIT();
#ifdef __LOCK_AUDIO_THREAD__
      af_lock_thread();
#endif
    }
    break;
  }
  case APP_AUDIO_CACHE_OK: {
    delay_mtu_count = 0;
#if defined(A2DP_LHDC_ON)
    newLen = 0;
    if (overlay_id == APP_OVERLAY_A2DP_LHDC) {
      uint32_t lSize = 0, rSize = 0;
      uint8_t *lPTR = NULL, *rPTR = NULL;
      // TRACE(2,"%s: input len(%d)",__func__, len);
      if (assemble_lhdc_packet(buf, len, &lPTR, &lSize, &rPTR, &rSize) > 0) {
        if (lPTR != NULL && lSize != 0) {
          newLen = lSize + sizeof(LHDC_HEADER);
          memcpy(lhdcTempBuf + sizeof(LHDC_HEADER), lPTR, lSize);
          memcpy(lhdcTempBuf, magicTag, sizeof(magicTag));
          memcpy(lhdcTempBuf + sizeof(magicTag), &lSize, sizeof(unsigned int));
          mtu_plus = true;
        }
      }
    }
#endif
    do {
      LOCK_APP_AUDIO_QUEUE();
#if defined(A2DP_LHDC_ON)
      if (overlay_id == APP_OVERLAY_A2DP_LHDC) {
        nRet = APP_AUDIO_EnCQueue(&sbc_queue, lhdcTempBuf, newLen);
      } else
#endif
#if defined(A2DP_AAC_ON)
          if (overlay_id == APP_OVERLAY_A2DP_AAC) {
        nRet = AvailableOfCQueue(&sbc_queue);
        if (nRet >= (int)(len + 2)) {
          nRet = APP_AUDIO_EnCQueue(&sbc_queue, (uint8_t *)&len, 2);
          nRet = APP_AUDIO_EnCQueue(&sbc_queue, buf, len);
        } else
          nRet = CQ_ERR;
      } else
#endif

      {
        nRet = APP_AUDIO_EnCQueue(&sbc_queue, buf, len);
      }
      if (CQ_OK == nRet) {
#if defined(A2DP_LHDC_ON)
        if (overlay_id == APP_OVERLAY_A2DP_LHDC) {
          if (mtu_plus) {
            mtu_count++;
            mtu_plus = false;
          }
        } else {
#endif
          if (overlay_id == APP_OVERLAY_A2DP
#if defined(A2DP_LDAC_ON)
              || overlay_id == APP_OVERLAY_A2DP_LDAC
#endif
          ) {
            mtu_count += sbc_frame_num;
            //							TRACE(1,"+mtu_count
            //= %d",mtu_count);
          } else {
            mtu_count++;
          }
#if defined(A2DP_LHDC_ON)
        }
#endif
      }
      //              size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
      UNLOCK_APP_AUDIO_QUEUE();
      //                TRACE(3,"cache add:%d %d/%d \n", len, size,
      //                g_sbc_queue_size);
      if (CQ_OK == nRet) {
        nRet = 0;
        break;
      } else {
        TRACE(1, "cache flow control:%d\n", cnt);
#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET, 0);
#endif
        nRet = -1;
#ifdef __LOCK_AUDIO_THREAD__
        af_unlock_thread();
#endif
        A2DP_SYNC_WITH_GET_MUTUX_ALLOC();
        A2DP_SYNC_WITH_GET_MUTUX_WAIT();
#ifdef __LOCK_AUDIO_THREAD__
        af_lock_thread();
#endif
      }
    } while (cnt++ < A2DP_SYNC_WITH_GET_MUTUX_TIMEROUT_CNT);
    break;
  }
  case APP_AUDIO_CACHE_QTY:
  default:
    break;
  }
#ifdef A2DP_SYNC_WITH_PUT_MUTUX
  A2DP_SYNC_WITH_PUT_MUTUX_SET();
#endif

  if (nRet) {
    TRACE(0, "cache overflow\n");
  }
  return 0;
}

#if defined(A2DP_LDAC_ON)

#define LDAC_READBUF_SIZE                                                      \
  1024 /* pick something big enough to hold a bunch of frames */
uint8_t ldac_input_mid_buf[LDAC_READBUF_SIZE + 2];

/*
 *  Grabe lhdc data from queue
 */
TEXT_LDAC_LOC
int get_ldac_data(unsigned char *frame, unsigned int len) {
  int status;
  unsigned int len1 = 0, len2 = 0;
  unsigned char *e1 = NULL, *e2 = NULL;
  LOCK_APP_AUDIO_QUEUE();
  unsigned int queu_len = (unsigned int)LengthOfCQueue(&sbc_queue);
  len = queu_len < len ? queu_len : len;
  status = PeekCQueue(&sbc_queue, len, &e1, &len1, &e2, &len2);
  UNLOCK_APP_AUDIO_QUEUE();
  if (status != CQ_OK) {
    return 0;
  }

  if (e1) {
    memcpy(frame, e1, len1);
  }
  if (e2) {
    memcpy(frame + len1, e2, len2);
  }
  return len;
}

/**
 * Decode LDAC data...
 */
// #include "os_tcb.h"
extern const char *get_error_code_string(int error_code);
extern int app_audio_mempool_force_set_buff_used(uint32_t size);
extern uint32_t ldac_buffer_used;
uint32_t skip_ldac_frame = 2;
static bool ldac_recovery;

TEXT_LDAC_LOC
int ldac_dec_init(void) {
  if (hLdacData) {
    return 0;
  }

  int sample_rate = bt_get_ladc_sample_rate();
  int channel_mode = bt_ldac_player_get_channelmode();
  TRACE(2, "a2dp_audio_init sample Rate=%d, channel_mode = %d\n", sample_rate,
        channel_mode);
  // TRACE(1,"sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(0));
  TRACE(0, "ldac need init here!!!! \n");
  if ((hLdacData = ldacBT_get_handle()) == (HANDLE_LDAC_BT)NULL) {
    TRACE(0, "Error: Can not Get LDAC Handle!\n");
    return 1;
  }
  int result =
      ldacBT_init_handle_decode(hLdacData, channel_mode, sample_rate, 0, 0, 0);
  if (result) {
    TRACE(1, "[ERR] Initializing LDAC Handle for synthesis! Error code %s\n",
          get_error_code_string(ldacBT_get_error_code(hLdacData)));
    return 2;
  }
  return 0;
}

TEXT_LDAC_LOC
int ldac_dec_deinit(void) {
  ldacBT_free_handle(hLdacData);
  hLdacData = NULL;
  app_audio_mempool_force_set_buff_used(ldac_buffer_used);
  return 0;
}

TEXT_LDAC_LOC
int ldac_seek_header(uint32_t n_read_bytes, uint32_t *p_parse_bytes) {
  int channel_mode = 0;
  int sample_rate = 0;

  if (p_parse_bytes) {
    *p_parse_bytes = 0;
  }

  if (n_read_bytes < 2) {
    return 1;
  }

  /* update used_bytes to skip current ldac frame */
  sample_rate = bt_get_ladc_sample_rate();
  channel_mode = bt_ldac_player_get_channelmode();

  if (1) {
    unsigned char *pStream, *p1st, *p2nd, *pTail;
    unsigned char sync2 = 0;
    unsigned char cci = 0;
    p1st = p2nd = NULL;
    pStream = ldac_input_mid_buf;
    pTail = ldac_input_mid_buf + n_read_bytes - 2;
    switch (channel_mode) {
    case LDACBT_CHANNEL_MODE_MONO:
      cci = LDAC_CCI_MONO;
      break;
    case LDACBT_CHANNEL_MODE_DUAL_CHANNEL:
      cci = LDAC_CCI_DUAL_CHANNEL;
      break;
    case LDACBT_CHANNEL_MODE_STEREO:
    default:
      cci = LDAC_CCI_STEREO;
      break;
    }
    if (0) {
      ;
    } else if (sample_rate == 1 * 44100) {
      sync2 = (0 << 5) | (cci << 3);
    } else if (sample_rate == 1 * 48000) {
      sync2 = (1 << 5) | (cci << 3);
    } else if (sample_rate == 2 * 44100) {
      sync2 = (2 << 5) | (cci << 3);
    } else if (sample_rate == 2 * 48000) {
      sync2 = (3 << 5) | (cci << 3);
    }
    while (pStream < pTail) {
      if (pStream[0] == 0xAA) { /* syncword */
        if ((pStream[1] & 0xF8) == sync2) {
          if (p1st == NULL) {
            p1st = pStream;
          }      /* 1st syncword found */
          else { /* 2nd syncword found, this frame must decode next time */
            p2nd = pStream;
            if (p_parse_bytes) {
              *p_parse_bytes = pStream - ldac_input_mid_buf;
            }
            break;
          }
        }
      }
      ++pStream;
    }
    if (p2nd == NULL) {
      /* faild to find next frame header */
      TRACE(0, "Error: Can not get next LDAC frame_header!\n");
      return 1;
    }
  }

  return 0;
}

TEXT_LDAC_LOC
int load_ldac_frame(uint8_t **p_data, uint32_t *p_len) {
#if 1
  uint8_t retry = 0;
  int len;
  int result = 0;

  if (hLdacData == NULL) {
    return 1;
  }

  while (true) {
    len = get_ldac_data(ldac_input_mid_buf, LDAC_READBUF_SIZE);
    if (len == 0) {
      if (retry++ >= A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_CNT) {
        TRACE(0, "ldac No Data return");
        result = 1;
        break;
      }
      continue;
    }

    if (ldac_recovery) {
      uint32_t parse_bytes;

      result = ldac_seek_header(len, &parse_bytes);
      if (result == 0) {
        ldac_recovery = false;
      }
      LOCK_APP_AUDIO_QUEUE();
      DeCQueue(&sbc_queue, 0, parse_bytes);
      UNLOCK_APP_AUDIO_QUEUE();
      continue;
    }

    break;
  }

  if (skip_ldac_frame > 0) {
    skip_ldac_frame--;
  }
#endif

  return result;
}

TEXT_LDAC_LOC
int decode_ldac_frame(uint8_t *out, uint32_t out_max, uint32_t *p_out_len,
                      const uint8_t *in, uint32_t in_len,
                      uint32_t *p_consume_len) {
  int result;
  int n_read_bytes, used_bytes, wrote_bytes;

  n_read_bytes = LDAC_READBUF_SIZE;

  result =
      ldacBT_decode(hLdacData, ldac_input_mid_buf, out, LDACBT_SMPL_FMT_S16,
                    n_read_bytes, &used_bytes, &wrote_bytes);

  if (p_out_len) {
    *p_out_len = wrote_bytes;
  }
  if (p_consume_len) {
    *p_consume_len = used_bytes;
  }

  return result;
}

TEXT_LDAC_LOC
int consume_ldac_frame(const uint8_t *data, uint32_t len) {
  LOCK_APP_AUDIO_QUEUE();
  DeCQueue(&sbc_queue, 0, len);
  mtu_count--;
  UNLOCK_APP_AUDIO_QUEUE();

  return 0;
}

TEXT_LDAC_LOC
void decode_ldac_end(uint32_t expect_out_len, uint32_t out_len,
                     uint32_t in_len) {
  if (expect_out_len != out_len) {
    if (hLdacData) {
      int error_code;
      error_code = ldacBT_get_error_code(hLdacData);
      TRACE(6, "error_code = %4d, %4d, %4d,FrameHeader[%02x:%02x:%02x]\n",
            LDACBT_API_ERR(error_code), LDACBT_HANDLE_ERR(error_code),
            LDACBT_BLOCK_ERR(error_code), ldac_input_mid_buf[0],
            ldac_input_mid_buf[1], ldac_input_mid_buf[2]);
      if (LDACBT_FATAL(error_code)) {
        ldac_dec_deinit();
      }
    }
    if (hLdacData == NULL) {
      /* Recovery Process */
      int result;
      result = ldac_dec_init();
      if (result) {
        ldac_dec_deinit();
      }
      ldac_recovery = true;
    }
  }
}
#endif

#if defined(A2DP_AAC_ON)
#include "aacdecoder_lib.h"
#include "aacenc_lib.h"
#include "heap_api.h"
#define DECODE_AAC_PCM_FRAME_LENGTH (1024 * 4)

HANDLE_AACDECODER aacDec_handle = NULL;
#define AAC_MEMPOLL_SIZE (40596)
heap_handle_t aac_memhandle = NULL;
uint8_t *aac_mempoll = NULL;

static unsigned char aac_input_mid_buf[AAC_READBUF_SIZE];
STATIC_ASSERT(sizeof(aac_input_mid_buf) >= 2048, "aac_input_mid_buf too small");
static uint32_t byte_in_buffer = 0;

int aac_meminit() {
  int ret = 1;
  if (aac_mempoll == NULL)
    ret =
        app_audio_mempool_get_buff((uint8_t **)&aac_mempoll, AAC_MEMPOLL_SIZE);
  if (ret > 0 && aac_memhandle == NULL) {
    aac_memhandle = heap_register(aac_mempoll, AAC_MEMPOLL_SIZE);
  }
  return ret;
}

void aac_memdeinit() {
  extern int total_calloc;
  aac_mempoll = 0;
  aac_memhandle = 0;
  total_calloc = 0;
  byte_in_buffer = 0;
}

volatile int aac_slave_codec_patch_bytes = 0;

int aacdec_init(void) {
  if (aacDec_handle == NULL) {
    TRANSPORT_TYPE transportFmt = TT_MP4_LATM_MCP1;
    aacDec_handle = aacDecoder_Open(transportFmt, 1 /* nrOfLayers */);

    if (!aacDec_handle) {
      TRACE(1, "%s Error initializing AAC decoder", __func__);
      return 1;
    }
    aacDecoder_SetParam(aacDec_handle, AAC_PCM_LIMITER_ENABLE, 0);
    aacDecoder_SetParam(aacDec_handle, AAC_DRC_ATTENUATION_FACTOR, 0);
    aacDecoder_SetParam(aacDec_handle, AAC_DRC_BOOST_FACTOR, 0);
  }
  return 0;
}

int aacdec_deinit(void) {
  if (aacDec_handle != NULL) {
    aacDecoder_Close(aacDec_handle);
    aacDec_handle = NULL;
    size_t total = 0, used = 0, max_used = 0;
    heap_memory_info(aac_memhandle, &total, &used, &max_used);
    TRACE(3, "AAC MALLOC MEM: total - %d, used - %d, max_used - %d.", total,
          used, max_used);
  }
  return 0;
}

TEXT_AAC_LOC
static int pcm_buffer_read_no_protction(CQueue *queue, uint8_t *buff,
                                        uint16_t len) {
  uint8_t *e1 = NULL, *e2 = NULL;
  unsigned int len1 = 0, len2 = 0;
  int status;
  status = PeekCQueue(queue, len, &e1, &len1, &e2, &len2);
  // TRACE(5,"pcm_buffer_read_no_protction
  // len=%d,len1=%d,len2=%d,e1=0x%x,e2=0x%x",len,len1,len2,e1,e2);
  if (len == (len1 + len2)) {
    memcpy(buff, e1, len1);
    memcpy(buff + len1, e2, len2);
    DeCQueue(queue, 0, len);
  } else {
    memset(buff, 0x00, len);
    status = -1;
  }
  return status;
}

TEXT_AAC_LOC
int load_aac_frame(uint8_t **p_data, uint32_t *p_len) {
  // uint32_t lock;
  unsigned short aac_len = 0;
  int status;

  LOCK_APP_AUDIO_QUEUE();
  status = pcm_buffer_read_no_protction(&sbc_queue, (uint8_t *)&aac_len, 2);
  if (status != CQ_OK) {
    UNLOCK_APP_AUDIO_QUEUE();
    // a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
    return 1;
  }

  if (aac_len < 64) {
    aac_maxreadBytes = 64;
  } else if (aac_len < 128) {
    aac_maxreadBytes = 128;
  } else if (aac_len < 256) {
    aac_maxreadBytes = 256;
  } else if (aac_len < 512) {
    aac_maxreadBytes = 512;
  } else if (aac_len < 1024) {
    aac_maxreadBytes = 1024;
  } else {
    if (aac_len >= 2048) {
      // Header is bad!
      dec_reset_queue = true;
      return 2;
    }
    aac_maxreadBytes = 2048;
  }

  status = pcm_buffer_read_no_protction(&sbc_queue, aac_input_mid_buf, aac_len);
  if (status != CQ_OK) {
    UNLOCK_APP_AUDIO_QUEUE();
    // a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
    return 1;
  }
  mtu_count--;
  UNLOCK_APP_AUDIO_QUEUE();

  // uint32_t a2dp_queue_bytes = LengthOfCQueue(&sbc_queue);
  // TRACE(3,"d aac_len=%d, a2dp_bytes=%d, aac_maxreadBytes=%d\n", aac_len,
  // a2dp_queue_bytes, aac_maxreadBytes);

  if (p_data) {
    *p_data = (unsigned char *)aac_input_mid_buf;
  }
  if (p_len) {
    *p_len = aac_maxreadBytes;
  }

  return 0;
}

TEXT_AAC_LOC
int decode_aac_frame(uint8_t *out, uint32_t out_max, uint32_t *p_out_len,
                     const uint8_t *in, uint32_t in_len,
                     uint32_t *p_consume_len) {
  unsigned int bufferSize = in_len;
  unsigned int bytesValid = in_len;
  AAC_DECODER_ERROR err = AAC_DEC_OK;
  CStreamInfo *info = NULL;

  // at leat should large than AAC_MAX_NSAMPS*2  (one channel)
  if (out_max < 2048) {
    TRACE(2, "%s daac pcm_len = %d \n", __func__, out_max);
    return 1;
  }

  if (a2dp_channel_num[app_bt_device.curr_a2dp_stream_id] == 1) {
    out_max >>= 1;
  }

  // int tt = hal_sys_timer_get();
  err =
      aacDecoder_Fill(aacDec_handle, (uint8_t **)&in, &bufferSize, &bytesValid);
  if (err != AAC_DEC_OK) {
    TRACE(1, "aacDecoder_Fill failed:0x%x", err);
    // if aac failed reopen it again
    if (is_aacDecoder_Close(aacDec_handle)) {
      aacDec_handle = NULL;
      aacdec_init();
      TRACE(1, "%s reopen aac codec \n", __func__);
    }
    return 2;
  }
  /* decode one AAC frame */
  err = aacDecoder_DecodeFrame(aacDec_handle, (short *)out, out_max / 2,
                               0 /* flags */);

#if 0
    TRACE_IMM(0," ");
    for(uint32_t i=0;i<aac_len;i=i+32)
    {
        DUMP8("%02x", &(aac_input_mid_buf[i]), 32);
    }
#endif

  if (err != AAC_DEC_OK) {
    TRACE(1, "aacDecoder_DecodeFrame failed:0x%x", err);
    // if aac failed reopen it again
    if (is_aacDecoder_Close(aacDec_handle)) {
      aacDec_handle = NULL;
      aacdec_init();
      TRACE(1, "%s reopen aac codec \n", __func__);
    }
    return 3;
  }

  info = aacDecoder_GetStreamInfo(aacDec_handle);

  if (!info || info->sampleRate <= 0) {
    TRACE(2, "%s Invalid stream info %d", __func__, info->sampleRate);
    return 4;
  }
  int frame_len =
      info->frameSize * info->numChannels * 2; // sizeof(pcm_buffer[0]);
  // TRACE(4,"aac: %d,frameSize=%d,numChannels=%d use %d ms\n",
  // frame_len,info->frameSize,info->numChannels,TICKS_TO_MS(hal_sys_timer_get()-tt));

  if (p_out_len) {
    *p_out_len = frame_len;
  }
  if (p_consume_len) {
    *p_consume_len = in_len - bytesValid;
  }

  if (a2dp_channel_num[app_bt_device.curr_a2dp_stream_id] == 1) {
    expand_1_channel_to_2_channels_16bits(out, frame_len);
    *p_out_len = frame_len << 1;
  }
  return 0;
}
#endif

#ifdef A2DP_EQ_24BIT
static void convert_16bit_to_24bit(int32_t *out, int16_t *in, int len) {
  for (int i = len - 1; i >= 0; i--) {
    out[i] = ((int32_t)in[i] << 8);
  }
}
#endif

#if defined(A2DP_SCALABLE_ON)

extern "C" {
#include "ssc.h"
}

unsigned char *scalable_input_mid_buf = NULL;
unsigned char *scalable_decoder_place = NULL;
unsigned char *scalable_decoder_temp_buf = NULL;
void *hSSDecoder = NULL;

#if 0
uint8_t ss_dump[484*10];
uint32_t ss_dump_index = 0;
#endif
extern "C" int ssc_decoder_init(void *s, int channels, int Fs);

TEXT_SSC_LOC
void ss_to_24bit_buf(int32_t *out, int32_t *in, int size) {
  for (int i = 0; i < size; i++) {
    out[i] = in[i];
  }
}

TEXT_SSC_LOC
void ss_to_16bit_buf(int16_t *out, int32_t *in, int size) {
  for (int i = 0; i < size; i++) {
    out[i] = in[i];
  }
}

static int ss_pcm_buff[SCALABLE_FRAME_SIZE << 1];
static int scalable_uhq_flag = 0;

TEXT_SSC_LOC
int load_scalable_frame(uint8_t **p_data, uint32_t *p_len) {
  int hw_tmp, len, bitrate_bps, frame_size;
  int r = 0;
  unsigned char *e1 = NULL, *e2 = NULL;
  unsigned int len1 = 0, len2 = 0;
  int sampling_rate = 44100;
  int extends_flag;

  TRACE(0, "##decode_scalable_frame");
  uint8_t head[4];

  LOCK_APP_AUDIO_QUEUE();
  len1 = len2 = 0;
  e1 = e2 = 0;
  r = PeekCQueue(&sbc_queue, SCALABLE_HEAD_SIZE, &e1, &len1, &e2, &len2);
  UNLOCK_APP_AUDIO_QUEUE();
  if (r == CQ_ERR) {
    // osDelay(2);
    TRACE(2, "no data head xxx %d/%d", LengthOfCQueue(&sbc_queue),
          AvailableOfCQueue(&sbc_queue));
    // goto get_scalable_head_again;
    LOCK_APP_AUDIO_QUEUE();
    DeCQueue(&sbc_queue, head, 4);
    UNLOCK_APP_AUDIO_QUEUE();
    hal_trace_dump("sss %01x", 1, 4, head);
    return 1;
  } else {
    // normal
    if (e1) {
      memcpy(scalable_input_mid_buf, e1, len1);
    }
    if (e2) {
      memcpy(scalable_input_mid_buf + len1, e2, len2);
    }
  }

  LOCK_APP_AUDIO_QUEUE();
  DeCQueue(&sbc_queue, 0, 4);
  UNLOCK_APP_AUDIO_QUEUE();

  scalable_uhq_flag = 0;

  extends_flag = ((scalable_input_mid_buf[3] >> 3) & 1);
  switch ((scalable_input_mid_buf[3] & 0xf7)) {
  case 0xF0:
    bitrate_bps = 88000;
    break;
  case 0xF1:
    bitrate_bps = 96000;
    break;
  case 0xF2:
    bitrate_bps = 128000;
    break;
  case 0xF3:
    bitrate_bps = 192000;
    break;
  case 0xF5:
    scalable_uhq_flag = 1;
    bitrate_bps = 328000;
    sampling_rate = 96000;
    break;
  default:
    bitrate_bps = 88000;
    break;
  }

  frame_size = SCALABLE_FRAME_SIZE;

  len = bitrate_bps * frame_size / sampling_rate / 8;
  if (scalable_uhq_flag == 0) {
    hw_tmp = (len * 3) >> 7;
    len = hw_tmp + len;
    len = len + ((len & 1) ^ 1);
  } else {
    len = 369; // 744/2-4+1
  }
  TRACE(4, "len %d,uhq:%d ext:%d extbitrate_bps %d", len, scalable_uhq_flag,
        extends_flag, bitrate_bps);

  LOCK_APP_AUDIO_QUEUE();
  len1 = len2 = 0;
  e1 = e2 = 0;
  r = PeekCQueue(&sbc_queue, len - 1, &e1, &len1, &e2, &len2);
  if (r == CQ_ERR) {
    // osDelay(2);
    TRACE(0, "no data ");
    UNLOCK_APP_AUDIO_QUEUE();
    return 2;
  } else {
    // normal
    if (e1) {
      memcpy(scalable_input_mid_buf + 4, e1, len1);
    }
    if (e2) {
      memcpy(scalable_input_mid_buf + 4 + len1, e2, len2);
    }
  }
  //	UNLOCK_APP_AUDIO_QUEUE();

  //   LOCK_APP_AUDIO_QUEUE();
  DeCQueue(&sbc_queue, 0, len - 1);
  UNLOCK_APP_AUDIO_QUEUE();
  //    TRACE(2,"len1 %d, len2 %d\n", len1, len2);

  if (p_data) {
    *p_data = scalable_input_mid_buf;
  }
  if (p_len) {
    *p_len = len - 1;
  }

  return 0;
}

TEXT_SSC_LOC
int decode_scalable_frame(uint8_t *out, uint32_t out_max, uint32_t *p_out_len,
                          const uint8_t *in, uint32_t in_len,
                          uint32_t *p_consume_len) {
  uint32_t out_len;
  int decoder_size;
  int output_samples;
  int err;

  out_len = SCALABLE_FRAME_SIZE * 2;

  if (hSSDecoder == NULL) {
    hSSDecoder = (void *)scalable_decoder_place;
    // err = ssc_decoder_create(44100, 2, hSSDecoder);
    decoder_size = ssc_decoder_get_size(2, 96000);
    err = ssc_decoder_init(hSSDecoder, 2, 96000);
    TRACE(2, "decoder_size %d init ret %d\n", decoder_size, err);
  }

  if (a2dp_channel_num[app_bt_device.curr_a2dp_stream_id] == 1) {
    ASSERT(0, "UHQ should check audio channel number");
  }
  output_samples =
      ssc_decode(hSSDecoder, scalable_input_mid_buf, (int *)ss_pcm_buff,
                 SCALABLE_FRAME_SIZE, scalable_decoder_temp_buf);
  if (scalable_uhq_flag) {
    ss_to_24bit_buf((int32_t *)out, (int32_t *)ss_pcm_buff, out_len);
    out_len *= 4;
  } else {
    ss_to_16bit_buf((int16_t *)out, (int32_t *)ss_pcm_buff, out_len);
    out_len *= 2;
  }

  TRACE(2, "pcm_len %d o:%d", out_len, output_samples);

  if (p_out_len) {
    *p_out_len = out_len;
  }
  if (p_consume_len) {
    *p_consume_len = 0;
  }

  return 0;
}
#endif

static unsigned int sbc_next_frame_size;
static btif_sbc_pcm_data_t sbc_pcm_data;
static uint8_t sbc_underflow;

TEXT_SBC_LOC
void decode_sbc_begin(void) { sbc_underflow = 0; }

TEXT_SBC_LOC
int load_sbc_frame(uint8_t **p_data, uint32_t *p_len) {
  unsigned char retry = 0;
  int r = 0;
  unsigned char *e1 = NULL, *e2 = NULL;
  unsigned int len1 = 0, len2 = 0;

  if (!sbc_next_frame_size) {
    sbc_next_frame_size = sbc_frame_size;
  }

  if (need_init_decoder) {
    sbc_next_frame_size = sbc_frame_size;
  }

get_again:
  LOCK_APP_AUDIO_QUEUE();
  len1 = len2 = 0;
  r = PeekCQueue(&sbc_queue, sbc_next_frame_size, &e1, &len1, &e2, &len2);
  UNLOCK_APP_AUDIO_QUEUE();
  if (r == CQ_ERR) {
#ifdef __LOCK_AUDIO_THREAD__
    TRACE(1, "cache sbc_underflow retry:%d\n", retry);
    goto exit;
#elif defined(A2DP_SYNC_WITH_PUT_MUTUX)
    int size;
    A2DP_SYNC_WITH_PUT_MUTUX_ALLOC();
    A2DP_SYNC_WITH_PUT_MUTUX_WAIT();
    if (retry++ < A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_CNT) {
      goto get_again;
    } else {
      LOCK_APP_AUDIO_QUEUE();
      size = LengthOfCQueue(&sbc_queue);
      UNLOCK_APP_AUDIO_QUEUE();
      sbc_underflow = 1;
      TRACE(2, "cache sbc_underflow size:%d retry:%d\n", size, retry);
      goto exit;
    }
#else
#if 1
    TRACE(1, "cache sbc_underflow retry:%d\n", retry);
    goto exit;
#else
    int size;
    osDelay(2);
    LOCK_APP_AUDIO_QUEUE();
    size = LengthOfCQueue(&sbc_queue);
    UNLOCK_APP_AUDIO_QUEUE();
    if (retry++ < 12) {
      goto get_again;
    } else {
      sbc_underflow = 1;
      TRACE(2, "cache sbc_underflow size:%d retry:%d\n", size, retry);
      goto exit;
    }
#endif
#endif
  } else {
    retry = 0;
  }

  if (!len1) {
    TRACE(2, "len1 sbc_underflow %d/%d\n", len1, len2);
    goto get_again;
  }

  if (p_data) {
    *p_data = e1;
  }
  if (p_len) {
    *p_len = len1;
  }

  return 0;

exit:
  return 1;
}

TEXT_SBC_LOC
int decode_sbc_frame(uint8_t *out, uint32_t out_max, uint32_t *p_out_len,
                     const uint8_t *in, uint32_t in_len,
                     uint32_t *p_consume_len) {
  uint16_t parse_len;
  static uint16_t parse_len_total_frame = 0;
  bt_status_t ret;

  if (need_init_decoder) {
    parse_len_total_frame = 0;
    btif_sbc_init_decoder(sbc_decoder);
  }

  need_init_decoder = false;

  sbc_pcm_data.data = out;
  sbc_pcm_data.dataLen = 0;

  if (a2dp_channel_num[app_bt_device.curr_a2dp_stream_id] == 1) {
    out_max >>= 1;
  }

  uint32_t lock = int_lock();
  ret = btif_sbc_decode_frames(sbc_decoder, (uint8_t *)in, in_len, &parse_len,
                               &sbc_pcm_data, out_max, sbc_eq_band_gain);
  int_unlock(lock);
  parse_len_total_frame += parse_len;
  if (parse_len_total_frame >= sbc_frame_size) {
    mtu_count--;
    //		TRACE(1,"-mtu_count = %d",mtu_count);
    parse_len_total_frame -= sbc_frame_size;
  }
  if (ret == BT_STS_SUCCESS) {
    sbc_next_frame_size = sbc_frame_size;
  } else {
    sbc_next_frame_size = (sbc_frame_size > parse_len)
                              ? (sbc_frame_size - parse_len)
                              : sbc_frame_size;

    if (ret == BT_STS_FAILED) {
      need_init_decoder = true;
      TRACE(1, "err mutelen:%d\n", sbc_pcm_data.dataLen);
    } else if (ret == BT_STS_NO_RESOURCES) {
      TRACE(1, "no_res mutelen:%d\n", sbc_pcm_data.dataLen);
    }
  }

  if (p_out_len) {
    *p_out_len = sbc_pcm_data.dataLen;
  }
  if (p_consume_len) {
    *p_consume_len = parse_len;
  }

  if (a2dp_channel_num[app_bt_device.curr_a2dp_stream_id] == 1) {
    expand_1_channel_to_2_channels_16bits(sbc_pcm_data.data,
                                          sbc_pcm_data.dataLen);
    sbc_pcm_data.dataLen <<= 1;
    *p_out_len = sbc_pcm_data.dataLen;
  }

  return (ret == BT_STS_CONTINUE || ret == BT_STS_SUCCESS) ? 0 : 1;
}

TEXT_SBC_LOC
int consume_sbc_frame(const uint8_t *data, uint32_t len) {
  LOCK_APP_AUDIO_QUEUE();
  DeCQueue(&sbc_queue, 0, len);
  UNLOCK_APP_AUDIO_QUEUE();

  return 0;
}

TEXT_SBC_LOC
void decode_sbc_end(uint32_t expect_out_len, uint32_t out_len,
                    uint32_t in_len) {
#ifndef A2DP_TRACE_DEC_TIME
  if (sbc_underflow || need_init_decoder) {
    dec_trace_time = true;
  }
#endif

#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
  if (expect_out_len == out_len) {
    int size;
    LOCK_APP_AUDIO_QUEUE();
    size = LengthOfCQueue(&sbc_queue);
    UNLOCK_APP_AUDIO_QUEUE();
    //        TRACE(2,"decode_sbc_frame Queue remain size:%d
    //        frame_size:%d\n",size, sbc_frame_size);
    A2DP_AUDIO_SYNC_TRACE(6,
                          "sync status:%d qsize:%d/%d fsize:%d, thr:%d used:%d",
                          sync_status, size, g_sbc_queue_size, sbc_frame_size,
                          A2DPPLAY_CACHE_OK_THRESHOLD, in_len);
    /*
    nor:
        if (size>7488+1170*2)
            goto inc
        if (size<7488-1170)
            goto dec
    dec:
        if (size>=7488+1170)
            goto nor
    inc:
        if (size<=7488)
            goto nor
    */
    switch (sync_status) {
    case A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEC:
      if (size >= (A2DPPLAY_CACHE_OK_THRESHOLD + in_len)) {
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET,
                             A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_DEFAULT);
        sync_status = A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEFAULT;
        A2DP_AUDIO_SYNC_TRACE(0, "default pll freq");
      } else {
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_PROC, 0);
      }
      break;
    case A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_INC:
      if (size <= (A2DPPLAY_CACHE_OK_THRESHOLD)) {
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET,
                             A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_DEFAULT);
        sync_status = A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEFAULT;
        A2DP_AUDIO_SYNC_TRACE(0, "default pll freq");
      } else {
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_PROC, 0);
      }
      break;
    case A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEFAULT:
    default:
      if (size > (A2DPPLAY_CACHE_OK_THRESHOLD + (in_len << 1))) {
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET,
                             A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_INC);
        sync_status = A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_INC;
        A2DP_AUDIO_SYNC_TRACE(0, "inc pll freq");
      } else if (size < (A2DPPLAY_CACHE_OK_THRESHOLD - in_len)) {
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET,
                             A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_DEC);
        sync_status = A2DP_AUDIO_SYNC_STATUS_SAMPLERATE_DEC;
        A2DP_AUDIO_SYNC_TRACE(0, "dec pll freq");
      } else {
        a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_PROC, 0);
      }
      break;
    }
  }
#endif
}

#if defined(A2DP_LHDC_ON)
#define PACKET_BUFFER_LENGTH 4 * 1024

uint8_t serial_no;
bool is_synced;
bool is_splited;
ASM_PKT_STATUS asm_pkt_st;
uint8_t packet_buffer[PACKET_BUFFER_LENGTH];
uint32_t packet_buf_len = 0;
uint32_t frames_nb_in_buffer = 0;
uint32_t total_frame_nb = 0;
void reset_lhdc_assmeble_packet() {
  is_synced = false;
  asm_pkt_st = ASM_PKT_WAT_STR;
  packet_buf_len = 0;
  total_frame_nb = 0;
}

void initial_lhdc_assemble_packet(bool splitFlg) {
  memset(packet_buffer, 0, PACKET_BUFFER_LENGTH);
  reset_lhdc_assmeble_packet();
  serial_no = 0xff;
  is_splited = splitFlg;
  TRACE(1, "is_splited = %s\n", splitFlg ? "true" : "false");
}

/**
 * get lhdc frame header
 */
bool get_lhdc_header(uint8_t *in, LHDC_FRAME_HDR *h) {
#define LHDC_HDR_LEN 4
  uint32_t hdr = 0;
  bool ret = false;
  memcpy(&hdr, in, LHDC_HDR_LEN);
  h->frame_len = (int)((hdr >> 8) & 0x1fff);
  h->isSplit = ((hdr & 0x00600000) == 0x00600000);
  h->isLeft = ((hdr & 0xf) == 0);

  if ((hdr & 0xff000000) != 0x4c000000) {
    TRACE(0, "lhdc hdr err!\n");
    ret = false;
  } else {
    // TRACE(4,"frame_len = %d, g_lhdc_split = %s, g_lhdc_channel = %s(%d)",
    // h->frame_len, h->isSplit ? "true" : "false", h->isLeft ? "left" :
    // "right", (hdr & 0xf) );
    ret = true;
  }
  return ret;
}

/**
 *Get splited lhdc frames fit to MTU size
 */
uint32_t get_lhdc_frame(uint8_t *frame_q, size_t q_available_size,
                        uint8_t *out_buf, uint32_t *out_size,
                        uint32_t *out_frames) {
  LHDC_FRAME_HDR hdr;
  size_t emptSize =
      q_available_size >= PACKET_MTU_SIZE ? PACKET_MTU_SIZE : q_available_size;
  uint32_t *offset = out_size;
  // uint32_t * frame_cnt = out_frames;
  bool start = TRUE;
  *offset = 0; // *frame_cnt = 0;
  while (start) {

    if (get_lhdc_header(frame_q + *offset, &hdr) == false) {
      start = false;
      continue;
    }

    if (emptSize < hdr.frame_len) {
      start = false;
      continue;
    }

    memcpy(out_buf + *offset, frame_q + *offset, hdr.frame_len);
    emptSize -= hdr.frame_len;
    *offset += hdr.frame_len;
    (*out_frames)++;
  }
  return *offset;
}
/**
 *  Grabe lhdc data from queue
 */
int get_lhdc_data(unsigned char *frame, unsigned int len) {
  int status;
  unsigned int len1 = 0, len2 = 0;
  unsigned char *e1 = NULL, *e2 = NULL;
  LOCK_APP_AUDIO_QUEUE();
  unsigned int queu_len = (unsigned int)LengthOfCQueue(&sbc_queue);
  len = queu_len < len ? queu_len : len;
  status = PeekCQueue(&sbc_queue, len, &e1, &len1, &e2, &len2);
  UNLOCK_APP_AUDIO_QUEUE();
  if (status != CQ_OK) {
    return 0;
  }

  if (e1) {
    memcpy(frame, e1, len1);
  }
  if (e2) {
    memcpy(frame + len1, e2, len2);
  }
  return len;
}

int assemble_lhdc_packet(uint8_t *input, uint32_t input_len, uint8_t **pLout,
                         uint32_t *pLlen, uint8_t **pRout, uint32_t *pRlen) {
  uint8_t hdr = 0, seqno = 0xff;
  int ret = -1;
  uint32_t status = 0;

  hdr = (*input);
  input++;
  seqno = (*input);
  input++;
  input_len -= 2;

  status = hdr & A2DP_LHDC_HDR_LATENCY_MASK;
  if (latencyUpdated != (uint8_t)status) {
    latencyUpdated = (uint8_t)status;
    TRACE(1, "New latency setting = 0x%02x", latencyUpdated);
  }

  if (is_synced) {
    if (seqno != serial_no) {
      reset_lhdc_assmeble_packet();
      if ((hdr & A2DP_LHDC_HDR_FLAG_MSK) == 0 ||
          (hdr & A2DP_LHDC_HDR_S_MSK) != 0) {
        goto lhdc_start;
      } else
        TRACE(1, "drop packet No. %u", seqno);
      return 0;
    }
    serial_no = seqno + 1;
  }

lhdc_start:
  switch (asm_pkt_st) {
  case ASM_PKT_WAT_STR: {
    if ((hdr & A2DP_LHDC_HDR_FLAG_MSK) == 0) {
      memcpy(&packet_buffer[0], input, input_len);
      if (pLlen && pLout) {
        *pLlen = input_len;
        *pLout = packet_buffer;
      }
      if (pRout && pRlen) {
        *pRout = NULL;
        *pRlen = 0;
      }
      // TRACE(1,"Single payload size = %d", *pLlen);
      asm_pkt_st = ASM_PKT_WAT_STR;
      packet_buf_len = 0; //= packet_buf_left_len = packet_buf_right_len = 0;
      total_frame_nb = 0;
      ret = 1;
    } else if (hdr & A2DP_LHDC_HDR_S_MSK) {
      ret = 0;
      if (packet_buf_len + input_len >= PACKET_BUFFER_LENGTH) {
        packet_buf_len = 0;
        asm_pkt_st = ASM_PKT_WAT_STR;
        TRACE(1, "ASM_PKT_WAT_STR:Frame buffer overflow!(%d)", packet_buf_len);
        break;
      }
      memcpy(&packet_buffer, input, input_len);
      packet_buf_len = input_len;
      asm_pkt_st = ASM_PKT_WAT_LST;
      // TRACE(1,"multi:first payload size = %d", input_len);
    } else
      ret = -1;

    if (ret >= 0) {
      if (!is_synced) {
        is_synced = true;
        serial_no = seqno + 1;
      }
    }
    break;
  }
  case ASM_PKT_WAT_LST: {
    if (packet_buf_len + input_len >= PACKET_BUFFER_LENGTH) {
      packet_buf_len = 0;
      asm_pkt_st = ASM_PKT_WAT_STR;
      TRACE(1, "ASM_PKT_WAT_LST:Frame buffer overflow(%d)", packet_buf_len);
      break;
    }
    memcpy(&packet_buffer[packet_buf_len], input, input_len);
    // TRACE(1,"multi:payload size = %d", input_len);
    packet_buf_len += input_len;
    ret = 0;

    if (hdr & A2DP_LHDC_HDR_L_MSK) {

      if (pLlen && pLout) {
        *pLlen = packet_buf_len;
        *pLout = packet_buffer;
      }
      // TRACE(1,"multi: all payload size = %d", packet_buf_len);
      packet_buf_len = 0; // packet_buf_left_len = packet_buf_right_len = 0;
      total_frame_nb = 0;
      ret = 1;
      asm_pkt_st = ASM_PKT_WAT_STR;
    }
    break;
  }
  default:
    ret = 0;
    break;
  }
  return ret;
}
/**
 * Decode LHDC data...
 */
TEXT_LHDC_LOC
int load_lhdc_frame(uint8_t **p_data, uint32_t *p_len) {
  int ret;
  LHDC_HEADER *ph = NULL;
  uint8_t retry = 0;
  int status;
  uint8_t *_buff = NULL;
  uint32_t _buff_len = 0;

  ret = 1;

  A2DP_SYNC_WITH_PUT_MUTUX_ALLOC();

  if (latencyIndex != latencyUpdated) {
    latencyIndex = latencyUpdated;
    goto _exit;
  }

  while (true) {
    int result = lhdcReadyForInput();
    if (result == 1) {
      /* code */
      status = get_lhdc_data(lhdc_input_mid_buf, sizeof(LHDC_HEADER));
      if (status == 0) {
        // No data return...
        if (retry++ >= A2DP_SYNC_WITH_PUT_MUTUX_TIMEROUT_CNT) {
          TRACE(1, "%s:No Data return", __func__);
          break;
        }
        A2DP_SYNC_WITH_PUT_MUTUX_WAIT();
        continue;
      }
      retry = 0;
      ph = (LHDC_HEADER *)&lhdc_input_mid_buf[0];
      result = memcmp((const char *)magicTag, (const char *)ph->tag,
                      sizeof(magicTag));
      if (result != 0) {
        uint8_t tmp[sizeof(magicTag) + 1];
        size_t i;
        for (i = 0; i < sizeof(magicTag); i++) {
          sprintf((char *)&tmp[i], "%c", (char)ph->tag[i]);
        }
        tmp[i] = 0;
        continue;
      }

      status = get_lhdc_data(lhdc_input_mid_buf,
                             sizeof(LHDC_HEADER) + ph->packetLen);
      if (status == 0) {
        // No data return...
        TRACE(1, "Fetch data error[lenght = %d]", ph->packetLen);
        continue;
      }
      ph = (LHDC_HEADER *)&lhdc_input_mid_buf[0];

      // TRACE(2,"%s:Get packet[%d]", __func__, ph->packetLen);

      LOCK_APP_AUDIO_QUEUE();
      DeCQueue(&sbc_queue, 0, sizeof(LHDC_HEADER) + ph->packetLen);
      mtu_count--;
      UNLOCK_APP_AUDIO_QUEUE();
      _buff = ph->dptr;
      _buff_len = ph->packetLen;
      ret = 0;
    } else {
      _buff = NULL;
      _buff_len = 0;
      ret = 0;
    }
    break;
  }

_exit:
  A2DP_SYNC_WITH_PUT_MUTUX_FREE();

  if (p_data) {
    *p_data = _buff;
  }
  if (p_len) {
    *p_len = _buff_len;
  }

  return ret;
}

TEXT_LHDC_LOC
int decode_lhdc_frame(uint8_t *out, uint32_t out_max, uint32_t *p_out_len,
                      const uint8_t *in, uint32_t in_len,
                      uint32_t *p_consume_len) {
  uint32_t need_again;
  uint32_t out_len;

  if (a2dp_channel_num[app_bt_device.curr_a2dp_stream_id] == 1) {
    ASSERT(0, "lhdc should check audio channel number");
  }

  out_len = lhdcDecodeProcess(out, (uint8_t *)in, in_len, &need_again);

  if (p_out_len) {
    *p_out_len = (out_len >= 0) ? out_len : 0;
  }
  if (p_consume_len) {
    *p_consume_len = (out_len >= 0) ? in_len : 0;
  }

  return (out_len >= 0) ? 0 : 1;
}

TEXT_LHDC_LOC
void decode_lhdc_end(uint32_t expect_out_len, uint32_t out_len,
                     uint32_t in_len) {
  if (expect_out_len != out_len) {
    dec_reset_queue = true;
  }
}
#endif

#ifdef A2DP_CP_ACCEL
#include "cp_accel.h"

#define CP_CACHE_ATTR ALIGNED(4) CP_BSS_LOC
#define CP_DEC_SLOT_CNT 3

enum CP_DEC_STATE_T {
  CP_DEC_STATE_IDLE,
  CP_DEC_STATE_WORKING,
  CP_DEC_STATE_DONE,
  CP_DEC_STATE_FAILED,
};

static CP_CACHE_ATTR uint8_t cp_in_cache[1024 * 10];
static CP_CACHE_ATTR uint8_t cp_out_cache[1024 * 16];
static CP_BSS_LOC uint32_t cp_in_wpos;
static CP_BSS_LOC uint32_t cp_in_rpos;
static CP_BSS_LOC uint32_t cp_dec_start_time;
static CP_BSS_LOC uint32_t cp_last_dec_time;
#ifdef A2DP_TRACE_CP_DEC_TIME
static CP_DATA_LOC const bool cp_dec_trace_time = true;
#else
static CP_BSS_LOC bool cp_dec_trace_time;
#endif
static CP_BSS_LOC uint8_t cp_dec_idx;

static bool mcu_dec_inited;
static enum APP_OVERLAY_ID_T cp_overlay_type;
static enum CP_DEC_STATE_T cp_dec_state[CP_DEC_SLOT_CNT];
static uint8_t cp_dec_mtu_cnt[CP_DEC_SLOT_CNT];
static uint32_t cp_out_len[CP_DEC_SLOT_CNT];
static uint8_t mcu_dec_idx;
static uint32_t cp_in_size;
static uint32_t cp_out_size;
static uint32_t cp_out_loop_size;

static uint32_t cp_get_in_cache_data_len(uint32_t in_wpos, uint32_t in_rpos);
static void cp_get_in_cache_data(uint8_t *data, uint32_t len, uint32_t in_wpos,
                                 uint32_t in_rpos);
static uint32_t cp_update_in_cache_pos(uint32_t in_pos, uint32_t len);

#if defined(A2DP_LDAC_ON)
static CP_BSS_LOC bool cp_ldac_fatal_error;

TEXT_LDAC_LOC
int cp_load_ldac_frame(uint8_t **p_data, uint32_t *p_len) {
  uint32_t len;
  uint32_t in_rpos;
  uint32_t in_wpos;

  if (hLdacData == NULL || cp_ldac_fatal_error) {
    return 1;
  }

  in_rpos = cp_in_rpos;
  in_wpos = cp_in_wpos;

_get_hdr:
  len = cp_get_in_cache_data_len(in_wpos, in_rpos);
  if (len == 0) {
    return 2;
  }
  if (len > LDAC_READBUF_SIZE) {
    len = LDAC_READBUF_SIZE;
  }

  cp_get_in_cache_data(&ldac_input_mid_buf[0], len, in_wpos, in_rpos);

  if (ldac_recovery) {
    uint32_t parse_bytes;
    int result;

    result = ldac_seek_header(len, &parse_bytes);
    if (result == 0) {
      ldac_recovery = false;
    }
    in_rpos = cp_update_in_cache_pos(in_rpos, parse_bytes);
    goto _get_hdr;
  }

  return 0;
}

TEXT_LDAC_LOC
static int cp_consume_ldac_frame(const uint8_t *data, uint32_t len) {
  cp_in_rpos = cp_update_in_cache_pos(cp_in_rpos, len);

  return 0;
}

TEXT_LDAC_LOC
void cp_decode_ldac_end(uint32_t expect_out_len, uint32_t out_len,
                        uint32_t in_len) {
  if (expect_out_len != out_len) {
    if (hLdacData) {
      int error_code;
      error_code = ldacBT_get_error_code(hLdacData);
      TRACE(6, "error_code = %4d, %4d, %4d,FrameHeader[%02x:%02x:%02x]\n",
            LDACBT_API_ERR(error_code), LDACBT_HANDLE_ERR(error_code),
            LDACBT_BLOCK_ERR(error_code), ldac_input_mid_buf[0],
            ldac_input_mid_buf[1], ldac_input_mid_buf[2]);
      if (LDACBT_FATAL(error_code)) {
        cp_ldac_fatal_error = true;
      }
    }
  }
}

TEXT_LDAC_LOC
void mcu_decode_ldac_end(uint32_t expect_out_len, uint32_t out_len,
                         uint32_t in_len) {
  if (expect_out_len != out_len) {
    if (cp_ldac_fatal_error) {
      ldac_dec_deinit();
    }
    if (hLdacData == NULL) {
      /* Recovery Process */
      int result;
      result = ldac_dec_init();
      if (result) {
        ldac_dec_deinit();
      }
      ldac_recovery = true;
      cp_ldac_fatal_error = false;
    }
  }
}
#endif

#if defined(A2DP_LHDC_ON)
TEXT_LHDC_LOC
int cp_load_lhdc_frame(uint8_t **p_data, uint32_t *p_len) {
  int ret;
  LHDC_HEADER ph;
  uint32_t len;
  uint32_t in_rpos;
  uint32_t in_wpos;
  uint8_t *frame_buf = NULL;
  uint32_t frame_len = 0;
  int result;

  ret = 1;

  if (latencyIndex != latencyUpdated) {
    latencyIndex = latencyUpdated;
    goto _exit;
  }

  result = lhdcReadyForInput();
  if (result == 0) {
    ret = 0;
    goto _exit;
  }

  in_rpos = cp_in_rpos;
  in_wpos = cp_in_wpos;

_get_hdr:
  len = cp_get_in_cache_data_len(in_wpos, in_rpos);
  if (len < sizeof(ph)) {
    goto _exit;
  }

  cp_get_in_cache_data(&lhdc_input_mid_buf[0], sizeof(ph), in_wpos, in_rpos);

  memcpy(&ph, &lhdc_input_mid_buf[0], sizeof(ph));
  result =
      memcmp((const char *)magicTag, (const char *)ph.tag, sizeof(magicTag));
  if (result != 0) {
    in_rpos = cp_update_in_cache_pos(in_rpos, 1);
    goto _get_hdr;
  }

  if (len < sizeof(ph) + ph.packetLen) {
    goto _exit;
  }

  in_rpos = cp_update_in_cache_pos(in_rpos, sizeof(ph));

  cp_get_in_cache_data(&lhdc_input_mid_buf[sizeof(ph)], ph.packetLen, in_wpos,
                       in_rpos);

  in_rpos = cp_update_in_cache_pos(in_rpos, ph.packetLen);
  cp_in_rpos = in_rpos;

  frame_buf = &((LHDC_HEADER *)&lhdc_input_mid_buf[0])->dptr[0];
  frame_len = ph.packetLen;

  ret = 0;

_exit:
  if (p_data) {
    *p_data = frame_buf;
  }
  if (p_len) {
    *p_len = frame_len;
  }

  return ret;
}
#endif

#if defined(A2DP_AAC_ON)
TEXT_AAC_LOC
int cp_load_aac_frame(uint8_t **p_data, uint32_t *p_len) {
  uint32_t len;
  unsigned short aac_len = 0;
  uint32_t in_rpos;
  uint32_t in_wpos;

  in_rpos = cp_in_rpos;
  in_wpos = cp_in_wpos;

  len = cp_get_in_cache_data_len(in_wpos, in_rpos);
  if (len < 2) {
    return 1;
  }

  if (in_wpos > in_rpos || cp_in_size >= in_rpos + 2) {
    aac_len = cp_in_cache[in_rpos] | (cp_in_cache[in_rpos + 1] << 8);
  } else {
    aac_len = cp_in_cache[in_rpos] | (cp_in_cache[0] << 8);
  }
  in_rpos = cp_update_in_cache_pos(in_rpos, 2);

  if (aac_len < 64) {
    aac_maxreadBytes = 64;
  } else if (aac_len < 128) {
    aac_maxreadBytes = 128;
  } else if (aac_len < 256) {
    aac_maxreadBytes = 256;
  } else if (aac_len < 512) {
    aac_maxreadBytes = 512;
  } else if (aac_len < 1024) {
    aac_maxreadBytes = 1024;
  } else {
    if (aac_len >= 2048) {
      // Header is bad!
      dec_reset_queue = true;
      return 2;
    }
    aac_maxreadBytes = 2048;
  }

  if (len < 2 + (uint32_t)aac_len) {
    return 3;
  }

  cp_dec_mtu_cnt[cp_dec_idx]++;

  cp_get_in_cache_data(&aac_input_mid_buf[0], aac_len, in_wpos, in_rpos);
  // Consume the packet
  in_rpos = cp_update_in_cache_pos(in_rpos, aac_len);
  cp_in_rpos = in_rpos;

  if (p_data) {
    *p_data = (unsigned char *)aac_input_mid_buf;
  }
  if (p_len) {
    *p_len = aac_maxreadBytes;
  }

  return 0;
}
#endif

TEXT_SBC_LOC
int cp_load_sbc_frame(uint8_t **p_data, uint32_t *p_len) {
  uint32_t len;
  uint32_t in_wpos;
  uint32_t in_rpos;

  if (!sbc_next_frame_size) {
    sbc_next_frame_size = sbc_frame_size;
  }

  if (need_init_decoder) {
    sbc_next_frame_size = sbc_frame_size;
  }

  in_wpos = cp_in_wpos;
  in_rpos = cp_in_rpos;

  if (in_wpos == in_rpos) {
    return 1;
  }

  if (in_wpos > in_rpos) {
    len = in_wpos - in_rpos;
  } else {
    len = cp_in_size - in_rpos;
  }

  if (len > sbc_next_frame_size) {
    len = sbc_next_frame_size;
  }

  if (p_data) {
    *p_data = &cp_in_cache[in_rpos];
  }
  if (p_len) {
    *p_len = len;
  }

  return 0;
}

TEXT_SBC_LOC
static int cp_consume_sbc_frame(const uint8_t *data, uint32_t len) {
  cp_in_rpos = cp_update_in_cache_pos(cp_in_rpos, len);

  return 0;
}

CP_TEXT_SRAM_LOC
static uint32_t cp_get_in_cache_data_len(uint32_t in_wpos, uint32_t in_rpos) {
  uint32_t len;

  if (in_wpos >= in_rpos) {
    len = in_wpos - in_rpos;
  } else {
    len = cp_in_size - in_rpos + in_wpos;
  }

  return len;
}

CP_TEXT_SRAM_LOC
static void cp_get_in_cache_data(uint8_t *data, uint32_t len, uint32_t in_wpos,
                                 uint32_t in_rpos) {
  // Assuming cp_get_in_cache_data_len(in_wpos, in_rpos) >= len

  if (in_wpos > in_rpos || cp_in_size >= in_rpos + len) {
    memcpy(&data[0], &cp_in_cache[in_rpos], len);
  } else {
    uint32_t copy_len;

    copy_len = cp_in_size - in_rpos;
    memcpy(&data[0], &cp_in_cache[in_rpos], copy_len);
    memcpy(&data[copy_len], &cp_in_cache[0], len - copy_len);
  }
}

CP_TEXT_SRAM_LOC
static uint32_t cp_update_in_cache_pos(uint32_t in_pos, uint32_t len) {
  // Consume the packet
  in_pos += len;
  if (in_pos >= cp_in_size) {
    in_pos -= cp_in_size;
  }

  return in_pos;
}

CP_TEXT_SRAM_LOC
static int cp_decode_a2dp_begin(enum APP_OVERLAY_ID_T overlay_type) {
  cp_dec_start_time = hal_fast_sys_timer_get();
#ifndef A2DP_TRACE_CP_DEC_TIME
  cp_dec_trace_time = false;
#endif

  if (cp_dec_state[cp_dec_idx] != CP_DEC_STATE_IDLE &&
      cp_dec_state[cp_dec_idx] != CP_DEC_STATE_WORKING) {
    return 1;
  }

  if (cp_dec_state[cp_dec_idx] == CP_DEC_STATE_IDLE) {
    cp_dec_state[cp_dec_idx] = CP_DEC_STATE_WORKING;
    cp_out_len[cp_dec_idx] = 0;
    cp_dec_mtu_cnt[cp_dec_idx] = 0;
  }

  return 0;
}

CP_TEXT_SRAM_LOC
static int cp_load_a2dp_encoded_data(enum APP_OVERLAY_ID_T overlay_type,
                                     uint8_t **p_data, uint32_t *p_len) {
  int ret = -1;

  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    ret = cp_load_ldac_frame(p_data, p_len);
#endif
#if defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    ret = cp_load_lhdc_frame(p_data, p_len);
#endif
#if defined(A2DP_AAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_AAC) {
    ret = cp_load_aac_frame(p_data, p_len);
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    ret = cp_load_sbc_frame(p_data, p_len);
  }

  return ret;
}

CP_TEXT_SRAM_LOC
static int cp_decode_a2dp_process(enum APP_OVERLAY_ID_T overlay_type,
                                  uint8_t *out, uint32_t out_max,
                                  uint32_t *p_out_len, const uint8_t *in,
                                  uint32_t in_len, uint32_t *p_consume_len) {
  int ret = -1;

  if (p_out_len) {
    *p_out_len = 0;
  }
  if (p_consume_len) {
    *p_consume_len = 0;
  }

  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    ret = decode_ldac_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
#endif
#if defined(A2DP_AAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_AAC) {
    ret = decode_aac_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
#endif
#if defined(A2DP_SCALABLE_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_SCALABLE) {
    ret = decode_scalable_frame(out, out_max, p_out_len, in, in_len,
                                p_consume_len);
#endif
#if defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    ret = decode_lhdc_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    ret = decode_sbc_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
  }

  return ret;
}

CP_TEXT_SRAM_LOC
static int cp_consume_a2dp_encoded_data(enum APP_OVERLAY_ID_T overlay_type,
                                        const uint8_t *data, uint32_t len) {
  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    cp_consume_ldac_frame(data, len);
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    cp_consume_sbc_frame(data, len);
  }

  return 0;
}

CP_TEXT_SRAM_LOC
static void cp_decode_a2dp_end(enum APP_OVERLAY_ID_T overlay_type,
                               uint32_t expect_out_len, uint32_t out_len,
                               uint32_t in_len, bool retry) {
  uint32_t etime;
  uint8_t old_cp_dec_idx;

  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    cp_decode_ldac_end(expect_out_len, out_len, in_len);
#endif
  }

  old_cp_dec_idx = cp_dec_idx;

  cp_out_len[cp_dec_idx] += out_len;

  if (!retry) {
    if (expect_out_len == out_len) {
      cp_dec_state[cp_dec_idx] = CP_DEC_STATE_DONE;
    } else {
      cp_dec_state[cp_dec_idx] = CP_DEC_STATE_FAILED;
    }

    cp_dec_idx++;
    if (cp_dec_idx >= CP_DEC_SLOT_CNT) {
      cp_dec_idx = 0;
    }
  }

  etime = hal_fast_sys_timer_get();
  if (cp_dec_trace_time) {
    TRACE(4, "cp_decode[%u]: %u us in %u us (r=%u)", old_cp_dec_idx,
          FAST_TICKS_TO_US(etime - cp_dec_start_time),
          FAST_TICKS_TO_US(etime - cp_last_dec_time), retry);
  }
#ifdef A2DP_TRACE_CP_ACCEL
  if (retry) {
    TRACE(3, "cp_decode[%u]:retry: cp_out_len=%u out_len=%u", old_cp_dec_idx,
          cp_out_len[old_cp_dec_idx], out_len);
  }
#endif
  cp_last_dec_time = etime;
}

CP_TEXT_SRAM_LOC
static int cp_decode_a2dp_frame(enum APP_OVERLAY_ID_T overlay_type,
                                unsigned char *pcm_buffer,
                                unsigned int pcm_len) {
  uint32_t all_out_len = 0, all_in_len = 0;
  int ret, ret2;
  uint32_t out_len;
  uint8_t *enc_data;
  uint32_t enc_len;
  uint32_t consume_len;
  bool retry = false;

  ret = cp_decode_a2dp_begin(overlay_type);
  if (ret) {
    return 0;
  }

  while (all_out_len < pcm_len) {
    ret = cp_load_a2dp_encoded_data(overlay_type, &enc_data, &enc_len);
    if (ret) {
      retry = true;
      break;
    }

    ret = cp_decode_a2dp_process(overlay_type, &pcm_buffer[all_out_len],
                                 pcm_len - all_out_len, &out_len, enc_data,
                                 enc_len, &consume_len);

    ret2 = cp_consume_a2dp_encoded_data(overlay_type, enc_data, consume_len);

    all_out_len += out_len;

    if (ret || ret2) {
      break;
    }
  }

  cp_decode_a2dp_end(overlay_type, pcm_len, all_out_len, all_in_len, retry);

  return all_out_len;
}

CP_TEXT_SRAM_LOC
unsigned int cp_a2dp_main(uint8_t event) {
  uint32_t out_wpos;
  uint32_t out_len;
  uint32_t pcm_len;

  do {
    if (cp_dec_state[cp_dec_idx] == CP_DEC_STATE_WORKING) {
      if (cp_out_len[cp_dec_idx] < cp_out_loop_size) {
        pcm_len = cp_out_loop_size - cp_out_len[cp_dec_idx];
      } else {
        cp_out_len[cp_dec_idx] = 0;
        pcm_len = cp_out_loop_size;
      }
    } else {
      pcm_len = cp_out_loop_size;
    }
    out_wpos = cp_out_loop_size * cp_dec_idx + cp_out_loop_size - pcm_len;

    out_len = cp_decode_a2dp_frame(cp_overlay_type, &cp_out_cache[out_wpos],
                                   cp_out_loop_size);
  } while (out_len == pcm_len);

  return 0;
}

FRAM_TEXT_LOC
static void mcu_decode_a2dp_init(uint32_t out_max) {
  uint32_t i;

  if (mcu_dec_inited == false) {
    mcu_dec_inited = true;

    cp_in_size = sizeof(cp_in_cache);
    cp_out_size = out_max * CP_DEC_SLOT_CNT;
    ASSERT(cp_out_size <= sizeof(cp_out_cache),
           "%s: cp_out_cache too small (should >= %u)", __func__, cp_out_size);
    cp_out_loop_size = out_max;

    cp_overlay_type = app_get_current_overlay();

    for (i = 0; i < CP_DEC_SLOT_CNT; i++) {
      cp_dec_state[i] = CP_DEC_STATE_IDLE;
      cp_dec_mtu_cnt[i] = 0;
      cp_out_len[i] = 0;
    }

#if defined(A2DP_LHDC_ON)
    // Increase latency to improve data buffering
    mcu_dec_idx = 1;
#else
    mcu_dec_idx = CP_DEC_SLOT_CNT - 1;
#endif
    for (i = mcu_dec_idx; i < CP_DEC_SLOT_CNT; i++) {
      cp_dec_state[i] = CP_DEC_STATE_DONE;
      cp_out_len[i] = cp_out_loop_size;
    }
  }
}

FRAM_TEXT_LOC
static int mcu_load_a2dp_encoded_data(uint8_t **p_data, uint32_t *p_len) {
  uint32_t free_len;
  uint32_t in_wpos, in_rpos;
  uint32_t size;
  int r = 0;
  unsigned char *e1 = NULL, *e2 = NULL;
  unsigned int len1 = 0, len2 = 0;

  in_wpos = cp_in_wpos;
  in_rpos = cp_in_rpos;

  if (in_wpos < in_rpos) {
    free_len = in_rpos - in_wpos - 1;
  } else {
    free_len = cp_in_size - in_wpos + in_rpos - 1;
  }

  if (free_len == 0) {
    return 0;
  }

  LOCK_APP_AUDIO_QUEUE();
  size = LengthOfCQueue(&sbc_queue);
  UNLOCK_APP_AUDIO_QUEUE();

  if (size > free_len) {
    size = free_len;
  }

  LOCK_APP_AUDIO_QUEUE();
  r = PeekCQueue(&sbc_queue, size, &e1, &len1, &e2, &len2);
  UNLOCK_APP_AUDIO_QUEUE();

  if (r == CQ_ERR) {
    TRACE(2, "%s: Failed to get %u from sbc_queue", __func__, size);
    return 1;
  }

  if (in_wpos < in_rpos) {
    if (len1) {
      memcpy(&cp_in_cache[in_wpos], e1, len1);
      in_wpos += len1;
    }
    if (len2) {
      memcpy(&cp_in_cache[in_wpos], e2, len2);
      in_wpos += len2;
    }
  } else {
    free_len = cp_in_size - in_wpos;
    if (len1) {
      if (free_len > len1) {
        free_len = len1;
      }
      memcpy(&cp_in_cache[in_wpos], e1, free_len);
      len1 -= free_len;
      e1 += free_len;
      in_wpos = cp_update_in_cache_pos(in_wpos, free_len);
      if (len1) {
        memcpy(&cp_in_cache[in_wpos], e1, len1);
        in_wpos = cp_update_in_cache_pos(in_wpos, len1);
      }
      if (in_wpos < in_rpos) {
        free_len = in_rpos - in_wpos - 1;
      } else {
        free_len = cp_in_size - in_wpos;
      }
    }
    if (len2) {
      if (free_len > len2) {
        free_len = len2;
      }
      memcpy(&cp_in_cache[in_wpos], e2, free_len);
      len2 -= free_len;
      e2 += free_len;
      in_wpos = cp_update_in_cache_pos(in_wpos, free_len);
      if (len2) {
        memcpy(&cp_in_cache[in_wpos], e2, len2);
        in_wpos = cp_update_in_cache_pos(in_wpos, len2);
      }
    }
  }

  cp_in_wpos = in_wpos;

  LOCK_APP_AUDIO_QUEUE();
  DeCQueue(&sbc_queue, NULL, size);
  UNLOCK_APP_AUDIO_QUEUE();

  return 0;
}

FRAM_TEXT_LOC
static int mcu_decode_a2dp_process(uint8_t *out, uint32_t out_max,
                                   uint32_t *p_out_len, const uint8_t *in,
                                   uint32_t in_len, uint32_t *p_consume_len) {
  int ret = 0;
  uint32_t out_rpos;

#ifdef A2DP_TRACE_CP_ACCEL
  TRACE(3, "mcu_dec_proc[%u]: state=%u len=%u", mcu_dec_idx,
        cp_dec_state[mcu_dec_idx], cp_out_len[mcu_dec_idx]);
#endif

  cp_accel_send_event_mcu2cp(
      CP_BUILD_ID(CP_TASK_A2DP_DECODE, CP_EVENT_A2DP_DECODE));

  if (cp_dec_state[mcu_dec_idx] != CP_DEC_STATE_DONE) {
    ret = 1;
    goto _exit;
  }
  if (cp_out_len[mcu_dec_idx] != out_max) {
    ret = 2;
    goto _exit;
  }

  out_rpos = cp_out_loop_size * mcu_dec_idx;

  memcpy(out, &cp_out_cache[out_rpos], out_max);

  if (p_out_len) {
    *p_out_len = out_max;
  }

_exit:
  if (cp_dec_state[mcu_dec_idx] == CP_DEC_STATE_DONE ||
      cp_dec_state[mcu_dec_idx] == CP_DEC_STATE_FAILED) {
    if (mtu_count > cp_dec_mtu_cnt[mcu_dec_idx]) {
      mtu_count -= cp_dec_mtu_cnt[mcu_dec_idx];
    } else {
      mtu_count = 0;
    }

    cp_dec_state[mcu_dec_idx] = CP_DEC_STATE_IDLE;

    mcu_dec_idx++;
    if (mcu_dec_idx >= CP_DEC_SLOT_CNT) {
      mcu_dec_idx = 0;
    }
  }

  return ret;
}

static struct cp_task_desc TASK_DESC_A2DP = {CP_ACCEL_STATE_CLOSED,
                                             cp_a2dp_main, NULL, NULL, NULL};
void cp_a2dp_init(void) {
  mcu_dec_inited = false;
  norflash_api_flush_disable(NORFLASH_API_USER_CP,
                             (uint32_t)cp_accel_init_done);
  cp_accel_open(CP_TASK_A2DP_DECODE, &TASK_DESC_A2DP);
  while (cp_accel_init_done() == false) {
    hal_sys_timer_delay_us(100);
  }
  norflash_api_flush_enable(NORFLASH_API_USER_CP);
}

void cp_a2dp_deinit(void) { cp_accel_close(CP_TASK_A2DP_DECODE); }
#endif

FRAM_TEXT_LOC
static int decode_a2dp_begin(enum APP_OVERLAY_ID_T overlay_type) {
#ifdef TIMER1_BASE
  dec_start_time = hal_fast_sys_timer_get();
#else
  dec_start_time = hal_sys_timer_get();
#endif
#ifndef A2DP_TRACE_DEC_TIME
  dec_trace_time = false;
#endif

#ifndef A2DP_CP_ACCEL
  if (overlay_type == APP_OVERLAY_A2DP) {
    decode_sbc_begin();
  }
#endif

  return 0;
}

FRAM_TEXT_LOC
static int load_a2dp_encoded_data(enum APP_OVERLAY_ID_T overlay_type,
                                  uint8_t **p_data, uint32_t *p_len) {
  int ret = -1;

#ifdef A2DP_CP_ACCEL
  ret = mcu_load_a2dp_encoded_data(p_data, p_len);
#else
  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    ret = load_ldac_frame(p_data, p_len);
#endif
#if defined(A2DP_AAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_AAC) {
    ret = load_aac_frame(p_data, p_len);
#endif
#if defined(A2DP_SCALABLE_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_SCALABLE) {
    ret = load_scalable_frame(p_data, p_len);
#endif
#if defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    ret = load_lhdc_frame(p_data, p_len);
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    ret = load_sbc_frame(p_data, p_len);
  }
#endif

  return ret;
}

FRAM_TEXT_LOC
static int decode_a2dp_process(enum APP_OVERLAY_ID_T overlay_type, uint8_t *out,
                               uint32_t out_max, uint32_t *p_out_len,
                               const uint8_t *in, uint32_t in_len,
                               uint32_t *p_consume_len) {
  int ret = -1;

  if (p_out_len) {
    *p_out_len = 0;
  }
  if (p_consume_len) {
    *p_consume_len = 0;
  }

#ifdef A2DP_CP_ACCEL
  ret = mcu_decode_a2dp_process(out, out_max, p_out_len, in, in_len,
                                p_consume_len);
#else
  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    ret = decode_ldac_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
#endif
#if defined(A2DP_AAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_AAC) {
    ret = decode_aac_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
#endif
#if defined(A2DP_SCALABLE_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_SCALABLE) {
    ret = decode_scalable_frame(out, out_max, p_out_len, in, in_len,
                                p_consume_len);
#endif
#if defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    ret = decode_lhdc_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    ret = decode_sbc_frame(out, out_max, p_out_len, in, in_len, p_consume_len);
  }
#endif

  return ret;
}

FRAM_TEXT_LOC
static int consume_a2dp_encoded_data(enum APP_OVERLAY_ID_T overlay_type,
                                     const uint8_t *data, uint32_t len) {
#ifndef A2DP_CP_ACCEL
  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    consume_ldac_frame(data, len);
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    consume_sbc_frame(data, len);
  }
#endif

  return 0;
}

FRAM_TEXT_LOC
static void decode_a2dp_end(enum APP_OVERLAY_ID_T overlay_type,
                            uint32_t expect_out_len, uint32_t out_len,
                            uint32_t in_len) {
  uint32_t etime;

#ifdef A2DP_CP_ACCEL

  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    mcu_decode_ldac_end(expect_out_len, out_len, in_len);
#endif
  }

#else

  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    decode_ldac_end(expect_out_len, out_len, in_len);
#endif
#if defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    decode_lhdc_end(expect_out_len, out_len, in_len);
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    decode_sbc_end(expect_out_len, out_len, in_len);
  }

#endif

  if (dec_reset_queue) {
    dec_reset_queue = false;
    LOCK_APP_AUDIO_QUEUE();
    ResetCQueue(&sbc_queue);
    TRACE(0, "Reset queue");
    UNLOCK_APP_AUDIO_QUEUE();
  }

#ifdef TIMER1_BASE
  etime = hal_fast_sys_timer_get();
  if (dec_trace_time) {
    TRACE(2, "a2dp decode: %u us in %u us",
          FAST_TICKS_TO_US(etime - dec_start_time),
          FAST_TICKS_TO_US(etime - last_dec_time));
  }
#else
  etime = hal_sys_timer_get();
  if (dec_trace_time) {
    TRACE(2, "a2dp decode: %u ms in %u ms", TICKS_TO_MS(etime - dec_start_time),
          TICKS_TO_MS(etime - last_dec_time));
  }
#endif
  last_dec_time = etime;
}

FRAM_TEXT_LOC
static int decode_a2dp_frame(enum APP_OVERLAY_ID_T overlay_type,
                             unsigned char *pcm_buffer, unsigned int pcm_len) {
  uint32_t all_out_len = 0, all_in_len = 0;
  int ret, ret2;
  uint32_t out_len;
  uint8_t *enc_data;
  uint32_t enc_len;
  uint32_t consume_len;

#ifdef A2DP_CP_ACCEL
  mcu_decode_a2dp_init(pcm_len);
#endif

  ret = decode_a2dp_begin(overlay_type);
  if (ret) {
    return 0;
  }

  while (all_out_len < pcm_len) {
    ret = load_a2dp_encoded_data(overlay_type, &enc_data, &enc_len);
    if (ret) {
      break;
    }

    ret = decode_a2dp_process(overlay_type, pcm_buffer + all_out_len,
                              pcm_len - all_out_len, &out_len, enc_data,
                              enc_len, &consume_len);

    ret2 = consume_a2dp_encoded_data(overlay_type, enc_data, consume_len);

    all_out_len += out_len;
    all_in_len += consume_len;

    if (ret || ret2) {
      break;
    }
  }

  decode_a2dp_end(overlay_type, pcm_len, all_out_len, all_in_len);

  return all_out_len;
}

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
#define AAC_RESAMPLE_ITER_LEN (1024 * 2 * 2)
#define SBC_RESAMPLE_ITER_LEN (128 * 2 * 2)

static struct APP_RESAMPLE_T *a2dp_resample;

FRAM_TEXT_LOC
static int a2dp_resample_iter(uint8_t *buf, uint32_t len) {
  enum APP_OVERLAY_ID_T overlay_type;

  overlay_type = app_get_current_overlay();

  uint32_t l = decode_a2dp_frame(overlay_type, buf, len);
  if (l != len) {
    TRACE(2, "a2dp_audio_more_data decode err %d/%d", l, len);
    return 1;
  }
  return 0;
}
#endif

extern bool process_delay(int32_t delay_ms);

void static attempt_slow_samplerate(uint8_t overlay_type) {
  uint32_t current_mtu_conut = 0;
  if (check_interval > 0)
    check_interval--;
  assumed_mtu_interval = 20;
  current_mtu_conut = mtu_count;

  if (lagest_mtu_in1s < current_mtu_conut)
    lagest_mtu_in1s = current_mtu_conut;
  if (least_mtu_in1s > current_mtu_conut)
    least_mtu_in1s = current_mtu_conut;

  if (check_interval == 0) {
    int32_t delay_ms = 0;
    if (overlay_type == APP_OVERLAY_A2DP) {
      delay_ms = (delay_mtu_limit - (int)lagest_mtu_in1s) * 128 * 1000 /
                 a2dp_sample_rate;
    } else
#if defined(A2DP_AAC_ON)
        if (overlay_type == APP_OVERLAY_A2DP_AAC) {
      delay_ms = (delay_mtu_limit - (int)lagest_mtu_in1s) * 1024 * 1000 /
                 a2dp_sample_rate;
    } else
#endif
#if defined(A2DP_SCALABLE_ON)
        if (overlay_type == APP_OVERLAY_A2DP_SCALABLE) {
      delay_ms = (delay_mtu_limit - (int)lagest_mtu_in1s) * 864 * 1000 /
                 a2dp_sample_rate;
    } else
#endif
#if defined(A2DP_LHDC_ON)
        if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
      delay_ms = (delay_mtu_limit - (int)lagest_mtu_in1s) * 512 * 1000 /
                 a2dp_sample_rate;
    } else
#endif
#if defined(A2DP_LDAC_ON)
        if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
      delay_ms = (delay_mtu_limit - (int)lagest_mtu_in1s) * 256 * 1000 /
                 a2dp_sample_rate;
    } else
#endif
    {
      delay_ms =
          (delay_mtu_limit - (int)lagest_mtu_in1s) * assumed_mtu_interval;
    }
    TRACE(6,
          "store_interval=%d,lagest_mtu_in1s=%d,least_mtu_in1s=%d,delay_ms=%d,"
          "frame_len=%d,mtu_count=%d",
          ssb_err_max, lagest_mtu_in1s, least_mtu_in1s, delay_ms,
          sbc_frame_rev_len, mtu_count);
    process_delay(delay_ms);
    check_interval = 50;
    lagest_mtu_in1s = 0;
    least_mtu_in1s = 0xffffffff;
  }
}

// A2DP_EQ_24BIT == 1, buf should be 32bit(24bit)
// A2DP_EQ_24BIT == 0, buf should be 16bit
uint32_t a2dp_audio_more_data(uint8_t overlay_type, uint8_t *buf,
                              uint32_t len) {
  uint32_t l = 0;

  if (!a2dp_audio_isrunning(A2DPPLAY_STRTEAM_GET, true)) {
  }

#ifdef A2DP_EQ_24BIT
  // Change len to 16-bit sample buffer length
  if ((bt_sbc_player_get_sample_bit() == 16)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
      || allow_resample
#endif
  ) {
    len = len / (sizeof(int32_t) / sizeof(int16_t));
  }
#endif

  if (a2dp_cache_status == APP_AUDIO_CACHE_CACHEING) {
    TRACE(1, "a2dp_audio_more_data cache not ready skip frame %d\n",
          overlay_type);
  } else {
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
    if (allow_resample) {
      app_playback_resample_run(a2dp_resample, buf, len);
    } else
#endif
    {
      l = decode_a2dp_frame((enum APP_OVERLAY_ID_T)overlay_type, buf, len);
      if (l != len) {
        TRACE(2, "a2dp_audio_more_data decode err %d/%d", l, len);
        if (l < len) {
          memset(buf + l, 0, len - l);
          // a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
          TRACE(0, "set to APP_AUDIO_CACHE_CACHEING");
        }
      } else {
        // TRACE(2,"a2dp_audio_more_data decode success %d/%d", l, len);
      }
    }
    if (a2dp_cache_status == APP_AUDIO_CACHE_OK) {
      attempt_slow_samplerate(overlay_type);
    }
  }

#ifdef A2DP_EQ_24BIT
  if ((bt_sbc_player_get_sample_bit() == 16)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
      || allow_resample
#endif
  ) {
    convert_16bit_to_24bit((int32_t *)buf, (int16_t *)buf,
                           len / sizeof(int16_t));
    // Restore len to 24-bit sample buffer length
    len = len * (sizeof(int32_t) / sizeof(int16_t));
  }
#endif

  A2DP_SYNC_WITH_GET_MUTUX_SET();

  return len;
}

#if defined(A2DP_LDAC_ON)
/* Convert LDAC Error Code to string */
#define CASE_RETURN_STR(const)                                                 \
  case const:                                                                  \
    return #const;
static const char *ldac_ErrCode2Str(int ErrCode) {
  switch (ErrCode) {
    CASE_RETURN_STR(LDACBT_ERR_NONE);
    CASE_RETURN_STR(LDACBT_ERR_NON_FATAL);
    CASE_RETURN_STR(LDACBT_ERR_BIT_ALLOCATION);
    CASE_RETURN_STR(LDACBT_ERR_NOT_IMPLEMENTED);
    CASE_RETURN_STR(LDACBT_ERR_NON_FATAL_ENCODE);
    CASE_RETURN_STR(LDACBT_ERR_FATAL);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_BAND);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_A);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_B);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_C);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_D);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_E);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_IDSF);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_SPEC);
    CASE_RETURN_STR(LDACBT_ERR_BIT_PACKING);
    CASE_RETURN_STR(LDACBT_ERR_ALLOC_MEMORY);
    CASE_RETURN_STR(LDACBT_ERR_FATAL_HANDLE);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SYNCWORD);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SMPL_FORMAT);
    CASE_RETURN_STR(LDACBT_ERR_ILL_PARAM);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SUP_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_CHECK_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_CHANNEL_CONFIG);
    CASE_RETURN_STR(LDACBT_ERR_CHECK_CHANNEL_CONFIG);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_FRAME_LENGTH);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SUP_FRAME_LENGTH);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_FRAME_STATUS);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_NSHIFT);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_CHANNEL_MODE);
    CASE_RETURN_STR(LDACBT_ERR_ENC_INIT_ALLOC);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADMODE);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_A);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_B);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_C);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_D);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_NBANDS);
    CASE_RETURN_STR(LDACBT_ERR_PACK_BLOCK_FAILED);
    CASE_RETURN_STR(LDACBT_ERR_DEC_INIT_ALLOC);
    CASE_RETURN_STR(LDACBT_ERR_INPUT_BUFFER_SIZE);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_BLOCK_FAILED);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_BLOCK_ALIGN);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_FRAME_ALIGN);
    CASE_RETURN_STR(LDACBT_ERR_FRAME_LENGTH_OVER);
    CASE_RETURN_STR(LDACBT_ERR_FRAME_ALIGN_OVER);
    CASE_RETURN_STR(LDACBT_ERR_ALTER_EQMID_LIMITED);
    CASE_RETURN_STR(LDACBT_ERR_ILL_EQMID);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ILL_NUM_CHANNEL);
    CASE_RETURN_STR(LDACBT_ERR_ILL_MTU_SIZE);
    CASE_RETURN_STR(LDACBT_ERR_HANDLE_NOT_INIT);
  default:
    return "unknown-error-code";
  }
}

char a_ErrorCodeStr[128];
const char *get_error_code_string(int error_code) {
  int errApi, errHdl, errBlk;

  errApi = LDACBT_API_ERR(error_code);
  errHdl = LDACBT_HANDLE_ERR(error_code);
  errBlk = LDACBT_BLOCK_ERR(error_code);

  a_ErrorCodeStr[0] = '\0';
  strcat(a_ErrorCodeStr, "API:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errApi));
  strcat(a_ErrorCodeStr, " Handle:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errHdl));
  strcat(a_ErrorCodeStr, " Block:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errBlk));
  return a_ErrorCodeStr;
}
#endif

extern uint8_t bt_sbc_player_get_bitsDepth(void);
#if defined(A2DP_LDAC_ON)
uint32_t ldac_buffer_used = 0;
#endif

static const char *a2dp_get_codec_name(enum APP_OVERLAY_ID_T overlay_type) {
  const char *codec_name;

  if (0) {
#if defined(A2DP_LDAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    codec_name = "LDAC";
#endif
#if defined(A2DP_AAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_AAC) {
    codec_name = "AAC";
#endif
#if defined(A2DP_SCALABLE_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_SCALABLE) {
    codec_name = "SCALABLE";
#endif
#if defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    codec_name = "LHDC";
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    codec_name = "SBC";
  } else {
    codec_name = "UNKNOWN";
  }

  return codec_name;
}

static void a2dp_trace_codec_name(const char *prefix,
                                  enum APP_OVERLAY_ID_T overlay_type) {
  TRACE(3, "\n\n%s: codecType=%x->:%s\n", prefix, overlay_type,
        a2dp_get_codec_name(overlay_type));
}

int a2dp_audio_init(void) {
  const float EQLevel[25] = {
      0.0630957, 0.0794328, 0.1,       0.1258925, 0.1584893,
      0.1995262, 0.2511886, 0.3162278, 0.398107,  0.5011872,
      0.6309573, 0.794328,  1,         1.258925,  1.584893,
      1.995262,  2.5118864, 3.1622776, 3.9810717, 5.011872,
      6.309573,  7.943282,  10,        12.589254, 15.848932}; //-12~12
  uint8_t *buff = NULL;
  uint8_t i;

  enum APP_OVERLAY_ID_T overlay_type;

  overlay_type = app_get_current_overlay();

  a2dp_trace_codec_name(__func__, overlay_type);

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
  APP_RESAMPLE_ITER_CALLBACK iter_cb;

  uint32_t iter_len;

  iter_cb = a2dp_resample_iter;

  if (0) {
#if defined(A2DP_AAC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_AAC) {
    iter_len = AAC_RESAMPLE_ITER_LEN;
#endif
  } else if (overlay_type == APP_OVERLAY_A2DP) {
    iter_len = SBC_RESAMPLE_ITER_LEN;
  } else {
    ASSERT(false, "%s: Bad app overlay type for play resample: %d", __func__,
           overlay_type);
  }

#ifdef CHIP_BEST1000
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 &&
      hal_sysfreq_get() <= HAL_CMU_FREQ_52M) {
    allow_resample = true;
  }
#endif
  if (allow_resample) {
#ifdef RESAMPLE_ANY_SAMPLE_RATE
    float ratio;
    if (a2dp_sample_rate % AUD_SAMPRATE_8000) {
      ratio = (float)CODEC_FREQ_22P5792M *
              ((float)a2dp_sample_rate / AUD_SAMPRATE_44100) / CODEC_FREQ_26M;
    } else {
      ratio = (float)CODEC_FREQ_24P576M *
              ((float)a2dp_sample_rate / AUD_SAMPRATE_48000) / CODEC_FREQ_26M;
    }
    a2dp_resample = app_playback_resample_any_open(AUD_CHANNEL_NUM_2, iter_cb,
                                                   iter_len, ratio);
#else
    a2dp_resample = app_playback_resample_open(
        a2dp_sample_rate, AUD_CHANNEL_NUM_2, iter_cb, iter_len);
#endif
  }
#endif

  for (i = 0; i < sizeof(sbc_eq_band_gain) / sizeof(float); i++) {
    sbc_eq_band_gain[i] = EQLevel[cfg_aud_eq_sbc_band_settings[i] + 12];
  }

  A2DP_SYNC_WITH_GET_MUTUX_FREE();
  A2DP_SYNC_WITH_PUT_MUTUX_FREE();
  if (overlay_type == APP_OVERLAY_A2DP) {
    app_audio_mempool_get_buff((uint8_t **)&sbc_decoder,
                               sizeof(btif_sbc_decoder_t));
  }

#if defined(A2DP_AAC_ON)
  if (overlay_type == APP_OVERLAY_A2DP_AAC) {
    int ret;
    aac_meminit();
    ret = aacdec_init();
    ASSERT(ret == 0, "%s: aacdec_init ERROR", __func__);
  }
#endif

#if defined(A2DP_SCALABLE_ON)
  if (overlay_type == APP_OVERLAY_A2DP_SCALABLE) {
    app_audio_mempool_get_buff((uint8_t **)&scalable_input_mid_buf,
                               SCALABLE_READBUF_SIZE);
    app_audio_mempool_get_buff((uint8_t **)&scalable_decoder_place,
                               SCALABLE_DECODER_SIZE);
    app_audio_mempool_get_buff((uint8_t **)&scalable_decoder_temp_buf,
                               SCALABLE_FRAME_SIZE * 16);
  }
  hSSDecoder = NULL;
#endif

#if defined(A2DP_LHDC_ON)
  if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    uint8_t bits_depth = bt_sbc_player_get_bitsDepth();
    AUD_SAMPRATE_T sample_rate = bt_get_sbc_sample_rate();
    TRACE(2, "a2dp_audio_init sample Rate=%d, bits_depth = %d\n", sample_rate,
          bits_depth);
    TRACE(1, "sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));

    // lhdcInit(bits_depth, sample_rate, bits_depth >= 24 ? 1 : 0);
    // extern const char __testkey_start[];
    extern const char testkey_bin[];
    lhdcSetLicenseKeyTable((uint8_t *)testkey_bin);

    TRACE(0, "lhdcSetLicenseKeyTable,testkey_bin:\n");
    for (uint8_t i = 0; i < 128; i++) {
      TRACE(1, "i = %d", i);
      DUMP8("0x%02x ", &testkey_bin[i], 32);
      i += 31;
    }
    lhdcInit(bits_depth, sample_rate, 0);
    initial_lhdc_assemble_packet(false);
  }
#endif

#ifdef SBC_QUEUE_SIZE
  if (0) {
#if defined(A2DP_AAC_ON) || defined(A2DP_SCALABLE_ON) || defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_AAC) {
#if defined(MIX_MIC_DURING_MUSIC)
    g_sbc_queue_size = app_audio_mempool_free_buff_size() - 4096;
#else
    g_sbc_queue_size = app_audio_mempool_free_buff_size();
#endif
#endif
#if defined(A2DP_SCALABLE_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_SCALABLE) {
#if defined(MIX_MIC_DURING_MUSIC)
    g_sbc_queue_size = app_audio_mempool_free_buff_size() - 4096;
#else
    g_sbc_queue_size = app_audio_mempool_free_buff_size();
#endif
#endif
#if defined(A2DP_LHDC_ON)
  } else if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
#if defined(MIX_MIC_DURING_MUSIC)
    g_sbc_queue_size = app_audio_mempool_free_buff_size() - 4096;
#else
    g_sbc_queue_size = app_audio_mempool_free_buff_size();
#endif
#endif
  } else {
    g_sbc_queue_size = SBC_QUEUE_SIZE;
  }
#else
  g_sbc_queue_size = app_audio_mempool_free_buff_size();
#endif

#if defined(A2DP_LDAC_ON)
  if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    // if((bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)){
    // app_audio_mempool_get_buff(&g_medMemHeap, MED_MEM_HEAP_SIZE);
    // speech_heap_init((uint8_t *)(&g_medMemHeap[0]), MED_MEM_HEAP_SIZE);
    g_sbc_queue_size = app_audio_mempool_free_buff_size() - MED_MEM_HEAP_SIZE;
    TRACE(1, "A2DP_LDAC_ON  g_sbc_queue_size %d\n", g_sbc_queue_size);
  }
#endif

  app_audio_mempool_get_buff(&buff, g_sbc_queue_size);
  ASSERT((buff != NULL), "%s:get sbc queue buff fail, size %d bytes!", __func__,
         g_sbc_queue_size);
  memset(buff, 0, g_sbc_queue_size);
  a2dp_audio_isrunning(A2DPPLAY_STRTEAM_PUT, false);
  a2dp_audio_isrunning(A2DPPLAY_STRTEAM_GET, false);

  LOCK_APP_AUDIO_QUEUE();
  APP_AUDIO_InitCQueue(&sbc_queue, g_sbc_queue_size, buff);
  UNLOCK_APP_AUDIO_QUEUE();

#if defined(A2DP_LDAC_ON)
  if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    ldac_buffer_used =
        app_audio_mempool_total_buf() - app_audio_mempool_free_buff_size();
    ldac_recovery = false;
    ldac_dec_init();
  }
#endif

#ifdef A2DP_AUDIO_SYNC_WITH_LOCAL
  a2dp_audio_sync_proc(A2DPPLAY_SYNC_STATUS_SET | A2DPPLAY_SYNC_STATUS_RESET,
                       A2DP_AUDIO_SYNC_WITH_LOCAL_SAMPLERATE_DEFAULT);
#endif

#ifdef A2DP_CP_ACCEL
  cp_a2dp_init();
#endif

  a2dp_cache_status = APP_AUDIO_CACHE_CACHEING;
  need_init_decoder = true;
  dec_reset_queue = false;

  sbc_frame_rev_len = 0;
  sbc_frame_num = 0;
  sbc_frame_size = SBC_TEMP_BUFFER_SIZE;
  mtu_count = 0;
  assumed_mtu_interval = 0;
  return 0;
}

int a2dp_audio_deinit(void) {
  enum APP_OVERLAY_ID_T overlay_type;

  overlay_type = app_get_current_overlay();

  a2dp_trace_codec_name(__func__, overlay_type);

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
  if (allow_resample) {
    app_playback_resample_close(a2dp_resample);
    a2dp_resample = NULL;
#ifdef CHIP_BEST1000
    allow_resample = false;
#endif
  }
#endif

#ifdef A2DP_CP_ACCEL
  cp_a2dp_deinit();
#endif

#if defined(A2DP_LDAC_ON)
  if (overlay_type == APP_OVERLAY_A2DP_LDAC) {
    ldac_dec_deinit();
  }
#endif

#if defined(A2DP_LHDC_ON)
  if (overlay_type == APP_OVERLAY_A2DP_LHDC) {
    lhdcDestroy();
  }
#endif

#if defined(A2DP_AAC_ON)
  aacdec_deinit();
  aac_memdeinit();
#endif

  A2DP_SYNC_WITH_GET_MUTUX_SET();
  A2DP_SYNC_WITH_PUT_MUTUX_SET();

  A2DP_SYNC_WITH_GET_MUTUX_FREE();
  A2DP_SYNC_WITH_PUT_MUTUX_FREE();

  a2dp_audio_isrunning(A2DPPLAY_STRTEAM_PUT, false);
  a2dp_audio_isrunning(A2DPPLAY_STRTEAM_GET, false);

  a2dp_cache_status = APP_AUDIO_CACHE_QTY;
  need_init_decoder = true;

  sbc_frame_rev_len = 0;
  sbc_frame_num = 0;
  sbc_frame_size = SBC_TEMP_BUFFER_SIZE;
  mtu_count = 0;
  assumed_mtu_interval = 0;

  return 0;
}

float a2dp_audio_latency_factor_get(void) { return 1.0f; }
#endif
