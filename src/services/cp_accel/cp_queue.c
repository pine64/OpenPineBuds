
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
 * cqueue.c - c circle queue c file
 */

#include "cp_queue.h"
#include "cmsis.h"
#include "hal_uart.h"
#include <stdio.h>
#include <string.h>

int InitCpQueue(CQueue *Q, unsigned int size, CQItemType *buf) {
  Q->size = size;
  Q->base = buf;
  Q->len = 0;
  if (!Q->base)
    return CQ_ERR;

  Q->read = Q->write = 0;
  return CQ_OK;
}

int IsEmptyCpQueue(CQueue *Q) {
  if (Q->len == 0)
    return CQ_OK;
  else
    return CQ_ERR;
}

int LengthOfCpQueue(CQueue *Q) { return Q->len; }

int AvailableOfCpQueue(CQueue *Q) { return (Q->size - Q->len); }

int EnCpQueue(CQueue *Q, CQItemType *e, unsigned int len) {
  if (AvailableOfCQueue(Q) < len) {
    return CQ_ERR;
  }

  Q->len += len;

  uint32_t bytesToTheEnd = Q->size - Q->write;
  if (bytesToTheEnd > len) {
    memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, len);
    Q->write += len;
  } else {
    memcpy((uint8_t *)&Q->base[Q->write], (uint8_t *)e, bytesToTheEnd);
    memcpy((uint8_t *)&Q->base[0], (((uint8_t *)e) + bytesToTheEnd),
           len - bytesToTheEnd);
    Q->write = len - bytesToTheEnd;
  }

  return CQ_OK;
}

int DeCpQueue(CQueue *Q, CQItemType *e, unsigned int len) {
  if (LengthOfCQueue(Q) < len)
    return CQ_ERR;

  Q->len -= len;

  if (e != NULL) {

    uint32_t bytesToTheEnd = Q->size - Q->read;
    if (bytesToTheEnd > len) {
      memcpy((uint8_t *)e, (uint8_t *)&Q->base[Q->read], len);
      Q->read += len;
    } else {
      memcpy((uint8_t *)e, (uint8_t *)&Q->base[Q->read], bytesToTheEnd);
      memcpy((((uint8_t *)e) + bytesToTheEnd), (uint8_t *)&Q->base[0],
             len - bytesToTheEnd);
      Q->read = len - bytesToTheEnd;
    }
  } else {
    Q->read = (Q->read + len) % Q->size;
  }

  return CQ_OK;
}

int PeekCpQueue(CQueue *Q, unsigned int len_want, CQItemType **e1,
                unsigned int *len1, CQItemType **e2, unsigned int *len2) {
  if (LengthOfCQueue(Q) < len_want) {
    return CQ_ERR;
  }

  *e1 = &(Q->base[Q->read]);
  if ((Q->write > Q->read) || (Q->size - Q->read >= len_want)) {
    *len1 = len_want;
    *e2 = NULL;
    *len2 = 0;
    return CQ_OK;
  } else {
    *len1 = Q->size - Q->read;
    *e2 = &(Q->base[0]);
    *len2 = len_want - *len1;
    return CQ_OK;
  }

  return CQ_ERR;
}

void ResetCpQueue(CQueue *Q) {
  Q->len = 0;
  Q->read = Q->write = 0;
}