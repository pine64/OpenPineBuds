#ifndef _HOGPD_TASK_H_
#define _HOGPD_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HOGPDTASK Task
 * @ingroup HOGPD
 * @brief HID Over GATT Profile Task.
 *
 * The HOGPD_TASK is responsible for handling the messages coming in and out of the
 * @ref HOGPD block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "rwip_task.h" // Task definitions
#include "hogp_common.h"
/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal number of HIDS that can be added in the DB
#define HOGPD_NB_HIDS_INST_MAX              (2)
/// Maximal number of Report Char. that can be added in the DB for one HIDS - Up to 11
#define HOGPD_NB_REPORT_INST_MAX            (5)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Messages for HID Over GATT Profile Device Role
enum hogpd_msg_id
{
    /// Restore bond data the HID Over GATT Profile Device Role Task
    HOGPD_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_HOGPD),
    /// Response of enabled request
    HOGPD_ENABLE_RSP,

    /// Request sending of a report to the host - notification
    HOGPD_REPORT_UPD_REQ,
    /// Response sending of a report to the host
    HOGPD_REPORT_UPD_RSP,


    /// Request from peer device to Read or update a report value
    HOGPD_REPORT_REQ_IND,
    /// Confirmation for peer device for Reading or Updating a report value
    HOGPD_REPORT_CFM,

    /// Inform Device APP that Protocol Mode Characteristic Value has been written on Device
    HOGPD_PROTO_MODE_REQ_IND,
    /// Confirm if the new protocol mode value
    HOGPD_PROTO_MODE_CFM,

    /// Inform Device APP that a Client Characteristic Configuration has been modified
    HOGPD_NTF_CFG_IND,
    /// Inform APP that HID Control Point Characteristic Value has been written on Device
    HOGPD_CTNL_PT_IND,
};

/// Report Char. Configuration Flag Values
enum hogpd_report_cfg
{
    /// Input Report
    HOGPD_CFG_REPORT_IN     = 0x01,
    /// Output Report
    HOGPD_CFG_REPORT_OUT    = 0x02,
    //HOGPD_CFG_REPORT_FEAT can be used as a mask to check Report type
    /// Feature Report
    HOGPD_CFG_REPORT_FEAT   = 0x03,
    /// Input report with Write capabilities
    HOGPD_CFG_REPORT_WR     = 0x10,
};

/// Features Flag Values
enum hogpd_cfg
{
    /// Keyboard Device
    HOGPD_CFG_KEYBOARD      = 0x01,
    /// Mouse Device
    HOGPD_CFG_MOUSE         = 0x02,
    /// Protocol Mode present
    HOGPD_CFG_PROTO_MODE    = 0x04,
    /// Extended Reference Present
    HOGPD_CFG_MAP_EXT_REF   = 0x08,
    /// Boot Keyboard Report write capability
    HOGPD_CFG_BOOT_KB_WR    = 0x10,
    /// Boot Mouse Report write capability
    HOGPD_CFG_BOOT_MOUSE_WR = 0x20,

    /// Valid Feature mask
    HOGPD_CFG_MASK          = 0x3F,

    /// Report Notification Enabled (to be shift for each report index)
    HOGPD_CFG_REPORT_NTF_EN = 0x40,
};

/// Type of reports
enum hogpd_report_type
{
    /// The Report characteristic is used to exchange data between a HID Device and a HID Host.
    HOGPD_REPORT,
    /// The Report Map characteristic
    HOGPD_REPORT_MAP,
    /// Boot Keyboard Input Report
    HOGPD_BOOT_KEYBOARD_INPUT_REPORT,
    /// Boot Keyboard Output Report
    HOGPD_BOOT_KEYBOARD_OUTPUT_REPORT,
    /// Boot Mouse Input Report
    HOGPD_BOOT_MOUSE_INPUT_REPORT,
};

/// type of operation requested by peer device
enum hogpd_op
{
    /// No operation
    HOGPD_OP_NO,
    /// Read report value
    HOGPD_OP_REPORT_READ,
    /// Modify/Set report value
    HOGPD_OP_REPORT_WRITE,
    /// Modify Protocol mode
    HOGPD_OP_PROT_UPDATE,
};

/*
 * APIs Structures
 ****************************************************************************************
 */
/// External Report Reference
struct hogpd_ext_ref
{
    /// External Report Reference - Included Service
    uint16_t inc_svc_hdl;
    /// External Report Reference - Characteristic UUID
    uint16_t rep_ref_uuid;
};

