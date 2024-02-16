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
// #include "mbed.h"
#include <assert.h>
#include <stdio.h>

#include "analog.h"
#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_bt_trace.h"
#include "app_overlay.h"
#include "app_utils.h"
#include "audioflinger.h"
#include "avdtp_i.h"
#include "cmsis_os.h"
#include "hal_cmu.h"
#include "hal_overlay.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "lockcqueue.h"
#include "tgt_hardware.h"
#ifdef ANC_APP
#include "app_anc.h"
#endif
#include "anc_wnr.h"
#include "bluetooth.h"
#include "hal_bootmode.h"
#include "hal_codec.h"
#include "hal_i2s.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include "resample_coef.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "app_media_player.h"
#include "resources.h"
#endif
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory_audio.h"
#endif
#ifdef TX_RX_PCM_MASK
#include "hal_chipid.h"
#endif

#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#endif

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
#include "app_ai_if.h"
#include "app_ai_voice.h"
#endif

#ifdef AI_AEC_CP_ACCEL
#include "app_ai_algorithm.h"
#endif

#include "app_a2dp.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "app_ring_merge.h"
#include "audio_dump.h"
#include "audio_process.h"
#include "besbt.h"
#include "bt_drv.h"
#include "bt_drv_reg_op.h"
#include "bt_xtal_sync.h"
#include "btapp.h"
#include "cqueue.h"
#include "hal_chipid.h"
#include "math.h"
#include "os_api.h"

#ifdef WL_DET
#include "app_mic_alg.h"
#endif

#if (A2DP_DECODER_VER == 2)
#include "a2dp_decoder.h"
#endif
#if defined(__AUDIO_SPECTRUM__)
#include "audio_spectrum.h"
#endif

#if defined(BONE_SENSOR_TDM)
#include "lis25ba.h"
#include "speech_utils.h"
#include "tdm_stream.h"
#endif

#if defined(ANC_NOISE_TRACKER)
#include "noise_tracker.h"
#include "noise_tracker_callback.h"
#endif

#if defined(IBRT)
#include "app_ibrt_a2dp.h"
#include "app_ibrt_if.h"
#include "app_ibrt_rssi.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_audio_analysis.h"
#include "app_tws_ibrt_audio_sync.h"
#undef MUSIC_DELAY_CONTROL
#endif

#if defined(AUDIO_ANC_FB_ADJ_MC)
#include "adj_mc.h"
#include "fftfilt2.h"
#endif

// NOTE: Modify parameters for your project.
// #define A2DP_STREAM_AUDIO_DUMP

#if defined(__SW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_sw_iir_index = 0;
extern const IIR_CFG_T *const audio_eq_sw_iir_cfg_list[];
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_fir_index = 0;
extern const FIR_CFG_T *const audio_eq_hw_fir_cfg_list[];
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_dac_iir_index = 0;
extern const IIR_CFG_T *const audio_eq_hw_dac_iir_cfg_list[];
#endif

#include "audio_prompt_sbc.h"

#if defined(__HW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_iir_index = 0;
extern const IIR_CFG_T *const audio_eq_hw_iir_cfg_list[];
#endif

#if defined(HW_DC_FILTER_WITH_IIR)
#include "hw_codec_iir_process.h"
#include "hw_filter_codec_iir.h"

hw_filter_codec_iir_cfg
    POSSIBLY_UNUSED
        adc_iir_cfg = {.bypass = 0,
                       .iir_device = HW_CODEC_IIR_ADC,
#if 1
                       .iir_cfg =
                           {.iir_filtes_l = {.iir_bypass_flag = 0,
                                             .iir_counter = 2,
                                             .iir_coef =
                                                 {
                                                     {{0.994406, -1.988812,
                                                       0.994406},
                                                      {1.000000,
                                                       -1.988781, 0.988843}}, // iir_designer('highpass',
                                                                              // 0, 20, 0.7,
                                                                              // 16000);
                                                     {{4.0, 0.0, 0.0}, {1.0, 0.0, 0.0}},
                                                 }},
                            .iir_filtes_r =
                                {
                                    .iir_bypass_flag = 0,
                                    .iir_counter = 2,
                                    .iir_coef =
                                        {
                                            {{0.994406, -1.988812, 0.994406},
                                             {1.000000, -1.988781, 0.988843}},
                                            {{4.0, 0.0, 0.0}, {1.0, 0.0, 0.0}},
                                        }}}
#else
                       .iir_cfg = {.gain0 = 0,
                                   .gain1 = 0,
                                   .num = 1,
                                   .param =
                                       {
                                           {IIR_TYPE_HIGH_PASS, 0, 20.0, 0.7},
                                       }}
#endif
};

hw_filter_codec_iir_state *hw_filter_codec_iir_st;
#endif

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                       \
    defined(CHIP_BEST3003) || defined(CHIP_BEST1400) ||                        \
    defined(CHIP_BEST1402) || defined(CHIP_BEST1000) ||                        \
    defined(CHIP_BEST2000) || defined(CHIP_BEST3001) || defined(CHIP_BEST2001)

#undef AUDIO_RESAMPLE_ANTI_DITHER

#else
#define AUDIO_RESAMPLE_ANTI_DITHER
#endif

#include "audio_cfg.h"

// #define SCO_DMA_SNAPSHOT_DEBUG

extern uint8_t bt_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,
                                     uint8_t anc_status);
extern uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type, uint8_t index);
extern uint8_t bt_audio_updata_eq_for_anc(uint8_t anc_status);

#include "app_bt_media_manager.h"

#include "hal_location.h"
#include "string.h"

#include "bt_drv_interface.h"

#include "audio_resample_ex.h"

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402) || defined(CHIP_BEST2001)
#define BT_INIT_XTAL_SYNC_FCAP_RANGE (0x1FF)
#else
#define BT_INIT_XTAL_SYNC_FCAP_RANGE (0xFF)
#endif
#define BT_INIT_XTAL_SYNC_MIN (20)
#define BT_INIT_XTAL_SYNC_MAX                                                  \
  (BT_INIT_XTAL_SYNC_FCAP_RANGE - BT_INIT_XTAL_SYNC_MIN)

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#include "anc_process.h"

#ifdef ANC_FB_MC_96KHZ
#define DELAY_SAMPLE_MC (29 * 2)       //  2:ch
#define SAMPLERATE_RATIO_THRESHOLD (4) // 384 = 96*4
#else
#define DELAY_SAMPLE_MC (31 * 2)       //  2:ch
#define SAMPLERATE_RATIO_THRESHOLD (8) // 384 = 48*8
#endif

static int32_t delay_buf_bt[DELAY_SAMPLE_MC];
#endif

#ifdef ANC_APP
static uint8_t anc_status_record = 0xff;
#endif

#if defined(SCO_DMA_SNAPSHOT)

#ifdef PCM_FAST_MODE
#define MASTER_MOBILE_BTCLK_OFFSET (4)
#elif TX_RX_PCM_MASK
#define MASTER_MOBILE_BTCLK_OFFSET (8)
#else
#define MASTER_MOBILE_BTCLK_OFFSET (4)
#endif
#define MASTER_MOBILE_BTCNT_OFFSET (MASTER_MOBILE_BTCLK_OFFSET * 625)

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402) ||                        \
    defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) ||                      \
    defined(CHIP_BEST2001)
#define MUTE_PATTERN (0x55)
#else
#define MUTE_PATTERN (0x00)
#endif

extern void app_tws_ibrt_audio_mobile_clkcnt_get(uint32_t btclk, uint16_t btcnt,
                                                 uint32_t *mobile_master_clk,
                                                 uint16_t *mobile_master_cnt);

static uint8_t *playback_buf_codecpcm;
static uint32_t playback_size_codecpcm;
static uint8_t *capture_buf_codecpcm;
static uint32_t capture_size_codecpcm;

static uint8_t *playback_buf_btpcm;
static uint32_t playback_size_btpcm;
static uint8_t *capture_buf_btpcm;
static uint32_t capture_size_btpcm;

#ifdef TX_RX_PCM_MASK
static uint8_t *playback_buf_btpcm_copy = NULL;
static uint32_t playback_size_btpcm_copy = 0;
static uint8_t *capture_buf_btpcm_copy = NULL;
static uint32_t capture_size_btpcm_copy = 0;
#endif

volatile int sco_btpcm_mute_flag = 0;
volatile int sco_disconnect_mute_flag = 0;

static uint8_t *playback_buf_btpcm_cache = NULL;

static enum AUD_SAMPRATE_T playback_samplerate_codecpcm;
static int32_t mobile_master_clk_offset_init;
#endif

enum PLAYER_OPER_T {
  PLAYER_OPER_START,
  PLAYER_OPER_STOP,
  PLAYER_OPER_RESTART,
};

#if defined(AF_ADC_I2S_SYNC)
extern "C" void hal_codec_capture_enable(void);
extern "C" void hal_codec_capture_enable_delay(void);

static uint32_t codec_capture_cnt = 0;
static uint32_t codec_playback_cnt = 0;
#endif

#if (AUDIO_OUTPUT_VOLUME_DEFAULT < 1) || (AUDIO_OUTPUT_VOLUME_DEFAULT > 17)
#error "AUDIO_OUTPUT_VOLUME_DEFAULT out of range"
#endif
int8_t stream_local_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);
#ifdef AUDIO_LINEIN
int8_t stream_linein_volume = (AUDIO_OUTPUT_VOLUME_DEFAULT);
#endif

struct btdevice_volume *btdevice_volume_p;
struct btdevice_volume current_btdevice_volume;

#ifdef __BT_ANC__
uint8_t bt_sco_samplerate_ratio = 0;
static uint8_t *bt_anc_sco_dec_buf;
extern void us_fir_init(void);
extern uint32_t voicebtpcm_pcm_resample(short *src_samp_buf,
                                        uint32_t src_smpl_cnt,
                                        short *dst_samp_buf);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static enum AUD_BITS_T sample_size_play_bt;
static enum AUD_SAMPRATE_T sample_rate_play_bt;
static uint32_t data_size_play_bt;

static uint8_t *playback_buf_bt;
static uint32_t playback_size_bt;
static int32_t playback_samplerate_ratio_bt;

static uint8_t *playback_buf_mc;
static uint32_t playback_size_mc;
static enum AUD_CHANNEL_NUM_T playback_ch_num_bt;
#ifdef AUDIO_ANC_FB_ADJ_MC
uint32_t adj_mc_capture_sample_rate;
#endif
#endif

#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))
static enum AUD_BITS_T lowdelay_sample_size_play_bt;
static enum AUD_SAMPRATE_T lowdelay_sample_rate_play_bt;
static uint32_t lowdelay_data_size_play_bt;
static enum AUD_CHANNEL_NUM_T lowdelay_playback_ch_num_bt;
#endif

extern uint8_t current_a2dp_non_type;

extern void bt_media_clear_media_type(uint16_t media_type,
                                      enum BT_DEVICE_ID_T device_id);

extern "C" uint8_t is_sbc_mode(void);
uint8_t bt_sbc_mode;
extern "C" uint8_t __attribute__((section(".fast_text_sram")))
is_sbc_mode(void) {
  return bt_sbc_mode;
}

extern "C" uint8_t is_sco_mode(void);

uint8_t bt_sco_mode;
extern "C" uint8_t __attribute__((section(".fast_text_sram")))
is_sco_mode(void) {
  return bt_sco_mode;
}

#define APP_BT_STREAM_TRIGGER_TIMEROUT (2000)

#define TRIGGER_CHECKER_A2DP_PLAYERBLACK (1 << 0)
#define TRIGGER_CHECKER_A2DP_DONE (TRIGGER_CHECKER_A2DP_PLAYERBLACK)

#define TRIGGER_CHECKER_HFP_BTPCM_PLAYERBLACK (1 << 1)
#define TRIGGER_CHECKER_HFP_BTPCM_CAPTURE (1 << 2)
#define TRIGGER_CHECKER_HFP_AUDPCM_PLAYERBLACK (1 << 3)
#define TRIGGER_CHECKER_HFP_AUDPCM_CAPTURE (1 << 4)
#define TRIGGER_CHECKER_HFP_DONE                                               \
  (TRIGGER_CHECKER_HFP_BTPCM_PLAYERBLACK | TRIGGER_CHECKER_HFP_BTPCM_CAPTURE | \
   TRIGGER_CHECKER_HFP_AUDPCM_PLAYERBLACK |                                    \
   TRIGGER_CHECKER_HFP_AUDPCM_CAPTURE)

static bool app_bt_stream_trigger_enable = 0;
static uint32_t app_bt_stream_trigger_checker = 0;
static void app_bt_stream_trigger_timeout_cb(void const *n);
osTimerDef(APP_BT_STREAM_TRIGGER_TIMEOUT, app_bt_stream_trigger_timeout_cb);
osTimerId app_bt_stream_trigger_timeout_id = NULL;

static void app_bt_stream_trigger_timeout_cb(void const *n) {
  TRACE_AUD_STREAM_I("[STRM_TRIG][CHK]timeout_cb\n");
  if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) {
    TRACE_AUD_STREAM_I("[STRM_TRIG][CHK]-->A2DP_SBC\n");
#if defined(IBRT)
    app_ibrt_if_force_audio_retrigger();
#else
    app_audio_sendrequest_param(APP_BT_STREAM_A2DP_SBC,
                                (uint8_t)APP_BT_SETTING_RESTART, 0, 0);
#endif
  } else if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
    TRACE_AUD_STREAM_I("[STRM_TRIG][CHK]-->HFP_PCM\n");
    app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                          (uint8_t)APP_BT_SETTING_RESTART, 0);
  }
}

static int app_bt_stream_trigger_checker_init(void) {
  if (app_bt_stream_trigger_timeout_id == NULL) {
    app_bt_stream_trigger_enable = false;
    app_bt_stream_trigger_checker = 0;
    app_bt_stream_trigger_timeout_id = osTimerCreate(
        osTimer(APP_BT_STREAM_TRIGGER_TIMEOUT), osTimerOnce, NULL);
  }

  return 0;
}

static int app_bt_stream_trigger_checker_start(void) {
  app_bt_stream_trigger_checker = 0;
  app_bt_stream_trigger_enable = true;
  osTimerStart(app_bt_stream_trigger_timeout_id,
               APP_BT_STREAM_TRIGGER_TIMEROUT);
  return 0;
}

static int app_bt_stream_trigger_checker_stop(void) {
  app_bt_stream_trigger_enable = false;
  app_bt_stream_trigger_checker = 0;
  osTimerStop(app_bt_stream_trigger_timeout_id);
  return 0;
}

int app_bt_stream_trigger_checker_handler(uint32_t trigger_checker) {
  bool trigger_ok = false;

  if (app_bt_stream_trigger_enable) {
    app_bt_stream_trigger_checker |= trigger_checker;
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) {
      if (app_bt_stream_trigger_checker == TRIGGER_CHECKER_A2DP_DONE) {
        trigger_ok = true;
      }
    } else if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
      if (app_bt_stream_trigger_checker == TRIGGER_CHECKER_HFP_DONE) {
        trigger_ok = true;
      }
    }
    if (trigger_ok) {
      TRACE_AUD_STREAM_I("[STRM_TRIG][CHK] ok\n");
      app_bt_stream_trigger_checker_stop();
    }
  }
  return 0;
}

#ifdef A2DP_LHDC_ON
extern struct BT_DEVICE_T app_bt_device;
#if defined(A2DP_LHDC_V3)
#define LHDC_AUDIO_96K_BUFF_SIZE (256 * 2 * 4 * 8)
// #define LHDC_AUDIO_96K_16BITS_BUFF_SIZE (256*2*2*8)

#define LHDC_AUDIO_BUFF_SIZE (256 * 2 * 4 * 4)
// #define LHDC_AUDIO_16BITS_BUFF_SIZE     (256*2*2*4)
#define LHDC_LLC_AUDIO_BUFF_SIZE (256 * 2 * 2 * 2)
#else
#define LHDC_AUDIO_BUFF_SIZE (512 * 2 * 4)
// #define LHDC_AUDIO_16BITS_BUFF_SIZE     (512*2*2)
#endif
#endif
uint16_t gStreamplayer = APP_BT_STREAM_INVALID;

uint32_t a2dp_audio_more_data(uint8_t codec_type, uint8_t *buf, uint32_t len);
int a2dp_audio_init(void);
int a2dp_audio_deinit(void);
enum AUD_SAMPRATE_T a2dp_sample_rate = AUD_SAMPRATE_48000;
uint32_t a2dp_data_buf_size;
#ifdef RB_CODEC
extern int app_rbplay_audio_onoff(bool onoff, uint16_t aud_id);
#endif

#if defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)
int app_a2dp_source_linein_on(bool on);
#endif
#if defined(APP_I2S_A2DP_SOURCE)
#include "app_a2dp_source.h"
#include "app_status_ind.h"
// player channel should <= capture channel number
// player must be 2 channel
#define LINEIN_PLAYER_CHANNEL (2)
#ifdef __AUDIO_INPUT_MONO_MODE__
#define LINEIN_CAPTURE_CHANNEL (1)
#else
#define LINEIN_CAPTURE_CHANNEL (2)
#endif

#if (LINEIN_CAPTURE_CHANNEL == 1)
#define LINEIN_PLAYER_BUFFER_SIZE (1024 * LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE / 2)
#elif (LINEIN_CAPTURE_CHANNEL == 2)
#define LINEIN_PLAYER_BUFFER_SIZE (1024 * LINEIN_PLAYER_CHANNEL)
// #define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE)
#define LINEIN_CAPTURE_BUFFER_SIZE (1024 * 10)
#endif

static int16_t *app_linein_play_cache = NULL;

int8_t app_linein_buffer_is_empty(void) {
  if (app_audio_pcmbuff_length()) {
    return 0;
  } else {
    return 1;
  }
}

uint32_t app_linein_pcm_come(uint8_t *pcm_buf, uint32_t len) {
  // DUMP16("%d ", pcm_buf, 10);
  DUMP8("0x%02x ", pcm_buf, 10);
  TRACE_AUD_STREAM_I("app_linein_pcm_come");
  app_audio_pcmbuff_put(pcm_buf, len);

  return len;
}

uint32_t app_linein_need_pcm_data(uint8_t *pcm_buf, uint32_t len) {

#if (LINEIN_CAPTURE_CHANNEL == 1)
  app_audio_pcmbuff_get((uint8_t *)app_linein_play_cache, len / 2);
  // app_play_audio_lineinmode_more_data((uint8_t
  // *)app_linein_play_cache,len/2);
  app_bt_stream_copy_track_one_to_two_16bits(
      (int16_t *)pcm_buf, app_linein_play_cache, len / 2 / 2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
  app_audio_pcmbuff_get((uint8_t *)pcm_buf, len);
  // app_play_audio_lineinmode_more_data((uint8_t *)pcm_buf, len);
#endif

#if defined(__AUDIO_OUTPUT_MONO_MODE__)
  merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)pcm_buf, len / 2);
#endif

#ifdef ANC_APP
  bt_audio_updata_eq_for_anc(app_anc_work_status());
#endif

  audio_process_run(pcm_buf, len);

  return len;
}
extern "C" void pmu_linein_onoff(unsigned char en);
extern "C" int hal_analogif_reg_read(unsigned short reg, unsigned short *val);
int app_a2dp_source_I2S_onoff(bool onoff) {
  static bool isRun = false;
  uint8_t *linein_audio_cap_buff = 0;
  uint8_t *linein_audio_play_buff = 0;
  uint8_t *linein_audio_loop_buf = NULL;
  struct AF_STREAM_CONFIG_T stream_cfg;

  TRACE_AUD_STREAM_I("app_a2dp_source_I2S_onoff work:%d op:%d", isRun, onoff);

  if (isRun == onoff)
    return 0;

  if (onoff) {
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
    app_overlay_select(APP_OVERLAY_A2DP);
    app_audio_mempool_init();
    app_audio_mempool_get_buff(&linein_audio_cap_buff,
                               LINEIN_CAPTURE_BUFFER_SIZE);
    //        app_audio_mempool_get_buff(&linein_audio_play_buff,
    //        LINEIN_PLAYER_BUFFER_SIZE);
    //        app_audio_mempool_get_buff(&linein_audio_loop_buf,
    //        LINEIN_PLAYER_BUFFER_SIZE<<2);
    //        app_audio_pcmbuff_init(linein_audio_loop_buf,
    //        LINEIN_PLAYER_BUFFER_SIZE<<2);

#if (LINEIN_CAPTURE_CHANNEL == 1)
    app_audio_mempool_get_buff((uint8_t **)&app_linein_play_cache,
                               LINEIN_PLAYER_BUFFER_SIZE / 2 / 2);
    // app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL,
    // LINEIN_PLAYER_BUFFER_SIZE/2/2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
    // app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL,
    // LINEIN_PLAYER_BUFFER_SIZE/2);
#endif

    sRet = memset_s(&stream_cfg, sizeof(stream_cfg), 0, sizeof(stream_cfg));
    if (sRet) {
      TRACE_AUD_STREAM_W("func-s line:%d sRet:%d %s ", __LINE__, sRet,
                         __func__);
    }
    stream_cfg.bits = AUD_BITS_16;
    stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_PLAYER_CHANNEL;
    stream_cfg.sample_rate = AUD_SAMPRATE_44100;

#if 0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;

        stream_cfg.vol = 10;//stream_linein_volume;
        //TRACE_AUD_STREAM_I("vol = %d",stream_linein_volume);
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_linein_need_pcm_data;
        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_play_buff);
        stream_cfg.data_size = LINEIN_PLAYER_BUFFER_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif

#if 1
    stream_cfg.device = AUD_STREAM_USE_I2S0_SLAVE;
    //     stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
    //      stream_cfg.handler = app_linein_pcm_come;
    stream_cfg.handler = a2dp_source_linein_more_pcm_data;
    //      stream_cfg.handler = app_linein_pcm_come;
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_cap_buff);
    stream_cfg.data_size = LINEIN_CAPTURE_BUFFER_SIZE; // 2k

    //        pmu_linein_onoff(1);
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

    audio_process_open(stream_cfg.sample_rate, stream_cfg.bits,
                       stream_cfg.channel_num, NULL, 0);

    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#endif
    // app_status_indication_set(APP_STATUS_INDICATION_LINEIN_ON);
  } else {
    //       clear buffer data
    a2dp_source.pcm_queue.write = 0;
    a2dp_source.pcm_queue.len = 0;
    a2dp_source.pcm_queue.read = 0;
    //       pmu_linein_onoff(0);
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    // app_status_indication_set(APP_STATUS_INDICATION_LINEIN_OFF);
    app_overlay_unloadall();
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
  }

  isRun = onoff;
  TRACE_AUD_STREAM_I("%s end!\n", __func__);
  return 0;
}
#endif

enum AUD_SAMPRATE_T bt_parse_sbc_sample_rate(uint8_t sbc_samp_rate) {
  enum AUD_SAMPRATE_T sample_rate;
  sbc_samp_rate = sbc_samp_rate & A2D_STREAM_SAMP_FREQ_MSK;

  switch (sbc_samp_rate) {
  case A2D_SBC_IE_SAMP_FREQ_16:
    //            sample_rate = AUD_SAMPRATE_16000;
    //            break;
  case A2D_SBC_IE_SAMP_FREQ_32:
    //            sample_rate = AUD_SAMPRATE_32000;
    //            break;
  case A2D_SBC_IE_SAMP_FREQ_48:
    sample_rate = AUD_SAMPRATE_48000;
    break;
  case A2D_SBC_IE_SAMP_FREQ_44:
    sample_rate = AUD_SAMPRATE_44100;
    break;
#if defined(A2DP_LHDC_ON) || defined(A2DP_LDAC_ON) || defined(A2DP_SCALABLE_ON)
  case A2D_SBC_IE_SAMP_FREQ_96:
    sample_rate = AUD_SAMPRATE_96000;
    break;
#endif

  default:
    ASSERT(0, "[%s] 0x%x is invalid", __func__, sbc_samp_rate);
    break;
  }
  return sample_rate;
}

void bt_store_sbc_sample_rate(enum AUD_SAMPRATE_T sample_rate) {
  a2dp_sample_rate = sample_rate;
}

enum AUD_SAMPRATE_T bt_get_sbc_sample_rate(void) { return a2dp_sample_rate; }

enum AUD_SAMPRATE_T bt_parse_store_sbc_sample_rate(uint8_t sbc_samp_rate) {
  enum AUD_SAMPRATE_T sample_rate;

  sample_rate = bt_parse_sbc_sample_rate(sbc_samp_rate);
  bt_store_sbc_sample_rate(sample_rate);

  return sample_rate;
}

int bt_sbc_player_setup(uint8_t freq) {
  struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
  static uint8_t sbc_samp_rate = 0xff;
  uint32_t ret;

  if (sbc_samp_rate == freq)
    return 0;

  switch (freq) {
  case A2D_SBC_IE_SAMP_FREQ_16:
  case A2D_SBC_IE_SAMP_FREQ_32:
  case A2D_SBC_IE_SAMP_FREQ_48:
    a2dp_sample_rate = AUD_SAMPRATE_48000;
    break;
#if defined(A2DP_LHDC_ON) || defined(A2DP_LDAC_ON) || defined(A2DP_SCALABLE_ON)
  case A2D_SBC_IE_SAMP_FREQ_96:
    a2dp_sample_rate = AUD_SAMPRATE_96000;
    break;
#if defined(A2DP_LDAC_ON)
  case A2D_SBC_IE_SAMP_FREQ_88:
    a2dp_sample_rate = AUD_SAMPRATE_88200;
    break;
#endif
#endif
  case A2D_SBC_IE_SAMP_FREQ_44:
    a2dp_sample_rate = AUD_SAMPRATE_44100;
    break;
  default:
    break;
  }
  TRACE_AUD_STREAM_I("%s:Sample rate :%d", __func__, a2dp_sample_rate);

  ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg,
                          true);
  if (ret == 0) {
    stream_cfg->sample_rate = a2dp_sample_rate;
    af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
  }

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
  ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg,
                          true);
  if (ret == 0) {
    stream_cfg->sample_rate = a2dp_sample_rate;
    sample_rate_play_bt = stream_cfg->sample_rate;
    af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, stream_cfg);
    anc_mc_run_setup(hal_codec_anc_convert_rate(sample_rate_play_bt));
  }
#endif

  sbc_samp_rate = freq;

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
  if (audio_prompt_is_playing_ongoing()) {
    audio_prompt_forcefully_stop();
  }
#endif

  return 0;
}

void merge_stereo_to_mono_16bits(int16_t *src_buf, int16_t *dst_buf,
                                 uint32_t src_len) {
  uint32_t i = 0;
  for (i = 0; i < src_len; i += 2) {
    dst_buf[i] = (src_buf[i] >> 1) + (src_buf[i + 1] >> 1);
    dst_buf[i + 1] = dst_buf[i];
  }
}

void merge_stereo_to_mono_24bits(int32_t *src_buf, int32_t *dst_buf,
                                 uint32_t src_len) {
  uint32_t i = 0;
  for (i = 0; i < src_len; i += 2) {
    dst_buf[i] = (src_buf[i] >> 1) + (src_buf[i + 1] >> 1);
    dst_buf[i + 1] = dst_buf[i];
  }
}

static char _player_type_str[168];
static char *_catstr(char *dst, const char *src) {
  while (*dst)
    dst++;
  while ((*dst++ = *src++))
    ;
  return --dst;
}
const char *player2str(uint16_t player_type) {
  const char *s = NULL;
  char _cat = 0, first = 1, *d = NULL;
  _player_type_str[0] = '\0';
  d = _player_type_str;
  d = _catstr(d, "[");
  if (player_type != 0) {
    for (int i = 15; i >= 0; i--) {
      _cat = 1;
      // TRACE_AUD_STREAM_I("i=%d,player_type=0x%d,player_type&(1<<i)=0x%x", i,
      // player_type, player_type&(1<<i));
      switch (player_type & (1 << i)) {
      case 0:
        _cat = 0;
        break;
      case APP_BT_STREAM_HFP_PCM:
        s = "HFP_PCM";
        break;
      case APP_BT_STREAM_HFP_CVSD:
        s = "HFP_CVSD";
        break;
      case APP_BT_STREAM_HFP_VENDOR:
        s = "HFP_VENDOR";
        break;
      case APP_BT_STREAM_A2DP_SBC:
        s = "A2DP_SBC";
        break;
      case APP_BT_STREAM_A2DP_AAC:
        s = "A2DP_AAC";
        break;
      case APP_BT_STREAM_A2DP_VENDOR:
        s = "A2DP_VENDOR";
        break;
#ifdef __FACTORY_MODE_SUPPORT__
      case APP_FACTORYMODE_AUDIO_LOOP:
        s = "AUDIO_LOOP";
        break;
#endif
      case APP_PLAY_BACK_AUDIO:
        s = "BACK_AUDIO";
        break;
#ifdef RB_CODEC
      case APP_BT_STREAM_RBCODEC:
        s = "RBCODEC";
        break;
#endif
#ifdef AUDIO_LINEIN
      case APP_PLAY_LINEIN_AUDIO:
        s = "LINEIN_AUDIO";
        break;
#endif
#if defined(APP_LINEIN_A2DP_SOURCE) || defined(__APP_A2DP_SOURCE__) ||         \
    (APP_I2S_A2DP_SOURCE)
      case APP_A2DP_SOURCE_LINEIN_AUDIO:
        s = "SRC_LINEIN_AUDIO";
        break;
      case APP_A2DP_SOURCE_I2S_AUDIO:
        s = "I2S_AUDIO";
        break;
#endif
#ifdef VOICE_DATAPATH
      case APP_BT_STREAM_VOICEPATH:
        s = "VOICEPATH";
        break;
#endif
#ifdef __AI_VOICE__
      case APP_BT_STREAM_AI_VOICE:
        s = "AI_VOICE";
        break;
#endif
#ifdef __THIRDPARTY
      case APP_BT_STREAM_THIRDPARTY_VOICE:
        s = "THIRDPARTY";
        break;
#endif
      default:
        s = "UNKNOWN";
        break;
      }
      if (_cat) {
        if (!first)
          d = _catstr(d, "|");
        // TRACE_AUD_STREAM_I("d=%s,s=%s", d, s);
        d = _catstr(d, s);
        first = 0;
      }
    }
  }

  _catstr(d, "]");

  return _player_type_str;
}

#ifdef __HEAR_THRU_PEAK_DET__
#include "peak_detector.h"
// Depend on codec_dac_vol
const float pkd_vol_multiple[18] = {
    0.281838, 0.000010, 0.005623, 0.007943, 0.011220, 0.015849,
    0.022387, 0.031623, 0.044668, 0.063096, 0.089125, 0.125893,
    0.177828, 0.251189, 0.354813, 0.501187, 0.707946, 1.000000};
int app_bt_stream_local_volume_get(void);
#endif
// extern void a2dp_get_curStream_remDev(btif_remote_device_t   ** p_remDev);
uint16_t a2dp_Get_curr_a2dp_conhdl(void);

#ifdef PLAYBACK_FORCE_48K
static struct APP_RESAMPLE_T *force48k_resample;
static int app_force48k_resample_iter(uint8_t *buf, uint32_t len);
struct APP_RESAMPLE_T *
app_force48k_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
                               APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
                               float ratio_step);
int app_playback_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf,
                              uint32_t len);
#endif

#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))

#define BT_USPERCLK (625)
#define BT_MUTIUSPERSECOND (1000000 / BT_USPERCLK)

#define CALIB_DEVIATION_MS (2)
#define CALIB_FACTOR_MAX_THRESHOLD (0.0001f)
#define CALIB_BT_CLOCK_FACTOR_STEP (0.0000005f)

#define CALIB_FACTOR_DELAY (0.001f)

// bt time
static int32_t bt_old_clock_us = 0;
static uint32_t bt_old_clock_mutius = 0;
static int32_t bt_old_offset_us = 0;

static int32_t bt_clock_us = 0;
static uint32_t bt_clock_total_mutius = 0;
static int32_t bt_total_offset_us = 0;

static int32_t bt_clock_ms = 0;

// local time
static uint32_t local_total_samples = 0;
static uint32_t local_total_frames = 0;

// static uint32_t local_clock_us=0;
static int32_t local_clock_ms = 0;

// bt and local time
static uint32_t bt_local_clock_s = 0;

// calib time
static int32_t calib_total_delay = 0;
static int32_t calib_flag = 0;

// calib factor
static float calib_factor_offset = 0.0f;
static int32_t calib_factor_flag = 0;
static volatile int calib_reset = 1;
#endif

bool process_delay(int32_t delay_ms) {
#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))
  if (delay_ms == 0)
    return 0;

  TRACE_AUD_STREAM_I("delay_ms:%d", delay_ms);

  if (calib_flag == 0) {
    calib_total_delay = calib_total_delay + delay_ms;
    calib_flag = 1;
    return 1;
  } else {
    return 0;
  }
#else
  return 0;
#endif
}

#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))
void a2dp_clock_calib_process(uint32_t len) {
  //    btif_remote_device_t   * p_a2dp_remDev=NULL;
  uint32_t smplcnt = 0;
  int32_t btoffset = 0;

  uint32_t btclk = 0;
  uint32_t btcnt = 0;
  uint32_t btofs = 0;
  btclk = *((volatile uint32_t *)0xd02201fc);
  btcnt = *((volatile uint32_t *)0xd02201f8);
  btcnt = 0;

  // TRACE_AUD_STREAM_I("bt_sbc_player_more_data btclk:%08x,btcnt:%08x\n",
  // btclk, btcnt);

  //  a2dp_get_curStream_remDev(&p_a2dp_remDev);
  if (a2dp_Get_curr_a2dp_conhdl() >= 0x80 &&
      a2dp_Get_curr_a2dp_conhdl() <= 0x82) {
    btofs = btdrv_rf_bitoffset_get(a2dp_Get_curr_a2dp_conhdl() - 0x80);

    if (calib_reset == 1) {
      calib_reset = 0;

      bt_clock_total_mutius = 0;

      bt_old_clock_us = btcnt;
      bt_old_clock_mutius = btclk;

      bt_total_offset_us = 0;

      local_total_samples = 0;
      local_total_frames = 0;
      local_clock_ms = 0;

      bt_local_clock_s = 0;
      bt_clock_us = 0;
      bt_clock_ms = 0;

      bt_old_offset_us = btofs;

      calib_factor_offset = 0.0f;
      calib_factor_flag = 0;
      calib_total_delay = 0;
      calib_flag = 0;
    } else {
      btoffset = btofs - bt_old_offset_us;

      if (btoffset < -BT_USPERCLK / 3) {
        btoffset = btoffset + BT_USPERCLK;
      } else if (btoffset > BT_USPERCLK / 3) {
        btoffset = btoffset - BT_USPERCLK;
      }

      bt_total_offset_us = bt_total_offset_us + btoffset;
      bt_old_offset_us = btofs;

      local_total_frames++;
      if (lowdelay_sample_size_play_bt == AUD_BITS_16) {
        smplcnt = len / (2 * lowdelay_playback_ch_num_bt);
      } else {
        smplcnt = len / (4 * lowdelay_playback_ch_num_bt);
      }

      local_total_samples = local_total_samples + smplcnt;

      bt_clock_us = btcnt - bt_old_clock_us - bt_total_offset_us;

      btoffset = btclk - bt_old_clock_mutius;
      if (btoffset < 0) {
        btoffset = 0;
      }
      bt_clock_total_mutius = bt_clock_total_mutius + btoffset;

      bt_old_clock_us = btcnt;
      bt_old_clock_mutius = btclk;

      if ((bt_clock_total_mutius > BT_MUTIUSPERSECOND) &&
          (local_total_samples > lowdelay_sample_rate_play_bt)) {
        bt_local_clock_s++;
        bt_clock_total_mutius = bt_clock_total_mutius - BT_MUTIUSPERSECOND;
        local_total_samples =
            local_total_samples - lowdelay_sample_rate_play_bt;
      }

      bt_clock_ms =
          (bt_clock_total_mutius * BT_USPERCLK / 1000) + bt_clock_us / 625;
      local_clock_ms =
          (local_total_samples * 1000) / lowdelay_sample_rate_play_bt;

      local_clock_ms = local_clock_ms + calib_total_delay;

      // TRACE_AUD_STREAM_I("A2DP
      // bt_clock_ms:%8d,local_clock_ms:%8d,bt_total_offset_us:%8d\n",bt_clock_ms,
      // local_clock_ms,bt_total_offset_us);

      if (bt_clock_ms > (local_clock_ms + CALIB_DEVIATION_MS)) {
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
        app_resample_tune(a2dp_resample, CALIB_FACTOR_DELAY);
#else
        af_codec_tune(AUD_STREAM_PLAYBACK, CALIB_FACTOR_DELAY);
#endif
        calib_factor_flag = 1;
        // TRACE_AUD_STREAM_I("*************1***************");
      } else if (bt_clock_ms < (local_clock_ms - CALIB_DEVIATION_MS)) {
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
        app_resample_tune(a2dp_resample, -CALIB_FACTOR_DELAY);
#else
        af_codec_tune(AUD_STREAM_PLAYBACK, -CALIB_FACTOR_DELAY);
#endif
        calib_factor_flag = -1;
        // TRACE_AUD_STREAM_I("*************-1***************");
      } else {
        if ((calib_factor_flag == 1 || calib_factor_flag == -1) &&
            (bt_clock_ms == local_clock_ms)) {
          if (calib_factor_offset < CALIB_FACTOR_MAX_THRESHOLD &&
              calib_flag == 0) {
            if (calib_factor_flag == 1) {
              calib_factor_offset =
                  calib_factor_offset + CALIB_BT_CLOCK_FACTOR_STEP;
            } else {
              calib_factor_offset =
                  calib_factor_offset - CALIB_BT_CLOCK_FACTOR_STEP;
            }
          }
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
          app_resample_tune(a2dp_resample, calib_factor_offset);
#else
          af_codec_tune(AUD_STREAM_PLAYBACK, calib_factor_offset);
#endif
          calib_factor_flag = 0;
          calib_flag = 0;
          // TRACE_AUD_STREAM_I("*************0***************");
        }
      }
      //  TRACE_AUD_STREAM_I("factoroffset:%d\n",(int32_t)((factoroffset)*(float)10000000.0f));
    }
  }

  return;
}

