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
#include "audioflinger.h"
#include "codec_int.h"
#include "codec_tlv32aic32.h"
#include "hal_btpcm.h"
#include "hal_codec.h"
#include "hal_dma.h"
#include "hal_i2s.h"
#include "hal_spdif.h"
#include "hal_tdm.h"

#include "analog.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include "string.h"
#include "tgt_hardware.h"

#include "cmsis.h"

#ifdef __RAND_FROM_MIC__
#include "randfrommic.h"
#endif

#ifdef TWS_PROMPT_SYNC
extern void tws_playback_ticks_check_for_mix_prompt(void);
#endif

#ifdef RTOS
#include "cmsis_os.h"
#else
#include "hal_sleep.h"
#endif

#ifdef AUDIO_OUTPUT_SW_LIMITER
#include "floatlimiter.h"
#endif

#define AF_TRACE_DEBUG() // TRACE(2,"%s:%d\n", __func__, __LINE__)

// #define AF_STREAM_ID_0_PLAYBACK_FADEOUT

// #define CORRECT_SAMPLE_VALUE

#ifdef AUDIO_OUTPUT_DC_CALIB
#ifdef CHIP_BEST1000
#define AUDIO_OUTPUT_DC_CALIB_SW
#define AUDIO_OUTPUT_PA_ON_FADE_IN
#if !(defined(__TWS__) || defined(IBRT))
#define AUDIO_OUTPUT_PA_OFF_FADE_OUT
#endif
#elif (defined(__TWS__) || defined(IBRT)) && !defined(ANC_APP)
#define AUDIO_OUTPUT_PA_ON_FADE_IN
#endif // !CHIP_BEST1000 && __TWS__
#elif defined(AUDIO_OUTPUT_DC_CALIB_ANA) &&                                    \
    (defined(__TWS__) || defined(IBRT)) && !defined(ANC_APP)
#define AUDIO_OUTPUT_PA_ON_FADE_IN
#endif // !AUDIO_OUTPUT_DC_CALIB && AUDIO_OUTPUT_DC_CALIB_ANA && __TWS__

#define AF_FADE_OUT_SIGNAL_ID 15
#define AF_FADE_IN_SIGNAL_ID 14

/* config params */
#define AF_CODEC_INST HAL_CODEC_ID_0
#define AF_SPDIF_INST HAL_SPDIF_ID_0
#define AF_BTPCM_INST HAL_BTPCM_ID_0

#define AF_CPU_WAKE_USER HAL_CPU_WAKE_LOCK_USER_AUDIOFLINGER

#define AF_CODEC_RIGHT_CHAN_ATTN 0.968277856 // -0.28 dB

#define AF_CODEC_VOL_UPDATE_STEP 0.00002

#define AF_CODEC_DC_MAX_SCALE 32767

#define AF_CODEC_DC_STABLE_INTERVAL MS_TO_TICKS(4)
#ifdef AUDIO_OUTPUT_PA_OFF_FADE_OUT
#define AF_CODEC_PA_RESTART_INTERVAL MS_TO_TICKS(160)
#else
#define AF_CODEC_PA_RESTART_INTERVAL MS_TO_TICKS(8)
#endif

// The following might have been defined in tgt_hardware.h
#ifndef AF_CODEC_FADE_IN_MS
#define AF_CODEC_FADE_IN_MS 20
#endif
#ifndef AF_CODEC_FADE_OUT_MS
#define AF_CODEC_FADE_OUT_MS 20
#endif
#define AF_CODEC_FADE_MIN_SAMPLE_CNT 200

#define PP_PINGPANG(v) (v == PP_PING ? PP_PANG : PP_PING)

#ifdef AUDIO_ANC_FB_ADJ_MC
#define AF_STACK_SIZE (1024 * 10)
#else
#ifndef AF_STACK_SIZE
#define AF_STACK_SIZE (1024 * 8)
#endif
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP)
#define DYNAMIC_AUDIO_BUFFER_COUNT
#endif

#ifdef DYNAMIC_AUDIO_BUFFER_COUNT
#define MAX_AUDIO_BUFFER_COUNT 16
#define MIN_AUDIO_BUFFER_COUNT 2
#define AUDIO_BUFFER_COUNT (role->dma_desc_cnt)
#else
#define MAX_AUDIO_BUFFER_COUNT 4
#define AUDIO_BUFFER_COUNT MAX_AUDIO_BUFFER_COUNT
#if (AUDIO_BUFFER_COUNT & 0x1)
#error "AUDIO_BUFFER_COUNT must be an even number"
#endif
#endif

/* internal use */
enum AF_BOOL_T { AF_FALSE = 0, AF_TRUE = 1 };

enum AF_RESULT_T { AF_RES_SUCCESS = 0, AF_RES_FAILD = 1 };

enum AF_DAC_PA_STATE_T {
  AF_DAC_PA_NULL,
  AF_DAC_PA_ON_TRIGGER,
  AF_DAC_PA_ON_SOFT_START,
  AF_DAC_PA_OFF_TRIGGER,
  AF_DAC_PA_OFF_SOFT_END,
  AF_DAC_PA_OFF_SOFT_END_DONE,
  AF_DAC_PA_OFF_DC_START,
};

// status machine
enum AF_STATUS_T {
  AF_STATUS_NULL = 0x00,
  AF_STATUS_OPEN_CLOSE = 0x01,
  AF_STATUS_STREAM_OPEN_CLOSE = 0x02,
  AF_STATUS_STREAM_START_STOP = 0x04,
  AF_STATUS_STREAM_PAUSE_RESTART = 0x08,
  AF_STATUS_MASK = 0x0F,
};

struct af_stream_ctl_t {
  enum AF_PP_T pp_index; // pingpong operate
  uint8_t pp_cnt;        // use to count the lost signals
  uint8_t status;        // status machine
  enum AUD_STREAM_USE_DEVICE_T use_device;
};

struct af_stream_cfg_t {
  // used inside
  struct af_stream_ctl_t ctl;

  // dma buf parameters, RAM can be alloced in different way
  uint8_t *dma_buf_ptr;
  uint32_t dma_buf_size;

  // store stream cfg parameters
  struct AF_STREAM_CONFIG_T cfg;

  // dma cfg parameters
#ifdef DYNAMIC_AUDIO_BUFFER_COUNT
  uint8_t dma_desc_cnt;
#endif
  struct HAL_DMA_DESC_T dma_desc[MAX_AUDIO_BUFFER_COUNT];
  struct HAL_DMA_CH_CFG_T dma_cfg;

  // callback function
  AF_STREAM_HANDLER_T handler;
};

static struct af_stream_cfg_t af_stream[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];
static uint8_t af_sig_lost_cnt[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];

static AF_CODEC_PLAYBACK_POST_HANDLER_T codec_play_post_hdlr;

#ifdef AUDIO_OUTPUT_SW_GAIN
volatile static float saved_output_coef;
#ifdef AUDIO_OUTPUT_SW_LIMITER
static FloatLimiterPtr FloatLimiterP;
#else
typedef struct {
  float coefs_b[3];
  float coefs_a[3];
  float history_x[2];
  float history_y[2];
} SW_GAIN_IIR_T;
SW_GAIN_IIR_T sw_gain_iir = {
    .coefs_b = {0.00000042780818597736f, 0.00000085561637195472f,
                0.00000042780818597736f},
    .coefs_a = {1.00000000000000000000f, -1.99738371810092970000f,
                0.99738542933367369000f},
    .history_x = {0, 0},
    .history_y = {0, 0},
};
#endif
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB
static bool dac_dc_valid;
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
static int16_t dac_dc[2];
#endif
#endif

#if defined(AUDIO_OUTPUT_PA_ON_FADE_IN) || defined(AUDIO_OUTPUT_PA_OFF_FADE_OUT)
static volatile enum AF_DAC_PA_STATE_T dac_pa_state;
static uint32_t dac_dc_start_time;
static uint32_t dac_pa_stop_time;
#endif

#if defined(AUDIO_OUTPUT_PA_OFF_FADE_OUT) && defined(RTOS)
static osThreadId fade_thread_id;
#ifdef AF_STREAM_ID_0_PLAYBACK_FADEOUT
#error "Cannot enable 2 kinds of fade out codes at the same time"
#endif
#endif

#ifdef RTOS

#ifdef AF_STREAM_ID_0_PLAYBACK_FADEOUT
struct af_stream_fade_out_t {
  bool stop_on_process;
  uint8_t stop_process_cnt;
  osThreadId stop_request_tid;
  uint32_t need_fadeout_len;
  uint32_t need_fadeout_len_processed;
};

static struct af_stream_fade_out_t af_stream_fade_out = {
    .stop_on_process = false,
    .stop_process_cnt = 0,
    .stop_request_tid = NULL,
    .need_fadeout_len = 0,
    .need_fadeout_len_processed = 0,
};
#endif

static osThreadId af_thread_tid;

static void af_thread(void const *argument);
osThreadDef(af_thread, osPriorityAboveNormal, 1, AF_STACK_SIZE,
            "audio_flinger");
static int af_default_priority;

osMutexId audioflinger_mutex_id = NULL;
osMutexDef(audioflinger_mutex);

static AF_IRQ_NOTIFICATION_T irq_notif;

#else // !RTOS

static volatile uint32_t af_flag_lock;

#endif // RTOS

#ifdef CODEC_DSD
static bool af_dsd_enabled;
#endif

void af_lock_thread(void) {
  void *POSSIBLY_UNUSED lr = __builtin_return_address(0);
#ifdef RTOS
  osMutexWait(audioflinger_mutex_id, osWaitForever);
#else
  static void *POSSIBLY_UNUSED locked_lr;
  ASSERT(af_flag_lock == 0, "audioflinger has been locked by %p. LR=%p",
         (void *)locked_lr, (void *)lr);
  af_flag_lock = 1;
  locked_lr = lr;
#endif
}

void af_unlock_thread(void) {
  void *POSSIBLY_UNUSED lr = __builtin_return_address(0);
#ifdef RTOS
  osMutexRelease(audioflinger_mutex_id);
#else
  static void *POSSIBLY_UNUSED unlocked_lr;
  ASSERT(af_flag_lock == 1,
         "audioflinger not locked before (lastly unlocked by %p). LR=%p",
         (void *)unlocked_lr, (void *)lr);
  af_flag_lock = 0;
  unlocked_lr = lr;
#endif
}

#if defined(RTOS) && defined(AF_STREAM_ID_0_PLAYBACK_FADEOUT)
int af_stream_fadeout_start(uint32_t sample) {
  TRACE(1, "fadein_config sample:%d", sample);
  af_stream_fade_out.need_fadeout_len = sample;
  af_stream_fade_out.need_fadeout_len_processed = sample;
  return 0;
}

int af_stream_fadeout_stop(void) {
  af_stream_fade_out.stop_process_cnt = 0;
  af_stream_fade_out.stop_on_process = false;
  return 0;
}

uint32_t af_stream_fadeout(int16_t *buf, uint32_t len,
                           enum AUD_CHANNEL_NUM_T num) {
  uint32_t i;
  uint32_t j = 0;
  uint32_t start;
  uint32_t end;

  start = af_stream_fade_out.need_fadeout_len_processed;
  end = af_stream_fade_out.need_fadeout_len_processed > len
            ? af_stream_fade_out.need_fadeout_len_processed - len
            : 0;

  if (start <= end) {
    //        TRACE(0,"skip fadeout");
    memset(buf, 0, len * 2);
    return len;
  }
  //    TRACE(3,"fadeout l:%d start:%d end:%d", len, start, end);
  //    DUMP16("%05d ", buf, 10);
  //    DUMP16("%05d ", buf+len-10, 10);

  if (num == AUD_CHANNEL_NUM_1) {
    for (i = start; i > end; i--) {
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
    }
  } else if (num == AUD_CHANNEL_NUM_2) {
    for (i = start; i > end; i -= 2) {
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
    }
  } else if (num == AUD_CHANNEL_NUM_4) {
    for (i = start; i > end; i -= 4) {
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
    }
  } else if (num == AUD_CHANNEL_NUM_8) {
    for (i = start; i > end; i -= 8) {
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
      *(buf + j) = *(buf + j) * i / af_stream_fade_out.need_fadeout_len;
      j++;
    }
  }
  af_stream_fade_out.need_fadeout_len_processed -= j;

  //    TRACE(3,"out i:%d process:%d x:%d", i,
  //    af_stream_fade_out.need_fadeout_len_processed,
  //    end+((start-end)/AUD_CHANNEL_NUM_2)); DUMP16("%05d ", buf, 10);
  //    DUMP16("%05d ", buf+len-10, 10);

  return len;
}

void af_stream_stop_wait_finish() {
  af_stream_fade_out.stop_on_process = true;
  af_stream_fade_out.stop_request_tid = osThreadGetId();
  osSignalClear(af_stream_fade_out.stop_request_tid,
                (1 << AF_FADE_OUT_SIGNAL_ID));
  af_unlock_thread();
  osSignalWait((1 << AF_FADE_OUT_SIGNAL_ID), 300);
  af_lock_thread();
}

