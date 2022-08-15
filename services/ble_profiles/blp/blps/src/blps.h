#ifndef _BLPS_H_
#define _BLPS_H_

/**
 ****************************************************************************************
 * @addtogroup BLPS Blood Pressure Profile Sensor
 * @ingroup BLP
 * @brief Blood Pressure Profile Sensor
 *
 * Blood pressure sensor (BPS) profile provides functionalities to upper layer module
 * application. The device using this profile takes the role of Blood pressure sensor.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Notify peer device during Blood Pressure measurement (from APP)
 *  - Indicate measurements performed to peer device (from APP)
 *
 * Profile didn't manages multiple users configuration and storage of offline measurements.
 * This must be handled by application.
 *
 * Blood Pressure Profile Sensor. (BLPS): A BLPS (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can perform blood pressure
 * measurement and notify about on-going measurement and indicate final result to a peer
 * BLE device.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "blp_common.h"

#if (BLE_BP_SENSOR)

#include "prf_types.h"
#include "prf.h"
#include "blps_task.h"
#include "attm.h"

/*
 * DEFINES
 ****************************************************************************************
 */
///Maximum number of Blood Pressure task instances
#define BLPS_IDX_MAX     0x01

/// Maximum notification length
#define BLPS_BP_MEAS_MAX_LEN            (19)

///BPS Configuration Flag Masks
#define BLPS_MANDATORY_MASK              (0x003F)
#define BLPS_INTM_CUFF_PRESS_MASK        (0x01C0)

/*
 * MACROS
 ****************************************************************************************
 */

/// Possible states of the BLPS task
enum
{
    /// Idle state
    BLPS_IDLE,
    /// Busy state
    BLPS_BUSY,

    /// Number of defined states.
    BLPS_STATE_MAX
};


#define BLPS_IS_SUPPORTED(mask) \
    (((blps_env->prfl_cfg & mask) == mask))

#define BLPS_IS_ENABLED(mask, conidx) \
    (((blps_env->prfl_ntf_ind_cfg[conidx] & mask) == mask))

///Attributes State Machine
enum
{
    BPS_IDX_SVC,

    BPS_IDX_BP_MEAS_CHAR,
    BPS_IDX_BP_MEAS_VAL,
    BPS_IDX_BP_MEAS_IND_CFG,

    BPS_IDX_BP_FEATURE_CHAR,
    BPS_IDX_BP_FEATURE_VAL,

    BPS_IDX_INTM_CUFF_PRESS_CHAR,
    BPS_IDX_INTM_CUFF_PRESS_VAL,
    BPS_IDX_INTM_CUFF_PRESS_NTF_CFG,

    BPS_IDX_NB,
};

///Characteristic Codes
enum
{
    BPS_BP_MEAS_CHAR,
    BPS_INTM_CUFF_MEAS_CHAR,
    BPS_BP_FEATURE_CHAR,
};

///Database Configuration Bit Field Flags
enum
{
    /// support of Intermediate Cuff Pressure
    BLPS_INTM_CUFF_PRESS_SUP        = 0x01,
};

/// Indication/notification configuration (put in feature flag to optimize memory usage)
enum
{
    /// Bit used to know if blood pressure measurement indication is enabled
    BLPS_BP_MEAS_IND_CFG            = 0x02,
    /// Bit used to know if cuff pressure measurement notification is enabled
    BLPS_INTM_CUFF_PRESS_NTF_CFG    = 0x04,
    /// indication/notification config mask
    BLPS_NTFIND_MASK                = 0x06,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Blood Pressure Profile Sensor environment variable per connection
struct blps_cnx_env
{
    /// Profile Notify/Indication Flags
    uint8_t prfl_ntf_ind_cfg;
};

/// Blood Pressure Profile Sensor environment variable
struct blps_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Blood Pressure Service Start Handle
    uint16_t shdl;
    /// Feature Configuration Flags
    uint16_t features;
    /// Profile configuration flags
    uint8_t prfl_cfg;
    /// Event (notification/indication) config
    uint8_t evt_cfg;
    /// Environment variable pointer for each connections
    uint8_t prfl_ntf_ind_cfg[BLE_CONNECTION_MAX];
    /// State of different task instances
    ke_state_t state[BLPS_IDX_MAX];
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
 * @brief Retrieve BLP service profile interface
 *
 * @return BLP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* blps_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Pack Blood Pressure measurement value
 *
 * @param p_meas_val Blood Pressure measurement value
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t blps_pack_meas_value(uint8_t *packed_bp, const struct bps_bp_meas* pmeas_val);

/**
 ****************************************************************************************
 * @brief Send a BLPS_MEAS_SEND_RSP to the application
 ****************************************************************************************
 */
void blps_meas_send_rsp_send(uint8_t conidx, uint8_t status);

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void blps_task_init(struct ke_task_desc *task_desc);


#endif /* #if (BLE_BP_SENSOR) */

/// @} BLPS

#endif /* _BLPS_H_ */
