/**
 ****************************************************************************************
 * @addtogroup HOGPDTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_DEVICE)
#include "gap.h"
#include "gattc_task.h"

//HID Over GATT Profile Device Role Functions
#include "hogpd.h"
#include "hogpd_task.h"

#include "prf_utils.h"

#include "co_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_ENABLE_REQ message.
 * The handler enables the HID Over GATT Profile Device Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hogpd_enable_req_handler(ke_msg_id_t const msgid,
                                    struct hogpd_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Counter for HIDS instance
    uint8_t svc_idx;
    uint8_t status = GAP_ERR_NO_ERROR;

    struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);

    ASSERT_ERR(hogpd_env!=NULL);

    // Check provided values and check if connection exists
    if((param->conidx > BLE_CONNECTION_MAX)
            || (gapc_get_conhdl(param->conidx) == GAP_INVALID_CONHDL))
    {
        // an error occurs, trigg it.
        status = (param->conidx > BLE_CONNECTION_MAX) ? GAP_ERR_INVALID_PARAM : PRF_ERR_REQ_DISALLOWED;
    }
    else
    {
        for (svc_idx = 0; svc_idx < hogpd_env->hids_nb; svc_idx++)
        {
            // Retrieve notification configuration
            hogpd_env->svcs[svc_idx].ntf_cfg[param->conidx]   = param->ntf_cfg[svc_idx];
        }
    }

    // Send back response
    struct hogpd_enable_rsp* rsp = KE_MSG_ALLOC(HOGPD_ENABLE_RSP, src_id, dest_id, hogpd_enable_rsp);
    rsp->conidx = param->conidx;
    rsp->status = status;
    ke_msg_send(rsp);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_REPORT_UPD_REQ message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hogpd_report_upd_req_handler(ke_msg_id_t const msgid,
                                        struct hogpd_report_upd_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);

    // check that task is in idle state
    if((state & HOGPD_REQ_BUSY) == HOGPD_IDLE)
    {
        // Status
        uint8_t status = PRF_ERR_INVALID_PARAM;
        // Check provided values and check if connection exists
        if((param->conidx > BLE_CONNECTION_MAX)
                || (gapc_get_conhdl(param->conidx) == GAP_INVALID_CONHDL))
        {
            status = (param->conidx > BLE_CONNECTION_MAX) ? GAP_ERR_INVALID_PARAM : PRF_ERR_REQ_DISALLOWED;
        }
        else
        {
            status = hogpd_ntf_send(param->conidx, &(param->report));
        }

        // an error occurs inform application
        if (status != GAP_ERR_NO_ERROR)
        {
            // send report update response
            struct hogpd_report_upd_rsp *rsp = KE_MSG_ALLOC(HOGPD_REPORT_UPD_RSP,
                                                            src_id, dest_id, hogpd_report_upd_rsp);
            rsp->conidx = param->conidx;
            rsp->status = status;
            ke_msg_send(rsp);
        }
        // go in a busy state
        else
        {
            ke_state_set(dest_id, state | HOGPD_REQ_BUSY);
        }
    }
    // else process it later
    else
    {
        msg_status = KE_MSG_SAVED;
    }

    return (msg_status);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_REPORT_CFM message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hogpd_report_cfm_handler(ke_msg_id_t const msgid,
                                    struct hogpd_report_cfm const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{

    uint8_t state = ke_state_get(dest_id);

    // check that an operation is ongoing
    if((state & HOGPD_OP_BUSY) == HOGPD_OP_BUSY)
    {
        uint16_t handle = ATT_INVALID_HANDLE;
        uint8_t  status = PRF_APP_ERROR;
        struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);

        // check received status
        if(param->status != GAP_ERR_NO_ERROR)
        {
            status = param->status;
        }
        // perform operation sanity check
        else if((param->operation == hogpd_env->op.operation) && (param->conidx == hogpd_env->op.conidx))
        {
            // retrieve attribute index.
            uint8_t att_idx = 0;
            switch(param->report.type)
            {
                case HOGPD_REPORT:
                {
                    att_idx = HOGPD_IDX_REPORT_VAL;
                }break;
                case HOGPD_REPORT_MAP:
                {
                    att_idx = HOGPD_IDX_REPORT_MAP_VAL;
                }break;
                case HOGPD_BOOT_KEYBOARD_INPUT_REPORT:
                {
                    att_idx = HOGPD_IDX_BOOT_KB_IN_REPORT_VAL;
                }break;
                case HOGPD_BOOT_KEYBOARD_OUTPUT_REPORT:
                {
                    att_idx = HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL;
                }break;
                case HOGPD_BOOT_MOUSE_INPUT_REPORT:
                {
                    att_idx = HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL;
                }break;
                default: /* Nothing to do */ break;
            }

            // retrieve handle
            handle = hogpd_get_att_handle(hogpd_env, param->report.hid_idx, att_idx, param->report.idx);

            // check if answer correspond to expected one
            if(handle == hogpd_env->op.handle)
            {
                status = GAP_ERR_NO_ERROR;
            }
        }

        // read operation
        if(hogpd_env->op.operation == HOGPD_OP_REPORT_READ)
        {
            uint16_t length = (status == GAP_ERR_NO_ERROR) ? param->report.length : 0;
            //Send read response
            struct gattc_read_cfm * rd_cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM,
                    KE_BUILD_ID(TASK_GATTC,  hogpd_env->op.conidx), dest_id, gattc_read_cfm, length);
            rd_cfm->handle = hogpd_env->op.handle;
            rd_cfm->status = status;
            rd_cfm->length = length;

            if(status == GAP_ERR_NO_ERROR)
            {
                memcpy(rd_cfm->value, param->report.value, length);
            }

            ke_msg_send(rd_cfm);
        }
        // write operation
        else
        {
            //Send write response
            struct gattc_write_cfm * wr_cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM,
                    KE_BUILD_ID(TASK_GATTC,  hogpd_env->op.conidx), dest_id, gattc_write_cfm);
            wr_cfm->handle = hogpd_env->op.handle;
            wr_cfm->status = status;
            ke_msg_send(wr_cfm);
        }

        // cleanup environment
        hogpd_env->op.operation = HOGPD_OP_NO;
        hogpd_env->op.handle    = ATT_INVALID_HDL;
        hogpd_env->op.conidx    = 0xFF;

        // go back in idle state
        ke_state_set(dest_id, state & ~HOGPD_OP_BUSY);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_PROTO_MODE_CFM message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hogpd_proto_mode_cfm_handler(ke_msg_id_t const msgid,
                                    struct hogpd_proto_mode_cfm const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{

    uint8_t state = ke_state_get(dest_id);

    // check that an operation is ongoing
    if((state & HOGPD_OP_BUSY) == HOGPD_OP_BUSY)
    {
        uint16_t handle = ATT_INVALID_HANDLE;
        uint8_t  status = PRF_APP_ERROR;
        struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);

        // check received status
        if(param->status != GAP_ERR_NO_ERROR)
        {
            status = param->status;
        }
        // perform operation sanity check
        else if((hogpd_env->op.operation == HOGPD_OP_PROT_UPDATE) && (param->conidx == hogpd_env->op.conidx))
        {
            // retrieve handle
            handle = hogpd_get_att_handle(hogpd_env, param->hid_idx, HOGPD_IDX_PROTO_MODE_VAL, 0);

            // check if answer correspond to expected one
            if(handle == hogpd_env->op.handle)
            {
                status = GAP_ERR_NO_ERROR;
                hogpd_env->svcs[param->hid_idx].proto_mode = param->proto_mode;
            }
        }

        // read operation/something failed
        if(hogpd_env->op.operation == HOGPD_OP_REPORT_READ)
        {
            //Send read response
            struct gattc_read_cfm * rd_cfm = KE_MSG_ALLOC(GATTC_READ_CFM,
                    KE_BUILD_ID(TASK_GATTC,  hogpd_env->op.conidx), dest_id, gattc_read_cfm);
            rd_cfm->handle = hogpd_env->op.handle;
            rd_cfm->status = status;
            ke_msg_send(rd_cfm);
        }
        // write operation
        else
        {
            //Send write response
            struct gattc_write_cfm * wr_cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM,
                    KE_BUILD_ID(TASK_GATTC,  hogpd_env->op.conidx), dest_id, gattc_write_cfm);
            wr_cfm->handle = hogpd_env->op.handle;
            wr_cfm->status = status;
            ke_msg_send(wr_cfm);
        }

        // cleanup environment
        hogpd_env->op.operation = HOGPD_OP_NO;
        hogpd_env->op.handle    = ATT_INVALID_HDL;
        hogpd_env->op.conidx    = 0xFF;

        // go back in idle state
        ke_state_set(dest_id, state & ~HOGPD_OP_BUSY);
    }

    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_ATT_INFO_REQ_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_att_info_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t hid_idx, att_idx, report_idx;
    struct gattc_att_info_cfm * cfm;
    struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);
    // retrieve attribute requested
    uint8_t status = hogpd_get_att_idx(hogpd_env, param->handle, &hid_idx, &att_idx, &report_idx);

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

    if(status == GAP_ERR_NO_ERROR)
    {
        // check which attribute is requested by peer device
        switch(att_idx)
        {
            case HOGPD_IDX_BOOT_KB_IN_REPORT_VAL:
            case HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL:
            case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL:
            case HOGPD_IDX_REPORT_VAL:
            {
                cfm->length = 0;
            }break;

            // Notification configuration
            case HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG:
            case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG:
            case HOGPD_IDX_REPORT_NTF_CFG:
            {
                cfm->length = 2;
            }break;

            default:
            {
                cfm->length = 0;
                status = ATT_ERR_WRITE_NOT_PERMITTED;
            } break;
        }
    }

    cfm->status = status;

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_REQ_IND message.
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
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);

    // check that task is in idle state
    if((state & HOGPD_OP_BUSY) == HOGPD_IDLE)
    {
        uint8_t hid_idx, att_idx, report_idx;

        // used to know if gatt_write confirmation should be sent immediately
        uint8_t conidx = KE_IDX_GET(src_id);

        struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);
        // retrieve attribute requested
        uint8_t status = hogpd_get_att_idx(hogpd_env, param->handle, &hid_idx, &att_idx, &report_idx);


        // prepare operation info
        hogpd_env->op.conidx    = conidx;
        hogpd_env->op.operation = HOGPD_OP_NO;
        hogpd_env->op.handle    = param->handle;

        if(status == GAP_ERR_NO_ERROR)
        {

            // check which attribute is requested by peer device
            switch(att_idx)
            {
                // Control point value updated
                case HOGPD_IDX_HID_CTNL_PT_VAL:
                {
                    // send control point indication
                    struct hogpd_ctnl_pt_ind *cp_ind = KE_MSG_ALLOC(HOGPD_CTNL_PT_IND,
                                                                prf_dst_task_get(&(hogpd_env->prf_env), conidx), dest_id,
                                                                hogpd_ctnl_pt_ind);
                    cp_ind->conidx      = conidx;
                    cp_ind->hid_idx     = hid_idx;
                    cp_ind->hid_ctnl_pt = param->value[0];
                    ke_msg_send(cp_ind);
                }break;

                // Modification of protocol mode requested
                case HOGPD_IDX_PROTO_MODE_VAL:
                {
                    // send control point indication
                    struct hogpd_proto_mode_req_ind *prot_ind = KE_MSG_ALLOC(HOGPD_PROTO_MODE_REQ_IND,
                                                                prf_dst_task_get(&(hogpd_env->prf_env), conidx), dest_id,
                                                                hogpd_proto_mode_req_ind);
                    prot_ind->conidx      = conidx;
                    prot_ind->operation   = HOGPD_OP_PROT_UPDATE;
                    prot_ind->hid_idx     = hid_idx;
                    prot_ind->proto_mode  = param->value[0];
                    ke_msg_send(prot_ind);

                    hogpd_env->op.operation = HOGPD_OP_PROT_UPDATE;
                }break;

                // Modification of report value requested
                case HOGPD_IDX_BOOT_KB_IN_REPORT_VAL:
                case HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL:
                case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL:
                case HOGPD_IDX_REPORT_VAL:
                {
                    // send report indication
                    struct hogpd_report_req_ind *report_ind = KE_MSG_ALLOC_DYN(HOGPD_REPORT_REQ_IND,
                            prf_dst_task_get(&(hogpd_env->prf_env), conidx), dest_id,
                            hogpd_report_req_ind, param->length);

                    report_ind->conidx         = conidx;
                    report_ind->operation      = HOGPD_OP_REPORT_WRITE;
                    report_ind->report.length  = param->length;
                    report_ind->report.hid_idx = hid_idx;
                    report_ind->report.idx     = report_idx;

                    memcpy(report_ind->report.value, param->value, param->length);

                    // retrieve report type
                    switch(att_idx)
                    {
                        // An Input Report
                        case HOGPD_IDX_BOOT_KB_IN_REPORT_VAL:
                        {
                            report_ind->report.type = HOGPD_BOOT_KEYBOARD_INPUT_REPORT;
                        }break;
                        // Boot Keyboard input report
                        case HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL:
                        {
                            report_ind->report.type = HOGPD_BOOT_KEYBOARD_OUTPUT_REPORT;
                        }break;
                        // Boot Mouse input report
                        case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL:
                        {
                            report_ind->report.type = HOGPD_BOOT_MOUSE_INPUT_REPORT;
                        }break;
                        // Normal report
                        case HOGPD_IDX_REPORT_VAL:
                        {
                            report_ind->report.type = HOGPD_REPORT;
                        }break;

                        default:
                        {
                            ASSERT_ERR(0);
                        }break;
                    }

                    ke_msg_send(report_ind);

                    hogpd_env->op.operation = HOGPD_OP_REPORT_WRITE;
                }break;

                // Notification configuration update
                case HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG:
                case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG:
                case HOGPD_IDX_REPORT_NTF_CFG:
                {
                    uint16_t ntf_cfg = co_read16p(param->value);
                    status = hogpd_ntf_cfg_ind_send(conidx, hid_idx, att_idx, report_idx, ntf_cfg);
                }break;

                default:
                {
                    status = PRF_APP_ERROR;
                } break;
            }
        }

        // check if peer operation is over
        if(hogpd_env->op.operation == HOGPD_OP_NO)
        {
            //Send write response
            struct gattc_write_cfm * cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
            cfm->handle = param->handle;
            cfm->status = status;
            ke_msg_send(cfm);
        }
        else
        {
            // go into operation busy state
            ke_state_set(dest_id, state | HOGPD_OP_BUSY);
        }
    }
    // else process it later
    else
    {
        msg_status = KE_MSG_SAVED;
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
__STATIC int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);

    // check that task is in idle state
    if((state & HOGPD_OP_BUSY) == HOGPD_IDLE)
    {
        uint8_t hid_idx, att_idx, report_idx;
        bool finished = true;
        uint8_t report_type = 0;
        uint16_t value = 0;
        uint16_t length = 0;

        struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);

        // used to know if gatt_write confirmation should be sent immediately
        uint8_t conidx = KE_IDX_GET(src_id);

        // retrieve attribute requested
        uint8_t status = hogpd_get_att_idx(hogpd_env, param->handle, &hid_idx, &att_idx, &report_idx);

        if(status == GAP_ERR_NO_ERROR)
        {
            // check which attribute is requested by peer device
            switch(att_idx)
            {
                //  ------------ READ report value requested
                case HOGPD_IDX_BOOT_KB_IN_REPORT_VAL:
                {
                    report_type = HOGPD_BOOT_KEYBOARD_INPUT_REPORT;
                    finished = false;
                }break;
                case HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL:
                {
                    report_type = HOGPD_BOOT_KEYBOARD_OUTPUT_REPORT;
                    finished = false;
                }break;
                case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL:
                {
                    report_type = HOGPD_BOOT_MOUSE_INPUT_REPORT;
                    finished = false;
                }break;
                case HOGPD_IDX_REPORT_VAL:
                {
                    report_type = HOGPD_REPORT;
                    finished = false;
                }break;
                case HOGPD_IDX_REPORT_MAP_VAL:
                {
                    report_type = HOGPD_REPORT_MAP;
                    finished = false;
                }break;

                //  ------------ READ active protocol mode
                case HOGPD_IDX_PROTO_MODE_VAL:
                {
                    value =  hogpd_env->svcs[hid_idx].proto_mode;
                    length = sizeof(uint8_t);
                }break;

                //  ------------ READ Notification configuration
                case HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG:
                {
                    value = ((hogpd_env->svcs[hid_idx].ntf_cfg[conidx] & HOGPD_CFG_KEYBOARD) != 0)
                            ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

                    length = sizeof(uint16_t);
                }break;
                case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG:
                {
                    value = ((hogpd_env->svcs[hid_idx].ntf_cfg[conidx] & HOGPD_CFG_MOUSE) != 0)
                            ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;
                    length = sizeof(uint16_t);
                }break;
                case HOGPD_IDX_REPORT_NTF_CFG:
                {
                    value = ((hogpd_env->svcs[hid_idx].ntf_cfg[conidx] & (HOGPD_CFG_REPORT_NTF_EN << report_idx)) != 0)
                            ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;
                    length = sizeof(uint16_t);
                }break;

                default:
                {
                    status = PRF_APP_ERROR;
                } break;
            }
        }

        // check if peer operation is over
        if(finished)
        {
            //Send write response
            struct gattc_read_cfm * cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, length);
            cfm->handle = param->handle;
            cfm->status = status;
            cfm->length = length;
            memcpy(cfm->value, &value, length);
            ke_msg_send(cfm);
        }
        else
        {
            // send report indication
            struct hogpd_report_req_ind *report_ind = KE_MSG_ALLOC(HOGPD_REPORT_REQ_IND,
                    prf_dst_task_get(&(hogpd_env->prf_env), conidx), dest_id,
                    hogpd_report_req_ind);

            report_ind->conidx         = conidx;
            report_ind->operation      = HOGPD_OP_REPORT_READ;
            report_ind->report.length  = 0;
            report_ind->report.hid_idx = hid_idx;
            report_ind->report.type    = report_type;
            report_ind->report.idx     = report_idx;

            ke_msg_send(report_ind);

            hogpd_env->op.conidx    = conidx;
            hogpd_env->op.operation = HOGPD_OP_REPORT_READ;
            hogpd_env->op.handle    = param->handle;

            // go into operation busy state
            ke_state_set(dest_id, state | HOGPD_OP_BUSY);
        }
    }
    // else process it later
    else
    {
        msg_status = KE_MSG_SAVED;
    }

    return (msg_status);
}


