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
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "hal_aud.h"
#include "hal_sleep.h"
#include "hal_location.h"
#include "hal_codec.h"
#include "string.h"
#include "pmu.h"
#include "analog.h"
#include "audioflinger.h"
#include "usb_audio.h"
#include "usb_audio_sync.h"
#include "usb_audio_app.h"
#include "usb_audio_frm_defs.h"
#include "cmsis.h"
#include "safe_queue.h"
#include "memutils.h"
#include "tgt_hardware.h"
#include "audio_resample_ex.h"
#include "resample_coef.h"

#ifdef _VENDOR_MSG_SUPPT_
#include "usb_vendor_msg.h"
#endif

#if defined(USB_AUDIO_SPEECH)
#include "speech_process.h"
#include "app_overlay.h"
#endif

#if defined(__HW_FIR_DSD_PROCESS__)
#include "dsd_process.h"
#endif

#if defined(AUDIO_ANC_FB_MC)
#include"anc_process.h"
#endif

#include "hw_codec_iir_process.h"
#include "hw_iir_process.h"

#ifdef __AUDIO_RESAMPLE__
#ifdef SW_PLAYBACK_RESAMPLE
#if defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_))
#error "Software playback resample conflicts with ANC/AuxMic"
#endif
#else
#ifdef CHIP_BEST1000
#error "No hardware playback resample support"
#endif
#endif

#ifndef SW_CAPTURE_RESAMPLE
#if defined(CHIP_BEST1000) || defined(CHIP_BEST2000)
#error "No hardware capture resample support"
#endif
#endif
#endif // __AUDIO_RESAMPLE__

#ifdef CODEC_DSD
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__) || defined(__HW_DAC_IIR_EQ_PROCESS__)
#error "EQ conflicts with CODEC_DSD"
#endif
#ifdef FREQ_RESP_EQ
#error "FREQ_RESP_EQ conflicts with CODEC_DSD"
#endif
#ifdef __HW_FIR_DSD_PROCESS__
#error "__HW_FIR_DSD_PROCESS__ conflicts with CODEC_DSD"
#endif
#ifdef AUDIO_ANC_FB_MC
#error "AUDIO_ANC_FB_MC conflicts with CODEC_DSD"
#endif
#ifdef NOISE_GATING
#error "NOISE_GATING conflicts with CODEC_DSD"
#endif
#ifdef NOISE_REDUCTION
#error "NOISE_REDUCTION conflicts with CODEC_DSD"
#endif
#endif

#if defined(__SW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_sw_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_sw_iir_cfg_list[];
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_fir_index = 0;
extern const FIR_CFG_T * const audio_eq_hw_fir_cfg_list[];
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_dac_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_hw_dac_iir_cfg_list[];
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
static uint8_t audio_eq_hw_iir_index = 0;
extern const IIR_CFG_T * const audio_eq_hw_iir_cfg_list[];
#endif

#define KEY_TRACE
//#define UNMUTE_WHEN_SET_VOL
#define STREAM_RATE_BITS_SETUP

#if defined(USB_AUDIO_DYN_CFG) && defined(ANC_L_R_MISALIGN_WORKAROUND)
#ifndef AUDIO_PLAYBACK_24BIT
//#define AUDIO_PLAYBACK_24BIT
#endif
#endif

#if defined(__HW_FIR_DSD_PROCESS__) || defined(CODEC_DSD)
#define DSD_SUPPORT
#endif

#ifdef TARGET_TO_MAX_DIFF
#if defined(USB_AUDIO_RECV_ENABLE) && defined(USB_AUDIO_SEND_ENABLE)
#if defined(__AUDIO_RESAMPLE__) && defined(PLL_TUNE_SAMPLE_RATE)
#define UAUD_SYNC_STREAM_TARGET
#else
#error "TARGET_TO_MAX_DIFF can be enabled only with __AUDIO_RESAMPLE__ and PLL_TUNE_SAMPLE_RATE if both playback and capture streams exist"
#endif
#endif

#ifdef USB_AUDIO_MULTIFUNC
#error "TARGET_TO_MAX_DIFF can NOT be enabled with USB_AUDIO_MULTIFUNC"
#endif
#endif // TARGET_TO_MAX_DIFF

#define VERBOSE_TRACE                   0 //0xFFFF

#ifdef TIMER1_BASE
#define TRACE_TIME(num,s, ...)              TRACE(num+1,"[%u] " s, FAST_TICKS_TO_US(hal_fast_sys_timer_get()), ##__VA_ARGS__)
#else
#define TRACE_TIME(num,s, ...)              TRACE(num+1,"[%X] " s, hal_sys_timer_get(), ##__VA_ARGS__)
#endif

#define USB_AUDIO_CPU_WAKE_USER         HAL_CPU_WAKE_LOCK_USER_3

#define RECV_PAUSED_SAMPLE_RATE_FLAG    (1 << 31)

#define DIFF_ERR_THRESH_PLAYBACK        (sample_rate_recv / 1000 / 2)
#define DIFF_ERR_THRESH_CAPTURE         (sample_rate_send / 1000 / 2)

#define DIFF_SYNC_THRESH_PLAYBACK       10
#define DIFF_SYNC_THRESH_CAPTURE        10

#define DIFF_AVG_CNT                    30

#define MAX_TARGET_RATIO                0.000020f

#define MIN_VOLUME_VAL                  (TGT_VOLUME_LEVEL_MUTE)
#define MAX_VOLUME_VAL                  (TGT_VOLUME_LEVEL_QTY - 1)

#define MIN_CAP_VOLUME_VAL              (TGT_ADC_VOL_LEVEL_0)
#define MAX_CAP_VOLUME_VAL              (TGT_ADC_VOL_LEVEL_QTY - 1)

#define NOISE_GATING_INTERVAL           MS_TO_TICKS(800)
#define NOISE_GATING_THRESH_16BIT       3 // -80dB
#define NOISE_GATING_THRESH_24BIT       (NOISE_GATING_THRESH_16BIT << 8)

#define NOISE_REDUCTION_INTERVAL        MS_TO_TICKS(100)
#define NOISE_REDUCTION_MATCH_CNT       200
#define NOISE_REDUCTION_THRESH_16BIT    100 // -50dB
#define NOISE_REDUCTION_THRESH_24BIT    (NOISE_REDUCTION_THRESH_16BIT << 8)

#define CONFLICT_CMD_TRIG_COUNT         10
#define CONFLICT_CMD_TRIG_INTERVAL      MS_TO_TICKS(20000)
#define CONFLICT_STATE_RESET_INTERVAL   MS_TO_TICKS(10000)

#define CAPTURE_STABLE_INTERVAL         MS_TO_TICKS(200)

#define USB_MAX_XFER_INTERVAL           MS_TO_TICKS(10)

#define USB_XFER_ERR_REPORT_INTERVAL    MS_TO_TICKS(5000)

#define USB_AUDIO_MIN_DBVAL             (-99)

#define USB_AUDIO_VOL_UPDATE_STEP       0.00002

#ifndef USB_AUDIO_KEY_MAP
#ifdef ANDROID_ACCESSORY_SPEC
#ifdef ANDROID_VOICE_CMD_KEY
#define USB_AUDIO_KEY_VOICE_CMD         \
    { USB_AUDIO_HID_VOICE_CMD,      HAL_KEY_CODE_FN4,   KEY_EVENT_SET2(DOWN, UP), },
#else
#define USB_AUDIO_KEY_VOICE_CMD
#endif
#define USB_AUDIO_KEY_MAP               { \
    { USB_AUDIO_HID_PLAY_PAUSE,     HAL_KEY_CODE_FN1,   KEY_EVENT_SET2(DOWN, UP), }, \
    { USB_AUDIO_HID_VOL_UP,         HAL_KEY_CODE_FN2,   KEY_EVENT_SET2(DOWN, UP), }, \
    { USB_AUDIO_HID_VOL_DOWN,       HAL_KEY_CODE_FN3,   KEY_EVENT_SET2(DOWN, UP), }, \
    USB_AUDIO_KEY_VOICE_CMD \
}
#else
#define USB_AUDIO_KEY_MAP               { \
    { USB_AUDIO_HID_PLAY_PAUSE,     HAL_KEY_CODE_FN1,   KEY_EVENT_SET(CLICK),       }, \
    { USB_AUDIO_HID_SCAN_NEXT,      HAL_KEY_CODE_FN1,   KEY_EVENT_SET(DOUBLECLICK), }, \
    { USB_AUDIO_HID_SCAN_PREV,      HAL_KEY_CODE_FN1,   KEY_EVENT_SET(TRIPLECLICK), }, \
    { USB_AUDIO_HID_VOL_UP,         HAL_KEY_CODE_FN2,   KEY_EVENT_SET4(CLICK, DOUBLECLICK, TRIPLECLICK, REPEAT), }, \
    { USB_AUDIO_HID_VOL_DOWN,       HAL_KEY_CODE_FN3,   KEY_EVENT_SET4(CLICK, DOUBLECLICK, TRIPLECLICK, REPEAT), }, \
    { USB_AUDIO_HID_HOOK_SWITCH,    HAL_KEY_CODE_FN4,   KEY_EVENT_SET(CLICK),       }, \
}
#endif
#endif

enum AUDIO_CMD_T {
    AUDIO_CMD_START_PLAY = 0,
    AUDIO_CMD_STOP_PLAY,
    AUDIO_CMD_START_CAPTURE,
    AUDIO_CMD_STOP_CAPTURE,
    AUDIO_CMD_SET_VOLUME,
    AUDIO_CMD_SET_CAP_VOLUME,
    AUDIO_CMD_MUTE_CTRL,
    AUDIO_CMD_CAP_MUTE_CTRL,
    AUDIO_CMD_USB_RESET,
    AUDIO_CMD_USB_DISCONNECT,
    AUDIO_CMD_USB_CONFIG,
    AUDIO_CMD_USB_SLEEP,
    AUDIO_CMD_USB_WAKEUP,
    AUDIO_CMD_RECV_PAUSE,
    AUDIO_CMD_RECV_CONTINUE,
    AUDIO_CMD_SET_RECV_RATE,
    AUDIO_CMD_SET_SEND_RATE,
    AUDIO_CMD_RESET_CODEC,
    AUDIO_CMD_NOISE_GATING,
    AUDIO_CMD_NOISE_REDUCTION,
    AUDIO_CMD_TUNE_RATE,
    AUDIO_CMD_SET_DSD_CFG,

    TEST_CMD_PERF_TEST_POWER,
    TEST_CMD_PA_ON_OFF,

    AUDIO_CMD_QTY
};

enum AUDIO_ITF_STATE_T {
    AUDIO_ITF_STATE_STOPPED = 0,
    AUDIO_ITF_STATE_STARTED,
};

enum AUDIO_STREAM_REQ_USER_T {
    AUDIO_STREAM_REQ_USB = 0,
    AUDIO_STREAM_REQ_ANC,

    AUDIO_STREAM_REQ_USER_QTY,
    AUDIO_STREAM_REQ_USER_ALL = AUDIO_STREAM_REQ_USER_QTY,
};

enum AUDIO_STREAM_STATUS_T {
    AUDIO_STREAM_OPENED = 0,
    AUDIO_STREAM_STARTED,

    AUDIO_STREAM_STATUS_QTY
};

enum AUDIO_STREAM_RUNNING_T {
    AUDIO_STREAM_RUNNING_NULL,
    AUDIO_STREAM_RUNNING_ENABLED,
    AUDIO_STREAM_RUNNING_DISABLED,
};

enum CODEC_CONFIG_LOCK_USER_T {
    CODEC_CONFIG_LOCK_RESTART_PLAY,
    CODEC_CONFIG_LOCK_RESTART_CAP,

    CODEC_CONFIG_LOCK_USER_QTY
};

enum CODEC_MUTE_USER_T {
    CODEC_MUTE_USER_CMD,
    CODEC_MUTE_USER_NOISE_GATING,

    CODEC_MUTE_USER_QTY,
    CODEC_MUTE_USER_ALL = CODEC_MUTE_USER_QTY,
};

struct USB_AUDIO_KEY_MAP_T {
    enum USB_AUDIO_HID_EVENT_T hid_event;
    enum HAL_KEY_CODE_T key_code;
    uint32_t key_event_bitset;
};

STATIC_ASSERT(8 * sizeof(((struct USB_AUDIO_KEY_MAP_T *)0)->key_event_bitset) >= HAL_KEY_EVENT_NUM,
    "key_event_bitset size is too small");

#ifdef USB_AUDIO_DYN_CFG

static enum AUD_SAMPRATE_T sample_rate_play;
static enum AUD_SAMPRATE_T sample_rate_cap;
static enum AUD_SAMPRATE_T sample_rate_recv;
static enum AUD_SAMPRATE_T sample_rate_send;
static enum AUD_SAMPRATE_T new_sample_rate_recv;
static enum AUD_SAMPRATE_T new_sample_rate_send;
#ifdef AUDIO_PLAYBACK_24BIT
static
#ifndef CODEC_DSD
    const
#endif
    uint8_t sample_size_play = 4;
#else
static uint8_t sample_size_play;
#endif
static const uint8_t sample_size_cap = SAMPLE_SIZE_CAPTURE;
static uint8_t sample_size_recv;
static const uint8_t sample_size_send = SAMPLE_SIZE_SEND;
static uint8_t new_sample_size_recv;

#else // !USB_AUDIO_DYN_CFG

static const enum AUD_SAMPRATE_T sample_rate_play = SAMPLE_RATE_PLAYBACK;
static const enum AUD_SAMPRATE_T sample_rate_cap = SAMPLE_RATE_CAPTURE;
static const enum AUD_SAMPRATE_T sample_rate_recv = SAMPLE_RATE_RECV;
static const enum AUD_SAMPRATE_T sample_rate_send = SAMPLE_RATE_SEND;
static
#ifndef CODEC_DSD
    const
#endif
    uint8_t sample_size_play = SAMPLE_SIZE_PLAYBACK;
static const uint8_t sample_size_cap = SAMPLE_SIZE_CAPTURE;
static const uint8_t sample_size_recv = SAMPLE_SIZE_RECV;
static const uint8_t sample_size_send = SAMPLE_SIZE_SEND;

#endif // !USB_AUDIO_DYN_CFG

#if defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_))
#ifdef USB_AUDIO_DYN_CFG
static enum AUD_SAMPRATE_T sample_rate_ref_cap;
#else // !USB_AUDIO_DYN_CFG
#ifdef __AUDIO_RESAMPLE__
static const enum AUD_SAMPRATE_T sample_rate_ref_cap = SAMPLE_RATE_CAPTURE;
#elif defined(USB_AUDIO_16K)
static const enum AUD_SAMPRATE_T sample_rate_ref_cap = SAMPLE_RATE_CAPTURE;
#else
static const enum AUD_SAMPRATE_T sample_rate_ref_cap = (SAMPLE_RATE_RECV % AUD_SAMPRATE_8000) ? AUD_SAMPRATE_44100 : AUD_SAMPRATE_48000;
#endif
#endif // !USB_AUDIO_DYN_CFG
#endif // (CHIP_BEST1000 && (ANC_APP || _DUAL_AUX_MIC_))

static enum AUDIO_ITF_STATE_T playback_state = AUDIO_ITF_STATE_STOPPED;
static enum AUDIO_ITF_STATE_T capture_state = AUDIO_ITF_STATE_STOPPED;
static enum AUDIO_ITF_STATE_T recv_state = AUDIO_ITF_STATE_STOPPED;
static enum AUDIO_ITF_STATE_T send_state = AUDIO_ITF_STATE_STOPPED;

static uint8_t codec_stream_map[AUD_STREAM_NUM][AUDIO_STREAM_STATUS_QTY];
static uint8_t eq_opened;
static enum AUDIO_STREAM_RUNNING_T streams_running_req;

static uint8_t playback_paused;

static uint8_t playback_conflicted;
static uint8_t capture_conflicted;

#ifdef NOISE_GATING
enum NOISE_GATING_CMD_T {
    NOISE_GATING_CMD_NULL,
    NOISE_GATING_CMD_MUTE,
    NOISE_GATING_CMD_UNMUTE,
};
static enum NOISE_GATING_CMD_T noise_gating_cmd = NOISE_GATING_CMD_NULL;
static uint32_t last_high_signal_time;

#ifdef NOISE_REDUCTION
enum NOISE_REDUCTION_CMD_T {
    NOISE_REDUCTION_CMD_NULL,
    NOISE_REDUCTION_CMD_FIRE,
    NOISE_REDUCTION_CMD_RESTORE,
};
static enum NOISE_REDUCTION_CMD_T noise_reduction_cmd = NOISE_REDUCTION_CMD_NULL;
static bool nr_active;
static bool prev_samp_positive[2];
static uint16_t prev_zero_diff[2][2];
static uint16_t cur_zero_diff[2][2];
static uint16_t nr_cont_cnt;
static uint32_t last_nr_restore_time;

static void restore_noise_reduction_status(void);
#endif
#endif

static uint8_t *playback_buf;
static uint32_t playback_size;
static uint32_t playback_pos;

#ifdef AUDIO_ANC_FB_MC
static int32_t playback_samplerate_ratio;
#endif

static uint8_t *capture_buf;
static uint32_t capture_size;
static uint32_t capture_pos;

static uint8_t *playback_eq_buf;
static uint32_t playback_eq_size;

#ifdef USB_AUDIO_DYN_CFG
#ifdef KEEP_SAME_LATENCY
static uint32_t playback_max_size;
static uint32_t capture_max_size;
static uint32_t usb_recv_max_size;
static uint32_t usb_send_max_size;
#endif
static uint8_t playback_itf_set;
static uint8_t capture_itf_set;
#endif

#ifdef SW_CAPTURE_RESAMPLE
static RESAMPLE_ID resample_id;
static bool resample_cap_enabled;
static uint8_t *resample_history_buf;
static uint32_t resample_history_size;
static uint8_t *resample_input_buf;
static uint32_t resample_input_size;
#endif

static uint8_t *usb_recv_buf;
static uint32_t usb_recv_size;
static uint32_t usb_recv_rpos;
static uint32_t usb_recv_wpos;

static uint8_t *usb_send_buf;
static uint32_t usb_send_size;
static uint32_t usb_send_rpos;
static uint32_t usb_send_wpos;

static uint8_t usb_configured;

static uint8_t usb_recv_valid;
static uint8_t usb_send_valid;

static uint8_t usb_recv_init_rpos;
static uint8_t usb_send_init_wpos;

static uint8_t codec_play_seq;
static uint8_t codec_cap_seq;
static volatile uint8_t usb_recv_seq;
static volatile uint8_t usb_send_seq;

static uint8_t codec_play_valid;
static uint8_t codec_cap_valid;

#ifdef USB_AUDIO_MULTIFUNC
static const uint8_t playback_vol = AUDIO_OUTPUT_VOLUME_DEFAULT;
#else
static uint8_t playback_vol;
#endif
static uint8_t new_playback_vol;
static uint8_t new_mute_state;
static uint8_t mute_user_map;
STATIC_ASSERT(sizeof(mute_user_map) * 8 >= CODEC_MUTE_USER_QTY, "mute_user_map size too small");

static uint8_t capture_vol;
static uint8_t new_capture_vol;
static uint8_t new_cap_mute_state;

static uint32_t last_usb_recv_err_time;
static uint32_t usb_recv_err_cnt;
static uint32_t usb_recv_ok_cnt;

static uint32_t last_usb_send_err_time;
static uint32_t usb_send_err_cnt;
static uint32_t usb_send_ok_cnt;

#ifdef USB_AUDIO_MULTIFUNC
static uint8_t playback_conflicted2;
static enum AUDIO_ITF_STATE_T recv2_state = AUDIO_ITF_STATE_STOPPED;
static uint8_t *usb_recv2_buf;
static uint32_t usb_recv2_size;
static uint32_t usb_recv2_rpos;
static uint32_t usb_recv2_wpos;
static uint8_t usb_recv2_valid;
static uint8_t usb_recv2_init_rpos;
static uint8_t new_playback2_vol;
static uint8_t new_mute2_state;
static float new_playback_coef;
static float new_playback2_coef;
static float playback_coef;
static float playback2_coef;

static uint32_t last_usb_recv2_err_time;
static uint32_t usb_recv2_err_cnt;
static uint32_t usb_recv2_ok_cnt;

static float playback_gain_to_float(uint32_t level);
#endif

static uint8_t codec_config_lock;
STATIC_ASSERT(sizeof(codec_config_lock) * 8 >= CODEC_CONFIG_LOCK_USER_QTY, "codec_config_lock size too small");

static struct USB_AUDIO_STREAM_INFO_T playback_info;
static struct USB_AUDIO_STREAM_INFO_T capture_info;

static uint8_t nonconflict_start;
static uint16_t conflict_cnt;
static uint32_t conflict_time;
static uint32_t nonconflict_time;

static uint32_t codec_cap_start_time;

static uint32_t last_usb_recv_time;
static uint32_t last_usb_send_time;

#define CMD_LIST_SIZE                   30
static uint32_t cmd_list[CMD_LIST_SIZE];
static uint32_t cmd_watermark;
static struct SAFE_QUEUE_T cmd_queue;

#define EXTRACT_CMD(d)                  ((d) & 0xFF)
#define EXTRACT_SEQ(d)                  (((d) >> 8) & 0xFF)
#define EXTRACT_ARG(d)                  (((d) >> 16) & 0xFF)
#define MAKE_QUEUE_DATA(c, s, a)        (((c) & 0xFF) | ((s) & 0xFF) << 8 | ((a) & 0xFF) << 16)
STATIC_ASSERT(AUDIO_CMD_QTY <= 0xFF, "audio cmd num exceeds size in queue");
STATIC_ASSERT(USB_AUDIO_STATE_EVENT_QTY <= 0xFF, "uaud evt num exceeds size in queue");
STATIC_ASSERT(sizeof(usb_recv_seq) <= 1, "usb recv seq exceeds size in queue");
STATIC_ASSERT(sizeof(usb_send_seq) <= 1, "usb send seq exceeds size in queue");

static const struct USB_AUDIO_KEY_MAP_T key_map[] = USB_AUDIO_KEY_MAP;

#ifdef PERF_TEST_POWER_KEY
static enum HAL_CODEC_PERF_TEST_POWER_T perft_power_type;
#endif
#ifdef PA_ON_OFF_KEY
static bool pa_on_off_muted;
#endif

static float rate_tune_ratio[AUD_STREAM_NUM];

#ifdef DSD_SUPPORT
static bool usb_dsd_enabled;
static bool codec_dsd_enabled;
static uint16_t usb_dsd_cont_cnt = 0;
#ifdef CODEC_DSD
static uint8_t dsd_saved_sample_size;
#endif
#endif

#ifdef BT_USB_AUDIO_DUAL_MODE
static USB_AUDIO_ENQUEUE_CMD_CALLBACK enqueue_cmd_cb;
#endif

static void enqueue_unique_cmd(enum AUDIO_CMD_T cmd);
static void usb_audio_set_codec_volume(enum AUD_STREAM_T stream, uint8_t vol);
static void usb_audio_cmd_tune_rate(enum AUD_STREAM_T stream);

#if defined(CHIP_BEST1000) && defined(_DUAL_AUX_MIC_)

extern void damic_init(void);
extern void damic_deinit(void);

// second-order IR filter
short soir_filter(int32_t PcmValue)
{
    float gain = 2.0F;
    const float NUM[3] = {0.013318022713065147, 0.02375408448278904, 0.010780527256429195};
    const float DEN[3] = {1, -1.6983977556228638,  0.74625039100646973};

    static float y0 = 0, y1 = 0, y2 = 0, x0 = 0, x1 = 0, x2 = 0;
    int32_t PcmOut = 0;

    // Left channel
    x0 = PcmValue* gain;
    y0 = x0*NUM[0] + x1*NUM[1] + x2*NUM[2] - y1*DEN[1] - y2*DEN[2];
    y2 = y1;
    y1 = y0;
    x2 = x1;
    x1 = x0;

    PcmOut = (int32_t)y0;

    PcmOut = __SSAT(PcmOut,16);

    return (short)PcmOut;
}

#ifdef DUAL_AUX_MIC_MORE_FILTER
// second-order IR filter
short soir_filter1(int32_t PcmValue)
{
    //float gain = 0.003031153698;
    const float NUM[3] = {0.013318022713065147, 0.02375408448278904, 0.010780527256429195};
    const float DEN[3] = {1, -1.6983977556228638,  0.74625039100646973};

    static float y0 = 0, y1 = 0, y2 = 0, x0 = 0, x1 = 0, x2 = 0;
    int32_t PcmOut = 0;

    // Left channel
    //x0 = PcmValue* gain;
    x0 = PcmValue;
    y0 = x0*NUM[0] + x1*NUM[1] + x2*NUM[2] - y1*DEN[1] - y2*DEN[2];
    y2 = y1;
    y1 = y0;
    x2 = x1;
    x1 = x0;

    PcmOut = (int32_t)y0;

    PcmOut = __SSAT(PcmOut,16);

    return (short)PcmOut;
}

// second-order IR filter
short soir_filter2(int32_t PcmValue)
{
    //float gain = 0.003031153698;
    const float NUM[3] = {0.013318022713065147, 0.02375408448278904, 0.010780527256429195};
    const float DEN[3] = {1, -1.6983977556228638,  0.74625039100646973};

    static float y0 = 0, y1 = 0, y2 = 0, x0 = 0, x1 = 0, x2 = 0;
    int32_t PcmOut = 0;

    // Left channel
    //x0 = PcmValue* gain;
    x0 = PcmValue;
    y0 = x0*NUM[0] + x1*NUM[1] + x2*NUM[2] - y1*DEN[1] - y2*DEN[2];
    y2 = y1;
    y1 = y0;
    x2 = x1;
    x1 = x0;

    PcmOut = (int32_t)y0;

    PcmOut = __SSAT(PcmOut,16);

    return (short)PcmOut;
}
#endif

static float s_f_amic3_dc;
static float s_f_amic4_dc;
static short s_amic3_dc;
static short s_amic4_dc;

static void init_amic_dc (void)
{
    s_amic3_dc = 0;
    s_amic4_dc = 0;
    s_f_amic3_dc = 0.0;
    s_f_amic4_dc = 0.0;
}

static void get_amic_dc (short* src, uint32_t samp_cnt, uint32_t step)
{
    uint32_t i = 0;

    float u3_1 = 0.00001;
    float u3_2 = 1 - u3_1;

    float u4_1 = 0.00001;
    float u4_2 = 1 - u4_1;

    for (i = 0; i < samp_cnt; i+=(2<<step)) {
        s_f_amic3_dc = u3_2*s_f_amic3_dc + u3_1*src[i<<1];
        s_f_amic4_dc = u4_2*s_f_amic4_dc + u4_1*src[(i<<1)+1];
    }

    s_amic3_dc = __SSAT((int)s_f_amic3_dc, 16);
    s_amic4_dc = __SSAT((int)s_f_amic4_dc, 16);
}
#endif

#ifdef FREQ_RESP_EQ

#ifndef AUDIO_PLAYBACK_24BIT
#error "FREQ_RESP_EQ requires AUDIO_PLAYBACK_24BIT"
#endif

#define FREQ_RESP_EQ_COEF_NUM           7
static float freq_resp_eq_coef[FREQ_RESP_EQ_COEF_NUM] = {
//        -0.000155,  0.000774,  -0.003854,  0.994971,  -0.003854,  0.000774,  -0.000155
        -0.000138,  0.000687,  -0.003426,  0.990852,  -0.003426,  0.000687,  -0.000138
};

static int32_t freq_resp_eq_hist[2][(FREQ_RESP_EQ_COEF_NUM - 1) * 2];
static uint8_t freq_resp_eq_hist_idx;

void freq_resp_eq_init(void)
{
    int i;

    freq_resp_eq_hist_idx = 0;
    for (i = 0; i < ARRAY_SIZE(freq_resp_eq_hist[0]); i++) {
        freq_resp_eq_hist[0][i] = 0;
        freq_resp_eq_hist[1][i] = 0;
    }
}

