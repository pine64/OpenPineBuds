#ifndef _CSCP_COMMON_H_
#define _CSCP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup CSCP Cycling Speed and Cadence Profile
 * @ingroup PROFILE
 * @brief Cycling Speed and Cadence Profile
 *
 * The Cycling Speed and Cadence profile enables a Collector device to connect and
 * interact with a Cycling Speed and Cadence Sensor for use in sports and fitness
 * applications.
 *
 * This file contains all definitions that are common for the server and the client parts
 * of the profile.
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */
/// Procedure Already in Progress Error Code
#define CSCP_ERROR_PROC_IN_PROGRESS         (0x80)
/// Client Characteristic Configuration descriptor improperly configured Error Code
#define CSCP_ERROR_CCC_INVALID_PARAM        (0x81)

/// CSC Measurement Value Min Length
#define CSCP_CSC_MEAS_MIN_LEN               (1)
/// CSC Measurement Value Max Length
#define CSCP_CSC_MEAS_MAX_LEN               (11)
/// SC Control Point Request Value Min Length
#define CSCP_SC_CNTL_PT_REQ_MIN_LEN         (1)
/// SC Control Point Request Value Max Length
#define CSCP_SC_CNTL_PT_REQ_MAX_LEN         (5)
/// SC Control Point Response Value Min Length
#define CSCP_SC_CNTL_PT_RSP_MIN_LEN         (3)
/// SC Control Point Response Value Max Length
#define CSCP_SC_CNTL_PT_RSP_MAX_LEN         (CSCP_SC_CNTL_PT_RSP_MIN_LEN + CSCP_LOC_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Service Characteristics
enum cscp_cscs_char
{
    /// CSC Measurement
    CSCP_CSCS_CSC_MEAS_CHAR,
    /// CSC Feature
    CSCP_CSCS_CSC_FEAT_CHAR,
    /// Sensor Location
    CSCP_CSCS_SENSOR_LOC_CHAR,
    /// SC Control Point
    CSCP_CSCS_SC_CTNL_PT_CHAR,

    CSCP_CSCS_CHAR_MAX,
};

/// CSC Measurement Flags
enum cscp_meas_flags
{
    /// Wheel Revolution Data Present
    CSCP_MEAS_WHEEL_REV_DATA_PRESENT    = 0x01,
    /// Crank Revolution Data Present
    CSCP_MEAS_CRANK_REV_DATA_PRESENT    = 0x02,

    /// All present
    CSCP_MEAS_ALL_PRESENT               = 0x03,
};

/// CSC Feature Flags
enum cscp_feat_flags
{
    /// Wheel Revolution Data Supported
    CSCP_FEAT_WHEEL_REV_DATA_SUPP       = 0x0001,
    /// Crank Revolution Data Supported
    CSCP_FEAT_CRANK_REV_DATA_SUPP       = 0x0002,
    /// Multiple Sensor Location Supported
    CSCP_FEAT_MULT_SENSOR_LOC_SUPP      = 0x0004,

    /// All supported
    CSCP_FEAT_ALL_SUPP                  = 0x0007,
};

/// Sensor Locations Keys
enum cscp_sensor_loc
{
    /// Other (0)
    CSCP_LOC_OTHER          = 0,
    /// Front Wheel (4)
    CSCP_LOC_FRONT_WHEEL    = 4,
    /// Left Crank (5)
    CSCP_LOC_LEFT_CRANK,
    /// Right Crank (6)
    CSCP_LOC_RIGHT_CRANK,
    /// Left Pedal (7)
    CSCP_LOC_LEFT_PEDAL,
    /// Right Pedal (8)
    CSCP_LOC_RIGHT_PEDAL,
    /// Rear Dropout (9)
    CSCP_LOC_REAR_DROPOUT,
    /// Chainstay (10)
    CSCP_LOC_CHAINSTAY,
    /// Front Hub (11)
    CSCP_LOC_FRONT_HUB,
    /// Rear Wheel (12)
    CSCP_LOC_REAR_WHEEL,
    /// Rear Hub (13)
    CSCP_LOC_REAR_HUB,

    CSCP_LOC_MAX,
};

/// Control Point Operation Code Keys
enum cscp_sc_ctnl_pt_op_code
{
    /// Reserved value
    CSCP_CTNL_PT_OP_RESERVED        = 0,

    /// Set Cumulative Value
    CSCP_CTNL_PT_OP_SET_CUMUL_VAL,
    /// Start Sensor Calibration
    CSCP_CTNL_PT_OP_START_CALIB,
    /// Update Sensor Location
    CSCP_CTNL_PT_OP_UPD_LOC,
    /// Request Supported Sensor Locations
    CSCP_CTNL_PT_OP_REQ_SUPP_LOC,

    /// Response Code
    CSCP_CTNL_PT_RSP_CODE           = 16,
};

/// Control Point Response Value
enum cscp_ctnl_pt_resp_val
{
    /// Reserved value
    CSCP_CTNL_PT_RESP_RESERVED      = 0,

    /// Success
    CSCP_CTNL_PT_RESP_SUCCESS,
    /// Operation Code Not Supported
    CSCP_CTNL_PT_RESP_NOT_SUPP,
    /// Invalid Parameter
    CSCP_CTNL_PT_RESP_INV_PARAM,
    /// Operation Failed
    CSCP_CTNL_PT_RESP_FAILED,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// CSC Measurement
struct cscp_csc_meas
{
    /// Flags
    uint8_t flags;
    /// Cumulative Crank Revolution
    uint16_t cumul_crank_rev;
    /// Last Crank Event Time
    uint16_t last_crank_evt_time;
    /// Last Wheel Event Time
    uint16_t last_wheel_evt_time;
    /// Cumulative Wheel Revolution
    uint32_t cumul_wheel_rev;
};

/// SC Control Point Request
struct cscp_sc_ctnl_pt_req
{
    /// Operation Code
    uint8_t op_code;
    /// Value
    union cscp_sc_ctnl_pt_req_val
    {
        /// Sensor Location
        uint8_t sensor_loc;
        /// Cumulative value
        uint32_t cumul_val;
    } value;
};

/// SC Control Point Response
struct cscp_sc_ctnl_pt_rsp
{
    /// Requested Operation Code
    uint8_t req_op_code;
    /// Response Value
    uint8_t resp_value;
    /// List of supported locations
    uint16_t supp_loc;
};


/// @} cscp_common

#endif //(_CSCP_COMMON_H_)
