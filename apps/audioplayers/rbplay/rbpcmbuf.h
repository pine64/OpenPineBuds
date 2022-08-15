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
#ifndef RBPCMBUF_HEADER
#define RBPCMBUF_HEADER

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _RB_PCMBUF_AUD_STATE_T {
    RB_PCMBUF_AUD_STATE_STOP = 0,
    RB_PCMBUF_AUD_STATE_START,
} RB_PCMBUF_AUD_STATE_T;

void rb_pcmbuf_init(void);
void rb_pcmbuf_stop(void);
void *rb_pcmbuf_request_buffer(int *size);
void rb_pcmbuf_write(unsigned int size);

#ifdef __cplusplus
}
#endif

#endif /* RBPCMBUF_HEADER */
