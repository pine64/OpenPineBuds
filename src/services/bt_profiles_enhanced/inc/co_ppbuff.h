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

#ifndef __CO_PPBUFF_H__
#define __CO_PPBUFF_H__

#include "btlib_type.h"
#include "btlib_more.h"
#include "bt_co_list.h"

/* protocol type */
#define PPB_GET_PROTO_TYPE(ppb) (ppb->flag & 0x0F)
#define PPB_SET_PROTO_TYPE(ppb,type) ppb->flag |= type

#define PP_BUFF_FLAG_NORMAL	0x00 /* normal buffer */
#define PP_BUFF_FLAG_L2CAPE	0x01 /* l2cap enhanced retrans buffer */

/* alloc type */
#define PPB_GET_ALLOC_TYPE(ppb) (ppb->flag & 0xF0)
#define PPB_SET_ALLOC_TYPE(ppb,type) ppb->flag |= type

#define PP_BUFF_FLAG_ALLOC_DYNAMIC 0x10 /* dynamic alloced, free inside */
#define PP_BUFF_FLAG_ALLOC_INPLACE 0x20 /* inplace alloced, free outside */

struct pp_buff {
    struct pp_buff *next;
    struct pp_buff *prev;

    uint16 len;
    uint16 flag;
    uint8 *head;
    uint8 *data;
    uint8 *tail;
    uint8 *end;

    void *context;
#if DBG_PPBUFF_NEED_STATISTIC==1
    struct list_node node;
    uint32 ca;
#endif
};

struct pp_buff_head {
    struct pp_buff *next;
    struct pp_buff *prev;
    uint32 qlen;
};

#if defined(__cplusplus)
extern "C" {
#endif

struct pp_buff *ppb_alloc(uint32 size);
struct pp_buff *ppb_alloc_inplace(uint32 size, uint8 *inplace_buff, uint32 inplace_buff_len);
void ppb_free( struct pp_buff *ppb);
uint8 *ppb_put(struct pp_buff *ppb, uint32 len);
void ppb_put_data(struct pp_buff *ppb, uint8 *data, uint32 len);
uint8 *ppb_push(struct pp_buff *ppb, uint32 len);
uint8 *ppb_pull(struct pp_buff *ppb, uint32 len);
void ppb_reserve(struct pp_buff *ppb, int len);
void ppb_trim(struct pp_buff *ppb, unsigned int len);
struct pp_buff *ppb_peek(struct pp_buff_head *list_);
struct pp_buff *ppb_peek_tail(struct pp_buff_head *list_);
void ppb_queue_head_init(struct pp_buff_head *list);
void ppb_queue_after(struct pp_buff_head *list, struct pp_buff *prev,struct pp_buff *newpp);
void ppb_queue_head(struct pp_buff_head *list, struct pp_buff *newpp);
void ppb_queue_tail(struct pp_buff_head *list, struct pp_buff *newpp);
struct pp_buff *ppb_dequeue(struct pp_buff_head *list);
void ppb_insert(struct pp_buff *newpp, struct pp_buff *prev, struct pp_buff *next, struct pp_buff_head *list);
void ppb_append(struct pp_buff *old, struct pp_buff *newpp, struct pp_buff_head *list);
void ppb_unlink(struct pp_buff *ppb, struct pp_buff_head *list);
struct pp_buff *ppb_dequeue_tail(struct pp_buff_head *list);
struct pp_buff *ppb_alloc_fixpacket(uint32 size);
void ppb_print_statistic(void);

#if defined(__cplusplus)
}
#endif

#endif /* __CO_PPBUFF_H__ */