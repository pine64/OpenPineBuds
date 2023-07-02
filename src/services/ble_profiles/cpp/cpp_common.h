#ifndef _CPP_COMMON_H_
#define _CPP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup CPP Cycling Power Profile
 * @ingroup PROFILE
 * @brief Cycling Power Profile
 *
 * The Cycling Power Profile is used to enable a collector device in order to obtain
 * data from a Cycling Power Sensor (CP Sensor) that exposes the Cycling Power Service
 * in sports and fitness applications.
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
#define CPP_ERROR_PROC_IN_PROGRESS        (0x80)

/// CP Measurement Notification Value Max Length
#define CPP_CP_MEAS_NTF_MAX_LEN           (35)
/// CP Measurement Value Min Length
#define CPP_CP_MEAS_NTF_MIN_LEN           (4)

/// ADV Header size
#define CPP_CP_ADV_HEADER_LEN             (3)
/// ADV Length size
#define CPP_CP_ADV_LENGTH_LEN             (1)

/// CP Measurement Advertisement Value Max Length
#define CPP_CP_MEAS_ADV_MAX_LEN           (CPP_CP_MEAS_NTF_MAX_LEN + CPP_CP_ADV_HEADER_LEN)
/// CP Measurement Value Min Length
#define CPP_CP_MEAS_ADV_MIN_LEN           (CPP_CP_MEAS_NTF_MIN_LEN + CPP_CP_ADV_HEADER_LEN)

/// CP Vector Value Max Length
#define CPP_CP_VECTOR_MAX_LEN             (19)
/// CP Vector Value Min Length
#define CPP_CP_VECTOR_MIN_LEN             (1)

/// CP Control Point Value Max Length
#define CPP_CP_CNTL_PT_REQ_MAX_LEN        (9)
/// CP Control Point Value Min Length
#define CPP_CP_CNTL_PT_REQ_MIN_LEN        (1)

/// CP Control Point Value Max Length
#define CPP_CP_CNTL_PT_RSP_MAX_LEN        (20)
/// CP Control Point Value Min Length
#define CPP_CP_CNTL_PT_RSP_MIN_LEN        (3)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// CPP Service Characteristics
enum cpp_cpps_char
{
    /// CPP Measurement
    CPP_CPS_MEAS_CHAR,
    /// CPP Feature
    CPP_CPS_FEAT_CHAR,
    /// Sensor Location
    CPP_CPS_SENSOR_LOC_CHAR,
    ///Cycling Power Vector
    CPP_CPS_VECTOR_CHAR,
    /// CP Control Point
    CPP_CPS_CTNL_PT_CHAR,

    CPP_CPS_CHAR_MAX,
};

/// CPP Measurement Flags
enum cpp_meas_flags
{
    /// Pedal Power Balance Present
    CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT         = 0x0001,
    /// Pedal Power Balance Reference
    CPP_MEAS_PEDAL_POWER_BALANCE_REFERENCE       = 0x0002,
    /// Accumulated Torque Present
    CPP_MEAS_ACCUM_TORQUE_PRESENT                = 0x0004,
    /// Accumulated Torque Source
    CPP_MEAS_ACCUM_TORQUE_SOURCE                 = 0x0008,
    /// Wheel Revolution Data Present
    CPP_MEAS_WHEEL_REV_DATA_PRESENT              = 0x0010,
    /// Crank Revolution Data Present
    CPP_MEAS_CRANK_REV_DATA_PRESENT              = 0x0020,
    /// Extreme Force Magnitudes Present
    CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT    = 0x0040,
    /// Extreme Torque Magnitudes Present
    CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT   = 0x0080,
    /// Extreme Angles Present
    CPP_MEAS_EXTREME_ANGLES_PRESENT              = 0x0100,
    /// Top Dead Spot Angle Present
    CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT         = 0x0200,
    /// Bottom Dead Spot Angle Present
    CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT      = 0x0400,
    /// Accumulated Energy Present
    CPP_MEAS_ACCUM_ENERGY_PRESENT                = 0x0800,
    /// Offset Compensation Indicator
    CPP_MEAS_OFFSET_COMPENSATION_INDICATOR       = 0x1000,

