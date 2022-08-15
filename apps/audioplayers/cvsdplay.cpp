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
#ifdef MBED
#include "mbed.h"
#endif
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef MBED
//#include "rtos.h"
#endif
#ifdef MBED
#include "SDFileSystem.h"
#endif
#include "cqueue.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "app_audio.h"


// BT

#if 0
/* mutex */
osMutexId g_voicecvsd_queue_mutex_id = NULL;
osMutexDef(g_voicecvsd_queue_mutex);

/* cvsd queue */
#define VOICECVSD_TEMP_BUFFER_SIZE 128
#define VOICECVSD_QUEUE_SIZE (VOICECVSD_TEMP_BUFFER_SIZE*20)
CQueue voicecvsd_queue;

static enum APP_AUDIO_CACHE_T voicecvsd_cache_status = APP_AUDIO_CACHE_QTY;

#define LOCK_VOICECVSD_QUEUE() \
    osMutexWait(g_voicecvsd_queue_mutex_id, osWaitForever)

#define UNLOCK_VOICECVSD_QUEUE() \
    osMutexRelease(g_voicecvsd_queue_mutex_id)

void xLOCK_VOICECVSD_QUEUE(void)
{
    osMutexWait(g_voicecvsd_queue_mutex_id, osWaitForever);
}

void xUNLOCK_VOICECVSD_QUEUE(void)
{
    osMutexRelease(g_voicecvsd_queue_mutex_id);
}

//static void dump_buffer_to_psram(char *buf, unsigned int len)
//{
//    static unsigned int offset = 0;
//    memcpy((void *)(0x1c000000 + offset), buf, len);
//}

static void copy_one_trace_to_two_track_16bits(uint16_t *src_buf, uint16_t *dst_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; ++i) {
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

int store_voicecvsd_buffer(unsigned char *buf, unsigned int len)
{
    LOCK_VOICECVSD_QUEUE();
    EnCQueue(&voicecvsd_queue, buf, len);
#if 0
    dump_buffer_to_psram((char *)buf, len);
    TRACE(1,"%d\n", LengthOfCQueue(&voicecvsd_queue));
#endif
    if (LengthOfCQueue(&voicecvsd_queue) > VOICECVSD_TEMP_BUFFER_SIZE*10) {
        voicecvsd_cache_status = APP_AUDIO_CACHE_OK;
    }
    UNLOCK_VOICECVSD_QUEUE();

    return 0;
}

int store_voice_pcm2cvsd(unsigned char *buf, unsigned int len)
{
    uint8_t cvsdtmpbuffer[VOICECVSD_TEMP_BUFFER_SIZE];

    short * pBuffProcessed = (short *)buf;
    uint32_t processed_len = 0;
    uint32_t remain_len = 0;

    len >>= 1;

    LOCK_VOICECVSD_QUEUE();
    while(processed_len < len){
        remain_len = len-processed_len;
        if (remain_len>VOICECVSD_TEMP_BUFFER_SIZE){
            Pcm8kToCvsd(pBuffProcessed + processed_len, cvsdtmpbuffer, VOICECVSD_TEMP_BUFFER_SIZE);
            EnCQueue(&voicecvsd_queue, cvsdtmpbuffer, VOICECVSD_TEMP_BUFFER_SIZE);
            processed_len += VOICECVSD_TEMP_BUFFER_SIZE;
        }else{
            Pcm8kToCvsd(pBuffProcessed + processed_len, cvsdtmpbuffer, remain_len);
            EnCQueue(&voicecvsd_queue, cvsdtmpbuffer, remain_len);
            processed_len += remain_len;
        }
    }
    UNLOCK_VOICECVSD_QUEUE();

#if 0
    dump_buffer_to_psram((char *)buf, len);
    TRACE(1,"%d\n", LengthOfCQueue(&voicecvsd_queue));
#endif
    if (LengthOfCQueue(&voicecvsd_queue) > VOICECVSD_TEMP_BUFFER_SIZE*10) {
        voicecvsd_cache_status = APP_AUDIO_CACHE_OK;
    }

    return 0;
}

int get_voicecvsd_buffer_size(void)
{
    int n;
    LOCK_VOICECVSD_QUEUE();
    n = LengthOfCQueue(&voicecvsd_queue);
    UNLOCK_VOICECVSD_QUEUE();
    return n;
}

static short tmp_decode_buf[VOICECVSD_TEMP_BUFFER_SIZE*2];
int decode_cvsd_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    uint32_t r = 0, decode_len = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    while (decode_len < pcm_len) {
get_again:
        LOCK_VOICECVSD_QUEUE();
        len1 = len2 = 0;
        r = PeekCQueue(&voicecvsd_queue, VOICECVSD_TEMP_BUFFER_SIZE, &e1, &len1, &e2, &len2);
        UNLOCK_VOICECVSD_QUEUE();

        if(r == CQ_ERR || len1 == 0) {
            //return 0;
            Thread::wait(4);
            goto get_again;
        }

        if (len1 != 0) {
            //CvsdToPcm8k(e1, (short *)(pcm_buffer + decode_len*4), len1, 0);
            CvsdToPcm8k(e1, (short *)(tmp_decode_buf), len1, 0);
            copy_one_trace_to_two_track_16bits((uint16_t *)tmp_decode_buf, (uint16_t *)(pcm_buffer + decode_len), len1);

            LOCK_VOICECVSD_QUEUE();
            DeCQueue(&voicecvsd_queue, 0, len1);
            UNLOCK_VOICECVSD_QUEUE();

            decode_len += len1*4;
        }

        if (len2 != 0) {
            //CvsdToPcm8k(e2, (short *)(pcm_buffer + decode_len*4), len2, 0);
            CvsdToPcm8k(e2, (short *)(tmp_decode_buf), len2, 0);
            copy_one_trace_to_two_track_16bits((uint16_t *)tmp_decode_buf, (uint16_t *)(pcm_buffer + decode_len), len2);

            LOCK_VOICECVSD_QUEUE();
            DeCQueue(&voicecvsd_queue, 0, len2);
            UNLOCK_VOICECVSD_QUEUE();

            decode_len += len2*4;
        }
    }

    return 0;
}

