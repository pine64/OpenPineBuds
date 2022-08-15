/**
 ****************************************************************************************
 * @addtogroup BASCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BATT_CLIENT)

#include "gap.h"
#include "basc.h"
#include "basc_task.h"
#include "gattc_task.h"
#include "co_math.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// State machine used to retrieve Battery Service characteristics information
const struct prf_char_def basc_bas_char[BAS_CHAR_MAX] =
{
    /// Battery Level
    [BAS_CHAR_BATT_LEVEL]  = {ATT_CHAR_BATTERY_LEVEL,
                              ATT_MANDATORY,
                              ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve Battery Service characteristic description information
const struct prf_char_desc_def basc_bas_char_desc[BAS_DESC_MAX] =
{
    /// Battery Level Characteristic Presentation Format
    [BAS_DESC_BATT_LEVEL_PRES_FORMAT]   = {ATT_DESC_CHAR_PRES_FORMAT,  ATT_OPTIONAL, BAS_CHAR_BATT_LEVEL},
    /// Battery Level Client Config
    [BAS_DESC_BATT_LEVEL_CFG]           = {ATT_DESC_CLIENT_CHAR_CFG,   ATT_OPTIONAL, BAS_CHAR_BATT_LEVEL},

};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BASC_ENABLE_REQ message.
 * The handler enables the Battery Service Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int basc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct basc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    int msg_status = KE_MSG_CONSUMED;

    uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Battery service Client Role Task Environment
    struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);

    ASSERT_INFO(basc_env != NULL, dest_id, src_id);
    if((state == BASC_IDLE) && (basc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        basc_env->env[conidx] = (struct basc_cnx_env*) ke_malloc(sizeof(struct basc_cnx_env),KE_MEM_ATT_DB);
        memset(basc_env->env[conidx], 0, sizeof(struct basc_cnx_env));

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering BAS on peer
            prf_disc_svc_send(&(basc_env->prf_env), conidx, ATT_SVC_BATTERY_SERVICE);

            // Go to DISCOVERING state
            ke_state_set(dest_id, BASC_BUSY);
            basc_env->env[conidx]->operation = ke_param2msg(param);
            msg_status = KE_MSG_NO_FREE;
        }
        //normal connection, get saved att details
        else
        {
            basc_env->env[conidx]->bas_nb = param->bas_nb;
            memcpy(&(basc_env->env[conidx]->bas[0]), &(param->bas[0]), sizeof(struct bas_content) * BASC_NB_BAS_INSTANCES_MAX);

            //send APP confirmation that can start normal connection to TH
            basc_enable_rsp_send(basc_env, conidx, GAP_ERR_NO_ERROR);
        }
    }
    else if(state != BASC_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        basc_enable_rsp_send(basc_env, conidx, status);
    }

    return (msg_status);
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

    if(state == BASC_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);

        ASSERT_INFO(basc_env != NULL, dest_id, src_id);
        ASSERT_INFO(basc_env->env[conidx] != NULL, dest_id, src_id);

        if(basc_env->env[conidx]->bas_nb < BASC_NB_BAS_INSTANCES_MAX)
        {
            // Retrieve DIS characteristics
            prf_extract_svc_info(ind, BAS_CHAR_MAX, &basc_bas_char[0],
                    &(basc_env->env[conidx]->bas[basc_env->env[conidx]->bas_nb].chars[0]),
                    BAS_DESC_MAX, &basc_bas_char_desc[0],
                    &(basc_env->env[conidx]->bas[basc_env->env[conidx]->bas_nb].descs[0]));

            //Even if we get multiple responses we only store 1 range
            basc_env->env[conidx]->bas[basc_env->env[conidx]->bas_nb].svc.shdl = ind->start_hdl;
            basc_env->env[conidx]->bas[basc_env->env[conidx]->bas_nb].svc.ehdl = ind->end_hdl;
        }

        basc_env->env[conidx]->bas_nb++;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BASC_READ_INFO_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int basc_read_info_req_handler(ke_msg_id_t const msgid, struct basc_read_info_req const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == BASC_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);
        ASSERT_INFO(basc_env != NULL, dest_id, src_id);
        // environment variable not ready
        if(basc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        // check parameter range
        else if(param->bas_nb > basc_env->env[conidx]->bas_nb)
        {
            status = PRF_ERR_INVALID_PARAM;
        }
        else
        {
            uint16_t handle = ATT_INVALID_HANDLE;
            status = PRF_ERR_INEXISTENT_HDL;

            // check requested info
            switch(param->info)
            {
                /// Battery Level value
                case BASC_BATT_LVL_VAL:
                {
                    handle = basc_env->env[conidx]->bas[param->bas_nb].chars[BAS_CHAR_BATT_LEVEL].val_hdl;
                }break;
                /// Battery Level Client Characteristic Configuration
                case BASC_NTF_CFG:
                {
                    handle = basc_env->env[conidx]->bas[param->bas_nb].descs[BAS_DESC_BATT_LEVEL_CFG].desc_hdl;
                }break;
                /// Battery Level Characteristic Presentation Format
                case BASC_BATT_LVL_PRES_FORMAT:
                {
                    handle = basc_env->env[conidx]->bas[param->bas_nb].descs[BAS_DESC_BATT_LEVEL_PRES_FORMAT].desc_hdl;
                }break;

                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                }break;
            }

            if(handle != ATT_INVALID_HANDLE)
            {
                status = GAP_ERR_NO_ERROR;
                // read information
                prf_read_char_send(&(basc_env->prf_env), conidx,
                        basc_env->env[conidx]->bas[param->bas_nb].svc.shdl,
                        basc_env->env[conidx]->bas[param->bas_nb].svc.ehdl,  handle);

                // store context of request and go into busy state
                basc_env->env[conidx]->operation = ke_param2msg(param);
                ke_state_set(dest_id, BASC_BUSY);
                msg_status = KE_MSG_NO_FREE;
            }
        }
    }
    // process message later
    else if (state == BASC_BUSY)
    {
        status = GAP_ERR_NO_ERROR;
        msg_status = KE_MSG_SAVED;
    }


    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct basc_read_info_rsp * rsp = KE_MSG_ALLOC(BASC_READ_INFO_RSP,
                src_id, dest_id, basc_read_info_rsp);
        // set error status
        rsp->status = status;
        rsp->info   = param->info;
        rsp->bas_nb   = param->bas_nb;

        ke_msg_send(rsp);
    }

    return (msg_status);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BASC_BATT_LEVEL_NTF_CFG_REQ message.
 * It allows configuration of the peer ntf/stop characteristic for Battery Level Characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int basc_batt_level_ntf_cfg_req_handler(ke_msg_id_t const msgid,
                                               struct basc_batt_level_ntf_cfg_req const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{

    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == BASC_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);
        ASSERT_INFO(basc_env != NULL, dest_id, src_id);
        // environment variable not ready
        if(basc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        // check parameter range
        else if((param->bas_nb > basc_env->env[conidx]->bas_nb)
                || (param->ntf_cfg > PRF_CLI_START_NTF))
        {
            status = PRF_ERR_INVALID_PARAM;
        }
        else
        {
            uint16_t handle = basc_env->env[conidx]->bas[param->bas_nb].descs[BAS_DESC_BATT_LEVEL_CFG].desc_hdl;
            status = PRF_ERR_INEXISTENT_HDL;

            if(handle == ATT_INVALID_HDL)
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
            else
            {
                status = GAP_ERR_NO_ERROR;
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&basc_env->prf_env, conidx, handle, param->ntf_cfg);
                // store context of request and go into busy state
                basc_env->env[conidx]->operation = ke_param2msg(param);
                ke_state_set(dest_id, BASC_BUSY);
                msg_status = KE_MSG_NO_FREE;
            }
        }
    }
    // process message later
    else if (state == BASC_BUSY)
    {
        status = GAP_ERR_NO_ERROR;
        msg_status = KE_MSG_SAVED;
    }


    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct basc_batt_level_ntf_cfg_rsp * rsp = KE_MSG_ALLOC(BASC_BATT_LEVEL_NTF_CFG_RSP,
                src_id, dest_id, basc_batt_level_ntf_cfg_rsp);
        // set error status
        rsp->status = status;
        rsp->bas_nb = param->bas_nb;

        ke_msg_send(rsp);
    }

    return (msg_status);
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
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Get the address of the environment
    struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);

    // sanity check
    if((state == BASC_BUSY) && (basc_env->env[conidx] != NULL)
            && (basc_env->env[conidx]->operation != NULL))
    {
        switch(basc_env->env[conidx]->operation->id)
        {
            case BASC_ENABLE_REQ:
            {
                uint8_t status = param->status;

                if (param->status == ATT_ERR_NO_ERROR)
                {
                    // check characteristic validity
                    if(basc_env->env[conidx]->bas_nb > 0)
                    {
                        uint8_t i;
                        for (i = 0 ; (i < co_min(basc_env->env[conidx]->bas_nb, BASC_NB_BAS_INSTANCES_MAX))
                        && (status == GAP_ERR_NO_ERROR) ; i++)
                        {
                            status = prf_check_svc_char_validity(BAS_CHAR_MAX, basc_env->env[conidx]->bas[i].chars,
                                    basc_bas_char);

                            // check descriptor validity
                            if(status == GAP_ERR_NO_ERROR)
                            {
                                struct prf_char_desc_def bas_desc[BAS_DESC_MAX];
                                memcpy(bas_desc, basc_bas_char_desc, sizeof(basc_bas_char_desc));

                                if (basc_env->env[conidx]->bas_nb > 1)
                                {
                                    bas_desc[BAS_DESC_BATT_LEVEL_PRES_FORMAT].req_flag = ATT_MANDATORY;
                                }
                                if ((basc_env->env[conidx]->bas[i].chars[BAS_CHAR_BATT_LEVEL].prop & ATT_CHAR_PROP_NTF) == ATT_CHAR_PROP_NTF)
                                {
                                    bas_desc[BAS_DESC_BATT_LEVEL_CFG].req_flag = ATT_MANDATORY;
                                }

                                status = prf_check_svc_char_desc_validity(BAS_DESC_MAX,
                                        basc_env->env[conidx]->bas[i].descs, bas_desc,
                                        basc_env->env[conidx]->bas[i].chars);
                            }
                        }
                    }
                    // no services found
                    else
                    {
                        status = PRF_ERR_STOP_DISC_CHAR_MISSING;
                    }

                }
                basc_enable_rsp_send(basc_env, conidx, status);
            }break;
            case BASC_READ_INFO_REQ:
            {
                struct basc_read_info_req* req = (struct basc_read_info_req*) ke_msg2param(basc_env->env[conidx]->operation);

                struct basc_read_info_rsp * rsp = KE_MSG_ALLOC(BASC_READ_INFO_RSP,
                        prf_dst_task_get(&(basc_env->prf_env), conidx), dest_id, basc_read_info_rsp);
                // set error status
                rsp->status = param->status;
                rsp->bas_nb = req->bas_nb;
                rsp->info   = req->info;

                ke_msg_send(rsp);

            }break;
            case BASC_BATT_LEVEL_NTF_CFG_REQ:
            {
                struct basc_batt_level_ntf_cfg_req* req = (struct basc_batt_level_ntf_cfg_req*) ke_msg2param(basc_env->env[conidx]->operation);

                struct basc_batt_level_ntf_cfg_rsp * rsp = KE_MSG_ALLOC(BASC_BATT_LEVEL_NTF_CFG_RSP,
                        prf_dst_task_get(&(basc_env->prf_env), conidx), dest_id, basc_batt_level_ntf_cfg_rsp);
                // set error status
                rsp->status =param->status;
                rsp->bas_nb = req->bas_nb;

                ke_msg_send(rsp);
            }break;
            default:
            {
                // Not Expected at all
                ASSERT_ERR(0);
            }break;
        }

        // operation is over - go back to idle state
        ke_free(basc_env->env[conidx]->operation);
        basc_env->env[conidx]->operation = NULL;
        ke_state_set(dest_id, BASC_IDLE);
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

    if(state == BASC_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);
        struct basc_read_info_req* req = (struct basc_read_info_req*) ke_msg2param(basc_env->env[conidx]->operation);

        ASSERT_INFO(basc_env != NULL, dest_id, src_id);
        ASSERT_INFO(basc_env->env[conidx] != NULL, dest_id, src_id);


        struct basc_read_info_rsp * rsp = KE_MSG_ALLOC(BASC_READ_INFO_RSP,
                prf_dst_task_get(&(basc_env->prf_env),conidx), dest_id, basc_read_info_rsp);
        // set error status
        rsp->status = GAP_ERR_NO_ERROR;
        rsp->bas_nb = req->bas_nb;
        rsp->info   = req->info;

        switch(req->info)
        {
            /// Battery Level value
            case BASC_BATT_LVL_VAL:
            {
                rsp->data.batt_level = param->value[0];
            }break;
            /// Battery Level Client Characteristic Configuration
            case BASC_NTF_CFG:
            {
                rsp->data.ntf_cfg    = co_read16p(&param->value[0]);

            }break;
            /// Battery Level Characteristic Presentation Format
            case BASC_BATT_LVL_PRES_FORMAT:
            {
                prf_unpack_char_pres_fmt(&param->value[0], &rsp->data.char_pres_format);
            }break;

            default:
            {
                ASSERT_ERR(0);
            }break;
        }

        // send response
        ke_msg_send(rsp);

        // operation is over - go back to idle state
        ke_free(basc_env->env[conidx]->operation);
        basc_env->env[conidx]->operation = NULL;
        ke_state_set(dest_id, BASC_IDLE);
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

    if(state != BASC_FREE)
    {

        // BAS Instance
        uint8_t bas_nb;
        uint8_t conidx = KE_IDX_GET(src_id);
        // Get the address of the environment
        struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);

        //Battery Level - BAS instance is unknown.
        for (bas_nb = 0; (bas_nb < basc_env->env[conidx]->bas_nb); bas_nb++)
        {
            if (param->handle == basc_env->env[conidx]->bas[bas_nb].chars[BAS_CHAR_BATT_LEVEL].val_hdl)
            {
                struct basc_batt_level_ind * ind = KE_MSG_ALLOC(BASC_BATT_LEVEL_IND,
                        prf_dst_task_get(&(basc_env->prf_env), conidx), dest_id,
                        basc_batt_level_ind);

                ind->batt_level = param->value[0];
                ind->bas_nb     = bas_nb;

                //Send Battery Level value to APP
                ke_msg_send(ind);
                break;
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/// Default State handlers definition
KE_MSG_HANDLER_TAB(basc)
{
    {BASC_ENABLE_REQ,               (ke_msg_func_t)basc_enable_req_handler},
    {BASC_READ_INFO_REQ,            (ke_msg_func_t)basc_read_info_req_handler},
    {BASC_BATT_LEVEL_NTF_CFG_REQ,   (ke_msg_func_t)basc_batt_level_ntf_cfg_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_SDP_SVC_IND,             (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

void basc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct basc_env_tag *basc_env = PRF_ENV_GET(BASC, basc);

    task_desc->msg_handler_tab = basc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(basc_msg_handler_tab);
    task_desc->state           = basc_env->state;
    task_desc->idx_max         = BASC_IDX_MAX;
}



#endif /* (BLE_BATT_CLIENT) */

/// @} BASCTASK
