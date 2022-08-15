/**
 ****************************************************************************************
 * @addtogroup TIPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_TIP_CLIENT)
#include "co_utils.h"
#include "tipc_task.h"
#include "tipc.h"
#include "gap.h"
#include "attm.h"
#include "gattc_task.h"

#include "ke_mem.h"
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */



/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve Current Time service characteristics information
const struct prf_char_def tipc_cts_char[TIPC_CHAR_CTS_MAX] =
{
    /// Current Time
    [TIPC_CHAR_CTS_CURR_TIME]        = {ATT_CHAR_CT_TIME,
                                        ATT_MANDATORY,
                                        ATT_CHAR_PROP_RD|ATT_CHAR_PROP_NTF},
    /// Local Time Info
    [TIPC_CHAR_CTS_LOCAL_TIME_INFO]  = {ATT_CHAR_LOCAL_TIME_INFO,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
    /// Reference Time Info
    [TIPC_CHAR_CTS_REF_TIME_INFO]    = {ATT_CHAR_REFERENCE_TIME_INFO,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve Current Time service characteristic description information
const struct prf_char_desc_def tipc_cts_char_desc[TIPC_DESC_CTS_MAX] =
{
    /// Current Time client config
    [TIPC_DESC_CTS_CURR_TIME_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                         ATT_MANDATORY,
                                         TIPC_CHAR_CTS_CURR_TIME},
};

/// State machine used to retrieve Next DST Change service characteristics information
const struct prf_char_def tipc_ndcs_char[TIPC_CHAR_NDCS_MAX] =
{
    /// Current Time
    [TIPC_CHAR_NDCS_TIME_WITH_DST]    = {ATT_CHAR_TIME_WITH_DST,
                                         ATT_MANDATORY,
                                         ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve Reference Time Update service characteristics information
const struct prf_char_def tipc_rtus_char[TIPC_CHAR_RTUS_MAX] =
{
    /// Time Update Control Point
    [TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT]    = {ATT_CHAR_TIME_UPDATE_CNTL_POINT,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_WR_NO_RESP},
    /// Time Update State
    [TIPC_CHAR_RTUS_TIME_UPD_STATE]      = {ATT_CHAR_TIME_UPDATE_STATE,
                                            ATT_MANDATORY,
                                            ATT_CHAR_PROP_RD},
};


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

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

    if(state == TIPC_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);

        ASSERT_INFO(tipc_env != NULL, dest_id, src_id);
        ASSERT_INFO(tipc_env->env[conidx] != NULL, dest_id, src_id);

        if(attm_uuid16_comp((unsigned char *)ind->uuid, ind->uuid_len, ATT_SVC_CURRENT_TIME))
        {
            // Retrieve TIS characteristics and descriptors
            prf_extract_svc_info(ind, TIPC_CHAR_CTS_MAX, &tipc_cts_char[0],  &tipc_env->env[conidx]->cts.chars[0],
                    TIPC_DESC_CTS_MAX, &tipc_cts_char_desc[0], &tipc_env->env[conidx]->cts.descs[0]);

            //Even if we get multiple responses we only store 1 range
            tipc_env->env[conidx]->cts.svc.shdl = ind->start_hdl;
            tipc_env->env[conidx]->cts.svc.ehdl = ind->end_hdl;
        }

        if(attm_uuid16_comp((unsigned char *)ind->uuid, ind->uuid_len, ATT_SVC_NEXT_DST_CHANGE))
        {
            // Retrieve NDCS characteristics and descriptors
            prf_extract_svc_info(ind, TIPC_CHAR_NDCS_MAX, &tipc_ndcs_char[0],  &tipc_env->env[conidx]->ndcs.chars[0],
                    0, NULL, NULL);

            //Even if we get multiple responses we only store 1 range
            tipc_env->env[conidx]->ndcs.svc.shdl = ind->start_hdl;
            tipc_env->env[conidx]->ndcs.svc.ehdl = ind->end_hdl;
        }

        if(attm_uuid16_comp((unsigned char *)ind->uuid, ind->uuid_len, ATT_SVC_REF_TIME_UPDATE))
        {
            // Retrieve RTUS characteristics and descriptors
            prf_extract_svc_info(ind, TIPC_CHAR_RTUS_MAX, &tipc_rtus_char[0],  &tipc_env->env[conidx]->rtus.chars[0],
                    0, NULL, NULL);

            //Even if we get multiple responses we only store 1 range
            tipc_env->env[conidx]->rtus.svc.shdl = ind->start_hdl;
            tipc_env->env[conidx]->rtus.svc.ehdl = ind->end_hdl;
        }

        // Increment number of services
        tipc_env->env[conidx]->nb_svc++;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_ENABLE_REQ message.
 * The handler enables the Time Profile Client Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int tipc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct tipc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = GAP_ERR_NO_ERROR;
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Get state
    uint8_t state = ke_state_get(dest_id);

    // Time Service Client Role Task Environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);

    ASSERT_INFO(tipc_env != NULL, dest_id, src_id);
    if ((state == TIPC_IDLE) && (tipc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        tipc_env->env[conidx] = (struct tipc_cnx_env*) ke_malloc(sizeof(struct tipc_cnx_env),KE_MEM_ATT_DB);
        memset(tipc_env->env[conidx], 0, sizeof(struct tipc_cnx_env));

        //config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering CTS on peer
            prf_disc_svc_send(&(tipc_env->prf_env), conidx, ATT_SVC_CURRENT_TIME);

            tipc_env->env[conidx]->last_uuid_req = ATT_SVC_CURRENT_TIME;
            tipc_env->env[conidx]->last_svc_req  = ATT_SVC_CURRENT_TIME;

            // Go to DISCOVERING state
            ke_state_set(dest_id, TIPC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            tipc_env->env[conidx]->cts = param->cts;
            tipc_env->env[conidx]->ndcs = param->ndcs;
            tipc_env->env[conidx]->rtus = param->rtus;

            //send APP confirmation that can start normal connection
            tipc_enable_rsp_send(tipc_env, conidx, GAP_ERR_NO_ERROR);
        }
    }

    else if(state != TIPC_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        tipc_enable_rsp_send(tipc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_RD_CHAR_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int tipc_rd_char_req_handler(ke_msg_id_t const msgid,
                                        struct tipc_rd_char_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{

    uint8_t state = ke_state_get(dest_id);
    uint8_t status = GAP_ERR_NO_ERROR;

    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if((state == TIPC_IDLE) && (tipc_env->env[conidx] != NULL))
    {
        // Service
        struct prf_svc *svc = NULL;
        // Attribute Handle
        uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;

        //Next DST Change Service Characteristic
        if (((param->char_code & TIPC_CHAR_NDCS_MASK) == TIPC_CHAR_NDCS_MASK) &&
            ((param->char_code & ~TIPC_CHAR_NDCS_MASK) < TIPC_CHAR_NDCS_MAX))
        {
            svc = &tipc_env->env[conidx]->ndcs.svc;
            search_hdl = tipc_env->env[conidx]->ndcs.
                    chars[param->char_code & ~TIPC_CHAR_NDCS_MASK].val_hdl;
        }
        //Reference Time Update Service Characteristic
        else if (((param->char_code & TIPC_CHAR_RTUS_MASK) == TIPC_CHAR_RTUS_MASK) &&
                 ((param->char_code & ~TIPC_CHAR_RTUS_MASK) < TIPC_CHAR_RTUS_MAX))
        {
            svc = &tipc_env->env[conidx]->rtus.svc;
            search_hdl = tipc_env->env[conidx]->rtus.
                    chars[param->char_code & ~TIPC_CHAR_RTUS_MASK].val_hdl;
        }
        else
        {
            svc = &tipc_env->env[conidx]->cts.svc;

            //Current Time Characteristic Descriptor
            if (((param->char_code & TIPC_DESC_CTS_MASK) == TIPC_DESC_CTS_MASK) &&
                ((param->char_code & ~TIPC_DESC_CTS_MASK) < TIPC_DESC_CTS_MAX))
            {
                search_hdl = tipc_env->env[conidx]->cts.
                        descs[param->char_code & ~TIPC_DESC_CTS_MASK].desc_hdl;
            }
            //Current Time Service Characteristic
            else if (param->char_code < TIPC_CHAR_CTS_MAX)
            {
                search_hdl = tipc_env->env[conidx]->cts.
                        chars[param->char_code].val_hdl;
            }
        }

        // Check if handle is viable
        if ((search_hdl != ATT_INVALID_SEARCH_HANDLE) && (svc != NULL))
        {
            // Save char code
            tipc_env->env[conidx]->last_char_code = param->char_code;
            // Send read request
            prf_read_char_send(&(tipc_env->prf_env), conidx, svc->shdl, svc->ehdl, search_hdl);
        }
        else
        {
            status =  PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        //send app error indication
        struct tipc_rd_char_rsp *ind = KE_MSG_ALLOC(
                TIPC_RD_CHAR_RSP,
                prf_dst_task_get(&(tipc_env->prf_env), conidx),
                prf_src_task_get(&(tipc_env->prf_env), conidx),
                tipc_rd_char_rsp);

        // It will be an TIPC status code
        ind->status    = status;
        ind->op_code = param->char_code;

        // send the message
        ke_msg_send(ind);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_CT_NTF_CFG_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int tipc_ct_ntf_cfg_req_handler(ke_msg_id_t const msgid,
                                       struct tipc_ct_ntf_cfg_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    uint16_t cfg_hdl = ATT_INVALID_SEARCH_HANDLE;
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if((tipc_env->env[conidx] != NULL) && (ke_state_get(dest_id) == TIPC_IDLE))
    {
        //Only NTF
        if((param->cfg_val == PRF_CLI_STOP_NTFIND)||(param->cfg_val == PRF_CLI_START_NTF))
        {
            cfg_hdl = tipc_env->env[conidx]->cts.descs[TIPC_DESC_CTS_CURR_TIME_CLI_CFG].desc_hdl;

            //check if the handle value exists
            if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&tipc_env->prf_env, conidx, cfg_hdl, param->cfg_val);
                status = GAP_ERR_NO_ERROR;
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        struct tipc_ct_ntf_cfg_rsp *ind = KE_MSG_ALLOC(
                TIPC_CT_NTF_CFG_RSP,
                prf_dst_task_get(&(tipc_env->prf_env), conidx),
                prf_src_task_get(&(tipc_env->prf_env), conidx),
                tipc_ct_ntf_cfg_rsp);

        // It will be an TIPC status code
        ind->status    = status;
        // send the message
        ke_msg_send(ind);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPC_WR_TIME_UPD_CTNL_PT_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int tipc_wr_time_upd_ctnl_pt_req_handler(ke_msg_id_t const msgid,
                                                struct tipc_wr_time_udp_ctnl_pt_req const *param,
                                                ke_task_id_t const dest_id,
                                                ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if((ke_state_get(dest_id) == TIPC_IDLE) && (tipc_env->env[conidx] != NULL))
    {
        // Check provided parameters
        if ((param->value == TIPS_TIME_UPD_CTNL_PT_GET) ||
                (param->value == TIPS_TIME_UPD_CTNL_PT_CANCEL))
        {
            if (tipc_env->env[conidx]->cts.chars[TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT].char_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Send GATT Write Request
                prf_gatt_write(&tipc_env->prf_env, conidx,
                        tipc_env->env[conidx]->rtus.chars[TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT].val_hdl,
                        (uint8_t *)&param->value, sizeof(uint8_t), GATTC_WRITE_NO_RESPONSE);

                status = GAP_ERR_NO_ERROR;
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        //send app error indication
        struct tipc_wr_time_upd_ctnl_pt_rsp *ind = KE_MSG_ALLOC(
                TIPC_WR_TIME_UPD_CTNL_PT_RSP,
                prf_dst_task_get(&(tipc_env->prf_env), conidx),
                prf_src_task_get(&(tipc_env->prf_env), conidx),
                tipc_wr_time_upd_ctnl_pt_rsp);

        // It will be an TIPC status code
        ind->status    = status;

        // send the message
        ke_msg_send(ind);
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
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;
    bool finished = false;

    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);

    if((state == TIPC_DISCOVERING) && (tipc_env->env[conidx] != NULL))
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (param->status == ATT_ERR_NO_ERROR))
        {
            // check characteristic validity
            if(tipc_env->env[conidx]->nb_svc <= 1)
            {
                switch(tipc_env->env[conidx]->last_svc_req)
                {
                    case ATT_SVC_CURRENT_TIME:
                    {
                        // Check service (mandatory)
                        status = prf_check_svc_char_validity(TIPC_CHAR_CTS_MAX,
                                tipc_env->env[conidx]->cts.chars,
                                tipc_cts_char);

                        // Check Descriptors (mandatory)
                        if(status == GAP_ERR_NO_ERROR)
                        {
                            status = prf_check_svc_char_desc_validity(TIPC_DESC_CTS_MAX,
                                    tipc_env->env[conidx]->cts.descs,
                                    tipc_cts_char_desc,
                                    tipc_env->env[conidx]->cts.chars);
                            // Prepare to discovery next service
                            tipc_env->env[conidx]->last_svc_req = ATT_SVC_NEXT_DST_CHANGE;
                        }
                    }break;

                    case ATT_SVC_NEXT_DST_CHANGE:
                    {
                        // Check service (if found)
                        if(tipc_env->env[conidx]->nb_svc)
                        {
                            status = prf_check_svc_char_validity(TIPC_CHAR_NDCS_MAX,
                                    tipc_env->env[conidx]->ndcs.chars,
                                    tipc_ndcs_char);
                        }
                        else
                        {
                            status = GAP_ERR_NO_ERROR;
                        }
                        // Prepare to discovery next service
                        tipc_env->env[conidx]->last_svc_req = ATT_SVC_REF_TIME_UPDATE;
                    }break;

                    case ATT_SVC_REF_TIME_UPDATE:
                    {
                        // Check service (if found)
                        if(tipc_env->env[conidx]->nb_svc)
                        {
                            status = prf_check_svc_char_validity(TIPC_CHAR_RTUS_MAX,
                                    tipc_env->env[conidx]->rtus.chars,
                                    tipc_rtus_char);
                        }
                        else
                        {
                            status = GAP_ERR_NO_ERROR;
                        }

                        if(status == GAP_ERR_NO_ERROR)
                        {
                            // send app the details about the discovered TIPS DB to save
                            tipc_enable_rsp_send(tipc_env, conidx, GAP_ERR_NO_ERROR);
                            // Discovery is finished
                            finished = true;
                        }
                    }break;

                    default:
                        break;
                }
            }
            // too many services
            else
            {
                status = PRF_ERR_MULTIPLE_SVC;
            }

            if(status == GAP_ERR_NO_ERROR)
            {
                // reset number of services
                tipc_env->env[conidx]->nb_svc = 0;

                if (!finished)
                {
                    //start discovering following service on peer
                    prf_disc_svc_send(&(tipc_env->prf_env), conidx, tipc_env->env[conidx]->last_svc_req);
                }
            }
            else
            {
                // stop discovery procedure
                tipc_enable_rsp_send(tipc_env, conidx, status);
            }
        }
    }
    else if(state == TIPC_IDLE)
    {
        switch(param->operation)
        {
            case GATTC_WRITE:
            {
                struct tipc_ct_ntf_cfg_rsp *wr_cfm = KE_MSG_ALLOC(
                        TIPC_CT_NTF_CFG_RSP,
                        prf_dst_task_get(&(tipc_env->prf_env), conidx),
                        prf_src_task_get(&(tipc_env->prf_env), conidx),
                        tipc_ct_ntf_cfg_rsp);

                //it will be a GATT status code
                wr_cfm->status    = param->status;
                // send the message
                ke_msg_send(wr_cfm);
            } break;

            case GATTC_WRITE_NO_RESPONSE:
            {
                struct tipc_wr_time_upd_ctnl_pt_rsp *wr_cfm = KE_MSG_ALLOC(
                        TIPC_WR_TIME_UPD_CTNL_PT_RSP,
                        prf_dst_task_get(&(tipc_env->prf_env), conidx),
                        prf_src_task_get(&(tipc_env->prf_env), conidx),
                        tipc_wr_time_upd_ctnl_pt_rsp);

                //it will be a GATT status code
                wr_cfm->status    = param->status;
                // send the message
                ke_msg_send(wr_cfm);
            }
            break;

            case GATTC_READ:
            {
                // an error occurs while reading peer attribute, inform app
                if(param->status != GAP_ERR_NO_ERROR)
                {
                    struct tipc_rd_char_rsp *rd_cfm = KE_MSG_ALLOC(
                            TIPC_RD_CHAR_RSP,
                            prf_dst_task_get(&(tipc_env->prf_env), conidx),
                            prf_src_task_get(&(tipc_env->prf_env), conidx),
                            tipc_rd_char_rsp);

                    //it will be a GATT status code
                    rd_cfm->status    = param->status;
                    rd_cfm->op_code   = tipc_env->env[conidx]->last_char_code;
                    // send the message
                    ke_msg_send(rd_cfm);
                }
            }
            break;

            default:
                break;
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
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);
    uint8_t conidx = KE_IDX_GET(dest_id);

    //Build the message
    struct tipc_rd_char_rsp * ind = KE_MSG_ALLOC(
            TIPC_RD_CHAR_RSP,
            prf_dst_task_get(&(tipc_env->prf_env), conidx),
            dest_id,
            tipc_rd_char_rsp);

    //Current Time Characteristic
    if (tipc_env->env[conidx]->last_char_code == TIPC_RD_CTS_CURR_TIME)
    {
        ind->op_code = TIPC_RD_CTS_CURR_TIME;
        // Unpack Current Time Value.
        tipc_unpack_curr_time_value(&(ind->value.curr_time), (uint8_t *) param->value);
    }
    //Local Time Information Characteristic
    else if (tipc_env->env[conidx]->last_char_code == TIPC_RD_CTS_LOCAL_TIME_INFO)
    {
        ind->op_code = TIPC_RD_CTS_LOCAL_TIME_INFO;
        // Local Time Information Value
        ind->value.loc_time_info.time_zone         = param->value[0];
        ind->value.loc_time_info.dst_offset        = param->value[1];
    }
    //Reference Time Information Characteristic
    else if (tipc_env->env[conidx]->last_char_code == TIPC_RD_CTS_REF_TIME_INFO)
    {
        ind->op_code = TIPC_RD_CTS_REF_TIME_INFO;
        // Reference Time Information Value
        ind->value.ref_time_info.time_source      = param->value[0];
        ind->value.ref_time_info.time_accuracy    = param->value[1];
        ind->value.ref_time_info.days_update      = param->value[2];
        ind->value.ref_time_info.hours_update     = param->value[3];
    }
    //Time with DST Characteristic
    else if (tipc_env->env[conidx]->last_char_code == TIPC_RD_NDCS_TIME_WITH_DST)
    {
        ind->op_code = TIPC_RD_NDCS_TIME_WITH_DST;
        // Time with DST Value
        tipc_unpack_time_dst_value(&ind->value.time_with_dst, (uint8_t *) param->value);
    }
    //Time Update State Characteristic
    else if (tipc_env->env[conidx]->last_char_code == TIPC_RD_RTUS_TIME_UPD_STATE)
    {
        ind->op_code = TIPC_RD_RTUS_TIME_UPD_STATE;
        // Reference Time Information Value
        ind->value.time_upd_state.current_state     = param->value[0];
        ind->value.time_upd_state.result            = param->value[1];
    }
    //Current Time Characteristic - Client Characteristic Configuration Descriptor
    else if (tipc_env->env[conidx]->last_char_code == TIPC_RD_CTS_CURR_TIME_CLI_CFG)
    {
        ind->op_code = TIPC_RD_CTS_CURR_TIME_CLI_CFG;
        // Notification Configuration
        memcpy(&ind->value.ntf_cfg, &param->value[0], sizeof(uint16_t));
    }

    ke_msg_send(ind);

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
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);
    uint8_t conidx = KE_IDX_GET(dest_id);

    if(tipc_env->env[conidx] != NULL)
    {
        if(param->handle == tipc_env->env[conidx]->cts.chars[TIPC_CHAR_CTS_CURR_TIME].val_hdl)
        {
            //Build a TIPC_CT_IND message
            struct tipc_ct_ind * ind = KE_MSG_ALLOC(
                    TIPC_CT_IND,
                    prf_dst_task_get(&(tipc_env->prf_env), conidx),
                    prf_src_task_get(&(tipc_env->prf_env), conidx),
                    tipc_ct_ind);

            // Unpack Current Time Value.
            tipc_unpack_curr_time_value(&(ind->ct_val), (uint8_t*) param->value);

            ke_msg_send(ind);
        }
    }
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
KE_MSG_HANDLER_TAB(tipc)
{
    {TIPC_ENABLE_REQ,               (ke_msg_func_t)tipc_enable_req_handler},
    {GATTC_SDP_SVC_IND,             (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
    {TIPC_CT_NTF_CFG_REQ,           (ke_msg_func_t)tipc_ct_ntf_cfg_req_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {TIPC_RD_CHAR_REQ,              (ke_msg_func_t)tipc_rd_char_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {TIPC_WR_TIME_UPD_CTNL_PT_REQ,  (ke_msg_func_t)tipc_wr_time_upd_ctnl_pt_req_handler},
};

void tipc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct tipc_env_tag *tipc_env = PRF_ENV_GET(TIPC, tipc);

    task_desc->msg_handler_tab = tipc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(tipc_msg_handler_tab);
    task_desc->state           = tipc_env->state;
    task_desc->idx_max         = TIPC_IDX_MAX;
}

#endif /* (BLE_TIP_CLIENT) */
/// @} TIPCTASK