    /// All Supported
    CPP_MEAS_ALL_SUPP                            = 0x1FFF,
};

/// CPP Feature Flags
enum cpp_feat_flags
{
    /// Pedal Power Balance Supported
    CPP_FEAT_PEDAL_POWER_BALANCE_SUPP           = 0x00000001,
    /// Accumulated Torque Supported
    CPP_FEAT_ACCUM_TORQUE_SUPP                  = 0x00000002,
    /// Wheel Revolution Data Supported
    CPP_FEAT_WHEEL_REV_DATA_SUPP                = 0x00000004,
    /// Crank Revolution Data Supported
    CPP_FEAT_CRANK_REV_DATA_SUPP                = 0x00000008,
    /// Extreme Magnitudes Supported
    CPP_FEAT_EXTREME_MAGNITUDES_SUPP            = 0x00000010,
    /// Extreme Angles Supported
    CPP_FEAT_EXTREME_ANGLES_SUPP                = 0x00000020,
    /// Top and Bottom Dead Spot Angles Supported
    CPP_FEAT_TOPBOT_DEAD_SPOT_ANGLES_SUPP       = 0x00000040,
    /// Accumulated Energy Supported
    CPP_FEAT_ACCUM_ENERGY_SUPP                  = 0x00000080,
    /// Offset Compensation Indicator Supported
    CPP_FEAT_OFFSET_COMP_IND_SUPP               = 0x00000100,
    /// Offset Compensation Supported
    CPP_FEAT_OFFSET_COMP_SUPP                   = 0x00000200,
    /// CP Measurement CH Content Masking Supported
    CPP_FEAT_CP_MEAS_CH_CONTENT_MASKING_SUPP    = 0x00000400,
    /// Multiple Sensor Locations Supported
    CPP_FEAT_MULT_SENSOR_LOC_SUPP               = 0x00000800,
    /// Crank Length Adjustment Supported
    CPP_FEAT_CRANK_LENGTH_ADJ_SUPP              = 0x00001000,
    /// Chain Length Adjustment Supported
    CPP_FEAT_CHAIN_LENGTH_ADJ_SUPP              = 0x00002000,
    /// Chain Weight Adjustment Supported
    CPP_FEAT_CHAIN_WEIGHT_ADJ_SUPP              = 0x00004000,
    /// Span Length Adjustment Supported
    CPP_FEAT_SPAN_LENGTH_ADJ_SUPP               = 0x00008000,
    /// Sensor Measurement Context
    CPP_FEAT_SENSOR_MEAS_CONTEXT                = 0x00010000,
    /// Instantaneous Measurement Direction Supported
    CPP_FEAT_INSTANT_MEAS_DIRECTION_SUPP        = 0x00020000,
    /// Factory Calibration Date Supported
    CPP_FEAT_FACTORY_CALIBRATION_DATE_SUPP      = 0x00040000,

    /// All supported
    CPP_FEAT_ALL_SUPP                            = 0x0007FFFF,
};

/// CPP Sensor Locations Keys
enum cpp_sensor_loc
{
    /// Other (0)
    CPP_LOC_OTHER          = 0,
    /// Top of shoe (1)
    CPP_LOC_TOP_SHOE,
    /// In shoe (2)
    CPP_LOC_IN_SHOE,
    /// Hip (3)
    CPP_LOC_HIP,
    /// Front Wheel (4)
    CPP_LOC_FRONT_WHEEL,
    /// Left Crank (5)
    CPP_LOC_LEFT_CRANK,
    /// Right Crank (6)
    CPP_LOC_RIGHT_CRANK,
    /// Left Pedal (7)
    CPP_LOC_LEFT_PEDAL,
    /// Right Pedal (8)
    CPP_LOC_RIGHT_PEDAL,
    /// Front Hub (9)
    CPP_LOC_FRONT_HUB,
    /// Rear Dropout (10)
    CPP_LOC_REAR_DROPOUT,
    /// Chainstay (11)
    CPP_LOC_CHAINSTAY,
    /// Rear Wheel (12)
    CPP_LOC_REAR_WHEEL,
    /// Rear Hub (13)
    CPP_LOC_REAR_HUB,
    /// Chest (14)
    CPP_LOC_CHEST,

