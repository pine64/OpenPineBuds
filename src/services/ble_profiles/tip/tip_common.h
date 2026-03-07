#ifndef _TIP_COMMON_H_
#define _TIP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup TIP Time Profile
 * @ingroup PROFILE
 * @brief Time Profile
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "prf_types.h"
#include <stdint.h>

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

///Adjust Reason Flags field bit values
enum
{
    TIPS_FLAG_MAN_TIME_UPDATE = 0x01,
    TIPS_FLAG_EXT_TIME_UPDATE = 0x02,
    TIPS_FLAG_CHG_TIME_ZONE = 0x04,
    TIPS_FLAG_DST_CHANGE = 0x08,
};

///Time Update Control Point Key values
enum
{
    TIPS_TIME_UPD_CTNL_PT_GET = 0x01,
    TIPS_TIME_UPD_CTNL_PT_CANCEL,
};

///Time Update State Current State Key Values
enum
{
    TIPS_TIME_UPD_STATE_IDLE = 0x00,
    TIPS_TIME_UPD_STATE_PENDING,
};

///Time Update State Result Key Values
enum
{
    TIPS_TIME_UPD_RESULT_SUCCESS = 0x00,
    TIPS_TIME_UPD_RESULT_CANCELED,
    TIPS_TIME_UPD_RESULT_NO_CONN,
    TIPS_TIME_UPD_RESULT_ERROR_RSP,
    TIPS_TIME_UPD_RESULT_TIMEOUT,
    TIPS_TIME_UPD_NOT_ATTEMPTED,
};

///Time Profile Supported Features bit flags
enum
{
    ///NDCS bit
    ///NDCS supported
    TIPS_NDCS_SUPPORTED            = 0x01,

    ///RTUS bit
    ///RTUS supported
    TIPS_RTUS_SUPPORTED            = 0x02,
};

enum
{
    TIP_RD_RSP        = 0x00,
    TIP_NTF            = 0x01,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Each of the following structure is related to a characteristic.
 */

/*
 * Current Time Service
 */

///Current Time Characteristic Structure
struct tip_curr_time
{
    /// Date time
    struct prf_date_time date_time;
    /// Day of the week
    uint8_t day_of_week;
    /// 1/256th of a second
    uint8_t fraction_256;
    /// Adjust reason
    uint8_t adjust_reason;
};

/**
 *
 * Time Zone Characteristic - UUID:0x2A0E
 * Min value : -48 (UTC-12:00), Max value : 56 (UTC+14:00)
 * -128 : Time zone offset is not known
 */
typedef int8_t tip_time_zone;

/**
 * DST Offset Characteristic - UUID:0x2A2D
 * Min value : 0, Max value : 8
 * 255 = DST is not known
 */
typedef uint8_t tip_dst_offset;

///Local Time Info Characteristic Structure - UUID:0x2A0F
struct tip_loc_time_info
{
    tip_time_zone time_zone;
    tip_dst_offset dst_offset;
};

/**
 * Time Source Characteristic - UUID:0x2A13
 * Min value : 0, Max value : 6
 * 0 = Unknown
 * 1 = Network Time Protocol
 * 2 = GPS
 * 3 = Radio Time Signal
 * 4 = Manual
 * 5 = Atomic Clock
 * 6 = Cellular Network
 */
typedef uint8_t tip_time_source;

/**
 * Time Accuracy Characteristic - UUID:0x2A12
 * Accuracy (drift) of time information in steps of 1/8 of a second (125ms) compared
 * to a reference time source. Valid range from 0 to 253 (0s to 31.5s). A value of
 * 254 means Accuracy is out of range (> 31.5s). A value of 255 means Accuracy is
 * unknown.
 */
typedef uint8_t tip_time_accuracy;

///Reference Time Info Characteristic Structure - UUID:0x2A14
struct tip_ref_time_info
{
    tip_time_source time_source;
    tip_time_accuracy time_accuracy;
    /**
     * Days since last update about Reference Source
     * Min value : 0, Max value : 254
     * 255 = 255 or more days
     */
    uint8_t days_update;
    /**
     * Hours since update about Reference Source
     * Min value : 0, Mac value : 23
     * 255 = 255 or more days (If Days Since Update = 255, then Hours Since Update shall
     * also be set to 255)
     */
    uint8_t hours_update;
};

/*
 * Next DST Change Service
 */
///Time With DST Characteristic Structure - UUID:0x2A11
struct tip_time_with_dst
{
    ///Date and Time of the Next DST Change
    struct prf_date_time date_time;
    ///DST Offset that will be in effect after this change
    tip_dst_offset dst_offset;
};

/*
 * Reference Time Update Service
 */
/**
 * Time Update Control Point Characteristic - UUID:0x2A16
 * The Time Update Control Point Characteristic enables a client to issue a command
 * request to update the time in the time server.
 * 0x01 = Get Reference Update = Forces the state machine to Update Pending and
 * starts the time update procedure
 * 0x02 = Cancel Reference Update = Forces the state machine to idle and stops the
 * attempt to receive a time update.
 */
typedef uint8_t tip_time_upd_contr_pt;

///Time Update State Characteristic Structure - UUID:0x2A17
struct tip_time_upd_state
{
    /**
     * The Time Update Status Characteristic exposes the status of the time update
     * process and the result of the last update in the server.
     */

    /**
     * Current State
     * Min value : 0, Max value = 1
     * 0 = Idle
     * 1 = Update Pending
     */
    uint8_t current_state;
    /**
     * Result
     * Min value : 0, Max Value : 5
     * 0 = Successful
     * 1 = Canceled
     * 2 = No Connection To Reference
     * 3 = Reference responded with an error
     * 4 = Timeout
     * 5 = Update not attempted after reset
     */
    uint8_t result;
};


/// @} tip_common

#endif /* _TIP_COMMON_H_ */
