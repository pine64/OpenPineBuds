/**
 ****************************************************************************************
 * @addtogroup GLPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_GL_COLLECTOR)
#include "gap.h"
#include "attm.h"
#include "glpc_task.h"
#include "glpc.h"
#include "gattc_task.h"
#include "gapc_task.h"
#include "ke_timer.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve Glucose service characteristics information
const struct prf_char_def glpc_gls_char[GLPC_CHAR_MAX] =
{

    /// Glucose Measurement
    [GLPC_CHAR_MEAS]            = {ATT_CHAR_GLUCOSE_MEAS,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_NTF},
    /// Glucose Measurement Context
    [GLPC_CHAR_MEAS_CTX]        = {ATT_CHAR_GLUCOSE_MEAS_CTX,
                                   ATT_OPTIONAL,
                                   ATT_CHAR_PROP_NTF},
    /// Glucose Feature
    [GLPC_CHAR_FEATURE]         = {ATT_CHAR_GLUCOSE_FEATURE,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_RD},
    /// Record Access control point
    [GLPC_CHAR_RACP]            = {ATT_CHAR_REC_ACCESS_CTRL_PT,
                                   ATT_MANDATORY,
                                   ATT_CHAR_PROP_WR|ATT_CHAR_PROP_IND},
};

/// State machine used to retrieve Glucose service characteristic description information
const struct prf_char_desc_def glpc_gls_char_desc[GLPC_DESC_MAX] =
{
     /// Glucose Measurement client config
    [GLPC_DESC_MEAS_CLI_CFG]     = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, GLPC_CHAR_MEAS},

        /// Glucose Measurement context client config
    [GLPC_DESC_MEAS_CTX_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, GLPC_CHAR_MEAS_CTX},

        /// Record Access control point client config
    [GLPC_DESC_RACP_CLI_CFG]     = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, GLPC_CHAR_RACP},
};


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_ENABLE_REQ message.
 * The handler enables the Glucose Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int glpc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct glpc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;

    uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Glucose Profile Client Role Task Environment
    struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

    ASSERT_INFO(glpc_env != NULL, dest_id, src_id);
    if((state == GLPC_IDLE) && (glpc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        glpc_env->env[conidx] = (struct glpc_cnx_env*) ke_malloc(sizeof(struct glpc_cnx_env),KE_MEM_ATT_DB);
        memset(glpc_env->env[conidx], 0, sizeof(struct glpc_cnx_env));

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering GLS on peer
            prf_disc_svc_send(&(glpc_env->prf_env), conidx, ATT_SVC_GLUCOSE);

            // Go to DISCOVERING state
            ke_state_set(dest_id, GLPC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            glpc_env->env[conidx]->gls = param->gls;

            //send APP confirmation that can start normal connection to TH
            glpc_enable_rsp_send(glpc_env, conidx, GAP_ERR_NO_ERROR);
        }
    }

    else if(state != GLPC_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        glpc_enable_rsp_send(glpc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_SDP_SVC_IND_HANDLER message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_sdp_svc_ind_handler(ke_msg_id_t const msgid,
                                             struct gattc_sdp_svc_ind const *ind,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state == GLPC_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

        ASSERT_INFO(glpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(glpc_env->env[conidx] != NULL, dest_id, src_id);

        if(glpc_env->env[conidx]->nb_svc == 0)
        {
            // Retrieve GLS characteristics and descriptors
            prf_extract_svc_info(ind, GLPC_CHAR_MAX, &glpc_gls_char[0],  &glpc_env->env[conidx]->gls.chars[0],
                                      GLPC_DESC_MAX, &glpc_gls_char_desc[0], &glpc_env->env[conidx]->gls.descs[0]);

            //Even if we get multiple responses we only store 1 range
            glpc_env->env[conidx]->gls.svc.shdl = ind->start_hdl;
            glpc_env->env[conidx]->gls.svc.ehdl = ind->end_hdl;
        }

        glpc_env->env[conidx]->nb_svc++;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * This generic event is received for different requests, so need to keep track.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gattc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status;
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);
    uint8_t conidx = KE_IDX_GET(dest_id);

    if(state == GLPC_DISCOVERING)
    {
        status = param->status;

        if (param->status == ATT_ERR_NO_ERROR)
        {
            // check characteristic validity
            if(glpc_env->env[conidx]->nb_svc == 1)
            {
                status = prf_check_svc_char_validity(GLPC_CHAR_MAX, glpc_env->env[conidx]->gls.chars,
                        glpc_gls_char);
            }
            // too much services
            else if (glpc_env->env[conidx]->nb_svc > 1)
            {
                status = PRF_ERR_MULTIPLE_SVC;
            }
            // no services found
            else
            {
                status = PRF_ERR_STOP_DISC_CHAR_MISSING;
            }

            // check descriptor validity
            if(status == GAP_ERR_NO_ERROR)
            {
                status = prf_check_svc_char_desc_validity(GLPC_DESC_MAX,
                        glpc_env->env[conidx]->gls.descs,
                        glpc_gls_char_desc,
                        glpc_env->env[conidx]->gls.chars);
            }
        }

        glpc_enable_rsp_send(glpc_env, conidx, status);


    }
    else if (state == GLPC_IDLE)
    {
        switch(param->operation)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                // RACP write done.
                if(glpc_env->env[conidx]->last_uuid_req == ATT_CHAR_REC_ACCESS_CTRL_PT)
                {
                    // an error occurs while writing RACP command
                    if(param->status != GAP_ERR_NO_ERROR)
                    {
                        struct glpc_racp_rsp * rsp = KE_MSG_ALLOC(GLPC_RACP_RSP,
                                prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id,
                                glpc_racp_rsp);

                        // set error status
                        rsp->status = param->status;

                        // stop timer.
                        ke_timer_clear(GLPC_RACP_REQ_TIMEOUT, dest_id);

                        ke_msg_send(rsp);
                        glpc_env->env[conidx]->last_uuid_req = 0;
                    }
                }
                else
                {
                    bool finished = false;

                    // Restore if measurement context notification should be register in from
                    // unused variable
                    bool meas_ctx_en = glpc_env->env[conidx]->meas_ctx_en;

                    // Registration succeed
                    if(param->status == GAP_ERR_NO_ERROR)
                    {
                        // Glucose measurement  notification registration done
                        if(glpc_env->env[conidx]->last_uuid_req == GLPC_DESC_MEAS_CLI_CFG)
                        {
                            // register to RACP indications
                            prf_gatt_write_ntf_ind(&(glpc_env->prf_env), conidx,
                                    glpc_env->env[conidx]->gls.descs[GLPC_DESC_RACP_CLI_CFG].desc_hdl,
                                    PRF_CLI_START_IND);
                            glpc_env->env[conidx]->last_uuid_req = GLPC_DESC_RACP_CLI_CFG;
                        }
                        // Record access control point indication registration done
                        // Register to Glucose Measurement Context notifications if requested.
                        else if((glpc_env->env[conidx]->last_uuid_req == GLPC_DESC_RACP_CLI_CFG)
                                && meas_ctx_en)
                        {
                            // register to Glucose Measurement Context notifications
                            prf_gatt_write_ntf_ind(&(glpc_env->prf_env), conidx,
                                    glpc_env->env[conidx]->gls.descs[GLPC_DESC_MEAS_CTX_CLI_CFG].desc_hdl,
                                    PRF_CLI_START_NTF);
                            glpc_env->env[conidx]->last_uuid_req = GLPC_DESC_MEAS_CTX_CLI_CFG;
                        }
                        // All registration done
                        else
                        {
                            // indication/notification registration finished
                            finished = true;
                        }
                    }

                    // send status if registration done or if an error occurs.
                    if((param->status != GAP_ERR_NO_ERROR) || (finished))
                    {
                        struct glpc_register_rsp * cfm = KE_MSG_ALLOC(GLPC_REGISTER_RSP,
                                prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id,
                                glpc_register_rsp);
                        // set error status
                        cfm->status = param->status;

                        ke_msg_send(cfm);
                    }
                }
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GAP_ERR_NO_ERROR)
                {
                    struct glpc_read_features_rsp * rsp = KE_MSG_ALLOC(GLPC_READ_FEATURES_RSP,
                            prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id,
                            glpc_read_features_rsp);

                    // set error status
                    rsp->status = param->status;

                    ke_msg_send(rsp);
                }
            }
            break;
            default: break;
        }
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_READ_FEATURES_REQ message.
 * Send by application task, it's used to retrieve Glucose Sensor features.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int glpc_read_features_req_handler(ke_msg_id_t const msgid,
                                          void const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == GLPC_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

        ASSERT_INFO(glpc_env != NULL, dest_id, src_id);

        // environment variable not ready
        if(glpc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        else
        {
            status = glpc_validate_request(glpc_env, conidx, GLPC_CHAR_FEATURE);

            // request can be performed
            if(status == GAP_ERR_NO_ERROR)
            {
                // read glucose sensor featur
                prf_read_char_send(&(glpc_env->prf_env), conidx, glpc_env->env[conidx]->gls.svc.shdl,
                        glpc_env->env[conidx]->gls.svc.ehdl,  glpc_env->env[conidx]->gls.chars[GLPC_CHAR_FEATURE].val_hdl);
            }
        }
    }

    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct glpc_read_features_rsp * rsp = KE_MSG_ALLOC(GLPC_READ_FEATURES_RSP,
                src_id, dest_id, glpc_read_features_rsp);
        // set error status
        rsp->status = status;

        ke_msg_send(rsp);
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_IND message.
 * Generic event received after every simple read command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                    struct gattc_read_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state == GLPC_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

        ASSERT_INFO(glpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(glpc_env->env[conidx] != NULL, dest_id, src_id);

        struct glpc_read_features_rsp * rsp = KE_MSG_ALLOC(GLPC_READ_FEATURES_RSP,
                                                           prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id,
                                                           glpc_read_features_rsp);
        // set error status
        rsp->status = ATT_ERR_NO_ERROR;
        // unpack feature information
        rsp->features = co_read16p(param->value);

        ke_msg_send(rsp);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_REGISTER_REQ message.
 * When receiving this request, Glucose collector register to measurement notifications
 * and RACP indications.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int glpc_register_req_handler(ke_msg_id_t const msgid,
                                     struct glpc_register_req const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == GLPC_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

        ASSERT_INFO(glpc_env != NULL, dest_id, src_id);

        // environment variable not ready
        if(glpc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        // check if client characteristics are present
        else if ((glpc_env->env[conidx]->gls.descs[GLPC_DESC_MEAS_CLI_CFG].desc_hdl == ATT_INVALID_HANDLE)
                || (glpc_env->env[conidx]->gls.descs[GLPC_DESC_RACP_CLI_CFG].desc_hdl == ATT_INVALID_HANDLE)
                || ((param->meas_ctx_en)
                        && (glpc_env->env[conidx]->gls.descs[GLPC_DESC_MEAS_CTX_CLI_CFG].desc_hdl == ATT_INVALID_HANDLE)))
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
        // request can be performed
        else
        {
            // register to notification
            prf_gatt_write_ntf_ind(&(glpc_env->prf_env), conidx,
                                     glpc_env->env[conidx]->gls.descs[GLPC_DESC_MEAS_CLI_CFG].desc_hdl,
                                     PRF_CLI_START_NTF);
            glpc_env->env[conidx]->last_uuid_req = GLPC_DESC_MEAS_CLI_CFG;

            // save if measurement context notification should be register in an unused variable
            glpc_env->env[conidx]->meas_ctx_en = param->meas_ctx_en;

            status = GAP_ERR_NO_ERROR;
        }
    }

    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct glpc_register_rsp * cfm = KE_MSG_ALLOC(GLPC_REGISTER_RSP, src_id, dest_id, glpc_register_rsp);
        // set error status
        cfm->status = status;
        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPC_RACP_REQ message.
 * When receiving this request, Glucose collector send a RACP command to Glucose sensor.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int glpc_racp_req_handler(ke_msg_id_t const msgid,
                                     struct glpc_racp_req const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == GLPC_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

        ASSERT_INFO(glpc_env != NULL, dest_id, src_id);

        // environment variable not ready
        if(glpc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        else
        {
            status = glpc_validate_request(glpc_env, conidx, GLPC_CHAR_RACP);

            // request can be performed
            if(status == GAP_ERR_NO_ERROR)
            {
                // send command request
                uint8_t value[GLP_REC_ACCESS_CTRL_MAX_LEN];
                uint16_t length;

                // pack Record Access Control Point request
                length = glpc_pack_racp_req((uint8_t*)&(value[0]), &(param->racp_req));

                // write Record Access Control Point request
                prf_gatt_write(&(glpc_env->prf_env), conidx,
                        glpc_env->env[conidx]->gls.chars[GLPC_CHAR_RACP].val_hdl,
                        &(value[0]),length, GATTC_WRITE);

                glpc_env->env[conidx]->last_uuid_req = ATT_CHAR_REC_ACCESS_CTRL_PT;

                // start the timer; will destroy the link if it expires
                ke_timer_set(GLPC_RACP_REQ_TIMEOUT, dest_id, GLPC_RACP_TIMEOUT);
            }
        }
    }

    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct glpc_racp_rsp * rsp = KE_MSG_ALLOC(GLPC_RACP_RSP, src_id, dest_id, glpc_racp_rsp);
        // set error status
        rsp->status = status;
        ke_msg_send(rsp);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                         struct gattc_event_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state != GLPC_FREE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

        ASSERT_INFO(glpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(glpc_env->env[conidx] != NULL, dest_id, src_id);

        switch(param->type)
        {
            case GATTC_INDICATE:
            {
                // confirm that indication has been correctly received
                struct gattc_event_cfm * cfm = KE_MSG_ALLOC(GATTC_EVENT_CFM, src_id, dest_id, gattc_event_cfm);
                cfm->handle = param->handle;
                ke_msg_send(cfm);

                // check if it's a RACP indication
                if (glpc_env->env[conidx]->gls.chars[GLPC_CHAR_RACP].val_hdl == param->handle)
                {
                    struct glpc_racp_rsp * rsp = KE_MSG_ALLOC(GLPC_RACP_RSP,
                            prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id,
                            glpc_racp_rsp);

                    // set error status
                    rsp->status = GAP_ERR_NO_ERROR;

                    // stop timer.
                    ke_timer_clear(GLPC_RACP_REQ_TIMEOUT, dest_id);

                    // unpack RACP response indication.
                    glpc_unpack_racp_rsp((uint8_t*)param->value, &(rsp->racp_rsp));
                    ke_msg_send(rsp);
                }
            }
            break;
            case GATTC_NOTIFY:
            {
                // check if it's a Glucose measurement notification
                if (glpc_env->env[conidx]->gls.chars[GLPC_CHAR_MEAS].val_hdl == param->handle)
                {

                    // build a GLPC_MEAS_IND message with glucose measurement value
                    struct glpc_meas_ind * ind = KE_MSG_ALLOC(GLPC_MEAS_IND,
                            prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id,
                            glpc_meas_ind);

                    // unpack Glucose measurement.
                    glpc_unpack_meas_value((uint8_t*) param->value, &(ind->meas_val), &(ind->seq_num));

                    if(glpc_env->env[conidx]->last_uuid_req == ATT_CHAR_REC_ACCESS_CTRL_PT)
                    {
                        // restart the timer; will destroy the link if it expires
                        ke_timer_set(GLPC_RACP_REQ_TIMEOUT, dest_id, GLPC_RACP_TIMEOUT);
                    }

                    ke_msg_send(ind);
                }
                // check if it's a Glucose measurement context notification
                else if(glpc_env->env[conidx]->gls.chars[GLPC_CHAR_MEAS_CTX].val_hdl == param->handle)
                {
                    // build a GLPC_MEAS_CTX_IND message with glucose measurement context value
                    struct glpc_meas_ctx_ind * ind = KE_MSG_ALLOC(GLPC_MEAS_CTX_IND,
                            prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id,
                            glpc_meas_ctx_ind);

                    // unpack Glucose measurement context.
                    glpc_unpack_meas_ctx_value((uint8_t*) param->value, &(ind->ctx), &(ind->seq_num));

                    if(glpc_env->env[conidx]->last_uuid_req == ATT_CHAR_REC_ACCESS_CTRL_PT)
                    {
                        // restart the timer; will destroy the link if it expires
                        ke_timer_set(GLPC_RACP_REQ_TIMEOUT, dest_id, GLPC_RACP_TIMEOUT);
                    }

                    ke_msg_send(ind);
                }
            }
            break;
            default: break;
        }
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief RACP request not executed by peer device or is freezed.
 * It can be a connection problem, disconnect the link.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GATTC).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
__STATIC int glpc_racp_req_timeout_handler(ke_msg_id_t const msgid, void const *param,
                                         ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state != GLPC_FREE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

        ASSERT_INFO(glpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(glpc_env->env[conidx] != NULL, dest_id, src_id);

        // inform that racp execution is into a timeout state
        struct glpc_racp_rsp * rsp = KE_MSG_ALLOC(GLPC_RACP_RSP,
                prf_dst_task_get(&(glpc_env->prf_env), conidx), dest_id, glpc_racp_rsp);
        // set error status
        rsp->status = PRF_ERR_PROC_TIMEOUT;

        ke_msg_send(rsp);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
KE_MSG_HANDLER_TAB(glpc)
{
    {GATTC_SDP_SVC_IND,             (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {GLPC_READ_FEATURES_REQ,        (ke_msg_func_t)glpc_read_features_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GLPC_REGISTER_REQ,             (ke_msg_func_t)glpc_register_req_handler},
    {GLPC_RACP_REQ,                 (ke_msg_func_t)glpc_racp_req_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_EVENT_REQ_IND,           (ke_msg_func_t)gattc_event_ind_handler},
    {GLPC_RACP_REQ_TIMEOUT,         (ke_msg_func_t)glpc_racp_req_timeout_handler},
    {GLPC_ENABLE_REQ,               (ke_msg_func_t)glpc_enable_req_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

void glpc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct glpc_env_tag *glpc_env = PRF_ENV_GET(GLPC, glpc);

    task_desc->msg_handler_tab = glpc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(glpc_msg_handler_tab);
    task_desc->state           = glpc_env->state;
    task_desc->idx_max         = GLPC_IDX_MAX;
}

#endif /* (BLE_GL_COLLECTOR) */
/// @} GLPCTASK
