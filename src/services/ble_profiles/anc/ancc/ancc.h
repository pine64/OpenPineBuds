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


#ifndef _ANCC_H_
#define _ANCC_H_

/**
 ****************************************************************************************
 * @addtogroup ANCC.
 * @ingroup ANC
 * @brief ANCS - Client Role.
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
#include "prf_types.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of ANCC task instances
#define ANCC_IDX_MAX        (BLE_CONNECTION_MAX)

/// Possible states of the ancc task
enum ancc_states
{
    /// Idle state
    ANCC_FREE,
    /// Connected state
    ANCC_IDLE,
    /// Discovery
    ANCC_DISCOVERING,

    /// Number of defined states.
    ANCC_STATE_MAX
};

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// ANCS Characteristics
enum ancc_anc_chars
{
    /// Notification Source
    ANCC_CHAR_NTF_SRC,
    /// Control Point
    ANCC_CHAR_CTRL_PT,
    /// Data Source
    ANCC_CHAR_DATA_SRC,

    ANCC_CHAR_MAX,
};

/// ANCS Characteristic Descriptors
enum ancc_anc_descs
{
    /// Notification Source - Client Characteristic Configuration
    ANCC_DESC_NTF_SRC_CL_CFG,
    /// Data Source - Client Characteristic Configuration
    ANCC_DESC_DATA_SRC_CL_CFG,

    ANCC_DESC_MAX,

    ANCC_DESC_MASK = 0x10,
};

/// Pointer to the connection clean-up function
#define ANCC_CLEANUP_FNCT        (ancc_cleanup)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Alert Notification Service
 */
struct ancc_anc_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Notification Source
    ///  - Control Point
    ///  - Data Source
    struct prf_char_inf chars[ANCC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Notification Source Client Char Cfg
    ///  - Data Source Client Char Cfg
    struct prf_char_desc_inf descs[ANCC_DESC_MAX];
};

struct ancc_cnx_env
{
    /// Current Operation
    void *operation;

    /// Provide an indication about the last operation
    uint16_t last_req;
    /// Last characteristic code discovered
    uint8_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    /// ANCS Characteristics
    struct ancc_anc_content anc;
};

/// ANCS Client environment variable
struct ancc_env_tag
{
    /// profile environment
    prf_env_t prf_env;

    // TODO(jkessinger): Is this safe? Reliable? WTF?
    uint16_t last_write_handle[ANCC_IDX_MAX];
    /// Environment variable pointer for each connections
    struct ancc_cnx_env* env[ANCC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[ANCC_IDX_MAX];
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve ANCS client profile interface
 *
 * @return ANCS client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* ancc_prf_itf_get(void);
void ancc_enable_rsp_send(struct ancc_env_tag *ancc_env, uint8_t conidx, uint8_t status);
void ancc_task_init(struct ke_task_desc *task_desc);

#endif //(BLE_ANC_CLIENT)

/// @} ANCC

#endif //(_ANCC_H_)
