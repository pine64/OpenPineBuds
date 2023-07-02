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


#ifndef _TOTA_H_
#define _TOTA_H_

/**
 ****************************************************************************************
 * @addtogroup TOTA Datapath Profile Server
 * @ingroup TOTAP
 * @brief Datapath Profile Server
 *
 * Datapath Profile Server provides functionalities to upper layer module
 * application. The device using this profile takes the role of Datapath Server.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Send data to peer device via notifications  (from APP)
 *  - Receive data from peer device via write no response (from APP)
 *
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_TOTA)
#include "prf_types.h"
#include "prf.h"
// #include "tota_task.h"
#include "attm.h"
#include "prf_utils.h"


/*
 * DEFINES
 ****************************************************************************************
 */
#define TOTA_MAX_LEN            			(509)	

/*
 * DEFINES
 ****************************************************************************************
 */
/// Possible states of the TOTA task
enum
{
    /// Idle state
    TOTA_IDLE,
    /// Connected state
    TOTA_BUSY,

    /// Number of defined states.
    TOTA_STATE_MAX,
};

///Attributes State Machine
enum
{
    TOTA_IDX_SVC,

    TOTA_IDX_CHAR,
    TOTA_IDX_VAL,
    TOTA_IDX_NTF_CFG,

    TOTA_IDX_NB,
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Datapath Profile Server environment variable
struct tota_env_tag
{
    /// profile environment
    prf_env_t 	prf_env;
	uint8_t		isNotificationEnabled[BLE_CONNECTION_MAX];
    /// Service Start Handle
    uint16_t 	shdl;
    /// State of different task instances
    ke_state_t 	state;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve HRP service profile interface
 *
 * @return HRP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* tota_prf_itf_get(void);


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void tota_task_init(struct ke_task_desc *task_desc);



#endif /* #if (BLE_TOTA) */

/// @} TOTA

#endif /* _TOTA_H_ */