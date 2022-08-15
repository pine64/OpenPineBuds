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
#include "string.h"

#ifndef VQE_SIMULATE
#include "hal_trace.h"
#else
#define ASSERT(cond, str, ...)      { if (!(cond)) { fprintf(stderr, str, ##__VA_ARGS__); while(1); } }
#define TRACE(num, str, ...)        do { fprintf(stdout, str, ##__VA_ARGS__); fprintf(stdout, "\n"); } while (0)
#endif

#ifndef HEAP_BUFF_SIZE
#define HEAP_BUFF_SIZE       (1024 * 12)
#endif

static uint8_t heap_buff[HEAP_BUFF_SIZE];
static uint32_t heap_buff_size_used;

static int32_t ext_heap_init(void)
{
    TRACE(2, "[%s] Heap size = %d", __func__, HEAP_BUFF_SIZE);

    heap_buff_size_used = 0;
    memset((uint8_t *)heap_buff, 0, HEAP_BUFF_SIZE);
    return 0;
}

static POSSIBLY_UNUSED int32_t ext_heap_deinit(void)
{
    TRACE(4,"[%s] heap = %d, used = %d, free = %d", __func__, HEAP_BUFF_SIZE, heap_buff_size_used, HEAP_BUFF_SIZE - heap_buff_size_used);

    return 0;
}

static POSSIBLY_UNUSED uint32_t ext_heap_get_used_buff_size()
{
    return heap_buff_size_used;
}

static uint32_t ext_heap_get_free_buff_size()
{
    return HEAP_BUFF_SIZE - heap_buff_size_used;
}

static void *ext_get_buff(uint32_t size)
{
    uint32_t buff_size_free;
    uint8_t *buf_ptr = &heap_buff[heap_buff_size_used];

    buff_size_free = ext_heap_get_free_buff_size();

    if (size % 4){
        size = size + (4 - size % 4);
    }

    TRACE(3,"[%s] Free: %d; Alloc: %d", __func__, buff_size_free, size);

    ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__, size, buff_size_free);

    heap_buff_size_used += size;
    // TRACE("Allocate %d, now used %d left %d", size, heap_buff_size_used, ext_heap_get_free_buff_size());

    return (void *)buf_ptr;
}

static void *ext_alloc(int size)
{
    void *mem_ptr = ext_get_buff(size);

    memset(mem_ptr, 0, size);

    return mem_ptr;
}

static POSSIBLY_UNUSED void *ext_malloc(int size)
{
    return ext_alloc(size);
}

static POSSIBLY_UNUSED void *ext_calloc(int nitems, int size)
{
    return ext_alloc(nitems * size);
}

static POSSIBLY_UNUSED void ext_free(void *mem_ptr)
{
    ;
}
