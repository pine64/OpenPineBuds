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
#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_thread.h"

#include "hal_timer.h"
#include "app_audtest_pattern.h"

#include "hal_aud.h"
#include "audioflinger.h"
#include "audiobuffer.h"
#include "stdbool.h"
#include <string.h>
#include "eq_export.h"
#include "app_utils.h"

#if defined(APP_TEST_AUDIO) && defined(ANC_APP)
#include "anc_usb_app.h"
#include "usb_audio_app.h"
#include "usb_audio_frm_defs.h"
//#include "dualadc_audio_app.h"
#endif

#define USB_AUDIO_PLAYBACK_BUFF_SIZE    (FRAME_SIZE_PLAYBACK * 4)
#define USB_AUDIO_CAPTURE_BUFF_SIZE     (FRAME_SIZE_CAPTURE * 4)

#define USB_AUDIO_RECV_BUFF_SIZE        (FRAME_SIZE_RECV * 8)
#define USB_AUDIO_SEND_BUFF_SIZE        (FRAME_SIZE_SEND * 8)

#define APP_TEST_PLAYBACK_BUFF_SIZE     (128 * 20)
#define APP_TEST_CAPTURE_BUFF_SIZE      (128 * 20)

#if (USB_AUDIO_PLAYBACK_BUFF_SIZE > APP_TEST_PLAYBACK_BUFF_SIZE)
#define REAL_PLAYBACK_BUFF_SIZE         USB_AUDIO_PLAYBACK_BUFF_SIZE
#else
#define REAL_PLAYBACK_BUFF_SIZE         APP_TEST_PLAYBACK_BUFF_SIZE
#endif

#if (USB_AUDIO_CAPTURE_BUFF_SIZE > APP_TEST_CAPTURE_BUFF_SIZE)
#define REAL_CAPTURE_BUFF_SIZE          USB_AUDIO_CAPTURE_BUFF_SIZE
#else
#define REAL_CAPTURE_BUFF_SIZE          APP_TEST_CAPTURE_BUFF_SIZE
#endif

#define ALIGNED4                        ALIGNED(4)

static uint8_t ALIGNED4 app_test_playback_buff[REAL_PLAYBACK_BUFF_SIZE];
static uint8_t ALIGNED4 app_test_capture_buff[REAL_CAPTURE_BUFF_SIZE];

#if defined(APP_TEST_AUDIO) && defined(ANC_APP)
static uint8_t ALIGNED4 app_test_recv_buff[USB_AUDIO_RECV_BUFF_SIZE];
static uint8_t ALIGNED4 app_test_send_buff[USB_AUDIO_SEND_BUFF_SIZE];
#endif

uint32_t pcm_1ksin_more_data(uint8_t *buf, uint32_t len)
{
    static uint32_t nextPbufIdx = 0;
    uint32_t remain_size = len;
    uint32_t curr_size = 0;
    static uint32_t pcm_preIrqTime = 0;;
    uint32_t stime = 0;


    stime = hal_sys_timer_get();
    TRACE(3,"pcm_1ksin_more_data irqDur:%d readbuff:0x%08x %d\n ", TICKS_TO_MS(stime - pcm_preIrqTime), buf, len);
    pcm_preIrqTime = stime;

    //  TRACE(2,"[pcm_1ksin_more_data] len=%d nextBuf:%d\n", len, nextPbufIdx);
    if (remain_size > sizeof(sinwave))
    {
        do{
            if (nextPbufIdx)
            {
                curr_size = sizeof(sinwave)-nextPbufIdx;
                memcpy(buf,&sinwave[nextPbufIdx/2], curr_size);
                remain_size -= curr_size;
                nextPbufIdx = 0;
            }
            else if (remain_size>sizeof(sinwave))
            {
                memcpy(buf+curr_size,sinwave,sizeof(sinwave));
                curr_size += sizeof(sinwave);
                remain_size -= sizeof(sinwave);
            }
            else
            {
                memcpy(buf+curr_size,sinwave,remain_size);
                nextPbufIdx = remain_size;
                remain_size = 0;
            }
        }while(remain_size);
    }
    else
    {
        if ((sizeof(sinwave) - nextPbufIdx) >= len)
        {
            memcpy(buf, &sinwave[nextPbufIdx/2],len);
            nextPbufIdx += len;
        }
        else
        {
            curr_size = sizeof(sinwave)-nextPbufIdx;
            memcpy(buf, &sinwave[nextPbufIdx/2],curr_size);
            nextPbufIdx = len - curr_size;
            memcpy(buf+curr_size,sinwave, nextPbufIdx);
        }
    }

    return 0;
}

uint32_t pcm_mute_more_data(uint8_t *buf, uint32_t len)
{
    memset(buf, 0 , len);
    return 0;
}