#endif

bool app_if_need_fix_target_rxbit(void) {
  return (!bt_drv_is_enhanced_ibrt_rom());
}

static uint8_t isBtPlaybackTriggered = false;

bool bt_is_playback_triggered(void) { return isBtPlaybackTriggered; }

static void bt_set_playback_triggered(bool isEnable) {
  isBtPlaybackTriggered = isEnable;
}

FRAM_TEXT_LOC uint32_t bt_sbc_player_more_data(uint8_t *buf, uint32_t len) {
  app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_A2DP_PLAYERBLACK);
#ifdef A2DP_STREAM_AUDIO_DUMP
  audio_dump_clear_up();
  audio_dump_add_channel_data_from_multi_channels(0, buf, len / sizeof(int) / 2,
                                                  2, 0);
  audio_dump_run();
#endif

  bt_set_playback_triggered(true);

#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))
  a2dp_clock_calib_process(len);
#endif

#if defined(IBRT) && defined(RSSI_GATHERING_ENABLED)
  app_ibrt_ui_rssi_process();
#endif

#ifdef VOICE_DATAPATH
  // if (AI_SPEC_GSOUND == ai_manager_get_current_spec())
  {
    if (app_voicepath_get_stream_pending_state(VOICEPATH_STREAMING)) {
#ifdef __DUAL_MIC_RECORDING__
      app_ibrt_voice_capture_trigger_init();
#endif
      af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#ifdef MIX_MIC_DURING_MUSIC
      app_voicepath_enable_hw_sidetone(0, HW_SIDE_TONE_MAX_ATTENUATION_COEF);
#endif
      app_voicepath_set_stream_state(VOICEPATH_STREAMING, true);
      app_voicepath_set_pending_started_stream(VOICEPATH_STREAMING, false);
    }
  }
#endif
#ifdef BT_XTAL_SYNC
#ifdef BT_XTAL_SYNC_NEW_METHOD
  if (a2dp_Get_curr_a2dp_conhdl() >= 0x80 &&
      a2dp_Get_curr_a2dp_conhdl() <= 0x82) {
    uint32_t bitoffset =
        btdrv_rf_bitoffset_get(a2dp_Get_curr_a2dp_conhdl() - 0x80);
    if (app_if_need_fix_target_rxbit() == false) {
      if (bitoffset < XTAL_OFFSET)
        bitoffset = XTAL_OFFSET;
      else if (bitoffset > SLOT_SIZE - XTAL_OFFSET)
        bitoffset = SLOT_SIZE - XTAL_OFFSET;
    }
#ifdef BT_XTAL_SYNC_SLOW
    bt_xtal_sync_new(bitoffset, app_if_need_fix_target_rxbit(),
                     BT_XTAL_SYNC_MODE_WITH_MOBILE);
#else
    bt_xtal_sync_new_new(bitoffset, app_if_need_fix_target_rxbit(),
                         BT_XTAL_SYNC_MODE_WITH_MOBILE);
#endif
  }

#else
  bt_xtal_sync(BT_XTAL_SYNC_MODE_MUSIC);
#endif
#endif

  uint8_t codec_type = bt_sbc_player_get_codec_type();
  uint32_t overlay_id = 0;
  if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
    overlay_id = APP_OVERLAY_A2DP_AAC;
  } else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
#if defined(A2DP_LHDC_ON)
    if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LHDC) {
      overlay_id = APP_OVERLAY_A2DP_LHDC;
    }
#endif

#if defined(A2DP_LDAC_ON)
    if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LDAC) {
      overlay_id = APP_OVERLAY_A2DP_LDAC;
    }
#endif

#if defined(A2DP_SCALABLE_ON)
    if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_SCALABLE) {
      overlay_id = APP_OVERLAY_A2DP_SCALABLE;
    }
#endif
  } else {
    overlay_id = APP_OVERLAY_A2DP;
  }

  memset(buf, 0, len);

  if (app_get_current_overlay() != overlay_id) {
    return len;
  }

#ifdef PLAYBACK_FORCE_48K
  app_playback_resample_run(force48k_resample, buf, len);
#else
#if (A2DP_DECODER_VER == 2)
  a2dp_audio_playback_handler(buf, len);
#else
  a2dp_audio_more_data(overlay_id, buf, len);
#endif
#endif

#ifdef __AUDIO_SPECTRUM__
  audio_spectrum_run(buf, len);
#endif

#ifdef __KWS_AUDIO_PROCESS__
  short *pdata = (short *)buf;
  short pdata_mono = 0;
  for (unsigned int i = 0; i < len / 4; i++) {
    pdata_mono = pdata[2 * i] / 2 + pdata[2 * i + 1] / 2;
    pdata[2 * i] = pdata_mono;
    pdata[2 * i + 1] = pdata_mono;
  }
#endif

#ifdef __AUDIO_OUTPUT_MONO_MODE__
#ifdef A2DP_EQ_24BIT
  merge_stereo_to_mono_24bits((int32_t *)buf, (int32_t *)buf,
                              len / sizeof(int32_t));
#else
  merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)buf,
                              len / sizeof(int16_t));
#endif
#endif

#ifdef __HEAR_THRU_PEAK_DET__
#ifdef ANC_APP
  if (app_anc_work_status())
#endif
  {
    int vol_level = 0;
    vol_level = app_bt_stream_local_volume_get();
    peak_detector_run(buf, len, pkd_vol_multiple[vol_level]);
  }
#endif

#ifdef ANC_APP
  bt_audio_updata_eq_for_anc(app_anc_work_status());
#endif

  audio_process_run(buf, len);

#if defined(IBRT)
  app_tws_ibrt_audio_analysis_audiohandler_tick();
#endif

  osapi_notify_evm();

  return len;
}

FRAM_TEXT_LOC void
bt_sbc_player_playback_post_handler(uint8_t *buf, uint32_t len, void *cfg) {
  POSSIBLY_UNUSED struct AF_STREAM_CONFIG_T *config =
      (struct AF_STREAM_CONFIG_T *)cfg;

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
#ifdef TWS_PROMPT_SYNC
  tws_playback_ticks_check_for_mix_prompt();
#endif
  if (audio_prompt_is_playing_ongoing()) {
    audio_prompt_processing_handler(len, buf);
  }
#else
  app_ring_merge_more_data(buf, len);
#endif
}

#ifdef __THIRDPARTY
bool start_by_sbc = false;
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
static int32_t mid_p_8_old_l = 0;
static int32_t mid_p_8_old_r = 0;
#ifdef AUDIO_ANC_FB_ADJ_MC
#define ADJ_MC_STREAM_ID AUD_STREAM_ID_1

#define ADJ_MC_SAMPLE_BITS (16)
#define ADJ_MC_SAMPLE_BYTES (ADJ_MC_SAMPLE_BITS / 8)
#define ADJ_MC_CHANNEL_NUM (2)
#define ADJ_MC_FRAME_LEN (256)
#define ADJ_MC_BUF_SIZE                                                        \
  (ADJ_MC_FRAME_LEN * ADJ_MC_CHANNEL_NUM * ADJ_MC_SAMPLE_BYTES * 2) // pingpong
static uint8_t POSSIBLY_UNUSED adj_mc_buf[ADJ_MC_BUF_SIZE];

static uint32_t audio_adj_mc_data_playback_a2dp(uint8_t *buf,
                                                uint32_t mc_len_bytes) {
  uint32_t begin_time;
  // uint32_t end_time;
  begin_time = hal_sys_timer_get();
  TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL] begin_time: %d", begin_time);

  float left_gain;
  float right_gain;
  int playback_len_bytes, mc_len_bytes_run;
  int i, j, k;
  int delay_sample;

  hal_codec_get_dac_gain(&left_gain, &right_gain);

  // TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL]playback_samplerate_ratio:
  // %d",playback_samplerate_ratio);

  // TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL]left_gain:
  // %d",(int)(left_gain*(1<<12)));
  // TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL]right_gain:
  // %d",(int)(right_gain*(1<<12)));

  playback_len_bytes = mc_len_bytes / playback_samplerate_ratio_bt;

  mc_len_bytes_run = mc_len_bytes / SAMPLERATE_RATIO_THRESHOLD;

  if (sample_size_play_bt == AUD_BITS_16) {
    int16_t *sour_p = (int16_t *)(playback_buf_bt + playback_size_bt / 2);
    int16_t *mid_p = (int16_t *)(buf);
    int16_t *mid_p_8 = (int16_t *)(buf + mc_len_bytes - mc_len_bytes_run);
    int16_t *dest_p = (int16_t *)buf;

    if (buf == playback_buf_mc) {
      sour_p = (int16_t *)playback_buf_bt;
    }

    delay_sample = DELAY_SAMPLE_MC / 2;

    for (i = 0, j = 0; i < delay_sample; i = i + 1) {
      mid_p[j++] = delay_buf_bt[i];
    }

    for (i = 0; i < playback_len_bytes / 2 - delay_sample; i = i + 1) {
      mid_p[j++] = sour_p[i];
    }

    for (j = 0; i < playback_len_bytes / 2; i = i + 1) {
      delay_buf_bt[j++] = sour_p[i];
    }

    if (playback_samplerate_ratio_bt <= SAMPLERATE_RATIO_THRESHOLD) {
      for (i = 0, j = 0; i < playback_len_bytes / 2;
           i = i +
               (SAMPLERATE_RATIO_THRESHOLD / playback_samplerate_ratio_bt)) {
        mid_p_8[j++] = mid_p[i];
      }
    } else {
      for (i = 0, j = 0; i < playback_len_bytes / 2; i = i + 1) {
        for (k = 0;
             k < playback_samplerate_ratio_bt / SAMPLERATE_RATIO_THRESHOLD;
             k++) {
          mid_p_8[j++] = mid_p[i];
        }
      }
    }

    anc_adj_mc_run_mono((uint8_t *)mid_p_8, mc_len_bytes_run, AUD_BITS_16);

    for (i = 0, j = 0; i < (mc_len_bytes_run) / 2; i = i + 1) {
      float delta_l =
          (mid_p_8[i] - mid_p_8_old_l) / (float)SAMPLERATE_RATIO_THRESHOLD;
      for (k = 1; k <= SAMPLERATE_RATIO_THRESHOLD; k++) {
        dest_p[j++] = mid_p_8_old_l + (int32_t)(delta_l * k);
      }
      mid_p_8_old_l = mid_p_8[i];
    }

  } else if (sample_size_play_bt == AUD_BITS_24) {
    int32_t *sour_p = (int32_t *)(playback_buf_bt + playback_size_bt / 2);
    int32_t *mid_p = (int32_t *)(buf);
    int32_t *mid_p_8 = (int32_t *)(buf + mc_len_bytes - mc_len_bytes_run);
    int32_t *dest_p = (int32_t *)buf;

    if (buf == (playback_buf_mc)) {
      sour_p = (int32_t *)playback_buf_bt;
    }

    delay_sample = DELAY_SAMPLE_MC / 2;

    for (i = 0, j = 0; i < delay_sample; i = i + 1) {
      mid_p[j++] = delay_buf_bt[i];
    }

    for (i = 0; i < playback_len_bytes / 4 - delay_sample; i = i + 1) {
      mid_p[j++] = sour_p[i];
    }

    for (j = 0; i < playback_len_bytes / 4; i = i + 1) {
      delay_buf_bt[j++] = sour_p[i];
    }

    if (playback_samplerate_ratio_bt <= SAMPLERATE_RATIO_THRESHOLD) {
      for (i = 0, j = 0; i < playback_len_bytes / 4;
           i = i +
               (SAMPLERATE_RATIO_THRESHOLD / playback_samplerate_ratio_bt)) {
        mid_p_8[j++] = mid_p[i];
      }
    } else {
      for (i = 0, j = 0; i < playback_len_bytes / 4; i = i + 1) {
        for (k = 0;
             k < playback_samplerate_ratio_bt / SAMPLERATE_RATIO_THRESHOLD;
             k++) {
          mid_p_8[j++] = mid_p[i];
        }
      }
    }

    anc_adj_mc_run_mono((uint8_t *)mid_p_8, mc_len_bytes_run, AUD_BITS_24);

    for (i = 0, j = 0; i < (mc_len_bytes_run) / 4; i = i + 1) {
      float delta_l =
          (mid_p_8[i] - mid_p_8_old_l) / (float)SAMPLERATE_RATIO_THRESHOLD;
      for (k = 1; k <= SAMPLERATE_RATIO_THRESHOLD; k++) {
        dest_p[j++] = mid_p_8_old_l + (int32_t)(delta_l * k);
      }
      mid_p_8_old_l = mid_p_8[i];
    }
  }

  //  end_time = hal_sys_timer_get();

  //   TRACE_AUD_STREAM_I("[A2DP][MUSIC_CANCEL] run time: %d",
  //   end_time-begin_time);

  return 0;
}
#else
static uint32_t audio_mc_data_playback_a2dp(uint8_t *buf,
                                            uint32_t mc_len_bytes) {
  //    uint32_t begin_time;
  //    uint32_t end_time;
  //    begin_time = hal_sys_timer_get();
  //    TRACE_AUD_STREAM_I("music cancel: %d",begin_time);

  float left_gain;
  float right_gain;
  int playback_len_bytes, mc_len_bytes_run;
  int i, j, k;
  int delay_sample;

  hal_codec_get_dac_gain(&left_gain, &right_gain);

  // TRACE_AUD_STREAM_I("playback_samplerate_ratio:
  // %d",playback_samplerate_ratio);

  // TRACE_AUD_STREAM_I("left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE_AUD_STREAM_I("right_gain: %d",(int)(right_gain*(1<<12)));

  playback_len_bytes = mc_len_bytes / playback_samplerate_ratio_bt;

  mc_len_bytes_run = mc_len_bytes / SAMPLERATE_RATIO_THRESHOLD;

  if (sample_size_play_bt == AUD_BITS_16) {
    int16_t *sour_p = (int16_t *)(playback_buf_bt + playback_size_bt / 2);
    int16_t *mid_p = (int16_t *)(buf);
    int16_t *mid_p_8 = (int16_t *)(buf + mc_len_bytes - mc_len_bytes_run);
    int16_t *dest_p = (int16_t *)buf;

    if (buf == playback_buf_mc) {
      sour_p = (int16_t *)playback_buf_bt;
    }

    delay_sample = DELAY_SAMPLE_MC;

    for (i = 0, j = 0; i < delay_sample; i = i + 2) {
      mid_p[j++] = delay_buf_bt[i];
      mid_p[j++] = delay_buf_bt[i + 1];
    }

    for (i = 0; i < playback_len_bytes / 2 - delay_sample; i = i + 2) {
      mid_p[j++] = sour_p[i];
      mid_p[j++] = sour_p[i + 1];
    }

    for (j = 0; i < playback_len_bytes / 2; i = i + 2) {
      delay_buf_bt[j++] = sour_p[i];
      delay_buf_bt[j++] = sour_p[i + 1];
    }

    if (playback_samplerate_ratio_bt <= SAMPLERATE_RATIO_THRESHOLD) {
      for (i = 0, j = 0; i < playback_len_bytes / 2;
           i = i + 2 * (SAMPLERATE_RATIO_THRESHOLD /
                        playback_samplerate_ratio_bt)) {
        mid_p_8[j++] = mid_p[i];
        mid_p_8[j++] = mid_p[i + 1];
      }
    } else {
      for (i = 0, j = 0; i < playback_len_bytes / 2; i = i + 2) {
        for (k = 0;
             k < playback_samplerate_ratio_bt / SAMPLERATE_RATIO_THRESHOLD;
             k++) {
          mid_p_8[j++] = mid_p[i];
          mid_p_8[j++] = mid_p[i + 1];
        }
      }
    }

    anc_mc_run_stereo((uint8_t *)mid_p_8, mc_len_bytes_run, left_gain,
                      right_gain, AUD_BITS_16);

    for (i = 0, j = 0; i < (mc_len_bytes_run) / 2; i = i + 2) {
      float delta_l =
          (mid_p_8[i] - mid_p_8_old_l) / (float)SAMPLERATE_RATIO_THRESHOLD;
      float delta_r =
          (mid_p_8[i + 1] - mid_p_8_old_r) / (float)SAMPLERATE_RATIO_THRESHOLD;
      for (k = 1; k <= SAMPLERATE_RATIO_THRESHOLD; k++) {
        dest_p[j++] = mid_p_8_old_l + (int32_t)(delta_l * k);
        dest_p[j++] = mid_p_8_old_r + (int32_t)(delta_r * k);
      }
      mid_p_8_old_l = mid_p_8[i];
      mid_p_8_old_r = mid_p_8[i + 1];
    }

  } else if (sample_size_play_bt == AUD_BITS_24) {
    int32_t *sour_p = (int32_t *)(playback_buf_bt + playback_size_bt / 2);
    int32_t *mid_p = (int32_t *)(buf);
    int32_t *mid_p_8 = (int32_t *)(buf + mc_len_bytes - mc_len_bytes_run);
    int32_t *dest_p = (int32_t *)buf;

    if (buf == (playback_buf_mc)) {
      sour_p = (int32_t *)playback_buf_bt;
    }

    delay_sample = DELAY_SAMPLE_MC;

    for (i = 0, j = 0; i < delay_sample; i = i + 2) {
      mid_p[j++] = delay_buf_bt[i];
      mid_p[j++] = delay_buf_bt[i + 1];
    }

    for (i = 0; i < playback_len_bytes / 4 - delay_sample; i = i + 2) {
      mid_p[j++] = sour_p[i];
      mid_p[j++] = sour_p[i + 1];
    }

    for (j = 0; i < playback_len_bytes / 4; i = i + 2) {
      delay_buf_bt[j++] = sour_p[i];
      delay_buf_bt[j++] = sour_p[i + 1];
    }

    if (playback_samplerate_ratio_bt <= SAMPLERATE_RATIO_THRESHOLD) {
      for (i = 0, j = 0; i < playback_len_bytes / 4;
           i = i + 2 * (SAMPLERATE_RATIO_THRESHOLD /
                        playback_samplerate_ratio_bt)) {
        mid_p_8[j++] = mid_p[i];
        mid_p_8[j++] = mid_p[i + 1];
      }
    } else {
      for (i = 0, j = 0; i < playback_len_bytes / 4; i = i + 2) {
        for (k = 0;
             k < playback_samplerate_ratio_bt / SAMPLERATE_RATIO_THRESHOLD;
             k++) {
          mid_p_8[j++] = mid_p[i];
          mid_p_8[j++] = mid_p[i + 1];
        }
      }
    }

    anc_mc_run_stereo((uint8_t *)mid_p_8, mc_len_bytes_run, left_gain,
                      right_gain, AUD_BITS_24);

    for (i = 0, j = 0; i < (mc_len_bytes_run) / 4; i = i + 2) {
      float delta_l =
          (mid_p_8[i] - mid_p_8_old_l) / (float)SAMPLERATE_RATIO_THRESHOLD;
      float delta_r =
          (mid_p_8[i + 1] - mid_p_8_old_r) / (float)SAMPLERATE_RATIO_THRESHOLD;
      for (k = 1; k <= SAMPLERATE_RATIO_THRESHOLD; k++) {
        dest_p[j++] = mid_p_8_old_l + (int32_t)(delta_l * k);
        dest_p[j++] = mid_p_8_old_r + (int32_t)(delta_r * k);
      }
      mid_p_8_old_l = mid_p_8[i];
      mid_p_8_old_r = mid_p_8[i + 1];
    }
  }

  //  end_time = hal_sys_timer_get();

  //   TRACE_AUD_STREAM_I("%s:run time: %d", __FUNCTION__, end_time-begin_time);

  return 0;
}
#endif
#endif

static uint8_t g_current_eq_index = 0xff;
static bool isMeridianEQON = false;

bool app_is_meridian_on() { return isMeridianEQON; }

uint8_t app_audio_get_eq() { return g_current_eq_index; }

bool app_meridian_eq(bool onoff) {
  isMeridianEQON = onoff;
  return onoff;
}

int app_audio_set_eq(uint8_t index) {
#ifdef __SW_IIR_EQ_PROCESS__
  if (index >= EQ_SW_IIR_LIST_NUM)
    return -1;
#endif
#ifdef __HW_FIR_EQ_PROCESS__
  if (index >= EQ_HW_FIR_LIST_NUM)
    return -1;
#endif
#ifdef __HW_DAC_IIR_EQ_PROCESS__
  if (index >= EQ_HW_DAC_IIR_LIST_NUM)
    return -1;
#endif
#ifdef __HW_IIR_EQ_PROCESS__
  if (index >= EQ_HW_IIR_LIST_NUM)
    return -1;
#endif
  g_current_eq_index = index;
  return index;
}

void bt_audio_updata_eq(uint8_t index) {
  TRACE_AUD_STREAM_I("[EQ] update idx = %d", index);
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) ||        \
    defined(__HW_DAC_IIR_EQ_PROCESS__) || defined(__HW_IIR_EQ_PROCESS__)
  AUDIO_EQ_TYPE_T audio_eq_type;
#ifdef __SW_IIR_EQ_PROCESS__
  audio_eq_type = AUDIO_EQ_TYPE_SW_IIR;
#endif

#ifdef __HW_FIR_EQ_PROCESS__
  audio_eq_type = AUDIO_EQ_TYPE_HW_FIR;
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
  audio_eq_type = AUDIO_EQ_TYPE_HW_DAC_IIR;
#endif

#ifdef __HW_IIR_EQ_PROCESS__
  audio_eq_type = AUDIO_EQ_TYPE_HW_IIR;
#endif
  bt_audio_set_eq(audio_eq_type, index);
#endif
}

#ifdef ANC_APP
uint8_t bt_audio_updata_eq_for_anc(uint8_t anc_status) {
  anc_status = app_anc_work_status();
  if (anc_status_record != anc_status) {
    anc_status_record = anc_status;
    TRACE_AUD_STREAM_I("[EQ] update anc_status = %d", anc_status);
#ifdef __SW_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR, anc_status));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR, anc_status));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    bt_audio_set_eq(
        AUDIO_EQ_TYPE_HW_DAC_IIR,
        bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR, anc_status));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR, anc_status));
#endif
  }
  return 0;
}
#endif

uint8_t bt_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,
                              uint8_t anc_status) {
  uint8_t index_eq = 0;

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) ||        \
    defined(__HW_DAC_IIR_EQ_PROCESS__) || defined(__HW_IIR_EQ_PROCESS__)
  switch (audio_eq_type) {
#if defined(__SW_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_SW_IIR: {
    if (anc_status) {
      index_eq = audio_eq_sw_iir_index + 1;
    } else {
      index_eq = audio_eq_sw_iir_index;
    }

  } break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_FIR: {
    if (a2dp_sample_rate == AUD_SAMPRATE_44100) {
      index_eq = 0;
    } else if (a2dp_sample_rate == AUD_SAMPRATE_48000) {
      index_eq = 1;
    } else if (a2dp_sample_rate == AUD_SAMPRATE_96000) {
      index_eq = 2;
    } else {
      ASSERT(0, "[%s] sample_rate_recv(%d) is not supported", __func__,
             a2dp_sample_rate);
    }
    audio_eq_hw_fir_index = index_eq;

    if (anc_status) {
      index_eq = index_eq + 3;
    }
  } break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_DAC_IIR: {
    if (anc_status) {
      index_eq = audio_eq_hw_dac_iir_index + 1;
    } else {
      index_eq = audio_eq_hw_dac_iir_index;
    }
  } break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_IIR: {
    if (anc_status) {
      index_eq = audio_eq_hw_iir_index + 1;
    } else {
      index_eq = audio_eq_hw_iir_index;
    }
  } break;
#endif
  default: {
    ASSERT(false, "[%s]Error eq type!", __func__);
  }
  }
#endif
  return index_eq;
}

uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type, uint8_t index) {
  const FIR_CFG_T *fir_cfg = NULL;
  const IIR_CFG_T *iir_cfg = NULL;

  TRACE_AUD_STREAM_I("[EQ] set type=%d,index=%d", audio_eq_type, index);

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) ||        \
    defined(__HW_DAC_IIR_EQ_PROCESS__) || defined(__HW_IIR_EQ_PROCESS__)
  switch (audio_eq_type) {
#if defined(__SW_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_SW_IIR: {
    if (index >= EQ_SW_IIR_LIST_NUM) {
      TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_SW_IIR_LIST_NUM", index);
      return 1;
    }

    iir_cfg = audio_eq_sw_iir_cfg_list[index];
  } break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_FIR: {
    if (index >= EQ_HW_FIR_LIST_NUM) {
      TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_HW_FIR_LIST_NUM", index);
      return 1;
    }

    fir_cfg = audio_eq_hw_fir_cfg_list[index];
  } break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_DAC_IIR: {
    if (index >= EQ_HW_DAC_IIR_LIST_NUM) {
      TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_HW_DAC_IIR_LIST_NUM", index);
      return 1;
    }

    iir_cfg = audio_eq_hw_dac_iir_cfg_list[index];
  } break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_IIR: {
    if (index >= EQ_HW_IIR_LIST_NUM) {
      TRACE_AUD_STREAM_W("[EQ] SET index %u > EQ_HW_IIR_LIST_NUM", index);
      return 1;
    }

    iir_cfg = audio_eq_hw_iir_cfg_list[index];
  } break;
#endif
  default: {
    ASSERT(false, "[%s]Error eq type!", __func__);
  }
  }
#endif

#ifdef AUDIO_SECTION_ENABLE
  const IIR_CFG_T *iir_cfg_from_audio_section =
      (const IIR_CFG_T *)load_audio_cfg_from_audio_section(
          AUDIO_PROCESS_TYPE_IIR_EQ);
  if (iir_cfg_from_audio_section) {
    iir_cfg = iir_cfg_from_audio_section;
  }
#endif

  return audio_eq_set_cfg(fir_cfg, iir_cfg, audio_eq_type);
}

#define A2DP_PLAYER_PLAYBACK_WATER_LINE                                        \
  ((uint32_t)(3.f * a2dp_audio_latency_factor_get() + 0.5f))
#define A2DP_PLAYER_PLAYBACK_WATER_LINE_UPPER (25)

/********************************
        AUD_BITS_16
        dma_buffer_delay_us =
 stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
        AUD_BITS_24
        dma_buffer_delay_us =
 stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

        dma_buffer_delay_us
        scalable delay = 864/sample*1000*n ms
        scalable delay = 864/44100*1000*13 = 117ms
        scalable delay = 864/96000*1000*6 = 118ms
        waterline delay = 864/sample*1000*n ms
        waterline delay = 864/44100*1000*3 = 58ms
        waterline delay = 864/96000*1000*3 = 27ms
        audio_delay = scalable delay + waterline delay
 *********************************/

#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU (13)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_BASE (9000)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_US                           \
  (A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_BASE *                            \
   A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU)

#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU (6)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_BASE (19500)
#define A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_US                         \
  (A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_BASE *                          \
   A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU)

/********************************
        AUD_BITS_16
        dma_buffer_delay_us =
 stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
        AUD_BITS_24
        dma_buffer_delay_us =
 stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

        dma_buffer_delay_us
        aac delay = 1024/sample*1000*n ms
        aac delay = 1024/44100*1000*5 = 116ms
        waterline delay = 1024/sample*1000*n ms
        waterline delay = 1024/44100*1000*3 = 69ms
        audio_delay = aac delay + waterline delay
 *********************************/
#ifndef A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU (3)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU (6)
#endif
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_BASE (23000)
#define A2DP_PLAYER_PLAYBACK_DELAY_AAC_US                                      \
  (A2DP_PLAYER_PLAYBACK_DELAY_AAC_BASE * A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU)

/********************************
    AUD_BITS_16
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    sbc delay = 128/sample*n ms
    sbc delay = 128/44100*45 = 130ms
    sbc_delay = sbc delay(23219us)
    waterline delay = 128/sample*SBC_FRAME_MTU*n ms
    waterline delay = 128/44100*5*3 = 43ms
    audio_delay = aac delay + waterline delay
*********************************/
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_FRAME_MTU (5)
#ifndef A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU (35)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU (50)
#endif
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_BASE (2800)
#define A2DP_PLAYER_PLAYBACK_DELAY_SBC_US                                      \
  (A2DP_PLAYER_PLAYBACK_DELAY_SBC_BASE * A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU)

#if defined(A2DP_LHDC_ON)
/********************************
    AUD_BITS_16
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    lhdc delay = 512/sample*1000*n ms
    lhdc delay =    *28 = 149ms
    audio_delay = lhdc delay

    lhdc_v2 delay = 512/96000*1000*38 =  202ms
    lhdc_v3 delay = 256/96000*1000*58 = 154ms
    audio_delay = lhdc_v3 delay
*********************************/
#if defined(IBRT)
#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU (68)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU (38)
#endif
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU (28)
#endif

#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE (2666)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE (5333)
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_US                               \
  (A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU *                                 \
   A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE)

/********************************
    AUD_BITS_16
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    lhdc delay = 512/sample*1000*n ms
    lhdc_v2 delay = 512/48000*1000*14 = 149ms
    lhdc_v3 delay = 256/48000*1000*28 = 149ms
    audio_delay = lhdc delay
*********************************/
#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU (38)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU (14)
#endif

#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE (5333)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE (10666)
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_US                             \
  (A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU *                               \
   A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE)

/********************************
    AUD_BITS_16
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    lhdc delay = 512/sample*1000*n ms
    lhdc_v2 delay = 512/48000*1000*9 = 96ms
    lhdc_v3 delay = 256/48000*1000*19 = 101ms
    audio_delay = lhdc delay
*********************************/
#if defined(IBRT)
#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU (15)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU (9)
#endif
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU (6)
#endif

#if defined(A2DP_LHDC_V3)
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE (5333)
#else
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE (10666)
#endif
#define A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_US                                 \
  (A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU *                                   \
   A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE)
#endif

#if defined(A2DP_LDAC_ON)
/********************************
    AUD_BITS_16
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/2*1000000LL/stream_cfg->sample_rate;
    AUD_BITS_24
    dma_buffer_delay_us =
stream_cfg->data_size/stream_cfg->channel_num/4*1000000LL/stream_cfg->sample_rate;

    ldac delay = 256/sample*1000*n ms
    ldac delay = 256/96000*1000*56 = 149ms
    audio_delay = ldac delay
*********************************/
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_FRAME_MTU (5)
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU (60)
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE (2667)
#define A2DP_PLAYER_PLAYBACK_DELAY_LDAC_US                                     \
  (A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE * A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU)
#endif

enum BT_STREAM_TRIGGER_STATUS_T {
  BT_STREAM_TRIGGER_STATUS_NULL = 0,
  BT_STREAM_TRIGGER_STATUS_INIT,
  BT_STREAM_TRIGGER_STATUS_WAIT,
  BT_STREAM_TRIGGER_STATUS_OK,
};

static uint32_t tg_acl_trigger_time = 0;
static uint32_t tg_acl_trigger_start_time = 0;
static uint32_t tg_acl_trigger_init_time = 0;
static enum BT_STREAM_TRIGGER_STATUS_T bt_stream_trigger_status =
    BT_STREAM_TRIGGER_STATUS_NULL;

void app_bt_stream_playback_irq_notification(enum AUD_STREAM_ID_T id,
                                             enum AUD_STREAM_T stream);

inline void
app_bt_stream_trigger_stauts_set(enum BT_STREAM_TRIGGER_STATUS_T stauts) {
  TRACE_AUD_STREAM_I("[STRM_TRIG] stauts_set %d->%d", bt_stream_trigger_status,
                     stauts);
  bt_stream_trigger_status = stauts;
}

inline enum BT_STREAM_TRIGGER_STATUS_T app_bt_stream_trigger_stauts_get(void) {
  TRACE_AUD_STREAM_I("[STRM_TRIG] stauts_get:%d", bt_stream_trigger_status);
  return bt_stream_trigger_status;
}

uint32_t app_bt_stream_get_dma_buffer_delay_us(void) {
  uint32_t dma_buffer_delay_us = 0;
  struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
  if (!af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg,
                         false)) {
    if (stream_cfg->bits <= AUD_BITS_16) {
      dma_buffer_delay_us = stream_cfg->data_size / stream_cfg->channel_num /
                            2 * 1000000LL / stream_cfg->sample_rate;
    } else {
      dma_buffer_delay_us = stream_cfg->data_size / stream_cfg->channel_num /
                            4 * 1000000LL / stream_cfg->sample_rate;
    }
  }
  return dma_buffer_delay_us;
}

uint32_t app_bt_stream_get_dma_buffer_samples(void) {
  uint32_t dma_buffer_delay_samples = 0;
  struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
  if (!af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg,
                         false)) {
    if (stream_cfg->bits <= AUD_BITS_16) {
      dma_buffer_delay_samples =
          stream_cfg->data_size / stream_cfg->channel_num / 2;
    } else {
      dma_buffer_delay_samples =
          stream_cfg->data_size / stream_cfg->channel_num / 4;
    }
  }
  return dma_buffer_delay_samples;
}

#if defined(IBRT)
typedef enum {
  APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE,
  APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_ONPROCESS,
  APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_SYNCOK,
} APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE;

#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_CNT_LIMIT (100)
void app_bt_stream_ibrt_set_trigger_time(
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger);
void app_bt_stream_ibrt_auto_synchronize_initsync_start(
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger);
static APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T
    app_bt_stream_ibrt_auto_synchronize_trigger;
static uint32_t app_bt_stream_ibrt_auto_synchronize_cnt = 0;
int app_bt_stream_ibrt_audio_mismatch_stopaudio(void);
void app_bt_stream_ibrt_auto_synchronize_hungup(void);
void app_bt_stream_ibrt_auto_synchronize_stop(void);
static APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE ibrt_auto_synchronize_status =
    APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE;

int app_bt_stream_ibrt_auto_synchronize_status_set(
    APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE status) {
  TRACE_AUD_STREAM_I("[AUTO_SYNC] status:%d", status);
  ibrt_auto_synchronize_status = status;
  return 0;
}

APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE
app_bt_stream_ibrt_auto_synchronize_status_get(void) {
  TRACE_AUD_STREAM_I("[AUTO_SYNC] status:%d", ibrt_auto_synchronize_status);
  return ibrt_auto_synchronize_status;
}

int app_bt_stream_ibrt_auto_synchronize_trigger_start(
    btif_media_header_t *header, unsigned char *buf, unsigned int len) {
  APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger =
      &app_bt_stream_ibrt_auto_synchronize_trigger;
  app_bt_stream_ibrt_auto_synchronize_stop();
  TRACE_AUD_STREAM_I(
      "[AUTO_SYNC] trigger:%d Seq:%d timestamp:%d SubSeq:%d/%d currSeq:%d",
      sync_trigger->trigger_time, sync_trigger->audio_info.sequenceNumber,
      sync_trigger->audio_info.timestamp,
      sync_trigger->audio_info.curSubSequenceNumber,
      sync_trigger->audio_info.totalSubSequenceNumber, header->sequenceNumber);

  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (app_tws_ibrt_slave_ibrt_link_connected()) {
    if (sync_trigger->trigger_time >=
        bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle)) {
      app_bt_stream_ibrt_set_trigger_time(sync_trigger);
    } else {
      TRACE_AUD_STREAM_W("[AUTO_SYNC]failed trigger(%d)-->tg(%d) need resume",
                         bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle),
                         sync_trigger->trigger_time);
      app_ibrt_if_force_audio_retrigger();
      // app_tws_ibrt_audio_sync_mismatch_resume_notify();
    }
  } else {
    TRACE_AUD_STREAM_I("[AUTO_SYNC] ok but currRole:%d mismatch\n",
                       p_ibrt_ctrl->current_role);
    app_ibrt_if_force_audio_retrigger();
  }

  return 0;
}

