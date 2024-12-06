#ifndef DISS_TASK_H_
#define DISS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup DISSTASK Task
 * @ingroup DISS
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
    ///Set the value of an attribute - Request
    DISS_SET_VALUE_REQ = TASK_FIRST_MSG(TASK_ID_DISS),
    ///Set the value of an attribute - Response
    DISS_SET_VALUE_RSP,
    
    /// Peer device request to get profile attribute value
    DISS_VALUE_REQ_IND,
    /// Peer device confirm value of requested attribute
    DISS_VALUE_CFM,
};

///Attribute Table Indexes
enum diss_info
{
    /// Manufacturer Name
    DIS_MANUFACTURER_NAME_CHAR,
    /// Model Number
    DIS_MODEL_NB_STR_CHAR,
    /// Serial Number
    DIS_SERIAL_NB_STR_CHAR,
    /// HW Revision Number
    DIS_HARD_REV_STR_CHAR,
    /// FW Revision Number
    DIS_FIRM_REV_STR_CHAR,
    /// SW Revision Number
    DIS_SW_REV_STR_CHAR,
    /// System Identifier Name
    DIS_SYSTEM_ID_CHAR,
    /// IEEE Certificate
    DIS_IEEE_CHAR,
    /// Plug and Play Identifier
    DIS_PNP_ID_CHAR,

    DIS_CHAR_MAX,
};

///Database Configuration Flags
enum diss_features
{
    ///Indicate if Manufacturer Name String Char. is supported
    DIS_MANUFACTURER_NAME_CHAR_SUP       = 0x0001,
    ///Indicate if Model Number String Char. is supported
    DIS_MODEL_NB_STR_CHAR_SUP            = 0x0002,
    ///Indicate if Serial Number String Char. is supported
    DIS_SERIAL_NB_STR_CHAR_SUP           = 0x0004,
    ///Indicate if Hardware Revision String Char. supports indications
    DIS_HARD_REV_STR_CHAR_SUP            = 0x0008,
    ///Indicate if Firmware Revision String Char. is writable
    DIS_FIRM_REV_STR_CHAR_SUP            = 0x0010,
    ///Indicate if Software Revision String Char. is writable
    DIS_SW_REV_STR_CHAR_SUP              = 0x0020,
    ///Indicate if System ID Char. is writable
    DIS_SYSTEM_ID_CHAR_SUP               = 0x0040,
    ///Indicate if IEEE Char. is writable
    DIS_IEEE_CHAR_SUP                    = 0x0080,
    ///Indicate if PnP ID Char. is writable
    DIS_PNP_ID_CHAR_SUP                  = 0x0100,

    ///All features are supported
    DIS_ALL_FEAT_SUP                     = 0x01FF,
};
/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters for the database creation
struct diss_db_cfg
{
    /// Database configuration @see enum diss_features
    uint16_t features;
};


///Set the value of an attribute - Request
struct diss_set_value_req
{
    /// Value to Set
    uint8_t value;
    /// Value length
    uint8_t length;
    /// Value data
    uint8_t data[__ARRAY_EMPTY];
};

///Set the value of an attribute - Response
struct diss_set_value_rsp
{
    /// Value Set
    uint8_t value;
    /// status of the request
    uint8_t status;
};

/// Peer device request to get profile attribute value
struct diss_value_req_ind
{
    /// Requested value
    uint8_t value;
};

/// Peer device  value of requested attribute
struct diss_value_cfm
{
    /// Requested value
    uint8_t value;
    /// Value length
    uint8_t length;
    /// Value data
    uint8_t data[__ARRAY_EMPTY];
};

/// @} DISSTASK
#endif // DISS_TASK_H_
