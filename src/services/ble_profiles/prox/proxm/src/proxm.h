#ifndef PROXM_H_
#define PROXM_H_

/**
 ****************************************************************************************
 * @addtogroup PROX Proximity Profile
 * @ingroup PROFILE
 * @brief Proximity Profile
 *
 * The PROX module is the responsible block for implementing the proximity profile
 * functionalities in the BLE Host.
 *
 * The Proximity Profile defines the functionality required in a device that can
 * alert the user when the user's personal device moves further away or closer together
 * to another communicating device.
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PROXM Proximity Monitor
 * @ingroup PROX
 * @brief Proximity Profile Monitor
 *
 * The PROXM is responsible for providing proximity profile monitor functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of a proximity monitor role.
 *
 * Proximity Monitor (PM): A PM (e.g. PC, phone, electronic door entry system, etc)
 * is the term used by this profile to describe a device that monitors the distance
 * between itself and the connected PR device. The profile on the PM device constantly
 * monitors the path loss between itself and the communicating Proximity Reporter
 * device. The profile provides indications to an application which can cause an alert
 * to the user.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_PROX_MONITOR)

#include "proxm_task.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Proximity Monitor task instances
#define PROXM_IDX_MAX    (BLE_CONNECTION_MAX)


/*
 * ENUMERATIONS
 ****************************************************************************************
 */
/// Possible states of the PROXM task
enum proxm_state
{
    /// FREE state
    PROXM_FREE,
    /// IDLE state
    PROXM_IDLE,
    /// Busy state
    PROXM_BUSY,
    ///Discovering
    PROXM_DISCOVERING,

    /// Number of defined states.
    PROXM_STATE_MAX
};


/// Characteristics Link loss
enum
{
    PROXM_LK_LOSS_CHAR,

    PROXM_LK_LOSS_CHAR_MAX,
};

/// Characteristics IAS
enum
{
    PROXM_IAS_CHAR,

    PROXM_IAS_CHAR_MAX,
};

/// Characteristics Tx power
enum
{
    PROXM_TX_POWER_CHAR,

    PROXM_TX_POWER_CHAR_MAX,
};

/// Max characteristic number on services
enum
{
    PROXM_SVCS_CHAR_NB = 1
};

///Link Loss or Immediate Alert code for setting alert through one message
enum
{
    ///Code for LLS Alert Level Char.
    PROXM_SET_LK_LOSS_ALERT = 0x00,
    ///Code for IAS Alert Level Char.
    PROXM_SET_IMMDT_ALERT,
};

/// Read Characteristic Code
enum
{
    /// Read Link Loss Service Alert Level Characteristic Value
    PROXM_RD_LL_ALERT_LVL = 0x00,
    /// Read TX Power Service TX Power Level Characteristic Value
    PROXM_RD_TX_POWER_LVL,
};

///Alert Level Values
enum
{
    PROXM_ALERT_NONE    = 0x00,
    PROXM_ALERT_MILD,
    PROXM_ALERT_HIGH,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */


/// Proximity Monitor environment variable per connection
struct proxm_cnx_env
{
    /// Last service for which something was discovered
    uint8_t last_svc_req;
    /// counter used to check service uniqueness
    uint8_t nb_svc;
    /// used to store if measurement context
    uint8_t meas_ctx_en;

    struct svc_content prox[PROXM_SVC_NB];
};

/// Proximity monitor environment variable
struct proxm_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct proxm_cnx_env* env[PROXM_IDX_MAX];
    /// State of different task instances
    ke_state_t state[PROXM_IDX_MAX];
};


/*
 * GLOBAL VARIABLE DEFINITIONS
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
const struct prf_task_cbs* proxm_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send enable confirmation to application.
 * @param status Status of the enable: either OK or error encountered while discovery.
 ****************************************************************************************
 */
void proxm_enable_rsp_send(struct proxm_env_tag *proxm_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Check if collector request is possible or not
 *
 * @param[in] proxm_env Client environment.
 * @param[in] conidx    Connection Index.
 * @param[in] char_code Characteristic number.
 *
 * @return GAP_ERR_NO_ERROR if request can be performed, error code else.
 ****************************************************************************************
 */
uint8_t proxm_validate_request(struct proxm_env_tag *proxm_env, uint8_t conidx, uint8_t char_code);

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void proxm_task_init(struct ke_task_desc *task_desc);

#endif //BLE_PROX_MONITOR

/// @} PROXM

#endif // PROXM_H_
