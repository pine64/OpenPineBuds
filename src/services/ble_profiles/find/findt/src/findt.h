#ifndef FINDT_H_
#define FINDT_H_

/**
 ****************************************************************************************
 * @addtogroup FINDT Find Me Target
 * @ingroup FIND
 * @brief Find Me Profile Target.
 *
 * In the Find Me BLE Profile, the device that is in Target role will act as GATT server.
 *
 * The Target will react to alert levels written by the Locator in the Alert Level
 * Characteristic of the Immediate Alert Service(IAS) present in the ATT DB of the device.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Indicate that the alert level has been written by the Locator. (to APP)
 *
 * There shall be only one IAS instance on the device. There shall be only one
 * Alert Level Characteristic in the IAS. The characteristic properties shall be
 * Write No Response only.
 *
 *  The enable/disable of the profile and devices disconnection is handled in the application,
 *  depending on its User Input.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "find_common.h"
#if (BLE_FINDME_TARGET)
#include <stdint.h>
#include <stdbool.h>
#include "gap.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of instances of the Find Me Target task
#define FINDT_IDX_MAX     0x01

#define FINDT_MANDATORY_MASK        (0x07)

/// Possible states of the FINDT task
enum findt_state
{
    /// Idle state
    FINDT_IDLE,

    /// Number of defined states.
    FINDT_STATE_MAX
};

/// Find Me Target environment variable
struct findt_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    ///IAS Start Handle
    uint16_t shdl;
    /// State of different task instances
    ke_state_t state[FINDT_IDX_MAX];
};

/*
 * MACROS
 ****************************************************************************************
 */

/// Get database attribute handle
#define FINDT_HANDLE(idx) \
    (findt_env->start_hdl + (idx) )

/// Get database attribute index
#define FINDT_IDX(hdl) \
    ( (hdl) - findt_env->shdl )


/*
 * ENUMS
 ****************************************************************************************
 */

///FINDT Attributes database handle list
enum findt_att_db_handles
{
    FINDT_IAS_IDX_SVC,

    FINDT_IAS_IDX_ALERT_LVL_CHAR,
    FINDT_IAS_IDX_ALERT_LVL_VAL,

    FINDT_IAS_IDX_NB,
};

/// Alert levels
enum
{
    /// No alert
    FINDT_ALERT_NONE    = 0x00,
    /// Mild alert
    FINDT_ALERT_MILD,
    /// High alert
    FINDT_ALERT_HIGH
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve IAS service profile interface
 *
 * @return IAS service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* findt_prf_itf_get(void);


/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void findt_task_init(struct ke_task_desc *task_desc);
#endif //BLE_FINDME_TARGET

/// @} FINDT

#endif // FINDT_H_
