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
#include "speech_process.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "cmsis.h"

#if defined(SPEECH_TX_AEC2FLOAT) && !defined(KEEP_SAME_LATENCY)
#error "capture/playback size should be equal when enable AEC"
#endif

#include "bt_sco_chain.h"
#include "iir_resample.h"
#include "speech_utils.h"
#include "speech_memory.h"

#define MED_MEM_POOL_SIZE (1024*160)
static uint8_t g_medMemPool[MED_MEM_POOL_SIZE];

#define FFSE_SAMPLE_RATE    (16000)

int capture_channel_num = 1;
int capture_sample_rate = 48000;
int capture_sample_bit = 16;
int playback_channel_num = 2;
int playback_sample_rate = 48000;
int playback_sample_bit = 24;

int send_channel_num = 2;
int recv_channel_num = 2;

// resample related
static bool resample_needed_flag = false;
static IirResampleState *upsample_st;
static IirResampleState *downsample_st[4];
int16_t *capture_buffer_deinter = NULL;
int16_t *process_buffer = NULL;
int16_t *process_buffer_inter = NULL;

static short *aec_echo_buf = NULL;
static IirResampleState *rx_downsample_st;

static volatile bool is_speech_init = false;

static void speech_deinterleave(int16_t *in, int16_t *out, int len, int ch_num)
{
    int len_per_channel = len / ch_num;

    for (int i = 0, j = 0; i < len; i += ch_num, j++) {
        int16_t *pout = &out[j];
        int16_t *pin = &in[i];
        for (int c= 0; c < ch_num; c++) {
            *pout = *pin;
            pout += len_per_channel;
            pin += 1;
        }
    }
}

static void speech_interleave(int16_t *in, int16_t *out, int len, int ch_num)
{
    int len_per_channel = len / ch_num;

    for (int i = 0, j = 0; j < len; i++, j += ch_num) {
        int16_t *pout = &out[j];
        int16_t *pin = &in[i];
        for (int c= 0; c < ch_num; c++) {
            *pout = *pin;
            pout += 1;
            pin += len_per_channel;
        }
    }
}

static void speech_extend(int16_t *in, int16_t *out, int len, int ch_num)
{
    int16_t *pout = out + len * ch_num - 1;
    for (int i = len - 1; i >= 0; i--) {
        for (int c= 0; c < ch_num; c++) {
            *pout-- = in[i];
        }
    }
}

// This function output remains the same sample rate as input,
// output channel number shoule be CHAN_NUM_SEND.
// TODO: add multi-channel support in iir resampler
void speech_process_capture_run(uint8_t *buf, uint32_t *len)
{
    //TRACE(2,"[%s], pcm_len: %d", __FUNCTION__, *len / 2);

    if (is_speech_init == false)
        return;

    int16_t *pcm_buf = (int16_t *)buf;
    int pcm_len = *len / 2;
    int process_len = pcm_len * FFSE_SAMPLE_RATE / capture_sample_rate;

    if (resample_needed_flag == true) {
        if (capture_channel_num > 1)
            speech_deinterleave(pcm_buf, capture_buffer_deinter, pcm_len, capture_channel_num);
        else
            speech_copy_int16(capture_buffer_deinter, pcm_buf, pcm_len);

        int in_len_per_channel = pcm_len / capture_channel_num;
        int out_len_per_channel = process_len / capture_channel_num;
        for (int i = 0; i < capture_channel_num; i++) {
            iir_resample_process(downsample_st[i], &capture_buffer_deinter[i * in_len_per_channel],
                &process_buffer[i * out_len_per_channel], in_len_per_channel);
        }

        if (capture_channel_num > 1)
            speech_interleave(process_buffer, process_buffer_inter, process_len, capture_channel_num);
        else
            speech_copy_int16(process_buffer_inter, process_buffer, process_len);

        speech_tx_process(process_buffer_inter, aec_echo_buf, &process_len);

        iir_resample_process(upsample_st, process_buffer_inter, pcm_buf, process_len);

        if (send_channel_num > 1)
            speech_extend(pcm_buf, pcm_buf, in_len_per_channel, send_channel_num);
    } else {
        speech_tx_process(pcm_buf, aec_echo_buf, &process_len);

        if (send_channel_num > 1)
            speech_extend(pcm_buf, pcm_buf, process_len, send_channel_num);
    }

    pcm_len = pcm_len / capture_channel_num * send_channel_num;
    *len = pcm_len * sizeof(int16_t);
}

