#ifndef _GLPS_H_
#define _GLPS_H_

/**
 ****************************************************************************************
 * @addtogroup GLPS Glucose Profile Sensor
 * @ingroup GLP
 * @brief Glucose Profile Sensor
 *
 * Glucose sensor (GLS) profile provides functionalities to upper layer module
 * application. The device using this profile takes the role of Glucose sensor.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Notify peer device during Glucose measurement (from APP)
 *  - Indicate measurements performed to peer device (from APP)
 *
 * Profile didn't manages multiple users configuration and storage of offline measurements.
 * This must be handled by application.
 *
 * Glucose Profile Sensor. (GLPS): A GLPS (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can perform Glucose
 * measurement and notify about on-going measurement and indicate final result to a peer
 * BLE device.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_GL_SENSOR)

#include "glp_common.h"
#include "prf_types.h"
#include "prf.h"
#include "attm.h"

/*
 * DEFINES
 ****************************************************************************************
 */
///Maximum number of Glucose task instances
#define GLPS_IDX_MAX     BLE_CONNECTION_MAX


///GLS Configuration Flag Masks
#define GLPS_MANDATORY_MASK                (0x1F8F)
#define GLPS_MEAS_CTX_PRES_MASK            (0x0070)

/*
 * MACROS
 ****************************************************************************************
 */

/// check if flag enable
#define GLPS_IS(IDX, FLAG) ((glps_env->env[IDX]->flags & (1<< (GLPS_##FLAG))) == (1<< (GLPS_##FLAG)))

/// Set flag enable
#define GLPS_SET(IDX, FLAG) (glps_env->env[IDX]->flags |= (1<< (GLPS_##FLAG)))

/// Set flag disable
#define GLPS_CLEAR(IDX, FLAG) (glps_env->env[IDX]->flags &= ~(1<< (GLPS_##FLAG)))


/// Get database attribute handle
#define GLPS_HANDLE(idx) \
    (glps_env->start_hdl + (idx) - \
        ((!(glps_env->meas_ctx_supported) && ((idx) > GLS_IDX_MEAS_CTX_NTF_CFG))? (3) : (0)))

/// Get database attribute index
#define GLPS_IDX(hdl) \
    (((((hdl) - glps_env->start_hdl) > GLS_IDX_MEAS_CTX_NTF_CFG) && !(glps_env->meas_ctx_supported)) ?\
        ((hdl) - glps_env->start_hdl + 3) : ((hdl) - glps_env->start_hdl))

/// Get event Indication/notification configuration bit
#define GLPS_IND_NTF_EVT(idx) (1 << ((idx - 1) / 3))
/*
 * ENUMS
 ****************************************************************************************
 */

/// Possible states of the GLPS task
enum glps_state
{
    /// Not Connected state
    GLPS_FREE,
    /// Idle state
    GLPS_IDLE,

    /// Number of defined states.
    GLPS_STATE_MAX
};

///Attributes State Machine
enum
{
    /// Glucose Service
    GLS_IDX_SVC,
    /// Glucose Measurement
    GLS_IDX_MEAS_CHAR,
    GLS_IDX_MEAS_VAL,
    GLS_IDX_MEAS_NTF_CFG,
    /// Glucose Measurement Context
    GLS_IDX_MEAS_CTX_CHAR,
    GLS_IDX_MEAS_CTX_VAL,
    GLS_IDX_MEAS_CTX_NTF_CFG,
    ///Glucose Feature
    GLS_IDX_FEATURE_CHAR,
    GLS_IDX_FEATURE_VAL,
    ///Record Access Control Point
    GLS_IDX_REC_ACCESS_CTRL_CHAR,
    GLS_IDX_REC_ACCESS_CTRL_VAL,
    GLS_IDX_REC_ACCESS_CTRL_IND_CFG,

    GLS_IDX_NB,
};

///Characteristic Codes
enum
{
    /// Glucose Measurement
    GLS_MEAS_CHAR,
    /// Glucose Measurement Context
    GLS_MEAS_CTX_CHAR,
    /// Glucose Feature
    GLS_FEATURE_CHAR,
    /// Record Access Control Point
    GLS_REC_ACCESS_CTRL_CHAR,
};

///Database Configuration Bit Field Flags
enum
{
    /// support of Glucose Measurement Context
    GLPS_MEAS_CTX_SUP        = 0x01,
};