int app_bt_stream_ibrt_auto_synchronize_dataind_cb(btif_media_header_t *header,
                                                   unsigned char *buf,
                                                   unsigned int len) {
  APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger =
      &app_bt_stream_ibrt_auto_synchronize_trigger;
  bool synchronize_ok = false;
  int32_t timestamp_diff = 0;
  int32_t dma_buffer_samples = 0;
  int32_t frame_totle_samples = 0;

  frame_totle_samples = sync_trigger->audio_info.totalSubSequenceNumber *
                        sync_trigger->audio_info.frame_samples;
  timestamp_diff = sync_trigger->audio_info.timestamp - header->timestamp;

  TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] seq:%d/%d timestamp:%d/%d cnt:%d",
                     header->sequenceNumber,
                     sync_trigger->audio_info.sequenceNumber, header->timestamp,
                     sync_trigger->audio_info.timestamp,
                     app_bt_stream_ibrt_auto_synchronize_cnt);

  if (++app_bt_stream_ibrt_auto_synchronize_cnt >
      APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_CNT_LIMIT) {
    app_bt_stream_ibrt_auto_synchronize_stop();
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNC][DATAIND] SYNCHRONIZE_CNT_LIMIT, we need force retrigger");
    app_ibrt_if_force_audio_retrigger();
  } else if (app_tws_ibrt_mobile_link_connected()) {
    app_bt_stream_ibrt_auto_synchronize_stop();
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNC][DATAIND] find role to master, we need force retrigger");
    app_ibrt_if_force_audio_retrigger();
  } else if (sync_trigger->audio_info.sequenceNumber < header->sequenceNumber) {
    app_bt_stream_ibrt_auto_synchronize_stop();
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNC][DATAIND] seq timestamp:%d/%d mismatch need resume",
        header->timestamp, sync_trigger->audio_info.timestamp);
    app_ibrt_if_force_audio_retrigger();
  } else {
    if (header->sequenceNumber >= sync_trigger->audio_info.sequenceNumber &&
        !sync_trigger->audio_info.totalSubSequenceNumber) {
      synchronize_ok = true;
    } else if (header->timestamp == sync_trigger->audio_info.timestamp) {
      synchronize_ok = true;
    }

    dma_buffer_samples = app_bt_stream_get_dma_buffer_samples() / 2;

    if (sync_trigger->audio_info.timestamp >= header->timestamp &&
        sync_trigger->audio_info.totalSubSequenceNumber) {
      if (timestamp_diff < dma_buffer_samples) {
        TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] timestamp_diff < "
                           "dma_buffer_samples synchronize ok");
        synchronize_ok = true;
      } else if (timestamp_diff < frame_totle_samples) {
        TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] timestamp_diff < "
                           "frame_totle_samples synchronize ok");
        synchronize_ok = true;
      }
    }

    if (!synchronize_ok &&
        header->sequenceNumber >= sync_trigger->audio_info.sequenceNumber) {

      TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND] timestamp %d vs %d",
                         sync_trigger->audio_info.timestamp - header->timestamp,
                         frame_totle_samples);
      if ((sync_trigger->audio_info.timestamp - header->timestamp) <=
          (uint32_t)(frame_totle_samples * 3)) {
        sync_trigger->audio_info.sequenceNumber++;
        TRACE_AUD_STREAM_I(
            "[AUTO_SYNC][DATAIND] timestamp try sequenceNumber:%d",
            header->sequenceNumber);
      }
    }

    // flush all
    a2dp_audio_synchronize_dest_packet_mut(0);

    if (synchronize_ok) {
      A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
      if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0) {
        TRACE_AUD_STREAM_I(
            "[AUTO_SYNC][DATAIND]synchronize ok but lastframe error");
        goto exit;
      }

      TRACE_AUD_STREAM_I("[AUTO_SYNC][DATAIND]synchronize ok timestamp_diff:%d "
                         "frame_samples:%d",
                         timestamp_diff, lastframe_info.frame_samples);
      sync_trigger->trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_LOCAL;
      sync_trigger->audio_info.sequenceNumber = header->sequenceNumber;
      sync_trigger->audio_info.timestamp = header->timestamp;
      if (sync_trigger->audio_info.totalSubSequenceNumber) {
        sync_trigger->audio_info.curSubSequenceNumber =
            timestamp_diff / lastframe_info.frame_samples;
        TRACE_AUD_STREAM_I(
            "[AUTO_SYNC][DATAIND]synchronize ok tstmp_diff:%d/%d SubSeq:%d",
            timestamp_diff, sync_trigger->audio_info.frame_samples,
            sync_trigger->audio_info.curSubSequenceNumber);
      } else {
        sync_trigger->audio_info.curSubSequenceNumber = 0;
      }
      sync_trigger->audio_info.totalSubSequenceNumber =
          lastframe_info.totalSubSequenceNumber;
      sync_trigger->audio_info.frame_samples = lastframe_info.frame_samples;
      if (sync_trigger->audio_info.totalSubSequenceNumber &&
          sync_trigger->audio_info.curSubSequenceNumber >=
              sync_trigger->audio_info.totalSubSequenceNumber) {
        TRACE_AUD_STREAM_W("[AUTO_SYNC][DATAIND]synchronize ok but sbc & "
                           "timestamp is ms so force trigger");
        app_bt_stream_ibrt_auto_synchronize_stop();
        app_ibrt_if_force_audio_retrigger();
      } else {
        a2dp_audio_detect_store_packet_callback_register(
            app_bt_stream_ibrt_auto_synchronize_trigger_start);
      }
    } else {
      a2dp_audio_detect_first_packet();
    }
  }
exit:
  return 0;
}

void app_bt_stream_ibrt_auto_synchronize_start(
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger) {
  TRACE_AUD_STREAM_I(
      "[AUTO_SYNC][DATAIND] trigger_time:%d seq:%d timestamp:%d SubSeq:%d/%d",
      sync_trigger->trigger_time, sync_trigger->audio_info.sequenceNumber,
      sync_trigger->audio_info.timestamp,
      sync_trigger->audio_info.curSubSequenceNumber,
      sync_trigger->audio_info.totalSubSequenceNumber);
  app_bt_stream_ibrt_auto_synchronize_status_set(
      APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_ONPROCESS);
  app_bt_stream_ibrt_auto_synchronize_cnt = 0;
  app_bt_stream_ibrt_auto_synchronize_trigger = *sync_trigger;
  a2dp_audio_detect_next_packet_callback_register(
      app_bt_stream_ibrt_auto_synchronize_dataind_cb);
  a2dp_audio_detect_first_packet();
}

void app_bt_stream_ibrt_auto_synchronize_hungup(void) {
  a2dp_audio_detect_next_packet_callback_register(NULL);
  a2dp_audio_detect_store_packet_callback_register(NULL);
}

void app_bt_stream_ibrt_auto_synchronize_stop(void) {
  app_bt_stream_ibrt_auto_synchronize_hungup();
  app_bt_stream_ibrt_auto_synchronize_cnt = 0;
  app_bt_stream_ibrt_auto_synchronize_status_set(
      APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE);
}

bool app_bt_stream_ibrt_auto_synchronize_on_porcess(void) {
  bool nRet = true;
  APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_TYPE synchronize_status =
      app_bt_stream_ibrt_auto_synchronize_status_get();
  if (synchronize_status == APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_IDLE) {
    nRet = false;
  }
  return nRet;
}

void app_bt_stream_ibrt_start_sbc_player_callback(uint32_t status,
                                                  uint32_t param) {
  if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) {
    TRACE_AUD_STREAM_I(
        "start_sbc_player_cb trigger(%d)-->tg(%d)", param,
        ((APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *)(uintptr_t)param)->trigger_time);
    app_bt_stream_ibrt_set_trigger_time(
        (APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *)(uintptr_t)param);
  } else {
    TRACE_AUD_STREAM_I("start_sbc_player_cb try again");
    app_audio_manager_sendrequest_need_callback(
        APP_BT_STREAM_MANAGER_START, BT_STREAM_SBC, BT_DEVICE_ID_1,
        MAX_RECORD_NUM,
        (uint32_t)(uintptr_t)app_bt_stream_ibrt_start_sbc_player_callback,
        (uint32_t)param);
  }
}

int app_bt_stream_ibrt_start_sbc_player(
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger) {
  TRACE_AUD_STREAM_I("start_sbc_player tg(%d)", sync_trigger->trigger_time);
  app_audio_manager_sendrequest_need_callback(
      APP_BT_STREAM_MANAGER_START, BT_STREAM_SBC, BT_DEVICE_ID_1,
      MAX_RECORD_NUM,
      (uint32_t)(uintptr_t)app_bt_stream_ibrt_start_sbc_player_callback,
      (uint32_t)(uintptr_t)sync_trigger);
  return 0;
}

uint16_t app_bt_stream_ibrt_trigger_seq_diff_calc(int32_t dma_samples,
                                                  int32_t frame_samples,
                                                  int32_t total_subseq,
                                                  int32_t interval) {
  float seq_factor = 1.0f;
  if (total_subseq) {
    seq_factor = (float)(dma_samples / frame_samples) / (float)total_subseq;
  } else {
    seq_factor = (float)(dma_samples / frame_samples);
  }
  return (uint16_t)(seq_factor * (float)interval);
}

#define MOBILE_LINK_PLAYBACK_INFO_TRIG_DUMMY_DMA_CNT (5)
#define SYNCHRONIZE_DATAIND_CNT_LIMIT (25)

static int synchronize_need_discards_dma_cnt = 0;
static int synchronize_dataind_cnt = 0;

int app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb_v2(
    btif_media_header_t *header, unsigned char *buf, unsigned int len) {
  bool synchronize_ok = false;
  bool discards_samples_finished = false;
  int dest_discards_samples = 0;
  uint32_t list_samples = 0;
  uint32_t curr_ticks = 0;
  A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger =
      &app_bt_stream_ibrt_auto_synchronize_trigger;

  synchronize_dataind_cnt++;
  if (app_tws_ibrt_mobile_link_connected()) {
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNCV2] mobile_link is connect retrigger because role switch");
    app_ibrt_if_force_audio_retrigger();
    return 0;
  }

  if (synchronize_dataind_cnt >= SYNCHRONIZE_DATAIND_CNT_LIMIT) {
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNCV2] mobile_link is connect retrigger because CNT_LIMIT");
    app_ibrt_if_force_audio_retrigger();
    return 0;
  }

  dest_discards_samples = app_bt_stream_get_dma_buffer_samples() / 2 *
                          synchronize_need_discards_dma_cnt;
  a2dp_audio_convert_list_to_samples(&list_samples);
  if ((int)list_samples > dest_discards_samples) {
    discards_samples_finished = true;
    a2dp_audio_discards_samples(dest_discards_samples);
  }
  a2dp_audio_decoder_headframe_info_get(&headframe_info);
  TRACE_AUD_STREAM_I("[AUTO_SYNCV2] sample:%d->%d seq:%d sub_seq:%d/%d",
                     list_samples, dest_discards_samples,
                     headframe_info.sequenceNumber,
                     headframe_info.curSubSequenceNumber,
                     headframe_info.totalSubSequenceNumber);

  curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
  TRACE_AUD_STREAM_I("[AUTO_SYNCV2] trig:%x/%x", curr_ticks,
                     sync_trigger->trigger_time);

  if (discards_samples_finished) {
    if (sync_trigger->trigger_time > curr_ticks) {
      synchronize_ok = true;
    } else {
      TRACE_AUD_STREAM_I("[AUTO_SYNCV2] synchronize_failed");
      app_ibrt_if_force_audio_retrigger();
      return 0;
    }
  }
  if (synchronize_ok) {
    TRACE_AUD_STREAM_I("[AUTO_SYNCV2] synchronize_ok");
    tg_acl_trigger_time = sync_trigger->trigger_time;
    btdrv_syn_trigger_codec_en(1);
    btdrv_syn_clr_trigger();
    btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);
    bt_syn_set_tg_ticks(sync_trigger->trigger_time, p_ibrt_ctrl->ibrt_conhandle,
                        BT_TRIG_SLAVE_ROLE);
    app_tws_ibrt_audio_analysis_start(sync_trigger->handler_cnt,
                                      AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
    app_tws_ibrt_audio_sync_start();
    app_tws_ibrt_audio_sync_new_reference(sync_trigger->factor_reference);
    TRACE_AUD_STREAM_I("[AUTO_SYNCV2] trigger curr(%d)-->tg(%d)",
                       bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle),
                       sync_trigger->trigger_time);
    synchronize_need_discards_dma_cnt = 0;
    synchronize_dataind_cnt = 0;
    a2dp_audio_detect_first_packet_clear();
    a2dp_audio_detect_next_packet_callback_register(NULL);
    a2dp_audio_detect_store_packet_callback_register(NULL);
  } else {
    a2dp_audio_detect_first_packet();
  }

  return 0;
}

void app_bt_stream_ibrt_mobile_link_playback_info_receive(
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger) {
  uint32_t tg_tick = 0;
  uint32_t next_dma_cnt = 0;
  A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
  A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
  A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
  APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger_loc =
      &app_bt_stream_ibrt_auto_synchronize_trigger;
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  TRACE_AUD_STREAM_I(
      "[AUTO_SYNCV2][INFO_RECV] session:%d hdl:%d clk:%d cnt:%d seq:%d/%d/%d",
      sync_trigger->a2dp_session, sync_trigger->handler_cnt,
      sync_trigger->trigger_bt_clk, sync_trigger->trigger_bt_cnt,
      sync_trigger->audio_info.sequenceNumber,
      sync_trigger->audio_info.curSubSequenceNumber,
      sync_trigger->audio_info.totalSubSequenceNumber);

  if (app_bt_stream_ibrt_auto_synchronize_on_porcess()) {
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNCV2][INFO_RECV] auto_synchronize_on_porcess skip it");
    return;
  }

  if (!app_bt_is_a2dp_streaming(BTIF_DEVICE_ID_1)) {
    TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] streaming not ready skip it");
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    return;
  }

  if (a2dp_ibrt_session_get() != sync_trigger->a2dp_session) {
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNCV2][INFO_RECV] session mismatch skip it loc:%d rmt:%d",
        a2dp_ibrt_session_get(), sync_trigger->a2dp_session);
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNCV2][INFO_RECV] session froce resume and try retrigger");
    a2dp_ibrt_session_set(sync_trigger->a2dp_session);
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    return;
  }

  if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0) {
    TRACE_AUD_STREAM_W(
        "[AUTO_SYNCV2][INFO_RECV] lastframe not ready mismatch_stopaudio");
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    return;
  }

  *sync_trigger_loc = *sync_trigger;

  sync_info.sequenceNumber = sync_trigger->audio_info.sequenceNumber;
  sync_info.timestamp = sync_trigger->audio_info.timestamp;
  sync_info.curSubSequenceNumber =
      sync_trigger->audio_info.curSubSequenceNumber;
  sync_info.totalSubSequenceNumber =
      sync_trigger->audio_info.totalSubSequenceNumber;
  sync_info.frame_samples = sync_trigger->audio_info.frame_samples;
  if (a2dp_audio_synchronize_packet(&sync_info,
                                    A2DP_AUDIO_SYNCFRAME_MASK_ALL)) {
    TRACE_AUD_STREAM_W("[AUTO_SYNCV2][INFO_RECV] synchronize_packe mismatch");
    app_ibrt_if_force_audio_retrigger();
    return;
  }

  a2dp_audio_decoder_headframe_info_get(&headframe_info);
  TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] sync with master packet step1 "
                     "seq:%d sub_seq:%d/%d",
                     headframe_info.sequenceNumber,
                     headframe_info.curSubSequenceNumber,
                     headframe_info.totalSubSequenceNumber);
  a2dp_audio_discards_samples(lastframe_info.list_samples);
  a2dp_audio_decoder_headframe_info_get(&headframe_info);
  TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] sync with master packet step2 "
                     "seq:%d sub_seq:%d/%d",
                     headframe_info.sequenceNumber,
                     headframe_info.curSubSequenceNumber,
                     headframe_info.totalSubSequenceNumber);

  uint32_t btclk;
  uint16_t btcnt;
  uint32_t mobile_master_clk = 0;
  uint16_t mobile_master_cnt = 0;
  int64_t mobile_master_us = 0;
  btclk = btdrv_syn_get_curr_ticks() / 2;
  btcnt = 0;
  app_tws_ibrt_audio_mobile_clkcnt_get(btclk, btcnt, &mobile_master_clk,
                                       &mobile_master_cnt);
  mobile_master_us = (int64_t)mobile_master_clk * (int64_t)SLOT_SIZE +
                     (int64_t)mobile_master_cnt;

  uint32_t rmt_mobile_master_clk = sync_trigger->trigger_bt_clk;
  uint16_t rmt_mobile_master_cnt = sync_trigger->trigger_bt_cnt;
  int64_t rmt_mobile_master_us = 0;
  int64_t tmp_mobile_master_us = 0;
  rmt_mobile_master_us = (int64_t)rmt_mobile_master_clk * (int64_t)SLOT_SIZE +
                         (int64_t)rmt_mobile_master_cnt;

  uint32_t dma_buffer_us = 0;
  dma_buffer_us = app_bt_stream_get_dma_buffer_delay_us() / 2;

  tmp_mobile_master_us = rmt_mobile_master_us;
  do {
    if (tmp_mobile_master_us - mobile_master_us >= 0) {
      break;
    }
    tmp_mobile_master_us += dma_buffer_us;
    next_dma_cnt++;
  } while (1);
  next_dma_cnt += MOBILE_LINK_PLAYBACK_INFO_TRIG_DUMMY_DMA_CNT;
  tmp_mobile_master_us +=
      dma_buffer_us * (MOBILE_LINK_PLAYBACK_INFO_TRIG_DUMMY_DMA_CNT - 1);
  synchronize_need_discards_dma_cnt =
      next_dma_cnt + a2dp_audio_frame_delay_get() - 1;
  synchronize_dataind_cnt = 0;
  TRACE_AUD_STREAM_I(
      "[AUTO_SYNCV2][INFO_RECV] loc:%08x%08x rmt:%08x%08x tg:%08x%08x",
      (uint32_t)((uint64_t)mobile_master_us >> 32U),
      (uint32_t)((uint64_t)mobile_master_us & 0xffffffff),
      (uint32_t)((uint64_t)rmt_mobile_master_us >> 32U),
      (uint32_t)((uint64_t)rmt_mobile_master_us & 0xffffffff),
      (uint32_t)((uint64_t)tmp_mobile_master_us >> 32U),
      (uint32_t)((uint64_t)tmp_mobile_master_us & 0xffffffff));

  tmp_mobile_master_us = tmp_mobile_master_us / SLOT_SIZE;

  tg_tick = tmp_mobile_master_us * 2;
  tg_tick &= 0x0fffffff;

  sync_trigger_loc->trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_LOCAL;
  sync_trigger_loc->trigger_time = tg_tick;
  sync_trigger_loc->handler_cnt += next_dma_cnt;
  a2dp_audio_detect_next_packet_callback_register(
      app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb_v2);
  a2dp_audio_detect_first_packet();

  TRACE_AUD_STREAM_I("[AUTO_SYNCV2][INFO_RECV] mobile clk:%x/%x tg:%x",
                     bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle) / 2,
                     mobile_master_clk, tg_tick / 2);

  TRACE_AUD_STREAM_I(
      "[AUTO_SYNCV2][INFO_RECV] master_us:%x/%x/%x dma_cnt:%d/%d",
      (int32_t)mobile_master_us, (int32_t)rmt_mobile_master_us,
      (int32_t)tmp_mobile_master_us, next_dma_cnt,
      sync_trigger_loc->handler_cnt);
}

void app_bt_stream_ibrt_set_trigger_time(
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger) {
  uint32_t curr_ticks = 0;
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  uint32_t tg_tick = sync_trigger->trigger_time;
  A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
  int synchronize_ret;
  A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
  A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;

  if (app_bt_stream_ibrt_auto_synchronize_on_porcess()) {
    TRACE_AUD_STREAM_W(
        "[STRM_TRIG][A2DP][IBRT] auto_synchronize_on_porcess skip it");
    return;
  }

  if (!app_bt_is_a2dp_streaming(BTIF_DEVICE_ID_1)) {
    TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] streaming not ready skip it");
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    return;
  }

  if (a2dp_ibrt_session_get() != sync_trigger->a2dp_session) {
    TRACE_AUD_STREAM_W(
        "[STRM_TRIG][A2DP][IBRT] session mismatch skip it loc:%d rmt:%d",
        a2dp_ibrt_session_get(), sync_trigger->a2dp_session);
    TRACE_AUD_STREAM_W(
        "[STRM_TRIG][A2DP][IBRT] session froce resume and try retrigger");
    a2dp_ibrt_session_set(sync_trigger->a2dp_session);
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    return;
  }

  if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0) {
    TRACE_AUD_STREAM_W(
        "[STRM_TRIG][A2DP][IBRT] lastframe not ready mismatch_stopaudio");
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    return;
  }

  if (a2dp_audio_decoder_headframe_info_get(&headframe_info) < 0) {
    TRACE_AUD_STREAM_W(
        "[STRM_TRIG][A2DP][IBRT] lastframe not ready mismatch_stopaudio");
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    goto exit;
  }
  TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] info base_seq:%d/%d",
                     headframe_info.sequenceNumber,
                     sync_trigger->sequenceNumberStart);

  a2dp_audio_detect_next_packet_callback_register(NULL);
  a2dp_audio_detect_store_packet_callback_register(NULL);

  sync_info.sequenceNumber = sync_trigger->sequenceNumberStart;
  synchronize_ret =
      a2dp_audio_synchronize_packet(&sync_info, A2DP_AUDIO_SYNCFRAME_MASK_SEQ);
  if (synchronize_ret) {
    TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] synchronize_packet failed");
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    goto exit;
  }

  curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
  if (tg_tick < curr_ticks) {
    TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] synchronize tick failed:%x->%x",
                       curr_ticks, tg_tick);
    app_bt_stream_ibrt_audio_mismatch_stopaudio();
    goto exit;
  }
  sync_info.sequenceNumber = sync_trigger->audio_info.sequenceNumber;
  sync_info.timestamp = sync_trigger->audio_info.timestamp;
  sync_info.curSubSequenceNumber =
      sync_trigger->audio_info.curSubSequenceNumber;
  sync_info.totalSubSequenceNumber =
      sync_trigger->audio_info.totalSubSequenceNumber;
  sync_info.frame_samples = sync_trigger->audio_info.frame_samples;

  if (app_tws_ibrt_slave_ibrt_link_connected()) {
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) {
      if (sync_trigger->trigger_type ==
          APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC) {
        TRACE_AUD_STREAM_I(
            "[STRM_TRIG][A2DP][IBRT] TRIGGER_TYPE_INIT_SYNC needskip:%d",
            sync_trigger->trigger_skip_frame);
        // limter to water line upper
        uint32_t list_samples = 0;
        uint32_t limter_water_line_samples = 0;
        limter_water_line_samples =
            (a2dp_audio_dest_packet_mut_get() * lastframe_info.list_samples);
        a2dp_audio_convert_list_to_samples(&list_samples);
        TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] synchronize:%d/%d",
                           list_samples, limter_water_line_samples);
        if (list_samples > limter_water_line_samples) {
          TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][IBRT] skip discards:%d",
                             list_samples - limter_water_line_samples);
          a2dp_audio_discards_samples(list_samples - limter_water_line_samples);
        }
        app_bt_stream_ibrt_auto_synchronize_initsync_start(sync_trigger);
        app_bt_stream_ibrt_auto_synchronize_status_set(
            APP_TWS_IBRT_AUDIO_SYNCHRONIZE_STATUS_SYNCOK);
      }
    } else if (!app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
      TRACE_AUD_STREAM_I(
          "[STRM_TRIG][A2DP][IBRT] sbc player not active, so try to start it");
      app_bt_stream_ibrt_auto_synchronize_trigger = *sync_trigger;
      app_bt_stream_ibrt_audio_mismatch_stopaudio();
    }
  } else {
    TRACE_AUD_STREAM_W("[STRM_TRIG][A2DP][IBRT] Not Connected");
  }
exit:
  return;
}

void app_bt_stream_ibrt_audio_mismatch_resume(void) {
  ibrt_a2dp_status_t a2dp_status;

  a2dp_ibrt_sync_get_status(&a2dp_status);

  TRACE_AUD_STREAM_I("[MISMATCH] resume state:%d", a2dp_status.state);

  if (a2dp_status.state == BTIF_AVDTP_STRM_STATE_STREAMING) {
    if (app_tws_ibrt_mobile_link_connected()) {
      TRACE_AUD_STREAM_I(
          "[MISMATCH] resume find role switch so force retrigger");
      app_ibrt_if_force_audio_retrigger();
    } else {
      app_tws_ibrt_audio_sync_mismatch_resume_notify();
    }
  }
}

void app_bt_stream_ibrt_audio_mismatch_stopaudio_cb(uint32_t status,
                                                    uint32_t param) {
  TRACE_AUD_STREAM_I("[MISMATCH] stopaudio_cb");

  if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) {
    TRACE_AUD_STREAM_I("[MISMATCH] stopaudio_cb try again");
    app_audio_manager_sendrequest_need_callback(
        APP_BT_STREAM_MANAGER_STOP, BT_STREAM_SBC, BT_DEVICE_ID_1,
        MAX_RECORD_NUM,
        (uintptr_t)app_bt_stream_ibrt_audio_mismatch_stopaudio_cb,
        (uint32_t)NULL);
  } else {
    app_bt_stream_ibrt_audio_mismatch_resume();
  }
}

int app_bt_stream_ibrt_audio_mismatch_stopaudio(void) {
  ibrt_a2dp_status_t a2dp_status;

  a2dp_ibrt_sync_get_status(&a2dp_status);

  TRACE_AUD_STREAM_I("[MISMATCH] stopaudio state:%d sco:%d sbc:%d media:%d",
                     a2dp_status.state,
                     app_audio_manager_hfp_is_active(BT_DEVICE_ID_1),
                     app_audio_manager_a2dp_is_active(BT_DEVICE_ID_1),
                     app_bt_stream_isrun(APP_PLAY_BACK_AUDIO));

  if (a2dp_status.state == BTIF_AVDTP_STRM_STATE_STREAMING) {
    if (app_audio_manager_a2dp_is_active(BT_DEVICE_ID_1)) {
      TRACE_AUD_STREAM_I("[MISMATCH] stopaudio");
      app_audio_sendrequest_param(APP_BT_STREAM_A2DP_SBC,
                                  (uint8_t)APP_BT_SETTING_RESTART, 0,
                                  APP_SYSFREQ_52M);
      app_bt_stream_ibrt_audio_mismatch_resume();
    } else {
      if (app_ibrt_ui_is_profile_exchanged()) {
        if (!bt_media_is_sbc_media_active()) {
          TRACE_AUD_STREAM_I(
              "[MISMATCH] stopaudio not active resume it & force retrigger");
          app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                                        BT_STREAM_SBC, BT_DEVICE_ID_1,
                                        MAX_RECORD_NUM);
          app_ibrt_if_force_audio_retrigger();
        } else {
          if (app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)
#ifdef MEDIA_PLAYER_SUPPORT
              && app_play_audio_get_aud_id() == AUDIO_ID_BT_MUTE
#endif
          ) {
            TRACE_AUD_STREAM_I("[MISMATCH] stopaudio resum on process skip it");
          } else {
            TRACE_AUD_STREAM_I(
                "[MISMATCH] stopaudio cancel_media and force retrigger");
            app_ibrt_if_force_audio_retrigger();
          }
        }
      } else {
        TRACE_AUD_STREAM_I(
            "[MISMATCH] stopaudio profile not exchanged skip it");
      }
    }
  }

  return 0;
}
#endif

void app_bt_stream_set_trigger_time(uint32_t trigger_time_us) {
  struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
  uint32_t curr_ticks = 0;
  uint32_t dma_buffer_delay_us = 0;
  uint32_t tg_acl_trigger_offset_time = 0;

  if (trigger_time_us) {
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    uint16_t conhandle = INVALID_HANDLE;
    if (app_tws_ibrt_mobile_link_connected()) {
      conhandle = p_ibrt_ctrl->mobile_conhandle;
      curr_ticks = bt_syn_get_curr_ticks(conhandle);
    } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
      conhandle = p_ibrt_ctrl->ibrt_conhandle;
      curr_ticks = bt_syn_get_curr_ticks(conhandle);
    } else {
      return;
    }
#else
    curr_ticks = btdrv_syn_get_curr_ticks();
#endif
    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);
    btdrv_syn_trigger_codec_en(0);
    btdrv_syn_clr_trigger();

    btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

    dma_buffer_delay_us = app_bt_stream_get_dma_buffer_delay_us();
    dma_buffer_delay_us /= 2;
    TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][SETTIME] %d-%d-%d dma_sz:%d dly:%d",
                       stream_cfg->sample_rate, stream_cfg->channel_num,
                       stream_cfg->bits, stream_cfg->data_size,
                       dma_buffer_delay_us);

    tg_acl_trigger_offset_time =
        US_TO_BTCLKS(trigger_time_us - dma_buffer_delay_us);
    tg_acl_trigger_time = curr_ticks + tg_acl_trigger_offset_time;
    tg_acl_trigger_start_time = curr_ticks;
#if defined(IBRT)
    bt_syn_set_tg_ticks(tg_acl_trigger_time, conhandle, BT_TRIG_SLAVE_ROLE);
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][A2DP][SETTIME] %d->%d trig_dly:%d aud_dly:%dus",
        curr_ticks, tg_acl_trigger_time, trigger_time_us - dma_buffer_delay_us,
        trigger_time_us + dma_buffer_delay_us);
#else
    bt_syn_set_tg_ticks(tg_acl_trigger_time, 0, BT_TRIG_NONE_ROLE);
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][A2DP][SETTIME] %d->%d trig_dly:%d aud_dly:%dus",
        curr_ticks, tg_acl_trigger_time, trigger_time_us - dma_buffer_delay_us,
        trigger_time_us + dma_buffer_delay_us);
#endif

    btdrv_syn_trigger_codec_en(1);
    app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_WAIT);
  } else {
    tg_acl_trigger_time = 0;
    tg_acl_trigger_start_time = 0;
    btdrv_syn_trigger_codec_en(0);
    btdrv_syn_clr_trigger();
    app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_NULL);
    TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][SETTIME] trigger clear");
  }
}

void app_bt_stream_trigger_result(void) {
  uint32_t curr_ticks = 0;

  if (tg_acl_trigger_time) {
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()) {
      curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
      bt_syn_trig_checker(p_ibrt_ctrl->mobile_conhandle);
    } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
      curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
      bt_syn_trig_checker(p_ibrt_ctrl->ibrt_conhandle);
    } else {
      TRACE_AUD_STREAM_I(
          "[STRM_TRIG][A2DP][RESULT] mobile_link:%d %04x ibrt_link:%d %04x",
          app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
          app_tws_ibrt_slave_ibrt_link_connected(),
          p_ibrt_ctrl->ibrt_conhandle);
    }
#else
    curr_ticks = btdrv_syn_get_curr_ticks();
#endif
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][A2DP][RESULT] trig:%d curr:%d tg:%d start:%d",
        (curr_ticks -
         (uint32_t)US_TO_BTCLKS(app_bt_stream_get_dma_buffer_delay_us() / 2)),
        curr_ticks, tg_acl_trigger_time, tg_acl_trigger_start_time);
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][A2DP][RESULT] tg_trig_diff:%d trig_diff:%d",
        (uint32_t)BTCLKS_TO_US(curr_ticks - tg_acl_trigger_time),
        (uint32_t)BTCLKS_TO_US(curr_ticks - tg_acl_trigger_start_time));
    app_bt_stream_set_trigger_time(0);
    app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_OK);
    A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
    a2dp_audio_decoder_headframe_info_get(&headframe_info);
    TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][RESULT] synchronize_ok :%d",
                       headframe_info.sequenceNumber);
  }
}

void app_bt_stream_playback_irq_notification(enum AUD_STREAM_ID_T id,
                                             enum AUD_STREAM_T stream) {
  if (id != AUD_STREAM_ID_0 || stream != AUD_STREAM_PLAYBACK) {
    return;
  }
  app_bt_stream_trigger_result();
#if defined(IBRT)
  app_tws_ibrt_audio_analysis_interrupt_tick();
#endif
}
extern void a2dp_audio_set_mtu_limit(uint8_t mut);
extern float a2dp_audio_latency_factor_get(void);
extern uint8_t a2dp_lhdc_config_llc_get(void);

void app_bt_stream_trigger_init(void) {
#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (app_tws_ibrt_mobile_link_connected()) {
    tg_acl_trigger_init_time =
        bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    tg_acl_trigger_init_time =
        bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
  } else {
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][A2DP][INIT] mobile_link:%d %04x ibrt_link:%d %04x",
        app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
        app_tws_ibrt_slave_ibrt_link_connected(), p_ibrt_ctrl->ibrt_conhandle);
  }
#else
  tg_acl_trigger_init_time = btdrv_syn_get_curr_ticks();
#endif
  app_bt_stream_set_trigger_time(0);
#ifdef PLAYBACK_USE_I2S
  af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, false);
  af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, true);
#else
  af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
  af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
#endif
  app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_INIT);
}

void app_bt_stream_trigger_deinit(void) { app_bt_stream_set_trigger_time(0); }

void app_bt_stream_trigger_start(uint8_t offset) {
  float tg_trigger_time = 0;
  uint32_t curr_ticks;
  struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
  af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, false);

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (app_tws_ibrt_mobile_link_connected()) {
    curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle);
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle);
  } else {
    return;
  }
#else
  curr_ticks = btdrv_syn_get_curr_ticks();
#endif

  TRACE_AUD_STREAM_I(
      "[STRM_TRIG][A2DP][START] init(%d)-->set_trig(%d) %dus",
      tg_acl_trigger_init_time, curr_ticks,
      (uint32_t)BTCLKS_TO_US(curr_ticks - tg_acl_trigger_init_time));

#if defined(A2DP_AAC_ON)
  if (bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
    tg_trigger_time =
        A2DP_PLAYER_PLAYBACK_DELAY_AAC_US * a2dp_audio_latency_factor_get();
    tg_trigger_time += offset * A2DP_PLAYER_PLAYBACK_DELAY_AAC_BASE;
#if (A2DP_DECODER_VER < 2)
    a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU);
#endif
  } else
#endif
      if (bt_sbc_player_get_codec_type() == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
#if defined(A2DP_SCALABLE_ON)
    if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_SCALABLE) {
      if (stream_cfg->sample_rate > AUD_SAMPRATE_48000) {
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_US *
                          a2dp_audio_latency_factor_get();

        tg_trigger_time +=
            offset * A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_BASE;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU);
#endif
      } else {
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_US *
                          a2dp_audio_latency_factor_get();
        tg_trigger_time +=
            offset * A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_BASE;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(
            A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU);
#endif
      }
    }
#endif
#if defined(A2DP_LHDC_ON)
    if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LHDC) {
      if (a2dp_lhdc_config_llc_get()) {
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_US *
                          a2dp_audio_latency_factor_get();
        tg_trigger_time += offset * A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_BASE;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU);
#endif
      } else if (stream_cfg->sample_rate > AUD_SAMPRATE_48000) {
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_US *
                          a2dp_audio_latency_factor_get();
        tg_trigger_time += offset * A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_BASE;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU);
#endif
      } else {
        tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_US *
                          a2dp_audio_latency_factor_get();
        tg_trigger_time +=
            offset * A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_BASE;
#if (A2DP_DECODER_VER < 2)
        a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU);
#endif
      }
    }
#endif
#if defined(A2DP_LDAC_ON)
    if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LDAC) {
      uint32_t frame_mtu = A2DP_PLAYER_PLAYBACK_DELAY_LDAC_FRAME_MTU;
#if (A2DP_DECODER_VER == 2)
      A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
      if (!a2dp_audio_lastframe_info_get(&lastframe_info)) {
        frame_mtu = lastframe_info.totalSubSequenceNumber;
      }
#endif
      tg_trigger_time =
          A2DP_PLAYER_PLAYBACK_DELAY_LDAC_US * a2dp_audio_latency_factor_get();
      tg_trigger_time +=
          offset * A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE * frame_mtu;
#if (A2DP_DECODER_VER < 2)
      a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU);
#endif
      TRACE_AUD_STREAM_I("[STRM_TRIG][A2DP][START] [%d,%d,%d]",
                         A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU,
                         A2DP_PLAYER_PLAYBACK_DELAY_LDAC_BASE,
                         A2DP_PLAYER_PLAYBACK_DELAY_LDAC_US);
    }
#endif
  } else {
    uint32_t frame_mtu = A2DP_PLAYER_PLAYBACK_DELAY_SBC_FRAME_MTU;
#if (A2DP_DECODER_VER == 2)
    A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
    if (!a2dp_audio_lastframe_info_get(&lastframe_info)) {
      frame_mtu = lastframe_info.totalSubSequenceNumber;
    }
#endif
    tg_trigger_time = A2DP_PLAYER_PLAYBACK_DELAY_SBC_US;
    tg_trigger_time += offset * A2DP_PLAYER_PLAYBACK_DELAY_SBC_BASE * frame_mtu;
#if (A2DP_DECODER_VER < 2)
    a2dp_audio_set_mtu_limit(A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU);
#endif
  }
  app_bt_stream_set_trigger_time((uint32_t)tg_trigger_time);
}

bool app_bt_stream_trigger_onprocess(void) {
  if (app_bt_stream_trigger_stauts_get() == BT_STREAM_TRIGGER_STATUS_INIT) {
    return true;
  } else {
    return false;
  }
}

#if defined(IBRT)
#ifdef A2DP_CP_ACCEL
#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME (8)
#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC (12)

#else
#define APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME (4)
#endif

APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T
app_bt_stream_ibrt_auto_synchronize_initsync_trigger;

