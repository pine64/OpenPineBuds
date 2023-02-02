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
#include "app_utils.h"
#include "string.h"

#include "anc_process.h"
#include "audioflinger.h"
#include "hal_aud.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
//#include "audio_dump.h"
//#include "anc_usb_app.h"

#include "anc_wnr.h"
#include "app_ibrt_keyboard.h"
#include "app_ibrt_ui.h"
#include "app_thread.h"
#include "bt_drv_interface.h"
#include "cmsis_os.h"
#include "speech_cfg.h"
#include "wind_detection_2mic.h"

#if defined(SPEECH_TX_24BIT)
#define _24BITS_ENABLE
#endif

// Wind detection algorithm
#define _SAMPLE_RATE (8000)
#define _FRAME_LEN (60)
#define _CHANNEL_NUM (2)

#if defined(_24BITS_ENABLE)
#define _SAMPLE_BITS (24)
typedef int WNR_PCM_T;
#else
#define _SAMPLE_BITS (16)
typedef short WNR_PCM_T;
#endif

#define SAMPLE_BYTES sizeof(WNR_PCM_T)

#define _FRAME_LEN_MAX (128)
#define _SAMPLE_BITS_MAX (32)
#define WINDINDICATOR_SIZE (10)
#define _LOOP_CNT (3)

// ff gain coefficient tunning
#define _NO_WIND_FF_GAIN_COEF (1.0)
#define _SMALL_WIND_FF_GAIN_COEF (0.5)
#define _STRONG_WIND_FF_GAIN_COEF (0.0)
#define _STRONG2SMALL_WIND_FF_MID_GAIN_COEF (0.25)
#define _SMALL2NO_WIND_FF_MID_GAIN_COEF (0.75)

// wind indictor threshold
#define _NO_WIND_THD (0.75)
#define _SMALL_WIND_THD (0.55)
#define _STRONG_WIND_THD (0.45)

// energy threshold
#define _POWER_THD (0.1)

// ff close time
#define _PERIOD (12)
#define _TARGET_TIME (4)

//#define TEST_MIPS
#ifdef TEST_MIPS
static uint32_t pre_ticks = 0;
static uint32_t start_ticks = 0;
static uint32_t end_ticks = 0;
static uint32_t used_mips = 0;
#endif

WindDetection2MicState *wind_st = NULL;

static WindDetection2MicConfig wind_cfg = {
    .bypass = 0,
    .power_thd = _POWER_THD,
};
// static uint8_t heap_buf[1024 * 12];

// Wind factor process

extern bool app_anc_work_status(void);

// Mic operation
#define ANC_WNR_STREAM_ID AUD_STREAM_ID_0
static anc_wnr_open_mode_t g_open_mode = ANC_WNR_OPEN_MODE_QTY;
// 2ch, pingpong
#define AF_STREAM_BUFF_SIZE (_FRAME_LEN * SAMPLE_BYTES * 2 * 2)
static uint8_t __attribute__((aligned(4)))
af_stream_buff[AF_STREAM_BUFF_SIZE * _LOOP_CNT];
static uint8_t __attribute__((aligned(4)))
af_stream_mic1[_FRAME_LEN_MAX * (_SAMPLE_BITS_MAX / 8)];
static uint8_t __attribute__((aligned(4)))
af_stream_mic2[_FRAME_LEN_MAX * (_SAMPLE_BITS_MAX / 8)];

static int32_t g_sample_rate = _SAMPLE_RATE;
static int32_t g_frame_len = _FRAME_LEN;

static void _open_mic(void);
static void _close_mic(void);

#define WNR_SYNC_COUNTER_THRESHOLD_FOR_SCO 18    // talk:180ms * 18 = 3240ms
#define WNR_SYNC_COUNTER_THRESHOLD_FOR_NORMAL 35 // normal:90ms * 35 = 3150ms

// if not need to set ANC FF GAIN by trigger delay way, set macro to 0.
#define WNR_SYNC_TRIGGER_DELAY 300 // unit:ms

// if not need to printf information about sync, set macro to 0.
#define WNR_SYNC_DEBUG_LOG 1

// default disable twostage mode so that reduce delay to set ANC FF GAIN, also
// to simplify code. in order to avoid pop voice, recommend to enable twostage
// mode if chip is based on 1303 lower platform. if need to use twostage mode to
// set ANC FF GAIN, set macro to 1.
#define WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE 0

#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
#define WNR_SYNC_TWOSTAGE_DELAY 300 // unit:tick
uint8_t Twostage = 0;
uint8_t FFstate = 0;
#endif

static void app_wnr_sync_timer_handler(void const *param);
osTimerDef(APP_WNR_SYNC_TIMER, app_wnr_sync_timer_handler);
static osTimerId app_wnr_sync_timer = NULL;

#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
static void app_wnr_twostage_handler(void const *param);
osTimerDef(APP_WNR_TWOSTAGE_TIMER, app_wnr_twostage_handler);
static osTimerId app_wnr_twostage_timer = NULL;
#endif

uint8_t g_wind_st = 0;
uint8_t cnt = 0;
uint8_t period = 12;
float windsum = 0.0;
float wind_indictor = 0.0;
static uint8_t g_set_Windstate = 0;
static uint8_t g_local_Windstate = 0;
static uint8_t g_peer_Windstate = 0;
static bool g_local_Wind_module_onoff = false;
static bool g_peer_Wind_module_onoff = false;
static uint8_t wnr_sync_counter_for_sco = 0;
static uint8_t wnr_sync_counter_for_normal = 0;
static anc_wnr_open_mode_t g_wnr_open_mode = ANC_WNR_OPEN_MODE_QTY;
static bool g_wnr_notify_flag = false;
static bool g_wnr_request_flag = false;
static bool g_wnr_sync_flag = false;

struct anc_wnr_state_queue_t {
  uint8_t buf_for_sco[WNR_SYNC_COUNTER_THRESHOLD_FOR_SCO];
  uint8_t buf_for_normal[WNR_SYNC_COUNTER_THRESHOLD_FOR_NORMAL];
  uint8_t index_for_sco;
  uint8_t index_for_normal;
  uint8_t number_for_sco;
  uint8_t number_for_normal;
} g_anc_wnr_state_queue;
float windindicator[WINDINDICATOR_SIZE] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
float windthd[3] = {
    _NO_WIND_THD, _SMALL_WIND_THD,
    _STRONG_WIND_THD}; //{0.75,0.35,0.30};//{0.75,0.40,0.35};{0.75,0.45,0.40};{0.75,0.50,0.45};

#ifndef HW_SUPPORT_SMOOOTHING_GAIN
#define ANC_TCTRL_NUM 2
#define ANC_TIMER_PERIOD_GAIN 50
#define TIMER_CNT 20

