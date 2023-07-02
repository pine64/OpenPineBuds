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
#ifndef __BT_SCO_CHAIN_CFG_H__
#define __BT_SCO_CHAIN_CFG_H__

#include "speech_cfg.h"

typedef struct {
#if defined(SPEECH_TX_DC_FILTER)
    SpeechDcFilterConfig    tx_dc_filter;
#endif
#if defined(SPEECH_TX_AEC)
    SpeechAecConfig         tx_aec;
#endif
#if defined(SPEECH_TX_AEC2)
    SpeechAec2Config        tx_aec2;
#endif
#if defined(SPEECH_TX_AEC2FLOAT)
    Ec2FloatConfig          tx_aec2float;
#endif
#if defined(SPEECH_TX_AEC3)
    SubBandAecConfig        tx_aec3;
#endif
#if defined(SPEECH_TX_2MIC_NS)
    DUAL_MIC_DENOISE_CFG_T  tx_2mic_ns;
#endif
#if defined(SPEECH_TX_2MIC_NS2)
    Speech2MicNs2Config   tx_2mic_ns2;
#endif
#if defined(SPEECH_TX_2MIC_NS4)
    SensorMicDenoiseConfig  tx_2mic_ns4;
#endif
#if defined(SPEECH_TX_2MIC_NS5)
    LeftRightDenoiseConfig   tx_2mic_ns5;
#endif
#if defined(SPEECH_TX_3MIC_NS)
    Speech3MicNsConfig      tx_3mic_ns;
#endif
#if defined(SPEECH_TX_3MIC_NS3)
    TripleMicDenoise3Config tx_3mic_ns3;
#endif
#if defined(SPEECH_TX_NS)
    SpeechNsConfig          tx_ns;
#endif
#if defined(SPEECH_TX_NS2)
    SpeechNs2Config         tx_ns2;
#endif
#if defined(SPEECH_TX_NS2FLOAT)
    SpeechNs2FloatConfig    tx_ns2float;
#endif
#if defined(SPEECH_TX_NS3)
    Ns3Config               tx_ns3;
#endif
#if defined(SPEECH_TX_WNR)
    WnrConfig               tx_wnr;
#endif
#if defined(SPEECH_TX_NOISE_GATE)
    NoisegateConfig         tx_noise_gate;
#endif
#if defined(SPEECH_TX_COMPEXP)
    CompexpConfig           tx_compexp;
#endif
#if defined(SPEECH_TX_AGC)
    AgcConfig               tx_agc;
#endif
#if defined(SPEECH_TX_EQ)
    EqConfig                tx_eq;
#endif
#if defined(SPEECH_TX_POST_GAIN)
    SpeechGainConfig        tx_post_gain;
#endif
// #if defined(SPEECH_CS_VAD)
//     VADConfig               cs_vad;
// #endif
#if defined(SPEECH_RX_NS)
    SpeechNsConfig          rx_ns;
#endif
#if defined(SPEECH_RX_NS2)
    SpeechNs2Config         rx_ns2;
#endif
#if defined(SPEECH_RX_NS2FLOAT)
    SpeechNs2FloatConfig    rx_ns2float;
#endif
#if defined(SPEECH_RX_NS3)
    Ns3Config               rx_ns3;
#endif
#if defined(SPEECH_RX_NOISE_GATE)
    NoisegateConfig         rx_noise_gate;
#endif
#if defined(SPEECH_RX_COMPEXP)
    CompexpConfig           rx_compexp;
#endif
#if defined(SPEECH_RX_AGC)
    AgcConfig               rx_agc;
#endif
#if defined(SPEECH_RX_EQ)
    EqConfig                rx_eq;
#endif
#if defined(SPEECH_RX_POST_GAIN)
    SpeechGainConfig        rx_post_gain;
#endif
    // Add more process
} SpeechConfig;

#endif




























