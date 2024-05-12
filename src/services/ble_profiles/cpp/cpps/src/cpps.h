#ifndef _CPPS_H_
#define _CPPS_H_

/**
 ****************************************************************************************
 * @addtogroup CPPS Cycling Power Profile Sensor
 * @ingroup CPP
 * @brief Cycling Power Profile Sensor
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"

#if (BLE_CP_SENSOR)

#include "prf_types.h"
#include "prf.h"
#include "cpps_task.h"
#include "attm.h"
#include "co_math.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximum number of Cycling Power Profile Sensor role task instances
#define CPPS_IDX_MAX        1

/********************************************
 ******* CPPS Configuration Flag Masks ******
 ********************************************/

/// Mandatory Attributes (CP Measurement + CP Feature + CP Sensor Location)
#define CPPS_MANDATORY_MASK           (0x01EF)
/// Broadcast Attribute
#define CPPS_MEAS_BCST_MASK           (0x0010)
/// Vector Attributes
#define CPPS_VECTOR_MASK              (0x0E00)
/// Control Point Attributes
#define CPPS_CTNL_PT_MASK             (0x7000)

/// Broadcast supported flag
#define CPPS_BROADCASTER_SUPP_FLAG    (0x01)
/// Control Point supported flag
#define CPPS_CTNL_PT_CHAR_SUPP_FLAG    (0x02)


/*
 * MACROS
 ****************************************************************************************
 */

#define CPPS_IS_FEATURE_SUPPORTED(features, flag) ((features & flag) == flag)

#define CPPS_IS_PRESENT(features, flag)           ((features & flag) == flag)

#define CPPS_IS_SET(features, flag)               (features & flag)

#define CPPS_IS_CLEAR(features, flag)             ((features & flag) == 0)

#define CPPS_ENABLE_NTF_IND_BCST(idx, ccc_flag)              (cpps_env->env[idx].prfl_ntf_ind_cfg |= ccc_flag)

#define CPPS_DISABLE_NTF_IND_BCST(idx, ccc_flag)             (cpps_env->env[idx].prfl_ntf_ind_cfg &= ~ccc_flag)

#define CPPS_IS_NTF_IND_BCST_ENABLED(idx, ccc_flag)          ((cpps_env->env[idx].prfl_ntf_ind_cfg & ccc_flag) == ccc_flag)

// MACRO TO CALCULATE HANDLE    shdl + idx - BCST - VECTOR
// BCST is 1 if the broadcast mode is supported otherwise 0
// VECTOR is 3 if the Vector characteristic is supported otherwise 0
#define CPPS_HANDLE(idx) \
    (cpps_env->shdl + (idx) - \
        ((!(CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_MEAS_BCST_MASK)) && \
                ((idx) > CPS_IDX_CP_MEAS_BCST_CFG))? (1) : (0)) - \
        ((!(CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_VECTOR_MASK)) && \
                ((idx) > CPS_IDX_VECTOR_CHAR))? (3) : (0)))

// Get database attribute index
 #define CPPS_IDX(hdl) \
    ((hdl - cpps_env->shdl) + \
        ((!(CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_MEAS_BCST_MASK)) && \
                ((hdl - cpps_env->shdl) > CPS_IDX_CP_MEAS_BCST_CFG)) ? (1) : (0)) + \
        ((!(CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_VECTOR_MASK)) && \
                ((hdl - cpps_env->shdl) > CPS_IDX_VECTOR_CHAR)) ? (3) : (0)))


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the CPPS task
enum cpps_state
{
    /// Idle state
    CPPS_IDLE,
    /// Busy state
    CPPS_BUSY,

    /// Number of defined states.
    CPPS_STATE_MAX
};
/// Cycling Power Service - Attribute List
enum cpps_cps_att_list
{
    /// Cycling Power Service
    CPS_IDX_SVC,
    /// CP Measurement
    CPS_IDX_CP_MEAS_CHAR,
    CPS_IDX_CP_MEAS_VAL,
    CPS_IDX_CP_MEAS_NTF_CFG,
    CPS_IDX_CP_MEAS_BCST_CFG,
    /// CP Feature
    CPS_IDX_CP_FEAT_CHAR,
    CPS_IDX_CP_FEAT_VAL,
    /// Sensor Location
    CPS_IDX_SENSOR_LOC_CHAR,
    CPS_IDX_SENSOR_LOC_VAL,
    /// CP Vector
    CPS_IDX_VECTOR_CHAR,
    CPS_IDX_VECTOR_VAL,
    CPS_IDX_VECTOR_NTF_CFG,
    /// CP Control Point
    CPS_IDX_CTNL_PT_CHAR,
    CPS_IDX_CTNL_PT_VAL,
    CPS_IDX_CTNL_PT_IND_CFG,

    /// Number of attributes
    CPS_IDX_NB,
};


/// Profile Configuration Additional Flags ()
enum cpps_prf_cfg_flag
{
    /// CP Measurement - Client Char. Cfg
    CPP_PRF_CFG_FLAG_CP_MEAS_NTF  = 0x01,

    /// CP Measurement - Server Char. Cfg
    CPP_PRF_CFG_FLAG_SP_MEAS_NTF  = 0x02,