uint32_t decode_cvsd_frame_noblock(uint8_t *pcm_buffer, uint32_t pcm_len)
{
    unsigned int len[2];
    uint8_t *e[2];
    uint32_t decode_len = 0;

    LOCK_VOICECVSD_QUEUE();
    while (decode_len < pcm_len)
    {
        len[0] = len[1] = 0;
        e[0] = e[1] = NULL;

        if(PeekCQueue(&voicecvsd_queue, VOICECVSD_TEMP_BUFFER_SIZE, &e[0], &len[0], &e[1], &len[1]) == CQ_OK)
        {
            for(uint8_t i=0; i<2; i++)
            {
                if (len[i] != 0)
                {
                    CvsdToPcm8k(e[i], (short *)(tmp_decode_buf), len[i], 0);

                    copy_one_trace_to_two_track_16bits((uint16_t *)tmp_decode_buf, (uint16_t *)(pcm_buffer + decode_len), len[i]);

                    DeCQueue(&voicecvsd_queue, 0, len[i]);

                    decode_len += len[i]*4;
                }
            }
        }
        else
        {
            break;
        }
    }
    UNLOCK_VOICECVSD_QUEUE();
    return decode_len;
}

uint32_t voicecvsd_audio_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
    uint32_t cur_ticks = 0, ticks = 0;

    if (voicecvsd_cache_status == APP_AUDIO_CACHE_CACHEING)
       return 0;

    TRACE(2,"%s enter reamin:%d", __func__, LengthOfCQueue(&voicecvsd_queue));

    ticks = hal_sys_timer_get();
    l = decode_cvsd_frame_noblock(buf, len);
    cur_ticks = hal_sys_timer_get();
    TRACE(3,"%s exit  reamin:%d spend:%d\n", __func__, LengthOfCQueue(&voicecvsd_queue) ,TICKS_TO_MS(cur_ticks-ticks));

    return l;
}

int voicecvsd_audio_init(void)
{
    uint8_t *buff = NULL;

    app_audio_mempool_get_buff(&buff, VOICECVSD_QUEUE_SIZE);
    memset(buff, 0, VOICECVSD_QUEUE_SIZE);

    /* voicebtpcm_pcm queue*/
    APP_AUDIO_InitCQueue(&voicecvsd_queue, VOICECVSD_QUEUE_SIZE, buff);

    Pcm8k_CvsdInit();
    if (g_voicecvsd_queue_mutex_id == NULL)
    {
        g_voicecvsd_queue_mutex_id = osMutexCreate((osMutex(g_voicecvsd_queue_mutex)));
    }

	voicecvsd_cache_status = APP_AUDIO_CACHE_CACHEING;
}
#endif
