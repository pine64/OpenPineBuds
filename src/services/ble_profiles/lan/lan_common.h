#ifndef _LANP_COMMON_H_
#define _LANP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup LANP Location and Navigation Profile
 * @ingroup PROFILE
 * @brief Location and Navigation Profile
 *
 * The Location and Navigation Profile is used to enable a collector device in order to obtain
 * data from a Location and Navigation Sensor (LAN Sensor) that exposes the Location and Navigation Service.
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
/// Inappropriate Connection Parameters Error Code
#define LAN_ERROR_INAPP_CONNECT_PARAMS  (0x80)
/// Procedure Already in Progress Error Code
#define LAN_ERROR_PROC_IN_PROGRESS      (0x80)


#define LANP_LAN_LSPEED_MAX_PAIR_SIZE           (8)

/// LAN Location and Speed Value Max Length
#define LANP_LAN_LOC_SPEED_MAX_LEN              (28)
/// LAN Location and Speed Value Min Length
#define LANP_LAN_LOC_SPEED_MIN_LEN              (2)

/// LAN Position Quality Value Max Length
#define LANP_LAN_POSQ_MAX_LEN                   (16)
/// LAN Position Quality Value Min Length
#define LANP_LAN_POSQ_MIN_LEN                   (2)

/// LAN Control Point Value Request Max Length
#define LANP_LAN_LN_CNTL_PT_REQ_MAX_LEN         (5)
/// LAN Control Point Value Request Min Length
#define LANP_LAN_LN_CNTL_PT_REQ_MIN_LEN         (1)

/// LAN Control Point data Max Length (from 2 to max route name)
#define LANP_LAN_LN_CNTL_DATA_MAX_LEN           (128)
/// LAN Control Point Value Response Max Length
#define LANP_LAN_LN_CNTL_PT_RSP_MAX_LEN         (3 + LANP_LAN_LN_CNTL_DATA_MAX_LEN)
/// LAN Control Point Value Response Min Length
#define LANP_LAN_LN_CNTL_PT_RSP_MIN_LEN         (3)

/// LAN Navigation Value Max Length
#define LANP_LAN_NAVIGATION_MAX_LEN             (21)
/// LAN Navigation Value Min Length
#define LANP_LAN_NAVIGATION_MIN_LEN             (6)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// LANP Service Characteristics
enum lanp_lans_char
{
    /// LN Feature
    LANP_LANS_LN_FEAT_CHAR,
    /// Location and speed
    LANP_LANS_LOC_SPEED_CHAR,
    /// Position quality
    LANP_LANS_POS_Q_CHAR,
    /// LN Control Point
    LANP_LANS_LN_CTNL_PT_CHAR,
    /// Navigation
    LANP_LANS_NAVIG_CHAR,

    LANP_LANS_CHAR_MAX
};

/// LANP Feature Flags
enum lanp_feat_flags
{
    /// Instantaneous Speed Supported
    LANP_FEAT_INSTANTANEOUS_SPEED_SUPP              = 0x00000001,
    /// Total Distance Supported
    LANP_FEAT_TOTAL_DISTANCE_SUPP                   = 0x00000002,
    /// Location Supported
    LANP_FEAT_LOCATION_SUPP                         = 0x00000004,
    /// Elevation Supported
    LANP_FEAT_ELEVATION_SUPP                        = 0x00000008,
    /// Heading Supported
    LANP_FEAT_HEADING_SUPP                          = 0x00000010,
    /// Rolling Time Supported
    LANP_FEAT_ROLLING_TIME_SUPP                     = 0x00000020,
    /// UTC Time Supported
    LANP_FEAT_UTC_TIME_SUPP                         = 0x00000040,
    /// Remaining Distance Supported
    LANP_FEAT_REMAINING_DISTANCE_SUPP               = 0x00000080,
    /// Remaining Vertical Distance Supported
    LANP_FEAT_REMAINING_VERTICAL_DISTANCE_SUPP      = 0x00000100,
    /// Estimated Time of Arrival Supported
    LANP_FEAT_ESTIMATED_TIME_OF_ARRIVAL_SUPP        = 0x00000200,
    /// Number of Beacons in Solution Supported
    LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP    = 0x00000400,
    /// Number of Beacons in View Supported
    LANP_FEAT_NUMBER_OF_BEACONS_IN_VIEW_SUPP        = 0x00000800,
    /// Time to First Fix Supported
    LANP_FEAT_TIME_TO_FIRST_FIX_SUPP                = 0x00001000,
    /// Estimated Horizontal Position Error Supported
    LANP_FEAT_ESTIMATED_HOR_POSITION_ERROR_SUPP     = 0x00002000,
    /// Estimated Vertical Position Error Supported
    LANP_FEAT_ESTIMATED_VER_POSITION_ERROR_SUPP     = 0x00004000,
    /// Horizontal Dilution of Precision Supported
    LANP_FEAT_HOR_DILUTION_OF_PRECISION_SUPP        = 0x00008000,
    /// Vertical Dilution of Precision Supported
    LANP_FEAT_VER_DILUTION_OF_PRECISION_SUPP        = 0x00010000,
    /// Location and Speed Characteristic Content Masking Supported
    LANP_FEAT_LSPEED_CHAR_CT_MASKING_SUPP           = 0x00020000,
    /// Fix Rate Setting Supported
    LANP_FEAT_FIX_RATE_SETTING_SUPP                 = 0x00040000,
    /// Elevation Setting Supported
    LANP_FEAT_ELEVATION_SETTING_SUPP                = 0x00080000,
    /// Position Status Supported
    LANP_FEAT_POSITION_STATUS_SUPP                  = 0x00100000,

