/**
 ****************************************************************************************
 * @addtogroup GLPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_GL_SENSOR)

#include "gap.h"
#include "gattc_task.h"
#include "glps.h"
#include "glps_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 *  GLUCOSE PROFILE ATTRIBUTES
 ****************************************************************************************
 */


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPS_ENABLE_REQ message.
 * The handler enables the Glucose Sensor Profile and initialize readable values.
 * @param[in] msgid Id of the message received (probably unused).off
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int glps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct glps_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    uint8_t state  = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    struct glps_enable_rsp * cmp_evt;

    if(state == GLPS_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);
        if(!GLPS_IS(conidx, BOND_DATA_PRESENT))
        {
            GLPS_SET(conidx, BOND_DATA_PRESENT);
            status = GAP_ERR_NO_ERROR;
            glps_env->env[conidx]->evt_cfg = param->evt_cfg;
        }
    }

    // send completed information to APP task that contains error status
    cmp_evt = KE_MSG_ALLOC(GLPS_ENABLE_RSP, src_id, dest_id, glps_enable_rsp);
    cmp_evt->status     = status;
    ke_msg_send(cmp_evt);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPS_MEAS_SEND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int glps_meas_send_req_handler(ke_msg_id_t const msgid,
                                      struct glps_send_meas_with_ctx_cmd const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t state  = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == GLPS_IDLE)
    {
        // Get the address of the environment
        struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);

        //Cannot send another measurement in parallel
        if(!GLPS_IS(conidx, SENDING_MEAS))
        {
            // inform that device is sending a measurement
            GLPS_SET(conidx, SENDING_MEAS);

            // check if context is supported
            if((msgid == GLPS_SEND_MEAS_WITH_CTX_CMD) && !(glps_env->meas_ctx_supported))
            {
                // Context not supported
                status = (PRF_ERR_FEATURE_NOT_SUPPORTED);
            }
            // check if notifications enabled
            else if(((glps_env->env[conidx]->evt_cfg & GLPS_MEAS_NTF_CFG) == 0)
                    || (((glps_env->env[conidx]->evt_cfg & GLPS_MEAS_CTX_NTF_CFG) == 0)
                            && (msgid == GLPS_SEND_MEAS_WITH_CTX_CMD)))
            {
                // Not allowed to send measurement if Notifications not enabled.
                status = (PRF_ERR_NTF_DISABLED);
            }
            else
            {
                // Allocate the GATT notification message
                struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                        KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                        gattc_send_evt_cmd, GLP_MEAS_MAX_LEN);

                // Fill in the parameter structure
                meas_val->operation = GATTC_NOTIFY;
                meas_val->handle = GLPS_HANDLE(GLS_IDX_MEAS_VAL);
                // pack measured value in database
                meas_val->length = glps_pack_meas_value(meas_val->value, &(param->meas), param->seq_num);

                // Measurement value notification not yet sent
                GLPS_CLEAR(conidx, MEAS_SENT);

                // Send the event
                ke_msg_send(meas_val);

                if(msgid == GLPS_SEND_MEAS_WITH_CTX_CMD)
                {
                    // Allocate the GATT notification message
                    struct gattc_send_evt_cmd *meas_ctx_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                            KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                            gattc_send_evt_cmd, GLP_MEAS_CTX_MAX_LEN);

                    // Fill in the parameter structure
                    meas_ctx_val->operation = GATTC_NOTIFY;
                    meas_ctx_val->handle = GLPS_HANDLE(GLS_IDX_MEAS_CTX_VAL);
                    // pack measured value in database
                    meas_ctx_val->length = glps_pack_meas_ctx_value(meas_ctx_val->value, &(param->ctx), param->seq_num);

                    // 2 notification complete messages expected
                    GLPS_SET(conidx, MEAS_CTX_SENT);

                    // Send the event
                    ke_msg_send(meas_ctx_val);
                }
                else
                {
                    // 1 notification complete messages expected
                    GLPS_CLEAR(conidx, MEAS_CTX_SENT);
                }

                status = (GAP_ERR_NO_ERROR);
            }
        }
    }

    // send command complete if an error occurs
    if(status != GAP_ERR_NO_ERROR)
    {
        // send completed information to APP task that contains error status
        struct glps_cmp_evt * cmp_evt = KE_MSG_ALLOC(GLPS_CMP_EVT, src_id,
                dest_id, glps_cmp_evt);

        cmp_evt->request    = GLPS_SEND_MEAS_REQ_NTF_CMP;
        cmp_evt->status     = status;

        ke_msg_send(cmp_evt);
    }


    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPS_SEND_RACP_RSP_CMD message.
 * Send when a RACP requests is finished
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int glps_send_racp_rsp_cmd_handler(ke_msg_id_t const msgid,
                                     struct glps_send_racp_rsp_cmd const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t state  = ke_state_get(dest_id);

    if(state == GLPS_IDLE)
    {
        // Get the address of the environment
        struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);

        // check if op code valid
        if((param->op_code < GLP_REQ_REP_STRD_RECS)
                || (param->op_code > GLP_REQ_REP_NUM_OF_STRD_RECS))
        {
            //Wrong op code
            status = PRF_ERR_INVALID_PARAM;
        }
        // check if RACP on going
        else if((param->op_code != GLP_REQ_ABORT_OP) && !(GLPS_IS(conidx, RACP_ON_GOING)))
        {
            //Cannot send response since no RACP on going
            status = PRF_ERR_REQ_DISALLOWED;
        }
        // Indication not enabled on peer device
        else if((glps_env->env[conidx]->evt_cfg & GLPS_RACP_IND_CFG) == 0)
        {
            status = PRF_ERR_IND_DISABLED;
            // There is no more RACP on going
            GLPS_CLEAR(conidx, RACP_ON_GOING);
        }
        else
        {
            struct glp_racp_rsp racp_rsp;
            // Allocate the GATT indication message
            struct gattc_send_evt_cmd *racp_rsp_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                    KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                    gattc_send_evt_cmd, GLP_REC_ACCESS_CTRL_MAX_LEN);

            // Fill in the parameter structure
            racp_rsp_val->operation = GATTC_INDICATE;
            racp_rsp_val->handle = GLPS_HANDLE(GLS_IDX_REC_ACCESS_CTRL_VAL);

            // Number of stored record calculation succeed.
            if((param->op_code == GLP_REQ_REP_NUM_OF_STRD_RECS)
                    && ( param->status == GLP_RSP_SUCCESS))
            {
                racp_rsp.op_code = GLP_REQ_NUM_OF_STRD_RECS_RSP;
                racp_rsp.operand.num_of_record = param->num_of_record;
            }
            // Send back status information
            else
            {
                racp_rsp.op_code = GLP_REQ_RSP_CODE;
                racp_rsp.operand.rsp.op_code_req = param->op_code;
                racp_rsp.operand.rsp.status = param->status;
            }

            // pack measured value in database
            racp_rsp_val->length = glps_pack_racp_rsp(racp_rsp_val->value, &racp_rsp);

            // There is no more RACP on going
            GLPS_CLEAR(conidx, RACP_ON_GOING);
            status = GAP_ERR_NO_ERROR;

            // Store that response sent by application
            GLPS_SET(conidx, RACP_RSP_SENT_BY_APP);

            // send RACP Response indication
            ke_msg_send(racp_rsp_val);
        }
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        // send completed information about request failed
        struct glps_cmp_evt * cmp_evt = KE_MSG_ALLOC(GLPS_CMP_EVT, src_id, dest_id, glps_cmp_evt);
        cmp_evt->request    = GLPS_SEND_RACP_RSP_IND_CMP;
        cmp_evt->status     = status;
        ke_msg_send(cmp_evt);
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the attribute info request message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
        uint8_t state  = ke_state_get(dest_id);

        if(state == GLPS_IDLE)
        {
            struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);
            uint8_t att_idx = GLPS_IDX(param->handle);
            struct gattc_att_info_cfm * cfm;

            //Send write response
            cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
            cfm->handle = param->handle;

            // check if it's a client configuration char
            if((att_idx == GLS_IDX_MEAS_NTF_CFG)
                    || (att_idx == GLS_IDX_MEAS_CTX_NTF_CFG)
                    || (att_idx == GLS_IDX_REC_ACCESS_CTRL_IND_CFG))
            {
                // CCC attribute length = 2
                cfm->length = 2;
                cfm->status = GAP_ERR_NO_ERROR;
            }
            else if (att_idx == GLS_IDX_REC_ACCESS_CTRL_VAL)
            {
                // force length to zero to reject any write starting from something != 0
                cfm->length = 0;
                cfm->status = GAP_ERR_NO_ERROR;
            }
            // not expected request
            else
            {
                cfm->length = 0;
                cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
            }
            ke_msg_send(cfm);
        }


        return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_req_ind *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t state  = ke_state_get(dest_id);
    struct glp_racp_rsp racp_rsp;
    racp_rsp.op_code = 0;

    if(state == GLPS_IDLE)
    {
        uint8_t status = GAP_ERR_NO_ERROR;
        struct gattc_write_cfm * cfm;
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);
        uint8_t att_idx = GLPS_IDX(param->handle);

        if(param->offset != 0)
        {
            status = ATT_ERR_UNLIKELY_ERR;
        }
        // check if it's a client configuration char
        else if((att_idx == GLS_IDX_MEAS_NTF_CFG)
                || (att_idx == GLS_IDX_MEAS_CTX_NTF_CFG)
                || (att_idx == GLS_IDX_REC_ACCESS_CTRL_IND_CFG))
        {
            uint16_t cli_cfg;
            uint8_t evt_mask = GLPS_IND_NTF_EVT(att_idx);

            // get client configuration
            cli_cfg = co_read16(&(param->value));

            // stop indication/notification
            if(cli_cfg == PRF_CLI_STOP_NTFIND)
            {
                glps_env->env[conidx]->evt_cfg &= ~evt_mask;
            }
            // start indication/notification (check that char value accept it)
            else if(((att_idx == GLS_IDX_REC_ACCESS_CTRL_IND_CFG) && (cli_cfg == PRF_CLI_START_IND))
               ||((att_idx != GLS_IDX_REC_ACCESS_CTRL_IND_CFG) && (cli_cfg == PRF_CLI_START_NTF)))
            {
                glps_env->env[conidx]->evt_cfg |= evt_mask;
            }
            // improper value
            else
            {
                status = GLP_ERR_IMPROPER_CLI_CHAR_CFG;
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of configuration change
                struct glps_cfg_indntf_ind * ind = KE_MSG_ALLOC(GLPS_CFG_INDNTF_IND,
                        prf_dst_task_get(&(glps_env->prf_env), conidx), dest_id, glps_cfg_indntf_ind);
                ind->evt_cfg = glps_env->env[conidx]->evt_cfg;
                ke_msg_send(ind);
            }
        }

        else if (att_idx == GLS_IDX_REC_ACCESS_CTRL_VAL)
        {
            if((glps_env->env[conidx]->evt_cfg & GLPS_RACP_IND_CFG) == 0)
            {
                // do nothing since indication not enabled for this characteristic
                status = GLP_ERR_IMPROPER_CLI_CHAR_CFG;
            }
            // If a request is on going
            else if(GLPS_IS(conidx, RACP_ON_GOING))
            {
                // if it's an abort command, execute it.
                if((param->offset == 0) && (param->value[0] == GLP_REQ_ABORT_OP))
                {
                    //forward abort operation to application
                    struct glps_racp_req_rcv_ind * req = KE_MSG_ALLOC(GLPS_RACP_REQ_RCV_IND,
                            prf_dst_task_get(&(glps_env->prf_env), conidx), dest_id, glps_racp_req_rcv_ind);
                    req->racp_req.op_code = GLP_REQ_ABORT_OP;
                    req->racp_req.filter.operator = 0;
                    ke_msg_send(req);
                }
                else
                {
                    // do nothing since a procedure already in progress
                    status = GLP_ERR_PROC_ALREADY_IN_PROGRESS;
                }
            }
            else
            {
                struct glp_racp_req racp_req;
                // unpack record access control point value
                uint8_t reqstatus = glps_unpack_racp_req(param->value, param->length, &racp_req);

                // check unpacked status
                switch(reqstatus)
                {
                    case PRF_APP_ERROR:
                    {
                        /* Request failed */
                        status = ATT_ERR_UNLIKELY_ERR;
                    }
                    break;
                    case GAP_ERR_NO_ERROR:
                    {
                        // check wich request shall be send to api task
                        switch(racp_req.op_code)
                        {
                            case GLP_REQ_REP_STRD_RECS:
                            case GLP_REQ_DEL_STRD_RECS:
                            case GLP_REQ_REP_NUM_OF_STRD_RECS:
                            {
                                //forward request operation to application
                                struct glps_racp_req_rcv_ind * req = KE_MSG_ALLOC(GLPS_RACP_REQ_RCV_IND,
                                        prf_dst_task_get(&(glps_env->prf_env), conidx), dest_id,
                                        glps_racp_req_rcv_ind);
                                req->racp_req = racp_req;
                                // RACP on going.
                                GLPS_SET(conidx, RACP_ON_GOING);
                                ke_msg_send(req);
                            }
                            break;
                            case GLP_REQ_ABORT_OP:
                            {
                                // nothing to abort, send an error message.
                                racp_rsp.op_code = GLP_REQ_RSP_CODE;
                                racp_rsp.operand.rsp.op_code_req =
                                        racp_req.op_code;
                                racp_rsp.operand.rsp.status = GLP_RSP_ABORT_UNSUCCESSFUL;
                            }
                            break;
                            default:
                            {
                                // nothing to do since it's handled during unpack
                            }
                            break;
                        }
                    }
                    break;
                    default:
                    {
                        /* There is an error in record access control request, inform peer
                         * device that request is incorrect. */
                        racp_rsp.op_code = GLP_REQ_RSP_CODE;
                        racp_rsp.operand.rsp.op_code_req = racp_req.op_code;
                        racp_rsp.operand.rsp.status = reqstatus;
                    }
                    break;
                }
            }
        }
        // not expected request
        else
        {
            status = ATT_ERR_WRITE_NOT_PERMITTED;
        }

        //Send write response
        cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
        cfm->handle = param->handle;
        cfm->status = status;
        ke_msg_send(cfm);

        /* The racp response has to be sent - after write confirm */
        if(racp_rsp.op_code != 0)
        {
            // Allocate the GATT indication message
            struct gattc_send_evt_cmd *racp_rsp_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                    KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                    gattc_send_evt_cmd, GLP_REC_ACCESS_CTRL_MAX_LEN);

            racp_rsp_val->operation = GATTC_INDICATE;
            racp_rsp_val->handle = param->handle;
            // pack measured value in database
            racp_rsp_val->length = glps_pack_racp_rsp(racp_rsp_val->value, &racp_rsp);

            // Store that response internally sent by profile
            GLPS_CLEAR(conidx, RACP_RSP_SENT_BY_APP);

            // send RACP Response indication
            ke_msg_send(racp_rsp_val);
        }
    }


    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the read request from peer device
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    ke_state_t state = ke_state_get(dest_id);

    if(state == GLPS_IDLE)
    {
        // Send value to peer device.
        struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(uint16_t));
        cfm->handle = param->handle;
        cfm->status = ATT_ERR_NO_ERROR;
        cfm->length = sizeof(uint16_t);


        // Get the address of the environment
        struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);
        uint8_t att_idx = GLPS_IDX(param->handle);
        uint8_t conidx = KE_IDX_GET(dest_id);

        switch(att_idx)
        {
            case GLS_IDX_MEAS_NTF_CFG:
            {
                co_write16p(cfm->value, (glps_env->env[conidx]->evt_cfg & GLPS_MEAS_NTF_CFG)
                        ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
            }
            break;
            case GLS_IDX_MEAS_CTX_NTF_CFG:
            {
                co_write16p(cfm->value, (glps_env->env[conidx]->evt_cfg & GLPS_MEAS_CTX_NTF_CFG)
                        ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
            }
            break;
            case GLS_IDX_REC_ACCESS_CTRL_IND_CFG:
            {
                co_write16p(cfm->value, (glps_env->env[conidx]->evt_cfg & GLPS_RACP_IND_CFG)
                        ? PRF_CLI_START_IND : PRF_CLI_STOP_NTFIND);
            }
            break;
            case GLS_IDX_FEATURE_VAL:
            {
                co_write16p(cfm->value, glps_env->features);
            }
            break;
            default:
            {
                cfm->status = ATT_ERR_INSUFF_AUTHOR;
            }
            break;
        }

        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY and GATT_INDICATE message meaning
 * that Measurement notification/indication has been correctly sent to peer device
 *
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t state  = ke_state_get(dest_id);

    if(state == GLPS_IDLE)
    {
        // Get the address of the environment
        struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);

        switch(param->operation)
        {
            case GATTC_NOTIFY:
            {
                /* send message indication if an error occurs,
                 * or if all notification complete event has been received
                 */
                if((param->status != GAP_ERR_NO_ERROR)
                        || (!(GLPS_IS(conidx, MEAS_CTX_SENT)))
                        || (GLPS_IS(conidx, MEAS_CTX_SENT) && (GLPS_IS(conidx, MEAS_SENT))))
                {
                    GLPS_CLEAR(conidx, SENDING_MEAS);

                    // send completed information to APP task
                    struct glps_cmp_evt * cmp_evt = KE_MSG_ALLOC(GLPS_CMP_EVT, prf_dst_task_get(&(glps_env->prf_env), conidx),
                            dest_id, glps_cmp_evt);

                    cmp_evt->request    = GLPS_SEND_MEAS_REQ_NTF_CMP;
                    cmp_evt->status     = param->status;

                    ke_msg_send(cmp_evt);
                }
                else
                {
                    // Measurement value notification sent
                    GLPS_SET(conidx, MEAS_SENT);
                }
            }
            break;
            case GATTC_INDICATE:
            {
                // verify if indication should be conveyed to application task
                if(GLPS_IS(conidx, RACP_RSP_SENT_BY_APP))
                {
                    // send completed information to APP task
                    struct glps_cmp_evt * cmp_evt = KE_MSG_ALLOC(GLPS_CMP_EVT, prf_dst_task_get(&(glps_env->prf_env), conidx),
                            dest_id, glps_cmp_evt);

                    cmp_evt->request    = GLPS_SEND_RACP_RSP_IND_CMP;
                    cmp_evt->status     = param->status;

                    ke_msg_send(cmp_evt);
                }

                GLPS_CLEAR(conidx, RACP_RSP_SENT_BY_APP);
            }
            break;

            default: break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
KE_MSG_HANDLER_TAB(glps)
{
    {GLPS_ENABLE_REQ,                     (ke_msg_func_t) glps_enable_req_handler},
    {GATTC_ATT_INFO_REQ_IND,              (ke_msg_func_t) gattc_att_info_req_ind_handler},
    {GATTC_WRITE_REQ_IND,                 (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_READ_REQ_IND,                  (ke_msg_func_t) gattc_read_req_ind_handler},
    {GLPS_SEND_MEAS_WITH_CTX_CMD,         (ke_msg_func_t) glps_meas_send_req_handler},
    {GLPS_SEND_MEAS_WITHOUT_CTX_CMD,      (ke_msg_func_t) glps_meas_send_req_handler},
    {GLPS_SEND_RACP_RSP_CMD,                   (ke_msg_func_t) glps_send_racp_rsp_cmd_handler},
    {GATTC_CMP_EVT,                       (ke_msg_func_t) gattc_cmp_evt_handler},
};

void glps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct glps_env_tag *glps_env = PRF_ENV_GET(GLPS, glps);

    task_desc->msg_handler_tab = glps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(glps_msg_handler_tab);
    task_desc->state           = glps_env->state;
    task_desc->idx_max         = GLPS_IDX_MAX;
}

#endif /* #if (BLE_GL_SENSOR) */

/// @} GLPSTASK