int freq_resp_eq_run(uint8_t *buf, uint32_t len)
{
    ASSERT(sample_size_play == 4, "Freq resp eq is running on 24-bit playback only");

    if (sample_rate_play > AUD_SAMPRATE_96000) {
        // Sample rate higher than 96K will require higher system freq, which consumes more power
        return 0;
    }

    int32_t *src_buf = (int32_t *)buf;
    int32_t *dst_buf = (int32_t *)buf;
    uint32_t mono_samp_num = len / 2 / sample_size_play;
    int32_t start_idx, in_idx, coef_idx;
    int32_t in_l, in_r;
    float out_l, out_r;
    uint8_t new_seq_idx = !freq_resp_eq_hist_idx;

    if (mono_samp_num >= (FREQ_RESP_EQ_COEF_NUM - 1)) {
        start_idx = (mono_samp_num - FREQ_RESP_EQ_COEF_NUM + 1) * 2;
        for (in_idx = 0; in_idx < (FREQ_RESP_EQ_COEF_NUM - 1) * 2; in_idx++) {
            freq_resp_eq_hist[new_seq_idx][in_idx] = src_buf[start_idx + in_idx];
        }
    } else {
        start_idx = (FREQ_RESP_EQ_COEF_NUM - 1 - mono_samp_num) * 2;
        for (in_idx = 0; in_idx < start_idx; in_idx++) {
            freq_resp_eq_hist[new_seq_idx][in_idx] = freq_resp_eq_hist[freq_resp_eq_hist_idx][mono_samp_num * 2 + in_idx];
        }
        for (in_idx = 0; in_idx < mono_samp_num * 2; in_idx++) {
            freq_resp_eq_hist[new_seq_idx][start_idx + in_idx] = src_buf[in_idx];
        }
    }

    for (in_idx = mono_samp_num - 1; in_idx >= 0; in_idx--) {
        out_l = 0;
        out_r = 0;
        for (coef_idx = 0; coef_idx < FREQ_RESP_EQ_COEF_NUM; coef_idx++) {
            if (in_idx + coef_idx - FREQ_RESP_EQ_COEF_NUM + 1 >= 0) {
                in_l = src_buf[(in_idx + coef_idx - FREQ_RESP_EQ_COEF_NUM + 1) * 2];
                in_r = src_buf[(in_idx + coef_idx - FREQ_RESP_EQ_COEF_NUM + 1) * 2 + 1];
            } else {
                in_l = freq_resp_eq_hist[freq_resp_eq_hist_idx][(in_idx + coef_idx) * 2];
                in_r = freq_resp_eq_hist[freq_resp_eq_hist_idx][(in_idx + coef_idx) * 2 + 1];
            }
            out_l += in_l * freq_resp_eq_coef[coef_idx];
            out_r += in_r * freq_resp_eq_coef[coef_idx];
        }

        // TODO: Is it really neccessary to round the float?
        //dst_buf[in_idx * 2]     = __SSAT(ftoi_nearest(out_l), 24);
        //dst_buf[in_idx * 2 + 1] = __SSAT(ftoi_nearest(out_r), 24);
        dst_buf[in_idx * 2]     = __SSAT((int32_t)out_l, 24);
        dst_buf[in_idx * 2 + 1] = __SSAT((int32_t)out_r, 24);
    }

    freq_resp_eq_hist_idx = new_seq_idx;

    return 0;
}

#endif // FREQ_RESP_EQ

static enum AUD_BITS_T sample_size_to_enum_playback(uint32_t size)
{
    if (size == 2) {
        return AUD_BITS_16;
    } else if (size == 4) {
        return AUD_BITS_24;
    } else {
        ASSERT(false, "%s: Invalid sample size: %u", __FUNCTION__, size);
    }

    return 0;
}

static enum AUD_BITS_T sample_size_to_enum_capture(uint32_t size)
{
    if (size == 2) {
        return AUD_BITS_16;
    } else if (size == 4) {
#ifdef USB_AUDIO_SEND_32BIT
        return AUD_BITS_32;
#else
        return AUD_BITS_24;
#endif
    } else {
        ASSERT(false, "%s: Invalid sample size: %u", __FUNCTION__, size);
    }

    return 0;
}

static enum AUD_CHANNEL_NUM_T chan_num_to_enum(uint32_t num)
{
    return AUD_CHANNEL_NUM_1 + (num - 1);
}

static uint32_t POSSIBLY_UNUSED byte_to_samp_playback(uint32_t n)
{
#if defined(USB_AUDIO_DYN_CFG) || defined(CODEC_DSD)
    return n / sample_size_play / CHAN_NUM_PLAYBACK;
#else
    return BYTE_TO_SAMP_PLAYBACK(n);
#endif
}

static uint32_t POSSIBLY_UNUSED byte_to_samp_capture(uint32_t n)
{
#ifdef USB_AUDIO_DYN_CFG
    return n / sample_size_cap / CHAN_NUM_CAPTURE;
#else
    return BYTE_TO_SAMP_CAPTURE(n);
#endif
}

static uint32_t POSSIBLY_UNUSED byte_to_samp_recv(uint32_t n)
{
#ifdef USB_AUDIO_DYN_CFG
    return n / sample_size_recv / CHAN_NUM_RECV;
#else
    return BYTE_TO_SAMP_RECV(n);
#endif
}

static uint32_t POSSIBLY_UNUSED byte_to_samp_send(uint32_t n)
{
#ifdef USB_AUDIO_DYN_CFG
    return n / sample_size_send / CHAN_NUM_SEND;
#else
    return BYTE_TO_SAMP_SEND(n);
#endif
}

static uint32_t POSSIBLY_UNUSED samp_to_byte_playback(uint32_t n)
{
#if defined(USB_AUDIO_DYN_CFG) || defined(CODEC_DSD)
    return n * sample_size_play * CHAN_NUM_PLAYBACK;
#else
    return SAMP_TO_BYTE_PLAYBACK(n);
#endif
}

static uint32_t POSSIBLY_UNUSED samp_to_byte_capture(uint32_t n)
{
#ifdef USB_AUDIO_DYN_CFG
    return n * sample_size_cap * CHAN_NUM_CAPTURE;
#else
    return SAMP_TO_BYTE_CAPTURE(n);
#endif
}

static uint32_t POSSIBLY_UNUSED samp_to_byte_recv(uint32_t n)
{
#ifdef USB_AUDIO_DYN_CFG
    return n * sample_size_recv * CHAN_NUM_RECV;
#else
    return SAMP_TO_BYTE_RECV(n);
#endif
}

static uint32_t POSSIBLY_UNUSED samp_to_byte_send(uint32_t n)
{
#ifdef USB_AUDIO_DYN_CFG
    return n * sample_size_send * CHAN_NUM_SEND;
#else
    return SAMP_TO_BYTE_SEND(n);
#endif
}

static uint32_t playback_to_recv_len(uint32_t n)
{
#if defined(USB_AUDIO_DYN_CFG) || defined(CODEC_DSD)
    uint32_t len;

    // 1) When changing recv sample rate, the play stream will be stopped and then restarted.
    //    So when calculating the len, play sample rate has been changed according to recv sample rate.
    // 2) Play and recv sample rates are integral multiple of each other.

    len = samp_to_byte_recv(byte_to_samp_playback(n));
    if (sample_rate_recv == sample_rate_play) {
        return len;
    } else if (sample_rate_recv < sample_rate_play) {
        return len / (sample_rate_play / sample_rate_recv);
    } else {
        return len * (sample_rate_recv / sample_rate_play);
    }
#else
    return PLAYBACK_TO_RECV_LEN(n);
#endif
}

static uint32_t capture_to_send_len(uint32_t n)
{
#ifdef USB_AUDIO_DYN_CFG
    uint32_t len;
    enum AUD_SAMPRATE_T send_rate;
    enum AUD_SAMPRATE_T cap_rate;

    // 1) When changing send sample rate, the cap stream will not be changed if the play stream is active.
    //    Moreover, the cap stream might be changed if changing recv sample rate (and then play sample rate).
    //    So when calculating the len, cap sample rate might be inconsistent with send sample rate.
    // 2) Resample might be applied to the cap stream. Cap and send sample rates have no integral multiple relationship.

    send_rate = sample_rate_send;
    cap_rate = sample_rate_cap;

    len = samp_to_byte_send(byte_to_samp_capture(n));
    if (send_rate == cap_rate) {
        return len;
    } else {
        // Unlikely to overflow for the max send_rate is 48000
        return ALIGN((len * send_rate + cap_rate - 1) / cap_rate, sample_size_send * CHAN_NUM_SEND);
    }
#else
    return CAPTURE_TO_SEND_LEN(n);
#endif
}

#ifdef USB_AUDIO_DYN_CFG
static uint32_t POSSIBLY_UNUSED capture_to_ref_len(uint32_t n)
{
    uint32_t len;

    len = samp_to_byte_send(byte_to_samp_capture(n));

#if defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_))
    // Capture reference sample rate and capture sample rate are integral multiple of each other.

    if (sample_rate_ref_cap == sample_rate_cap) {
        return len;
    } else if (sample_rate_ref_cap < sample_rate_cap) {
        return len / (sample_rate_cap / sample_rate_ref_cap);
    } else {
        return len * (sample_rate_ref_cap / sample_rate_cap);
    }
#else
    return len;
#endif
}

#ifdef KEEP_SAME_LATENCY
static uint32_t calc_usb_recv_size(enum AUD_SAMPRATE_T rate, uint8_t sample_size)
{
    uint32_t size;
    size = (SAMP_RATE_TO_FRAME_SIZE(rate) * sample_size * CHAN_NUM_RECV) * (usb_recv_max_size / MAX_FRAME_SIZE_RECV);
    return NON_EXP_ALIGN(size, RECV_BUFF_ALIGN);
}

static uint32_t calc_usb_send_size(enum AUD_SAMPRATE_T rate)
{
    uint32_t size;
    size = samp_to_byte_send(SAMP_RATE_TO_FRAME_SIZE(rate)) * usb_send_max_size / MAX_FRAME_SIZE_SEND;
    return NON_EXP_ALIGN(size, SEND_BUFF_ALIGN);
}

static uint32_t calc_playback_size(enum AUD_SAMPRATE_T rate)
{
    uint32_t size;
    size = samp_to_byte_playback(SAMP_RATE_TO_FRAME_SIZE(rate)) * (playback_max_size / MAX_FRAME_SIZE_PLAYBACK);
    return NON_EXP_ALIGN(size, DAC_BUFF_ALIGN);
}

static uint32_t calc_capture_size(enum AUD_SAMPRATE_T rate)
{
    uint32_t size;
    size = samp_to_byte_capture(SAMP_RATE_TO_FRAME_SIZE(rate)) * (capture_max_size / MAX_FRAME_SIZE_CAPTURE);
    return NON_EXP_ALIGN(size, ADC_BUFF_ALIGN);
}
#endif
#endif

static void record_conflict(int conflicted)
{
    uint32_t lock;
    uint32_t interval;
    bool reset = false;

    lock = int_lock();

    if (conflicted) {
        nonconflict_start = 0;
        if (conflict_cnt == 0) {
            conflict_time = hal_sys_timer_get();
        }
        conflict_cnt++;

        if (conflict_cnt >= CONFLICT_CMD_TRIG_COUNT) {
            interval = hal_sys_timer_get() - conflict_time;
            if (interval < CONFLICT_CMD_TRIG_INTERVAL) {
                //TRACE(2,"RESET CODEC: conflict cnt=%u interval=%u ms", conflict_cnt, TICKS_TO_MS(interval));
                reset = true;
            }
            conflict_cnt = 0;
        }
    } else {
        if (nonconflict_start == 0) {
            nonconflict_start = 1;
            if (conflict_cnt) {
                nonconflict_time = hal_sys_timer_get();
            }
        } else {
            if (conflict_cnt) {
                interval = hal_sys_timer_get() - nonconflict_time;
                if (interval >= CONFLICT_STATE_RESET_INTERVAL) {
                    //TRACE(2,"RESET CONFLICT STATE: conflict cnt=%u nonconflict interval=%u ms", conflict_cnt, TICKS_TO_MS(interval));
                    conflict_cnt = 0;
                }
            }
        }
    }

    int_unlock(lock);

    if (reset) {
        // Avoid invoking cmd callback when irq is locked
        enqueue_unique_cmd(AUDIO_CMD_RESET_CODEC);
    }
}

static void reset_conflict(void)
{
    uint32_t lock;

    lock = int_lock();
    conflict_cnt = 0;
    nonconflict_start = 0;
    int_unlock(lock);
}

uint8_t usb_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status)
{
    uint8_t index_eq=0;

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if(anc_status)
            {
                index_eq=audio_eq_sw_iir_index+1;
            }
            else
            {
                index_eq=audio_eq_sw_iir_index;
            }

        }
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
#ifdef USB_AUDIO_DYN_CFG
            if(sample_rate_recv == AUD_SAMPRATE_44100) {
                index_eq = 0;
            } else if(sample_rate_recv == AUD_SAMPRATE_48000) {
                index_eq = 1;
            } else if(sample_rate_recv == AUD_SAMPRATE_96000) {
                index_eq = 2;
            } else if(sample_rate_recv == AUD_SAMPRATE_192000) {
                index_eq = 3;
            } else {
                ASSERT(0, "[%s] sample_rate_recv(%d) is not supported", __func__, sample_rate_recv);
            }
            audio_eq_hw_fir_index=index_eq;
#else
            index_eq=audio_eq_hw_fir_index;
#endif
            if(anc_status)
            {
                index_eq=index_eq+4;
            }
        }
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if(anc_status)
            {
                index_eq=audio_eq_hw_dac_iir_index+1;
            }
            else
            {
                index_eq=audio_eq_hw_dac_iir_index;
            }
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(anc_status)
            {
                index_eq=audio_eq_hw_iir_index+1;
            }
            else
            {
                index_eq=audio_eq_hw_iir_index;
            }
        }
        break;
#endif
        default:
        {
            ASSERT(false,"[%s]Error eq type!",__func__);
        }
    }
#endif
    return index_eq;
}


uint32_t usb_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t index)
{
    const FIR_CFG_T *fir_cfg=NULL;
    const IIR_CFG_T *iir_cfg=NULL;

    TRACE(3,"[%s]audio_eq_type=%d,index=%d", __func__, audio_eq_type,index);

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    switch (audio_eq_type)
    {
#if defined(__SW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_SW_IIR:
        {
            if(index >= EQ_SW_IIR_LIST_NUM)
            {
                TRACE(2,"[%s] index[%d] > EQ_SW_IIR_LIST_NUM", __func__, index);
                return 1;
            }

            iir_cfg=audio_eq_sw_iir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_FIR:
        {
            if(index >= EQ_HW_FIR_LIST_NUM)
            {
                TRACE(2,"[%s] index[%d] > EQ_HW_FIR_LIST_NUM", __func__, index);
                return 1;
            }

            fir_cfg=audio_eq_hw_fir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_DAC_IIR:
        {
            if(index >= EQ_HW_DAC_IIR_LIST_NUM)
            {
                TRACE(2,"[%s] index[%d] > EQ_HW_DAC_IIR_LIST_NUM", __func__, index);
                return 1;
            }

            iir_cfg=audio_eq_hw_dac_iir_cfg_list[index];
        }
        break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
        case AUDIO_EQ_TYPE_HW_IIR:
        {
            if(index >= EQ_HW_IIR_LIST_NUM)
            {
                TRACE(2,"[%s] index[%d] > EQ_HW_IIR_LIST_NUM", __func__, index);
                return 1;
            }

            iir_cfg=audio_eq_hw_iir_cfg_list[index];
        }
        break;
#endif
        default:
        {
            ASSERT(false,"[%s]Error eq type!",__func__);
        }
    }
#endif

    return audio_eq_set_cfg(fir_cfg,iir_cfg,audio_eq_type);
}

static void set_codec_config_status(enum CODEC_CONFIG_LOCK_USER_T user, bool lock)
{
    if (lock) {
        codec_config_lock |= (1 << user);
    } else {
        codec_config_lock &= ~(1 << user);
    }
}

static void usb_audio_codec_mute(enum CODEC_MUTE_USER_T user)
{
    TRACE(3,"%s: user=%d map=0x%02X", __FUNCTION__, user, mute_user_map);

    if (user >= CODEC_MUTE_USER_QTY || mute_user_map == 0) {
        af_stream_mute(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, true);
    }
    if (user < CODEC_MUTE_USER_QTY) {
        mute_user_map |= (1 << user);
    }
}

static void usb_audio_codec_unmute(enum CODEC_MUTE_USER_T user)
{
    uint8_t old_map;
    STATIC_ASSERT(sizeof(old_map) * 8 >= CODEC_MUTE_USER_QTY, "old_map size too small");

    TRACE(3,"%s: user=%d map=0x%02X", __FUNCTION__, user, mute_user_map);

    old_map = mute_user_map;

    if (user < CODEC_MUTE_USER_QTY) {
        mute_user_map &= ~(1 << user);
    }
    if (user >= CODEC_MUTE_USER_QTY || (old_map && mute_user_map == 0)) {
        af_stream_mute(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, false);
    }
}

#ifdef AUDIO_ANC_FB_MC

#define DELAY_SAMPLE_MC (33*2)     //  2:ch
static int32_t delay_buf[DELAY_SAMPLE_MC];
static int32_t mid_p_8_old_l=0;
static int32_t mid_p_8_old_r=0;
static uint32_t audio_mc_data_playback(uint8_t *buf, uint32_t mc_len_bytes)
{
    //uint32_t begin_time;
    //uint32_t end_time;
    //begin_time = hal_sys_timer_get();
    //TRACE(1,"cancel: %d",begin_time);

    float left_gain;
    float right_gain;
    uint32_t playback_len_bytes;
    int i,j,k;
    int delay_sample;

    hal_codec_get_dac_gain(&left_gain,&right_gain);

    //TRACE(1,"playback_samplerate_ratio:  %d",playback_samplerate_ratio);

    //TRACE(1,"left_gain:  %d",(int)(left_gain*(1<<12)));
    //TRACE(1,"right_gain: %d",(int)(right_gain*(1<<12)));

    playback_len_bytes=mc_len_bytes/playback_samplerate_ratio;

    if (sample_size_play == 2)
    {
        int16_t *sour_p=(int16_t *)(playback_buf+playback_size/2);
        int16_t *mid_p=(int16_t *)(buf);
        int16_t *mid_p_8=(int16_t *)(buf+mc_len_bytes-mc_len_bytes/8);
        int16_t *dest_p=(int16_t *)buf;
        int i,j,k;

        if(buf == (playback_buf+playback_size))
        {
            sour_p=(int16_t *)playback_buf;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf[i];
            mid_p[j++]=delay_buf[i+1];
        }

        for(i=0;i<playback_len_bytes/2-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

        for(j=0;i<playback_len_bytes/2;i=i+2)
        {
            delay_buf[j++]=sour_p[i];
            delay_buf[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio<=8)
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2*(8/playback_samplerate_ratio))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/2;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio/8;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes/8,left_gain,right_gain,AUD_BITS_16);

        for(i=0,j=0;i<(mc_len_bytes/8)/2;i=i+2)
        {
           float delta_l=(mid_p_8[i]-mid_p_8_old_l)/8.0f;
           float delta_r=(mid_p_8[i+1]-mid_p_8_old_r)/8.0f;
            for(k=0;k<8;k++)
            {
                dest_p[j++]=mid_p_8_old_l+(int32_t)(delta_l*k);
                dest_p[j++]=mid_p_8_old_r+(int32_t)(delta_r*k);
            }
            mid_p_8_old_l=mid_p_8[i];
            mid_p_8_old_r=mid_p_8[i+1];
        }
    }
    else if (sample_size_play == 4)
    {
        int32_t *sour_p=(int32_t *)(playback_buf+playback_size/2);
        int32_t *mid_p=(int32_t *)(buf);
        int32_t *mid_p_8=(int32_t *)(buf+mc_len_bytes-mc_len_bytes/8);
        int32_t *dest_p=(int32_t *)buf;

        if(buf == (playback_buf+playback_size))
        {
            sour_p=(int32_t *)playback_buf;
        }

        delay_sample=DELAY_SAMPLE_MC;

        for(i=0,j=0;i<delay_sample;i=i+2)
        {
            mid_p[j++]=delay_buf[i];
            mid_p[j++]=delay_buf[i+1];

        }

         for(i=0;i<playback_len_bytes/4-delay_sample;i=i+2)
        {
            mid_p[j++]=sour_p[i];
            mid_p[j++]=sour_p[i+1];
        }

         for(j=0;i<playback_len_bytes/4;i=i+2)
        {
            delay_buf[j++]=sour_p[i];
            delay_buf[j++]=sour_p[i+1];
        }

        if(playback_samplerate_ratio<=8)
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2*(8/playback_samplerate_ratio))
            {
                mid_p_8[j++]=mid_p[i];
                mid_p_8[j++]=mid_p[i+1];
            }
        }
        else
        {
            for(i=0,j=0;i<playback_len_bytes/4;i=i+2)
            {
                for(k=0;k<playback_samplerate_ratio/8;k++)
                {
                    mid_p_8[j++]=mid_p[i];
                    mid_p_8[j++]=mid_p[i+1];
                }
            }
        }

        anc_mc_run_stereo((uint8_t *)mid_p_8,mc_len_bytes/8,left_gain,right_gain,AUD_BITS_24);

        for(i=0,j=0;i<(mc_len_bytes/8)/4;i=i+2)
        {
           float delta_l=(mid_p_8[i]-mid_p_8_old_l)/8.0f;
           float delta_r=(mid_p_8[i+1]-mid_p_8_old_r)/8.0f;
            for(k=0;k<8;k++)
            {
                dest_p[j++]=mid_p_8_old_l+(int32_t)(delta_l*k);
                dest_p[j++]=mid_p_8_old_r+(int32_t)(delta_r*k);
            }
            mid_p_8_old_l=mid_p_8[i];
            mid_p_8_old_r=mid_p_8[i+1];
        }
    }

    //end_time = hal_sys_timer_get();
    //TRACE(2,"%s:run time: %d", __FUNCTION__, end_time-begin_time);

    return 0;
}
#endif

