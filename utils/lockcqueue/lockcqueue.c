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
 * lock cqueue
 * YuLongWang @2015
*/

#include "stdio.h"
#include "cmsis_os.h"
#include "lockcqueue.h"

int lockcqueue_init(struct lockcqueue *q, uint32_t size, uint8_t *buf)
{
    InitCQueue(&q->cqueue, size, buf);
    q->os_mutex_def_name.mutex = q->os_mutex_cb_name;
    q->queue_mutex_id = osMutexCreate(&q->os_mutex_def_name);
}

int lockcqueue_enqueue(struct lockcqueue *q, uint8_t *buf, uint32_t size)
{
    int ret = 0;
	osMutexWait(q->queue_mutex_id, osWaitForever);
    ret = EnCQueue(&q->cqueue, buf, size);
	osMutexRelease(q->queue_mutex_id);

    return ret;
}

int lockcqueue_dequeue(struct lockcqueue *q, uint8_t *buf, uint32_t size)
{
    int ret = 0;
	osMutexWait(q->queue_mutex_id, osWaitForever);
    ret = DeCQueue(&q->cqueue, buf, size);
	osMutexRelease(q->queue_mutex_id);

    return ret;
}