void af_stream_stop_process(struct af_stream_cfg_t *af_cfg, uint8_t *buf,
                            uint32_t len) {
  af_lock_thread();
  if (af_stream_fade_out.stop_on_process) {
    //        TRACE(5,"%s num:%d size:%d len:%d cnt:%d", __func__,
    //        af_cfg->cfg.channel_num, af_cfg->cfg.data_size, len,
    //        af_stream_fade_out.stop_process_cnt);
    af_stream_fadeout((int16_t *)buf, len / 2, af_cfg->cfg.channel_num);

    //        TRACE(3,"process ret:%d %d %d", *(int16_t *)(buf+len-2-2-2),
    //        *(int16_t *)(buf+len-2-2), *(int16_t *)(buf+len-2));
    if (af_stream_fade_out.stop_process_cnt++ > 3) {
      TRACE(0, "stop_process end");
      osSignalSet(af_stream_fade_out.stop_request_tid,
                  (1 << AF_FADE_OUT_SIGNAL_ID));
    }
  }
  af_unlock_thread();
}
#endif // RTOS && AF_STREAM_ID_0_PLAYBACK_FADEOUT

// used by dma irq and af_thread
static inline struct af_stream_cfg_t *
af_get_stream_role(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream) {
  ASSERT(id < AUD_STREAM_ID_NUM, "[%s] Bad id=%d", __func__, id);
  ASSERT(stream < AUD_STREAM_NUM, "[%s] Bad stream=%d", __func__, stream);

  return &af_stream[id][stream];
}

static void af_set_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
                          enum AF_STATUS_T status) {
  struct af_stream_cfg_t *role = NULL;

  role = af_get_stream_role(id, stream);

  role->ctl.status |= status;
}

static void af_clear_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
                            enum AF_STATUS_T status) {
  struct af_stream_cfg_t *role = NULL;

  role = af_get_stream_role(id, stream);

  role->ctl.status &= ~status;
}

// get current stream config parameters
uint32_t af_stream_get_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
                           struct AF_STREAM_CONFIG_T **cfg, bool needlock) {
  AF_TRACE_DEBUG();

  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;

  if (needlock) {
    af_lock_thread();
  }
  role = af_get_stream_role(id, stream);
  // check stream is open
  if (role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE) {
    *cfg = &(role->cfg);
    ret = AF_RES_SUCCESS;
  } else {
    ret = AF_RES_FAILD;
  }
  if (needlock) {
    af_unlock_thread();
  }
  return ret;
}

#if 0
static void af_dump_cfg()
{
    struct af_stream_cfg_t *role = NULL;

    TRACE(0,"dump cfg.........start");
    //initial parameter
    for(uint8_t id=0; id< AUD_STREAM_ID_NUM; id++)
    {
        for(uint8_t stream=0; stream < AUD_STREAM_NUM; stream++)
        {
            role = af_get_stream_role((enum AUD_STREAM_ID_T)id, (enum AUD_STREAM_T)stream);

            TRACE(2,"id = %d, stream = %d:", id, stream);
            TRACE(1,"ctl.use_device = %d", role->ctl.use_device);
            TRACE(1,"cfg.device = %d", role->cfg.device);
            TRACE(1,"dma_cfg.ch = %d", role->dma_cfg.ch);
        }
    }
    TRACE(0,"dump cfg.........end");
}
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
static void af_codec_output_gain_changed(float coef) {
  saved_output_coef = coef;
}
#ifndef AUDIO_OUTPUT_SW_LIMITER
static float af_codec_sw_gain_filter(float gain_in) {
  float gain_out = saved_output_coef * sw_gain_iir.coefs_b[0] +
                   sw_gain_iir.history_x[0] * sw_gain_iir.coefs_b[1] +
                   sw_gain_iir.history_x[1] * sw_gain_iir.coefs_b[2] -
                   sw_gain_iir.history_y[0] * sw_gain_iir.coefs_a[1] -
                   sw_gain_iir.history_y[1] * sw_gain_iir.coefs_a[2];

  sw_gain_iir.history_y[1] = sw_gain_iir.history_y[0];
  sw_gain_iir.history_y[0] = gain_out;
  sw_gain_iir.history_x[1] = sw_gain_iir.history_x[0];
  sw_gain_iir.history_x[0] = saved_output_coef;

  return gain_out;
}
static void af_codec_sw_gain_process(uint8_t *buf, uint32_t size,
                                     enum AUD_BITS_T bits,
                                     enum AUD_CHANNEL_NUM_T chans) {
  uint32_t i, pcm_len;
  int32_t pcm_out;
  float output_gain = 1.0f;

  // TRACE(1,"af_codec_sw_gain_process:%d",(int32_t)(saved_output_coef*1000));

  if (chans == AUD_CHANNEL_NUM_1) {
    if (bits <= AUD_BITS_16) {
      pcm_len = size / sizeof(int16_t);
      int16_t *pcm_buf = (int16_t *)buf;
      for (i = 0; i < pcm_len; i++) {
        output_gain = af_codec_sw_gain_filter(saved_output_coef);

        pcm_out = (int32_t)(pcm_buf[i] * output_gain);
        pcm_buf[i] = __SSAT(pcm_out, 16);
      }
    } else {
      pcm_len = size / sizeof(int32_t);
      int32_t *pcm_buf = (int32_t *)buf;
      for (i = 0; i < pcm_len; i++) {
        output_gain = af_codec_sw_gain_filter(saved_output_coef);

        pcm_out = (int32_t)(pcm_buf[i] * output_gain);
        pcm_buf[i] = __SSAT(pcm_out, 24);
      }
    }
  } else if (chans == AUD_CHANNEL_NUM_2) {
    if (bits <= AUD_BITS_16) {
      pcm_len = size / sizeof(int16_t);
      int16_t *pcm_buf = (int16_t *)buf;
      for (i = 0; i < pcm_len; i = i + 2) {
        output_gain = af_codec_sw_gain_filter(saved_output_coef);

        pcm_out = (int32_t)(pcm_buf[i] * output_gain);
        pcm_buf[i] = __SSAT(pcm_out, 16);
        pcm_out = (int32_t)(pcm_buf[i + 1] * output_gain);
        pcm_buf[i + 1] = __SSAT(pcm_out, 16);
      }
    } else {
      pcm_len = size / sizeof(int32_t);
      int32_t *pcm_buf = (int32_t *)buf;
      for (i = 0; i < pcm_len; i = i + 2) {
        output_gain = af_codec_sw_gain_filter(saved_output_coef);

        pcm_out = (int32_t)(pcm_buf[i] * output_gain);
        pcm_buf[i] = __SSAT(pcm_out, 24);
        pcm_out = (int32_t)(pcm_buf[i + 1] * output_gain);
        pcm_buf[i + 1] = __SSAT(pcm_out, 24);
      }
    }
  }
}
#endif
#endif // AUDIO_OUTPUT_SW_GAIN

static uint32_t POSSIBLY_UNUSED af_codec_get_sample_count(
    uint32_t size, enum AUD_BITS_T bits, enum AUD_CHANNEL_NUM_T chans) {
  uint32_t div;

  if (bits <= AUD_BITS_16) {
    div = 2;
  } else {
    div = 4;
  }
  div *= chans;
  return size / div;
}

static void POSSIBLY_UNUSED af_zero_mem(void *dst, unsigned int size) {
  uint32_t *d = dst;
  uint32_t count = size / 4;

  while (count--) {
    *d++ = 0;
  }
}

static bool af_codec_playback_pre_handler(uint8_t *buf, uint32_t len,
                                          const struct af_stream_cfg_t *role) {
  uint32_t POSSIBLY_UNUSED time;

  if (0) {

#ifdef AUDIO_OUTPUT_PA_ON_FADE_IN
  } else if (dac_pa_state == AF_DAC_PA_ON_TRIGGER &&
             (time = hal_sys_timer_get()) - dac_dc_start_time >=
                 AF_CODEC_DC_STABLE_INTERVAL &&
             time - dac_pa_stop_time >= AF_CODEC_PA_RESTART_INTERVAL) {
    analog_aud_codec_speaker_enable(true);
    dac_pa_state = AF_DAC_PA_NULL;
#endif // AUDIO_OUTPUT_PA_ON_FADE_IN

#ifdef AUDIO_OUTPUT_PA_OFF_FADE_OUT
  } else if (dac_pa_state == AF_DAC_PA_OFF_TRIGGER) {
    dac_dc_start_time = hal_sys_timer_get();
    dac_pa_state = AF_DAC_PA_OFF_DC_START;
  } else if (dac_pa_state == AF_DAC_PA_OFF_DC_START) {
    time = hal_sys_timer_get();
    if (time - dac_dc_start_time >= AF_CODEC_DC_STABLE_INTERVAL) {
      dac_pa_state = AF_DAC_PA_NULL;
      analog_aud_codec_speaker_enable(false);
      dac_pa_stop_time = time;
#ifdef RTOS
      osSignalSet(fade_thread_id, (1 << AF_FADE_OUT_SIGNAL_ID));
#endif
    }
#endif // AUDIO_OUTPUT_PA_OFF_FADE_OUT
  }

#if defined(AUDIO_OUTPUT_PA_ON_FADE_IN) || defined(AUDIO_OUTPUT_PA_OFF_FADE_OUT)
  if (dac_pa_state == AF_DAC_PA_ON_TRIGGER ||
      dac_pa_state == AF_DAC_PA_OFF_DC_START) {
    af_zero_mem(buf, len);
    return true;
  }
#endif

  return false;
}

#ifdef AUDIO_OUTPUT_DC_CALIB_SW
static void af_codec_playback_sw_dc_calib(uint8_t *buf, uint32_t len,
                                          enum AUD_BITS_T bits,
                                          enum AUD_CHANNEL_NUM_T chans) {
  uint32_t cnt;

  if (bits <= AUD_BITS_16) {
    int16_t *ptr16 = (int16_t *)buf;
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
    int16_t dc_l = dac_dc[0];
    int16_t dc_r = dac_dc[1];
#endif
    if (chans == AUD_CHANNEL_NUM_1) {
      cnt = len / sizeof(int16_t);
      while (cnt-- > 0) {
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
        *ptr16 = __SSAT(*ptr16 + dc_l, 16);
#endif
        ptr16++;
      }
    } else {
      cnt = len / sizeof(int16_t) / 2;
      while (cnt-- > 0) {
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
        *ptr16 = __SSAT(*ptr16 + dc_l, 16);
        *(ptr16 + 1) = __SSAT(*(ptr16 + 1) + dc_r, 16);
#endif
        ptr16 += 2;
      }
    }
  } else {
    int32_t *ptr32 = (int32_t *)buf;
    int32_t dac_bits =
#ifdef CHIP_BEST1000
        CODEC_PLAYBACK_BIT_DEPTH;
#else
        24;
#endif
    int32_t val_shift;

    if (dac_bits < 24) {
      val_shift = 24 - dac_bits;
    } else {
      val_shift = 0;
    }

#ifdef AUDIO_OUTPUT_DC_CALIB_SW
    int32_t dc_l = dac_dc[0] << 8;
    int32_t dc_r = dac_dc[1] << 8;
#endif
    if (chans == AUD_CHANNEL_NUM_1) {
      cnt = len / sizeof(int32_t);
      while (cnt-- > 0) {
#ifdef CORRECT_SAMPLE_VALUE
        *ptr32 = ((*ptr32) << (32 - dac_bits)) >> (32 - dac_bits);
#endif
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
        *ptr32 = __SSAT((*ptr32 + dc_l) >> val_shift, dac_bits);
#elif defined(CORRECT_SAMPLE_VALUE)
        *ptr32 = __SSAT(*ptr32 >> val_shift, dac_bits);
#else
        *ptr32 = *ptr32 >> val_shift;
#endif
        ptr32++;
      }
    } else {
      cnt = len / sizeof(int32_t) / 2;
      while (cnt-- > 0) {
#ifdef CORRECT_SAMPLE_VALUE
        *ptr32 = ((*ptr32) << (32 - dac_bits)) >> (32 - dac_bits);
        *(ptr32 + 1) = ((*(ptr32 + 1)) << (32 - dac_bits)) >> (32 - dac_bits);
#endif
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
        *ptr32 = __SSAT((*ptr32 + dc_l) >> val_shift, dac_bits);
        *(ptr32 + 1) = __SSAT((*(ptr32 + 1) + dc_r) >> val_shift, dac_bits);
#elif defined(CORRECT_SAMPLE_VALUE)
        *ptr32 = __SSAT(*ptr32 >> val_shift, dac_bits);
        *(ptr32 + 1) = __SSAT(*(ptr32 + 1) >> val_shift, dac_bits);
#else
        *ptr32 = *ptr32 >> val_shift;
        *(ptr32 + 1) = *(ptr32 + 1) >> val_shift;
#endif
        ptr32 += 2;
      }
    }
  }
}
#endif // AUDIO_OUTPUT_DC_CALIB_SW