struct anc_tuning_ctrl {
  uint8_t timer_init;
  uint8_t timer_ff_gain_run;
  const struct_anc_cfg *cfg;
  enum ANC_TYPE_T type;
  HWTIMER_ID timer_ff_gain_id;
};

static struct anc_tuning_ctrl anc_tctrl;

static uint32_t timer_cnt = 0;
static bool ff_smoothing_flag = false;
#endif

static int32_t anc_ff_gain_l = 512;
static int32_t anc_ff_gain_r = 512;
static int32_t tgt_ff_gain_l, tgt_ff_gain_r;
static int32_t pre_ff_gain_l, pre_ff_gain_r;

#ifndef HW_SUPPORT_SMOOOTHING_GAIN
static int32_t delta_ff_gain_l, delta_ff_gain_r;
#endif

#ifndef HW_SUPPORT_SMOOOTHING_GAIN
void anc_gain_tuning_init(void) {
  timer_cnt = 0;
  ff_smoothing_flag = false;
}

static void anc_tuning_ff_gain_timer_timeout(void *param) {
  timer_cnt++;
  int32_t ff_gain_l = 0;
  int32_t ff_gain_r = 0;
  if ((timer_cnt < delta_ff_gain_l) || (timer_cnt < delta_ff_gain_r)) {
    if (pre_ff_gain_l < tgt_ff_gain_l) {
      pre_ff_gain_l = pre_ff_gain_l + 1;
      ff_gain_l = pre_ff_gain_l;
    } else if (pre_ff_gain_l > tgt_ff_gain_l) {
      pre_ff_gain_l = pre_ff_gain_l - 1;
      ff_gain_l = pre_ff_gain_l;
    }

    if (pre_ff_gain_r < tgt_ff_gain_r) {
      pre_ff_gain_r = pre_ff_gain_r + 1;
      ff_gain_r = pre_ff_gain_r;
    } else if (pre_ff_gain_r > tgt_ff_gain_r) {
      pre_ff_gain_r = pre_ff_gain_r - 1;
      ff_gain_r = pre_ff_gain_r;
    }

    anc_set_gain(ff_gain_l, ff_gain_r, ANC_FEEDFORWARD);
    TRACE(3, "[%s] ff_gain_l = %d, ff_gain_r = %d", __func__, ff_gain_l,
          ff_gain_r);

    struct anc_tuning_ctrl *c = (struct anc_tuning_ctrl *)param;
    hwtimer_start(c->timer_ff_gain_id, US_TO_TICKS(ANC_TIMER_PERIOD_GAIN));
    ff_smoothing_flag = 1;
  } else {
    anc_set_gain(tgt_ff_gain_l, tgt_ff_gain_r, ANC_FEEDFORWARD);
    ff_smoothing_flag = 0;
  }
}
#endif

static void _set_anc_ff_gain(bool update_anc_gain, float gain_coef,
                             enum ANC_TYPE_T type) {

#ifdef HW_SUPPORT_SMOOOTHING_GAIN
  if (update_anc_gain) {
    anc_get_gain(&pre_ff_gain_l, &pre_ff_gain_r, ANC_FEEDFORWARD);
    anc_ff_gain_l = pre_ff_gain_l;
    anc_ff_gain_r = pre_ff_gain_r;
    TRACE(3, "[%s] Update anc_gain_l = %d, anc_gain_r = %d.", __func__,
          anc_ff_gain_l, anc_ff_gain_r);
  }
  tgt_ff_gain_l = (int32_t)(anc_ff_gain_l * gain_coef);
  tgt_ff_gain_r = (int32_t)(anc_ff_gain_r * gain_coef);
  TRACE(3, "[%s] tgt_ff_gain_l = %d, tgt_ff_gain_r = %d.", __func__,
        tgt_ff_gain_l, tgt_ff_gain_r);
  anc_set_gain(tgt_ff_gain_l, tgt_ff_gain_r, ANC_FEEDFORWARD);
#else
  struct anc_tuning_ctrl *c = &anc_tctrl;
  // uint32_t cur_time = TICKS_TO_MS(hal_sys_timer_get());

  // reset timer
  if (c->timer_ff_gain_run) {
    hwtimer_stop(c->timer_ff_gain_id);
  }
  anc_get_gain(&pre_ff_gain_l, &pre_ff_gain_r, ANC_FEEDFORWARD);
  TRACE(3, "[%s] pre_ff_gain_l = %d, pre_ff_gain_r = %d.", __func__,
        pre_ff_gain_l, pre_ff_gain_r);
  if (update_anc_gain && !ff_smoothing_flag) {
    anc_ff_gain_l = pre_ff_gain_l;
    anc_ff_gain_r = pre_ff_gain_r;
    TRACE(3, "[%s] Update anc_gain_l = %d, anc_gain_r = %d.", __func__,
          anc_ff_gain_l, anc_ff_gain_r);
  }
  tgt_ff_gain_l = (int32_t)(anc_ff_gain_l * gain_coef);
  tgt_ff_gain_r = (int32_t)(anc_ff_gain_r * gain_coef);
  delta_ff_gain_l = abs(tgt_ff_gain_l - pre_ff_gain_l);
  delta_ff_gain_r = abs(tgt_ff_gain_r - pre_ff_gain_r);
  TRACE(3, "[%s] tgt_ff_gain_l = %d, tgt_ff_gain_r = %d.", __func__,
        tgt_ff_gain_l, tgt_ff_gain_r);

  timer_cnt = 0;

  // timer restart
  hwtimer_start(c->timer_ff_gain_id, MS_TO_TICKS(ANC_TIMER_PERIOD_GAIN));
  c->timer_ff_gain_run = 1;
  // set gain directly
  // anc_set_gain(gain_ch_l, gain_ch_r, type);

  // AUD_TRACE(TRACE_MASK_INFO, "set anc gain :[%d], gain_ch_l=%d, gian_ch_r=%d,
  // type=%d", cur_time, gain_ch_l, gain_ch_r, (int)type);
#endif
}

void anc_release_gain(void) {
  TRACE(1, "[%s] ...Release FF gain", __func__);

  _set_anc_ff_gain(false, 1, ANC_FEEDFORWARD);
}

int32_t anc_wnr_ctrl(int32_t sample_rate, int32_t frame_len) {
  TRACE(3, "[%s] sample_rate = %d, frame_len = %d", __func__, sample_rate,
        frame_len);

  g_sample_rate = sample_rate;
  g_frame_len = frame_len;

  if (g_sample_rate == 16000) {
    g_frame_len >>= 1;
  }

  return 0;
}

