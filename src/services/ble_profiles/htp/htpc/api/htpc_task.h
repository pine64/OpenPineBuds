#ifndef HTPC_TASK_H_
#define HTPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HTPCTASK Health Thermometer Profile Collector Task
 * @ingroup HTPC
 * @brief Health Thermometer Profile Collector Task
 *
 * The HTPCTASK is responsible for handling the messages coming in and out of the
 * @ref HTPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_task.h" // Task definitions
#include "htp_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */


///  Message id
enum htpc_msg_id
{
    /// Start the Health Thermometer Collector profile - at connection
    HTPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_HTPC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    HTPC_ENABLE_RSP,


    /// Write Health Thermometer Notification Configuration Value request
    HTPC_HEALTH_TEMP_NTF_CFG_REQ,
    /// Write Health Thermometer Notification Configuration Value response
    HTPC_HEALTH_TEMP_NTF_CFG_RSP,

    ///APP request for measurement interval write
    HTPC_WR_MEAS_INTV_REQ,
    ///APP request for measurement interval write
    HTPC_WR_MEAS_INTV_RSP,

    /// Temperature value received from peer sensor
    HTPC_TEMP_IND,
    /// Measurement interval update indication received from peer sensor
    HTPC_MEAS_INTV_IND,

    /// Generic message to read a HTP characteristic value
    HTPC_RD_CHAR_REQ,
    /// Read HTP characteristic value response
    HTPC_RD_CHAR_RSP,
};



/// Health Thermometer Service Characteristics - Char. Code
enum htpc_chars
{
    /// Temperature Measurement
    HTPC_CHAR_HTS_TEMP_MEAS = 0x0,
    /// Temperature Type
    HTPC_CHAR_HTS_TEMP_TYPE,
    /// Intermediate Temperature
    HTPC_CHAR_HTS_INTM_TEMP,
    /// Measurement Interval
    HTPC_CHAR_HTS_MEAS_INTV,

    HTPC_CHAR_HTS_MAX,
};

/// Health Thermometer Service Characteristic Descriptors
enum htpc_descs
{
    /// Temp. Meas. Client Config
    HTPC_DESC_HTS_TEMP_MEAS_CLI_CFG,
    /// Intm. Meas. Client Config
    HTPC_DESC_HTS_INTM_MEAS_CLI_CFG,
    /// Meas. Intv. Client Config
    HTPC_DESC_HTS_MEAS_INTV_CLI_CFG,
    /// Meas. Intv. Valid Range,
    HTPC_DESC_HTS_MEAS_INTV_VAL_RGE,

    HTPC_DESC_HTS_MAX,
};

/// Internal codes for reading a HTS characteristic with one single request
enum htpc_rd_char
{
    ///Read HTS Temp. Type
    HTPC_RD_TEMP_TYPE           = 0,
    ///Read HTS Measurement Interval
    HTPC_RD_MEAS_INTV,

    ///Read HTS Temperature Measurement Client Cfg. Desc
    HTPC_RD_TEMP_MEAS_CLI_CFG,
    ///Read HTS Intermediate Temperature Client Cfg. Desc
    HTPC_RD_INTM_TEMP_CLI_CFG,
    ///Read HTS Measurement Interval Client Cfg. Desc
    HTPC_RD_MEAS_INTV_CLI_CFG,
    ///Read HTS Measurement Interval Client Cfg. Desc
    HTPC_RD_MEAS_INTV_VAL_RGE,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Health Thermometer Service
 */
struct htpc_hts_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic info:
    struct prf_char_inf chars[HTPC_CHAR_HTS_MAX];

    /// Descriptor handles:
    struct prf_char_desc_inf descs[HTPC_DESC_HTS_MAX];
};

/// Parameters of the @ref HTPC_ENABLE_REQ message
struct htpc_enable_req
{
    ///Connection type
    uint8_t con_type;

    ///Existing handle values hts
    struct htpc_hts_content hts;
};

/// Parameters of the @ref HTPC_ENABLE_RSP message
struct htpc_enable_rsp
{
    ///status
    uint8_t status;
    /// HTS handle values and characteristic properties
    struct htpc_hts_content hts;
};

///Parameters of the @ref HTPC_HEALTH_TEMP_NTF_CFG_REQ message
struct htpc_health_temp_ntf_cfg_req
{
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
    ///Own code for differentiating between Temperature Measurement, Intermediate Temperature and Measurement Interval chars
    uint8_t char_code;
};

///Parameters of the @ref HTPC_HEALTH_TEMP_NTF_CFG_RSP message
struct htpc_health_temp_ntf_cfg_rsp
{
    ///status
    uint8_t status;
};

///Parameters of the @ref HTPC_WR_MEAS_INTV_REQ message
struct htpc_wr_meas_intv_req
{
    ///Interval value
    uint16_t intv;
};

///Parameters of the @ref HTPC_WR_MEAS_INTV_RSP message
struct htpc_wr_meas_intv_rsp
{
    ///status
    uint8_t status;
};

///Parameters of the @ref HTPC_TEMP_IND message
struct htpc_temp_ind
{
    /// Temperature Measurement Structure
    struct htp_temp_meas temp_meas;
    /// Stable or intermediary type of temperature
    bool stable_meas;
};

///Parameters of @ref HTPC_MEAS_INTV_IND message
struct htpc_meas_intv_ind
{
    ///Interval
    uint16_t intv;
};

///Parameters of the @ref HTPC_RD_CHAR_REQ message
struct htpc_rd_char_req
{
    ///Characteristic value code (@see enum htpc_rd_char)
    uint8_t char_code;
};

///Parameters of the @ref HTPC_RD_CHAR_RSP message
struct htpc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};



/// @} HTPCTASK

#endif // HTPC_TASK_H_
