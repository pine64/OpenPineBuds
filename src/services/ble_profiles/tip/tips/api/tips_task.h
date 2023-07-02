#ifndef TIPS_TASK_H_
#define TIPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup TIPSTASK Task
 * @ingroup TIPS
 * @brief Time Profile Server Task
 *
 * The TIPSTASK is responsible for handling the messages coming in and out of the
 * @ref TIPS reporter block of the BLE Host.
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


/// Messages for Time Profile Server
enum
{
    ///Start the Time Profile Server Role - at connection
    TIPS_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_TIPS),
    ///Inform the application about the task creation result
    TIPS_ENABLE_RSP,

    ///Update Current Time Request from APP
    TIPS_UPD_CURR_TIME_REQ,
    ///Update Current Time Response to APP
    TIPS_UPD_CURR_TIME_RSP,

    ///Inform APP about a requested read
    TIPS_RD_REQ_IND,
    ///Read request Response
    TIPS_RD_CFM,

    ///Inform APP about modification of Current Time Characteristic Client. Charact. Cfg
    TIPS_CURRENT_TIME_CCC_IND,
    ///Inform APP about modification of Time Update Control Point Characteristic Value
    TIPS_TIME_UPD_CTNL_PT_IND,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref TIPS_ENABLE_REQ message
struct tips_enable_req
{
    ///Current Time notification configuration
    uint16_t current_time_ntf_en;
};

/// Parameters of the @ref TIPS_ENABLE_RSP message
struct tips_enable_rsp
{
    ///Status
    uint8_t status;
};

///Parameters of the @ref TIPS_UPD_CURR_TIME_REQ message
struct tips_upd_curr_time_req
{
    ///Current Time
    struct tip_curr_time current_time;
    /**
     * Indicate if the new Current Time value can be sent if the current_time_ntf_en parameter is
     * set to 1.
     * (0 - Disable; 1- Enable)
     *
     * If the time of the Current Time Server is changed because of reference time update (adjust reason)
     * then no notifications shall be sent to the Current Time Service Client within the 15 minutes from
     * the last notification, unless one of both of the two statements below are true :
     *         - The new time information differs by more than 1min from the Current Time Server
     *         time previous to the update
     *         - The update was caused by the client (interacting with another service)
     */
    uint8_t enable_ntf_send;
};

///Parameters of the @ref TIPS_UPD_CURR_TIME_RSP message
struct tips_upd_curr_time_rsp
{
    /// Status of the operation
    uint8_t status;
};

///Parameters of the @ref TIPS_RD_REQ_IND message
struct tips_rd_req_ind
{
    /// Characteristic code
    uint8_t char_code;
};

///Parameters of the @ref TIPS_RD_CFM message
struct tips_rd_cfm
{
    uint8_t op_code;

    union tips_rd_value_tag
    {
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

///Parameters of the @ref TIPS_CURRENT_TIME_CCC_IND message
struct tips_current_time_ccc_ind
{
    ///Configuration Value
    uint16_t cfg_val;
};

///Parameters of the @ref TIPS_TIME_UPD_CTNL_PT_IND message
struct tips_time_upd_ctnl_pt_ind
{
    ///Time Update Control Point Value
    tip_time_upd_contr_pt value;
};

///Parameters of the @ref TIPS_CREATE_DB_REQ message
struct tips_db_cfg
{
    ///Database configuration
    uint8_t features;
};

/// @} TIPSTASK
#endif // TIPS_TASK_H_