static void app_wnr_reset_state_queue(void) {
  g_anc_wnr_state_queue.index_for_normal = 0;
  g_anc_wnr_state_queue.index_for_sco = 0;
  g_anc_wnr_state_queue.number_for_normal = 0;
  g_anc_wnr_state_queue.number_for_sco = 0;
  memset(g_anc_wnr_state_queue.buf_for_normal, 0,
         WNR_SYNC_COUNTER_THRESHOLD_FOR_NORMAL);
  memset(g_anc_wnr_state_queue.buf_for_sco, 0,
         WNR_SYNC_COUNTER_THRESHOLD_FOR_SCO);
}
static void app_wnr_push_state_to_queue(uint8_t open_mode, uint8_t arg0) {
  if (open_mode == ANC_WNR_OPEN_MODE_STANDALONE) // normal mode...
  {
    g_anc_wnr_state_queue
        .buf_for_normal[g_anc_wnr_state_queue.index_for_normal] = arg0;

    if (++g_anc_wnr_state_queue.index_for_normal >=
        WNR_SYNC_COUNTER_THRESHOLD_FOR_NORMAL) {
      g_anc_wnr_state_queue.index_for_normal = 0;
    }

    if (g_anc_wnr_state_queue.number_for_normal <
        WNR_SYNC_COUNTER_THRESHOLD_FOR_NORMAL) {
      g_anc_wnr_state_queue.number_for_normal++;
    }
  } else // sco mode...
  {
    g_anc_wnr_state_queue.buf_for_sco[g_anc_wnr_state_queue.index_for_sco] =
        arg0;

    if (++g_anc_wnr_state_queue.index_for_sco >=
        WNR_SYNC_COUNTER_THRESHOLD_FOR_SCO) {
      g_anc_wnr_state_queue.index_for_sco = 0;
    }

    if (g_anc_wnr_state_queue.number_for_sco <
        WNR_SYNC_COUNTER_THRESHOLD_FOR_SCO) {
      g_anc_wnr_state_queue.number_for_sco++;
    }
  }
}
static uint8_t app_wnr_get_state_from_queue(uint8_t open_mode) {
  uint8_t counter0 = 0;
  uint8_t counter1 = 0;
  uint8_t counter2 = 0;

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(2, "[%s] open_mode:%d", __func__, open_mode);
  TRACE(2, "[%s] index_for_normal:%d", __func__,
        g_anc_wnr_state_queue.index_for_normal);
  TRACE(2, "[%s] index_for_sco:%d", __func__,
        g_anc_wnr_state_queue.index_for_sco);
  TRACE(2, "[%s] number_for_normal:%d", __func__,
        g_anc_wnr_state_queue.number_for_normal);
  TRACE(2, "[%s] number_for_sco:%d", __func__,
        g_anc_wnr_state_queue.number_for_sco);
  TRACE(1, "[%s] buf_for_normal:", __func__);
  DUMP8("%d ", g_anc_wnr_state_queue.buf_for_normal,
        g_anc_wnr_state_queue.number_for_normal);
  TRACE(1, "[%s] buf_for_sco:", __func__);
  DUMP8("%d ", g_anc_wnr_state_queue.buf_for_sco,
        g_anc_wnr_state_queue.number_for_sco);
#endif

  if (open_mode == ANC_WNR_OPEN_MODE_STANDALONE) // normal mode...
  {
    for (uint8_t i = 0; i < g_anc_wnr_state_queue.number_for_normal; i++) {
      if (g_anc_wnr_state_queue.buf_for_normal[i] == 0) {
        counter0++;
        continue;
      }
      if (g_anc_wnr_state_queue.buf_for_normal[i] == 1) {
        counter1++;
        continue;
      }
      if (g_anc_wnr_state_queue.buf_for_normal[i] == 2) {
        counter2++;
        continue;
      }
    }
  } else // sco mode...
  {
    for (uint8_t i = 0; i < g_anc_wnr_state_queue.number_for_sco; i++) {
      if (g_anc_wnr_state_queue.buf_for_sco[i] == 0) {
        counter0++;
        continue;
      }
      if (g_anc_wnr_state_queue.buf_for_sco[i] == 1) {
        counter1++;
        continue;
      }
      if (g_anc_wnr_state_queue.buf_for_sco[i] == 2) {
        counter2++;
        continue;
      }
    }
  }

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(3, "counter0:%d counter1:%d counter2:%d", counter0, counter1, counter2);
#endif

  if (counter0 >= counter1) {
    if (counter0 >= counter2) {
      return 0;
    } else {
      return 2;
    }
  } else {
    if (counter1 >= counter2) {
      return 1;
    } else {
      return 2;
    }
  }
}
static void app_wnr_trigger_internal_event(uint32_t evt, uint32_t arg0,
                                           uint32_t arg1, uint32_t arg2) {
  APP_MESSAGE_BLOCK msg;
#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(5, "[%s] evt:%d arg0:%d arg1:%d arg2:%d", __func__, evt, arg0, arg1,
        arg2);
#endif
  msg.mod_id = APP_MODUAL_WNR;
  msg.msg_body.message_id = evt;
  msg.msg_body.message_Param0 = arg0;
  msg.msg_body.message_Param1 = arg1;
  msg.msg_body.message_Param2 = arg2;
  app_mailbox_put(&msg);
}
static void app_wnr_share_module_info(void) {
  uint8_t buf[3] = {0};

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(2, "[%s] local_module_onoff:%d", __func__, g_local_Wind_module_onoff);
#endif

  buf[0] = IBRT_ACTION_SYNC_WNR;
  buf[1] = (uint8_t)APP_WNR_SHARE_MODULE_INFO;
  buf[2] = (uint8_t)g_local_Wind_module_onoff;

  app_ibrt_ui_send_user_action(buf, 3);
}
void app_wnr_sync_state(void) {
#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(1, "[%s] enter...", __func__);
#endif

  wnr_sync_counter_for_sco = 0;
  wnr_sync_counter_for_normal = 0;
  g_wnr_notify_flag = false;
  g_wnr_request_flag = false;
  g_peer_Wind_module_onoff = false;
  g_peer_Windstate = 0;

  if (g_local_Wind_module_onoff == true) {
    g_wnr_sync_flag = true;
  }
}

#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
static void app_wnr_twostage_handler(void const *param) {
  float gain_coef = 0.0;

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(1, "[%s] enter...", __func__);
#endif

  if ((Twostage == 1) && (FFstate == 7)) {
    FFstate = 1; // 25%->50%
    Twostage = 0;
    gain_coef = _SMALL_WIND_FF_GAIN_COEF;
    _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);

    TRACE(1, "[%s] Enable 0.5 FF ANC.", __func__);
  } else if ((Twostage == 1) && (FFstate == 6)) {
    FFstate = 0; // 75%->100%
    Twostage = 0;
    gain_coef = _NO_WIND_FF_GAIN_COEF;
    _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
    TRACE(1, "[%s] Enable FF ANC.", __func__);
  }
}
#endif

