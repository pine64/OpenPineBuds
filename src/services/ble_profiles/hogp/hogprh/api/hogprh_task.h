#ifndef _HOGPRH_TASK_H_
#define _HOGPRH_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HOGPRHTASK HID Over GATT Profile Report Host Task
 * @ingroup HOGPRH
 * @brief HID Over GATT Profile Report Host Task
 *
 * The HOGPRHTASK is responsible for handling the messages coming in and out of the
 * @ref HOGPRH monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "rwip_task.h" // Task definitions
#include "prf_types.h"
#include "hogp_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/// Maximal number of hids instances that can be handled
#define HOGPRH_NB_HIDS_INST_MAX              (2)
/// Maximal number of Report Char. that can be added in the DB for one HIDS - Up to 11
#define HOGPRH_NB_REPORT_INST_MAX            (5)

/// Maximal length of Report Map Char. Value
#define HOGPRH_REPORT_MAP_MAX_LEN            (512)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


enum hogprh_msg_id
{
    /// Start the HID Over GATT profile - at connection
    HOGPRH_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_HOGPRH),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    HOGPRH_ENABLE_RSP,

    /// Read Characteristic Value Request
    HOGPRH_READ_INFO_REQ,
    /// Read Characteristic Value Request
    HOGPRH_READ_INFO_RSP,

    /// Write/Configure peer device attribute Request
    HOGPRH_WRITE_REQ,
    /// Write/Configure peer device attribute Response
    HOGPRH_WRITE_RSP,

    /// Report value send to APP (after Notification)
    HOGPRH_REPORT_IND,
};


/// Characteristics
enum hogprh_chars
{
    /// Report Map
    HOGPRH_CHAR_REPORT_MAP,
    /// HID Information
    HOGPRH_CHAR_HID_INFO,
    /// HID Control Point
    HOGPRH_CHAR_HID_CTNL_PT,
    /// Protocol Mode
    HOGPRH_CHAR_PROTOCOL_MODE,
    /// Report
    HOGPRH_CHAR_REPORT,

    HOGPRH_CHAR_MAX = HOGPRH_CHAR_REPORT + HOGPRH_NB_REPORT_INST_MAX,
};


/// Characteristic descriptors
enum hogprh_descs
{
    /// Report Map Char. External Report Reference Descriptor
    HOGPRH_DESC_REPORT_MAP_EXT_REP_REF,
    /// Report Char. Report Reference
    HOGPRH_DESC_REPORT_REF,
    /// Report Client Config
    HOGPRH_DESC_REPORT_CFG = HOGPRH_DESC_REPORT_REF + HOGPRH_NB_REPORT_INST_MAX,

    HOGPRH_DESC_MAX = HOGPRH_DESC_REPORT_CFG + HOGPRH_NB_REPORT_INST_MAX,
};

/// Peer HID service info that can be read/write
enum hogprh_info
{
    /// Protocol Mode
    HOGPRH_PROTO_MODE,
    /// Report Map
    HOGPRH_REPORT_MAP,
    /// Report Map Char. External Report Reference Descriptor
    HOGPRH_REPORT_MAP_EXT_REP_REF,

    /// HID Information
    HOGPRH_HID_INFO,
    /// HID Control Point
    HOGPRH_HID_CTNL_PT,
    /// Report
    HOGPRH_REPORT,
    /// Report Char. Report Reference
    HOGPRH_REPORT_REF,
    /// Report Notification config
    HOGPRH_REPORT_NTF_CFG,

    HOGPRH_INFO_MAX,
};

/*
 * APIs Structure
 ****************************************************************************************
 */


///Structure containing the characteristics handles, value handles and descriptors
struct hogprh_content
{
    /// Service info
    struct prf_svc svc;

    /// Included service info
    struct prf_incl_svc incl_svc;

    /// Characteristic info:
    struct prf_char_inf chars[HOGPRH_CHAR_MAX];

    /// Descriptor handles:
    struct prf_char_desc_inf descs[HOGPRH_DESC_MAX];

    /// Number of Report Char. that have been found
    uint8_t report_nb;
};


/// Parameters of the @ref HOGPRH_ENABLE_REQ message
struct hogprh_enable_req
{
    /// Connection type
    uint8_t con_type;

    /// Number of HIDS instances
    uint8_t hids_nb;
    /// Existing handle values hids
    struct hogprh_content hids[HOGPRH_NB_HIDS_INST_MAX];
};

/// Parameters of the @ref HOGPRH_ENABLE_RSP message
struct hogprh_enable_rsp
{
    ///status
    uint8_t status;

    /// Number of HIDS instances
    uint8_t hids_nb;
    /// Existing handle values hids
    struct hogprh_content hids[HOGPRH_NB_HIDS_INST_MAX];
};


