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
/**
// Speech process macro change table
// TX: Transimt process
// RX: Receive process
// 16k: base 25M/208M(1300,1302) base 28M/104M(1400,1402)
|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
| TX/RX |        New         |                Old                | description
|  MIPS(M)  |         Note         | |       |                    | | | 16k 8k |
|
|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
|       | SPEECH_TX_DC_FILTER|                                   | Direct
Current filter            | 1~2    \  |                      | |       |
SPEECH_TX_AEC      | SPEECH_ECHO_CANCEL                | Acoustic Echo
Cancellation(old)  | 40     \  | HWFFT: 37            | |       | SPEECH_TX_AEC2
| SPEECH_AEC_FIX                    | Acoustic Echo Cancellation(new)  | 39 \  |
enable NLP           | |       | SPEECH_TX_AEC3     | | Acoustic Echo
Cancellation(new)  | 14/18  \  | 2 filters/4 filters  | |       |
SPEECH_TX_AEC2FLOAT|                                   | Acoustic Echo
Cancellation(new)  | 23/22  \  | nlp/af(blocks=1)     | |       |
SPEECH_TX_AEC2FLOAT|                                   | Acoustic Echo
Cancellation(ns)   | 14/10  \  | banks=256/banks=128  | |       | (ns_enabled)
|                                   |                                  | 8/6 \
| banks=64/banks=32    | |       | SPEECH_TX_NS       | SPEECH_NOISE_SUPPRESS |
1 mic noise suppress(old)        | 30     \  | HWFFT: 19            | |       |
SPEECH_TX_NS2      | LC_MMSE_NOISE_SUPPRESS            | 1 mic noise
suppress(new)        | 16     \  | HWFFT: 12            | |       |
SPEECH_TX_NS3      |                                   | 1 mic noise
suppress(new)        | 26     \  |                      | |       |
SPEECH_TX_NS2FLOAT | LC_MMSE_NOISE_SUPPRESS_FLOAT      | 1 mic noise
suppress(new float)  | 12.5   \  | banks=64             | | TX    |
SPEECH_TX_2MIC_NS  | DUAL_MIC_DENOISE                  | 2 mic noise
suppres(long space)  | \         |                      | |       |
SPEECH_TX_2MIC_NS2 | 2MIC_DENOISE                      | 2 mic noise
suppres(short space) | 22        | delay_taps 5M        | |       |
freq_smooth_enable 1.5M |                 | |       | wnr_enable 1.5M      | |
| SPEECH_TX_2MIC_NS4 | SENSORMIC_DENOISE                 | sensor mic noise
suppress        | 31.5      |                      | |       |
SPEECH_TX_2MIC_NS3 | FAR_FIELD_SPEECH_ENHANCEMENT      | 2 mic noise suppres(far
field)   | \         |                      | |       | SPEECH_TX_2MIC_NS5 |
LEFTRIGHT_DENOISE                 | 2 mic noise suppr(left right end)| \ | | |
| SPEECH_TX_2MIC_NS6 | FF_2MIC_DENOISE                   | 2 mic noise suppr(far
field)     | 70        |                      | |       | SPEECH_TX_AGC      |
SPEECH_AGC                        | Automatic Gain Control           | 3 | | |
| SPEECH_TX_COMPEXP  |                                   | Compressor and
expander          | 4         |                      | |       | SPEECH_TX_EQ |
SPEECH_WEIGHTING_FILTER_SUPPRESS  | Default EQ                       | 0.5     \
| each section         |
|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
|       | SPEECH_RX_NS       | SPEAKER_NOISE_SUPPRESS            | 1 mic noise
suppress(old)        | 30      \ |                      | | RX    |
SPEECH_RX_NS2      | LC_MMSE_NOISE_SUPPRESS_SPK        | 1 mic noise
suppress(new)        | 16      \ |                      | |       |
SPEECH_RX_AGC      | SPEECH_AGC_SPK                    | Automatic Gain Control
| 3         |                      | |       | SPEECH_RX_EQ       |
SPEAKER_WEIGHTING_FILTER_SUPPRESS | Default EQ                       | 0.5     \
| each section         |
|-------|--------------------|-----------------------------------|----------------------------------|-----------|----------------------|
**/

#include "bt_sco_chain.h"
#include "audio_dump.h"
#include "bt_sco_chain_cfg.h"
#include "bt_sco_chain_tuning.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "plat_types.h"
#include "speech_cfg.h"
#if defined(SPEECH_TX_2MIC_NS4)
#include "app_anc.h"
#endif
#include "app_utils.h"

#if defined(SCO_CP_ACCEL)
#include "bt_sco_chain_cp.h"
#include "hal_location.h"

