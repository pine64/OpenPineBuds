#ifndef _HRP_COMMON_H_
#define _HRP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup HRP Heart Rate Profile
 * @ingroup PROFILE
 * @brief Heart Rate Profile
 *
 * The HRP module is the responsible block for implementing the Heart Rate Profile
 * functionalities in the BLE Host.
 *
 * The Heart Rate Profile defines the functionality required in a device that allows
 * the user (Collector device) to configure and recover Heart Rate measurements from
 * a Heart Rate device.
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

/// maximum number of RR-Interval supported
#define HRS_MAX_RR_INTERVAL  (4)

/// Heart Rate Control Point Not Supported error code
#define HRS_ERR_HR_CNTL_POINT_NOT_SUPPORTED   (0x80)

///HRS codes for the 2 possible client configuration characteristic descriptors determination
enum
{
    /// Heart Rate Measurement
    HRS_HR_MEAS_CODE = 0x01,
    /// Energy Expended - Heart Rate Control Point
    HRS_HR_CNTL_POINT_CODE,
};


/// Heart Rate Measurement Flags field bit values
enum
{
    /// Heart Rate Value Format bit
    /// Heart Rate Value Format is set to UINT8. Units: beats per minute (bpm)
    HRS_FLAG_HR_8BITS_VALUE                = 0x00,
    /// Heart Rate Value Format is set to UINT16. Units: beats per minute (bpm)
    HRS_FLAG_HR_16BITS_VALUE               = 0x01,

    /// Sensor Contact Status bits
    /// Sensor Contact feature is not supported in the current connection
    HRS_FLAG_SENSOR_CCT_FET_NOT_SUPPORTED  = 0x00,
    /// Sensor Contact feature supported in the current connection
    HRS_FLAG_SENSOR_CCT_FET_SUPPORTED      = 0x04,
    /// Contact is not detected
    HRS_FLAG_SENSOR_CCT_NOT_DETECTED       = 0x00,
    /// Contact is detected
    HRS_FLAG_SENSOR_CCT_DETECTED           = 0x02,

    /// Energy Expended Status bit
    /// Energy Expended field is not present
    HRS_FLAG_ENERGY_EXPENDED_NOT_PRESENT   = 0x00,
    /// Energy Expended field is present. Units: kilo Joules
    HRS_FLAG_ENERGY_EXPENDED_PRESENT       = 0x08,

    /// RR-Interval bit
    /// RR-Interval values are not present.
    HRS_FLAG_RR_INTERVAL_NOT_PRESENT       = 0x00,
    /// One or more RR-Interval values are present. Units: 1/1024 seconds
    HRS_FLAG_RR_INTERVAL_PRESENT           = 0x10,
};

/// Heart Rate Feature Flags field bit values
enum
{
    /// Body Sensor Location support bit
    /// Body Sensor Location feature Not Supported
    HRS_F_BODY_SENSOR_LOCATION_NOT_SUPPORTED     = 0x00,
    /// Body Sensor Location feature Supported
    HRS_F_BODY_SENSOR_LOCATION_SUPPORTED         = 0x01,

    /// Energy Expended support bit
    /// Energy Expended feature Not Supported
    HRS_F_ENERGY_EXPENDED_NOT_SUPPORTED          = 0x00,
    /// Energy Expended feature Supported
    HRS_F_ENERGY_EXPENDED_SUPPORTED              = 0x02,
};

/// Body Sensor Location
enum
{
    HRS_LOC_OTHER,
    HRS_LOC_CHEST,
    HRS_LOC_WRIST,
    HRS_LOC_FINGER,
    HRS_LOC_HAND,
    HRS_LOC_EAR_LOBE,
    HRS_LOC_FOOT,
    HRS_LOC_MAX,
};



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Heart Rate measurement structure
struct hrs_hr_meas
{
    /// Flag
    uint8_t flags;
    /// Heart Rate Measurement Value
    uint16_t heart_rate;
    /// Energy Expended
    uint16_t energy_expended;
    /// RR-Interval numbers (max 4)
    uint8_t nb_rr_interval;
    /// RR-Intervals
    uint16_t rr_intervals[HRS_MAX_RR_INTERVAL];
};


/// @} hrp_common

#endif /* _HRP_COMMON_H_ */
