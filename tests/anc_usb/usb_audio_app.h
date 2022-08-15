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
#ifndef __USB_AUDIO_APP_H__
#define __USB_AUDIO_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_key.h"
#include "audio_process.h"

struct USB_AUDIO_BUF_CFG {
    uint8_t *play_buf;
    uint32_t play_size;
    uint8_t *cap_buf;
    uint32_t cap_size;
    uint8_t *recv_buf;
    uint32_t recv_size;
    uint8_t *send_buf;
    uint32_t send_size;
    uint8_t *eq_buf;
    uint32_t eq_size;
    uint8_t *resample_buf;
    uint32_t resample_size;
#ifdef USB_AUDIO_MULTIFUNC
    uint8_t *recv2_buf;
    uint32_t recv2_size;
#endif
};

typedef void (*USB_AUDIO_ENQUEUE_CMD_CALLBACK)(uint32_t data);

void usb_audio_app(bool on);
void usb_audio_keep_streams_running(bool enable);
void usb_audio_app_init(const struct USB_AUDIO_BUF_CFG *cfg);

void usb_audio_app_term(void);
int usb_audio_app_key(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event);
void usb_audio_app_loop(void);
uint32_t usb_audio_get_capture_sample_rate(void);

uint32_t usb_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t index);
uint8_t usb_audio_get_eq_index(AUDIO_EQ_TYPE_T audio_eq_type,uint8_t anc_status);

void usb_audio_set_enqueue_cmd_callback(USB_AUDIO_ENQUEUE_CMD_CALLBACK cb);

#ifdef __cplusplus
}
#endif

#endif
