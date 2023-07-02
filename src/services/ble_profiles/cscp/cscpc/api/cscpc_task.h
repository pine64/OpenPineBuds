#ifndef _CSCPC_TASK_H_
#define _CSCPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CSCPCTASK Cycling Speed and Cadence Profile Collector Task
 * @ingroup CSCPC
 * @brief Cycling Speed and Cadence Profile Collector Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cscp_common.h"

#include "rwip_task.h" // Task definitions
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Message IDs
enum cscpc_msg_ids
{
    /// Enable the Cycling Speed and Cadence Profile Collector task - at connection
    CSCPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_CSCPC),
    /// Enable the Cycling Speed and Cadence Profile Collector task - at connection
    CSCPC_ENABLE_RSP,

    /// Read the value of an attribute in the peer device database
    CSCPC_READ_CMD,

    /// Configure sending of notification or indication
    CSCPC_CFG_NTFIND_CMD,

    /// Configure the SC Control Point value
    CSCPC_CTNL_PT_CFG_REQ,
    /// SC Control Point value response
    CSCPC_CTNL_PT_RSP,

    /// Indicate that an attribute value has been received either upon notification or read response
    CSCPC_VALUE_IND,

    /// Complete Event Information
    CSCPC_CMP_EVT,

    /// Procedure Timeout Timer
    CSCPC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum cscpc_op_codes
{
    /// Reserved operation code
    CSCPC_RESERVED_OP_CODE  = 0x00,

    /// Read attribute value Procedure
    CSCPC_READ_OP_CODE,

    /// Wait for the Write Response after having written a Client Char. Cfg. Descriptor.
    CSCPC_CFG_NTF_IND_OP_CODE,
    /// Wait for the Write Response after having written the SC Control Point Char.
    CSCPC_CTNL_PT_CFG_WR_OP_CODE,
    /// Wait for the Indication Response after having written the SC Control Point Char.
    CSCPC_CTNL_PT_CFG_IND_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Service Characteristic Descriptors
enum cscpc_cscs_descs
{
    /// CSC Measurement Char. - Client Characteristic Configuration
    CSCPC_DESC_CSC_MEAS_CL_CFG,
    /// SC Control Point Char. - Client Characteristic Configuration
    CSCPC_DESC_SC_CTNL_PT_CL_CFG,

    CSCPC_DESC_MAX,

    CSCPC_DESC_MASK = 0x10,
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Cycling Speed and Cadence Service
 */
struct cscpc_cscs_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - CSC Measurement
    ///  - CSC Feature
    ///  - Sensor Location
    ///  - SC Control Point
    struct prf_char_inf chars[CSCP_CSCS_CHAR_MAX];

    /// Descriptor handles:
    ///  - CSC Measurement Client Cfg
    ///  - SC Control Point Client Cfg
    struct prf_char_desc_inf descs[CSCPC_DESC_MAX];
};

/// Parameters of the @ref CSCPC_ENABLE_REQ message
struct cscpc_enable_req
{
    /// Connection type
    uint8_t con_type;
    /// Existing handle values CSCS
    struct cscpc_cscs_content cscs;
};

/// Parameters of the @ref CSCPC_ENABLE_RSP message
struct cscpc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values CPS
    struct cscpc_cscs_content cscs;
};

/// Parameters of the @ref CSCPC_READ_CMD message
struct cscpc_read_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref CSCPC_CFG_NTFIND_CMD message
struct cscpc_cfg_ntfind_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Descriptor code
    uint8_t desc_code;
    /// Ntf/Ind Configuration
    uint16_t ntfind_cfg;
};

/// Parameters of the @ref CSCPC_CTNL_PT_CFG_REQ message
struct cscpc_ctnl_pt_cfg_req
{
    /// Operation Code
    uint8_t operation;
    /// SC Control Point Request
    struct cscp_sc_ctnl_pt_req sc_ctnl_pt;
};

/// Parameters of the @ref CSCPC_CTNL_PT_RSP message
struct cscpc_ctnl_pt_rsp
{
    /// SC Control Point Response
    struct cscp_sc_ctnl_pt_rsp ctnl_pt_rsp;
};


/// Parameters of the @ref CSCPC_VALUE_IND message
struct cscpc_value_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union cscpc_value_tag
    {
        /// CSC Measurement
        struct cscp_csc_meas csc_meas;
        /// CSC Feature
        uint16_t sensor_feat;
        /// Sensor Location
        uint8_t sensor_loc;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
    } value;
};

/// Parameters of the @ref CSCPC_CMP_EVT message
struct cscpc_cmp_evt
{
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};

/// @} CSCPCTASK

#endif //(_CSCPC_TASK_H_)
