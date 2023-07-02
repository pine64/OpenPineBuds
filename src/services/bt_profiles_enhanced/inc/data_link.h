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

#ifndef __DATA_LINK_H__
#define __DATA_LINK_H__

#include "bt_co_list.h"

#ifdef __cplusplus
extern "C" {
#endif

enum data_link_type {
    DL_TYPE_VALUE_U8 = 0,
    DL_TYPE_VALUE_U16,
    DL_TYPE_VALUE_U32,
    DL_TYPE_VALUE_S8,
    DL_TYPE_VALUE_S16,
    DL_TYPE_VALUE_S32,
    DL_TYPE_VALUE_FLOAT,
    DL_TYPE_VALUE_DOUBLE,
    DL_TYPE_BUFFER,
};

struct data_link {
    struct list_node node;
    enum data_link_type type;
    uint32 len;
    union {
        unsigned char _u8;
        unsigned short _u16;
        unsigned int _u32;
        unsigned char _s8;
        unsigned short _s16;
        unsigned int _s32;
        //float _float;
        //double _double;
        uint8 *_ptr;
    } data;
};

#define link_to_tail(data,head) \
    colist_addto_tail(data,head)

#define iter_data_link(head,node) \
    colist_iterate_entry(node,struct data_link,&head->node,node)

#define data_link_total_len(head,p) \
    { \
        uint32 _total_len = 0; \
        struct data_link *node = NULL; \
        iter_data_link(head, node) { \
            _total_len += node->len; \
        }  \
        *p = _total_len; \
    }


#ifdef __cplusplus
}
#endif

#endif /* __DATA_LINK_H__ */