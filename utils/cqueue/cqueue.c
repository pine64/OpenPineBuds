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
/***
* cqueue.c - c circle queue c file
*/

#include "cqueue.h"
#include <stdio.h>
#include "cmsis.h"
#include "hal_uart.h"
#include <string.h>

//#define DEBUG_CQUEUE 1

CQ_FUNC_ATTR
int InitCQueue(CQueue *Q, unsigned int size, CQItemType *buf)
{
    Q->size = size;
    Q->base = buf;
    Q->len  = 0;
    if (!Q->base)
        return CQ_ERR;

    Q->read = Q->write = 0;
    return CQ_OK;
}

CQ_FUNC_ATTR
int IsEmptyCQueue(CQueue *Q)
{
    if (Q->len == 0)
        return CQ_OK;
    else
        return CQ_ERR;
}

CQ_FUNC_ATTR
int LengthOfCQueue(CQueue *Q)
{
    return Q->len;
}

CQ_FUNC_ATTR
int AvailableOfCQueue(CQueue *Q)
{
    return (Q->size - Q->len);
}

#if 1
CQ_FUNC_ATTR
int EnCQueue(CQueue *Q, CQItemType *e, unsigned int len)
{
    if (AvailableOfCQueue(Q) < len) {
        return CQ_ERR;
    }

    Q->len += len;

    uint32_t bytesToTheEnd = Q->size - Q->write;
    if (bytesToTheEnd > len)
    {
        memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, len);
        Q->write += len;
    }
    else
    {
        memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, bytesToTheEnd);
        memcpy((uint8_t *)&Q->base[0], (((uint8_t *)e)+bytesToTheEnd), len-bytesToTheEnd);
        Q->write = len-bytesToTheEnd;
    }

    return CQ_OK;
}


int EnCQueue_AI(CQueue *Q, CQItemType *e, unsigned int len)
{
	uint32_t bytesToTheEnd = Q->size - Q->write;
	uint32_t readBytesToTheEnd = Q->size - Q->read;

    if(AvailableOfCQueue(Q) < len) {
		if(readBytesToTheEnd > len) {
			Q->read += len;
		} else {
			Q->read = len - readBytesToTheEnd;
		}
    } else {
        Q->len += len;
    }
    
	if (bytesToTheEnd > len) {
		memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, len);
		Q->write += len;
	} else {
		memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, bytesToTheEnd);
		memcpy((uint8_t *)&Q->base[0], (((uint8_t *)e)+bytesToTheEnd), len-bytesToTheEnd);
		Q->write = len-bytesToTheEnd;
	}
    
    return CQ_OK;
}

#else
static inline void memcpy_u8(void *dst, void *src, unsigned int len)
{
    unsigned int i = 0;
    unsigned int *d8, *s8;
    d8 = dst;
    s8 = src;
    for(i=0;i < len;i++)
        d8[i] = s8[i];
}

static inline void memcpy_u32(void *dst, void *src, unsigned int len)
{
    unsigned int i = 0;
    unsigned int *d32, *s32;
    d32 = dst;
    s32 = src;
    for(i = 0;i < len;i++)
        d32[i] = s32[i];
}

CQ_FUNC_ATTR
int EnCQueue(CQueue *Q, CQItemType *e, unsigned int len)
{
    unsigned char *src = e;
    unsigned char *dst = &(Q->base[Q->write]);

    unsigned int cnt_u8;
    unsigned int cnt_u32;
    unsigned int cnt_u32_res;

    unsigned int front_u8;
    unsigned int front_u32;
    unsigned int front_u32_res;

    unsigned int end_u8;
    unsigned int end_u32;
    unsigned int end_u32_res;

    bool unaligned_en;

    if (AvailableOfCQueue(Q) < len) {
        return CQ_ERR;
    }

    Q->len += len;

    end_u8 = Q->size - Q->write;
    end_u32 = end_u8 / 4;
    end_u32_res = end_u8 % 4;

    cnt_u8 = len;
    cnt_u32 = cnt_u8 / 4;
    cnt_u32_res = cnt_u8 % 4;

    unaligned_en = config_unaligned_access(true);

    if(cnt_u8 <= end_u8)
    {
        memcpy_u32(dst, src, cnt_u32);

        if(cnt_u32_res)
        {
            src += cnt_u32 * 4;
            dst += cnt_u32 * 4;
            memcpy_u8(dst, src, cnt_u32_res);
        }

        // Deal with Q->write
        Q->write += cnt_u8;

    }
    else
    {
        front_u8 = len - end_u8;
        front_u32 = front_u8 / 4;
        front_u32_res = front_u8 % 4;

        // Deal with end data
        memcpy_u32(dst, src, end_u32);
        src += end_u32 * 4;
        dst += end_u32 * 4;

        memcpy_u8(dst, src, end_u32_res);
        src += end_u32_res;
        dst = &(Q->base[0]);

        // Deal with front data
        memcpy_u32(dst, src, front_u32);

        if(front_u32_res)
        {
            src += front_u32 * 4;
            dst += front_u32 * 4;

            memcpy_u8(dst, src, front_u32_res);
        }

        // Deal with Q->write
        Q->write = front_u8;
    }

    config_unaligned_access(unaligned_en);

    return CQ_OK;
}