static uint32_t usb_audio_data_playback(uint8_t *buf, uint32_t len)
{
    uint32_t usb_len;
    uint32_t old_rpos, saved_old_rpos, new_rpos, wpos;
    uint8_t recv_valid, init_pos;
    uint8_t conflicted;
    uint32_t reset_len = usb_recv_size / 2;
#ifdef USB_AUDIO_MULTIFUNC
    uint32_t old_rpos2, saved_old_rpos2, new_rpos2, wpos2;
    uint8_t recv2_valid, init_pos2;
    uint8_t conflicted2;
    uint32_t reset_len2 = usb_recv2_size / 2;
#endif
    uint32_t play_next;
    uint32_t lock;
    uint8_t *conv_buf;
    uint32_t cur_time;
    uint32_t usb_time;

    usb_time = last_usb_recv_time;
    cur_time = hal_sys_timer_get();

    //----------------------------------------
    // Speech processing start
    //----------------------------------------
#ifdef USB_AUDIO_SPEECH
    speech_process_playback_run(buf, &len);
#endif
    //----------------------------------------
    // Speech processing end
    //----------------------------------------

    ASSERT(((uint32_t)buf & 0x3) == 0, "%s: Invalid buf: %p", __FUNCTION__, buf);

    // playback is valid when reaching here
    codec_play_valid = 1;

    init_pos = 0;
    // Allow to play till the end of the buffer when itf stopped
    if (usb_recv_valid && !(recv_state == AUDIO_ITF_STATE_STOPPED && usb_recv_rpos == usb_recv_wpos)) {
        recv_valid = 1;
        if (usb_recv_init_rpos == 0) {
            init_pos = 1;
        }
    } else {
        recv_valid = 0;
    }
#ifdef USB_AUDIO_MULTIFUNC
    init_pos2 = 0;
    if (usb_recv2_valid && !(recv2_state == AUDIO_ITF_STATE_STOPPED && usb_recv2_rpos == usb_recv2_wpos)) {
        recv2_valid = 1;
        if (usb_recv2_init_rpos == 0) {
            init_pos2 = 1;
        }
    } else {
        recv2_valid = 0;
    }
#endif

    if (codec_play_seq != usb_recv_seq ||
            playback_state == AUDIO_ITF_STATE_STOPPED ||
#ifdef USB_AUDIO_MULTIFUNC
            (recv_valid == 0 && recv2_valid == 0) ||
#else
            recv_valid == 0 ||
#endif
            cur_time - usb_time > USB_MAX_XFER_INTERVAL) {
_invalid_play:
        zero_mem32(buf, len);
        conv_buf = buf;
        // If usb recv is stopped, set usb_recv_rpos to the value of usb_recv_wpos -- avoid playing noise.
        // Otherwise set usb_recv_rpos to 0 -- avoid buffer conflicts at usb recv startup.
        new_rpos = (recv_state == AUDIO_ITF_STATE_STOPPED) ? usb_recv_wpos : 0;
        conflicted = 0;
#ifdef USB_AUDIO_MULTIFUNC
        new_rpos2 = (recv2_state == AUDIO_ITF_STATE_STOPPED) ? usb_recv2_wpos : 0;
        conflicted2 = 0;
#endif

        goto _conv_end;
    }

    //----------------------------------------
    // Check conflict
    //----------------------------------------

    usb_len = playback_to_recv_len(len);
    conflicted = 0;
#ifdef USB_AUDIO_MULTIFUNC
    conflicted2 = 0;
#endif

    lock = int_lock();
    wpos = usb_recv_wpos;
    if (init_pos) {
        old_rpos = wpos + reset_len;
        if (old_rpos >= usb_recv_size) {
            old_rpos -= usb_recv_size;
        }
        usb_recv_rpos = old_rpos;
        usb_recv_init_rpos = 1;
    } else {
        old_rpos = usb_recv_rpos;
    }
    saved_old_rpos = old_rpos;
    new_rpos = usb_recv_rpos + usb_len;
    if (new_rpos >= usb_recv_size) {
        new_rpos -= usb_recv_size;
    }
    if (recv_valid && init_pos == 0) {
        if (old_rpos <= wpos) {
            if (new_rpos < old_rpos || wpos < new_rpos) {
                if (recv_state == AUDIO_ITF_STATE_STOPPED) {
                    if (new_rpos < old_rpos) {
                        zero_mem(usb_recv_buf + wpos, usb_recv_size - wpos);
                        zero_mem(usb_recv_buf, new_rpos);
                    } else {
                        zero_mem(usb_recv_buf + wpos, new_rpos - wpos);
                    }
                    wpos = new_rpos;
                    usb_recv_wpos = new_rpos;
                } else {
                    conflicted = 1;
                }
            }
        } else {
            if (wpos < new_rpos && new_rpos < old_rpos) {
                if (recv_state == AUDIO_ITF_STATE_STOPPED) {
                    zero_mem(usb_recv_buf + wpos, new_rpos - wpos);
                    wpos = new_rpos;
                    usb_recv_wpos = new_rpos;
                } else {
                    conflicted = 1;
                }
            }
        }
        if (conflicted) {
            // Reset read position
            old_rpos = wpos + reset_len;
            if (old_rpos >= usb_recv_size) {
                old_rpos -= usb_recv_size;
            }
            new_rpos = old_rpos + usb_len;
            if (new_rpos >= usb_recv_size) {
                new_rpos -= usb_recv_size;
            }
            // Update global read position
            usb_recv_rpos = old_rpos;
        }
    }

#ifdef USB_AUDIO_MULTIFUNC
    wpos2 = usb_recv2_wpos;
    if (init_pos2) {
        old_rpos2 = wpos2 + reset_len2;
        if (old_rpos2 >= usb_recv2_size) {
            old_rpos2 -= usb_recv2_size;
        }
        usb_recv2_rpos = old_rpos2;
        usb_recv2_init_rpos = 1;
    } else {
        old_rpos2 = usb_recv2_rpos;
    }
    saved_old_rpos2 = old_rpos2;
    new_rpos2 = usb_recv2_rpos + usb_len;
    if (new_rpos2 >= usb_recv2_size) {
        new_rpos2 -= usb_recv2_size;
    }
    if (recv2_valid && init_pos2 == 0) {
        if (old_rpos2 <= wpos2) {
            if (new_rpos2 < old_rpos2 || wpos2 < new_rpos2) {
                if (recv2_state == AUDIO_ITF_STATE_STOPPED) {
                    if (new_rpos2 < old_rpos2) {
                        zero_mem(usb_recv2_buf + wpos2, usb_recv2_size - wpos2);
                        zero_mem(usb_recv2_buf, new_rpos2);
                    } else {
                        zero_mem(usb_recv2_buf + wpos2, new_rpos2 - wpos2);
                    }
                    wpos2 = new_rpos2;
                    usb_recv2_wpos = new_rpos2;
                } else {
                    conflicted2 = 1;
                }
            }
        } else {
            if (wpos2 < new_rpos2 && new_rpos2 < old_rpos2) {
                if (recv2_state == AUDIO_ITF_STATE_STOPPED) {
                    zero_mem(usb_recv2_buf + wpos2, new_rpos2 - wpos2);
                    wpos2 = new_rpos2;
                    usb_recv2_wpos = new_rpos2;
                } else {
                    conflicted2 = 1;
                }
            }
        }
        if (conflicted2) {
            // Reset read position
            old_rpos2 = wpos2 + reset_len2;
            if (old_rpos2 >= usb_recv2_size) {
                old_rpos2 -= usb_recv2_size;
            }
            new_rpos2 = old_rpos2 + usb_len;
            if (new_rpos2 >= usb_recv2_size) {
                new_rpos2 -= usb_recv2_size;
            }
            // Update global read position
            usb_recv2_rpos = old_rpos2;
        }
    }
#endif
    int_unlock(lock);

    if (conflicted) {
        TRACE(4,"playback: Error: rpos=%u goes beyond wpos=%u with usb_len=%u. Reset to %u", saved_old_rpos, wpos, usb_len, old_rpos);
    }
#ifdef USB_AUDIO_MULTIFUNC
    if (conflicted2) {
        TRACE(4,"playback: Error: rpos2=%u goes beyond wpos2=%u with usb_len=%u. Reset to %u", saved_old_rpos2, wpos2, usb_len, old_rpos2);
    }
#endif

#if (VERBOSE_TRACE & (1 << 0))
    {
        int hw_play_pos = af_stream_get_cur_dma_pos(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        uint32_t play_pos = buf - playback_buf;
#ifdef USB_AUDIO_MULTIFUNC
        if (recv_valid == 0) {
            old_rpos = -1;
            wpos = -1;
        }
        if (recv2_valid == 0) {
            old_rpos2 = -1;
            wpos2 = -1;
        }
        TRACE_TIME(8,"playback: rpos=%d/%d usb_len=%d wpos=%d/%d play_pos=%d len=%d hw_play_pos=%d", old_rpos, old_rpos2, usb_len,
            wpos, wpos2, play_pos, len, hw_play_pos);
#else
        TRACE_TIME(6,"playback: rpos=%d usb_len=%d wpos=%d play_pos=%d len=%d hw_play_pos=%d", old_rpos, usb_len,
            wpos, play_pos, len, hw_play_pos);
#endif
    }
#endif

    if (codec_play_seq != usb_recv_seq) {
        goto _invalid_play;
    }

#ifdef USB_AUDIO_MULTIFUNC
    record_conflict(conflicted || conflicted2);
#else
    record_conflict(conflicted);
#endif

    //----------------------------------------
    // USB->CODEC stream format conversion start
    //----------------------------------------

    conv_buf = buf;

#ifdef USB_AUDIO_MULTIFUNC

    {
        uint8_t POSSIBLY_UNUSED *cur_buf, *buf_end, *usb_cur_buf, *usb_buf_end, *usb_cur_buf2, *usb_buf_end2;
        uint32_t codec_step, usb_step;
        float cur_gain, new_gain, cur_gain2, new_gain2, gain_step;

        cur_buf = conv_buf;
        buf_end = conv_buf + len;
        usb_cur_buf = usb_recv_buf + old_rpos;
        usb_buf_end = usb_recv_buf + usb_recv_size;
        usb_cur_buf2 = usb_recv2_buf + old_rpos2;
        usb_buf_end2 = usb_recv2_buf + usb_recv2_size;

        cur_gain = playback_coef;
        new_gain = new_playback_coef;
        cur_gain2 = playback2_coef;
        new_gain2 = new_playback2_coef;
        gain_step = USB_AUDIO_VOL_UPDATE_STEP;

        ASSERT(sample_rate_play == sample_rate_recv, "2:1: Invalid sample_rate_play=%u sample_rate_recv=%u",
            sample_rate_play, sample_rate_recv);

        if (0) {

        } else if (sample_size_play == 4 && (sample_size_recv == 4 || sample_size_recv == 3 || sample_size_recv == 2)) {

            int32_t left, right;
            int32_t left2, right2;

            if (sample_size_recv == 4) {
                usb_step = 8;
            } else if (sample_size_recv == 3) {
                usb_step = 6;
            } else {
                usb_step = 4;
            }

            codec_step = 8;

            while (cur_buf + codec_step <= buf_end) {
                if (usb_cur_buf + usb_step > usb_buf_end) {
                    usb_cur_buf = usb_recv_buf;
                }
                if (usb_cur_buf2 + usb_step > usb_buf_end2) {
                    usb_cur_buf2 = usb_recv2_buf;
                }
                if (recv_valid) {
                    if (usb_step == 8) {
                        left = *(int32_t *)usb_cur_buf;
                        right = *((int32_t *)usb_cur_buf + 1);
                    } else if (usb_step == 6) {
                        left = ((usb_cur_buf[0] << 8) | (usb_cur_buf[1] << 16) | (usb_cur_buf[2] << 24));
                        right = ((usb_cur_buf[3] << 8) | (usb_cur_buf[4] << 16) | (usb_cur_buf[5] << 24));
                    } else {
                        left = *(int16_t *)usb_cur_buf << 16;
                        right = *((int16_t *)usb_cur_buf + 1) << 16;
                    }
                    if (cur_gain > new_gain) {
                        cur_gain -= gain_step;
                        if (cur_gain < new_gain) {
                            cur_gain = new_gain;
                        }
                    } else if (cur_gain < new_gain) {
                        cur_gain += gain_step;
                        if (cur_gain > new_gain) {
                            cur_gain = new_gain;
                        }
                    }
                    left = (int32_t)(left * cur_gain);
                    right = (int32_t)(right * cur_gain);
                } else {
                    left = right = 0;
                }
                if (recv2_valid) {
                    if (usb_step == 8) {
                        left2 = *(int32_t *)usb_cur_buf2;
                        right2 = *((int32_t *)usb_cur_buf2 + 1);
                    } else if (usb_step == 6) {
                        left2 = ((usb_cur_buf2[0] << 8) | (usb_cur_buf2[1] << 16) | (usb_cur_buf2[2] << 24));
                        right2 = ((usb_cur_buf2[3] << 8) | (usb_cur_buf2[4] << 16) | (usb_cur_buf2[5] << 24));
                    } else {
                        left2 = *(int16_t *)usb_cur_buf2 << 16;
                        right2 = *((int16_t *)usb_cur_buf2 + 1) << 16;
                    }
                    if (cur_gain2 > new_gain2) {
                        cur_gain2 -= gain_step;
                        if (cur_gain2 < new_gain2) {
                            cur_gain2 = new_gain2;
                        }
                    } else if (cur_gain2 < new_gain2) {
                        cur_gain2 += gain_step;
                        if (cur_gain2 > new_gain2) {
                            cur_gain2 = new_gain2;
                        }
                    }
                    left2 = (int32_t)(left2 * cur_gain2);
                    right2 = (int32_t)(right2 * cur_gain2);
                } else {
                    left2 = right2 = 0;
                }
                left = __QADD(left, left2);
                right = __QADD(right, right2);
                // Convert to 24-bit sample
                left >>= 8;
                right >>= 8;

                *(int32_t *)cur_buf = left;
                *((int32_t *)cur_buf + 1) = right;

                cur_buf += codec_step;
                usb_cur_buf += usb_step;
                usb_cur_buf2 += usb_step;
            }
        } else if (sample_size_play == 2 && sample_size_recv == 2) {

            int32_t left, right;
            int32_t left2, right2;

            usb_step = 4;
            codec_step = 4;

            while (cur_buf + codec_step <= buf_end) {
                if (usb_cur_buf + usb_step > usb_buf_end) {
                    usb_cur_buf = usb_recv_buf;
                }
                if (usb_cur_buf2 + usb_step > usb_buf_end2) {
                    usb_cur_buf2 = usb_recv2_buf;
                }
                if (recv_valid) {
                    left = *(int16_t *)usb_cur_buf;
                    right = *((int16_t *)usb_cur_buf + 1);
                    if (cur_gain > new_gain) {
                        cur_gain -= gain_step;
                        if (cur_gain < new_gain) {
                            cur_gain = new_gain;
                        }
                    } else if (cur_gain < new_gain) {
                        cur_gain += gain_step;
                        if (cur_gain > new_gain) {
                            cur_gain = new_gain;
                        }
                    }
                    left = (int32_t)(left * cur_gain);
                    right = (int32_t)(right * cur_gain);
                } else {
                    left = right = 0;
                }
                if (recv2_valid) {
                    left2 = *(int16_t *)usb_cur_buf2;
                    right2 = *((int16_t *)usb_cur_buf2 + 1);
                    if (cur_gain2 > new_gain2) {
                        cur_gain2 -= gain_step;
                        if (cur_gain2 < new_gain2) {
                            cur_gain2 = new_gain2;
                        }
                    } else if (cur_gain2 < new_gain2) {
                        cur_gain2 += gain_step;
                        if (cur_gain2 > new_gain2) {
                            cur_gain2 = new_gain2;
                        }
                    }
                    left2 = (int32_t)(left2 * cur_gain2);
                    right2 = (int32_t)(right2 * cur_gain2);
                } else {
                    left2 = right2 = 0;
                }
                left = __SSAT(left + left2, 16);
                right = __SSAT(right + right2, 16);

                *(int16_t *)cur_buf = left;
                *((int16_t *)cur_buf + 1) = right;

                cur_buf += codec_step;
                usb_cur_buf += usb_step;
            }

        } else {

            ASSERT(false, "2:3: Invalid sample_size_play=%u sample_size_recv=%u",
                sample_size_play, sample_size_recv);

        }

        playback_coef = cur_gain;
        playback2_coef = cur_gain2;
    }

#else

    {
        uint8_t POSSIBLY_UNUSED *cur_buf, *buf_end, *usb_cur_buf, *usb_buf_end;
        uint32_t codec_step, usb_step;
        bool down_sampling = false;

        cur_buf = conv_buf;
        buf_end = conv_buf + len;
        usb_cur_buf = usb_recv_buf + old_rpos;
        usb_buf_end = usb_recv_buf + usb_recv_size;

        if (0) {

#ifdef CODEC_DSD
        } else if (codec_dsd_enabled) {

            ASSERT(sample_size_play == 2, "Bad DSD playback sample size: %u", sample_size_play);
            ASSERT(sample_size_recv == 4 || sample_size_recv == 3, "Bad DSD recv sample size: %u", sample_size_recv);

            uint32_t left, right;

            if (sample_size_recv == 4) {
                usb_step = 8;
            } else {
                usb_step = 6;
            }

            codec_step = 4;

            while (cur_buf + codec_step <= buf_end) {
                if (usb_cur_buf + usb_step > usb_buf_end) {
                    usb_cur_buf = usb_recv_buf;
                }
                if (usb_step == 8) {
                    left = *(uint32_t *)usb_cur_buf;
                    right = *((uint32_t *)usb_cur_buf + 1);
                } else {
                    left = ((usb_cur_buf[0] << 8) | (usb_cur_buf[1] << 16));
                    right = ((usb_cur_buf[3] << 8) | (usb_cur_buf[4] << 16));
                }

                left = __RBIT(left) >> 8;
                right = __RBIT(right) >> 8;

                *(uint16_t *)cur_buf = left;
                *((uint16_t *)cur_buf + 1) = right;

                cur_buf += codec_step;
                usb_cur_buf += usb_step;
            }
#endif

        } else if (sample_size_play == 4 && (sample_size_recv == 4 || sample_size_recv == 3 || sample_size_recv == 2)) {

            int32_t left, right;

            if (sample_size_recv == 4) {
                usb_step = 8;
            } else if (sample_size_recv == 3) {
                usb_step = 6;
            } else {
                usb_step = 4;
            }

            if ((sample_rate_play == 96000 && sample_rate_recv == 48000) ||
                    (sample_rate_play == 88200 && sample_rate_recv == 44100)) {
                codec_step = 16;
            } else if (sample_rate_play == sample_rate_recv) {
                codec_step = 8;
            } else if (sample_rate_play == 96000 && sample_rate_recv == 192000) {
                codec_step = 8;
                down_sampling = true;
            } else {
                ASSERT(false, "1: Invalid sample_rate_play=%u sample_rate_recv=%u",
                    sample_rate_play, sample_rate_recv);
            }

            while (cur_buf + codec_step <= buf_end) {
                if (usb_cur_buf + usb_step > usb_buf_end) {
                    usb_cur_buf = usb_recv_buf;
                }
                if (usb_step == 8) {
                    left = *(int32_t *)usb_cur_buf;
                    right = *((int32_t *)usb_cur_buf + 1);
                } else if (usb_step == 6) {
                    left = ((usb_cur_buf[0] << 8) | (usb_cur_buf[1] << 16) | (usb_cur_buf[2] << 24));
                    right = ((usb_cur_buf[3] << 8) | (usb_cur_buf[4] << 16) | (usb_cur_buf[5] << 24));
                } else {
                    left = *(int16_t *)usb_cur_buf << 16;
                    right = *((int16_t *)usb_cur_buf + 1) << 16;
                }
                // Convert to 24-bit sample
                left >>= 8;
                right >>= 8;

                if (down_sampling) {
                    int32_t left_next, right_next;

                    usb_cur_buf += usb_step;
                    if (usb_cur_buf + usb_step > usb_buf_end) {
                        usb_cur_buf = usb_recv_buf;
                    }
                    if (usb_step == 8) {
                        left_next = *(int32_t *)usb_cur_buf;
                        right_next = *((int32_t *)usb_cur_buf + 1);
                    } else if (usb_step == 6) {
                        left_next = ((usb_cur_buf[0] << 8) | (usb_cur_buf[1] << 16) | (usb_cur_buf[2] << 24));
                        right_next = ((usb_cur_buf[3] << 8) | (usb_cur_buf[4] << 16) | (usb_cur_buf[5] << 24));
                    } else {
                        left_next = *(int16_t *)usb_cur_buf << 16;
                        right_next = *((int16_t *)usb_cur_buf + 1) << 16;
                    }
                    // Convert to 24-bit sample
                    left_next >>= 8;
                    right_next >>= 8;

                    // The addition of two 24-bit integers will not saturate
                    left = (left + left_next) / 2;
                    right = (right + right_next) / 2;
                }

                *(int32_t *)cur_buf = left;
                *((int32_t *)cur_buf + 1) = right;
                if (codec_step == 16) {
                    *((int32_t *)cur_buf + 2) =  left;
                    *((int32_t *)cur_buf + 3) =  right;
                }

                cur_buf += codec_step;
                usb_cur_buf += usb_step;
            }

        } else if (sample_size_play == 2 && sample_size_recv == 2) {

            int16_t left, right;

            usb_step = 4;

            if ((sample_rate_play == 96000 && sample_rate_recv == 48000) ||
                    (sample_rate_play == 88200 && sample_rate_recv == 44100)) {
                codec_step = 8;
            } else if (sample_rate_play == sample_rate_recv) {
                codec_step = 4;
            } else if (sample_rate_play == 96000 && sample_rate_recv == 192000) {
                codec_step = 4;
                down_sampling = true;
            } else {
                ASSERT(false, "2: Invalid sample_rate_play=%u sample_rate_recv=%u",
                    sample_rate_play, sample_rate_recv);
            }

            while (cur_buf + codec_step <= buf_end) {
                if (usb_cur_buf + usb_step > usb_buf_end) {
                    usb_cur_buf = usb_recv_buf;
                }
                left = *(int16_t *)usb_cur_buf;
                right = *((int16_t *)usb_cur_buf + 1);
                if (down_sampling) {
                    // Use 32-bit integers to avoid saturation
                    int32_t left32, right32;

                    usb_cur_buf += usb_step;
                    if (usb_cur_buf + usb_step > usb_buf_end) {
                        usb_cur_buf = usb_recv_buf;
                    }
                    left32 = *(int16_t *)usb_cur_buf;
                    right32 = *((int16_t *)usb_cur_buf + 1);
                    left = (left + left32) / 2;
                    right = (right + right32) / 2;
                }
                *(int16_t *)cur_buf = left;
                *((int16_t *)cur_buf + 1) = right;
                if (codec_step == 8) {
                    *((int16_t *)cur_buf + 2) = left;
                    *((int16_t *)cur_buf + 3) = right;
                }
                cur_buf += codec_step;
                usb_cur_buf += usb_step;
            }

        } else {

            ASSERT(false, "3: Invalid sample_size_play=%u sample_size_recv=%u",
                sample_size_play, sample_size_recv);

        }

    }

#endif // !USB_AUDIO_MULTIFUNC

    //----------------------------------------
    // USB->CODEC stream format conversion end
    //----------------------------------------
_conv_end:

    play_next = buf + len - playback_buf;
    if (play_next >= playback_size) {
        play_next -= playback_size;
    }

    lock = int_lock();
    if (codec_play_seq == usb_recv_seq) {
#ifdef USB_AUDIO_MULTIFUNC
        if (playback_state == AUDIO_ITF_STATE_STARTED) {
            if (recv_valid) {
                usb_recv_rpos = new_rpos;
            }
            if (recv2_valid) {
                usb_recv2_rpos = new_rpos2;
            }
        } else {
            if (recv_valid) {
                usb_recv_rpos = 0;
            }
            if (recv2_valid) {
                usb_recv2_rpos = 0;
            }
        }
        if (conflicted) {
            playback_conflicted = conflicted;
        }
        if (conflicted2) {
            playback_conflicted2 = conflicted2;
        }
#else
        if (playback_state == AUDIO_ITF_STATE_STARTED) {
            usb_recv_rpos = new_rpos;
        } else {
            usb_recv_rpos = 0;
        }
        if (conflicted) {
            playback_conflicted = conflicted;
        }
#endif
        playback_pos = play_next;
    }
    int_unlock(lock);

    //----------------------------------------
    // Audio processing start
    //----------------------------------------

#ifdef FREQ_RESP_EQ
    freq_resp_eq_run(conv_buf, len);
#endif
#if defined(__HW_FIR_DSD_PROCESS__)
    if (codec_dsd_enabled) {
        dsd_process(conv_buf,len);
    }
#endif

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    if (eq_opened) {
        audio_process_run(conv_buf, len);
    }
#endif

    // If conv_buf != buf, copy the output to buf

    //----------------------------------------
    // Audio processing end
    //----------------------------------------

#ifdef NOISE_GATING
    bool noise_mute = true;
#ifdef NOISE_REDUCTION
    bool nr_fire = true;
    uint8_t chan = 0;
#endif

    if (sample_size_play == 2) {
        int16_t *samp = (int16_t *)buf;
        for (uint32_t i = 0; i < len / 2; i++) {
            if (ABS(samp[i]) >= NOISE_GATING_THRESH_16BIT) {
                noise_mute = false;
                break;
            }
            // NOISE_REDUCTION is unstable for 16-bit streams for unknown reasons. Skip by now.
        }
    } else {
        int32_t *samp = (int32_t *)buf;
        for (uint32_t i = 0; i < len / 4; i++) {
            if (ABS(samp[i]) >= NOISE_GATING_THRESH_24BIT) {
                noise_mute = false;
#ifndef NOISE_REDUCTION
                break;
#endif
            }
#ifdef NOISE_REDUCTION
            if (nr_fire) {
                if (ABS(samp[i]) >= NOISE_REDUCTION_THRESH_24BIT) {
                    nr_fire = false;
                } else {
                    cur_zero_diff[chan][0]++;
                    cur_zero_diff[chan][1]++;
                    if ((prev_samp_positive[chan] && samp[i] < 0) ||
                            (!prev_samp_positive[chan] && samp[i] >= 0)) {
                        if (ABS(cur_zero_diff[chan][prev_samp_positive[chan]] -
                                prev_zero_diff[chan][prev_samp_positive[chan]]) < 2) {
                            if (nr_cont_cnt < NOISE_REDUCTION_MATCH_CNT) {
                                nr_cont_cnt++;
                            }
                        } else if (nr_cont_cnt > 2) {
                            nr_fire = false;
                        }
                        prev_zero_diff[chan][prev_samp_positive[chan]] = cur_zero_diff[chan][prev_samp_positive[chan]];
                        cur_zero_diff[chan][prev_samp_positive[chan]] = 0;
                    }
                    prev_samp_positive[chan] = (samp[i] >= 0);
                    // Switch channel
                    chan = !chan;
                }
            }
#endif
        }
    }

#ifdef NOISE_REDUCTION
    if (noise_mute) {
        nr_fire = false;
    }
    if (nr_fire) {
        if (nr_cont_cnt >= NOISE_REDUCTION_MATCH_CNT &&
                !nr_active && noise_reduction_cmd != NOISE_REDUCTION_CMD_FIRE) {
            if (cur_time - last_nr_restore_time >= NOISE_REDUCTION_INTERVAL) {
                TRACE(8,"\nFire noise reduction: cur0=%u/%u cur1=%u/%u prev0=%u/%u prev1=%u/%u\n",
                    cur_zero_diff[0][0], cur_zero_diff[0][1], cur_zero_diff[1][0], cur_zero_diff[1][1],
                    prev_zero_diff[0][0], prev_zero_diff[0][1], prev_zero_diff[1][0], prev_zero_diff[1][1]);
                noise_reduction_cmd = NOISE_REDUCTION_CMD_FIRE;
                enqueue_unique_cmd(AUDIO_CMD_NOISE_REDUCTION);
            }
        }
    } else {
        if (nr_active && noise_reduction_cmd != NOISE_REDUCTION_CMD_RESTORE) {
            TRACE(8,"\nRestore noise reduction: cur0=%u/%u cur1=%u/%u prev0=%u/%u prev1=%u/%u\n",
                cur_zero_diff[0][0], cur_zero_diff[0][1], cur_zero_diff[1][0], cur_zero_diff[1][1],
                prev_zero_diff[0][0], prev_zero_diff[0][1], prev_zero_diff[1][0], prev_zero_diff[1][1]);
            noise_reduction_cmd = NOISE_REDUCTION_CMD_RESTORE;
            enqueue_unique_cmd(AUDIO_CMD_NOISE_REDUCTION);
        }
        memset(prev_zero_diff, 0, sizeof(prev_zero_diff));
        memset(cur_zero_diff, 0, sizeof(cur_zero_diff));
        nr_cont_cnt = 0;
        last_nr_restore_time = cur_time;
    }
#endif

    if (noise_mute) {
        if ((mute_user_map & (1 << CODEC_MUTE_USER_NOISE_GATING)) == 0 && noise_gating_cmd != NOISE_GATING_CMD_MUTE) {
            if (cur_time - last_high_signal_time >= NOISE_GATING_INTERVAL) {
                noise_gating_cmd = NOISE_GATING_CMD_MUTE;
                enqueue_unique_cmd(AUDIO_CMD_NOISE_GATING);
            }
        }
    } else {
        if ((mute_user_map & (1 << CODEC_MUTE_USER_NOISE_GATING)) && noise_gating_cmd != NOISE_GATING_CMD_UNMUTE) {
            noise_gating_cmd = NOISE_GATING_CMD_UNMUTE;
            enqueue_unique_cmd(AUDIO_CMD_NOISE_GATING);
        }
        last_high_signal_time = cur_time;
    }
#endif

#if (VERBOSE_TRACE & (1 << 1))
    {
        int hw_play_pos = af_stream_get_cur_dma_pos(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        TRACE_TIME(2,"playbackEnd: playback_pos=%u hw_play_pos=%d", playback_pos, hw_play_pos);
    }
#endif

    return 0;
}

static uint32_t usb_audio_data_capture(uint8_t *buf, uint32_t len)
{
    uint32_t usb_len;
    uint32_t old_wpos, saved_old_wpos, new_wpos, rpos;
    uint8_t init_pos;
    uint8_t conflicted;
    uint32_t reset_len = usb_send_size / 2;
    uint32_t cap_next;
    uint32_t lock;
    uint8_t *conv_buf;
    uint32_t cur_time;
    uint32_t usb_time;
#ifdef SW_CAPTURE_RESAMPLE
    enum RESAMPLE_STATUS_T ret;
    struct RESAMPLE_IO_BUF_T io;
    uint32_t in_size;
    uint32_t out_size;
    uint32_t resample_frame_len = 0;
#endif

    usb_time = last_usb_send_time;
    cur_time = hal_sys_timer_get();

    ASSERT(((uint32_t)buf & 0x3) == 0, "%s: Invalid buf: %p", __FUNCTION__, buf);

    if (codec_cap_valid == 0 && cur_time - codec_cap_start_time >= CAPTURE_STABLE_INTERVAL) {
        codec_cap_valid = 1;
    }

    if (codec_cap_seq != usb_send_seq ||
            capture_state == AUDIO_ITF_STATE_STOPPED ||
            send_state == AUDIO_ITF_STATE_STOPPED ||
            codec_cap_valid == 0 ||
            usb_send_valid == 0 ||
            cur_time - usb_time > USB_MAX_XFER_INTERVAL) {
_invalid_cap:
        new_wpos = 0;
        conflicted = 0;
        goto _conv_end;
    }

    if (usb_send_init_wpos) {
        init_pos = 0;
    } else {
        init_pos = 1;
    }

    //----------------------------------------
    // Check conflict
    //----------------------------------------

    usb_len = capture_to_send_len(len);
    conflicted = 0;

    lock = int_lock();
    rpos = usb_send_rpos;
    if (init_pos) {
        old_wpos = rpos + reset_len;
        if (old_wpos >= usb_send_size) {
            old_wpos -= usb_send_size;
        }
        usb_send_wpos = old_wpos;
        usb_send_init_wpos = 1;
    } else {
        old_wpos = usb_send_wpos;
    }
    saved_old_wpos = old_wpos;
    new_wpos = usb_send_wpos + usb_len;
    if (new_wpos >= usb_send_size) {
        new_wpos -= usb_send_size;
    }
    if (init_pos == 0) {
        if (old_wpos <= rpos) {
            if (new_wpos < old_wpos || rpos < new_wpos) {
                conflicted = 1;
            }
        } else {
            if (rpos < new_wpos && new_wpos < old_wpos) {
                conflicted = 1;
            }
        }
        if (conflicted) {
            // Reset write position
            old_wpos = rpos + reset_len;
            if (old_wpos >= usb_send_size) {
                old_wpos -= usb_send_size;
            }
            new_wpos = old_wpos + usb_len;
            if (new_wpos >= usb_send_size) {
                new_wpos -= usb_send_size;
            }
            // Update global write position
            usb_send_wpos = old_wpos;
        }
    }
    int_unlock(lock);

    if (conflicted) {
        TRACE(4,"capture: Error: wpos=%u goes beyond rpos=%u with usb_len=%u. Reset to %u", saved_old_wpos, rpos, usb_len, old_wpos);
    }

#if (VERBOSE_TRACE & (1 << 2))
    TRACE_TIME(5,"capture: wpos=%u usb_len=%u rpos=%u cap_pos=%u len=%u", old_wpos, usb_len, rpos, buf + len - capture_buf, len);
#endif

    if (codec_cap_seq != usb_send_seq) {
        goto _invalid_cap;
    }

    record_conflict(conflicted);

    //----------------------------------------
    // Audio processing start
    //----------------------------------------

    // TODO ...

    //----------------------------------------
    // Audio processing end
    //----------------------------------------

    //----------------------------------------
    // Speech processing start
    //----------------------------------------
#ifdef USB_AUDIO_SPEECH
    speech_process_capture_run(buf, &len);
#endif
    //----------------------------------------
    // Speech processing end
    //----------------------------------------

    //----------------------------------------
    // CODEC->USB stream format conversion start
    //----------------------------------------

    conv_buf = buf;

    {
        uint8_t POSSIBLY_UNUSED *cur_buf, *buf_end, *dst_buf_start, *dst_cur_buf, *dst_buf_end;

        cur_buf = conv_buf;
        buf_end = conv_buf + len;
#ifdef SW_CAPTURE_RESAMPLE
        if (resample_cap_enabled) {
#if (CHAN_NUM_CAPTURE == CHAN_NUM_SEND)
#if (defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_)))
            // codec capture buffer --(ref rate conversion)--> resample input buffer --(resample)--> usb send buffer

#ifdef USB_AUDIO_DYN_CFG
            // Resample frame len is the capture reference len after reference rate conversion
            resample_frame_len = capture_to_ref_len(len);
#else
            // Resample frame len is the capture len
            resample_frame_len = len;
#endif
            ASSERT(resample_frame_len <= resample_input_size,
                "resample_input_size too small: %u len=%u resample_frame_len=%u",
                resample_input_size, len, resample_frame_len);

            dst_buf_start = resample_input_buf;
            dst_cur_buf = resample_input_buf;
            dst_buf_end = resample_input_buf + resample_frame_len;
#else // !(CHIP_BEST1000 && (ANC_APP || _DUAL_AUX_MIC_))
            // codec capture buffer --(resample)--> usb send buffer

            resample_input_buf = cur_buf;
            // Resample frame len is the capture len
            resample_frame_len = len;

            dst_buf_start = cur_buf;
            dst_cur_buf = cur_buf;
            dst_buf_end = cur_buf + len;
#endif // !(CHIP_BEST1000 && (ANC_APP || _DUAL_AUX_MIC_))
#elif (CHAN_NUM_CAPTURE == 2) && (CHAN_NUM_SEND == 1)
            // codec capture buffer --(chan num conversion)--> codec capture buffer --(resample)--> usb send buffer

            resample_input_buf = cur_buf;
            // Resample frame len is half of the capture len
            resample_frame_len = len / 2;

            dst_buf_start = cur_buf;
            dst_cur_buf = cur_buf;
            dst_buf_end = cur_buf + resample_frame_len;
#elif (CHAN_NUM_CAPTURE == 1) && (CHAN_NUM_SEND == 2)
            // codec capture buffer --(resample)--> usb send buffer --(chan num conversion)--> usb send buffer

            // Resample frame len is the capture len
            resample_frame_len = len;

            io.in = cur_buf;
            io.in_size = resample_frame_len;
            io.out = usb_send_buf + old_wpos;
            io.out_size = usb_len / 2;
            io.out_cyclic_start = usb_send_buf;
            io.out_cyclic_end = usb_send_buf + usb_send_size;

            ret = audio_resample_ex_run(resample_id, &io, &in_size, &out_size);
            ASSERT((ret == RESAMPLE_STATUS_IN_EMPTY || ret == RESAMPLE_STATUS_DONE) && io.out_size >= out_size,
                "Failed to resample: %d io.out_size=%u in_size=%u out_size=%u",
                ret, io.out_size, in_size, out_size);

            uint32_t diff = usb_len - out_size * 2;
            if (new_wpos >= diff) {
                new_wpos -= diff;
            } else {
                new_wpos = new_wpos + usb_send_size - diff;
            }

#if (VERBOSE_TRACE & (1 << 3))
            TRACE(5,"cap_resample: new_wpos=%u, in: %u -> %u, out: %u -> %u", new_wpos, io.in_size, in_size, io.out_size, out_size);
#endif

            dst_buf_start = usb_send_buf;
            dst_cur_buf = usb_send_buf + old_wpos;
            dst_buf_end = usb_send_buf + usb_send_size;
            cur_buf = dst_cur_buf;
            buf_end = cur_buf + out_size;
            if (buf_end >= usb_send_buf + usb_send_size) {
                buf_end -= usb_send_size;
            }
#else
#error "Unsupported CHAN_NUM_CAPTURE and CHAN_NUM_SEND configuration"
#endif
        } else
#endif // SW_CAPTURE_RESAMPLE
        {
            dst_buf_start = usb_send_buf;
            dst_cur_buf = usb_send_buf + old_wpos;
            dst_buf_end = usb_send_buf + usb_send_size;
        }

#if defined(CHIP_BEST1000) && defined(_DUAL_AUX_MIC_)

    // Assuming codec adc records stereo data, and usb sends 48K/44.1K stereo data
#if (CHAN_NUM_CAPTURE != 2) || (CHAN_NUM_SEND != 2)
#error "Unsupported CHAN_NUM_CAPTURE and CHAN_NUM_SEND configuration"
#endif

    {
#define MERGE_VER                       2
#if (MERGE_VER == 2)
#define THRES_MIC3                      32500
#else
#define THRES_MIC3                      27168
#endif
#define SHIFT_BITS                      4

        uint32_t i = 0;
        short* BufSrc = (short*)cur_buf;
        short* BufDst = (short*)dst_cur_buf;
        short* BufDstStart = (short*)dst_buf_start;
        short* BufDstEnd = (short*)dst_buf_end;

        int32_t PcmValue = 0;
        int32_t BufSrcRVal = 0;

        uint32_t step;

        //TRACE(3,"%s - %d, %d", __func__, BufSrc[0], BufSrc[1]);
        //TRACE(2,"%s - %d", __func__, TICKS_TO_MS(cur_time));

        if (sample_rate_cap == 384000 || sample_rate_cap == 352800) {
            step = 2;
        } else if (sample_rate_cap == 192000 || sample_rate_cap == 176400) {
            step = 1;
        } else {
            ASSERT(false, "1: Invalid sample_rate_cap=%u sample_rate_ref_cap=%u sample_rate_send=%u", sample_rate_cap, sample_rate_ref_cap, sample_rate_send);
        }

        get_amic_dc(BufSrc, len>>2, step);

        for (i = 0; i < (len>>2); i += step)
        {
            if (step == 2) {
                if (!(i&0x1)) {
                    PcmValue = ((int32_t)(BufSrc[i<<1]));
                    BufSrcRVal = ((int32_t)(BufSrc[(i<<1)+1]));
#if (MERGE_VER == 2)
                    if ((PcmValue < THRES_MIC3) && (PcmValue > -THRES_MIC3)) {
                        PcmValue = (PcmValue - s_amic3_dc)>>SHIFT_BITS;
                    } else {
                        PcmValue = BufSrcRVal - s_amic4_dc;
                    }
#else   // (MERGE_VER == 1)
                    PcmValue -= s_amic3_dc;
                    if ((PcmValue < THRES_MIC3) && (PcmValue > -THRES_MIC3)) {
                        PcmValue >>= SHIFT_BITS;
                    } else {
                        PcmValue = BufSrcRVal - s_amic4_dc;
                    }
#endif  // MERGE_VER
                    PcmValue = soir_filter(PcmValue);
#ifdef DUAL_AUX_MIC_MORE_FILTER
                    PcmValue = soir_filter1(PcmValue);
                    PcmValue = soir_filter2(PcmValue);
#endif
                    if (!(i&7)) {
                        if (BufDst + 2 > BufDstEnd) {
                            BufDst = BufDstStart;
                        }
                        *BufDst++ = PcmValue;
                        *BufDst++ = PcmValue;
                    }
                }
            } else {
                if (!(i&0x1)) {
                    PcmValue = ((int32_t)(BufSrc[i<<1]));
                    BufSrcRVal = ((int32_t)(BufSrc[(i<<1)+1]));
#if (MERGE_VER == 2)
                    if ((PcmValue < THRES_MIC3) && (PcmValue > -THRES_MIC3)) {
                        PcmValue = (PcmValue-s_amic3_dc)>>SHIFT_BITS;
                    } else {
                        PcmValue = BufSrcRVal-s_amic4_dc;
                    }
#else   // (MERGE_VER == 1)
                    PcmValue -= s_amic3_dc;
                    if ((PcmValue < THRES_MIC3) && (PcmValue > -THRES_MIC3)) {
                        PcmValue >>= SHIFT_BITS;
                    } else {
                        PcmValue = BufSrcRVal - s_amic4_dc;
                    }
#endif  // MERGE_VER
                    PcmValue = soir_filter(PcmValue);
#ifdef DUAL_AUX_MIC_MORE_FILTER
                    PcmValue = soir_filter1(PcmValue);
                    PcmValue = soir_filter2(PcmValue);
#endif
                    if (!(i&3)) {
                        if (BufDst + 2 > BufDstEnd) {
                            BufDst = BufDstStart;
                        }
                        *BufDst++ = PcmValue;
                        *BufDst++ = PcmValue;
                    }
                }
            }
        }
    }

#elif defined(CHIP_BEST1000) && defined(ANC_APP)

    // Assuming codec adc records stereo data, and usb sends 48K/44.1K stereo data
#if (CHAN_NUM_CAPTURE != 2) || (CHAN_NUM_SEND != 2)
#error "Unsupported CHAN_NUM_CAPTURE and CHAN_NUM_SEND configuration"
#endif

    if ((sample_rate_ref_cap == 48000 || sample_rate_ref_cap == 44100) &&
            (sample_rate_cap == sample_rate_ref_cap * 2 || sample_rate_cap == sample_rate_ref_cap * 4 ||
            sample_rate_cap == sample_rate_ref_cap * 8 || sample_rate_cap == sample_rate_ref_cap * 16)) {

        uint32_t POSSIBLY_UNUSED factor = sample_rate_cap / sample_rate_ref_cap;
        int32_t POSSIBLY_UNUSED left = 0;
        int32_t POSSIBLY_UNUSED right = 0;
        uint32_t step = factor * 4;

        while (cur_buf + step <= buf_end) {
            if (dst_cur_buf + 4 > dst_buf_end) {
                dst_cur_buf = dst_buf_start;
            }

            // Downsampling the adc data
#if 1
            left = 0;
            right = 0;
            for (int i = 0; i < factor; i++) {
                left += *(int16_t *)(cur_buf + 4 * i);
                right += *(int16_t *)(cur_buf + 4 * i + 2);
            }
            *(int16_t *)dst_cur_buf = left / factor;
            *(int16_t *)(dst_cur_buf + 2) = right / factor;
#else
            *(int16_t *)dst_cur_buf = *(int16_t *)cur_buf;
            *(int16_t *)(dst_cur_buf + 2) = *(int16_t *)(cur_buf + 2);
#endif

            // Move to next data
            dst_cur_buf += 4;
            cur_buf += step;
        }

    } else {
        ASSERT(false, "2: Invalid sample_rate_cap=%u sample_rate_ref_cap=%u sample_rate_send=%u", sample_rate_cap, sample_rate_ref_cap, sample_rate_send);
    }

#else // !(CHIP_BEST1000 && (ANC_APP || _DUAL_AUX_MIC_))

#ifndef SW_CAPTURE_RESAMPLE
    ASSERT(sample_rate_cap == sample_rate_send,"3: Invalid sample_rate_cap=%u sample_rate_send=%u", sample_rate_cap, sample_rate_send);
#endif

// When USB_AUDIO_SPEECH is enable, buf after algorithm shoule be CHAN_NUM_SEND,
// do not need to convert any more.
#if (CHAN_NUM_CAPTURE == 2) && (CHAN_NUM_SEND == 1) && !defined(USB_AUDIO_SPEECH)

        // Assuming codec adc always records stereo data

        while (cur_buf + 4 <= buf_end) {
            // Copy left channel data
            if (dst_cur_buf + 2 > dst_buf_end) {
                dst_cur_buf = dst_buf_start;
            }
            *(int16_t *)dst_cur_buf = *(int16_t *)cur_buf;

            // Move to next data
            dst_cur_buf += 2;
            cur_buf += 4;
        }

#elif (CHAN_NUM_CAPTURE == 1) && (CHAN_NUM_SEND == 2) && !defined(USB_AUDIO_SPEECH)

        // Assuming codec adc always records mono data

#ifdef SW_CAPTURE_RESAMPLE
#if (CHAN_NUM_CAPTURE == 1) && (CHAN_NUM_SEND == 2)
        if (resample_cap_enabled) {
            //ASSERT(cur_buf == dst_cur_buf, "Bad assumption");
            uint32_t src_len;

            if (buf_end < cur_buf) {
                src_len = buf_end + usb_send_size - cur_buf;
            } else {
                src_len = buf_end - cur_buf;
            }
            cur_buf = buf_end - 2;
            if (cur_buf < dst_buf_start) {
                cur_buf += usb_send_size;
            }
            dst_cur_buf += src_len * 2 - 4;
            if (dst_cur_buf >= dst_buf_end) {
                dst_cur_buf -= usb_send_size;
            }
            while (src_len > 0) {
                *(int16_t *)dst_cur_buf = *(int16_t *)cur_buf;
                *(int16_t *)(dst_cur_buf + 2) = *(int16_t *)cur_buf;

                // Move to next data
                src_len -= 2;
                cur_buf -= 2;
                if (cur_buf < dst_buf_start) {
                    cur_buf += usb_send_size;
                }
                dst_cur_buf -= 4;
                if (dst_cur_buf < dst_buf_start) {
                    dst_cur_buf += usb_send_size;
                }
            }
        } else
#endif
#endif
        {
            while (cur_buf + 2 <= buf_end) {
                // Duplicate the mono data
                if (dst_cur_buf + 4 > dst_buf_end) {
                    dst_cur_buf = dst_buf_start;
                }
                *(int16_t *)dst_cur_buf = *(int16_t *)cur_buf;
                *(int16_t *)(dst_cur_buf + 2) = *(int16_t *)cur_buf;

                // Move to next data
                dst_cur_buf += 4;
                cur_buf += 2;
            }
        }

#else // (CHAN_NUM_CAPTURE == CHAN_NUM_SEND)

        // The channel numbers of codec adc and USB data are the same

        if (sample_size_cap == sample_size_send) {
#if ((CHAN_NUM_CAPTURE & 1) == 0)
#define COPY_MEM(dst, src, size)        copy_mem32(dst, src, size)
#else
#define COPY_MEM(dst, src, size)        copy_mem16(dst, src, size)
#endif
            if (dst_cur_buf != cur_buf) {
                if (dst_cur_buf + len <= dst_buf_end) {
                    COPY_MEM(dst_cur_buf, cur_buf, len);
                } else {
                    uint32_t copy_len;

                    if (dst_cur_buf >= dst_buf_end) {
                        copy_len = 0;
                    } else {
                        copy_len = dst_buf_end - dst_cur_buf;
                        COPY_MEM(dst_cur_buf, cur_buf, copy_len);
                    }
                    COPY_MEM(dst_buf_start, cur_buf + copy_len, len - copy_len);
                }
            }
#undef COPY_MEM
        } else if (sample_size_cap == 4 && sample_size_send == 3) {
            uint32_t codec_step, usb_step;
            uint32_t ch;

            codec_step = CHAN_NUM_CAPTURE * 4;
            usb_step = CHAN_NUM_SEND * 3;

            while (cur_buf + codec_step <= buf_end) {
                if (dst_cur_buf + usb_step > dst_buf_end) {
                    dst_cur_buf = dst_buf_start;
                }
                for (ch = 0; ch < CHAN_NUM_CAPTURE; ch++) {
                    *(uint16_t *)dst_cur_buf = *(uint16_t *)cur_buf;
                    *(uint8_t *)(dst_cur_buf + 2) = *(uint8_t *)(cur_buf + 2);
                    // Move to next channel
                    dst_cur_buf += 3;
                    cur_buf += 4;
                }
            }
        } else {
            ASSERT(false, "4: Invalid sample_size_cap=%u sample_size_send=%u", sample_size_cap, sample_size_send);
        }

#endif // (CHAN_NUM_CAPTURE == CHAN_NUM_SEND)

#endif // !(CHIP_BEST1000 && (ANC_APP || _DUAL_AUX_MIC_))

#ifdef SW_CAPTURE_RESAMPLE
#if !((CHAN_NUM_CAPTURE == 1) && (CHAN_NUM_SEND == 2))
        if (resample_cap_enabled) {
            io.in = resample_input_buf;
            io.in_size = resample_frame_len;
            io.out = usb_send_buf + old_wpos;
            io.out_size = usb_len;
            io.out_cyclic_start = usb_send_buf;
            io.out_cyclic_end = usb_send_buf + usb_send_size;

            ret = audio_resample_ex_run(resample_id, &io, &in_size, &out_size);
            ASSERT((ret == RESAMPLE_STATUS_IN_EMPTY || ret == RESAMPLE_STATUS_DONE) && io.out_size >= out_size,
                "Failed to resample: %d io.out_size=%u io.in_size=%u in_size=%u out_size=%u",
                ret, io.out_size, io.in_size, in_size, out_size);

            uint32_t diff = usb_len - out_size;
            if (new_wpos >= diff) {
                new_wpos -= diff;
            } else {
                new_wpos = new_wpos + usb_send_size - diff;
            }

#if (VERBOSE_TRACE & (1 << 3))
            TRACE(5,"cap_resample: new_wpos=%u, in: %u -> %u, out: %u -> %u", new_wpos, io.in_size, in_size, io.out_size, out_size);
#endif
        }
#endif
#endif
    }

    //----------------------------------------
    // CODEC->USB stream format conversion end
    //----------------------------------------
_conv_end:

    cap_next = buf + len - capture_buf;
    if (cap_next >= capture_size) {
        cap_next -= capture_size;
    }

    lock = int_lock();
    if (codec_cap_seq == usb_send_seq) {
        if (capture_state == AUDIO_ITF_STATE_STARTED) {
            usb_send_wpos = new_wpos;
        } else {
            usb_send_wpos = 0;
        }
        capture_pos = cap_next;
        if (conflicted) {
            capture_conflicted = conflicted;
        }
    }
    int_unlock(lock);

#if (VERBOSE_TRACE & (1 << 4))
    TRACE_TIME(0,"captureEnd");
#endif

    return 0;
}

static void update_playback_sync_info(void)
{
    playback_info.err_thresh = DIFF_ERR_THRESH_PLAYBACK;
    playback_info.samp_rate = sample_rate_play;
    playback_info.samp_cnt = byte_to_samp_recv(usb_recv_size);
#ifdef TARGET_TO_MAX_DIFF
    playback_info.max_target_thresh = playback_info.samp_cnt / 2;
    playback_info.diff_target = byte_to_samp_recv(playback_to_recv_len(playback_size / 2)) + playback_info.samp_cnt / 2;
    playback_info.diff_target = usb_audio_sync_normalize_diff(-playback_info.diff_target, playback_info.samp_cnt);
#endif

    TRACE(4,"%s: rate=%u cnt=%u (%u)", __FUNCTION__, playback_info.samp_rate, playback_info.samp_cnt, usb_recv_size);
}

static void update_capture_sync_info(void)
{
    capture_info.err_thresh = DIFF_ERR_THRESH_CAPTURE;
    capture_info.samp_rate = sample_rate_cap;
    capture_info.samp_cnt = byte_to_samp_send(usb_send_size);
#ifdef TARGET_TO_MAX_DIFF
    capture_info.max_target_thresh = capture_info.samp_cnt / 2;
    capture_info.diff_target = byte_to_samp_send(capture_to_send_len(capture_size / 2)) + capture_info.samp_cnt / 2;
    capture_info.diff_target = usb_audio_sync_normalize_diff(capture_info.diff_target, capture_info.samp_cnt);
#endif

    TRACE(4,"%s: rate=%u cnt=%u (%u)", __FUNCTION__, capture_info.samp_rate, capture_info.samp_cnt, usb_send_size);
}

static int usb_audio_open_codec_stream(enum AUD_STREAM_T stream, enum AUDIO_STREAM_REQ_USER_T user)
{
    int ret = 0;
    struct AF_STREAM_CONFIG_T stream_cfg;

    TRACE(4,"%s: stream=%d user=%d map=0x%x", __FUNCTION__, stream, user, codec_stream_map[stream][AUDIO_STREAM_OPENED]);

    if (user >= AUDIO_STREAM_REQ_USER_QTY || codec_stream_map[stream][AUDIO_STREAM_OPENED] == 0) {
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        if (stream == AUD_STREAM_PLAYBACK) {
#if defined(USB_AUDIO_DYN_CFG) && defined(KEEP_SAME_LATENCY)
            playback_size = calc_playback_size(sample_rate_play);
#endif

            update_playback_sync_info();

#ifdef CODEC_DSD
            if (codec_dsd_enabled) {
                stream_cfg.bits = AUD_BITS_24;
            } else
#endif
            {
                stream_cfg.bits = sample_size_to_enum_playback(sample_size_play);
            }
            stream_cfg.sample_rate = sample_rate_play;
            stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_PLAYBACK);
#ifdef USB_I2S_APP
#if defined(USB_I2S_ID) && (USB_I2S_ID == 0)
            stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
#else
            stream_cfg.device = AUD_STREAM_USE_I2S1_MASTER;
#endif
#else
            stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
            stream_cfg.vol = playback_vol;
            stream_cfg.handler = usb_audio_data_playback;
            stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
            stream_cfg.data_ptr = playback_buf;
            stream_cfg.data_size = playback_size;

            //TRACE(3,"[%s] sample_rate: %d, bits = %d", __func__, stream_cfg.sample_rate, stream_cfg.bits);

            ret = af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
            ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);

#ifdef AUDIO_ANC_FB_MC
            stream_cfg.bits = sample_size_to_enum_playback(sample_size_play);
            stream_cfg.sample_rate = sample_rate_play;
            stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_PLAYBACK);
            stream_cfg.device = AUD_STREAM_USE_MC;
            stream_cfg.vol = playback_vol;
            stream_cfg.handler = audio_mc_data_playback;
            stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
            stream_cfg.data_ptr = playback_buf+playback_size;

            mid_p_8_old_l=0;
            mid_p_8_old_r=0;

            if(sample_rate_play==AUD_SAMPRATE_8000)
            {
                playback_samplerate_ratio=8*6;
            }
            else if(sample_rate_play==AUD_SAMPRATE_16000)
            {
                playback_samplerate_ratio=8*3;
            }
            else if((sample_rate_play==AUD_SAMPRATE_44100)||(sample_rate_play==AUD_SAMPRATE_48000)||(sample_rate_play==AUD_SAMPRATE_50781))
            {
                playback_samplerate_ratio=8;
            }
            else if((sample_rate_play==AUD_SAMPRATE_88200)||(sample_rate_play==AUD_SAMPRATE_96000))
            {
                playback_samplerate_ratio=4;
            }
            else if((sample_rate_play==AUD_SAMPRATE_176400)||(sample_rate_play==AUD_SAMPRATE_192000))
            {
                playback_samplerate_ratio=2;
            }
            else if(sample_rate_play==AUD_SAMPRATE_384000)
            {
                playback_samplerate_ratio=1;
            }
            else
            {
                playback_samplerate_ratio=1;
                ASSERT(false, "Music cancel can't support playback sample rate:%d",sample_rate_play);
            }

            anc_mc_run_init((sample_rate_play*playback_samplerate_ratio)/8);

            stream_cfg.data_size = playback_size*playback_samplerate_ratio;

            //TRACE(3,"[%s] sample_rate: %d, bits = %d", __func__, stream_cfg.sample_rate, stream_cfg.bits);

            ret = af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
            ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);
#endif
        } else {
#if defined(USB_AUDIO_DYN_CFG) && defined(KEEP_SAME_LATENCY)
            capture_size = calc_capture_size(sample_rate_cap);
#endif

            update_capture_sync_info();

            stream_cfg.bits = sample_size_to_enum_capture(sample_size_cap);
            stream_cfg.sample_rate = sample_rate_cap;
            stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_CAPTURE);