static void af_codec_playback_post_handler(uint8_t *buf, uint32_t len,
                                           const struct af_stream_cfg_t *role) {
  POSSIBLY_UNUSED enum AUD_BITS_T bits;
  POSSIBLY_UNUSED enum AUD_CHANNEL_NUM_T chans;
  POSSIBLY_UNUSED uint32_t cnt;

  bits = role->cfg.bits;
  chans = role->cfg.channel_num;

#ifdef AUDIO_OUTPUT_SW_GAIN
#ifdef AUDIO_OUTPUT_SW_LIMITER
  if (bits <= AUD_BITS_16) {
    cnt = len / sizeof(int16_t);
  } else {
    cnt = len / sizeof(int32_t);
  }
  ApplyFloatLimiter(FloatLimiterP, buf, saved_output_coef, cnt);
#else
  af_codec_sw_gain_process(buf, len, bits, chans);
#endif
#endif

  if (codec_play_post_hdlr) {
    codec_play_post_hdlr(buf, len, (void *)&role->cfg);
  }

#if defined(AUDIO_OUTPUT_INVERT_RIGHT_CHANNEL)
  if (chans == AUD_CHANNEL_NUM_2) {
    if (bits == AUD_BITS_16) {
      int16_t *buf16 = (int16_t *)buf;
      for (int i = 1; i < len / sizeof(int16_t); i += 2) {
        int32_t tmp = -buf16[i];
        buf16[i] = __SSAT(tmp, 16);
      }
    } else {
      int32_t *buf32 = (int32_t *)buf;
      for (int i = 1; i < len / sizeof(int32_t); i += 2) {
        int32_t tmp = -buf32[i];
        buf32[i] = __SSAT(tmp, 24);
      }
    }
  }
#endif

#if (defined(CHIP_BEST1000) || defined(CHIP_BEST2000)) &&                      \
    defined(AUDIO_OUTPUT_CALIB_GAIN_MISSMATCH)
  // gain must less than 1.0
  float gain = 0.9441f;
  if (chans == AUD_CHANNEL_NUM_2) {
    if (bits == AUD_BITS_16) {
      int16_t *buf16 = (int16_t *)buf;
      for (int i = 1; i < len / sizeof(int16_t); i += 2) {
        int32_t tmp = (int)(buf16[i + 1] * gain);
        buf16[i + 1] = __SSAT(tmp, 16);
      }
    } else {
      int32_t *buf32 = (int32_t *)buf;
      for (int i = 1; i < len / sizeof(int32_t); i += 2) {
        int32_t tmp = (int)(buf32[i + 1] * gain);
        buf32[i + 1] = __SSAT(tmp, 24);
      }
    }
  }
#endif

#if defined(AUDIO_OUTPUT_DC_CALIB_SW)
  af_codec_playback_sw_dc_calib(buf, len, bits, chans);
#elif defined(CHIP_BEST1000)
  if (bits == AUD_BITS_24 || bits == AUD_BITS_32) {
    int32_t *ptr32 = (int32_t *)buf;
    int32_t val_shift;

    if (bits == AUD_BITS_24) {
      val_shift = 24 - CODEC_PLAYBACK_BIT_DEPTH;
    } else {
      val_shift = 32 - CODEC_PLAYBACK_BIT_DEPTH;
    }

    cnt = len / sizeof(int32_t);
    while (cnt-- > 0) {
      *ptr32 >>= val_shift;
      ptr32++;
    }
  }
#endif

#if defined(CHIP_BEST1000) && defined(AUDIO_OUTPUT_GAIN_M60DB_CHECK)
  hal_codec_dac_gain_m60db_check(HAL_CODEC_PERF_TEST_M60DB);
#endif
}

static inline void af_thread_stream_handler(enum AUD_STREAM_ID_T id,
                                            enum AUD_STREAM_T stream) {
  uint32_t lock;
  struct af_stream_cfg_t *role;
  uint32_t dma_addr, hw_pos;
  uint8_t *buf;
  uint32_t len;
  uint32_t pp_cnt;
  bool codec_playback;
  bool skip_handler;
  enum AF_PP_T pp_index;

  role = af_get_stream_role(id, stream);

  af_lock_thread();

  if (role->handler && (role->ctl.status & AF_STATUS_STREAM_START_STOP)) {
    lock = int_lock();
    pp_cnt = role->ctl.pp_cnt;
    role->ctl.pp_cnt = 0;
    int_unlock(lock);

    af_sig_lost_cnt[id][stream] = pp_cnt - 1;
    if (af_sig_lost_cnt[id][stream]) {
      TRACE(3, "af_thread:WARNING: id=%d stream=%d lost %u signals", id, stream,
            af_sig_lost_cnt[id][stream]);
    }

    pp_index = PP_PINGPANG(role->ctl.pp_index);

    // Get PP index from accurate DMA pos
    dma_addr = af_stream_get_cur_dma_addr(id, stream);
    hw_pos = dma_addr - (uint32_t)role->dma_buf_ptr;
    if (hw_pos > role->dma_buf_size) {
      TRACE(
          3,
          "af_thread: Failed to get valid dma addr for id=%d stream=%d: 0x%08x",
          id, stream, dma_addr);
    }
#ifndef CHIP_BEST1000
    if (role->cfg.chan_sep_buf && role->cfg.channel_num > AUD_CHANNEL_NUM_1) {
      uint32_t chan_size;

      chan_size = role->dma_buf_size / role->cfg.channel_num;

      if (hw_pos <= role->dma_buf_size) {
        hw_pos = hw_pos % chan_size;
        if (hw_pos < chan_size / 2) {
          pp_index = PP_PANG;
        } else if (hw_pos < chan_size) {
          pp_index = PP_PING;
        }
      }
      if (pp_index == PP_PING) {
        buf = role->dma_buf_ptr;
      } else {
        buf = role->dma_buf_ptr + chan_size / 2;
      }
    } else
#endif
    {
      if (hw_pos < role->dma_buf_size / 2) {
        pp_index = PP_PANG;
      } else if (hw_pos < role->dma_buf_size) {
        pp_index = PP_PING;
      }
      if (pp_index == PP_PING) {
        buf = role->dma_buf_ptr;
      } else {
        buf = role->dma_buf_ptr + role->dma_buf_size / 2;
      }
    }

    if (stream == AUD_STREAM_PLAYBACK &&
        role->ctl.use_device == AUD_STREAM_USE_INT_CODEC) {
      codec_playback = true;
    } else {
      codec_playback = false;
    }

    skip_handler = false;
    len = role->dma_buf_size / 2;

    if (codec_playback) {
      skip_handler = af_codec_playback_pre_handler(buf, len, role);
    }

    if (!skip_handler) {
#ifdef __RAND_FROM_MIC__
      if ((AUD_STREAM_CAPTURE == stream) &&
          (AUD_STREAM_USE_INT_CODEC == role->cfg.device)) {
        random_data_process(buf, role->dma_buf_size / 2, role->cfg.bits,
                            role->cfg.channel_num);
      }
#endif
      role->handler(buf, len);
    }

    if (codec_playback) {
      af_codec_playback_post_handler(buf, len, role);
    }

#if defined(RTOS) && defined(AF_STREAM_ID_0_PLAYBACK_FADEOUT)
    af_stream_stop_process(role, buf, len);
#endif

    if (role->ctl.pp_cnt) {
      TRACE(3,
            "af_thread:WARNING: id=%d stream=%d hdlr ran too long (pp_cnt=%u)",
            id, stream, role->ctl.pp_cnt);
    }
  }

  af_unlock_thread();
}

#ifdef RTOS

static void af_thread(void const *argument) {
  osEvent evt;
  uint32_t signals = 0;
  enum AUD_STREAM_ID_T id;
  enum AUD_STREAM_T stream;

  while (1) {
    // wait any signal
    evt = osSignalWait(0x0, osWaitForever);
    signals = evt.value.signals;
    //        TRACE(3,"[%s] status = %x, signals = %d", __func__, evt.status,
    //        evt.value.signals);

    if (evt.status == osEventSignal) {
      for (uint8_t i = 0; i < AUD_STREAM_ID_NUM * AUD_STREAM_NUM; i++) {
        if (signals & (1 << i)) {
          id = (enum AUD_STREAM_ID_T)(i >> 1);
          stream = (enum AUD_STREAM_T)(i & 1);

          af_thread_stream_handler(id, stream);
        }
      }
    } else {
      TRACE(2, "[%s] ERROR: evt.status = %d", __func__, evt.status);
      continue;
    }
  }
}

#else // !RTOS

#include "cmsis.h"
static volatile uint32_t af_flag_open;
static volatile uint32_t af_flag_signal;

static void af_set_flag(volatile uint32_t *flag, uint32_t set) {
  uint32_t lock;

  lock = int_lock();
  *flag |= set;
  int_unlock(lock);
}

static void af_clear_flag(volatile uint32_t *flag, uint32_t clear) {
  uint32_t lock;

  lock = int_lock();
  *flag &= ~clear;
  int_unlock(lock);
}

static bool af_flag_active(volatile uint32_t *flag, uint32_t bits) {
  return !!(*flag & bits);
}

void af_thread(void const *argument) {
  uint32_t lock;
  uint32_t i;
  enum AUD_STREAM_ID_T id;
  enum AUD_STREAM_T stream;

  if (af_flag_open == 0) {
    return;
  }

  for (i = 0; i < AUD_STREAM_ID_NUM * AUD_STREAM_NUM; i++) {
    if (!af_flag_active(&af_flag_signal, 1 << i)) {
      continue;
    }
    af_clear_flag(&af_flag_signal, 1 << i);

    id = (enum AUD_STREAM_ID_T)(i >> 1);
    stream = (enum AUD_STREAM_T)(i & 1);

    af_thread_stream_handler(id, stream);
  }

  lock = int_lock();
  if (af_flag_signal == 0) {
    hal_cpu_wake_unlock(AF_CPU_WAKE_USER);
  }
  int_unlock(lock);
}

#endif // !RTOS

static void af_dma_irq_handler(uint8_t ch, uint32_t remain_dst_tsize,
                               uint32_t error, struct HAL_DMA_DESC_T *lli) {
  struct af_stream_cfg_t *role = NULL;

  // initial parameter
  for (uint8_t id = 0; id < AUD_STREAM_ID_NUM; id++) {
    for (uint8_t stream = 0; stream < AUD_STREAM_NUM; stream++) {
      role = af_get_stream_role((enum AUD_STREAM_ID_T)id,
                                (enum AUD_STREAM_T)stream);

      if (role->dma_cfg.ch == ch) {
        role->ctl.pp_index = PP_PINGPANG(role->ctl.pp_index);
        role->ctl.pp_cnt++;
        // TRACE(4,"[%s] id = %d, stream = %d, ch = %d", __func__, id, stream,
        // ch); TRACE(2,"[%s] PLAYBACK pp_cnt = %d", __func__,
        // role->ctl.pp_cnt);
#ifdef RTOS
        if (irq_notif) {
          irq_notif(id, stream);
        }
        osSignalSet(af_thread_tid, 0x01 << (id * 2 + stream));
#else
        af_set_flag(&af_flag_signal, 0x01 << (id * 2 + stream));
        hal_cpu_wake_lock(AF_CPU_WAKE_USER);
#endif
        return;
      }
    }
  }

  // invalid dma irq
  ASSERT(0, "[%s] ERROR: channel id = %d", __func__, ch);
}

#ifdef __CODEC_ASYNC_CLOSE__
static void af_codec_async_close(void) {
  af_lock_thread();
  codec_int_close(CODEC_CLOSE_ASYNC_REAL);
  af_unlock_thread();
}
#endif

void *af_thread_tid_get(void) {
#ifdef RTOS
  return (void *)af_thread_tid;
#else
  return NULL;
#endif
}

#ifdef CODEC_ANC_BOOST
static void af_codec_anc_boost_delay(uint32_t ms) {
  af_unlock_thread();

  osDelay(ms);

  af_lock_thread();
}
#endif

uint32_t af_open(void) {
  AF_TRACE_DEBUG();
  struct af_stream_cfg_t *role = NULL;

#ifdef RTOS
  if (audioflinger_mutex_id == NULL) {
    audioflinger_mutex_id = osMutexCreate((osMutex(audioflinger_mutex)));
  }
#endif

  af_lock_thread();

#ifdef AUDIO_OUTPUT_DC_CALIB
  if (!dac_dc_valid) {
    int16_t dc_l, dc_r;
    uint16_t max_dc;
    float attn;

    dac_dc_valid = true;
    analog_aud_get_dc_calib_value(&dc_l, &dc_r);
    if (ABS(dc_l) >= ABS(dc_r)) {
      max_dc = ABS(dc_l);
    } else {
      max_dc = ABS(dc_r);
    }
    ASSERT(max_dc + 1 < AF_CODEC_DC_MAX_SCALE, "Bad dc values: (%d, %d)", dc_l,
           dc_r);
    if (max_dc) {
      attn = 1 - (float)(max_dc + 1) / AF_CODEC_DC_MAX_SCALE;
    } else {
      attn = 1;
    }
    hal_codec_set_dac_dc_gain_attn(attn);
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
    dac_dc[0] = dc_l;
    dac_dc[1] = dc_r;
#else
    hal_codec_set_dac_dc_offset(dc_l, dc_r);
#endif
  }
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
  hal_codec_set_sw_output_coef_callback(af_codec_output_gain_changed);
#endif

#ifdef __CODEC_ASYNC_CLOSE__
  codec_int_set_close_handler(af_codec_async_close);
#endif

#ifdef CODEC_ANC_BOOST
  codec_set_anc_boost_delay_func(af_codec_anc_boost_delay);
#endif

  // initial parameters
  for (uint8_t id = 0; id < AUD_STREAM_ID_NUM; id++) {
    for (uint8_t stream = 0; stream < AUD_STREAM_NUM; stream++) {
      role = af_get_stream_role((enum AUD_STREAM_ID_T)id,
                                (enum AUD_STREAM_T)stream);

      if (role->ctl.status == AF_STATUS_NULL) {
        role->dma_buf_ptr = NULL;
        role->dma_buf_size = 0;
        role->ctl.pp_index = PP_PING;
        role->ctl.status = AF_STATUS_OPEN_CLOSE;
        role->ctl.use_device = AUD_STREAM_USE_DEVICE_NULL;
        role->dma_cfg.ch = HAL_DMA_CHAN_NONE;
      } else {
        ASSERT(0, "[%s] ERROR: id = %d, stream = %d", __func__, id, stream);
      }
    }
  }

#ifdef RTOS
  af_thread_tid = osThreadCreate(osThread(af_thread), NULL);
  af_default_priority = af_get_priority();
  osSignalSet(af_thread_tid, 0x0);
#endif
#ifdef __RAND_FROM_MIC__
  randInit();
#endif
  af_unlock_thread();

  return AF_RES_SUCCESS;
}

