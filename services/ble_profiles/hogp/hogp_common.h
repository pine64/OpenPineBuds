#ifndef _HOGP_COMMON_H_
#define _HOGP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup HOGP HID Over GATT Profile
 * @ingroup PROFILE
 * @brief HID Over GATT Profile
 *
 * The HOGP module is the responsible block for implementing the HID Over GATT Profile
 * functionalities in the BLE Host.
 *
 * The HID Over GATT Profile defines the functionality required to allow HID reports and
 * HID data to be exchanged between a HID Device and a HID Host
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

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// HID Information bit values
enum hogp_info_bit
{
    /// Device capable of providing wake-up signal to a HID host
    HIDS_REMOTE_WAKE_CAPABLE           = 0x01,
    /// Normally connectable support bit
    HIDS_NORM_CONNECTABLE              = 0x02,
};

/// HID Control Point Characteristic value keys
enum hogp_ctnl_pt
{
    /// Suspend
    HOGP_CTNL_PT_SUSPEND            = 0x00,
    /// Exit suspend
    HOGP_CTNL_PT_EXIT_SUSPEND,
};

/// Protocol Mode Char. value Keys
enum hogp_boot_prot_mode
{
    /// Boot Protocol Mode
    HOGP_BOOT_PROTOCOL_MODE         = 0x00,
    /// Report Protocol Mode
    HOGP_REPORT_PROTOCOL_MODE,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Report Reference structure
struct hids_report_ref
{
    ///Report ID
    uint8_t report_id;
    ///Report Type
    uint8_t report_type;
};

/// HID Information structure
struct hids_hid_info
{
    /// bcdHID
    uint16_t bcdHID;
    /// bCountryCode
    uint8_t  bCountryCode;
    /// Flags
    uint8_t  flags;
};



/// @} hogp_common

#endif /* _HOGP_COMMON_H_ */
