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
#ifdef A2DP_CP_ACCEL

#include "a2dp_decoder_cp.h"
#include "a2dp_decoder_internal.h"
#include "cp_accel.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "heap_api.h"
#include "norflash_api.h"

#define CP_IN_FRAME_CNT 100
#define CP_OUT_FRAME_CNT 3

#define CP_IN_CACHE_SIZE (1024 * 10)

#if defined(A2DP_LHDC_V3)
#define CP_HEAP_SIZE (1024 * 64)
#else
#define CP_HEAP_SIZE (1024 * 32)
#endif

enum CP_DEC_STATE_T {
  CP_DEC_STATE_IDLE,
  CP_DEC_STATE_WORKING,
  CP_DEC_STATE_DONE,
  CP_DEC_STATE_INIT_DONE,
};

struct CP_IN_FRAME_INFO_T {
  uint32_t pos;
  uint32_t len;
};

struct CP_OUT_FRAME_INFO_T {
  enum CP_DEC_STATE_T state;
  uint32_t pos;
  uint32_t len;
};

static CP_BSS_LOC uint32_t cp_heap_buf[CP_HEAP_SIZE / 4];
static CP_BSS_LOC heap_handle_t cp_heap;

static CP_BSS_LOC uint16_t cp_in_widx;
static CP_BSS_LOC uint16_t cp_in_ridx;

static CP_BSS_LOC uint8_t cp_out_widx;
static CP_BSS_LOC uint8_t cp_out_ridx;

static CP_BSS_LOC struct CP_IN_FRAME_INFO_T *in_frames;
static CP_BSS_LOC struct CP_OUT_FRAME_INFO_T *out_frames;

static CP_BSS_LOC uint8_t *cp_in_cache;
static CP_BSS_LOC uint8_t *cp_out_cache;

static CP_BSS_LOC bool reset_frames;

#ifdef A2DP_TRACE_CP_DEC_TIME
static CP_BSS_LOC uint32_t cp_last_dec_time;
#endif

static uint8_t max_buffer_frames = 2;
static bool mcu_dec_inited;
static A2DP_CP_DECODE_T decode_frame;
static enum CP_PROC_DELAY_T proc_delay;

static bool cp_need_reset;

CP_TEXT_SRAM_LOC
unsigned int set_cp_reset_flag(uint8_t evt) {
  cp_need_reset = true;
  return 0;
}

bool is_cp_need_reset(void) {
  bool ret = cp_need_reset;
  cp_need_reset = false;
  return ret;
}

bool is_cp_init_done(void) { return cp_accel_init_done(); }

CP_TEXT_SRAM_LOC
static void reset_frame_info(void) {
  uint32_t idle_cnt;
  int i;

  if (proc_delay == CP_PROC_DELAY_0_FRAME) {
    idle_cnt = CP_OUT_FRAME_CNT;
  } else if (proc_delay == CP_PROC_DELAY_1_FRAME) {
    idle_cnt = CP_OUT_FRAME_CNT - 1;
  } else {
    idle_cnt = CP_OUT_FRAME_CNT - 2;
  }

  for (i = 0; i < CP_OUT_FRAME_CNT; i++) {
    if (i < idle_cnt) {
      out_frames[i].state = CP_DEC_STATE_IDLE;
    } else {
      out_frames[i].state = CP_DEC_STATE_INIT_DONE;
    }
  }

  cp_in_widx = 0;
  cp_in_ridx = 0;
  cp_out_widx = 0;
  cp_out_ridx = idle_cnt;

  reset_frames = false;
}

CP_TEXT_SRAM_LOC
static unsigned int cp_a2dp_main(uint8_t event) {
  int ret;
#ifdef A2DP_TRACE_CP_DEC_TIME
  uint32_t stime;
  uint32_t etime;
#endif

  if (!mcu_dec_inited) {
    return 0;
  }

  if (decode_frame) {
    do {
#ifdef A2DP_TRACE_CP_DEC_TIME
      stime = hal_fast_sys_timer_get();
#endif

      ret = decode_frame();

#ifdef A2DP_TRACE_CP_DEC_TIME
      etime = hal_fast_sys_timer_get();
      TRACE_A2DP_DECODER_I("cp_decode: %5u us in %5u us",
                           FAST_TICKS_TO_US(etime - stime),
                           FAST_TICKS_TO_US(etime - cp_last_dec_time));
      cp_last_dec_time = etime;
#endif
    } while (ret == 0);
  }

  if (reset_frames) {
    reset_frame_info();
#ifdef A2DP_TRACE_CP_ACCEL
    TRACE_A2DP_DECODER_I("%s: Reset frames", __func__);
#endif
  }

  return 0;
}

