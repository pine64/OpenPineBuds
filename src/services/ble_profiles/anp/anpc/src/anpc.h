#ifndef _ANPC_H_
#define _ANPC_H_

/**
 ****************************************************************************************
 * @addtogroup ANPC Alert Notification Profile Client
 * @ingroup ANP
 * @brief Phone Alert Notification Profile Client
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_AN_CLIENT)
#include "anp_common.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "anpc_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Alert Notification Client task instances
#define ANPC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the ANPC task
enum anpc_states
{
    /// Free state
    ANPC_FREE,
    /// Idle state
    ANPC_IDLE,
    /// Connected state
    ANPC_DISCOVERING,
    /// Busy state
    ANPC_BUSY,

    /// Number of defined states.
    ANPC_STATE_MAX
};

/// Codes for reading/writing a ANS characteristic with one single request
enum anpc_rd_wr_ntf_codes
{
    /// Read ANS Supported New Alert Category
    ANPC_RD_SUP_NEW_ALERT_CAT           = ANPC_CHAR_SUP_NEW_ALERT_CAT,
    /// NTF New Alert
    ANPC_NTF_NEW_ALERT                  = ANPC_CHAR_NEW_ALERT,
    /// Read ANS Supported Unread Alert Category
    ANPC_RD_SUP_UNREAD_ALERT_CAT        = ANPC_CHAR_SUP_UNREAD_ALERT_CAT,
    /// NTF Unread Alert Categories
    ANPC_NTF_UNREAD_ALERT               = ANPC_CHAR_UNREAD_ALERT_STATUS,
    /// Write ANS Alert Notification Control Point
    ANPC_WR_ALERT_NTF_CTNL_PT           = ANPC_CHAR_ALERT_NTF_CTNL_PT,

    /// Read/Write ANS New Alert Client Characteristic Configuration Descriptor
    ANPC_RD_WR_NEW_ALERT_CFG            = (ANPC_DESC_NEW_ALERT_CL_CFG | ANPC_DESC_MASK),
    /// Read ANS Unread Alert Status Client Characteristic Configuration Descriptor
    ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG  = (ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG | ANPC_DESC_MASK),
};

/*
 * STRUCTURES
 ****************************************************************************************
 */


/// Command Message Basic Structure
struct anpc_cmd
{
    /// Operation Code
    uint8_t operation;

    /// MORE DATA
};

struct anpc_cnx_env
{
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last characteristic code used in a read or a write request
    uint16_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;
    /// Current operation code
    void *operation;
    /// Phone Alert Status Service Characteristics
    struct anpc_ans_content ans;
};

/// Alert Notification Profile Client environment variable
struct anpc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct anpc_cnx_env* env[ANPC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[ANPC_IDX_MAX];
};

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Send Alert Notification ATT DB discovery results to ANPC host.
 * @param[in] paspc_env environment variable
 * @param[in] conidx Connection index
 * @param[in] status Satus
 ****************************************************************************************
 */
void anpc_enable_rsp_send(struct anpc_env_tag *anpc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Initialization of the ANPC module.
 * This function performs all the initializations of the ANPC module.
 ****************************************************************************************
 */
bool anpc_found_next_alert_cat(struct anpc_env_tag *idx_env, uint8_t conidx,
        struct anp_cat_id_bit_mask cat_id);

/**
 ****************************************************************************************
 * @brief Initialization of the ANPC module.
 * This function performs all the initializations of the ANPC module.
 ****************************************************************************************
 */
void anpc_write_alert_ntf_ctnl_pt(struct anpc_env_tag *idx_env, uint8_t conidx,
        uint8_t cmd_id, uint8_t cat_id);

/**
 ****************************************************************************************
 * @brief Send a ANPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void anpc_send_cmp_evt(struct anpc_env_tag *anpc_env, uint8_t conidx, uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Retrieve ANP client profile interface
 * @return ANP client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* anpc_prf_itf_get(void);

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
void anpc_task_init(struct ke_task_desc *task_desc);

#endif //(BLE_AN_CLIENT)

/// @} ANPC

#endif //(_ANPC_H_)