int app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb(
    btif_media_header_t *header, unsigned char *buf, unsigned int len) {
  APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger =
      &app_bt_stream_ibrt_auto_synchronize_initsync_trigger;
  bool synchronize_ok = false;
  A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;

  TRACE_AUD_STREAM_I(
      "[AUTO_SYNC][INITSYNC][DATAIND] dataind_seq:%d/%d timestamp:%d/%d",
      header->sequenceNumber, sync_trigger->audio_info.sequenceNumber,
      header->timestamp, sync_trigger->audio_info.timestamp);

  if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0) {
    TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] force retrigger");
    app_ibrt_if_force_audio_retrigger();
    return 0;
  }

  if (app_bt_stream_trigger_stauts_get() != BT_STREAM_TRIGGER_STATUS_WAIT) {
    TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] already end");
    a2dp_audio_detect_next_packet_callback_register(NULL);
    a2dp_audio_detect_store_packet_callback_register(NULL);
    return 0;
  }

  if (sync_trigger->audio_info.sequenceNumber < header->sequenceNumber) {
    TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] force retrigger");
    app_ibrt_if_force_audio_retrigger();
  } else {
    if (header->sequenceNumber >= sync_trigger->audio_info.sequenceNumber) {
      synchronize_ok = true;
    }

    if (synchronize_ok &&
        app_bt_stream_trigger_stauts_get() != BT_STREAM_TRIGGER_STATUS_WAIT) {
      TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] synchronize_failed");
      app_ibrt_if_force_audio_retrigger();
      return 0;
    }
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
    // limter to water line upper
    uint32_t list_samples = 0;
    uint32_t limter_water_line_samples = 0;
    limter_water_line_samples =
        (a2dp_audio_dest_packet_mut_get() * lastframe_info.list_samples);
    a2dp_audio_convert_list_to_samples(&list_samples);
    TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] synchronize:%d/%d",
                       list_samples, limter_water_line_samples);
    if (list_samples > limter_water_line_samples) {
      TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] skip discards:%d",
                         list_samples - limter_water_line_samples);
      a2dp_audio_discards_samples(list_samples - limter_water_line_samples);
    }
#else
    // flush all
    a2dp_audio_synchronize_dest_packet_mut(0);
#endif
    if (synchronize_ok) {
      A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
      a2dp_audio_decoder_headframe_info_get(&headframe_info);
      TRACE_AUD_STREAM_W("[AUTO_SYNC][INITSYNC][DATAIND] synchronize_ok :%d",
                         headframe_info.sequenceNumber);
      a2dp_audio_detect_next_packet_callback_register(NULL);
      a2dp_audio_detect_store_packet_callback_register(NULL);
    } else {
      a2dp_audio_detect_first_packet();
    }
  }
  return 0;
}

void app_bt_stream_ibrt_auto_synchronize_initsync_start(
    APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T *sync_trigger) {
  TRACE_AUD_STREAM_I("[AUTO_SYNC][INITSYNC] start trigger_time:%d seq:%d "
                     "timestamp:%d SubSeq:%d/%d",
                     sync_trigger->trigger_time,
                     sync_trigger->audio_info.sequenceNumber,
                     sync_trigger->audio_info.timestamp,
                     sync_trigger->audio_info.curSubSequenceNumber,
                     sync_trigger->audio_info.totalSubSequenceNumber);
  app_bt_stream_ibrt_auto_synchronize_initsync_trigger = *sync_trigger;
  a2dp_audio_detect_next_packet_callback_register(
      app_bt_stream_ibrt_auto_synchronize_initsync_dataind_cb);
  a2dp_audio_detect_first_packet();
  if (app_tws_ibrt_slave_ibrt_link_connected()) {
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (sync_trigger->trigger_time >
        bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle)) {
      uint32_t tg_tick = sync_trigger->trigger_time;
      btdrv_syn_trigger_codec_en(1);
      btdrv_syn_clr_trigger();
      btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);
      bt_syn_set_tg_ticks(tg_tick, p_ibrt_ctrl->ibrt_conhandle,
                          BT_TRIG_SLAVE_ROLE);
      tg_acl_trigger_time = tg_tick;
      app_bt_stream_trigger_stauts_set(BT_STREAM_TRIGGER_STATUS_WAIT);
      TRACE_AUD_STREAM_I(
          "[AUTO_SYNC][INITSYNC] start slave trigger curr(%d)-->tg(%d)",
          bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle), tg_tick);
      app_tws_ibrt_audio_analysis_start(
          sync_trigger->handler_cnt, AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
      app_tws_ibrt_audio_sync_start();
      app_tws_ibrt_audio_sync_new_reference(sync_trigger->factor_reference);
    } else {
      TRACE_AUD_STREAM_I("[AUTO_SYNC][INITSYNC] start slave failed "
                         "trigger(%d)-->tg(%d) need resume",
                         bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle),
                         sync_trigger->trigger_time);
      app_tws_ibrt_audio_sync_mismatch_resume_notify();
    }
  }
}

int app_bt_stream_ibrt_audio_master_detect_next_packet_cb(
    btif_media_header_t *header, unsigned char *buf, unsigned int len) {
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
  A2DP_AUDIO_SYNCFRAME_INFO_T sync_info;
  A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
#endif

  if (app_bt_stream_trigger_stauts_get() == BT_STREAM_TRIGGER_STATUS_INIT) {
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    int32_t dma_buffer_samples = app_bt_stream_get_dma_buffer_samples() / 2;

    if (app_tws_ibrt_slave_ibrt_link_connected()) {
      TRACE_AUD_STREAM_W(
          "[AUTO_SYNC][MASTER] cache ok but currRole:%d mismatch\n",
          p_ibrt_ctrl->current_role);
      app_ibrt_if_force_audio_retrigger();
    } else if (!app_ibrt_ui_is_profile_exchanged() &&
               !app_ibrt_if_start_ibrt_onporcess() &&
               !app_ibrt_sync_a2dp_status_onporcess()) {
      if (p_ibrt_ctrl->mobile_mode == IBRT_SNIFF_MODE) {
        // flush all
        a2dp_audio_synchronize_dest_packet_mut(0);
        a2dp_audio_detect_first_packet();
        TRACE_AUD_STREAM_I(
            "[AUTO_SYNC][MASTER] cache skip delay dma trigger1\n");
        return 0;
      }
      TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache ok use dma trigger1\n");
      a2dp_audio_detect_next_packet_callback_register(NULL);
      a2dp_audio_detect_store_packet_callback_register(NULL);
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
      app_bt_stream_trigger_start(A2DP_PLAYER_PLAYBACK_WATER_LINE);
#else
      app_bt_stream_trigger_start(0);
#endif
    } else if (app_ibrt_if_start_ibrt_onporcess() ||
               app_ibrt_sync_a2dp_status_onporcess() ||
               app_ibrt_waiting_cmd_rsp()) {
      // flush all
      a2dp_audio_synchronize_dest_packet_mut(0);
      a2dp_audio_detect_first_packet();
      TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache skip profile_exchanged "
                         "sync_a2dp_status_onporcess\n");
      return 0;
    } else {
      if (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE ||
          p_ibrt_ctrl->mobile_mode == IBRT_SNIFF_MODE) {
        // flush all
        a2dp_audio_synchronize_dest_packet_mut(0);
        a2dp_audio_detect_first_packet();
        TRACE_AUD_STREAM_I(
            "[AUTO_SYNC][MASTER] cache skip delay dma trigger2\n");
        return 0;
      }
      uint8_t codec_type = bt_sbc_player_get_codec_type();
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
      uint32_t dest_waterline_samples = 0;
      uint32_t list_samples = 0;
      dest_waterline_samples = app_bt_stream_get_dma_buffer_samples() / 2 *
                               A2DP_PLAYER_PLAYBACK_WATER_LINE;
      a2dp_audio_convert_list_to_samples(&list_samples);
      if (list_samples < dest_waterline_samples) {
        a2dp_audio_detect_first_packet();
        TRACE_AUD_STREAM_I(
            "[AUTO_SYNC][MASTER] cache skip fill data sample:%d\n",
            list_samples);
        return 0;
      }
      a2dp_audio_decoder_headframe_info_get(&headframe_info);
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
        sync_info.sequenceNumber = headframe_info.sequenceNumber;
      } else {
        sync_info.sequenceNumber = headframe_info.sequenceNumber + 1;
      }
      a2dp_audio_synchronize_packet(&sync_info, A2DP_AUDIO_SYNCFRAME_MASK_SEQ);
#else
      a2dp_audio_synchronize_dest_packet_mut(0);
#endif
      TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] cache ok use dma trigger2\n");
      // uint8_t codec_type = bt_sbc_player_get_codec_type();
#ifdef A2DP_CP_ACCEL
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
        app_bt_stream_trigger_start(
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC -
            a2dp_audio_frame_delay_get());
      } else {
        app_bt_stream_trigger_start(
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME -
            a2dp_audio_frame_delay_get());
      }
#else
      app_bt_stream_trigger_start(
          APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME);
#endif
      APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T sync_trigger;
      A2DP_AUDIO_HEADFRAME_INFO_T headframe_info;
      A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info;
      sync_trigger.trigger_time = tg_acl_trigger_time;
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
        sync_trigger.trigger_skip_frame =
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC -
            a2dp_audio_frame_delay_get();
      } else {
        sync_trigger.trigger_skip_frame =
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME -
            a2dp_audio_frame_delay_get();
      }
      sync_trigger.trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC;
      if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0) {
        goto exit;
      }

      a2dp_audio_decoder_headframe_info_get(&headframe_info);
      sync_trigger.sequenceNumberStart = headframe_info.sequenceNumber;
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
        sync_trigger.audio_info.sequenceNumber =
            lastframe_info.sequenceNumber +
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
        if (lastframe_info.totalSubSequenceNumber) {
          sync_trigger.audio_info.timestamp =
              lastframe_info.timestamp +
              (lastframe_info.totalSubSequenceNumber *
               lastframe_info.frame_samples) *
                  APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
        } else {
          sync_trigger.audio_info.timestamp =
              lastframe_info.timestamp +
              dma_buffer_samples *
                  APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
        }
      } else {
        sync_trigger.audio_info.sequenceNumber =
            lastframe_info.sequenceNumber +
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
        if (lastframe_info.totalSubSequenceNumber) {
          sync_trigger.audio_info.timestamp =
              lastframe_info.timestamp +
              (lastframe_info.totalSubSequenceNumber *
               lastframe_info.frame_samples) *
                  APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
        } else {
          sync_trigger.audio_info.timestamp =
              lastframe_info.timestamp +
              dma_buffer_samples *
                  APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
        }
      }
      sync_trigger.audio_info.curSubSequenceNumber =
          lastframe_info.curSubSequenceNumber;
      sync_trigger.audio_info.totalSubSequenceNumber =
          lastframe_info.totalSubSequenceNumber;
      sync_trigger.audio_info.frame_samples = lastframe_info.frame_samples;
      sync_trigger.factor_reference =
          a2dp_audio_get_output_config()->factor_reference;
      sync_trigger.a2dp_session = a2dp_ibrt_session_get();
      sync_trigger.handler_cnt = 0;

      app_bt_stream_ibrt_auto_synchronize_initsync_start(&sync_trigger);

      if (app_tws_ibrt_mobile_link_connected() &&
          app_ibrt_ui_is_profile_exchanged()) {
        tws_ctrl_send_cmd_high_priority(
            APP_TWS_CMD_SET_TRIGGER_TIME, (uint8_t *)&sync_trigger,
            sizeof(APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T));
      }
    }
  } else {
    if (app_bt_stream_trigger_stauts_get() == BT_STREAM_TRIGGER_STATUS_NULL) {
      TRACE_AUD_STREAM_W("[AUTO_SYNC][MASTER] audio not ready skip it");
    } else {
      TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] unhandle status:%d",
                         app_bt_stream_trigger_stauts_get());
      app_ibrt_if_force_audio_retrigger();
    }
  }
exit:
  return 0;
}

int app_bt_stream_ibrt_audio_master_detect_next_packet_start(void) {
  TRACE_AUD_STREAM_I("[AUTO_SYNC][MASTER] start");

  a2dp_audio_detect_next_packet_callback_register(
      app_bt_stream_ibrt_audio_master_detect_next_packet_cb);
  return 0;
}

#define SLAVE_DETECT_NEXT_PACKET_TO_RETRIGGER_THRESHOLD (120)
static uint32_t slave_detect_next_packet_cnt = 0;
int app_bt_stream_ibrt_audio_slave_detect_next_packet_waitforever_cb(
    btif_media_header_t *header, unsigned char *buf, unsigned int len) {
  if (app_tws_ibrt_mobile_link_connected() ||
      ++slave_detect_next_packet_cnt >
          SLAVE_DETECT_NEXT_PACKET_TO_RETRIGGER_THRESHOLD) {
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE_AUD_STREAM_W("[AUTO_SYNC][SLAVE] detect_next_packet ok but "
                       "currRole:%d mismatch packet_cnt:%d\n",
                       p_ibrt_ctrl->current_role, slave_detect_next_packet_cnt);
    slave_detect_next_packet_cnt = 0;
    app_ibrt_if_force_audio_retrigger();
  } else {
    TRACE_AUD_STREAM_I("[AUTO_SYNC][SLAVE] detect_next_packet cnt:%d\n",
                       slave_detect_next_packet_cnt);
    a2dp_audio_detect_first_packet();
  }
  return 0;
}

int app_bt_stream_ibrt_audio_slave_detect_next_packet_cb(
    btif_media_header_t *header, unsigned char *buf, unsigned int len) {
  if (app_bt_stream_trigger_onprocess()) {
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    int32_t dma_buffer_samples = app_bt_stream_get_dma_buffer_samples() / 2;

    if (app_tws_ibrt_mobile_link_connected()) {
      TRACE_AUD_STREAM_W(
          "[AUTO_SYNC][SLAVE]cache ok but currRole:%d mismatch\n",
          p_ibrt_ctrl->current_role);
      app_ibrt_if_force_audio_retrigger();
    } else {
      if (app_tws_ibrt_slave_ibrt_link_connected() &&
          (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE ||
           p_ibrt_ctrl->mobile_mode == IBRT_SNIFF_MODE ||
           !app_ibrt_ui_is_profile_exchanged())) {
        // flush all
        a2dp_audio_synchronize_dest_packet_mut(0);
        a2dp_audio_detect_first_packet();
        TRACE_AUD_STREAM_W("[AUTO_SYNC][SLAVE]cache skip delay dma trigger2\n");
        return 0;
      }
      TRACE_AUD_STREAM_I("[AUTO_SYNC][SLAVE]cache ok use dma trigger2\n");
      // flush all
      a2dp_audio_synchronize_dest_packet_mut(0);
      uint8_t codec_type = bt_sbc_player_get_codec_type();
#ifdef A2DP_CP_ACCEL
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
        app_bt_stream_trigger_start(
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC -
            a2dp_audio_frame_delay_get());
      } else {
        app_bt_stream_trigger_start(
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME -
            a2dp_audio_frame_delay_get());
      }
#else
      app_bt_stream_trigger_start(
          APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME);
#endif
      APP_TWS_IBRT_AUDIO_SYNC_TRIGGER_T sync_trigger = {
          0,
      };
      A2DP_AUDIO_LASTFRAME_INFO_T lastframe_info = {
          0,
      };
      sync_trigger.trigger_time = tg_acl_trigger_time;
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
        sync_trigger.trigger_skip_frame =
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC -
            a2dp_audio_frame_delay_get();
      } else {
        sync_trigger.trigger_skip_frame =
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME -
            a2dp_audio_frame_delay_get();
      }
      sync_trigger.trigger_type = APP_TWS_IBRT_AUDIO_TRIGGER_TYPE_INIT_SYNC;
      if (a2dp_audio_lastframe_info_get(&lastframe_info) < 0) {
        goto exit;
      }
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
        sync_trigger.audio_info.sequenceNumber =
            lastframe_info.sequenceNumber +
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
        sync_trigger.audio_info.timestamp =
            lastframe_info.timestamp +
            dma_buffer_samples *
                APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME_LHDC;
      } else {
        sync_trigger.audio_info.sequenceNumber =
            lastframe_info.sequenceNumber +
            APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
        sync_trigger.audio_info.timestamp =
            lastframe_info.timestamp +
            dma_buffer_samples *
                APP_BT_STREAM_IBRT_AUTO_SYNCHRONIZE_INITSYNC_SKIP_FRAME;
      }
      sync_trigger.audio_info.curSubSequenceNumber =
          lastframe_info.curSubSequenceNumber;
      sync_trigger.audio_info.totalSubSequenceNumber =
          lastframe_info.totalSubSequenceNumber;
      sync_trigger.audio_info.frame_samples = lastframe_info.frame_samples;
      sync_trigger.factor_reference =
          a2dp_audio_get_output_config()
              ? a2dp_audio_get_output_config()->factor_reference
              : 1.0f;
      sync_trigger.a2dp_session = a2dp_ibrt_session_get();
      sync_trigger.handler_cnt = 0;

      app_bt_stream_ibrt_auto_synchronize_initsync_start(&sync_trigger);
    }
  }
exit:
  return 0;
}

int app_bt_stream_ibrt_audio_slave_detect_next_packet_start(
    int need_autotrigger) {
  TRACE_AUD_STREAM_I("[AUTO_SYNC][SLAVE] start");
  slave_detect_next_packet_cnt = 0;
  if (need_autotrigger) {
    a2dp_audio_detect_next_packet_callback_register(
        app_bt_stream_ibrt_audio_slave_detect_next_packet_waitforever_cb);
  } else {
    app_tws_ibrt_audio_analysis_start(0,
                                      AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
    app_tws_ibrt_audio_sync_start();
    a2dp_audio_detect_next_packet_callback_register(
        app_bt_stream_ibrt_audio_slave_detect_next_packet_cb);
  }
  return 0;
}

#else
int app_bt_stream_detect_next_packet_cb(btif_media_header_t *header,
                                        unsigned char *buf, unsigned int len) {
  if (app_bt_stream_trigger_onprocess()) {
    TRACE_AUD_STREAM_I("[AUTO_SYNC] start");
    app_bt_stream_trigger_start(0);
  }
  return 0;
}
#endif

void app_audio_buffer_check(void) {
  bool buffer_check = (APP_AUDIO_BUFFER_SIZE + APP_CAPTURE_AUDIO_BUFFER_SIZE) <=
                      syspool_original_size();

  TRACE_AUD_STREAM_I(
      "audio buf size[%d] capture buf size[%d] total available space[%d]",
      APP_AUDIO_BUFFER_SIZE, APP_CAPTURE_AUDIO_BUFFER_SIZE,
      syspool_original_size());

  ASSERT(
      buffer_check,
      "Audio buffer[%d]+Capture buffer[%d] exceeds the maximum ram sapce[%d]",
      APP_AUDIO_BUFFER_SIZE, APP_CAPTURE_AUDIO_BUFFER_SIZE,
      syspool_original_size());
}

static bool isRun = false;

int bt_sbc_player(enum PLAYER_OPER_T on, enum APP_SYSFREQ_FREQ_T freq) {
  struct AF_STREAM_CONFIG_T stream_cfg;
  enum AUD_SAMPRATE_T sample_rate;
  const char *g_log_player_oper_str[] = {
      "PLAYER_OPER_START",
      "PLAYER_OPER_STOP",
      "PLAYER_OPER_RESTART",
  };

  uint8_t *bt_audio_buff = NULL;

  uint8_t POSSIBLY_UNUSED *bt_eq_buff = NULL;
  uint32_t POSSIBLY_UNUSED eq_buff_size = 0;
  uint8_t POSSIBLY_UNUSED play_samp_size;
  TRACE_AUD_STREAM_I("[A2DP_PLAYER] work:%d op:%s freq:%d :sample:%d \n", isRun,
                     g_log_player_oper_str[on], freq, a2dp_sample_rate);

  bt_set_playback_triggered(false);

  if ((isRun && on == PLAYER_OPER_START) ||
      (!isRun && on == PLAYER_OPER_STOP)) {
    TRACE_AUD_STREAM_W("[A2DP_PLAYER],fail,isRun=%x,on=%x", isRun, on);
    return 0;
  }

  uint8_t codec_type = bt_sbc_player_get_codec_type();

  if (on == PLAYER_OPER_STOP || on == PLAYER_OPER_RESTART) {
#ifdef __THIRDPARTY
    start_by_sbc = false;
#endif

#ifdef WL_DET
    app_mic_alg_audioloop(false, APP_SYSFREQ_78M);
#endif

#ifdef PLAYBACK_USE_I2S
    hal_cmu_audio_resample_enable();
#endif

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    bool isToClearActiveMedia =
        audio_prompt_clear_pending_stream(PENDING_TO_STOP_A2DP_STREAMING);
    if (isToClearActiveMedia) {
      // clear active media mark
      bt_media_clear_media_type(BT_STREAM_SBC, BT_DEVICE_ID_1);
      bt_media_current_sbc_set(BT_DEVICE_NUM);
    }
#endif

#if defined(IBRT)
    app_bt_stream_ibrt_auto_synchronize_stop();
    app_tws_ibrt_audio_analysis_stop();
    app_tws_ibrt_audio_sync_stop();
#endif

#if (A2DP_DECODER_VER == 2)
    a2dp_audio_stop();
#endif
    af_codec_set_playback_post_handler(NULL);
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#ifdef AUDIO_ANC_FB_ADJ_MC
    af_stream_stop(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE);
#endif
    af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))
    calib_reset = 1;
    af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
    af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

#if defined(__AUDIO_SPECTRUM__)
    audio_spectrum_close();
#endif

    audio_process_close();

    TRACE_AUD_STREAM_I("[A2DP_PLAYER] syspool free size: %d/%d",
                       syspool_free_size(), syspool_total_size());

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
    af_codec_tune(AUD_STREAM_PLAYBACK, 0);
#endif

    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
#ifdef AUDIO_ANC_FB_ADJ_MC
    af_stream_close(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE);
#endif

    af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

    bt_sbc_mode = 0;

#ifdef VOICE_DATAPATH
    app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, false);
    app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, false);
#endif
#ifdef AI_AEC_CP_ACCEL
    cp_aec_deinit();
#endif
    af_set_irq_notification(NULL);
    if (on == PLAYER_OPER_STOP) {

      osThreadId ctrl_thread_id = NULL;
      osPriority ctrl_thread_priority;
      ctrl_thread_id = osThreadGetId();
      ctrl_thread_priority = osThreadGetPriority(ctrl_thread_id);
      osThreadSetPriority(ctrl_thread_id, osPriorityLow);
      app_bt_stream_trigger_checker_stop();
#ifdef USER_REBOOT_PLAY_MUSIC_AUTO
      TRACE_AUD_STREAM_I("[A2DP_PLAYER] hal_sw_bootmode_clear "
                         "HAL_SW_BOOTMODE_LOCAL_PLAYER!!!!!!");
      hal_sw_bootmode_clear(HAL_SW_BOOTMODE_LOCAL_PLAYER);
#endif
#ifdef __A2DP_PLAYER_USE_BT_TRIGGER__
      app_bt_stream_trigger_deinit();
#endif
#ifdef BT_XTAL_SYNC
      bt_term_xtal_sync(false);
#ifndef BT_XTAL_SYNC_NO_RESET
      bt_term_xtal_sync_default();
#endif
#endif
      a2dp_audio_deinit();
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) &&                            \
    !defined(__AUDIO_RESAMPLE__) && defined(AUDIO_ANC_FB_ADJ_MC)
      adj_mc_deinit();
#endif
      app_overlay_unloadall();
#ifdef __THIRDPARTY
      app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                               THIRDPARTY_STOP2MIC);
      app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                               THIRDPARTY_START);
#endif
      app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, APP_SYSFREQ_32K);
      osThreadSetPriority(ctrl_thread_id, ctrl_thread_priority);
      af_set_priority(AF_USER_SBC, osPriorityAboveNormal);

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
      app_ai_if_inform_music_or_prompt_status(false, 0);
#endif

#if defined(IBRT)
      app_ibrt_if_exec_sleep_hook_blocker_clr(
          APP_IBRT_IF_SLEEP_HOOK_BLOCKER_A2DP_STREAMING);
#endif
    }
  }

  if (on == PLAYER_OPER_START || on == PLAYER_OPER_RESTART) {
#if USER_REBOOT_PLAY_MUSIC_AUTO
    TRACE_AUD_STREAM_I(
        "[A2DP_PLAYER] hal_sw_bootmode_set HAL_SW_BOOTMODE_LOCAL_PLAYER!!!!!!");
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_LOCAL_PLAYER);
#endif

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    app_ai_if_inform_music_or_prompt_status(true, a2dp_sample_rate);
    app_ai_if_pre_music_or_prompt_check();
#endif
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    audio_prompt_stop_playing();
#endif

#if defined(IBRT)
    app_ibrt_if_exec_sleep_hook_blocker_set(
        APP_IBRT_IF_SLEEP_HOOK_BLOCKER_A2DP_STREAMING);
#endif
#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_STOP);
#endif
    af_set_priority(AF_USER_SBC, osPriorityHigh);
    bt_media_volume_ptr_update_by_mediatype(BT_STREAM_SBC);
    stream_local_volume = btdevice_volume_p->a2dp_vol;
    app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);

#ifdef __BT_ONE_BRING_TWO__
    if (btif_me_get_activeCons() > 1) {
      if (freq < APP_SYSFREQ_104M) {
        freq = APP_SYSFREQ_104M;
      }
    }
#endif
#ifdef __PC_CMD_UART__
    if (freq < APP_SYSFREQ_104M) {
      freq = APP_SYSFREQ_104M;
    }
#endif
#if defined(__SW_IIR_EQ_PROCESS__) && defined(__HW_FIR_EQ_PROCESS__) &&        \
    defined(CHIP_BEST1000)
    if (audio_eq_hw_fir_cfg_list[bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR, 0)]
            ->len > 128) {
      if (freq < APP_SYSFREQ_104M) {
        freq = APP_SYSFREQ_104M;
      }
    }
#endif
#if defined(APP_MUSIC_26M) && !defined(__SW_IIR_EQ_PROCESS__) &&               \
    !defined(__HW_IIR_EQ_PROCESS__) && !defined(__HW_FIR_EQ_PROCESS__)
    if (freq < APP_SYSFREQ_26M) {
      freq = APP_SYSFREQ_26M;
    }
#else
    if (freq < APP_SYSFREQ_52M) {
      freq = APP_SYSFREQ_52M;
    }
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    if (freq < APP_SYSFREQ_104M) {
      freq = APP_SYSFREQ_104M;
    }
#endif
    TRACE_AUD_STREAM_I("[A2DP_PLAYER] codec_type:%d", codec_type);
#if defined(A2DP_AAC_ON)
    if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
      if (freq < APP_SYSFREQ_52M) {
        freq = APP_SYSFREQ_52M;
      }
    }
#endif
#if defined(A2DP_SCALABLE_ON)
    if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
      if (a2dp_sample_rate == 44100) {
        if (freq < APP_SYSFREQ_78M) {
          freq = APP_SYSFREQ_78M;
        }
      } else if (a2dp_sample_rate == 96000) {
        if (freq < APP_SYSFREQ_208M) {
          freq = APP_SYSFREQ_208M;
        }
      }
    }
    TRACE_AUD_STREAM_I("[A2DP_PLAYER] a2dp_sample_rate=%d", a2dp_sample_rate);
#endif

#if defined(__AUDIO_DRC__) || defined(__AUDIO_DRC2__)
    freq = (freq < APP_SYSFREQ_208M) ? APP_SYSFREQ_208M : freq;
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
    freq = (freq < APP_SYSFREQ_104M) ? APP_SYSFREQ_104M : freq;
#endif

#ifdef PLAYBACK_FORCE_48K
    freq = (freq < APP_SYSFREQ_104M) ? APP_SYSFREQ_104M : freq;
#endif

#ifdef A2DP_CP_ACCEL
    // Default freq for SBC
    freq = APP_SYSFREQ_26M;
#if defined(A2DP_AAC_ON)
    if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
      freq = APP_SYSFREQ_26M;
    }
#endif

#if defined(A2DP_LHDC_ON) || defined(A2DP_SCALABLE_ON) || defined(A2DP_LDAC_ON)
    if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
      freq = APP_SYSFREQ_52M;
    }

    if (a2dp_sample_rate == AUD_SAMPRATE_96000) {
      freq = APP_SYSFREQ_104M;
    }
#endif
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    if (freq < APP_SYSFREQ_52M) {
      freq = APP_SYSFREQ_52M;
    }
#endif
#if defined(A2DP_LDAC_ON)
    if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
      if (a2dp_sample_rate == AUD_SAMPRATE_96000 ||
          a2dp_sample_rate == AUD_SAMPRATE_88200) {
        freq = APP_SYSFREQ_104M;
      } else {
        freq = APP_SYSFREQ_52M;
      }
    }
#endif
#endif
    freq = APP_SYSFREQ_52M;
    app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, freq);
    TRACE_AUD_STREAM_I("[A2DP_PLAYER] sysfreq %d", freq);
    TRACE_AUD_STREAM_I("[A2DP_PLAYER] sysfreq calc : %d\n",
                       hal_sys_timer_calc_cpu_freq(5, 0));

    if (on == PLAYER_OPER_START) {
      af_set_irq_notification(app_bt_stream_playback_irq_notification);
      ASSERT(!app_ring_merge_isrun(),
             "Ring playback will be abnormal, please check.");
      if (0) {
      }
#if defined(A2DP_AAC_ON)
      else if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
        app_overlay_select(APP_OVERLAY_A2DP_AAC);
      }
#endif

      else if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
        TRACE(1, "current_a2dp_non_type %d", current_a2dp_non_type);
#if defined(A2DP_LHDC_ON)
        if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LHDC) {
          app_overlay_select(APP_OVERLAY_A2DP_LHDC);
        }
#endif
#if defined(A2DP_SCALABLE_ON)
        else if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_SCALABLE) {
          app_overlay_select(APP_OVERLAY_A2DP_SCALABLE);
        }
#endif
#if defined(A2DP_LDAC_ON)
        else if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LDAC) {
          TRACE_AUD_STREAM_I("[A2DP_PLAYER] ldac overlay select \n"); // toto
          app_overlay_select(APP_OVERLAY_A2DP_LDAC);
        }
#endif
      } else {
        app_overlay_select(APP_OVERLAY_A2DP);
      }

#ifdef BT_XTAL_SYNC

#ifdef __TWS__
      if (app_tws_mode_is_only_mobile()) {
        btdrv_rf_bit_offset_track_enable(false);
      } else
#endif
      {
        btdrv_rf_bit_offset_track_enable(true);
      }
      bt_init_xtal_sync(BT_XTAL_SYNC_MODE_MUSIC, BT_INIT_XTAL_SYNC_MIN,
                        BT_INIT_XTAL_SYNC_MAX, BT_INIT_XTAL_SYNC_FCAP_RANGE);
#endif // BT_XTAL_SYNC
#ifdef __THIRDPARTY
      app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                               THIRDPARTY_START2MIC);
#endif

      bt_sbc_mode = 1;
    }

#ifdef __THIRDPARTY
    // app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_STOP);
    start_by_sbc = true;
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_START);
#endif
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
    sample_rate = AUD_SAMPRATE_50781;
#else
    sample_rate = a2dp_sample_rate;
#endif

#ifdef PLAYBACK_USE_I2S
    hal_cmu_audio_resample_disable();
#endif

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
#ifdef PLAYBACK_FORCE_48K
    stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#else
    stream_cfg.sample_rate = sample_rate;
#endif

#ifdef PLAYBACK_USE_I2S
    stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
#else
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif

#ifdef PLAYBACK_USE_I2S
    stream_cfg.io_path = AUD_IO_PATH_NULL;
#else
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
#endif

    stream_cfg.vol = stream_local_volume;
    stream_cfg.handler = bt_sbc_player_more_data;

    if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
#if defined(A2DP_SCALABLE_ON)
      if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_SCALABLE) {
        stream_cfg.data_size = SCALABLE_FRAME_SIZE * 8;
      }
#endif
#if defined(A2DP_LHDC_ON)
      if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LHDC) {
#if defined(A2DP_LHDC_V3)
        if (bt_get_sbc_sample_rate() == AUD_SAMPRATE_96000) {
          stream_cfg.data_size = LHDC_AUDIO_96K_BUFF_SIZE;
        } else {
          stream_cfg.data_size = LHDC_AUDIO_BUFF_SIZE;
        }
        if (a2dp_lhdc_config_llc_get()) {
          TRACE_AUD_STREAM_I("[A2DP_PLAYER] USE LHDC_LLC");
          stream_cfg.data_size = LHDC_LLC_AUDIO_BUFF_SIZE;
        }
#else
        stream_cfg.data_size = LHDC_AUDIO_BUFF_SIZE;
#endif
      }
#endif
#if defined(A2DP_LDAC_ON)
      if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LDAC) {
        stream_cfg.data_size = BT_AUDIO_BUFF_SIZE_LDAC;
      }
#endif
    } else

#if defined(A2DP_AAC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
      stream_cfg.data_size = BT_AUDIO_BUFF_AAC_SIZE;
    } else
#endif
    {
      if (stream_cfg.sample_rate == AUD_SAMPRATE_44100) {
        stream_cfg.data_size = BT_AUDIO_BUFF_SBC_44P1K_SIZE;
      } else {
        stream_cfg.data_size = BT_AUDIO_BUFF_SBC_48K_SIZE;
      }
    }

    stream_cfg.bits = AUD_BITS_16;

#ifdef A2DP_EQ_24BIT
    stream_cfg.data_size *= 2;
    stream_cfg.bits = AUD_BITS_24;
#elif defined(A2DP_SCALABLE_ON) || defined(A2DP_LHDC_ON) ||                    \
    defined(A2DP_LDAC_ON)
    if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
      stream_cfg.data_size *= 2;
      stream_cfg.bits = AUD_BITS_24;
    }
#endif

#if 0 // defined(A2DP_LHDC_ON)
        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
        {
            if(bt_sbc_player_get_sample_bit() == AUD_BITS_16)
            {
                stream_cfg.bits = AUD_BITS_16;
            }
        }
#endif

    a2dp_data_buf_size = stream_cfg.data_size;

    app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))
    lowdelay_sample_size_play_bt = stream_cfg.bits;
    lowdelay_sample_rate_play_bt = stream_cfg.sample_rate;
    lowdelay_data_size_play_bt = stream_cfg.data_size;
    lowdelay_playback_ch_num_bt = stream_cfg.channel_num;
#endif

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    uint8_t *promptTmpSourcePcmDataBuf;
    uint8_t *promptTmpTargetPcmDataBuf;
    uint8_t *promptPcmDataBuf;
    uint8_t *promptResamplerBuf;

    app_audio_mempool_get_buff(&promptTmpSourcePcmDataBuf,
                               AUDIO_PROMPT_SOURCE_PCM_BUFFER_SIZE);
    app_audio_mempool_get_buff(&promptTmpTargetPcmDataBuf,
                               AUDIO_PROMPT_TARGET_PCM_BUFFER_SIZE);
    app_audio_mempool_get_buff(&promptPcmDataBuf, AUDIO_PROMPT_PCM_BUFFER_SIZE);
    app_audio_mempool_get_buff(&promptResamplerBuf,
                               AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);

    audio_prompt_buffer_config(MIX_WITH_A2DP_STREAMING, stream_cfg.channel_num,
                               stream_cfg.bits, promptTmpSourcePcmDataBuf,
                               promptTmpTargetPcmDataBuf, promptPcmDataBuf,
                               AUDIO_PROMPT_PCM_BUFFER_SIZE, promptResamplerBuf,
                               AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    sample_size_play_bt = stream_cfg.bits;
    sample_rate_play_bt = stream_cfg.sample_rate;
    data_size_play_bt = stream_cfg.data_size;
    playback_buf_bt = stream_cfg.data_ptr;
    playback_size_bt = stream_cfg.data_size;
    if (sample_rate_play_bt == AUD_SAMPRATE_96000) {
      playback_samplerate_ratio_bt = 4;
    } else {
      playback_samplerate_ratio_bt = 8;
    }
    playback_ch_num_bt = stream_cfg.channel_num;
    mid_p_8_old_l = 0;
    mid_p_8_old_r = 0;
#endif

#ifdef PLAYBACK_FORCE_48K
    force48k_resample = app_force48k_resample_any_open(
        stream_cfg.channel_num, app_force48k_resample_iter,
        stream_cfg.data_size / stream_cfg.channel_num,
        (float)sample_rate / AUD_SAMPRATE_48000);
#endif

    TRACE(4,
          "A2DP Playback: sample rate: %d, bits = %d, channel number = %d, "
          "data size:%d",
          stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num,
          stream_cfg.data_size);

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    stream_cfg.bits = sample_size_play_bt;
    stream_cfg.channel_num = playback_ch_num_bt;
    stream_cfg.sample_rate = sample_rate_play_bt;
    stream_cfg.device = AUD_STREAM_USE_MC;
    stream_cfg.vol = 0;
#ifdef AUDIO_ANC_FB_ADJ_MC
    stream_cfg.handler = audio_adj_mc_data_playback_a2dp;
#else
    stream_cfg.handler = audio_mc_data_playback_a2dp;
#endif
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
    app_audio_mempool_get_buff(
        &bt_audio_buff, data_size_play_bt * playback_samplerate_ratio_bt);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
    stream_cfg.data_size = data_size_play_bt * playback_samplerate_ratio_bt;

    playback_buf_mc = stream_cfg.data_ptr;
    playback_size_mc = stream_cfg.data_size;

    anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

    memset(delay_buf_bt, 0, sizeof(delay_buf_bt));

    af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
    // ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);
#ifdef AUDIO_ANC_FB_ADJ_MC
    adj_mc_capture_sample_rate = sample_rate_play_bt / 3;
    adj_mc_init(ADJ_MC_FRAME_LEN);
    memset(&stream_cfg, 0, sizeof(stream_cfg));
    stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)ADJ_MC_CHANNEL_NUM;
    stream_cfg.data_size = ADJ_MC_BUF_SIZE;
    stream_cfg.sample_rate = (enum AUD_SAMPRATE_T)adj_mc_capture_sample_rate;
    stream_cfg.bits = AUD_BITS_16;
    stream_cfg.vol = 12;
    stream_cfg.chan_sep_buf = true;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_INPUT_PATH_ASRMIC;
    stream_cfg.handler = adj_mc_filter_estimate;
    stream_cfg.data_ptr = adj_mc_buf;
    TRACE(2, "[A2DP_PLAYER] capture sample_rate:%d, data_size:%d",
          stream_cfg.sample_rate, stream_cfg.data_size);
    af_stream_open(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg);
