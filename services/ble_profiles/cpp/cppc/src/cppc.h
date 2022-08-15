#ifndef _CPPC_H_
#define _CPPC_H_

/**
 ****************************************************************************************
 * @addtogroup CPPC Cycling Power Profile Collector
 * @ingroup CPP
 * @brief Cycling Power Profile Collector
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"

#if (BLE_CP_COLLECTOR)

#include "cppc_task.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"



/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximum number of Cycling Power Collector task instances
#define CPPC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the CPPC task
enum cppc_states
{
    /// Idle state
    CPPC_FREE,
    /// Connected state
    CPPC_IDLE,
    /// Busy state
    CPPC_DISCOVERING,
    /// Busy state
    CPPC_BUSY,

    /// Number of defined states.
    CPPC_STATE_MAX
};

/// Internal codes for reading/writing a CPS characteristic with one single request
enum cppc_code
{
    /// Notified CP Measurement
    CPPC_NTF_CP_MEAS          = CPP_CPS_MEAS_CHAR,
    /// Read CP Feature
    CPPC_RD_CP_FEAT           = CPP_CPS_FEAT_CHAR,
    /// Read Sensor Location
    CPPC_RD_SENSOR_LOC        = CPP_CPS_SENSOR_LOC_CHAR,
    /// Notified Vector
    CPPC_NTF_CP_VECTOR           = CPP_CPS_VECTOR_CHAR,
    /// Indicated SC Control Point
    CPPC_IND_CTNL_PT       = CPP_CPS_CTNL_PT_CHAR,

    /// Read/Write CP Measurement Client Char. Configuration Descriptor
    CPPC_RD_WR_CP_MEAS_CL_CFG    = (CPPC_DESC_CP_MEAS_CL_CFG   | CPPC_DESC_MASK),
    /// Read/Write CP Measurement Server Char. Configuration Descriptor
    CPPC_RD_WR_CP_MEAS_SV_CFG    = (CPPC_DESC_CP_MEAS_SV_CFG   | CPPC_DESC_MASK),

    /// Read/Write Vector Client Char. Configuration Descriptor
    CPPC_RD_WR_VECTOR_CFG    = (CPPC_DESC_VECTOR_CL_CFG   | CPPC_DESC_MASK),
    /// Read SC Control Point Client Char. Configuration Descriptor
    CPPC_RD_WR_CTNL_PT_CFG  = (CPPC_DESC_CTNL_PT_CL_CFG | CPPC_DESC_MASK),
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

struct cppc_cnx_env
{
    /// Current Operation
    void *operation;
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;
    /// Cycling Power Service Characteristics
    struct cppc_cps_content cps;
};

/// Cycling Power Profile Collector environment variable
struct cppc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct cppc_cnx_env* env[CPPC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[CPPC_IDX_MAX];
};

/// Command Message Basic Structure
struct cppc_cmd
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
 * @brief Retrieve CPP client profile interface
 * @return CPP client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* cppc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Cycling Power ATT DB discovery results to CPPC host.
 * @param[in] cppc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] status Satus
 ****************************************************************************************
 */
void cppc_enable_rsp_send(struct cppc_env_tag *cppc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a CPPC_CMP_EVT message when no connection exists (no environment)
 * @param[in] src_id Source task
 * @param[in] dest_id Destination task
 * @param[in] operation Operation
 ****************************************************************************************
 */
void cppc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a CPPC_CMP_EVT message to the task which enabled the profile
 * @param[in] cppc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] operation Operation
 * @param[in] status Satus
 ****************************************************************************************
 */
void cppc_send_cmp_evt(struct cppc_env_tag *cppc_env, uint8_t conidx, uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Gets correct read handle according to the request
 * @param[in] cppc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @return handle
 ****************************************************************************************
 */
uint16_t cppc_get_read_handle_req (struct cppc_env_tag *cppc_env, uint8_t conidx, struct cpps_read_cmd *param);

/**
 ****************************************************************************************
 * @brief Gets correct write handle according to the request
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @param[out] handle handle
 * @return status of the operation
 ****************************************************************************************
 */
uint8_t cppc_get_write_desc_handle_req (uint8_t conidx, struct cppc_cfg_ntfind_cmd *param, struct cppc_env_tag *cppc_env, uint16_t *handle);

/**
 ****************************************************************************************
 * @brief Unpacks measurement data and sends the indication
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return length
 ****************************************************************************************
 */
uint8_t cppc_unpack_meas_ind (uint8_t conidx, struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env);

/**
 ****************************************************************************************
 * @brief Unpacks Vector data and sends the indication
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return length
 ****************************************************************************************
 */
uint8_t cppc_unpack_vector_ind (uint8_t conidx, struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env);

/**
 ****************************************************************************************
 * @brief Packs Control Point data
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] req packed message
 * @param[out] status status of the operation
 * @return length
 ****************************************************************************************
 */
uint8_t cppc_pack_ctnl_pt_req (struct cppc_ctnl_pt_cfg_req *param, uint8_t *req, uint8_t *status);

/**
 ****************************************************************************************
 * @brief Unpacks Control Point data and sends the indication
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return length
 ****************************************************************************************
 */
uint8_t cppc_unpack_ctln_pt_ind (uint8_t conidx, struct gattc_event_ind const *param,struct cppc_env_tag *cppc_env);

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
void cppc_task_init(struct ke_task_desc *task_desc);


#endif //(BLE_CP_COLLECTOR)

/// @} CPPC

#endif //(_CPPC_H_)
