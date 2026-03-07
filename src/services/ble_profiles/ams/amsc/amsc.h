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


#ifndef _AMSC_H_
#define _AMSC_H_

/**
 ****************************************************************************************
 * @addtogroup AMSC.
 * @ingroup AMS
 * @brief AMS - Client Role.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ams_common.h"

#if (BLE_AMS_CLIENT)

#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of AMSC task instances
#define AMSC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the amsc task
enum amsc_states
{
    /// Idle state
    AMSC_FREE,
    /// Connected state
    AMSC_IDLE,
    /// Discovery
    AMSC_DISCOVERING,
    /// Number of defined states.
    AMSC_STATE_MAX
};

/// AMS Characteristics
enum amsc_chars
{
    /// Remote Command
    AMSC_REMOTE_COMMAND_CHAR = 0,
    /// Entity Update
    AMSC_ENTITY_UPDATE_CHAR,
    /// Entity Attribute
    AMSC_ENTITY_ATTRIBUTE_CHAR,
    /// Number of defined states.
    AMSC_CHAR_MAX,
};

/// AMS Characteristic Descriptors
enum amsc_descs
{
    /// Remote Command - Client Characteristic Configuration
    AMSC_DESC_REMOTE_CMD_CL_CFG,
    /// Entity Update - Client Characteristic Configuration
    AMSC_DESC_ENTITY_UPDATE_CL_CFG,

    AMSC_DESC_MAX,

    AMSC_DESC_MASK = 0x10,
};

/// Pointer to the connection clean-up function
#define AMSC_CLEANUP_FNCT        (amsc_cleanup)

#define AMSC_INVALID_STATE		0xA0
#define AMSC_INVALID_COMMAND	0xA1
#define AMSC_ABSENT_ATTRIBUTE	0xA2

/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Apple Media Service
 */
struct amsc_ams_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Remote Command
    ///  - Entity Update
    ///  - Entity Attribute
    struct prf_char_inf chars[AMSC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Remote Command Client Char Cfg
    ///  - Entity Update Client Char Cfg
    struct prf_char_desc_inf descs[AMSC_DESC_MAX];
};

struct amsc_cnx_env
{
    /// Current Operation
    void *operation;

    /// Provide an indication about the last operation
    uint16_t last_req;
    /// Last characteristic code discovered
    uint8_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    /// AMS Characteristics
    struct amsc_ams_content ams;    
};

/// AMS Client environment variable
struct amsc_env_tag
{
    /// profile environment
    prf_env_t prf_env;

    // TODO(jkessinger): Verify this will work).
    uint16_t last_write_handle[AMSC_IDX_MAX];

    /// Environment variable pointer for each connections
    struct amsc_cnx_env* env[AMSC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[AMSC_IDX_MAX];

};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve AMS client profile interface
 *
 * @return AMS client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* amsc_prf_itf_get(void);
void amsc_enable_rsp_send(struct amsc_env_tag *amsc_env, uint8_t conidx, uint8_t status);
void amsc_task_init(struct ke_task_desc *task_desc);

#endif //(BLE_AMS_CLIENT)

/// @} AMSC

#endif //(_AMSC_H_)