/**
 ****************************************************************************************
 * @brief Handles @ref GATT_NOTIFY_CMP_EVT message meaning that Report notification
 * has been correctly sent to peer device (but not confirmed by peer device).
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if(param->operation == GATTC_NOTIFY)
    {
        uint8_t conidx = KE_IDX_GET(src_id);
        struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);

        // send report update response
        struct hogpd_report_upd_rsp *rsp = KE_MSG_ALLOC(HOGPD_REPORT_UPD_RSP,
                                                         prf_dst_task_get(&(hogpd_env->prf_env), conidx),
                                                         dest_id, hogpd_report_upd_rsp);
        rsp->conidx = conidx;
        rsp->status = param->status;
        ke_msg_send(rsp);

        // go back in to idle mode
        ke_state_set(dest_id, ke_state_get(dest_id) & ~HOGPD_REQ_BUSY);
    } // else ignore the message

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/// Default State handlers definition
KE_MSG_HANDLER_TAB(hogpd)
{
    { HOGPD_ENABLE_REQ,              (ke_msg_func_t) hogpd_enable_req_handler },
    { HOGPD_REPORT_UPD_REQ,          (ke_msg_func_t) hogpd_report_upd_req_handler },
    { HOGPD_REPORT_CFM,              (ke_msg_func_t) hogpd_report_cfm_handler },
    { HOGPD_PROTO_MODE_CFM,          (ke_msg_func_t) hogpd_proto_mode_cfm_handler },

    { GATTC_ATT_INFO_REQ_IND,        (ke_msg_func_t) gattc_att_info_req_ind_handler },
    { GATTC_WRITE_REQ_IND,           (ke_msg_func_t) gattc_write_req_ind_handler },
    { GATTC_READ_REQ_IND,            (ke_msg_func_t) gattc_read_req_ind_handler },
    { GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler },
};

void hogpd_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct hogpd_env_tag *hogpd_env = PRF_ENV_GET(HOGPD, hogpd);

    task_desc->msg_handler_tab = hogpd_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(hogpd_msg_handler_tab);
    task_desc->state           = hogpd_env->state;
    task_desc->idx_max         = HOGPD_IDX_MAX;
}

#endif /* #if (BLE_HID_DEVICE) */

/// @} HOGPDTASK
