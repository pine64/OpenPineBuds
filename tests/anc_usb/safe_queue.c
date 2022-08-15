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
#include "cmsis.h"
#include "safe_queue.h"
#include "hal_trace.h"

void safe_queue_init(struct SAFE_QUEUE_T *queue, uint32_t *items, uint32_t size)
{
    // queue->first points to the first valid item
    queue->first = 0;
    // queue->last points to the first empty slot (beyond the last valid item)
    queue->last = 0;
    queue->count = 0;
    queue->size = size;
    queue->watermark = 0;
    queue->items = items;
}

int safe_queue_put(struct SAFE_QUEUE_T *queue, uint32_t item)
{
    int ret = 0;
    uint32_t lock;

    lock = int_lock();

    if (queue->count >= queue->size) {
        ret = 1;
    } else {
        queue->items[queue->last] = item;
        queue->last++;
        if (queue->last >= queue->size) {
            queue->last = 0;
        }

        queue->count++;
        if (queue->count > queue->watermark) {
            queue->watermark = queue->count;
        }
    }

    int_unlock(lock);

    return ret;
}

int safe_queue_get(struct SAFE_QUEUE_T *queue, uint32_t *itemp)
{
    int ret = 0;
    uint32_t lock;

    lock = int_lock();

    if (queue->count == 0) {
        ret = 1;
    } else {
        if (itemp) {
            *itemp = queue->items[queue->first];
        }
        queue->first++;
        if (queue->first >= queue->size) {
            queue->first = 0;
        }

        queue->count--;
    }

    int_unlock(lock);

    return ret;
}

int safe_queue_pop(struct SAFE_QUEUE_T *queue, uint32_t *itemp)
{
    int ret = 0;
    uint32_t lock;

    lock = int_lock();

    if (queue->count == 0) {
        ret = 1;
    } else {
        if (queue->last) {
            queue->last--;
        } else {
            queue->last = queue->size - 1;
        }
        if (itemp) {
            *itemp = queue->items[queue->last];
        }

        queue->count--;
    }

    int_unlock(lock);

    return ret;
}

int safe_queue_peek(const struct SAFE_QUEUE_T *queue, int offset, uint32_t *itemp)
{
    int ret = 0;
    uint32_t lock;
    uint8_t idx;

    lock = int_lock();

    if (offset >= 0 && offset < queue->count) {
        idx = queue->first + offset;
    } else if (offset < 0 && -offset <= queue->count) {
        idx = queue->size + queue->last + offset;
    } else {
        ret = 1;
    }

    if (ret == 0) {
        if (idx >= queue->size) {
            idx -= queue->size;
        }

        *itemp = queue->items[idx];
    }

    int_unlock(lock);

    return ret;
}

uint32_t safe_queue_watermark(const struct SAFE_QUEUE_T *queue)
{
    return queue->watermark;
}

int safe_queue_dump(const struct SAFE_QUEUE_T *queue)
{
    uint32_t cnt;
    int i;
    uint32_t item;

    cnt = queue->count;
    for (i = 0; i < cnt; i++) {
        if (safe_queue_peek(queue, i, &item)) {
            break;
        }
        TRACE_IMM(3,"SAFE-QUEUE-DUMP: [%02d/%02d] 0x%08x", i, cnt, item);
    }

    return 0;
}