static void af_stream_update_dma_buffer(enum AUD_STREAM_T stream,
                                        struct af_stream_cfg_t *role,
                                        const struct AF_STREAM_CONFIG_T *cfg) {
  int i;
  enum HAL_DMA_RET_T dma_ret;
  struct HAL_DMA_DESC_T *dma_desc, *next_desc;
  struct HAL_DMA_CH_CFG_T *dma_cfg;
  int irq;
  uint32_t desc_xfer_size;
  uint32_t align;
  uint8_t samp_size;
  enum HAL_DMA_WDITH_T width;
#ifndef CHIP_BEST1000
  bool dma_2d_en;
  uint32_t chan_desc_xfer_size = 0;
#endif

  dma_desc = &role->dma_desc[0];
  dma_cfg = &role->dma_cfg;

#ifdef CODEC_DSD
  if (stream == AUD_STREAM_PLAYBACK &&
      role->cfg.device == AUD_STREAM_USE_INT_CODEC) {
    if (dma_cfg->dst_periph == HAL_AUDMA_DSD_TX) {
      dma_cfg->dst_bsize = HAL_DMA_BSIZE_1;
    } else {
      dma_cfg->dst_bsize = HAL_DMA_BSIZE_4;
    }
  }
#endif

  if (role->cfg.device == AUD_STREAM_USE_BT_PCM) {
    dma_cfg->dst_bsize = HAL_DMA_BSIZE_1;
    dma_cfg->src_bsize = HAL_DMA_BSIZE_1;
    dma_cfg->try_burst = 0;
  }

  role->dma_buf_ptr = cfg->data_ptr;
  role->dma_buf_size = cfg->data_size;

  if (cfg->bits == AUD_BITS_24 || cfg->bits == AUD_BITS_32) {
    width = HAL_DMA_WIDTH_WORD;
    samp_size = 4;
  } else if (cfg->bits == AUD_BITS_16) {
    width = HAL_DMA_WIDTH_HALFWORD;
    samp_size = 2;
  } else {
    ASSERT(false, "%s: Invalid stream config bits=%d", __func__, cfg->bits);
    width = HAL_DMA_WIDTH_BYTE;
    samp_size = 1;
  }

#ifdef DYNAMIC_AUDIO_BUFFER_COUNT
  uint32_t desc_cnt;

  desc_cnt = (cfg->data_size / samp_size + HAL_DMA_MAX_DESC_XFER_SIZE - 1) /
             HAL_DMA_MAX_DESC_XFER_SIZE;
  if (desc_cnt < 2) {
    desc_cnt = 2;
  } else if (desc_cnt & (desc_cnt - 1)) {
    desc_cnt = 1 << (31 - __CLZ(desc_cnt) + 1);
  }
  TRACE(4, "%s: desc_cnt=%u data_size=%u samp_size=%u", __func__, desc_cnt,
        cfg->data_size, samp_size);
  ASSERT(MIN_AUDIO_BUFFER_COUNT <= desc_cnt &&
             desc_cnt <= MAX_AUDIO_BUFFER_COUNT,
         "%s: Bad desc_cnt=%u", __func__, desc_cnt);
  role->dma_desc_cnt = desc_cnt;
#endif

  desc_xfer_size = cfg->data_size / AUDIO_BUFFER_COUNT;

#ifndef CHIP_BEST1000
  if (cfg->chan_sep_buf && cfg->channel_num > AUD_CHANNEL_NUM_1) {
    dma_2d_en = true;
  } else {
    dma_2d_en = false;
  }

  if (dma_2d_en) {
    enum HAL_DMA_BSIZE_T bsize;
    uint8_t burst_size;

    chan_desc_xfer_size = desc_xfer_size / cfg->channel_num;

    if (stream == AUD_STREAM_PLAYBACK) {
      bsize = dma_cfg->src_bsize;
    } else {
      bsize = dma_cfg->dst_bsize;
    }
    if (bsize == HAL_DMA_BSIZE_1) {
      burst_size = 1;
    } else if (bsize == HAL_DMA_BSIZE_4) {
      burst_size = 4;
    } else {
      burst_size = 8;
    }
    align = burst_size * samp_size * cfg->channel_num;
    // Ensure word-aligned too
    if (align & 0x1) {
      align *= 4;
    } else if (align & 0x2) {
      align *= 2;
    }
  } else
#endif
  {
    align = 4;
  }
  ASSERT(desc_xfer_size * AUDIO_BUFFER_COUNT == cfg->data_size &&
             (desc_xfer_size % align) == 0,
         "%s: Dma data size is not aligned: data_size=%u AUDIO_BUFFER_COUNT=%u "
         "align=%u",
         __func__, cfg->data_size, AUDIO_BUFFER_COUNT, align);

  dma_cfg->dst_width = width;
  dma_cfg->src_width = width;
  dma_cfg->src_tsize = desc_xfer_size / samp_size;

  for (i = 0; i < AUDIO_BUFFER_COUNT; i++) {
    if (i == AUDIO_BUFFER_COUNT - 1) {
      next_desc = &dma_desc[0];
      irq = 1;
    } else {
      next_desc = &dma_desc[i + 1];
      if (i == AUDIO_BUFFER_COUNT / 2 - 1) {
        irq = 1;
      } else {
        irq = 0;
      }
    }

    if (stream == AUD_STREAM_PLAYBACK) {
#ifndef CHIP_BEST1000
      if (dma_2d_en) {
        dma_cfg->src = (uint32_t)(role->dma_buf_ptr + chan_desc_xfer_size * i);
      } else
#endif
      {
        dma_cfg->src = (uint32_t)(role->dma_buf_ptr + desc_xfer_size * i);
      }
    } else {
#ifndef CHIP_BEST1000
      if (dma_2d_en) {
        dma_cfg->dst = (uint32_t)(role->dma_buf_ptr + chan_desc_xfer_size * i);
      } else
#endif
      {
        dma_cfg->dst = (uint32_t)(role->dma_buf_ptr + desc_xfer_size * i);
      }
    }

    dma_ret = hal_audma_init_desc(&dma_desc[i], dma_cfg, next_desc, irq);
    ASSERT(dma_ret == HAL_DMA_OK,
           "[%s] Failed to init dma desc for stream %d: ret=%d", __func__,
           stream, dma_ret);
  }
}

// Support memory<-->peripheral
// Note: Do not support peripheral <--> peripheral
uint32_t af_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
                        const struct AF_STREAM_CONFIG_T *cfg) {
  AF_TRACE_DEBUG();
  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;
  struct HAL_DMA_CH_CFG_T *dma_cfg = NULL;

#ifdef __RAND_FROM_MIC__
  random_status_sync();
#endif

  role = af_get_stream_role(id, stream);
  TRACE(3, "[%s] id = %d, stream = %d", __func__, id, stream);

  ASSERT(cfg->data_ptr != NULL, "[%s] ERROR: data_ptr == NULL!!!", __func__);
  ASSERT(((uint32_t)cfg->data_ptr) % 4 == 0,
         "[%s] ERROR: data_ptr(%p) is not align!!!", __func__, cfg->data_ptr);
  ASSERT(cfg->data_size != 0, "[%s] ERROR: data_size == 0!!!", __func__);
#ifndef CHIP_BEST1000
  if (cfg->chan_sep_buf && cfg->channel_num > AUD_CHANNEL_NUM_1) {
    ASSERT(cfg->device == AUD_STREAM_USE_INT_CODEC ||
               cfg->device == AUD_STREAM_USE_I2S0_MASTER ||
               cfg->device == AUD_STREAM_USE_I2S0_SLAVE ||
               cfg->device == AUD_STREAM_USE_I2S1_MASTER ||
               cfg->device == AUD_STREAM_USE_I2S1_SLAVE ||
               cfg->device == AUD_STREAM_USE_TDM0_MASTER ||
               cfg->device == AUD_STREAM_USE_TDM0_SLAVE ||
               cfg->device == AUD_STREAM_USE_TDM1_MASTER ||
               cfg->device == AUD_STREAM_USE_TDM1_SLAVE,
           "[%s] ERROR: Unsupport chan_sep_buf for device %d", __func__,
           cfg->device);
  }
#endif

  ret = AF_RES_FAILD;

  af_lock_thread();

  // check af is open
  if (role->ctl.status != AF_STATUS_OPEN_CLOSE) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    goto _exit;
  }

  role->cfg = *cfg;

  device = cfg->device;
  role->ctl.use_device = device;

  dma_cfg = &role->dma_cfg;
  memset(dma_cfg, 0, sizeof(*dma_cfg));

  if (device == AUD_STREAM_USE_BT_PCM) {
    dma_cfg->dst_bsize = HAL_DMA_BSIZE_1;
    dma_cfg->src_bsize = HAL_DMA_BSIZE_1;
    dma_cfg->try_burst = 0;
  } else {
    dma_cfg->dst_bsize = HAL_DMA_BSIZE_4;

#ifndef CHIP_BEST1000
    // If channel num > 1, burst size should be set to 1 for:
    // 1) all playback streams with 2D DMA enabled; or
    // 2) internal codec capture streams with 2D DMA disabled.
    if (cfg->channel_num > AUD_CHANNEL_NUM_1 &&
        ((stream == AUD_STREAM_PLAYBACK && cfg->chan_sep_buf) ||
         (device == AUD_STREAM_USE_INT_CODEC && stream == AUD_STREAM_CAPTURE &&
          !cfg->chan_sep_buf))) {
      dma_cfg->src_bsize = HAL_DMA_BSIZE_1;
    } else
#endif
    {
      dma_cfg->src_bsize = HAL_DMA_BSIZE_4;
    }
    dma_cfg->try_burst = 1;
  }

  dma_cfg->handler = af_dma_irq_handler;

  if (stream == AUD_STREAM_PLAYBACK) {
    AF_TRACE_DEBUG();
    dma_cfg->src_periph = (enum HAL_DMA_PERIPH_T)0;
    dma_cfg->type = HAL_DMA_FLOW_M2P_DMA;

    // open device and stream
    if (0) {

    }
#ifdef AF_DEVICE_EXT_CODEC
    else if (device == AUD_STREAM_USE_EXT_CODEC) {
      AF_TRACE_DEBUG();
      tlv32aic32_open();
      tlv32aic32_stream_open(stream);

      dma_cfg->dst_periph = HAL_AUDMA_I2S0_TX;
    }
#endif
#ifdef AF_DEVICE_I2S
    else if (device == AUD_STREAM_USE_I2S0_MASTER) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->dst_periph = HAL_AUDMA_I2S0_TX;
    } else if (device == AUD_STREAM_USE_I2S0_SLAVE) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->dst_periph = HAL_AUDMA_I2S0_TX;
    }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
    else if (device == AUD_STREAM_USE_I2S1_MASTER) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->dst_periph = HAL_AUDMA_I2S1_TX;
    } else if (device == AUD_STREAM_USE_I2S1_SLAVE) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->dst_periph = HAL_AUDMA_I2S1_TX;
    }
#endif
#endif
#ifdef AF_DEVICE_TDM
    else if (device == AUD_STREAM_USE_TDM0_MASTER) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->dst_periph = HAL_AUDMA_I2S0_TX;
    } else if (device == AUD_STREAM_USE_TDM0_SLAVE) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->dst_periph = HAL_AUDMA_I2S0_TX;
    }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
    else if (device == AUD_STREAM_USE_TDM1_MASTER) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->dst_periph = HAL_AUDMA_I2S1_TX;
    } else if (device == AUD_STREAM_USE_TDM1_SLAVE) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->dst_periph = HAL_AUDMA_I2S1_TX;
    }
#endif
#endif
#ifdef AF_DEVICE_INT_CODEC
    else if (device == AUD_STREAM_USE_INT_CODEC) {
      AF_TRACE_DEBUG();
      codec_int_open();
      codec_int_stream_open(stream);

#ifdef AUDIO_OUTPUT_SW_GAIN
#ifdef AUDIO_OUTPUT_SW_LIMITER
      FLOATLIMITER_ERROR limiter_err;

      if (FloatLimiterP == NULL) {
        FloatLimiterP = CreateFloatLimiter(
            FL_ATTACK_DEFAULT_MS, FL_RELEASE_DEFAULT_MS, FL_THRESHOLD,
            MAX_CHANEL, MAX_SAMPLERATE, MAX_SAMPLEBIT);
      }

      limiter_err = SetFloatLimiterNChannels(FloatLimiterP, cfg->channel_num);
      if (limiter_err != FLOATLIMIT_OK) {
        goto _exit;
      }
      limiter_err = SetFloatLimiterSampleRate(FloatLimiterP, cfg->sample_rate);
      if (limiter_err != FLOATLIMIT_OK) {
        goto _exit;
      }
      limiter_err = SetFloatLimiterSampleBit(FloatLimiterP, cfg->bits);
      if (limiter_err != FLOATLIMIT_OK) {
        goto _exit;
      }
#else
      sw_gain_iir.history_x[0] = 0.0f;
      sw_gain_iir.history_x[1] = 0.0f;
      sw_gain_iir.history_y[0] = 0.0f;
      sw_gain_iir.history_y[1] = 0.0f;
#endif
#endif

#ifdef CODEC_DSD
      if (af_dsd_enabled) {
        dma_cfg->dst_periph = HAL_AUDMA_DSD_TX;
        dma_cfg->dst_bsize = HAL_DMA_BSIZE_1;
      } else
#endif
      {
        dma_cfg->dst_periph = HAL_AUDMA_CODEC_TX;
      }
    }
