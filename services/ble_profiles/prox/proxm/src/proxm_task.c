/**
 ****************************************************************************************
 * @addtogroup PROXMTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_PROX_MONITOR)
#include "co_utils.h"

#include "gap.h"
#include "gapc.h"
#include "proxm_task.h"
#include "proxm.h"
#include "gattc_task.h"
#include "prf_types.h"
#include "prf_utils.h"

#include "ke_mem.h"

///// State machine used to retrieve services characteristics information

const struct prf_char_def proxm_svc_char[PROXM_SVC_NB][PROXM_SVCS_CHAR_NB] =  {
        [PROXM_LK_LOSS_SVC] = {{ ATT_CHAR_ALERT_LEVEL,
                                ATT_MANDATORY,
                                ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR}},

        [PROXM_IAS_SVC] =     {{ ATT_CHAR_ALERT_LEVEL,
                                ATT_MANDATORY,
                                ATT_CHAR_PROP_WR_NO_RESP}},

        [PROXM_TX_POWER_SVC] = {{ATT_CHAR_TX_POWER_LEVEL,
                                ATT_MANDATORY,
                                ATT_CHAR_PROP_RD}}
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable the Proximity Monitor role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int proxm_enable_req_handler(ke_msg_id_t const msgid,
                                    struct proxm_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;

    uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Proximity Monitor Environment
    struct proxm_env_tag *proxm_env = PRF_ENV_GET(PROXM, proxm);

    ASSERT_INFO(proxm_env != NULL, dest_id, src_id);
    if((state == PROXM_IDLE) && (proxm_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        proxm_env->env[conidx] = (struct proxm_cnx_env*) ke_malloc(sizeof(struct proxm_cnx_env),KE_MEM_ATT_DB);
        memset(proxm_env->env[conidx], 0, sizeof(struct proxm_cnx_env));

        if(param->con_type == PRF_CON_DISCOVERY)
        {
            proxm_env->env[conidx]->last_svc_req = PROXM_LK_LOSS_SVC;
            prf_disc_svc_send(&(proxm_env->prf_env), conidx, ATT_SVC_LINK_LOSS);

            // Go to DISCOVERING state
            ke_state_set(dest_id, PROXM_DISCOVERING);
        }
        else
        {
            proxm_env->env[conidx]->prox[PROXM_LK_LOSS_SVC]  = param->lls;
            proxm_env->env[conidx]->prox[PROXM_IAS_SVC]      = param->ias;
            proxm_env->env[conidx]->prox[PROXM_TX_POWER_SVC] = param->txps;

           //send APP confirmation that can start normal connection to TH
           proxm_enable_rsp_send(proxm_env, conidx, GAP_ERR_NO_ERROR);
        }

    }
    else if(state != PROXM_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        proxm_enable_rsp_send(proxm_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PROXM_RD_REQ message.
 * Request to read the LLS alert level.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int proxm_rd_req_handler(ke_msg_id_t const msgid,
                                  struct proxm_rd_req const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = GAP_ERR_NO_ERROR;
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t current_svc = PROXM_SVC_NB;

    if(state == PROXM_IDLE)
    {
        struct proxm_env_tag *proxm_env = PRF_ENV_GET(PROXM, proxm);

        ASSERT_INFO(proxm_env != NULL, dest_id, src_id);

        // environment variable not ready
        if(proxm_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        else
        {
            // Get the request type and handle value for this type of write
            switch(param->svc_code)
            {
                case PROXM_RD_LL_ALERT_LVL:
                    current_svc = PROXM_LK_LOSS_SVC;
                    status = proxm_validate_request(proxm_env, conidx, PROXM_LK_LOSS_SVC);
                    break;
                case PROXM_RD_TX_POWER_LVL:
                    current_svc = PROXM_TX_POWER_SVC;
                    status = proxm_validate_request(proxm_env, conidx, PROXM_TX_POWER_SVC);
                    break;
                default:
                    current_svc = PROXM_SVC_NB;
                    status = PRF_ERR_INVALID_PARAM;
                    break;
            }

            if(status == GAP_ERR_NO_ERROR)
            {
                prf_read_char_send(&(proxm_env->prf_env), conidx,
                        proxm_env->env[conidx]->prox[current_svc].characts[0].char_hdl,
                        proxm_env->env[conidx]->prox[current_svc].characts[0].char_ehdl_off,
                        proxm_env->env[conidx]->prox[current_svc].characts[0].val_hdl);
                // wait for end of read request
                proxm_env->env[conidx]->last_svc_req = param->svc_code;

                ke_state_set(dest_id, PROXM_BUSY);
            }
        }

        // request cannot be performed
        if(status != GAP_ERR_NO_ERROR)
        {
            struct proxm_rd_rsp * rsp = KE_MSG_ALLOC(PROXM_RD_RSP,
                   src_id, dest_id, proxm_rd_rsp);
            rsp->svc_code = param->svc_code;
            // set error status
            rsp->status = status;
            ke_msg_send(rsp);
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PROXM_WR_ALERT_LVL_REQ message.
 * Request to write either the LLS or IAS alert levels.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int proxm_wr_alert_lvl_req_handler(ke_msg_id_t const msgid,
                                          struct proxm_wr_alert_lvl_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = GAP_ERR_NO_ERROR;
    uint8_t  operation = 0x00;
    uint16_t val_hdl  = 0x0000;
    uint8_t conidx = KE_IDX_GET(dest_id);
    struct proxm_env_tag *proxm_env = PRF_ENV_GET(PROXM, proxm);

    if(state == PROXM_IDLE)
    {
        ASSERT_INFO(proxm_env != NULL, dest_id, src_id);

        // environment variable not ready
        if(proxm_env->env[conidx] == NULL)
        {
           status = PRF_APP_ERROR;
        }
        else
        {
            // Alert level only has 3 values, other not useful
            if(param->lvl <= PROXM_ALERT_HIGH)
            {
                // Get the request type and handle value for this type of write
                switch(param->svc_code)
                {
                    case PROXM_SET_LK_LOSS_ALERT:
                        operation = GATTC_WRITE;
                        val_hdl = proxm_env->env[conidx]->prox[PROXM_LK_LOSS_SVC].characts[0].val_hdl;
                        break;
                    case PROXM_SET_IMMDT_ALERT:
                        operation = GATTC_WRITE_NO_RESPONSE;
                        val_hdl  = proxm_env->env[conidx]->prox[PROXM_IAS_SVC].characts[0].val_hdl;
                        break;
                    default:
                        status = PRF_ERR_INVALID_PARAM;
                        break;
                }

                if(status == GAP_ERR_NO_ERROR)
                {
                    // Send GATT Write Request
                    prf_gatt_write(&(proxm_env->prf_env), conidx, val_hdl, (uint8_t *)&param->lvl,
                            sizeof(uint8_t), operation);
                    // wait for end of write request
                    ke_state_set(dest_id, PROXM_BUSY);
                }
            }
            else
            {
                //wrong level - not one of the possible 3
                status = PRF_ERR_INVALID_PARAM;
            }
        }
        // An error occurs, send the response with error status
        if(status != GAP_ERR_NO_ERROR)
        {
            struct proxm_wr_alert_lvl_rsp * rsp = KE_MSG_ALLOC(PROXM_WR_ALERT_LVL_RSP,
                    src_id, dest_id, proxm_wr_alert_lvl_rsp);
            // set error status
            rsp->status = status;

            ke_msg_send(rsp);
        }
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

    if(state == PROXM_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        uint16_t current_svc = PROXM_SVC_NB;
        const struct prf_char_def* current_char_def;

        struct proxm_env_tag *proxm_env = PRF_ENV_GET(PROXM, proxm);

        ASSERT_INFO(proxm_env != NULL, dest_id, src_id);
        ASSERT_INFO(proxm_env->env[conidx] != NULL, dest_id, src_id);

        // check if a valid service is found
        if (attm_uuid16_comp((unsigned char *)ind->uuid, ind->uuid_len, ATT_SVC_LINK_LOSS))
        {
            current_svc = PROXM_LK_LOSS_SVC;
            current_char_def = &proxm_svc_char[PROXM_LK_LOSS_SVC][PROXM_LK_LOSS_CHAR];
        }
        else if(attm_uuid16_comp((unsigned char *)ind->uuid, ind->uuid_len, ATT_SVC_IMMEDIATE_ALERT))
        {
            current_svc = PROXM_IAS_SVC;
            current_char_def = &proxm_svc_char[PROXM_IAS_SVC][PROXM_IAS_CHAR];
        }
        else if(attm_uuid16_comp((unsigned char *)ind->uuid, ind->uuid_len, ATT_SVC_TX_POWER))
        {
            current_svc = PROXM_TX_POWER_SVC;
            current_char_def = &proxm_svc_char[PROXM_TX_POWER_SVC][PROXM_TX_POWER_CHAR];
        }

        // if a valid service found, put it into environment variable
        if(current_svc != PROXM_SVC_NB)
        {
            prf_extract_svc_info(ind, PROXM_CHAR_NB_MAX, current_char_def,
                    proxm_env->env[conidx]->prox[current_svc].characts, 0, NULL, NULL);

            proxm_env->env[conidx]->prox[current_svc].svc.shdl = ind->start_hdl;
            proxm_env->env[conidx]->prox[current_svc].svc.ehdl = ind->end_hdl;

            proxm_env->env[conidx]->nb_svc++;
        }
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
    // Get the address of the environment
    struct proxm_env_tag *proxm_env = PRF_ENV_GET(PROXM, proxm);
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint16_t current_att_svc = ATT_INVALID_UUID;
    uint8_t status = GAP_ERR_NO_ERROR;

    if(state == PROXM_DISCOVERING)
    {
        status = prf_check_svc_char_validity(PROXM_SVCS_CHAR_NB,
                    proxm_env->env[conidx]->prox[proxm_env->env[conidx]->last_svc_req].characts,
                    proxm_svc_char[proxm_env->env[conidx]->last_svc_req]);

        // Too many services found only one such service should exist
        if(proxm_env->env[conidx]->nb_svc > 1)
        {
            status = PRF_ERR_MULTIPLE_SVC;
        }
        // if an error append, send confirmation with error status and stop discovering
        if(status != GAP_ERR_NO_ERROR)
        {
            proxm_enable_rsp_send(proxm_env, conidx, status);
        }
        // if a valid service found, discover and then send confirmation message
        else
        {
            proxm_env->env[conidx]->nb_svc =0;
            proxm_env->env[conidx]->last_svc_req += 1;

            if((proxm_env->env[conidx]->last_svc_req) < PROXM_SVC_NB)
            {
                switch(proxm_env->env[conidx]->last_svc_req)
                {
                    case PROXM_LK_LOSS_SVC:
                        current_att_svc = ATT_SVC_LINK_LOSS;
                        break;
                    case PROXM_IAS_SVC:
                        current_att_svc = ATT_SVC_IMMEDIATE_ALERT;
                        break;
                    case PROXM_TX_POWER_SVC:
                        current_att_svc = ATT_SVC_TX_POWER;
                        break;
                    default : break;
                }

                prf_disc_svc_send(&(proxm_env->prf_env), conidx, current_att_svc);
            }
            else
            {
                proxm_enable_rsp_send(proxm_env, conidx, GAP_ERR_NO_ERROR);
            }
        }
    }
    else if (state == PROXM_BUSY)
    {
        switch(param->operation)
        {
            case GATTC_WRITE:
            case GATTC_WRITE_NO_RESPONSE:
            {
                struct proxm_wr_alert_lvl_rsp * rsp = KE_MSG_ALLOC(PROXM_WR_ALERT_LVL_RSP,
                        prf_dst_task_get(&(proxm_env->prf_env), conidx),
                        prf_src_task_get(&(proxm_env->prf_env), conidx),
                        proxm_wr_alert_lvl_rsp);
                // set error status
                rsp->status = param->status;
                ke_msg_send(rsp);

                ke_state_set(dest_id, PROXM_IDLE);
            }
            break;

            case GATTC_READ:
            {
                if(param->status != GAP_ERR_NO_ERROR)
                {
                    struct proxm_rd_rsp * rsp = KE_MSG_ALLOC(PROXM_RD_RSP,
                            prf_dst_task_get(&(proxm_env->prf_env), conidx),
                            prf_src_task_get(&(proxm_env->prf_env), conidx),
                            proxm_rd_rsp);
                    // set error status
                    rsp->status = param->status;
                    rsp->svc_code = proxm_env->env[conidx]->last_svc_req;
                    ke_msg_send(rsp);
                }
                ke_state_set(dest_id, PROXM_IDLE);
            }
            break;
            default: break;
        }
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
    if(ke_state_get(dest_id) == PROXM_BUSY)
    {
        struct proxm_env_tag *proxm_env = PRF_ENV_GET(PROXM, proxm);
        uint8_t conidx = KE_IDX_GET(dest_id);

        ASSERT_INFO(proxm_env != NULL, dest_id, src_id);
        ASSERT_INFO(proxm_env->env[conidx] != NULL, dest_id, src_id);

        struct proxm_rd_rsp * rsp = KE_MSG_ALLOC(PROXM_RD_RSP,
                                                   prf_dst_task_get(&(proxm_env->prf_env), conidx),
                                                   dest_id,
                                                   proxm_rd_rsp);
        rsp->status = GAP_ERR_NO_ERROR;
        rsp->svc_code = proxm_env->env[conidx]->last_svc_req;
        rsp->value = param->value[0];
        ke_msg_send(rsp);
    }
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
KE_MSG_HANDLER_TAB(proxm)
{
    {GATTC_SDP_SVC_IND,           (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {PROXM_ENABLE_REQ,            (ke_msg_func_t)proxm_enable_req_handler},
    {PROXM_WR_ALERT_LVL_REQ,      (ke_msg_func_t)proxm_wr_alert_lvl_req_handler},
    {PROXM_RD_REQ,                (ke_msg_func_t)proxm_rd_req_handler},
    {GATTC_READ_IND,              (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_CMP_EVT,               (ke_msg_func_t)gattc_cmp_evt_handler},
};

void proxm_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct proxm_env_tag *proxm_env = PRF_ENV_GET(PROXM, proxm);

    task_desc->msg_handler_tab = proxm_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(proxm_msg_handler_tab);
    task_desc->state           = proxm_env->state;
    task_desc->idx_max         = PROXM_IDX_MAX;
}

#endif //BLE_PROX_MONITOR

/// @} PROXMTASK
