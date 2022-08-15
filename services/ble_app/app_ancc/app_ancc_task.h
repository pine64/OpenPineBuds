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


#include "rwip_config.h"

#ifndef APP_ANCC_TASK_H_
#define APP_ANCC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 *
 * @brief 
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#if BLE_ANC_CLIENT

#include "ancc_task.h"

/// Table of message handlers
extern const struct ke_state_handler app_ancc_table_handler;

#endif //BLE_ANC_CLIENT

/// @} APP

#endif // APP_ANCC_TASK_H_
