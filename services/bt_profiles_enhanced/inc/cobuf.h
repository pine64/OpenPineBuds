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

#ifndef _COBUF_H
#define _COBUF_H
#include "bt_sys_cfg.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*must be 4 align*/
struct cobuf_statistic_packet 
{
    unsigned short stat_flag; /*must be 0x5a5a if used, or 0xa5a5 if released*/
    unsigned short alloc_place_id;
    unsigned short block_index;
    unsigned short realsize;
};

struct cobuf_base
{
    struct cobuf_base *link;
#if DBG_COBUF_NEED_STATISTIC == 1
    struct cobuf_statistic_packet statistic;
#endif
    unsigned char buf[2];
};

void init_buf( struct cobuf_base **head,
                 struct cobuf_base *first,
                 int elm_size,
                 int buf_num);
unsigned char*  alloc_buf(struct cobuf_base **head_ptr);
unsigned char*  alloc_buf_infiq(struct cobuf_base **head_ptr);
void  free_buf(unsigned char *buf);

void cobuf_init(void);

#if DBG_COBUF_NEED_STATISTIC == 1
#define DBG_COBUF_STATISTIC_PLACE_RECORD_ENABLE  1
#define DBG_COBUF_STATISTIC_PLACE_RECORD_OPTIMIZE_ENABLE  1

unsigned char *_cobuf_malloc_with_statistic(int size, char *module_str, int line);
unsigned char *_cobuf_malloc_with_statistic_infiq(int size, char *module_str, int line);
void  _cobuf_free_with_statistic(unsigned char *buf);

#if BA_GCC==1
#define __CO_MODULE  __func__
#endif

#define cobuf_malloc(size)  _cobuf_malloc_with_statistic(size,(char *)__CO_MODULE,__LINE__)
#define cobuf_malloc_infiq(size)  _cobuf_malloc_with_statistic_infiq(size,(char *)__CO_MODULE,__LINE__)
#define cobuf_free(buf) _cobuf_free_with_statistic(buf);

#else

unsigned char *_cobuf_malloc(int size);
unsigned char *_cobuf_malloc_infiq(int size);
void  _cobuf_free(unsigned char *buf);
#define cobuf_malloc(size) _cobuf_malloc(size)
#define cobuf_malloc_infiq(size) _cobuf_malloc_infiq(size)
#define cobuf_free(buf) _cobuf_free(buf)

#endif

unsigned char *cobuf_zmalloc(int size);
void cobuf_print_statistic(void);
void cobuf_print_block_statistic(void);
void cobuf_print_place_statistic(void);

#if defined(__cplusplus)
}
#endif

#endif
