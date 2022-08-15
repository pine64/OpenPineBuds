#ifndef PROXR_H_
#define PROXR_H_

/**
 ****************************************************************************************
 * @addtogroup PROXR Proximity Reporter
 * @ingroup PROX
 * @brief Proximity Profile Reporter.
 *
 * The PROXR is responsible for providing proximity reporter functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of a proximity reporter role.
 *
 * Proximity Reporter (PR): A PR (e.g. a watch, key fob, etc) is the term used by
 * this profile to describe a personal device that a user carries with them and that
 * has low power requirement (i.e. it is operating with a button cell), allowing another
 * device to monitor their connection. The device may have a simple user alert
 * functionality, for example a blinking LED or audible output.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_PROX_REPORTER)

#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Proximity task instances
#define PROXR_IDX_MAX    (1)

#define PROXR_IAS_MANDATORY_MASK        (0x07)
#define PROXR_LLS_MANDATORY_MASK        (0x07)
#define PROXR_TXP_MANDATORY_MASK        (0x07)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the PROXR task
enum
{
    /// Idle state
    PROXR_IDLE,
    /// Proximity reporter in a busy state
    PROXR_BUSY,

    /// Number of defined states.
    PROXR_STATE_MAX
};

/// LLS Handles offsets
enum
{
    LLS_IDX_SVC,

    LLS_IDX_ALERT_LVL_CHAR,
    LLS_IDX_ALERT_LVL_VAL,

    LLS_IDX_NB,
};

/// IAS Handles offsets
enum
{
    IAS_IDX_SVC,

    IAS_IDX_ALERT_LVL_CHAR,
    IAS_IDX_ALERT_LVL_VAL,

    IAS_IDX_NB,
};

/// TXPS Handles offsets
enum
{
    TXPS_IDX_SVC,

    TXPS_IDX_TX_POWER_LVL_CHAR,
    TXPS_IDX_TX_POWER_LVL_VAL,

    TXPS_IDX_NB,
};


///Alert Level Values
enum
{
    PROXR_ALERT_NONE,
    PROXR_ALERT_MILD,
    PROXR_ALERT_HIGH,
};


/// Proximity service processing flags
enum
{
    /// Use To know if bond data are present
    PROXR_BOND_DATA_PRESENT,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Proximity reporter environment variable
struct proxr_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// IAS Start Handle
    uint16_t ias_start_hdl;
    /// LLS Start Handle
    uint16_t lls_start_hdl;
    /// TXP Start Handle
    uint16_t txp_start_hdl;
    /// Proximity Feature
    uint16_t features;
    /// Connection index backup, needed due to GAP request for TX power read
    uint8_t conidx_saved;
    /// State of different task instances
    ke_state_t state[PROXR_IDX_MAX];
    /// LLS alert level
    uint8_t lls_alert_lvl[PROXR_IDX_MAX];
};

/*
 * PROXIMITY PROFILE ATTRIBUTES DECLARATION
 ****************************************************************************************
 */

/*
 *  PROXIMITY PROFILE ATTRIBUTES VALUES DECLARATION
 ****************************************************************************************
 */

/// Link Loss Service
extern const att_svc_desc_t proxr_lls_svc;
/// Link Loss Service - Alert Level Characteristic
extern const struct att_char_desc proxr_lls_alert_lvl_char;

/// Immediate Alert Service
extern const att_svc_desc_t proxr_ias_svc;
/// Immediate Alert Service - Alert Level Characteristic
extern const struct att_char_desc proxr_ias_alert_lvl_char;

/// TX Power Service
extern const att_svc_desc_t proxr_txps_svc;
/// TX Power Service -TX Power Level Characteristic
extern const struct att_char_desc proxr_txps_tx_power_lvl_char;

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
 * @brief Retrieve Proximity service profile interface
 *
 * @return Proximity service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* proxr_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Disable role.
 ****************************************************************************************
 */
void proxr_disable(uint16_t conhdl); 

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
void proxr_task_init(struct ke_task_desc *task_desc);


#endif //BLE_PROX_REPORTER

/// @} PROXR

#endif // PROXR_H_
