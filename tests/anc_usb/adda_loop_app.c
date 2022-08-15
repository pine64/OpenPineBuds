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
#include "tgt_hardware.h"
#include "string.h"
#include "audioflinger.h"
#include "adda_loop_app.h"

#define ADDA_SAMPLE_RATE                48000
#define ADDA_SAMPLE_SIZE                2

#define ADDA_PLAYBACK_CHAN              2
#define ADDA_CAPTURE_CHAN               2

#define RATE_TO_SIZE(n)                 (((n) + (1000 - 1)) / 1000)

#define ADDA_PLAYBACK_FRAME_SIZE        (RATE_TO_SIZE(ADDA_SAMPLE_RATE) * ADDA_SAMPLE_SIZE * ADDA_PLAYBACK_CHAN)
#define ADDA_CAPTURE_FRAME_SIZE         (RATE_TO_SIZE(ADDA_SAMPLE_RATE) * ADDA_SAMPLE_SIZE * ADDA_CAPTURE_CHAN)

#define PLAYBACK_FRAME_NUM              4
#define CAPTURE_FRAME_NUM               8

#define ADDA_NON_EXP_ALIGN(val, exp)    (((val) + ((exp) - 1)) / (exp) * (exp))
#define BUFF_ALIGN                      (4 * 4)

#define PLAYBACK_SIZE                   ADDA_NON_EXP_ALIGN(ADDA_PLAYBACK_FRAME_SIZE * PLAYBACK_FRAME_NUM, BUFF_ALIGN)
#define CAPTURE_SIZE                    ADDA_NON_EXP_ALIGN(ADDA_CAPTURE_FRAME_SIZE * CAPTURE_FRAME_NUM, BUFF_ALIGN)

#define ALIGNED4                        ALIGNED(4)

static uint8_t ALIGNED4 adda_playback_buf[PLAYBACK_SIZE];
static uint8_t ALIGNED4 adda_capture_buf[CAPTURE_SIZE];

static uint32_t cap_rpos;
static uint32_t cap_wpos;

static enum AUD_BITS_T sample_size_to_enum(uint32_t size)
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

static enum AUD_CHANNEL_NUM_T chan_num_to_enum(uint32_t num)
{
    if (num == 2) {
        return AUD_CHANNEL_NUM_2;
    } else if (num == 1) {
        return AUD_CHANNEL_NUM_1;
    } else {
        ASSERT(false, "%s: Invalid channel num: %u", __FUNCTION__, num);
    }

    return 0;
}

static uint32_t adda_data_playback(uint8_t *buf, uint32_t len)
{
    uint32_t cur_time;
    int16_t *src;
    int16_t *dst;
    int16_t *src_end;
    int16_t *dst_end;
    uint32_t avail;

    cur_time = hal_sys_timer_get();

    if (cap_wpos >= cap_rpos) {
        avail = cap_wpos - cap_rpos;
    } else {
        avail = sizeof(adda_capture_buf) + cap_wpos - cap_rpos;
    }

    if (avail * ADDA_PLAYBACK_CHAN / ADDA_CAPTURE_CHAN >= len) {
        src = (int16_t *)(adda_capture_buf + cap_rpos);
        src_end = (int16_t *)(adda_capture_buf + sizeof(adda_capture_buf));
        dst = (int16_t *)buf;
        dst_end = (int16_t *)(buf + len);

        while (dst < dst_end) {
            *dst++ = *src++;
            if (src == src_end) {
                src = (int16_t *)adda_capture_buf;
            }
#if (ADDA_PLAYBACK_CHAN == 2) && (ADDA_CAPTURE_CHAN == 1)
            *dst = *(dst - 1);
            dst++;
#endif
        }

        cap_rpos = (uint32_t)src - (uint32_t)adda_capture_buf;
    } else {
        memset(buf, 0, len);
    }

    TRACE(5,"[%X] playback: len=%4u wpos=%4u rpos=%4u avail=%4u", cur_time, len, cap_wpos, cap_rpos, avail);
    return 0;
}

static uint32_t adda_data_capture(uint8_t *buf, uint32_t len)
{
    uint32_t cur_time;

    cur_time = hal_sys_timer_get();

    cap_wpos += len;
    if (cap_wpos >= sizeof(adda_capture_buf)) {
        cap_wpos = 0;
    }

    TRACE(4,"[%X] capture : len=%4u wpos=%4u rpos=%4u", cur_time, len, cap_wpos, cap_rpos);
    return 0;
}

static void adda_loop_start(void)
{
    int ret = 0;
    struct AF_STREAM_CONFIG_T stream_cfg;

    hal_sysfreq_req(HAL_SYSFREQ_USER_APP_2, HAL_CMU_FREQ_52M);

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.bits = sample_size_to_enum(ADDA_SAMPLE_SIZE);
    stream_cfg.channel_num = chan_num_to_enum(ADDA_PLAYBACK_CHAN);
    stream_cfg.sample_rate = ADDA_SAMPLE_RATE;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.vol = AUDIO_OUTPUT_VOLUME_DEFAULT;
    stream_cfg.handler = adda_data_playback;
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
    stream_cfg.data_ptr = adda_playback_buf;
    stream_cfg.data_size = sizeof(adda_playback_buf);

    TRACE(3,"[%s] playback sample_rate: %d, bits = %d", __func__, stream_cfg.sample_rate, stream_cfg.bits);

    ret = af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
    ASSERT(ret == 0, "af_stream_open playback failed: %d", ret);

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.bits = sample_size_to_enum(ADDA_SAMPLE_SIZE);
    stream_cfg.channel_num = chan_num_to_enum(ADDA_CAPTURE_CHAN);
    stream_cfg.sample_rate = ADDA_SAMPLE_RATE;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.vol = CODEC_SADC_VOL;
    stream_cfg.handler = adda_data_capture;
    stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
    stream_cfg.data_ptr = adda_capture_buf;
    stream_cfg.data_size = sizeof(adda_capture_buf);

    TRACE(3,"[%s] capture sample_rate: %d, bits = %d", __func__, stream_cfg.sample_rate, stream_cfg.bits);

    ret = af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
    ASSERT(ret == 0, "af_stream_open catpure failed: %d", ret);

    ret = af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    ASSERT(ret == 0, "af_stream_start playback failed: %d", ret);

    ret = af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    ASSERT(ret == 0, "af_stream_start catpure failed: %d", ret);
}

static void adda_loop_stop(void)
{
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

    hal_sysfreq_req(HAL_SYSFREQ_USER_APP_2, HAL_CMU_FREQ_32K);
}

void adda_loop_app(bool on)
{
    if (on) {
        adda_loop_start();
    } else {
        adda_loop_stop();
    }
}

void adda_loop_app_loop(void)
{
#ifndef RTOS
    extern void af_thread(void const *argument);
    af_thread(NULL);
#endif
}

