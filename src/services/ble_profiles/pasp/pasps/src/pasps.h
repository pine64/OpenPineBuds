#ifndef PASPS_H_
#define PASPS_H_

/**
 ****************************************************************************************
 * @addtogroup PASPS Phone Alert Status Profile Server
 * @ingroup PASP
 * @brief Phone Alert Status Profile Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#if (BLE_PAS_SERVER)
#include "pasp_common.h"

#include <stdint.h>
#include <stdbool.h>
#include "prf_types.h"
#include "prf.h"
#include "attm.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Number of parallel instances of the task
#define PASPS_IDX_MAX           (BLE_CONNECTION_MAX)
/// Database Configuration Flag
#define PASPS_DB_CFG_FLAG       (0x01FF)

/// Notification States Flags
#define PASPS_FLAG_ALERT_STATUS_CFG         (0x01)
#define PASPS_FLAG_RINGER_SETTING_CFG       (0x02)
/// CP Discovery or Bonded data used
#define PASPS_FLAG_CFG_PERFORMED_OK         (0x80)


/*
 * MACROS
 ****************************************************************************************
 */

#define PASPS_IS_NTF_ENABLED(conidx, idx_env, flag) ((idx_env->env[conidx]->ntf_state & flag) == flag)

#define PASPS_ENABLE_NTF(conidx, idx_env, flag)     (idx_env->env[conidx]->ntf_state |= flag)

#define PASPS_DISABLE_NTF(conidx, idx_env, flag)    (idx_env->env[conidx]->ntf_state &= ~flag)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the PASPS task
enum pasps_states
{
    /// Not connected state
    PASPS_FREE,
    /// Idle state
    PASPS_IDLE,
    /// Busy state
    PASPS_BUSY,

    /// Number of defined states.
    PASPS_STATE_MAX
};

/// Attributes State Machine
enum pasps_pass_att_list
{
    PASS_IDX_SVC,

    PASS_IDX_ALERT_STATUS_CHAR,
    PASS_IDX_ALERT_STATUS_VAL,
    PASS_IDX_ALERT_STATUS_CFG,

    PASS_IDX_RINGER_SETTING_CHAR,
    PASS_IDX_RINGER_SETTING_VAL,
    PASS_IDX_RINGER_SETTING_CFG,

    PASS_IDX_RINGER_CTNL_PT_CHAR,
    PASS_IDX_RINGER_CTNL_PT_VAL,

    PASS_IDX_NB,
};

/// Attribute Codes
enum pasps_att_code
{
    /// Alert Status Characteristic
    PASPS_ALERT_STATUS_CHAR_VAL,
    /// Ringer Setting Characteristic
    PASPS_RINGER_SETTING_CHAR_VAL,
    /// Ringer Control Point Characteristic
    PASPS_RINGER_CTNL_PT_CHAR_VAL,

    /// Alert Status Characteristic - Notification Configuration
    PASPS_ALERT_STATUS_NTF_CFG,
    /// Ringer Setting Characteristic - Notification Configuration
    PASPS_RINGER_SETTING_NTF_CFG,
};



/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Phone Alert Status Profile Server Connection Dependent Environment Variable
struct pasps_cnx_env
{
    /// Ringer State
    uint8_t ringer_state;

    /**
     * Ringer State + Notification State
     *     Bit 0: Alert Status notification configuration
     *     Bit 1: Ringer setting notification configuration
     */
    uint8_t ntf_state;
};

/// Phone Alert Status Profile Server Environment Variable
struct pasps_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Phone Alert Status Service Start Handle
    uint16_t shdl;
    /// Environment variable pointer for each connections
    struct pasps_cnx_env* env[BLE_CONNECTION_MAX];

    /// Current Operation Code
    uint8_t operation;
    /// Alert Status Char. Value
    uint8_t alert_status;
    /// Ringer Settings Char. Value
    uint8_t ringer_setting;

    /// State of different task instances
    ke_state_t state[PASPS_IDX_MAX];
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve PASP service profile interface
 *
 * @return PASP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* pasps_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send a PASPS_CMP_EVT message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_PASPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void pasps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id,
                        uint8_t operation, uint8_t status);

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
void pasps_task_init(struct ke_task_desc *task_desc);


#endif //(BLE_PAS_SERVER)

/// @} PASPS

#endif // PASPS_H_
