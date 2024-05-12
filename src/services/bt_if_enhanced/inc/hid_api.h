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

#ifndef __BTIF_HID_API_H__
#define __BTIF_HID_API_H__

#ifdef __cplusplus
extern "C" {
#endif

struct hid_control_t;
struct hid_callback_parms_t;

typedef enum {
    HID_DEVICE_ROLE,
    HID_HOST_ROLE,
} hid_role_enum_t;

typedef enum {
    BTIF_HID_EVENT_REMOTE_NOT_SUPPORT = 1,
    BTIF_HID_EVENT_CONN_OPENED,
    BTIF_HID_EVENT_CONN_CLOSED,
} btif_hid_event_t;

typedef struct
{
    btif_hid_event_t event;
    uint8_t error_code;
    bt_bdaddr_t remote;
} btif_hid_callback_param_t;

void btif_hid_init(void (*cb)(struct hid_control_t *hid_ctl, const struct hid_callback_parms_t *info), hid_role_enum_t role);

struct hid_control_t *btif_hid_channel_alloc(void);

bt_status_t btif_hid_connect(bt_bdaddr_t *addr);

void btif_hid_disconnect(struct hid_control_t *hid_ctl);

bool btif_hid_is_connected(struct hid_control_t *hid_ctl);

int btif_hid_get_state(struct hid_control_t *hid_ctl);

void btif_hid_keyboard_input_report(struct hid_control_t *hid_ctl, uint8_t modifier_key, uint8_t key_code);

void btif_hid_keyboard_send_ctrl_key(struct hid_control_t *hid_ctl, uint8_t ctrl_key);

#if defined(IBRT)
uint32_t hid_save_ctx(struct hid_control_t *hid_ctl, uint8_t *buf, uint32_t buf_len);
uint32_t hid_restore_ctx(struct hid_ctx_input *input);
uint32_t btif_hid_profile_save_ctx(btif_remote_device_t *rem_dev, uint8_t *buf, uint32_t buf_len);
uint32_t btif_hid_profile_restore_ctx(uint8_t *buf, uint32_t buf_len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BTIF_HID_API_H__ */

