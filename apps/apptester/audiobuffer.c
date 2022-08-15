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
#include "cqueue.h"
#include "string.h"
#include "audiobuffer.h"


#ifndef _AUDIO_NO_THREAD_
static osMutexId g_audio_queue_mutex_id = NULL;
osMutexDef(g_audio_queue_mutex);
#endif

static CQueue audio_queue;
static unsigned char audio_queue_buf[AUDIO_BUFFER_FRAME_SIZE*AUDIO_BUFFER_FRAME_NUM];

void audio_mono2stereo_16bits(uint16_t *dst_buf, uint16_t *src_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; ++i) {
        dst_buf[i*2 + 0] = dst_buf[i*2 + 1] = src_buf[i];
    }
}

void audio_stereo2mono_16bits(uint8_t chnlsel, uint16_t *dst_buf, uint16_t *src_buf, uint32_t src_len)
{
    uint32_t i = 0;
    for (i = 0; i < src_len; i+=2) {
        dst_buf[i/2] = src_buf[i + chnlsel];
    }
}

void audio_buffer_init(void)
{
#ifndef _AUDIO_NO_THREAD_
	if (g_audio_queue_mutex_id == NULL)
		g_audio_queue_mutex_id = osMutexCreate((osMutex(g_audio_queue_mutex)));
#endif
	InitCQueue(&audio_queue, sizeof(audio_queue_buf), (unsigned char *)&audio_queue_buf);
	memset(&audio_queue_buf, 0x00, sizeof(audio_queue_buf));
}

int audio_buffer_length(void)
{
	int len;
#ifndef _AUDIO_NO_THREAD_
	osMutexWait(g_audio_queue_mutex_id, osWaitForever);
#endif
	len = LengthOfCQueue(&audio_queue);
#ifndef _AUDIO_NO_THREAD_
	osMutexRelease(g_audio_queue_mutex_id);
#endif
	return len;
}

int audio_buffer_set(uint8_t *buff, uint16_t len)
{
	int status;

#ifndef _AUDIO_NO_THREAD_
	osMutexWait(g_audio_queue_mutex_id, osWaitForever);
#endif
	status = EnCQueue(&audio_queue, buff, len);
#ifndef _AUDIO_NO_THREAD_
	osMutexRelease(g_audio_queue_mutex_id);
#endif
	return status;
}

int audio_buffer_get(uint8_t *buff, uint16_t len)
{
	uint8_t *e1 = NULL, *e2 = NULL;
	unsigned int len1 = 0, len2 = 0;
	int status;
#ifndef _AUDIO_NO_THREAD_
	osMutexWait(g_audio_queue_mutex_id, osWaitForever);
#endif
	status = PeekCQueue(&audio_queue, len, &e1, &len1, &e2, &len2);
	if (len==(len1+len2)){
		memcpy(buff,e1,len1);	
		memcpy(buff+len1,e2,len2);
		DeCQueue(&audio_queue, 0, len);
        DeCQueue(&audio_queue, 0, len2);
	}else{
		memset(buff, 0x00, len);
		status = -1;
	}
#ifndef _AUDIO_NO_THREAD_
	osMutexRelease(g_audio_queue_mutex_id);	
#endif
	return status;
}

int audio_buffer_set_stereo2mono_16bits(uint8_t *buff, uint16_t len, uint8_t chnlsel)
{
	int status;

#ifndef _AUDIO_NO_THREAD_
		osMutexWait(g_audio_queue_mutex_id, osWaitForever);
#endif
		audio_stereo2mono_16bits(chnlsel, (uint16_t *)buff, (uint16_t *)buff, len>>1);
		status = EnCQueue(&audio_queue, buff, len>>1);
#ifndef _AUDIO_NO_THREAD_
		osMutexRelease(g_audio_queue_mutex_id);
#endif
	return status;
}

int audio_buffer_get_mono2stereo_16bits(uint8_t *buff, uint16_t len)
{
	uint8_t *e1 = NULL, *e2 = NULL;
	unsigned int len1 = 0, len2 = 0;
	int status;

#ifndef _AUDIO_NO_THREAD_
	osMutexWait(g_audio_queue_mutex_id, osWaitForever);
#endif
	status = PeekCQueue(&audio_queue, len>>1, &e1, &len1, &e2, &len2);
	if (len>>1== len1+len2){
		audio_mono2stereo_16bits((uint16_t *)buff, (uint16_t *)e1, len1>>1);
		audio_mono2stereo_16bits((uint16_t *)(buff+(len1<<1)), (uint16_t *)e2, len2>>1);
		DeCQueue(&audio_queue, 0, len1);
        DeCQueue(&audio_queue, 0, len2);
		status = len;
	}else{
		memset(buff, 0x00, len);
		status = -1;
	}
#ifndef _AUDIO_NO_THREAD_
	osMutexRelease(g_audio_queue_mutex_id);
#endif
	return status;
}