static void app_wnr_sync_timer_handler(void const *param) {
  uint8_t temp = *((uint8_t *)param);
#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(2, "[%s] arg0:%d", __func__, temp);
#endif

  if (g_local_Wind_module_onoff == true)
    app_wnr_trigger_internal_event(APP_WNR_EXCUTE_TRIGGER, (uint32_t)temp, 0,
                                   0);
}
static void app_wnr_notify_detect_result(void) {
  uint8_t buf[3] = {0};

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(2, "[%s] local_Windstate:%d", __func__, g_local_Windstate);
#endif
  g_wnr_sync_flag = false;
  g_wnr_notify_flag = true;

  buf[0] = IBRT_ACTION_SYNC_WNR;
  buf[1] = (uint8_t)APP_WNR_NOTIFY_DETECT_RESULT;
  buf[2] = g_local_Windstate;

  app_ibrt_ui_send_user_action(buf, 3);
}
static void app_wnr_request_detect_result(void) {
  uint8_t buf[3] = {0};

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(2, "[%s] local_Windstate:%d", __func__, g_local_Windstate);
#endif
  g_wnr_sync_flag = false;
  g_wnr_request_flag = true;

  buf[0] = IBRT_ACTION_SYNC_WNR;
  buf[1] = (uint8_t)APP_WNR_REQUEST_DETECT_RESULT;
  buf[2] = g_local_Windstate;

  app_ibrt_ui_send_user_action(buf, 3);
}
static void app_wnr_response_detect_result(uint32_t arg0) {
  uint8_t buf[3] = {0};

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(1, "[%s] enter...", __func__);
#endif

  if (g_wnr_notify_flag == true) {
    g_wnr_notify_flag = false;
    return;
  }

  g_wnr_sync_flag = false;
  g_local_Windstate = app_wnr_get_state_from_queue(g_wnr_open_mode);
  g_peer_Windstate = (uint8_t)arg0;

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(3, "[%s] local_Windstate:%d peer_Windstate:%d", __func__,
        g_local_Windstate, g_peer_Windstate);
#endif

  buf[0] = IBRT_ACTION_SYNC_WNR;
  buf[1] = (uint8_t)APP_WNR_RESPONSE_DETECT_RESULT;
  buf[2] = g_local_Windstate;

  app_ibrt_ui_send_user_action(buf, 3);
}
static void app_wnr_process_detect_result(uint32_t arg0) {
  if (g_wnr_request_flag == false) {
    g_local_Windstate = app_wnr_get_state_from_queue(g_wnr_open_mode);
  }

  g_wnr_sync_flag = false;
  g_wnr_request_flag = false;
  g_peer_Windstate = (uint8_t)arg0;

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(3, "[%s] local_Windstate:%d peer_wnr_state:%d", __func__,
        g_local_Windstate, arg0);
#endif

  if (g_local_Windstate >= g_peer_Windstate) {
    if (g_local_Wind_module_onoff == true)
      app_wnr_trigger_internal_event(APP_WNR_SET_TRIGGER,
                                     (uint32_t)g_local_Windstate, 0, 0);
  } else {
    if (g_local_Wind_module_onoff == true)
      app_wnr_trigger_internal_event(APP_WNR_SET_TRIGGER,
                                     (uint32_t)g_peer_Windstate, 0, 0);
  }
}
static void app_wnr_set_trigger(uint32_t arg0, uint32_t arg1) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  uint32_t current_ticks = 0;

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(2, "[%s] set_Windstate:%d", __func__, arg0);
#endif

  if ((app_tws_ibrt_tws_link_connected() == true) &&
      (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
      (g_peer_Wind_module_onoff == true)) {
    uint8_t buf[7] = {0};
    current_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle);
    uint32_t tg_ticks = current_ticks + MS_TO_TICKS(WNR_SYNC_TRIGGER_DELAY);

#if (WNR_SYNC_DEBUG_LOG == 1)
    TRACE(3, "[%s] current_ticks:%d tg_ticks:%d", __func__, current_ticks,
          tg_ticks);
#endif

    buf[0] = IBRT_ACTION_SYNC_WNR;
    buf[1] = (uint8_t)APP_WNR_SET_TRIGGER;
    buf[2] = (uint8_t)arg0;
    buf[3] = (uint8_t)((tg_ticks & 0xff000000) >> 24);
    buf[4] = (uint8_t)((tg_ticks & 0x00ff0000) >> 16);
    buf[5] = (uint8_t)((tg_ticks & 0x0000ff00) >> 8);
    buf[6] = (uint8_t)(tg_ticks & 0x000000ff);

    app_ibrt_ui_send_user_action(buf, 7);

    if ((app_wnr_sync_timer != NULL) && (tg_ticks != current_ticks)) {
      g_set_Windstate = (uint8_t)arg0;
      osTimerStop(app_wnr_sync_timer);
      osTimerStart(app_wnr_sync_timer, WNR_SYNC_TRIGGER_DELAY);
    } else {
      if (g_local_Wind_module_onoff == true)
        app_wnr_trigger_internal_event(APP_WNR_EXCUTE_TRIGGER, arg0, 0, 0);
    }
    return;
  }

  if ((app_tws_ibrt_tws_link_connected() == true) &&
      (p_ibrt_ctrl->nv_role == IBRT_SLAVE) &&
      (g_peer_Wind_module_onoff == true)) {
    g_wnr_notify_flag = false;
    current_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle);

#if (WNR_SYNC_DEBUG_LOG == 1)
    TRACE(3, "[%s] current_ticks:%d tg_ticks:%d", __func__, current_ticks,
          arg1);
#endif

    if (arg1 > current_ticks) {
      uint32_t diff_ticks = arg1 - current_ticks;

      g_set_Windstate = (uint8_t)arg0;
      if (app_wnr_sync_timer != NULL) {
        osTimerStop(app_wnr_sync_timer);
        osTimerStart(app_wnr_sync_timer, TICKS_TO_MS(diff_ticks));
      }
    } else {
#if (WNR_SYNC_DEBUG_LOG == 1)
      TRACE(1, "[%s] tg_ticks < current_ticks...", __func__);
#endif
      if (g_local_Wind_module_onoff == true)
        app_wnr_trigger_internal_event(APP_WNR_EXCUTE_TRIGGER, arg0, 0, 0);
    }
    return;
  }

  if ((app_tws_ibrt_tws_link_connected() == false) ||
      (g_peer_Wind_module_onoff == false)) {
    if (g_local_Wind_module_onoff == true)
      app_wnr_trigger_internal_event(APP_WNR_EXCUTE_TRIGGER, arg0, 0, 0);
    return;
  }
}

