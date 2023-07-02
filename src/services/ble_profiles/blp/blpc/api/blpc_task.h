#ifndef _BLPC_TASK_H_
#define _BLPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BLPCTASK Blood Pressure Profile Collector Task
 * @ingroup BLPC
 * @brief Blood Pressure Profile Collector Task
 *
 * The BLPCTASK is responsible for handling the messages coming in and out of the
 * @ref BLPC monitor block of the BLE Host.
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
#include "blp_common.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

enum
{
    /// Start the blood pressure profile - at connection
    BLPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_BLPC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    BLPC_ENABLE_RSP,

    /// Generic message to read a BPS or DIS characteristic value
    BLPC_RD_CHAR_REQ,
    ///Generic message for read responses for APP
    BLPC_RD_CHAR_RSP,

    ///Generic message for configuring the 2 characteristics that can be handled
    BLPC_CFG_INDNTF_REQ,
    ///Generic message for write characteristic response status to APP
    BLPC_CFG_INDNTF_RSP,

    /// Blood pressure value send to APP
    BLPC_BP_MEAS_IND,
};

/// Characteristics
enum
{
    /// Blood Pressure Measurement
    BLPC_CHAR_BP_MEAS,
    /// Intermediate Cuff pressure
    BLPC_CHAR_CP_MEAS,
    /// Blood Pressure Feature
    BLPC_CHAR_BP_FEATURE,

    BLPC_CHAR_MAX,
};

/// Characteristic descriptors
enum
{
    /// Blood Pressure Measurement client config
    BLPC_DESC_BP_MEAS_CLI_CFG,
    /// Intermediate Cuff pressure client config
    BLPC_DESC_IC_MEAS_CLI_CFG,
    BLPC_DESC_MAX,
    BLPC_DESC_MASK = 0x10,
};

///Structure containing the characteristics handles, value handles and descriptors
struct bps_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Blood Pressure Measurement
    ///  - Intermediate Cuff pressure
    ///  - Blood Pressure Feature
    struct prf_char_inf chars[BLPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Blood Pressure Measurement client cfg
    ///  - Intermediate Cuff pressure client cfg
    struct prf_char_desc_inf descs[BLPC_DESC_MAX];
};


/// Parameters of the @ref BLPC_ENABLE_REQ message
struct blpc_enable_req
{
    ///Connection type
    uint8_t con_type;
    ///Existing handle values bps
    struct bps_content bps;
};

/// Parameters of the @ref BLPC_ENABLE_RSP message
struct blpc_enable_rsp
{
    ///status
    uint8_t status;
    ///Existing handle values bps
    struct bps_content bps;
};


///Parameters of the @ref BLPC_RD_CHAR_REQ message
struct blpc_rd_char_req
{
    ///Characteristic value code
    uint8_t  char_code;
};

///Parameters of the @ref BLPC_RD_CHAR_RSP message
struct blpc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};

///Parameters of the @ref BLPC_CFG_INDNTF_REQ message
struct blpc_cfg_indntf_req
{
    ///Own code for differentiating between blood pressure and
    ///intermediate cuff pressure measurements
    uint8_t char_code;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

///Parameters of the @ref BLPC_CFG_INDNTF_RSP message
struct blpc_cfg_indntf_rsp
{
    ///Status
    uint8_t  status;
};

///Parameters of the @ref BLPC_BP_MEAS_IND message
struct blpc_bp_meas_ind
{
    /// Flag indicating if it is a intermediary cuff pressure measurement (0) or
    /// stable blood pressure measurement (1).
    uint16_t flag_interm_cp;
    ///Blood Pressure measurement
    struct bps_bp_meas meas_val;
};

/// @} BLPCTASK

#endif /* _BLPC_TASK_H_ */