    CPP_LOC_MAX,
};

/// CPP Vector Flags
enum cpp_vector_flags
{
    /// Crank Revolution Data Present
    CPP_VECTOR_CRANK_REV_DATA_PRESENT                = 0x01,
    /// First Crank Measurement Angle Present
    CPP_VECTOR_FIRST_CRANK_MEAS_ANGLE_PRESENT        = 0x02,
    /// Instantaneous Force Magnitude Array Present
    CPP_VECTOR_INST_FORCE_MAGNITUDE_ARRAY_PRESENT    = 0x04,
    /// Instantaneous Torque Magnitude Array Present
    CPP_VECTOR_INST_TORQUE_MAGNITUDE_ARRAY_PRESENT   = 0x08,
    /// Instantaneous Measurement Direction LSB
    CPP_VECTOR_INST_MEAS_DIRECTION_LSB                 = 0x10,
    /// Instantaneous Measurement Direction MSB
    CPP_VECTOR_INST_MEAS_DIRECTION_MSB                 = 0x20,

    ///All suported
    CPP_VECTOR_ALL_SUPP                                 = 0x3F,
};

/// CPP Control Point Code Keys
enum cpp_ctnl_pt_code
{
    /// Reserved value
    CPP_CTNL_PT_RESERVED        = 0,

    /// Set Cumulative Value
    CPP_CTNL_PT_SET_CUMUL_VAL,
    /// Update Sensor Location
    CPP_CTNL_PT_UPD_SENSOR_LOC,
    /// Request Supported Sensor Locations
    CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC,
    /// Set Crank Length
    CPP_CTNL_PT_SET_CRANK_LENGTH,
    /// Request Crank Length
    CPP_CTNL_PT_REQ_CRANK_LENGTH,
    /// Set Chain Length
    CPP_CTNL_PT_SET_CHAIN_LENGTH,
    /// Request Chain Length
    CPP_CTNL_PT_REQ_CHAIN_LENGTH,
    /// Set Chain Weight
    CPP_CTNL_PT_SET_CHAIN_WEIGHT,
    /// Request Chain Weight
    CPP_CTNL_PT_REQ_CHAIN_WEIGHT,
    /// Set Span Length
    CPP_CTNL_PT_SET_SPAN_LENGTH,
    /// Request Span Length
    CPP_CTNL_PT_REQ_SPAN_LENGTH,
    /// Start Offset Compensation
    CPP_CTNL_PT_START_OFFSET_COMP,
    /// Mask CP Measurement Characteristic Content
    CPP_CTNL_MASK_CP_MEAS_CH_CONTENT,
    /// Request Sampling Rate
    CPP_CTNL_REQ_SAMPLING_RATE,
    /// Request Factory Calibration Date
    CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE,

    /// Response Code
    CPP_CTNL_PT_RSP_CODE             = 32,
};

/// CPP Control Point Response Value
enum cpp_ctnl_pt_resp_val
{
    /// Reserved value
    CPP_CTNL_PT_RESP_RESERVED      = 0,

