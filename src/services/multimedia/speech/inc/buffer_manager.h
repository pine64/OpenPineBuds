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
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <stdint.h>

struct BufferManager_;

typedef struct BufferManager_ BufferManager;

typedef enum
{
    BUFFER_MANAGER_CACHING = 0,
    BUFFER_MANAGER_CACHE_OK = 1,
    BUFFER_MANAGER_FLUSH = 2
} BufferManagerStatus;

#ifdef __cplusplus
extern "C" {
#endif

// Buffer management for decoupling codec frame size and vqe frame size,
// vqe frame size must be larger than codec frame size.
// When init buffer manager, input buffer should inited by
// buffer_manager_create(codec_frame_size, vqe_frame_size)
// output buffer should inited by
// buffer_manager_create(vqe_frame_size, codec_frame_size)
BufferManager *buffer_manager_create(int inlen, int outlen, int sample_size);

void buffer_manager_destroy(BufferManager *st);

BufferManagerStatus buffer_manager_process(BufferManager *st, void *inbuf, int inlen, void *outbuf, int outlen);

#ifdef __cplusplus
}
#endif

#endif