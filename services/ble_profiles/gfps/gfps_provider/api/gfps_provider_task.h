#ifndef GFPSP_TASK_H_
#define GFPSP_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup GFPSPTASK Task
 * @ingroup GFPSP
 * @brief Device Information Service Server Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "rwip_task.h" // Task definitions
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */



/// Messages for Device Information Service Server
enum
{
    //port msg use to reference ..
    ///Set the value of an attribute - Request
    GFPSP_SET_VALUE_REQ = TASK_FIRST_MSG(TASK_ID_GFPSP),
    ///Set the value of an attribute - Response
    GFPSP_SET_VALUE_RSP,    
    /// Peer device request to get profile attribute value
    GFPSP_VALUE_REQ_IND,
    /// Peer device confirm value of requested attribute
    GFPSP_VALUE_CFM,
    
    GFPSP_KEY_BASED_PAIRING_NTF_CFG,    
    GFPSP_KEY_BASED_PAIRING_WRITE_IND,
    GFPSP_KEY_BASED_PAIRING_WRITE_NOTIFY,

    GFPSP_KEY_PASS_KEY_NTF_CFG,
    GFPSP_KEY_PASS_KEY_WRITE_IND,
    GFPSP_KEY_PASS_KEY_WRITE_NOTIFY,

    GFPSP_KEY_ACCOUNT_KEY_WRITE_IND,
    GFPSP_NAME_WRITE_IND,
    GFPSP_NAME_NOTIFY,

    GFPSP_SEND_WRITE_RESPONSE,
    
};

///Attribute Table Indexes
enum gfpsp_info
{
    /// Manufacturer Name
    GFPSP_MANUFACTURER_NAME_CHAR,
    /// Model Number
    GFPSP_MODEL_NB_STR_CHAR,
    /// Serial Number
    GFPSP_SERIAL_NB_STR_CHAR,
    /// HW Revision Number
    GFPSP_HARD_REV_STR_CHAR,
    /// FW Revision Number
    GFPSP_FIRM_REV_STR_CHAR,
    /// SW Revision Number
    GFPSP_SW_REV_STR_CHAR,
    /// System Identifier Name
    GFPSP_SYSTEM_ID_CHAR,
    /// IEEE Certificate
    GFPSP_IEEE_CHAR,
    /// Plug and Play Identifier
    GFPSP_PNP_ID_CHAR,

    GFPSP_CHAR_MAX,
};

///Database Configuration Flags
enum gfpsp_features
{
    ///Indicate if Manufacturer Name String Char. is supported
    GFPSP_MANUFACTURER_NAME_CHAR_SUP       = 0x0001,
    ///Indicate if Model Number String Char. is supported
    GFPSP_MODEL_NB_STR_CHAR_SUP            = 0x0002,
    ///Indicate if Serial Number String Char. is supported
    GFPSP_SERIAL_NB_STR_CHAR_SUP           = 0x0004,
    ///Indicate if Hardware Revision String Char. supports indications
    GFPSP_HARD_REV_STR_CHAR_SUP            = 0x0008,
    ///Indicate if Firmware Revision String Char. is writable
    GFPSP_FIRM_REV_STR_CHAR_SUP            = 0x0010,
    ///Indicate if Software Revision String Char. is writable
    GFPSP_SW_REV_STR_CHAR_SUP              = 0x0020,
    ///Indicate if System ID Char. is writable
    GFPSP_SYSTEM_ID_CHAR_SUP               = 0x0040,
    ///Indicate if IEEE Char. is writable
    GFPSP_IEEE_CHAR_SUP                    = 0x0080,
    ///Indicate if PnP ID Char. is writable
    GFPSP_PNP_ID_CHAR_SUP                  = 0x0100,

    ///All features are supported
    GFPSP_ALL_FEAT_SUP                     = 0x01FF,
};
/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters for the database creation
struct gfpsp_db_cfg
{
    /// Database configuration @see enum gfpsp_features
    uint16_t features;
};


///Set the value of an attribute - Request
struct gfpsp_set_value_req
{
    /// Value to Set
    uint8_t value;
    /// Value length
    uint8_t length;
    /// Value data
    uint8_t data[__ARRAY_EMPTY];
};

///Set the value of an attribute - Response
struct gfpsp_set_value_rsp
{
    /// Value Set
    uint8_t value;
    /// status of the request
    uint8_t status;
};

/// Peer device request to get profile attribute value
struct gfpsp_value_req_ind
{
    /// Requested value
    uint8_t value;
};

/// Peer device  value of requested attribute
struct gfpsp_value_cfm
{
    /// Requested value
    uint8_t value;
    /// Value length
    uint8_t length;
    /// Value data
    uint8_t data[__ARRAY_EMPTY];
};

struct gfpsp_send_write_rsp_t
{
    uint16_t src_task_id;
    uint16_t dst_task_id;
    uint16_t handle;
    uint16_t status;
};

struct gfpsp_write_ind_t
{
    struct gfpsp_send_write_rsp_t pendingWriteRsp;
    uint16_t    length;
    uint8_t     data[0];
};
struct gfpsp_send_data_req_t
{
    uint8_t     connecionIndex;
    uint32_t    length;
    uint8_t     value[__ARRAY_EMPTY];
};

struct app_gfps_key_based_notif_config_t
{
    bool    isNotificationEnabled;
};

struct app_gfps_pass_key_notif_config_t
{
    bool    isNotificationEnabled;
};

/// @} GFPSPTASK
#endif // GFPSP_TASK_H_
