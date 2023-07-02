#ifndef _LANC_H_
#define _LANC_H_

/**
 ****************************************************************************************
 * @addtogroup LANC Location and Navigation Profile Collector
 * @ingroup LAN
 * @brief Location and Navigation Profile Collector
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#if (BLE_LN_COLLECTOR)

#include "lan_common.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "lanc_task.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Location and Navigation Collector task instances
#define LANC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
/// Possible states of the LANC task
enum lanc_states
{
    /// Idle state
    LANC_FREE,
    /// Connected state
    LANC_IDLE,
    /// SDP state
    LANC_DISCOVERING,
    /// Busy state
    LANC_BUSY,

    /// Number of defined states.
    LANC_STATE_MAX
};


/// Internal codes for reading/writing a LNS characteristic with one single request
enum lanc_code
{
    /// Read LN Feature
    LANC_RD_LN_FEAT           = LANP_LANS_LN_FEAT_CHAR,
    /// Notified Location and Speed
    LANC_NTF_LOC_SPEED        = LANP_LANS_LOC_SPEED_CHAR,
    /// Read Position quality
    LANC_RD_POS_Q             = LANP_LANS_POS_Q_CHAR,
    /// Indicated LN Control Point
    LANC_IND_LN_CTNL_PT       = LANP_LANS_LN_CTNL_PT_CHAR,
    /// Notified Navigation
    LANC_NTF_NAVIGATION       = LANP_LANS_NAVIG_CHAR,

    /// Read/Write Location and Speed Client Char. Configuration Descriptor
    LANC_RD_WR_LOC_SPEED_CL_CFG = (LANC_DESC_LOC_SPEED_CL_CFG | LANC_DESC_MASK),

    /// Read LN Control Point Client Char. Configuration Descriptor
    LANC_RD_WR_LN_CTNL_PT_CFG   = (LANC_DESC_LN_CTNL_PT_CL_CFG | LANC_DESC_MASK),

    /// Read/Write Vector Client Char. Configuration Descriptor
    LANC_RD_WR_NAVIGATION_CFG   = (LANC_DESC_NAVIGATION_CL_CFG   | LANC_DESC_MASK),

};


/*
 * STRUCTURES
 ****************************************************************************************
 */

struct lanc_cnx_env
{
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;
    /// Location and Navigation Service Characteristics
    struct lanc_lns_content lans;
    /// Current Operation
    void *operation;
};

/// Location and Navigation Profile Collector environment variable
struct lanc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct lanc_cnx_env* env[LANC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[LANC_IDX_MAX];
};

/// Command Message Basic Structure
struct lanc_cmd
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
 * @brief Retrieve LAN client profile interface
 *
 * @return LAN client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* lanc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Location and Navigation ATT DB discovery results to LANC host.
 * @param[in] lanc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] status Status
 * @return handle
 ****************************************************************************************
 */
void lanc_enable_rsp_send(struct lanc_env_tag *lanc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a LANC_CMP_EVT message when no connection exists (no environment)
 * @param[in] src_id Source task
 * @param[in] dest_id Destination task
 * @param[in] operation Operation
 ****************************************************************************************
 */
void lanc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a LANC_CMP_EVT message to the task which enabled the profile
 * @param[in] lanc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] operation Operation
 * @param[in] status Status
 * @return handle
 ****************************************************************************************
 */
void lanc_send_cmp_evt(struct lanc_env_tag *lanc_env, uint8_t conidx, uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Gets correct read handle according to the request
 * @param[in] lanc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @return handle
 ****************************************************************************************
 */
uint16_t lanc_get_read_handle_req (struct lanc_env_tag *lanc_env, uint8_t conidx, struct lanc_read_cmd *param);

/**
 ****************************************************************************************
 * @brief Gets correct write handle according to the request
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] lanc_env environment variable
 * @param[out] handle handle
 * @return handle
 ****************************************************************************************
 */
uint8_t lanc_get_write_desc_handle_req (uint8_t conidx, struct lanc_cfg_ntfind_cmd *param, struct lanc_env_tag *lanc_env, uint16_t *handle);

/**
 ****************************************************************************************
 * @brief Unpacks location and speed data and sends the indication
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] lanc_env environment variable
 * @return length
 ****************************************************************************************
 */
uint8_t lanc_unpack_loc_speed_ind (uint8_t conidx, struct gattc_event_ind const *param, struct lanc_env_tag *lanc_env);

/**
 ****************************************************************************************
 * @brief Unpacks Navigation and sends the indication
 * @param[in] conidx Connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] lanc_env environment variable
 * @return length
 ****************************************************************************************
 */
uint8_t lanc_unpack_navigation_ind (uint8_t conidx, struct gattc_event_ind const *param, struct lanc_env_tag *lanc_env);

/**
 ****************************************************************************************
 * @brief Unpacks position quality
 * @param[in] param Pointer to the parameters of the message
 * @param[out] ind Pointer to the value indication
 * @return length
 ****************************************************************************************
 */
uint8_t lanc_unpack_pos_q_ind (struct gattc_read_ind const *param, struct lanc_value_ind *ind);

/**
 ****************************************************************************************
 * @brief Packs Control Point data
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] req packed message
 * @param[out] status status of the operation
 * @return length
 ****************************************************************************************
 */
uint8_t lanc_pack_ln_ctnl_pt_req (struct lanc_ln_ctnl_pt_cfg_req *param, uint8_t *req, uint8_t *status);

/**
 ****************************************************************************************
 * @brief Unpacks Control Point data and sends the indication
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] src Source task
 * @param[in] dest Destination task
 * @return length
 ****************************************************************************************
 */
uint8_t lanc_unpack_ln_ctln_pt_ind (struct gattc_event_ind const *param, ke_task_id_t src, ke_task_id_t dest);

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
void lanc_task_init(struct ke_task_desc *task_desc);
#endif //(BLE_LN_COLLECTOR)

/// @} LANC

#endif //(_LANC_H_)