    /// All Supported
    LANP_FEAT_ALL_SUPP                              = 0x001FFFFF
};

/// LANP Location and speed Flags
enum lanp_lspeed_flags
{
    /// Instantaneous Speed Present
    LANP_LSPEED_INST_SPEED_PRESENT          = 0x0001,
    /// Total Distance Present
    LANP_LSPEED_TOTAL_DISTANCE_PRESENT      = 0x0002,
    /// Location Present
    LANP_LSPEED_LOCATION_PRESENT            = 0x0004,
    /// Elevation Present
    LANP_LSPEED_ELEVATION_PRESENT           = 0x0008,
    /// Heading Present
    LANP_LSPEED_HEADING_PRESENT             = 0x0010,
    /// Rolling Time Present
    LANP_LSPEED_ROLLING_TIME_PRESENT        = 0x0020,
    /// UTC Time Present
    LANP_LSPEED_UTC_TIME_PRESENT            = 0x0040,
    /// Position Status LSB
    LANP_LSPEED_POSITION_STATUS_LSB         = 0x0080,
    /// Position Status MSB
    LANP_LSPEED_POSITION_STATUS_MSB         = 0x0100,
    /// Speed and Distance format
    LANP_LSPEED_SPEED_AND_DISTANCE_FORMAT   = 0x0200,
    /// Elevation Source LSB
    LANP_LSPEED_ELEVATION_SOURCE_LSB        = 0x0400,
    /// Elevation Source MSB
    LANP_LSPEED_ELEVATION_SOURCE_MSB        = 0x0800,
    /// Heading Source
    LANP_LSPEED_HEADING_SOURCE              = 0x1000,

    /// All Present
    LANP_LSPEED_ALL_PRESENT                 = 0x1FFF
};

/// LANP Position quality Flags
enum lanp_posq_flags
{
    /// Number of Beacons in Solution Present
    LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT   = 0x0001,
    /// Number of Beacons in View Present
    LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT       = 0x0002,
    /// Time to First Fix Present
    LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT               = 0x0004,
    /// EHPE Present
    LANP_POSQ_EHPE_PRESENT                            = 0x0008,
    /// EVPE Present
    LANP_POSQ_EVPE_PRESENT                            = 0x0010,
    /// HDOP Present
    LANP_POSQ_HDOP_PRESENT                            = 0x0020,
    /// VDOP Present
    LANP_POSQ_VDOP_PRESENT                            = 0x0040,
    /// Position Status LSB
    LANP_POSQ_POSITION_STATUS_LSB                     = 0x0080,
    /// Position Status MSB
    LANP_POSQ_POSITION_STATUS_MSB                     = 0x0100,

    /// All Present
    LANP_POSQ_ALL_PRESENT                             = 0x01FF
};

/// LANP Control Point Keys
enum lanp_ln_ctnl_pt_code
{
    /// Reserved value
    LANP_LN_CTNL_PT_RESERVED               = 0,

    /// Set Cumulative Value
    LANP_LN_CTNL_PT_SET_CUMUL_VALUE,
    /// Mask Location and Speed Characteristic Content
    LANP_LN_CTNL_PT_MASK_LSPEED_CHAR_CT,
    /// Navigation Control
    LANP_LN_CTNL_PT_NAVIGATION_CONTROL,
    /// Request Number of Routes
    LANP_LN_CTNL_PT_REQ_NUMBER_OF_ROUTES,
    /// Request Name of Route
    LANP_LN_CTNL_PT_REQ_NAME_OF_ROUTE,
    /// Select Route
    LANP_LN_CTNL_PT_SELECT_ROUTE,
    /// Set Fix Rate
    LANP_LN_CTNL_PT_SET_FIX_RATE,
    /// Set Elevation
    LANP_LN_CTNL_PT_SET_ELEVATION,
    /// Response Code
    LANP_LN_CTNL_PT_RESPONSE_CODE          = 32
};

/// LANP Control Point Response Value
enum lanp_ctnl_pt_resp_val
{
    /// Reserved value
    LANP_LN_CTNL_PT_RESP_RESERVED      = 0,

