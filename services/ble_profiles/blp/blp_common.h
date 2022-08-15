#ifndef _BLP_COMMON_H_
#define _BLP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup BLP Blood Pressure Profile
 * @ingroup PROFILE
 * @brief Blood Pressure Profile
 *
 * The BLP module is the responsible block for implementing the Blood Pressure Profile
 * functionalities in the BLE Host.
 *
 * The Blood Pressure Profile defines the functionality required in a device that allows
 * the user (Collector device) to configure and recover blood pressure measurements from
 * a blood pressure device.
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

///BLPC codes for the 2 possible client configuration characteristic descriptors determination in BPS
enum
{
    ///Blood Pressure Measurement
    BPS_BP_MEAS_CODE = 0x01,
    ///Intermediate Cuff Pressure Measurement
    BPS_INTERM_CP_CODE,
};

/// Blood Pressure Measurement Flags field bit values
enum
{
    /// pressure in millimetre of mercury
    BPS_FLAG_MMHG                  = 0x00,
    /// Pressure in pascal
    BPS_FLAG_KPA                   = 0x01,
    /// Time Stamp present
    BPS_FLAG_TIME_STAMP_PRESENT    = 0x02,
    /// Pulse Rate present
    BPS_FLAG_PULSE_RATE_PRESENT    = 0x04,
    /// User ID present
    BPS_FLAG_USER_ID_PRESENT       = 0x08,
    /// Measurement Status present
    BPS_FLAG_MEAS_STATUS_PRESENT   = 0x10,
};

/// Blood Pressure Measurement Status Flags field bit values
enum
{
    /// Body Movement Detection Flag
    /// No body movement
    BPS_STATE_NO_BODY_MVMT_DETECTED       = 0x0000,
    /// Body movement during measurement
    BPS_STATE_BODY_MVMT_DETECTED          = 0x0001,

    /// Cuff Fit Detection Flag
    /// Cuff fits properly
    BPS_STATE_CUFF_FITS_PROPERLY          = 0x0000,
    /// Cuff too loose
    BPS_STATE_CUFF_TOO_LOOSE              = 0x0002,

    /// Irregular Pulse Detection Flag
    /// No irregular pulse detected
    BPS_STATE_NO_IRREGULAR_PULSE_DETECTED = 0x0000,
    /// Irregular pulse detected
    BPS_STATE_IRREGULAR_PULSE_DETECTED    = 0x0004,

    /// Pulse Rate Range Detection Flags
    /// Pulse rate is within the range
    BPS_STATE_PR_IN_RANGE                 = 0x0000,
    /// Pulse rate exceeds upper limit
    BPS_STATE_PR_IN_TOO_HIGH              = 0x0008,
    /// Pulse rate is less than lower limit
    BPS_STATE_PR_IN_TOO_LOW               = 0x0010,

    /// Measurement Position Detection Flag
    /// Proper measurement position
    BPS_STATE_MEAS_POS_OK                 = 0x0000,
    /// Improper measurement position
    BPS_STATE_MEAS_POS_KO                 = 0x0020,
};

/// Blood Pressure Feature Flags field bit values
enum
{
    ///Body Movement Detection Support bit
    ///Body Movement Detection feature not supported
    BPS_F_BODY_MVMT_DETECT_NOT_SUPPORTED         = 0x0000,
    ///Body Movement Detection feature supported
    BPS_F_BODY_MVMT_DETECT_SUPPORTED             = 0x0001,

    ///Cuff Fit Detection Support bit
    ///Cuff Fit Detection feature not supported
    BPS_F_CUFF_FIT_DETECT_NOT_SUPPORTED          = 0x0000,
    ///Cuff Fit Detection feature supported
    BPS_F_CUFF_FIT_DETECT_SUPPORTED              = 0x0002,

    ///Irregular Pulse Detection Support bit
    ///Irregular Pulse Detection feature not supported
    BPS_F_IRREGULAR_PULSE_DETECT_NOT_SUPPORTED   = 0x0000,
    ///Irregular Pulse Detection feature supported
    BPS_F_IRREGULAR_PULSE_DETECT_SUPPORTED       = 0x0004,

    ///Pulse Rate Range Detection Support bit
    ///Pulse Rate Range Detection feature not supported
    BPS_F_PULSE_RATE_RANGE_DETECT_NOT_SUPPORTED  = 0x0000,
    ///Pulse Rate Range Detection feature supported
    BPS_F_PULSE_RATE_RANGE_DETECT_SUPPORTED      = 0x0008,

    ///Measurement Position Detection Support bit
    ///Measurement Position Detection feature not supported
    BPS_F_MEAS_POS_DETECT_NOT_SUPPORTED          = 0x0000,
    ///Measurement Position Detection feature supported
    BPS_F_MEAS_POS_DETECT_SUPPORTED              = 0x0010,

    ///Multiple Bond Support bit
    ///Multiple Bonds not supported
    BPS_F_MULTIPLE_BONDS_NOT_SUPPORTED           = 0x0000,
    ///Multiple Bonds supported
    BPS_F_MULTIPLE_BONDS_SUPPORTED               = 0x0020,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Blood Pressure measurement structure
struct bps_bp_meas
{
    /// Flag
    uint8_t  flags;
    /// User ID
    uint8_t  user_id;
    /// Systolic (mmHg/kPa)
    prf_sfloat systolic;
    /// Diastolic (mmHg/kPa)
    prf_sfloat diastolic;
    /// Mean Arterial Pressure (mmHg/kPa)
    prf_sfloat mean_arterial_pressure;
    /// Pulse Rate
    prf_sfloat pulse_rate;
    /// Measurement Status
    uint16_t meas_status;
    /// Time stamp
    struct prf_date_time  time_stamp;
};

/// @} blp_common

#endif /* _BLP_COMMON_H_ */