#ifdef ANC_PROD_TEST
            stream_cfg.channel_map=anc_fb_mic_ch_l|anc_fb_mic_ch_r;
#endif
#ifdef USB_I2S_APP
#if defined(USB_I2S_ID) && (USB_I2S_ID == 0)
            stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
#else
            stream_cfg.device = AUD_STREAM_USE_I2S1_MASTER;
#endif
#else
            stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#endif
            stream_cfg.vol = capture_vol;
            stream_cfg.handler = usb_audio_data_capture;
#if !defined(USB_AUDIO_SPEECH) && defined(BTUSB_AUDIO_MODE)
            stream_cfg.io_path = AUD_INPUT_PATH_USBAUDIO;
#else
            stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
#endif
            stream_cfg.data_ptr = capture_buf;
            stream_cfg.data_size = capture_size;

            //TRACE(4,"[%s] sample_rate: %d, bits = %d, ch_num = %d", __func__, stream_cfg.sample_rate, stream_cfg.bits, stream_cfg.channel_num);

            ret = af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
            ASSERT(ret == 0, "af_stream_open catpure failed: %d", ret);
        }
    }

    if (user < AUDIO_STREAM_REQ_USER_QTY) {
        codec_stream_map[stream][AUDIO_STREAM_OPENED] |= (1 << user);
    }

    return ret;
}

static int usb_audio_close_codec_stream(enum AUD_STREAM_T stream, enum AUDIO_STREAM_REQ_USER_T user)
{
    int ret = 0;

    TRACE(4,"%s: stream=%d user=%d map=0x%x", __FUNCTION__, stream, user, codec_stream_map[stream][AUDIO_STREAM_OPENED]);

    if (user < AUDIO_STREAM_REQ_USER_QTY) {
        codec_stream_map[stream][AUDIO_STREAM_OPENED] &= ~(1 << user);
    }

    if (user >= AUDIO_STREAM_REQ_USER_QTY || codec_stream_map[stream][AUDIO_STREAM_OPENED] == 0) {
        ret = af_stream_close(AUD_STREAM_ID_0, stream);

#ifdef AUDIO_ANC_FB_MC
        if(stream == AUD_STREAM_PLAYBACK)
        {
            ret = af_stream_close(AUD_STREAM_ID_2, stream);
        }
#endif
    }

    return ret;
}

static int usb_audio_open_eq(void)
{
    int ret = 0;

    TRACE(4,"[%s] EQ: sample_rate_recv=%d, sample_rate_play=%d, sample_bits=%d", __func__,
        sample_rate_recv, sample_rate_play, sample_size_to_enum_playback(sample_size_play));

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) || defined(__HW_IIR_EQ_PROCESS__) || defined(__HW_DAC_IIR_EQ_PROCESS__)
    enum AUD_BITS_T sample_bits = sample_size_to_enum_playback(sample_size_play);
    enum AUD_CHANNEL_NUM_T chan_num = chan_num_to_enum(CHAN_NUM_PLAYBACK);
    ret = audio_process_open(sample_rate_play, sample_bits, chan_num, playback_eq_size/2, playback_eq_buf, playback_eq_size);

    //TRACE(1,"audio_process_open: %d", ret);

#ifdef __SW_IIR_EQ_PROCESS__
    usb_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,0));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
    usb_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,0));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
    usb_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,0));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
    usb_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,0));
#endif

#endif

    eq_opened = 1;

    return ret;
}

static int usb_audio_close_eq(void)
{
    int ret = 0;

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) || defined(__HW_IIR_EQ_PROCESS__) || defined(__HW_DAC_IIR_EQ_PROCESS__)
    ret = audio_process_close();
#endif

    eq_opened = 0;

    return ret;
}

static int usb_audio_start_codec_stream(enum AUD_STREAM_T stream, enum AUDIO_STREAM_REQ_USER_T user)
{
    int ret = 0;

    TRACE(4,"%s: stream=%d user=%d map=0x%x", __FUNCTION__, stream, user, codec_stream_map[stream][AUDIO_STREAM_STARTED]);

    if (user >= AUDIO_STREAM_REQ_USER_QTY || codec_stream_map[stream][AUDIO_STREAM_STARTED] == 0) {
        reset_conflict();

        if (stream == AUD_STREAM_PLAYBACK) {
            zero_mem32(playback_buf, playback_size);
            codec_play_valid = 0;
            usb_recv_init_rpos = 0;
            playback_pos = 0;
            playback_conflicted = 0;
#ifdef USB_AUDIO_MULTIFUNC
            usb_recv2_init_rpos = 0;
            playback_conflicted2 = 0;
#endif
#ifdef DSD_SUPPORT
            if (codec_dsd_enabled) {
#if defined(__HW_FIR_DSD_PROCESS__)
                uint8_t *dsd_buf;
                uint32_t dsd_size;

                // DoP format requirement
                dsd_size = playback_size * 16;;
                dsd_buf = (uint8_t *)(HW_FIR_DSD_BUF_MID_ADDR - dsd_size / 2);
                dsd_open(sample_rate_play,AUD_BITS_24,AUD_CHANNEL_NUM_2,dsd_buf,dsd_size);
#elif defined(CODEC_DSD)
                af_dsd_enable();
#endif
            }
#endif
            usb_audio_open_eq();
        } else {
#if defined(CHIP_BEST1000) && defined(_DUAL_AUX_MIC_)
            damic_init();
            init_amic_dc();
#endif

#ifdef USB_AUDIO_SPEECH
            app_overlay_select(APP_OVERLAY_HFP);

            speech_process_init(sample_rate_cap, CHAN_NUM_CAPTURE, sample_size_to_enum_playback(sample_size_cap),
                                sample_rate_play, CHAN_NUM_PLAYBACK, sample_size_to_enum_playback(sample_size_play),
                                16, 16,
                                CHAN_NUM_SEND, CHAN_NUM_RECV);
#endif

            codec_cap_valid = 0;
            usb_send_init_wpos = 0;
            capture_pos = 0;
            capture_conflicted = 0;
        }

        // Tune rate according to the newest sync ratio info
        usb_audio_cmd_tune_rate(stream);

        ret = af_stream_start(AUD_STREAM_ID_0, stream);
        ASSERT(ret == 0, "af_stream_start %d failed: %d", stream, ret);

        if (stream == AUD_STREAM_PLAYBACK) {
#ifdef AUDIO_ANC_FB_MC
            ret = af_stream_start(AUD_STREAM_ID_2, stream);
            ASSERT(ret == 0, "af_stream_start %d failed: %d", stream, ret);
#endif
        } else {
            codec_cap_start_time = hal_sys_timer_get();
        }
    }

    if (user < AUDIO_STREAM_REQ_USER_QTY) {
        codec_stream_map[stream][AUDIO_STREAM_STARTED] |= (1 << user);
    }

    return ret;
}

