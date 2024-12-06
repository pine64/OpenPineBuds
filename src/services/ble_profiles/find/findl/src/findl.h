#ifndef FINDL_H_
#define FINDL_H_

/**
 ****************************************************************************************
 * @addtogroup FIND Find Me Profile
 * @ingroup PROFILE
 * @brief Find Me Profile
 *
 * The FIND module is the responsible block for implementing the Find Me profile
 * functionalities in the BLE Host.
 *
 * The Find Me Profile defines the functionality required in a device that allows the user
 * to find a peer device by setting its alarm level.
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup FINDL Find Me Locator
 * @ingroup FIND
 * @brief Find Me Profile Locator
 *
 * The FINDL is responsible for providing Find Me profile Locator functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of find me locator.
 *
 * Find Me Locator (LOC): A LOC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can set an alarm level value
 * in the peer Find Me target Device (TG), causing the TG to start a sound or flashing light
 * or other type of signal allowing it to be located.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Discover Immediate Alert Service range(from APP) and Send result (to APP)
 *  - Discover Alert Level Characteristic in the IAS range(from APP) and Send Result (to APP)
 *  - Set alert level in Target (from APP)
 *  - Error indications (to APP)
 *
 *  The application should remember the discovered IAS range and Alert Level Char. handles
 *  for the next connection to a known peer (that may also advertise it supports Find Me Profile.)
 *  This allows the setting of the alert level to be faster.
 *
 *  The enable/disable of the profile and devices disconnection is handled in the application,
 *  depending on its User Input.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_FINDME_LOCATOR)
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "findl_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Find me Locator task instances
#define FINDL_IDX_MAX    (BLE_CONNECTION_MAX)


/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Possible states of the FINDL task
enum findl_state
{
    /// Not Connected State
    FINDL_FREE,
    /// Idle state
    FINDL_IDLE,
    /// Discovering state
    FINDL_DISCOVERING,
    /// Busy State
    FINDL_BUSY,

    /// Number of defined states.
    FINDL_STATE_MAX
};
enum
{
    FINDL_ALERT_LVL_CHAR,

    FINDL_IAS_CHAR_MAX,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Environment variable for each Connections
struct findl_cnx_env
{
    /// Found IAS details
    struct ias_content ias;
    /// Last char. code requested to read.
    uint8_t  last_char_code;
    /// counter used to check service uniqueness
    uint8_t  nb_svc;
};

/// Find me Locator environment variable
struct findl_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct findl_cnx_env* env[FINDL_IDX_MAX];
    /// State of different task instances
    ke_state_t state[FINDL_IDX_MAX];
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
 * @brief Retrieve IAS service profile interface
 *
 * @return IAS service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* findl_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Enable Confirm message to the application.
 * @param status  Status to send.
 ****************************************************************************************
 */
void findl_enable_rsp_send(struct findl_env_tag *findl_env, uint8_t conidx, uint8_t status);

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
void findl_task_init(struct ke_task_desc *task_desc);


#endif //BLE_FINDME_LOCATOR

/// @} FINDL
#endif // FINDL_H_
