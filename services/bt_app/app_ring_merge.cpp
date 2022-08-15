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
#include "audioflinger.h"
#include "app_ring_merge.h"
#include "res_audio_ring.h"

// control queue access
osMutexId app_ring_merge_mutex_id = NULL;
osMutexDef(app_ring_merge_mutex);

extern "C" void app_stop_a2dp_media_stream(uint8_t devId);
extern "C" void app_stop_sco_media_stream(uint8_t devId);

static void app_ring_merge_finished_callback(void);

enum APP_RING_MERGE_STATUS {
    APP_RING_MERGE_STATUS_STOP,
    APP_RING_MERGE_STATUS_START,
    APP_RING_MERGE_STATUS_RUNNING
};

struct APP_RING_MERGE_CONFIG_T {
    APP_RING_MERGE_STATUS      status;
    int16_t *                  pbuf;
    uint32_t                   len;
    uint32_t                   next;
    AUD_BITS_T                 bits;
    enum APP_RING_MERGE_PLAY_T play;
    AF_STREAM_HANDLER_T        handler;

    uint8_t pendingStopOp;
    int8_t  savedStoppedStreamId;
};

struct APP_RING_MERGE_CONFIG_T app_ring_merge_config = {
    .status = APP_RING_MERGE_STATUS_STOP,
    .pbuf    = NULL,
    .len     = 0,
    .next    = 0,
    .bits    = AUD_BITS_16,
    .play    = APP_RING_MERGE_PLAY_QTY,
    .handler = NULL};

template <typename DataType>
static DataType convertTo(int16_t x)
{
    return (x);
}

template <>
int32_t convertTo<int32_t>(int16_t x)
{
    return (x << 8);
}

template <typename DataType>
static void app_ring_merge_track_2_in_1(DataType *src_buf0, int16_t *src_buf1, DataType *dst_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i++)
    {
        dst_buf[i] = (src_buf0[i] >> 1) + ((convertTo<DataType>(src_buf1[i])) >> 1);
    }
}

/*
static void app_ring_merge_track_2_in_1_16bits(int16_t *src_buf0, int16_t *src_buf1, int16_t *dst_buf,  uint32_t src_len)
{
    app_ring_merge_track_2_in_1<int16_t>(src_buf0, src_buf1, dst_buf, src_len);
}

static void app_ring_merge_track_2_in_1_24bits(int32_t *src_buf0, int16_t *src_buf1, int32_t *dst_buf,  uint32_t src_len)
{
    app_ring_merge_track_2_in_1<int32_t>(src_buf0, src_buf1, dst_buf, src_len);
}
*/

template <typename DataType>
static uint32_t app_ring_merge_oneshot_more_data_impl(uint8_t *buf, uint32_t len)
{
    uint32_t  need_len = len / sizeof(DataType);
    DataType *pBuf     = ( DataType * )buf;

    uint32_t curr_size = 0;

    if (app_ring_merge_config.next == app_ring_merge_config.len)
    {
        return len;
    }

    if (need_len > app_ring_merge_config.len)
    {
        app_ring_merge_track_2_in_1(pBuf, app_ring_merge_config.pbuf, pBuf, app_ring_merge_config.len);
        app_ring_merge_config.next    = app_ring_merge_config.len;
        app_ring_merge_finished_callback();
    }
    else
    {
        if ((app_ring_merge_config.len - app_ring_merge_config.next) >= need_len)
        {
            app_ring_merge_track_2_in_1(pBuf, app_ring_merge_config.pbuf + app_ring_merge_config.next, pBuf, need_len);
            app_ring_merge_config.next += need_len;
        }
        else
        {
            curr_size = app_ring_merge_config.len - app_ring_merge_config.next;
            app_ring_merge_track_2_in_1(pBuf, app_ring_merge_config.pbuf + app_ring_merge_config.next, pBuf, curr_size);
            app_ring_merge_config.next    = app_ring_merge_config.len;
            app_ring_merge_finished_callback();
        }
    }
    return len;
}

static uint32_t app_ring_merge_oneshot_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t ret = 0;

    if (app_ring_merge_config.bits == AUD_BITS_16)
        ret = app_ring_merge_oneshot_more_data_impl<int16_t>(buf, len);
    else if (app_ring_merge_config.bits == AUD_BITS_24)
        ret = app_ring_merge_oneshot_more_data_impl<int32_t>(buf, len);
    else
        TRACE(1,"[%s] warning not suitable callback available", __func__);

    return ret;
}

