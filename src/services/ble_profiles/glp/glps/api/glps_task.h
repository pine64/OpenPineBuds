#ifndef _GLPS_TASK_H_
#define _GLPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup GLPSTASK Task
 * @ingroup GLPS
 * @brief Glucose Profile Task.
 *
 * The GLPSTASK is responsible for handling the messages coming in and out of the
 * @ref GLPS collector block of the BLE Host.
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
#include "glp_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Messages for Glucose Profile Sensor
enum glps_msg_id
{
    /// Start the Glucose Profile Sensor - at connection
    GLPS_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_GLPS),
    /// Confirm that Glucose Profile Sensor is started.
    GLPS_ENABLE_RSP,

    ///Inform APP of new configuration value
    GLPS_CFG_INDNTF_IND,

    /// Record Access Control Point Request
    GLPS_RACP_REQ_RCV_IND,

    /// Record Access Control Point Resp
    GLPS_SEND_RACP_RSP_CMD,

    /// Send Glucose measurement with context information
    GLPS_SEND_MEAS_WITH_CTX_CMD,
    /// Send Glucose measurement without context information
    GLPS_SEND_MEAS_WITHOUT_CTX_CMD,

    ///Inform that requested action has been performed
    GLPS_CMP_EVT,
};

///Parameters of the Glucose service database
struct glps_db_cfg
{
    /// Glucose Feature
    uint16_t features;
    /// Measurement context supported
    uint8_t meas_ctx_supported;
};


/// Parameters of the @ref GLPS_ENABLE_REQ message
struct glps_enable_req
{
    /// Glucose indication/notification configuration
    uint16_t evt_cfg;
};

///Parameters of the @ref GLPS_ENABLE_RSP message
struct glps_enable_rsp
{
    ///Status
    uint8_t status;
};

///Parameters of the @ref GLPS_CFG_INDNTF_IND message
struct glps_cfg_indntf_ind
{
    /// Glucose indication/notification configuration
    uint8_t evt_cfg;
};

///Parameters of the @ref GLPS_SEND_MEAS_WITH_CTX_CMD message
struct glps_send_meas_with_ctx_cmd
{
    /// Sequence Number
    uint16_t seq_num;
    /// Glucose measurement
    struct glp_meas meas;
    /// Glucose measurement context
    struct glp_meas_ctx ctx;
};


///Parameters of the @ref GLPS_SEND_MEAS_WITHOUT_CTX_CMD message
struct glps_send_meas_without_ctx_cmd
{
    /// Sequence Number
    uint16_t seq_num;
    /// Glucose measurement
    struct glp_meas meas;
};


///Parameters of the @ref GLPS_RACP_REQ_RCV_IND message
struct glps_racp_req_rcv_ind
{
    ///RACP Request
    struct glp_racp_req racp_req;
};

///Parameters of the @ref GLPS_SEND_RACP_RSP_CMD message
struct glps_send_racp_rsp_cmd
{
    ///Number of records.
    uint16_t num_of_record;
    /// operation code
    uint8_t op_code;
    ///Command status
    uint8_t status;
};

///Parameters of the @ref GLPS_CMP_EVT message
struct glps_cmp_evt
{
    /// completed request
    uint8_t request;
    ///Command status
    uint8_t status;
};


/// @} GLPSTASK

#endif /* _GLPS_TASK_H_ */
