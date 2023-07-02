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
#ifndef __APP_THREAD_H__
#define __APP_THREAD_H__
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_MAILBOX_MAX (20)

enum APP_MODUAL_ID_T {
  APP_MODUAL_KEY = 0,
  APP_MODUAL_AUDIO,
  APP_MODUAL_BATTERY,
  APP_MODUAL_BT,
  APP_MODUAL_FM,
  APP_MODUAL_SD,
  APP_MODUAL_LINEIN,
  APP_MODUAL_USBHOST,
  APP_MODUAL_USBDEVICE,
  APP_MODUAL_WATCHDOG,
  APP_MODUAL_AUDIO_MANAGE,
  APP_MODUAL_ANC,
  APP_MODUAL_SMART_MIC,
#ifdef __PC_CMD_UART__
  APP_MODUAL_CMD,
#endif
#ifdef TILE_DATAPATH
  APP_MODUAL_TILE,
#endif
  APP_MODUAL_MIC,
#ifdef VOICE_DETECTOR_EN
  APP_MODUAL_VOICE_DETECTOR,
#endif
  APP_MODUAL_CUSTOM_FUNCTION,
  APP_MODUAL_OHTER,
  APP_MODUAL_WNR,

  APP_MODUAL_NUM
};

typedef struct {
  uint32_t message_id;
  uint32_t message_ptr;
  uint32_t message_Param0;
  uint32_t message_Param1;
  uint32_t message_Param2;
} APP_MESSAGE_BODY;

typedef struct {
  uint32_t src_thread;
  uint32_t dest_thread;
  uint32_t system_time;
  uint32_t mod_id;
  APP_MESSAGE_BODY msg_body;
} APP_MESSAGE_BLOCK;

typedef int (*APP_MOD_HANDLER_T)(APP_MESSAGE_BODY *);

int app_mailbox_put(APP_MESSAGE_BLOCK *msg_src);

int app_mailbox_free(APP_MESSAGE_BLOCK *msg_p);

int app_mailbox_get(APP_MESSAGE_BLOCK **msg_p);

int app_os_init(void);

int app_set_threadhandle(enum APP_MODUAL_ID_T mod_id,
                         APP_MOD_HANDLER_T handler);

void *app_os_tid_get(void);

bool app_is_module_registered(enum APP_MODUAL_ID_T mod_id);

#ifdef __cplusplus
}
#endif

#endif //__FMDEC_H__