static void app_wnr_excute_trigger(uint32_t arg0) {
  uint8_t t_Windstate = (uint8_t)arg0;
  float gain_coef = 0.0;

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(3, "[%s] last_Windstate:%d set_Windstate:%d", __func__, g_wind_st,
        t_Windstate);
#endif

  if ((t_Windstate != g_wind_st) && (app_anc_work_status())) {
    if (g_wind_st > t_Windstate) {
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
      Twostage = 1;
#endif
      if (t_Windstate == 1) {
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
        FFstate = 7; // 25%
        gain_coef = _STRONG2SMALL_WIND_FF_MID_GAIN_COEF;
        _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
        TRACE(1, "[%s] Enable 0.25 FF ANC.", __func__);
#else
        gain_coef = _SMALL_WIND_FF_GAIN_COEF;
        _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
        TRACE(1, "[%s] Enable 0.5 FF ANC.", __func__);
#endif
      } // 2->1 0%->25%->50%
      else if (t_Windstate == 0) {
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
        FFstate = 6; // 75%
        gain_coef = _SMALL2NO_WIND_FF_MID_GAIN_COEF;
        _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
        TRACE(1, "[%s] Enable 0.75 FF ANC.", __func__);
#else
        gain_coef = _NO_WIND_FF_GAIN_COEF;
        _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
        TRACE(1, "[%s] Enable FF ANC.", __func__);
#endif
      } // 1->0 50%->75%->100%
    } else {
      if (t_Windstate == 2) // 0%
      {
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
        FFstate = 2;
#endif
        gain_coef = _STRONG_WIND_FF_GAIN_COEF;
        _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
        TRACE(1, "[%s] Disable FF ANC.", __func__);
      } else if (t_Windstate == 1) // 50%
      {
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
        FFstate = 1;
#endif
        gain_coef = _SMALL_WIND_FF_GAIN_COEF;
        _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
        TRACE(1, "[%s] Enable 0.5 FF ANC.", __func__);
      } else {
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
        FFstate = 0;
#endif
        gain_coef = _NO_WIND_FF_GAIN_COEF;
        _set_anc_ff_gain(false, gain_coef, ANC_FEEDFORWARD);
        TRACE(1, "[%s] Enable FF ANC.", __func__);
      }
    }

    g_wind_st = t_Windstate;
  }

#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
  if (((Twostage == 1) && (FFstate == 7)) ||
      ((Twostage == 1) && (FFstate == 6))) {
    if (app_wnr_twostage_timer != NULL) {
      osTimerStop(app_wnr_twostage_timer);
      osTimerStart(app_wnr_twostage_timer, WNR_SYNC_TWOSTAGE_DELAY);
    }
  }
#endif
}

static int app_wnr_internal_event_process(APP_MESSAGE_BODY *msg_body) {
  uint32_t evt = msg_body->message_id;
  uint32_t arg0 = msg_body->message_Param0;
  uint32_t arg1 = msg_body->message_Param1;

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(4, "[%s] evt:%d arg0:%d arg1:%d", __func__, evt, arg0, arg1);
#endif

  wnr_sync_counter_for_sco = 0;
  wnr_sync_counter_for_normal = 0;

  switch (evt) {
  case APP_WNR_NOTIFY_DETECT_RESULT:
    app_wnr_notify_detect_result();
    break;
  case APP_WNR_REQUEST_DETECT_RESULT:
    app_wnr_request_detect_result();
    break;
  case APP_WNR_RESPONSE_DETECT_RESULT:
    app_wnr_response_detect_result(arg0);
    break;
  case APP_WNR_PROCESS_DETECT_RESULT:
    app_wnr_process_detect_result(arg0);
    break;
  case APP_WNR_SET_TRIGGER:
    app_wnr_set_trigger(arg0, arg1);
    break;
  case APP_WNR_EXCUTE_TRIGGER:
    app_wnr_excute_trigger(arg0);
    break;
  default:
    TRACE(2, "[%s] invalid evt:%d", __func__, evt);
    break;
  }

  return 0;
}

void app_wnr_cmd_receive_process(uint8_t *p_buff, uint16_t length) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

#if (WNR_SYNC_DEBUG_LOG == 1)
  TRACE(1, "[%s] enter...", __func__);
#endif

  if ((p_buff[0] == IBRT_ACTION_SYNC_WNR) &&
      (p_buff[1] == APP_WNR_NOTIFY_DETECT_RESULT) &&
      (app_tws_ibrt_tws_link_connected() == true) &&
      (p_ibrt_ctrl->nv_role == IBRT_MASTER)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
    TRACE(1, "NOTIFY_DETECT_RESULT peer_wnr_state:%d", p_buff[2]);
#endif
    g_peer_Wind_module_onoff = true;
    if (g_local_Wind_module_onoff == true)
      app_wnr_trigger_internal_event(APP_WNR_PROCESS_DETECT_RESULT,
                                     (uint32_t)p_buff[2], 0, 0);
    return;
  }

  if ((p_buff[0] == IBRT_ACTION_SYNC_WNR) &&
      (p_buff[1] == APP_WNR_REQUEST_DETECT_RESULT) &&
      (app_tws_ibrt_tws_link_connected() == true) &&
      (p_ibrt_ctrl->nv_role == IBRT_SLAVE)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
    TRACE(1, "REQUEST_DETECT_RESULT peer_wnr_state:%d", p_buff[2]);
#endif
    g_peer_Wind_module_onoff = true;
    if (g_local_Wind_module_onoff == true)
      app_wnr_trigger_internal_event(APP_WNR_RESPONSE_DETECT_RESULT,
                                     (uint32_t)p_buff[2], 0, 0);
    return;
  }

  if ((p_buff[0] == IBRT_ACTION_SYNC_WNR) &&
      (p_buff[1] == APP_WNR_RESPONSE_DETECT_RESULT) &&
      (app_tws_ibrt_tws_link_connected() == true) &&
      (p_ibrt_ctrl->nv_role == IBRT_MASTER)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
    TRACE(1, "RESPONSE_DETECT_RESULT peer_wnr_state:%d", p_buff[2]);
#endif
    g_peer_Wind_module_onoff = true;
    if (g_local_Wind_module_onoff == true)
      app_wnr_trigger_internal_event(APP_WNR_PROCESS_DETECT_RESULT,
                                     (uint32_t)p_buff[2], 0, 0);
    return;
  }

  if ((p_buff[0] == IBRT_ACTION_SYNC_WNR) &&
      (p_buff[1] == APP_WNR_SET_TRIGGER) &&
      (app_tws_ibrt_tws_link_connected() == true) &&
      (p_ibrt_ctrl->nv_role == IBRT_SLAVE)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
    TRACE(1, "SET_TRIGGER set_Windstate:%d", p_buff[2]);
#endif
    uint32_t tg_ticks = 0;
    tg_ticks += p_buff[3];
    tg_ticks = tg_ticks << 8;
    tg_ticks += p_buff[4];
    tg_ticks = tg_ticks << 8;
    tg_ticks += p_buff[5];
    tg_ticks = tg_ticks << 8;
    tg_ticks += p_buff[6];
    if (g_local_Wind_module_onoff == true)
      app_wnr_trigger_internal_event(APP_WNR_SET_TRIGGER, (uint32_t)p_buff[2],
                                     tg_ticks, 0);
    return;
  }

  if ((p_buff[0] == IBRT_ACTION_SYNC_WNR) &&
      (p_buff[1] == APP_WNR_SHARE_MODULE_INFO)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
    TRACE(1, "APP_WNR_SHARE_MODULE_INFO peer_module_onoff:%d", p_buff[2]);
#endif
    g_peer_Wind_module_onoff = (bool)p_buff[2];

    if ((app_tws_ibrt_tws_link_connected() == true) &&
        (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
        (g_local_Wind_module_onoff == true) &&
        (g_peer_Wind_module_onoff == true)) {
      wnr_sync_counter_for_normal = 0;
      wnr_sync_counter_for_sco = 0;
      if (g_local_Wind_module_onoff == true) {
        g_wnr_sync_flag = true;
      }
      return;
    }
    if ((app_tws_ibrt_tws_link_connected() == true) &&
        (p_ibrt_ctrl->nv_role == IBRT_SLAVE) &&
        (g_local_Wind_module_onoff == true) &&
        (g_peer_Wind_module_onoff == true)) {
      wnr_sync_counter_for_normal = 0;
      wnr_sync_counter_for_sco = 0;
      if (g_local_Wind_module_onoff == true) {
        g_wnr_sync_flag = true;
      }
      return;
    }
    return;
  }
}

