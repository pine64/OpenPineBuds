#ifndef _GLPC_H_
#define _GLPC_H_

/**
 ****************************************************************************************
 * @addtogroup GLPC Glucose Profile Collector
 * @ingroup GLP
 * @brief Glucose Profile Collector
 *
 * The GLPC is responsible for providing Glucose Profile Collector functionalities
 * to upper layer module or application. The device using this profile takes the role
 * of Glucose Profile Collector.
 *
 * Glucose Profile Collector. (GLPC): A GLPC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can interpret Glucose
 * measurement in a way suitable to the user application.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_GL_COLLECTOR)
#include "glp_common.h"
#include "ke_task.h"
#include "prf.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "glpc_task.h"
/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximum number of Glucose Collector task instances
#define GLPC_IDX_MAX    (BLE_CONNECTION_MAX)


/// 30 seconds record access control point timer
#define GLPC_RACP_TIMEOUT                  0x0BB8

/// Possible states of the GLPC task
enum glpc_state
{
    /// Not Connected state
    GLPC_FREE,
    /// IDLE state
    GLPC_IDLE,
    /// Discovering Glucose SVC and Chars
    GLPC_DISCOVERING,

    /// Number of defined states.
    GLPC_STATE_MAX
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Glucose Profile Collector environment variable per connection
struct glpc_cnx_env
{
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// counter used to check service uniqueness
    uint8_t nb_svc;
    /// used to store if measurement context
    uint8_t meas_ctx_en;
    ///HTS characteristics
    struct gls_content gls;
};

/// Glucose Profile Collector environment variable
struct glpc_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Environment variable pointer for each connections
    struct glpc_cnx_env* env[GLPC_IDX_MAX];
    /// State of different task instances
    ke_state_t state[GLPC_IDX_MAX];
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve GLP client profile interface
 *
 * @return GLP client profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* glpc_prf_itf_get(void);

/**
 ****************************************************************************************
 * @brief Send Glucose ATT DB discovery results to GLPC host.
 ****************************************************************************************
 */
void glpc_enable_rsp_send(struct glpc_env_tag *glpc_env, uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack Glucose measurement value
 *
 * @param[in] packed_meas Glucose measurement value packed
 * @param[out] meas_val Glucose measurement value
 * @param[out] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glpc_unpack_meas_value(uint8_t *packed_meas, struct glp_meas* meas_val,
                              uint16_t* seq_num);


/**
 ****************************************************************************************
 * @brief Unpack Glucose measurement context value
 *
 * @param[in] packed_meas Glucose measurement context value packed
 * @param[out] meas_val Glucose measurement context value
 * @param[out] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glpc_unpack_meas_ctx_value(uint8_t *packed_meas_ctx,
                                 struct glp_meas_ctx* meas_ctx_val,
                                 uint16_t* seq_num);



/**
 ****************************************************************************************
 * @brief Pack Record Access Control request
 *
 * @param[out] packed_val Record Access Control Point value packed
 * @param[in] racp_req Record Access Control Request value
 *
 * @return size of packed data
 ****************************************************************************************
 */
uint8_t glpc_pack_racp_req(uint8_t *packed_val,
                               const struct glp_racp_req* racp_req);


/**
 ****************************************************************************************
 * @brief Unpack Record Access Control response
 *
 * @param[in] packed_val Record Access Control Point value packed
 * @param[out] racp_rsp Record Access Control Response value
 *
 * @return size of packed data
 ****************************************************************************************
 */
uint8_t glpc_unpack_racp_rsp(uint8_t *packed_val,
                                      struct glp_racp_rsp* racp_rsp);


/**
 ****************************************************************************************
 * @brief Check if collector request is possible or not
 *
 * @param[in] conidx Connection Index.
 * @param[in] atthdl Attribute handle.
 *
 * @return GAP_ERR_NO_ERROR if request can be performed, error code else.
 ****************************************************************************************
 */
uint8_t glpc_validate_request(struct glpc_env_tag *glpc_env, uint8_t conidx, uint8_t char_code);


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
void glpc_task_init(struct ke_task_desc *task_desc);

#endif /* (BLE_GL_COLLECTOR) */

/// @} GLPC

#endif /* _GLPC_H_ */
