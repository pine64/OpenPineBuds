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


#ifndef _AMSC_TASK_H_
#define _AMSC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup AMSCTASK AMS Client Task
 * @ingroup AMSC
 * @brief AMS Client Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_AMS_CLIENT)

#include "ke_task.h"
#include "gattm_task.h"
#include "amsc.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

/// Message IDs
enum
{
    /// Enable the AMS Client task - at connection
    AMSC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_AMSC),

    /// Enable the AMS Client task - at connection
    AMSC_ENABLE_RSP,

    /// Read the value of a Client Characteristic Configuration Descriptor in the peer device database
    AMSC_READ_CMD,
    
    /// Write the value of a Client Characteristic Configuration Descriptor in the peer device database
    AMSC_WRITE_CMD,

    /// Procedure Timeout Timer
    AMSC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum
{
    /// Reserved operation code
    AMSC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    AMSC_ENABLE_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref AMSC_ENABLE_REQ message
struct amsc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
	uint8_t conidx;
};


/// Parameters of the @ref AMSC_ENABLE_RSP message
struct amsc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values AMS
    struct amsc_ams_content ams;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

//extern const struct ke_state_handler amsc_state_handler[AMSC_STATE_MAX];
//extern const struct ke_state_handler amsc_default_handler;
//extern ke_state_t amsc_state[AMSC_IDX_MAX];

#endif //(BLE_ANC_CLIENT)

/// @} AMSCTASK

#endif //(_AMSC_TASK_H_)
