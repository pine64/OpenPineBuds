/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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

#include "bt_co_list.h"
#include "btlib_type.h"
#include "btm_i.h"
#include "sdp.h"

#ifndef __HID_I_H__
#define __HID_I_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HID_STATE_CLOSED,
    HID_STATE_STANDBY = 1, // ready
    HID_STATE_QUERING,
    HID_STATE_CONNECTING,
    HID_STATE_WAITING_INTRUPT_CHANNEL,
    HID_STATE_CONNECTING_INTRUPT_CHANNEL,
    HID_STATE_OPEN,
    HID_STATE_DISCONNECTING,
} hid_state_t;

typedef enum {
    HID_EVENT_REMOTE_NOT_SUPPORT = 1,
    HID_EVENT_CONN_OPENED,
    HID_EVENT_CONN_CLOSED,
} hid_event_t;

typedef enum {
    HID_SESSION_CLOSE,
    HID_SESSION_CONNECTING,
    HID_SESSION_OPEN,
} hid_session_state_t;

typedef enum {
    HID_PENDING_OP_NULL = 0,
    HID_PENDING_OP_GET_REPORT,
    HID_PENDING_OP_GET_PROTOCOL,
    HID_PENDING_OP_SET_REPORT,
    HID_PENDING_OP_SET_PROTOCOL,
} hid_pending_op_t;

struct hid_session_t {
    uint32 l2cap_handle;
    hid_session_state_t state;
};

struct hid_callback_parms_t
{
    hid_event_t event;
    uint8_t error_code;
    struct bdaddr_t remote;
};

#define HID_BOOT_PROTOCOL_MODE      0x0
#define HID_REPORT_PROTOCOL_MODE    0x1

typedef enum {
    HID_REPORT_TYPE_OTHER = 0,
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE,
} hid_report_type_enum_t;

#define HID_FRAME_DATA_MAX_LEN 11 /* DM1 is recommended packet */
extern struct hid_stack_interface_t hid_stack_if;

struct hid_report_data_t {
    uint8_t report_id;
    uint8_t data[HID_FRAME_DATA_MAX_LEN];
    uint8_t data_len;
};

struct hid_frame_t {
    uint8_t header;
    uint8_t data[HID_FRAME_DATA_MAX_LEN];
    uint8_t data_len;
    bool waiting_rsp;
    hid_pending_op_t pending_op;
};

struct hid_frame_node_t {
    struct list_node node;
    struct hid_frame_t frame;
};

struct hid_rx_msg_t {
    uint8_t msg_type;
    uint8_t param;
    uint8_t data_len;
    uint8_t *data;
};

struct hid_control_t {
    struct bdaddr_t remote;
    bool initiator;
    bool is_hid_device_role;
    struct hid_session_t control_session;
    struct hid_session_t interrupt_session;
    hid_state_t state;
    hid_pending_op_t pending_op;
    uint8 pending_protocol_set;
    uint8 unplug_op_pending;
    uint8 local_protocol_mode;
    uint8 remote_protocol_mode;
    uint8 remote_report_descriptor_has_report_id;
    uint8 disc_reason;
    void (*hid_callback)(struct hid_control_t *hid_ctl, const struct hid_callback_parms_t *info);
    struct sdp_request sdp_request;
    struct hid_report_data_t device_input_data;
    struct hid_report_data_t device_output_data;
    struct hid_report_data_t device_feature_data;
    struct list_node ctrl_tx_list;
    struct list_node intr_tx_list;
    bool ctrl_tx_busy;
    bool intr_tx_busy;
    bool use_interrupt_channel;
};

struct hid_stack_interface_t {
    struct hid_control_t *(*hid_alloc_standby_control)(void *remote);
    struct hid_control_t *(*hid_search_from_control_handle)(uint32_t l2cap_handle);
    struct hid_control_t *(*hid_search_from_interrupt_handle)(uint32_t l2cap_handle);
    struct hid_control_t *(*hid_search_from_remote_bdaddr)(void *remote);
};

struct hid_ctx_input {
    struct ctx_content ctx;
    struct bdaddr_t *remote;
    struct hid_control_t *hid_ctl;
    uint32 hid_ctrl_handle;
    uint32 hid_intr_handle;
};

int hid_stack_init(struct hid_stack_interface_t *stack_if, uint8_t is_hid_device_role);

int hid_connect_req(struct hid_control_t *hid_ctl);

void hid_disconnect_req(struct hid_control_t *hid_ctl);

hid_state_t hid_get_state(struct hid_control_t *hid_ctl);

int hid_send_handshake(struct hid_control_t *hid_ctl, uint8_t code);

void hid_keyboard_input_report(struct hid_control_t *hid_ctl, uint8_t modifier_key, uint8_t key_code);

void hid_keyboard_send_ctrl_key(struct hid_control_t *hid_ctl, uint8_t ctrl_key);

int hid_l2cap_control_notify_callback(enum l2cap_event_enum event, uint32 l2cap_handle, void *pdata, uint8 reason);
int hid_l2cap_interrupt_notify_callback(enum l2cap_event_enum event, uint32 l2cap_handle, void *pdata, uint8 reason);
void hid_l2cap_control_data_receive(uint32 l2cap_handle, struct pp_buff *ppb);
void hid_l2cap_interrupt_data_receive(uint32 l2cap_handle, struct pp_buff *ppb);

#ifdef __cplusplus
}
#endif

#endif /* __HID_I_H__ */

