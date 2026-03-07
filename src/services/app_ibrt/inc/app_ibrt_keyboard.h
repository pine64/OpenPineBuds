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
#ifndef __APP_IBRT_KEYBOARD__
#define __APP_IBRT_KEYBOARD__
#include "app_key.h"

#ifdef IBRT_SEARCH_UI
void app_ibrt_search_ui_handle_key(APP_KEY_STATUS *status, void *param);
#else
void app_ibrt_normal_ui_handle_key(APP_KEY_STATUS *status, void *param);
#endif

void app_ibrt_send_keyboard_request(uint8_t *p_buff, uint16_t length);

void app_ibrt_keyboard_request_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

#define IBRT_ACTION_PLAY                                0x01
#define IBRT_ACTION_PAUSE                               0x02
#define IBRT_ACTION_FORWARD                             0x03
#define IBRT_ACTION_BACKWARD                            0x04
#define IBRT_ACTION_AVRCP_VOLUP                         0x05
#define IBRT_ACTION_AVRCP_VOLDN                         0x06
#define IBRT_ACTION_HFSCO_CREATE                        0x07
#define IBRT_ACTION_HFSCO_DISC                          0x08
#define IBRT_ACTION_REDIAL                              0x09
#define IBRT_ACTION_ANSWER                              0x0a
#define IBRT_ACTION_HANGUP                              0x0b
#define IBRT_ACTION_LOCAL_VOLUP                         0x0c
#define IBRT_ACTION_LOCAL_VOLDN                         0x0d
#define IBRT_ACTION_ANC_NOTIRY_MASTER_EXCHANGE_COEF     0x0e
#define IBRT_ACTION_SYNC_WNR                            0x0f


void app_ibrt_if_start_user_action(uint8_t *p_buff, uint16_t length);
void app_ibrt_ui_perform_user_action(uint8_t *p_buff, uint16_t length);

extern bool Is_Slave_Key_Down_Event;

#define TWS_Sync_Shutdowm		1
#define QXW_TOUCH_INEAR_DET	0
#define BT_FACTORY_CLEAR_RECORD	0

#define Auto_Shutdowm_TIME		300

#if(TWS_Sync_Shutdowm)
int app_ibrt_poweroff_notify_force(void);
int app_ibrt_reboot_notify_force(void);
#endif
#endif