    /// Success
    CPP_CTNL_PT_RESP_SUCCESS,
    /// Operation Code Not Supported
    CPP_CTNL_PT_RESP_NOT_SUPP,
    /// Invalid Parameter
    CPP_CTNL_PT_RESP_INV_PARAM,
    /// Operation Failed
    CPP_CTNL_PT_RESP_FAILED,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// CP Measurement
struct cpp_cp_meas
{
    /// Flags
    uint16_t flags;
    /// Instantaneous Power
    int16_t inst_power;
    /// Pedal Power Balance
    uint8_t pedal_power_balance;
    /// Accumulated torque
    uint16_t accum_torque;
    /// Cumulative Wheel Revolutions
    int16_t cumul_wheel_rev;
    /// Last Wheel Event Time
    uint16_t last_wheel_evt_time;
    /// Cumulative Crank Revolution
    uint16_t cumul_crank_rev;
    /// Last Crank Event Time
    uint16_t last_crank_evt_time;
    /// Maximum Force Magnitude
    int16_t max_force_magnitude;
    /// Minimum Force Magnitude
    int16_t min_force_magnitude;
    /// Maximum Torque Magnitude
    int16_t max_torque_magnitude;
    /// Minimum Torque Magnitude
    int16_t min_torque_magnitude;
    /// Maximum Angle (12 bits)
    uint16_t max_angle;
    /// Minimum Angle (12bits)
    uint16_t min_angle;
    /// Top Dead Spot Angle
    uint16_t top_dead_spot_angle;
    /// Bottom Dead Spot Angle
    uint16_t bot_dead_spot_angle;
    ///Accumulated energy
    uint16_t accum_energy;

};

/// CP Measurement
struct cpp_cp_meas_ind
{
    /// Flags
    uint16_t flags;
    /// Instantaneous Power
    int16_t inst_power;
    /// Pedal Power Balance
    uint8_t pedal_power_balance;
    /// Accumulated torque
    uint16_t accum_torque;
    /// Cumulative Wheel Revolutions
    uint32_t cumul_wheel_rev;
    /// Last Wheel Event Time
    uint16_t last_wheel_evt_time;
    /// Cumulative Crank Revolution
    uint16_t cumul_crank_rev;
    /// Last Crank Event Time
    uint16_t last_crank_evt_time;
    /// Maximum Force Magnitude
    int16_t max_force_magnitude;
    /// Minimum Force Magnitude
    int16_t min_force_magnitude;
    /// Maximum Torque Magnitude
    int16_t max_torque_magnitude;
    /// Minimum Torque Magnitude
    int16_t min_torque_magnitude;
    /// Maximum Angle (12 bits)
    uint16_t max_angle;
    /// Minimum Angle (12bits)
    uint16_t min_angle;
    /// Top Dead Spot Angle
    uint16_t top_dead_spot_angle;
    /// Bottom Dead Spot Angle
    uint16_t bot_dead_spot_angle;
    ///Accumulated energy
    uint16_t accum_energy;

};


///CP Vector
struct cpp_cp_vector
{
    /// Flags
    uint8_t flags;
    /// Force-Torque Magnitude Array Length
    uint8_t nb;
    /// Cumulative Crank Revolutions
    uint16_t cumul_crank_rev;
    /// Last Crank Event Time
    uint16_t last_crank_evt_time;
    /// First Crank Measurement Angle
    uint16_t first_crank_meas_angle;
    ///Mutually excluded Force and Torque Magnitude Arrays
    int16_t  force_torque_magnitude[__ARRAY_EMPTY];
};

/// CP Control Point Request
struct cpp_ctnl_pt_req
{
    /// Operation Code
    uint8_t op_code;
    /// Value
    union cpp_ctnl_pt_req_val
    {
        /// Cumulative Value
        uint32_t cumul_val;
        /// Sensor Location
        uint8_t sensor_loc;
        /// Crank Length
        uint16_t crank_length;
        /// Chain Length
        uint16_t chain_length;
        /// Chain Weight
        uint16_t chain_weight;
        /// Span Length
        uint16_t span_length;
        /// Mask Content
        uint16_t mask_content;
    } value;
};

/// CP Control Point Response
struct cpp_ctnl_pt_rsp
{
    /// Requested Operation Code
    uint8_t req_op_code;
    /// Response Value
    uint8_t resp_value;

    ///Value
    union cpp_ctnl_pt_rsp_val
    {
        /// List of supported locations
        uint32_t supp_loc;
        /// Crank Length
        uint16_t crank_length;
        /// Chain Length
        uint16_t chain_length;
        /// Chain Weight
        uint16_t chain_weight;
        /// Span Length
        uint16_t span_length;
        /// Start Offset Compensation
        int16_t offset_comp;
        /// Sampling Rate Procedure
        uint8_t sampling_rate;
        /// Calibration date
        struct prf_date_time factory_calibration;
    } value;
};

/// @} cpp_common

#endif //(_CPP_COMMON_H_)