#endif
#ifdef AF_DEVICE_SPDIF
    else if (device == AUD_STREAM_USE_INT_SPDIF) {
      AF_TRACE_DEBUG();
      hal_spdif_open(AF_SPDIF_INST, stream);
      dma_cfg->dst_periph = HAL_AUDMA_SPDIF0_TX;
    }
#endif
#ifdef AF_DEVICE_BT_PCM
    else if (device == AUD_STREAM_USE_BT_PCM) {
      AF_TRACE_DEBUG();
      hal_btpcm_open(AF_BTPCM_INST, stream);
      dma_cfg->dst_periph = HAL_AUDMA_BTPCM_TX;
    }
#endif
#ifdef AUDIO_ANC_FB_MC
    else if (device == AUD_STREAM_USE_MC) {
      AF_TRACE_DEBUG();
      dma_cfg->dst_periph = HAL_AUDMA_MC_RX;
    }
#endif
    else {
      ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
    }

    dma_cfg->ch = hal_audma_get_chan(dma_cfg->dst_periph, HAL_DMA_HIGH_PRIO);
  } else {
    AF_TRACE_DEBUG();
    dma_cfg->dst_periph = (enum HAL_DMA_PERIPH_T)0;
    dma_cfg->type = HAL_DMA_FLOW_P2M_DMA;

    // open device and stream
    if (0) {
    }
#ifdef AF_DEVICE_EXT_CODEC
    else if (device == AUD_STREAM_USE_EXT_CODEC) {
      AF_TRACE_DEBUG();
      tlv32aic32_open();
      tlv32aic32_stream_open(stream);

      dma_cfg->src_periph = HAL_AUDMA_I2S0_RX;
    }
#endif
#ifdef AF_DEVICE_I2S
    else if (device == AUD_STREAM_USE_I2S0_MASTER) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->src_periph = HAL_AUDMA_I2S0_RX;
    } else if (device == AUD_STREAM_USE_I2S0_SLAVE) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->src_periph = HAL_AUDMA_I2S0_RX;
    }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
    else if (device == AUD_STREAM_USE_I2S1_MASTER) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->src_periph = HAL_AUDMA_I2S1_RX;
    } else if (device == AUD_STREAM_USE_I2S1_SLAVE) {
      AF_TRACE_DEBUG();
      hal_i2s_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->src_periph = HAL_AUDMA_I2S1_RX;
    }
#endif
#endif
#ifdef AF_DEVICE_TDM
    else if (device == AUD_STREAM_USE_TDM0_MASTER) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->src_periph = HAL_AUDMA_I2S0_RX;
      AF_TRACE_DEBUG();
    } else if (device == AUD_STREAM_USE_TDM0_SLAVE) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_0, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->src_periph = HAL_AUDMA_I2S0_RX;
    }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
    else if (device == AUD_STREAM_USE_TDM1_MASTER) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_MASTER);
      dma_cfg->src_periph = HAL_AUDMA_I2S1_RX;
    } else if (device == AUD_STREAM_USE_TDM1_SLAVE) {
      AF_TRACE_DEBUG();
      hal_tdm_open(HAL_I2S_ID_1, stream, HAL_I2S_MODE_SLAVE);
      dma_cfg->src_periph = HAL_AUDMA_I2S1_RX;
    }
#endif
#endif
#ifdef AF_DEVICE_INT_CODEC
    else if (device == AUD_STREAM_USE_INT_CODEC) {
      AF_TRACE_DEBUG();
      codec_int_open();
      codec_int_stream_open(stream);

      dma_cfg->src_periph = HAL_AUDMA_CODEC_RX;
    }
#endif
#ifdef AF_DEVICE_SPDIF
    else if (device == AUD_STREAM_USE_INT_SPDIF) {
      AF_TRACE_DEBUG();
      hal_spdif_open(AF_SPDIF_INST, stream);

      dma_cfg->src_periph = HAL_AUDMA_SPDIF0_RX;
    }
#endif
#ifdef AF_DEVICE_BT_PCM
    else if (device == AUD_STREAM_USE_BT_PCM) {
      AF_TRACE_DEBUG();
      hal_btpcm_open(AF_BTPCM_INST, stream);

      dma_cfg->src_periph = HAL_AUDMA_BTPCM_RX;
    }
#endif
#ifdef AF_DEVICE_DPD_RX
    else if (device == AUD_STREAM_USE_DPD_RX) {
      AF_TRACE_DEBUG();
      dma_cfg->src_periph = HAL_AUDMA_DPD_RX;
    }
#endif
    else {
      ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
    }
    dma_cfg->ch = hal_audma_get_chan(dma_cfg->src_periph, HAL_DMA_HIGH_PRIO);
  }
  af_stream_update_dma_buffer(stream, role, cfg);
  role->handler = cfg->handler;

  af_set_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);
#ifndef RTOS
  af_clear_flag(&af_flag_signal, 1 << (id * 2 + stream));
  af_set_flag(&af_flag_open, 1 << (id * 2 + stream));
#endif

  AF_TRACE_DEBUG();
  ret = AF_RES_SUCCESS;

_exit:
  af_unlock_thread();
  if (ret == AF_RES_SUCCESS) {
    af_stream_setup(id, stream, &role->cfg);
  }

  return ret;
}

// volume, path, sample rate, channel num ...
uint32_t af_stream_setup(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
                         const struct AF_STREAM_CONFIG_T *cfg) {
  AF_TRACE_DEBUG();

  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;

  role = af_get_stream_role(id, stream);
  TRACE(3, "[%s] id = %d, stream = %d", __func__, id, stream);

  ret = AF_RES_FAILD;

  af_lock_thread();

  // check stream is open
  if (!(role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE)) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    goto _exit;
  }

  device = role->ctl.use_device;

  if (&role->cfg != cfg) {
    bool update_dma = false;

    if (role->cfg.bits != cfg->bits) {
      TRACE(4, "[%s] Change bits from %d to %d for stream %d", __func__,
            role->cfg.bits, cfg->bits, stream);
      update_dma = true;
    }
    if (role->cfg.data_ptr != cfg->data_ptr) {
      TRACE(4, "[%s] Change data_ptr from %p to %p for stream %d", __func__,
            role->cfg.data_ptr, cfg->data_ptr, stream);
      update_dma = true;
    }
    if (role->cfg.data_size != cfg->data_size) {
      TRACE(4, "[%s] Change data_size from %d to %d for stream %d", __func__,
            role->cfg.data_size, cfg->data_size, stream);
      update_dma = true;
    }
    if (update_dma) {
      // To avoid FIFO corruption, streams must be stopped before changing
      // sample size
      ASSERT((role->ctl.status & AF_STATUS_STREAM_START_STOP) == 0,
             "[%s] ERROR: Update dma while stream %d started", __func__,
             stream);

      af_stream_update_dma_buffer(stream, role, cfg);
    }

    if (role->cfg.sample_rate != cfg->sample_rate) {
      TRACE(4, "[%s] Change sample rate from %d to %d for stream %d", __func__,
            role->cfg.sample_rate, cfg->sample_rate, stream);

      // To avoid L/R sample misalignment, streams must be stopped before
      // changing sample rate
      ASSERT((role->ctl.status & AF_STATUS_STREAM_START_STOP) == 0,
             "[%s] ERROR: Change sample rate from %d to %d while stream %d "
             "started",
             __func__, role->cfg.sample_rate, cfg->sample_rate, stream);
    }

    role->cfg = *cfg;
  }

  AF_TRACE_DEBUG();
  if (0) {
  }
#ifdef AF_DEVICE_EXT_CODEC
  else if (device == AUD_STREAM_USE_EXT_CODEC) {
    AF_TRACE_DEBUG();

    struct tlv32aic32_config_t tlv32aic32_cfg;

    memset(&tlv32aic32_cfg, 0, sizeof(tlv32aic32_cfg));
    tlv32aic32_cfg.bits = cfg->bits;
    tlv32aic32_cfg.channel_num = cfg->channel_num;
    tlv32aic32_cfg.channel_map = cfg->channel_map;
    tlv32aic32_cfg.sample_rate = cfg->sample_rate;
    tlv32aic32_cfg.use_dma = AF_TRUE;
    tlv32aic32_cfg.chan_sep_buf = cfg->chan_sep_buf;
    tlv32aic32_cfg.sync_start = cfg->sync_start;
    tlv32aic32_cfg.slot_cycles = cfg->slot_cycles;
    tlv32aic32_stream_setup(stream, &tlv32aic32_cfg);
  }
#endif
#ifdef AF_DEVICE_I2S
  else if (device == AUD_STREAM_USE_I2S0_MASTER ||
           device == AUD_STREAM_USE_I2S0_SLAVE
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
           || device == AUD_STREAM_USE_I2S1_MASTER ||
           device == AUD_STREAM_USE_I2S1_SLAVE
#endif
  ) {
    AF_TRACE_DEBUG();

    struct HAL_I2S_CONFIG_T i2s_cfg;
    enum HAL_I2S_ID_T i2s_id;

    i2s_id = HAL_I2S_ID_0;
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
    if (device == AUD_STREAM_USE_I2S1_MASTER ||
        device == AUD_STREAM_USE_I2S1_SLAVE) {
      i2s_id = HAL_I2S_ID_1;
    }
#endif

    memset(&i2s_cfg, 0, sizeof(i2s_cfg));
    i2s_cfg.use_dma = AF_TRUE;
    i2s_cfg.chan_sep_buf = cfg->chan_sep_buf;
    i2s_cfg.sync_start = cfg->sync_start;
    i2s_cfg.cycles = cfg->slot_cycles;
    i2s_cfg.bits = cfg->bits;
    i2s_cfg.channel_num = cfg->channel_num;
    i2s_cfg.channel_map = cfg->channel_map;
    i2s_cfg.sample_rate = cfg->sample_rate;
    hal_i2s_setup_stream(i2s_id, stream, &i2s_cfg);
  }
#endif
#ifdef AF_DEVICE_TDM
  else if (device == AUD_STREAM_USE_TDM0_MASTER ||
           device == AUD_STREAM_USE_TDM0_SLAVE
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
           || device == AUD_STREAM_USE_TDM1_MASTER ||
           device == AUD_STREAM_USE_TDM1_SLAVE
#endif
  ) {
    AF_TRACE_DEBUG();

    struct HAL_TDM_CONFIG_T tdm_cfg;
    enum HAL_I2S_ID_T i2s_id;

    i2s_id = HAL_I2S_ID_0;
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
    if (device == AUD_STREAM_USE_TDM1_MASTER ||
        device == AUD_STREAM_USE_TDM1_SLAVE) {
      i2s_id = HAL_I2S_ID_1;
    }
#endif

    memset(&tdm_cfg, 0, sizeof(tdm_cfg));
    tdm_cfg.mode = HAL_TDM_MODE_FS_ASSERTED_AT_FIRST;
    tdm_cfg.edge = HAL_TDM_FS_EDGE_POSEDGE;
    tdm_cfg.cycles = (cfg->channel_num * cfg->slot_cycles);
    tdm_cfg.fs_cycles = cfg->fs_cycles;
    tdm_cfg.slot_cycles = cfg->slot_cycles;
    tdm_cfg.data_offset = 0;
    tdm_cfg.sync_start = cfg->sync_start;
    TRACE(5, "%s: cycle = %d, fs_cycles = %d, slot_cycles = %d,wait = %d.",
          __func__, tdm_cfg.cycles, tdm_cfg.fs_cycles, tdm_cfg.slot_cycles,
          tdm_cfg.sync_start);
    hal_tdm_setup_stream(i2s_id, stream, cfg->sample_rate, &tdm_cfg);
  }

#endif
#ifdef AF_DEVICE_INT_CODEC
  else if (device == AUD_STREAM_USE_INT_CODEC) {
    AF_TRACE_DEBUG();
    struct HAL_CODEC_CONFIG_T codec_cfg;
    memset(&codec_cfg, 0, sizeof(codec_cfg));
    codec_cfg.bits = cfg->bits;
    codec_cfg.sample_rate = cfg->sample_rate;
    codec_cfg.channel_num = cfg->channel_num;
    codec_cfg.channel_map = cfg->channel_map;
    codec_cfg.use_dma = AF_TRUE;
    codec_cfg.vol = cfg->vol;
    codec_cfg.io_path = cfg->io_path;

    AF_TRACE_DEBUG();
    codec_int_stream_setup(stream, &codec_cfg);
  }
#endif
#ifdef AF_DEVICE_SPDIF
  else if (device == AUD_STREAM_USE_INT_SPDIF) {
    AF_TRACE_DEBUG();
    struct HAL_SPDIF_CONFIG_T spdif_cfg;

    memset(&spdif_cfg, 0, sizeof(spdif_cfg));
    spdif_cfg.use_dma = AF_TRUE;
    spdif_cfg.bits = cfg->bits;
    spdif_cfg.channel_num = cfg->channel_num;
    spdif_cfg.sample_rate = cfg->sample_rate;
    hal_spdif_setup_stream(AF_SPDIF_INST, stream, &spdif_cfg);
  }