#define SCO_CP_ACCEL_ALGO_START()                                              \
  *_pcm_len = pcm_len;                                                         \
  }                                                                            \
  CP_TEXT_SRAM_LOC                                                             \
  void _speech_tx_process_mid(short *pcm_buf, short *ref_buf, int *_pcm_len) { \
    int pcm_len = *_pcm_len;

#define SCO_CP_ACCEL_ALGO_END()                                                \
  *_pcm_len = pcm_len;                                                         \
  }                                                                            \
  void _speech_tx_process_post(short *pcm_buf, short *ref_buf,                 \
                               int *_pcm_len) {                                \
    int pcm_len = *_pcm_len;

#define SPEECH_TX_AEC2FLOAT_CORE 1
#define SPEECH_TX_NS2FLOAT_CORE 0
#define SPEECH_RX_NS2FLOAT_CORE 0

#else
#define SCO_CP_ACCEL_ALGO_START()
#define SCO_CP_ACCEL_ALGO_END()

#define SPEECH_TX_AEC2FLOAT_CORE 0
#define SPEECH_TX_NS2FLOAT_CORE 0
#define SPEECH_RX_NS2FLOAT_CORE 0

#endif

#if defined(SCO_OPTIMIZE_FOR_RAM)
extern uint8_t *sco_overlay_ram_buf;
extern int sco_overlay_ram_buf_len;
#endif

//#define BT_SCO_CHAIN_PROFILE
//#define BT_SCO_CHAIN_AUDIO_DUMP

#if defined(BT_SCO_CHAIN_PROFILE)
static uint32_t tx_start_ticks = 0;
static uint32_t tx_end_ticks = 0;
#endif

extern const SpeechConfig speech_cfg_default;
static SpeechConfig *speech_cfg = NULL;

FrameResizeState *speech_frame_resize_st = NULL;

#if defined(SPEECH_TX_24BIT)
int32_t *aec_echo_buf = NULL;

static int16_t *tx_pcmbuf16 = NULL;
static int32_t *tx_pcmbuf32 = NULL;
#else
short *aec_echo_buf = NULL;
#endif

#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                       \
    defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
// Use to free buffer
#if defined(SPEECH_TX_24BIT)
static int32_t *aec_echo_buf_ptr;
static int16_t *tx_refbuf16 = NULL;
static int32_t *tx_refbuf32 = NULL;
#else
static short *aec_echo_buf_ptr;
#endif
static short *aec_out_buf;
#endif

/*--------------------TX state--------------------*/
#if defined(SPEECH_TX_DC_FILTER)
SpeechDcFilterState *speech_tx_dc_filter_st = NULL;
#endif

#if defined(SPEECH_TX_MIC_CALIBRATION)
SpeechIirCalibState *speech_tx_mic_calib_st = NULL;
extern const SpeechIirCalibConfig speech_tx_mic_calib_cfg;
#endif

#if defined(SPEECH_TX_MIC_FIR_CALIBRATION)
SpeechFirCalibState *speech_tx_mic_fir_calib_st = NULL;
extern const SpeechFirCalibConfig speech_tx_mic_fir_calib_cfg;
#endif

// AEC
#if defined(SPEECH_TX_AEC)
SpeechAecState *speech_tx_aec_st = NULL;
void *speech_tx_aec_lib_st = NULL;
#endif

#if defined(SPEECH_TX_AEC2)
SpeechAec2State *speech_tx_aec2_st = NULL;
#endif

#if defined(SPEECH_TX_AEC3)
short delay = 70;
short bufferstate[356];
short buf_out[256];
void CODEC_OpVecCpy(short *pshwDes, short *pshwSrc, short swLen) {
  short i = 0;

  for (i = 0; i < swLen; i++) {
    pshwDes[i] = pshwSrc[i];
  }
}

SubBandAecState *speech_tx_aec3_st = NULL;
#endif

#if defined(SPEECH_TX_AEC2FLOAT)
static Ec2FloatState *speech_tx_aec2float_st = NULL;
#endif

// 2MIC NS
#if defined(SPEECH_TX_2MIC_NS)
// float mic2_ft_caliration[] =
// {0.395000,0.809000,0.939000,1.748000,1.904000,1.957000,1.944000,1.906000,1.935000,1.940000,1.937000,1.931000,1.929000,1.911000,1.887000,1.871000,1.846000,1.779000,1.748000,2.086000,2.055000,2.002000,1.903000,1.885000,1.854000,1.848000,1.848000,1.844000,1.852000,1.870000,1.866000,1.843000,1.838000,1.824000,1.861000,1.871000,1.866000,1.833000,1.800000,1.769000,1.749000,1.690000,1.664000,1.573000,1.602000,1.692000,1.759000,1.758000,1.698000,1.628000,1.525000,1.509000,1.492000,1.515000,1.530000,1.644000,1.653000,1.617000,1.667000,1.746000,1.663000,1.606000,1.560000,1.500000,1.579000,1.632000,1.623000,1.549000,1.524000,1.512000,1.493000,1.476000,1.421000,1.396000,1.386000,1.459000,1.463000,1.496000,1.568000,1.544000,1.555000,1.547000,1.619000,1.630000,1.574000,1.491000,1.414000,1.383000,1.352000,1.464000,1.474000,1.450000,1.419000,1.425000,1.411000,1.479000,1.517000,1.486000,1.428000,1.389000,1.330000,1.284000,1.267000,1.249000,1.256000,1.215000,1.250000,1.402000,1.386000,1.334000,1.287000,1.329000,1.337000,1.292000,1.226000,1.212000,1.142000,1.087000,1.086000,1.112000,1.145000,1.194000,1.163000,1.131000,1.162000,1.166000,1.259000,1.218000,1.218000,1.322000,1.347000,1.436000,1.890000,1.693000,1.591000,1.518000,1.422000,1.345000,1.331000,1.308000,1.330000,1.305000,1.218000,1.286000,1.340000,1.399000,1.406000,1.353000,1.280000,1.246000,1.185000,1.129000,1.014000,0.985000,0.981000,1.189000,1.533000,1.694000,1.613000,1.464000,1.419000,1.448000,1.449000,1.442000,1.367000,1.283000,1.232000,1.381000,1.484000,1.497000,1.554000,1.438000,1.365000,1.326000,1.332000,1.335000,1.367000,1.301000,1.288000,1.168000,1.103000,1.067000,1.026000,1.076000,1.126000,1.068000,1.045000,0.978000,0.926000,0.939000,0.854000,0.772000,0.902000,0.742000,1.073000,1.220000,1.177000,1.762000,1.573000,1.390000,1.406000,1.148000,1.054000,1.210000,1.344000,1.849000,2.078000,1.756000,1.646000,1.597000,1.447000,1.322000,1.279000,1.007000,0.921000,0.871000,0.864000,1.067000,1.129000,1.128000,1.027000,0.983000,0.889000,0.774000,0.759000,0.724000,0.949000,1.237000,1.499000,1.658000,1.837000,1.492000,1.452000,1.151000,1.100000,0.996000,0.986000,1.023000,1.071000,1.252000,1.295000,1.309000,1.343000,1.220000,1.161000,1.142000,1.041000,0.974000,0.885000,0.799000,0.669000,0.732000,0.953000,0.861000,0.881000,0.988000,0.891000};
#endif

#if defined(SPEECH_TX_2MIC_NS2)
Speech2MicNs2State *speech_tx_2mic_ns2_st = NULL;
#endif

// #if defined(SPEECH_CS_VAD)
// VADState *speech_cs_vad_st = NULL;
// #endif

#if defined(SPEECH_TX_2MIC_NS4)
SensorMicDenoiseState *speech_tx_2mic_ns4_st = NULL;
#endif

#if defined(SPEECH_TX_2MIC_NS5)
LeftRightDenoiseState *speech_tx_2mic_ns5_st = NULL;
#endif

#if defined(SPEECH_TX_2MIC_NS6)
SpeechFF2MicNs2State *speech_tx_2mic_ns6_st = NULL;
#endif

#if defined(SPEECH_TX_3MIC_NS)
Speech3MicNsState *speech_tx_3mic_ns_st = NULL;
#endif

#if defined(SPEECH_TX_3MIC_NS3)
TripleMicDenoise3State *speech_tx_3mic_ns3_st = NULL;
#endif

// NS
#if defined(SPEECH_TX_NS)
SpeechNsState *speech_tx_ns_st = NULL;
#endif

#if defined(SPEECH_TX_NS2)
SpeechNs2State *speech_tx_ns2_st = NULL;
#endif

#if defined(SPEECH_TX_NS2FLOAT)
SpeechNs2FloatState *speech_tx_ns2float_st = NULL;
#endif

#if defined(SPEECH_TX_NS3)
static Ns3State *speech_tx_ns3_st = NULL;
#endif

#if defined(SPEECH_TX_WNR)
static WnrState *speech_tx_wnr_st = NULL;
#endif

#if defined(SPEECH_TX_NOISE_GATE)
static NoisegateState *speech_tx_noise_gate_st = NULL;
#endif

// Gain
#if defined(SPEECH_TX_COMPEXP)
static CompexpState *speech_tx_compexp_st = NULL;
#endif

#if defined(SPEECH_TX_AGC)
static AgcState *speech_tx_agc_st = NULL;
#endif

// EQ
#if defined(SPEECH_TX_EQ)
EqState *speech_tx_eq_st = NULL;
#endif

// GAIN
#if defined(SPEECH_TX_POST_GAIN)
SpeechGainState *speech_tx_post_gain_st = NULL;
#endif

/*--------------------RX state--------------------*/
// NS
#if defined(SPEECH_RX_NS)
SpeechNsState *speech_rx_ns_st = NULL;
#endif

#if defined(SPEECH_RX_NS2)
SpeechNs2State *speech_rx_ns2_st = NULL;
#endif

#if defined(SPEECH_RX_NS2FLOAT)
SpeechNs2FloatState *speech_rx_ns2float_st = NULL;
#endif

#if defined(SPEECH_RX_NS3)
static Ns3State *speech_rx_ns3_st = NULL;
#endif

// GAIN
#if defined(SPEECH_RX_AGC)
static AgcState *speech_rx_agc_st = NULL;
#endif

// EQ
#if defined(SPEECH_RX_EQ)
EqState *speech_rx_eq_st = NULL;
#endif

// GAIN
#if defined(SPEECH_RX_POST_GAIN)
SpeechGainState *speech_rx_post_gain_st = NULL;
#endif

#if defined(BONE_SENSOR_TDM)
#define SPEECH_TX_CHANNEL_NUM (SPEECH_CODEC_CAPTURE_CHANNEL_NUM + 1)
#else
#define SPEECH_TX_CHANNEL_NUM (SPEECH_CODEC_CAPTURE_CHANNEL_NUM)
#endif

static bool dualmic_enable = true;

void switch_dualmic_status(void) {
  TRACE(3, "[%s] dualmic status: %d -> %d", __FUNCTION__, dualmic_enable,
        !dualmic_enable);
  dualmic_enable ^= true;
}

static int speech_tx_sample_rate = 16000;
static int speech_rx_sample_rate = 16000;
static int speech_tx_frame_len = 256;
static int speech_rx_frame_len = 256;
static bool speech_tx_frame_resizer_enable = false;
static bool speech_rx_frame_resizer_enable = false;

static int32_t _speech_tx_process_(void *pcm_buf, void *ref_buf,
                                   int32_t *pcm_len);
static int32_t _speech_rx_process_(void *pcm_buf, int32_t *pcm_len);
enum APP_SYSFREQ_FREQ_T speech_get_proper_sysfreq(int *needed_mips);

void *speech_get_ext_buff(int size) {
  void *pBuff = NULL;
  if (size % 4) {
    size = size + (4 - size % 4);
  }

  pBuff = speech_calloc(size, sizeof(uint8_t));
  TRACE(2, "[%s] len:%d", __func__, size);

  return pBuff;
}

int speech_store_config(const SpeechConfig *cfg) {
  if (speech_cfg) {
    memcpy(speech_cfg, cfg, sizeof(SpeechConfig));
  } else {
    TRACE(1, "[%s] WARNING: Please phone call...", __func__);
  }

  return 0;
}

int speech_tx_init(int sample_rate, int frame_len) {
  TRACE(3, "[%s] Start, sample_rate: %d, frame_len: %d", __func__, sample_rate,
        frame_len);

#if defined(SPEECH_TX_DC_FILTER)
  int channel_num = SPEECH_TX_CHANNEL_NUM;
  int data_separation = 0;

  speech_tx_dc_filter_st =
      speech_dc_filter_create(sample_rate, &speech_cfg->tx_dc_filter);
  speech_dc_filter_ctl(speech_tx_dc_filter_st, SPEECH_DC_FILTER_SET_CHANNEL_NUM,
                       &channel_num);
  speech_dc_filter_ctl(speech_tx_dc_filter_st,
                       SPEECH_DC_FILTER_SET_DATA_SEPARATION, &data_separation);
#endif

#if defined(SPEECH_TX_MIC_CALIBRATION)
  speech_tx_mic_calib_st =
      speech_iir_calib_init(sample_rate, frame_len, &speech_tx_mic_calib_cfg);
#endif

#if defined(SPEECH_TX_MIC_FIR_CALIBRATION)
  speech_tx_mic_fir_calib_st = speech_fir_calib_init(
      sample_rate, frame_len, &speech_tx_mic_fir_calib_cfg);
#endif

#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                       \
    defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
  // #if !(defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE))
  aec_out_buf = (short *)speech_calloc(frame_len, sizeof(short));
#if defined(SPEECH_TX_24BIT)
  aec_echo_buf = (int32_t *)speech_calloc(frame_len, sizeof(int32_t));
#else
  aec_echo_buf = (short *)speech_calloc(frame_len, sizeof(short));
#endif
  aec_echo_buf_ptr = aec_echo_buf;
// #endif
#endif

#if defined(SPEECH_TX_AEC)
  speech_tx_aec_st =
      speech_aec_create(sample_rate, frame_len, &speech_cfg->tx_aec);
  speech_aec_ctl(speech_tx_aec_st, SPEECH_AEC_GET_LIB_ST,
                 &speech_tx_aec_lib_st);
#endif

#if defined(SPEECH_TX_AEC2)
  speech_tx_aec2_st =
      speech_aec2_create(sample_rate, frame_len, &speech_cfg->tx_aec2);
#endif
#if defined(SPEECH_TX_AEC3)
  speech_tx_aec3_st =
      SubBandAec_init(sample_rate, frame_len, &speech_cfg->tx_aec3);
#endif

#if defined(SPEECH_TX_AEC2FLOAT)
  speech_tx_aec2float_st =
      ec2float_create(sample_rate, frame_len, SPEECH_TX_AEC2FLOAT_CORE,
                      &speech_cfg->tx_aec2float);
#endif

#if defined(SPEECH_TX_2MIC_NS)
  dual_mic_denoise_init(sample_rate, frame_len, &speech_cfg->tx_2mic_ns, NULL);
  // dual_mic_denoise_ctl(DUAL_MIC_DENOISE_SET_CALIBRATION_COEF,
  // mic2_ft_caliration);
#endif

#if defined(SPEECH_TX_2MIC_NS2)
  speech_tx_2mic_ns2_st =
      speech_2mic_ns2_create(sample_rate, frame_len, &speech_cfg->tx_2mic_ns2);
#endif

  // #if defined(SPEECH_CS_VAD)
  //     speech_cs_vad_st = VAD_process_state_init(sample_rate, 64,
  //     &speech_cfg->cs_vad);
  // #endif

#if defined(SPEECH_TX_2MIC_NS3)
  far_field_speech_enhancement_init();
  far_field_speech_enhancement_start();
#endif

#if defined(SPEECH_TX_2MIC_NS5)
  speech_tx_2mic_ns5_st =
      leftright_denoise_create(sample_rate, 64, &speech_cfg->tx_2mic_ns5);
#endif

#if defined(SPEECH_TX_2MIC_NS6)
  speech_tx_2mic_ns6_st = speech_ff_2mic_ns2_create(16000, 128);
#endif

#if defined(SPEECH_TX_2MIC_NS4)
  // speech_tx_2mic_ns4_st = sensormic_denoise_create(sample_rate, 64,
  // &speech_cfg->tx_2mic_ns4);
  speech_tx_2mic_ns4_st = sensormic_denoise_create(sample_rate, 128, 256, 31,
                                                   &speech_cfg->tx_2mic_ns4);
#endif

#if defined(SPEECH_TX_3MIC_NS)
  speech_tx_3mic_ns_st =
      speech_3mic_ns_create(sample_rate, 64, &speech_cfg->tx_3mic_ns);
#endif

#if defined(SPEECH_TX_3MIC_NS3)
  speech_tx_3mic_ns3_st = triple_mic_denoise3_init(sample_rate, frame_len,
                                                   &speech_cfg->tx_3mic_ns3);
#endif

#if defined(SPEECH_TX_NS)
  speech_tx_ns_st =
      speech_ns_create(sample_rate, frame_len, &speech_cfg->tx_ns);
#if defined(SPEECH_TX_AEC)
  speech_ns_ctl(speech_tx_ns_st, SPEECH_NS_SET_AEC_STATE, speech_tx_aec_lib_st);
  int32_t echo_supress = -39;
  speech_ns_ctl(speech_tx_ns_st, SPEECH_NS_SET_AEC_SUPPRESS, &echo_supress);
#endif
#endif

#if defined(SPEECH_TX_NS2)
  speech_tx_ns2_st =
      speech_ns2_create(sample_rate, frame_len, &speech_cfg->tx_ns2);
#if defined(SPEECH_TX_AEC)
  speech_ns2_set_echo_state(speech_tx_ns2_st, speech_tx_aec_lib_st);
  speech_ns2_set_echo_suppress(speech_tx_ns2_st, -40);
#endif
#endif

#if defined(SPEECH_TX_NS2FLOAT)
  speech_tx_ns2float_st =
      speech_ns2float_create(sample_rate, frame_len, SPEECH_TX_NS2FLOAT_CORE,
                             &speech_cfg->tx_ns2float);
#if defined(SPEECH_TX_AEC)
  speech_ns2float_set_echo_state(speech_tx_ns2float_st, speech_tx_aec_lib_st);
  speech_ns2float_set_echo_suppress(speech_tx_ns2float_st, -40);
#endif
#endif

#if defined(SPEECH_TX_NS3)
  speech_tx_ns3_st = ns3_create(sample_rate, frame_len, &speech_cfg->tx_ns3);
#endif

#if defined(SPEECH_TX_WNR)
  speech_tx_wnr_st = wnr_create(sample_rate, frame_len, &speech_cfg->tx_wnr);
#endif

#if defined(SPEECH_TX_NOISE_GATE)
  speech_tx_noise_gate_st = speech_noise_gate_create(
      sample_rate, frame_len, &speech_cfg->tx_noise_gate);
#endif

#if defined(SPEECH_TX_COMPEXP)
  speech_tx_compexp_st =
      compexp_create(sample_rate, frame_len, &speech_cfg->tx_compexp);
#endif

#if defined(SPEECH_TX_AGC)
  speech_tx_agc_st =
      agc_state_create(sample_rate, frame_len, &speech_cfg->tx_agc);
#endif

#if defined(SPEECH_TX_EQ)
  speech_tx_eq_st = eq_init(sample_rate, frame_len, &speech_cfg->tx_eq);
#endif

#if defined(SPEECH_TX_POST_GAIN)
  speech_tx_post_gain_st =
      speech_gain_create(sample_rate, frame_len, &speech_cfg->tx_post_gain);
#endif

  TRACE(1, "[%s] End", __func__);

  return 0;
}

int speech_rx_init(int sample_rate, int frame_len) {
  TRACE(3, "[%s] Start, sample_rate: %d, frame_len: %d", __func__, sample_rate,
        frame_len);

#if defined(SPEECH_RX_NS)
  speech_rx_ns_st =
      speech_ns_create(sample_rate, frame_len, &speech_cfg->rx_ns);
#endif

#if defined(SPEECH_RX_NS2)
  speech_rx_ns2_st =
      speech_ns2_create(sample_rate, frame_len, &speech_cfg->rx_ns2);
#endif

#if defined(SPEECH_RX_NS2FLOAT)
  speech_rx_ns2float_st =
      speech_ns2float_create(sample_rate, frame_len, SPEECH_RX_NS2FLOAT_CORE,
                             &speech_cfg->rx_ns2float);
#endif

#if defined(SPEECH_RX_NS3)
  speech_rx_ns3_st = ns3_create(sample_rate, frame_len, &speech_cfg->rx_ns3);
#endif

#if defined(SPEECH_RX_AGC)
  speech_rx_agc_st =
      agc_state_create(sample_rate, frame_len, &speech_cfg->rx_agc);
#endif

#if defined(SPEECH_RX_EQ)
  speech_rx_eq_st = eq_init(sample_rate, frame_len, &speech_cfg->rx_eq);
#endif

#if defined(SPEECH_RX_POST_GAIN)
  speech_rx_post_gain_st =
      speech_gain_create(sample_rate, frame_len, &speech_cfg->rx_post_gain);
#endif

  TRACE(1, "[%s] End", __func__);

  return 0;
}

int speech_init(int tx_sample_rate, int rx_sample_rate, int tx_frame_ms,
                int rx_frame_ms, int sco_frame_ms, uint8_t *buf, int len) {
  TRACE(1, "[%s] Start...", __func__);

  // ASSERT(frame_ms == SPEECH_PROCESS_FRAME_MS, "[%s] frame_ms(%d) !=
  // SPEECH_PROCESS_FRAME_MS(%d)", __func__,
  //                                                                                                frame_ms,
  //                                                                                                SPEECH_PROCESS_FRAME_MS);

  speech_tx_sample_rate = tx_sample_rate;
  speech_rx_sample_rate = rx_sample_rate;
  speech_tx_frame_len = SPEECH_FRAME_MS_TO_LEN(tx_sample_rate, tx_frame_ms);
  speech_rx_frame_len = SPEECH_FRAME_MS_TO_LEN(rx_sample_rate, rx_frame_ms);

  speech_heap_init(buf, len);

#if defined(SCO_OPTIMIZE_FOR_RAM)
  TRACE(2, "[%s] sco_overlay_ram_buf_len = %d", __func__,
        sco_overlay_ram_buf_len);
  if (sco_overlay_ram_buf_len >= 1024) {
    speech_heap_add_block(sco_overlay_ram_buf, sco_overlay_ram_buf_len);
  }
#endif

  // When phone call again, speech cfg will be default.
  // If do not want to reset speech cfg, need define bt_sco_chain_init()
  // and call in apps.cpp: app_init()
  speech_cfg = (SpeechConfig *)speech_calloc(1, sizeof(SpeechConfig));
  speech_store_config(&speech_cfg_default);

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_init(speech_tx_frame_len, sizeof(short), 2);
#endif

#ifdef AUDIO_DEBUG_V0_1_0
  speech_tuning_open();
#endif

  int aec_enable = 0;
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                       \
    defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
  aec_enable = 1;
#endif

  int capture_sample_size = sizeof(int16_t),
      playback_sample_size = sizeof(int16_t);
#if defined(SPEECH_TX_24BIT)
  capture_sample_size = sizeof(int32_t);
#endif
#if defined(SPEECH_RX_24BIT)
  playback_sample_size = sizeof(int32_t);
#endif

  CAPTURE_HANDLER_T tx_handler =
      (tx_frame_ms == sco_frame_ms) ? NULL : _speech_tx_process_;
  PLAYBACK_HANDLER_T rx_handler =
      (rx_frame_ms == sco_frame_ms) ? NULL : _speech_rx_process_;

  speech_tx_frame_resizer_enable = (tx_handler != NULL);
  speech_rx_frame_resizer_enable = (rx_handler != NULL);

  if (speech_tx_frame_resizer_enable || speech_rx_frame_resizer_enable) {
    speech_frame_resize_st = frame_resize_create(
        SPEECH_FRAME_MS_TO_LEN(tx_sample_rate, sco_frame_ms),
        SPEECH_TX_CHANNEL_NUM, speech_tx_frame_len, capture_sample_size,
        playback_sample_size, aec_enable, tx_handler, rx_handler);
  }

#if defined(SCO_CP_ACCEL)
  // NOTE: change channel number for different case.
  sco_cp_init(speech_tx_frame_len, 1);
#endif

  speech_tx_init(speech_tx_sample_rate, speech_tx_frame_len);
  speech_rx_init(speech_rx_sample_rate, speech_rx_frame_len);

#if !defined(SCO_CP_ACCEL)
  int needed_freq = 0;
  enum APP_SYSFREQ_FREQ_T min_system_freq =
      speech_get_proper_sysfreq(&needed_freq);
  enum APP_SYSFREQ_FREQ_T freq = hal_sysfreq_get();

  if (freq < min_system_freq) {
    freq = min_system_freq;

    app_sysfreq_req(APP_SYSFREQ_USER_BT_SCO, freq);

    int system_freq = hal_sys_timer_calc_cpu_freq(5, 0);
    TRACE(2, "[%s]: app_sysfreq_req %d", __FUNCTION__, freq);
    TRACE(2, "[%s]: sys freq calc : %d\n", __FUNCTION__, system_freq);

    if (system_freq <= needed_freq * 1000 * 1000) {
      TRACE(3,
            "[%s] WARNING: system freq(%d) seems to be lower than needed(%d).",
            __FUNCTION__, system_freq / 1000000, needed_freq);
    }
  }
#endif

  TRACE(1, "[%s] End", __func__);

  return 0;
}

int speech_tx_deinit(void) {
  TRACE(1, "[%s] Start...", __func__);

#if defined(SPEECH_TX_POST_GAIN)
  speech_gain_destroy(speech_tx_post_gain_st);
#endif

#if defined(SPEECH_TX_EQ)
  eq_destroy(speech_tx_eq_st);
#endif

#if defined(SPEECH_TX_AGC)
  agc_state_destroy(speech_tx_agc_st);
#endif

#if defined(SPEECH_TX_3MIC_NS)
  speech_3mic_ns_destroy(speech_tx_3mic_ns_st);
#endif

#if defined(SPEECH_TX_3MIC_NS3)
  triple_mic_denoise3_destroy(speech_tx_3mic_ns3_st);
#endif

#if defined(SPEECH_TX_2MIC_NS)
  dual_mic_denoise_deinit();
#endif

#if defined(SPEECH_TX_2MIC_NS2)
  speech_2mic_ns2_destroy(speech_tx_2mic_ns2_st);
#endif

  // #if defined(SPEECH_CS_VAD)
  //     VAD_destroy(speech_cs_vad_st);
  // #endif

#if defined(SPEECH_TX_2MIC_NS3)
  far_field_speech_enhancement_deinit();
#endif

#if defined(SPEECH_TX_2MIC_NS4)
  sensormic_denoise_destroy(speech_tx_2mic_ns4_st);
#endif

#if defined(SPEECH_TX_2MIC_NS5)
  leftright_denoise_destroy(speech_tx_2mic_ns5_st);
#endif

#if defined(SPEECH_TX_2MIC_NS6)
  speech_ff_2mic_ns2_destroy(speech_tx_2mic_ns6_st);
#endif

#if defined(SPEECH_TX_NS)
  speech_ns_destroy(speech_tx_ns_st);
#endif

#if defined(SPEECH_TX_NS2)
  speech_ns2_destroy(speech_tx_ns2_st);
#endif

#if defined(SPEECH_TX_NS2FLOAT)
  speech_ns2float_destroy(speech_tx_ns2float_st);
#endif

#ifdef SPEECH_TX_NS3
  ns3_destroy(speech_tx_ns3_st);
#endif

#ifdef SPEECH_TX_WNR
  wnr_destroy(speech_tx_wnr_st);
#endif

#ifdef SPEECH_TX_NOISE_GATE
  speech_noise_gate_destroy(speech_tx_noise_gate_st);
#endif

#ifdef SPEECH_TX_COMPEXP
  compexp_destroy(speech_tx_compexp_st);
#endif

#if defined(SPEECH_TX_AEC)
  speech_aec_destroy(speech_tx_aec_st);
#endif

#if defined(SPEECH_TX_AEC2)
  speech_aec2_destroy(speech_tx_aec2_st);
#endif

#if defined(SPEECH_TX_AEC3)
  SubBandAec_destroy(speech_tx_aec3_st);
#endif

#if defined(SPEECH_TX_AEC2FLOAT)
  ec2float_destroy(speech_tx_aec2float_st);
#endif

#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                       \
    defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
  speech_free(aec_echo_buf_ptr);
  speech_free(aec_out_buf);
#endif

#if defined(SPEECH_TX_MIC_CALIBRATION)
  speech_iir_calib_destroy(speech_tx_mic_calib_st);
#endif

#if defined(SPEECH_TX_MIC_FIR_CALIBRATION)
  speech_fir_calib_destroy(speech_tx_mic_fir_calib_st);
#endif

#if defined(SPEECH_TX_DC_FILTER)
  speech_dc_filter_destroy(speech_tx_dc_filter_st);
#endif

  TRACE(1, "[%s] End", __func__);

  return 0;
}

int speech_rx_deinit(void) {
  TRACE(1, "[%s] Start...", __func__);

#if defined(SPEECH_RX_POST_GAIN)
  speech_gain_destroy(speech_rx_post_gain_st);
#endif

#if defined(SPEECH_RX_EQ)
  eq_destroy(speech_rx_eq_st);
#endif

#if defined(SPEECH_RX_AGC)
  agc_state_destroy(speech_rx_agc_st);
#endif

#if defined(SPEECH_RX_NS)
  speech_ns_destroy(speech_rx_ns_st);
#endif

#if defined(SPEECH_RX_NS2)
  speech_ns2_destroy(speech_rx_ns2_st);
#endif

#if defined(SPEECH_RX_NS2FLOAT)
  speech_ns2float_destroy(speech_rx_ns2float_st);
#endif

#ifdef SPEECH_RX_NS3
  ns3_destroy(speech_rx_ns3_st);
#endif

  TRACE(1, "[%s] End", __func__);

  return 0;
}

int speech_deinit(void) {
  TRACE(1, "[%s] Start...", __func__);

  speech_rx_deinit();
  speech_tx_deinit();

#if defined(SCO_CP_ACCEL)
  sco_cp_deinit();
#endif

  if (speech_frame_resize_st != NULL) {
    frame_resize_destroy(speech_frame_resize_st);
    speech_tx_frame_resizer_enable = false;
    speech_rx_frame_resizer_enable = false;
  }

#ifdef AUDIO_DEBUG_V0_1_0
  speech_tuning_close();
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_deinit();
#endif

  speech_free(speech_cfg);
  speech_cfg = NULL;

  size_t total = 0, used = 0, max_used = 0;
  speech_memory_info(&total, &used, &max_used);
  TRACE(3, "SPEECH MALLOC MEM: total - %d, used - %d, max_used - %d.", total,
        used, max_used);
  ASSERT(used == 0, "[%s] used != 0", __func__);

  TRACE(1, "[%s] End", __func__);

  return 0;
}

float speech_tx_get_required_mips(void) {
  float mips = 0;

#if defined(SPEECH_TX_DC_FILTER)
  mips += speech_dc_filter_get_required_mips(speech_tx_dc_filter_st);
#endif

#if defined(SPEECH_TX_MIC_CALIBRATION)
  mips += speech_iir_calib_get_required_mips(speech_tx_mic_calib_st);
#endif

#if defined(SPEECH_TX_MIC_FIR_CALIBRATION)
  mips += speech_fir_calib_get_required_mips(speech_tx_mic_fir_calib_st);
#endif

#if defined(SPEECH_TX_AEC)
  mips += speech_aec_get_required_mips(speech_tx_aec_st);
#endif

#if defined(SPEECH_TX_AEC2)
  mips += speech_aec2_get_required_mips(speech_tx_aec2_st);
#endif

#if defined(SPEECH_TX_AEC3)
  mips += SubBandAec_get_required_mips(speech_tx_aec3_st);
#endif

#if defined(SPEECH_TX_AEC2FLOAT)
  mips += ec2float_get_required_mips(speech_tx_aec2float_st);
#endif

#if defined(SPEECH_TX_2MIC_NS)
  mips += dual_mic_denoise_get_required_mips();
#endif

#if defined(SPEECH_TX_2MIC_NS2)
  mips += speech_2mic_ns2_get_required_mips(speech_tx_2mic_ns2_st);
#endif

  // #if defined(SPEECH_CS_VAD)
  //     mips += VAD_process_get_required_mips(speech_cs_vad_st);
  // #endif

#if defined(SPEECH_TX_2MIC_NS3)
  mips += far_field_speech_enhancement_get_required_mips();
#endif

#if defined(SPEECH_TX_2MIC_NS5)
  mips += leftright_denoise_get_required_mips(speech_tx_2mic_ns5_st);
#endif

#if defined(SPEECH_TX_2MIC_NS6)
  mips += speech_ff_2mic_ns2_get_required_mips(speech_tx_2mic_ns6_st);
#endif

#if defined(SPEECH_TX_2MIC_NS4)
  mips += sensormic_denoise_get_required_mips(speech_tx_2mic_ns4_st);
#endif

#if defined(SPEECH_TX_3MIC_NS)
  mips += speech_3mic_get_required_mips(speech_tx_3mic_ns_st);
#endif

#if defined(SPEECH_TX_3MIC_NS3)
  mips += triple_mic_denoise3_get_required_mips(speech_tx_3mic_ns3_st);
#endif

#if defined(SPEECH_TX_NS)
  mips += speech_ns_get_required_mips(speech_tx_ns_st);
#endif

#if defined(SPEECH_TX_NS2)
  mips += speech_ns2_get_required_mips(speech_tx_ns2_st);
#endif

#if defined(SPEECH_TX_NS2FLOAT)
  mips += speech_ns2float_get_required_mips(speech_tx_ns2float_st);
#endif

#if defined(SPEECH_TX_NS3)
  mips += ns3_get_required_mips(speech_tx_ns3_st);
#endif

#if defined(SPEECH_TX_WNR)
  mips += wnr_get_required_mips(speech_tx_wnr_st);
#endif

#if defined(SPEECH_TX_NOISE_GATE)
  mips += speech_noise_gate_get_required_mips(speech_tx_noise_gate_st);
#endif

#if defined(SPEECH_TX_COMPEXP)
  mips += compexp_get_required_mips(speech_tx_compexp_st);
#endif

#if defined(SPEECH_TX_AGC)
  mips += agc_get_required_mips(speech_tx_agc_st);
#endif

#if defined(SPEECH_TX_EQ)
  mips += eq_get_required_mips(speech_tx_eq_st);
#endif

#if defined(SPEECH_TX_POST_GAIN)
  mips += speech_gain_get_required_mips(speech_tx_post_gain_st);
#endif

  return mips;
}

float speech_rx_get_required_mips(void) {
  float mips = 0;

#if defined(SPEECH_RX_NS)
  mips += speech_ns_get_required_mips(speech_rx_ns_st);
#endif

#if defined(SPEECH_RX_NS2)
  mips += speech_ns2_get_required_mips(speech_rx_ns2_st);
#endif

#if defined(SPEECH_RX_NS2FLOAT)
  mips += speech_ns2float_get_required_mips(speech_rx_ns2float_st);
#endif

#if defined(SPEECH_RX_NS3)
  mips += ns3_get_required_mips(speech_rx_ns3_st);
#endif

#if defined(SPEECH_RX_AGC)
  mips += agc_get_required_mips(speech_rx_agc_st);
#endif

#if defined(SPEECH_RX_EQ)
  mips += eq_get_required_mips(speech_rx_eq_st);
#endif

#if defined(SPEECH_RX_POST_GAIN)
  mips += speech_gain_get_required_mips(speech_rx_post_gain_st);
#endif

  return mips;
}

float speech_get_required_mips(void) {
  return speech_tx_get_required_mips() + speech_rx_get_required_mips();
}

#define SYSTEM_BASE_MIPS (18)

enum APP_SYSFREQ_FREQ_T speech_get_proper_sysfreq(int *needed_mips) {
  enum APP_SYSFREQ_FREQ_T freq = APP_SYSFREQ_32K;
  int required_mips = (int)ceilf(speech_get_required_mips() + SYSTEM_BASE_MIPS);

  if (required_mips >= 104)
    freq = APP_SYSFREQ_208M;
  else if (required_mips >= 78)
    freq = APP_SYSFREQ_104M;
  else if (required_mips >= 52)
    freq = APP_SYSFREQ_78M;
  else if (required_mips >= 26)
    freq = APP_SYSFREQ_52M;
  else
    freq = APP_SYSFREQ_26M;

  *needed_mips = required_mips;

  return freq;
}

int speech_set_config(const SpeechConfig *cfg) {
#if defined(SPEECH_TX_DC_FILTER)
  speech_dc_filter_set_config(speech_tx_dc_filter_st, &cfg->tx_dc_filter);
#endif
#if defined(SPEECH_TX_AEC)
  speech_aec_set_config(speech_tx_aec_st, &cfg->tx_aec);
#endif
#if defined(SPEECH_TX_AEC2)
  speech_aec2_set_config(speech_tx_aec2_st, &cfg->tx_aec2);
#endif
#if defined(SPEECH_TX_AEC2FLOAT)
  ec2float_set_config(speech_tx_aec2float_st, &cfg->tx_aec2float,
                      SPEECH_TX_AEC2FLOAT_CORE);
#endif
#if defined(SPEECH_TX_2MIC_NS)
  TRACE(1, "[%s] WARNING: Can not support tuning SPEECH_TX_2MIC_NS", __func__);
#endif
#if defined(SPEECH_TX_2MIC_NS2)
  speech_2mic_ns2_set_config(speech_tx_2mic_ns2_st, &cfg->tx_2mic_ns2);
#endif
#if defined(SPEECH_TX_2MIC_NS4)
  sensormic_denoise_set_config(speech_tx_2mic_ns4_st, &cfg->tx_2mic_ns4);
#endif
#if defined(SPEECH_TX_2MIC_NS5)
  leftright_denoise_set_config(speech_tx_2mic_ns5_st, &cfg->tx_2mic_ns5);
#endif
#if defined(SPEECH_TX_3MIC_NS)
  speech_3mic_ns_set_config(speech_tx_3mic_ns_st, &cfg->tx_3mic_ns);
#endif
#if defined(SPEECH_TX_NS)
  speech_ns_set_config(speech_tx_ns_st, &cfg->tx_ns);
#endif
#if defined(SPEECH_TX_NS2)
  speech_ns2_set_config(speech_tx_ns2_st, &cfg->tx_ns2);
#endif
#if defined(SPEECH_TX_NS2FLOAT)
  speech_ns2float_set_config(speech_tx_ns2float_st, &cfg->tx_ns2float);
#endif
#if defined(SPEECH_TX_NS3)
  ns3_set_config(speech_tx_ns3_st, &cfg->tx_ns3);
#endif
#if defined(SPEECH_TX_NOISE_GATE)
  speech_noise_gate_set_config(speech_tx_noise_gate_st, &cfg->tx_noise_gate);
#endif
#if defined(SPEECH_TX_COMPEXP)
  compexp_set_config(speech_tx_compexp_st, &cfg->tx_compexp);
#endif
#if defined(SPEECH_TX_AGC)
  agc_set_config(speech_tx_agc_st, &cfg->tx_agc);
#endif
#if defined(SPEECH_TX_EQ)
  eq_set_config(speech_tx_eq_st, &cfg->tx_eq);
#endif
#if defined(SPEECH_TX_POST_GAIN)
  speech_gain_set_config(speech_tx_post_gain_st, &cfg->tx_post_gain);
#endif
// #if defined(SPEECH_CS_VAD)
//     VAD_set_config(speech_cs_vad_st, &cfg->cs_vad);
// #endif
#if defined(SPEECH_RX_NS)
  speech_ns_set_config(speech_rx_ns_st, &cfg->rx_ns);
#endif
#if defined(SPEECH_RX_NS2)
  speech_ns2_set_config(speech_rx_ns2_st, &cfg->rx_ns2);
#endif
#if defined(SPEECH_RX_NS2FLOAT)
  speech_ns2float_set_config(speech_rx_ns2float_st, &cfg->rx_ns2float,
                             SPEECH_RX_NS2FLOAT_CORE);
#endif
#if defined(SPEECH_RX_NS3)
  ns3_set_config(speech_rx_ns3_st, &cfg->rx_ns3);
#endif
#if defined(SPEECH_RX_NOISE_GATE)
  speech_noise_gate_set_config(speech_rx_noise_gate_st, &cfg->rx_noise_gate);
#endif
#if defined(SPEECH_RX_COMPEXP)
  compexp_set_config(speech_rx_compexp_st, &cfg->rx_compexp);
#endif
#if defined(SPEECH_RX_AGC)
  agc_set_config(speech_rx_agc_st, &cfg->rx_agc);
#endif
#if defined(SPEECH_RX_EQ)
  eq_set_config(speech_rx_eq_st, &cfg->rx_eq);
#endif
#if defined(SPEECH_RX_POST_GAIN)
  speech_gain_set_config(speech_rx_post_gain_st, &cfg->rx_post_gain);
#endif
  // Add more process

  return 0;
}

void _speech_tx_process_pre(short *pcm_buf, short *ref_buf, int *_pcm_len) {
  int pcm_len = *_pcm_len;

#if defined(BT_SCO_CHAIN_PROFILE)
  tx_start_ticks = hal_fast_sys_timer_get();
#endif

  // TRACE(2,"[%s] pcm_len = %d", __func__, pcm_len);

#ifdef AUDIO_DEBUG_V0_1_0
  if (speech_tuning_get_status()) {
    speech_set_config(speech_cfg);

    speech_tuning_set_status(false);

    // If has MIPS problem, can move this block code into speech_rx_process or
    // return directly return 0
  }
#endif

#if defined(SPEECH_TX_24BIT)
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                       \
    defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
  tx_refbuf16 = (int16_t *)ref_buf;
  tx_refbuf32 = (int32_t *)ref_buf;
  for (int i = 0; i < pcm_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM; i++) {
    tx_refbuf16[i] = (tx_refbuf32[i] >> 8);
  }
#endif

  tx_pcmbuf16 = (int16_t *)pcm_buf;
  tx_pcmbuf32 = (int32_t *)pcm_buf;
  for (int i = 0; i < pcm_len; i++) {
    tx_pcmbuf16[i] = (tx_pcmbuf32[i] >> 8);
  }
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_clear_up();
#endif

#if 0
    // Test MIC: Get one channel data
    // Enable this, should bypass SPEECH_TX_2MIC_NS and SPEECH_TX_2MIC_NS2
    for(uint32_t i=0; i<pcm_len/2; i++)
    {
        pcm_buf[i] = pcm_buf[2*i];      // Left channel
        // pcm_buf[i] = pcm_buf[2*i+1]; // Right channel
    }

    pcm_len >>= 1;
#endif

#if defined(SPEECH_TX_DC_FILTER)
  speech_dc_filter_process(speech_tx_dc_filter_st, pcm_buf, pcm_len);
#endif

#if (SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2) && defined(AUDIO_DUMP) &&          \
    defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_add_channel_data_from_multi_channels(0, pcm_buf, pcm_len / 2, 2,
                                                  0);
  audio_dump_add_channel_data_from_multi_channels(1, pcm_buf, pcm_len / 2, 2,
                                                  1);
#endif

#if defined(SPEECH_TX_MIC_CALIBRATION)
  speech_iir_calib_process(speech_tx_mic_calib_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_MIC_FIR_CALIBRATION)
  speech_fir_calib_process(speech_tx_mic_fir_calib_st, pcm_buf, pcm_len);
#endif

#if (SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2) && defined(AUDIO_DUMP) &&          \
    defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_add_channel_data_from_multi_channels(2, pcm_buf, pcm_len / 2, 2,
                                                  0);
  audio_dump_add_channel_data_from_multi_channels(3, pcm_buf, pcm_len / 2, 2,
                                                  1);
#endif

#if defined(SPEECH_TX_2MIC_NS)
  dual_mic_denoise_run(pcm_buf, pcm_len, pcm_buf);
  // Channel num: two-->one
  pcm_len >>= 1;
#endif

#if defined(SPEECH_TX_2MIC_NS2)
  speech_2mic_ns2_process(speech_tx_2mic_ns2_st, pcm_buf, pcm_len, pcm_buf);
  // Channel num: two-->one
  pcm_len >>= 1;
#endif

#if defined(SPEECH_TX_2MIC_NS4)
  if (dualmic_enable == true) {
#if defined(ANC_APP)
    sensormic_denoise_set_anc_status(speech_tx_2mic_ns4_st,
                                     app_anc_work_status());
#endif
    sensormic_denoise_process(speech_tx_2mic_ns4_st, pcm_buf, pcm_len, pcm_buf);
  } else {
    int16_t *pcm16 = (int16_t *)pcm_buf;
    for (int i = 0, j = 0; i < pcm_len / 2; i++, j += 2)
      pcm16[i] = pcm16[j];
  }
  // Channel num: two-->one
  pcm_len >>= 1;
#endif

#if defined(SPEECH_TX_2MIC_NS5)
  leftright_denoise_process(speech_tx_2mic_ns5_st, pcm_buf, pcm_len, pcm_buf);
  // Channel num: two-->one
  pcm_len >>= 1;
#endif

#if defined(SPEECH_TX_2MIC_NS6)
  // TRACE(0,"NS6");
  speech_2mic_ns6_process(speech_tx_2mic_ns6_st, pcm_buf, pcm_len, pcm_buf);
  // Channel num: two-->one
  pcm_len >>= 1;
#endif

#if defined(SPEECH_TX_3MIC_NS)
  speech_3mic_ns_process(speech_tx_3mic_ns_st, pcm_buf, pcm_len, pcm_buf);
  // Channel num: three-->one
  pcm_len = pcm_len / 3;
#endif

#if defined(SPEECH_TX_3MIC_NS3)
  triple_mic_denoise3_process(speech_tx_3mic_ns3_st, pcm_buf, pcm_len, pcm_buf);
  // Channel num: three-->one
  pcm_len = pcm_len / 3;
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_add_channel_data(0, pcm_buf, pcm_len);
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                       \
    defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
  audio_dump_add_channel_data(1, ref_buf, pcm_len);
#else
  audio_dump_add_channel_data(1, pcm_buf, pcm_len);
#endif
#endif

#if defined(SPEECH_TX_AEC)
  speech_aec_process(speech_tx_aec_st, pcm_buf, ref_buf, pcm_len, aec_out_buf);
  speech_copy_int16(pcm_buf, aec_out_buf, pcm_len);
#endif

#if defined(SPEECH_TX_AEC2)
  speech_aec2_process(speech_tx_aec2_st, pcm_buf, ref_buf, pcm_len);
#endif

#if defined(SPEECH_TX_AEC3)
  CODEC_OpVecCpy(bufferstate + delay, ref_buf, pcm_len);
  CODEC_OpVecCpy(buf_out, bufferstate, pcm_len);
  CODEC_OpVecCpy(bufferstate, bufferstate + pcm_len, delay);
  SubBandAec_process(speech_tx_aec3_st, pcm_buf, buf_out, pcm_buf, pcm_len);
  // audio_dump_add_channel_data(1, pcm_buf, pcm_len);
#endif

  SCO_CP_ACCEL_ALGO_START();

#if defined(SPEECH_TX_AEC2FLOAT)
#if defined(SPEECH_TX_2MIC_NS4)
  if (dualmic_enable == true)
    TRACE(2, "[%s] vad: %d", __FUNCTION__,
          sensormic_denoise_get_vad(speech_tx_2mic_ns4_st));
  ec2float_set_external_vad(speech_tx_aec2float_st,
                            sensormic_denoise_get_vad(speech_tx_2mic_ns4_st));
#endif
  ec2float_process(speech_tx_aec2float_st, pcm_buf, ref_buf, pcm_len,
                   aec_out_buf);
  speech_copy_int16(pcm_buf, aec_out_buf, pcm_len);
#endif

  SCO_CP_ACCEL_ALGO_END();

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_add_channel_data(2, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_NS)
  speech_ns_process(speech_tx_ns_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_NS2)
  speech_ns2_process(speech_tx_ns2_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_NS2FLOAT)
#if defined(SPEECH_TX_2MIC_NS4)
  if (dualmic_enable == true)
    speech_ns2float_set_external_vad(
        speech_tx_ns2float_st,
        sensormic_denoise_get_vad(speech_tx_2mic_ns4_st));
#endif
  speech_ns2float_process(speech_tx_ns2float_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_NS3)
  ns3_process(speech_tx_ns3_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_WNR)
  wnr_process(speech_tx_wnr_st, pcm_buf, pcm_len);
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_add_channel_data(3, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_NOISE_GATE)
  speech_noise_gate_process(speech_tx_noise_gate_st, pcm_buf, pcm_len);
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  audio_dump_add_channel_data(4, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_COMPEXP)
  compexp_process(speech_tx_compexp_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_AGC)
  agc_process(speech_tx_agc_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_EQ)
  eq_process(speech_tx_eq_st, pcm_buf, pcm_len);
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  // audio_dump_add_channel_data(0, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_TX_POST_GAIN)
  speech_gain_process(speech_tx_post_gain_st, pcm_buf, pcm_len);
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  // audio_dump_add_channel_data(1, pcm_buf, pcm_len);

  audio_dump_add_channel_data(5, pcm_buf, pcm_len);
  audio_dump_run();
#endif

#if defined(SPEECH_TX_24BIT)
  tx_pcmbuf16 = (int16_t *)pcm_buf;
  tx_pcmbuf32 = (int32_t *)pcm_buf;
  for (int i = pcm_len - 1; i >= 0; i--) {
    tx_pcmbuf32[i] = ((int32_t)tx_pcmbuf16[i] << 8);
  }
#endif

  *_pcm_len = pcm_len;

#if defined(BT_SCO_CHAIN_PROFILE)
  tx_end_ticks = hal_fast_sys_timer_get();
  TRACE(2, "[%s] takes %d us", __FUNCTION__,
        FAST_TICKS_TO_US(tx_end_ticks - tx_start_ticks));
#endif
}

#if defined(SCO_CP_ACCEL)
CP_TEXT_SRAM_LOC
int sco_cp_algo(short *pcm_buf, short *ref_buf, int *_pcm_len) {
#if defined(SCO_TRACE_CP_ACCEL)
  TRACE(1, "[%s] ...", __func__);
#endif

  _speech_tx_process_mid(pcm_buf, ref_buf, _pcm_len);

  return 0;
}
#endif

int32_t _speech_tx_process_(void *pcm_buf, void *ref_buf, int32_t *_pcm_len) {
  _speech_tx_process_pre(pcm_buf, ref_buf, (int *)_pcm_len);
#if defined(SCO_CP_ACCEL)
  sco_cp_process(pcm_buf, ref_buf, (int *)_pcm_len);
  _speech_tx_process_post(pcm_buf, ref_buf, (int *)_pcm_len);
#endif

  return 0;
}

int32_t _speech_rx_process_(void *pcm_buf, int32_t *_pcm_len) {
  int32_t pcm_len = *_pcm_len;

#if defined(BT_SCO_CHAIN_PROFILE)
  uint32_t start_ticks = hal_fast_sys_timer_get();
#endif

#if defined(SPEECH_RX_24BIT)
  int32_t *buf32 = (int32_t *)pcm_buf;
  int16_t *buf16 = (int16_t *)pcm_buf;
  for (int i = 0; i < pcm_len; i++) {
    buf16[i] = (int16_t)(buf32[i] >> 8);
  }
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  // audio_dump_add_channel_data(0, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_RX_NS)
  speech_ns_process(speech_rx_ns_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_RX_NS2)
  // fix 0dB signal
  int16_t *pcm_buf16 = (int16_t *)pcm_buf;
  for (int i = 0; i < pcm_len; i++) {
    pcm_buf16[i] = (int16_t)(pcm_buf16[i] * 0.94);
  }
  speech_ns2_process(speech_rx_ns2_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_RX_NS2FLOAT)
  // FIXME
  int16_t *pcm_buf16 = (int16_t *)pcm_buf;
  for (int i = 0; i < pcm_len; i++) {
    pcm_buf16[i] = (int16_t)(pcm_buf16[i] * 0.94);
  }
  speech_ns2float_process(speech_rx_ns2float_st, pcm_buf, pcm_len);
#endif

#ifdef SPEECH_RX_NS3
  ns3_process(speech_rx_ns3_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_RX_AGC)
  agc_process(speech_rx_agc_st, pcm_buf, pcm_len);
#endif

#if defined(SPEECH_RX_24BIT)
  for (int i = pcm_len - 1; i >= 0; i--) {
    buf32[i] = ((int32_t)buf16[i] << 8);
  }
#endif

#if defined(SPEECH_RX_EQ)
#if defined(SPEECH_RX_24BIT)
  eq_process_int24(speech_rx_eq_st, pcm_buf, pcm_len);
#else
  eq_process(speech_rx_eq_st, pcm_buf, pcm_len);
#endif
#endif

#if defined(SPEECH_RX_POST_GAIN)
  speech_gain_process(speech_rx_post_gain_st, pcm_buf, pcm_len);
#endif

#if defined(BT_SCO_CHAIN_AUDIO_DUMP)
  // audio_dump_add_channel_data(1, pcm_buf, pcm_len);
  // audio_dump_run();
#endif

  *_pcm_len = pcm_len;

#if defined(BT_SCO_CHAIN_PROFILE)
  uint32_t end_ticks = hal_fast_sys_timer_get();
  TRACE(2, "[%s] takes %d us", __FUNCTION__,
        FAST_TICKS_TO_US(end_ticks - start_ticks));
#endif

  return 0;
}

int speech_tx_process(void *pcm_buf, void *ref_buf, int *pcm_len) {
  if (speech_tx_frame_resizer_enable == false) {
    _speech_tx_process_(pcm_buf, ref_buf, (int32_t *)pcm_len);
  } else {
    // MUST use (int32_t *)??????
    frame_resize_process_capture(speech_frame_resize_st, pcm_buf, ref_buf,
                                 (int32_t *)pcm_len);
  }

  return 0;
}

int speech_rx_process(void *pcm_buf, int *pcm_len) {
  if (speech_rx_frame_resizer_enable == false) {
    _speech_rx_process_(pcm_buf, (int32_t *)pcm_len);
  } else {
    frame_resize_process_playback(speech_frame_resize_st, pcm_buf,
                                  (int32_t *)pcm_len);
  }

  return 0;
}
