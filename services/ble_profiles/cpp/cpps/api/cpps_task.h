#ifndef _CPPS_TASK_H_
#define _CPPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup CPPSTASK Task
 * @ingroup CPPS
 * @brief Cycling Power Profile Task.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"


#include <stdint.h>
#include "rwip_task.h" // Task definitions

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/// Messages for Cycling Power Profile Sensor
enum cpps_msg_id
{
    /// Start the Cycling Power Profile Server profile
    CPPS_ENABLE_REQ = TASK_FIRST_MSG(TASK_ID_CPPS),
    /// Confirm profile
    CPPS_ENABLE_RSP,

    /// Gets packed data for advertising
    CPPS_GET_ADV_DATA_REQ,
    /// Response with packed data for advertising
    CPPS_GET_ADV_DATA_RSP,

    /// Send a CP Measurement to the peer device (Notification)
    CPPS_NTF_CP_MEAS_REQ,
    /// CP Measurement response
    CPPS_NTF_CP_MEAS_RSP,

    /// Send a CP Vector to the peer device (Notification)
    CPPS_NTF_CP_VECTOR_REQ,
    /// CP Vector response
    CPPS_NTF_CP_VECTOR_RSP,

    /// Send a complete event status to the application
    CPPS_CMP_EVT,

    /// Indicate that an attribute value has been written
    CPPS_CFG_NTFIND_IND,

    /// Request to change vector client configuration
    CPPS_VECTOR_CFG_REQ_IND,
    /// Confirmation to change vector client configuration
    CPPS_VECTOR_CFG_CFM,

    /// Indicate that Control Point characteristic value has been written
    CPPS_CTNL_PT_REQ_IND,
    /// Application response after receiving a CPPS_CTNL_PT_REQ_IND message
    CPPS_CTNL_PT_CFM,
};

/// Operation Code used in the profile state machine
enum cpps_op_code
{
    /// Reserved Operation Code
    CPPS_RESERVED_OP_CODE          = 0x00,

    /// Enable Profile Operation Code
    CPPS_ENABLE_REQ_OP_CODE,

    /// Send CP Measurement Operation Code
    CPPS_NTF_MEAS_OP_CODE,

    /// Send Vector Operation Code
    CPPS_NTF_VECTOR_OP_CODE,

    /**
     * Control Point Operation
     */
    /// Set Cumulative Value
    CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE,
    /// Update Sensor Location
    CPPS_CTNL_PT_UPD_SENSOR_LOC_OP_CODE,
    /// Request Supported Sensor Locations
    CPPS_CTNL_PT_REQ_SUPP_SENSOR_LOC_OP_CODE,
    /// Set Crank Length
    CPPS_CTNL_PT_SET_CRANK_LENGTH_OP_CODE,
    /// Request Crank Length
    CPPS_CTNL_PT_REQ_CRANK_LENGTH_OP_CODE,
    /// Set Chain Length
    CPPS_CTNL_PT_SET_CHAIN_LENGTH_OP_CODE,
    /// Request Chain Length
    CPPS_CTNL_PT_REQ_CHAIN_LENGTH_OP_CODE,
    /// Set Chain Weight
    CPPS_CTNL_PT_SET_CHAIN_WEIGHT_OP_CODE,
    /// Request Chain Weight
    CPPS_CTNL_PT_REQ_CHAIN_WEIGHT_OP_CODE,
    /// Set Span Length
    CPPS_CTNL_PT_SET_SPAN_LENGTH_OP_CODE,
    /// Request Span Length
    CPPS_CTNL_PT_REQ_SPAN_LENGTH_OP_CODE,
    /// Start Offset Compensation
    CPPS_CTNL_PT_START_OFFSET_COMP_OP_CODE,
    /// Mask CP Measurement Characteristic Content
    CPPS_CTNL_MASK_CP_MEAS_CH_CONTENT_OP_CODE,
    /// Request Sampling Rate
    CPPS_CTNL_REQ_SAMPLING_RATE_OP_CODE,
    /// Request Factory Calibration Date
    CPPS_CTNL_REQ_FACTORY_CALIBRATION_DATE_OP_CODE,

