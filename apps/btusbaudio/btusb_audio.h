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
#ifndef _BTUSB_AUDIO_H_
#define _BTUSB_AUDIO_H_
#ifdef __cplusplus
extern "C" {
#endif

#define BTUSB_OUTTIME_MS 1000
enum BTUSB_MODE{
    BTUSB_MODE_BT,
    BTUSB_MODE_USB,
    BTUSB_MODE_INVALID
};

void btusb_usbaudio_open(void);
void btusb_usbaudio_close(void);
void btusb_btaudio_open(bool is_wait);
void btusb_btaudio_close(bool is_wait);
void btusb_switch(enum BTUSB_MODE mode);
bool btusb_is_bt_mode(void);
bool btusb_is_usb_mode(void);
#ifdef __cplusplus
}
#endif
#endif // _BTUSB_AUDIO_H_

