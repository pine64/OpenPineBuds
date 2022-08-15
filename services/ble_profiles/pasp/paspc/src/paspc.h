#ifndef _PASPC_H_
#define _PASPC_H_

/**
 ****************************************************************************************
 * @addtogroup PASPC Phone Alert Status Profile Client
 * @ingroup PASP
 * @brief Phone Alert Status Profile Client
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PAS_CLIENT)
#include "pasp_common.h"
#include "paspc_task.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Phone Alert Status Client task instances
#define PASPC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the PASPC task
enum paspc_states
{
    /// Idle state
    PASPC_FREE,
    /// Connected state
    PASPC_IDLE,
    /// Busy state
    PASPC_DISCOVERING,
    /// Busy state
    PASPC_BUSY,

    /// Number of defined states.
    PASPC_STATE_MAX
};


/// Internal codes for reading/writing a PAS characteristic with one single request
enum
{
    /// Read PAS Alert Status
    PASPC_RD_ALERT_STATUS              = PASPC_CHAR_ALERT_STATUS,
    /// Read PAS Ringer Setting
    PASPC_RD_RINGER_SETTING            = PASPC_CHAR_RINGER_SETTING,
    /// Write PAS Ringer Control Point
    PASPC_WR_RINGER_CTNL_PT            = PASPC_CHAR_RINGER_CTNL_PT,

    /// Read/Write PAS Alert Status Client Characteristic Configuration Descriptor
    PASPC_RD_WR_ALERT_STATUS_CFG       = (PASPC_DESC_ALERT_STATUS_CL_CFG | PASPC_DESC_MASK),
    /// Read PAS Ringer Setting Client Characteristic Configuration Descriptor
    PASPC_RD_WR_RINGER_SETTING_CFG     = (PASPC_DESC_RINGER_SETTING_CL_CFG | PASPC_DESC_MASK),
};



/*
 * STRUCTURES
 ****************************************************************************************
 */


struct paspc_cnx_env
{
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last characteristic code used in a read or a write request
    uint16_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;
    /// Current operation code
    uint8_t operation;
    /// Phone Alert Status Service Characteristics
    struct paspc_pass_content pass;
};

/// Phone Alert Status Profile Client environment variable
struct paspc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct paspc_cnx_env* env[PASPC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[PASPC_IDX_MAX];
};

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve PASP client profile interface
 * @return PASP client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* paspc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Phone Alert Status ATT DB discovery results to PASPC host.
 * @param[in] paspc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] status Satus
 ****************************************************************************************
 */
void paspc_enable_rsp_send(struct paspc_env_tag *paspc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a PASPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void paspc_send_cmp_evt(struct paspc_env_tag *paspc_env, uint8_t conidx,
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
void paspc_task_init(struct ke_task_desc *task_desc);

#endif //(BLE_PAS_CLIENT)

/// @} PASPC

#endif //(_PASPC_H_)
