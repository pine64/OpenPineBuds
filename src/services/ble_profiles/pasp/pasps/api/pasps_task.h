#ifndef PASPS_TASK_H_
#define PASPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup PASPSTASK Task
 * @ingroup PASPS
 * @brief Phone Alert Status Profile Server Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "pasp_common.h"
#include "rwip_task.h" // Task definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Messages for Phone Alert Status Profile Server
enum pasps_msg_ids
{
    /// Set Bonded Data
    PASPS_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_PASPS),
    /// Bonded data confirmation
    PASPS_ENABLE_RSP,

    /// Update the value of a characteristic
    PASPS_UPDATE_CHAR_VAL_CMD,
    /// Indicate that an attribute value has been written
    PASPS_WRITTEN_CHAR_VAL_IND,

    /// Complete Event Information
    PASPS_CMP_EVT,
};

/// Operation Codes
enum pasps_op_codes
{
    PASPS_RESERVED_OP_CODE              = 0x00,

    /// Enable Procedure
    PASPS_ENABLE_OP_CODE,
    /// Update Alert Status Char. value
    PASPS_UPD_ALERT_STATUS_OP_CODE,
    /// Update Ringer Setting Char. value
    PASPS_UPD_RINGER_SETTING_OP_CODE,
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref PASPS_CREATE_DB_REQ message
struct pasps_db_cfg
{
    /// Alert Status Char. Value
    uint8_t alert_status;
    /// Ringer Settings Char. Value
    uint8_t ringer_setting;
};

/// Parameters of the @ref PASPS_ENABLE_REQ message
struct pasps_enable_req
{
    /// Alert Status Characteristic Notification Configuration
    uint16_t alert_status_ntf_cfg;
    /// Ringer Setting Characteristic Notification Configuration
    uint16_t ringer_setting_ntf_cfg;
};

/// Parameters of the @ref PASPS_ENABLE_RSP message
struct pasps_enable_rsp
{
    /// status
    uint8_t status;
};

///Parameters of the @ref PASPS_UPDATE_CHAR_VAL_CMD message
struct pasps_update_char_val_cmd
{
    /// Operation Code (PASPS_UPD_ALERT_STATUS_OP_CODE or PASPS_UPD_RINGER_SETTING_OP_CODE)
    uint8_t operation;
    /// Alert Status Characteristic Value or Ringer Setting Characteristic Value
    uint8_t value;
};

///Parameters of the @ref PASPS_WRITTEN_CHAR_VAL_IND message
struct pasps_written_char_val_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Written value
    union written_value_tag
    {
        /// Ringer Control Point Char. Value
        uint8_t ringer_ctnl_pt;
        /// Alert Status Notification Configuration
        uint16_t alert_status_ntf_cfg;
        /// Ringer Setting Notification Configuration
        uint16_t ringer_setting_ntf_cfg;
    } value;
};

///Parameters of the @ref PASPS_CMP_EVT message
struct pasps_cmp_evt
{
    /// Operation
    uint8_t operation;
    /// Status
    uint8_t status;
};

/// @} PASPSTASK

#endif // PASPS_TASK_H_