void da_output_sin1k(bool  on)
{
    static bool isRun =  false;
    struct AF_STREAM_CONFIG_T stream_cfg;
    memset(&stream_cfg, 0, sizeof(stream_cfg));

    if (isRun==on)
        return;
    else
        isRun=on;
    TRACE(2,"%s %d\n", __func__, on);

    if (on){
        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = AUD_SAMPRATE_44100;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = 16;

        stream_cfg.handler = pcm_1ksin_more_data;
        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = APP_TEST_PLAYBACK_BUFF_SIZE;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }else{
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }

}

void da_tester(uint8_t on)
{
	da_output_sin1k(on);
}

extern int voicecvsd_audio_init(void);
extern uint32_t voicecvsd_audio_more_data(uint8_t *buf, uint32_t len);
extern int get_voicecvsd_buffer_size(void);
extern int store_voice_pcm2cvsd(unsigned char *buf, unsigned int len);

static uint32_t pcm_data_capture(uint8_t *buf, uint32_t len)
{
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;

    stime = hal_sys_timer_get();
//  audio_buffer_set_stereo2mono_16bits(buf, len, 1);
    audio_buffer_set(buf, len);
    etime = hal_sys_timer_get();
    TRACE(4,"%s irqDur:%d fsSpend:%d, Len:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len);
    preIrqTime = stime;
    return 0;
}

static uint32_t pcm_data_playback(uint8_t *buf, uint32_t len)
{
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;
    stime = hal_sys_timer_get();
//  audio_buffer_get_mono2stereo_16bits(buf, len);
    audio_buffer_get(buf, len);
    etime = hal_sys_timer_get();
    TRACE(4,"%s irqDur:%d fsSpend:%d, Len:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len);
    preIrqTime = stime;
    return 0;
}

uint32_t pcm_cvsd_data_capture(uint8_t *buf, uint32_t len)
{
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;

    //TRACE(1,"%s enter", __func__);
    stime = hal_sys_timer_get();
    len >>= 1;
    audio_stereo2mono_16bits(0, (uint16_t *)buf, (uint16_t *)buf, len);
    store_voice_pcm2cvsd(buf, len);
    etime = hal_sys_timer_get();
    TRACE(4,"%s exit irqDur:%d fsSpend:%d, add:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len);
    preIrqTime = stime;
    return 0;
}

uint32_t pcm_cvsd_data_playback(uint8_t *buf, uint32_t len)
{
    int n;
    uint32_t stime, etime;
    static uint32_t preIrqTime = 0;

    //TRACE(1,"%s enter", __func__);
    stime = hal_sys_timer_get();
    pcm_1ksin_more_data(buf, len);
    voicecvsd_audio_more_data(buf, len);
    n = get_voicecvsd_buffer_size();
    etime = hal_sys_timer_get();
    TRACE(5,"%s exit irqDur:%d fsSpend:%d, get:%d remain:%d", __func__, TICKS_TO_MS(stime - preIrqTime), TICKS_TO_MS(etime - stime), len, n);
    preIrqTime = stime;
    return 0;
}

void adc_looptester(bool on, enum AUD_IO_PATH_T input_path, enum AUD_SAMPRATE_T sample_rate)
{
    struct AF_STREAM_CONFIG_T stream_cfg;

    static bool isRun = false;

    if (isRun==on)
        return;
    else
        isRun=on;

    if (on){
        audio_buffer_init();
        memset(&stream_cfg, 0, sizeof(stream_cfg));

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = sample_rate;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = input_path;
        stream_cfg.vol = 0x03;

        stream_cfg.handler = pcm_data_capture;

        stream_cfg.data_ptr = app_test_capture_buff;
        stream_cfg.data_size = APP_TEST_CAPTURE_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.handler = pcm_data_playback;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = APP_TEST_PLAYBACK_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    }else{
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
}

#if defined(APP_TEST_AUDIO) && defined(ANC_APP)
void app_anc_usb_init(void)
{
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);

    anc_usb_app_init(AUD_INPUT_PATH_MAINMIC, AUD_SAMPRATE_96000, AUD_SAMPRATE_192000);

    struct USB_AUDIO_BUF_CFG cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.play_buf = app_test_playback_buff;
    cfg.play_size = USB_AUDIO_PLAYBACK_BUFF_SIZE;
    cfg.cap_buf = app_test_capture_buff;
    cfg.cap_size = USB_AUDIO_CAPTURE_BUFF_SIZE;
    cfg.recv_buf = app_test_recv_buff;
    cfg.recv_size = USB_AUDIO_RECV_BUFF_SIZE;
    cfg.send_buf = app_test_send_buff;
    cfg.send_size = USB_AUDIO_SEND_BUFF_SIZE;

    usb_audio_app_init(&cfg);

    //dualadc_audio_app_init(app_test_playback_buff, USB_AUDIO_PLAYBACK_BUFF_SIZE,
        //app_test_capture_buff, USB_AUDIO_CAPTURE_BUFF_SIZE);
}
#endif

