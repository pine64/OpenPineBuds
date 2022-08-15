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
#ifndef __ANC_USB_APP_H__
#define __ANC_USB_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_aud.h"
#include "hal_key.h"

void anc_usb_app(bool on);

bool anc_usb_app_get_status();

void anc_usb_app_init(enum AUD_IO_PATH_T input_path, enum AUD_SAMPRATE_T playback_rate, enum AUD_SAMPRATE_T capture_rate);

void anc_usb_app_term(void);

void anc_usb_app_loop(void);

int anc_usb_app_key(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event);

#ifdef __cplusplus
}
#endif

#endif


