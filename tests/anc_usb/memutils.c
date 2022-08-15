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
#include "plat_types.h"
#include "string.h"
#include "memutils.h"

#define ALIGNED32(a)                    (((uint32_t)(a) & 3) == 0)
#define ALIGNED16(a)                    (((uint32_t)(a) & 1) == 0)

void *copy_mem32(void *dst, const void *src, unsigned int size)
{
    uint32_t *d = dst;
    const uint32_t *s = src;
    const uint32_t *e = s + size / 4;

    while (s < e) {
        *d++ = *s++;
    }

    return dst;
}

void *copy_mem16(void *dst, const void *src, unsigned int size)
{
    uint16_t *d = dst;
    const uint16_t *s = src;
    const uint16_t *e = s + size / 2;

    while (s < e) {
        *d++ = *s++;
    }

    return dst;
}

void *copy_mem(void *dst, const void *src, unsigned int size)
{
    if (ALIGNED32(dst) && ALIGNED32(src) && ALIGNED32(size)) {
        return copy_mem32(dst, src, size);
    } else if (ALIGNED16(dst) && ALIGNED16(src) && ALIGNED16(size)) {
        return copy_mem16(dst, src, size);
    } else {
        return memcpy(dst, src, size);
    }
}

void *zero_mem32(void *dst, unsigned int size)
{
    uint32_t *d = dst;
    uint32_t count = size / 4;

    while (count--) {
        *d++ = 0;
    }

    return dst;
}

void *zero_mem16(void *dst, unsigned int size)
{
    uint16_t *d = dst;
    uint32_t count = size / 2;

    while (count--) {
        *d++ = 0;
    }

    return dst;
}

void *zero_mem(void *dst, unsigned int size)
{
    if (ALIGNED32(dst) && ALIGNED32(size)) {
        return zero_mem32(dst, size);
    } else if (ALIGNED16(dst) && ALIGNED16(size)) {
        return zero_mem16(dst, size);
    } else {
        return memset(dst, 0, size);
    }
}