    /// Success
    LANP_LN_CTNL_PT_RESP_SUCCESS,
    /// Operation Code Not Supported
    LANP_LN_CTNL_PT_RESP_NOT_SUPP,
    /// Invalid Parameter
    LANP_LN_CTNL_PT_RESP_INV_PARAM,
    /// Operation Failed
    LANP_LN_CTNL_PT_RESP_FAILED,
};

/// LANP Navigation Control parameter
enum lanp_navi_control
{
    /// Stop Navigation
    LANP_LN_CTNL_STOP_NAVI      = 0x00,
    /// Start Navigation
    LANP_LN_CTNL_START_NAVI     = 0x01,
    /// Pause Navigation
    LANP_LN_CTNL_PAUSE_NAVI     = 0x02,
    /// Resume Navigation
    LANP_LN_CTNL_RESUME_NAVI    = 0x03,
    /// Skip waypoint on route
    LANP_LN_CTNL_SKIP_WPT       = 0x04,
    /// Start Navigation from the nearest waypoint
    LANP_LN_CTNL_START_NST_WPT  = 0x05,
};

/// LANP Navigation flags
enum lanp_navi_flags
{
    /// Remaining Distance Present
    LANP_NAVI_REMAINING_DIS_PRESENT             = 0x0001,
    /// Remaining Vertical Distance Present
    LANP_NAVI_REMAINING_VER_DIS_PRESENT         = 0x0002,
    /// Estimated Time of Arrival Present
    LANP_NAVI_ESTIMATED_TIME_OF_ARRIVAL_PRESENT = 0x0004,
    /// Position Status lsb
    LANP_NAVI_POSITION_STATUS_LSB               = 0x0008,
    /// Position Status msb
    LANP_NAVI_POSITION_STATUS_MSB               = 0x0010,
    /// Heading Source
    LANP_NAVI_HEADING_SOURCE                    = 0x0020,
    /// Navigation Indicator Type
    LANP_NAVI_NAVIGATION_INDICATOR_TYPE         = 0x0040,
    /// Waypoint Reached
    LANP_NAVI_WAYPOINT_REACHED                  = 0x0080,
    /// Destination Reached
    LANP_NAVI_DESTINATION_REACHED               = 0x0100,
    /// ALl present
    LANP_NAVI_ALL_PRESENT                       = 0x01FF

};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Location and Speed
struct lanp_loc_speed
{
    /// Flags
    uint16_t flags;
    /// Instantaneous Speed
    uint16_t inst_speed;
    /// Total distance
    uint32_t total_dist;
    /// Location - Latitude
    int32_t latitude;
    /// Location - Longitude
    int32_t longitude;
    /// Elevation
    int32_t elevation;
    /// Heading
    uint16_t heading;
    /// Rolling time
    uint8_t rolling_time;
    /// UTC Time
    struct prf_date_time date_time;
};

/// LAN Position quality
struct lanp_posq
{
    /// Flags
    uint16_t flags;
    /// Time to First Fix
    uint16_t time_first_fix;
    /// EHPE
    uint32_t ehpe;
    /// EVPE
    uint32_t evpe;
    /// Number of Beacons in Solution
    uint8_t n_beacons_solution;
    /// Number of Beacons in view
    uint8_t n_beacons_view;
    /// HDOP
    uint8_t hdop;
    /// VDOP
    uint8_t vdop;
};

/// LN Control Point Request
struct lan_ln_ctnl_pt_req
{
    /// Operation code
    uint8_t op_code;

    /// Value
    union lanp_ln_ctnl_pt_req_val
    {
        /// Cumulative Value (24 bits)
        uint32_t cumul_val;
        /// Mask Content
        uint16_t mask_content;
        /// Navigation Control
        uint8_t control_value;
        /// Route number
        uint16_t route_number;
        /// Fix rate
        uint8_t fix_rate;
        /// Elevation
        int32_t elevation;
    } value;
};

/// LAN Control Point Response
struct lanp_ln_ctnl_pt_rsp
{
    /// Operation code
    uint8_t req_op_code;
    /// Response Value
    uint8_t resp_value;

    /// Value
    union lanp_ctnl_pt_rsp_val
    {
        /// Number of routes
        uint16_t number_of_routes;

        struct lanp_route_name
        {
            uint8_t length;
            /// Name of Route UTF-8
            uint8_t name[__ARRAY_EMPTY];
        } route;
    } value;
};

/// LAN Navigation
struct lanp_navigation
{
    /// Flags
    uint16_t flags;
    /// Bearing
    uint16_t bearing;
    /// Heading
    uint16_t heading;
    /// Remaining Distance (24 bits)
    uint32_t remaining_distance;
    /// Remaining Vertical Distance (24 bits)
    uint32_t remaining_ver_distance;
    /// Estimated Time of Arrival
    struct prf_date_time estimated_arrival_time;
};


/// @} lanp_common

#endif //(_LANP_COMMON_H_)
