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
#ifndef __APP_VOICEPATH_BLE_H__
#define __APP_VOICEPATH_BLE_H__

#include "ke_task.h"

void app_ble_voicepath_init(void);

const struct ke_state_handler *app_voicepath_ble_get_msg_handler_table(void);

void app_ble_voicepath_add_svc(void);

void app_ble_bms_add_svc(void);

#if (ANCS_PROXY_ENABLE)
void app_ble_ancsp_add_svc(void);

void app_ble_amsp_add_svc(void);
#endif

void app_voicepath_disconnected_evt_handler(uint8_t conidx);

void app_voicepath_mtu_exchanged_handler(uint8_t conidx, uint16_t MTU);

void app_voicepath_ble_conn_parameter_updated(uint8_t conidx,
                                              uint16_t connInterval, 
                                              uint16_t connSlavelatency);

#endif