static int usb_audio_stop_codec_stream(enum AUD_STREAM_T stream, enum AUDIO_STREAM_REQ_USER_T user)
{
    int ret = 0;

    TRACE(4,"%s: stream=%d user=%d map=0x%x", __FUNCTION__, stream, user, codec_stream_map[stream][AUDIO_STREAM_STARTED]);

    if (user < AUDIO_STREAM_REQ_USER_QTY) {
        codec_stream_map[stream][AUDIO_STREAM_STARTED] &= ~(1 << user);
    }

    if (user >= AUDIO_STREAM_REQ_USER_QTY || codec_stream_map[stream][AUDIO_STREAM_STARTED] == 0) {
        ret = af_stream_stop(AUD_STREAM_ID_0, stream);

        if (stream == AUD_STREAM_PLAYBACK) {
#ifdef AUDIO_ANC_FB_MC
            ret = af_stream_stop(AUD_STREAM_ID_2, stream);
#endif
#ifdef DSD_SUPPORT
            if (codec_dsd_enabled) {
#if defined(__HW_FIR_DSD_PROCESS__)
                dsd_close();
#elif defined(CODEC_DSD)
                af_dsd_disable();
#endif
            }
#endif
            usb_audio_close_eq();
        } else {
#if defined(CHIP_BEST1000) && defined(_DUAL_AUX_MIC_)
            damic_deinit();
#endif

#ifdef USB_AUDIO_SPEECH
            speech_process_deinit();
            app_overlay_unloadall();
#endif
        }
    }

    return ret;
}

static int usb_audio_start_usb_stream(enum AUD_STREAM_T stream)
{
    int ret;

    reset_conflict();

    if (stream == AUD_STREAM_PLAYBACK) {
        usb_audio_sync_reset(&playback_info);

        usb_recv_valid = 0;
#ifdef USB_AUDIO_MULTIFUNC
        if (recv2_state == AUDIO_ITF_STATE_STOPPED)
#endif
        {
            usb_recv_seq++;
        }

        usb_recv_rpos = 0;
        usb_recv_wpos = usb_recv_size / 2;
        zero_mem32(usb_recv_buf, usb_recv_size);

        usb_recv_err_cnt = 0;

#ifdef DSD_SUPPORT
        usb_dsd_enabled = false;
        usb_dsd_cont_cnt = 0;
#endif

        ret = usb_audio_start_recv(usb_recv_buf, usb_recv_wpos, usb_recv_size);
        if (ret == 1) {
            TRACE(0,"usb_audio_start_recv failed: usb not configured");
        } else {
            ASSERT(ret == 0, "usb_audio_start_recv failed: %d", ret);
        }
    } else {
#ifndef UAUD_SYNC_STREAM_TARGET
        if (recv_state == AUDIO_ITF_STATE_STOPPED)
#endif
        {
            usb_audio_sync_reset(&capture_info);
        }

        usb_send_valid = 0;
        usb_send_seq++;

        usb_send_rpos = usb_send_size / 2;
        usb_send_wpos = 0;
        zero_mem32(usb_send_buf, usb_send_size);

        usb_send_err_cnt = 0;

        ret = usb_audio_start_send(usb_send_buf, usb_send_rpos, usb_send_size);
        if (ret == 1) {
            TRACE(0,"usb_audio_start_send failed: usb not configured");
        } else {
            ASSERT(ret == 0, "usb_audio_start_send failed: %d", ret);
        }
    }

    return ret;
}

static int usb_audio_stop_usb_stream(enum AUD_STREAM_T stream)
{
    if (stream == AUD_STREAM_PLAYBACK) {
        usb_audio_stop_recv();

#ifdef DSD_SUPPORT
        usb_dsd_enabled = false;
#endif

        usb_audio_sync_reset(&playback_info);
    } else {
        usb_audio_stop_send();

#ifndef UAUD_SYNC_STREAM_TARGET
        if (recv_state == AUDIO_ITF_STATE_STOPPED)
#endif
        {
            usb_audio_sync_reset(&capture_info);
        }
    }

    return 0;
}

static void usb_audio_set_codec_volume(enum AUD_STREAM_T stream, uint8_t vol)
{
    struct AF_STREAM_CONFIG_T *cfg;
    uint8_t POSSIBLY_UNUSED old_vol;
    uint32_t ret;

    ret = af_stream_get_cfg(AUD_STREAM_ID_0, stream, &cfg, true);
    if (ret == 0) {
        old_vol = cfg->vol;
        if (old_vol != vol) {
            cfg->vol = vol;
            af_stream_setup(AUD_STREAM_ID_0, stream, cfg);
        }
    }

#ifdef AUDIO_ANC_FB_MC
    if (stream == AUD_STREAM_PLAYBACK) {
        ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &cfg, true);
        if (ret == 0) {
            if (cfg->vol != vol) {
                cfg->vol = vol;
                af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, cfg);
            }
        }
    }
#endif
}

static void usb_audio_enqueue_cmd(uint32_t data)
{
    int ret;
    uint32_t mark;

    ret = safe_queue_put(&cmd_queue, data);
    ASSERT(ret == 0 || safe_queue_dump(&cmd_queue), "%s: cmd queue overflow", __FUNCTION__);

    hal_cpu_wake_lock(USB_AUDIO_CPU_WAKE_USER);

    mark = safe_queue_watermark(&cmd_queue);
    if (mark > cmd_watermark) {
        cmd_watermark = mark;
        TRACE(2,"%s: new watermark %u", __FUNCTION__, mark);
    }

#ifdef BT_USB_AUDIO_DUAL_MODE
    if (enqueue_cmd_cb) {
        enqueue_cmd_cb(data);
    }
#endif
}

#ifdef BT_USB_AUDIO_DUAL_MODE
void usb_audio_set_enqueue_cmd_callback(USB_AUDIO_ENQUEUE_CMD_CALLBACK cb)
{
    enqueue_cmd_cb = cb;
}
#endif

static void enqueue_unique_cmd_with_opp(enum AUDIO_CMD_T cmd, uint8_t seq, uint8_t arg, enum AUDIO_CMD_T oppsite_cmd)
{
    uint32_t last[3];
    uint32_t lock;
    bool dup = false;

    lock = int_lock();
    if (safe_queue_peek(&cmd_queue, -1, &last[0]) == 0 &&
            safe_queue_peek(&cmd_queue, -2, &last[1]) == 0 &&
            safe_queue_peek(&cmd_queue, -3, &last[2]) == 0) {
        if (EXTRACT_CMD(last[0]) == cmd) {
            dup = true;
            safe_queue_pop(&cmd_queue, NULL);
        } else if (EXTRACT_CMD(last[0]) == oppsite_cmd &&
                EXTRACT_CMD(last[1]) == cmd &&
                EXTRACT_CMD(last[2]) == oppsite_cmd) {
            dup = true;
            safe_queue_pop(&cmd_queue, NULL);
            safe_queue_pop(&cmd_queue, NULL);
        }
    }
    int_unlock(lock);

    usb_audio_enqueue_cmd(MAKE_QUEUE_DATA(cmd, seq, arg));

    if (dup) {
        TRACE(2,"%s: Remove old duplicate cmd %d", __FUNCTION__, cmd);
    }
}

static void enqueue_unique_cmd_arg(enum AUDIO_CMD_T cmd, uint8_t seq, uint8_t arg)
{
    uint32_t last;
    uint32_t cur;
    uint32_t lock;
    bool enqueue = true;

    cur = MAKE_QUEUE_DATA(cmd, seq, arg);

    lock = int_lock();
    if (safe_queue_peek(&cmd_queue, -1, &last) == 0) {
        if (last == cur) {
            enqueue = false;
        }
    }
    int_unlock(lock);

    if (enqueue) {
        usb_audio_enqueue_cmd(cur);
    } else {
        TRACE(2,"%s: Skip duplicate cmd %d", __FUNCTION__, cmd);
    }
}

static void enqueue_unique_cmd(enum AUDIO_CMD_T cmd)
{
    enqueue_unique_cmd_arg(cmd, 0, 0);
}

static void usb_audio_playback_start(enum USB_AUDIO_ITF_CMD_T cmd)
{
    if (cmd == USB_AUDIO_ITF_STOP) {
        // Stop the stream right now
        if (recv_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_usb_stream(AUD_STREAM_PLAYBACK);
            recv_state = AUDIO_ITF_STATE_STOPPED;
        }

#ifdef USB_AUDIO_DYN_CFG
        playback_itf_set = 0;
#ifdef USB_AUDIO_UAC2
        enqueue_unique_cmd_with_opp(AUDIO_CMD_STOP_PLAY, usb_recv_seq, 0, AUDIO_CMD_START_PLAY);
#else
        enqueue_unique_cmd_with_opp(AUDIO_CMD_STOP_PLAY, usb_recv_seq, 0, AUDIO_CMD_SET_RECV_RATE);
#endif
#else

#ifdef USB_AUDIO_MULTIFUNC
        if (recv2_state == AUDIO_ITF_STATE_STARTED) {
            return;
        }
#endif

        enqueue_unique_cmd_with_opp(AUDIO_CMD_STOP_PLAY, usb_recv_seq, 0, AUDIO_CMD_START_PLAY);
#endif
    } else {
#ifdef USB_AUDIO_DYN_CFG
        if (cmd == USB_AUDIO_ITF_START_16BIT) {
            new_sample_size_recv = 2;
        } else if (cmd == USB_AUDIO_ITF_START_24BIT) {
            new_sample_size_recv = 3;
        } else {
            new_sample_size_recv = 4;
        }
        playback_itf_set = 1;

        // Some host applications will not stop current stream before changing sample size
        if (recv_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_usb_stream(AUD_STREAM_PLAYBACK);
            recv_state = AUDIO_ITF_STATE_STOPPED;
        }

#ifdef USB_AUDIO_UAC2
#ifdef KEEP_SAME_LATENCY
        usb_recv_size = calc_usb_recv_size(new_sample_rate_recv, new_sample_size_recv);
        TRACE(2,"%s: Set usb_recv_size=%u", __FUNCTION__, usb_recv_size);
#endif

        usb_audio_start_usb_stream(AUD_STREAM_PLAYBACK);
        recv_state = AUDIO_ITF_STATE_STARTED;

        enqueue_unique_cmd_with_opp(AUDIO_CMD_START_PLAY, usb_recv_seq, 0, AUDIO_CMD_STOP_PLAY);
#else
        // Wait for sampling freq ctrl msg to start the stream
#endif
        return;
#else
        if (recv_state == AUDIO_ITF_STATE_STOPPED) {
            usb_audio_start_usb_stream(AUD_STREAM_PLAYBACK);
            recv_state = AUDIO_ITF_STATE_STARTED;
        }

#ifdef USB_AUDIO_MULTIFUNC
        if (recv2_state == AUDIO_ITF_STATE_STARTED) {
            return;
        }
#endif

        enqueue_unique_cmd_with_opp(AUDIO_CMD_START_PLAY, usb_recv_seq, 0, AUDIO_CMD_STOP_PLAY);
#endif
    }
}

static void usb_audio_capture_start(enum USB_AUDIO_ITF_CMD_T cmd)
{
    if (cmd == USB_AUDIO_ITF_STOP) {
        // Stop the stream right now
        if (send_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_usb_stream(AUD_STREAM_CAPTURE);
            send_state = AUDIO_ITF_STATE_STOPPED;
        }

#ifdef USB_AUDIO_DYN_CFG
        capture_itf_set = 0;
#ifdef USB_AUDIO_UAC2
        enqueue_unique_cmd_with_opp(AUDIO_CMD_STOP_CAPTURE, usb_send_seq, 0, AUDIO_CMD_START_CAPTURE);
#else
        enqueue_unique_cmd_with_opp(AUDIO_CMD_STOP_CAPTURE, usb_send_seq, 0, AUDIO_CMD_SET_SEND_RATE);
#endif
#else
        enqueue_unique_cmd_with_opp(AUDIO_CMD_STOP_CAPTURE, usb_send_seq, 0, AUDIO_CMD_START_CAPTURE);
#endif
    } else {
#ifdef USB_AUDIO_DYN_CFG
        capture_itf_set = 1;

        // Some host applications will not stop current stream before changing sample size
        if (send_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_usb_stream(AUD_STREAM_CAPTURE);
            send_state = AUDIO_ITF_STATE_STOPPED;
        }

#ifdef USB_AUDIO_UAC2
        usb_audio_start_usb_stream(AUD_STREAM_CAPTURE);
        send_state = AUDIO_ITF_STATE_STARTED;

        enqueue_unique_cmd_with_opp(AUDIO_CMD_START_CAPTURE, usb_send_seq, 0, AUDIO_CMD_STOP_CAPTURE);
#else
        // Wait for sampling freq ctrl msg to start the stream
#endif
        return;
#else
        if (send_state == AUDIO_ITF_STATE_STOPPED) {
            usb_audio_start_usb_stream(AUD_STREAM_CAPTURE);
            send_state = AUDIO_ITF_STATE_STARTED;
        }

        enqueue_unique_cmd_with_opp(AUDIO_CMD_START_CAPTURE, usb_send_seq, 0, AUDIO_CMD_STOP_CAPTURE);
#endif
    }
}

static void usb_audio_mute_control(uint32_t mute)
{
    TRACE(1,"MUTE CTRL: %u", mute);

    new_mute_state = mute;
#ifdef USB_AUDIO_MULTIFUNC
    if (mute) {
        new_playback_coef = 0;
    } else {
        new_playback_coef = playback_gain_to_float(new_playback_vol);
    }
#endif
    usb_audio_enqueue_cmd(AUDIO_CMD_MUTE_CTRL);
}

static void usb_audio_cap_mute_control(uint32_t mute)
{
    TRACE(1,"CAP MUTE CTRL: %u", mute);

    new_cap_mute_state = mute;
    usb_audio_enqueue_cmd(AUDIO_CMD_CAP_MUTE_CTRL);
}

static void usb_audio_vol_control(uint32_t percent)
{
    if (percent >= 100) {
        new_playback_vol = MAX_VOLUME_VAL;
    } else {
        new_playback_vol = MIN_VOLUME_VAL + (percent * (MAX_VOLUME_VAL - MIN_VOLUME_VAL) + 50) / 100;
    }

    TRACE(2,"VOL CTRL: percent=%u new_playback_vol=%u", percent, new_playback_vol);

#ifdef USB_AUDIO_MULTIFUNC
    new_playback_coef = playback_gain_to_float(new_playback_vol);
#else
    usb_audio_enqueue_cmd(AUDIO_CMD_SET_VOLUME);
#endif
}

static void usb_audio_cap_vol_control(uint32_t percent)
{
    if (percent >= 100) {
        new_capture_vol = MAX_CAP_VOLUME_VAL;
    } else {
        new_capture_vol = MIN_CAP_VOLUME_VAL + (percent * (MAX_CAP_VOLUME_VAL - MIN_CAP_VOLUME_VAL) + 50) / 100;
    }

    TRACE(2,"CAP VOL CTRL: percent=%u new_capture_vol=%u", percent, new_capture_vol);

    usb_audio_enqueue_cmd(AUDIO_CMD_SET_CAP_VOLUME);
}

static uint32_t usb_audio_get_vol_percent(void)
{
    if (new_playback_vol >= MAX_VOLUME_VAL) {
        return 100;
    } else if (new_playback_vol <= MIN_VOLUME_VAL) {
        return 0;
    } else {
        return ((new_playback_vol - MIN_VOLUME_VAL) * 100 + (MAX_VOLUME_VAL - MIN_VOLUME_VAL) / 2)
            / (MAX_VOLUME_VAL - MIN_VOLUME_VAL);
    }
}

static uint32_t usb_audio_get_cap_vol_percent(void)
{
    if (new_capture_vol >= MAX_CAP_VOLUME_VAL) {
        return 100;
    } else if (new_capture_vol <= MIN_CAP_VOLUME_VAL) {
        return 0;
    } else {
        return ((new_capture_vol - MIN_CAP_VOLUME_VAL) * 100 + (MAX_CAP_VOLUME_VAL - MIN_CAP_VOLUME_VAL) / 2)
            / (MAX_CAP_VOLUME_VAL - MIN_CAP_VOLUME_VAL);
    }
}

static void usb_audio_tune_rate(enum AUD_STREAM_T stream, float ratio)
{
    bool update_play, update_cap;

    update_play = true;
    update_cap = false;

#if defined(__AUDIO_RESAMPLE__) && defined(PLL_TUNE_SAMPLE_RATE)
#ifdef UAUD_SYNC_STREAM_TARGET
    if (stream == AUD_STREAM_PLAYBACK) {
        update_play = true;
        update_cap = false;
    } else {
        update_play = false;
        update_cap = true;
    }
#else
    update_play = true;
    update_cap = true;
#endif
#endif

    if (update_play) {
        rate_tune_ratio[AUD_STREAM_PLAYBACK] = rate_tune_ratio[AUD_STREAM_PLAYBACK] +
            ratio + rate_tune_ratio[AUD_STREAM_PLAYBACK] * ratio;
    }
    if (update_cap) {
        rate_tune_ratio[AUD_STREAM_CAPTURE] = rate_tune_ratio[AUD_STREAM_CAPTURE] +
            ratio + rate_tune_ratio[AUD_STREAM_CAPTURE] * ratio;
    }

    TRACE(4,"%s[%u]: ratio=%d resample_ratio=%d", __FUNCTION__, stream, FLOAT_TO_PPB_INT(ratio),
        update_cap ? FLOAT_TO_PPB_INT(rate_tune_ratio[stream]) : FLOAT_TO_PPB_INT(rate_tune_ratio[AUD_STREAM_PLAYBACK]));

    enqueue_unique_cmd_arg(AUDIO_CMD_TUNE_RATE, 0, stream);
}

