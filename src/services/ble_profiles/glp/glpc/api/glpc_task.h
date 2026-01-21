#ifndef _GLPC_TASK_H_
#define _GLPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup GLPCTASK Glucose Profile Collector Task
 * @ingroup GLPC
 * @brief Glucose Profile Collector Task
 *
 * The GLPCTASK is responsible for handling the messages coming in and out of the
 * @ref GLPC monitor block of the BLE Host.
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
#include "glp_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


enum glpc_msg_id
{
    /// Start the Glucose Profile - at connection
    GLPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_GLPC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    GLPC_ENABLE_RSP,

    /// Register to Glucose sensor notifications - request
    GLPC_REGISTER_REQ,
    /// Register to Glucose sensor notifications - confirmation
    GLPC_REGISTER_RSP,

    /// Read Glucose sensor features - request
    GLPC_READ_FEATURES_REQ,
    /// Read Glucose sensor features - response
    GLPC_READ_FEATURES_RSP,

    /// Record Access Control Point - Request
    GLPC_RACP_REQ,
    /// Record Access Control Point - Resp
    GLPC_RACP_RSP,

    /// Glucose measurement value received
    GLPC_MEAS_IND,

    /// Glucose measurement context received
    GLPC_MEAS_CTX_IND,


    /// Glucose Record Access Control Point Request Timeout
    GLPC_RACP_REQ_TIMEOUT,
};

/// Characteristics
enum
{
    /// Glucose Measurement
    GLPC_CHAR_MEAS,
    /// Glucose Measurement Context
    GLPC_CHAR_MEAS_CTX,
    /// Glucose Sensor Feature
    GLPC_CHAR_FEATURE,
    /// Record Access control point
    GLPC_CHAR_RACP,

    GLPC_CHAR_MAX,
};


/// Characteristic descriptors
enum
{
    /// Glucose Measurement client config
    GLPC_DESC_MEAS_CLI_CFG,
    /// Glucose Measurement context client config
    GLPC_DESC_MEAS_CTX_CLI_CFG,
    /// Record Access control point client config
    GLPC_DESC_RACP_CLI_CFG,

    GLPC_DESC_MAX,
};

///Structure containing the characteristics handles, value handles and descriptors
struct gls_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Glucose Measurement
    ///  - Glucose Measurement Context
    ///  - Glucose Feature
    ///  - Record Access Control Point
    struct prf_char_inf chars[GLPC_CHAR_MAX];

    /// Descriptor handles:info:
    ///  - Glucose Measurement client cfg
    ///  - Glucose Measurement Context client cfg
    ///  - Record Access Control Point client cfg
    struct prf_char_desc_inf descs[GLPC_DESC_MAX];
};


/// Parameters of the @ref GLPC_ENABLE_REQ message
struct glpc_enable_req
{
    /// Connection type
    uint8_t con_type;
    /// Existing handle values gls
    struct gls_content gls;
};

/// Parameters of the @ref GLPC_ENABLE_RSP message
struct glpc_enable_rsp
{
    /// status
    uint8_t status;
    /// Existing handle values gls
    struct gls_content gls;
};

/// Parameters of the @ref GLPC_REGISTER_REQ message
struct glpc_register_req
{
    /// Register or not Glucose measurement context notifications
    bool meas_ctx_en;
};
/// Parameters of the @ref GLPC_REGISTER_RSP message
struct glpc_register_rsp
{
    /// Status
    uint8_t  status;
};

/// Parameters of the @ref GLPC_READ_FEATURES_RSP message
struct glpc_read_features_rsp
{
    /// Glucose sensor features
    uint16_t features;
    /// Status
    uint8_t  status;
};

/// Parameters of the @ref GLPC_RACP_REQ message
struct glpc_racp_req
{
    /// Record Access Control Point Request
    struct glp_racp_req racp_req;
};
/// Parameters of the @ref GLPC_RACP_RSP message
struct glpc_racp_rsp
{
    ///record access control response
    struct glp_racp_rsp racp_rsp;
    /// Status
    uint8_t  status;
};

/// Parameters of the @ref GLPC_MEAS_IND message
struct glpc_meas_ind
{
    /// Sequence Number
    uint16_t seq_num;
    /// Glucose measurement
    struct glp_meas meas_val;
};

/// Parameters of the @ref GLPC_MEAS_CTX_IND message
struct glpc_meas_ctx_ind
{
    /// Sequence Number
    uint16_t seq_num;
    /// Glucose measurement
    struct glp_meas_ctx ctx;
};

/// @} GLPCTASK

#endif /* _GLPC_TASK_H_ */
