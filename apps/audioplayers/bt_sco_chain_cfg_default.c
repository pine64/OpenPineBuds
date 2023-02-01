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
#include "bt_sco_chain_cfg.h"
#include "plat_types.h"

#if defined(SPEECH_TX_2MIC_NS4)
static float ff_fb_h[256] = {
    1.f,
};
#endif

#if defined(SPEECH_TX_MIC_CALIBRATION)
/****************************************************************************************************
 * Describtion:
 *     Mic Calibration Equalizer, implementation with 2 order iir filters
 * Parameters:
 *     bypass(0/1): bypass enable or disable.
 *     mic_num(1~4): the number of microphones. The filter num is (mic_num - 1)
 *     calib: {bypass, gain, num, {type, frequency, gain, q}}. Please refer to
 *SPEECH_TX_EQ section in this file Resource consumption: RAM:     None FLASE:
 *None MIPS:    fs = 16kHz, 0.5M/section; Note: None
 ****************************************************************************************************/
const SpeechIirCalibConfig WEAK speech_tx_mic_calib_cfg = {
    .bypass = 0,
    .mic_num = 2,
    .calib =
        {
            {.bypass = 0,
             .gain = 0.f,
             .num = 0,
             .params =
                 {
                     {IIR_BIQUARD_LOWSHELF, 150, -2.5, 0.707},
                 }},
        },
};
#endif

#if defined(SPEECH_TX_MIC_FIR_CALIBRATION)
/****************************************************************************************************
 * Describtion:
 *     Mic Calibration Equalizer, implementation with fir filter
 * Parameters:
 *     bypass(0/1): bypass enable or disable.
 *     mic_num(1~4): the number of microphones. The filter num is (mic_num - 1)
 *     calib: {filter, filter_length, nfft}.
 * Resource consumption:
 *     RAM:     None
 *     FLASE:   None
 *     MIPS:    fs = 16kHz, 0.5M/section;
 * Note:
 *     None
 ****************************************************************************************************/
float mic2_ft_caliration[256] = {
    1.f,
};
const SpeechFirCalibConfig WEAK speech_tx_mic_fir_calib_cfg = {
    .bypass = 0,
    .mic_num = SPEECH_CODEC_CAPTURE_CHANNEL_NUM,
    .delay = 0,
    .calib =
        {
            {
                .filter = mic2_ft_caliration,
                .filter_length = ARRAY_SIZE(mic2_ft_caliration),
            },
        },
};
#endif