#endif
#ifdef AF_DEVICE_BT_PCM
  else if (device == AUD_STREAM_USE_BT_PCM) {
    AF_TRACE_DEBUG();
    struct HAL_BTPCM_CONFIG_T btpcm_cfg;

    memset(&btpcm_cfg, 0, sizeof(btpcm_cfg));
    btpcm_cfg.use_dma = AF_TRUE;
    btpcm_cfg.bits = cfg->bits;
    btpcm_cfg.channel_num = cfg->channel_num;
    btpcm_cfg.sample_rate = cfg->sample_rate;
    hal_btpcm_setup_stream(AF_BTPCM_INST, stream, &btpcm_cfg);
  }
#endif
#ifdef AF_DEVICE_DPD_RX
  else if (device == AUD_STREAM_USE_DPD_RX) {
    AF_TRACE_DEBUG();
  }
#endif
#ifdef AUDIO_ANC_FB_MC
  else if (device == AUD_STREAM_USE_MC) {
    AF_TRACE_DEBUG();
    hal_codec_setup_mc(cfg->channel_num, cfg->bits);
  }
#endif
  else {
    ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
  }

  AF_TRACE_DEBUG();

  ret = AF_RES_SUCCESS;

_exit:
  af_unlock_thread();

  return ret;
}

uint32_t af_stream_mute(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream,
                        bool mute) {
  AF_TRACE_DEBUG();

  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;

  ret = AF_RES_FAILD;
  role = af_get_stream_role(id, stream);

  af_lock_thread();

  if ((role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE) == 0) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    goto _exit;
  }

  device = role->ctl.use_device;

  if (device == AUD_STREAM_USE_INT_CODEC) {
    codec_int_stream_mute(stream, mute);
    ret = AF_RES_SUCCESS;
  }

_exit:
  af_unlock_thread();

  return ret;
}

uint32_t af_stream_set_chan_vol(enum AUD_STREAM_ID_T id,
                                enum AUD_STREAM_T stream,
                                enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol) {
  AF_TRACE_DEBUG();

  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;

  ret = AF_RES_FAILD;
  role = af_get_stream_role(id, stream);

  af_lock_thread();

  if ((role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE) == 0) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    goto _exit;
  }

  device = role->ctl.use_device;

  if (device == AUD_STREAM_USE_INT_CODEC) {
    codec_int_stream_set_chan_vol(stream, ch_map, vol);
    ret = AF_RES_SUCCESS;
  }

_exit:
  af_unlock_thread();

  return ret;
}

uint32_t af_stream_restore_chan_vol(enum AUD_STREAM_ID_T id,
                                    enum AUD_STREAM_T stream) {
  AF_TRACE_DEBUG();

  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;

  ret = AF_RES_FAILD;
  role = af_get_stream_role(id, stream);

  af_lock_thread();

  if ((role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE) == 0) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    goto _exit;
  }

  device = role->ctl.use_device;

  if (device == AUD_STREAM_USE_INT_CODEC) {
    codec_int_stream_restore_chan_vol(stream);
    ret = AF_RES_SUCCESS;
  }

_exit:
  af_unlock_thread();

  return ret;
}

#ifdef CODEC_PLAY_BEFORE_CAPTURE
static struct af_stream_cfg_t *codec_capture_role;

static void af_codec_stream_pre_start(enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role = NULL;

  if (stream == AUD_STREAM_CAPTURE) {
    return;
  }

  for (uint8_t id = 0; id < AUD_STREAM_ID_NUM; id++) {
    role = af_get_stream_role((enum AUD_STREAM_ID_T)id, AUD_STREAM_CAPTURE);
    if (role->cfg.device == AUD_STREAM_USE_INT_CODEC &&
        role->ctl.status ==
            (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
             AF_STATUS_STREAM_START_STOP)) {
      hal_audma_stop(role->dma_cfg.ch);
      codec_int_stream_stop(AUD_STREAM_CAPTURE);
      codec_capture_role = role;
      return;
    }
  }
}

static void af_codec_stream_post_start(enum AUD_STREAM_T stream) {
  if (stream == AUD_STREAM_CAPTURE) {
    return;
  }

  if (codec_capture_role) {
    hal_audma_sg_start(&codec_capture_role->dma_desc[0],
                       &codec_capture_role->dma_cfg);
    codec_int_stream_start(AUD_STREAM_CAPTURE);
    codec_capture_role = NULL;
  }
}

static void af_codec_stream_pre_stop(enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role = NULL;

  if (stream == AUD_STREAM_CAPTURE) {
    return;
  }

  for (uint8_t id = 0; id < AUD_STREAM_ID_NUM; id++) {
    role = af_get_stream_role((enum AUD_STREAM_ID_T)id, AUD_STREAM_CAPTURE);
    if (role->cfg.device == AUD_STREAM_USE_INT_CODEC &&
        role->ctl.status ==
            (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
             AF_STATUS_STREAM_START_STOP)) {
      hal_audma_stop(role->dma_cfg.ch);
      codec_int_stream_stop(AUD_STREAM_CAPTURE);
      codec_capture_role = role;
      return;
    }
  }
}

static void af_codec_stream_post_stop(enum AUD_STREAM_T stream) {
  if (stream == AUD_STREAM_CAPTURE) {
    return;
  }

  if (codec_capture_role) {
    hal_audma_sg_start(&codec_capture_role->dma_desc[0],
                       &codec_capture_role->dma_cfg);
    codec_int_stream_start(AUD_STREAM_CAPTURE);
    codec_capture_role = NULL;
  }
}
#endif

uint32_t af_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;
  enum HAL_DMA_RET_T dma_ret;

  role = af_get_stream_role(id, stream);
  TRACE(3, "[%s] id = %d, stream = %d", __func__, id, stream);

  af_lock_thread();

  // check stream is open and not start.
  if (role->ctl.status !=
      (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    ret = AF_RES_FAILD;
    goto _exit;
  }

  device = role->ctl.use_device;

  role->ctl.pp_index = PP_PING;
  role->ctl.pp_cnt = 0;

#ifndef RTOS
  af_clear_flag(&af_flag_signal, 1 << (id * 2 + stream));
#endif

  if (device == AUD_STREAM_USE_INT_CODEC && stream == AUD_STREAM_PLAYBACK) {
#ifdef AUDIO_OUTPUT_PA_ON_FADE_IN
    dac_pa_state = AF_DAC_PA_ON_TRIGGER;
    // Has the buffer been zero-ed out?
    af_zero_mem(role->dma_buf_ptr, role->dma_buf_size);
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
    af_codec_playback_sw_dc_calib(role->dma_buf_ptr, role->dma_buf_size,
                                  role->cfg.bits, role->cfg.channel_num);
#endif
    dac_dc_start_time = hal_sys_timer_get();
#endif
  }

#ifndef CHIP_BEST1000
  if (role->cfg.chan_sep_buf && role->cfg.channel_num > AUD_CHANNEL_NUM_1) {
    struct HAL_DMA_2D_CFG_T *src, *dst;
    struct HAL_DMA_2D_CFG_T dma_2d_cfg;
    uint8_t burst_size = 1;
    uint32_t chan_xfer_cnt;

    src = NULL;
    dst = NULL;
    if (stream == AUD_STREAM_PLAYBACK) {
      if (role->cfg.channel_num == AUD_CHANNEL_NUM_2) {
        ASSERT(role->dma_cfg.src_bsize == HAL_DMA_BSIZE_1,
               "Play 2D DMA: Bad src burst size: %d", role->dma_cfg.src_bsize);
        burst_size = 1;
        src = &dma_2d_cfg;
      }
    } else {
      if (role->cfg.channel_num > AUD_CHANNEL_NUM_1) {
        ASSERT(role->dma_cfg.src_bsize == role->dma_cfg.dst_bsize,
               "Cap 2D DMA: src burst size (%d) != dst (%d)",
               role->dma_cfg.src_bsize, role->dma_cfg.dst_bsize);
        if (role->dma_cfg.dst_bsize == HAL_DMA_BSIZE_1) {
          burst_size = 1;
        } else if (role->dma_cfg.dst_bsize == HAL_DMA_BSIZE_4) {
          burst_size = 4;
        } else if (role->dma_cfg.dst_bsize == HAL_DMA_BSIZE_8) {
          burst_size = 8;
        } else {
          ASSERT(false, "Cap 2D DMA: Bad dst burst size: %d",
                 role->dma_cfg.dst_bsize);
        }
        dst = &dma_2d_cfg;
      }
    }

    if (src || dst) {
      chan_xfer_cnt =
          role->dma_cfg.src_tsize * AUDIO_BUFFER_COUNT / role->cfg.channel_num;
      dma_2d_cfg.xcount = role->cfg.channel_num;
      dma_2d_cfg.xmodify = chan_xfer_cnt - burst_size;
      dma_2d_cfg.ycount = chan_xfer_cnt / burst_size;
      dma_2d_cfg.ymodify = -chan_xfer_cnt * (role->cfg.channel_num - 1);
    }

    dma_ret = hal_dma_sg_2d_start(&role->dma_desc[0], &role->dma_cfg, src, dst);
  } else
#endif
  {
    dma_ret = hal_dma_sg_start(&role->dma_desc[0], &role->dma_cfg);
  }
  ASSERT(dma_ret == HAL_DMA_OK,
         "[%s] Failed to start dma for stream %d: ret=%d", __func__, stream,
         dma_ret);

  AF_TRACE_DEBUG();
  if (0) {
  }
#ifdef AF_DEVICE_EXT_CODEC
  else if (device == AUD_STREAM_USE_EXT_CODEC) {
    AF_TRACE_DEBUG();
    tlv32aic32_stream_start(stream);
  }
#endif
#ifdef AF_DEVICE_I2S
  else if (device == AUD_STREAM_USE_I2S0_MASTER ||
           device == AUD_STREAM_USE_I2S0_SLAVE) {
    hal_i2s_start_stream(HAL_I2S_ID_0, stream);
  }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
  else if (device == AUD_STREAM_USE_I2S1_MASTER ||
           device == AUD_STREAM_USE_I2S1_SLAVE) {
    hal_i2s_start_stream(HAL_I2S_ID_1, stream);
  }
#endif
#endif
#ifdef AF_DEVICE_TDM
  else if (device == AUD_STREAM_USE_TDM0_MASTER ||
           device == AUD_STREAM_USE_TDM0_SLAVE) {
    hal_tdm_start_stream(HAL_I2S_ID_0, stream);
  }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
  else if (device == AUD_STREAM_USE_TDM1_MASTER ||
           device == AUD_STREAM_USE_TDM1_SLAVE) {
    hal_tdm_start_stream(HAL_I2S_ID_1, stream);
  }
#endif
#endif
#ifdef AF_DEVICE_INT_CODEC
  else if (device == AUD_STREAM_USE_INT_CODEC) {
    AF_TRACE_DEBUG();
#ifdef CODEC_PLAY_BEFORE_CAPTURE
    af_codec_stream_pre_start(stream);
#endif
    codec_int_stream_start(stream);
#ifdef CODEC_PLAY_BEFORE_CAPTURE
    af_codec_stream_post_start(stream);
#endif
  }
#endif
#ifdef AF_DEVICE_SPDIF
  else if (device == AUD_STREAM_USE_INT_SPDIF) {
    AF_TRACE_DEBUG();
    hal_spdif_start_stream(AF_SPDIF_INST, stream);
  }
#endif
#ifdef AF_DEVICE_BT_PCM
  else if (device == AUD_STREAM_USE_BT_PCM) {
    AF_TRACE_DEBUG();
    hal_btpcm_start_stream(AF_BTPCM_INST, stream);
  }
#endif
#ifdef AF_DEVICE_DPD_RX
  else if (device == AUD_STREAM_USE_DPD_RX) {
    AF_TRACE_DEBUG();
    hal_btpcm_start_stream(AF_BTPCM_INST, stream);
  }
#endif
#ifdef AUDIO_ANC_FB_MC
  else if (device == AUD_STREAM_USE_MC) {
    AF_TRACE_DEBUG();
  }
#endif
  else {
    ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
  }

  AF_TRACE_DEBUG();
  af_set_status(id, stream, AF_STATUS_STREAM_START_STOP);

  ret = AF_RES_SUCCESS;

_exit:
  af_unlock_thread();

  return ret;
}

uint32_t af_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;

  role = af_get_stream_role(id, stream);
  TRACE(3, "[%s] id = %d, stream = %d", __func__, id, stream);

  af_lock_thread();

  device = role->ctl.use_device;

  if (device == AUD_STREAM_USE_INT_CODEC &&
      role->ctl.status ==
          (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
           AF_STATUS_STREAM_START_STOP | AF_STATUS_STREAM_PAUSE_RESTART)) {
    af_clear_status(id, stream, AF_STATUS_STREAM_PAUSE_RESTART);
    goto _pause_stop;
  }

  // check stream is start and not stop
  if (role->ctl.status != (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
                           AF_STATUS_STREAM_START_STOP)) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    ret = AF_RES_FAILD;
    goto _exit;
  }

#if defined(RTOS) && defined(AF_STREAM_ID_0_PLAYBACK_FADEOUT)
  if (id == AUD_STREAM_ID_0 && stream == AUD_STREAM_PLAYBACK) {
    af_stream_fadeout_start(512);
    af_stream_stop_wait_finish();
  }
