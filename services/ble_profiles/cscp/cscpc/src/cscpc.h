#ifndef _CSCPC_H_
#define _CSCPC_H_

/**
 ****************************************************************************************
 * @addtogroup CSCPC Cycling Speed and Cadence Profile Collector
 * @ingroup CSCP
 * @brief Cycling Speed and Cadence Profile Collector
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Cycling Speed and Cadence Collector task instances
#define CSCPC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_CSC_COLLECTOR)

#include "cscpc_task.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the CSCPC task
enum cscpc_states
{
    /// Idle state
    CSCPC_FREE,
    /// Connected state
    CSCPC_IDLE,
    /// Discovery
    CSCPC_DISCOVERING,
    /// Busy state
    CSCPC_BUSY,

    /// Number of defined states.
    CSCPC_STATE_MAX
};


/// Internal codes for reading/writing a CSCS characteristic with one single request
enum cscpc_codes
{
    /// Notified CSC Measurement
    CSCPC_NTF_CSC_MEAS          = CSCP_CSCS_CSC_MEAS_CHAR,
    /// Read CSC Feature
    CSCPC_RD_CSC_FEAT           = CSCP_CSCS_CSC_FEAT_CHAR,
    /// Read Sensor Location
    CSCPC_RD_SENSOR_LOC         = CSCP_CSCS_SENSOR_LOC_CHAR,
    /// Indicated SC Control Point
    CSCPC_IND_SC_CTNL_PT        = CSCP_CSCS_SC_CTNL_PT_CHAR,

    /// Read/Write CSC Measurement Client Char. Configuration Descriptor
    CSCPC_RD_WR_CSC_MEAS_CFG    = (CSCPC_DESC_CSC_MEAS_CL_CFG   | CSCPC_DESC_MASK),
    /// Read SC Control Point Client Char. Configuration Descriptor
    CSCPC_RD_WR_SC_CTNL_PT_CFG  = (CSCPC_DESC_SC_CTNL_PT_CL_CFG | CSCPC_DESC_MASK),
};

/*
 * STRUCTURES
 ****************************************************************************************
 */


struct cscpc_cnx_env
{
    /// Current Operation
    void *operation;
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;
    /// Cycling Speed and Cadence Service Characteristics
    struct cscpc_cscs_content cscs;
};


/// Cycling Speed and Cadence Profile Collector environment variable
struct cscpc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct cscpc_cnx_env* env[CSCPC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[CSCPC_IDX_MAX];
};

/// Command Message Basic Structure
struct cscpc_cmd
{
    /// Operation Code
    uint8_t operation;

    /// MORE DATA
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve CSCP client profile interface
 *
 * @return CSCP client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* cscpc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Cycling Power ATT DB discovery results to CSCPC host.
 ****************************************************************************************
 */
void cscpc_enable_rsp_send(struct cscpc_env_tag *cscpc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a CSCPC_CMP_EVT message when no connection exists (no environment)
 ****************************************************************************************
 */
void cscpc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a CSCPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void cscpc_send_cmp_evt(struct cscpc_env_tag *cscpc_env, uint8_t conidx, uint8_t operation, uint8_t status);

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
void cscpc_task_init(struct ke_task_desc *task_desc);

#endif //(BLE_CSC_COLLECTOR)

/// @} CSCPC

#endif //(_CSCPC_H_)
