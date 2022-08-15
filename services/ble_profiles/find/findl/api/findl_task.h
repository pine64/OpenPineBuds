#ifndef FINDL_TASK_H_
#define FINDL_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup FINDLTASK Find Me Locator Task
 * @ingroup FINDL
 * @brief Find Me Locator Task
 *
 * The FINDLTASK is responsible for handling the API messages coming from the application
 * or internal tasks.
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
#include "find_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */


///Find Me Locator API messages
enum findl_msg_id
{
    /// Start the find me locator profile - at connection
    FINDL_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_FINDL),
    /// Enable confirm message, containing IAS attribute details if discovery connection type
    FINDL_ENABLE_RSP,

    /// Alert level set request
    FINDL_SET_ALERT_REQ,
    /// Alert level set response
    FINDL_SET_ALERT_RSP,
};

/// Alert levels
enum
{
    /// No alert
    FINDL_ALERT_NONE    = 0x00,
    /// Mild alert
    FINDL_ALERT_MILD,
    /// High alert
    FINDL_ALERT_HIGH
};


/// Immediate Alert service details container
struct ias_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    /// - Alert Level
    struct prf_char_inf alert_lvl_char;
};

/// Parameters of the @ref FINDL_ENABLE_REQ message
struct findl_enable_req
{
    ///Connection type
    uint8_t con_type;
    ///Discovered IAS details if any
    struct ias_content ias;
};

/// Parameters of the @ref FINDL_ENABLE_RSP message
struct findl_enable_rsp
{
    ///Status
    uint8_t status;
    ///Discovered IAS details if any
    struct ias_content ias;
};

/// Parameters of the @ref FINDL_SET_ALERT_REQ message
struct findl_set_alert_req
{
    /// Alert level
    uint8_t  alert_lvl;
};

///Parameters of the @ref FINDL_SET_ALERT_RSP message
struct findl_set_alert_rsp
{
    /// Status
    uint8_t  status;
};

/// @} FINDLTASK

#endif // FINDL_TASK_H_
