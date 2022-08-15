#ifndef _LANC_TASK_H_
#define _LANC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup LANCTASK Location and Navigation Profile Collector Task
 * @ingroup LANC
 * @brief Location and Navigation Profile Collector Task
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

/// Location and Navigation Service Characteristic Descriptors
enum lanc_lns_descs
{
    /// Location and Speed Char. - Client Characteristic Configuration
    LANC_DESC_LOC_SPEED_CL_CFG,
    /// LN Control Point Char. - Client Characteristic Configuration
    LANC_DESC_LN_CTNL_PT_CL_CFG,
    /// Navigation Char. - Client Characteristic Configuration
    LANC_DESC_NAVIGATION_CL_CFG,

    LANC_DESC_MAX,

    LANC_DESC_MASK = 0x10,
};

/// Message IDs
enum lanc_msg_ids
{
    /// Enable the Location and Navigation Profile Collector task - at connection
    LANC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_LANC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    LANC_ENABLE_RSP,

    /// Read the value of an attribute in the peer device database
    LANC_READ_CMD,
    /// Configure sending of notification or indication
    LANC_CFG_NTFIND_CMD,

    /// Configure the SC Control Point value
    LANC_LN_CTNL_PT_CFG_REQ,
    /// Indicate that an control point response has been triggered by peer device
    LANC_LN_CTNL_PT_RSP,

    /// Indicate that an attribute value has been received either upon notification or read response
    LANC_VALUE_IND,

    /// Complete Event Information
    LANC_CMP_EVT,

    /// Procedure Timeout Timer
    LANC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum lanc_op_code
{
    /// Reserved operation code
    LANC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    LANC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    LANC_READ_OP_CODE,

    /// Wait for the Write Response after having written a Client Char. Cfg. Descriptor.
    LANC_CFG_NTF_IND_OP_CODE,

    /// Wait for the Write Response after having written the Control Point Char.
    LANC_LN_CTNL_PT_CFG_WR_OP_CODE,
    /// Wait for the Indication Response after having written the Control Point Char.
    LANC_LN_CTNL_PT_CFG_IND_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */
/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Location and Navigation Service
 */
struct lanc_lns_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - LN Feature
    ///  - Location and Speed
    ///  - Position quality
    ///  - LN Control Point
    ///  - Navigation
    struct prf_char_inf chars[LANP_LANS_CHAR_MAX];

    /// Descriptor handles:
    ///  - Location and Speed Client Cfg
    ///  - Control Point Client Cfg
    ///  - Navigation Client Cfg
    struct prf_char_desc_inf descs[LANC_DESC_MAX];
};

/// Parameters of the @ref LANC_ENABLE_REQ message
struct lanc_enable_req
{
    /// Connection type
    uint8_t con_type;
    /// Existing handle values LNS
    struct lanc_lns_content lans;
};

/// Parameters of the @ref LANC_ENABLE_RSP message
struct lanc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values LNS
    struct lanc_lns_content lns;
};

/// Parameters of the @ref LANC_READ_CMD message
struct lanc_read_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref LANC_VALUE_IND message
struct lanc_value_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union lanc_value_tag
    {
        /// LN Feature
        uint32_t LN_feat;
        /// Location and Speed
        struct lanp_loc_speed loc_speed;
        /// LAN Position quality
        struct lanp_posq pos_q;
        /// Navigation
        struct lanp_navigation navigation;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
    } value;
};

/// Parameters of the @ref LANC_CFG_NTFIND_CMD message
struct lanc_cfg_ntfind_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Descriptor code
    uint8_t desc_code;
    /// Ntf/Ind Configuration
    uint16_t ntfind_cfg;
};

/// Parameters of the @ref LANC_LN_CTNL_PT_CFG_REQ message
struct lanc_ln_ctnl_pt_cfg_req
{
    /// Operation Code
    uint8_t operation;
    /// SC Control Point Request
    struct lan_ln_ctnl_pt_req ln_ctnl_pt;
};

/// Parameters of the @ref LANC_LN_CTNL_PT_RSP message
struct lanc_ln_ctnl_pt_rsp
{
    /// SC Control Point Response
    struct lanp_ln_ctnl_pt_rsp rsp;
};

/// Parameters of the @ref LANC_CMP_EVT message
struct lanc_cmp_evt
{
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};



/// @} LANCTASK

#endif //(_LANC_TASK_H_)
