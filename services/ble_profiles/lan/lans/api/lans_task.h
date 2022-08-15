#ifndef _LANS_TASK_H_
#define _LANS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup LANSTASK Task
 * @ingroup LANS
 * @brief Location and Navigation Profile Task.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "lan_common.h"
#include "rwip_task.h" // Task definitions

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Messages for Location and Navigation Profile Sensor
enum lans_msg_id
{
    /// Start the Location and Navigation Profile Server profile
    LANS_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_LANS),
    /// Confirm profile
    LANS_ENABLE_RSP,

    /// Update Position Quality
    LANS_UPD_POS_Q_REQ,
    /// Update Position Quality
    LANS_UPD_POS_Q_RSP,

    /// Send a Location and Speed to the peer device (Notification)
    LANS_NTF_LOC_SPEED_REQ,
    /// Response of Send a Location and Speed request
    LANS_NTF_LOC_SPEED_RSP,

    /// Send a LN Vector to the peer device (Notification)
    LANS_NTF_NAVIGATION_REQ,
    /// Response of Send LN Vector request
    LANS_NTF_NAVIGATION_RSP,

    /// Send a complete event status to the application
    LANS_CMP_EVT,

    /// Indicate that an attribute value has been written
    LANS_CFG_NTFIND_IND,

    /// Indicate that Control Point characteristic value has been written
    LANS_LN_CTNL_PT_REQ_IND,
    /// Application response after receiving a LANS_CTNL_PT_REQ_IND message
    LANS_LN_CTNL_PT_CFM,
};

/// Operation Code used in the profile state machine
enum lans_op_code
{
    /// Reserved Operation Code
    LANS_RESERVED_OP_CODE          = 0x00,

    /// Enable Profile Operation Code
    LANS_ENABLE_REQ_OP_CODE,

    /// Send Location and Speed Operation Code
    LANS_NTF_LOC_SPEED_OP_CODE,

    /// Update Position quality
    LANS_UPD_POS_Q_OP_CODE,

    /// Send Navigation Operation Code
    LANS_NTF_NAVIGATION_OP_CODE,

    /**
     * Control Point Operation
     */
    /// Set Cumulative Value
    LANS_SET_CUMUL_VALUE_OP_CODE,
    /// Mask Location and Speed Characteristic Content
    LANS_MASK_LSPEED_CHAR_CT_OP_CODE,
    /// Navigation Control
    LANS_NAVIGATION_CONTROL_OP_CODE,
    /// Request Number of Routes
    LANS_REQ_NUMBER_OF_ROUTES_OP_CODE,
    /// Request Name of Route
    LANS_REQ_NAME_OF_ROUTE_OP_CODE,
    /// Select Route
    LANS_SELECT_ROUTE_OP_CODE,
    /// Set Fix Rate
    LANS_SET_FIX_RATE_OP_CODE,
    /// Set Elevation
    LANS_SET_ELEVATION_OP_CODE,

    /// Error Indication Sent Operation Code
    LANS_LN_CTNL_ERR_IND_OP_CODE,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref LANS_CREATE_DB_REQ message
struct lans_db_cfg
{
    /**
     * LN Feature Value - Not supposed to be modified during the lifetime of the device
     * This bit field is set in order to decide which features are supported:
     */
    uint32_t ln_feature;

    /**
     * Bit 0 - Enable LN Control Point
     * Bit 1 - Enable Navigation Feature
     */
    uint8_t prfl_config;
};

/// Parameters of the @ref LANS_ENABLE_REQ message
struct lans_enable_req
{
    /// Connection index
    uint8_t conidx;
    /**
     * Profile characteristic configuration:
     *   Location and speed Characteristic NTF config -- Bit 0
     *   LN Control Point Characteristic IND config ---- Bit 1
     *   Navigation Characteristic NTF config ---------- Bit 2
     */
    uint16_t prfl_ntf_ind_cfg;
};

/// Parameters of the @ref LANS_ENABLE_RSP message
struct lans_enable_rsp
{
    /// Connection index
    uint8_t conidx;
    /// status
    uint8_t status;
};

/// Parameters of the @ref LANS_CMP_EVT message
struct lans_cmp_evt
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t operation;
    /// Operation Status
    uint8_t status;
};

/// Parameters of the @ref LANS_CFG_NTFIND_IND message
struct lans_cfg_ntfind_ind
{
    /// Connection index
    uint8_t conidx;
    /// Characteristic Code (Location and speed, Control Point or navigation)
    uint8_t char_code;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref LANS_NTF_LOC_SPEED_REQ message
struct  lans_ntf_loc_speed_req
{
    ///Parameters
    struct lanp_loc_speed parameters;
};

/// Parameters of the @ref LANS_NTF_LOC_SPEED_RSP message
struct  lans_ntf_loc_speed_rsp
{
    /// Operation Status
    uint8_t status;
};

/// Parameters of the @ref LANS_NTF_NAVIGATION_REQ message
struct  lans_ntf_navigation_req
{
    ///Parameters
    struct lanp_navigation parameters;
};

/// Parameters of the @ref LANS_NTF_NAVIGATION_RSP message
struct  lans_ntf_navigation_rsp
{
    /// Operation Status
    uint8_t status;
};

/// Parameters of the @ref LANS_UPD_POS_Q_REQ message
struct  lans_upd_pos_q_req
{
    ///Parameters
    struct lanp_posq parameters;
};

/// Parameters of the @ref LANS_UPD_POS_Q_RSP message
struct  lans_upd_pos_q_rsp
{
    /// Operation Status
    uint8_t status;
};

/// Parameters of the @ref LANS_LN_CTNL_PT_REQ_IND message
struct lans_ln_ctnl_pt_req_ind
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t op_code;
    /// Value
    union lans_ln_ctnl_pt_req_ind_value
    {
        /// Cumulative Value (24 bits)
        uint32_t cumul_val;
        /// Mask Content
        uint16_t mask_content;
        /// Navigation Control
        uint8_t control_value;
        /// Route number
        uint16_t route_number;
        /// Fix rate
        uint8_t fix_rate;
        /// Elevation
        int32_t elevation;
    } value;
};

/// Parameters of the @ref LANS_CTNL_PT_CFM message
struct lans_ln_ctnl_pt_cfm
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t op_code;
    /// Status
    uint8_t status;
    /// Value
    union lans_ln_ctnl_pt_cfm_value
    {
        /// Number of routes
        uint16_t number_of_routes;
        /// Mask content
        uint16_t mask_lspeed_content;
        struct lan_route_name
        {
            uint8_t length;
            /// Name of Route UTF-8
            uint8_t name[__ARRAY_EMPTY];
        } route;
    } value;
};

/// @} LANSTASK

#endif //(_LANS_TASK_H_)