    /// CP Vector - Client Characteristic configuration
    CPP_PRF_CFG_FLAG_VECTOR_NTF   = 0x04,

    /// Control Point - Client Characteristic configuration
    CPP_PRF_CFG_FLAG_CTNL_PT_IND  = 0x08,

    /// Bonded data used
    CPP_PRF_CFG_PERFORMED_OK      = 0x80,
};



/*
 * STRUCTURES
 ****************************************************************************************
 */

/// ongoing operation information
struct cpps_op
{
    /// On-going operation command
    struct ke_msg * cmd;
    /// notification pending
    struct gattc_send_evt_cmd* ntf_pending;

    /// Cursor on connection
    uint8_t cursor;
};

/// Cycling Power Profile Sensor environment variable per connection
struct cpps_cnx_env
{
    /// Measurement content mask
    uint16_t mask_meas_content;
    /// Profile Notify/Indication Flags
    uint8_t prfl_ntf_ind_cfg;
};

/// Cycling Power Profile Sensor environment variable
struct cpps_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct cpps_cnx_env env[BLE_CONNECTION_MAX];
    /// On-going operation
    struct cpps_op * op_data;

    /// Feature Configuration Flags
    uint32_t features;

    /// Instantaneous Power
    uint32_t inst_power;
    /// Cumulative Value
    uint32_t cumul_wheel_rev;

    /// Cycling Speed and Cadence Service Start Handle
    uint16_t shdl;
    /// Profile Configuration Flags
    uint16_t prfl_cfg;
    /// Operation
    uint8_t operation;

    /// Sensor Location
    uint8_t sensor_loc;

    /// State of different task instances
    ke_state_t state[CPPS_IDX_MAX];

    /// Broadcast advertisements are active
    bool broadcast_enabled;
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
 * @brief Retrieve CPP service profile interface
 *
 * @return CPP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* cpps_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Packs measurement notifications
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] pckd_meas pointer to packed message
 * @return status of the operation
 ****************************************************************************************
 */
uint8_t cpps_pack_meas_ntf(struct cpp_cp_meas *param, uint8_t *pckd_meas);

/**
 ****************************************************************************************
 * @brief Splits notifications in order to be sent with default MTU
 * @param[in] conidx connection index
 * @param[out] meas_val message to split
 * @return length of the second notification
 ****************************************************************************************
 */
uint8_t cpps_split_meas_ntf(uint8_t conidx, struct gattc_send_evt_cmd *meas_val);

/**
 ****************************************************************************************
 * @brief updates the environment with the descriptor configuration and sends indication
 * @param[in] conidx connection index
 * @param[in] prfl_config profile descriptor code
 * @param[in] param pointer to the message parameters
 * @return status of the operation
 ****************************************************************************************
 */
uint8_t cpps_update_characteristic_config(uint8_t conidx, uint8_t prfl_config, struct gattc_write_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Packs vector notifications
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] pckd_vector pointer to packed message
 * @return length
 ****************************************************************************************
 */
uint8_t cpps_pack_vector_ntf(struct cpp_cp_vector *param, uint8_t *pckd_vector);

/**
 ****************************************************************************************
 * @brief Unpack control point and sends indication
 * @param[in] conidx connection index
 * @param[in] param pointer to message
 * @return status of the operation
 ****************************************************************************************
 */
uint8_t cpps_unpack_ctnl_point_ind (uint8_t conidx, struct gattc_write_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Packs control point
 * @param[in] conidx connection index
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] rsp pointer to message
 * @return status of the operation
 ****************************************************************************************
 */
uint8_t cpps_pack_ctnl_point_cfm (uint8_t conidx, struct cpps_ctnl_pt_cfm *param, uint8_t *rsp);

/**
 ****************************************************************************************
 * @brief Update database in non discovery modes
 * @param[in] conidx connection index
 * @param[in] con_type Connection type
 * @param[in] handle handle
 * @param[in] ntf_cfg provided flag
 * @param[in] cfg_flag requested flag
 * @return status of the operation
 ****************************************************************************************
 */
int cpps_update_cfg_char_value (uint8_t conidx, uint8_t con_type, uint8_t handle, uint16_t ntf_cfg, uint16_t cfg_flag);

/**
 ****************************************************************************************
 * @brief Send a CPPS_CMP_EVT message to the application.
 * @param[in] conidx connection index
 * @param[in] src_id Source task
 * @param[in] dest_id Destination task
 * @param[in] operation Operation completed
 * @param[in] status status of the operation
 ****************************************************************************************
 */
void cpps_send_cmp_evt(uint8_t conidx, uint8_t src_id, uint8_t dest_id, uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a control point response to the peer
 * @param[in] conidx connection index
 * @param[in] req_op_code operation code
 * @param[in] status status of the operation
 ****************************************************************************************
 */
void cpps_send_rsp_ind(uint8_t conidx, uint8_t req_op_code, uint8_t status);

/**
 ****************************************************************************************
 * @brief  This function fully manages notification of measurement and vector
 ****************************************************************************************
 */
void cpps_exe_operation(void);

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
void cpps_task_init(struct ke_task_desc *task_desc);

#endif //(BLE_CP_SENSOR)

/// @} CPPS

#endif //(_CPPS_H_)