static void usb_audio_data_recv_handler(const struct USB_AUDIO_XFER_INFO_T *info)
{
    const uint8_t *data;
    uint32_t size;
    uint32_t old_wpos, new_wpos, wpos_boundary, rpos;
    uint32_t old_play_pos = 0;
    uint32_t play_pos;
    uint32_t lock;
    int conflicted;
    uint32_t recv_samp, play_samp;
    uint32_t cur_time;

    cur_time = hal_sys_timer_get();
    last_usb_recv_time = cur_time;

    data = info->data;
    size = info->size;

    if (info->cur_compl_err || info->next_xfer_err) {
        if (usb_recv_err_cnt == 0) {
            usb_recv_err_cnt++;
            usb_recv_ok_cnt = 0;
            last_usb_recv_err_time = cur_time;
        } else {
            usb_recv_err_cnt++;
        }
    } else {
        if (usb_recv_err_cnt) {
            usb_recv_ok_cnt++;
        }
    }
    if (usb_recv_err_cnt && cur_time - last_usb_recv_err_time >= USB_XFER_ERR_REPORT_INTERVAL) {
        if (info->cur_compl_err || info->next_xfer_err) {
            TRACE(2,"recv: ERROR: cur_err=%d next_err=%d", info->cur_compl_err, info->next_xfer_err);
        }
        TRACE(3,"recv: ERROR-CNT: err=%u ok=%u in %u ms", usb_recv_err_cnt, usb_recv_ok_cnt, TICKS_TO_MS(USB_XFER_ERR_REPORT_INTERVAL));
        usb_recv_err_cnt = 0;
    }

    if (usb_recv_buf <= data && data <= usb_recv_buf + usb_recv_size) {
        if (data != usb_recv_buf + usb_recv_wpos) {
            TRACE(3,"recv: WARNING: Invalid wpos=0x%x data=%p recv_buf=%p. IRQ missing?", usb_recv_wpos, data, usb_recv_buf);
            usb_recv_wpos = data - usb_recv_buf;
        }
    }

    old_wpos = usb_recv_wpos;
    new_wpos = old_wpos + size;
    if (new_wpos >= usb_recv_size) {
        new_wpos -= usb_recv_size;
    }

    if (recv_state == AUDIO_ITF_STATE_STOPPED ||
            size == 0 || // rx paused
            0) {
        usb_recv_valid = 0;
    } else {
        if (info->cur_compl_err == 0) {
            usb_recv_valid = 1;
        }
    }

    if (usb_recv_valid == 0 ||
            usb_recv_init_rpos == 0 ||
            codec_config_lock ||
            codec_play_seq != usb_recv_seq ||
            codec_play_valid == 0 ||
            playback_state == AUDIO_ITF_STATE_STOPPED ||
            0) {
        usb_recv_wpos = new_wpos;
        return;
    }

    conflicted = 0;

    lock = int_lock();
    if (playback_conflicted) {
        playback_conflicted = 0;
        rpos = 0; // Avoid compiler warnings
        conflicted = 1;
    } else {
        rpos = usb_recv_rpos;
        old_play_pos = playback_pos;
        play_pos = af_stream_get_cur_dma_pos(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
    int_unlock(lock);

    if (conflicted == 0) {
        if (info->pool_enabled) {
            // USBC will write to memory pool.
            wpos_boundary = new_wpos;
        } else {
            // USBC will write to new_wpos directly, so the buffer [new_wpos, new_wpos + info->next_size) must be protected too.
            wpos_boundary = new_wpos + info->next_size;
            if (wpos_boundary >= usb_recv_size) {
                wpos_boundary -= usb_recv_size;
            }
        }

        if (old_wpos <= rpos) {
            if (wpos_boundary < old_wpos || rpos < wpos_boundary) {
                conflicted = 1;
            }
        } else {
            if (rpos < wpos_boundary && wpos_boundary < old_wpos) {
                conflicted = 1;
            }
        }

        if (conflicted) {
            uint32_t reset_len = usb_recv_size / 2;
            uint32_t saved_old_wpos = old_wpos;

            // Reset write position
            old_wpos = rpos + reset_len;
            if (old_wpos >= usb_recv_size) {
                old_wpos -= usb_recv_size;
            }
            new_wpos = old_wpos + size;
            if (new_wpos >= usb_recv_size) {
                new_wpos -= usb_recv_size;
            }

#if 0
            usb_audio_stop_recv();
            usb_audio_start_recv(usb_recv_buf, new_wpos, usb_recv_size);
#else
            usb_audio_set_recv_pos(new_wpos);
#endif

            TRACE(4,"recv: Error: wpos=%u goes beyond rpos=%u with len=%u. Reset to %u", saved_old_wpos, rpos, size, old_wpos);
        }
        record_conflict(conflicted);
    }

    usb_recv_wpos = new_wpos;

#if (VERBOSE_TRACE & (1 << 5))
    {
        int hw_play_pos = af_stream_get_cur_dma_pos(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        TRACE_TIME(5,"recv: wpos=%u size=%u rpos=%u playback_pos=%u hw_play_pos=%d", old_wpos, size,
            rpos, playback_pos, hw_play_pos);
    }
#endif

    if (conflicted) {
        usb_audio_sync_reset(&playback_info);
        return;
    }

    if (usb_recv_buf <= data && data <= usb_recv_buf + usb_recv_size) {
        recv_samp = byte_to_samp_recv(new_wpos);
    } else {
        recv_samp = -1;
    }

    if ((int)play_pos >= 0) {
        if (play_pos >= playback_size) {
            play_pos = 0;
        }
        // Count the bytes of data waiting to play in the codec buffer
        uint32_t bytes_to_play;
        if (old_play_pos <= play_pos) {
            bytes_to_play = playback_size + old_play_pos - play_pos;
        } else {
            bytes_to_play = old_play_pos - play_pos;
        }
        uint32_t usb_bytes_to_play;
        usb_bytes_to_play = playback_to_recv_len(bytes_to_play);
        if (rpos >= usb_bytes_to_play) {
            play_pos = rpos - usb_bytes_to_play;
        } else {
            play_pos = usb_recv_size + rpos - usb_bytes_to_play;
        }
        play_samp = byte_to_samp_recv(play_pos);
    } else {
        play_samp = -1;
    }

    if (recv_samp != -1 && play_samp != -1) {
        enum UAUD_SYNC_RET_T ret;
        float ratio;

        ret = usb_audio_sync(play_samp, recv_samp, &playback_info, &ratio);
        if (ret == UAUD_SYNC_START) {
            usb_audio_tune_rate(AUD_STREAM_PLAYBACK, ratio);
        }
    } else {
        //TRACE(2,"recv_hdlr: recv_samp=0x%08x play_samp=0x%08x", recv_samp, play_samp);
    }

#ifdef DSD_SUPPORT
    if (sample_size_recv == 3 && (sample_rate_recv == 176400 || sample_rate_recv == 352800)) {
        // First DoP solution marks
        static const uint8_t pattern[2] = { 0x05, 0xFA, };
        // Second DoP solution marks -- not supported yet
        //static const uint8_t pattern2[2] = { 0x06, 0xF9, };
        static const uint16_t dsd_detect_thresh = 32;
        uint8_t idx;
        const uint8_t *dsd, *dsd_end, *dsd_end2;
        bool dsd_valid = true;

        dsd = data + 2;
        dsd_end = data + size;
        dsd_end2 = usb_recv_buf + usb_recv_size;
        if (dsd_end <= dsd_end2) {
            dsd_end2 = NULL;
        } else {
            dsd_end = dsd_end2;
            dsd_end2 = data + size - usb_recv_size;
        }
        if (*dsd == pattern[0]) {
            idx = 0;
        } else {
            idx = 1;
        }

_check_dsd:
        while (dsd < dsd_end) {
            if (*dsd == pattern[idx]) {
#if (CHAN_NUM_RECV == 2)
                dsd += 3;
                if (dsd >= dsd_end || *dsd != pattern[idx]) {
                    dsd_valid = false;
                    break;
                }
#endif
                if (usb_dsd_cont_cnt < dsd_detect_thresh) {
                    usb_dsd_cont_cnt += CHAN_NUM_RECV;
                }
                idx = !idx;
                dsd += 3;
            } else {
                dsd_valid = false;
                break;
            }
        }
        if (dsd_valid && dsd_end2) {
            dsd = usb_recv_buf + 2;
            dsd_end = dsd_end2;
            dsd_end2 = NULL;
            goto _check_dsd;
        }

        if (dsd_valid && usb_dsd_cont_cnt < dsd_detect_thresh) {
            dsd_valid = false;
        }
        if (!dsd_valid) {
            usb_dsd_cont_cnt = 0;
        }

        if (dsd_valid != usb_dsd_enabled) {
            usb_dsd_enabled = dsd_valid;
            enqueue_unique_cmd(AUDIO_CMD_SET_DSD_CFG);
        }
    }
#endif
}

static void usb_audio_data_send_handler(const struct USB_AUDIO_XFER_INFO_T *info)
{
    const uint8_t *data;
    uint32_t size;
    uint32_t old_rpos, new_rpos, rpos_boundary, wpos;
    uint32_t old_cap_pos = 0;
    uint32_t cap_pos;
    uint32_t lock;
    int conflicted;
    uint32_t send_samp, cap_samp;
    uint32_t cur_time;

    cur_time = hal_sys_timer_get();
    last_usb_send_time = cur_time;

    data = info->data;
    size = info->size;

    if (info->cur_compl_err || info->next_xfer_err) {
        if (usb_send_err_cnt == 0) {
            usb_send_err_cnt++;
            usb_send_ok_cnt = 0;
            last_usb_send_err_time = cur_time;
        } else {
            usb_send_err_cnt++;
        }
    } else {
        if (usb_send_err_cnt) {
            usb_send_ok_cnt++;
        }
    }
    if (usb_send_err_cnt && cur_time - last_usb_send_err_time >= USB_XFER_ERR_REPORT_INTERVAL) {
        if (info->cur_compl_err || info->next_xfer_err) {
            TRACE(2,"send: ERROR: cur_err=%d next_err=%d", info->cur_compl_err, info->next_xfer_err);
        }
        TRACE(3,"send: ERROR-CNT: err=%u ok=%u in %u ms", usb_send_err_cnt, usb_send_ok_cnt, TICKS_TO_MS(USB_XFER_ERR_REPORT_INTERVAL));
        usb_send_err_cnt = 0;
    }

    if (info->pool_enabled) {
        // The buffer [data, size) has been sent completely, and
        // the buffer [data + size, data + size + info->next_size) has been copied to memory pool.
        // Make usb_send_rpos point to the pending send buffer
        data += size;
        if (data >= usb_send_buf + usb_send_size) {
            data -= usb_send_size;
        }
        size = info->next_size;
    }

    if (usb_send_buf <= data && data <= usb_send_buf + usb_send_size) {
        if (data != usb_send_buf + usb_send_rpos) {
            if (usb_send_valid == 0 && info->pool_enabled && info->data != usb_send_buf + usb_send_rpos) {
                TRACE(3,"send: WARNING: Invalid rpos=0x%x data=%p send_buf=%p. IRQ missing?", usb_send_rpos, data, usb_send_buf);
            }
            usb_send_rpos = data - usb_send_buf;
        }
    }

    old_rpos = usb_send_rpos;
    new_rpos = old_rpos + size;
    if (new_rpos >= usb_send_size) {
        new_rpos -= usb_send_size;
    }

    if (send_state == AUDIO_ITF_STATE_STOPPED ||
            size == 0 || // tx paused
            0) {
        usb_send_valid = 0;
    } else {
        if (info->cur_compl_err == 0) {
            usb_send_valid = 1;
        }
    }

    if (usb_send_valid == 0 ||
            usb_send_init_wpos == 0 ||
            codec_config_lock ||
            codec_cap_seq != usb_send_seq ||
            codec_cap_valid == 0 ||
            capture_state == AUDIO_ITF_STATE_STOPPED ||
            0) {
        usb_send_rpos = new_rpos;
        if (usb_send_valid) {
            // Zero the send buffer next to the part being sent by USB h/w
            if (info->pool_enabled) {
                uint32_t clr_len;

                clr_len = samp_to_byte_send(SAMP_RATE_TO_FRAME_SIZE(sample_rate_send));
                if (usb_send_rpos + clr_len <= usb_send_size) {
                    zero_mem16(usb_send_buf + usb_send_rpos, clr_len);
                } else {
                    zero_mem16(usb_send_buf + usb_send_rpos, usb_send_size - usb_send_rpos);
                    zero_mem16(usb_send_buf, clr_len - (usb_send_size - usb_send_rpos));
                }
            } else {
                // If no pool is enabled, the send pkt size is a fixed value, and
                // the send buffer size is a integral multiple of the send pkt size
                if (usb_send_rpos + size + size <= usb_send_size) {
                    zero_mem32(usb_send_buf + usb_send_rpos + size, size);
                } else {
                    zero_mem32(usb_send_buf, size);
                }
            }
        }
        return;
    }

    conflicted = 0;

    lock = int_lock();
    if (capture_conflicted) {
        capture_conflicted = 0;
        wpos = 0; // Avoid compiler warnings
        conflicted = 1;
    } else {
        wpos = usb_send_wpos;
        old_cap_pos = capture_pos;
        cap_pos = af_stream_get_cur_dma_pos(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    }
    int_unlock(lock);

    if (conflicted == 0) {
        if (info->pool_enabled) {
            // old_rpos points to the pending send buffer, which has been copied to memory pool.
            // USBC will read from memory pool.
            rpos_boundary = new_rpos;
        } else {
            // old_rpos points to the buffer just sent completely.
            // USBC will read from new_rpos directly, so the buffer [new_rpos, new_rpos + info->next_size) must be protected too.
            rpos_boundary = new_rpos + info->next_size;
            if (rpos_boundary >= usb_recv_size) {
                rpos_boundary -= usb_recv_size;
            }
        }

        if (old_rpos <= wpos) {
            if (new_rpos < old_rpos || wpos < new_rpos) {
                conflicted = 1;
            }
        } else {
            if (wpos < new_rpos && new_rpos < old_rpos) {
                conflicted = 1;
            }
        }

        if (conflicted) {
            uint32_t reset_len = usb_send_size / 2;
            uint32_t saved_old_rpos = old_rpos;

            // Reset read position
            old_rpos = wpos + reset_len;
            if (old_rpos >= usb_send_size) {
                old_rpos -= usb_send_size;
            }
            new_rpos = old_rpos + size;
            if (new_rpos >= usb_send_size) {
                new_rpos -= usb_send_size;
            }

#if 0
            usb_audio_stop_send();
            usb_audio_start_send(usb_send_buf, new_rpos, usb_send_size);
#else
            usb_audio_set_send_pos(new_rpos);
#endif

            TRACE(4,"send: Error: rpos=%u goes beyond wpos=%u with len=%u. Reset to %u", saved_old_rpos, wpos, size, old_rpos);
        }
        record_conflict(conflicted);
    }

    usb_send_rpos = new_rpos;

#if (VERBOSE_TRACE & (1 << 6))
    TRACE_TIME(3,"send: rpos=%u size=%u wpos=%u", old_rpos, size, wpos);
#endif

    if (conflicted) {
        usb_audio_sync_reset(&capture_info);
        return;
    }

#ifndef UAUD_SYNC_STREAM_TARGET
    // Recv takes precedence
    if (recv_state == AUDIO_ITF_STATE_STARTED) {
        return;
    }
#endif

    if (usb_send_buf <= data && data <= usb_send_buf + usb_send_size) {
        send_samp = byte_to_samp_send(new_rpos);
    } else {
        send_samp = -1;
    }

    if ((int)cap_pos >= 0) {
        if (cap_pos >= capture_size) {
            cap_pos = 0;
        }
        // Count the bytes of data newly captured in the codec buffer
        uint32_t bytes_cap;
        if (old_cap_pos <= cap_pos) {
            bytes_cap = cap_pos - old_cap_pos;
        } else {
            bytes_cap = capture_size + cap_pos - old_cap_pos;
        }
        uint32_t usb_bytes_cap;
        usb_bytes_cap = capture_to_send_len(bytes_cap);
        cap_pos = wpos + usb_bytes_cap;
        if (cap_pos >= usb_send_size) {
            cap_pos -= usb_send_size;
        }
        cap_samp = byte_to_samp_send(cap_pos);
    } else {
        cap_samp = -1;
    }

    if (send_samp != -1 && cap_samp != -1) {
        enum UAUD_SYNC_RET_T ret;
        float ratio;

        ret = usb_audio_sync(cap_samp, send_samp, &capture_info, &ratio);
        if (ret == UAUD_SYNC_START) {
            usb_audio_tune_rate(AUD_STREAM_CAPTURE, ratio);
        }
    } else {
        //TRACE(2,"send_hdlr: send_samp=0x%08x cap_samp=0x%08x", send_samp, cap_samp);
    }
}

#ifdef USB_AUDIO_MULTIFUNC
static float playback_gain_to_float(uint32_t level)
{
    int32_t db;

    if (level > MAX_VOLUME_VAL || level < MIN_VOLUME_VAL) {
        return 0;
    }

    db = codec_dac_vol[level].sdac_volume;
    if (db <= USB_AUDIO_MIN_DBVAL) {
        return 0;
    }

    return db_to_float(db);
}

static void usb_audio_playback2_start(enum USB_AUDIO_ITF_CMD_T cmd)
{
    if (cmd == USB_AUDIO_ITF_STOP) {
        // Stop the stream right now
        if (recv2_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_recv2();
            if (recv_state == AUDIO_ITF_STATE_STOPPED && send_state == AUDIO_ITF_STATE_STOPPED) {
                usb_audio_sync_reset(&playback_info);
            }
            recv2_state = AUDIO_ITF_STATE_STOPPED;
        }

        if (recv_state == AUDIO_ITF_STATE_STARTED) {
            return;
        }

        enqueue_unique_cmd_with_opp(AUDIO_CMD_STOP_PLAY, usb_recv_seq, 0, AUDIO_CMD_START_PLAY);
    } else {
        if (recv2_state == AUDIO_ITF_STATE_STOPPED) {
            int ret;

            reset_conflict();

            if (recv_state == AUDIO_ITF_STATE_STOPPED && send_state == AUDIO_ITF_STATE_STOPPED) {
                usb_audio_sync_reset(&playback_info);
            }

            usb_recv2_valid = 0;
            if (recv_state == AUDIO_ITF_STATE_STOPPED) {
                usb_recv_seq++;
            }

            usb_recv2_rpos = 0;
            usb_recv2_wpos = usb_recv_size / 2;
            zero_mem32(usb_recv2_buf, usb_recv2_size);

            usb_recv2_err_cnt = 0;

            ret = usb_audio_start_recv2(usb_recv2_buf, usb_recv2_wpos, usb_recv2_size);
            if (ret == 1) {
                TRACE(0,"usb_audio_start_recv2 failed: usb not configured");
            } else {
                ASSERT(ret == 0, "usb_audio_start_recv2 failed: %d", ret);
            }
            recv2_state = AUDIO_ITF_STATE_STARTED;
        }

        if (recv_state == AUDIO_ITF_STATE_STARTED) {
            return;
        }

        enqueue_unique_cmd_with_opp(AUDIO_CMD_START_PLAY, usb_recv_seq, 0, AUDIO_CMD_STOP_PLAY);
    }
}

static void usb_audio_mute2_control(uint32_t mute)
{
    TRACE(1,"MUTE2 CTRL: %u", mute);

    new_mute2_state = mute;
    if (mute) {
        new_playback2_coef = 0;
    } else {
        new_playback2_coef = playback_gain_to_float(new_playback2_vol);
    }
    usb_audio_enqueue_cmd(AUDIO_CMD_MUTE_CTRL);
}

static void usb_audio_vol2_control(uint32_t percent)
{
    if (percent >= 100) {
        new_playback2_vol = MAX_VOLUME_VAL;
    } else {
        new_playback2_vol = MIN_VOLUME_VAL + (percent * (MAX_VOLUME_VAL - MIN_VOLUME_VAL) + 50) / 100;
    }

    new_playback2_coef = playback_gain_to_float(new_playback2_vol);

    TRACE(2,"VOL2 CTRL: percent=%u new_playback_vol=%u", percent, new_playback2_vol);
}

static uint32_t usb_audio_get_vol2_percent(void)
{
    if (new_playback2_vol >= MAX_VOLUME_VAL) {
        return 100;
    } else {
        return ((new_playback2_vol - MIN_VOLUME_VAL) * 100 + (MAX_VOLUME_VAL - MIN_VOLUME_VAL) / 2)
            / (MAX_VOLUME_VAL - MIN_VOLUME_VAL);
    }
}

static void usb_audio_data_recv2_handler(const struct USB_AUDIO_XFER_INFO_T *info)
{
    const uint8_t *data;
    uint32_t size;
    uint32_t old_wpos, new_wpos, wpos_boundary, rpos;
    uint32_t old_play_pos = 0;
    uint32_t play_pos;
    uint32_t lock;
    int conflicted;
    uint32_t recv_samp, play_samp;
    uint32_t cur_time;

    cur_time = hal_sys_timer_get();
    last_usb_recv_time = cur_time;

    data = info->data;
    size = info->size;

    if (info->cur_compl_err || info->next_xfer_err) {
        if (usb_recv2_err_cnt == 0) {
            usb_recv2_err_cnt++;
            usb_recv2_ok_cnt = 0;
            last_usb_recv2_err_time = cur_time;
        } else {
            usb_recv2_err_cnt++;
        }
    } else {
        if (usb_recv2_err_cnt) {
            usb_recv2_ok_cnt++;
        }
    }
    if (usb_recv2_err_cnt && cur_time - last_usb_recv2_err_time >= USB_XFER_ERR_REPORT_INTERVAL) {
        if (info->cur_compl_err || info->next_xfer_err) {
            TRACE(2,"recv2: ERROR: cur_err=%d next_err=%d", info->cur_compl_err, info->next_xfer_err);
        }
        TRACE(3,"recv2: ERROR-CNT: err=%u ok=%u in %u ms", usb_recv2_err_cnt, usb_recv2_ok_cnt, TICKS_TO_MS(USB_XFER_ERR_REPORT_INTERVAL));
        usb_recv2_err_cnt = 0;
    }

    if (usb_recv2_buf <= data && data <= usb_recv2_buf + usb_recv2_size) {
        if (data != usb_recv2_buf + usb_recv2_wpos) {
            TRACE(3,"recv2: WARNING: Invalid wpos=0x%x data=%p recv2_buf=%p. IRQ missing?", usb_recv2_wpos, data, usb_recv2_buf);
            usb_recv2_wpos = data - usb_recv2_buf;
        }
    }

    old_wpos = usb_recv2_wpos;
    new_wpos = old_wpos + size;
    if (new_wpos >= usb_recv2_size) {
        new_wpos -= usb_recv2_size;
    }

    if (codec_config_lock ||
            codec_play_seq != usb_recv_seq ||
            codec_play_valid == 0 ||
            playback_state == AUDIO_ITF_STATE_STOPPED ||
            recv2_state == AUDIO_ITF_STATE_STOPPED ||
            size == 0 || // rx paused
            0) {
        usb_recv2_valid = 0;
    } else {
        if (info->cur_compl_err == 0) {
            usb_recv2_valid = 1;
        }
    }

    if (usb_recv2_valid == 0 || usb_recv2_init_rpos == 0) {
        usb_recv2_wpos = new_wpos;
        return;
    }

    conflicted = 0;

    lock = int_lock();
    if (playback_conflicted2) {
        playback_conflicted2 = 0;
        rpos = 0; // Avoid compiler warnings
        conflicted = 1;
    } else {
        rpos = usb_recv2_rpos;
        old_play_pos = playback_pos;
        play_pos = af_stream_get_cur_dma_pos(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
    int_unlock(lock);

    if (conflicted == 0) {
        if (info->pool_enabled) {
            // USBC will write to memory pool.
            wpos_boundary = new_wpos;
        } else {
            // USBC will write to new_wpos directly, so the buffer [new_wpos, new_wpos + info->next_size) must be protected too.
            wpos_boundary = new_wpos + info->next_size;
            if (wpos_boundary >= usb_recv2_size) {
                wpos_boundary -= usb_recv2_size;
            }
        }

        if (old_wpos <= rpos) {
            if (wpos_boundary < old_wpos || rpos < wpos_boundary) {
                conflicted = 1;
            }
        } else {
            if (rpos < wpos_boundary && wpos_boundary < old_wpos) {
                conflicted = 1;
            }
        }

        if (conflicted) {
            uint32_t reset_len = usb_recv2_size / 2;
            uint32_t saved_old_wpos = old_wpos;

            // Reset read position
            old_wpos = rpos + reset_len;
            if (old_wpos >= usb_recv2_size) {
                old_wpos -= usb_recv2_size;
            }
            new_wpos = old_wpos + size;
            if (new_wpos >= usb_recv2_size) {
                new_wpos -= usb_recv2_size;
            }

#if 0
            usb_audio_stop_recv();
            usb_audio_start_recv(usb_recv2_buf, new_wpos, usb_recv2_size);
#else
            usb_audio_set_recv2_pos(new_wpos);
#endif

            TRACE(4,"recv2: Error: wpos=%u goes beyond rpos=%u with len=%u. Reset to %u", saved_old_wpos, rpos, size, old_wpos);
        }
        record_conflict(conflicted);
    }

    usb_recv2_wpos = new_wpos;

#if (VERBOSE_TRACE & (1 << 5))
    {
        int hw_play_pos = af_stream_get_cur_dma_pos(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        TRACE_TIME(5,"recv2: wpos=%u size=%u rpos=%u playback_pos=%u hw_play_pos=%d", old_wpos, size,
            rpos, playback_pos, hw_play_pos);
    }
#endif

    if (conflicted) {
        usb_audio_sync_reset(&playback_info);
        return;
    }

    // Recv and send takes precedence
    if (recv_state == AUDIO_ITF_STATE_STARTED || send_state == AUDIO_ITF_STATE_STARTED) {
        return;
    }

    if (usb_recv2_buf <= data && data <= usb_recv2_buf + usb_recv2_size) {
        recv_samp = byte_to_samp_recv(new_wpos);
    } else {
        recv_samp = -1;
    }

    if ((int)play_pos >= 0) {
        if (play_pos >= playback_size) {
            play_pos = 0;
        }
        // Count the bytes of data waiting to play in the codec buffer
        uint32_t bytes_to_play;
        if (old_play_pos <= play_pos) {
            bytes_to_play = playback_size + old_play_pos - play_pos;
        } else {
            bytes_to_play = old_play_pos - play_pos;
        }
        uint32_t usb_bytes_to_play;
        usb_bytes_to_play = playback_to_recv_len(bytes_to_play);
        if (rpos >= usb_bytes_to_play) {
            play_pos = rpos - usb_bytes_to_play;
        } else {
            play_pos = usb_recv2_size + rpos - usb_bytes_to_play;
        }
        play_samp = byte_to_samp_recv(play_pos);
    } else {
        play_samp = -1;
    }

    if (recv_samp != -1 && play_samp != -1) {
        enum UAUD_SYNC_RET_T ret;
        float ratio;

        ret = usb_audio_sync(play_samp, recv_samp, &playback_info, &ratio);
        if (ret == UAUD_SYNC_START) {
            usb_audio_tune_rate(AUD_STREAM_PLAYBACK, ratio);
        }
    } else {
        //TRACE(2,"recv_hdlr: recv_samp=0x%08x play_samp=0x%08x", recv_samp, play_samp);
    }
}
#endif

static void usb_audio_reset_usb_stream_state(bool init)
{
    if (init) {
        recv_state = AUDIO_ITF_STATE_STOPPED;
        send_state = AUDIO_ITF_STATE_STOPPED;

#ifdef USB_AUDIO_DYN_CFG
        // Reset itf setting
        playback_itf_set = 0;
        capture_itf_set = 0;
#endif

        // Reset mute and volume setting
        new_mute_state = 0;
        new_cap_mute_state = 0;

        new_playback_vol = AUDIO_OUTPUT_VOLUME_DEFAULT;
        if (new_playback_vol > MAX_VOLUME_VAL) {
            new_playback_vol = MAX_VOLUME_VAL;
        } else if (new_playback_vol < MIN_VOLUME_VAL) {
            new_playback_vol = MIN_VOLUME_VAL;
        }

        new_capture_vol = CODEC_SADC_VOL;
        if (new_capture_vol > MAX_CAP_VOLUME_VAL) {
            new_capture_vol = MAX_CAP_VOLUME_VAL;
        } else if (new_capture_vol < MIN_CAP_VOLUME_VAL) {
            new_capture_vol = MIN_CAP_VOLUME_VAL;
        }

#ifdef USB_AUDIO_MULTIFUNC
        new_playback_coef = playback_gain_to_float(new_playback_vol);
        playback_coef = new_playback_coef;

        recv2_state = AUDIO_ITF_STATE_STOPPED;
        new_mute2_state = 0;
        new_playback2_vol = AUDIO_OUTPUT_VOLUME_DEFAULT;
        if (new_playback2_vol > MAX_VOLUME_VAL) {
            new_playback2_vol = MAX_VOLUME_VAL;
        } else if (new_playback2_vol < MIN_VOLUME_VAL) {
            new_playback2_vol = MIN_VOLUME_VAL;
        }
        new_playback2_coef = playback_gain_to_float(new_playback2_vol);
        playback2_coef = new_playback2_coef;
#endif
    } else {
        if (recv_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_usb_stream(AUD_STREAM_PLAYBACK);
            recv_state = AUDIO_ITF_STATE_STOPPED;
        }
        if (send_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_usb_stream(AUD_STREAM_CAPTURE);
            send_state = AUDIO_ITF_STATE_STOPPED;
        }

#ifdef USB_AUDIO_MULTIFUNC
        if (recv2_state == AUDIO_ITF_STATE_STARTED) {
            usb_audio_stop_recv2();
            if (recv_state == AUDIO_ITF_STATE_STOPPED && send_state == AUDIO_ITF_STATE_STOPPED) {
                usb_audio_sync_reset(&playback_info);
            }
            recv2_state = AUDIO_ITF_STATE_STOPPED;
        }
#endif

        // Keep old itf setting unless it is in init mode

        // Keep old mute and playback volume setting unless it is in init mode
    }
}

static void usb_audio_reset_codec_stream_state(bool init)
{
    playback_state = AUDIO_ITF_STATE_STOPPED;
    capture_state = AUDIO_ITF_STATE_STOPPED;

#ifndef USB_AUDIO_MULTIFUNC
    playback_vol = new_playback_vol;
#endif
    capture_vol = new_capture_vol;

    playback_paused = 0;

    playback_conflicted = 0;
#ifdef USB_AUDIO_MULTIFUNC
    playback_conflicted2 = 0;
#endif
    capture_conflicted = 0;

    // Keep old mute setting unless it is in init mode
    if (init) {
#ifdef PERF_TEST_POWER_KEY
        perft_power_type = 0;
        af_codec_set_perf_test_power(perft_power_type);
#endif
#ifdef PA_ON_OFF_KEY
        if (pa_on_off_muted) {
            pa_on_off_muted = false;
            if (mute_user_map == 0) {
                usb_audio_codec_unmute(CODEC_MUTE_USER_ALL);
            }
        }
#endif
        if (mute_user_map) {
            mute_user_map = 0;
            usb_audio_codec_unmute(CODEC_MUTE_USER_ALL);
        }
#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
        restore_noise_reduction_status();
#endif
    }
}

#ifdef USB_AUDIO_DYN_CFG
static void usb_audio_set_recv_rate(enum AUD_SAMPRATE_T rate)
{
#ifndef USB_AUDIO_UAC2
    if (playback_itf_set == 0) {
        TRACE(1,"\nWARNING: Set recv rate while itf not set: %d\n", rate);
        return;
    }
#endif

    // Some host applications will not stop current stream before changing sample rate
    if (recv_state == AUDIO_ITF_STATE_STARTED) {
        usb_audio_stop_usb_stream(AUD_STREAM_PLAYBACK);
        recv_state = AUDIO_ITF_STATE_STOPPED;
    }

    TRACE(3,"%s: Change recv sample rate from %u to %u", __FUNCTION__, sample_rate_recv, rate);

    new_sample_rate_recv = rate;

#ifdef KEEP_SAME_LATENCY
    usb_recv_size = calc_usb_recv_size(new_sample_rate_recv, new_sample_size_recv);
    TRACE(2,"%s: Set usb_recv_size=%u", __FUNCTION__, usb_recv_size);
#endif

    if (playback_itf_set) {
        usb_audio_start_usb_stream(AUD_STREAM_PLAYBACK);
        recv_state = AUDIO_ITF_STATE_STARTED;
    }

#ifdef USB_AUDIO_UAC2
    enqueue_unique_cmd_arg(AUDIO_CMD_SET_RECV_RATE, usb_recv_seq, 0);
#else
    enqueue_unique_cmd_with_opp(AUDIO_CMD_SET_RECV_RATE, usb_recv_seq, 0, AUDIO_CMD_STOP_PLAY);
#endif
}

static void usb_audio_set_send_rate(enum AUD_SAMPRATE_T rate)
{
#ifndef USB_AUDIO_UAC2
    if (capture_itf_set == 0) {
        TRACE(1,"\nWARNING: Set send rate while itf not set: %d\n", rate);
        return;
    }
#endif

    // Some host applications will not stop current stream before changing sample rate
    if (send_state == AUDIO_ITF_STATE_STARTED) {
        usb_audio_stop_usb_stream(AUD_STREAM_CAPTURE);
        send_state = AUDIO_ITF_STATE_STOPPED;
    }

    TRACE(3,"%s: Change send sample rate from %u to %u", __FUNCTION__, sample_rate_send, rate);

    new_sample_rate_send = rate;

#ifdef KEEP_SAME_LATENCY
    usb_send_size = calc_usb_send_size(new_sample_rate_send);
    TRACE(2,"%s: Set usb_send_size=%u", __FUNCTION__, usb_send_size);
#endif

    if (capture_itf_set) {
        usb_audio_start_usb_stream(AUD_STREAM_CAPTURE);
        send_state = AUDIO_ITF_STATE_STARTED;
    }

#ifdef USB_AUDIO_UAC2
    enqueue_unique_cmd_arg(AUDIO_CMD_SET_SEND_RATE, usb_send_seq, 0);
#else
    enqueue_unique_cmd_with_opp(AUDIO_CMD_SET_SEND_RATE, usb_send_seq, 0, AUDIO_CMD_STOP_CAPTURE);
#endif
}
#endif

static void usb_audio_state_control(enum USB_AUDIO_STATE_EVENT_T event, uint32_t param)
{
    TRACE(3,"%s: %d (%u)", __FUNCTION__, event, param);

    if (event == USB_AUDIO_STATE_RESET || event == USB_AUDIO_STATE_DISCONNECT || event == USB_AUDIO_STATE_CONFIG) {
        usb_audio_reset_usb_stream_state(true);
    } else if (event == USB_AUDIO_STATE_SLEEP || event == USB_AUDIO_STATE_WAKEUP) {
        usb_audio_reset_usb_stream_state(false);
    }

    if (event == USB_AUDIO_STATE_RESET) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_RESET);
    } else if (event == USB_AUDIO_STATE_DISCONNECT) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_DISCONNECT);
    } else if (event == USB_AUDIO_STATE_CONFIG) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_CONFIG);
    } else if (event == USB_AUDIO_STATE_SLEEP) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_SLEEP);
    } else if (event == USB_AUDIO_STATE_WAKEUP) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_WAKEUP);
    } else if (event == USB_AUDIO_STATE_RECV_PAUSE || event == USB_AUDIO_STATE_RECV2_PAUSE) {
#ifndef USB_AUDIO_MULTIFUNC
        enqueue_unique_cmd_with_opp(AUDIO_CMD_RECV_PAUSE, usb_recv_seq, 0, AUDIO_CMD_RECV_CONTINUE);
#endif
    } else if (event == USB_AUDIO_STATE_RECV_CONTINUE || event == USB_AUDIO_STATE_RECV2_CONTINUE) {
#ifndef USB_AUDIO_MULTIFUNC
        enqueue_unique_cmd_with_opp(AUDIO_CMD_RECV_CONTINUE, usb_recv_seq, 0, AUDIO_CMD_RECV_PAUSE);
#endif
#ifdef USB_AUDIO_DYN_CFG
    } else if (event == USB_AUDIO_STATE_SET_RECV_RATE) {
        usb_audio_set_recv_rate(param);
    } else if (event == USB_AUDIO_STATE_SET_SEND_RATE) {
        usb_audio_set_send_rate(param);
#endif
    } else {
        ASSERT(false, "Bad state event");
    }
}

static void usb_audio_acquire_freq(void)
{
    enum HAL_CMU_FREQ_T freq;

#if defined(CHIP_BEST1000) && defined(_DUAL_AUX_MIC_)
#ifdef SW_CAPTURE_RESAMPLE
    if (resample_cap_enabled) {
        freq = HAL_CMU_FREQ_104M;
    } else
#endif
    if (capture_state == AUDIO_ITF_STATE_STARTED) {
#ifdef DUAL_AUX_MIC_MORE_FILTER
        freq = HAL_CMU_FREQ_104M;
#else
        freq = HAL_CMU_FREQ_78M;
#endif
    } else
#else // !(CHIP_BEST1000 && _DUAL_AUX_MIC_)
#ifdef SW_CAPTURE_RESAMPLE
    if (resample_cap_enabled) {
#ifdef CHIP_BEST1000
        freq = HAL_CMU_FREQ_78M;
#else
        freq = HAL_CMU_FREQ_52M;
#endif
    } else
#endif
#endif // !(CHIP_BEST1000 && _DUAL_AUX_MIC_)
    {
        freq = HAL_CMU_FREQ_52M;
    }

#if defined(USB_HIGH_SPEED) && defined(USB_AUDIO_UAC2)
    if (playback_state == AUDIO_ITF_STATE_STARTED && sample_rate_play >= AUD_SAMPRATE_352800) {
        if (freq < HAL_CMU_FREQ_104M) {
            freq = HAL_CMU_FREQ_104M;
        }
    }
#endif

#ifdef AUDIO_ANC_FB_MC
     if (freq < HAL_CMU_FREQ_104M) {
        freq = HAL_CMU_FREQ_104M;
     }
#endif

#ifdef __HW_FIR_DSD_PROCESS__
     if (freq < HAL_CMU_FREQ_104M) {
        freq = HAL_CMU_FREQ_104M;
     }
#endif

#ifdef USB_AUDIO_SPEECH
    enum HAL_CMU_FREQ_T speech_freq;

    speech_freq = speech_process_need_freq();
    if (freq < speech_freq) {
        freq = speech_freq;
    }
#endif

    hal_sysfreq_req(HAL_SYSFREQ_USER_APP_2, freq);
    //TRACE(2,"[%s] app_sysfreq_req %d", __FUNCTION__, freq);
    //TRACE(2,"[%s] sys freq calc : %d\n", __FUNCTION__, hal_sys_timer_calc_cpu_freq(5, 0));
}

static void usb_audio_release_freq(void)
{
    hal_sysfreq_req(HAL_SYSFREQ_USER_APP_2, HAL_CMU_FREQ_32K);
}

static void usb_audio_update_freq(void)
{
    usb_audio_acquire_freq();
}

#ifdef SW_CAPTURE_RESAMPLE