#endif

#endif

#ifdef A2DP_STREAM_AUDIO_DUMP
    audio_dump_init(1024, sizeof(int), 1);
#endif

#ifdef __HEAR_THRU_PEAK_DET__
    PEAK_DETECTOR_CFG_T peak_detector_cfg;
    peak_detector_cfg.fs = stream_cfg.sample_rate;
    peak_detector_cfg.bits = stream_cfg.bits;
    peak_detector_cfg.factor_up = 0.6;
    peak_detector_cfg.factor_down = 2.0;
    peak_detector_cfg.reduce_dB = -30;
    peak_detector_init();
    peak_detector_setup(&peak_detector_cfg);
#endif

#if defined(__AUDIO_SPECTRUM__)
    audio_spectrum_open(stream_cfg.sample_rate, stream_cfg.bits);
#endif

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    eq_buff_size = stream_cfg.data_size * 2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)

    play_samp_size = (stream_cfg.bits <= AUD_BITS_16) ? 2 : 4;
#if defined(CHIP_BEST2000)
    eq_buff_size = stream_cfg.data_size * sizeof(int32_t) / play_samp_size;
#elif defined(CHIP_BEST1000)
    eq_buff_size = stream_cfg.data_size * sizeof(int16_t) / play_samp_size;
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                     \
    defined(CHIP_BEST2300A)
    eq_buff_size = stream_cfg.data_size;
#endif
#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    eq_buff_size = stream_cfg.data_size;
#else
    eq_buff_size = 0;
    bt_eq_buff = NULL;
#endif

    if (eq_buff_size > 0) {
      app_audio_mempool_get_buff(&bt_eq_buff, eq_buff_size);
    }

#if defined(IBRT)
    enum AUD_CHANNEL_NUM_T sw_ch_num = AUD_CHANNEL_NUM_1;
#else
    enum AUD_CHANNEL_NUM_T sw_ch_num = stream_cfg.channel_num;
#endif

    audio_process_open(stream_cfg.sample_rate, stream_cfg.bits, sw_ch_num,
                       stream_cfg.channel_num,
                       stream_cfg.data_size / stream_cfg.channel_num /
                           (stream_cfg.bits <= AUD_BITS_16 ? 2 : 4) / 2,
                       bt_eq_buff, eq_buff_size);

// disable audio eq config on a2dp start for audio tuning tools
#ifndef __PC_CMD_UART__
#ifdef __SW_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR, 0));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR, 0));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR, 0));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR, 0));
#endif
#endif

#ifdef ANC_APP
    anc_status_record = 0xff;
#endif

#if defined(IBRT)
    APP_TWS_IBRT_AUDIO_SYNC_CFG_T sync_config;
    sync_config.factor_reference = TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE;
    sync_config.factor_fast_limit = TWS_IBRT_AUDIO_SYNC_FACTOR_FAST_LIMIT;
    sync_config.factor_slow_limit = TWS_IBRT_AUDIO_SYNC_FACTOR_SLOW_LIMIT;
    ;
    sync_config.dead_zone_us = TWS_IBRT_AUDIO_SYNC_DEAD_ZONE_US;
    app_tws_ibrt_audio_sync_reconfig(&sync_config);
#else
#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
    af_codec_tune(AUD_STREAM_PLAYBACK, 0);
#endif
#endif
    if (on == PLAYER_OPER_START) {
      // This might use all of the rest buffer in the mempool,
      // so it must be the last configuration before starting stream.
#if (A2DP_DECODER_VER == 2)
      A2DP_AUDIO_OUTPUT_CONFIG_T output_config;
      A2DP_AUDIO_CODEC_TYPE a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SBC;
      output_config.sample_rate = sample_rate;
      output_config.num_channels = 2;
#ifdef A2DP_EQ_24BIT
      output_config.bits_depth = 24;
#else
      output_config.bits_depth = 16;
#endif
      output_config.frame_samples = app_bt_stream_get_dma_buffer_samples() / 2;
      output_config.factor_reference = 1.0f;
#if defined(IBRT)
      ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
      uint8_t codec_type = bt_sbc_player_get_codec_type();
      float dest_packet_mut = 0;
      int need_autotrigger =
          a2dp_ibrt_stream_need_autotrigger_getandclean_flag();

      uint32_t offset_mut = 0;
      switch (codec_type) {
      case BTIF_AVDTP_CODEC_TYPE_SBC:
        a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SBC;
        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU;
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
        offset_mut = A2DP_PLAYER_PLAYBACK_DELAY_SBC_FRAME_MTU *
                     A2DP_PLAYER_PLAYBACK_WATER_LINE;
#endif
        break;
      case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
        a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC;
        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU;
#ifdef A2DP_PLAYER_PLAYBACK_WATER_LINE
        offset_mut = A2DP_PLAYER_PLAYBACK_WATER_LINE;
#endif
        break;
      case BTIF_AVDTP_CODEC_TYPE_NON_A2DP:
#if defined(A2DP_LHDC_ON)
        if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LHDC) {
          a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LHDC;
          if (a2dp_lhdc_config_llc_get()) {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU;
          } else if (sample_rate > AUD_SAMPRATE_48000) {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU;
          } else {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU;
          }
        }
#endif
#if defined(A2DP_LDAC_ON)
        if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LDAC) {
          a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LDAC;
          dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU;
        }
#endif

#if defined(A2DP_SCALABLE_ON)
        if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_SCALABLE) {
          a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SCALABL;
          if (sample_rate > AUD_SAMPRATE_48000) {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU;
          } else {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU;
          }
        }
#endif

        break;
      default:
        break;
      }
      output_config.factor_reference = TWS_IBRT_AUDIO_SYNC_FACTOR_REFERENCE;
#if defined(A2DP_LHDC_ON) || defined(A2DP_SCALABLE_ON) || defined(A2DP_LDAC_ON)
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
        if (bt_sbc_player_get_sample_bit() == AUD_BITS_16) {
          output_config.curr_bits = AUD_BITS_16;
        } else {
          output_config.curr_bits = AUD_BITS_24;
        }
      }
#endif

      dest_packet_mut *= a2dp_audio_latency_factor_get();
      A2DP_AUDIO_CHANNEL_SELECT_E a2dp_audio_channel_sel =
          A2DP_AUDIO_CHANNEL_SELECT_STEREO;
      switch ((AUDIO_CHANNEL_SELECT_E)p_ibrt_ctrl->audio_chnl_sel) {
      case AUDIO_CHANNEL_SELECT_STEREO:
        a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_STEREO;
        break;
      case AUDIO_CHANNEL_SELECT_LRMERGE:
        a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_LRMERGE;
        break;
      case AUDIO_CHANNEL_SELECT_LCHNL:
        a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
        break;
      case AUDIO_CHANNEL_SELECT_RCHNL:
        a2dp_audio_channel_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        break;
      default:
        break;
      }
      dest_packet_mut += offset_mut;
      a2dp_audio_init(freq, a2dp_audio_codec_type, &output_config,
                      a2dp_audio_channel_sel, (uint16_t)dest_packet_mut);

      app_tws_ibrt_audio_analysis_interval_set(sample_rate > AUD_SAMPRATE_48000
                                                   ? AUDIO_ANALYSIS_INTERVAL * 2
                                                   : AUDIO_ANALYSIS_INTERVAL);
      if (app_tws_ibrt_mobile_link_connected()) {
        app_tws_ibrt_audio_analysis_start(
            0, AUDIO_ANALYSIS_CHECKER_INTERVEL_INVALID);
        app_tws_ibrt_audio_sync_start();
        app_bt_stream_ibrt_audio_master_detect_next_packet_start();
      } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
        app_bt_stream_ibrt_audio_slave_detect_next_packet_start(
            need_autotrigger);
      } else {
        TRACE_AUD_STREAM_E(
            "[A2DP_PLAYER] mobile_link:%d %04x ibrt_link:%d %04x",
            app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
            app_tws_ibrt_slave_ibrt_link_connected(),
            p_ibrt_ctrl->ibrt_conhandle);
      }
#else
      uint8_t codec_type = bt_sbc_player_get_codec_type();
      uint16_t dest_packet_mut = 0;

      switch (codec_type) {
      case BTIF_AVDTP_CODEC_TYPE_SBC:
        a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SBC;
        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SBC_MTU;
        break;
      case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
        a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC;
        dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_AAC_MTU;
        break;
      case BTIF_AVDTP_CODEC_TYPE_NON_A2DP:
#if defined(A2DP_LHDC_ON)
        if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LHDC) {
          a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LHDC;
          if (a2dp_lhdc_config_llc_get()) {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_LLC_MTU;
          } else if (sample_rate > AUD_SAMPRATE_48000) {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_HIRES_MTU;
          } else {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LHDC_BASERES_MTU;
          }
        }
#endif
#if defined(A2DP_LDAC_ON)
        if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_LDAC) {
          a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_LDAC;
          dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_LDAC_MTU;
        }
#endif
#if defined(A2DP_SCALABLE_ON)
        if (current_a2dp_non_type == A2DP_NON_CODEC_TYPE_SCALABLE) {
          a2dp_audio_codec_type = A2DP_AUDIO_CODEC_TYPE_SCALABL;
          if (sample_rate > AUD_SAMPRATE_48000) {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_HIRES_MTU;
          } else {
            dest_packet_mut = A2DP_PLAYER_PLAYBACK_DELAY_SCALABLE_BASERES_MTU;
          }
        }
#endif

        if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
          if (bt_sbc_player_get_sample_bit() == AUD_BITS_16) {
            output_config.curr_bits = AUD_BITS_16;
          } else {
            output_config.curr_bits = AUD_BITS_24;
          }
        }

        break;
      default:
        break;
      }
#if defined(A2DP_LHDC_ON)
      if (codec_type == BTIF_AVDTP_CODEC_TYPE_NON_A2DP) {
        if (bt_sbc_player_get_sample_bit() == AUD_BITS_16) {
          output_config.curr_bits = AUD_BITS_16;
        } else {
          output_config.curr_bits = AUD_BITS_24;
        }
      }
#endif
      a2dp_audio_init(freq, a2dp_audio_codec_type, &output_config,
                      A2DP_AUDIO_CHANNEL_SELECT_STEREO, dest_packet_mut);
      a2dp_audio_detect_next_packet_callback_register(
          app_bt_stream_detect_next_packet_cb);
#endif
      a2dp_audio_start();
#else
      a2dp_audio_init();
#endif
    }

#if defined(MUSIC_DELAY_CONTROL) &&                                            \
    (defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                      \
     defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                      \
     defined(CHIP_BEST1402))
    calib_reset = 1;
#endif
    af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_codec_set_playback_post_handler(bt_sbc_player_playback_post_handler);
#ifdef __A2DP_PLAYER_USE_BT_TRIGGER__
    app_bt_stream_trigger_init();
#endif
#ifdef VOICE_DATAPATH
    if (app_voicepath_get_stream_state(VOICEPATH_STREAMING)) {
      app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, true);
    } else {
      app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, true);
      af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
#else
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#endif
#ifdef AI_AEC_CP_ACCEL
    cp_aec_init();
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#ifdef AUDIO_ANC_FB_ADJ_MC
    af_stream_start(ADJ_MC_STREAM_ID, AUD_STREAM_CAPTURE);
#endif
#endif

#ifdef WL_DET
    app_mic_alg_audioloop(true, APP_SYSFREQ_78M);
#endif

#ifdef __THIRDPARTY
    // app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_OTHER_EVENT);
#endif
    app_bt_stream_trigger_checker_start();
  }

  isRun = (on != PLAYER_OPER_STOP);
  return 0;
}

#if defined(SCO_DMA_SNAPSHOT)
static uint32_t sco_trigger_wait_codecpcm = 0;
static uint32_t sco_trigger_wait_btpcm = 0;
void app_bt_stream_sco_trigger_set_codecpcm_triggle(uint8_t triggle_en) {
  sco_trigger_wait_codecpcm = triggle_en;
}

uint32_t app_bt_stream_sco_trigger_wait_codecpcm_triggle(void) {
  return sco_trigger_wait_codecpcm;
}

void app_bt_stream_sco_trigger_set_btpcm_triggle(uint32_t triggle_en) {
  sco_trigger_wait_btpcm = triggle_en;
}

uint32_t app_bt_stream_sco_trigger_wait_btpcm_triggle(void) {
  return sco_trigger_wait_btpcm;
}

int app_bt_stream_sco_trigger_codecpcm_tick(void) {
  if (app_bt_stream_sco_trigger_wait_codecpcm_triggle()) {
    app_bt_stream_sco_trigger_set_codecpcm_triggle(0);
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()) {
      TRACE_AUD_STREAM_I("[SCO_PLAYER] codecpcm_tick:%x/%x",
                         btdrv_syn_get_curr_ticks(),
                         bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle));
    } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
      TRACE_AUD_STREAM_I("[SCO_PLAYER] codecpcm_tick:%x/%x",
                         btdrv_syn_get_curr_ticks(),
                         bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle));
    } else {
      TRACE_AUD_STREAM_E(
          "[SCO_PLAYER] codecpcm_tick mobile_link:%d %04x ibrt_link:%d %04x",
          app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
          app_tws_ibrt_slave_ibrt_link_connected(),
          p_ibrt_ctrl->ibrt_conhandle);
    }
#else
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_NUM) {
      conhdl = btif_hf_get_remote_hci_handle(
          *app_audio_manager_get_active_sco_chnl());
    }
    if (conhdl != 0xFFFF) {
      TRACE_AUD_STREAM_I("[SCO_PLAYER] codecpcm_tick:%x",
                         bt_syn_get_curr_ticks(conhdl));
    }
#endif
    btdrv_syn_clr_trigger();
    return 1;
  }
  return 0;
}

int app_bt_stream_sco_trigger_btpcm_tick(void) {
  if (app_bt_stream_sco_trigger_wait_btpcm_triggle()) {
    app_bt_stream_sco_trigger_set_btpcm_triggle(0);
    btdrv_set_bt_pcm_triggler_en(0);
#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()) {
      TRACE_AUD_STREAM_I("[SCO_PLAYER] btpcm_tick:%x/%x",
                         btdrv_syn_get_curr_ticks(),
                         bt_syn_get_curr_ticks(p_ibrt_ctrl->mobile_conhandle));
    } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
      TRACE_AUD_STREAM_I("[SCO_PLAYER] btpcm_tick:tick:%x/%x",
                         btdrv_syn_get_curr_ticks(),
                         bt_syn_get_curr_ticks(p_ibrt_ctrl->ibrt_conhandle));
    } else {
      TRACE_AUD_STREAM_I(
          "[SCO_PLAYER] btpcm_tick: mobile_link:%d %04x ibrt_link:%d %04x",
          app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
          app_tws_ibrt_slave_ibrt_link_connected(),
          p_ibrt_ctrl->ibrt_conhandle);
    }
#else
    uint16_t conhdl = 0xFFFF;
    int curr_sco;

    curr_sco = app_audio_manager_get_active_sco_num();
    if (curr_sco != BT_DEVICE_NUM) {
      conhdl = btif_hf_get_remote_hci_handle(
          *app_audio_manager_get_active_sco_chnl());
    }
    TRACE_AUD_STREAM_I("[SCO_PLAYER] btpcm_tick:%x",
                       bt_syn_get_curr_ticks(conhdl));
#endif
    btdrv_syn_clr_trigger();
    return 1;
  }
  return 0;
}

void app_bt_stream_sco_trigger_btpcm_start(void) {
  uint32_t curr_ticks = 0;
  uint32_t tg_acl_trigger_offset_time = 0;
  uint16_t conhdl = 0xFFFF;

  uint32_t lock;

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (app_tws_ibrt_mobile_link_connected()) {
    conhdl = p_ibrt_ctrl->mobile_conhandle;
#ifdef __SW_TRIG__
    btdrv_sync_sw_trig_store_conhdl(p_ibrt_ctrl->mobile_conhandle);
#endif
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    conhdl = p_ibrt_ctrl->ibrt_conhandle;
#ifdef __SW_TRIG__
    btdrv_sync_sw_trig_store_conhdl(p_ibrt_ctrl->ibrt_conhandle);
#endif
  } else {
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][SCO] btpcm_startr mobile:%d %04x ibrt:%d %04x",
        app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
        app_tws_ibrt_slave_ibrt_link_connected(), p_ibrt_ctrl->ibrt_conhandle);
  }
#else
  int curr_sco;
  curr_sco = app_audio_manager_get_active_sco_num();
  if (curr_sco != BT_DEVICE_NUM) {
    conhdl =
        btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
  }
#endif

  lock = int_lock();
  curr_ticks = bt_syn_get_curr_ticks(conhdl);

  tg_acl_trigger_offset_time =
      (curr_ticks + 0x180) - ((curr_ticks + 0x180) % 192);

  btdrv_set_bt_pcm_triggler_en(0);
  btdrv_set_bt_pcm_en(0);
  btdrv_syn_clr_trigger();
  btdrv_enable_playback_triggler(SCO_TRIGGLE_MODE);

#if defined(IBRT)
  if (app_tws_ibrt_mobile_link_connected()) {
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time,
                        p_ibrt_ctrl->mobile_conhandle, BT_TRIG_SLAVE_ROLE);
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_startr set ticks:%d,",
                       tg_acl_trigger_offset_time);
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, p_ibrt_ctrl->ibrt_conhandle,
                        BT_TRIG_SLAVE_ROLE);
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_startr set ticks:%d,",
                       tg_acl_trigger_offset_time);
  } else {
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][SCO] btpcm_startr mobile:%d %04x ibrt:%d %04x",
        app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
        app_tws_ibrt_slave_ibrt_link_connected(), p_ibrt_ctrl->ibrt_conhandle);
  }
  TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_startr get ticks:%d,", curr_ticks);

#else
  bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, conhdl, BT_TRIG_SLAVE_ROLE);
#endif
  btdrv_set_bt_pcm_triggler_en(1);
  btdrv_set_bt_pcm_en(1);
  app_bt_stream_sco_trigger_set_btpcm_triggle(1);
  TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] btpcm_startr curr:%x, trig:%x, curr:%x",
                     btdrv_syn_get_curr_ticks(), tg_acl_trigger_offset_time,
                     bt_syn_get_curr_ticks(conhdl));
  int_unlock(lock);
}

void app_bt_stream_sco_trigger_btpcm_stop(void) { return; }

#define TIRG_DELAY_THRESHOLD_325US                                             \
  (15)                      // total:TIRG_DELAY_THRESHOLD_325US*325us
#define TIRG_DELAY_MAX (20) // total:20*TIRG_DELAY_325US*325us

#define TIRG_DELAY_325US                                                       \
  (96) // total:TIRG_DELAY_325US*325us It' up to the codec and bt pcm pingpang
       // buffer.

void app_bt_stream_sco_trigger_codecpcm_start(uint32_t btclk, uint16_t btcnt) {
  uint32_t curr_ticks = 0;
  uint32_t tg_acl_trigger_offset_time = 0;
  uint32_t lock;
  uint16_t conhdl = 0xFFFF;
  // must lock the interrupts when set trig ticks.

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (app_tws_ibrt_mobile_link_connected()) {
    conhdl = p_ibrt_ctrl->mobile_conhandle;
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    conhdl = p_ibrt_ctrl->ibrt_conhandle;
  } else {
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][SCO] codecpcm_start mobile_link:%d %04x ibrt_link:%d %04x",
        app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
        app_tws_ibrt_slave_ibrt_link_connected(), p_ibrt_ctrl->ibrt_conhandle);
  }
#else
  int curr_sco;
  curr_sco = app_audio_manager_get_active_sco_num();
  if (curr_sco != BT_DEVICE_NUM) {
    conhdl =
        btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
  }
#endif

  lock = int_lock();
  curr_ticks = bt_syn_get_curr_ticks(conhdl);
  TRACE_AUD_STREAM_I("[[STRM_TRIG][SCO] codecpcm_start 1 curr:%d clk:%dcnt:%d",
                     curr_ticks, btclk, btcnt);
#ifdef LOW_DELAY_SCO
  tg_acl_trigger_offset_time = btclk + 12 + MASTER_MOBILE_BTCLK_OFFSET;
#else
  tg_acl_trigger_offset_time = btclk + 24 + MASTER_MOBILE_BTCLK_OFFSET;
#endif

  tg_acl_trigger_offset_time = tg_acl_trigger_offset_time * 2;
  if (tg_acl_trigger_offset_time < curr_ticks + TIRG_DELAY_THRESHOLD_325US) {
    int tirg_delay = 0;
    tirg_delay = ((curr_ticks + TIRG_DELAY_THRESHOLD_325US) -
                  tg_acl_trigger_offset_time) /
                 TIRG_DELAY_325US;
    tirg_delay = tirg_delay + 1;
    if (tirg_delay > TIRG_DELAY_MAX) {
      tirg_delay = TIRG_DELAY_MAX;
      TRACE_AUD_STREAM_W("[STRM_TRIG][SCO] codecpcm_start bt clk convolution!");
    }
    tg_acl_trigger_offset_time =
        tg_acl_trigger_offset_time + tirg_delay * TIRG_DELAY_325US;
    TRACE_AUD_STREAM_W("[STRM_TRIG][SCO] codecpcm_start need more "
                       "tirg_delay:%d offset:%d curr:%d,",
                       tirg_delay, tg_acl_trigger_offset_time, curr_ticks);
  }
  tg_acl_trigger_offset_time &= 0x0fffffff;

  btdrv_syn_trigger_codec_en(0);
  //    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
  //    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
  btdrv_syn_clr_trigger();

#if defined(IBRT)
  if (app_tws_ibrt_mobile_link_connected()) {
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time,
                        p_ibrt_ctrl->mobile_conhandle, BT_TRIG_SLAVE_ROLE);
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] codecpcm_start set 2 offset:%d",
                       tg_acl_trigger_offset_time);
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, p_ibrt_ctrl->ibrt_conhandle,
                        BT_TRIG_SLAVE_ROLE);
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] codecpcm_start set 2 offset:%d",
                       tg_acl_trigger_offset_time);
  } else {
    TRACE_AUD_STREAM_I(
        "[STRM_TRIG][SCO] codecpcm_start mobile_link:%d %04x ibrt_link:%d %04x",
        app_tws_ibrt_mobile_link_connected(), p_ibrt_ctrl->mobile_conhandle,
        app_tws_ibrt_slave_ibrt_link_connected(), p_ibrt_ctrl->ibrt_conhandle);
  }
#else
  bt_syn_set_tg_ticks(tg_acl_trigger_offset_time, conhdl, BT_TRIG_SLAVE_ROLE);
#endif
  btdrv_syn_trigger_codec_en(1);
  app_bt_stream_sco_trigger_set_codecpcm_triggle(1);

  btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

  int_unlock(lock);

  TRACE_AUD_STREAM_I(
      "[STRM_TRIG][SCO] codecpcm_start enable curr:%x trig:%x curr:%x",
      btdrv_syn_get_curr_ticks(), tg_acl_trigger_offset_time, curr_ticks);

  //    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
  //    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);
}

void app_bt_stream_sco_trigger_codecpcm_stop(void) {
#ifdef PLAYBACK_USE_I2S
  af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, false);
#else
  af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, false);
#endif
  af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
}
#endif

void speech_tx_aec_set_frame_len(int len);
int voicebtpcm_pcm_echo_buf_queue_init(uint32_t size);
void voicebtpcm_pcm_echo_buf_queue_reset(void);
void voicebtpcm_pcm_echo_buf_queue_deinit(void);
int voicebtpcm_pcm_audio_init(int sco_sample_rate, int codec_sample_rate);
int voicebtpcm_pcm_audio_deinit(void);
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len);
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len);
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);
static uint32_t mic_force_mute = 0;
static uint32_t spk_force_mute = 0;
static uint32_t bt_sco_player_code_type = 0;

static enum AUD_CHANNEL_NUM_T sco_play_chan_num;
static enum AUD_CHANNEL_NUM_T sco_cap_chan_num;

bool bt_sco_codec_is_msbc(void) {
  bool en = false;
#ifdef HFP_1_6_ENABLE
  if (app_audio_manager_get_scocodecid() == BTIF_HF_SCO_CODEC_MSBC) {
    en = true;
  } else
#endif
  {
    en = false;
  }

  return en;
}

void bt_sco_mobile_clkcnt_get(uint32_t btclk, uint16_t btcnt,
                              uint32_t *mobile_master_clk,
                              uint16_t *mobile_master_cnt) {
#if defined(IBRT)
  app_tws_ibrt_audio_mobile_clkcnt_get(btclk, btcnt, mobile_master_clk,
                                       mobile_master_cnt);
#else
  uint16_t conhdl = 0xFFFF;
  int32_t clock_offset;
  uint16_t bit_offset;
  int curr_sco;

  curr_sco = app_audio_manager_get_active_sco_num();
  if (curr_sco != BT_DEVICE_NUM) {
    conhdl =
        btif_hf_get_remote_hci_handle(*app_audio_manager_get_active_sco_chnl());
  }

  if (conhdl != 0xFFFF) {
    bt_drv_reg_op_piconet_clk_offset_get(conhdl, &clock_offset, &bit_offset);
    // TRACE_AUD_STREAM_I("mobile piconet clk:%d bit:%d loc clk:%d cnt:%d",
    // clock_offset, bit_offset, btclk, btcnt);
    btdrv_slave2master_clkcnt_convert(btclk, btcnt, clock_offset, bit_offset,
                                      mobile_master_clk, mobile_master_cnt);
  } else {
    TRACE_AUD_STREAM_W(
        "[STRM_TRIG][SCO] mobile_clkcnt_get warning conhdl NULL conhdl:%x",
        conhdl);
    *mobile_master_clk = 0;
    *mobile_master_cnt = 0;
  }
#endif
}

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)

#ifdef CHIP_BEST1000
#error "Unsupport SW_SCO_RESAMPLE on best1000 by now"
#endif
#ifdef NO_SCO_RESAMPLE
#error "Conflicted config: NO_SCO_RESAMPLE and SW_SCO_RESAMPLE"
#endif

// The decoded playback data in the first irq is output to DAC after the second
// irq (PING-PONG buffer)
#define SCO_PLAY_RESAMPLE_ALIGN_CNT 2

static uint8_t sco_play_irq_cnt;
static bool sco_dma_buf_err;
static struct APP_RESAMPLE_T *sco_capture_resample;
static struct APP_RESAMPLE_T *sco_playback_resample;

static int bt_sco_capture_resample_iter(uint8_t *buf, uint32_t len) {
  voicebtpcm_pcm_audio_data_come(buf, len);
  return 0;
}

static int bt_sco_playback_resample_iter(uint8_t *buf, uint32_t len) {
  voicebtpcm_pcm_audio_more_data(buf, len);
  return 0;
}

#endif
#if defined(SCO_DMA_SNAPSHOT)
extern int process_downlink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len,
                                            uint8_t *out_buf, uint32_t out_len,
                                            int32_t codec_type);
extern int process_uplink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len,
                                          uint8_t *ref_buf, uint32_t ref_len,
                                          uint8_t *out_buf, uint32_t out_len,
                                          int32_t codec_type);
#define MSBC_FRAME_LEN (60)
#define PCM_LEN_PER_FRAME (240)
#define CAL_FRAME_NUM (22)

static void bt_sco_codec_tuning(void) {
  uint32_t btclk;
  uint16_t btcnt;

  uint32_t mobile_master_clk;
  uint16_t mobile_master_cnt;

  uint32_t mobile_master_clk_offset;
  int32_t mobile_master_cnt_offset;

  static float fre_offset = 0.0f;
  static int32_t mobile_master_cnt_offset_init;
  static int32_t mobile_master_cnt_offset_old;
  static uint32_t first_proc_flag = 0;
#if defined(__AUDIO_RESAMPLE__) && !defined(AUDIO_RESAMPLE_ANTI_DITHER)
  static uint32_t frame_counter = 0;
  static int32_t mobile_master_cnt_offset_max = 0;
  static int32_t mobile_master_cnt_offset_min = 0;
  static int32_t mobile_master_cnt_offset_resample = 0;

  int32_t offset_max = 0;
  int32_t offset_min = 0;
#endif

  bt_drv_reg_op_dma_tc_clkcnt_get(&btclk, &btcnt);
  bt_sco_mobile_clkcnt_get(btclk, btcnt, &mobile_master_clk,
                           &mobile_master_cnt);

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
  TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune btclk:%d,btcnt:%d,",
                     mobile_master_clk, mobile_master_cnt);
#endif

  mobile_master_clk_offset =
      (mobile_master_clk - mobile_master_clk_offset_init) % 12;
  mobile_master_cnt_offset =
      mobile_master_clk_offset * 625 + (625 - mobile_master_cnt);
  mobile_master_cnt_offset =
      mobile_master_cnt_offset -
      (MASTER_MOBILE_BTCNT_OFFSET + mobile_master_cnt_offset_init);

  if (app_bt_stream_sco_trigger_codecpcm_tick()) {
    fre_offset = 0.0f;
    if (mobile_master_clk_offset < MASTER_MOBILE_BTCLK_OFFSET) {
      mobile_master_cnt_offset_init = -mobile_master_cnt;
    } else {
      mobile_master_cnt_offset_init = 625 - mobile_master_cnt;
    }

    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune first_cnt:%d offset_init:%d,",
                       mobile_master_cnt, mobile_master_cnt_offset_init);

    if (playback_samplerate_codecpcm == AUD_SAMPRATE_16000) {
#ifdef ANC_APP
      mobile_master_cnt_offset_init = -232;
#else
#if defined(__AUDIO_RESAMPLE__)
      if (hal_cmu_get_audio_resample_status()) {
#if defined(AUDIO_RESAMPLE_ANTI_DITHER)
        mobile_master_cnt_offset_init = 171;
#else
        mobile_master_cnt_offset_init = 146;
#endif
      } else
#endif
      {
        mobile_master_cnt_offset_init = 113;
      }
#endif
    } else if (playback_samplerate_codecpcm == AUD_SAMPRATE_8000) {
#ifdef ANC_APP
      mobile_master_cnt_offset_init = -512;
#else
#if defined(__AUDIO_RESAMPLE__)
      if (hal_cmu_get_audio_resample_status()) {
#if defined(AUDIO_RESAMPLE_ANTI_DITHER)
        mobile_master_cnt_offset_init = -270;
#else
        mobile_master_cnt_offset_init = -327;
#endif
      } else
#endif
      {
        mobile_master_cnt_offset_init = -386;
      }
#endif
    }

#if defined(__AUDIO_RESAMPLE__) && !defined(AUDIO_RESAMPLE_ANTI_DITHER)
    mobile_master_cnt_offset =
        mobile_master_clk_offset * 625 + (625 - mobile_master_cnt);
    mobile_master_cnt_offset =
        mobile_master_cnt_offset -
        (MASTER_MOBILE_BTCNT_OFFSET + mobile_master_cnt_offset_init);
    TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune mobile_master_cnt_offset:%d,",
                       mobile_master_cnt_offset);
#ifdef LOW_DELAY_SCO
    fre_offset =
        (float)mobile_master_cnt_offset / (CAL_FRAME_NUM * 7.5f * 1000.0f);
#else
    fre_offset =
        (float)mobile_master_cnt_offset / (CAL_FRAME_NUM * 15.0f * 1000.0f);
#endif
    first_proc_flag = 0;
#endif

#if defined(__AUDIO_RESAMPLE__) && !defined(AUDIO_RESAMPLE_ANTI_DITHER)
    if (hal_cmu_get_audio_resample_status()) {
      frame_counter = 0;
      mobile_master_cnt_offset_max = 0;
      mobile_master_cnt_offset_min = 0;
      mobile_master_cnt_offset_resample = 0;
    }
#endif
    mobile_master_cnt_offset = 0;
    mobile_master_cnt_offset_old = 0;
  }

#if defined(__AUDIO_RESAMPLE__) && !defined(SW_PLAYBACK_RESAMPLE) &&           \
    !defined(AUDIO_RESAMPLE_ANTI_DITHER)
  if (hal_cmu_get_audio_resample_status()) {
    if (playback_samplerate_codecpcm == AUD_SAMPRATE_16000) {
      offset_max = 28;
      offset_min = -33;
    } else if (playback_samplerate_codecpcm == AUD_SAMPRATE_8000) {
      offset_max = 12;
      offset_min = -112;
    }

    if (mobile_master_cnt_offset > mobile_master_cnt_offset_max) {
      mobile_master_cnt_offset_max = mobile_master_cnt_offset;
    }

    if (mobile_master_cnt_offset < mobile_master_cnt_offset_min) {
      mobile_master_cnt_offset_min = mobile_master_cnt_offset;
    }

    frame_counter++;

    if (frame_counter >= CAL_FRAME_NUM) {
      if (mobile_master_cnt_offset_min < offset_min) {
        mobile_master_cnt_offset_resample =
            mobile_master_cnt_offset_min - offset_min;
      } else if (mobile_master_cnt_offset_max > offset_max) {
        mobile_master_cnt_offset_resample =
            mobile_master_cnt_offset_max - offset_max;
      } else {
        mobile_master_cnt_offset_resample = 0;
      }
      TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune mobile_master_cnt_offset:%d/%d",
                         mobile_master_cnt_offset_min,
                         mobile_master_cnt_offset_max);
      mobile_master_cnt_offset = mobile_master_cnt_offset_resample;

      if (first_proc_flag == 0) {
        fre_offset = ((int32_t)(mobile_master_cnt_offset * 0.5f)) * 0.0000001f +
                     (mobile_master_cnt_offset - mobile_master_cnt_offset_old) *
                         0.0000001f;
        first_proc_flag = 1;
      } else {
        fre_offset = fre_offset +
                     ((int32_t)(mobile_master_cnt_offset * 0.5f)) * 0.0000001f +
                     (mobile_master_cnt_offset - mobile_master_cnt_offset_old) *
                         0.0000001f;
        first_proc_flag = 1;
      }

      mobile_master_cnt_offset_old = mobile_master_cnt_offset;
#if defined(SCO_DMA_SNAPSHOT_DEBUG)
      TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune mobile_master_cnt_offset:%d",
                         mobile_master_cnt_offset);
#endif
      mobile_master_cnt_offset_max = 0;
      mobile_master_cnt_offset_min = 0;
      frame_counter = 0;
    }
  } else
#endif
  {
    fre_offset =
        fre_offset +
        ((int32_t)(mobile_master_cnt_offset * 0.5f)) * 0.00000001f +
        (mobile_master_cnt_offset - mobile_master_cnt_offset_old) * 0.00000001f;

    mobile_master_cnt_offset_old = mobile_master_cnt_offset;
    // TRACE_AUD_STREAM_I("mobile_master_cnt_offset:%d",mobile_master_cnt_offset);
    first_proc_flag = 1;
  }

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
  TRACE_AUD_STREAM_I("[STRM_TRIG][SCO] tune fre_offset:%d",
                     (int)(fre_offset * 10000000.0f));
#endif
  if (first_proc_flag == 1) {
    if (fre_offset > 0.0001f)
      fre_offset = 0.0001f;
    if (fre_offset < -0.0001f)
      fre_offset = -0.0001f;
  }
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
  app_resample_tune(playback_samplerate_codecpcm, fre_offset);
#else
  af_codec_tune(AUD_STREAM_NUM, fre_offset);
#endif

  return;
}
#endif
extern CQueue *get_tx_esco_queue_ptr();

#if defined(BONE_SENSOR_TDM)
#define MIC_TDM_FRAME_MS (15)
#define MIC_TDM_MAX_CH (3)
static int16_t mic_tdm_buf[SPEECH_FRAME_MS_TO_LEN(16000, MIC_TDM_FRAME_MS)];

// forward
static uint32_t bt_sco_codec_capture_data(uint8_t *buf, uint32_t len);

uint32_t tdm_callback_func(uint8_t *buf_ptr, uint32_t frame_len) {
  int16_t *pcm_buf = (int16_t *)buf_ptr;

  // TODO: Check frame_len, depend on sample rate

  for (uint32_t i = 0; i < frame_len; i++) {
    mic_tdm_buf[i] = pcm_buf[i];
  }

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
  ASSERT(0,
         "[%s] Do not support defined(__AUDIO_RESAMPLE__) && "
         "defined(SW_SCO_RESAMPLE)",
         __func__);
#endif

  return 0;
}

void bt_sco_get_tdm_buffer(uint8_t **buf, uint32_t *len) {
  *buf = (uint8_t *)&mic_tdm_buf[0];
  *len = sizeof(mic_tdm_buf);
}
#endif

#if defined(ANC_NOISE_TRACKER)
static int16_t *anc_buf = NULL;
#endif

#if defined(ANC_WNR_ENABLED)
#if defined(SPEECH_TX_24BIT)
static int32_t wnr_buf[256 * 2];
#else
static short wnr_buf[256 * 2];
#endif
#endif
// #define BT_SCO_HANDLER_PROFILE

//( codec:mic-->btpcm:tx
// codec:mic
static uint32_t bt_sco_codec_capture_data(uint8_t *buf, uint32_t len) {
  app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_AUDPCM_CAPTURE);

