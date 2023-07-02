#ifndef _BLPC_H_
#define _BLPC_H_


/**
 ****************************************************************************************
 * @addtogroup BLPC Blood Pressure Profile Collector
 * @ingroup BLP
 * @brief Blood Pressure Profile Collector
 *
 * The BLPC is responsible for providing Blood Pressure Profile Collector functionalities
 * to upper layer module or application. The device using this profile takes the role
 * of Blood Pressure Profile Collector.
 *
 * Blood Pressure Profile Collector. (BLPC): A BLPC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can interpret blood pressure
 * measurement in a way suitable to the user application.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BP_COLLECTOR)

#include "blpc_task.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Blood Pressure Collector task instances
#define BLPC_IDX_MAX    (BLE_CONNECTION_MAX)

/// Possible states of the BLPC task
enum
{
    /// Free state
    BLPC_FREE,
    /// IDLE state
    BLPC_IDLE,
    /// Discovering Blood Pressure SVC and CHars
    BLPC_DISCOVERING,
    /// Busy state
    BLPC_BUSY,

    /// Number of defined states.
    BLPC_STATE_MAX
};

/// Internal codes for reading a BPS or DIS characteristic with one single request
enum
{
    ///Read BPS Blood pressure Measurement
    BLPC_RD_BPS_BP_MEAS          = BLPC_CHAR_BP_MEAS,
    ///Read BPS Intermdiate Cuff Pressure
    BLPC_RD_BPS_CP_MEAS          = BLPC_CHAR_CP_MEAS,
    ///Read BPS Blood pressure Features
    BLPC_RD_BPS_FEATURE          = BLPC_CHAR_BP_FEATURE,

    ///Read BPS Blood pressure Measurement Client Cfg. Desc
    BLPC_RD_BPS_BP_MEAS_CFG      = (BLPC_DESC_MASK | BLPC_DESC_BP_MEAS_CLI_CFG),
    ///Read BPS Intermdiate Cuff Pressure Client Cfg. Desc
    BLPC_RD_BPS_CP_MEAS_CFG      = (BLPC_DESC_MASK | BLPC_DESC_IC_MEAS_CLI_CFG),
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
struct blpc_cnx_env
{
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last characteristic code used in a read or a write request
    uint16_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;
    /// Current operation code
    uint8_t operation;
    ///HTS characteristics
    struct bps_content bps;
};

/// Blood Pressure Profile Collector environment variable
struct blpc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct blpc_cnx_env* env[BLPC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[BLPC_IDX_MAX];
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve BLP client profile interface
 * @return BLP client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* blpc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Blood Pressure ATT DB discovery results to BLPC host.
 ****************************************************************************************
 */
void blpc_enable_rsp_send(struct blpc_env_tag *blpc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack Blood pressure measurement data into a comprehensive structure.
 *
 * @param[out] pmeas_val  Pointer to Blood pressure measurement structure destination
 * @param[in] packed_bp   Pointer of the packed data of Blood Pressure Measurement
 *                        information
 ****************************************************************************************
 */
void blpc_unpack_meas_value(struct bps_bp_meas* pmeas_val, uint8_t* packed_bp);

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
void blpc_task_init(struct ke_task_desc *task_desc);


#endif /* (BLE_BP_COLLECTOR) */

/// @} BLPC

#endif /* _BLPC_H_ */