static enum AUD_CHANNEL_NUM_T get_capture_resample_chan_num(void)
{
#if (CHAN_NUM_CAPTURE == CHAN_NUM_SEND)
    return chan_num_to_enum(CHAN_NUM_CAPTURE);
#else
    return AUD_CHANNEL_NUM_1;
#endif
}

static void capture_stream_resample_config(void)
{
    struct RESAMPLE_CFG_T cfg;
    enum RESAMPLE_STATUS_T ret;
    enum AUD_SAMPRATE_T cap_rate;

#if defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_))
    cap_rate = sample_rate_ref_cap;
#else
    cap_rate = sample_rate_cap;
#endif

#ifndef __AUDIO_RESAMPLE__
    if (cap_rate == sample_rate_send) {
        if (resample_cap_enabled) {
            TRACE(1,"!!! %s: Disable capture resample", __FUNCTION__);

            audio_resample_ex_close(resample_id);
            resample_cap_enabled = false;
        }
    } else
#endif
    {
        if (!resample_cap_enabled) {
            // Resample chan num is usb send chan num
            memset(&cfg, 0, sizeof(cfg));
            cfg.chans = get_capture_resample_chan_num();
            cfg.bits = sample_size_to_enum_capture(sample_size_cap);
#ifdef __AUDIO_RESAMPLE__
            if (sample_rate_send % AUD_SAMPRATE_8000) {
                ASSERT(hal_cmu_get_crystal_freq() / (CODEC_FREQ_44_1K_SERIES / sample_rate_send) == cap_rate,
                    "%s: Bad cap_rate=%u with sample_rate_send=%u", __FUNCTION__, cap_rate, sample_rate_send);
#if (CODEC_FREQ_26M / CODEC_FREQ_CRYSTAL * CODEC_FREQ_CRYSTAL == CODEC_FREQ_26M)
                cfg.coef = &resample_coef_50p7k_to_44p1k;
#else
                cfg.coef = &resample_coef_46p8k_to_44p1k;
#endif
            } else {
                ASSERT(hal_cmu_get_crystal_freq() / (CODEC_FREQ_48K_SERIES / sample_rate_send) == cap_rate,
                    "%s: Bad cap_rate=%u with sample_rate_send=%u", __FUNCTION__, cap_rate, sample_rate_send);
#if (CODEC_FREQ_26M / CODEC_FREQ_CRYSTAL * CODEC_FREQ_CRYSTAL == CODEC_FREQ_26M)
                cfg.coef = &resample_coef_50p7k_to_48k;
#else
                cfg.coef = &resample_coef_46p8k_to_48k;
#endif
            }
#else
            if (sample_rate_send % AUD_SAMPRATE_8000) {
                ASSERT(CODEC_FREQ_48K_SERIES / (CODEC_FREQ_44_1K_SERIES / sample_rate_send) == cap_rate,
                    "%s: Bad cap_rate=%u with sample_rate_send=%u", __FUNCTION__, cap_rate, sample_rate_send);
                cfg.coef = &resample_coef_48k_to_44p1k;
            } else {
                ASSERT(CODEC_FREQ_44_1K_SERIES / (CODEC_FREQ_48K_SERIES / sample_rate_send) == cap_rate,
                    "%s: Bad cap_rate=%u with sample_rate_send=%u", __FUNCTION__, cap_rate, sample_rate_send);
                cfg.coef = &resample_coef_44p1k_to_48k;
            }
#endif
            cfg.buf = resample_history_buf;
            cfg.size = resample_history_size;

            TRACE(3,"!!! %s: Enable capture resample %u => %u", __FUNCTION__, cap_rate, sample_rate_send);

            ret = audio_resample_ex_open(&cfg, &resample_id);
            ASSERT(ret == RESAMPLE_STATUS_OK, "%s: Failed to init resample: %d", __FUNCTION__, ret);

            resample_cap_enabled = true;
        }
    }
}

#endif

static void usb_audio_init_streams(enum AUDIO_STREAM_REQ_USER_T user)
{
    bool action;

    action = false;
#ifndef DELAY_STREAM_OPEN
    if (user == AUDIO_STREAM_REQ_USB) {
        action = true;
    }
#endif
#ifdef ANC_L_R_MISALIGN_WORKAROUND
    if (user == AUDIO_STREAM_REQ_ANC) {
        action = true;
    }
#endif
    if (action)
    {
        usb_audio_open_codec_stream(AUD_STREAM_PLAYBACK, user);
        usb_audio_open_codec_stream(AUD_STREAM_CAPTURE, user);
    }

#ifdef ANC_L_R_MISALIGN_WORKAROUND
    if (user == AUDIO_STREAM_REQ_ANC) {
        usb_audio_start_codec_stream(AUD_STREAM_PLAYBACK, user);
        usb_audio_start_codec_stream(AUD_STREAM_CAPTURE, user);
    }
#endif
}

static void usb_audio_term_streams(enum AUDIO_STREAM_REQ_USER_T user)
{
    // Ensure all the streams for this user are stopped and closed
    usb_audio_stop_codec_stream(AUD_STREAM_CAPTURE, user);
    usb_audio_stop_codec_stream(AUD_STREAM_PLAYBACK, user);

    usb_audio_close_codec_stream(AUD_STREAM_CAPTURE, user);
    usb_audio_close_codec_stream(AUD_STREAM_PLAYBACK, user);
}

#if defined(ANDROID_ACCESSORY_SPEC) || defined(CFG_MIC_KEY)
static void hid_data_send_handler(enum USB_AUDIO_HID_EVENT_T event, int error)
{
    TRACE(2,"HID SENT: event=0x%04x error=%d", event, error);
}
#endif

static void usb_audio_itf_callback(enum USB_AUDIO_ITF_ID_T id, enum USB_AUDIO_ITF_CMD_T cmd)
{
    if (0) {
#ifdef USB_AUDIO_MULTIFUNC
    } else if (id == USB_AUDIO_ITF_ID_RECV2) {
        usb_audio_playback2_start(cmd);
#endif
    } else if (id == USB_AUDIO_ITF_ID_RECV) {
        usb_audio_playback_start(cmd);
    } else {
        usb_audio_capture_start(cmd);
    }
}

static void usb_audio_mute_callback(enum USB_AUDIO_ITF_ID_T id, uint32_t mute)
{
    if (0) {
#ifdef USB_AUDIO_MULTIFUNC
    } else if (id == USB_AUDIO_ITF_ID_RECV2) {
        usb_audio_mute2_control(mute);
#endif
    } else if (id == USB_AUDIO_ITF_ID_RECV) {
        usb_audio_mute_control(mute);
    } else {
        usb_audio_cap_mute_control(mute);
    }
}

static void usb_audio_set_volume(enum USB_AUDIO_ITF_ID_T id, uint32_t percent)
{
    if (0) {
#ifdef USB_AUDIO_MULTIFUNC
    } else if (id == USB_AUDIO_ITF_ID_RECV2) {
        usb_audio_vol2_control(percent);
#endif
    } else if (id == USB_AUDIO_ITF_ID_RECV) {
        usb_audio_vol_control(percent);
    } else {
        usb_audio_cap_vol_control(percent);
    }
}

static uint32_t usb_audio_get_volume(enum USB_AUDIO_ITF_ID_T id)
{
    uint32_t percent = 0;

    if (0) {
#ifdef USB_AUDIO_MULTIFUNC
    } else if (id == USB_AUDIO_ITF_ID_RECV2) {
        percent = usb_audio_get_vol2_percent();
#endif
    } else if (id == USB_AUDIO_ITF_ID_RECV) {
        percent = usb_audio_get_vol_percent();
    } else {
        percent = usb_audio_get_cap_vol_percent();
    }

    return percent;
}

static void usb_audio_xfer_callback(enum USB_AUDIO_ITF_ID_T id, const struct USB_AUDIO_XFER_INFO_T *info)
{
    if (0) {
#ifdef USB_AUDIO_MULTIFUNC
    } else if (id == USB_AUDIO_ITF_ID_RECV2) {
        usb_audio_data_recv2_handler(info);
#endif
    } else if (id == USB_AUDIO_ITF_ID_RECV) {
        usb_audio_data_recv_handler(info);
    } else {
        usb_audio_data_send_handler(info);
    }
}

static void usb_audio_stream_running_handler(void)
{
    enum AUDIO_STREAM_RUNNING_T req;
    uint32_t lock;

    req = streams_running_req;

    if (req == AUDIO_STREAM_RUNNING_NULL) {
        return;
    } else if (req == AUDIO_STREAM_RUNNING_ENABLED) {
        usb_audio_init_streams(AUDIO_STREAM_REQ_ANC);
    } else if (req == AUDIO_STREAM_RUNNING_DISABLED) {
        usb_audio_term_streams(AUDIO_STREAM_REQ_ANC);
    }

    lock = int_lock();
    if (req == streams_running_req) {
        streams_running_req = AUDIO_STREAM_RUNNING_NULL;
    }
    int_unlock(lock);
}

void usb_audio_keep_streams_running(bool enable)
{
    if (enable) {
        streams_running_req = AUDIO_STREAM_RUNNING_ENABLED;
    } else {
        streams_running_req = AUDIO_STREAM_RUNNING_DISABLED;
    }

    // TODO: Move usb_audio_stream_running_handler() into usb_audio_app_loop() ?
    //       How to ensure that streams are opened before ANC is enabled?
    usb_audio_stream_running_handler();
}

#ifdef USB_AUDIO_DYN_CFG

static enum AUD_SAMPRATE_T calc_play_sample_rate(enum AUD_SAMPRATE_T usb_rate)
{
    enum AUD_SAMPRATE_T codec_rate;

#if defined(CHIP_BEST1000) && defined(ANC_APP)

    codec_rate = (usb_rate % AUD_SAMPRATE_8000) ? AUD_SAMPRATE_88200 : AUD_SAMPRATE_96000;

#else

    codec_rate = usb_rate;

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
    if (codec_rate % AUD_SAMPRATE_8000) {
        codec_rate = hal_cmu_get_crystal_freq() / (CODEC_FREQ_44_1K_SERIES / codec_rate);
    } else {
        codec_rate = hal_cmu_get_crystal_freq() / (CODEC_FREQ_48K_SERIES / codec_rate);
    }
#endif

#endif

    return codec_rate;
}

#if defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_))
static enum AUD_SAMPRATE_T calc_anc_cap_sample_rate_best1000(enum AUD_SAMPRATE_T usb_rate, enum AUD_SAMPRATE_T play_rate)
{
    enum AUD_SAMPRATE_T rate_cap;

#ifdef ANC_APP

#ifdef AUD_PLL_DOUBLE
#if defined(_DUAL_AUX_MIC_) || defined(CAPTURE_ANC_DATA)
    rate_cap = play_rate * 8;
#else
    rate_cap = play_rate * 2;
#endif
#else // !AUD_PLL_DOUBLE
#if defined(_DUAL_AUX_MIC_) || defined(CAPTURE_ANC_DATA)
    rate_cap = play_rate * 4;
#else
    rate_cap = play_rate;
#endif
#endif // !AUD_PLL_DOUBLE

#else // _DUAL_AUX_MIC_

    // Capture reference
    enum AUD_SAMPRATE_T rate_cap_ref;

    rate_cap_ref = (usb_rate % AUD_SAMPRATE_8000) ? AUD_SAMPRATE_44100 : AUD_SAMPRATE_48000;

    rate_cap = rate_cap_ref * 4;

#endif

    return rate_cap;
}
#endif

static enum AUD_SAMPRATE_T calc_cap_sample_rate(enum AUD_SAMPRATE_T usb_rate, enum AUD_SAMPRATE_T play_rate)
{
#if defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_))

    return calc_anc_cap_sample_rate_best1000(usb_rate, play_rate);

#else // !(CHIP_BEST1000 && (ANC_APP || _DUAL_AUX_MIC_))

    enum AUD_SAMPRATE_T codec_rate;

#ifdef __AUDIO_RESAMPLE__

    codec_rate = usb_rate;

#ifdef SW_CAPTURE_RESAMPLE
    if (codec_rate % AUD_SAMPRATE_8000) {
        codec_rate = hal_cmu_get_crystal_freq() / (CODEC_FREQ_44_1K_SERIES / codec_rate);
    } else {
        codec_rate = hal_cmu_get_crystal_freq() / (CODEC_FREQ_48K_SERIES / codec_rate);
    }
#endif

#else // ! __AUDIO_RESAMPLE__

    bool usb_rate_44p1k;
    bool play_rate_44p1k;

    usb_rate_44p1k = !!(usb_rate % AUD_SAMPRATE_8000);
    play_rate_44p1k = !!(play_rate % AUD_SAMPRATE_8000);

    if (usb_rate_44p1k ^ play_rate_44p1k) {
        // Playback pll is NOT compatible with capture sample rate
        if (usb_rate_44p1k) {
            codec_rate = CODEC_FREQ_48K_SERIES / (CODEC_FREQ_44_1K_SERIES / usb_rate);
        } else {
            codec_rate = CODEC_FREQ_44_1K_SERIES / (CODEC_FREQ_48K_SERIES / usb_rate);
        }
    } else {
        // Playback pll is compatible with capture sample rate
        codec_rate = usb_rate;
    }

#endif // !__AUDIO_RESAMPLE__

    return codec_rate;

#endif // !(CHIP_BEST1000 && (ANC_APP || _DUAL_AUX_MIC_))
}

#endif // USB_AUDIO_DYN_CFG

static void POSSIBLY_UNUSED usb_audio_update_codec_stream(enum AUD_STREAM_T stream)
{
    bool update_play;
    bool update_cap;
#ifdef STREAM_RATE_BITS_SETUP
    struct AF_STREAM_CONFIG_T *cfg, stream_cfg;
    uint32_t ret;
#endif

    update_play = false;
    update_cap = false;

#ifdef USB_AUDIO_DYN_CFG
    enum AUD_SAMPRATE_T new_recv_rate;
    enum AUD_SAMPRATE_T new_send_rate;
    enum AUD_SAMPRATE_T play_rate;
    enum AUD_SAMPRATE_T cap_rate;
    uint8_t new_recv_size;
    uint32_t lock;

    lock = int_lock();
    new_recv_rate = new_sample_rate_recv;
    new_recv_size = new_sample_size_recv;
    new_send_rate = new_sample_rate_send;
    int_unlock(lock);

    play_rate = sample_rate_play;
    cap_rate = sample_rate_cap;

    if (stream == AUD_STREAM_PLAYBACK) {
        if (sample_rate_recv == new_recv_rate && sample_size_recv == new_recv_size) {
            // No change
            goto _done_rate_size_check;
        }
#ifdef AUDIO_PLAYBACK_24BIT
        if (sample_rate_recv == new_recv_rate && sample_size_recv != new_recv_size) {
            // No codec change. Only a change in usb sample size
            TRACE(3,"%s:1: Update recv sample size from %u to %u", __FUNCTION__, sample_size_recv, new_recv_size);
            sample_size_recv = new_recv_size;
            update_playback_sync_info();
            goto _done_rate_size_check;
        }
#endif
    } else {
        if (sample_rate_send == new_send_rate) {
            // No change
            goto _done_rate_size_check;
        }
    }

    if (stream == AUD_STREAM_PLAYBACK) {
        play_rate = calc_play_sample_rate(new_recv_rate);
        if (sample_rate_play == play_rate) {
            if (sample_size_recv == new_recv_size) {
                // No codec change. Only a change in usb sample rate
                TRACE(3,"%s:1: Update recv sample rate from %u to %u", __FUNCTION__, sample_rate_recv, new_recv_rate);
                sample_rate_recv = new_recv_rate;
                update_playback_sync_info();
                goto _done_rate_size_check;
            }
        } else {
#ifndef __AUDIO_RESAMPLE__
            bool new_rate_44p1k;
            bool old_rate_44p1k;

            new_rate_44p1k = !!(play_rate % AUD_SAMPRATE_8000);
            old_rate_44p1k = !!(sample_rate_play % AUD_SAMPRATE_8000);

            if (new_rate_44p1k ^ old_rate_44p1k) {
                cap_rate = calc_cap_sample_rate(new_send_rate, play_rate);
                update_cap = true;
            }
#endif
        }
        update_play = true;
    } else {
#ifndef __AUDIO_RESAMPLE__
        if (playback_state == AUDIO_ITF_STATE_STOPPED) {
            bool usb_rate_44p1k;
            bool play_rate_44p1k;

            usb_rate_44p1k = !!(new_send_rate % AUD_SAMPRATE_8000);
            play_rate_44p1k = !!(play_rate % AUD_SAMPRATE_8000);

            if (usb_rate_44p1k ^ play_rate_44p1k) {
                play_rate = calc_play_sample_rate(usb_rate_44p1k ? AUD_SAMPRATE_44100 : AUD_SAMPRATE_48000);
                update_play = true;
            }
        }
#endif
        cap_rate = calc_cap_sample_rate(new_send_rate, play_rate);
        if (sample_rate_cap == cap_rate) {
            // No codec change. Only a change in usb sample rate
            TRACE(3,"%s:1: Update send sample rate from %u to %u", __FUNCTION__, sample_rate_send, new_send_rate);
            sample_rate_send = new_send_rate;
            update_capture_sync_info();
            goto _done_rate_size_check;
        }
        update_cap = true;
    }

_done_rate_size_check:;
#endif

#ifdef DSD_SUPPORT
    bool new_dsd_state;

    new_dsd_state = usb_dsd_enabled;
    if (stream == AUD_STREAM_PLAYBACK) {
        if (codec_dsd_enabled != new_dsd_state) {
            update_play = true;
        }
    }
#endif

    if (!update_play && !update_cap) {
        return;
    }

    // 1) To avoid L/R sample misalignment, streams must be stopped before changing sample rate
    // 2) To avoid FIFO corruption, streams must be stopped before changing sample size
    if (update_cap && codec_stream_map[AUD_STREAM_CAPTURE][AUDIO_STREAM_STARTED]) {
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_CAP, true);
        usb_audio_stop_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USER_ALL);
    }
    if (update_play && codec_stream_map[AUD_STREAM_PLAYBACK][AUDIO_STREAM_STARTED]) {
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_PLAY, true);
        usb_audio_stop_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USER_ALL);
    }

    if (update_play) {
#ifdef USB_AUDIO_DYN_CFG
        if (sample_rate_recv != new_recv_rate) {
            TRACE(3,"%s:2: Update recv sample rate from %u to %u", __FUNCTION__, sample_rate_recv, new_recv_rate);
            sample_rate_recv = new_recv_rate;
        }
        if (sample_rate_play != play_rate) {
            TRACE(3,"%s:2: Update play sample rate from %u to %u", __FUNCTION__, sample_rate_play, play_rate);
            sample_rate_play = play_rate;
        }
        if (sample_size_recv != new_recv_size) {
            TRACE(3,"%s:2: Update recv sample size from %u to %u", __FUNCTION__, sample_size_recv, new_recv_size);
            sample_size_recv = new_recv_size;
#ifndef AUDIO_PLAYBACK_24BIT
            uint8_t old_play_size;

            old_play_size = sample_size_play;
            sample_size_play = (new_recv_size == 2) ? 2 : 4;
            TRACE(3,"%s:2: Update play sample size from %u to %u", __FUNCTION__, old_play_size, sample_size_play);
#endif
        }
#endif
#ifdef DSD_SUPPORT
        if (codec_dsd_enabled != new_dsd_state) {
            codec_dsd_enabled = new_dsd_state;
#ifdef CODEC_DSD
            uint8_t old_play_size;

            old_play_size = sample_size_play;
            if (codec_dsd_enabled) {
                playback_size /= ((sample_size_play + 1) / 2);
                dsd_saved_sample_size = sample_size_play;
                sample_size_play = 2;
            } else {
                playback_size *= ((dsd_saved_sample_size + 1) / 2);
                sample_size_play = dsd_saved_sample_size;
            }
            TRACE(4,"%s:2: CODEC_DSD=%u update play sample size from %u to %u", __FUNCTION__, codec_dsd_enabled, old_play_size, sample_size_play);
#endif
        }
#endif
    }
    if (update_cap) {
#ifdef USB_AUDIO_DYN_CFG
        if (sample_rate_send != new_send_rate) {
            TRACE(3,"%s:2: Update send sample rate from %u to %u", __FUNCTION__, sample_rate_send, new_send_rate);
            sample_rate_send = new_send_rate;
        }
        if (sample_rate_cap != cap_rate) {
            TRACE(3,"%s:2: Update cap sample rate from %u to %u", __FUNCTION__, sample_rate_cap, cap_rate);
            sample_rate_cap = cap_rate;
        }
#endif
    }

#if defined(USB_AUDIO_DYN_CFG) && defined(KEEP_SAME_LATENCY)
    playback_size = calc_playback_size(sample_rate_play);
    capture_size = calc_capture_size(sample_rate_cap);
#endif

#ifdef SW_CAPTURE_RESAMPLE
    // Check whether to start capture resample
    if (update_cap && capture_state == AUDIO_ITF_STATE_STARTED) {
        capture_stream_resample_config();
    }
#endif

#ifdef STREAM_RATE_BITS_SETUP
    if (update_play) {
        update_playback_sync_info();

        ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &cfg, true);
        if (ret == 0) {
            // Changes in AF_STREAM_CONFIG_T::bits must be in another cfg variable
            stream_cfg = *cfg;
#ifdef CODEC_DSD
            if (codec_dsd_enabled) {
                stream_cfg.bits = AUD_BITS_24;
            } else
#endif
            {
                stream_cfg.bits = sample_size_to_enum_playback(sample_size_play);
            }
            stream_cfg.sample_rate = sample_rate_play;
#if (defined(USB_AUDIO_DYN_CFG) && defined(KEEP_SAME_LATENCY)) || defined(CODEC_DSD)
            stream_cfg.data_size = playback_size;
#endif
            af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        }

#ifdef AUDIO_ANC_FB_MC
        ret = af_stream_get_cfg(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &cfg, true);
        if (ret == 0) {
            // Changes in AF_STREAM_CONFIG_T::bits must be in another cfg variable
            stream_cfg = *cfg;
            stream_cfg.bits = sample_size_to_enum_playback(sample_size_play);

            if(sample_rate_play==AUD_SAMPRATE_8000)
            {
                playback_samplerate_ratio=8*6;
            }
            else if(sample_rate_play==AUD_SAMPRATE_16000)
            {
                playback_samplerate_ratio=8*3;
            }
            else if((sample_rate_play==AUD_SAMPRATE_44100)||(sample_rate_play==AUD_SAMPRATE_48000)||(sample_rate_play==AUD_SAMPRATE_50781))
            {
                playback_samplerate_ratio=8;
            }
            else if((sample_rate_play==AUD_SAMPRATE_88200)||(sample_rate_play==AUD_SAMPRATE_96000))
            {
                playback_samplerate_ratio=4;
            }
            else if((sample_rate_play==AUD_SAMPRATE_176400)||(sample_rate_play==AUD_SAMPRATE_192000))
            {
                playback_samplerate_ratio=2;
            }
            else if(sample_rate_play==AUD_SAMPRATE_384000)
            {
                playback_samplerate_ratio=1;
            }
            else
            {
                playback_samplerate_ratio=1;
                ASSERT(false, "Music cancel can't support playback sample rate:%d",sample_rate_play);
            }

            stream_cfg.data_ptr = playback_buf+playback_size;
            stream_cfg.data_size = playback_size*playback_samplerate_ratio;

            stream_cfg.sample_rate = sample_rate_play;
            af_stream_setup(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
            anc_mc_run_setup((sample_rate_play*playback_samplerate_ratio)/8);
        }
#endif
    }
    if (update_cap) {
        update_capture_sync_info();

        ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &cfg, true);
        if (ret == 0) {
            stream_cfg = *cfg;
            stream_cfg.bits = sample_size_to_enum_capture(sample_size_cap);
#if defined(USB_AUDIO_DYN_CFG) && defined(KEEP_SAME_LATENCY)
            stream_cfg.data_size = capture_size;
#endif
            stream_cfg.sample_rate = sample_rate_cap;
            af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        }
    }
#else // !STREAM_RATE_BITS_SETUP
    // Close streams
    if (update_cap && codec_stream_map[AUD_STREAM_CAPTURE][AUDIO_STREAM_OPENED]) {
        usb_audio_close_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USER_ALL);
    }
    if (update_play && codec_stream_map[AUD_STREAM_PLAYBACK][AUDIO_STREAM_OPENED]) {
        usb_audio_close_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USER_ALL);
    }
    // Open streams
    if (update_play && codec_stream_map[AUD_STREAM_PLAYBACK][AUDIO_STREAM_OPENED]) {
        usb_audio_open_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USER_ALL);
    }
    if (update_cap && codec_stream_map[AUD_STREAM_CAPTURE][AUDIO_STREAM_OPENED]) {
        usb_audio_open_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USER_ALL);
    }
#endif // !STREAM_RATE_BITS_SETUP

    if (update_play && codec_stream_map[AUD_STREAM_PLAYBACK][AUDIO_STREAM_STARTED]) {
        usb_audio_start_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USER_ALL);
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_PLAY, false);
    }
    if (update_cap && codec_stream_map[AUD_STREAM_CAPTURE][AUDIO_STREAM_STARTED]) {
        usb_audio_start_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USER_ALL);
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_CAP, false);
    }
}

static void start_play(uint8_t seq)
{
    if (playback_state == AUDIO_ITF_STATE_STOPPED) {
#ifdef FREQ_RESP_EQ
        freq_resp_eq_init();
#endif

#ifdef DELAY_STREAM_OPEN
        usb_audio_open_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USB);
#endif

#ifdef NOISE_GATING
        last_high_signal_time = hal_sys_timer_get();
#ifdef NOISE_REDUCTION
        last_nr_restore_time = hal_sys_timer_get();
#endif
#endif

        usb_audio_start_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USB);

        playback_state = AUDIO_ITF_STATE_STARTED;
    }

    codec_play_seq = seq;
    playback_paused = 0;

    usb_audio_update_freq();
}

static void usb_audio_cmd_start_play(uint8_t seq)
{
#if defined(USB_AUDIO_DYN_CFG) && defined(USB_AUDIO_UAC2)
    usb_audio_update_codec_stream(AUD_STREAM_PLAYBACK);
#endif

    start_play(seq);
}

static void usb_audio_cmd_stop_play(void)
{
    if (playback_state == AUDIO_ITF_STATE_STARTED) {
        usb_audio_stop_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USB);

#ifdef PA_ON_OFF_KEY
        if (pa_on_off_muted) {
            pa_on_off_muted = false;
            if (mute_user_map == 0) {
                usb_audio_codec_unmute(CODEC_MUTE_USER_ALL);
            }
        }
#endif
#ifdef NOISE_GATING
        // Restore the noise gating status
        if (mute_user_map & (1 << CODEC_MUTE_USER_NOISE_GATING)) {
            usb_audio_codec_unmute(CODEC_MUTE_USER_NOISE_GATING);
        }
#ifdef NOISE_REDUCTION
        restore_noise_reduction_status();
#endif
#endif

#ifdef DELAY_STREAM_OPEN
        usb_audio_close_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USB);
#endif

        playback_paused = 0;
        playback_state = AUDIO_ITF_STATE_STOPPED;
#ifdef DSD_SUPPORT
        if (codec_dsd_enabled) {
            usb_audio_update_codec_stream(AUD_STREAM_PLAYBACK);
        }
#endif
    }

    usb_audio_update_freq();
}

static void start_capture(uint8_t seq)
{
    if (capture_state == AUDIO_ITF_STATE_STOPPED) {
#ifdef SW_CAPTURE_RESAMPLE
        // Check whether to start capture resample
        capture_stream_resample_config();
#endif

#ifdef DELAY_STREAM_OPEN
        usb_audio_open_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USB);
#endif
        usb_audio_start_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USB);

        capture_state = AUDIO_ITF_STATE_STARTED;
    }

    codec_cap_seq = seq;

    usb_audio_update_freq();
}

static void usb_audio_cmd_start_capture(uint8_t seq)
{
    start_capture(seq);
}

static void usb_audio_cmd_stop_capture(void)
{
    if (capture_state == AUDIO_ITF_STATE_STARTED) {
        usb_audio_stop_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USB);

#ifdef DELAY_STREAM_OPEN
        usb_audio_close_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USB);
#endif

#ifdef SW_CAPTURE_RESAMPLE
        if (resample_cap_enabled) {
            audio_resample_ex_close(resample_id);
            resample_cap_enabled = false;
        }
#endif

        capture_state = AUDIO_ITF_STATE_STOPPED;
    }

    usb_audio_update_freq();
}