#if defined(ANC_NOISE_TRACKER)
  int16_t *pcm_buf = (int16_t *)buf;
  uint32_t pcm_len = len / sizeof(short);
  uint32_t ch_num =
      SPEECH_CODEC_CAPTURE_CHANNEL_NUM + ANC_NOISE_TRACKER_CHANNEL_NUM;
  uint32_t remain_ch_num = SPEECH_CODEC_CAPTURE_CHANNEL_NUM;

#if defined(SPEECH_TX_AEC_CODEC_REF)
  ch_num += 1;
  remain_ch_num += 1;
#endif

  ASSERT(pcm_len % ch_num == 0, "[%s] input data length error", __FUNCTION__);

  // assume anc mic in ch0
  for (uint32_t i = 0, j = 0; i < pcm_len;
       i += ch_num, j += ANC_NOISE_TRACKER_CHANNEL_NUM) {
    for (uint32_t ch = 0; ch < ANC_NOISE_TRACKER_CHANNEL_NUM; ch++)
      anc_buf[j + ch] = pcm_buf[i + ch];
  }

  noise_tracker_process(anc_buf,
                        pcm_len / ch_num * ANC_NOISE_TRACKER_CHANNEL_NUM);

  for (uint32_t i = 0, j = 0; i < pcm_len; i += ch_num, j += remain_ch_num) {
    for (uint32_t chi = ANC_NOISE_TRACKER_CHANNEL_NUM, cho = 0; chi < ch_num;
         chi++, cho++)
      pcm_buf[j + cho] = pcm_buf[i + chi];
  }

  len = len / ch_num * remain_ch_num;
#endif

#if defined(AF_ADC_I2S_SYNC)
  // TRACE_AUD_STREAM_I("[SCO][MIC] cnt = %d", codec_capture_cnt++);
#endif

#if defined(ANC_WNR_ENABLED)

#if defined(SPEECH_TX_24BIT)
  int32_t *pcm_buf = (int32_t *)buf;
  uint32_t pcm_len = len / sizeof(int32_t);
#else
  int16_t *pcm_buf = (int16_t *)buf;
  uint32_t pcm_len = len / sizeof(short);
#endif

  for (uint32_t i = 0; i < pcm_len / sco_cap_chan_num; i++) {
    wnr_buf[2 * i] = pcm_buf[sco_cap_chan_num * i];
    wnr_buf[2 * i + 1] = pcm_buf[sco_cap_chan_num * i + 1];
  }

  // TRACE("sco_cap_chan_num=%d",sco_cap_chan_num);
  if (app_anc_work_status()) {
    // 2ch, interleave, 24bits
    anc_wnr_process(wnr_buf, pcm_len * 2 / sco_cap_chan_num);
  }
#endif

  if (mic_force_mute || btapp_hfp_mic_need_skip_frame() ||
      btapp_hfp_need_mute()) {
    memset(buf, 0, len);
  }

#if defined(SCO_DMA_SNAPSHOT)

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

  int pingpang;

  // processing  ping pang flag
  if (buf == capture_buf_codecpcm) {
    pingpang = 0;
  } else {
    pingpang = 1;
  }

  uint16_t *playback_dst =
      (uint16_t *)(playback_buf_btpcm + (pingpang)*playback_size_btpcm / 2);
  uint16_t *playback_src = (uint16_t *)playback_buf_btpcm_cache;

  for (uint32_t i = 0; i < playback_size_btpcm / 4; i++) {
    playback_dst[i] = playback_src[i];
  }
#ifdef TX_RX_PCM_MASK
  // processing btpcm.(It must be from CPU's copy )
  if (btdrv_is_pcm_mask_enable() == 1 && bt_sco_codec_is_msbc()) {
    uint32_t lock;
    uint32_t i;
    // must lock the interrupts when exchanging data.
    lock = int_lock();
    uint16_t *playback_src =
        (uint16_t *)(playback_buf_btpcm + (pingpang)*playback_size_btpcm / 2);
    for (i = 0; i < playback_size_btpcm_copy; i++) {
      playback_buf_btpcm_copy[i] = (uint8_t)(playback_src[i] >> 8);
    }
    int_unlock(lock);
  }
#endif

  // TRACE_AUD_STREAM_I("pcm length:%d",len);

  // processing clock
  bt_sco_codec_tuning();

  // processing mic
  uint8_t *capture_pcm_frame_p =
      capture_buf_codecpcm + pingpang * (capture_size_codecpcm) / 2;
  // uint8_t *dst=(uint8_t
  // *)(playback_buf_btpcm+(pingpang)*playback_size_btpcm/2);
  uint8_t *dst = playback_buf_btpcm_cache;
  uint8_t *ref_pcm_frame_p =
      playback_buf_codecpcm + (pingpang ^ 1) * (playback_size_codecpcm) / 2;

#if defined(HFP_1_6_ENABLE)
  process_uplink_bt_voice_frames(capture_pcm_frame_p, len, ref_pcm_frame_p,
                                 (playback_size_codecpcm) / 2, dst, len,
                                 app_audio_manager_get_scocodecid());
#else
  process_uplink_bt_voice_frames(capture_pcm_frame_p, len, ref_pcm_frame_p,
                                 (playback_size_codecpcm) / 2, dst, len,
                                 BTIF_HF_SCO_CODEC_CVSD);
#endif

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t end_ticks = hal_fast_sys_timer_get();
  TRACE_AUD_STREAM_I("[SCO][MIC] takes %d us",
                     FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

  return len;

#else

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

#if defined(BONE_SENSOR_TDM)
  int16_t *pcm_buf = (int16_t *)buf;
  uint32_t frame_len = len / sizeof(short) / SPEECH_CODEC_CAPTURE_CHANNEL_NUM;
  uint32_t ch_num = SPEECH_CODEC_CAPTURE_CHANNEL_NUM + 1;

  ASSERT(SPEECH_CODEC_CAPTURE_CHANNEL_NUM < MIC_TDM_MAX_CH,
         "[%s] SPEECH_CODEC_CAPTURE_CHANNEL_NUM(%d) is invalid", __func__,
         SPEECH_CODEC_CAPTURE_CHANNEL_NUM);
  // TODO: Check len, depend on sample rate

  for (uint32_t ch = 0; ch < SPEECH_CODEC_CAPTURE_CHANNEL_NUM; ch++) {
    for (uint32_t i = 0; i < frame_len; i++) {
      mic_tdm_buf[ch_num * i + ch] =
          pcm_buf[SPEECH_CODEC_CAPTURE_CHANNEL_NUM * i + ch];
    }
  }

  return len;
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
  if (hal_cmu_get_audio_resample_status()) {
    if (af_stream_buffer_error(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE)) {
      sco_dma_buf_err = true;
    }
    // The decoded playback data in the first irq is output to DAC after the
    // second irq (PING-PONG buffer), so it is aligned with the capture data
    // after 2 playback irqs.
    if (sco_play_irq_cnt < SCO_PLAY_RESAMPLE_ALIGN_CNT) {
      // Skip processing
      return len;
    }
    app_capture_resample_run(sco_capture_resample, buf, len);
  } else
#endif
  {
    voicebtpcm_pcm_audio_data_come(buf, len);
  }

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t end_ticks = hal_fast_sys_timer_get();
  TRACE_AUD_STREAM_I("[SCO][MIC] takes %d us",
                     FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

  return len;
#endif
}

#ifdef _SCO_BTPCM_CHANNEL_
// btpcm:tx
static uint32_t bt_sco_btpcm_playback_data(uint8_t *buf, uint32_t len) {
  app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_BTPCM_PLAYERBLACK);

#if defined(SCO_DMA_SNAPSHOT)
  return len;
#else

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

  get_voicebtpcm_p2m_frame(buf, len);

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t end_ticks = hal_fast_sys_timer_get();
  TRACE_AUD_STREAM_I("[SCO][SPK] takes %d us",
                     FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

  return len;

#endif
}
//)

#if defined(AF_ADC_I2S_SYNC)
void codec_capture_i2s_enable(void) {
  uint32_t lock;

  TRACE_AUD_STREAM_I("[SCO][IIS] Start...", __func__);

  lock = int_lock();
  hal_codec_capture_enable();
  hal_i2s_enable(HAL_I2S_ID_0);
  int_unlock(lock);
}
#endif

extern CQueue *get_rx_esco_queue_ptr();

static volatile bool is_codec_stream_started = false;
#ifdef PCM_PRIVATE_DATA_FLAG

void bt_sco_btpcm_get_pcm_priv_data(struct PCM_DATA_FLAG_T *pcm_data,
                                    uint8_t *buf, uint32_t len) {
  uint8_t frame_num = len / 120;
  for (uint8_t i = 0; i < frame_num; i++) {
    uint8_t head_pos = 120 * i;
    pcm_data[i].undef = buf[head_pos];
    pcm_data[i].bitcnt = (buf[head_pos + 2] | (buf[head_pos + 4] << 8)) & 0x3ff;
    pcm_data[i].softbit_flag = (buf[head_pos + 4] >> 5) & 3;
    pcm_data[i].btclk = buf[head_pos + 6] | (buf[head_pos + 8] << 8) |
                        (buf[head_pos + 10] << 16) | (buf[head_pos + 12] << 24);
    pcm_data[i].reserved = buf[head_pos + 14] | (buf[head_pos + 16] << 8) |
                           (buf[head_pos + 18] << 16) |
                           (buf[head_pos + 20] << 24);
    // clear private msg in buffer
    for (uint8_t j = 0; j < PCM_PRIVATE_DATA_LENGTH; j++)
      buf[head_pos + 2 * j] = 0;
  }
}
#endif
//( btpcm:rx-->codec:spk
// btpcm:rx
extern void bt_drv_reg_op_set_music_ongong_flag();
extern void bt_drv_reg_op_clear_music_ongong_flag();
static uint32_t bt_sco_btpcm_capture_data(uint8_t *buf, uint32_t len) {
  int POSSIBLY_UNUSED sRet = 0;

  app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_BTPCM_CAPTURE);

#ifdef __BT_ONE_BRING_TWO__
  if (a2dp_is_music_ongoing())
    bt_drv_reg_op_set_music_ongong_flag();
  else
    bt_drv_reg_op_clear_music_ongong_flag();
#endif
#if defined(PCM_PRIVATE_DATA_FLAG) && defined(PCM_FAST_MODE)
  bt_sco_btpcm_get_pcm_priv_data(pcm_data_param, buf, len);

#endif

#if defined(SCO_DMA_SNAPSHOT)
  uint32_t btclk;
  uint16_t btcnt;

  uint32_t mobile_master_clk;
  uint16_t mobile_master_cnt;

  bool codec_stream_trig = false;

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

  if ((is_codec_stream_started == false) && (buf == capture_buf_btpcm)) {
    if (!af_stream_buffer_error(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE)) {
      bt_drv_reg_op_dma_tc_clkcnt_get(&btclk, &btcnt);
      bt_sco_mobile_clkcnt_get(btclk, btcnt, &mobile_master_clk,
                               &mobile_master_cnt);
      hal_sys_timer_delay_us(1);
      if (!af_stream_buffer_error(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE)) {
        codec_stream_trig = true;
      }
    }
  }
  /*
      uint32_t curr_ticks;

      ibbt_ctrl_t *p_ibbt_ctrl = app_tws_ibbt_get_bt_ctrl_ctx();
      if (app_tws_ibbt_tws_link_connected()){
          curr_ticks = bt_syn_get_curr_ticks(IBBT_MASTER ==
     p_ibbt_ctrl->current_role ? p_ibbt_ctrl->mobile_conhandle :
     p_ibbt_ctrl->ibbt_conhandle); TRACE_AUD_STREAM_I("bt_sco_btpcm_capture_data
     +++++++++++++++++++++++++++++++++curr_ticks:%d,",curr_ticks); }else{
          curr_ticks = btdrv_syn_get_curr_ticks();
       TRACE_AUD_STREAM_I("--------------------------------------");
      }
  */
  sco_btpcm_mute_flag = 0;

  if (codec_stream_trig) {
    if (app_bt_stream_sco_trigger_btpcm_tick()) {
      af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
      af_stream_dma_tc_irq_disable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
      af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
#endif
      TRACE_AUD_STREAM_I("[SCO][BTPCMRX] buf:%p,capture_buf_btpcm:%p", buf,
                         capture_buf_btpcm);

      // uint16_t *source=(uint16_t *)buf;
      // DUMP16("%02x,", source, MSBC_FRAME_LEN);

#if defined(BONE_SENSOR_TDM)
#if defined(AF_ADC_I2S_SYNC)
      hal_i2s_enable_delay(HAL_I2S_ID_0);
      // hal_codec_capture_enable_delay();
#endif
      tdm_stream_start();
#endif
      mobile_master_clk_offset_init = mobile_master_clk % 12;
      app_bt_stream_sco_trigger_codecpcm_start(mobile_master_clk,
                                               mobile_master_cnt);
      is_codec_stream_started = true;

#if defined(SCO_DMA_SNAPSHOT_DEBUG)
      TRACE_AUD_STREAM_I("[SCO][BTPCMRX] btclk:%d,btcnt:%d,", mobile_master_clk,
                         mobile_master_cnt);
#endif
    }
  } else {
#if defined(SCO_DMA_SNAPSHOT_DEBUG)
    bt_drv_reg_op_dma_tc_clkcnt_get(&btclk, &btcnt);
    bt_sco_mobile_clkcnt_get(btclk, btcnt, &mobile_master_clk,
                             &mobile_master_cnt);
    TRACE_AUD_STREAM_I("[SCO][BTPCMRX]:btclk:%d,btcnt:%d,", mobile_master_clk,
                       mobile_master_cnt);
#endif
  }

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t end_ticks = hal_fast_sys_timer_get();
  TRACE_AUD_STREAM_I("[SCO][BTPCMRX] takes %d us",
                     FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

  return len;

#else
#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

  if (!is_sco_mode()) {
    TRACE_AUD_STREAM_E("[SCO][BTPCMRX] player exit!");
    memset(buf, 0x0, len);
    return len;
  }

#if defined(TX_RX_PCM_MASK)
  TRACE_AUD_STREAM_I("[SCO][BTPCMRX] TX_RX_PCM_MASK");
  CQueue *Rx_esco_queue_temp = NULL;
  Rx_esco_queue_temp = get_rx_esco_queue_ptr();
  if (bt_sco_codec_is_msbc() && btdrv_is_pcm_mask_enable() == 1) {
    memset(buf, 0, len);
    int status = 0;
    len /= 2;
    uint8_t rx_data[len];
    status = DeCQueue(Rx_esco_queue_temp, rx_data, len);
    for (uint32_t i = 0; i < len; i++) {
      buf[2 * i + 1] = rx_data[i];
    }
    len *= 2;
    if (status) {
      TRACE_AUD_STREAM_E("[SCO][BTPCMRX] Rx Dec Fail");
    }
  }
#endif

  if (is_codec_stream_started == false) {
    if (bt_sco_codec_is_msbc() == false)
      hal_sys_timer_delay_us(3000);

    TRACE_AUD_STREAM_I("[SCO][BTPCMRX] start codec %d",
                       FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(BONE_SENSOR_TDM)
#if defined(AF_ADC_I2S_SYNC)
    hal_i2s_enable_delay(HAL_I2S_ID_0);
    hal_codec_capture_enable_delay();
#endif
    tdm_stream_start();
#endif

    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

#if defined(AF_ADC_I2S_SYNC)
    codec_capture_i2s_enable();
#endif
    is_codec_stream_started = true;

    return len;
  }
  store_voicebtpcm_m2p_buffer(buf, len);

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t end_ticks = hal_fast_sys_timer_get();
  TRACE_AUD_STREAM_I("[SCO][BTPCMRX] takes %d us",
                     FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

  return len;
#endif
}
#endif

#ifdef __BT_ANC__
static void bt_anc_sco_down_sample_16bits(int16_t *dst, int16_t *src,
                                          uint32_t dst_cnt) {
  for (uint32_t i = 0; i < dst_cnt; i++) {
    dst[i] = src[i * bt_sco_samplerate_ratio * sco_play_chan_num];
  }
}
#endif

static void bt_sco_codec_playback_data_post_handler(uint8_t *buf, uint32_t len,
                                                    void *cfg) {
  POSSIBLY_UNUSED struct AF_STREAM_CONFIG_T *config =
      (struct AF_STREAM_CONFIG_T *)cfg;

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
#ifdef TWS_PROMPT_SYNC
  tws_playback_ticks_check_for_mix_prompt();
#endif
  if (audio_prompt_is_playing_ongoing()) {
    audio_prompt_processing_handler(len, buf);
  }
#else
  app_ring_merge_more_data(buf, len);
#endif
}

static uint32_t bt_sco_codec_playback_data(uint8_t *buf, uint32_t len) {
  app_bt_stream_trigger_checker_handler(TRIGGER_CHECKER_HFP_AUDPCM_PLAYERBLACK);

  bt_set_playback_triggered(true);

#if defined(IBRT) && defined(RSSI_GATHERING_ENABLED)
  app_ibrt_ui_rssi_process();
#endif

#ifdef BT_XTAL_SYNC
#ifdef BT_XTAL_SYNC_NEW_METHOD
#ifdef IBRT
  bool valid = false;
  uint32_t bitoffset = 0;
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (app_tws_ibrt_mobile_link_connected()) {
    valid = true;
    bitoffset = btdrv_rf_bitoffset_get(p_ibrt_ctrl->mobile_conhandle - 0x80);
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    valid = true;
    bitoffset = btdrv_rf_bitoffset_get(p_ibrt_ctrl->ibrt_conhandle - 0x80);
  }

  if (valid) {
    if (app_if_need_fix_target_rxbit() == false) {
      if (bitoffset < XTAL_OFFSET)
        bitoffset = XTAL_OFFSET;
      else if (bitoffset > SLOT_SIZE - XTAL_OFFSET)
        bitoffset = SLOT_SIZE - XTAL_OFFSET;
    }
#ifdef BT_XTAL_SYNC_SLOW
    bt_xtal_sync_new_new(bitoffset, app_if_need_fix_target_rxbit(),
                         BT_XTAL_SYNC_MODE_WITH_MOBILE);
#else
    bt_xtal_sync_new(bitoffset, app_if_need_fix_target_rxbit(),
                     BT_XTAL_SYNC_MODE_WITH_MOBILE);
#endif
  }
#endif
#else
  bt_xtal_sync(BT_XTAL_SYNC_MODE_VOICE);
#endif
#endif

#if defined(AF_ADC_I2S_SYNC)
  // TRACE_AUD_STREAM_I("[%s] cnt = %d", __func__, codec_playback_cnt++);
#endif

#if defined(SCO_DMA_SNAPSHOT)
  // processing  ping pang flag
  int pingpang;

  if (buf == playback_buf_codecpcm) {
    pingpang = 0;
  } else {
    pingpang = 1;
  }
#ifdef TX_RX_PCM_MASK
  // processing btpcm.(It must be from CPU's copy )
  if (btdrv_is_pcm_mask_enable() == 1 && bt_sco_codec_is_msbc()) {
    uint32_t lock;
    uint32_t i;
    // must lock the interrupts when exchanging data.
    lock = int_lock();
    uint16_t *capture_dst =
        (uint16_t *)(capture_buf_btpcm + pingpang * capture_size_btpcm / 2);

    for (i = 0; i < capture_size_btpcm / 4; i++) {
      capture_dst[i] = (uint16_t)capture_buf_btpcm_copy[i] << 8;
    }
    int_unlock(lock);
  }
#endif

  // processing spk
  uint8_t *playbakce_pcm_frame_p =
      playback_buf_codecpcm + pingpang * playback_size_codecpcm / 2;
  uint8_t *source = capture_buf_btpcm + pingpang * capture_size_btpcm / 2;

  if (sco_btpcm_mute_flag == 1 || sco_disconnect_mute_flag == 1) {
    for (uint32_t i = 0; i < playback_size_btpcm / 2; i++) {
      source[i] = MUTE_PATTERN;
    }

    TRACE_AUD_STREAM_I("[SCO][SPK]mute....................");
  } else {
    sco_btpcm_mute_flag = 1;
  }

#if defined(HFP_1_6_ENABLE)
  uint32_t source_len = playback_size_btpcm / 2;
  if (app_audio_manager_get_scocodecid() == BTIF_HF_SCO_CODEC_MSBC) {
    uint16_t *source_u16 = (uint16_t *)source;
    for (uint32_t i = 0; i < source_len / 2; i++) {
      source[i] = (source_u16[i] >> 8);
    }
    source_len >>= 1;
  }
  process_downlink_bt_voice_frames(source, source_len, playbakce_pcm_frame_p,
                                   (playback_size_codecpcm) /
                                       sco_play_chan_num / 2,
                                   app_audio_manager_get_scocodecid());
#else
  process_downlink_bt_voice_frames(
      source, (playback_size_btpcm) / 2, playbakce_pcm_frame_p,
      (playback_size_codecpcm) / sco_play_chan_num / 2, BTIF_HF_SCO_CODEC_CVSD);
#endif

  if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
    // Convert mono data to stereo data
#if defined(SPEECH_RX_24BIT)
    app_bt_stream_copy_track_one_to_two_24bits(
        (int32_t *)playbakce_pcm_frame_p, (int32_t *)playbakce_pcm_frame_p,
        playback_size_codecpcm / 2 / sco_play_chan_num / sizeof(int32_t));
#else
    app_bt_stream_copy_track_one_to_two_16bits(
        (int16_t *)playbakce_pcm_frame_p, (int16_t *)playbakce_pcm_frame_p,
        playback_size_codecpcm / 2 / sco_play_chan_num / sizeof(int16_t));
#endif
  }
  return len;
#else
#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

  uint8_t *dec_buf;
  uint32_t mono_len;

#if defined(SPEECH_RX_24BIT)
  len /= 2;
#endif

#ifdef __BT_ANC__
  mono_len = len / sco_play_chan_num / bt_sco_samplerate_ratio;
  dec_buf = bt_anc_sco_dec_buf;
#else
  mono_len = len / sco_play_chan_num;
  dec_buf = buf;
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
  if (hal_cmu_get_audio_resample_status()) {
    if (sco_play_irq_cnt < SCO_PLAY_RESAMPLE_ALIGN_CNT) {
      sco_play_irq_cnt++;
    }
    if (sco_dma_buf_err ||
        af_stream_buffer_error(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK)) {
      sco_dma_buf_err = false;
      sco_play_irq_cnt = 0;
      app_resample_reset(sco_playback_resample);
      app_resample_reset(sco_capture_resample);
      voicebtpcm_pcm_echo_buf_queue_reset();
      TRACE_AUD_STREAM_I("[SCO][SPK]: DMA buffer error: reset resample");
    }
    app_playback_resample_run(sco_playback_resample, dec_buf, mono_len);
  } else
#endif
  {
#ifdef __BT_ANC__
    bt_anc_sco_down_sample_16bits((int16_t *)dec_buf, (int16_t *)buf,
                                  mono_len / 2);
#else
    if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
      // Convert stereo data to mono data (to save into echo_buf)
      app_bt_stream_copy_track_two_to_one_16bits((int16_t *)dec_buf,
                                                 (int16_t *)buf, mono_len / 2);
    }
#endif
    voicebtpcm_pcm_audio_more_data(dec_buf, mono_len);
  }

#ifdef __BT_ANC__
  voicebtpcm_pcm_resample((int16_t *)dec_buf, mono_len / 2, (int16_t *)buf);
#endif

#if defined(SPEECH_RX_24BIT)
  len <<= 1;
#endif

  if (sco_play_chan_num == AUD_CHANNEL_NUM_2) {
    // Convert mono data to stereo data
#if defined(SPEECH_RX_24BIT)
    app_bt_stream_copy_track_one_to_two_24bits((int32_t *)buf, (int32_t *)buf,
                                               len / 2 / sizeof(int32_t));
#else
    app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, (int16_t *)buf,
                                               len / 2 / sizeof(int16_t));
#endif
  }

  if (spk_force_mute) {
    memset(buf, 0, len);
  }

#if defined(BT_SCO_HANDLER_PROFILE)
  uint32_t end_ticks = hal_fast_sys_timer_get();
  TRACE_AUD_STREAM_I("[SCO][SPK] takes %d us",
                     FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

  return len;
#endif
}

int bt_sco_player_forcemute(bool mic_mute, bool spk_mute) {
  mic_force_mute = mic_mute;
  spk_force_mute = spk_mute;
  return 0;
}

int bt_sco_player_get_codetype(void) {
  if (gStreamplayer & APP_BT_STREAM_HFP_PCM) {
    return bt_sco_player_code_type;
  } else {
    return 0;
  }
}

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) &&                        \
    !defined(__AUDIO_RESAMPLE__)
static uint32_t audio_mc_data_playback_sco(uint8_t *buf,
                                           uint32_t mc_len_bytes) {
  // uint32_t begin_time;
  // uint32_t end_time;
  // begin_time = hal_sys_timer_get();
  // TRACE_AUD_STREAM_I("phone cancel: %d",begin_time);

  float left_gain;
  float right_gain;
  int32_t playback_len_bytes, mc_len_bytes_8;
  int32_t i, j, k;
  int delay_sample;

  mc_len_bytes_8 = mc_len_bytes / 8;

  hal_codec_get_dac_gain(&left_gain, &right_gain);

  TRACE_AUD_STREAM_I(
      "[SCO][SPK][MC] playback_samplerate_ratio:  %d,ch:%d,sample_size:%d.",
      playback_samplerate_ratio_bt, playback_ch_num_bt, sample_size_play_bt);
  TRACE_AUD_STREAM_I("[SCO][SPK][MC] len:  %d", mc_len_bytes);

  // TRACE_AUD_STREAM_I("left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE_AUD_STREAM_I("right_gain: %d",(int)(right_gain*(1<<12)));

  playback_len_bytes = mc_len_bytes / playback_samplerate_ratio_bt;

  if (sample_size_play_bt == AUD_BITS_16) {
    int16_t *sour_p = (int16_t *)(playback_buf_bt + playback_size_bt / 2);
    int16_t *mid_p = (int16_t *)(buf);
    int16_t *mid_p_8 = (int16_t *)(buf + mc_len_bytes - mc_len_bytes_8);
    int16_t *dest_p = (int16_t *)buf;

    if (buf == playback_buf_mc) {
      sour_p = (int16_t *)playback_buf_bt;
    }

    if (playback_ch_num_bt == AUD_CHANNEL_NUM_2) {
      delay_sample = DELAY_SAMPLE_MC;

      for (i = 0, j = 0; i < delay_sample; i = i + 2) {
        mid_p[j++] = delay_buf_bt[i];
        mid_p[j++] = delay_buf_bt[i + 1];
      }

      for (i = 0; i < playback_len_bytes / 2 - delay_sample; i = i + 2) {
        mid_p[j++] = sour_p[i];
        mid_p[j++] = sour_p[i + 1];
      }

      for (j = 0; i < playback_len_bytes / 2; i = i + 2) {
        delay_buf_bt[j++] = sour_p[i];
        delay_buf_bt[j++] = sour_p[i + 1];
      }

      if (playback_samplerate_ratio_bt <= 8) {
        for (i = 0, j = 0; i < playback_len_bytes / 2;
             i = i + 2 * (8 / playback_samplerate_ratio_bt)) {
          mid_p_8[j++] = mid_p[i];
          mid_p_8[j++] = mid_p[i + 1];
        }
      } else {
        for (i = 0, j = 0; i < playback_len_bytes / 2; i = i + 2) {
          for (k = 0; k < playback_samplerate_ratio_bt / 8; k++) {
            mid_p_8[j++] = mid_p[i];
            mid_p_8[j++] = mid_p[i + 1];
          }
        }
      }

      anc_mc_run_stereo((uint8_t *)mid_p_8, mc_len_bytes_8, left_gain,
                        right_gain, AUD_BITS_16);

      for (i = 0, j = 0; i < (mc_len_bytes_8) / 2; i = i + 2) {
        for (k = 0; k < 8; k++) {
          dest_p[j++] = mid_p_8[i];
          dest_p[j++] = mid_p_8[i + 1];
        }
      }

    } else if (playback_ch_num_bt == AUD_CHANNEL_NUM_1) {
      delay_sample = DELAY_SAMPLE_MC / 2;

      for (i = 0, j = 0; i < delay_sample; i = i + 1) {
        mid_p[j++] = delay_buf_bt[i];
      }

      for (i = 0; i < playback_len_bytes / 2 - delay_sample; i = i + 1) {
        mid_p[j++] = sour_p[i];
      }

      for (j = 0; i < playback_len_bytes / 2; i = i + 1) {
        delay_buf_bt[j++] = sour_p[i];
      }

      if (playback_samplerate_ratio_bt <= 8) {
        for (i = 0, j = 0; i < playback_len_bytes / 2;
             i = i + 1 * (8 / playback_samplerate_ratio_bt)) {
          mid_p_8[j++] = mid_p[i];
        }
      } else {
        for (i = 0, j = 0; i < playback_len_bytes / 2; i = i + 1) {
          for (k = 0; k < playback_samplerate_ratio_bt / 8; k++) {
            mid_p_8[j++] = mid_p[i];
          }
        }
      }

      anc_mc_run_mono((uint8_t *)mid_p_8, mc_len_bytes_8, left_gain,
                      AUD_BITS_16);

      for (i = 0, j = 0; i < (mc_len_bytes_8) / 2; i = i + 1) {
        for (k = 0; k < 8; k++) {
          dest_p[j++] = mid_p_8[i];
        }
      }
    }

  } else if (sample_size_play_bt == AUD_BITS_24) {
    int32_t *sour_p = (int32_t *)(playback_buf_bt + playback_size_bt / 2);
    int32_t *mid_p = (int32_t *)(buf);
    int32_t *mid_p_8 = (int32_t *)(buf + mc_len_bytes - mc_len_bytes_8);
    int32_t *dest_p = (int32_t *)buf;

    if (buf == playback_buf_mc) {
      sour_p = (int32_t *)playback_buf_bt;
    }

    if (playback_ch_num_bt == AUD_CHANNEL_NUM_2) {
      delay_sample = DELAY_SAMPLE_MC;

      for (i = 0, j = 0; i < delay_sample; i = i + 2) {
        mid_p[j++] = delay_buf_bt[i];
        mid_p[j++] = delay_buf_bt[i + 1];
      }

      for (i = 0; i < playback_len_bytes / 4 - delay_sample; i = i + 2) {
        mid_p[j++] = sour_p[i];
        mid_p[j++] = sour_p[i + 1];
      }

      for (j = 0; i < playback_len_bytes / 4; i = i + 2) {
        delay_buf_bt[j++] = sour_p[i];
        delay_buf_bt[j++] = sour_p[i + 1];
      }

      if (playback_samplerate_ratio_bt <= 8) {
        for (i = 0, j = 0; i < playback_len_bytes / 4;
             i = i + 2 * (8 / playback_samplerate_ratio_bt)) {
          mid_p_8[j++] = mid_p[i];
          mid_p_8[j++] = mid_p[i + 1];
        }
      } else {
        for (i = 0, j = 0; i < playback_len_bytes / 4; i = i + 2) {
          for (k = 0; k < playback_samplerate_ratio_bt / 8; k++) {
            mid_p_8[j++] = mid_p[i];
            mid_p_8[j++] = mid_p[i + 1];
          }
        }
      }

      anc_mc_run_stereo((uint8_t *)mid_p_8, mc_len_bytes_8, left_gain,
                        right_gain, AUD_BITS_24);

      for (i = 0, j = 0; i < (mc_len_bytes_8) / 4; i = i + 2) {
        for (k = 0; k < 8; k++) {
          dest_p[j++] = mid_p_8[i];
          dest_p[j++] = mid_p_8[i + 1];
        }
      }

    } else if (playback_ch_num_bt == AUD_CHANNEL_NUM_1) {
      delay_sample = DELAY_SAMPLE_MC / 2;

      for (i = 0, j = 0; i < delay_sample; i = i + 1) {
        mid_p[j++] = delay_buf_bt[i];
      }

      for (i = 0; i < playback_len_bytes / 4 - delay_sample; i = i + 1) {
        mid_p[j++] = sour_p[i];
      }

      for (j = 0; i < playback_len_bytes / 4; i = i + 1) {
        delay_buf_bt[j++] = sour_p[i];
      }

      if (playback_samplerate_ratio_bt <= 8) {
        for (i = 0, j = 0; i < playback_len_bytes / 4;
             i = i + 1 * (8 / playback_samplerate_ratio_bt)) {
          mid_p_8[j++] = mid_p[i];
        }
      } else {
        for (i = 0, j = 0; i < playback_len_bytes / 4; i = i + 1) {
          for (k = 0; k < playback_samplerate_ratio_bt / 8; k++) {
            mid_p_8[j++] = mid_p[i];
          }
        }
      }

      anc_mc_run_mono((uint8_t *)mid_p_8, mc_len_bytes_8, left_gain,
                      AUD_BITS_24);

      for (i = 0, j = 0; i < (mc_len_bytes_8) / 4; i = i + 1) {
        for (k = 0; k < 8; k++) {
          dest_p[j++] = mid_p_8[i];
        }
      }
    }
  }

  //  end_time = hal_sys_timer_get();

  //   TRACE_AUD_STREAM_I("[SCO][SPK][MC]:run time: %d", end_time-begin_time);

  return 0;
}
#endif

#if defined(LOW_DELAY_SCO)
int speech_get_frame_size(int fs, int ch, int ms) {
  return (fs / 1000 * ch * ms) / 2;
}
#else
int speech_get_frame_size(int fs, int ch, int ms) {
  return (fs / 1000 * ch * ms);
}
#endif

int speech_get_af_buffer_size(int fs, int ch, int ms) {
  return speech_get_frame_size(fs, ch, ms) * 2 * 2;
}

enum AUD_SAMPRATE_T speech_sco_get_sample_rate(void) {
  enum AUD_SAMPRATE_T sample_rate;

#if defined(HFP_1_6_ENABLE)
  if (bt_sco_codec_is_msbc()) {
    sample_rate = AUD_SAMPRATE_16000;
  } else
#endif
  {
    sample_rate = AUD_SAMPRATE_8000;
  }

  return sample_rate;
}

enum AUD_SAMPRATE_T speech_codec_get_sample_rate(void) {
  enum AUD_SAMPRATE_T sample_rate;

#if defined(MSBC_8K_SAMPLE_RATE)
  sample_rate = AUD_SAMPRATE_8000;
#else
  if (bt_sco_codec_is_msbc()) {

    sample_rate = AUD_SAMPRATE_16000;
  } else {
    sample_rate = AUD_SAMPRATE_8000;
  }
#endif

  return sample_rate;
}

int app_bt_stream_volumeset(int8_t vol);

enum AUD_SAMPRATE_T sco_sample_rate;

#if defined(AF_ADC_I2S_SYNC)
void bt_sco_bt_trigger_callback(void) {
  TRACE_AUD_STREAM_I("[SCO][IIS] Start...", __func__);

  hal_i2s_enable(HAL_I2S_ID_0);
}
#endif

extern void bt_drv_reg_op_pcm_set(uint8_t en);
extern uint8_t bt_drv_reg_op_pcm_get();
int bt_sco_player(bool on, enum APP_SYSFREQ_FREQ_T freq) {
  struct AF_STREAM_CONFIG_T stream_cfg;
  static bool isRun = false;
  uint8_t *bt_audio_buff = NULL;
  enum AUD_SAMPRATE_T sample_rate;

  TRACE_AUD_STREAM_I("[SCO_PLAYER] work:%d op:%d freq:%d", isRun, on, freq);

#ifdef CHIP_BEST2000
  btdrv_enable_one_packet_more_head(0);
#endif

  bt_set_playback_triggered(false);
  if (isRun == on)
    return 0;

  if (on) {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    audio_prompt_stop_playing();
#endif

#ifdef WL_DET
    app_mic_alg_audioloop(false, APP_SYSFREQ_78M);
#endif

#if defined(IBRT)
    app_ibrt_ui_rssi_reset();
    app_ibrt_if_exec_sleep_hook_blocker_set(
        APP_IBRT_IF_SLEEP_HOOK_BLOCKER_HFP_SCO);
#endif
#ifdef __IAG_BLE_INCLUDE__
    app_ble_force_switch_adv(BLE_SWITCH_USER_SCO, false);
#endif
#ifdef TX_RX_PCM_MASK
    if (btdrv_is_pcm_mask_enable() == 1 && bt_sco_codec_is_msbc()) {
      bt_drv_reg_op_pcm_set(1);
      TRACE_AUD_STREAM_I("[SCO_PLAYER] PCM MASK");
    }
#endif

#if defined(PCM_FAST_MODE)
    btdrv_open_pcm_fast_mode_enable();
#ifdef PCM_PRIVATE_DATA_FLAG
    bt_drv_reg_op_set_pcm_flag();
#endif
#endif

#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_STOP);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,
                                             THIRDPARTY_MIC_OPEN);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,
                                             THIRDPARTY_STOP);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_KWS,
                                             THIRDPARTY_CALL_START);
#endif
    // bt_syncerr set to max(0x0a)
    //        BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x0f);
    //        af_set_priority(AF_USER_SCO, osPriorityRealtime);
    af_set_priority(AF_USER_SCO, osPriorityHigh);
    bt_media_volume_ptr_update_by_mediatype(BT_STREAM_VOICE);
    stream_local_volume = btdevice_volume_p->hfp_vol;
    app_audio_manager_sco_status_checker();

#if defined(HFP_1_6_ENABLE)
    bt_sco_player_code_type = app_audio_manager_get_scocodecid();
