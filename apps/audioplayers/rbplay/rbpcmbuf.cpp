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
/* rbpcmbuf source */
/* pcmbuf management & af control & mixer */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#ifdef MBED
#include "mbed.h"
#include "rtos.h"
#endif
#include "audioflinger.h"
#include "cqueue.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_trace.h"

#include "rbplay.h"
#include "rbpcmbuf.h"
#include "utils.h"

#define RB_PCMBUF_DMA_BUFFER_SIZE (1024*12)
#define RB_PCMBUF_MEDIA_BUFFER_SIZE (1024*12)
#define RB_DECODE_OUT_BUFFER_SIZE 1024

static uint8_t *rb_decode_out_buff;
static uint8_t *rbplay_dma_buffer;
static uint8_t *rb_pcmbuf_media_buf;
static CQueue rb_pcmbuf_media_buf_queue;
static osMutexId _rb_media_buf_queue_mutex_id = NULL;
static osMutexDef(_rb_media_buf_queue_mutex);

#define LOCK_MEDIA_BUF_QUEUE() \
    if(osErrorISR == osMutexWait(_rb_media_buf_queue_mutex_id, osWaitForever))	{\
        error("%s LOCK_MEDIA_BUF_QUEUE from IRQ!!!!!!!\n",__func__);\
    }\

#define UNLOCK_MEDIA_BUF_QUEUE() \
    if(osErrorISR == osMutexRelease(_rb_media_buf_queue_mutex_id)){    \
        error("%s UNLOCK_MEDIA_BUF_QUEUE from IRQ!!!!!!\n");     \
    }    \

static uint32_t rbplay_more_data(uint8_t *buf, uint32_t len)
{
    CQItemType *e1 = NULL;
    CQItemType *e2 = NULL;
    unsigned int len1 = 0;
    unsigned int len2 = 0;

    LOCK_MEDIA_BUF_QUEUE();
    int ret = PeekCQueue(&rb_pcmbuf_media_buf_queue, len, &e1, &len1, &e2, &len2);
    UNLOCK_MEDIA_BUF_QUEUE();

    if (ret == CQ_OK) {
        if (len1 > 0)
            memcpy(buf, e1, len1);
        if (len2 > 0)
            memcpy(buf + len1, e2, len - len1);
        LOCK_MEDIA_BUF_QUEUE();
        DeCQueue(&rb_pcmbuf_media_buf_queue, 0, len);
        UNLOCK_MEDIA_BUF_QUEUE();
    } else {
        warn("RBplay cache underflow");
    }

    return len;
}

extern uint8_t rb_ctl_get_vol(void);
void rb_pcmbuf_init(void)
{
    info("pcmbuff init");
    if(!_rb_media_buf_queue_mutex_id)
        _rb_media_buf_queue_mutex_id  = osMutexCreate((osMutex(_rb_media_buf_queue_mutex)));

    app_audio_mempool_init();

    app_audio_mempool_get_buff(&rb_pcmbuf_media_buf, RB_PCMBUF_MEDIA_BUFFER_SIZE);
    InitCQueue(&rb_pcmbuf_media_buf_queue, RB_PCMBUF_MEDIA_BUFFER_SIZE, (unsigned char *)rb_pcmbuf_media_buf);

    app_audio_mempool_get_buff(&rbplay_dma_buffer, RB_PCMBUF_DMA_BUFFER_SIZE);
    app_audio_mempool_get_buff(&rb_decode_out_buff, RB_DECODE_OUT_BUFFER_SIZE);

    struct AF_STREAM_CONFIG_T stream_cfg;

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.bits = AUD_BITS_16;
    stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
    stream_cfg.sample_rate = AUD_SAMPRATE_44100;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
    stream_cfg.vol = rb_ctl_get_vol();
    stream_cfg.handler = rbplay_more_data;
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(rbplay_dma_buffer);
    stream_cfg.data_size = RB_PCMBUF_DMA_BUFFER_SIZE;

    af_stream_open(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK, &stream_cfg);
    af_stream_start(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK);
}

void *rb_pcmbuf_request_buffer(int *size)
{
    *size = RB_DECODE_OUT_BUFFER_SIZE / 4;
    return rb_decode_out_buff;
}

void rb_pcmbuf_write(unsigned int size)
{
    int ret ;
    do {
        LOCK_MEDIA_BUF_QUEUE();
        ret = EnCQueue(&rb_pcmbuf_media_buf_queue, (CQItemType *)rb_decode_out_buff, size*(2*2));
        UNLOCK_MEDIA_BUF_QUEUE();
        osThreadYield();
    } while (ret == CQ_ERR);
}

void rb_pcmbuf_stop(void)
{
    af_stream_stop(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK);
    af_stream_close(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK);
}

