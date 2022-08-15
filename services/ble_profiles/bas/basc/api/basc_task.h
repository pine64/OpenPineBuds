#ifndef _BASC_TASK_H_
#define _BASC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BASCTASK Battery Service Client Task
 * @ingroup BASC
 * @brief Battery Service Client Task
 *
 * The BASCTASK is responsible for handling the messages coming in and out of the
 * @ref BASC block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_task.h" // Task definitions
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */


///Maximum number of Battery Service instances we can handle
#define BASC_NB_BAS_INSTANCES_MAX         (2)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


enum basc_msg_id
{
    /// Start the Battery Service Client Role - at connection
    BASC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_BASC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    BASC_ENABLE_RSP,

    /// Read Characteristic Value Request
    BASC_READ_INFO_REQ,
    /// Read Characteristic Value Request
    BASC_READ_INFO_RSP,

    /// Write Battery Level Notification Configuration Value request
    BASC_BATT_LEVEL_NTF_CFG_REQ,
    /// Write Battery Level Notification Configuration Value response
    BASC_BATT_LEVEL_NTF_CFG_RSP,

    /// Indicate to APP that the Battery Level value has been received
    BASC_BATT_LEVEL_IND,
};

/// Peer battery info that can be read
enum basc_info
{
    /// Battery Level value
    BASC_BATT_LVL_VAL,
    /// Battery Level Client Characteristic Configuration
    BASC_NTF_CFG,
    /// Battery Level Characteristic Presentation Format
    BASC_BATT_LVL_PRES_FORMAT,

    BASC_INFO_MAX,
};


/// Battery Service Characteristics
enum bass_char_type
{
    /// Battery Level
    BAS_CHAR_BATT_LEVEL,

    BAS_CHAR_MAX,
};

/// Battery Service Descriptors
enum bass_desc_type
{
    /// Battery Level Characteristic Presentation Format
    BAS_DESC_BATT_LEVEL_PRES_FORMAT,
    /// Battery Level Client Characteristic Configuration
    BAS_DESC_BATT_LEVEL_CFG,

    BAS_DESC_MAX,
};

/*
 * APIs Structure
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct bas_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic Info:
    /// - Battery Level
    struct prf_char_inf chars[BAS_CHAR_MAX];

    /// Descriptor handles:
    /// - Battery Level Client Characteristic Configuration
    /// - Battery Level Characteristic Presentation Format
    struct prf_char_desc_inf descs[BAS_DESC_MAX];
};


/// Parameters of the @ref BASC_ENABLE_REQ message
struct basc_enable_req
{
    ///Connection type
    uint8_t con_type;

    /// Number of BAS instances that have previously been found
    uint8_t bas_nb;
    /// Existing handle values bas
    struct bas_content bas[BASC_NB_BAS_INSTANCES_MAX];
};

/// Parameters of the @ref BASC_ENABLE_RSP message
struct basc_enable_rsp
{
    /// Status
    uint8_t status;
    /// Number of BAS that have been found
    uint8_t bas_nb;
    ///Existing handle values bas
    struct bas_content bas[BASC_NB_BAS_INSTANCES_MAX];
};


///Parameters of the @ref BASC_READ_INFO_REQ message
struct basc_read_info_req
{
    ///Characteristic info @see enum basc_info
    uint8_t info;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
};

///Parameters of the @ref  BASC_READ_INFO_RSP message
struct basc_read_info_rsp
{
    /// status of the request
    uint8_t status;
    ///Characteristic info @see enum basc_info
    uint8_t info;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;

    /// Information data
    union basc_data
    {
        /// Battery Level - if info = BASC_BATT_LVL_VAL
        uint8_t batt_level;
        ///Notification Configuration Value - if info = BASC_NTF_CFG
        uint16_t ntf_cfg;
        ///Characteristic Presentation Format - if info = BASC_BATT_LVL_PRES_FORMAT
        struct prf_char_pres_fmt char_pres_format;
    } data;
};

///Parameters of the @ref BASC_BATT_LEVEL_NTF_CFG_REQ message
struct basc_batt_level_ntf_cfg_req
{
    ///Notification Configuration
    uint16_t ntf_cfg;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
};

///Parameters of the @ref BASC_BATT_LEVEL_NTF_CFG_RSP message
struct basc_batt_level_ntf_cfg_rsp
{
    ///Status
    uint8_t status;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
};

///Parameters of the @ref BASC_BATT_LEVEL_IND message
struct basc_batt_level_ind
{
    ///Battery Level
    uint8_t batt_level;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
};


/// @} BASCTASK

#endif /* _BASC_TASK_H_ */