#endif

    if (freq < APP_SYSFREQ_104M) {
      freq = APP_SYSFREQ_104M;
    }

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) &&                        \
    !defined(__AUDIO_RESAMPLE__)
    if (freq < APP_SYSFREQ_208M) {
      freq = APP_SYSFREQ_208M;
    }
#endif

#if defined(SCO_CP_ACCEL)
    freq = APP_SYSFREQ_52M;
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, freq);
    TRACE_AUD_STREAM_I("[SCO_PLAYER] sysfreq:%d", freq);
    TRACE_AUD_STREAM_I("[SCO_PLAYER] sysfreq calc : %d\n",
                       hal_sys_timer_calc_cpu_freq(5, 0));

    app_overlay_select(APP_OVERLAY_HFP);
#ifdef BT_XTAL_SYNC
    bt_init_xtal_sync(BT_XTAL_SYNC_MODE_VOICE, BT_INIT_XTAL_SYNC_MIN,
                      BT_INIT_XTAL_SYNC_MAX, BT_INIT_XTAL_SYNC_FCAP_RANGE);
#endif
    btdrv_rf_bit_offset_track_enable(true);

#if !defined(SCO_DMA_SNAPSHOT)
    int aec_frame_len = speech_get_frame_size(speech_codec_get_sample_rate(), 1,
                                              SPEECH_SCO_FRAME_MS);
    speech_tx_aec_set_frame_len(aec_frame_len);
#endif

    bt_sco_player_forcemute(false, false);

    bt_sco_mode = 1;

    app_audio_mempool_init();

#ifndef _SCO_BTPCM_CHANNEL_
    memset(&hf_sendbuff_ctrl, 0, sizeof(hf_sendbuff_ctrl));
#endif

    sample_rate = speech_codec_get_sample_rate();

    sco_cap_chan_num = (enum AUD_CHANNEL_NUM_T)SPEECH_CODEC_CAPTURE_CHANNEL_NUM;


#if defined(SPEECH_TX_AEC_CODEC_REF)
    sco_cap_chan_num = (enum AUD_CHANNEL_NUM_T)(sco_cap_chan_num + 1);
#endif

#if defined(ANC_NOISE_TRACKER)
    sco_cap_chan_num = (enum AUD_CHANNEL_NUM_T)(sco_cap_chan_num +
                                                ANC_NOISE_TRACKER_CHANNEL_NUM);
#endif

#ifdef PLAYBACK_USE_I2S
    hal_cmu_audio_resample_disable();
#endif

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    // codec:mic
    stream_cfg.channel_num = sco_cap_chan_num;
    stream_cfg.data_size = speech_get_af_buffer_size(
        sample_rate, sco_cap_chan_num, SPEECH_SCO_FRAME_MS);

#if defined(__AUDIO_RESAMPLE__) && defined(NO_SCO_RESAMPLE)
    // When __AUDIO_RESAMPLE__ is defined,
    // resample is off by default on best1000, and on by default on other
    // platforms
#ifndef CHIP_BEST1000
    hal_cmu_audio_resample_disable();
#endif
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    if (sample_rate == AUD_SAMPRATE_8000) {
      stream_cfg.sample_rate = AUD_SAMPRATE_8463;
    } else if (sample_rate == AUD_SAMPRATE_16000) {
      stream_cfg.sample_rate = AUD_SAMPRATE_16927;
    }
#ifdef RESAMPLE_ANY_SAMPLE_RATE
    sco_capture_resample = app_capture_resample_any_open(
        stream_cfg.channel_num, bt_sco_capture_resample_iter,
        stream_cfg.data_size / 2, (float)CODEC_FREQ_26M / CODEC_FREQ_24P576M);
#else
    sco_capture_resample = app_capture_resample_open(
        sample_rate, stream_cfg.channel_num, bt_sco_capture_resample_iter,
        stream_cfg.data_size / 2);
#endif
    uint32_t mono_cap_samp_cnt =
        stream_cfg.data_size / 2 / 2 / stream_cfg.channel_num;
    uint32_t cap_irq_cnt_per_frm =
        ((mono_cap_samp_cnt * stream_cfg.sample_rate + (sample_rate - 1)) /
             sample_rate +
         (aec_frame_len - 1)) /
        aec_frame_len;
    if (cap_irq_cnt_per_frm == 0) {
      cap_irq_cnt_per_frm = 1;
    }
#else
    stream_cfg.sample_rate = sample_rate;
#endif

#if defined(SPEECH_TX_24BIT)
    stream_cfg.bits = AUD_BITS_24;
    stream_cfg.data_size *= 2;
#else
    stream_cfg.bits = AUD_BITS_16;
#endif
    stream_cfg.vol = stream_local_volume;

    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
    stream_cfg.handler = bt_sco_codec_capture_data;
    app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#if defined(SCO_DMA_SNAPSHOT)
    capture_buf_codecpcm = stream_cfg.data_ptr;
    capture_size_codecpcm = stream_cfg.data_size;
#endif
    TRACE_AUD_STREAM_I("[SCO_PLAYER] capture sample_rate:%d, data_size:%d",
                       stream_cfg.sample_rate, stream_cfg.data_size);

#if defined(ANC_WNR_ENABLED)
    if (app_anc_work_status()) {
      anc_wnr_close();
      anc_wnr_ctrl(stream_cfg.sample_rate,
                   speech_get_frame_size(stream_cfg.sample_rate, 1,
                                         SPEECH_SCO_FRAME_MS));
      anc_release_gain();
      anc_wnr_open(ANC_WNR_OPEN_MODE_CONFIGURE);
    } else {
      anc_wnr_ctrl(stream_cfg.sample_rate,
                   speech_get_frame_size(stream_cfg.sample_rate, 1,
                                         SPEECH_SCO_FRAME_MS));
    }
#endif

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

#if defined(HW_DC_FILTER_WITH_IIR)
    hw_filter_codec_iir_st = hw_filter_codec_iir_create(
        stream_cfg.sample_rate, stream_cfg.channel_num, stream_cfg.bits,
        &adc_iir_cfg);
#endif

#if defined(CHIP_BEST2300)
    btdrv_set_bt_pcm_triggler_delay(60);
#elif defined(CHIP_BEST1400) || defined(CHIP_BEST1402) || defined(CHIP_BEST2001)

#if defined(SCO_DMA_SNAPSHOT)
    btdrv_set_bt_pcm_triggler_delay(2);
#else
    btdrv_set_bt_pcm_triggler_delay(60);
#endif

#elif defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)

#if defined(SCO_DMA_SNAPSHOT)
    btdrv_set_bt_pcm_triggler_delay(2);
#else
    btdrv_set_bt_pcm_triggler_delay(59);
#endif

#else
    btdrv_set_bt_pcm_triggler_delay(55);
#endif
    // codec:spk
    sample_rate = speech_codec_get_sample_rate();
#if defined(CHIP_BEST1000)
    sco_play_chan_num = AUD_CHANNEL_NUM_2;
#else
#ifdef PLAYBACK_USE_I2S
    sco_play_chan_num = AUD_CHANNEL_NUM_2;
#else
    sco_play_chan_num = AUD_CHANNEL_NUM_1;
#endif
#endif

#if defined(BONE_SENSOR_TDM)
#if defined(AF_ADC_I2S_SYNC)
    codec_capture_cnt = 0;
    codec_playback_cnt = 0;
#endif
    lis25ba_init();
    tdm_stream_register(tdm_callback_func);
#endif

    stream_cfg.channel_num = sco_play_chan_num;
    // stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE * stream_cfg.channel_num;
    stream_cfg.data_size = speech_get_af_buffer_size(
        sample_rate, sco_play_chan_num, SPEECH_SCO_FRAME_MS);
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    if (sample_rate == AUD_SAMPRATE_8000) {
      stream_cfg.sample_rate = AUD_SAMPRATE_8463;
    } else if (sample_rate == AUD_SAMPRATE_16000) {
      stream_cfg.sample_rate = AUD_SAMPRATE_16927;
    }
#ifdef RESAMPLE_ANY_SAMPLE_RATE
    sco_playback_resample = app_playback_resample_any_open(
        AUD_CHANNEL_NUM_1, bt_sco_playback_resample_iter,
        stream_cfg.data_size / stream_cfg.channel_num / 2,
        (float)CODEC_FREQ_24P576M / CODEC_FREQ_26M);
#else
    sco_playback_resample = app_playback_resample_open(
        sample_rate, AUD_CHANNEL_NUM_1, bt_sco_playback_resample_iter,
        stream_cfg.data_size / stream_cfg.channel_num / 2);
#endif
    sco_play_irq_cnt = 0;
    sco_dma_buf_err = false;

    uint32_t mono_play_samp_cnt =
        stream_cfg.data_size / 2 / 2 / stream_cfg.channel_num;
    uint32_t play_irq_cnt_per_frm =
        ((mono_play_samp_cnt * stream_cfg.sample_rate + (sample_rate - 1)) /
             sample_rate +
         (aec_frame_len - 1)) /
        aec_frame_len;
    if (play_irq_cnt_per_frm == 0) {
      play_irq_cnt_per_frm = 1;
    }
    uint32_t play_samp_cnt_per_frm = mono_play_samp_cnt * play_irq_cnt_per_frm;
    uint32_t cap_samp_cnt_per_frm = mono_cap_samp_cnt * cap_irq_cnt_per_frm;
    uint32_t max_samp_cnt_per_frm =
        (play_samp_cnt_per_frm >= cap_samp_cnt_per_frm) ? play_samp_cnt_per_frm
                                                        : cap_samp_cnt_per_frm;
    uint32_t echo_q_samp_cnt =
        (((max_samp_cnt_per_frm +
           mono_play_samp_cnt * SCO_PLAY_RESAMPLE_ALIGN_CNT) *
              // convert to 8K/16K sample cnt
              sample_rate +
          (stream_cfg.sample_rate - 1)) /
             stream_cfg.sample_rate +
         // aligned with aec_frame_len
         (aec_frame_len - 1)) /
        aec_frame_len * aec_frame_len;
    if (echo_q_samp_cnt == 0) {
      echo_q_samp_cnt = aec_frame_len;
    }
    voicebtpcm_pcm_echo_buf_queue_init(echo_q_samp_cnt * 2);
#else
    stream_cfg.sample_rate = sample_rate;
#endif

#ifdef __BT_ANC__
    // Mono channel decoder buffer (8K or 16K sample rate)
    app_audio_mempool_get_buff(&bt_anc_sco_dec_buf,
                               stream_cfg.data_size / 2 / sco_play_chan_num);
    // The playback size for the actual sample rate
    bt_sco_samplerate_ratio = 6 / (sample_rate / AUD_SAMPRATE_8000);
    stream_cfg.data_size *= bt_sco_samplerate_ratio;
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
    stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#endif
    // damic_init();
    // init_amic_dc_bt();
    // ds_fir_init();
    us_fir_init();
#endif
    stream_cfg.bits = AUD_BITS_16;
#ifdef PLAYBACK_USE_I2S
    stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
    stream_cfg.io_path = AUD_IO_PATH_NULL;
#else
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
#endif
    stream_cfg.handler = bt_sco_codec_playback_data;

#if defined(SPEECH_RX_24BIT)
    stream_cfg.bits = AUD_BITS_24;
    stream_cfg.data_size *= 2;
#endif

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    uint8_t *promptTmpSourcePcmDataBuf;
    uint8_t *promptTmpTargetPcmDataBuf;
    uint8_t *promptPcmDataBuf;
    uint8_t *promptResamplerBuf;

    sco_sample_rate = stream_cfg.sample_rate;
    app_audio_mempool_get_buff(&promptTmpSourcePcmDataBuf,
                               AUDIO_PROMPT_SOURCE_PCM_BUFFER_SIZE);
    app_audio_mempool_get_buff(&promptTmpTargetPcmDataBuf,
                               AUDIO_PROMPT_TARGET_PCM_BUFFER_SIZE);
    app_audio_mempool_get_buff(&promptPcmDataBuf, AUDIO_PROMPT_PCM_BUFFER_SIZE);
    app_audio_mempool_get_buff(&promptResamplerBuf,
                               AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);

    audio_prompt_buffer_config(MIX_WITH_SCO_STREAMING, stream_cfg.channel_num,
                               stream_cfg.bits, promptTmpSourcePcmDataBuf,
                               promptTmpTargetPcmDataBuf, promptPcmDataBuf,
                               AUDIO_PROMPT_PCM_BUFFER_SIZE, promptResamplerBuf,
                               AUDIO_PROMPT_BUF_SIZE_FOR_RESAMPLER);
#endif

    app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

    TRACE_AUD_STREAM_I("[SCO_PLAYER] playback sample_rate:%d, data_size:%d",
                       stream_cfg.sample_rate, stream_cfg.data_size);

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) &&                        \
    !defined(__AUDIO_RESAMPLE__)
    sample_size_play_bt = stream_cfg.bits;
    sample_rate_play_bt = stream_cfg.sample_rate;
    data_size_play_bt = stream_cfg.data_size;
    playback_buf_bt = stream_cfg.data_ptr;
    playback_size_bt = stream_cfg.data_size;

#ifdef __BT_ANC__
    playback_samplerate_ratio_bt = 8;
#else
    if (sample_rate_play_bt == AUD_SAMPRATE_8000) {
      playback_samplerate_ratio_bt = 8 * 3 * 2;
    } else if (sample_rate_play_bt == AUD_SAMPRATE_16000) {
      playback_samplerate_ratio_bt = 8 * 3;
    }
#endif

    playback_ch_num_bt = stream_cfg.channel_num;
#endif

#if defined(SCO_DMA_SNAPSHOT)
    playback_buf_codecpcm = stream_cfg.data_ptr;
    playback_size_codecpcm = stream_cfg.data_size;
    playback_samplerate_codecpcm = stream_cfg.sample_rate;
#endif

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

    af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

    af_codec_set_playback_post_handler(bt_sco_codec_playback_data_post_handler);

#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) &&                        \
    !defined(__AUDIO_RESAMPLE__)
    stream_cfg.bits = sample_size_play_bt;
    stream_cfg.channel_num = playback_ch_num_bt;
    stream_cfg.sample_rate = sample_rate_play_bt;
    stream_cfg.device = AUD_STREAM_USE_MC;
    stream_cfg.vol = 0;
    stream_cfg.handler = audio_mc_data_playback_sco;
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

    app_audio_mempool_get_buff(
        &bt_audio_buff, data_size_play_bt * playback_samplerate_ratio_bt);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
    stream_cfg.data_size = data_size_play_bt * playback_samplerate_ratio_bt;

    playback_buf_mc = stream_cfg.data_ptr;
    playback_size_mc = stream_cfg.data_size;

    anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

    memset(delay_buf_bt, 0, sizeof(delay_buf_bt));

    af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
#endif

#if defined(BONE_SENSOR_TDM)
    tdm_stream_open();

#if defined(AF_ADC_I2S_SYNC)
    af_codec_bt_trigger_config(true, bt_sco_bt_trigger_callback);
#endif
#endif

#if defined(ANC_NOISE_TRACKER)
    app_audio_mempool_get_buff(
        (uint8_t **)&anc_buf,
        speech_get_frame_size(speech_codec_get_sample_rate(),
                              ANC_NOISE_TRACKER_CHANNEL_NUM,
                              SPEECH_SCO_FRAME_MS) *
            sizeof(int16_t));
    noise_tracker_init(nt_demo_words_cb, ANC_NOISE_TRACKER_CHANNEL_NUM, -20);
#endif

    // Must call this function before af_stream_start
    // Get all free app audio buffer except SCO_BTPCM used(2k)
    voicebtpcm_pcm_audio_init(speech_sco_get_sample_rate(),
                              speech_codec_get_sample_rate());

    /*
    TRACE_AUD_STREAM_I("[SCO_PLAYER] start codec %d",
FAST_TICKS_TO_US(hal_fast_sys_timer_get())); #if defined(AUDIO_ANC_FB_MC) &&
defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    */

#ifdef SPEECH_SIDETONE
    hal_codec_sidetone_enable();
#endif

#ifdef _SCO_BTPCM_CHANNEL_
    stream_cfg.bits = AUD_BITS_16;
    stream_cfg.sample_rate = speech_sco_get_sample_rate();
    stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
    // stream_cfg.data_size = BT_AUDIO_SCO_BUFF_SIZE * stream_cfg.channel_num;

    if (bt_sco_codec_is_msbc()) {
      stream_cfg.data_size = speech_get_af_buffer_size(stream_cfg.sample_rate,
                                                       stream_cfg.channel_num,
                                                       SPEECH_SCO_FRAME_MS) /
                             2;
    } else {
      stream_cfg.data_size = speech_get_af_buffer_size(
          stream_cfg.sample_rate, stream_cfg.channel_num, SPEECH_SCO_FRAME_MS);
    }

    // btpcm:rx
    stream_cfg.device = AUD_STREAM_USE_BT_PCM;
    stream_cfg.handler = bt_sco_btpcm_capture_data;
    app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

    TRACE_AUD_STREAM_I("[SCO_PLAYER] sco btpcm sample_rate:%d, data_size:%d",
                       stream_cfg.sample_rate, stream_cfg.data_size);

#if defined(SCO_DMA_SNAPSHOT)
    sco_btpcm_mute_flag = 0;
    sco_disconnect_mute_flag = 0;

    capture_buf_btpcm = stream_cfg.data_ptr;
    capture_size_btpcm = stream_cfg.data_size;
#ifdef TX_RX_PCM_MASK
    capture_size_btpcm_copy =
        stream_cfg.data_size / 4; // only need ping or pang;
    app_audio_mempool_get_buff(&capture_buf_btpcm_copy,
                               capture_size_btpcm_copy);
#endif
#endif
    af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE, &stream_cfg);

    // btpcm:tx
    stream_cfg.device = AUD_STREAM_USE_BT_PCM;
    stream_cfg.handler = bt_sco_btpcm_playback_data;
    app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#if defined(SCO_DMA_SNAPSHOT)
    playback_buf_btpcm = stream_cfg.data_ptr;
    playback_size_btpcm = stream_cfg.data_size;
#ifdef TX_RX_PCM_MASK
    playback_size_btpcm_copy =
        stream_cfg.data_size / 4; // only need ping or pang;
    app_audio_mempool_get_buff(&playback_buf_btpcm_copy,
                               playback_size_btpcm_copy);
#endif
    // only need ping or pang;
    app_audio_mempool_get_buff(&playback_buf_btpcm_cache,
                               stream_cfg.data_size / 2);
#endif

    af_stream_open(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK, &stream_cfg);

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
    af_codec_tune(AUD_STREAM_NUM, 0);
#endif

    TRACE_AUD_STREAM_I("[SCO_PLAYER] start btpcm %d",
                       FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
    af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
    af_stream_start(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
#if defined(SCO_DMA_SNAPSHOT)
    af_stream_dma_tc_irq_enable(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);

#ifdef PLAYBACK_USE_I2S
    af_i2s_sync_config(AUD_STREAM_PLAYBACK, AF_I2S_SYNC_TYPE_BT, true);
#else
    af_codec_sync_config(AUD_STREAM_PLAYBACK, AF_CODEC_SYNC_TYPE_BT, true);
#endif
    af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);

    btdrv_disable_playback_triggler();
    app_bt_stream_sco_trigger_btpcm_start();
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#endif

    is_codec_stream_started = false;

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                       \
    defined(CHIP_BEST2300A) || defined(CHIP_BEST1400) ||                       \
    defined(CHIP_BEST1402) || defined(CHIP_BEST2001)
#if defined(CVSD_BYPASS)
    btdrv_cvsd_bypass_enable(bt_sco_codec_is_msbc());
#endif
#if !defined(SCO_DMA_SNAPSHOT)
    btdrv_pcm_enable();
#endif
#endif

#endif

    app_bt_stream_trigger_checker_start();
    TRACE_AUD_STREAM_I("[SCO_PLAYER] on");
  } else {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    bool isToClearActiveMedia =
        audio_prompt_clear_pending_stream(PENDING_TO_STOP_SCO_STREAMING);
    if (isToClearActiveMedia) {
      bt_media_clear_media_type(BT_STREAM_VOICE, BT_DEVICE_ID_1);
    }
#endif
    app_bt_stream_trigger_checker_stop();
#if defined(SCO_DMA_SNAPSHOT)
#ifdef TX_RX_PCM_MASK
    playback_buf_btpcm_copy = NULL;
    capture_buf_btpcm_copy = NULL;
    playback_size_btpcm_copy = 0;
    capture_size_btpcm_copy = 0;
#endif
#endif

#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,
                                             THIRDPARTY_MIC_CLOSE);
#endif
#if defined(SCO_DMA_SNAPSHOT)
    app_bt_stream_sco_trigger_codecpcm_stop();
#endif
    af_stream_dma_tc_irq_disable(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_codec_set_playback_post_handler(NULL);

    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) &&                        \
    !defined(__AUDIO_RESAMPLE__)
    af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
    is_codec_stream_started = false;

#if defined(BONE_SENSOR_TDM)
    tdm_stream_stop();
#endif

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
    af_codec_tune(AUD_STREAM_NUM, 0);
#endif

#ifdef _SCO_BTPCM_CHANNEL_
    af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    af_stream_stop(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);

    af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_1, AUD_STREAM_PLAYBACK);
#endif
#ifdef TX_RX_PCM_MASK
    if (btdrv_is_pcm_mask_enable() == 1 && bt_drv_reg_op_pcm_get()) {
      bt_drv_reg_op_pcm_set(0);
      TRACE_AUD_STREAM_I("[SCO_PLAYER] PCM UNMASK");
    }
#endif
#if defined(PCM_FAST_MODE)
    btdrv_open_pcm_fast_mode_disable();
#endif

#if defined(HW_DC_FILTER_WITH_IIR)
    hw_filter_codec_iir_destroy(hw_filter_codec_iir_st);
#endif

    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#if defined(AUDIO_ANC_FB_MC_SCO) && defined(ANC_APP) &&                        \
    !defined(__AUDIO_RESAMPLE__)
    af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

#if defined(BONE_SENSOR_TDM)
    tdm_stream_close();
#endif

#ifdef SPEECH_SIDETONE
    hal_codec_sidetone_disable();
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    app_capture_resample_close(sco_capture_resample);
    sco_capture_resample = NULL;
    app_capture_resample_close(sco_playback_resample);
    sco_playback_resample = NULL;
#endif

#if defined(__AUDIO_RESAMPLE__) && defined(NO_SCO_RESAMPLE)
#ifndef CHIP_BEST1000
    // When __AUDIO_RESAMPLE__ is defined,
    // resample is off by default on best1000, and on by default on other
    // platforms
    hal_cmu_audio_resample_enable();
#endif
#endif

#ifdef PLAYBACK_USE_I2S
    hal_cmu_audio_resample_enable();
#endif

    bt_sco_mode = 0;

#ifdef __BT_ANC__
    bt_anc_sco_dec_buf = NULL;
    // damic_deinit();
    // app_cap_thread_stop();
#endif
    voicebtpcm_pcm_audio_deinit();

#if defined(BONE_SENSOR_TDM)
    lis25ba_deinit();
#endif

#ifdef BT_XTAL_SYNC
    bt_term_xtal_sync(false);
#ifndef BT_XTAL_SYNC_NO_RESET
    bt_term_xtal_sync_default();
#endif
#endif
#if defined(HFP_1_6_ENABLE)
    TRACE(1, "clear sco tx fifo codec:%d", bt_sco_player_code_type);
    bt_drv_reg_op_sco_txfifo_reset(bt_sco_player_code_type);
    bt_sco_player_code_type = BTIF_HF_SCO_CODEC_NONE;
#else
    bt_drv_reg_op_sco_txfifo_reset(1);
#endif

#if defined(ANC_WNR_ENABLED)
    if (app_anc_work_status()) {
      anc_wnr_close();
      anc_release_gain();
      anc_wnr_open(ANC_WNR_OPEN_MODE_STANDALONE);
    }
#endif

#ifdef __IAG_BLE_INCLUDE__
    app_ble_force_switch_adv(BLE_SWITCH_USER_SCO, true);
#endif

    TRACE_AUD_STREAM_I("[SCO_PLAYER] off");
    app_overlay_unloadall();
    app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, APP_SYSFREQ_32K);
    af_set_priority(AF_USER_SCO, osPriorityAboveNormal);

    // bt_syncerr set to default(0x07)
//       BTDIGITAL_REG_SET_FIELD(REG_BTCORE_BASE_ADDR, 0x0f, 0, 0x07);
#ifdef __THIRDPARTY
    // app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,THIRDPARTY_START);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,
                                             THIRDPARTY_START);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_KWS,
                                             THIRDPARTY_CALL_STOP);
#endif
#if defined(IBRT)
    app_ibrt_if_exec_sleep_hook_blocker_clr(
        APP_IBRT_IF_SLEEP_HOOK_BLOCKER_HFP_SCO);
    app_ibrt_ui_rssi_reset();
#endif

#ifdef WL_DET
    app_mic_alg_audioloop(true, APP_SYSFREQ_78M);
#endif
  }

  isRun = on;
  return 0;
}

#ifdef AUDIO_LINEIN
#include "app_status_ind.h"
// player channel should <= capture channel number
// player must be 2 channel
#define LINEIN_PLAYER_CHANNEL (2)
#ifdef __AUDIO_OUTPUT_MONO_MODE__
#define LINEIN_CAPTURE_CHANNEL (1)
#else
#define LINEIN_CAPTURE_CHANNEL (2)
#endif

#if (LINEIN_CAPTURE_CHANNEL == 1)
#define LINEIN_PLAYER_BUFFER_SIZE (1024 * LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE / 2)
#elif (LINEIN_CAPTURE_CHANNEL == 2)
#define LINEIN_PLAYER_BUFFER_SIZE (1024 * LINEIN_PLAYER_CHANNEL)
#define LINEIN_CAPTURE_BUFFER_SIZE (LINEIN_PLAYER_BUFFER_SIZE)
#endif

int8_t app_linein_buffer_is_empty(void) {
  if (app_audio_pcmbuff_length()) {
    return 0;
  } else {
    return 1;
  }
}

uint32_t app_linein_pcm_come(uint8_t *pcm_buf, uint32_t len) {
  app_audio_pcmbuff_put(pcm_buf, len);

  return len;
}

uint32_t app_linein_need_pcm_data(uint8_t *pcm_buf, uint32_t len) {
#if (LINEIN_CAPTURE_CHANNEL == 1)
  app_audio_pcmbuff_get((uint8_t *)app_linein_play_cache, len / 2);
  app_play_audio_lineinmode_more_data((uint8_t *)app_linein_play_cache,
                                      len / 2);
  app_bt_stream_copy_track_one_to_two_16bits(
      (int16_t *)pcm_buf, app_linein_play_cache, len / 2 / 2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
  app_audio_pcmbuff_get((uint8_t *)pcm_buf, len);
  app_play_audio_lineinmode_more_data((uint8_t *)pcm_buf, len);
#endif

#if defined(__AUDIO_OUTPUT_MONO_MODE__)
  merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)pcm_buf, len / 2);
#endif

#ifdef ANC_APP
  bt_audio_updata_eq_for_anc(app_anc_work_status());
#else
  bt_audio_updata_eq(app_audio_get_eq());
#endif

  audio_process_run(pcm_buf, len);

  return len;
}

int app_play_linein_onoff(bool onoff) {
  static bool isRun = false;
  uint8_t *linein_audio_cap_buff = 0;
  uint8_t *linein_audio_play_buff = 0;
  uint8_t *linein_audio_loop_buf = NULL;
  struct AF_STREAM_CONFIG_T stream_cfg;

  uint8_t POSSIBLY_UNUSED *bt_eq_buff = NULL;
  uint32_t POSSIBLY_UNUSED eq_buff_size;
  uint8_t POSSIBLY_UNUSED play_samp_size;

  TRACE_AUD_STREAM_I("[LINEIN_PLAYER] work:%d op:%d", isRun, onoff);

  if (isRun == onoff)
    return 0;

  if (onoff) {
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
    app_overlay_select(APP_OVERLAY_A2DP);
    app_audio_mempool_init();
    app_audio_mempool_get_buff(&linein_audio_cap_buff,
                               LINEIN_CAPTURE_BUFFER_SIZE);
    app_audio_mempool_get_buff(&linein_audio_play_buff,
                               LINEIN_PLAYER_BUFFER_SIZE);
    app_audio_mempool_get_buff(&linein_audio_loop_buf,
                               LINEIN_PLAYER_BUFFER_SIZE << 2);
    app_audio_pcmbuff_init(linein_audio_loop_buf,
                           LINEIN_PLAYER_BUFFER_SIZE << 2);

#if (LINEIN_CAPTURE_CHANNEL == 1)
    app_audio_mempool_get_buff((uint8_t **)&app_linein_play_cache,
                               LINEIN_PLAYER_BUFFER_SIZE / 2 / 2);
    app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL,
                                   LINEIN_PLAYER_BUFFER_SIZE / 2 / 2);
#elif (LINEIN_CAPTURE_CHANNEL == 2)
    app_play_audio_lineinmode_init(LINEIN_CAPTURE_CHANNEL,
                                   LINEIN_PLAYER_BUFFER_SIZE / 2);
#endif

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.bits = AUD_BITS_16;
    stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_PLAYER_CHANNEL;
#if defined(__AUDIO_RESAMPLE__)
    stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
    stream_cfg.sample_rate = AUD_SAMPRATE_44100;
#endif
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.vol = stream_linein_volume;
    TRACE_AUD_STREAM_I("[LINEIN_PLAYER] vol = %d", stream_linein_volume);
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
    stream_cfg.handler = app_linein_need_pcm_data;
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_play_buff);
    stream_cfg.data_size = LINEIN_PLAYER_BUFFER_SIZE;

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    sample_size_play_bt = stream_cfg.bits;
    sample_rate_play_bt = stream_cfg.sample_rate;
    data_size_play_bt = stream_cfg.data_size;
    playback_buf_bt = stream_cfg.data_ptr;
    playback_size_bt = stream_cfg.data_size;
    if (sample_rate_play_bt == AUD_SAMPRATE_96000) {
      playback_samplerate_ratio_bt = 4;
    } else {
      playback_samplerate_ratio_bt = 8;
    }
    playback_ch_num_bt = stream_cfg.channel_num;
    mid_p_8_old_l = 0;
    mid_p_8_old_r = 0;
#endif

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    eq_buff_size = stream_cfg.data_size * 2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)

    play_samp_size = (stream_cfg.bits <= AUD_BITS_16) ? 2 : 4;
#if defined(CHIP_BEST2000)
    eq_buff_size = stream_cfg.data_size * sizeof(int32_t) / play_samp_size;
#elif defined(CHIP_BEST1000)
    eq_buff_size = stream_cfg.data_size * sizeof(int16_t) / play_samp_size;
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                     \
    defined(CHIP_BEST2300A)
    eq_buff_size = stream_cfg.data_size;
#endif

#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
    eq_buff_size = stream_cfg.data_size;
#else
    eq_buff_size = 0;
    bt_eq_buff = NULL;
#endif

    if (eq_buff_size > 0) {
      app_audio_mempool_get_buff(&bt_eq_buff, eq_buff_size);
    }

#if defined(IBRT)
    enum AUD_CHANNEL_NUM_T sw_ch_num = AUD_CHANNEL_NUM_1;
#else
    enum AUD_CHANNEL_NUM_T sw_ch_num = stream_cfg.channel_num;
#endif

    audio_process_open(stream_cfg.sample_rate, stream_cfg.bits, sw_ch_num,
                       stream_cfg.channel_num,
                       stream_cfg.data_size / stream_cfg.channel_num /
                           (stream_cfg.bits <= AUD_BITS_16 ? 2 : 4) / 2,
                       bt_eq_buff, eq_buff_size);

#ifdef __SW_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR, 0));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR, 0));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR, 0));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    bt_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,
                    bt_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR, 0));
#endif

#ifdef ANC_APP
    anc_status_record = 0xff;
#endif

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    uint8_t *bt_audio_buff = NULL;
    stream_cfg.bits = sample_size_play_bt;
    stream_cfg.channel_num = playback_ch_num_bt;
    stream_cfg.sample_rate = sample_rate_play_bt;
    stream_cfg.device = AUD_STREAM_USE_MC;
    stream_cfg.vol = 0;
    stream_cfg.handler = audio_mc_data_playback_a2dp;
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

    app_audio_mempool_get_buff(
        &bt_audio_buff, data_size_play_bt * playback_samplerate_ratio_bt);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
    stream_cfg.data_size = data_size_play_bt * playback_samplerate_ratio_bt;

    playback_buf_mc = stream_cfg.data_ptr;
    playback_size_mc = stream_cfg.data_size;

    anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));

    memset(delay_buf_bt, 0, sizeof(delay_buf_bt));

    af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
    // ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);
#endif

    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

#if defined(AUDIO_ANC_FB_MC) && defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
    af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.bits = AUD_BITS_16;
#if defined(__AUDIO_RESAMPLE__)
    stream_cfg.sample_rate = AUD_SAMPRATE_50781;
#else
    stream_cfg.sample_rate = AUD_SAMPRATE_44100;
#endif
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
    stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)LINEIN_CAPTURE_CHANNEL;
    stream_cfg.channel_map =
        (enum AUD_CHANNEL_MAP_T)hal_codec_get_input_path_cfg(
            stream_cfg.io_path);
    stream_cfg.handler = app_linein_pcm_come;
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(linein_audio_cap_buff);
    stream_cfg.data_size = LINEIN_CAPTURE_BUFFER_SIZE;

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
  } else {
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

    audio_process_close();

    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

    app_overlay_unloadall();
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
  }

  isRun = onoff;
  TRACE_AUD_STREAM_I("[LINEIN_PLAYER] end!\n");
  return 0;
}
#endif

int app_bt_stream_open(APP_AUDIO_STATUS *status) {
  int nRet = -1;
  uint16_t player = status->id;
  APP_AUDIO_STATUS next_status;
  enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

  TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] prev:0x%x%s freq:%d", gStreamplayer,
                     player2str(gStreamplayer), freq);
  TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] cur:0x%x%s freq:%d", player,
                     player2str(player), freq);

  APP_AUDIO_STATUS streamToClose;

  if (gStreamplayer != APP_BT_STREAM_INVALID) {
#if !ISOLATED_AUDIO_STREAM_ENABLED
    TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] close 0x%x%s prev opening",
                       gStreamplayer, player2str(gStreamplayer));
    nRet = app_bt_stream_close(gStreamplayer);
    if (nRet) {
      return -1;
    } else {
      streamToClose.id = gStreamplayer;
      app_audio_list_rmv_callback(&streamToClose, &next_status,
                                  APP_BT_SETTING_Q_POS_TAIL, false);
    }
#else
    if (gStreamplayer & player) {
      TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] 0x%x%s has opened", player,
                         player2str(player));
      return -1;
    }

    if (player >= APP_BT_STREAM_BORDER_INDEX) {
      if (APP_BT_INPUT_STREAM_INDEX(gStreamplayer) > 0) {
        TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] close 0x%x%s prev opening",
                           gStreamplayer, player2str(gStreamplayer));
      }
    } else {
      if (APP_BT_OUTPUT_STREAM_INDEX(gStreamplayer) > 0) {
        TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] close 0x%x%s prev opening",
                           gStreamplayer, player2str(gStreamplayer));
        uint16_t player2close = APP_BT_OUTPUT_STREAM_INDEX(gStreamplayer);
        nRet = app_bt_stream_close(player2close);
        if (nRet) {
          return -1;
        } else {
          streamToClose.id = player2close;
          app_audio_list_rmv_callback(&streamToClose, &next_status,
                                      APP_BT_SETTING_Q_POS_TAIL, false);
        }
      }
    }
#endif
  }

  switch (player) {
  case APP_BT_STREAM_HFP_PCM:
  case APP_BT_STREAM_HFP_CVSD:
  case APP_BT_STREAM_HFP_VENDOR:
    nRet = bt_sco_player(true, freq);
    break;
  case APP_BT_STREAM_A2DP_SBC:
  case APP_BT_STREAM_A2DP_AAC:
  case APP_BT_STREAM_A2DP_VENDOR:
    nRet = bt_sbc_player(PLAYER_OPER_START, freq);
    break;
#ifdef __FACTORY_MODE_SUPPORT__
  case APP_FACTORYMODE_AUDIO_LOOP:
    nRet = app_factorymode_audioloop(true, freq);
    break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
  case APP_PLAY_BACK_AUDIO:
    nRet = app_play_audio_onoff(true, status);
    break;
#endif

#ifdef RB_CODEC
  case APP_BT_STREAM_RBCODEC:
    nRet = app_rbplay_audio_onoff(true, 0);
    break;
#endif

#ifdef AUDIO_LINEIN
  case APP_PLAY_LINEIN_AUDIO:
    nRet = app_play_linein_onoff(true);
    break;
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)
  case APP_A2DP_SOURCE_LINEIN_AUDIO:
    nRet = app_a2dp_source_linein_on(true);
    break;
#endif
#if defined(APP_I2S_A2DP_SOURCE)
  case APP_A2DP_SOURCE_I2S_AUDIO:
    nRet = app_a2dp_source_I2S_onoff(true);
    break;
#endif

#ifdef VOICE_DATAPATH
  case APP_BT_STREAM_VOICEPATH:
    nRet = app_voicepath_start_audio_stream();
    break;
#endif
#ifdef __AI_VOICE__
  case APP_BT_STREAM_AI_VOICE:
    nRet = app_ai_voice_start_mic_stream();
    break;
#endif
#ifdef __THIRDPARTY
  case APP_BT_STREAM_THIRDPARTY_VOICE:
    nRet = app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_KWS,
                                                    THIRDPARTY_MIC_OPEN);
    break;
