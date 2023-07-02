#ifndef FINDT_TASK_H_
#define FINDT_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup FINDTTASK Find Me Target Task
 * @ingroup FINDT
 * @brief Find Me Target Task
 *
 * The FINDTTASK is responsible for handling the APi messages from the application or
 * internal tasks.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "rwip_task.h" // Task definitions
#include "find_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/// Messages for Find Me Target
enum findt_msg_id
{
    /// Alert indication
    FINDT_ALERT_IND = TASK_FIRST_MSG(TASK_ID_FINDT),
};


/// Parameters of the @ref FINDT_ALERT_IND message
struct findt_alert_ind
{
    /// Alert level
    uint8_t alert_lvl;
    /// Connection index
    uint8_t conidx;
};


/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */
/// Parameters for the database creation
struct findt_db_cfg
{
    /// Database configuration
    uint8_t dummy;
};


/// @} FINDTTASK
#endif // FINDT_TASK_H_
