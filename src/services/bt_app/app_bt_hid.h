
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
#ifndef __APP_BT_HID_H__
#define __APP_BT_HID_H__

#ifdef BTIF_HID_DEVICE

#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HID_CTRL_KEY_NULL       0x00
#define HID_CTRL_KEY_POWER      0x01
#define HID_CTRL_KEY_PLAY       0x02
#define HID_CTRL_KEY_PAUSE      0x04
#define HID_CTRL_KEY_STOP       0x08
#define HID_CTRL_KEY_EJECT      0x10
#define HID_CTRL_KEY_PLAY_PAUSE 0x20
#define HID_CTRL_KEY_VOLUME_INC 0x40
#define HID_CTRL_KEY_VOLUME_DEC 0x80

#define HID_MOD_KEY_NULL    0x00
#define HID_MOD_KEY_L_CTRL  0x01
#define HID_MOD_KEY_L_SHIFT 0x02
#define HID_MOD_KEY_L_ALT   0x04
#define HID_MOD_KEY_L_WIN   0x08
#define HID_MOD_KEY_R_CTRL  0x10
#define HID_MOD_KEY_R_SHIFT 0x20
#define HID_MOD_KEY_R_ALT   0x40
#define HID_MOD_KEY_R_WIN   0x80

#define HID_KEY_CODE_NULL   0x00
#define HID_KEY_CODE_A      0x04
#define HID_KEY_CODE_Z      0x1d
#define HID_KEY_CODE_1      0x1e
#define HID_KEY_CODE_9      0x26
#define HID_KEY_CODE_0      0x27
#define HID_KEY_CODE_ENTER  0x28
#define HID_KEY_CODE_ESC    0x29
#define HID_KEY_CODE_DEL    0x2a
#define HID_KEY_CODE_TAB    0x2b
#define HID_KEY_CODE_SPACE  0x2c
#define HID_KEY_CODE_VOLUP  0x80
#define HID_KEY_CODE_VOLDN  0x81

typedef struct hid_control_t* hid_channel_t;

void app_bt_hid_init(void);

void app_bt_hid_enter_shutter_mode(void);

void app_bt_hid_exit_shutter_mode(void);

void app_bt_hid_send_capture(hid_channel_t chnl);

#ifdef __cplusplus
}
#endif

#endif /* BTIF_HID_DEVICE */

#endif /* __APP_BT_HID_H__ */

