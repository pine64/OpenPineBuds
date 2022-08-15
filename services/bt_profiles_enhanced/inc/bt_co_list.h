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

#ifndef __BT_CO_LIST_H__
#define __BT_CO_LIST_H__

#include "btlib_more.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ITER_WARN "list too much loop=%d!!!!"

#if !defined(ITER_ASSERT)

#if defined(ASSERT_SHOW_FILE_FUNC)
#define ITER_ASSERT(head)  hal_trace_assert_dump(__FILE__, __FUNCTION__, __LINE__, ITER_WARN, (head)->__iter__cnt)
#elif defined(ASSERT_SHOW_FILE)
#define ITER_ASSERT(head)  hal_trace_assert_dump(__FILE__, __FUNCTION__, __LINE__, ITER_WARN, (head)->__iter__cnt)
#elif defined(ASSERT_SHOW_FUNC)
#define ITER_ASSERT(head)  hal_trace_assert_dump(__FUNCTION__, __LINE__, ITER_WARN, (head)->__iter__cnt)
#else
#define ITER_ASSERT(head)  hal_trace_assert_dump(ITER_WARN, (head)->__iter__cnt)
#endif

#endif /* ITER_ASSERT */

#if 1
#define ITER_INIT(head) ,(head)->__iter__cnt = 0
#define ITER_CHK(head) ,((head)->__iter__cnt>100?ITER_ASSERT((head)):(void)((head)->__iter__cnt++))
#else
#define ITER_INIT(head)
#define ITER_CHK(head)
#endif

#ifndef OFFSETOF
#define OFFSETOF(type, member) ((unsigned int) &((type *)0)->member)
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) ((type *)( (char *)ptr - OFFSETOF(type,member) ))
#endif

struct list_node {
    struct list_node *next;
    struct list_node *prev;
    unsigned int __iter__cnt;
};

#define DEF_LIST_HEAD(head) \
    struct list_node head = { &(head), &(head) }

#define INIT_LIST_HEAD(head) do { \
    (head)->next = (head); (head)->prev = (head); \
} while (0)

void colist_addto_head(struct list_node *n, struct list_node *head);
void colist_addto_tail(struct list_node *n, struct list_node *head);
void colist_delete(struct list_node *entry);
void colist_moveto_head(struct list_node *list, struct list_node *head);
int colist_is_node_on_list(struct list_node *list, struct list_node *node);
struct list_node *colist_get_head(struct list_node *head);
int colist_is_list_empty(struct list_node *head);

#define colist_structure(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))
#define colist_iterate(pos, head) \
    for (pos = (head)->next ITER_INIT((head)); pos != (head); \
            pos = pos->next ITER_CHK((head)))
#define colist_iterate_prev(pos, head) \
    for (pos = (head)->prev ITER_INIT((head)); pos != (head); \
            pos = pos->prev ITER_CHK((head))
#define colist_iterate_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next ITER_INIT((head)); pos != (head); \
        pos = n, n = pos->next ITER_CHK((head)))
#define colist_iterate_entry(pos, type, head, member)               \
    for (pos = colist_structure((head)->next, type, member) ITER_INIT((head));            \
         &pos->member != (head);                    \
         pos = colist_structure(pos->member.next, type, member) ITER_CHK((head)))
#define colist_iterate_entry_safe(pos, n, type, head, member)           \
    for (pos = colist_structure((head)->next, type, member),    \
        n = colist_structure(pos->member.next, type, member) ITER_INIT((head));   \
         &pos->member != (head);                    \
         pos = n, n = colist_structure(n->member.next, type, member) ITER_CHK((head)))

#ifdef __cplusplus
}
#endif

#endif /* __BT_CO_LIST_H__ */