#endif
  default:
    nRet = -1;
    break;
  }

  if (!nRet) {
    gStreamplayer |= player;
    TRACE_AUD_STREAM_I("[STRM_PLAYER][OPEN] updated to  0x%x%s", gStreamplayer,
                       player2str(gStreamplayer));
  }
  return nRet;
}

int app_bt_stream_close(uint16_t player) {
  int nRet = -1;
  TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSE] gStreamplayer: 0x%x%s",
                     gStreamplayer, player2str(gStreamplayer));
  TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSE] player:0x%x%s", gStreamplayer,
                     player2str(gStreamplayer));

  if ((gStreamplayer & player) != player) {
    return -1;
  }

  switch (player) {
  case APP_BT_STREAM_HFP_PCM:
  case APP_BT_STREAM_HFP_CVSD:
  case APP_BT_STREAM_HFP_VENDOR:
    nRet = bt_sco_player(false, APP_SYSFREQ_32K);
    break;
  case APP_BT_STREAM_A2DP_SBC:
  case APP_BT_STREAM_A2DP_AAC:
  case APP_BT_STREAM_A2DP_VENDOR:
    nRet = bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);
    break;
#ifdef __FACTORY_MODE_SUPPORT__
  case APP_FACTORYMODE_AUDIO_LOOP:
    nRet = app_factorymode_audioloop(false, APP_SYSFREQ_32K);
    break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
  case APP_PLAY_BACK_AUDIO:
    nRet = app_play_audio_onoff(false, NULL);
    break;
#endif
#ifdef RB_CODEC
  case APP_BT_STREAM_RBCODEC:
    nRet = app_rbplay_audio_onoff(false, 0);
    break;
#endif

#ifdef AUDIO_LINEIN
  case APP_PLAY_LINEIN_AUDIO:
    nRet = app_play_linein_onoff(false);
    break;
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)
  case APP_A2DP_SOURCE_LINEIN_AUDIO:
    nRet = app_a2dp_source_linein_on(false);
    break;
#endif
#if defined(APP_I2S_A2DP_SOURCE)
  case APP_A2DP_SOURCE_I2S_AUDIO:
    nRet = app_a2dp_source_I2S_onoff(false);
    break;
#endif

#ifdef VOICE_DATAPATH
  case APP_BT_STREAM_VOICEPATH:
    nRet = app_voicepath_stop_audio_stream();
    break;
#endif
#ifdef __AI_VOICE__
  case APP_BT_STREAM_AI_VOICE:
    nRet = app_ai_voice_stop_mic_stream();
    break;
#endif
#ifdef __THIRDPARTY
  case APP_BT_STREAM_THIRDPARTY_VOICE:
    nRet = app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_KWS,
                                                    THIRDPARTY_MIC_CLOSE);
    break;
#endif
  default:
    nRet = -1;
    break;
  }
  if (!nRet) {
    gStreamplayer &= (~player);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSE] updated to 0x%x%s", gStreamplayer,
                       player2str(gStreamplayer));
  }
  return nRet;
}

int app_bt_stream_setup(uint16_t player, uint8_t status) {
  int nRet = -1;

  TRACE_AUD_STREAM_I("[STRM_PLAYER][SETUP] prev:%d%s sample:%d", gStreamplayer,
                     player2str(gStreamplayer), status);
  TRACE_AUD_STREAM_I("[STRM_PLAYER][SETUP] cur:%d%s sample:%d", player,
                     player2str(player), status);

  switch (player) {
  case APP_BT_STREAM_HFP_PCM:
  case APP_BT_STREAM_HFP_CVSD:
  case APP_BT_STREAM_HFP_VENDOR:
    break;
  case APP_BT_STREAM_A2DP_SBC:
  case APP_BT_STREAM_A2DP_AAC:
  case APP_BT_STREAM_A2DP_VENDOR:
    bt_sbc_player_setup(status);
    break;
  default:
    nRet = -1;
    break;
  }

  return nRet;
}

int app_bt_stream_restart(APP_AUDIO_STATUS *status) {
  int nRet = -1;
  uint16_t player = status->id;
  enum APP_SYSFREQ_FREQ_T freq = (enum APP_SYSFREQ_FREQ_T)status->freq;

  TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] prev:%d%s freq:%d", gStreamplayer,
                     player2str(gStreamplayer), freq);
  TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] cur:%d%s freq:%d", player,
                     player2str(player), freq);

  if ((gStreamplayer & player) != player) {
    return -1;
  }

  switch (player) {
  case APP_BT_STREAM_HFP_PCM:
  case APP_BT_STREAM_HFP_CVSD:
  case APP_BT_STREAM_HFP_VENDOR:
    nRet = bt_sco_player(false, freq);
    nRet = bt_sco_player(true, freq);
    break;
  case APP_BT_STREAM_A2DP_SBC:
  case APP_BT_STREAM_A2DP_AAC:
  case APP_BT_STREAM_A2DP_VENDOR:
#if defined(IBRT)
    ibrt_a2dp_status_t a2dp_status;
    a2dp_ibrt_sync_get_status(&a2dp_status);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] state:%d", a2dp_status.state);
    if (a2dp_status.state == BTIF_AVDTP_STRM_STATE_STREAMING) {
      if (app_audio_manager_a2dp_is_active(BT_DEVICE_ID_1)) {
        TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] resume");
        nRet = bt_sbc_player(PLAYER_OPER_STOP, freq);
        nRet = bt_sbc_player(PLAYER_OPER_START, freq);
      } else {
        if (app_ibrt_ui_is_profile_exchanged()) {
          TRACE_AUD_STREAM_I("[STRM_PLAYER][RESTART] force_audio_retrigger");
          app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                                        BT_STREAM_SBC, BT_DEVICE_ID_1,
                                        MAX_RECORD_NUM);
          app_ibrt_if_force_audio_retrigger();
        }
      }
    }
#elif defined(__BT_ONE_BRING_TWO__)
    if (btif_me_get_activeCons() > 1) {
      enum APP_SYSFREQ_FREQ_T sysfreq;

#ifdef A2DP_CP_ACCEL
      sysfreq = APP_SYSFREQ_26M;
#else
      sysfreq = APP_SYSFREQ_104M;
#endif
      app_sysfreq_req(APP_SYSFREQ_USER_BT_A2DP, sysfreq);
      bt_media_volume_ptr_update_by_mediatype(BT_STREAM_SBC);
      app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
    }
#endif
    break;
#ifdef __FACTORY_MODE_SUPPORT__
  case APP_FACTORYMODE_AUDIO_LOOP:
    break;
#endif
#ifdef MEDIA_PLAYER_SUPPORT
  case APP_PLAY_BACK_AUDIO:
    break;
#endif
  default:
    nRet = -1;
    break;
  }

  return nRet;
}

void app_bt_stream_volumeup(void) {
#if defined AUDIO_LINEIN
  if (app_bt_stream_isrun(APP_PLAY_LINEIN_AUDIO)) {
    stream_linein_volume++;
    if (stream_linein_volume > TGT_VOLUME_LEVEL_15)
      stream_linein_volume = TGT_VOLUME_LEVEL_15;
    app_bt_stream_volumeset(stream_linein_volume);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UP] set linein volume %d\n",
                       stream_linein_volume);
  } else
#endif
  {
    if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
      TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UP] hfp volume");

      int8_t updatedVol = btdevice_volume_p->hfp_vol;
      updatedVol++;
      if (updatedVol > TGT_VOLUME_LEVEL_15) {
        updatedVol = TGT_VOLUME_LEVEL_15;
      }
      uint32_t lock = nv_record_pre_write_operation();
      btdevice_volume_p->hfp_vol = updatedVol;
      nv_record_post_write_operation(lock);
      current_btdevice_volume.hfp_vol = updatedVol;
      if (updatedVol < TGT_VOLUME_LEVEL_15) {
        app_bt_stream_volumeset(updatedVol);
      }
      if (btdevice_volume_p->hfp_vol == TGT_VOLUME_LEVEL_15) {
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING, 0);
#endif
      }

    } else if ((app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) ||
               (app_bt_stream_isrun(APP_BT_STREAM_INVALID))) {
      TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UP] a2dp volume");
      int8_t updatedVol = btdevice_volume_p->a2dp_vol;
      updatedVol++;
      if (updatedVol > TGT_VOLUME_LEVEL_15) {
        updatedVol = TGT_VOLUME_LEVEL_15;
      }
      uint32_t lock = nv_record_pre_write_operation();
      btdevice_volume_p->a2dp_vol = updatedVol;
      nv_record_post_write_operation(lock);
      current_btdevice_volume.a2dp_vol = updatedVol;
      if (updatedVol < TGT_VOLUME_LEVEL_15) {
        app_bt_stream_volumeset(updatedVol);
      }
      if (btdevice_volume_p->a2dp_vol == TGT_VOLUME_LEVEL_15) {
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING, 0);
#endif
      }
    }

    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UP]  a2dp: %d",
                       btdevice_volume_p->a2dp_vol);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UP]  hfp: %d",
                       btdevice_volume_p->hfp_vol);
  }
  nv_record_touch_cause_flush();
}

void app_bt_set_volume(uint16_t type, uint8_t level) {
  if ((type & APP_BT_STREAM_HFP_PCM) &&
      app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] set hfp volume");
    if (level >= TGT_VOLUME_LEVEL_MUTE && level <= TGT_VOLUME_LEVEL_15) {
      uint32_t lock = nv_record_pre_write_operation();
      btdevice_volume_p->hfp_vol = level;
      nv_record_post_write_operation(lock);
      app_bt_stream_volumeset(btdevice_volume_p->hfp_vol);
    }
    if (btdevice_volume_p->hfp_vol == TGT_VOLUME_LEVEL_0) {
#ifdef MEDIA_PLAYER_SUPPORT
      media_PlayAudio(AUD_ID_BT_WARNING, 0);
#endif
    }
  }
  if ((type & APP_BT_STREAM_A2DP_SBC) &&
      ((app_bt_stream_isrun(APP_BT_STREAM_INVALID)) ||
       (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)))) {
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] set a2dp volume");
    if (level >= TGT_VOLUME_LEVEL_MUTE && level <= TGT_VOLUME_LEVEL_15) {
      uint32_t lock = nv_record_pre_write_operation();
      btdevice_volume_p->a2dp_vol = level;
      nv_record_post_write_operation(lock);
      app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);
    }
    if (btdevice_volume_p->a2dp_vol == TGT_VOLUME_LEVEL_MUTE) {
#ifdef MEDIA_PLAYER_SUPPORT
      media_PlayAudio(AUD_ID_BT_WARNING, 0);
#endif
    }
  }

  TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] a2dp: %d",
                     btdevice_volume_p->a2dp_vol);
  TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL] hfp: %d", btdevice_volume_p->hfp_vol);
  nv_record_touch_cause_flush();
}

void app_bt_stream_volumedown(void) {
#if defined AUDIO_LINEIN
  if (app_bt_stream_isrun(APP_PLAY_LINEIN_AUDIO)) {
    stream_linein_volume--;
    if (stream_linein_volume < TGT_VOLUME_LEVEL_MUTE)
      stream_linein_volume = TGT_VOLUME_LEVEL_MUTE;
    app_bt_stream_volumeset(stream_linein_volume);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][DONW] set linein volume %d\n",
                       stream_linein_volume);
  } else
#endif
  {
    if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
      TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][DONW] set hfp volume");

      int8_t updatedVol = btdevice_volume_p->hfp_vol;
      updatedVol--;
      if (updatedVol < TGT_VOLUME_LEVEL_0) {
        updatedVol = TGT_VOLUME_LEVEL_0;
      }

      uint32_t lock = nv_record_pre_write_operation();
      btdevice_volume_p->hfp_vol = updatedVol;
      nv_record_post_write_operation(lock);
      current_btdevice_volume.hfp_vol = updatedVol;
      app_bt_stream_volumeset(updatedVol);
      if (btdevice_volume_p->hfp_vol == TGT_VOLUME_LEVEL_0) {
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING, 0);
#endif
      }
    } else if ((app_bt_stream_isrun(APP_BT_STREAM_INVALID)) ||
               (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC))) {
      TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][DONW] set a2dp volume");
      int8_t updatedVol = btdevice_volume_p->a2dp_vol;
      updatedVol--;
      if (updatedVol < TGT_VOLUME_LEVEL_MUTE) {
        updatedVol = TGT_VOLUME_LEVEL_MUTE;
      }

      uint32_t lock = nv_record_pre_write_operation();
      btdevice_volume_p->a2dp_vol = updatedVol;
      nv_record_post_write_operation(lock);
      current_btdevice_volume.a2dp_vol = updatedVol;
      app_bt_stream_volumeset(updatedVol);
      if (btdevice_volume_p->a2dp_vol == TGT_VOLUME_LEVEL_MUTE) {
#ifdef MEDIA_PLAYER_SUPPORT
        media_PlayAudio(AUD_ID_BT_WARNING, 0);
#endif
      }
    }

    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][DONW] a2dp: %d",
                       btdevice_volume_p->a2dp_vol);
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][DONW] hfp: %d",
                       btdevice_volume_p->hfp_vol);
  }
  nv_record_touch_cause_flush();
}

void app_bt_stream_volumeset_handler(int8_t vol) {
  uint32_t ret;
  struct AF_STREAM_CONFIG_T *stream_cfg = NULL;
  ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg,
                          false);
  if (ret == 0) {
    stream_cfg->vol = vol;
    af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
  }
#if (defined(AUDIO_ANC_FB_MC) || defined(AUDIO_ANC_FB_MC_SCO)) &&              \
    defined(ANC_APP) && !defined(__AUDIO_RESAMPLE__)
  ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg,
                          false);
  if (ret == 0) {
    stream_cfg->vol = vol;
    af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, stream_cfg);
  }
#endif
}

int app_bt_stream_volumeset(int8_t vol) {
  TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][SET] vol=%d", vol);

  if (vol > TGT_VOLUME_LEVEL_15)
    vol = TGT_VOLUME_LEVEL_15;
  if (vol < TGT_VOLUME_LEVEL_MUTE)
    vol = TGT_VOLUME_LEVEL_MUTE;

  stream_local_volume = vol;
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
  if ((!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)) &&
      (audio_prompt_is_allow_update_volume()))
#else
  if (!app_bt_stream_isrun(APP_PLAY_BACK_AUDIO))
#endif
  {
    app_bt_stream_volumeset_handler(vol);
  }
  return 0;
}

int app_bt_stream_local_volume_get(void) { return stream_local_volume; }

uint8_t app_bt_stream_a2dpvolume_get(void) {
  // return btdevice_volume_p->a2dp_vol;
  return current_btdevice_volume.a2dp_vol;
}

uint8_t app_bt_stream_hfpvolume_get(void) {
  // return btdevice_volume_p->hfp_vol;
  return current_btdevice_volume.hfp_vol;
}

void app_bt_stream_a2dpvolume_reset(void) {
  btdevice_volume_p->a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT;
  current_btdevice_volume.a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT;
}

void app_bt_stream_hfpvolume_reset(void) {
  btdevice_volume_p->hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
  current_btdevice_volume.hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
}

void app_bt_stream_volume_ptr_update(uint8_t *bdAddr) {
  static struct btdevice_volume stream_volume = {
      NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT,
      NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT};

  nvrec_btdevicerecord *record = NULL;

  memset(&current_btdevice_volume, 0, sizeof(btdevice_volume));

  if (bdAddr &&
      !nv_record_btdevicerecord_find((bt_bdaddr_t *)bdAddr, &record)) {
    btdevice_volume_p = &(record->device_vol);
    DUMP8("0x%02x ", bdAddr, BTIF_BD_ADDR_SIZE);
    TRACE_AUD_STREAM_I(
        "[STRM_PLAYER][VOL][UPDATE] a2dp_vol:%d hfp_vol:%d ptr:%p",
        btdevice_volume_p->a2dp_vol, btdevice_volume_p->hfp_vol,
        btdevice_volume_p);
  } else
  {
    btdevice_volume_p = &stream_volume;
    TRACE_AUD_STREAM_I("[STRM_PLAYER][VOL][UPDATE] default");
    if (bdAddr) {
      DUMP8("0x%02x ", bdAddr, BTIF_BD_ADDR_SIZE);
    }
  }
  current_btdevice_volume.a2dp_vol = btdevice_volume_p->a2dp_vol;
  current_btdevice_volume.hfp_vol = btdevice_volume_p->hfp_vol;
}

struct btdevice_volume *app_bt_stream_volume_get_ptr(void) {
  return btdevice_volume_p;
}

bool app_bt_stream_isrun(uint16_t player) {
  if ((gStreamplayer & player) == player) {
    return true;
  } else {
    return false;
  }
}

int app_bt_stream_closeall() {
  TRACE_AUD_STREAM_I("[STRM_PLAYER][CLOSEALL]");

  bt_sco_player(false, APP_SYSFREQ_32K);
  bt_sbc_player(PLAYER_OPER_STOP, APP_SYSFREQ_32K);

#ifdef MEDIA_PLAYER_SUPPORT
  app_play_audio_onoff(false, 0);
#endif
#ifdef RB_CODEC
  app_rbplay_audio_onoff(false, 0);
#endif

#ifdef VOICE_DATAPATH
  app_voicepath_stop_audio_stream();
#endif

  gStreamplayer = APP_BT_STREAM_INVALID;

  return 0;
}

void app_bt_stream_copy_track_one_to_two_24bits(int32_t *dst_buf,
                                                int32_t *src_buf,
                                                uint32_t src_len) {
  // Copy from tail so that it works even if dst_buf == src_buf
  for (int i = (int)(src_len - 1); i >= 0; i--) {
    dst_buf[i * 2 + 0] = dst_buf[i * 2 + 1] = src_buf[i];
  }
}

void app_bt_stream_copy_track_one_to_two_16bits(int16_t *dst_buf,
                                                int16_t *src_buf,
                                                uint32_t src_len) {
  // Copy from tail so that it works even if dst_buf == src_buf
  for (int i = (int)(src_len - 1); i >= 0; i--) {
    dst_buf[i * 2 + 0] = dst_buf[i * 2 + 1] = src_buf[i];
  }
}

void app_bt_stream_copy_track_two_to_one_16bits(int16_t *dst_buf,
                                                int16_t *src_buf,
                                                uint32_t dst_len) {
  for (uint32_t i = 0; i < dst_len; i++) {
    dst_buf[i] = src_buf[i * 2];
  }
}

void app_bt_stream_adaptive_frequency_adjusting(void) {
#if 0 //(A2DP_DECODER_VER == 2)
    if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)){
        if (!a2dp_audio_sysfreq_boost_running()){
            a2dp_audio_sysfreq_boost_start(1);
        }
    }
#endif
}

#ifdef PLAYBACK_FORCE_48K
static int app_force48k_resample_iter(uint8_t *buf, uint32_t len) {
  uint8_t codec_type = bt_sbc_player_get_codec_type();
  uint32_t overlay_id = 0;
  if (0) {
#if defined(A2DP_LHDC_ON)
  } else if (codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC) {
    overlay_id = APP_OVERLAY_A2DP_LHDC;
#endif
#if defined(A2DP_AAC_ON)
  } else if (codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC) {
    overlay_id = APP_OVERLAY_A2DP_AAC;
#endif
  } else if (codec_type == BTIF_AVDTP_CODEC_TYPE_SBC) {
    overlay_id = APP_OVERLAY_A2DP;
  }
  a2dp_audio_more_data(overlay_id, buf, len);
  return 0;
}

struct APP_RESAMPLE_T *
app_force48k_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
                               APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
                               float ratio_step) {
  return app_playback_resample_any_open(chans, cb, iter_len, ratio_step);
}
#endif

// =======================================================
// APP RESAMPLE
// =======================================================

#ifndef MIX_MIC_DURING_MUSIC
#include "resample_coef.h"
#endif

static APP_RESAMPLE_BUF_ALLOC_CALLBACK resamp_buf_alloc =
    app_audio_mempool_get_buff;

static void memzero_int16(void *dst, uint32_t len) {
  if (dst) {
    int16_t *dst16 = (int16_t *)dst;
    int16_t *dst16_end = dst16 + len / 2;

    while (dst16 < dst16_end) {
      *dst16++ = 0;
    }
  } else {
    TRACE_AUD_STREAM_I("WRN: receive null pointer");
  }
}

struct APP_RESAMPLE_T *app_resample_open_with_preallocated_buf(
    enum AUD_STREAM_T stream, const struct RESAMPLE_COEF_T *coef,
    enum AUD_CHANNEL_NUM_T chans, APP_RESAMPLE_ITER_CALLBACK cb,
    uint32_t iter_len, float ratio_step, uint8_t *buf, uint32_t bufSize) {
  TRACE_AUD_STREAM_I("[STRM_PLAYER][PROMPT_MIXER][OPEN]");
  struct APP_RESAMPLE_T *resamp;
  struct RESAMPLE_CFG_T cfg;
  enum RESAMPLE_STATUS_T status;
  uint32_t size, resamp_size;

  resamp_size = audio_resample_ex_get_buffer_size(chans, AUD_BITS_16,
                                                  coef->phase_coef_num);

  size = sizeof(struct APP_RESAMPLE_T);
  size += ALIGN(iter_len, 4);
  size += resamp_size;

  ASSERT(size < bufSize,
         "Pre-allocated buffer size %d is smaller than the needed size %d",
         bufSize, size);

  resamp = (struct APP_RESAMPLE_T *)buf;
  buf += sizeof(*resamp);
  resamp->stream = stream;
  resamp->cb = cb;
  resamp->iter_buf = buf;
  buf += ALIGN(iter_len, 4);
  resamp->iter_len = iter_len;
  resamp->offset = iter_len;
  resamp->ratio_step = ratio_step;

  memset(&cfg, 0, sizeof(cfg));
  cfg.chans = chans;
  cfg.bits = AUD_BITS_16;
  cfg.ratio_step = ratio_step;
  cfg.coef = coef;
  cfg.buf = buf;
  cfg.size = resamp_size;

  status = audio_resample_ex_open(&cfg, (RESAMPLE_ID *)&resamp->id);
  ASSERT(status == RESAMPLE_STATUS_OK, "%s: Failed to open resample: %d",
         __func__, status);

#ifdef CHIP_BEST1000
  hal_cmu_audio_resample_enable();
#endif

  return resamp;
}

static struct APP_RESAMPLE_T *
app_resample_open(enum AUD_STREAM_T stream, const struct RESAMPLE_COEF_T *coef,
                  enum AUD_CHANNEL_NUM_T chans, APP_RESAMPLE_ITER_CALLBACK cb,
                  uint32_t iter_len, float ratio_step) {
  TRACE_AUD_STREAM_I("[STRM_PLAYER][RESAMPLE][OPEN] ratio: %d/1000",
                     uint32_t(ratio_step * 1000));
  struct APP_RESAMPLE_T *resamp;
  struct RESAMPLE_CFG_T cfg;
  enum RESAMPLE_STATUS_T status;
  uint32_t size, resamp_size;
  uint8_t *buf;

  resamp_size = audio_resample_ex_get_buffer_size(chans, AUD_BITS_16,
                                                  coef->phase_coef_num);

  size = sizeof(struct APP_RESAMPLE_T);
  size += ALIGN(iter_len, 4);
  size += resamp_size;

  resamp_buf_alloc(&buf, size);

  resamp = (struct APP_RESAMPLE_T *)buf;
  buf += sizeof(*resamp);
  resamp->stream = stream;
  resamp->cb = cb;
  resamp->iter_buf = buf;
  buf += ALIGN(iter_len, 4);
  resamp->iter_len = iter_len;
  resamp->offset = iter_len;
  resamp->ratio_step = ratio_step;

  memset(&cfg, 0, sizeof(cfg));
  cfg.chans = chans;
  cfg.bits = AUD_BITS_16;
  cfg.ratio_step = ratio_step;
  cfg.coef = coef;
  cfg.buf = buf;
  cfg.size = resamp_size;

  status = audio_resample_ex_open(&cfg, (RESAMPLE_ID *)&resamp->id);
  ASSERT(status == RESAMPLE_STATUS_OK, "%s: Failed to open resample: %d",
         __func__, status);

#ifdef CHIP_BEST1000
  hal_cmu_audio_resample_enable();
#endif

  return resamp;
}

static int app_resample_close(struct APP_RESAMPLE_T *resamp) {
#ifdef CHIP_BEST1000
  hal_cmu_audio_resample_disable();
#endif

  if (resamp) {
    audio_resample_ex_close((RESAMPLE_ID *)resamp->id);
  }

  return 0;
}

struct APP_RESAMPLE_T *
app_playback_resample_open(enum AUD_SAMPRATE_T sample_rate,
                           enum AUD_CHANNEL_NUM_T chans,
                           APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len) {
  const struct RESAMPLE_COEF_T *coef = NULL;

  if (sample_rate == AUD_SAMPRATE_8000) {
    coef = &resample_coef_8k_to_8p4k;
  } else if (sample_rate == AUD_SAMPRATE_16000) {
    coef = &resample_coef_8k_to_8p4k;
  } else if (sample_rate == AUD_SAMPRATE_32000) {
    coef = &resample_coef_32k_to_50p7k;
  } else if (sample_rate == AUD_SAMPRATE_44100) {
    coef = &resample_coef_44p1k_to_50p7k;
  } else if (sample_rate == AUD_SAMPRATE_48000) {
    coef = &resample_coef_48k_to_50p7k;
  } else {
    ASSERT(false, "%s: Bad sample rate: %u", __func__, sample_rate);
  }

  return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len, 0);
}

#ifdef RESAMPLE_ANY_SAMPLE_RATE
struct APP_RESAMPLE_T *
app_playback_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
                               APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
                               float ratio_step) {
  const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;

  return app_resample_open(AUD_STREAM_PLAYBACK, coef, chans, cb, iter_len,
                           ratio_step);
}

struct APP_RESAMPLE_T *app_playback_resample_any_open_with_pre_allocated_buffer(
    enum AUD_CHANNEL_NUM_T chans, APP_RESAMPLE_ITER_CALLBACK cb,
    uint32_t iter_len, float ratio_step, uint8_t *ptrBuf, uint32_t bufSize) {
  const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;

  return app_resample_open_with_preallocated_buf(AUD_STREAM_PLAYBACK, coef,
                                                 chans, cb, iter_len,
                                                 ratio_step, ptrBuf, bufSize);
}

struct APP_RESAMPLE_T *app_capture_resample_14k7_to_16k(
    enum AUD_CHANNEL_NUM_T chans, APP_RESAMPLE_ITER_CALLBACK cb,
    uint32_t iter_len, float ratio_step, uint8_t *ptrBuf, uint32_t bufSize) {
  const struct RESAMPLE_COEF_T *coef = &resample_coef_44p1k_to_48k;

  return app_resample_open_with_preallocated_buf(AUD_STREAM_PLAYBACK, coef,
                                                 chans, cb, iter_len,
                                                 ratio_step, ptrBuf, bufSize);
}

#endif

int app_playback_resample_close(struct APP_RESAMPLE_T *resamp) {
  return app_resample_close(resamp);
}

int app_playback_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf,
                              uint32_t len) {
  uint32_t in_size, out_size;
  struct RESAMPLE_IO_BUF_T io;
  enum RESAMPLE_STATUS_T status;
  int ret;
  // uint32_t lock;

  if (resamp == NULL) {
    goto _err_exit;
  }

  io.out_cyclic_start = NULL;
  io.out_cyclic_end = NULL;

  if (resamp->offset < resamp->iter_len) {
    io.in = resamp->iter_buf + resamp->offset;
    io.in_size = resamp->iter_len - resamp->offset;
    io.out = buf;
    io.out_size = len;

    // lock = int_lock();
    status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size,
                                   &out_size);
    // int_unlock(lock);
    if (status != RESAMPLE_STATUS_OUT_FULL &&
        status != RESAMPLE_STATUS_IN_EMPTY && status != RESAMPLE_STATUS_DONE) {
      goto _err_exit;
    }

    buf += out_size;
    len -= out_size;
    resamp->offset += in_size;

    ASSERT(len == 0 || resamp->offset == resamp->iter_len,
           "%s: Bad resample offset: len=%d offset=%u iter_len=%u", __func__,
           len, resamp->offset, resamp->iter_len);
  }

  while (len) {
    ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
    if (ret) {
      goto _err_exit;
    }

    io.in = resamp->iter_buf;
    io.in_size = resamp->iter_len;
    io.out = buf;
    io.out_size = len;

    // lock = int_lock();
    status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size,
                                   &out_size);
    // int_unlock(lock);
    if (status != RESAMPLE_STATUS_OUT_FULL &&
        status != RESAMPLE_STATUS_IN_EMPTY && status != RESAMPLE_STATUS_DONE) {
      goto _err_exit;
    }

    ASSERT(out_size <= len, "%s: Bad resample out_size: out_size=%u len=%d",
           __func__, out_size, len);
    ASSERT(in_size <= resamp->iter_len,
           "%s: Bad resample in_size: in_size=%u iter_len=%u", __func__,
           in_size, resamp->iter_len);

    buf += out_size;
    len -= out_size;
    if (in_size != resamp->iter_len) {
      resamp->offset = in_size;

      ASSERT(len == 0, "%s: Bad resample len: len=%d out_size=%u", __func__,
             len, out_size);
    }
  }

  return 0;

_err_exit:
  if (resamp) {
    app_resample_reset(resamp);
  }

  memzero_int16(buf, len);

  return 1;
}

struct APP_RESAMPLE_T *
app_capture_resample_open(enum AUD_SAMPRATE_T sample_rate,
                          enum AUD_CHANNEL_NUM_T chans,
                          APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len) {
  const struct RESAMPLE_COEF_T *coef = NULL;

  if (sample_rate == AUD_SAMPRATE_8000) {
    coef = &resample_coef_8p4k_to_8k;
  } else if (sample_rate == AUD_SAMPRATE_16000) {
    // Same coef as 8K sample rate
    coef = &resample_coef_8p4k_to_8k;
  } else {
    ASSERT(false, "%s: Bad sample rate: %u", __func__, sample_rate);
  }

  return app_resample_open(AUD_STREAM_CAPTURE, coef, chans, cb, iter_len, 0);
}

#ifdef RESAMPLE_ANY_SAMPLE_RATE
struct APP_RESAMPLE_T *
app_capture_resample_any_open(enum AUD_CHANNEL_NUM_T chans,
                              APP_RESAMPLE_ITER_CALLBACK cb, uint32_t iter_len,
                              float ratio_step) {
  const struct RESAMPLE_COEF_T *coef = &resample_coef_any_up256;
  return app_resample_open(AUD_STREAM_CAPTURE, coef, chans, cb, iter_len,
                           ratio_step);
}
#endif

int app_capture_resample_close(struct APP_RESAMPLE_T *resamp) {
  return app_resample_close(resamp);
}

int app_capture_resample_run(struct APP_RESAMPLE_T *resamp, uint8_t *buf,
                             uint32_t len) {
  uint32_t in_size, out_size;
  struct RESAMPLE_IO_BUF_T io;
  enum RESAMPLE_STATUS_T status;
  int ret;

  if (resamp == NULL) {
    goto _err_exit;
  }

  io.out_cyclic_start = NULL;
  io.out_cyclic_end = NULL;

  if (resamp->offset < resamp->iter_len) {
    io.in = buf;
    io.in_size = len;
    io.out = resamp->iter_buf + resamp->offset;
    io.out_size = resamp->iter_len - resamp->offset;

    status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size,
                                   &out_size);
    if (status != RESAMPLE_STATUS_OUT_FULL &&
        status != RESAMPLE_STATUS_IN_EMPTY && status != RESAMPLE_STATUS_DONE) {
      goto _err_exit;
    }

    buf += in_size;
    len -= in_size;
    resamp->offset += out_size;

    ASSERT(len == 0 || resamp->offset == resamp->iter_len,
           "%s: Bad resample offset: len=%d offset=%u iter_len=%u", __func__,
           len, resamp->offset, resamp->iter_len);

    if (resamp->offset == resamp->iter_len) {
      ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
      if (ret) {
        goto _err_exit;
      }
    }
  }

  while (len) {
    io.in = buf;
    io.in_size = len;
    io.out = resamp->iter_buf;
    io.out_size = resamp->iter_len;

    status = audio_resample_ex_run((RESAMPLE_ID *)resamp->id, &io, &in_size,
                                   &out_size);
    if (status != RESAMPLE_STATUS_OUT_FULL &&
        status != RESAMPLE_STATUS_IN_EMPTY && status != RESAMPLE_STATUS_DONE) {
      goto _err_exit;
    }

    ASSERT(in_size <= len, "%s: Bad resample in_size: in_size=%u len=%u",
           __func__, in_size, len);
    ASSERT(out_size <= resamp->iter_len,
           "%s: Bad resample out_size: out_size=%u iter_len=%u", __func__,
           out_size, resamp->iter_len);

    buf += in_size;
    len -= in_size;
    if (out_size == resamp->iter_len) {
      ret = resamp->cb(resamp->iter_buf, resamp->iter_len);
      if (ret) {
        goto _err_exit;
      }
    } else {
      resamp->offset = out_size;

      ASSERT(len == 0, "%s: Bad resample len: len=%u in_size=%u", __func__, len,
             in_size);
    }
  }

  return 0;

_err_exit:
  if (resamp) {
    app_resample_reset(resamp);
  }

  memzero_int16(buf, len);

  return 1;
}

void app_resample_reset(struct APP_RESAMPLE_T *resamp) {
  audio_resample_ex_flush((RESAMPLE_ID *)resamp->id);
  resamp->offset = resamp->iter_len;
}

void app_resample_tune(struct APP_RESAMPLE_T *resamp, float ratio) {
  float new_step;

  if (resamp == NULL) {
    return;
  }

  TRACE_AUD_STREAM_I("%s: stream=%d ratio=%d", __FUNCTION__, resamp->stream,
                     FLOAT_TO_PPB_INT(ratio));

  if (resamp->stream == AUD_STREAM_PLAYBACK) {
    new_step = resamp->ratio_step + resamp->ratio_step * ratio;
  } else {
    new_step = resamp->ratio_step - resamp->ratio_step * ratio;
  }
  audio_resample_ex_set_ratio_step(resamp->id, new_step);
}

APP_RESAMPLE_BUF_ALLOC_CALLBACK
app_resample_set_buf_alloc_callback(APP_RESAMPLE_BUF_ALLOC_CALLBACK cb) {
  APP_RESAMPLE_BUF_ALLOC_CALLBACK old_cb;

  old_cb = resamp_buf_alloc;
  resamp_buf_alloc = cb;

  return old_cb;
}

#ifdef TX_RX_PCM_MASK

#ifdef SCO_DMA_SNAPSHOT

#define MSBC_LEN 60

void store_encode_frame2buff() {
  if (bt_sco_codec_is_msbc()) {
    uint32_t len;
    // processing uplink msbc data.
    if (playback_buf_btpcm_copy != NULL) {
      len = playback_size_btpcm_copy - MSBC_LEN;
      memcpy((uint8_t *)(*(volatile uint32_t *)(MIC_BUFF_ADRR_REG)),
             playback_buf_btpcm_copy, MSBC_LEN);
      memcpy(playback_buf_btpcm_copy, playback_buf_btpcm_copy + MSBC_LEN, len);
    }
    // processing downlink msbc data.
    if (capture_buf_btpcm_copy != NULL) {
      len = capture_size_btpcm_copy - MSBC_LEN;
      memcpy(capture_buf_btpcm_copy, capture_buf_btpcm_copy + MSBC_LEN, len);
      memcpy(capture_buf_btpcm_copy + len,
             (uint8_t *)(*(volatile uint32_t *)(RX_BUFF_ADRR)), MSBC_LEN);
    }
#if defined(CHIP_BEST2300A)
    uint8_t sco_toggle = *(volatile uint8_t *)(RX_BUFF_ADRR + 8);
    pcm_data_param[sco_toggle].curr_time =
        *(volatile uint32_t *)(RX_BUFF_ADRR + 4);
    pcm_data_param[sco_toggle].toggle = sco_toggle;
    pcm_data_param[sco_toggle].flag = *(volatile uint8_t *)(RX_BUFF_ADRR + 9);
    pcm_data_param[sco_toggle].counter =
        *(volatile uint16_t *)(RX_BUFF_ADRR + 10);
#endif
  }
  return;
}
#else
extern CQueue *get_tx_esco_queue_ptr();
extern CQueue *get_rx_esco_queue_ptr();
void store_encode_frame2buff() {
  CQueue *Tx_esco_queue_temp = NULL;
  CQueue *Rx_esco_queue_temp = NULL;
  Tx_esco_queue_temp = get_tx_esco_queue_ptr();
  Rx_esco_queue_temp = get_rx_esco_queue_ptr();
  unsigned int len;
  len = 60;
  int status = 0;
  if (bt_sco_codec_is_msbc()) {
    status =
        DeCQueue(Tx_esco_queue_temp,
                 (uint8_t *)(*(volatile uint32_t *)(MIC_BUFF_ADRR_REG)), len);
    if (status) {
      // TRACE_AUD_STREAM_I("TX DeC Fail");
    }
    status = EnCQueue(Rx_esco_queue_temp,
                      (uint8_t *)(*(volatile uint32_t *)(RX_BUFF_ADRR)), len);
    if (status) {
      // TRACE_AUD_STREAM_I("RX EnC Fail");
    }
  }
}
#endif
#endif

int app_bt_stream_init(void) {
  app_bt_stream_trigger_checker_init();
  return 0;
}
