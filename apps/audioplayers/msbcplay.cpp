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
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "cqueue.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"

// BT


#if 0
/* mutex */
osMutexId g_voicemsbc_queue_mutex_id;
osMutexDef(g_voicemsbc_queue_mutex);

/* msbc queue */
#define VOICEMSBC_TEMP_BUFFER_SIZE 128
#define VOICEMSBC_QUEUE_SIZE (VOICEMSBC_TEMP_BUFFER_SIZE*100)
unsigned char voicemsbc_queue_buf[VOICEMSBC_QUEUE_SIZE];
CQueue voicemsbc_queue;
static uint32_t ok_to_decode = 0;

#define LOCK_VOICEMSBC_QUEUE() \
	osMutexWait(g_voicemsbc_queue_mutex_id, osWaitForever)

#define UNLOCK_VOICEMSBC_QUEUE() \
	osMutexRelease(g_voicemsbc_queue_mutex_id)

static void dump_buffer_to_psram(char *buf, unsigned int len)
{
    static unsigned int offset = 0;
    memcpy((void *)(0x1c000000 + offset), buf, len);
}

static void copy_one_trace_to_two_track_16bits(uint16_t *src_buf, uint16_t *dst_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; ++i) {
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

int store_voicemsbc_buffer(unsigned char *buf, unsigned int len)
{
	LOCK_VOICEMSBC_QUEUE();
	EnCQueue(&voicemsbc_queue, buf, len);
    dump_buffer_to_psram((char *)buf, len);
    TRACE(1,"%d\n", LengthOfCQueue(&voicemsbc_queue));
    if (LengthOfCQueue(&voicemsbc_queue) > VOICEMSBC_TEMP_BUFFER_SIZE*20) {
        ok_to_decode = 1;
    }
	UNLOCK_VOICEMSBC_QUEUE();

	return 0;
}

static short tmp_decode_buf[VOICEMSBC_TEMP_BUFFER_SIZE*2];
int decode_msbc_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
	int r = 0;
    uint32_t decode_len = 0;
	unsigned char *e1 = NULL, *e2 = NULL;
	unsigned int len1 = 0, len2 = 0;

    while (decode_len < pcm_len) {
get_again:
        LOCK_VOICEMSBC_QUEUE();
        len1 = len2 = 0;
        r = PeekCQueue(&voicemsbc_queue, VOICEMSBC_TEMP_BUFFER_SIZE, &e1, &len1, &e2, &len2);
        UNLOCK_VOICEMSBC_QUEUE();

        if(r == CQ_ERR || len1 == 0) {
            //return 0;
            Thread::wait(4);
            goto get_again;
        }

        if (len1 != 0) {
            //msbcToPcm8k(e1, (short *)(pcm_buffer + decode_len*4), len1, 0);
            CvsdToPcm8k(e1, (short *)(tmp_decode_buf), len1, 0);
            copy_one_trace_to_two_track_16bits((uint16_t *)tmp_decode_buf, (uint16_t *)(pcm_buffer + decode_len), len1);

            LOCK_VOICEMSBC_QUEUE();
            DeCQueue(&voicemsbc_queue, 0, len1);
            UNLOCK_VOICEMSBC_QUEUE();

            decode_len += len1*4;
        }

        if (len2 != 0) {
            //msbcToPcm8k(e2, (short *)(pcm_buffer + decode_len*4), len2, 0);
            CvsdToPcm8k(e2, (short *)(tmp_decode_buf), len2, 0);
            copy_one_trace_to_two_track_16bits((uint16_t *)tmp_decode_buf, (uint16_t *)(pcm_buffer + decode_len), len2);

            LOCK_VOICEMSBC_QUEUE();
            DeCQueue(&voicemsbc_queue, 0, len2);
            UNLOCK_VOICEMSBC_QUEUE();

            decode_len += len2*4;
        }
    }

    return 0;
}

uint32_t voicemsbc_audio_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
    uint32_t cur_ticks = 0, ticks = 0;

    if (ok_to_decode == 0)
        return 0;

    ticks = hal_sys_timer_get();
    l = decode_msbc_frame(buf, len);
    cur_ticks = hal_sys_timer_get();
    TRACE(1,"msbc %d t\n", (cur_ticks-ticks));

    return l;
}

int voicemsbc_audio_init(void)
{
    Pcm8k_CvsdInit();

    g_voicemsbc_queue_mutex_id = osMutexCreate((osMutex(g_voicemsbc_queue_mutex)));
	/* msbc queue*/
	InitCQueue(&voicemsbc_queue, VOICEMSBC_QUEUE_SIZE, (unsigned char *)&voicemsbc_queue_buf);

	return 0;
}
#endif
