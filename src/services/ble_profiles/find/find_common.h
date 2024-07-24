#ifndef _FIND_COMMON_H_
#define _FIND_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup FIND Find Profile
 * @ingroup PROFILE
 * @brief Find Profile
 *
 * The FIND module is the responsible block for implementing the Find Profile
 * functionalities in the BLE Host.
 *
 * The Find Profile defines the functionality required in a device that allows
 * the user (Collector device) to configure and recover Find measurements from
 * a Find device.
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "prf_types.h"
#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/// Find measurement packet max length
#define FIND_MEAS_MAX_LEN                (17)
/// Find measurement context packet max length
#define FIND_MEAS_CTX_MAX_LEN            (17)
/// Record Access Control Point packet max length
#define FIND_REC_ACCESS_CTRL_MAX_LEN     (21)


/// Find Service Error Code
enum find_error_code
{
    /// RACP Procedure already in progress
    FIND_ERR_PROC_ALREADY_IN_PROGRESS  = (0x80),
    /// Client Characteristic Configuration Descriptor Improperly Configured
    FIND_ERR_IMPROPER_CLI_CHAR_CFG     = (0x81),
};

/// Record access control point operation filter
struct find_filter
{
    /// function operator
    uint8_t operator;

    /// filter type
    uint8_t filter_type;

    /// filter union
    union
    {
        /// Sequence number filtering (filter_type = FIND_FILTER_SEQ_NUMBER)
        struct
        {
            /// Min sequence number
            uint16_t min;
            /// Max sequence number
            uint16_t max;
        }seq_num;

        /// User facing time filtering (filter_type = FIND_FILTER_USER_FACING_TIME)
        struct
        {
            /// Min base time
            struct prf_date_time facetime_min;

            /// Max base time
            struct prf_date_time facetime_max;
        } time;
    } val;
};

/// Record access control point request
struct find_racp_req
{
    /// operation code
    uint8_t op_code;

    /// operation filter
    struct find_filter filter;
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */



/// @} find_common

#endif /* _FIND_COMMON_H_ */