template <typename DataType>
static uint32_t app_ring_merge_periodic_more_data_impl(uint8_t *buf, uint32_t len)
{
    uint32_t  need_len    = len / sizeof(DataType);
    uint32_t  remain_size = len / sizeof(DataType);
    uint32_t  curr_size   = 0;
    DataType *pBuf        = ( DataType * )buf;
    if (need_len > app_ring_merge_config.len)
    {
        do
        {
            if (app_ring_merge_config.next)
            {
                curr_size = app_ring_merge_config.len - app_ring_merge_config.next;
                app_ring_merge_track_2_in_1(pBuf, app_ring_merge_config.pbuf + app_ring_merge_config.next, pBuf, curr_size);
                remain_size -= curr_size;
                app_ring_merge_config.next = 0;
            }
            else if (remain_size > app_ring_merge_config.len)
            {
                app_ring_merge_track_2_in_1(pBuf + curr_size, app_ring_merge_config.pbuf, pBuf + curr_size, app_ring_merge_config.len);
                curr_size += app_ring_merge_config.len;
                remain_size -= app_ring_merge_config.len;
            }
            else
            {
                app_ring_merge_track_2_in_1(pBuf + curr_size, app_ring_merge_config.pbuf, pBuf + curr_size, remain_size);
                app_ring_merge_config.next = remain_size;
                remain_size                = 0;
            }
        } while (remain_size);
    }
    else
    {
        if ((app_ring_merge_config.len - app_ring_merge_config.next) >= need_len)
        {
            app_ring_merge_track_2_in_1(pBuf, app_ring_merge_config.pbuf + app_ring_merge_config.next, pBuf, need_len);
            app_ring_merge_config.next += need_len;
        }
        else
        {
            curr_size = app_ring_merge_config.len - app_ring_merge_config.next;
            app_ring_merge_track_2_in_1(pBuf, app_ring_merge_config.pbuf + app_ring_merge_config.next, pBuf, curr_size);
            app_ring_merge_config.next = need_len - curr_size;
            app_ring_merge_track_2_in_1(pBuf + curr_size, app_ring_merge_config.pbuf, pBuf + curr_size, app_ring_merge_config.next);
        }
    }
    return len;
}

static uint32_t app_ring_merge_periodic_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t ret = 0;

    if (app_ring_merge_config.bits == AUD_BITS_16)
        ret = app_ring_merge_periodic_more_data_impl<int16_t>(buf, len);
    else if (app_ring_merge_config.bits == AUD_BITS_24)
        ret = app_ring_merge_periodic_more_data_impl<int32_t>(buf, len);
    else
        TRACE(1,"[%s] warning not suitable callback available", __func__);

    return ret;
}


uint32_t app_ring_merge_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t nRet = len;

    osMutexWait(app_ring_merge_mutex_id, osWaitForever);
    if (app_ring_merge_config.handler &&
        app_ring_merge_config.status != APP_RING_MERGE_STATUS_STOP)
    {
        if (app_ring_merge_config.status == APP_RING_MERGE_STATUS_START)
        {
            app_ring_merge_config.status = APP_RING_MERGE_STATUS_RUNNING;
        }
        nRet = app_ring_merge_config.handler(buf, len);
    }
    osMutexRelease(app_ring_merge_mutex_id);

    return nRet;
}

int app_ring_merge_setup(int16_t *buf, uint32_t len, enum APP_RING_MERGE_PLAY_T play)
{
    TRACE(3,"%s mode:%d len:%d", __func__, play, len);

    osMutexWait(app_ring_merge_mutex_id, osWaitForever);

    app_ring_merge_config.status  = APP_RING_MERGE_STATUS_STOP;
    app_ring_merge_config.pbuf    = ( int16_t * )buf;
    app_ring_merge_config.len     = len;
    app_ring_merge_config.next    = 0;

    app_ring_merge_config.play = play;

    if (play == APP_RING_MERGE_PLAY_ONESHOT)
    {
        app_ring_merge_config.handler = app_ring_merge_oneshot_more_data;
    }
    else if (play == APP_RING_MERGE_PLAY_PERIODIC)
    {
        app_ring_merge_config.handler = app_ring_merge_periodic_more_data;
    }

    osMutexRelease(app_ring_merge_mutex_id);

    return 0;
}

int app_ring_merge_init(void)
{
    if (app_ring_merge_mutex_id == NULL)
    {
        app_ring_merge_mutex_id = osMutexCreate((osMutex(app_ring_merge_mutex)));
    }

    app_ring_merge_config.savedStoppedStreamId = -1;
    return 0;
}

int app_ring_merge_deinit(void)
{
    osMutexWait(app_ring_merge_mutex_id, osWaitForever);

    app_ring_merge_config.status  = APP_RING_MERGE_STATUS_STOP;
    app_ring_merge_config.pbuf    = NULL;
    app_ring_merge_config.len     = 0;
    app_ring_merge_config.next    = 0;
    app_ring_merge_config.bits    = AUD_BITS_16;
    app_ring_merge_config.play    = APP_RING_MERGE_PLAY_QTY;
    app_ring_merge_config.handler = NULL;

    app_ring_merge_config.savedStoppedStreamId = -1;

    osMutexRelease(app_ring_merge_mutex_id);

    return 0;
}

