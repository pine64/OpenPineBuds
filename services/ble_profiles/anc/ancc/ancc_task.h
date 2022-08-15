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


#ifndef _ANCC_TASK_H_
#define _ANCC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup ANCCTASK ANCS Client Task
 * @ingroup ANCC
 * @brief ANCS Client Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "anc_common.h"

#if (BLE_ANC_CLIENT)

#include "ke_task.h"
#include "gattm_task.h"
#include "ancc.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

/// Message IDs
enum
{
    /// Enable the ANCS Client task - at connection
    ANCC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_ANCC),

    /// Enable the ANCS Client task - at connection
    ANCC_ENABLE_RSP,

    /// Disable Indication
    ANCC_DISABLE_IND,

    /// Read the value of a Client Characteristic Configuration Descriptor in the peer device database
    ANCC_READ_CMD,
    
    /// Write the value of a Client Characteristic Configuration Descriptor in the peer device database
    ANCC_WRITE_CMD,

    /// Procedure Timeout Timer
    ANCC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum
{
    /// Reserved operation code
    ANCC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    ANCC_ENABLE_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref ANCC_ENABLE_REQ message
struct ancc_enable_req
{
	uint8_t conidx;
};


/// Parameters of the @ref ANCC_ENABLE_RSP message
struct ancc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values ANC
    struct ancc_anc_content anc;
};

/// Parameters of the @ref ANCC_DISABLE_IND message
struct ancc_disable_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

//extern const struct ke_state_handler ancc_state_handler[ANCC_STATE_MAX];
//extern const struct ke_state_handler ancc_default_handler;
//extern ke_state_t ancc_state[ANCC_IDX_MAX];

#endif //(BLE_ANC_CLIENT)

/// @} ANCCTASK

#endif //(_ANCC_TASK_H_)