/// HID report info
struct hogprh_report
{
    /// Report Length
    uint8_t length;
    /// Report value
    uint8_t value[__ARRAY_EMPTY];
};

/// HID report Reference
struct hogprh_report_ref
{
    /// Report ID
    uint8_t id;
    /// Report Type
    uint8_t type;
};

/// HID report MAP info
struct hogprh_report_map
{
    /// Report MAP Length
    uint16_t length;
    /// Report MAP value
    uint8_t value[__ARRAY_EMPTY];
};

/// HID report MAP reference
struct hogprh_report_map_ref
{
    /// Reference UUID length
    uint8_t uuid_len;
    /// Reference UUID Value
    uint8_t uuid[__ARRAY_EMPTY];
};

/// Information data
union hogprh_data
{
    /// Protocol Mode
    ///  - info = HOGPRH_PROTO_MODE
    uint8_t proto_mode;

    /// HID Information value
    ///  - info = HOGPRH_HID_INFO
    struct hids_hid_info hid_info;

    /// HID Control Point value to write
    ///  - info = HOGPRH_HID_CTNL_PT
    uint8_t hid_ctnl_pt;

    /// Report information
    ///  - info = HOGPRH_REPORT
    struct hogprh_report report;

    ///Notification Configuration Value
    ///  - info = HOGPRH_REPORT_NTF_CFG
    uint16_t report_cfg;

    /// HID report Reference
    ///  - info = HOGPRH_REPORT_REF
    struct hogprh_report_ref report_ref;

    /// HID report MAP info
    ///  - info = HOGPRH_REPORT_MAP
    struct hogprh_report_map report_map;

    /// HID report MAP reference
    ///  - info = HOGPRH_REPORT_MAP_EXT_REP_REF
    struct hogprh_report_map_ref report_map_ref;
};



///Parameters of the @ref HOGPRH_READ_INFO_REQ message
struct hogprh_read_info_req
{
    ///Characteristic info @see enum hogprh_info
    uint8_t info;
    /// HID Service Instance - From 0 to HOGPRH_NB_HIDS_INST_MAX-1
    uint8_t hid_idx;
    /// HID Report Index: only relevant for:
    ///  - info = HOGPRH_REPORT
    ///  - info = HOGPRH_REPORT_REF
    ///  - info = HOGPRH_REPORT_NTF_CFG
    uint8_t report_idx;
};

///Parameters of the @ref HOGPRH_READ_INFO_RSP message
struct hogprh_read_info_rsp
{
    /// status of the request
    uint8_t status;
    ///Characteristic info @see enum hogprh_info
    uint8_t info;
    /// HID Service Instance - From 0 to HOGPRH_NB_HIDS_INST_MAX-1
    uint8_t hid_idx;
    /// HID Report Index: only relevant for:
    ///  - info = HOGPRH_REPORT
    ///  - info = HOGPRH_REPORT_REF
    ///  - info = HOGPRH_REPORT_NTF_CFG
    uint8_t report_idx;
    /// Information data
    union hogprh_data data;
};


///Parameters of the @ref HOGPRH_WRITE_REQ message
struct hogprh_write_req
{
    ///Characteristic info @see enum hogprh_info
    uint8_t info;
    /// HID Service Instance - From 0 to HOGPRH_NB_HIDS_INST_MAX-1
    uint8_t hid_idx;
    /// HID Report Index: only relevant for:
    ///  - info = HOGPRH_REPORT
    ///  - info = HOGPRH_REPORT_NTF_CFG
    uint8_t report_idx;
    /// Write type ( Write without Response True or Write Request)
    /// - only valid for HOGPRH_REPORT
    bool    wr_cmd;
    /// Information data
    union hogprh_data data;
};


///Parameters of the @ref HOGPRH_WRITE_RSP message
struct hogprh_write_rsp
{
    /// status of the request
    uint8_t status;
    ///Characteristic info @see enum hogprh_info
    uint8_t info;
    /// HID Service Instance - From 0 to HOGPRH_NB_HIDS_INST_MAX-1
    uint8_t hid_idx;
    /// HID Report Index: only relevant for:
    ///  - info = HOGPRH_REPORT
    ///  - info = HOGPRH_REPORT_REF
    ///  - info = HOGPRH_REPORT_NTF_CFG
    uint8_t report_idx;
};


///Parameters of the @ref HOGPRH_BOOT_REPORT_IND message
struct hogprh_report_ind
{
    /// HIDS Instance
    uint8_t hid_idx;
    /// HID Report Index
    uint8_t report_idx;
    /// Report data
    struct hogprh_report report;
};

/// @} HOGPRHTASK

#endif /* _HOGPRH_TASK_H_ */
