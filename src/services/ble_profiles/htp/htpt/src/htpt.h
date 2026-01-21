#ifndef HTPT_H_
#define HTPT_H_

/**
 ****************************************************************************************
 * @addtogroup HTPT Health Thermometer Profile Thermometer
 * @ingroup HTP
 * @brief Health Thermometer Profile Thermometer
 *
 * An actual thermometer device does not exist on current platform, so measurement values
 * that would come from a driver are replaced by simple counters sent at certain intervals
 * following by the profile attributes configuration.
 * When a measurement interval
 * has been set to a non-zero value in a configuration connection, once reconnected,
 * TH will send regular measurement INDs if Temp Meas Char Cfg is set to indicate and
 * using the Meas Intv Value. The INDs will continue until meas interval is set to 0
 * or connection gets disconnected by C. Measurements should be stored even so, until
 * profile is disabled.(?)
 *
 * If the measurement interval has been set to 0, then if Intermediate Temp is set to be
 * notified and Temp Meas to be indicated, then a timer of fixed length simulates
 * sending several NTF before and indication of a "stable" value. This fake behavior
 * should be replaced once a real driver exists. If Intermediary Temp cannot be notified,
 * just send the indication, if neither can be sent (the configuration connection should
 * never leave this like this) then disconnect.
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
#if (BLE_HT_THERMOM)
#include <stdint.h>
#include <stdbool.h>
#include "htp_common.h"
#include "prf_types.h"
#include "prf_utils.h"

#include "gap.h"

/*
 * MACROS
 ****************************************************************************************
 */
#define HTPT_IS_FEATURE_SUPPORTED(feat, bit_mask) (((feat & bit_mask) == bit_mask))

#define HTPT_HANDLE(idx) (htpt_att_hdl_get(htpt_env, (idx)))

#define HTPT_IDX(hdl)    (htpt_att_idx_get(htpt_env, (hdl)))


/*
 * DEFINES
 ****************************************************************************************
 */
///Maximum number of Health thermometer task instances
#define HTPT_IDX_MAX    (1)


///Valid range for measurement interval values (s)
#define HTPT_MEAS_INTV_DFLT_MIN           (0x0001)
#define HTPT_MEAS_INTV_DFLT_MAX           (0x000A)

#define HTPT_TEMP_MEAS_MAX_LEN            (13)
#define HTPT_TEMP_TYPE_MAX_LEN            (1)
#define HTPT_MEAS_INTV_MAX_LEN            (2)
#define HTPT_MEAS_INTV_RANGE_MAX_LEN      (4)
#define HTPT_IND_NTF_CFG_MAX_LEN          (2)

#define HTPT_TEMP_MEAS_MASK               (0x000F)
#define HTPT_TEMP_TYPE_MASK               (0x0030)
#define HTPT_INTM_TEMP_MASK               (0x01C0)
#define HTPT_MEAS_INTV_MASK               (0x0600)
#define HTPT_MEAS_INTV_CCC_MASK           (0x0800)
#define HTPT_MEAS_INTV_VALID_RGE_MASK     (0x1000)

#define HTPT_TEMP_MEAS_ATT_NB             (4)
#define HTPT_TEMP_TYPE_ATT_NB             (2)
#define HTPT_INTERM_MEAS_ATT_NB           (3)
#define HTPT_MEAS_INTV_ATT_NB             (2)
#define HTPT_MEAS_INTV_CCC_ATT_NB         (1)
#define HTPT_MEAS_INTV_RNG_ATT_NB         (1)


/// Possible states of the HTPT task
enum htpt_state
{
    /// Idle state
    HTPT_IDLE,
    /// Busy state
    HTPT_BUSY,

    /// Number of defined states.
    HTPT_STATE_MAX
};

///Attributes database elements
enum hts_att_db_list
{
    HTS_IDX_SVC,

    HTS_IDX_TEMP_MEAS_CHAR,
    HTS_IDX_TEMP_MEAS_VAL,
    HTS_IDX_TEMP_MEAS_IND_CFG,

    HTS_IDX_TEMP_TYPE_CHAR,
    HTS_IDX_TEMP_TYPE_VAL,

    HTS_IDX_INTERM_TEMP_CHAR,
    HTS_IDX_INTERM_TEMP_VAL,
    HTS_IDX_INTERM_TEMP_CFG,