static struct cp_task_desc TASK_DESC_A2DP = {
    CP_ACCEL_STATE_CLOSED, cp_a2dp_main, NULL, NULL, set_cp_reset_flag};
int a2dp_cp_init(A2DP_CP_DECODE_T decode_func, enum CP_PROC_DELAY_T delay) {
  if (delay >= CP_PROC_DELAY_QTY) {
    return 1;
  }
  mcu_dec_inited = false;
  decode_frame = decode_func;
  proc_delay = delay;
#ifdef A2DP_TRACE_CP_DEC_TIME
  cp_last_dec_time = hal_fast_sys_timer_get();
#endif

  norflash_api_flush_disable(NORFLASH_API_USER_CP,
                             (uint32_t)cp_accel_init_done);
  cp_accel_open(CP_TASK_A2DP_DECODE, &TASK_DESC_A2DP);
  while (cp_accel_init_done() == false) {
    hal_sys_timer_delay_us(100);
  }
  norflash_api_flush_enable(NORFLASH_API_USER_CP);
  return 0;
}
int a2dp_cp_deinit(void) {
  cp_accel_close(CP_TASK_A2DP_DECODE);

  return 0;
}

SRAM_TEXT_LOC
int a2dp_cp_decoder_init(uint32_t out_frame_len, uint8_t max_frames) {
  uint32_t i;
  max_buffer_frames = max_frames;
  if (!mcu_dec_inited) {
#ifdef A2DP_TRACE_CP_ACCEL
    TRACE_A2DP_DECODER_I("%s: Decoder init", __func__);
#endif
    if (cp_accel_init_done() == false) {
      TRACE_A2DP_DECODER_I("%s: CP ACCEL not init yet", __func__);
      return 6;
    }

    cp_heap = heap_register(cp_heap_buf, sizeof(cp_heap_buf));
    if (cp_heap == NULL) {
      return 1;
    }

    in_frames = heap_malloc(cp_heap, sizeof(in_frames[0]) * CP_IN_FRAME_CNT);
    if (in_frames == NULL) {
      return 2;
    }

    out_frames = heap_malloc(cp_heap, sizeof(out_frames[0]) * CP_OUT_FRAME_CNT);
    if (out_frames == NULL) {
      return 3;
    }

    cp_in_cache = heap_malloc(cp_heap, CP_IN_CACHE_SIZE);
    if (cp_in_cache == NULL) {
      return 4;
    }

    cp_out_cache = heap_malloc(cp_heap, out_frame_len * CP_OUT_FRAME_CNT);
    if (cp_out_cache == NULL) {
      return 5;
    }

    for (i = 0; i < CP_OUT_FRAME_CNT; i++) {
      out_frames[i].pos = out_frame_len * i;
      out_frames[i].len = out_frame_len;
    }
    reset_frame_info();

    mcu_dec_inited = true;
  }

  return 0;
}

CP_TEXT_SRAM_LOC
static uint32_t get_in_frame_cnt(uint32_t in_widx, uint32_t in_ridx) {
  uint32_t cnt;

  if (in_widx >= in_ridx) {
    cnt = in_widx - in_ridx;
  } else {
    cnt = CP_IN_FRAME_CNT - in_ridx + in_widx;
  }

  return cnt;
}

uint32_t get_in_cp_frame_cnt(void) {
  return get_in_frame_cnt(cp_in_widx, cp_in_ridx);
}

uint32_t get_in_cp_frame_delay(void) { return proc_delay; }

SRAM_TEXT_LOC
static uint32_t get_in_frame_free_cnt(uint32_t in_widx, uint32_t in_ridx) {
  uint32_t free_cnt;

  if (in_widx >= in_ridx) {
    free_cnt = CP_IN_FRAME_CNT - in_widx + in_ridx;
  } else {
    free_cnt = in_ridx - in_widx;
  }
  free_cnt -= 1;

  return free_cnt;
}

