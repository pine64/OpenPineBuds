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
#ifndef __SAFE_QUEUE_H__
#define __SAFE_QUEUE_H__

#include "plat_types.h"

struct SAFE_QUEUE_T {
    uint8_t first;
    uint8_t last;
    uint8_t count;
    uint8_t size;
    uint8_t watermark;
    uint32_t *items;
};

void safe_queue_init(struct SAFE_QUEUE_T *queue, uint32_t *items, uint32_t size);

int safe_queue_put(struct SAFE_QUEUE_T *queue, uint32_t item);

int safe_queue_get(struct SAFE_QUEUE_T *queue, uint32_t *itemp);

int safe_queue_pop(struct SAFE_QUEUE_T *queue, uint32_t *itemp);

int safe_queue_peek(const struct SAFE_QUEUE_T *queue, int offset, uint32_t *itemp);

uint32_t safe_queue_watermark(const struct SAFE_QUEUE_T *queue);

int safe_queue_dump(const struct SAFE_QUEUE_T *queue);

#endif
