#ifndef _RSCP_COMMON_H_
#define _RSCP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup RSCP Running Speed and Cadence Profile
 * @ingroup PROFILE
 * @brief Running Speed and Cadence Profile
 *
 * The Running Speed and Cadence profile enables a Collector device to connect and
 * interact with a Running Speed and Cadence Sensor for use in sports and fitness
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

#include "prf_types.h"
#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */
/// Procedure Already in Progress Error Code
#define RSCP_ERROR_PROC_IN_PROGRESS         (0x80)
/// Client Characteristic Configuration descriptor improperly configured Error Code
#define RSCP_ERROR_CCC_INVALID_PARAM        (0x81)

/// RSC Measurement Value Min Length
#define RSCP_RSC_MEAS_MIN_LEN               (4)
/// RSC Measurement Value Max Length
#define RSCP_RSC_MEAS_MAX_LEN               (10)
/// SC Control Point Request Value Min Length
#define RSCP_SC_CNTL_PT_REQ_MIN_LEN         (1)
/// SC Control Point Request Value Max Length
#define RSCP_SC_CNTL_PT_REQ_MAX_LEN         (5)
/// SC Control Point Response Value Min Length
#define RSCP_SC_CNTL_PT_RSP_MIN_LEN         (3)
/// SC Control Point Response Value Max Length
#define RSCP_SC_CNTL_PT_RSP_MAX_LEN         (RSCP_SC_CNTL_PT_RSP_MIN_LEN + RSCP_LOC_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Running Speed and Cadence Service Characteristics
enum rscp_rscs_char
{
    /// RSC Measurement
    RSCP_RSCS_RSC_MEAS_CHAR,
    /// RSC Feature
    RSCP_RSCS_RSC_FEAT_CHAR,
    /// Sensor Location
    RSCP_RSCS_SENSOR_LOC_CHAR,
    /// SC Control Point
    RSCP_RSCS_SC_CTNL_PT_CHAR,

    /// Maximum Characteristic
    RSCP_RSCS_CHAR_MAX,
};

/// RSC Measurement Flags
enum rscp_meas_flags
{
    /// Instantaneous Stride Length Present
    RSCP_MEAS_INST_STRIDE_LEN_PRESENT   = 0x01,
    /// Total Distance Present
    RSCP_MEAS_TOTAL_DST_MEAS_PRESENT    = 0x02,
    /// Walking or Running
    RSCP_MEAS_WALK_RUN_STATUS           = 0x04,

    /// All present
    RSCP_MEAS_ALL_PRESENT               = 0x07,
};

/// RSC Feature Flags
enum rscp_feat_flags
{
    /// Instantaneous Stride Length Measurement Supported
    RSCP_FEAT_INST_STRIDE_LEN_SUPP      = 0x0001,
    /// Total Distance Measurement Supported
    RSCP_FEAT_TOTAL_DST_MEAS_SUPP       = 0x0002,
    /// Walking or Running Status Supported
    RSCP_FEAT_WALK_RUN_STATUS_SUPP      = 0x0004,
    /// Calibration Procedure Supported
    RSCP_FEAT_CALIB_PROC_SUPP           = 0x0008,
    /// Multiple Sensor Locations Supported
    RSCP_FEAT_MULT_SENSOR_LOC_SUPP      = 0x0010,

    /// All supported
    RSCP_FEAT_ALL_SUPP                  = 0x001F,
};

/// Sensor Locations Keys
enum rscp_sensor_loc
{
    /// Other (0)
    RSCP_LOC_OTHER          = 0,
    /// Top of shoe (1)
    RSCP_LOC_TOP_SHOE,
    /// In shoe (2)
    RSCP_LOC_IN_SHOE,
    /// Hip (3)
    RSCP_LOC_HIP,
    /// Chest (14)
    RSCP_LOC_CHEST          = 14,

    /// Maximum Sensor Location
    RSCP_LOC_MAX,
};

/// Control Point Operation Code Keys
enum rscp_sc_ctnl_pt_op_code
{
    /// Reserved value
    RSCP_CTNL_PT_OP_RESERVED        = 0,

    /// Set Cumulative Value
    RSCP_CTNL_PT_OP_SET_CUMUL_VAL,
    /// Start Sensor Calibration
    RSCP_CTNL_PT_OP_START_CALIB,
    /// Update Sensor Location
    RSCP_CTNL_PT_OP_UPD_LOC,
    /// Request Supported Sensor Locations
    RSCP_CTNL_PT_OP_REQ_SUPP_LOC,

    /// Response Code
    RSCP_CTNL_PT_RSP_CODE           = 16,
};

/// Control Point Response Value
enum rscp_sc_ctnl_pt_resp_val
{
    /// Reserved value
    RSCP_CTNL_PT_RESP_RESERVED      = 0,

    /// Success
    RSCP_CTNL_PT_RESP_SUCCESS,
    /// Operation Code Not Supported
    RSCP_CTNL_PT_RESP_NOT_SUPP,
    /// Invalid Parameter
    RSCP_CTNL_PT_RESP_INV_PARAM,
    /// Operation Failed
    RSCP_CTNL_PT_RESP_FAILED,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// RSC Measurement
struct rscp_rsc_meas
{
    /// Flags
    uint8_t flags;
    /// Instantaneous Cadence
    uint8_t inst_cad;
    /// Instantaneous Speed
    uint16_t inst_speed;
    /// Instantaneous Stride Length
    uint16_t inst_stride_len;
    /// Total Distance
    uint32_t total_dist;
};

/// SC Control Point Request
struct rscp_sc_ctnl_pt_req
{
    /// Operation Code
    uint8_t op_code;
    /// Value
    union rscp_sc_ctnl_pt_req_val
    {
        /// Sensor Location
        uint8_t sensor_loc;
        /// Cumulative value
        uint32_t cumul_val;
    } value;
};

/// SC Control Point Response
struct rscp_sc_ctnl_pt_rsp
{
    /// Requested Operation Code
    uint8_t req_op_code;
    /// Response Value
    uint8_t resp_value;
    /// List of supported locations
    uint16_t supp_loc;
};


/// @} rscp_common

#endif //(_RSCP_COMMON_H_)
