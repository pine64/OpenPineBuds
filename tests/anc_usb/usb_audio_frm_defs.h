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
#ifndef __USB_AUDIO_FRM_DEFS_H__
#define __USB_AUDIO_FRM_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

// Recv/send sample rates
#if 0
#elif defined(USB_AUDIO_384K)
#define SAMPLE_RATE_RECV                384000
#define SAMPLE_RATE_SEND                48000
#elif defined(USB_AUDIO_352_8K)
#define SAMPLE_RATE_RECV                352800
#define SAMPLE_RATE_SEND                44100
#elif defined(USB_AUDIO_192K)
#define SAMPLE_RATE_RECV                192000
#define SAMPLE_RATE_SEND                48000
#elif defined(USB_AUDIO_176_4K)
#define SAMPLE_RATE_RECV                176400
#define SAMPLE_RATE_SEND                44100
#elif defined(USB_AUDIO_96K)
#define SAMPLE_RATE_RECV                96000
#define SAMPLE_RATE_SEND                48000
#elif defined(USB_AUDIO_48K)
#define SAMPLE_RATE_RECV                48000
#define SAMPLE_RATE_SEND                48000
#elif defined(USB_AUDIO_44_1K)
#define SAMPLE_RATE_RECV                44100
#define SAMPLE_RATE_SEND                44100
#elif defined(USB_AUDIO_16K)
#define SAMPLE_RATE_RECV                16000
#define SAMPLE_RATE_SEND                16000
#else // 48K by default
#define SAMPLE_RATE_RECV                48000
#define SAMPLE_RATE_SEND                48000
#endif

// Playback sample rate
#if defined(CHIP_BEST1000) && defined(ANC_APP)
#if (SAMPLE_RATE_RECV % 8000)
#define SAMPLE_RATE_PLAYBACK            88200
#else
#define SAMPLE_RATE_PLAYBACK            96000
#endif
#elif defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
#define SAMPLE_RATE_PLAYBACK            50781
#else
#define SAMPLE_RATE_PLAYBACK            SAMPLE_RATE_RECV
#endif

// Capture sample rate
#if defined(CHIP_BEST1000) && defined(ANC_APP)
#ifdef AUD_PLL_DOUBLE
#if defined(_DUAL_AUX_MIC_) || defined(CAPTURE_ANC_DATA)
#define SAMPLE_RATE_CAPTURE             (SAMPLE_RATE_PLAYBACK * 8)
#else
#define SAMPLE_RATE_CAPTURE             (SAMPLE_RATE_PLAYBACK * 2)
#endif
#else // !AUD_PLL_DOUBLE
#if defined(_DUAL_AUX_MIC_) || defined(CAPTURE_ANC_DATA)
#define SAMPLE_RATE_CAPTURE             (SAMPLE_RATE_PLAYBACK * 4)
#else
#define SAMPLE_RATE_CAPTURE             SAMPLE_RATE_PLAYBACK
#endif
#endif // !AUD_PLL_DOUBLE
#else // !(CHIP_BEST1000 && ANC_APP)
#if defined(CHIP_BEST1000) && defined(_DUAL_AUX_MIC_)
#define SAMPLE_RATE_CAPTURE             (SAMPLE_RATE_SEND * 4)
#elif defined(__AUDIO_RESAMPLE__) && defined(SW_CAPTURE_RESAMPLE)
#define SAMPLE_RATE_CAPTURE             50781
#else
#define SAMPLE_RATE_CAPTURE             SAMPLE_RATE_SEND
#endif
#endif // !(CHIP_BEST1000 && ANC_APP)

#ifdef USB_AUDIO_32BIT
#define SAMPLE_SIZE_PLAYBACK            4
#define SAMPLE_SIZE_RECV                4
#elif defined(USB_AUDIO_24BIT)
#define SAMPLE_SIZE_PLAYBACK            4
#define SAMPLE_SIZE_RECV                3
#else
#ifdef AUDIO_PLAYBACK_24BIT
#define SAMPLE_SIZE_PLAYBACK            4
#else
#define SAMPLE_SIZE_PLAYBACK            2
#endif
#define SAMPLE_SIZE_RECV                2
#endif