void memcpy_fast(char *dst, char *src, unsigned int len)
{
    int size = len/4;
    int mod = len%4;

    int *Dst = (int *)dst;
    int *Src = (int *)src;

    int dst_offset = (int)Dst - (int)dst;
    int src_offset = (int)Src -(int)src;

    if(!dst_offset && !src_offset){
        memcpy_u32(Dst, Src, size);
        if(mod){
            memcpy_u8(Dst + size, Src + size, mod);
        }
    }else{
        memcpy_u8(dst, src, len);
    }
}

#endif

CQ_FUNC_ATTR
int EnCQueueFront(CQueue *Q, CQItemType *e, unsigned int len)
{
    if (AvailableOfCQueue(Q) < len) {
        return CQ_ERR;
    }

    Q->len += len;

    /* walk to last item , revert write */
    e = e + len - 1;

    if(Q->read == 0)
        Q->read = Q->size - 1;
    else
        Q->read--;

    while(len > 0) {
        Q->base[Q->read] = *e;

        --Q->read;
        --e;
        --len;

        if(Q->read < 0)
            Q->read = Q->size - 1;
    }

    /* we walk one more, walk back */
    if(Q->read == Q->size - 1)
        Q->read = 0;
    else
        ++Q->read;

    return CQ_OK;
}

CQ_FUNC_ATTR
int DeCQueue(CQueue *Q, CQItemType *e, unsigned int len)
{
    if(LengthOfCQueue(Q) < len)
        return CQ_ERR;

    Q->len -= len;

    if(e != NULL)
    {
        uint32_t bytesToTheEnd = Q->size - Q->read;
        if (bytesToTheEnd > len)
        {
            memcpy((uint8_t *)e, (uint8_t *)&Q->base[Q->read], len);
            Q->read += len;
        }
        else
        {
            memcpy((uint8_t *)e, (uint8_t *)&Q->base[Q->read], bytesToTheEnd);
            memcpy((((uint8_t *)e)+bytesToTheEnd), (uint8_t *)&Q->base[0], len-bytesToTheEnd);
            Q->read = len-bytesToTheEnd;
        }
    }
    else
    {
        if (0 < Q->size)
        {
            Q->read = (Q->read+len)%Q->size;
        }
        else
        {
            Q->read = 0;
        }
    }

    return CQ_OK;
}

CQ_FUNC_ATTR
int PeekCQueue(CQueue *Q, unsigned int len_want, CQItemType **e1, unsigned int *len1, CQItemType **e2, unsigned int *len2)
{
    if(LengthOfCQueue(Q) < len_want) {
        return CQ_ERR;
    }

    *e1 = &(Q->base[Q->read]);
    if((Q->write > Q->read) || (Q->size - Q->read >= len_want)) {
        *len1 = len_want;
        *e2   = NULL;
        *len2 = 0;
        return CQ_OK;
    }
    else {
        *len1 = Q->size - Q->read;
        *e2   = &(Q->base[0]);
        *len2 = len_want - *len1;
        return CQ_OK;
    }

    return CQ_ERR;
}

int PeekCQueueToBuf(CQueue *Q, CQItemType *e, unsigned int len)
{
    int status = CQ_OK;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    
    status = PeekCQueue(Q, len, &e1, &len1, &e2, &len2);

    if(status == CQ_OK) {
        if (len == (len1 + len2)) {
            memcpy(e, e1, len1);
            memcpy(e + len1, e2, len2);
        } else {
            status = CQ_ERR;
        }
    }

    return status;
}

int PullCQueue(CQueue *Q, CQItemType *e, unsigned int len)
{
    int status = CQ_OK;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    
    status = PeekCQueue(Q, len, &e1, &len1, &e2, &len2);

    if(status == CQ_OK){
        if (len == (len1 + len2)){
            memcpy(e, e1, len1);
            memcpy(e + len1, e2, len2);
            DeCQueue(Q, 0, len);
        }else{
            status = CQ_ERR;
        }
    }

    return status;
}

#ifdef DEBUG_CQUEUE
int DumpCQueue(CQueue *Q)
{
    CQItemType e;
    int len = 0, i = 0;

    len = LengthOfCQueue(Q);

    if(len <= 0)
        return CQ_ERR;

    i = Q->read;
    while(len > 0) {
        e = Q->base[i];

        TRACE(stderr, "-0x%x", e);
        TRACE(stderr, "-%c", e);

        ++i;
        --len;
        if(i == Q->size)
            i = 0;
    }

    return CQ_ERR;
}
#endif

void ResetCQueue(CQueue *Q)
{
    Q->len  = 0;
    Q->read = Q->write = 0;
}


