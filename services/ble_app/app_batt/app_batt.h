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
#ifndef APP_BATT_H_
#define APP_BATT_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Battery Application Module entry point
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_BATT)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"         // Kernel Task Definition

/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */

/// Battery Application Module Environment Structure
struct app_batt_env_tag
{
    /// Connection handle
    uint8_t conidx;
    /// Current Battery Level
    uint8_t batt_lvl;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// Battery Application environment
extern struct app_batt_env_tag app_batt_env;

/// Table of message handlers
extern const struct ke_state_handler app_batt_table_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * Health Thermometer Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Battery Application Module
 ****************************************************************************************
 */
void app_batt_init(void);

/**
 ****************************************************************************************
 * @brief Add a Battery Service instance in the DB
 ****************************************************************************************
 */
void app_batt_add_bas(void);

/**
 ****************************************************************************************
 * @brief Enable the Battery Service
 ****************************************************************************************
 */
void app_batt_enable_prf(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Send a Battery level value
 ****************************************************************************************
 */
void app_batt_send_lvl(uint8_t conidx, uint8_t batt_lvl);

#endif //(BLE_APP_BATT)

/// @} APP

#endif // APP_BATT_H_
