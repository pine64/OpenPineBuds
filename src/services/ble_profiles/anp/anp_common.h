#ifndef _ANP_COMMON_H_
#define _ANP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup ANP Alert Notification Profile
 * @ingroup ANP
 * @brief Alert Notification Profile
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

/// Error Code
// Command Not Supported - Protocol
#define ANP_CMD_NOT_SUPPORTED               (0xA0)
// Category Not Supported - Proprietary
#define ANP_CAT_NOT_SUPPORTED               (0xA1)

/// Alert Category ID Bit Mask 0 Masks
#define ANP_CAT_ID_SPL_ALERT_SUP            (0x01)
#define ANP_CAT_ID_EMAIL_SUP                (0x02)
#define ANP_CAT_ID_NEWS_SUP                 (0x04)
#define ANP_CAT_ID_CALL_SUP                 (0x08)
#define ANP_CAT_ID_MISSED_CALL_SUP          (0x10)
#define ANP_CAT_ID_SMS_MMS_SUP              (0x20)
#define ANP_CAT_ID_VOICE_MAIL_SUP           (0x40)
#define ANP_CAT_ID_SCHEDULE_SUP             (0x80)

/// Alert Category ID Bit Mask 1 Masks
#define ANP_CAT_ID_HIGH_PRTY_ALERT          (0x01)
#define ANP_CAT_ID_INST_MSG                 (0x02)
/// Alert Category ID 1 Mask
#define ANP_CAT_ID_1_MASK                   (ANP_CAT_ID_HIGH_PRTY_ALERT | ANP_CAT_ID_INST_MSG)

/// New Alert Characteristic Value - Text String Information Max Length
#define ANS_NEW_ALERT_STRING_INFO_MAX_LEN   (18)
/// New Alert Characteristic Value Max Length
#define ANS_NEW_ALERT_MAX_LEN (ANS_NEW_ALERT_STRING_INFO_MAX_LEN + 2)
/// Bonded data configured
#define ANPS_FLAG_CFG_PERFORMED_OK          (0x10)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Category ID Field Keys
enum
{
    /// Simple Alert
    CAT_ID_SPL_ALERT                    = 0,
    /// Email
    CAT_ID_EMAIL,
    /// News Feed
    CAT_ID_NEWS,
    /// Incoming Call
    CAT_ID_CALL,
    /// Missed Call
    CAT_ID_MISSED_CALL,
    /// SMS/MMS
    CAT_ID_SMS_MMS,
    /// Voice Mail
    CAT_ID_VOICE_MAIL,
    /// Schedule
    CAT_ID_SCHEDULE,
    /// High Priority Alert
    CAT_ID_HIGH_PRTY_ALERT,
    /// Instant Message
    CAT_ID_INST_MSG,

    CAT_ID_NB,

    /// All supported category
    CAT_ID_ALL_SUPPORTED_CAT            = 255,
};

/// Command ID Field Keys
enum
{
    /// Enable New Incoming Alert Notification
    CMD_ID_EN_NEW_IN_ALERT_NTF          = 0,
    /// Enable Unread Category Status Notification
    CMD_ID_EN_UNREAD_CAT_STATUS_NTF,
    /// Disable New Incoming Alert Notification
    CMD_ID_DIS_NEW_IN_ALERT_NTF,
    /// Disable Unread Category Status Notification
    CMD_ID_DIS_UNREAD_CAT_STATUS_NTF,
    /// Notify New Incoming Alert immediately
    CMD_ID_NTF_NEW_IN_ALERT_IMM,
    /// Notify Unread Category Status immediately
    CMD_ID_NTF_UNREAD_CAT_STATUS_IMM,

    CMD_ID_NB,
};

/// Characteristic Codes
enum
{
    /// Supported New Alert Category Characteristic
    ANS_SUPP_NEW_ALERT_CAT_CHAR,
    /// New Alert Characteristic
    ANS_NEW_ALERT_CHAR,
    /// Supported Unread Alert Category Characteristic
    ANS_SUPP_UNREAD_ALERT_CAT_CHAR,
    /// Unread Alert Status Characteristic
    ANS_UNREAD_ALERT_STATUS_CHAR,
    /// Alert Notification Control Point Characteristic
    ANS_ALERT_NTF_CTNL_PT_CHAR,

    ANS_CHAR_MAX,
};

/// Alert codes
enum anp_alert_codes
{
    /// New Alert
    ANP_NEW_ALERT      = 0,
    ANP_UNREAD_ALERT,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Alert Category ID Bit Mask Structure
struct anp_cat_id_bit_mask
{
    /// Category ID Bit Mask 0
    uint8_t cat_id_mask_0;
    /// Category ID Bit Mask 1
    uint8_t cat_id_mask_1;
};

/// New Alert Characteristic Value Structure
struct anp_new_alert
{
    /// Information String Length
    uint8_t info_str_len;

    /// Category ID
    uint8_t cat_id;
    /// Number of alerts
    uint8_t nb_new_alert;
    /// Text String Information
    uint8_t str_info[1];
};

/// Unread Alert Characteristic Value Structure
struct anp_unread_alert
{
    /// Category ID
    uint8_t cat_id;
    /// Number of alert
    uint8_t nb_unread_alert;
};

/// Alert Notification Control Point Characteristic Value Structure
struct anp_ctnl_pt
{
    /// Command ID
    uint8_t cmd_id;
    /// Category ID
    uint8_t cat_id;
};

/// @} anp_common

#endif //(_ANP_COMMON_H_)
