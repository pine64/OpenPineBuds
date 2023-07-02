#ifndef _HRPS_TASK_H_
#define _HRPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HRPSTASK Task
 * @ingroup HRPS
 * @brief Heart Rate Profile Task.
 *
 * The HRPSTASK is responsible for handling the messages coming in and out of the
 * @ref HRPS collector block of the BLE Host.
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

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Messages for Heart Rate Profile Sensor
enum
{
    ///Start the Heart Rate Profile Sensor - at connection
    HRPS_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_HRPS),

    /// Disable confirmation with configuration to save after profile disabled
    HRPS_ENABLE_RSP,

    ///Send Heart Rate measurement value from APP
    HRPS_MEAS_SEND_REQ,
    ///Send Heart Rate measurement value confirm to APP so stable values can be erased
    ///if correctly sent.
    HRPS_MEAS_SEND_RSP,

    ///Inform APP of new configuration value
    HRPS_CFG_INDNTF_IND,
    ///Inform APP that Energy Expanded must be reset value
    HRPS_ENERGY_EXP_RESET_IND,
};

///Parameters of the @ref HRPS_CREATE_DB_REQ message
struct hrps_db_cfg
{
    ///Database configuration
    uint8_t features;
    ///Body Sensor Location
    uint8_t body_sensor_loc;
};

/// Parameters of the @ref HRPS_ENABLE_REQ message
struct hrps_enable_req
{
    ///Connection index
    uint8_t conidx;
    /// Heart Rate Notification configuration
    uint16_t hr_meas_ntf;
};

/// Parameters of the @ref HRPS_ENABLE_RSP message
struct hrps_enable_rsp
{
    /// Connection index
    uint8_t conidx;
    /// status
    uint8_t status;
};

///Parameters of the @ref HRPS_CFG_INDNTF_IND message
struct hrps_cfg_indntf_ind
{
    ///Connection idx
    uint8_t conidx;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

/////Parameters of the @ref HRPS_MEAS_SEND_REQ message
struct hrps_meas_send_req
{
    ///Heart Rate measurement
    struct hrs_hr_meas meas_val;
};

///Parameters of the @ref HRPS_MEAS_SEND_RSP message
struct hrps_meas_send_rsp
{
    ///Status
    uint8_t status;
};

///Parameters of the @ref HRPS_ENERGY_EXP_RESET_IND message
struct hrps_energy_exp_reset_ind
{
    ///Connection handle
    uint8_t conidx;
};


/// @} HRPSTASK

#endif /* _HRPS_TASK_H_ */
