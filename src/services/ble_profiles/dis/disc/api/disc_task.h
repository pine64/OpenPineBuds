#ifndef DISC_TASK_H_
#define DISC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup DISCTASK Device Information Service Client Task
 * @ingroup DISC
 * @brief Device Information Service Client Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_task.h" // Task definitions
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */


enum
{
    /// Start the find me locator profile - at connection
    DISC_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_DISC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    DISC_ENABLE_RSP,

    /// Generic message to read a DIS characteristic value
    DISC_RD_CHAR_REQ,
    /// Generic message for read responses for APP
    DISC_RD_CHAR_RSP,
};

enum
{
    DISC_MANUFACTURER_NAME_CHAR,
    DISC_MODEL_NB_STR_CHAR,
    DISC_SERIAL_NB_STR_CHAR,
    DISC_HARD_REV_STR_CHAR,
    DISC_FIRM_REV_STR_CHAR,
    DISC_SW_REV_STR_CHAR,
    DISC_SYSTEM_ID_CHAR,
    DISC_IEEE_CHAR,
    DISC_PNP_ID_CHAR,

    DISC_CHAR_MAX,
};
/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Device Information Service
 */
struct disc_dis_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic info:
    struct prf_char_inf chars[DISC_CHAR_MAX];
};

/// Parameters of the @ref DISC_ENABLE_REQ message
struct disc_enable_req
{
    ///Connection type
    uint8_t con_type;

    /// Existing handle values dis
    struct disc_dis_content dis;
};

/// Parameters of the @ref DISC_ENABLE_RSP message
struct disc_enable_rsp
{
    ///status
    uint8_t status;

    /// DIS handle values and characteristic properties
    struct disc_dis_content dis;
};

///Parameters of the @ref DISC_RD_CHAR_REQ message
struct disc_rd_char_req
{
    ///Characteristic value code
    uint8_t char_code;
};

///Parameters of the @ref DISC_RD_CHAR_RSP message
struct disc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};



/// @} DISCTASK

#endif // DISC_TASK_H_
