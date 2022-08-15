#ifndef _HOGPBH_H_
#define _HOGPBH_H_


/**
 ****************************************************************************************
 * @addtogroup HOGPBH HID Over GATT Profile Client
 * @ingroup HOGP
 * @brief HID Over GATT Profile Client
 * @{
 ****************************************************************************************
 */
#include "rwip_config.h"
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HID_BOOT_HOST)

#include "hogp_common.h"
#include "ke_task.h"
#include "hogpbh_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf.h"

/*
 * DEFINES
 ****************************************************************************************
 */


///Maximum number of HID Over GATT Boot Device task instances
#define HOGPBH_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * MACROS
 ****************************************************************************************
*/

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Possible states of the HOGPBH task
enum hogpbh_state
{
    /// Disconnected state
    HOGPBH_FREE,
    /// IDLE state
    HOGPBH_IDLE,
    /// Busy State
    HOGPBH_BUSY,
    /// Number of defined states.
    HOGPBH_STATE_MAX
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Environment variable for each Connections
struct hogpbh_cnx_env
{
    /// on-going operation
    struct ke_msg * operation;
    ///HIDS characteristics
    struct hogpbh_content hids[HOGPBH_NB_HIDS_INST_MAX];
    ///Number of HIDS instances found
    uint8_t hids_nb;
};

/// HID Over Gatt Profile Boot Host environment variable
struct hogpbh_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct hogpbh_cnx_env* env[HOGPBH_IDX_MAX];
    /// State of different task instances
    ke_state_t state[HOGPBH_IDX_MAX];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * GLOBAL FUNCTIONS DECLARATIONS
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief Retrieve HID boot host profile interface
 *
 * @return HID boot host profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* hogpbh_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send HID ATT DB discovery results to HOGPBH host.
 ****************************************************************************************
 */
void hogpbh_enable_rsp_send(struct hogpbh_env_tag *hogpbh_env, uint8_t conidx, uint8_t status);

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
void hogpbh_task_init(struct ke_task_desc *task_desc);



#endif /* (BLE_HID_BOOT_HOST) */

/// @} HOGPBH

#endif /* _HOGPBH_H_ */
