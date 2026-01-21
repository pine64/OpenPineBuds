#ifndef _PASPC_TASK_H_
#define _PASPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup PASPCTASK Phone Alert Status Profile Client Task
 * @ingroup PASPC
 * @brief Phone Alert Status Profile Client Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "pasp_common.h"
#include "rwip_task.h" // Task definitions
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Message IDs
enum paspc_msg_ids
{
    /// Enable the Phone Alert Status Profile Client task - at connection
    PASPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_PASPC),
    /// Enable the Phone Alert Status Profile Client task - at connection
    PASPC_ENABLE_RSP,

    /// Read the value of an attribute in the peer device database
    PASPC_READ_CMD,
    /// Write the value of an attribute in the peer device database
    PASPC_WRITE_CMD,

    /// Indicate that an attribute value has been received either upon notification or read response
    PASPC_VALUE_IND,

    /// Complete Event Information
    PASPC_CMP_EVT,
};

/// Operation Codes
enum paspc_op_codes
{
    /// Reserved operation code
    PASPC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    PASPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    PASPC_READ_OP_CODE,
    /// Write attribute value Procedure
    PASPC_WRITE_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Phone Alert Status Service Characteristics
enum
{
    /// Alert Status
    PASPC_CHAR_ALERT_STATUS,
    /// Ringer Setting
    PASPC_CHAR_RINGER_SETTING,
    /// Ringer Control Point
    PASPC_CHAR_RINGER_CTNL_PT,

    PASPC_CHAR_MAX,
};

/// Phone Alert Status Service Characteristic Descriptors
enum
{
    /// Alert Status Char. - Client Characteristic Configuration
    PASPC_DESC_ALERT_STATUS_CL_CFG,
    /// Ringer Setting Char. - Client Characteristic Configuration
    PASPC_DESC_RINGER_SETTING_CL_CFG,

    PASPC_DESC_MAX,

    PASPC_DESC_MASK = 0x10,
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Phone Alert Status Service
 */
struct paspc_pass_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Alert Status
    ///  - Ringer Setting
    ///  - Ringer Control Point
    struct prf_char_inf chars[PASPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Alert Status Client Cfg
    ///  - Ringer Setting Client Cfg
    struct prf_char_desc_inf descs[PASPC_DESC_MAX];
};


/// Parameters of the @ref PASPC_ENABLE_REQ message
struct paspc_enable_req
{
    /// Connection type
    uint8_t con_type;
    /// Existing handle values PASS
    struct paspc_pass_content pass;
};

/// Parameters of the @ref PASPC_ENABLE_RSP message
struct paspc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values PASS
    struct paspc_pass_content pass;
};

/// Parameters of the @ref PASPC_READ_CMD message
struct paspc_read_cmd
{
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref PASPC_WRITE_CMD message
struct paspc_write_cmd
{
    /// Write code
    uint8_t write_code;
    /// Value
    union write_value_tag
    {
        /// Ringer Control Point characteristic value
        uint8_t ringer_ctnl_pt;
        /// Alert Status characteristic client characteristic configuration value
        uint16_t alert_status_ntf_cfg;
        /// Ringer Setting characteristic client characteristic configuration value
        uint16_t ringer_setting_ntf_cfg;
    } value;
};

/// Parameters of the @ref PASPC_VALUE_IND message
struct paspc_value_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union value_tag
    {
        /// Alert Status characteristic value
        uint8_t alert_status;
        /// Ringer Setting characteristic value
        uint8_t ringer_setting;
        /// Alert Status characteristic client characteristic configuration value
        uint16_t alert_status_ntf_cfg;
        /// Ringer Setting characteristic client characteristic configuration value
        uint16_t ringer_setting_ntf_cfg;
    } value;
};

/// Parameters of the @ref PASPC_CMP_EVT message
struct paspc_cmp_evt
{
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};


/// @} PASPCTASK

#endif //(_PASPC_TASK_H_)