#endif

  if (device == AUD_STREAM_USE_INT_CODEC && stream == AUD_STREAM_PLAYBACK) {
#ifdef AUDIO_OUTPUT_PA_OFF_FADE_OUT
    dac_pa_state = AF_DAC_PA_OFF_TRIGGER;
    af_unlock_thread();
    while (dac_pa_state != AF_DAC_PA_NULL) {
#ifdef RTOS
      osSignalWait((1 << AF_FADE_OUT_SIGNAL_ID), 300);
      osSignalClear(fade_thread_id, (1 << AF_FADE_OUT_SIGNAL_ID));
#else
      af_thread(NULL);
#endif
    }
    af_lock_thread();
#elif defined(AUDIO_OUTPUT_PA_ON_FADE_IN)
    dac_pa_state = AF_DAC_PA_NULL;
    analog_aud_codec_speaker_enable(false);
    dac_pa_stop_time = hal_sys_timer_get();
#endif // !AUDIO_OUTPUT_PA_OFF_FADE_OUT && AUDIO_OUTPUT_PA_ON_FADE_IN
  }

  hal_audma_stop(role->dma_cfg.ch);

#if defined(RTOS) && defined(AF_STREAM_ID_0_PLAYBACK_FADEOUT)
  if (id == AUD_STREAM_ID_0 && stream == AUD_STREAM_PLAYBACK) {
    af_stream_fadeout_stop();
  }
#endif

  if (0) {
  }
#ifdef AF_DEVICE_EXT_CODEC
  else if (device == AUD_STREAM_USE_EXT_CODEC) {
    AF_TRACE_DEBUG();
    tlv32aic32_stream_stop(stream);
  }
#endif
#ifdef AF_DEVICE_I2S
  else if (device == AUD_STREAM_USE_I2S0_MASTER ||
           device == AUD_STREAM_USE_I2S0_SLAVE) {
    AF_TRACE_DEBUG();
    hal_i2s_stop_stream(HAL_I2S_ID_0, stream);
  }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
  else if (device == AUD_STREAM_USE_I2S1_MASTER ||
           device == AUD_STREAM_USE_I2S1_SLAVE) {
    AF_TRACE_DEBUG();
    hal_i2s_stop_stream(HAL_I2S_ID_1, stream);
  }
#endif
#endif
#ifdef AF_DEVICE_TDM
  else if (device == AUD_STREAM_USE_TDM0_MASTER ||
           device == AUD_STREAM_USE_TDM0_SLAVE) {
    AF_TRACE_DEBUG();
    hal_tdm_stop_stream(HAL_I2S_ID_0, stream);
  }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
  else if (device == AUD_STREAM_USE_TDM1_MASTER ||
           device == AUD_STREAM_USE_TDM1_SLAVE) {
    AF_TRACE_DEBUG();
    hal_tdm_stop_stream(HAL_I2S_ID_1, stream);
  }
#endif
#endif
#ifdef AF_DEVICE_INT_CODEC
  else if (device == AUD_STREAM_USE_INT_CODEC) {
    AF_TRACE_DEBUG();
#ifdef CODEC_PLAY_BEFORE_CAPTURE
    af_codec_stream_pre_stop(stream);
#endif
    codec_int_stream_stop(stream);
#ifdef CODEC_PLAY_BEFORE_CAPTURE
    af_codec_stream_post_stop(stream);
#endif
  }
#endif
#ifdef AF_DEVICE_SPDIF
  else if (device == AUD_STREAM_USE_INT_SPDIF) {
    AF_TRACE_DEBUG();
    hal_spdif_stop_stream(AF_SPDIF_INST, stream);
  }
#endif
#ifdef AF_DEVICE_BT_PCM
  else if (device == AUD_STREAM_USE_BT_PCM) {
    AF_TRACE_DEBUG();
    hal_btpcm_stop_stream(AF_BTPCM_INST, stream);
  }
#endif
#ifdef AF_DEVICE_DPD_RX
  else if (device == AUD_STREAM_USE_DPD_RX) {
    AF_TRACE_DEBUG();
    hal_btpcm_stop_stream(AF_BTPCM_INST, stream);
  }
#endif
#ifdef AUDIO_ANC_FB_MC
  else if (device == AUD_STREAM_USE_MC) {
    AF_TRACE_DEBUG();
  }
#endif
  else {
    ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
  }

_pause_stop:
  AF_TRACE_DEBUG();
  af_clear_status(id, stream, AF_STATUS_STREAM_START_STOP);

#ifndef RTOS
  af_clear_flag(&af_flag_signal, 1 << (id * 2 + stream));
#endif

  ret = AF_RES_SUCCESS;

_exit:
  af_unlock_thread();

  return ret;
}

uint32_t af_stream_pause(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream) {
#ifdef CHIP_BEST1000

  struct af_stream_cfg_t *role = NULL;
  enum AF_RESULT_T ret;

  role = af_get_stream_role(id, stream);
  TRACE(3, "[%s] id = %d, stream = %d", __func__, id, stream);

  af_lock_thread();

  if (role->cfg.device != AUD_STREAM_USE_INT_CODEC) {
    ret = AF_RES_FAILD;
    goto _exit;
  }
  if (role->ctl.status != (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
                           AF_STATUS_STREAM_START_STOP)) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    ret = AF_RES_FAILD;
    goto _exit;
  }

#if defined(AUDIO_OUTPUT_PA_ON_FADE_IN) || defined(AUDIO_OUTPUT_PA_OFF_FADE_OUT)
  if (AUD_STREAM_PLAYBACK == stream) {
    dac_pa_state = AF_DAC_PA_NULL;
    analog_aud_codec_speaker_enable(false);
    dac_pa_stop_time = hal_sys_timer_get();
  }
#endif

  hal_audma_stop(role->dma_cfg.ch);

  codec_int_stream_stop(stream);

  af_set_status(id, stream, AF_STATUS_STREAM_PAUSE_RESTART);

  ret = AF_RES_SUCCESS;

_exit:
  af_unlock_thread();

  return ret;

#else

  return af_stream_stop(id, stream);

#endif
}

uint32_t af_stream_restart(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream) {
#ifdef CHIP_BEST1000

  struct af_stream_cfg_t *role = NULL;
  enum AF_RESULT_T ret;

  role = af_get_stream_role(id, stream);
  TRACE(3, "[%s] id = %d, stream = %d", __func__, id, stream);

  af_lock_thread();

  if (role->cfg.device != AUD_STREAM_USE_INT_CODEC) {
    ret = AF_RES_FAILD;
    goto _exit;
  }
  if (role->ctl.status !=
      (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
       AF_STATUS_STREAM_START_STOP | AF_STATUS_STREAM_PAUSE_RESTART)) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    ret = AF_RES_FAILD;
    goto _exit;
  }

  role->ctl.pp_index = PP_PING;
  role->ctl.pp_cnt = 0;

#ifdef AUDIO_OUTPUT_PA_ON_FADE_IN
  if (AUD_STREAM_PLAYBACK == stream) {
    dac_pa_state = AF_DAC_PA_ON_TRIGGER;
    af_zero_mem(role->dma_buf_ptr, role->dma_buf_size);
#ifdef AUDIO_OUTPUT_DC_CALIB_SW
    af_codec_playback_sw_dc_calib(role->dma_buf_ptr, role->dma_buf_size,
                                  role->cfg.bits, role->cfg.channel_num);
#endif
    dac_dc_start_time = hal_sys_timer_get();
  }
#endif

  hal_audma_sg_start(&role->dma_desc[0], &role->dma_cfg);
  codec_int_stream_start(stream);
  af_clear_status(id, stream, AF_STATUS_STREAM_PAUSE_RESTART);
  ret = AF_RES_SUCCESS;

_exit:
  af_unlock_thread();

  return ret;

#else

  return af_stream_start(id, stream);

#endif
}

uint32_t af_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role;
  enum AF_RESULT_T ret;
  enum AUD_STREAM_USE_DEVICE_T device;

  role = af_get_stream_role(id, stream);
  TRACE(3, "[%s] id = %d, stream = %d", __func__, id, stream);

  af_lock_thread();

  // check stream is stop and not close.
  if (role->ctl.status !=
      (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) {
    TRACE(2, "[%s] ERROR: status = %d", __func__, role->ctl.status);
    ret = AF_RES_FAILD;
    goto _exit;
  }

  device = role->ctl.use_device;

  memset(role->dma_buf_ptr, 0, role->dma_buf_size);
  hal_audma_free_chan(role->dma_cfg.ch);

  //  TODO: more parameter should be set!!!
  //    memset(role, 0xff, sizeof(struct af_stream_cfg_t));
  role->handler = NULL;
  role->ctl.pp_index = PP_PING;
  role->ctl.use_device = AUD_STREAM_USE_DEVICE_NULL;
  role->dma_buf_ptr = NULL;
  role->dma_buf_size = 0;

  role->dma_cfg.ch = HAL_DMA_CHAN_NONE;

  if (0) {
  }
#ifdef AF_DEVICE_EXT_CODEC
  else if (device == AUD_STREAM_USE_EXT_CODEC) {
    AF_TRACE_DEBUG();
    tlv32aic32_stream_close(stream);
  }
#endif
#ifdef AF_DEVICE_I2S
  else if (device == AUD_STREAM_USE_I2S0_MASTER ||
           device == AUD_STREAM_USE_I2S0_SLAVE) {
    AF_TRACE_DEBUG();
    hal_i2s_close(HAL_I2S_ID_0, stream);
  }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
  else if (device == AUD_STREAM_USE_I2S1_MASTER ||
           device == AUD_STREAM_USE_I2S1_SLAVE) {
    AF_TRACE_DEBUG();
    hal_i2s_close(HAL_I2S_ID_1, stream);
  }
#endif
#endif
#ifdef AF_DEVICE_TDM
  else if (device == AUD_STREAM_USE_TDM0_MASTER ||
           device == AUD_STREAM_USE_TDM0_SLAVE) {
    AF_TRACE_DEBUG();
    hal_tdm_close(HAL_I2S_ID_0, stream);
  }
#if defined(CHIP_HAS_I2S) && (CHIP_HAS_I2S > 1)
  else if (device == AUD_STREAM_USE_TDM1_MASTER ||
           device == AUD_STREAM_USE_TDM1_SLAVE) {
    AF_TRACE_DEBUG();
    hal_tdm_close(HAL_I2S_ID_1, stream);
  }
#endif
#endif
#ifdef AF_DEVICE_INT_CODEC
  else if (device == AUD_STREAM_USE_INT_CODEC) {
    AF_TRACE_DEBUG();
    codec_int_stream_close(stream);
    codec_int_close(CODEC_CLOSE_NORMAL);
  }
#endif
#ifdef AF_DEVICE_SPDIF
  else if (device == AUD_STREAM_USE_INT_SPDIF) {
    AF_TRACE_DEBUG();
    hal_spdif_close(AF_SPDIF_INST, stream);
  }
#endif
#ifdef AF_DEVICE_BT_PCM
  else if (device == AUD_STREAM_USE_BT_PCM) {
    AF_TRACE_DEBUG();
    hal_btpcm_close(AF_BTPCM_INST, stream);
  }
#endif
#ifdef AF_DEVICE_DPD_RX
  else if (device == AUD_STREAM_USE_DPD_RX) {
    AF_TRACE_DEBUG();
  }
#endif
#ifdef AUDIO_ANC_FB_MC
  else if (device == AUD_STREAM_USE_MC) {
    AF_TRACE_DEBUG();
  }
#endif
  else {
    ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
  }

  AF_TRACE_DEBUG();
  af_clear_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);

#ifndef RTOS
  af_clear_flag(&af_flag_open, 1 << (id * 2 + stream));
  if (af_flag_open == 0) {
    hal_cpu_wake_unlock(AF_CPU_WAKE_USER);
  }
#endif

  ret = AF_RES_SUCCESS;

_exit:
  af_unlock_thread();

  return ret;
}

uint32_t af_close(void) {
  struct af_stream_cfg_t *role;

  // Avoid blocking shutdown process
  // af_lock_thread();

  for (uint8_t id = 0; id < AUD_STREAM_ID_NUM; id++) {
    for (uint8_t stream = 0; stream < AUD_STREAM_NUM; stream++) {
      role = af_get_stream_role((enum AUD_STREAM_ID_T)id,
                                (enum AUD_STREAM_T)stream);
      role->ctl.status = AF_STATUS_NULL;
    }
  }

  codec_int_close(CODEC_CLOSE_FORCED);

  // af_unlock_thread();

  return AF_RES_SUCCESS;
}

uint32_t af_stream_get_cur_dma_addr(enum AUD_STREAM_ID_T id,
                                    enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role;
  int i;
  uint32_t addr = 0;

  role = af_get_stream_role(id, stream);

  // check stream is start and not stop
  if (role->ctl.status == (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
                           AF_STATUS_STREAM_START_STOP)) {
    if (role->dma_cfg.ch != HAL_DMA_CHAN_NONE) {
      for (i = 0; i < 2; i++) {
        if (stream == AUD_STREAM_PLAYBACK) {
          addr = hal_audma_get_cur_src_addr(role->dma_cfg.ch);
        } else {
          addr = hal_audma_get_cur_dst_addr(role->dma_cfg.ch);
        }
        if (addr) {
          break;
        }
        // Previous link list item was just finished. Read current DMA address
        // again.
      }
    }
  }

  return addr;
}