#ifdef USB_AUDIO_SEND_32BIT
#define SAMPLE_SIZE_CAPTURE             4
#define SAMPLE_SIZE_SEND                4
#elif defined(USB_AUDIO_SEND_24BIT)
#define SAMPLE_SIZE_CAPTURE             4
#define SAMPLE_SIZE_SEND                3
#else
#define SAMPLE_SIZE_CAPTURE             2
#define SAMPLE_SIZE_SEND                2
#endif

#define CHAN_NUM_PLAYBACK               2
#define CHAN_NUM_RECV                   2
#ifdef USB_AUDIO_SEND_CHAN
#define CHAN_NUM_SEND                   USB_AUDIO_SEND_CHAN
#else
#define CHAN_NUM_SEND                   2
#endif
#if defined(CHIP_BEST1000) && defined(ANC_APP)
#define CHAN_NUM_CAPTURE                2
#elif defined(USB_AUDIO_SPEECH)
#define CHAN_NUM_CAPTURE                SPEECH_CODEC_CAPTURE_CHANNEL_NUM
#else
#define CHAN_NUM_CAPTURE                CHAN_NUM_SEND
#endif

#define BYTE_TO_SAMP_PLAYBACK(n)        ((n) / SAMPLE_SIZE_PLAYBACK / CHAN_NUM_PLAYBACK)
#define SAMP_TO_BYTE_PLAYBACK(n)        ((n) * SAMPLE_SIZE_PLAYBACK * CHAN_NUM_PLAYBACK)

#define BYTE_TO_SAMP_CAPTURE(n)         ((n) / SAMPLE_SIZE_CAPTURE / CHAN_NUM_CAPTURE)
#define SAMP_TO_BYTE_CAPTURE(n)         ((n) * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE)

#define BYTE_TO_SAMP_RECV(n)            ((n) / SAMPLE_SIZE_RECV / CHAN_NUM_RECV)
#define SAMP_TO_BYTE_RECV(n)            ((n) * SAMPLE_SIZE_RECV * CHAN_NUM_RECV)

#define BYTE_TO_SAMP_SEND(n)            ((n) / SAMPLE_SIZE_SEND / CHAN_NUM_SEND)
#define SAMP_TO_BYTE_SEND(n)            ((n) * SAMPLE_SIZE_SEND * CHAN_NUM_SEND)

#define SAMP_RATE_TO_FRAME_SIZE(n)      (((n) + (1000 - 1)) / 1000)

#define SAMPLE_FRAME_PLAYBACK           SAMP_RATE_TO_FRAME_SIZE(SAMPLE_RATE_PLAYBACK)
#define SAMPLE_FRAME_CAPTURE            SAMP_RATE_TO_FRAME_SIZE(SAMPLE_RATE_CAPTURE)

#define SAMPLE_FRAME_RECV               SAMP_RATE_TO_FRAME_SIZE(SAMPLE_RATE_RECV)
#define SAMPLE_FRAME_SEND               SAMP_RATE_TO_FRAME_SIZE(SAMPLE_RATE_SEND)

#define FRAME_SIZE_PLAYBACK             SAMP_TO_BYTE_PLAYBACK(SAMPLE_FRAME_PLAYBACK)
#define FRAME_SIZE_CAPTURE              SAMP_TO_BYTE_CAPTURE(SAMPLE_FRAME_CAPTURE)

#define FRAME_SIZE_RECV                 SAMP_TO_BYTE_RECV(SAMPLE_FRAME_RECV)
#define FRAME_SIZE_SEND                 SAMP_TO_BYTE_SEND(SAMPLE_FRAME_SEND)

#define PLAYBACK_TO_RECV_LEN(n)         (SAMP_TO_BYTE_RECV((BYTE_TO_SAMP_PLAYBACK(n) * \
    (SAMPLE_RATE_RECV / 100) + (SAMPLE_RATE_PLAYBACK / 100) - 1) / (SAMPLE_RATE_PLAYBACK / 100)))
#define CAPTURE_TO_SEND_LEN(n)          (SAMP_TO_BYTE_SEND((BYTE_TO_SAMP_CAPTURE(n) * \
    (SAMPLE_RATE_SEND / 100) + (SAMPLE_RATE_CAPTURE / 100) - 1) / (SAMPLE_RATE_CAPTURE / 100)))

