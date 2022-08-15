#ifndef _HRPC_TASK_H_
#define _HRPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HRPCTASK Heart Rate Profile Collector Task
 * @ingroup HRPC
 * @brief Heart Rate Profile Collector Task
 *
 * The HRPCTASK is responsible for handling the messages coming in and out of the
 * @ref HRPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "hrp_common.h"
#include "rwip_task.h" // Task definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Characteristics
enum
{
    /// Heart Rate Measurement
    HRPC_CHAR_HR_MEAS,
    /// Body Sensor Location
    HRPC_CHAR_BODY_SENSOR_LOCATION,
    /// Heart Rate Control Point
    HRPC_CHAR_HR_CNTL_POINT,

    HRPC_CHAR_MAX,
};


/// Characteristic descriptors
enum
{
    /// Heart Rate Measurement client config
    HRPC_DESC_HR_MEAS_CLI_CFG,
    HRPC_DESC_MAX,
    HRPC_DESC_MASK = 0x10,
};

enum
{
    /// Start the Heart Rate profile - at connection
    HRPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_HRPC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    HRPC_ENABLE_RSP,

    /// Generic message to read a HRS or DIS characteristic value
    HRPC_RD_CHAR_REQ,
    ///Generic message for read responses for APP
    HRPC_RD_CHAR_RSP,

    ///Generic message for configuring the 2 characteristics that can be handled
    HRPC_CFG_INDNTF_REQ,
    ///Update IND/NTF config response
    HRPC_CFG_INDNTF_RSP,

    ///APP request for control point write (to reset Energy Expanded)
    HRPC_WR_CNTL_POINT_REQ,
    ///Write Control Point response
    HRPC_WR_CNTL_POINT_RSP,

    /// Heart Rate value send to APP
    HRPC_HR_MEAS_IND,
};

///Structure containing the characteristics handles, value handles and descriptors
struct hrs_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Heart Rate Measurement
    ///  - Body Sensor Location
    ///  - Heart Rate Control Point
    struct prf_char_inf chars[HRPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Heart Rate Measurement client cfg
    struct prf_char_desc_inf descs[HRPC_DESC_MAX];
};

/// Parameters of the @ref HRPC_ENABLE_REQ message
struct hrpc_enable_req
{
    ///Connection type
    uint8_t con_type;
    ///Existing handle values hrs
    struct hrs_content hrs;
};

/// Parameters of the @ref HRPC_ENABLE_RSP message
struct hrpc_enable_rsp
{
    ///status
    uint8_t status;
    ///Existing handle values hrs
    struct hrs_content hrs;
};

///Parameters of the @ref HRPC_CFG_INDNTF_REQ message
struct hrpc_cfg_indntf_req
{
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};


///Parameters of the @ref HRPC_CFG_INDNTF_RSP message
struct hrpc_cfg_indntf_rsp
{
    ///Status
    uint8_t  status;
};

///Parameters of the @ref HRPC_RD_CHAR_REQ message
struct hrpc_rd_char_req
{
    ///Characteristic value code
    uint8_t  char_code;
};

///Parameters of the @ref HRPC_RD_CHAR_RSP message
struct hrpc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};

///Parameters of the @ref HRPC_WR_CNTL_POINT_REQ message
struct hrpc_wr_cntl_point_req
{
    ///value
    uint8_t val;
};

///Parameters of the @ref HRPC_WR_CNTL_POINT_RSP message
struct hrpc_wr_cntl_point_rsp
{
    ///Status
    uint8_t  status;
};

///Parameters of the @ref HRPC_HR_MEAS_IND message
struct hrpc_hr_meas_ind
{
    ///Heart Rate measurement
    struct hrs_hr_meas meas_val;
};

/// @} HRPCTASK

#endif /* _HRPC_TASK_H_ */
