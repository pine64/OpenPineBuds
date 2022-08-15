#ifndef _BASC_H_
#define _BASC_H_

/**
 ****************************************************************************************
 * @addtogroup BASC Battery Service Client
 * @ingroup BAS
 * @brief Battery Service Client
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_BATT_CLIENT)

#include <stdint.h>
#include <stdbool.h>
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "basc_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Battery Client task instances
#define BASC_IDX_MAX     (BLE_CONNECTION_MAX)

/// Possible states of the BAPC task
enum basc_state
{
    /// Disconnected state
    BASC_FREE,
    /// IDLE state
    BASC_IDLE,
    /// Busy State
    BASC_BUSY,

    /// Number of defined states.
    BASC_STATE_MAX
};

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Environment variable for each Connections
struct basc_cnx_env
{
    /// on-going operation
    struct ke_msg * operation;
    ///BAS characteristics
    struct bas_content bas[BASC_NB_BAS_INSTANCES_MAX];

    ///Number of BAS instances found
    uint8_t bas_nb;
};

/// Battery 'Profile' Client environment variable
struct basc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct basc_cnx_env* env[BASC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[BASC_IDX_MAX];
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
 * @brief Retrieve BAS client profile interface
 *
 * @return BAS client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* basc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send BAS ATT DB discovery results to BASC host.
 ****************************************************************************************
 */
void basc_enable_rsp_send(struct basc_env_tag *basc_env, uint8_t conidx, uint8_t status);

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
void basc_task_init(struct ke_task_desc *task_desc);

#endif /* (BLE_BATT_CLIENT) */

/// @} BASC

#endif /* _BASC_H_ */
