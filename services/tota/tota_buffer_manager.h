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

#ifndef __TOTA_BUFFER_MANAGER_H__
#define __TOTA_BUFFER_MANAGER_H__

#include <stdint.h>
#include <stddef.h>
#include "cmsis_os.h"

#define TOTA_STREAM_BUF_SIZE         (24 * 1024 * 2 * 1 * 1)

typedef struct{
    uint8_t     buf[TOTA_STREAM_BUF_SIZE];
    uint32_t    dataSize;
    uint32_t    availableSpace;
    uint32_t    writePos;
    uint32_t    readPos;
    uint32_t    flushBytes;
} stream_buf_t;

void tota_stream_buffer_init(osThreadId tid);
bool tota_stream_buffer_write(uint8_t * buf, uint32_t bufLen);
bool tota_stream_buffer_read(uint8_t * rbuf, uint32_t readSize, uint32_t * flushbytes = NULL);
void tota_stream_buffer_flush(void);
void tota_stream_buffer_clean(void);



#endif

