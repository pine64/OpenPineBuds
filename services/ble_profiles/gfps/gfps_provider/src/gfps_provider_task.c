/**
 ****************************************************************************************
 * @addtogroup GFPS STASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_GFPS_PROVIDER)
#include <string.h>

#include "co_utils.h"
#include "gap.h"
#include "gattc_task.h"
#include "gfps_provider.h"
//#include "crypto.h"
#include "gfps_provider_task.h"
#include "prf_utils.h"

#include "ke_mem.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static void send_notifiction(uint8_t conidx, uint8_t contentType, const uint8_t* ptrData, uint32_t length)
{
    struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&gfpsp_env->prf_env, conidx),
            gattc_send_evt_cmd, length);

    // Fill in the parameter structure
    report_ntf->operation = GATTC_NOTIFY;
    report_ntf->handle    = gfpsp_env->start_hdl + contentType;//GFPSP_IDX_KEY_BASED_PAIRING_VAL;//handle_offset;
    // pack measured value in database
    report_ntf->length    = length;
    memcpy(report_ntf->value, ptrData, length);
    // send notification to peer device
    ke_msg_send(report_ntf);
}

__STATIC int send_data_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, GFPSP_IDX_KEY_BASED_PAIRING_VAL, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

__STATIC int send_passkey_data_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, GFPSP_IDX_PASSKEY_VAL, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

__STATIC int send_name_via_notification_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, GFPSP_IDX_NAME_VAL, param->value, param->length);
    return (KE_MSG_CONSUMED);
}


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of write request message.
 * The handler will analyse what has been set and decide alert level
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
uint8_t Encrypted_req[80]={
    0xb9,0x7b,0x2c,0x4b,0x84,0x59,0x3e,0x69,0xf9,0xc5,
    0x1c,0x6f,0xf9,0xba,0x7e,0xc0,0x27,0xa6,0x13,0x55,
    0x26,0x84,0xbc,0xa2,0xd8,0x95,0xd6,0xf8,0xdd,0x5e,
    0xb5,0x91,0xfe,0xf7,0x31,0x1c,0x19,0x3e,0x38,0x8e,
    0x5f,0x3a,0xe6,0x6b,0x68,0x46,0xc4,0x14,0x1c,0x03,
    0xcb,0xc3,0x18,0x06,0x6b,0x52,0xd9,0x5c,0xa0,0xa2,
    0xb5,0x80,0xd9,0x90,0x4b,0xed,0x46,0x23,0x22,0x9b,
    0x42,0xe7,0xc2,0xde,0x2e,0x2a,0xba,0x7c,0xac,0x2b
};

__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{   
    // Get the address of the environment
    struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = GAP_ERR_NO_ERROR;

    if (GFPSP_IDX_KEY_BASED_PAIRING_VAL != (param->handle-gfpsp_env->start_hdl) &&
        (GFPSP_IDX_NAME_VAL != (param->handle-gfpsp_env->start_hdl)))
    {
        //Send write response
        struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
        cfm->handle = param->handle;
        cfm->status = status;
        ke_msg_send(cfm);
    }
    
    if (gfpsp_env != NULL)
    {
        switch (param->handle-gfpsp_env->start_hdl)
        {
            case GFPSP_IDX_KEY_BASED_PAIRING_VAL:
            {
                //inform APP of data
                struct gfpsp_write_ind_t * ind = KE_MSG_ALLOC_DYN(GFPSP_KEY_BASED_PAIRING_WRITE_IND,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        gfpsp_write_ind_t,
                        param->length);
                ind->length = param->length; 
                ind->pendingWriteRsp.dst_task_id = src_id;
                ind->pendingWriteRsp.src_task_id = dest_id;
                ind->pendingWriteRsp.handle = param->handle;
                memcpy((uint8_t *)(ind->data),&(param->value), param->length);

                ke_msg_send(ind);
                break;
            }
            case GFPSP_IDX_KEY_BASED_PAIRING_NTF_CFG:
            {
                uint16_t value = 0x0000;
                memcpy(&value, &(param->value), sizeof(uint16_t));
                struct app_gfps_key_based_notif_config_t * ind = KE_MSG_ALLOC(GFPSP_KEY_BASED_PAIRING_NTF_CFG,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        app_gfps_key_based_notif_config_t);

                ind->isNotificationEnabled = value;
                ke_msg_send(ind);
                break;
            }
            case GFPSP_IDX_PASSKEY_NTF_CFG:
            {
                uint16_t value = 0x0000;            
                memcpy(&value, &(param->value), sizeof(uint16_t));
                struct app_gfps_pass_key_notif_config_t * ind = KE_MSG_ALLOC(GFPSP_KEY_PASS_KEY_NTF_CFG,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        app_gfps_pass_key_notif_config_t);

                ind->isNotificationEnabled = value;
                ke_msg_send(ind);
            	break;  
            }
            case GFPSP_IDX_PASSKEY_VAL:
            {
                //inform APP of data
                struct gfpsp_write_ind_t * ind = KE_MSG_ALLOC_DYN(GFPSP_KEY_PASS_KEY_WRITE_IND,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        gfpsp_write_ind_t,
                        param->length);
                ind->length = param->length; 
                memcpy((uint8_t *)(ind->data),&(param->value), param->length);
    
                ke_msg_send(ind); 
                break;
            }
            case GFPSP_IDX_ACCOUNT_KEY_VAL:
            {
                TRACE(0,"Get account key:");
                DUMP8("%02x ", param->value, param->length);
                struct gfpsp_write_ind_t * ind = KE_MSG_ALLOC_DYN(GFPSP_KEY_ACCOUNT_KEY_WRITE_IND,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        gfpsp_write_ind_t,
                        param->length);
                ind->length = param->length; 
                memcpy((uint8_t *)(ind->data),&(param->value), param->length);
    
                ke_msg_send(ind);                
                break;
            }
            case GFPSP_IDX_NAME_VAL:
            {
                TRACE(0,"Get updated name:");
                DUMP8("%02x ", param->value, param->length);

                struct gfpsp_write_ind_t * ind = KE_MSG_ALLOC_DYN(GFPSP_NAME_WRITE_IND,
                        prf_dst_task_get(&gfpsp_env->prf_env, conidx),
                        prf_src_task_get(&gfpsp_env->prf_env, conidx),
                        gfpsp_write_ind_t,
                        param->length);
                ind->length = param->length; 
                ind->pendingWriteRsp.dst_task_id = src_id;
                ind->pendingWriteRsp.src_task_id = dest_id;
                ind->pendingWriteRsp.handle = param->handle;
                memcpy((uint8_t *)(ind->data),&(param->value), param->length);
    
                ke_msg_send(ind); 
                break;
            }
            default:
            {
                break;
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

__STATIC int gfpsp_send_write_rsp_handler(ke_msg_id_t const msgid,
                                      struct gfpsp_send_write_rsp_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
            GATTC_WRITE_CFM, param->dst_task_id, param->src_task_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = param->status;
    ke_msg_send(cfm);

    return KE_MSG_CONSUMED;
}

static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct gattc_att_info_cfm * cfm;

  

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

    struct gfpsp_env_tag* gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    TRACE(1,"gattc_att_info_req_ind_handler, is %d",param->handle-gfpsp_env->start_hdl);

    if (param->handle == (gfpsp_env->start_hdl + GFPSP_KEY_BASED_PAIRING_NTF_CFG)) {
        // CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
    } else if (param->handle == (gfpsp_env->start_hdl + GFPSP_KEY_PASS_KEY_NTF_CFG)) {
        // CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
    } else if (param->handle == (gfpsp_env->start_hdl + GFPSP_IDX_ACCOUNT_KEY_CFG)) {
        // CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
    } else if (param->handle == (gfpsp_env->start_hdl + GFPSP_IDX_NAME_CFG)) {
        // CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
    } else if (param->handle == (gfpsp_env->start_hdl + GFPSP_IDX_KEY_BASED_PAIRING_VAL)) {
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;         
    } else if (param->handle == (gfpsp_env->start_hdl + GFPSP_IDX_PASSKEY_VAL)) {
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;         
    } else if (param->handle == (gfpsp_env->start_hdl + GFPSP_IDX_ACCOUNT_KEY_VAL)) {
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;         
    } else if (param->handle == (gfpsp_env->start_hdl + GFPSP_IDX_NAME_VAL)) {
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;         
    } 
    else {
        cfm->length = 0;
        cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
//
/// Default State handlers definition
KE_MSG_HANDLER_TAB(gfpsp)
{
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t) gattc_write_req_ind_handler},
    {GFPSP_KEY_BASED_PAIRING_WRITE_NOTIFY,      (ke_msg_func_t)send_data_via_notification_handler},
    {GFPSP_KEY_PASS_KEY_WRITE_NOTIFY,           (ke_msg_func_t)send_passkey_data_via_notification_handler},
    {GFPSP_NAME_NOTIFY,             (ke_msg_func_t)send_name_via_notification_handler},
    {GFPSP_SEND_WRITE_RESPONSE,     (ke_msg_func_t)gfpsp_send_write_rsp_handler},
    {GATTC_ATT_INFO_REQ_IND,        (ke_msg_func_t)gattc_att_info_req_ind_handler },
};

void gfpsp_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct gfpsp_env_tag *gfpsp_env = PRF_ENV_GET(GFPSP, gfpsp);

    task_desc->msg_handler_tab = gfpsp_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(gfpsp_msg_handler_tab);
    task_desc->state           = gfpsp_env->state;
    task_desc->idx_max         = GFPSP_IDX_MAX;
}

#endif //BLE_GFPS_SERVER

/// @} DISSTASK