SRAM_TEXT_LOC
int a2dp_cp_put_in_frame(const void *buf1, uint32_t len1, const void *buf2,
                         uint32_t len2) {
  uint16_t free_cnt;
  uint16_t in_widx;
  uint16_t in_ridx;
  uint16_t prev_in_widx;
  uint32_t free_len;
  uint32_t in_wpos;
  uint32_t in_rpos;
  uint32_t aligned_len;

  if (reset_frames) {
    return -1;
  }

  in_widx = cp_in_widx;
  in_ridx = cp_in_ridx;

  if (max_buffer_frames < get_in_cp_frame_cnt()) {
    return 1;
  }

  free_cnt = get_in_frame_free_cnt(in_widx, in_ridx);
  if (free_cnt == 0) {
    return 1;
  }

  if (in_widx == in_ridx) {
    in_wpos = 0;
    in_rpos = 0;
  } else {
    if (in_widx == 0) {
      prev_in_widx = CP_IN_FRAME_CNT - 1;
    } else {
      prev_in_widx = in_widx - 1;
    }
    in_wpos = in_frames[prev_in_widx].pos + in_frames[prev_in_widx].len;
    if (in_wpos >= CP_IN_CACHE_SIZE) {
      in_wpos -= CP_IN_CACHE_SIZE;
    }
    in_rpos = in_frames[in_ridx].pos;
  }

  // Align to word boundary
  in_wpos = (in_wpos + 3) & ~3;
  if (in_wpos >= CP_IN_CACHE_SIZE) {
    in_wpos -= CP_IN_CACHE_SIZE;
  }
  aligned_len = (len1 + len2 + 3) & ~3;

  if (in_wpos >= in_rpos) {
    free_len = CP_IN_CACHE_SIZE - in_wpos;
    if (in_rpos == 0) {
      free_len -= 1;
    }
    if (free_len < aligned_len) {
      free_len = (in_rpos > 0) ? (in_rpos - 1) : 0;
      if (free_len < aligned_len) {
        return 2;
      }
      in_wpos = 0;
    }
  } else {
    free_len = in_rpos - in_wpos - 1;
    if (free_len < aligned_len) {
      return 3;
    }
  }

  in_frames[in_widx].pos = in_wpos;
  in_frames[in_widx].len = len1 + len2;

  if (len1) {
    memcpy(&cp_in_cache[in_wpos], buf1, len1);
    in_wpos += len1;
  }
  if (len2) {
    memcpy(&cp_in_cache[in_wpos], buf2, len2);
    in_wpos += len2;
  }

  in_widx += 1;
  if (in_widx >= CP_IN_FRAME_CNT) {
    in_widx -= CP_IN_FRAME_CNT;
  }
  cp_in_widx = in_widx;

  return 0;
}

CP_TEXT_SRAM_LOC
int a2dp_cp_get_in_frame(void **p_buf, uint32_t *p_len) {
  uint16_t in_widx;
  uint16_t in_ridx;
  uint32_t in_rpos;

  if (reset_frames) {
    return -1;
  }

  in_widx = cp_in_widx;
  in_ridx = cp_in_ridx;

  if (in_widx == in_ridx) {
    return 1;
  }

  in_rpos = in_frames[in_ridx].pos;

  if (p_buf) {
    *p_buf = &cp_in_cache[in_rpos];
  }
  if (p_len) {
    *p_len = in_frames[in_ridx].len;
  }

  return 0;
}

CP_TEXT_SRAM_LOC
int a2dp_cp_consume_in_frame(void) {
  uint16_t in_widx;
  uint16_t in_ridx;

  if (reset_frames) {
    return 0;
  }

  in_widx = cp_in_widx;
  in_ridx = cp_in_ridx;

  if (in_widx == in_ridx) {
    return 1;
  }

  in_ridx += 1;
  if (in_ridx >= CP_IN_FRAME_CNT) {
    in_ridx -= CP_IN_FRAME_CNT;
  }
  cp_in_ridx = in_ridx;

  return 0;
}

CP_TEXT_SRAM_LOC
uint32_t a2dp_cp_get_in_frame_index(void) { return cp_in_ridx; }

SRAM_TEXT_LOC
uint32_t a2dp_cp_get_in_frame_cnt_by_index(uint32_t ridx) {
  return get_in_frame_cnt(cp_in_widx, ridx);
}

SRAM_TEXT_LOC
void a2dp_cp_reset_frame(void) {
#ifdef A2DP_TRACE_CP_ACCEL
  TRACE_A2DP_DECODER_I("%s: Reset frames", __func__);
#endif

  reset_frames = true;

  // Notify CP to work again
  cp_accel_send_event_mcu2cp(1);
}

SRAM_TEXT_LOC
bool a2dp_cp_get_frame_reset_status(void) { return reset_frames; }