const SpeechConfig WEAK speech_cfg_default = {

#if defined(SPEECH_TX_DC_FILTER)
    .tx_dc_filter =
        {
            .bypass = 0,
            .gain = 0,
        },
#endif

#if defined(SPEECH_TX_AEC)
    /****************************************************************************************************
     * Describtion:
     *     Acoustic Echo Cancellation
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     delay(>0): delay samples from mic to speak, unit(sample).
     *     leak_estimate(0~32767): echo supression value. This is a fixed mode.
     *It has relatively large echo supression and large damage to speech signal.
     *     leak_estimate_shift(0~8): echo supression value. if leak_estimate ==
     *0, then leak_estimate_shift can take effect. This is a adaptive mode. It
     *has relatively small echo supression and also small damage to speech
     *signal. Resource consumption: RAM:     None FLASE:   None MIPS:    fs =
     *16kHz, 40M; Note: If possible, use SPEECH_TX_AEC2FLOAT
     ****************************************************************************************************/
    .tx_aec =
        {
            .bypass = 0,
            .delay = 60,
            .leak_estimate = 16383,
            .leak_estimate_shift = 4,
        },
#endif

#if defined(SPEECH_TX_AEC2)
    /****************************************************************************************************
     * Describtion:
     *     Acoustic Echo Cancellation
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     .
     *     .
     *     .
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz, enNlpEnable = 1, 39M;
     * Note:
     *     If possible, use SPEECH_TX_AEC2FLOAT
     ****************************************************************************************************/
    .tx_aec2 =
        {
            .bypass = 0,
            .enEAecEnable = 1,
            .enHpfEnable = 1,
            .enAfEnable = 0,
            .enNsEnable = 0,
            .enNlpEnable = 1,
            .enCngEnable = 0,
            .shwDelayLength = 0,
            .shwNlpBandSortIdx = 0.75f,
            .shwNlpBandSortIdxLow = 0.5f,
            .shwNlpTargetSupp = 3.0f,
            .shwNlpMinOvrd = 1.0f,
        },
#endif

#if defined(SPEECH_TX_AEC2FLOAT)
    /****************************************************************************************************
     * Describtion:
     *     Acoustic Echo Cancellation
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     hpf_enabled(0/1): high pass filter enable or disable. Used to remove
     *DC for Near and Ref signals. af_enabled(0/1): adaptive filter enable or
     *disable. If the echo signal is very large, enable this nlp_enabled(0/1):
     *non-linear process enable or disable. Enable this by default.
     *     ns_enabled(0/1): noise supression enable or disable. Enable this by
     *default. cng_enabled(0/1):  comfort noise generator enable or disable.
     *     blocks(1~8): the length of adaptive filter = blocks * frame length
     *     delay(>0): delay samples from mic to speak, unit(sample).
     *     min_ovrd(1~6): supression factor, min_ovrd becomes larger and echo
     *suppression becomes larger. target_supp(<0): target echo suppression,
     *unit(dB) noise_supp(-30~0): noise suppression, unit(dB) cng_type(0/1):
     *noise type(0: White noise; 1: Pink noise) cng_level(<0): noise gain,
     *unit(dB) Resource consumption: RAM:     None FLASE:   None MIPS:    fs =
     *16kHz; Note: This is the recommended AEC
     ****************************************************************************************************/
    .tx_aec2float =
        {
            .bypass = 0,
            .hpf_enabled = false,
            .af_enabled = false,
            .nlp_enabled = true,
            .clip_enabled = false,
            .stsupp_enabled = false,
            .ns_enabled = true,
            .cng_enabled = false,
            .blocks = 1,
            .delay = 70,
            .min_ovrd = 2,
            .target_supp = -40,
            .noise_supp = -15,
            .cng_type = 1,
            .cng_level = -60,
            .clip_threshold = -20.f,
            .banks = 64,
        },
#endif

#if defined(SPEECH_TX_AEC3)
    .tx_aec3 =
        {
            .bypass = 0,
            .filter_size = 16,
        },
#endif

#if defined(SPEECH_TX_2MIC_NS)
    /****************************************************************************************************
     * Describtion:
     *     2 MICs Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_2mic_ns =
        {
            .bypass = 0,
            .alpha_h = 0.8,
            .alpha_slow = 0.9,
            .alpha_fast = 0.6,
            .thegma = 0.01,
            .thre_corr = 0.2,
            .thre_filter_diff = 0.2,
            .cal_left_gain = 1.0,
            .cal_right_gain = 1.0,
            .delay_mono_sample = 6,
            .wnr_enable = 0,
        },
#endif

#if defined(SPEECH_TX_2MIC_NS2)
    /****************************************************************************************************
     * Describtion:
     *     2 MICs Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     delay_taps(0~4): ceil{[d(MIC1~mouth) - d(MIC2~mouth)] / 2}.
     *         ceil: Returns the largest integer less than or equal to the
     *specified data d(MIC~mouth): The dinstance from MIC to mouth e.g. 0:
     *0~2cm; 1: 2~4; 2: 5~6... freq_smooth_enable(1): Must enable
     *     wnr_enable(0/1): wind noise reduction enable or disable. This is also
     *useful for improving noise suppression, but it also has some damage to
     *speech signal. Resource consumption: RAM:     None FLASE:   None MIPS: fs
     *= 16kHz, 32M; Note: None
     ****************************************************************************************************/
    .tx_2mic_ns2 =
        {
            .bypass = 0,
            .delay_taps = 0.f,
            .freq_smooth_enable = 1,
            .wnr_enable = 1,
        },
#endif

#if defined(SPEECH_TX_2MIC_NS5)
    /****************************************************************************************************
     * Describtion:
     *     2 MICs Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     delay_taps(0~4): ceil{[d(MIC1~mouth) - d(MIC2~mouth)] / 2}. Default
     *as 0 ceil: Returns the largest integer less than or equal to the specified
     *data d(MIC~mouth): The dinstance from MIC to mouth e.g. 0: 0~2cm; 1: 2~4;
     *2: 5~6... freq_smooth_enable(1): Must enable wnr_enable(0/1): wind noise
     *reduction enable or disable. This is also useful for improving noise
     *suppression, but it also has some damage to speech signal.
     *     delay_enable(0/1): enable the delay_taps or not. Ideally, never need
     *to enable the delay and delay_taps will be a useless parameter. Resource
     *consumption: RAM:     None FLASE:   None MIPS:    fs = 16kHz, 32M; Note:
     *     None
     ****************************************************************************************************/
    .tx_2mic_ns5 =
        {
            .bypass = 0,
            .delay_taps = 0.0f,
            .freq_smooth_enable = 1,
            .wnr_enable = 0,
            .delay_enable = 0,
        },
#endif

#if defined(SPEECH_TX_2MIC_NS4)
    /****************************************************************************************************
     * Describtion:
     *     2 MICs Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_2mic_ns4 =
        {
            .bypass = 0,
            .blend_en = 1,
            // .left_gain          = 1.0f,
            // .right_gain         = 1.0f,
            .delay_tapsM = 0,
            .delay_tapsS = 0,
            // .delay_tapsC        = 32,
            //////////////////////////{{a0,a1,a2,a3,a4},{b0,b1,b2,b3,b4}}///////////////////////////
            // .coefH[0]           = {1.0, -1.88561808316413, 1.55555555555556,
            // -0.628539361054709, 0.111111111111111}, .coefH[1]           =
            // {0.323801506930344, -1.29520602772138, 1.94280904158206,
            // -1.29520602772138, 0.323801506930344}, .coefL[0]           =
            // {1.0, -1.88561808316413, 1.55555555555556, -0.628539361054709,
            // 0.111111111111111}, .coefL[1]           = {0.00953182640298944,
            // 0.0381273056119578, 0.0571909584179366, 0.0381273056119578,
            // 0.00953182640298944},
            .crossover_freq = 1000,
            .ff_fb_coeff = ff_fb_h,
            .ff_fb_coeff_len = ARRAY_SIZE(ff_fb_h),
        },
#endif

#if defined(SPEECH_TX_3MIC_NS)
    /****************************************************************************************************
     * Describtion:
     *     3 MICs Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     delay_tapsM(0~4): MIC L/R delay samples. Refer to SPEECH_TX_2MIC_NS2
     *delay_taps delay_tapsS(0~4): MIC L/S delay samples. Refer to
     *SPEECH_TX_2MIC_NS2 delay_taps freq_smooth_enable(1): Must enable
     *     wnr_enable(0/1): wind noise reduction enable or disable. This is also
     *useful for improving noise suppression, but it also has some damage to
     *speech signal. Resource consumption: RAM:     None FLASE:   None MIPS: fs
     *= 16kHz; Note: None
     ****************************************************************************************************/
    .tx_3mic_ns =
        {
            .bypass = 0,
            .blend_en = 1,
            .authen_en = 1,
            .delay_tapsM = 0.65f,
            .delay_tapsS = 2,
            .denoise_dB = -15.0f,
            // .coefH[0]           = {1.0,-3.3384,4.2195,-2.3924,0.5136},
            // .coefH[1]           = {0.7165,-2.8660,4.2990,-2.8660,0.7165},
            // .coefL[0]           = {1.0,-3.3384,4.2195,-2.3924,0.5136},
            // .coefL[1]           = {0.00014,0.00056,0.00084,0.00056,0.00014},
            .crossover_freq = 1000,
            .freq_smooth_enable = 1,
            .wnr_enable = 0,
        },
#endif

#if defined(SPEECH_TX_3MIC_NS3)
    /****************************************************************************************************
     * Describtion:
     *     3 MICs Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     delay_taps(0~4): ceil{[d(MIC1~mouth) - d(MIC2~mouth)] / 2}.
     *         ceil: Returns the largest integer less than or equal to the
     *specified data d(MIC~mouth): The dinstance from MIC to mouth e.g. 0:
     *0~2cm; 1: 2~4; 2: 5~6... freq_smooth_enable(1): Must enable
     *     wnr_enable(0/1): wind noise reduction enable or disable. This is also
     *useful for improving noise suppression, but it also has some damage to
     *speech signal. Resource consumption: RAM:     None FLASE:   None MIPS: fs
     *= 16kHz, 32M; Note: None
     ****************************************************************************************************/
    .tx_3mic_ns3 =
        {
            .bypass = 0,
            .endfire_enable = 1,
            .broadside_enable = 1,
            .delay_taps = 0.7f,
            .freq_smooth_enable = 1,
            .wnr_enable = 0,
        },
#endif

#if defined(SPEECH_TX_NS)
    /****************************************************************************************************
     * Describtion:
     *     Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz, 30M;
     * Note:
     *     If possible, use SPEECH_TX_NS2 or SPEECH_TX_NS2FLOAT
     ****************************************************************************************************/
    .tx_ns =
        {
            .bypass = 0,
            .denoise_dB = -12,
        },
#endif

#if defined(SPEECH_TX_NS2)
    /****************************************************************************************************
     * Describtion:
     *     Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     denoise_dB(-30~0): noise suppression, unit(dB).
     *         e.g. -15: Can reduce 15dB of stationary noise.
     * Resource consumption:
     *     RAM:     fs = 16k:   RAM = 8k;
     *              fs = 8k:    RAM = 4k;
     *              RAM = frame_size * 30
     *     FLASE:   None
     *     MIPS:    fs = 16kHz, 16M;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_ns2 =
        {
            .bypass = 0,
            .denoise_dB = -15,
        },
#endif

#if defined(SPEECH_TX_NS2FLOAT)
    /****************************************************************************************************
     * Describtion:
     *     Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     denoise_dB(-30~0): noise suppression, unit(dB).
     *         e.g. -15: Can reduce 15dB of stationary noise.
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     This is a 'float' version for SPEECH_TX_NS2.
     *     It needs more MIPS and RAM, but can redece quantization noise.
     ****************************************************************************************************/
    .tx_ns2float =
        {
            .bypass = 0,
            .denoise_dB = -15,
            .banks = 64,
        },
#endif

#if defined(SPEECH_TX_NS3)
    /****************************************************************************************************
     * Describtion:
     *     Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     mode: None
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_ns3 =
        {
            .bypass = 0,
            .mode = NS3_SUPPRESSION_HIGH,
        },
#endif

#if defined(SPEECH_TX_WNR)
    /****************************************************************************************************
     * Describtion:
     *     Wind Noise Suppression
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     lpf_cutoff: lowpass filter for wind noise detection
     *     hpf_cutoff: highpass filter for wind noise suppression
     *     power_ratio_thr: ratio of the power spectrum of the lower frequencies
    over the total power spectrum for all frequencies
     *     power_thr: normalized power of the low frequencies
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     None
    ****************************************************************************************************/
    .tx_wnr =
        {
            .bypass = 0,
            .lpf_cutoff = 1000,
            .hpf_cutoff = 400,
            .power_ratio_thr = 0.9f,
            .power_thr = 1.f,
        },
#endif

#if defined(SPEECH_TX_NOISE_GATE)
    /****************************************************************************************************
     * Describtion:
     *     Noise Gate
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     data_threshold(0~32767): distinguish between noise and speech,
     *unit(sample). data_shift(1~3): 1: -6dB; 2: -12dB; 3: -18dB
     *     factor_up(float): attack time, unit(s)
     *     factor_down(float): release time, unit(s)
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_noise_gate =
        {
            .bypass = 0,
            .data_threshold = 640,
            .data_shift = 1,
            .factor_up = 0.001f,
            .factor_down = 0.5f,
        },
#endif

#if defined(SPEECH_TX_COMPEXP)
    /****************************************************************************************************
     * Describtion:
     *     Compressor and expander
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     ...
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_compexp =
        {
            .bypass = 0,
            .comp_threshold = -10.f,
            .comp_ratio = 2.f,
            .expand_threshold = -45.f,
            .expand_ratio = 0.5556f,
            .attack_time = 0.001f,
            .release_time = 0.6f,
            .makeup_gain = 6,
            .delay = 128,
        },
#endif

#if defined(SPEECH_TX_AGC)
    /****************************************************************************************************
     * Describtion:
     *     Automatic Gain Control
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     target_level(>0): signal can not exceed (-1 * target_level)dB.
     *     compression_gain(>0): excepted gain.
     *     limiter_enable(0/1): 0: target_level does not take effect; 1:
     *target_level takes effect. Resource consumption: RAM:     None FLASE: None
     *     MIPS:    fs = 16kHz, 3M;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_agc =
        {
            .bypass = 0,
            .target_level = 3,
            .compression_gain = 6,
            .limiter_enable = 1,
        },
#endif

#if defined(SPEECH_TX_EQ)
    /****************************************************************************************************
     * Describtion:
     *     Equalizer, implementation with 2 order iir filters
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     *     gain(float): normalized gain. It is usually less than or equal to 0
     *     num(0~6): the number of EQs
     *     params: {type, frequency, gain, q}. It supports a lot of types,
     *please refer to iirfilt.h file Resource consumption: RAM:     None FLASE:
     *None MIPS:    fs = 16kHz, 0.5M/section; Note: None
     ****************************************************************************************************/
    .tx_eq =
        {
            .bypass = 0,
            .gain = 0.f,
            .num = 1,
            .params =
                {
                    {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
                },
        },
#endif

#if defined(SPEECH_TX_POST_GAIN)
    /****************************************************************************************************
     * Describtion:
     *     Linear Gain
     * Parameters:
     *     bypass(0/1): bypass enable or disable.
     * Resource consumption:
     *     RAM:     None
     *     FLASE:   None
     *     MIPS:    fs = 16kHz;
     * Note:
     *     None
     ****************************************************************************************************/
    .tx_post_gain =
        {
            .bypass = 0,
            .gain_dB = 6.0f,
        },
#endif

// #if defined(SPEECH_CS_VAD)
// /****************************************************************************************************
//  * Describtion:
//  *     Voice Activity Detector
//  * Parameters:
//  *     bypass(0/1): bypass enable or disable.
//  * Resource consumption:
//  *     RAM:     None
//  *     FLASE:   None
//  *     MIPS:    fs = 16kHz;
//  * Note:
//  *     None
// ****************************************************************************************************/
//     .tx_vad = {
//         .snrthd     = 5.f,
//         .energythd  = 100.f,
//     },
// #endif

#if defined(SPEECH_RX_NS)
    /****************************************************************************************************
     * Describtion:
     *     Acoustic Echo Cancellation
     * Parameters:
     *     Refer to SPEECH_TX_NS
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_ns =
        {
            .bypass = 0,
            .denoise_dB = -12,
        },
#endif

#if defined(SPEECH_RX_NS2)
    /****************************************************************************************************
     * Describtion:
     *     Acoustic Echo Cancellation
     * Parameters:
     *     Refer to SPEECH_TX_NS2
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_ns2 =
        {
            .bypass = 0,
            .denoise_dB = -15,
        },
#endif

#if defined(SPEECH_RX_NS2FLOAT)
    /****************************************************************************************************
     * Describtion:
     *     Acoustic Echo Cancellation
     * Parameters:
     *     Refer to SPEECH_TX_NS2FLOAT
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_ns2float =
        {
            .bypass = 0,
            .denoise_dB = -15,
            .banks = 64,
        },
#endif

#if defined(SPEECH_RX_NS3)
    /****************************************************************************************************
     * Describtion:
     *     Acoustic Echo Cancellation
     * Parameters:
     *     Refer to SPEECH_TX_NS3
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_ns3 =
        {
            .bypass = 0,
            .mode = NS3_SUPPRESSION_HIGH,
        },
#endif

#if defined(SPEECH_RX_NOISE_GATE)
    /****************************************************************************************************
     * Describtion:
     *     Noise Gate
     * Parameters:
     *     Refer to SPEECH_TX_NOISE_GATE
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_noise_gate =
        {
            .bypass = 0,
            .data_threshold = 640,
            .data_shift = 1,
            .factor_up = 0.001f,
            .factor_down = 0.5f,
        },
#endif

#if defined(SPEECH_RX_COMPEXP)
    /****************************************************************************************************
     * Describtion:
     *     Compressor and expander
     * Parameters:
     *     Refer to SPEECH_TX_COMPEXP
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_compexp =
        {
            .bypass = 0,
            .comp_threshold = -10.f,
            .comp_slope = 0.5f,
            .expand_threshold = -40.f,
            .expand_slope = -0.5f,
            .attack_time = 0.01f,
            .release_time = 0.6f,
            .makeup_gain = 6,
            .delay = 128,
        },
#endif

#if defined(SPEECH_RX_AGC)
    /****************************************************************************************************
     * Describtion:
     *      Automatic Gain Control
     * Parameters:
     *     Refer to SPEECH_TX_AGC
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_agc =
        {
            .bypass = 0,
            .target_level = 3,
            .compression_gain = 6,
            .limiter_enable = 1,
        },
#endif

#if defined(SPEECH_RX_EQ)
    /****************************************************************************************************
     * Describtion:
     *     Equalizer, implementation with 2 order iir filters
     * Parameters:
     *     Refer to SPEECH_TX_EQ
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_eq =
        {
            .bypass = 0,
            .gain = 0.f,
            .num = 1,
            .params =
                {
                    {IIR_BIQUARD_HPF, {{60, 0, 0.707f}}},
                },
        },
#endif

#if defined(SPEECH_RX_POST_GAIN)
    /****************************************************************************************************
     * Describtion:
     *     Linear Gain
     * Parameters:
     *     Refer to SPEECH_TX_POST_GAIN
     * Note:
     *     None
     ****************************************************************************************************/
    .rx_post_gain =
        {
            .bypass = 0,
            .gain_dB = 6.0f,
        },
#endif

};
