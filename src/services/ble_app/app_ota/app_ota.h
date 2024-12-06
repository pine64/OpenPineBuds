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


#ifndef APP_OTA_H_
#define APP_OTA_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief OTA Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_OTA)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"
#include "app_ble_mode_switch.h"
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

#define OTA_HIGH_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS		25
#define OTA_HIGH_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS		30
#define OTA_HIGH_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS	5000

#define OTA_LOW_SPEED_BLE_CONNECTION_INTERVAL_MIN_IN_MS			400
#define OTA_LOW_SPEED_BLE_CONNECTION_INTERVAL_MAX_IN_MS			500
#define OTA_LOW_SPEED_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS	5000


extern struct app_env_tag app_env;

/// health thermometer application environment structure
struct app_ota_env_tag
{
    uint8_t connectionIndex;
	uint8_t	isNotificationEnabled;
	uint16_t mtu[BLE_CONNECTION_MAX];
};

typedef void(*app_ota_tx_done_t)(void);

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// Health Thermomter Application environment
extern struct app_ota_env_tag app_ota_env;

/// Table of message handlers
extern const struct ke_state_handler app_ota_table_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize DataPath Server Application
 ****************************************************************************************
 */
void app_ota_init(void);

void app_ota_add_ota(void);

void app_ota_connected_evt_handler(uint8_t conidx);

void app_ota_disconnected_evt_handler(uint8_t conidx);

void app_ota_send_notification(uint8_t* ptrData, uint32_t length);

void app_ota_send_indication(uint8_t* ptrData, uint32_t length);

void app_ota_register_tx_done(app_ota_tx_done_t callback);

void app_ota_mtu_exchanged_handler(uint8_t conidx, uint16_t MTU);

uint8_t app_ota_get_conidx(void);

#ifdef __cplusplus
	}
#endif


#endif //(BLE_APP_OTA)

/// @} APP

#endif // APP_OTA_H_