CP_TEXT_SRAM_LOC
enum CP_EMPTY_OUT_FRM_T a2dp_cp_get_emtpy_out_frame(void **p_buf,
                                                    uint32_t *p_len) {
  enum CP_EMPTY_OUT_FRM_T ret;
  uint8_t out_widx;
  uint32_t out_wpos;
  enum CP_DEC_STATE_T state;

  if (reset_frames) {
    return CP_EMPTY_OUT_FRM_ERR;
  }

  out_widx = cp_out_widx;
  state = out_frames[out_widx].state;

  if (state != CP_DEC_STATE_IDLE && state != CP_DEC_STATE_WORKING) {
    return CP_EMPTY_OUT_FRM_ERR;
  }

  if (state == CP_DEC_STATE_WORKING) {
    ret = CP_EMPTY_OUT_FRM_WORKING;
  } else {
    out_frames[out_widx].state = CP_DEC_STATE_WORKING;
    ret = CP_EMPTY_OUT_FRM_OK;
  }

  out_wpos = out_frames[out_widx].pos;

  if (p_buf) {
    *p_buf = &cp_out_cache[out_wpos];
  }
  if (p_len) {
    *p_len = out_frames[out_widx].len;
  }

  return ret;
}

CP_TEXT_SRAM_LOC
int a2dp_cp_consume_emtpy_out_frame(void) {
  uint8_t out_widx;

  if (reset_frames) {
    return 0;
  }

  out_widx = cp_out_widx;

  if (out_frames[out_widx].state != CP_DEC_STATE_WORKING) {
    return 1;
  }

  out_frames[out_widx].state = CP_DEC_STATE_DONE;

#ifdef A2DP_TRACE_CP_ACCEL
  TRACE_A2DP_DECODER_I("AD2P-CP out frame W[%d]", out_widx);
#endif

  out_widx += 1;
  if (out_widx >= CP_OUT_FRAME_CNT) {
    out_widx -= CP_OUT_FRAME_CNT;
  }
  cp_out_widx = out_widx;

  return 0;
}

SRAM_TEXT_LOC
int a2dp_cp_get_full_out_frame(void **p_buf, uint32_t *p_len) {
  uint8_t out_ridx;
  uint32_t out_rpos;
  enum CP_DEC_STATE_T state;

  if (reset_frames) {
    return -1;
  }

  out_ridx = cp_out_ridx;
  state = out_frames[out_ridx].state;

  if (state != CP_DEC_STATE_DONE && state != CP_DEC_STATE_INIT_DONE) {
    // Notify CP to work again
    cp_accel_send_event_mcu2cp(
        CP_BUILD_ID(CP_TASK_A2DP_DECODE, CP_EVENT_A2DP_DECODE));
    if (state == CP_DEC_STATE_WORKING) {
      return CP_EMPTY_OUT_FRM_WORKING;
    } else {
      return (10 + state);
    }
  }

  out_rpos = out_frames[out_ridx].pos;

  if (state == CP_DEC_STATE_DONE) {
    if (p_buf) {
      *p_buf = &cp_out_cache[out_rpos];
    }
    if (p_len) {
      *p_len = out_frames[out_ridx].len;
    }
  } else {
    if (p_buf) {
      *p_buf = NULL;
    }
    if (p_len) {
      *p_len = 0;
    }
  }

  return 0;
}

SRAM_TEXT_LOC
int a2dp_cp_consume_full_out_frame(void) {
  uint8_t out_ridx;
  enum CP_DEC_STATE_T state;

  if (reset_frames) {
    return 0;
  }

  out_ridx = cp_out_ridx;
  state = out_frames[out_ridx].state;

  if (state != CP_DEC_STATE_DONE && state != CP_DEC_STATE_INIT_DONE) {
    return 1;
  }

#ifdef A2DP_TRACE_CP_ACCEL
  TRACE_A2DP_DECODER_I("AD2P-CP out frame R[%d]", out_ridx);
#endif

  out_frames[out_ridx].state = CP_DEC_STATE_IDLE;

  out_ridx += 1;
  if (out_ridx >= CP_OUT_FRAME_CNT) {
    out_ridx -= CP_OUT_FRAME_CNT;
  }
  cp_out_ridx = out_ridx;

  // Notify CP to work again
  cp_accel_send_event_mcu2cp(
      CP_BUILD_ID(CP_TASK_A2DP_DECODE, CP_EVENT_A2DP_DECODE));

  return 0;
}

#endif
