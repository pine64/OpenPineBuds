#ifndef ANPS_H_
#define ANPS_H_

/**
 ****************************************************************************************
 * @addtogroup ANPS Alert Notification Profile Server
 * @ingroup ANP
 * @brief Alert Notification Profile Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "anp_common.h"

#if (BLE_AN_SERVER)

#include "prf_types.h"
#include "prf.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal number of Alert Notification Server task instances
#define ANPS_IDX_MAX        (BLE_CONNECTION_MAX)
/// Database Configuration Flag
#define ANPS_DB_CONFIG_MASK         (0x1FFF)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the ANPS task
enum anps_states
{
    /// Not connected state
    ANPS_FREE,
    /// Idle state
    ANPS_IDLE,
    /// Busy state
    ANPS_BUSY,

    /// Number of defined states.
    ANPS_STATE_MAX
};
/// Alert Notification Service Attributes
enum anps_ans_att_list
{
    ANS_IDX_SVC,

    ANS_IDX_SUPP_NEW_ALERT_CAT_CHAR,
    ANS_IDX_SUPP_NEW_ALERT_CAT_VAL,

    ANS_IDX_NEW_ALERT_CHAR,
    ANS_IDX_NEW_ALERT_VAL,
    ANS_IDX_NEW_ALERT_CFG,

    ANS_IDX_SUPP_UNREAD_ALERT_CAT_CHAR,
    ANS_IDX_SUPP_UNREAD_ALERT_CAT_VAL,

    ANS_IDX_UNREAD_ALERT_STATUS_CHAR,
    ANS_IDX_UNREAD_ALERT_STATUS_VAL,
    ANS_IDX_UNREAD_ALERT_STATUS_CFG,

    ANS_IDX_ALERT_NTF_CTNL_PT_CHAR,
    ANS_IDX_ALERT_NTF_CTNL_PT_VAL,

    ANS_IDX_NB,
};


/*
 * MACROS
 * **************************************************************************************
 */

#define ANPS_IS_NEW_ALERT_CATEGORY_SUPPORTED(category_id) \
        (((anps_env->supp_new_alert_cat >> category_id) & 1) == 1)

#define ANPS_IS_UNREAD_ALERT_CATEGORY_SUPPORTED(category_id) \
        (((anps_env->supp_unread_alert_cat >> category_id) & 1) == 1)

#define ANPS_IS_ALERT_ENABLED(conidx, idx_env, alert_type) \
        (((idx_env->env[conidx]->ntf_cfg >> alert_type) & 1) == 1)

#define ANPS_IS_NEW_ALERT_CATEGORY_ENABLED(conidx, category_id, idx_env) \
        (((idx_env->env[conidx]->ntf_new_alert_cfg >> category_id) & 1) == 1)

#define ANPS_IS_UNREAD_ALERT_CATEGORY_ENABLED(conidx, category_id, idx_env) \
        (((idx_env->env[conidx]->ntf_unread_alert_cfg >> category_id) & 1) == 1)

#define ANPS_ENABLE_ALERT(conidx, idx_env, alert_type) \
        (idx_env->env[conidx]->ntf_cfg |= (1 << alert_type))

#define ANPS_DISABLE_ALERT(conidx, idx_env, alert_type) \
        (idx_env->env[conidx]->ntf_cfg &= ~(1 << alert_type))

#define ANPS_ENABLE_NEW_ALERT_CATEGORY(conidx, category_id, idx_env) \
        (idx_env->env[conidx]->ntf_new_alert_cfg |= (1 << category_id))

#define ANPS_ENABLE_UNREAD_ALERT_CATEGORY(conidx, category_id, idx_env) \
        (idx_env->env[conidx]->ntf_unread_alert_cfg |= (1 << category_id))

#define ANPS_DISABLE_NEW_ALERT_CATEGORY(conidx, category_id, idx_env) \
        (idx_env->env[conidx]->ntf_new_alert_cfg &= ~(1 << category_id))

#define ANPS_DISABLE_UNREAD_ALERT_CATEGORY(conidx, category_id, idx_env) \
        (idx_env->env[conidx]->ntf_unread_alert_cfg &= ~(1 << category_id))

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Alert Notification Profile Server Connection Dependent Environment Variable
struct anps_cnx_env
{
    /**
     * Client Characteristic Configuration Status
     *   Bit 0 : New Alert Characteristic
     *   Bit 1 : Unread Alert Status Characteristic
     */
    uint8_t ntf_cfg;

    /**
     * Category Notification Configuration
     *   Bit 0 : Simple Alert
     *   Bit 1 : Email
     *   Bit 2 : News
     *   Bit 3 : Call
     *   Bit 4 : Missed Call
     *   Bit 5 : SMS/MMS
     *   Bit 6 : Voice Mail
     *   Bit 7 : Schedule
     *   Bit 8 : High Prioritized Alert
     *   Bit 9 : Instance Message
     */
    uint16_t ntf_new_alert_cfg;
    uint16_t ntf_unread_alert_cfg;
};

/// Alert Notification Profile Server. Environment variable
struct anps_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// ANS Start Handle
    uint16_t shdl;
    /// Environment variable pointer for each connection
    struct anps_cnx_env* env[BLE_CONNECTION_MAX];

    /// Current Operation Code
    uint8_t operation;
    /// Supported New Alert Category Characteristic Value
    uint16_t supp_new_alert_cat;
    /// Supported Unread Alert Category Characteristic Value
    uint16_t supp_unread_alert_cat;

    /// State of different task instances
    ke_state_t state[ANPS_IDX_MAX];

};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve ANP service profile interface
 *
 * @return ANP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* anps_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Initialization of the ANPS module.
 * This function performs all the initializations of the ANPS module.
 ****************************************************************************************
 */
//void anps_init(void);

/**
 ****************************************************************************************
 * @brief Send an ANPS_NTF_STATUS_UPDATE_IND message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_ANPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void anps_send_ntf_status_update_ind(uint8_t conidx, struct anps_env_tag *idx_env, uint8_t alert_type);

/**
 ****************************************************************************************
 * @brief Send an ANPS_NTF_IMMEDIATE_REQ_IND message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_ANPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void anps_send_ntf_immediate_req_ind(uint8_t conidx, struct anps_env_tag *idx_env, uint8_t alert_type,
                                     uint8_t category_id);

/**
 ****************************************************************************************
 * @brief Send an ANPS_CMP_EVT message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_ANPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void anps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id, uint8_t operation, uint8_t status);

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
void anps_task_init(struct ke_task_desc *task_desc);


#endif //(BLE_AN_SERVER)

/// @} ANPS

#endif //(ANPS_H_)
