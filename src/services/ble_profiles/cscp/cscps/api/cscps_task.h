#ifndef _CSCPS_TASK_H_
#define _CSCPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CSCPSTASK Task
 * @ingroup CSCPS
 * @brief Cycling Speed and Cadence Profile Task.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cscp_common.h"
#include <stdint.h>
#include "rwip_task.h" // Task definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Messages for Cycling Speed and Cadence Profile Sensor
enum cscps_msg_id
{
    /// Enable the CSCP Sensor task for a connection
    CSCPS_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_CSCPS),
    /// Enable the CSCP Sensor task for a connection
    CSCPS_ENABLE_RSP,

    /// Send a CSC Measurement to the peer device (Notification)
    CSCPS_NTF_CSC_MEAS_REQ,
    /// Inform that CSC Measurement sent to peer devices.
    CSCPS_NTF_CSC_MEAS_RSP,

    /// Indicate that the SC Control Point characteristic value has been written
    CSCPS_SC_CTNL_PT_REQ_IND,
    /// Application response after receiving a CSCPS_SC_CTNL_PT_REQ_IND message
    CSCPS_SC_CTNL_PT_CFM,

    /// Indicate a new NTF/IND configuration to the application
    CSCPS_CFG_NTFIND_IND,

    /// Send a complete event status to the application
    CSCPS_CMP_EVT,
};

/// Operation Code used in the profile state machine
enum cscps_op_code
{
    /// Reserved Operation Code
    CSCPS_RESERVED_OP_CODE          = 0x00,

    /// Send CSC Measurement Operation Code
    CSCPS_SEND_CSC_MEAS_OP_CODE,

    /**
     * SC Control Point Operation
     */

    /// Set Cumulative Value Operation Code
    CSCPS_CTNL_PT_CUMUL_VAL_OP_CODE,
    /// Update Sensor Location Operation Code
    CSCPS_CTNL_PT_UPD_LOC_OP_CODE,
    /// Request Supported Sensor Locations Operation Code
    CSCPS_CTNL_PT_SUPP_LOC_OP_CODE,
    /// Error Indication Sent Operation Code
    CSCPS_CTNL_ERR_IND_OP_CODE,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the initialization function
struct cscps_db_cfg
{
    /**
     * CSC Feature Value - Not supposed to be modified during the lifetime of the device
     * This value is used to decide if the SC Control Point Characteristic is part of the
     * Cycling Speed and Cadence service.
     */
    uint16_t csc_feature;
    /**
     * Indicate if the Sensor Location characteristic is supported. Note that if the Multiple
     * Sensor Location feature is set has supported in the csc_feature parameter, the
     * characteristic will be added (mandatory).
     */
    uint8_t sensor_loc_supp;
    /// Sensor location
    uint8_t sensor_loc;
    /// Wheel revolution
    uint32_t wheel_rev;
};

/// Parameters of the @ref CSCPS_ENABLE_REQ message
struct cscps_enable_req
{
    /// Connection index
    uint8_t conidx;
    /// Stored CSC Measurement Char. Client Characteristic Configuration
    uint16_t csc_meas_ntf_cfg;
    /// Stored SC Control Point Char. Client Characteristic Configuration
    uint16_t sc_ctnl_pt_ntf_cfg;
};

/// Parameters of the @ref CSCPS_ENABLE_RSP message
struct cscps_enable_rsp
{
    /// Connection index
    uint8_t conidx;
    /// Status
    uint8_t status;
};

/// Parameters of the @ref CSCPS_NTF_CSC_MEAS_REQ message
struct cscps_ntf_csc_meas_req
{
    /// Flags
    uint8_t flags;
    /// Cumulative Crank Revolution
    uint16_t cumul_crank_rev;
    /// Last Crank Event Time
    uint16_t last_crank_evt_time;
    /// Last Wheel Event Time
    uint16_t last_wheel_evt_time;
    /// Wheel Revolution since the last wheel event time
    int16_t wheel_rev;
};

/// Parameters of the @ref CSCPS_NTF_CSC_MEAS_RSP message
struct cscps_ntf_csc_meas_rsp
{
    /// Operation Status
    uint8_t  status;
    /// Cummul Wheel revolution value
    uint32_t tot_wheel_rev;
};


/// Parameters of the @ref CSCPS_SC_CTNL_PT_REQ_IND message
struct cscps_sc_ctnl_pt_req_ind
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t op_code;
    /// Value
    union cscps_sc_ctnl_pt_req_ind_value
    {
        /// Cumulative value
        uint32_t cumul_value;
        /// Sensor Location
        uint8_t sensor_loc;
    } value;
};

/// Parameters of the @ref CSCPS_SC_CTNL_PT_CFM message
struct cscps_sc_ctnl_pt_cfm
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t op_code;
    /// Status
    uint8_t status;
    /// Value
    union cscps_sc_ctnl_pt_cfm_value
    {
        /// Sensor Location
        uint8_t sensor_loc;
        /// Supported sensor locations
        uint16_t supp_sensor_loc;
        /// New Cumulative Wheel revolution Value
        uint32_t cumul_wheel_rev;
    } value;
};

/// Parameters of the @ref CSCPS_CFG_NTFIND_IND message
struct cscps_cfg_ntfind_ind
{
    /// Connection handle
    uint8_t conidx;
    /// Characteristic Code (CSC Measurement or SC Control Point)
    uint8_t char_code;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref CSCPS_CMP_EVT message
struct cscps_cmp_evt
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t operation;
    /// Operation Status
    uint8_t status;
};

/// @} CSCPSTASK

#endif //(_CSCPS_TASK_H_)
