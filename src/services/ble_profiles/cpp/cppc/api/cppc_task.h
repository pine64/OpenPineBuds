#ifndef _CPPC_TASK_H_
#define _CPPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CPPCTASK Cycling Power Profile Collector Task
 * @ingroup CPPC
 * @brief Cycling Power Profile Collector Task
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"

#include "rwip_task.h" // Task definitions
#include "prf_types.h"


/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Cycling Power Service Characteristic Descriptors
enum cppc_cps_descs
{
    /// CP Measurement Char. - Client Characteristic Configuration
    CPPC_DESC_CP_MEAS_CL_CFG,
    /// CP Measurement Char. - Server Characteristic Configuration
    CPPC_DESC_CP_MEAS_SV_CFG,
    /// CP Vector Char. - Client Characteristic Configuration
    CPPC_DESC_VECTOR_CL_CFG,
    /// Control Point Char. - Client Characteristic Configuration
    CPPC_DESC_CTNL_PT_CL_CFG,

    CPPC_DESC_MAX,

    CPPC_DESC_MASK = 0x10,
};

/// Message IDs
enum cppc_msg_ids
{
    /// Enable the Cycling Power Profile Collector task - at connection
    CPPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_CPPC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    CPPC_ENABLE_RSP,

    /// Read the value of an attribute in the peer device database
    CPPC_READ_CMD,

    /// Configure sending of notification or indication
    CPPC_CFG_NTFIND_CMD,

    /// Configure the SC Control Point value
    CPPC_CTNL_PT_CFG_REQ,
    /// Indicate that an control point response has been triggered by peer device
    CPPC_CTNL_PT_RSP,

    /// Indicate that an attribute value has been received either upon notification or read response
    CPPC_VALUE_IND,

    /// Complete Event Information
    CPPC_CMP_EVT,

    /// Procedure Timeout Timer
    CPPC_TIMEOUT_TIMER_IND,
};

/// Operation Codes
enum cppc_op_code
{
    /// Reserved operation code
    CPPC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    CPPC_ENABLE_OP_CODE,
    /// Read attribute value Procedure
    CPPC_READ_OP_CODE,

    /// Wait for the Write Response after having written a Client Char. Cfg. Descriptor.
    CPPC_CFG_NTF_IND_OP_CODE,

    /// Wait for the Write Response after having written the Control Point Char.
    CPPC_CTNL_PT_CFG_WR_OP_CODE,
    /// Wait for the Indication Response after having written the Control Point Char.
    CPPC_CTNL_PT_CFG_IND_OP_CODE,
};

/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Cycling Power Service
 */
struct cppc_cps_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - CP Measurement
    ///  - CP Feature
    ///  - Sensor Location
    ///  - Vector
    ///  - SC Control Point
    struct prf_char_inf chars[CPP_CPS_CHAR_MAX];

    /// Descriptor handles:
    ///  - CP Measurement Client Cfg
    ///  - Vector Client Cfg
    ///  - Control Point Client Cfg
    struct prf_char_desc_inf descs[CPPC_DESC_MAX];
};

/// Parameters of the @ref CPPC_ENABLE_REQ message
struct cppc_enable_req
{
    /// Connection type
    uint8_t con_type;
    /// Existing handle values CPS
    struct cppc_cps_content cps;
};

/// Parameters of the @ref CPPC_ENABLE_RSP message
struct cppc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values CPS
    struct cppc_cps_content cps;
};

/// Parameters of the @ref CPPC_READ_CMD message
struct cpps_read_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref CPPC_CFG_NTFIND_CMD message
struct cppc_cfg_ntfind_cmd
{
    /// Operation Code
    uint8_t operation;
    /// Descriptor code
    uint8_t desc_code;
    /// Ntf/Ind Configuration
    uint16_t ntfind_cfg;
};

/// Parameters of the @ref CPPC_CTNL_PT_CFG_REQ message
struct cppc_ctnl_pt_cfg_req
{
    /// Operation Code
    uint8_t operation;
    /// SC Control Point Request
    struct cpp_ctnl_pt_req ctnl_pt;
};

/// Parameters of the @ref CPPC_VALUE_IND message
struct cppc_value_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union cppc_value_tag
    {
        /// CP Feature
        uint32_t sensor_feat;
        /// CP Measurement
        struct cpp_cp_meas cp_meas;
        /// Sensor Vector
        struct cpp_cp_vector cp_vector;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
        /// Sensor Location
        uint8_t sensor_loc;
    } value;
};


/// Parameters of the @ref CPPC_VALUE_IND message
struct cppc_value_meas_ind
{
    /// Attribute Code
    uint8_t att_code;
    /// Value
    union cppc_value_tag2
    {
        /// CP Feature
        uint32_t sensor_feat;
        /// CP Measurement
        struct cpp_cp_meas_ind cp_meas;
        /// Sensor Vector
        struct cpp_cp_vector cp_vector;
        /// Client Characteristic Configuration Descriptor Value
        uint16_t ntf_cfg;
        /// Sensor Location
        uint8_t sensor_loc;
    } value;
};




/// Parameters of the @ref CPPC_CTNL_PT_RSP message
struct cppc_ctnl_pt_rsp
{
    /// SC Control Point Response
    struct cpp_ctnl_pt_rsp rsp;
};

/// Parameters of the @ref CPPC_CMP_EVT message
struct cppc_cmp_evt
{
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};

/// @} CPPCTASK

#endif //(_CPPC_TASK_H_)