/// Database Creation Service Instance Configuration structure
struct hogpd_hids_cfg
{
    /// Service Features (@see enum hogpd_cfg)
    uint8_t svc_features;
    /// Number of Report Char. instances to add in the database
    uint8_t report_nb;
    /// Report Char. Configuration (@see enum hogpd_report_cfg)
    uint8_t report_char_cfg[HOGPD_NB_REPORT_INST_MAX];
    /// Report id number
    uint8_t report_id[HOGPD_NB_REPORT_INST_MAX];
    /// HID Information Char. Values
    struct hids_hid_info hid_info;
    /// External Report Reference
    struct hogpd_ext_ref ext_ref;

};

/// Parameters of the @ref HOGPD_CREATE_DB_REQ message
struct hogpd_db_cfg
{
    /// Number of HIDS to add
    uint8_t hids_nb;
    /// Initial configuration for each HIDS instance
    struct hogpd_hids_cfg cfg[HOGPD_NB_HIDS_INST_MAX];
};

/// Parameters of the @ref HOGPD_ENABLE_REQ message
struct hogpd_enable_req
{
    ///Connection index
    uint8_t conidx;
    /// Notification Configurations
    uint16_t ntf_cfg[HOGPD_NB_HIDS_INST_MAX];
};

/// Parameters of the @ref HOGPD_ENABLE_RSP message
struct hogpd_enable_rsp
{
    ///Connection index
    uint8_t conidx;
    /// status of the request
    uint8_t status;
};

///Parameters of the @ref HOGPD_NTF_CFG_IND message
struct hogpd_ntf_cfg_ind
{
    /// Connection Index
    uint8_t conidx;
    /// Notification Configurations
    uint16_t ntf_cfg[HOGPD_NB_HIDS_INST_MAX];
};


/// Inform Device APP that Protocol Mode Characteristic Value has been written on Device
struct hogpd_proto_mode_req_ind
{
    /// Connection Index
    uint8_t conidx;
    /// Operation requested (update protocol mode @see hogpd_op)
    uint8_t operation;
    /// HIDS Instance
    uint8_t hid_idx;
    /// New Protocol Mode Characteristic Value
    uint8_t proto_mode;
};

/// Confirm if the new protocol mode value
struct hogpd_proto_mode_cfm
{
    /// Connection Index
    uint8_t conidx;
    /// Status of the request
    uint8_t status;
    /// HIDS Instance
    uint8_t hid_idx;
    /// New Protocol Mode Characteristic Value
    uint8_t proto_mode;
};

/// HID Report Info
struct hogpd_report_info
{
    /// HIDS Instance
    uint8_t  hid_idx;
    /// type of report (@see enum hogpd_report_type)
    uint8_t  type;
    /// Report Length (uint8_t)
    uint16_t length;
    /// Report Instance - 0 for boot reports and report map
    uint8_t  idx;
    /// Report data
    uint8_t value[__ARRAY_EMPTY];
};


/// Request sending of a report to the host - notification
struct hogpd_report_upd_req
{
    /// Connection Index
    uint8_t conidx;
    /// Report Info
    struct hogpd_report_info report;
};

/// Response sending of a report to the host
struct hogpd_report_upd_rsp
{
    /// Connection Index
    uint8_t conidx;
    /// Status of the request
    uint8_t status;
};

/// Request from peer device to Read or update a report value
struct hogpd_report_req_ind
{
    /// Connection Index
    uint8_t conidx;
    /// Operation requested (read/write @see hogpd_op)
    uint8_t operation;
    /// Report Info
    struct hogpd_report_info report;
};

/// Confirmation for peer device for Reading or Updating a report value
struct hogpd_report_cfm
{
    /// Connection Index
    uint8_t conidx;
    /// Operation requested (read/write @see enum hogpd_op)
    uint8_t operation;
    /// Status of the request
    uint8_t status;
    /// Report Info
    struct hogpd_report_info report;
};


///Parameters of the @ref HOGPD_CTNL_PT_IND message
struct hogpd_ctnl_pt_ind
{
    /// Connection Index
    uint8_t conidx;
    /// HIDS Instance
    uint8_t hid_idx;
    /// New HID Control Point Characteristic Value
    uint8_t hid_ctnl_pt;
};



/// @} HOGPDTASK

#endif /* _HOGPD_TASK_H_ */
