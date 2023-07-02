#ifndef _ANPC_TASK_H_
#define _ANPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup ANPCTASK Alert Notification Profile Client Task
 * @ingroup ANPC
 * @brief Alert Notification Profile Client Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_task.h" // Task definitions
#include "prf_types.h"

#include "anp_common.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Message IDs
enum anpc_msg_ids
{
    /// Enable the Alert Notification Profile Client task - at connection
    ANPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_ANPC),
    /// Enable the Alert Notification Profile Client task - at connection
    ANPC_ENABLE_RSP,

    /// Read the value of an attribute in the peer device database
    ANPC_READ_CMD,
    /// Write the value of an attribute in the peer device database
    ANPC_WRITE_CMD,

    /// Indicate that an attribute value has been received either upon notification or read response
    ANPC_VALUE_IND,

    /// Complete Event Information
    ANPC_CMP_EVT,
};

/// Operation Codes
enum anpc_op_codes
{
    /// Reserved operation code
    ANPC_RESERVED_OP_CODE  = 0x00,

    /**
     * EXTERNAL OPERATION CODES
     */

    /// Discovery Procedure
    ANPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    ANPC_READ_OP_CODE,
    /// Write attribute value Procedure
    ANPC_WRITE_OP_CODE,

    /**
     * INTERNAL OPERATION CODES
     */

    /// Discovery Procedure - Read Supported New Alert Category
    ANPC_ENABLE_RD_NEW_ALERT_OP_CODE,
    /// Discovery Procedure - Read Supported Unread Alert Category
    ANPC_ENABLE_RD_UNREAD_ALERT_OP_CODE,
    /// Discovery Procedure - Enable New Alert Categories for Notifications
    ANPC_ENABLE_WR_NEW_ALERT_OP_CODE,
    /// Discovery Procedure - Ask to be notified immediately for New Alert Values
    ANPC_ENABLE_WR_NTF_NEW_ALERT_OP_CODE,
    /// Discovery Procedure - Enable Unread Alert Categories for Notifications
    ANPC_ENABLE_WR_UNREAD_ALERT_OP_CODE,
    /// Discovery Procedure - Ask to be notified immediately for Unread Alert Values
    ANPC_ENABLE_WR_NTF_UNREAD_ALERT_OP_CODE,
};

/// Alert Notification Service Characteristics
enum anpc_ans_chars
{
    /// Supported New Alert Category
    ANPC_CHAR_SUP_NEW_ALERT_CAT,
    /// New Alert
    ANPC_CHAR_NEW_ALERT,
    /// Supported Unread Alert Category
    ANPC_CHAR_SUP_UNREAD_ALERT_CAT,
    /// Unread Alert Status
    ANPC_CHAR_UNREAD_ALERT_STATUS,
    /// Alert Notification Control Point
    ANPC_CHAR_ALERT_NTF_CTNL_PT,

    ANPC_CHAR_MAX,
};

/// Alert Notification Service Characteristic Descriptors
enum anpc_ans_descs
{
    /// New Alert Char. - Client Characteristic Configuration
    ANPC_DESC_NEW_ALERT_CL_CFG,
    /// Unread Alert Status Char. - Client Characteristic Configuration
    ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG,

    ANPC_DESC_MAX,

    ANPC_DESC_MASK = 0x10,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */
/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Alert Notification Service
 */
struct anpc_ans_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Supported New Alert Category
    ///  - New Alert
    ///  - Supported Unread Alert Category
    ///  - Unread Alert Status
    ///  - Alert Notification Control Point
    struct prf_char_inf chars[ANPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - New Alert Client Cfg
    ///  - Unread Alert Status Client Cfg
    struct prf_char_desc_inf descs[ANPC_DESC_MAX];
};

/// Parameters of the @ref ANPC_ENABLE_REQ message
struct anpc_enable_req
{
    /// Connection type
    uint8_t con_type;

    /// New Alert Category to Enable for Notifications
    struct anp_cat_id_bit_mask new_alert_enable;
    /// Unread Alert Category to Enable for Notifications
    struct anp_cat_id_bit_mask unread_alert_enable;

    /// Existing handle values ANS
    struct anpc_ans_content ans;
};

/// Parameters of the @ref ANPC_ENABLE_RSP message
struct anpc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values ANS
    struct anpc_ans_content ans;
};

/// Parameters of the @ref ANPC_READ_CMD message
struct anpc_read_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref ANPC_WRITE_CMD message
struct anpc_write_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Write code
    uint8_t write_code;
    /// Value
    union anpc_write_value_tag
    {
        /// Alert Notification Control Point
        struct anp_ctnl_pt ctnl_pt;
        /// New Alert Notification Configuration
        uint16_t new_alert_ntf_cfg;
        /// Unread Alert Status Notification Configuration
        uint16_t unread_alert_status_ntf_cfg;
    } value;
};

/// Parameters of the @ref ANPC_VALUE_IND message
struct anpc_value_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union anpc_value_tag
    {
        /// List of supported categories
        struct anp_cat_id_bit_mask supp_cat;
        /// New Alert
        struct anp_new_alert new_alert;
        /// Unread Alert
        struct anp_unread_alert unread_alert;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
    } value;
};

/// Parameters of the @ref ANPC_CMP_EVT message
struct anpc_cmp_evt
{
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};

/// @} ANPCTASK

#endif //(_ANPC_TASK_H_)