    /// Error Indication Sent Operation Code
    CPPS_CTNL_ERR_IND_OP_CODE,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref CPPS_CREATE_DB_REQ message
struct cpps_db_cfg
{
    /**
     * CP Feature Value - Not supposed to be modified during the lifetime of the device
     * This bit field is set in order to decide which features are supported:
     *   Supported features (specification) ---------------- Bits 0 to 18
     */
    uint32_t cp_feature;
    /// Initial count for wheel revolutions
    uint32_t wheel_rev;
    /**
     * Profile characteristic configuration:
     *   Enable broadcaster mode in Measurement Characteristic --- Bit 0
     *   Enable Control Point Characteristic (*) ----------------- Bit 1
     *
     * (*) Note this characteristic is mandatory if server supports:
     *     - Wheel Revolution Data
     *     - Multiple Sensor Locations
     *     - Configurable Settings
     *     - Offset Compensation
     *     - Server allows to be requested for parameters (CPP_CTNL_PT_REQ... codes)
     */
    uint8_t prfl_config;
    /**
     * Indicate the sensor location.
     */
    uint8_t sensor_loc;
};

/// Parameters of the @ref CPPS_ENABLE_REQ message
struct cpps_enable_req
{
    /// Connection index
    uint8_t conidx;
    /**
     * Profile characteristic configuration:
     *   Measurement Characteristic notification config --- Bit 0
     *   Measurement Characteristic broadcast config ------ Bit 1
     *   Vector Characteristic notification config -------- Bit 2
     *   Control Point Characteristic indication config --- Bit 3
     */
    uint16_t prfl_ntf_ind_cfg;
};

/// Parameters of the @ref CPPS_ENABLE_RSP message
struct cpps_enable_rsp
{
    /// Connection index
    uint8_t conidx;
    /// status
    uint8_t status;
};

/// Parameters of the @ref CPPS_GET_ADV_DATA_REQ message
struct  cpps_get_adv_data_req
{
    ///Parameters
    struct cpp_cp_meas parameters;
};

/// Parameters of the @ref CPPS_GET_ADV_DATA_RSP message
struct  cpps_get_adv_data_rsp
{
    uint8_t status;
    /// Data length
    uint8_t data_len;
    /// Data to advertise
    uint8_t  adv_data[__ARRAY_EMPTY];
};

/// Parameters of the @ref CPPS_NTF_CP_MEAS_REQ message
struct  cpps_ntf_cp_meas_req
{
    ///Parameters
    struct cpp_cp_meas parameters;
};

/// Parameters of the @ref CPPS_NTF_CP_MEAS_RSP message
struct  cpps_ntf_cp_meas_rsp
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref CPPS_NTF_CP_VECTOR_REQ message
struct  cpps_ntf_cp_vector_req
{
    ///Parameters
    struct cpp_cp_vector parameters;
};

/// Parameters of the @ref CPPS_NTF_CP_VECTOR_RSP message
struct  cpps_ntf_cp_vector_rsp
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref CPPS_CTNL_PT_REQ_IND message
struct cpps_ctnl_pt_req_ind
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t op_code;
    /// Value
    union cpps_ctnl_pt_req_ind_value
    {
        /// Cumulative Value
        uint32_t cumul_val;
        /// Sensor Location
        uint8_t sensor_loc;
        /// Crank Length
        uint16_t crank_length;
        /// Chain Length
        uint16_t chain_length;
        /// Chain Weight
        uint16_t chain_weight;
        /// Span Length
        uint16_t span_length;
        /// Mask Content
        uint16_t mask_content;
    } value;
};

/// Parameters of the @ref CPPS_CTNL_PT_CFM message
struct cpps_ctnl_pt_cfm
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t op_code;
    /// Status
    uint8_t status;
    /// Value
    union cpps_sc_ctnl_pt_cfm_value
    {
        /// Supported sensor locations (up to 17)
        uint32_t supp_sensor_loc;
        /// New Cumulative Wheel revolution Value
        uint32_t cumul_wheel_rev;
        /// Crank Length
        uint16_t crank_length;
        /// Chain Length
        uint16_t chain_length;
        /// Chain Weight
        uint16_t chain_weight;
        /// Span Length
        uint16_t span_length;
        /// Offset compensation
        int16_t offset_comp;
        /// Mask Measurement content
        uint16_t mask_meas_content;
        ///Sampling rate
        uint8_t sampling_rate;
        /// New Sensor Location
        uint8_t sensor_loc;
        /// Calibration date
        struct prf_date_time factory_calibration;

    } value;
};

/// Parameters of the @ref CPPS_CFG_NTFIND_IND message
struct cpps_cfg_ntfind_ind
{
    /// Connection index
    uint8_t conidx;
    /// Characteristic Code (CP Measurement or Control Point)
    uint8_t char_code;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref CPPS_VECTOR_CFG_REQ_IND message
struct cpps_vector_cfg_req_ind
{
    /// Connection index
    uint8_t conidx;
    /// Characteristic Code
    uint8_t char_code;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref CPPS_VECTOR_CFG_CFM message
struct cpps_vector_cfg_cfm
{
    /// Connection index
    uint8_t conidx;
    /// Status
    uint8_t status;
    /// Char. Client Characteristic Configuration
    uint16_t ntf_cfg;
};

/// Parameters of the @ref CPPS_CMP_EVT message
struct cpps_cmp_evt
{
    /// Connection index
    uint8_t conidx;
    /// Operation Code
    uint8_t operation;
    /// Operation Status
    uint8_t status;
};

/// @} CPPSTASK

#endif //(_CPPS_TASK_H_)
