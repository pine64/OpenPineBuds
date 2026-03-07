#ifndef _HOGPD_H_
#define _HOGPD_H_

/**
 ****************************************************************************************
 * @addtogroup HOGPD HID Over GATT Profile Device Role
 * @ingroup HOGP
 * @brief HID Over GATT Profile Device Role
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HID_DEVICE)
#include "hogp_common.h"
#include "hogpd_task.h"

#include "prf.h"
#include "prf_types.h"


/*
 * DEFINES
 ****************************************************************************************
 */


///Maximum number of HID Over GATT Device task instances
#define HOGPD_IDX_MAX                   (0x01)

/// Maximal length of Report Char. Value
#define HOGPD_REPORT_MAX_LEN                (45)
/// Maximal length of Report Map Char. Value
#define HOGPD_REPORT_MAP_MAX_LEN            (512)

/// Length of Boot Report Char. Value Maximal Length
#define HOGPD_BOOT_REPORT_MAX_LEN           (8)

/// Boot KB Input Report Notification Configuration Bit Mask
#define HOGPD_BOOT_KB_IN_NTF_CFG_MASK       (0x40)
/// Boot KB Input Report Notification Configuration Bit Mask
#define HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK    (0x80)
/// Boot Report Notification Configuration Bit Mask
#define HOGPD_REPORT_NTF_CFG_MASK           (0x20)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the HOGPD task
enum hogpd_state
{
    /// Idle state
    HOGPD_IDLE,
    /// Request from application on-going
    HOGPD_REQ_BUSY  = (1 << 0),
    /// OPeration requested by peer device on-going
    HOGPD_OP_BUSY   = (1 << 1),
    /// Number of defined states.
    HOGPD_STATE_MAX
};

/// HID Service Attributes Indexes
enum
{
    HOGPD_IDX_SVC,

    // Included Service
    HOGPD_IDX_INCL_SVC,

    // HID Information
    HOGPD_IDX_HID_INFO_CHAR,
    HOGPD_IDX_HID_INFO_VAL,

    // HID Control Point
    HOGPD_IDX_HID_CTNL_PT_CHAR,
    HOGPD_IDX_HID_CTNL_PT_VAL,

    // Report Map
    HOGPD_IDX_REPORT_MAP_CHAR,
    HOGPD_IDX_REPORT_MAP_VAL,
    HOGPD_IDX_REPORT_MAP_EXT_REP_REF,

    // Protocol Mode
    HOGPD_IDX_PROTO_MODE_CHAR,
    HOGPD_IDX_PROTO_MODE_VAL,

    // Boot Keyboard Input Report
    HOGPD_IDX_BOOT_KB_IN_REPORT_CHAR,
    HOGPD_IDX_BOOT_KB_IN_REPORT_VAL,
    HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG,

    // Boot Keyboard Output Report
    HOGPD_IDX_BOOT_KB_OUT_REPORT_CHAR,
    HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL,

    // Boot Mouse Input Report
    HOGPD_IDX_BOOT_MOUSE_IN_REPORT_CHAR,
    HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL,
    HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG,

    /// number of attributes that are uniq in the service
    HOGPD_ATT_UNIQ_NB,

    // Report
    HOGPD_IDX_REPORT_CHAR = HOGPD_ATT_UNIQ_NB,
    HOGPD_IDX_REPORT_VAL,
    HOGPD_IDX_REPORT_REP_REF,
    HOGPD_IDX_REPORT_NTF_CFG,

    HOGPD_IDX_NB,

    // maximum number of attribute that can be present in the HID service
    HOGPD_ATT_MAX = HOGPD_ATT_UNIQ_NB + (4* (HOGPD_NB_REPORT_INST_MAX)),
};



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// HIDS service cfg
struct hogpd_svc_cfg
{
    /// Service Features (@see enum hogpd_cfg)
    uint16_t features;
    /// Notification configuration
    uint16_t ntf_cfg[BLE_CONNECTION_MAX];
    /// Number of attribute present in service
    uint8_t  nb_att;
    /// Number of Report Char. instances to add in the database
    uint8_t  nb_report;
    /// Handle offset where report are available - to enhance handle search
    uint8_t  report_hdl_offset;
    /// Current Protocol Mode
    uint8_t  proto_mode;
};

/// HIDS on-going operation
struct hogpd_operation
{
    /// Connection index impacted
    uint8_t  conidx;
    /// Operation type (@see enum hogpd_op)
    uint8_t  operation;
    /// Handle impacted by operation
    uint16_t handle;
};

/// HID Over GATT Profile HID Device Role Environment variable
struct hogpd_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Supported Features
    struct hogpd_svc_cfg svcs[HOGPD_NB_HIDS_INST_MAX];
    /// HIDS Start Handles
    uint16_t start_hdl;
    /// On-going operation (requested by peer device)
    struct hogpd_operation op;
    /// HID Over GATT task state
    ke_state_t state[HOGPD_IDX_MAX];
    /// Number of HIDS added in the database
    uint8_t hids_nb;
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
 * @brief Retrieve HID service profile interface
 *
 * @return HID service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* hogpd_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Retrieve Attribute handle from service and attribute index
 *
 * @param[in] hogpd_env  HID Service environment
 * @param[in] svc_idx    HID Service index
 * @param[in] att_idx    Attribute index
 * @param[in] report_idx Report index
 *
 * @return HID attribute handle or INVALID HANDLE if nothing found
 ****************************************************************************************
 */
uint16_t hogpd_get_att_handle(struct hogpd_env_tag* hogpd_env, uint8_t svc_idx, uint8_t att_idx, uint8_t report_idx);


/**
 ****************************************************************************************
 * @brief Retrieve Service and attribute index form attribute handle
 *
 * @param[out] handle     Attribute handle
 * @param[out] svc_idx    HID Service index
 * @param[out] att_idx    Attribute index
 * @param[out] report_idx Report Index
 *
 * @return Success if attribute and service index found, else Application error
 ****************************************************************************************
 */
uint8_t hogpd_get_att_idx(struct hogpd_env_tag* hogpd_env, uint16_t handle, uint8_t *svc_idx, uint8_t *att_idx, uint8_t *report_idx);


/**
 ****************************************************************************************
 * @brief Check if a report value can be notified to the peer central.
 *
 * @param[in] conidx Connection Index
 * @param[in] report Report information to notify
 *
 * @return Status Code to know if request succeed or not.
 ****************************************************************************************
 */
uint8_t hogpd_ntf_send(uint8_t conidx, const struct hogpd_report_info* report);

/**
 ****************************************************************************************
 * @brief Send a HOGPD_NTF_CFG_IND message to the application
 *
 * @param[in] conidx Connection Index
 * @param[in] svc_idx    HID Service index
 * @param[in] att_idx    Attribute index
 * @param[in] report_idx Report Index
 * @param[in] ntf_cfg       Client Characteristic Configuration Descriptor value
 *
 * @return Status Code to know if notification update succeed or not
 ****************************************************************************************
 */
uint8_t hogpd_ntf_cfg_ind_send(uint8_t conidx, uint8_t svc_idx, uint8_t att_idx, uint8_t report_idx, uint16_t ntf_cfg);


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
void hogpd_task_init(struct ke_task_desc *task_desc);

#endif /* #if (BLE_HID_DEVICE) */

/// @} HOGPD

#endif /* _HOGPD_H_ */
