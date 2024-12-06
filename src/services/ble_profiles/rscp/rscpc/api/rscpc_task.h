#ifndef _RSCPC_TASK_H_
#define _RSCPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup RSCPCTASK Running Speed and Cadence Profile Collector Task
 * @ingroup RSCPC
 * @brief Running Speed and Cadence Profile Collector Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rscp_common.h"
#include "rwip_task.h" // Task definitions

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Message IDs
enum rscpc_msg_ids
{
    /// Enable the Running Speed and Cadence Profile Collector task - at connection
    RSCPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_RSCPC),
    /// Enable the Running Speed and Cadence Profile Collector task - at connection
    RSCPC_ENABLE_RSP,

    /// Read the value of an attribute in the peer device database
    RSCPC_READ_CMD,
    /// Configure sending of notification or indication
    RSCPC_CFG_NTFIND_CMD,

    /// Configure the SC Control Point value
    RSCPC_CTNL_PT_CFG_REQ,
    /// SC Control Point value response
    RSCPC_CTNL_PT_CFG_RSP,

    /// Indicate that an attribute value has been received either upon notification or read response
    RSCPC_VALUE_IND,

    /// Complete Event Information
    RSCPC_CMP_EVT,

    /// Procedure Timeout Timer
    RSCPC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum rscpc_op_codes
{
    /// Reserved operation code
    RSCPC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    RSCPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    RSCPC_READ_OP_CODE,

    /// Wait for the Write Response after having written a Client Char. Cfg. Descriptor.
    RSCPC_CFG_NTF_IND_OP_CODE,
    /// Wait for the Write Response after having written the SC Control Point Char.
    RSCPC_CTNL_PT_CFG_WR_OP_CODE,
    /// Wait for the Indication Response after having written the SC Control Point Char.
    RSCPC_CTNL_PT_CFG_IND_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Running Speed and Cadence Service Characteristic Descriptors
enum rscpc_rscs_descs
{
    /// RSC Measurement Char. - Client Characteristic Configuration
    RSCPC_DESC_RSC_MEAS_CL_CFG,
    /// SC Control Point Char. - Client Characteristic Configuration
    RSCPC_DESC_SC_CTNL_PT_CL_CFG,

    RSCPC_DESC_MAX,

    RSCPC_DESC_MASK = 0x10,
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Running Speed and Cadence Service
 */
struct rscpc_rscs_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - RSC Measurement
    ///  - RSC Feature
    ///  - Sensor Location
    ///  - SC Control Point
    struct prf_char_inf chars[RSCP_RSCS_CHAR_MAX];

    /// Descriptor handles:
    ///  - RSC Measurement Client Cfg
    ///  - SC Control Point Client Cfg
    struct prf_char_desc_inf descs[RSCPC_DESC_MAX];
};

/// Parameters of the @ref RSCPC_ENABLE_REQ message
struct rscpc_enable_req
{
    /// Connection type
    uint8_t con_type;
    /// Existing handle values RSCS
    struct rscpc_rscs_content rscs;
};

/// Parameters of the @ref RSCPC_ENABLE_RSP message
struct rscpc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values RSCS
    struct rscpc_rscs_content rscs;
};

/// Parameters of the @ref RSCPC_READ_CMD message
struct rscpc_read_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref RSCPC_VALUE_IND message
struct rscpc_value_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union rscpc_value_tag
    {
        /// RSC Measurement
        struct rscp_rsc_meas rsc_meas;
        /// RSC Feature
        uint16_t sensor_feat;
        /// Sensor Location
        uint8_t sensor_loc;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
    } value;
};

/// Parameters of the @ref RSCPC_CFG_NTFIND_CMD message
struct rscpc_cfg_ntfind_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Descriptor code
    uint8_t desc_code;
    /// Ntf/Ind Configuration
    uint16_t ntfind_cfg;
};

/// Parameters of the @ref RSCPC_CTNL_PT_CFG_REQ message
struct rscpc_ctnl_pt_cfg_req
{
    /// Operation Code
    uint8_t operation;
    /// SC Control Point Request
    struct rscp_sc_ctnl_pt_req sc_ctnl_pt;
};

/// Parameters of the @ref RSCPC_CTNL_PT_CFG_RSP message
struct rscpc_ctnl_pt_cfg_rsp
{
    /// SC Control Point Response
    struct rscp_sc_ctnl_pt_rsp ctnl_pt_rsp;
};

/// Parameters of the @ref RSCPC_CMP_EVT message
struct rscpc_cmp_evt
{
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};

/// @} RSCPCTASK

#endif //(_RSCPC_TASK_H_)
