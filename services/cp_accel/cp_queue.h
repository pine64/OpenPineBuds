
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

#ifndef CP_QUEUE_H
#define CP_QUEUE_H 1

#if defined(__cplusplus)
extern "C" {
#endif

#include "cqueue.h"

/* Init Queue */
int InitCpQueue(CQueue *Q, unsigned int size, CQItemType *buf);
/* Is Queue Empty */
int IsEmptyCpQueue(CQueue *Q);
/* Filled Length Of Queue */
int LengthOfCpQueue(CQueue *Q);
/* Empty Length Of Queue */
int AvailableOfCpQueue(CQueue *Q);
/* Push Data Into Queue (Tail) */
int EnCpQueue(CQueue *Q, CQItemType *e, unsigned int len);
/* Pop Data Data From Queue (Front) */
int DeCpQueue(CQueue *Q, CQItemType *e, unsigned int len);
/* Peek But Not Pop Data From Queue (Front) */
int PeekCpQueue(CQueue *Q, unsigned int len_want, CQItemType **e1, unsigned int *len1, CQItemType **e2, unsigned int *len2);

void ResetCpQueue(CQueue *Q);

#if defined(__cplusplus)
}
#endif

#endif /* CP_QUEUE_H */