static bool wnr_open_flg = 0;
int32_t anc_wnr_open(anc_wnr_open_mode_t mode) {
  if (wnr_open_flg == 1) {
    return 0;
  }
  TRACE(4, "[%s] mode = %d, g_sample_rate = %d, g_frame_len = %d", __func__,
        mode, g_sample_rate, g_frame_len);

  hal_sysfreq_req(APP_SYSFREQ_USER_ANC_WNR, HAL_CMU_FREQ_26M);
  TRACE(2, "[%s] Sys freq: %d", __func__, hal_sys_timer_calc_cpu_freq(5, 0));

  app_wnr_reset_state_queue();
  app_set_threadhandle(APP_MODUAL_WNR, app_wnr_internal_event_process);
  g_local_Wind_module_onoff = true;
  if (app_wnr_sync_timer == NULL) {
    app_wnr_sync_timer = osTimerCreate(osTimer(APP_WNR_SYNC_TIMER), osTimerOnce,
                                       &g_set_Windstate);
  }
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
  if (app_wnr_twostage_timer == NULL) {
    app_wnr_twostage_timer =
        osTimerCreate(osTimer(APP_WNR_TWOSTAGE_TIMER), osTimerOnce, NULL);
  }
#endif
  g_set_Windstate = 0;
  g_local_Windstate = 0;
  g_peer_Windstate = 0;
  g_wnr_notify_flag = false;
  g_wnr_request_flag = false;
  g_wnr_sync_flag = false;
  g_wind_st = 0;
  anc_ff_gain_l = 512;
  anc_ff_gain_r = 512;
  app_wnr_share_module_info();
  if (mode == ANC_WNR_OPEN_MODE_STANDALONE) {
    g_wnr_open_mode = ANC_WNR_OPEN_MODE_STANDALONE; // normal mode...
    wnr_sync_counter_for_normal = 0;
    wnr_sync_counter_for_sco = 0;
    g_sample_rate = _SAMPLE_RATE;
    g_frame_len = _FRAME_LEN;
    wind_st = WindDetection2Mic_create(_SAMPLE_RATE, _SAMPLE_BITS, _FRAME_LEN,
                                       &wind_cfg);

    _open_mic();

    // audio_dump_init(_FRAME_LEN, sizeof(short), 2);

  } else if (mode == ANC_WNR_OPEN_MODE_CONFIGURE) {
    g_wnr_open_mode = ANC_WNR_OPEN_MODE_CONFIGURE; // sco mode...
    wnr_sync_counter_for_sco = 0;
    wnr_sync_counter_for_normal = 0;
    ASSERT(g_sample_rate == 8000 || g_sample_rate == 16000,
           "[%s] g_sample_rate(%d) is invalid.", __func__, g_sample_rate);
    ASSERT(g_frame_len == 60 || g_frame_len == 120,
           "[%s] g_frame_len(%d) is invalid.", __func__, g_frame_len);

    wind_st = WindDetection2Mic_create(_SAMPLE_RATE, _SAMPLE_BITS, g_frame_len,
                                       &wind_cfg);

    // audio_dump_init(g_frame_len, sizeof(int), 2);

  } else {
    ASSERT(0, "[%s] mode(%d) is invalid.", __func__, mode);
  }

#ifndef HW_SUPPORT_SMOOOTHING_GAIN
  anc_gain_tuning_init();

  struct anc_tuning_ctrl *c = &anc_tctrl;

  if (!c->timer_init) {
    c->timer_ff_gain_id =
        hwtimer_alloc(anc_tuning_ff_gain_timer_timeout, (void *)c);
    c->timer_ff_gain_run = 0;
    c->timer_init = 1;
  }
#endif

  g_open_mode = mode;

  TRACE(1, "[%s] End...", __func__);
  wnr_open_flg = 1;
  return 0;
}

int32_t anc_wnr_close(void) {
  TRACE(1, "[%s] ...", __func__);

#ifndef HW_SUPPORT_SMOOOTHING_GAIN
  struct anc_tuning_ctrl *c = &anc_tctrl;
  if (c->timer_ff_gain_run) {
    hwtimer_stop(c->timer_ff_gain_id);
  }
  hwtimer_free(c->timer_ff_gain_id);
  c->timer_init = 0;
#endif

  if (g_open_mode == ANC_WNR_OPEN_MODE_STANDALONE) {
    _close_mic();
    g_open_mode = ANC_WNR_OPEN_MODE_QTY;
  }

  WindDetection2Mic_destroy(wind_st);

  // size_t total = 0, used = 0, max_used = 0;
  // speech_memory_info(&total, &used, &max_used);
  // TRACE(3,"ANC WNR MALLOC MEM: total - %d, used - %d, max_used - %d.", total,
  // used, max_used); ASSERT(used == 0, "[%s] used != 0", __func__);
  app_wnr_reset_state_queue();
  app_set_threadhandle(APP_MODUAL_WNR, NULL);
  g_local_Wind_module_onoff = false;
  if (app_wnr_sync_timer != NULL) {
    osTimerStop(app_wnr_sync_timer);
  }
#if (WNR_SYNC_SET_ANC_FF_GAIN_TWOSTAGE == 1)
  if (app_wnr_twostage_timer != NULL) {
    osTimerStop(app_wnr_twostage_timer);
  }
#endif
  g_wnr_open_mode = ANC_WNR_OPEN_MODE_QTY;
  wnr_sync_counter_for_normal = 0;
  wnr_sync_counter_for_sco = 0;
  g_set_Windstate = 0;
  g_local_Windstate = 0;
  g_peer_Windstate = 0;
  g_wnr_notify_flag = false;
  g_wnr_request_flag = false;
  g_wnr_sync_flag = false;
  g_wind_st = 0;
  anc_ff_gain_l = 512;
  anc_ff_gain_r = 512;
  app_wnr_share_module_info();

  hal_sysfreq_req(APP_SYSFREQ_USER_ANC_WNR, HAL_CMU_FREQ_32K);
  wnr_open_flg = 0;
  return 0;
}