int af_stream_get_cur_dma_pos(enum AUD_STREAM_ID_T id,
                              enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role;
  uint32_t addr;
  int pos;

  role = af_get_stream_role(id, stream);

  addr = af_stream_get_cur_dma_addr(id, stream);

  pos = addr - (uint32_t)role->dma_buf_ptr;

  if (pos < 0 || pos > role->dma_buf_size) {
    return -1;
  }

#ifndef CHIP_BEST1000
  if (role->cfg.chan_sep_buf && role->cfg.channel_num > AUD_CHANNEL_NUM_1) {
    uint32_t chan_size;
    uint8_t chan_idx;
    uint8_t desc_idx;
    uint16_t chan_desc_offset;
    uint16_t chan_desc_xfer_size;
    uint32_t chan_offset;

    chan_size = role->dma_buf_size / role->cfg.channel_num;
    chan_desc_xfer_size = chan_size / AUDIO_BUFFER_COUNT;

    chan_idx = pos / chan_size;
    chan_offset = pos % chan_size;
    desc_idx = chan_offset / chan_desc_xfer_size;
    chan_desc_offset = chan_offset % chan_desc_xfer_size;

    pos = desc_idx * (role->dma_buf_size / AUDIO_BUFFER_COUNT) +
          chan_idx * chan_desc_xfer_size + chan_desc_offset;
  }
#endif

  return pos;
}

int af_stream_buffer_error(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream) {
  return af_sig_lost_cnt[id][stream];
}

uint32_t af_stream_dma_tc_irq_enable(enum AUD_STREAM_ID_T id,
                                     enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role;

  role = af_get_stream_role(id, stream);

  // Check if opened
  if ((role->ctl.status &
       (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) ==
      (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) {
    hal_dma_tc_irq_enable(role->dma_cfg.ch);
    return AF_RES_SUCCESS;
  }

  return AF_RES_FAILD;
}

uint32_t af_stream_dma_tc_irq_disable(enum AUD_STREAM_ID_T id,
                                      enum AUD_STREAM_T stream) {
  struct af_stream_cfg_t *role;

  role = af_get_stream_role(id, stream);

  // Check if opened
  if ((role->ctl.status &
       (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) ==
      (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) {
    hal_dma_tc_irq_disable(role->dma_cfg.ch);
    return AF_RES_SUCCESS;
  }

  return AF_RES_FAILD;
}

#ifdef RTOS
void af_set_irq_notification(AF_IRQ_NOTIFICATION_T notif) { irq_notif = notif; }

static int af_priority[AF_USER_NUM];

void af_set_priority(AF_USER_E user, int priority) {
  uint8_t i = 0;
  int max_priority = 0;
  af_priority[user] = priority;

  for (i = 0; i < AF_USER_NUM; i++) {
    if (max_priority < af_priority[i]) {
      max_priority = af_priority[i];
    }
  }

  if (max_priority != af_get_priority()) {
    osThreadSetPriority(af_thread_tid, max_priority);
  }
}

int af_get_priority(void) { return (int)osThreadGetPriority(af_thread_tid); }

int af_get_default_priority(void) { return af_default_priority; }

void af_reset_priority(void) {
  osThreadSetPriority(af_thread_tid, af_default_priority);
}
#endif

void af_codec_tune_resample_rate(enum AUD_STREAM_T stream, float ratio) {
  af_lock_thread();

  af_codec_direct_tune_resample_rate(stream, ratio);

  af_unlock_thread();
}

void af_codec_direct_tune_resample_rate(enum AUD_STREAM_T stream, float ratio) {
  ASSERT(stream < AUD_STREAM_NUM, "[%s] Bad stream=%d", __func__, stream);

  hal_codec_tune_resample_rate(stream, ratio);
}

void af_codec_tune_both_resample_rate(float ratio) {
  af_lock_thread();

  hal_codec_tune_both_resample_rate(ratio);

  af_unlock_thread();
}

void af_codec_tune_pll(float ratio) {
  af_lock_thread();

  analog_aud_pll_tune(ratio);

  af_unlock_thread();
}

void af_codec_tune_xtal(float ratio) {
  af_lock_thread();

  analog_aud_xtal_tune(ratio);

  af_unlock_thread();
}

void af_codec_tune(enum AUD_STREAM_T stream, float ratio) {
  af_lock_thread();

  af_codec_direct_tune(stream, ratio);

  af_unlock_thread();
}

void af_codec_direct_tune(enum AUD_STREAM_T stream, float ratio) {
  ASSERT(stream <= AUD_STREAM_NUM, "[%s] Bad stream=%d", __func__, stream);

#ifdef __AUDIO_RESAMPLE__
  if (hal_cmu_get_audio_resample_status()) {
    if (stream < AUD_STREAM_NUM) {
      hal_codec_tune_resample_rate(stream, ratio);
    } else {
      hal_codec_tune_both_resample_rate(ratio);
    }
  } else
#endif
  {
    analog_aud_pll_tune(ratio);
  }
}

void af_codec_set_perf_test_power(int type) {
  af_lock_thread();

  hal_codec_dac_gain_m60db_check((enum HAL_CODEC_PERF_TEST_POWER_T)type);

  af_unlock_thread();
}

void af_codec_set_noise_reduction(bool enable) {
  af_lock_thread();

  hal_codec_set_noise_reduction(enable);

  af_unlock_thread();
}

void af_codec_bt_trigger_config(bool en,
                                AF_CODEC_BT_TRIGGER_CALLBACK callback) {
  if (en) {
    hal_codec_set_bt_trigger_callback(callback);
    hal_codec_bt_trigger_start();
  } else {
    hal_codec_bt_trigger_stop();
  }
}

void af_codec_sync_config(enum AUD_STREAM_T stream,
                          enum AF_CODEC_SYNC_TYPE_T type, bool enable) {
#ifndef CHIP_BEST1000
  af_lock_thread();

  if (stream == AUD_STREAM_PLAYBACK) {
    if (enable) {
      hal_codec_sync_dac_enable((enum HAL_CODEC_SYNC_TYPE_T)type);
    } else {
      hal_codec_sync_dac_disable();
    }
  } else {
    if (enable) {
      hal_codec_sync_adc_enable((enum HAL_CODEC_SYNC_TYPE_T)type);
    } else {
      hal_codec_sync_adc_disable();
    }
  }

  af_unlock_thread();
#endif
}

void af_i2s_sync_config(enum AUD_STREAM_T stream, enum AF_I2S_SYNC_TYPE_T type,
                        bool enable) {
#ifdef CHIP_BEST2300A
  af_lock_thread();

  if (stream == AUD_STREAM_PLAYBACK) {
    if (enable) {
      hal_i2s_clk_sync_enable((enum HAL_I2S_SYNC_TYPE_T)type);
    } else {
      hal_i2s_clk_sync_disable();
    }
  } else {
    if (enable) {
      hal_i2s_clk_sync_enable((enum HAL_I2S_SYNC_TYPE_T)type);
    } else {
      hal_i2s_clk_sync_disable();
    }
  }

  af_unlock_thread();
#endif
}

void af_codec_sync_resample_rate_config(enum AUD_STREAM_T stream,
                                        enum AF_CODEC_SYNC_TYPE_T type,
                                        bool enable) {
#ifndef CHIP_BEST1000
  af_lock_thread();

  if (stream == AUD_STREAM_PLAYBACK) {
    if (enable) {
      hal_codec_sync_dac_resample_rate_enable((enum HAL_CODEC_SYNC_TYPE_T)type);
    } else {
      hal_codec_sync_dac_resample_rate_disable();
    }
  } else {
    if (enable) {
      hal_codec_sync_adc_resample_rate_enable((enum HAL_CODEC_SYNC_TYPE_T)type);
    } else {
      hal_codec_sync_adc_resample_rate_disable();
    }
  }

  af_unlock_thread();
#endif
}

void af_codec_sync_gain_config(enum AUD_STREAM_T stream,
                               enum AF_CODEC_SYNC_TYPE_T type, bool enable) {
#ifndef CHIP_BEST1000
  af_lock_thread();

  if (stream == AUD_STREAM_PLAYBACK) {
    if (enable) {
      hal_codec_sync_dac_gain_enable((enum HAL_CODEC_SYNC_TYPE_T)type);
    } else {
      hal_codec_sync_dac_gain_disable();
    }
  } else {
    if (enable) {
      hal_codec_sync_adc_gain_enable((enum HAL_CODEC_SYNC_TYPE_T)type);
    } else {
      hal_codec_sync_adc_gain_disable();
    }
  }

  af_unlock_thread();
#endif
}

void af_codec_swap_output(bool swap) {
  af_lock_thread();

  hal_codec_swap_output(swap);

  af_unlock_thread();
}

void af_codec_set_playback_post_handler(AF_CODEC_PLAYBACK_POST_HANDLER_T hdlr) {
  codec_play_post_hdlr = hdlr;
}

int af_anc_open(enum ANC_TYPE_T type, enum AUD_SAMPRATE_T play_rate,
                enum AUD_SAMPRATE_T capture_rate, AF_ANC_HANDLER handler) {
  FUNC_ENTRY_TRACE();

  af_lock_thread();

  codec_anc_open(type, play_rate, capture_rate, handler);

  af_unlock_thread();

  return AF_RES_SUCCESS;
}

int af_anc_close(enum ANC_TYPE_T type) {
  FUNC_ENTRY_TRACE();

  af_lock_thread();

  codec_anc_close(type);

  af_unlock_thread();

  return AF_RES_SUCCESS;
}

#ifdef VOICE_DETECTOR_EN
int af_vad_open(const struct AUD_VAD_CONFIG_T *cfg) {
  FUNC_ENTRY_TRACE();

  af_lock_thread();

  codec_vad_open(cfg);

  af_unlock_thread();

  return AF_RES_SUCCESS;
}

int af_vad_close(void) {
  FUNC_ENTRY_TRACE();

  af_lock_thread();

  codec_vad_close();

  af_unlock_thread();

  return AF_RES_SUCCESS;
}

int af_vad_start(void) {
  FUNC_ENTRY_TRACE();

  af_lock_thread();

  hal_codec_vad_start();

  af_unlock_thread();

  return AF_RES_SUCCESS;
}

int af_vad_stop(void) {
  FUNC_ENTRY_TRACE();

  af_lock_thread();

  hal_codec_vad_stop();

  af_unlock_thread();

  return AF_RES_SUCCESS;
}

uint32_t af_vad_get_data(uint8_t *buf, uint32_t len) {
  return hal_codec_vad_recv_data(buf, len);
}

void af_vad_get_data_info(struct CODEC_VAD_BUF_INFO_T *vad_buf_info) {
  hal_codec_get_vad_data_info(vad_buf_info);
}

#endif

#ifdef CODEC_DSD
void af_dsd_enable(void) {
  enum AUD_STREAM_ID_T id;
  struct af_stream_cfg_t *role;
  bool opened = false;

  af_lock_thread();

  for (id = AUD_STREAM_ID_0; id < AUD_STREAM_ID_NUM; id++) {
    role = af_get_stream_role(id, AUD_STREAM_PLAYBACK);
    if (role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE) {
      if (role->cfg.device == AUD_STREAM_USE_INT_CODEC &&
          role->dma_cfg.dst_periph == HAL_AUDMA_CODEC_TX) {
        ASSERT(role->ctl.status ==
                   (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE),
               "Bad stream status when DSD enabled: 0x%X", role->ctl.status);
        role->dma_cfg.dst_periph = HAL_AUDMA_DSD_TX;
        af_stream_update_dma_buffer(AUD_STREAM_PLAYBACK, role, &role->cfg);
        opened = true;
        break;
      }
    }
  }

  hal_codec_dsd_enable();
  af_dsd_enabled = true;

  af_unlock_thread();

  if (opened) {
    // Enable DSD sample rate handling
    af_stream_setup(id, AUD_STREAM_PLAYBACK, &role->cfg);
  }
}

void af_dsd_disable(void) {
  enum AUD_STREAM_ID_T id;
  struct af_stream_cfg_t *role;
  bool opened = false;

  af_lock_thread();

  for (id = AUD_STREAM_ID_0; id < AUD_STREAM_ID_NUM; id++) {
    role = af_get_stream_role(id, AUD_STREAM_PLAYBACK);
    if (role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE) {
      if (role->cfg.device == AUD_STREAM_USE_INT_CODEC &&
          role->dma_cfg.dst_periph == HAL_AUDMA_DSD_TX) {
        ASSERT(role->ctl.status ==
                   (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE),
               "Bad stream status when DSD disabled: 0x%X", role->ctl.status);
        role->dma_cfg.dst_periph = HAL_AUDMA_CODEC_TX;
        af_stream_update_dma_buffer(AUD_STREAM_PLAYBACK, role, &role->cfg);
        opened = true;
        break;
      }
    }
  }

  hal_codec_dsd_disable();
  af_dsd_enabled = false;

  af_unlock_thread();

  if (opened) {
    // Disable DSD sample rate handling
    af_stream_setup(id, AUD_STREAM_PLAYBACK, &role->cfg);
  }
}
#endif
#ifdef __RAND_FROM_MIC__
uint8_t random_mic_is_on(uint8_t *deviceId) {
  struct af_stream_cfg_t *role = NULL;

  *deviceId = 0;

  for (uint8_t id = 0; id < AUD_STREAM_ID_NUM; id++) {
    role = af_get_stream_role((enum AUD_STREAM_ID_T)id, AUD_STREAM_CAPTURE);
    if (role->cfg.device == AUD_STREAM_USE_INT_CODEC &&
        role->ctl.status ==
            (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE |
             AF_STATUS_STREAM_START_STOP)) {
      return RAND_STATUS_MIC_STARTED;
    } else if (role->cfg.device == AUD_STREAM_USE_INT_CODEC &&
               role->ctl.status ==
                   (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE)) {
      *deviceId = id;
      return RAND_STATUS_MIC_OPENED;
    }
  }

  return RAND_STATUS_CLOSE;
}
#endif