/// Indication/notification configuration (put in feature flag to optimize memory usage)
enum
{
    /// Bit used to know if Glucose measurement notification is enabled
    GLPS_MEAS_NTF_CFG      = GLPS_IND_NTF_EVT(GLS_IDX_MEAS_NTF_CFG),
    /// Bit used to know if Glucose measurement context notification is enabled
    GLPS_MEAS_CTX_NTF_CFG  = GLPS_IND_NTF_EVT(GLS_IDX_MEAS_CTX_NTF_CFG),
    /// Bit used to know if Glucose measurement context indication is enabled
    GLPS_RACP_IND_CFG      = GLPS_IND_NTF_EVT(GLS_IDX_REC_ACCESS_CTRL_IND_CFG),
};

/// Type of request completed
enum
{
    /// Glucose measurement notification sent completed
    GLPS_SEND_MEAS_REQ_NTF_CMP,
    /// Record Access Control Point Response Indication
    GLPS_SEND_RACP_RSP_IND_CMP
};

/// Glucose service processing flags
enum
{
    /// Use To know if bond data are present
    GLPS_BOND_DATA_PRESENT,
    /// Flag used to know if there is an on-going record access control point request
    GLPS_RACP_ON_GOING,
    /// Service id sending a measurement
    GLPS_SENDING_MEAS,
    /// Measurement sent
    GLPS_MEAS_SENT,
    /// Measurement context sent
    GLPS_MEAS_CTX_SENT,
    /// RACP Response sent by application
    GLPS_RACP_RSP_SENT_BY_APP,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Glucose Profile Sensor environment variable per connection
struct glps_cnx_env
{
    /// Glucose service processing flags
    uint8_t flags;
    /// Event (notification/indication) configuration
    uint8_t evt_cfg;
};

/// Glucose Profile Sensor environment variable
struct glps_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct glps_cnx_env* env[GLPS_IDX_MAX];
    /// Glucose Service Start Handle
    uint16_t start_hdl;
    /// Glucose Feature
    uint16_t features;
    /// Measurement context supported
    uint8_t meas_ctx_supported;
    /// State of different task instances
    ke_state_t state[GLPS_IDX_MAX];
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
 * @brief Retrieve GLP service profile interface
 *
 * @return GLP service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* glps_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Pack Glucose measurement value
 *
 * @param[out] packed_meas Glucose measurement value packed
 * @param[in] meas_val Glucose measurement value
 * @param[in] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glps_pack_meas_value(uint8_t *packed_meas, const struct glp_meas* meas_val,
                             uint16_t seq_num);


/**
 ****************************************************************************************
 * @brief Pack Glucose measurement context value
 *
 * @param[out] packed_meas Glucose measurement context value packed
 * @param[in] meas_val Glucose measurement context value
 * @param[in] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glps_pack_meas_ctx_value(uint8_t *packed_meas_ctx,
                                 const struct glp_meas_ctx* meas_ctx_val,
                                 uint16_t seq_num);



/**
 ****************************************************************************************
 * @brief Unpack Record Access Control request
 *
 * @param[in] packed_val Record Access Control Point value packed
 * @param[out] racp_req Record Access Control Request value
 *
 * @return status of unpacking
 ****************************************************************************************
 */
uint8_t glps_unpack_racp_req(uint8_t *packed_val, uint16_t length,
                             struct glp_racp_req* racp_req);


/**
 ****************************************************************************************
 * @brief Pack Record Access Control response
 *
 * @param[out] packed_val Record Access Control Point value packed
 * @param[in] racp_rsp Record Access Control Response value
 *
 * @return size of packed data
 ****************************************************************************************
 */
uint8_t glps_pack_racp_rsp(uint8_t *packed_val,
                           struct glp_racp_rsp* racp_rsp);

/**
 ****************************************************************************************
 * @brief Send Record Access Control Response Indication
 *
 * @param[in] racp_rsp Record Access Control Response value
 * @param[in] racp_ind_src Application that requests to send RACP response indication.
 *
 * @return PRF_ERR_IND_DISABLED if indication disabled by peer device, GAP_ERR_NO_ERROR else.
 ****************************************************************************************
 */
uint8_t glps_send_racp_rsp(struct glp_racp_rsp * racp_rsp,
                           ke_task_id_t racp_ind_src);

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
void glps_task_init(struct ke_task_desc *task_desc);

#endif /* #if (BLE_GL_SENSOR) */

/// @} GLPS

#endif /* _GLPS_H_ */
