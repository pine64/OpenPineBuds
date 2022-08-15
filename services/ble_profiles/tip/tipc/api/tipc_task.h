#ifndef _TIPC_TASK_H_
#define _TIPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup TIPCTASK Time Profile Client Task
 * @ingroup TIPC
 * @brief Time Profile Client Task
 *
 * The TIPCTASK is responsible for handling the messages coming in and out of the
 * @ref TIPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "tip_common.h"
#include "rwip_task.h" // Task definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


enum
{
    /// Start the time profile - at connection
    TIPC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_TIPC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    TIPC_ENABLE_RSP,

    /// Generic message to read a CTS, NDCS or RTUS characteristic value
    TIPC_RD_CHAR_REQ,
    /// Received read value
    TIPC_RD_CHAR_RSP,

    ///Generic message for configuring the Current Time Characteristic on the Server
    TIPC_CT_NTF_CFG_REQ,
    ///Current Time Characteristic configuration response
    TIPC_CT_NTF_CFG_RSP,

    /// Generic message for writing the Time Update Control Point Characteristic Value on a peer device
    TIPC_WR_TIME_UPD_CTNL_PT_REQ,
    ///Generic message for writing characteristic response status to APP
    TIPC_WR_TIME_UPD_CTNL_PT_RSP,

    /// Received Current Time value (Notification)
    TIPC_CT_IND,
};



/// Current Time Service Characteristics
enum
{
    /// Current Time
    TIPC_CHAR_CTS_CURR_TIME,
    /// Local Time Info
    TIPC_CHAR_CTS_LOCAL_TIME_INFO,
    /// Reference Time Info
    TIPC_CHAR_CTS_REF_TIME_INFO,

    TIPC_CHAR_CTS_MAX,
};

/// Next DST Change Service Characteristics
enum
{
    /// Time With DST
    TIPC_CHAR_NDCS_TIME_WITH_DST,

    TIPC_CHAR_NDCS_MAX,
    TIPC_CHAR_NDCS_MASK = 0x10,
};

/// Reference Time Update Service Characteristics
enum
{
    /// Time Update Control Point
    TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT,
    /// Time Update State
    TIPC_CHAR_RTUS_TIME_UPD_STATE,

    TIPC_CHAR_RTUS_MAX,
    TIPC_CHAR_RTUS_MASK = 0x20,
};

/// Current Time Service Characteristic Descriptors
enum
{
    /// Current Time client config
    TIPC_DESC_CTS_CURR_TIME_CLI_CFG,

    TIPC_DESC_CTS_MAX,
    TIPC_DESC_CTS_MASK = 0x30,
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Current Time Service
 */
struct tipc_cts_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Current Time
    ///  - Local Time Info
    ///  - Reference Time Info
    struct prf_char_inf chars[TIPC_CHAR_CTS_MAX];

    /// Descriptor handles:
    ///  - Current Time client cfg
    struct prf_char_desc_inf descs[TIPC_DESC_CTS_MAX];
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Next DST Change Service
 */
struct tipc_ndcs_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Time With DST
    struct prf_char_inf chars[TIPC_CHAR_NDCS_MAX];
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Reference Time Update Service
 */
struct tipc_rtus_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Time Update Control Point
    ///  - Time Update State
    struct prf_char_inf chars[TIPC_CHAR_RTUS_MAX];
};


/// Parameters of the @ref TIPC_ENABLE_REQ message
struct tipc_enable_req
{
    ///Connection type
    uint8_t con_type;

    ///Existing handle values cts
    struct tipc_cts_content cts;
    ///Existing handle values ndcs
    struct tipc_ndcs_content ndcs;
    ///Existing handle values rtus
    struct tipc_rtus_content rtus;
};

/// Parameters of the @ref TIPC_ENABLE_RSP message
struct tipc_enable_rsp
{
    ///status
    uint8_t status;

    ///Existing handle values cts
    struct tipc_cts_content cts;
    ///Existing handle values ndcs
    struct tipc_ndcs_content ndcs;
    ///Existing handle values rtus
    struct tipc_rtus_content rtus;
};

///Parameters of the @ref TIPC_CT_NTF_CFG_REQ message
struct tipc_ct_ntf_cfg_req
{
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

///Parameters of the @ref TIPC_CT_NTF_CFG_RSP message
struct tipc_ct_ntf_cfg_rsp
{
    ///Status
    uint8_t status;
};

///Parameters of the @ref TIPC_RD_CHAR_REQ message
struct tipc_rd_char_req
{
    ///Characteristic value code
    uint8_t  char_code;
};

///Parameters of the @ref TIPC_RD_CHAR_RSP message
struct tipc_rd_char_rsp
{
    /// Operation code
    uint8_t op_code;
    /// Status
    uint8_t status;

    union tipc_rd_value_tag
    {
        ///Notification Configuration
        uint16_t ntf_cfg;
        ///Current Time
        struct tip_curr_time curr_time;
        ///Local Time Information
        struct tip_loc_time_info loc_time_info;
        ///Reference Time Information
        struct tip_ref_time_info ref_time_info;
        ///Time With DST
        struct tip_time_with_dst time_with_dst;
        ///Time Update State
        struct tip_time_upd_state time_upd_state;
    } value;
};

///Parameters of the @ref TIPC_WR_TIME_UPD_CTNL_PT_REQ message
struct tipc_wr_time_udp_ctnl_pt_req
{
    ///Value
    uint8_t value;
};

///Parameters of the @ref TIPC_WR_TIME_UPD_CTNL_PT_RSP message
struct tipc_wr_time_upd_ctnl_pt_rsp
{
    ///Status
    uint8_t  status;
};

///Parameters of the @ref TIPC_CT_IND message
struct tipc_ct_ind
{
    ///Current Time Value
    struct tip_curr_time ct_val;
};


/// @} TIPCTASK

#endif /* _TIPC_TASK_H_ */