int app_ring_merge_start(void)
{
    uint32_t                   ret;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;

    ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    if(ret != 0){
        TRACE(0,"Failed to get stream cfg");
        return 0;
    }
    TRACE(4,"%s: sample_rate: %d, bits: %d, channel: %d\n",
          __func__,
          stream_cfg->sample_rate,
          stream_cfg->bits,
          stream_cfg->channel_num);

    osMutexWait(app_ring_merge_mutex_id, osWaitForever);
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
    switch (stream_cfg->sample_rate)
    {
    case AUD_SAMPRATE_8000:
        app_ring_merge_setup(( int16_t * )RES_AUD_RING_SAMPRATE_8000, sizeof(RES_AUD_RING_SAMPRATE_8000) / sizeof(int16_t), APP_RING_MERGE_PLAY_ONESHOT);
        app_ring_merge_config.next    = 0;
        app_ring_merge_config.status = APP_RING_MERGE_STATUS_START;
        break;
    case AUD_SAMPRATE_16000:
        TRACE(0,"sample rate 16000 merge");
        app_ring_merge_setup(( int16_t * )RES_AUD_RING_SAMPRATE_16000, sizeof(RES_AUD_RING_SAMPRATE_16000) / sizeof(int16_t), APP_RING_MERGE_PLAY_ONESHOT);
        app_ring_merge_config.next    = 0;
        app_ring_merge_config.status = APP_RING_MERGE_STATUS_START;
        break;
    case AUD_SAMPRATE_44100:
        app_ring_merge_setup(( int16_t * )RES_AUD_RING_SAMPRATE_44100, sizeof(RES_AUD_RING_SAMPRATE_44100) / sizeof(int16_t), APP_RING_MERGE_PLAY_ONESHOT);
        app_ring_merge_config.next    = 0;
        app_ring_merge_config.status = APP_RING_MERGE_STATUS_START;
        break;

    case AUD_SAMPRATE_48000:
        app_ring_merge_setup(( int16_t * )RES_AUD_RING_SAMPRATE_48000, sizeof(RES_AUD_RING_SAMPRATE_48000) / sizeof(int16_t), APP_RING_MERGE_PLAY_ONESHOT);
        app_ring_merge_config.next    = 0;
        app_ring_merge_config.status = APP_RING_MERGE_STATUS_START;
        break;
    case AUD_SAMPRATE_22050:
    case AUD_SAMPRATE_24000:
    case AUD_SAMPRATE_96000:
    case AUD_SAMPRATE_192000:
    case AUD_SAMPRATE_NULL:
    default:
        app_ring_merge_config.next    = 0;
        app_ring_merge_config.status = APP_RING_MERGE_STATUS_START;
        break;
    }
#else
    app_ring_merge_setup(( int16_t * )RES_AUD_RING_SAMPRATE_8000,
                         sizeof(RES_AUD_RING_SAMPRATE_8000) / sizeof(int16_t),
                         APP_RING_MERGE_PLAY_ONESHOT);

    app_ring_merge_config.next    = 0;
    app_ring_merge_config.status = APP_RING_MERGE_STATUS_START;
#endif
    app_ring_merge_config.bits = stream_cfg->bits;

    app_ring_merge_config.savedStoppedStreamId = -1;

    osMutexRelease(app_ring_merge_mutex_id);

    return 0;
}

int app_ring_merge_stop(void)
{
    osMutexWait(app_ring_merge_mutex_id, osWaitForever);
    app_ring_merge_config.status = APP_RING_MERGE_STATUS_STOP;
    app_ring_merge_config.next    = 0;
    osMutexRelease(app_ring_merge_mutex_id);

    app_ring_merge_finished_callback();

    return 0;
}

bool app_ring_merge_isrun(void)
{
    bool running = false;
    osMutexWait(app_ring_merge_mutex_id, osWaitForever);
    if (app_ring_merge_config.status == APP_RING_MERGE_STATUS_RUNNING)
    {
        running =  true;
    }
    osMutexRelease(app_ring_merge_mutex_id);
    return running;
}

void app_ring_merge_save_pending_start_stream_op(uint8_t pendingStopOp, uint8_t deviceId)
{
    app_ring_merge_config.pendingStopOp        = pendingStopOp;
    app_ring_merge_config.savedStoppedStreamId = deviceId;
}

static void app_ring_merge_finished_callback(void)
{
    TRACE(1,"%s", __func__);
    app_ring_merge_config.status = APP_RING_MERGE_STATUS_STOP;

    if (app_ring_merge_config.savedStoppedStreamId >= 0)
    {
        if (PENDING_TO_STOP_A2DP_STREAMING == app_ring_merge_config.pendingStopOp)
        {
            TRACE(0,"Stop the pending stopped a2dp media stream.");
            app_stop_a2dp_media_stream(app_ring_merge_config.savedStoppedStreamId);
        }
        else if (PENDING_TO_STOP_SCO_STREAMING == app_ring_merge_config.pendingStopOp)
        {
            TRACE(0,"Stop the pending stopped sco media stream.");
            app_stop_sco_media_stream(app_ring_merge_config.savedStoppedStreamId);
        }
    }
    app_ring_merge_config.savedStoppedStreamId = -1;
}
