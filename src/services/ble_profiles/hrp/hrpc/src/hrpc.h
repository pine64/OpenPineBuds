#ifndef _HRPC_H_
#define _HRPC_H_

/**
 ****************************************************************************************
 * @addtogroup HRPC Heart Rate Profile Collector
 * @ingroup HRP
 * @brief Heart Rate Profile Collector
 *
 * The HRPC is responsible for providing Heart Rate Profile Collector functionalities
 * to upper layer module or application. The device using this profile takes the role
 * of Heart Rate Profile Collector.
 *
 * Heart Rate Profile Collector. (HRPC): A HRPC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can interpret Heart Rate
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
#if (BLE_HR_COLLECTOR)

#include "hrp_common.h"
#include "hrpc_task.h"
#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Heart Rate Collector task instances
#define HRPC_IDX_MAX    (BLE_CONNECTION_MAX)

/// Possible states of the HRPC task
enum
{
    /// Free state
    HRPC_FREE,
    /// Idle state
    HRPC_IDLE,
    /// Discovering Heart Rate SVC and Chars
    HRPC_DISCOVERING,
    /// Busy state
    HRPC_BUSY,

    /// Number of defined states.
    HRPC_STATE_MAX
};

/// Internal codes for reading a HRS or DIS characteristic with one single request
enum
{
    ///Read HRS Heart Rate Measurement
    HRPC_RD_HRS_HR_MEAS          = HRPC_CHAR_HR_MEAS,
    ///Body Sensor Location
    HRPC_RD_HRS_BODY_SENSOR_LOC  = HRPC_CHAR_BODY_SENSOR_LOCATION,
    ///Heart Rate Control Point
    HRPC_RD_HRS_CNTL_POINT       = HRPC_CHAR_HR_CNTL_POINT,

    ///Read HRS Heart Rate Measurement Client Cfg. Desc
    HRPC_RD_HRS_HR_MEAS_CFG      = (HRPC_DESC_MASK | HRPC_DESC_HR_MEAS_CLI_CFG),
};



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct hrpc_cnx_env
{
    /// Current operation code
    void *operation;
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last characteristic code used in a read or a write request
    uint16_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;
    ///HTS characteristics
    struct hrs_content hrs;
};

/// Heart Rate Profile Collector environment variable
struct hrpc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connection
    struct hrpc_cnx_env* env[HRPC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[HRPC_IDX_MAX];
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
 * @brief Retrieve HRP client profile interface
 * @return HRP client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* hrpc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Heart Rate ATT DB discovery results to HRPC host.
 ****************************************************************************************
 */
void hrpc_enable_rsp_send(struct hrpc_env_tag *hrpc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack Heart Rate measurement data into a comprehensive structure.
 *
 * @param[out] pmeas_val  Pointer to Heart Rate measurement structure destination
 * @param[in]  packed_hr  Pointer of the packed data of Heart Rate Measurement
 *                        information
 * @param[in]  size       Packet data size
 ****************************************************************************************
 */
void hrpc_unpack_meas_value(struct hrs_hr_meas* pmeas_val, uint8_t* packed_hr, uint8_t size);

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
void hrpc_task_init(struct ke_task_desc *task_desc);


#endif /* (BLE_HR_COLLECTOR) */

/// @} HRPC

#endif /* _HRPC_H_ */
