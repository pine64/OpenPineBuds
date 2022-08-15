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
#ifndef __A2DPPLAY_H__
#define __A2DPPLAY_H__

#include "app_utils.h"
#include "avdtp_api.h"

#define A2DP_DECODER_HISTORY_SEQ_SAVE          (25)
//#define A2DP_DECODER_HISTORY_LOCTIME_SAVE      (1)
//#define A2DP_DECODER_HISTORY_CHECK_SUM_SAVE    (1)

typedef uint16_t A2DP_AUDIO_CODEC_TYPE;

#define A2DP_AUDIO_CODEC_TYPE_SBC           (1u<<0)
#define A2DP_AUDIO_CODEC_TYPE_MPEG2_4_AAC   (1u<<1)
#define A2DP_AUDIO_CODEC_TYPE_OPUS          (1u<<2)
#define A2DP_AUDIO_CODEC_TYPE_SCALABL       (1u<<3)
#define A2DP_AUDIO_CODEC_TYPE_LHDC          (1u<<4)
#define A2DP_AUDIO_CODEC_TYPE_LDAC          (1u<<5)

#define A2DP_AUDIO_SYNCFRAME_MASK_SEQ         (1u<<0)
#define A2DP_AUDIO_SYNCFRAME_MASK_TIMESTAMP   (1u<<1)
#define A2DP_AUDIO_SYNCFRAME_MASK_CURRSUBSEQ  (1u<<2)
#define A2DP_AUDIO_SYNCFRAME_MASK_TOTALSUBSEQ (1u<<3)

#define A2DP_AUDIO_SYNCFRAME_MASK_ALL (A2DP_AUDIO_SYNCFRAME_MASK_SEQ        | \
                                       A2DP_AUDIO_SYNCFRAME_MASK_TIMESTAMP  | \
                                       A2DP_AUDIO_SYNCFRAME_MASK_CURRSUBSEQ | \
                                       A2DP_AUDIO_SYNCFRAME_MASK_TOTALSUBSEQ)

#define A2DP_AUDIO_SYNCFRAME_CHK(equ, mask_val, mask) (((equ)|!(mask_val&mask)))

typedef enum {
    A2DP_AUDIO_LATENCY_STATUS_LOW,
    A2DP_AUDIO_LATENCY_STATUS_HIGH,
} A2DP_AUDIO_LATENCY_STATUS_E;

typedef struct  {
    uint32_t sample_rate;
    uint8_t num_channels;
    uint8_t bits_depth;
    uint8_t curr_bits;
    uint32_t frame_samples;    
    float factor_reference;     
} A2DP_AUDIO_OUTPUT_CONFIG_T;

typedef enum {
    A2DP_AUDIO_CHANNEL_SELECT_STEREO,
    A2DP_AUDIO_CHANNEL_SELECT_LRMERGE,
    A2DP_AUDIO_CHANNEL_SELECT_LCHNL,
    A2DP_AUDIO_CHANNEL_SELECT_RCHNL,
} A2DP_AUDIO_CHANNEL_SELECT_E;

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint16_t curSubSequenceNumber;
    uint16_t totalSubSequenceNumber;
    uint32_t frame_samples;
    uint32_t list_samples;
    uint32_t decoded_frames;
    uint32_t undecode_frames;    
    uint32_t undecode_min_frames;
    uint32_t undecode_max_frames;
    uint32_t average_frames;
    uint32_t check_sum;
    A2DP_AUDIO_OUTPUT_CONFIG_T stream_info;
} A2DP_AUDIO_LASTFRAME_INFO_T;

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint16_t curSubSequenceNumber;
    uint16_t totalSubSequenceNumber;
} A2DP_AUDIO_HEADFRAME_INFO_T;

typedef struct{          
    float proportiongain;     
    float integralgain;       
    float derivativegain;  
    float error[3];
    float result;
}A2DP_AUDIO_SYNC_PID_T;

typedef A2DP_AUDIO_LASTFRAME_INFO_T A2DP_AUDIO_SYNCFRAME_INFO_T;

typedef int(*A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK)(btif_media_header_t *, unsigned char *, unsigned int len);

#ifdef __cplusplus
extern "C" {
#endif

uint32_t a2dp_audio_playback_handler(uint8_t *buffer, uint32_t buffer_bytes);
float a2dp_audio_sync_pid_calc(A2DP_AUDIO_SYNC_PID_T *pid, float diff);
int a2dp_audio_sync_init(double ratio);
int a2dp_audio_sync_reset_data(void);
int a2dp_audio_sync_tune_sample_rate(double ratio);
int a2dp_audio_sync_direct_tune_sample_rate(double ratio);
int a2dp_audio_sync_tune_cancel(void);
int a2dp_audio_sysfreq_boost_start(uint32_t boost_cnt);
int a2dp_audio_sysfreq_boost_running(void);
int a2dp_audio_init(uint32_t sysfreq, A2DP_AUDIO_CODEC_TYPE codec_type, A2DP_AUDIO_OUTPUT_CONFIG_T *config, 
                           A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel, uint16_t dest_packet_mut);
int a2dp_audio_deinit(void);
int a2dp_audio_start(void);
int a2dp_audio_stop(void);
int a2dp_audio_detect_next_packet_callback_register(A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK callback);
int a2dp_audio_detect_store_packet_callback_register(A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK callback);
int a2dp_audio_detect_first_packet(void);
int a2dp_audio_detect_first_packet_clear(void);
int a2dp_audio_store_packet(btif_media_header_t * header, unsigned char *buf, unsigned int len);
int a2dp_audio_discards_packet(uint32_t packets);
int a2dp_audio_synchronize_dest_packet_mut(uint32_t mtu);
int a2dp_audio_discards_samples(uint32_t samples);
int a2dp_audio_convert_list_to_samples(uint32_t *samples);
int a2dp_audio_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info, uint32_t mask);
int a2dp_audio_decoder_headframe_info_get(A2DP_AUDIO_HEADFRAME_INFO_T *headframe_info);
int a2dp_audio_get_packet_samples(void);
int a2dp_audio_lastframe_info_get(A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info);
int a2dp_audio_lastframe_info_reset_undecodeframe(void);
void a2dp_audio_clear_input_raw_packet_list(void);
int a2dp_audio_refill_packet(void);
bool a2dp_audio_auto_synchronize_support(void);
A2DP_AUDIO_OUTPUT_CONFIG_T *a2dp_audio_get_output_config(void);
int a2dp_audio_latency_factor_setlow(void);
int a2dp_audio_latency_factor_sethigh(void);
float a2dp_audio_latency_factor_get(void);
int a2dp_audio_latency_factor_set(float factor);
int a2dp_audio_frame_delay_get(void);
int a2dp_audio_dest_packet_mut_get(void);
int a2dp_audio_latency_factor_status_get(A2DP_AUDIO_LATENCY_STATUS_E *latency_status, float *more_latency_factor);
#if A2DP_DECODER_HISTORY_SEQ_SAVE    
int a2dp_audio_show_history_seq(void);
#endif
int a2dp_audio_set_channel_select(A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel);

#ifdef __cplusplus
}
#endif

#endif//__A2DPPLAY_H__