    HTS_IDX_MEAS_INTV_CHAR,
    HTS_IDX_MEAS_INTV_VAL,
    HTS_IDX_MEAS_INTV_CFG,
    HTS_IDX_MEAS_INTV_VAL_RANGE,

    HTS_IDX_NB,
};


/// ongoing operation information
struct htpt_op
{
     /// Operation
     uint8_t op;
     /// Cursor on connection
     uint8_t cursor;
     /// Handle of the attribute to indicate/notify
     uint16_t handle;
     /// Task that request the operation that should receive completed message response
     uint16_t dest_id;
     /// Packed notification/indication data size
     uint8_t length;
     /// used to know on which device interval update has been requested, and to prevent
     /// indication to be triggered on this connection index
     uint8_t conidx;
     /// Packed notification/indication data
     uint8_t data[__ARRAY_EMPTY];
};


///Health Thermometer Profile Thermometer Environment Variable
struct htpt_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// On-going operation
    struct htpt_op * operation;
    /// Service Start Handle
    uint16_t shdl;
    /// Database configuration
    uint16_t features;

    /// Current Measure interval
    uint16_t meas_intv;
    /// measurement interval range min
    uint16_t meas_intv_min;
    /// measurement interval range max
    uint16_t meas_intv_max;
    /// Temperature Type Value
    uint8_t temp_type;

    /// Notification and indication configuration of peer devices.
    uint8_t ntf_ind_cfg[BLE_CONNECTION_MAX];
    /// State of different task instances
    ke_state_t state[HTPT_IDX_MAX];
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
 * @brief Compute the offset of the valid range descriptor.
 * The Measurement Interval characteristic has two optional descriptors. In the database,
 * the Client Characteristic Configuration descriptor will always be placed just after the
 * characteristic value. Thus, this function checks if the CCC descriptor has been added.
 * @return     0 if Measurement Interval Char. is not writable (no Valid Range descriptor)
 *             1 if Measurement Interval Char. doesn't support indications (no CCC descriptor)
 *             2 otherwise
 ****************************************************************************************
 */
uint8_t htpt_get_valid_rge_offset(uint16_t features);

/**
 ****************************************************************************************
 * @brief Retrieve HTS service profile interface
 *
 * @return HTS service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* htpt_prf_itf_get(void);


/**
 ****************************************************************************************
 * @brief Pack temperature value from several components
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t htpt_pack_temp_value(uint8_t *packed_temp, struct htp_temp_meas temp_meas);


/**
 ****************************************************************************************
 * @brief  This function fully manage notification and indication of health thermometer
 *         to peer(s) device(s) according to on-going operation requested by application:
 *            - Modification of Intermediate Temperature
 *            - Indicate to a known device that Temperature Measured has change
 *            - Indicate to a known device that Measure Interval has change
 ****************************************************************************************
 */
void htpt_exe_operation(void);



/**
 ****************************************************************************************
 * @brief Update Notification, Indication configuration
 *
 * @param[in] conidx    Connection index
 * @param[in] cfg       Indication configuration flag
 * @param[in] valid_val Valid value if NTF/IND enable
 * @param[in] value     Value set by peer device.
 *
 * @return status of configuration update
 ****************************************************************************************
 */
uint8_t htpt_update_ntf_ind_cfg(uint8_t conidx, uint8_t cfg, uint16_t valid_val, uint16_t value);


/**
 ****************************************************************************************
 * @brief Retrieve attribute handle from attribute index
 *
 * @param[in] htpt_env   Environment variable
 * @param[in] att_idx    Attribute Index
 *
 * @return attribute Handle
 ****************************************************************************************
 */
uint16_t htpt_att_hdl_get(struct htpt_env_tag* htpt_env, uint8_t att_idx);

/**
 ****************************************************************************************
 * @brief Retrieve attribute index from attribute handle
 *
 * @param[in] htpt_env   Environment variable
 * @param[in] handle     Attribute Handle
 *
 * @return attribute Index
 ****************************************************************************************
 */
uint8_t htpt_att_idx_get(struct htpt_env_tag* htpt_env, uint16_t handle);

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
void htpt_task_init(struct ke_task_desc *task_desc);



#endif //BLE_HT_THERMOM

/// @} HTPT

#endif // HTPT_H_
