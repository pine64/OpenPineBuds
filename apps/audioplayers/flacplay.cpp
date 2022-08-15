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
#include "cqueue.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif

/* mutex */
osMutexId g_flac_queue_mutex_id;
osMutexDef(g_flac_queue_mutex);

/* flac queue */
#define FLAC_TEMP_BUFFER_SIZE 2048
#define FLAC_QUEUE_SIZE (FLAC_TEMP_BUFFER_SIZE*4)
unsigned char flac_queue_buf[FLAC_QUEUE_SIZE];
CQueue flac_queue;
static uint32_t ok_to_decode = 0;

#define LOCK_FLAC_QUEUE() \
	osMutexWait(g_flac_queue_mutex_id, osWaitForever)

#define UNLOCK_FLAC_QUEUE() \
	osMutexRelease(g_flac_queue_mutex_id)

static void copy_one_trace_to_two_track_16bits(uint16_t *src_buf, uint16_t *dst_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; ++i) {
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

int store_flac_buffer(unsigned char *buf, unsigned int len)
{
	LOCK_FLAC_QUEUE();
	EnCQueue(&flac_queue, buf, len);
    if (LengthOfCQueue(&flac_queue) > FLAC_TEMP_BUFFER_SIZE*2) {
        ok_to_decode = 1;
    }
	UNLOCK_FLAC_QUEUE();

	return 0;
}

int decode_flac_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
	uint32_t r = 0, got_len = 0;
	unsigned char *e1 = NULL, *e2 = NULL;
	unsigned int len1 = 0, len2 = 0;
get_again:
	LOCK_FLAC_QUEUE();
	r = PeekCQueue(&flac_queue, (pcm_len - got_len)/2, &e1, &len1, &e2, &len2);
	UNLOCK_FLAC_QUEUE();

	if(r == CQ_ERR || len1 == 0) {
		osDelay(2);
		goto get_again;
	}

    //memcpy(pcm_buffer + got_len, e1, len1);
    copy_one_trace_to_two_track_16bits((uint16_t *)e1, (uint16_t *)(pcm_buffer + got_len), len1/2);

    LOCK_FLAC_QUEUE();
    DeCQueue(&flac_queue, 0, len1);
    UNLOCK_FLAC_QUEUE();

    got_len += len1*2;

    if (len2 != 0) {
        //memcpy(pcm_buffer + got_len, e2, len2);
        copy_one_trace_to_two_track_16bits((uint16_t *)e2, (uint16_t *)(pcm_buffer + got_len), len2/2);

        LOCK_FLAC_QUEUE();
        DeCQueue(&flac_queue, 0, len2);
        UNLOCK_FLAC_QUEUE();
    }

    got_len += len2*2;

    if (got_len < pcm_len)
        goto get_again;

    return pcm_len;
}

uint32_t flac_audio_data_come(uint8_t *buf, uint32_t len)
{
    TRACE(1,"data come %d\n", len);

    return 0;
}

uint32_t flac_audio_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
    //uint32_t cur_ticks = 0, ticks = 0;

    if (ok_to_decode == 0)
        return 0;

    //ticks = hal_sys_timer_get();
    l = decode_flac_frame(buf, len);
    //cur_ticks = hal_sys_timer_get();
//    TRACE(1,"flac %d t\n", (cur_ticks-ticks));

    return l;
}

int flac_audio_init(void)
{
    g_flac_queue_mutex_id = osMutexCreate((osMutex(g_flac_queue_mutex)));
	/* flac queue*/
	InitCQueue(&flac_queue, FLAC_QUEUE_SIZE, (unsigned char *)&flac_queue_buf);

	return 0;
}
