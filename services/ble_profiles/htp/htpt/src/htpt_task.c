/**
 ****************************************************************************************
 * @addtogroup HTPTTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HT_THERMOM)

#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "htpt.h"
#include "htpt_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HTPT_ENABLE_REQ message.
 * The handler enables the Health Thermometer Profile Thermometer Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int htpt_enable_req_handler(ke_msg_id_t const msgid,
                                   struct htpt_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    // check state of the task
    if(gapc_get_conhdl(param->conidx) != GAP_INVALID_CONHDL)
    {
        // restore Bond Data
        struct htpt_env_tag* htpt_env =  PRF_ENV_GET(HTPT, htpt);
        htpt_env->ntf_ind_cfg[param->conidx] = param->ntf_ind_cfg;
        status = GAP_ERR_NO_ERROR;

    }

    // send response
    struct htpt_enable_rsp *rsp = KE_MSG_ALLOC(HTPT_ENABLE_RSP, src_id, dest_id, htpt_enable_rsp);
    rsp->conidx = param->conidx;
    rsp->status = status;
    ke_msg_send(rsp);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HTPT_TEMP_SEND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int htpt_temp_send_req_handler(ke_msg_id_t const msgid,
                                      struct htpt_temp_send_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Status
    int msg_status = KE_MSG_SAVED;
    uint8_t state = ke_state_get(dest_id);

    // check state of the task
    if(state == HTPT_IDLE)
    {
        // Get the address of the environment
        struct htpt_env_tag *htpt_env = PRF_ENV_GET(HTPT, htpt);

        // for intermediate measurement, feature must be enabled
        if(!(param->stable_meas) && (!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_INTERM_TEMP_CHAR_SUP)))
        {
            struct htpt_temp_send_rsp *rsp = KE_MSG_ALLOC(HTPT_TEMP_SEND_RSP, src_id, dest_id, htpt_temp_send_rsp);
            rsp->status = PRF_ERR_FEATURE_NOT_SUPPORTED;
            ke_msg_send(rsp);
        }
        else
        {
            // allocate operation to execute
            htpt_env->operation    = (struct htpt_op *) ke_malloc(sizeof(struct htpt_op) + HTPT_TEMP_MEAS_MAX_LEN, KE_MEM_ATT_DB);

            // Initialize operation parameters
            htpt_env->operation->cursor  = 0;
            htpt_env->operation->dest_id = src_id;
            htpt_env->operation->conidx  = GAP_INVALID_CONIDX;

            // Stable measurement indication or intermediate measurement notification
            if(param->stable_meas)
            {
                htpt_env->operation->op      = HTPT_CFG_STABLE_MEAS_IND;
                htpt_env->operation->handle  = HTPT_HANDLE(HTS_IDX_TEMP_MEAS_VAL);
            }
            else
            {
                htpt_env->operation->op      = HTPT_CFG_INTERM_MEAS_NTF;
                htpt_env->operation->handle  = HTPT_HANDLE(HTS_IDX_INTERM_TEMP_VAL);
            }

            //Pack the temperature measurement value
            htpt_env->operation->length  = htpt_pack_temp_value(&(htpt_env->operation->data[0]), param->temp_meas);

            // put task in a busy state
            ke_state_set(dest_id, HTPT_BUSY);

            // execute operation
            htpt_exe_operation();
        }

        msg_status = KE_MSG_CONSUMED;
    }

    return (msg_status);
}


/**
 ****************************************************************************************
 * @brief Request to update Measurement Interval Value
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int htpt_meas_intv_upd_req_handler(ke_msg_id_t const msgid,
                                          struct htpt_meas_intv_upd_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_SAVED;
    uint8_t state = ke_state_get(dest_id);

    // check state of the task
    if(state == HTPT_IDLE)
    {
        // Get the address of the environment
        struct htpt_env_tag *htpt_env = PRF_ENV_GET(HTPT, htpt);

        // update measurement interval
        htpt_env->meas_intv = param->meas_intv;

        //Check if Measurement Interval indication is supported
        if(!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_CHAR_SUP))
        {
            struct htpt_meas_intv_upd_rsp *rsp = KE_MSG_ALLOC(HTPT_MEAS_INTV_UPD_RSP, src_id, dest_id, htpt_meas_intv_upd_rsp);
            rsp->status = PRF_ERR_FEATURE_NOT_SUPPORTED;
            ke_msg_send(rsp);
        }
        else
        {
            // update internal measurement interval value
            htpt_env->meas_intv = param->meas_intv;

            // no indication to trigger
            if(!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_IND_SUP))
            {
                struct htpt_meas_intv_upd_rsp *rsp = KE_MSG_ALLOC(HTPT_MEAS_INTV_UPD_RSP, src_id, dest_id, htpt_meas_intv_upd_rsp);
                rsp->status = GAP_ERR_NO_ERROR;
                ke_msg_send(rsp);
            }
            // trigger measurement update indication
            else
            {
                // allocate operation to execute
                htpt_env->operation    = (struct htpt_op *) ke_malloc(sizeof(struct htpt_op) + HTPT_MEAS_INTV_MAX_LEN, KE_MEM_ATT_DB);

                // Initialize operation parameters
                htpt_env->operation->op      = HTPT_CFG_MEAS_INTV_IND;
                htpt_env->operation->handle  = HTPT_HANDLE(HTS_IDX_MEAS_INTV_VAL);
                htpt_env->operation->dest_id = src_id;
                htpt_env->operation->cursor  = 0;
                htpt_env->operation->conidx  = GAP_INVALID_CONIDX;

                // Pack the interval value
                htpt_env->operation->length = HTPT_MEAS_INTV_MAX_LEN;
                co_write16p(htpt_env->operation->data, param->meas_intv);

                // put task in a busy state
                ke_state_set(dest_id, HTPT_BUSY);

                // execute operation
                htpt_exe_operation();
            }
        }

        msg_status = KE_MSG_CONSUMED;
    }

    return (msg_status);
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
    struct htpt_env_tag* htpt_env = PRF_ENV_GET(HTPT, htpt);
    uint8_t att_idx = HTPT_IDX(param->handle);
    struct gattc_att_info_cfm * cfm;

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

    switch(att_idx)
    {
        case HTS_IDX_MEAS_INTV_VAL:
        {
            // force length to zero to reject any write starting from something != 0
            cfm->length = 0;
            cfm->status = GAP_ERR_NO_ERROR;
        }break;

        case HTS_IDX_TEMP_MEAS_IND_CFG:
        case HTS_IDX_INTERM_TEMP_CFG:
        case HTS_IDX_MEAS_INTV_CFG:
        {
            cfm->length = HTPT_IND_NTF_CFG_MAX_LEN;
            cfm->status = GAP_ERR_NO_ERROR;
        }break;

        default:
        {
            cfm->status = ATT_ERR_REQUEST_NOT_SUPPORTED;
        }break;
    }

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{

    struct htpt_env_tag* htpt_env = PRF_ENV_GET(HTPT, htpt);
    uint8_t conidx  = KE_IDX_GET(src_id);
    uint8_t status = ATT_ERR_NO_ERROR;
    int msg_status = KE_MSG_CONSUMED;

    // to check if confirmation message should be send
    bool send_cfm = true;

    // retrieve handle information
    uint8_t att_idx = HTPT_IDX(param->handle);

    if(param->length != HTPT_MEAS_INTV_MAX_LEN)
    {
        status = PRF_ERR_UNEXPECTED_LEN;
    }
    else
    {
        switch(att_idx)
        {
            case HTS_IDX_MEAS_INTV_VAL:
            {
                uint16_t meas_intv = co_read16p(param->value);

                // check measurement length validity
                if(((meas_intv >= htpt_env->meas_intv_min) && (meas_intv <= htpt_env->meas_intv_max))
                    // notification can be disabled anyway
                    || (meas_intv == 0))
                {
                    uint8_t state = ke_state_get(dest_id);
                    send_cfm = false;

                    // check state of the task to know if it can be proceed immediately
                    if(state == HTPT_IDLE)
                    {
                        // inform application that update of measurement interval is requested by peer device.
                        struct htpt_meas_intv_chg_req_ind * req_ind = KE_MSG_ALLOC(HTPT_MEAS_INTV_CHG_REQ_IND,
                                prf_dst_task_get(&htpt_env->prf_env, conidx), dest_id, htpt_meas_intv_chg_req_ind);
                        req_ind->conidx = conidx;
                        req_ind->intv   = meas_intv;
                        ke_msg_send(req_ind);

                        // allocate operation to execute
                        htpt_env->operation    = (struct htpt_op *) ke_malloc(sizeof(struct htpt_op) + HTPT_MEAS_INTV_MAX_LEN, KE_MEM_ATT_DB);

                        // Initialize operation parameters
                        htpt_env->operation->op      = HTPT_CFG_MEAS_INTV_IND;
                        htpt_env->operation->handle  = HTPT_HANDLE(HTS_IDX_MEAS_INTV_VAL);
                        htpt_env->operation->dest_id = dest_id;
                        htpt_env->operation->conidx  = conidx;
                        // to be sure that no notification will be triggered
                        htpt_env->operation->cursor  = 0xFF;

                        // Pack the interval value
                        htpt_env->operation->length = HTPT_MEAS_INTV_MAX_LEN;
                        co_write16p(htpt_env->operation->data, meas_intv);

                        // put task in a busy state
                        ke_state_set(dest_id, HTPT_BUSY);
                    }
                    else
                    {
                        msg_status = KE_MSG_SAVED;
                    }
                }
                else
                {
                    // value not in expected range
                    status = HTP_OUT_OF_RANGE_ERR_CODE;
                }
            }break;

            case HTS_IDX_TEMP_MEAS_IND_CFG:
            {
                status = htpt_update_ntf_ind_cfg(conidx, HTPT_CFG_STABLE_MEAS_IND, PRF_CLI_START_IND, co_read16p(param->value));
            }break;

            case HTS_IDX_INTERM_TEMP_CFG:
            {
                status = htpt_update_ntf_ind_cfg(conidx, HTPT_CFG_INTERM_MEAS_NTF, PRF_CLI_START_NTF, co_read16p(param->value));
            }break;

            case HTS_IDX_MEAS_INTV_CFG:
            {
                status = htpt_update_ntf_ind_cfg(conidx, HTPT_CFG_MEAS_INTV_IND, PRF_CLI_START_IND, co_read16p(param->value));
            }break;

            default:
            {
                status = ATT_ERR_REQUEST_NOT_SUPPORTED;
            }break;
        }
    }

    if(send_cfm)
    {
        //Send write response
        struct gattc_write_cfm * cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
        cfm->handle = param->handle;
        cfm->status = status;
        ke_msg_send(cfm);
    }

    return (msg_status);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_REQ_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    struct htpt_env_tag* htpt_env = PRF_ENV_GET(HTPT, htpt);
    uint8_t conidx  = KE_IDX_GET(src_id);
    uint8_t value[HTPT_MEAS_INTV_RANGE_MAX_LEN];
    uint8_t value_size = 0;
    uint8_t status = ATT_ERR_NO_ERROR;

    // retrieve handle information
    uint8_t att_idx = HTPT_IDX(param->handle);

    switch(att_idx)
    {
        case HTS_IDX_MEAS_INTV_VAL:
        {
            value_size = HTPT_MEAS_INTV_MAX_LEN;
            co_write16p(&(value[0]), htpt_env->meas_intv);
        }break;

        case HTS_IDX_MEAS_INTV_VAL_RANGE:
        {
            value_size = HTPT_MEAS_INTV_RANGE_MAX_LEN;
            co_write16p(&(value[0]), htpt_env->meas_intv_min);
            co_write16p(&(value[2]), htpt_env->meas_intv_max);
        }break;

        case HTS_IDX_TEMP_MEAS_IND_CFG:
        {
            value_size = HTPT_IND_NTF_CFG_MAX_LEN;
            co_write16p(value, ((htpt_env->ntf_ind_cfg[conidx] & HTPT_CFG_STABLE_MEAS_IND) != 0) ? PRF_CLI_START_IND : PRF_CLI_STOP_NTFIND);
        }break;

        case HTS_IDX_INTERM_TEMP_CFG:
        {
            value_size = HTPT_IND_NTF_CFG_MAX_LEN;
            co_write16p(value, ((htpt_env->ntf_ind_cfg[conidx] & HTPT_CFG_INTERM_MEAS_NTF) != 0) ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
        }break;

        case HTS_IDX_MEAS_INTV_CFG:
        {
            value_size = HTPT_IND_NTF_CFG_MAX_LEN;
            co_write16p(value, ((htpt_env->ntf_ind_cfg[conidx] & HTPT_CFG_MEAS_INTV_IND) != 0) ? PRF_CLI_START_IND : PRF_CLI_STOP_NTFIND);
        }break;

        case HTS_IDX_TEMP_TYPE_VAL:
        {
            value_size = HTPT_TEMP_TYPE_MAX_LEN;
            value[0]   = htpt_env->temp_type;
        }break;

        default:
        {
            status = ATT_ERR_REQUEST_NOT_SUPPORTED;
        }break;
    }

    // Send data to peer device
    struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, value_size);
    cfm->length = value_size;
    memcpy(cfm->value, value, value_size);
    cfm->handle = param->handle;
    cfm->status = status;

    // Send value to peer device.
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HTPT_MEAS_INTV_UPD_CFM message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int htpt_meas_intv_chg_cfm_handler(ke_msg_id_t const msgid, struct htpt_meas_intv_chg_cfm const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    struct htpt_env_tag* htpt_env = PRF_ENV_GET(HTPT, htpt);

    uint8_t state = ke_state_get(dest_id);

    // check state of the task
    if(state == HTPT_BUSY)
    {
        // retrieve connection index from operation
        uint8_t conidx = htpt_env->operation->conidx;

        //Send write response
        struct gattc_write_cfm * cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM,
                KE_BUILD_ID(TASK_GATTC, conidx), dest_id, gattc_write_cfm);
        cfm->handle = HTPT_HANDLE(HTS_IDX_MEAS_INTV_VAL);
        cfm->status = (param->conidx == conidx) ? param->status : PRF_APP_ERROR;
        ke_msg_send(cfm);


        // check if no error occurs
        if(cfm->status == GAP_ERR_NO_ERROR)
        {
            // update the current measurement interval
            htpt_env->meas_intv = co_read16p(htpt_env->operation->data);

            // check if an indication of new measurement interval should be triggered
            if(HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_IND_SUP))
            {
                // set back cursor to zero in order to send indication
                htpt_env->operation->cursor  = 0;
            }
        }

        // send indication or terminate operation
        htpt_exe_operation();
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
    // continue operation execution
    htpt_exe_operation();

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
KE_MSG_HANDLER_TAB(htpt)
{
    {HTPT_ENABLE_REQ,            (ke_msg_func_t) htpt_enable_req_handler},

    {GATTC_ATT_INFO_REQ_IND,     (ke_msg_func_t) gattc_att_info_req_ind_handler},
    {GATTC_WRITE_REQ_IND,        (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_READ_REQ_IND,         (ke_msg_func_t) gattc_read_req_ind_handler},
    {GATTC_CMP_EVT,              (ke_msg_func_t) gattc_cmp_evt_handler},

    {HTPT_TEMP_SEND_REQ,         (ke_msg_func_t) htpt_temp_send_req_handler},
    {HTPT_MEAS_INTV_UPD_REQ,     (ke_msg_func_t) htpt_meas_intv_upd_req_handler},
    {HTPT_MEAS_INTV_CHG_CFM,     (ke_msg_func_t) htpt_meas_intv_chg_cfm_handler},
};

void htpt_task_init(struct ke_task_desc *task_desc)
{
	BLE_PRF_HP_FUNC_ENTER();

    // Get the address of the environment
    struct htpt_env_tag *htpt_env = PRF_ENV_GET(HTPT, htpt);

    task_desc->msg_handler_tab = htpt_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(htpt_msg_handler_tab);
    task_desc->state           = htpt_env->state;
    task_desc->idx_max         = HTPT_IDX_MAX;

	BLE_PRF_HP_FUNC_LEAVE();
}

#endif //BLE_HT_THERMOM

/// @} HTPTTASK