void speech_process_playback_run(uint8_t *buf, uint32_t *len)
{
    //TRACE(2,"[%s] pcm_len: %d", __FUNCTION__, *len / 2);

    if (is_speech_init == false)
        return;

#if defined(SPEECH_TX_AEC2FLOAT)
    int16_t *pcm_buf = (int16_t *)buf;
    int pcm_len = *len / 2;

    if (resample_needed_flag == true) {
        // Convert to 16bit if necessary
        if (playback_sample_bit == 24) {
            int32_t *pcm32 = (int32_t *)buf;
            for (int i = 0; i < pcm_len / 2; i++) {
                pcm_buf[i] = (pcm32[i] >> 8);
            }
            pcm_len >>= 1;
        }

        // Convert to mono if necessary, choose left channel
        if (playback_channel_num == 2) {
            for (int i = 0, j = 0; i < pcm_len; i += 2, j++)
                pcm_buf[j] = pcm_buf[i];
            pcm_len >>= 1;
        }

        iir_resample_process(rx_downsample_st, pcm_buf, pcm_buf, pcm_len);
    }
    speech_copy_int16(aec_echo_buf, pcm_buf, pcm_len * FFSE_SAMPLE_RATE / capture_sample_rate);
#endif
}

void speech_process_init(int tx_sample_rate, int tx_channel_num, int tx_sample_bit,
                                  int rx_sample_rate, int rx_channel_num, int rx_sample_bit,
                                  int tx_frame_ms, int rx_frame_ms,
                                  int tx_send_channel_num, int rx_recv_channel_num)
{
    ASSERT(tx_sample_rate == 16000 || tx_sample_rate == 48000, "[%s] sample rate(%d) not supported", __FUNCTION__, tx_sample_rate);
    ASSERT(tx_frame_ms == 16, "[%s] just support 16ms frame", __func__);

    capture_sample_rate = tx_sample_rate;
    capture_channel_num = tx_channel_num;
    capture_sample_bit = tx_sample_bit;
    playback_sample_rate = rx_sample_rate;
    playback_channel_num = rx_channel_num;
    playback_sample_bit = rx_sample_bit;

    send_channel_num = tx_send_channel_num;
    recv_channel_num = rx_recv_channel_num;

    resample_needed_flag = (capture_sample_rate != FFSE_SAMPLE_RATE);

    TRACE(5,"[%s] sample_rate: %d, frame_ms: %d, channel_num: %d, resample_needed_flag: %d", __FUNCTION__,
        tx_sample_rate, tx_frame_ms, tx_channel_num, resample_needed_flag);

    speech_init(FFSE_SAMPLE_RATE, FFSE_SAMPLE_RATE, tx_frame_ms, tx_frame_ms, tx_frame_ms, &g_medMemPool[0], MED_MEM_POOL_SIZE);

    if (resample_needed_flag == true) {
        capture_buffer_deinter = speech_calloc(SPEECH_FRAME_MS_TO_LEN(capture_sample_rate, tx_frame_ms) * capture_channel_num, sizeof(int16_t));

        // Resample state must be created after speech init, as it uses speech heap
        process_buffer = speech_calloc(SPEECH_FRAME_MS_TO_LEN(FFSE_SAMPLE_RATE, tx_frame_ms) * capture_channel_num, sizeof(int16_t));
        process_buffer_inter = speech_calloc(SPEECH_FRAME_MS_TO_LEN(FFSE_SAMPLE_RATE, tx_frame_ms) * capture_channel_num, sizeof(int16_t));

        upsample_st = iir_resample_init(SPEECH_FRAME_MS_TO_LEN(FFSE_SAMPLE_RATE, tx_frame_ms), iir_resample_choose_mode(FFSE_SAMPLE_RATE, capture_sample_rate));

        // as iir resample can only deal with mono signal, we should init channel_num states
        for (int i = 0; i < capture_channel_num; i++) {
            downsample_st[i] = iir_resample_init(SPEECH_FRAME_MS_TO_LEN(capture_sample_rate, tx_frame_ms), iir_resample_choose_mode(capture_sample_rate, FFSE_SAMPLE_RATE));
        }

        //
        aec_echo_buf = speech_calloc(SPEECH_FRAME_MS_TO_LEN(FFSE_SAMPLE_RATE, rx_frame_ms), sizeof(int16_t));
        rx_downsample_st = iir_resample_init(SPEECH_FRAME_MS_TO_LEN(playback_sample_rate, rx_frame_ms), iir_resample_choose_mode(playback_sample_rate, FFSE_SAMPLE_RATE));
    }

    is_speech_init = true;
}

void speech_process_deinit(void)
{
    if (is_speech_init == false)
        return;

    if (resample_needed_flag == true) {
        speech_free(capture_buffer_deinter);

        speech_free(process_buffer);
        speech_free(process_buffer_inter);

        iir_resample_destroy(upsample_st);

        for (int i = 0; i < capture_channel_num; i++) {
            iir_resample_destroy(downsample_st[i]);
        }

        speech_free(aec_echo_buf);
        iir_resample_destroy(rx_downsample_st);

        resample_needed_flag = false;
    }

    speech_deinit();

    is_speech_init = false;
}

enum HAL_CMU_FREQ_T speech_process_need_freq(void)
{
    return HAL_CMU_FREQ_208M;
}

