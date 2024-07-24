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


#ifndef __HCI_BUFF_H__
#define __HCI_BUFF_H__

// abbrs
// HCI_BUFF -> HB hb
// HCI_BUFF_TYPE -> HBT
// HCI_BUFF_DATA_HEAD_TYPE -> HBDHT

#include "string.h"
#include "btlib_type.h"
#include "stdint.h"
#include "bt_co_list.h"

#define DUMP_PUSH_POP_TIME 0

#if defined(__cplusplus)
extern "C" {
#endif

// hci buffer type
#define HBT_COMMAND       0x01
#define HBT_COMMAND_SYNC  0x11
#define HBT_RX_ACL        0x02
#define HBT_RX_SCO        0x03
#define HBT_EVENT         0x04
#define HBT_TX_ACL        0x05
#define HBT_TX_SCO        0x06
#define HBT_RX_BLE        0x07
#define HBT_TX_BLE        0x08
#define HBT_RX            0x09

#define HB_TYPE(hci_buff) (hci_buff->type)
#define HB_OPCODE(hci_buff) (hci_buff->u.opcode)
#define HB_HANDLE(hci_buff) (hci_buff->u.handle)
#define HB_DATA(hci_buff) (hci_buff->data)
#define HB_DATA_LEN(hci_buff) (hci_buff->data_len)
#define HB_PRIV(hci_buff) (hci_buff->priv)
#define HB_SHOULD_FREE(hci_buff) (hci_buff->should_free)

typedef struct {
    struct list_node node;
    uint8 type;
    uint8 should_free;
    union {
        uint16 handle;
        uint16 opcode;
    } u;
    uint16 data_len;
    uint8 *data;
    void *priv;
#if defined(DUMP_PUSH_POP_TIME)
    uint32 push_time;
#endif
} hci_buff_t;

#define hci_buff_list_iter_begin(type,hci_buff,error) \
    do{ \
        hci_buff_t *__hci_buff = NULL; \
        struct list_node *__list = hci_buff_get_pend_list(type); \
        struct list_node *__pos  = NULL, *__next = NULL; \
        error = 0; \
        if (__list == NULL) { \
            error = 1; \
            break; \
        } \
        colist_iterate_safe(__pos, __next, __list)  \
        { \
            __hci_buff = colist_structure(__pos, hci_buff_t, node); \
            if (__hci_buff == NULL) { \
                error = 2; \
            } else {\
                hci_buff = __hci_buff; \
            }

#define hci_buff_list_iter_end() \
        } \
    }while(0);

/*
    @brief To init list (add buff to free list, pend list is empty)
    @return SUCCESS/FAILURE
*/
int8 hci_buff_list_init(void);
/*
    @brief To get pend list
    @param type - buffer type
    @return list
*/
struct list_node *hci_buff_get_pend_list(uint8 type);
/*
    @brief To push buff to list tail
    @param type - buffer type
    @param buff - buffer
    @return SUCCESS/FAILURE
*/
int8 hci_buff_list_push(hci_buff_t *buff);
/*
    @brief To push buff to list head
    @param type - buffer type
    @param buff - buffer
    @return SUCCESS/FAILURE
*/
int8 hci_buff_list_push_front(hci_buff_t *buff);
/*
    @brief To see list head but no delete operation
    @param type - buffer type
    @param buff - buffer to store head
    @return SUCCESS/FAILURE
*/
int8 hci_buff_list_peek(uint8 type, hci_buff_t **buff);
/*
    @brief To get list head and delete it from list
    @param type - buffer type
    @param buff - buffer to store head
    @return SUCCESS/FAILURE
*/
int8 hci_buff_list_pop(uint8 type, hci_buff_t **buff);
/*
    @brief To remove a buffer from pending buffer
    @param buff - buffer to remove
    @return SUCCESS/FAILURE
*/
int8 hci_buff_list_remove(hci_buff_t *buff);
/*
    @brief To see if list is empty
    @param type - buffer type
    @return 1 - empty, 0 - empty
*/
int8 hci_buff_list_is_empty(uint8 type);
/*
    @brief To dump list
    @param type - buffer type
*/
void hci_buff_list_dump(uint8 type);
/*
    @brief To alloc a buffer
    @param type - buffer type
    @param buff - buffer to store head
    @return SUCCESS/FAILURE
*/
int8 hci_buff_alloc(uint8 type, hci_buff_t **buff);
/*
    @brief To free a buffer
    @param type - buffer type
    @param buff - buffer to store head
    @return SUCCESS/FAILURE
*/
int8 hci_buff_free(hci_buff_t *buff);
/*
    @brief To print hcibuff statistic
    @return SUCCESS/FAILURE
*/
int8 hci_buff_print_statistic(void);
int8 hci_buff_rx_print_statistic(void);
void hci_buff_list_filter_handle_peek(uint8 type,uint16 handle,bool equal,hci_buff_t **peek_buff);
#if defined(__cplusplus)
}
#endif

#endif /* __HCI_BUFF_H__ */