// TODO: Provide API to configure performance
static int32_t anc_wnr_process_frame(WNR_PCM_T *inF, WNR_PCM_T *inR,
                                     uint32_t frame_len) {
  // static short mutetimer = 0;
  // short targettimer = _TARGET_TIME;
  float windictor = 0.0;
  float wind_power;

  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (cnt <= _PERIOD) {
#if _SAMPLE_BITS == 16
    windictor = WindDetection2Mic_process_16bit(wind_st, inF, inR, frame_len,
                                                &wind_power);
#else
    windictor = WindDetection2Mic_process_24bit(wind_st, inF, inR, frame_len,
                                                &wind_power);
#endif

    // TRACE(2,"[%s] wind_power = %d.", __func__, (int)(wind_power*10000));
    windsum = windsum + windictor;
    cnt = cnt + 1;
    // TRACE(2,"[%s] cnt = %d, windsum = %d.", __func__, cnt, (int)windsum);
  }
  if (cnt == period) {
    cnt = 0;
    wind_indictor = 0.8 * wind_indictor + 0.2 * windsum / period;
    windsum = 0.0;

    // TRACE(2,"[%s] wind_indictor = %d.", __func__, (int)(wind_indictor*1000));

    // float gain_coef;

    static uint8_t Windstate = 0;
    // Windstate = wind_state_detect(g_wind_st, wind_indictor);
    // TRACE(2,"[%s] windstate = %d.", __func__, Windstate);
    // mutetimer = mutetimer + 1;
    wind_state_detect(g_wind_st, wind_indictor, windindicator, windthd,
                      &Windstate);

    for (int i = WINDINDICATOR_SIZE - 1; i > 0; i--) {
      windindicator[i] = windindicator[i - 1];
    }
    windindicator[0] = wind_indictor;

    // TRACE(2,"[%s] windstate = %d.", __func__, Windstate);

    app_wnr_push_state_to_queue(g_wnr_open_mode, Windstate);

    if (g_wnr_open_mode == ANC_WNR_OPEN_MODE_STANDALONE) {
      if (++wnr_sync_counter_for_normal >=
          WNR_SYNC_COUNTER_THRESHOLD_FOR_NORMAL) {
        wnr_sync_counter_for_normal = 0;
        wnr_sync_counter_for_sco = 0;
        g_local_Windstate = app_wnr_get_state_from_queue(g_wnr_open_mode);
#if (WNR_SYNC_DEBUG_LOG == 1)
        TRACE(1, "[%s] OPEN_MODE_STANDALONE", __func__);
        TRACE(2, "[%s] local_Windstate:%d", __func__, g_local_Windstate);
        TRACE(2, "[%s] last_Windstate:%d", __func__, g_wind_st);
#endif
        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
            (g_wnr_sync_flag == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] REQUEST_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_REQUEST_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_SLAVE) && (g_wnr_sync_flag == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] NOTIFY_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_NOTIFY_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
            (g_local_Windstate != g_wind_st) &&
            (g_peer_Wind_module_onoff == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] REQUEST_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_REQUEST_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_SLAVE) &&
            (g_local_Windstate != g_wind_st) &&
            (g_peer_Wind_module_onoff == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] NOTIFY_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_NOTIFY_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if (((app_tws_ibrt_tws_link_connected() == false) ||
             (g_peer_Wind_module_onoff == false)) &&
            (g_local_Windstate != g_wind_st)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] SET_TRIGGER", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_SET_TRIGGER,
                                           (uint32_t)g_local_Windstate, 0, 0);
          return 0;
        }
      }
    } else {
      if (++wnr_sync_counter_for_sco >= WNR_SYNC_COUNTER_THRESHOLD_FOR_SCO) {
        wnr_sync_counter_for_sco = 0;
        wnr_sync_counter_for_normal = 0;
        g_local_Windstate = app_wnr_get_state_from_queue(g_wnr_open_mode);
#if (WNR_SYNC_DEBUG_LOG == 1)
        TRACE(1, "[%s] OPEN_MODE_CONFIGURE", __func__);
        TRACE(2, "[%s] local_Windstate:%d", __func__, g_local_Windstate);
        TRACE(2, "[%s] last_Windstate:%d", __func__, g_wind_st);
#endif
        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
            (g_wnr_sync_flag == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] REQUEST_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_REQUEST_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_SLAVE) && (g_wnr_sync_flag == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] NOTIFY_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_NOTIFY_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
            (g_local_Windstate != g_wind_st) &&
            (g_peer_Wind_module_onoff == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] REQUEST_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_REQUEST_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if ((app_tws_ibrt_tws_link_connected() == true) &&
            (p_ibrt_ctrl->nv_role == IBRT_SLAVE) &&
            (g_local_Windstate != g_wind_st) &&
            (g_peer_Wind_module_onoff == true)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] NOTIFY_DETECT_RESULT", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_NOTIFY_DETECT_RESULT, 0, 0,
                                           0);
          return 0;
        }

        if (((app_tws_ibrt_tws_link_connected() == false) ||
             (g_peer_Wind_module_onoff == false)) &&
            (g_local_Windstate != g_wind_st)) {
#if (WNR_SYNC_DEBUG_LOG == 1)
          TRACE(1, "[%s] SET_TRIGGER", __func__);
#endif
          if (g_local_Wind_module_onoff == true)
            app_wnr_trigger_internal_event(APP_WNR_SET_TRIGGER,
                                           (uint32_t)g_local_Windstate, 0, 0);
          return 0;
        }
      }
    }
  }
  return 0;
}