static void usb_audio_cmd_set_volume(void)
{
#ifndef USB_AUDIO_MULTIFUNC
    playback_vol = new_playback_vol;
#endif

    usb_audio_set_codec_volume(AUD_STREAM_PLAYBACK, playback_vol);

#ifdef UNMUTE_WHEN_SET_VOL
    // Unmute if muted before
    if (mute_user_map & (1 << CODEC_MUTE_USER_CMD)) {
        usb_audio_codec_unmute(CODEC_MUTE_USER_CMD);
    }
#endif
}

static void usb_audio_cmd_set_cap_volume(void)
{
    capture_vol = new_capture_vol;

    usb_audio_set_codec_volume(AUD_STREAM_CAPTURE, capture_vol);
}

static void usb_audio_cmd_mute_ctrl(void)
{
    uint8_t mute;

#ifdef USB_AUDIO_MULTIFUNC
    mute = new_mute_state && new_mute2_state;
#else
    mute = new_mute_state;
#endif

    if (mute) {
        usb_audio_codec_mute(CODEC_MUTE_USER_CMD);
    } else {
        usb_audio_codec_unmute(CODEC_MUTE_USER_CMD);
    }
}

static void usb_audio_cmd_cap_mute_ctrl(void)
{
    uint8_t mute;

    mute = new_cap_mute_state;

    af_stream_mute(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, mute);
}

static void usb_audio_cmd_usb_state(enum AUDIO_CMD_T cmd)
{
    usb_audio_cmd_stop_capture();
    usb_audio_cmd_stop_play();

    if (cmd == AUDIO_CMD_USB_RESET || cmd == AUDIO_CMD_USB_WAKEUP) {
        usb_audio_acquire_freq();
        if (cmd == AUDIO_CMD_USB_RESET) {
            usb_audio_term_streams(AUDIO_STREAM_REQ_USB);
        }
        usb_audio_reset_codec_stream_state(cmd == AUDIO_CMD_USB_RESET);
        usb_audio_init_streams(AUDIO_STREAM_REQ_USB);
    } else if (cmd == AUDIO_CMD_USB_DISCONNECT || cmd == AUDIO_CMD_USB_SLEEP) {
        usb_audio_term_streams(AUDIO_STREAM_REQ_USB);
        usb_audio_reset_codec_stream_state(cmd == AUDIO_CMD_USB_DISCONNECT);
        if (cmd == AUDIO_CMD_USB_DISCONNECT || usb_configured) {
            usb_audio_release_freq();
        }
    } else if (cmd == AUDIO_CMD_USB_CONFIG) {
        usb_configured = 1;
        usb_audio_reset_codec_stream_state(true);
    }
}

static void usb_audio_cmd_recv_pause(uint8_t seq)
{
    TRACE(3,"%s: Recv pause: seq=%u usb_recv_seq=%u", __FUNCTION__, seq, usb_recv_seq);

    if (seq == usb_recv_seq) {
        usb_audio_cmd_stop_play();
        playback_paused = 1;
    }
}

static void usb_audio_cmd_recv_continue(uint8_t seq)
{
    TRACE(3,"%s: Recv continue: seq=%u usb_recv_seq=%u", __FUNCTION__, seq, usb_recv_seq);

    if (seq == usb_recv_seq && playback_paused) {
        start_play(seq);
        //playback_paused = 0;
    }
}

#ifdef USB_AUDIO_DYN_CFG
static void usb_audio_cmd_set_playback_rate(uint8_t seq, uint8_t index)
{
#ifndef USB_AUDIO_UAC2
    if (seq != usb_recv_seq) {
        TRACE(3,"%s: Bad seq: seq=%u usb_recv_seq=%u", __FUNCTION__, seq, usb_recv_seq);
        return;
    }
#endif

    usb_audio_update_codec_stream(AUD_STREAM_PLAYBACK);

#ifndef USB_AUDIO_UAC2
    start_play(seq);
#endif
}

static void usb_audio_cmd_set_capture_rate(uint8_t seq, uint8_t index)
{
#ifndef USB_AUDIO_UAC2
    if (seq != usb_send_seq) {
        TRACE(3,"%s: Bad seq: seq=%u usb_send_seq=%u", __FUNCTION__, seq, usb_send_seq);
        return;
    }
#endif

    usb_audio_update_codec_stream(AUD_STREAM_CAPTURE);

#ifndef USB_AUDIO_UAC2
    start_capture(seq);
#endif
}
#endif

static void usb_audio_cmd_reset_codec(void)
{
    TRACE(1,"%s: RESET CODEC", __FUNCTION__);

    // Regarding PLL reconfiguration, stream stop and start are enough.
    // Complete DAC/ADC reset (including analog parts) can be achieved by
    // stream close and open, which need to invoke explicitly here if
    // DELAY_STREAM_OPEN is not defined.

    if (codec_stream_map[AUD_STREAM_CAPTURE][AUDIO_STREAM_STARTED]) {
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_CAP, true);
        usb_audio_stop_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USER_ALL);
    }

    if (codec_stream_map[AUD_STREAM_PLAYBACK][AUDIO_STREAM_STARTED]) {
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_PLAY, true);
        usb_audio_stop_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USER_ALL);
    }

    rate_tune_ratio[AUD_STREAM_PLAYBACK] = 0;
    rate_tune_ratio[AUD_STREAM_CAPTURE] = 0;
#ifdef __AUDIO_RESAMPLE__
#ifdef PLL_TUNE_SAMPLE_RATE
    af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, rate_tune_ratio[AUD_STREAM_PLAYBACK]);
    af_codec_tune_resample_rate(AUD_STREAM_CAPTURE, rate_tune_ratio[AUD_STREAM_CAPTURE]);
#elif defined(PLL_TUNE_XTAL)
    af_codec_tune_xtal(rate_tune_ratio[AUD_STREAM_PLAYBACK]);
#endif
#else
    af_codec_tune_pll(rate_tune_ratio[AUD_STREAM_PLAYBACK]);
#endif

    if (codec_stream_map[AUD_STREAM_PLAYBACK][AUDIO_STREAM_STARTED]) {
        usb_audio_start_codec_stream(AUD_STREAM_PLAYBACK, AUDIO_STREAM_REQ_USER_ALL);
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_PLAY, false);
    }

    if (codec_stream_map[AUD_STREAM_CAPTURE][AUDIO_STREAM_STARTED]) {
        usb_audio_start_codec_stream(AUD_STREAM_CAPTURE, AUDIO_STREAM_REQ_USER_ALL);
        set_codec_config_status(CODEC_CONFIG_LOCK_RESTART_CAP, false);
    }
}

#ifdef NOISE_GATING
static void usb_audio_cmd_noise_gating(void)
{
    uint32_t lock;
    enum NOISE_GATING_CMD_T cmd;

    lock = int_lock();
    cmd = noise_gating_cmd;
    noise_gating_cmd = NOISE_GATING_CMD_NULL;
    int_unlock(lock);

    TRACE(3,"%s: cmd=%d map=0x%02X", __FUNCTION__, cmd, mute_user_map);

    if (cmd == NOISE_GATING_CMD_MUTE) {
        if ((mute_user_map & (1 << CODEC_MUTE_USER_NOISE_GATING)) == 0) {
            usb_audio_codec_mute(CODEC_MUTE_USER_NOISE_GATING);
        }
    } else if (cmd == NOISE_GATING_CMD_UNMUTE) {
        if (mute_user_map & (1 << CODEC_MUTE_USER_NOISE_GATING)) {
            usb_audio_codec_unmute(CODEC_MUTE_USER_NOISE_GATING);
        }
    }
}

#ifdef NOISE_REDUCTION
static void restore_noise_reduction_status(void)
{
    if (nr_active) {
        nr_active = false;
        af_codec_set_noise_reduction(false);
    }
    prev_samp_positive[0] = prev_samp_positive[1] = false;
    memset(prev_zero_diff, 0, sizeof(prev_zero_diff));
    memset(cur_zero_diff, 0, sizeof(cur_zero_diff));
    nr_cont_cnt = 0;
}

static void usb_audio_cmd_noise_reduction(void)
{
    uint32_t lock;
    enum NOISE_REDUCTION_CMD_T cmd;

    lock = int_lock();
    cmd = noise_reduction_cmd;
    noise_reduction_cmd = NOISE_REDUCTION_CMD_NULL;
    int_unlock(lock);

    TRACE(2,"%s: cmd=%d", __FUNCTION__, cmd);

    if (cmd == NOISE_REDUCTION_CMD_FIRE) {
        nr_active = true;
        af_codec_set_noise_reduction(true);
    } else if (cmd == NOISE_REDUCTION_CMD_RESTORE) {
        nr_active = false;
        af_codec_set_noise_reduction(false);
    }
}
#endif
#endif

static void usb_audio_cmd_tune_rate(enum AUD_STREAM_T stream)
{
#ifdef __AUDIO_RESAMPLE__
#ifdef PLL_TUNE_SAMPLE_RATE
    bool update_play, update_cap;

    update_play = true;
    update_cap = false;

#ifdef UAUD_SYNC_STREAM_TARGET
    if (stream == AUD_STREAM_PLAYBACK) {
        update_play = true;
        update_cap = false;
    } else {
        update_play = false;
        update_cap = true;
    }
#else
    update_play = true;
    update_cap = true;
#endif

#ifdef SW_RESAMPLE
    if (update_play) {
        audio_resample_ex_set_ratio_step(play_resample_id, nominal_ratio_step * rate_tune_ratio[AUD_STREAM_PLAYBACK]);
    }
    if (update_cap) {
        audio_resample_ex_set_ratio_step(cap_resample_id, nominal_ratio_step * rate_tune_ratio[AUD_STREAM_CAPTURE]);
    }
#else
    if (update_play) {
        af_codec_tune_resample_rate(AUD_STREAM_PLAYBACK, rate_tune_ratio[AUD_STREAM_PLAYBACK]);
    }
    if (update_cap) {
        af_codec_tune_resample_rate(AUD_STREAM_CAPTURE, rate_tune_ratio[AUD_STREAM_CAPTURE]);
    }
#endif
#elif defined(PLL_TUNE_XTAL)
    af_codec_tune_xtal(rate_tune_ratio[AUD_STREAM_PLAYBACK]);
#endif
#else // !__AUDIO_RESAMPLE__
    af_codec_tune_pll(rate_tune_ratio[AUD_STREAM_PLAYBACK]);
#endif // !__AUDIO_RESAMPLE__
}

#ifdef DSD_SUPPORT
static void usb_audio_cmd_set_dsd_cfg(void)
{
    if (codec_dsd_enabled != usb_dsd_enabled) {
        TRACE(3,"%s: %d => %d", __FUNCTION__, codec_dsd_enabled, usb_dsd_enabled);
        usb_audio_update_codec_stream(AUD_STREAM_PLAYBACK);
    }
}
#endif

#ifdef PERF_TEST_POWER_KEY
static void test_cmd_perf_test_power(void)
{
    perft_power_type++;
    if (perft_power_type >= HAL_CODEC_PERF_TEST_QTY) {
        perft_power_type = 0;
    }

    TRACE(2,"%s: %d", __FUNCTION__, perft_power_type);

    af_codec_set_perf_test_power(perft_power_type);
}
#endif

#ifdef PA_ON_OFF_KEY
static void test_cmd_pa_on_off(void)
{
    pa_on_off_muted = !pa_on_off_muted;

    TRACE(2,"%s: %d", __FUNCTION__, pa_on_off_muted);

    if (pa_on_off_muted) {
        usb_audio_codec_mute(CODEC_MUTE_USER_ALL);
    } else {
        usb_audio_codec_unmute(CODEC_MUTE_USER_ALL);
#ifdef PERF_TEST_POWER_KEY
        af_codec_set_perf_test_power(perft_power_type);
#endif
    }
}
#endif

static void usb_audio_cmd_handler(void *param)
{
    uint32_t data;
    enum AUDIO_CMD_T cmd;
    uint8_t seq;
    uint8_t POSSIBLY_UNUSED arg;

    hal_cpu_wake_unlock(USB_AUDIO_CPU_WAKE_USER);

    while (safe_queue_get(&cmd_queue, &data) == 0) {
        cmd = EXTRACT_CMD(data);
        seq = EXTRACT_SEQ(data);
        arg = EXTRACT_ARG(data);
        TRACE(4,"%s: cmd=%d seq=%d arg=%d", __FUNCTION__, cmd, seq, arg);

        switch (cmd) {
        case AUDIO_CMD_START_PLAY:
            usb_audio_cmd_start_play(seq);
            break;

        case AUDIO_CMD_STOP_PLAY:
            usb_audio_cmd_stop_play();
            break;

        case AUDIO_CMD_START_CAPTURE:
            usb_audio_cmd_start_capture(seq);
            break;

        case AUDIO_CMD_STOP_CAPTURE:
            usb_audio_cmd_stop_capture();
            break;

        case AUDIO_CMD_SET_VOLUME:
            usb_audio_cmd_set_volume();
            break;

        case AUDIO_CMD_SET_CAP_VOLUME:
            usb_audio_cmd_set_cap_volume();
            break;

        case AUDIO_CMD_MUTE_CTRL:
            usb_audio_cmd_mute_ctrl();
            break;

        case AUDIO_CMD_CAP_MUTE_CTRL:
            usb_audio_cmd_cap_mute_ctrl();
            break;

        case AUDIO_CMD_USB_RESET:
        case AUDIO_CMD_USB_DISCONNECT:
        case AUDIO_CMD_USB_CONFIG:
        case AUDIO_CMD_USB_SLEEP:
        case AUDIO_CMD_USB_WAKEUP:
            usb_audio_cmd_usb_state(cmd);
            break;

        case AUDIO_CMD_RECV_PAUSE:
            usb_audio_cmd_recv_pause(seq);
            break;
        case AUDIO_CMD_RECV_CONTINUE:
            usb_audio_cmd_recv_continue(seq);
            break;

#ifdef USB_AUDIO_DYN_CFG
        case AUDIO_CMD_SET_RECV_RATE:
            usb_audio_cmd_set_playback_rate(seq, arg);
            break;
        case AUDIO_CMD_SET_SEND_RATE:
            usb_audio_cmd_set_capture_rate(seq, arg);
            break;
#endif

        case AUDIO_CMD_RESET_CODEC:
            usb_audio_cmd_reset_codec();
            break;

#ifdef NOISE_GATING
        case AUDIO_CMD_NOISE_GATING:
            usb_audio_cmd_noise_gating();
            break;

#ifdef NOISE_REDUCTION
        case AUDIO_CMD_NOISE_REDUCTION:
            usb_audio_cmd_noise_reduction();
            break;
#endif
#endif

        case AUDIO_CMD_TUNE_RATE:
            usb_audio_cmd_tune_rate((enum AUD_STREAM_T)arg);
            break;

#ifdef DSD_SUPPORT
        case AUDIO_CMD_SET_DSD_CFG:
            usb_audio_cmd_set_dsd_cfg();
            break;
#endif

#ifdef PERF_TEST_POWER_KEY
        case TEST_CMD_PERF_TEST_POWER:
            test_cmd_perf_test_power();
            break;
#endif

#ifdef PA_ON_OFF_KEY
        case TEST_CMD_PA_ON_OFF:
            test_cmd_pa_on_off();
            break;
#endif

        default:
            ASSERT(false, "%s: Invalid cmd %d", __FUNCTION__, cmd);
            break;
        }
    }
}

void usb_audio_app_init(const struct USB_AUDIO_BUF_CFG *cfg)
{
#ifdef USB_AUDIO_DYN_CFG
    ASSERT(cfg->play_size / MAX_FRAME_SIZE_PLAYBACK < cfg->recv_size / MAX_FRAME_SIZE_RECV,
        "%s: play frames %u should < recv frames %u", __FUNCTION__,
        cfg->play_size / MAX_FRAME_SIZE_PLAYBACK, cfg->recv_size / MAX_FRAME_SIZE_RECV);
    ASSERT(cfg->cap_size / MAX_FRAME_SIZE_CAPTURE < cfg->send_size / MAX_FRAME_SIZE_SEND,
        "%s: cap frames %u should < send frames %u", __FUNCTION__,
        cfg->cap_size / MAX_FRAME_SIZE_CAPTURE, cfg->send_size / MAX_FRAME_SIZE_SEND);
#else
    ASSERT(cfg->play_size / FRAME_SIZE_PLAYBACK < cfg->recv_size / FRAME_SIZE_RECV,
        "%s: play frames %u should < recv frames %u", __FUNCTION__,
        cfg->play_size / FRAME_SIZE_PLAYBACK, cfg->recv_size / FRAME_SIZE_RECV);
    ASSERT(cfg->cap_size / FRAME_SIZE_CAPTURE < cfg->send_size / FRAME_SIZE_SEND,
        "%s: cap frames %u should < send frames %u", __FUNCTION__,
        cfg->cap_size / FRAME_SIZE_CAPTURE, cfg->send_size / FRAME_SIZE_SEND);
#endif

#ifdef SW_CAPTURE_RESAMPLE
    enum AUD_CHANNEL_NUM_T chans;
    enum AUD_BITS_T bits;
    uint8_t phase_coef_num;
    uint32_t resamp_calc_size;

    chans = get_capture_resample_chan_num();
    bits = sample_size_to_enum_capture(sample_size_cap);
    // Max phase coef num
    phase_coef_num = 100;

    resamp_calc_size = audio_resample_ex_get_buffer_size(chans, bits, phase_coef_num);
    ASSERT(cfg->resample_size > resamp_calc_size,
        "%s: Resample size too small %u (should > %u)", __FUNCTION__,
        cfg->resample_size, resamp_calc_size);

    resample_history_buf = (uint8_t *)ALIGN((uint32_t)cfg->resample_buf, 4);
    resample_history_size = resamp_calc_size;
    resample_input_buf = (uint8_t *)resample_history_buf + resample_history_size;
    ASSERT(cfg->resample_buf + cfg->resample_size > resample_input_buf,
        "%s: Resample size too small: %u", __FUNCTION__, cfg->resample_size);
    resample_input_size = cfg->resample_buf + cfg->resample_size - resample_input_buf;
#endif

    playback_buf = cfg->play_buf;
    capture_buf = cfg->cap_buf;
    usb_recv_buf = cfg->recv_buf;
    usb_send_buf = cfg->send_buf;
    playback_eq_buf = cfg->eq_buf;
    playback_eq_size = cfg->eq_size;

#if defined(USB_AUDIO_DYN_CFG) && defined(KEEP_SAME_LATENCY)
    playback_max_size = cfg->play_size;
    capture_max_size = cfg->cap_size;
    usb_recv_max_size = cfg->recv_size;
    usb_send_max_size = cfg->send_size;
#else
    playback_size = cfg->play_size;
    capture_size = cfg->cap_size;
    usb_recv_size = cfg->recv_size;
    usb_send_size = cfg->send_size;
#endif

    playback_pos = 0;
    usb_recv_rpos = 0;
    usb_recv_wpos = cfg->recv_size / 2;
    capture_pos = 0;
    usb_send_rpos = cfg->send_size / 2;
    usb_send_wpos = 0;

#ifdef USB_AUDIO_MULTIFUNC
    usb_recv2_buf = cfg->recv2_buf;
    usb_recv2_size = cfg->recv2_size;
    usb_recv2_rpos = 0;
    usb_recv2_wpos = cfg->recv2_size / 2;
#endif

    playback_info.id = 0;
    playback_info.sync_thresh = DIFF_SYNC_THRESH_PLAYBACK;
    playback_info.diff_avg_cnt = DIFF_AVG_CNT;
#ifdef TARGET_TO_MAX_DIFF
    playback_info.diff_target_enabled = true;
    playback_info.max_target_ratio = MAX_TARGET_RATIO;
    playback_info.time_to_ms = NULL;
#endif
    capture_info.id = 1;
    capture_info.sync_thresh = DIFF_SYNC_THRESH_CAPTURE;
    capture_info.diff_avg_cnt = DIFF_AVG_CNT;
#ifdef TARGET_TO_MAX_DIFF
    capture_info.diff_target_enabled = true;
    capture_info.max_target_ratio = MAX_TARGET_RATIO;
    capture_info.time_to_ms = NULL;
#endif

    safe_queue_init(&cmd_queue, cmd_list, CMD_LIST_SIZE);

#ifdef USB_AUDIO_PWRKEY_TEST
    hal_cmu_simu_set_val(0);
#endif
}

void usb_audio_app_term(void)
{
    playback_buf = NULL;
    playback_size = 0;
    capture_buf = NULL;
    capture_size = 0;
    usb_recv_buf = NULL;
    usb_recv_size = 0;
    usb_send_buf = NULL;
    usb_send_size = 0;

    playback_pos = 0;
    usb_recv_rpos = 0;
    usb_recv_wpos = 0;

    capture_pos = 0;
    usb_send_rpos = 0;
    usb_send_wpos = 0;

#ifdef USB_AUDIO_MULTIFUNC
    usb_recv2_buf = NULL;
    usb_recv2_size = 0;
    usb_recv2_rpos = 0;
    usb_recv2_wpos = 0;
#endif
}

void usb_audio_app(bool on)
{
    static const struct USB_AUDIO_CFG_T usb_audio_cfg = {
        .recv_sample_rate = SAMPLE_RATE_RECV,
        .send_sample_rate = SAMPLE_RATE_SEND,
        .itf_callback = usb_audio_itf_callback,
        .mute_callback = usb_audio_mute_callback,
        .set_volume = usb_audio_set_volume,
        .get_volume = usb_audio_get_volume,
        .state_callback = usb_audio_state_control,
        .xfer_callback = usb_audio_xfer_callback,
#if defined(ANDROID_ACCESSORY_SPEC) || defined(CFG_MIC_KEY)
        // Applications are responsible for clearing HID events
        .hid_send_callback = hid_data_send_handler,
#else
        // HID events will be cleared automatically once they are sent
        .hid_send_callback = NULL,
#endif
#ifdef _VENDOR_MSG_SUPPT_
        .vendor_msg_callback = usb_vendor_callback,
        .vendor_rx_buf = vendor_msg_rx_buf,
        .vendor_rx_size = VENDOR_RX_BUF_SZ,
#endif
    };
    static bool isRun = false;
    int ret;

    if (isRun==on)
        return;
    else
        isRun=on;

    TRACE(2,"%s: on=%d", __FUNCTION__, on);

    if (on) {
        usb_audio_acquire_freq();

        usb_audio_reset_usb_stream_state(true);
        usb_audio_reset_codec_stream_state(true);

#ifdef USB_AUDIO_DYN_CFG
        sample_rate_recv = new_sample_rate_recv = usb_audio_cfg.recv_sample_rate;
        sample_rate_send = new_sample_rate_send = usb_audio_cfg.send_sample_rate;
        // Init codec sample rates after initializing usb sample rates
        sample_rate_play = calc_play_sample_rate(sample_rate_recv);
        sample_rate_cap = calc_cap_sample_rate(sample_rate_send, sample_rate_play);
        // TODO: Check whether 16-bit sample is enabled
        sample_size_recv = new_sample_size_recv = 2;
#ifndef AUDIO_PLAYBACK_24BIT
        sample_size_play = 2;
#endif

#ifdef KEEP_SAME_LATENCY
        usb_recv_size = calc_usb_recv_size(sample_rate_recv, sample_size_recv);
        usb_send_size = calc_usb_send_size(sample_rate_send);
        playback_size = calc_playback_size(sample_rate_play);
        capture_size = calc_capture_size(sample_rate_cap);
#endif
#endif

        TRACE(3,"CODEC playback sample: rate=%u size=%u chan=%u", sample_rate_play, sample_size_play, CHAN_NUM_PLAYBACK);
        TRACE(3,"CODEC capture sample: rate=%u size=%u chan=%u", sample_rate_cap, sample_size_cap, CHAN_NUM_CAPTURE);
        TRACE(2,"USB recv sample: rate=%u size=%u", sample_rate_recv, sample_size_recv);
        TRACE(2,"USB send sample: rate=%u size=%u", sample_rate_send, sample_size_send);

        usb_audio_init_streams(AUDIO_STREAM_REQ_USB);

        pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
        ret = usb_audio_open(&usb_audio_cfg);
        ASSERT(ret == 0, "usb_audio_open failed: %d", ret);
    } else {
        usb_audio_close();
        pmu_usb_config(PMU_USB_CONFIG_TYPE_NONE);

        usb_audio_term_streams(AUDIO_STREAM_REQ_USB);

        usb_audio_reset_usb_stream_state(true);
        usb_audio_reset_codec_stream_state(true);

        usb_audio_release_freq();
    }
}

static void app_key_trace(uint32_t line, enum HAL_KEY_CODE_T code,
                          enum HAL_KEY_EVENT_T event, enum USB_AUDIO_HID_EVENT_T uevt, int state)
{
#ifdef KEY_TRACE
    TRACE(5,"APPKEY/%u: code=%d event=%d uevt=%s state=%d",
        line, code, event, usb_audio_get_hid_event_name(uevt), state);
#endif
}

int usb_audio_app_key(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event)
{
    int i;
    enum USB_AUDIO_HID_EVENT_T uevt;
    int state;

    if (event == HAL_KEY_EVENT_CLICK && playback_state == AUDIO_ITF_STATE_STARTED) {
#ifdef PERF_TEST_POWER_KEY
        if (code == PERF_TEST_POWER_KEY) {
            enqueue_unique_cmd(TEST_CMD_PERF_TEST_POWER);
        }
#endif
#ifdef PA_ON_OFF_KEY
        if (code == PA_ON_OFF_KEY) {
            enqueue_unique_cmd(TEST_CMD_PA_ON_OFF);
        }
#endif
    }

#ifdef USB_AUDIO_PWRKEY_TEST
    if (code == HAL_KEY_CODE_PWR) {
        uevt = hal_cmu_simu_get_val();
        if (uevt == 0) {
            uevt = USB_AUDIO_HID_VOL_UP;
        }
#if defined(ANDROID_ACCESSORY_SPEC) || defined(CFG_MIC_KEY)
        if (event == HAL_KEY_EVENT_DOWN || event == HAL_KEY_EVENT_UP) {
            state = (event != HAL_KEY_EVENT_UP);
            app_key_trace(__LINE__, code, event, uevt, state);
            usb_audio_hid_set_event(uevt, state);
        }
        // The key event has been processed
        return 0;
#else
        if (event == HAL_KEY_EVENT_CLICK) {
            state = 1;
            app_key_trace(__LINE__, code, event, uevt, state);
            usb_audio_hid_set_event(uevt, state);
            return 0;
        }
        // Let other applications check the key event
        return 1;
#endif
    }
#endif

    if (code != HAL_KEY_CODE_NONE) {
        for (i = 0; i < ARRAY_SIZE(key_map); i++) {
            if (key_map[i].key_code == code && (key_map[i].key_event_bitset & (1 << event))) {
                break;
            }
        }
        if (i < ARRAY_SIZE(key_map)) {
            uevt = key_map[i].hid_event;
            if (uevt != 0) {
#if defined(ANDROID_ACCESSORY_SPEC) || defined(CFG_MIC_KEY)
                state = (event != HAL_KEY_EVENT_UP);
#else
                state = 1;
#endif
                app_key_trace(__LINE__, code, event, uevt, state);
                usb_audio_hid_set_event(uevt, state);
                // The key event has been processed
                return 0;
            }
        }
    }

    // Let other applications check the key event
    return 1;
}

#ifdef ANC_APP
extern bool anc_usb_app_get_status();
static uint8_t anc_status = 0;
static uint8_t anc_status_record = 0xff;
#endif

void usb_audio_eq_loop(void)
{
#ifdef ANC_APP
    anc_status = anc_usb_app_get_status();
    if(anc_status_record != anc_status&&eq_opened == 1)
    {
        anc_status_record = anc_status;
        TRACE(2,"[%s]anc_status = %d", __func__, anc_status);
#ifdef __SW_IIR_EQ_PROCESS__
        usb_audio_set_eq(AUDIO_EQ_TYPE_SW_IIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_SW_IIR,anc_status));
#endif

#ifdef __HW_FIR_EQ_PROCESS__
        usb_audio_set_eq(AUDIO_EQ_TYPE_HW_FIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_HW_FIR,anc_status));
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
        usb_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_HW_DAC_IIR,anc_status));
#endif

#ifdef __HW_IIR_EQ_PROCESS__
        usb_audio_set_eq(AUDIO_EQ_TYPE_HW_IIR,usb_audio_get_eq_index(AUDIO_EQ_TYPE_HW_IIR,anc_status));
#endif
    }
#endif
}

void usb_audio_app_loop(void)
{
    usb_audio_cmd_handler(NULL);

    usb_audio_eq_loop();

#ifndef RTOS
    extern void af_thread(void const *argument);
    af_thread(NULL);
#endif
}

uint32_t usb_audio_get_capture_sample_rate(void)
{
    return sample_rate_cap;
}

