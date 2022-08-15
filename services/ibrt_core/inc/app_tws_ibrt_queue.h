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
#ifndef __APP_BT_QUEUE_H__
#define __APP_BT_QUEUE_H__

typedef struct __ibrt_queue
{
    CQueue queue;
    void *mutex;
}ibrt_queue;

void app_tws_ibrt_queue_init(ibrt_queue* ptrQueue, uint8_t* ptrBuf, uint32_t bufLen);
int app_tws_ibrt_queue_push(ibrt_queue* ptrQueue, uint8_t* ptrData, uint32_t length);
uint16_t app_tws_ibrt_queue_get_next_entry_length(ibrt_queue* ptrQueue);
void app_tws_ibrt_queue_pop(ibrt_queue* ptrQueue, uint8_t *ptrBuf, uint32_t len);
int app_tws_ibrt_dequeue(ibrt_queue *ptrQueue, void *e, unsigned int len);
int app_tws_ibrt_length_queue(ibrt_queue *ptrQueue);

#endif	// __APP_BT_QUEUE_H__