#ifdef USB_HIGH_SPEED
#if defined(USB_AUDIO_384K) || defined(USB_AUDIO_352_8K)
#define MAX_FRAME_SIZE_PLAYBACK         (SAMP_RATE_TO_FRAME_SIZE(384000) * 4 * CHAN_NUM_PLAYBACK)
#else
#define MAX_FRAME_SIZE_PLAYBACK         (SAMP_RATE_TO_FRAME_SIZE(192000) * 4 * CHAN_NUM_PLAYBACK)
#endif
#else
// Same as                              (SAMP_RATE_TO_FRAME_SIZE(192000) * 2 * CHAN_NUM_PLAYBACK)
#define MAX_FRAME_SIZE_PLAYBACK         (SAMP_RATE_TO_FRAME_SIZE(96000) * 4 * CHAN_NUM_PLAYBACK)
#endif
#if (defined(__AUDIO_RESAMPLE__) && defined(SW_CAPTURE_RESAMPLE)) || \
        (defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_)))
// Fixed capture sample rate
#define MAX_FRAME_SIZE_CAPTURE          (SAMP_RATE_TO_FRAME_SIZE(SAMPLE_RATE_CAPTURE) * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE)
#else
#define MAX_FRAME_SIZE_CAPTURE          (SAMP_RATE_TO_FRAME_SIZE(48000) * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE)
#endif

#ifdef USB_HIGH_SPEED
#if defined(USB_AUDIO_384K) || defined(USB_AUDIO_352_8K)
#define MAX_FRAME_SIZE_RECV             (SAMP_RATE_TO_FRAME_SIZE(384000) * 4 * CHAN_NUM_RECV)
#else
#define MAX_FRAME_SIZE_RECV             (SAMP_RATE_TO_FRAME_SIZE(192000) * 4 * CHAN_NUM_RECV)
#endif
#else
// Same as                              (SAMP_RATE_TO_FRAME_SIZE(192000) * 2 * CHAN_NUM_RECV)
#define MAX_FRAME_SIZE_RECV             (SAMP_RATE_TO_FRAME_SIZE(96000) * 4 * CHAN_NUM_RECV)
#endif
#define MAX_FRAME_SIZE_SEND             (SAMP_RATE_TO_FRAME_SIZE(48000) * SAMPLE_SIZE_SEND * CHAN_NUM_SEND)

// ----------------
// Buffer alignment
// ----------------

#define AF_DMA_LIST_CNT                 4

#ifdef ADC_CH_SEP_BUFF
#define ADC_BURST_NUM                   4
#define ADC_ALL_CH_DMA_BURST_SIZE       (ADC_BURST_NUM * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE)
#define ADC_BUFF_ALIGN                  (ADC_ALL_CH_DMA_BURST_SIZE * AF_DMA_LIST_CNT)
#else
#define ADC_BUFF_ALIGN                  (4 * AF_DMA_LIST_CNT)
#endif
#if defined(__HW_FIR_DSD_PROCESS__)
// FIR DSD requires DAC buffer is aligned to 16-sample boundary for each FIR DMA process loop
#define DAC_BUFF_ALIGN                  (16 * SAMPLE_SIZE_PLAYBACK * CHAN_NUM_PLAYBACK * 2)
#else
#define DAC_BUFF_ALIGN                  (4 * AF_DMA_LIST_CNT)
#endif

// USB buffer is split into 2 parts, and transfers from the second half (similar to PING-PONG buffer)
#ifdef USB_AUDIO_DYN_CFG
// RECV type-I frame slot (sample_size*chan_num): 2*2=4, 3*2=6, 4*2=8. The least common multiple is 24
#define RECV_BUFF_ALIGN                  (2 * 24)
// SEND type-I frame slot (sample_size*chan_num): 2*2=4. The least common multiple is 4
#define SEND_BUFF_ALIGN                  (2 * 4)
#else
// If usb rx/tx pool is NOT used, the buffer size should be aligned to frame packet size
#define RECV_BUFF_ALIGN                  SAMP_TO_BYTE_RECV(2)
#define SEND_BUFF_ALIGN                  SAMP_TO_BYTE_SEND(2)
#endif

#define NON_EXP_ALIGN(val, exp)         (((val) + ((exp) - 1)) / (exp) * (exp))

#ifdef __cplusplus
}
#endif

#endif
