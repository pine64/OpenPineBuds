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

#ifndef __COQUEUE_H__
#define __COQUEUE_H__

#include <stdbool.h>

typedef void (*coqueue_destroy_func_t)(void *data);

struct coqueue_entry {
    void *data;
    struct coqueue_entry *next;
};

struct coqueue {
    int ref_count;
    struct coqueue_entry *head;
    struct coqueue_entry *tail;
    unsigned int entries;
};

struct coqueue *coqueue_create(void);
void coqueue_destroy(struct coqueue *coqueue, coqueue_destroy_func_t destroy);
bool coqueue_push_tail(struct coqueue *coqueue, void *data);
bool coqueue_push_head(struct coqueue *coqueue, void *data);
bool coqueue_push_after(struct coqueue *coqueue, const void *entry, void *data);
void *coqueue_pop_head(struct coqueue *coqueue);
void *coqueue_peek_head(struct coqueue *coqueue);
void *coqueue_peek_tail(struct coqueue *coqueue);
typedef bool (*coqueue_match_func_t)(const void *data, const void *match_data);
void *coqueue_search(struct coqueue *coqueue, coqueue_match_func_t function,
                            const void *match_data);
bool coqueue_delete(struct coqueue *coqueue, const void *data);
void *coqueue_delete_if(struct coqueue *coqueue, coqueue_match_func_t function,
                            const void *user_data);
unsigned int coqueue_delete_all(struct coqueue *coqueue, coqueue_match_func_t function,
                void *user_data, coqueue_destroy_func_t destroy);

#endif /*__COQUEUE_H__*/