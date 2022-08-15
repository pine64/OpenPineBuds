#ifndef _DATAPATHPS_H_
#define _DATAPATHPS_H_

/**
 ****************************************************************************************
 * @addtogroup DATAPATHPS Datapath Profile Server
 * @ingroup DATAPATHP
 * @brief Datapath Profile Server
 *
 * Datapath Profile Server provides functionalities to upper layer module
 * application. The device using this profile takes the role of Datapath Server.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Send data to peer device via notifications  (from APP)
 *  - Receive data from peer device via write no response (from APP)
 *
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_DATAPATH_SERVER)
#include "prf_types.h"
#include "prf.h"
#include "datapathps_task.h"
#include "attm.h"
#include "prf_utils.h"


#define BLE_MAXIMUM_CHARACTERISTIC_DESCRIPTION	32

static const char custom_tx_desc[] = "Data Path TX Data";
static const char custom_rx_desc[] = "Data Path RX Data";



/*
 * DEFINES
 ****************************************************************************************
 */

#define DATAPATHPS_MAX_LEN            (509)	// consider the extended data length

#define DATAPATHPS_MANDATORY_MASK             (0x0F)
#define DATAPATHPS_BODY_SENSOR_LOC_MASK       (0x30)
#define DATAPATHPS_HR_CTNL_PT_MASK            (0xC0)

#define HRP_PRF_CFG_PERFORMED_OK        (0x80)

/*
 * MACROS
 ****************************************************************************************
 */

#define DATAPATHPS_IS_SUPPORTED(features, mask) ((features & mask) == mask)


/*
 * DEFINES
 ****************************************************************************************
 */
/// Possible states of the DATAPATHPS task
enum
{
    /// Idle state
    DATAPATHPS_IDLE,
    /// Connected state
    DATAPATHPS_BUSY,

    /// Number of defined states.
    DATAPATHPS_STATE_MAX,
};

///Attributes State Machine
enum
{
    DATAPATHPS_IDX_SVC,

    DATAPATHPS_IDX_TX_CHAR,
    DATAPATHPS_IDX_TX_VAL,
    DATAPATHPS_IDX_TX_NTF_CFG,
    DATAPATHPS_IDX_TX_DESC,

    DATAPATHPS_IDX_RX_CHAR,
    DATAPATHPS_IDX_RX_VAL,

    DATAPATHPS_IDX_RX_DESC,

    DATAPATHPS_IDX_NB,
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Datapath Profile Server environment variable
struct datapathps_env_tag
{
    /// profile environment
    prf_env_t 	prf_env;
    /// Service Start Handle
    uint16_t    shdl;
    /// flag to mark whether notification is enabled
    uint8_t     isNotificationEnabled[BLE_CONNECTION_MAX];
    /// State of different task instances
    ke_state_t  state;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve HRP service profile interface
 *
 * @return HRP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* datapathps_prf_itf_get(void);


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void datapathps_task_init(struct ke_task_desc *task_desc);



#endif /* #if (BLE_DATAPATH_SERVER) */

/// @} DATAPATHPS

#endif /* _DATAPATHPS_H_ */