static void inline stereo_resample_16k(WNR_PCM_T *pcm_buf, uint32_t pcm_len,
                                       WNR_PCM_T *mic1, WNR_PCM_T *mic2) {
  uint32_t frame_len = pcm_len / _CHANNEL_NUM;

  const float num[3] = {0.020083, 0.040166, 0.020083};
  const float den[3] = {1.000000, -1.561018, 0.641351};

  static float y0 = 0, y1 = 0, y2 = 0, x0 = 0, x1 = 0, x2 = 0;
  static float Y0 = 0, Y1 = 0, Y2 = 0, X0 = 0, X1 = 0, X2 = 0;

  for (uint32_t i = 0; i < frame_len; i++) {
    x0 = pcm_buf[_CHANNEL_NUM * i + 0];
    X0 = pcm_buf[_CHANNEL_NUM * i + 1];

    y0 = x0 * num[0] + x1 * num[1] + x2 * num[2] - y1 * den[1] - y2 * den[2];
    Y0 = X0 * num[0] + X1 * num[1] + X2 * num[2] - Y1 * den[1] - Y2 * den[2];

    y2 = y1;
    y1 = y0;
    x2 = x1;
    x1 = x0;

    Y2 = Y1;
    Y1 = Y0;
    X2 = X1;
    X1 = X0;

    if (i % 2 == 0) {
#if defined(_24BITS_ENABLE)
      mic1[i / 2] = speech_ssat_int24((int32_t)y0);
      mic2[i / 2] = speech_ssat_int24((int32_t)Y0);
#else
      mic1[i / 2] = speech_ssat_int16((int32_t)y0);
      mic2[i / 2] = speech_ssat_int16((int32_t)Y0);
#endif
    }
  }
}

int32_t anc_wnr_process(void *pcm_buf, uint32_t pcm_len) {
  if (wnr_open_flg == 0) {
    TRACE(2, "[%s] WARNING: wnr_open_flg = %d", __func__, wnr_open_flg);
    return 0;
  }
  TRACE(2, "[%s] pcm_len = %d", __func__, pcm_len);

  // audio_dump_clear_up();

  WNR_PCM_T *tmp_buf = (WNR_PCM_T *)pcm_buf;
  if (g_open_mode != ANC_WNR_OPEN_MODE_CONFIGURE) {
    return 1;
  }

  // resample 16k-->8k
  if (g_sample_rate == 16000) {
    stereo_resample_16k(tmp_buf, pcm_len, (WNR_PCM_T *)af_stream_mic1,
                        (WNR_PCM_T *)af_stream_mic2);
    // 2ch --> 1ch, 16k --> 8k
    pcm_len = pcm_len / _CHANNEL_NUM / 2;
  } else {

    WNR_PCM_T *mic1 = (WNR_PCM_T *)af_stream_mic1;
    WNR_PCM_T *mic2 = (WNR_PCM_T *)af_stream_mic2;
    pcm_len = pcm_len / _CHANNEL_NUM;
    for (uint32_t i = 0; i < pcm_len; i++) {
      mic1[i] = tmp_buf[2 * i];
      mic2[i] = tmp_buf[2 * i + 1];
    }
  }

  // TRACE(2,"[%s] new pcm_len = %d", __func__, pcm_len);

  anc_wnr_process_frame((WNR_PCM_T *)af_stream_mic1,
                        (WNR_PCM_T *)af_stream_mic2, pcm_len);

  return 0;
}

// uint32_t wnr_ticks;

static uint32_t anc_wnr_callback(uint8_t *buf, uint32_t len) {

  // TRACE(2,"[%s] len = %d", __func__, len);
  // audio_dump_clear_up();
#ifdef TEST_MIPS
  start_ticks = hal_fast_sys_timer_get();
#endif
  int32_t frame_len = len / SAMPLE_BYTES / _CHANNEL_NUM / _LOOP_CNT;
  ASSERT(frame_len == _FRAME_LEN, "[%s] frame len(%d) is invalid.", __func__,
         frame_len);
  WNR_PCM_T *pcm_buf = (WNR_PCM_T *)buf;

  WNR_PCM_T *mic1 = (WNR_PCM_T *)af_stream_mic1;
  WNR_PCM_T *mic2 = (WNR_PCM_T *)af_stream_mic2;

  for (int32_t j = 0; j < _LOOP_CNT; j++) {
    for (int32_t i = 0; i < frame_len; i++) {
      mic1[i] = pcm_buf[_CHANNEL_NUM * i + 0];
      mic2[i] = pcm_buf[_CHANNEL_NUM * i + 1];
    }

    anc_wnr_process_frame((WNR_PCM_T *)mic1, (WNR_PCM_T *)mic2, frame_len);
    pcm_buf += _FRAME_LEN * _CHANNEL_NUM;
  }

#ifdef TEST_MIPS
  end_ticks = hal_fast_sys_timer_get();
  used_mips = (end_ticks - start_ticks) * 1000 / (start_ticks - pre_ticks);
  TRACE(2, "[%s] Usage: %d in a thousand (MIPS).", __func__, used_mips);
  // wnr_ticks = start_ticks;
  // TRACE(2,"[%s] WNR frame takes %d ms.", __func__,
  // FAST_TICKS_TO_MS((start_ticks - pre_ticks)*100));
  pre_ticks = start_ticks;
#endif

  return len;
}

static void _open_mic(void) {
  struct AF_STREAM_CONFIG_T stream_cfg;

  TRACE(1, "[%s] ...", __func__);

  memset(&stream_cfg, 0, sizeof(stream_cfg));
  stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)_CHANNEL_NUM;
  stream_cfg.sample_rate = (enum AUD_SAMPRATE_T)_SAMPLE_RATE;
  stream_cfg.bits = (enum AUD_BITS_T)_SAMPLE_BITS;
  stream_cfg.vol = 12;
  stream_cfg.chan_sep_buf = false;
  stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
  stream_cfg.io_path = AUD_INPUT_PATH_ANC_WNR;
  stream_cfg.handler = anc_wnr_callback;
  stream_cfg.data_size = sizeof(af_stream_buff);
  stream_cfg.data_ptr = af_stream_buff;
  ASSERT(stream_cfg.channel_num == 2, "[%s] channel number(%d) is invalid.",
         __func__, stream_cfg.channel_num);
  TRACE(3, "[%s] sample_rate:%d, data_size:%d", __func__,
        stream_cfg.sample_rate, stream_cfg.data_size);
  TRACE(2, "[%s] af_stream_buff = %p", __func__, af_stream_buff);

  af_stream_open(ANC_WNR_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg);
  af_stream_start(ANC_WNR_STREAM_ID, AUD_STREAM_CAPTURE);
}

static void _close_mic(void) {
  TRACE(1, "[%s] ...", __func__);

  af_stream_stop(ANC_WNR_STREAM_ID, AUD_STREAM_CAPTURE);
  af_stream_close(ANC_WNR_STREAM_ID, AUD_STREAM_CAPTURE);
}
