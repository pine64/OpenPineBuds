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
* cqueue.h - c circle queue c header
*/

#ifndef C_QUEUE_H
#define C_QUEUE_H 1

#if defined(__cplusplus)
extern "C" {
#endif

enum {
    CQ_OK = 0,
    CQ_ERR,
};

typedef unsigned char CQItemType;

typedef struct __CQueue
{
    int read;
    int write;
    int size;
    int len;
    CQItemType *base;
}CQueue;

/* Init Queue */
int InitCQueue(CQueue *Q, unsigned int size, CQItemType *buf);
/* Is Queue Empty */
int IsEmptyCQueue(CQueue *Q);
/* Filled Length Of Queue */
int LengthOfCQueue(CQueue *Q);
/* Empty Length Of Queue */
int AvailableOfCQueue(CQueue *Q);
/* Push Data Into Queue (Tail) */
int EnCQueue(CQueue *Q, CQItemType *e, unsigned int len);
/* Push Data Into Queue (Tail) */
int EnCQueue_AI(CQueue *Q, CQItemType *e, unsigned int len);
/* Push Data Into Front Of Queue */
int EnCQueueFront(CQueue *Q, CQItemType *e, unsigned int len);
/* Pop Data Data From Queue (Front) */
int DeCQueue(CQueue *Q, CQItemType *e, unsigned int len);
/* Peek But Not Pop Data From Queue (Front) */
int PeekCQueue(CQueue *Q, unsigned int len_want, CQItemType **e1, unsigned int *len1, CQItemType **e2, unsigned int *len2);
/* Peek data to buf e, But Not Pop Data From Queue (Front) */
int PeekCQueueToBuf(CQueue *Q, CQItemType *e, unsigned int len);
int PullCQueue(CQueue *Q, CQItemType *e, unsigned int len);
/* Dump Queue */
int DumpCQueue(CQueue *Q);

void ResetCQueue(CQueue *Q);

#if defined(__cplusplus)
}
#endif

#endif /* C_QUEUE_H */
