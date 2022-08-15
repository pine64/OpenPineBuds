#ifndef _HOGPRH_H_
#define _HOGPRH_H_


/**
 ****************************************************************************************
 * @addtogroup HOGPRH HID Over GATT Profile Report Host
 * @ingroup HOGP
 * @brief HID Over GATT Profile Report Host
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_HID_REPORT_HOST)

#include "hogp_common.h"
#include "ke_task.h"
#include "hogprh_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of HID Over GATT Report Host task instances
#define HOGPRH_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the HOGPRH task
enum hogprh_state
{
    /// Disconnected state
    HOGPRH_FREE,
    /// IDLE state
    HOGPRH_IDLE,
    /// Busy State
    HOGPRH_BUSY,
    /// Number of defined states.
    HOGPRH_STATE_MAX
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Environment variable for each Connections
struct hogprh_cnx_env
{
    /// on-going operation
    struct ke_msg * operation;
    ///HIDS characteristics
    struct hogprh_content hids[HOGPRH_NB_HIDS_INST_MAX];
    ///Number of HIDS instances found
    uint8_t hids_nb;
};

/// HID Over Gatt Profile Boot Host environment variable
struct hogprh_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct hogprh_cnx_env* env[HOGPRH_IDX_MAX];
    /// State of different task instances
    ke_state_t state[HOGPRH_IDX_MAX];
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
 * @brief Retrieve HID report host profile interface
 *
 * @return HID report host profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* hogprh_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send HID ATT DB discovery results to HOGPRH host.
 ****************************************************************************************
 */
void hogprh_enable_rsp_send(struct hogprh_env_tag *hogprh_env, uint8_t conidx, uint8_t status);


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
void hogprh_task_init(struct ke_task_desc *task_desc);



#endif /* (BLE_HID_REPORT_HOST) */

/// @} HOGPRH

#endif /* _HOGPRH_H_ */
