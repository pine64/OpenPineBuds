/**
 ****************************************************************************************
 * @addtogroup HOGPRHTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_REPORT_HOST)

#include "gap.h"
#include "attm.h"
#include "hogprh_task.h"
#include "hogprh.h"
#include "gattc_task.h"
#include "gattc.h"
#include "hogp_common.h"

#include "ke_mem.h"
#include "co_utils.h"
#include "co_math.h"

/*
 * LOCAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/// State machine used to retrieve HID Service characteristics information
const struct prf_char_def hogprh_hids_char[HOGPRH_CHAR_REPORT + 1] =
{
    /// Report Map
    [HOGPRH_CHAR_REPORT_MAP]             = {ATT_CHAR_REPORT_MAP,    ATT_MANDATORY, ATT_CHAR_PROP_RD},
    /// HID Information
    [HOGPRH_CHAR_HID_INFO]               = {ATT_CHAR_HID_INFO,      ATT_MANDATORY, ATT_CHAR_PROP_RD},
    /// HID Control Point
    [HOGPRH_CHAR_HID_CTNL_PT]            = {ATT_CHAR_HID_CTNL_PT,   ATT_MANDATORY,  ATT_CHAR_PROP_WR_NO_RESP},
    /// Protocol Mode
    [HOGPRH_CHAR_PROTOCOL_MODE]          = {ATT_CHAR_PROTOCOL_MODE, ATT_OPTIONAL,  (ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP)},
    /// Report
    [HOGPRH_CHAR_REPORT]                 = {ATT_CHAR_REPORT,        ATT_MANDATORY, ATT_CHAR_PROP_RD},
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_ENABLE_REQ message.
 * The handler enables the HID Over GATT Profile Boot Host Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hogprh_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hogprh_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    int msg_status = KE_MSG_CONSUMED;

    uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Battery service Client Role Task Environment
    struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);

    ASSERT_INFO(hogprh_env != NULL, dest_id, src_id);
    if((state == HOGPRH_IDLE) && (hogprh_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        hogprh_env->env[conidx] = (struct hogprh_cnx_env*) ke_malloc(sizeof(struct hogprh_cnx_env),KE_MEM_ATT_DB);
        memset(hogprh_env->env[conidx], 0, sizeof(struct hogprh_cnx_env));

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HID on peer
            prf_disc_svc_send(&(hogprh_env->prf_env), conidx, ATT_SVC_HID);

            // Go to DISCOVERING state
            ke_state_set(dest_id, HOGPRH_BUSY);
            hogprh_env->env[conidx]->operation = ke_param2msg(param);
            msg_status = KE_MSG_NO_FREE;
        }
        //normal connection, get saved att details
        else
        {
            hogprh_env->env[conidx]->hids_nb = param->hids_nb;
            memcpy(&(hogprh_env->env[conidx]->hids[0]), &(param->hids[0]), sizeof(struct hogprh_content) * HOGPRH_NB_HIDS_INST_MAX);

            //send APP confirmation that can start normal connection to TH
            hogprh_enable_rsp_send(hogprh_env, conidx, GAP_ERR_NO_ERROR);
        }
    }
    else if(state != HOGPRH_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        hogprh_enable_rsp_send(hogprh_env, conidx, status);
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

    if(state == HOGPRH_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);

        ASSERT_INFO(hogprh_env != NULL, dest_id, src_id);
        ASSERT_INFO(hogprh_env->env[conidx] != NULL, dest_id, src_id);

        if(hogprh_env->env[conidx]->hids_nb < HOGPRH_NB_HIDS_INST_MAX)
        {
            uint8_t i;
            struct prf_char_def hids_char[HOGPRH_CHAR_MAX];
            struct prf_char_desc_def hids_char_desc[HOGPRH_DESC_MAX];
            uint8_t hid_idx = hogprh_env->env[conidx]->hids_nb;
            uint8_t fnd_att;



            // 1. Create characteristic reference
            memcpy(hids_char, hogprh_hids_char, sizeof(struct prf_char_def) * HOGPRH_CHAR_REPORT);
            for(i = HOGPRH_CHAR_REPORT ; i < HOGPRH_CHAR_MAX ; i++)
            {
                hids_char[i] = hogprh_hids_char[HOGPRH_CHAR_REPORT];
            }

            // 2. create descriptor reference
            // Report Map Char. External Report Reference Descriptor
            hids_char_desc[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].char_code = HOGPRH_CHAR_REPORT_MAP;
            hids_char_desc[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].req_flag  = ATT_OPTIONAL;
            hids_char_desc[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].uuid      = ATT_DESC_EXT_REPORT_REF;

            for(i = 0 ; i < HOGPRH_NB_REPORT_INST_MAX ; i++)
            {
                // Report Char. Report Reference
                hids_char_desc[HOGPRH_DESC_REPORT_REF + i].char_code = HOGPRH_CHAR_REPORT + i;
                hids_char_desc[HOGPRH_DESC_REPORT_REF + i].req_flag  = ATT_OPTIONAL;
                hids_char_desc[HOGPRH_DESC_REPORT_REF + i].uuid      = ATT_DESC_REPORT_REF;
                // Report Client Config
                hids_char_desc[HOGPRH_DESC_REPORT_CFG + i].char_code = HOGPRH_CHAR_REPORT + i;
                hids_char_desc[HOGPRH_DESC_REPORT_CFG + i].req_flag  = ATT_OPTIONAL;
                hids_char_desc[HOGPRH_DESC_REPORT_CFG + i].uuid      = ATT_DESC_CLIENT_CHAR_CFG;
            }

            // 3. Retrieve HID characteristics
            prf_extract_svc_info(ind, HOGPRH_CHAR_MAX, &hids_char[0],
                    &(hogprh_env->env[conidx]->hids[hid_idx].chars[0]),
                    HOGPRH_DESC_MAX, &hids_char_desc[0],
                    &(hogprh_env->env[conidx]->hids[hid_idx].descs[0]));

            // 4. Store service range
            hogprh_env->env[conidx]->hids[hid_idx].svc.shdl = ind->start_hdl;
            hogprh_env->env[conidx]->hids[hid_idx].svc.ehdl = ind->end_hdl;

            // 5. Search for an included service
            for (fnd_att=0; fnd_att< (ind->end_hdl - ind->start_hdl); fnd_att++)
            {
                if(ind->info[fnd_att].att_type == GATTC_SDP_INC_SVC)
                {
                    hogprh_env->env[conidx]->hids[hid_idx].incl_svc.handle    = ind->start_hdl + fnd_att + 1;
                    hogprh_env->env[conidx]->hids[hid_idx].incl_svc.start_hdl = ind->info[fnd_att].inc_svc.start_hdl;
                    hogprh_env->env[conidx]->hids[hid_idx].incl_svc.end_hdl   = ind->info[fnd_att].inc_svc.end_hdl;
                    hogprh_env->env[conidx]->hids[hid_idx].incl_svc.uuid_len  = ind->info[fnd_att].inc_svc.uuid_len;
                    memcpy(hogprh_env->env[conidx]->hids[hid_idx].incl_svc.uuid, ind->info[fnd_att].inc_svc.uuid,
                            ind->info[fnd_att].inc_svc.uuid_len);
                }
                if((ind->info[fnd_att].att_type == GATTC_SDP_ATT_VAL)
                        && (attm_uuid16_comp((uint8_t*)ind->info[fnd_att].att.uuid, ind->info[fnd_att].att.uuid_len, ATT_CHAR_REPORT)))
                {
                    hogprh_env->env[conidx]->hids[hid_idx].report_nb++;
                }
            }

            hogprh_env->env[conidx]->hids_nb++;
        }
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_READ_INFO_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hogprh_read_info_req_handler(ke_msg_id_t const msgid, struct hogprh_read_info_req const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == HOGPRH_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);
        ASSERT_INFO(hogprh_env != NULL, dest_id, src_id);
        // environment variable not ready
        if(hogprh_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        // check parameter range
        else if((param->hid_idx > hogprh_env->env[conidx]->hids_nb) || (param->report_idx >= HOGPRH_NB_REPORT_INST_MAX))
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
                /// Protocol Mode
                case HOGPRH_PROTO_MODE:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].chars[HOGPRH_CHAR_PROTOCOL_MODE].val_hdl;
                }break;
                /// Report Map
                case HOGPRH_REPORT_MAP:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].chars[HOGPRH_CHAR_REPORT_MAP].val_hdl;
                }break;
                /// Report Map Char. External Report Reference Descriptor
                case HOGPRH_REPORT_MAP_EXT_REP_REF:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].descs[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].desc_hdl;
                }break;
                /// HID Information
                case HOGPRH_HID_INFO:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].chars[HOGPRH_CHAR_HID_INFO].val_hdl;
                }break;
                /// Report
                case HOGPRH_REPORT:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].chars[HOGPRH_CHAR_REPORT + param->report_idx].val_hdl;
                }break;
                /// Report Char. Report Reference
                case HOGPRH_REPORT_REF:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].descs[HOGPRH_DESC_REPORT_REF + param->report_idx].desc_hdl;
                }break;
                /// Report Notification config
                case HOGPRH_REPORT_NTF_CFG:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].descs[HOGPRH_DESC_REPORT_CFG + param->report_idx].desc_hdl;
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
                prf_read_char_send(&(hogprh_env->prf_env), conidx,
                        hogprh_env->env[conidx]->hids[param->hid_idx].svc.shdl,
                        hogprh_env->env[conidx]->hids[param->hid_idx].svc.ehdl,  handle);

                // store context of request and go into busy state
                hogprh_env->env[conidx]->operation = ke_param2msg(param);
                ke_state_set(dest_id, HOGPRH_BUSY);
                msg_status = KE_MSG_NO_FREE;
            }
        }
    }
    // process message later
    else if (state == HOGPRH_BUSY)
    {
        status = GAP_ERR_NO_ERROR;
        msg_status = KE_MSG_SAVED;
    }


    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct hogprh_read_info_rsp * rsp = KE_MSG_ALLOC(HOGPRH_READ_INFO_RSP,
                src_id, dest_id, hogprh_read_info_rsp);
        // set error status
        rsp->status = status;
        rsp->info   = param->info;
        rsp->hid_idx   = param->hid_idx;

        ke_msg_send(rsp);
    }

    return (msg_status);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPRH_WRITE_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hogprh_write_req_handler(ke_msg_id_t const msgid, struct hogprh_write_req *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(state == HOGPRH_IDLE)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        // Get the address of the environment
        struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);
        ASSERT_INFO(hogprh_env != NULL, dest_id, src_id);
        // environment variable not ready
        if(hogprh_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        // check parameter range
        else if((param->hid_idx > hogprh_env->env[conidx]->hids_nb) || (param->report_idx >= HOGPRH_NB_REPORT_INST_MAX)
                || (param->info > HOGPRH_INFO_MAX)
                || ((param->info == HOGPRH_REPORT_NTF_CFG) && (param->data.report_cfg > PRF_CLI_START_NTF)))
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
                // Protocol Mode
                case HOGPRH_PROTO_MODE:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].chars[HOGPRH_CHAR_PROTOCOL_MODE].val_hdl;
                    prf_gatt_write(&hogprh_env->prf_env, conidx, handle, (uint8_t*)&param->data.proto_mode, 1, GATTC_WRITE_NO_RESPONSE);
                }break;
                // HID Control Point
                case HOGPRH_HID_CTNL_PT:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].chars[HOGPRH_CHAR_HID_CTNL_PT].val_hdl;
                    prf_gatt_write(&hogprh_env->prf_env, conidx, handle, (uint8_t*)&param->data.hid_ctnl_pt, 1, GATTC_WRITE_NO_RESPONSE);
                }break;
                // Report Char. Report Reference
                case HOGPRH_REPORT:
                {
                    // Get MTU to select between GATT_WRITE_LONG_CHAR and GATT_WRITE_CHAR
                    if (param->data.report.length > (gattc_get_mtu(conidx) - 3))
                    {
                        param->wr_cmd = false;
                    }

                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].chars[HOGPRH_CHAR_REPORT + param->report_idx].val_hdl;
                    prf_gatt_write(&hogprh_env->prf_env, conidx, handle, (uint8_t*)param->data.report.value ,
                            param->data.report.length, (param->wr_cmd ? GATTC_WRITE_NO_RESPONSE : GATTC_WRITE));
                }break;
                // Report Notification config
                case HOGPRH_REPORT_NTF_CFG:
                {
                    handle = hogprh_env->env[conidx]->hids[param->hid_idx].descs[HOGPRH_DESC_REPORT_CFG + param->report_idx].desc_hdl;
                    prf_gatt_write_ntf_ind(&hogprh_env->prf_env, conidx, handle, param->data.report_cfg);
                }break;
                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                }break;
            }

            if(handle != ATT_INVALID_HANDLE)
            {
                status = GAP_ERR_NO_ERROR;

                // store context of request and go into busy state
                hogprh_env->env[conidx]->operation = ke_param2msg(param);
                ke_state_set(dest_id, HOGPRH_BUSY);
                msg_status = KE_MSG_NO_FREE;
            }
        }
    }
    // process message later
    else if (state == HOGPRH_BUSY)
    {
        status = GAP_ERR_NO_ERROR;
        msg_status = KE_MSG_SAVED;
    }


    // request cannot be performed
    if(status != GAP_ERR_NO_ERROR)
    {
        struct hogprh_write_rsp * rsp = KE_MSG_ALLOC(HOGPRH_WRITE_RSP,
                src_id, dest_id, hogprh_write_rsp);
        // set error status
        rsp->status = status;
        rsp->info   = param->info;
        rsp->hid_idx = param->hid_idx;

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
    struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);

    // sanity check
    if((state == HOGPRH_BUSY) && (hogprh_env->env[conidx] != NULL)
            && (hogprh_env->env[conidx]->operation != NULL))
    {
        switch(hogprh_env->env[conidx]->operation->id)
        {
            case HOGPRH_ENABLE_REQ:
            {
                uint8_t status = param->status;

                if (param->status == ATT_ERR_NO_ERROR)
                {
                    // check characteristic validity
                    if(hogprh_env->env[conidx]->hids_nb > 0)
                    {
                        uint8_t report_idx;
                        uint8_t hid_idx;
                        for (hid_idx = 0 ; (hid_idx < co_min(hogprh_env->env[conidx]->hids_nb, HOGPRH_NB_HIDS_INST_MAX))
                                        && (status == GAP_ERR_NO_ERROR) ; hid_idx++)
                        {

                            struct prf_char_def hids_char[HOGPRH_CHAR_MAX];
                            struct prf_char_desc_def hids_char_desc[HOGPRH_DESC_MAX];

                            // 1. Create characteristic reference
                            memcpy(hids_char, hogprh_hids_char, sizeof(struct prf_char_def) * HOGPRH_CHAR_REPORT);
                            for(report_idx = HOGPRH_CHAR_REPORT ; report_idx < HOGPRH_CHAR_MAX ; report_idx++)
                            {
                                hids_char[report_idx] = hogprh_hids_char[HOGPRH_CHAR_REPORT];
                                hids_char[report_idx].req_flag = ATT_OPTIONAL;
                            }

                            // 2. create descriptor reference
                            // Report Map Char. External Report Reference Descriptor
                            hids_char_desc[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].char_code = HOGPRH_CHAR_REPORT_MAP;
                            hids_char_desc[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].req_flag  = ATT_OPTIONAL;
                            hids_char_desc[HOGPRH_DESC_REPORT_MAP_EXT_REP_REF].uuid      = ATT_DESC_EXT_REPORT_REF;

                            for(report_idx = 0 ; report_idx < HOGPRH_NB_REPORT_INST_MAX ; report_idx++)
                            {
                                // Report Char. Report Reference
                                hids_char_desc[HOGPRH_DESC_REPORT_REF + report_idx].char_code = HOGPRH_CHAR_REPORT + report_idx;
                                hids_char_desc[HOGPRH_DESC_REPORT_REF + report_idx].req_flag  = ATT_OPTIONAL;
                                hids_char_desc[HOGPRH_DESC_REPORT_REF + report_idx].uuid      = ATT_DESC_REPORT_REF;
                                // Report Client Config
                                hids_char_desc[HOGPRH_DESC_REPORT_CFG + report_idx].char_code = HOGPRH_CHAR_REPORT + report_idx;
                                hids_char_desc[HOGPRH_DESC_REPORT_CFG + report_idx].req_flag  = ATT_OPTIONAL;
                                hids_char_desc[HOGPRH_DESC_REPORT_CFG + report_idx].uuid      = ATT_DESC_CLIENT_CHAR_CFG;
                            }

                            // 3. Check Characteristic validity
                            status = prf_check_svc_char_validity(HOGPRH_CHAR_MAX, hogprh_env->env[conidx]->hids[hid_idx].chars,
                                    hids_char);

                            // 4. check descriptor validity
                            if(status == GAP_ERR_NO_ERROR)
                            {
                                status = prf_check_svc_char_desc_validity(HOGPRH_DESC_MAX,
                                        hogprh_env->env[conidx]->hids[hid_idx].descs, hids_char_desc,
                                        hogprh_env->env[conidx]->hids[hid_idx].chars);
                            }
                        }
                    }
                    // no services found
                    else
                    {
                        status = PRF_ERR_STOP_DISC_CHAR_MISSING;
                    }

                }
                hogprh_enable_rsp_send(hogprh_env, conidx, status);
            }break;
            case HOGPRH_READ_INFO_REQ:
            {
                // do not send anything if it's a local request
                if(hogprh_env->env[conidx]->operation->src_id != dest_id)
                {
                    struct hogprh_read_info_req* req = (struct hogprh_read_info_req*) ke_msg2param(hogprh_env->env[conidx]->operation);
                    struct hogprh_read_info_rsp * rsp = KE_MSG_ALLOC(HOGPRH_READ_INFO_RSP,
                            prf_dst_task_get(&(hogprh_env->prf_env), conidx), dest_id, hogprh_read_info_rsp);
                    // set error status
                    rsp->status     = param->status;
                    rsp->hid_idx    = req->hid_idx;
                    rsp->info       = req->info;
                    rsp->report_idx = req->report_idx;

                    ke_msg_send(rsp);
                }

            }break;
            case HOGPRH_WRITE_REQ:
            {
                struct hogprh_write_req* req = (struct hogprh_write_req*) ke_msg2param(hogprh_env->env[conidx]->operation);

                struct hogprh_write_rsp * rsp = KE_MSG_ALLOC(HOGPRH_WRITE_RSP,
                        prf_dst_task_get(&(hogprh_env->prf_env), conidx), dest_id, hogprh_write_rsp);
                // set error status
                rsp->status     = param->status;
                rsp->hid_idx     = req->hid_idx;
                rsp->info       = req->info;
                rsp->report_idx = req->report_idx;

                ke_msg_send(rsp);
            }break;
            default:
            {
                // Not Expected at all
                ASSERT_ERR(0);
            }break;
        }

        // operation is over - go back to idle state
        ke_free(hogprh_env->env[conidx]->operation);
        hogprh_env->env[conidx]->operation = NULL;
        ke_state_set(dest_id, HOGPRH_IDLE);
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

    if(state == HOGPRH_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);
        struct hogprh_read_info_req* req = (struct hogprh_read_info_req*) ke_msg2param(hogprh_env->env[conidx]->operation);

        ASSERT_INFO(hogprh_env != NULL, dest_id, src_id);
        ASSERT_INFO(hogprh_env->env[conidx] != NULL, dest_id, src_id);

        // check if read is requested locally due to a received notification or b
        if(hogprh_env->env[conidx]->operation->src_id == dest_id)
        {
            // send report indication
            struct hogprh_report_ind* ind = KE_MSG_ALLOC_DYN(HOGPRH_REPORT_IND,
                    prf_dst_task_get(&(hogprh_env->prf_env), conidx), dest_id,
                    hogprh_report_ind, param->length);

            ind->hid_idx       = req->hid_idx;
            ind->report_idx    = req->report_idx;
            ind->report.length = param->length;
            memcpy(ind->report.value, param->value, param->length);

            ke_msg_send(ind);
        }
        // or by the HID Client application
        else
        {
            struct hogprh_read_info_rsp * rsp = KE_MSG_ALLOC_DYN(HOGPRH_READ_INFO_RSP,
                prf_dst_task_get(&(hogprh_env->prf_env),conidx), dest_id, hogprh_read_info_rsp, param->length);

            // set error status
            rsp->status     = GAP_ERR_NO_ERROR;
            rsp->hid_idx    = req->hid_idx;
            rsp->info       = req->info;
            rsp->report_idx = req->report_idx;

            switch(req->info)
            {
                /// Protocol Mode
                case HOGPRH_PROTO_MODE:
                {
                    rsp->data.proto_mode = param->value[0];
                }break;
                /// Report Map
                case HOGPRH_REPORT_MAP:
                {
                    rsp->data.report_map.length = param->length;
                    memcpy(rsp->data.report_map.value, param->value, param->length);
                }break;
                /// Report Map Char. External Report Reference Descriptor
                case HOGPRH_REPORT_MAP_EXT_REP_REF:
                {
                    rsp->data.report_map_ref.uuid_len = param->length;
                    memcpy(rsp->data.report_map_ref.uuid, param->value, co_min(param->length, ATT_UUID_128_LEN));
                }break;
                /// HID Information
                case HOGPRH_HID_INFO:
                {
                    rsp->data.hid_info.bcdHID       = co_read16p(param->value);
                    rsp->data.hid_info.bCountryCode = param->value[2];
                    rsp->data.hid_info.flags        = param->value[3];
                }break;
                /// Report
                case HOGPRH_REPORT:
                {
                    rsp->data.report.length = param->length;
                    memcpy(rsp->data.report.value, param->value, param->length);
                }break;
                /// Report Char. Report Reference
                case HOGPRH_REPORT_REF:
                {
                    rsp->data.report_ref.id   = param->value[0];
                    rsp->data.report_ref.type = param->value[1];
                }break;
                /// Report Notification config
                case HOGPRH_REPORT_NTF_CFG:
                {
                    rsp->data.report_cfg = co_read16p(param->value);
                }break;
                default:
                {
                    ASSERT_ERR(0);
                }break;
            }

            // send response
            ke_msg_send(rsp);
        }

        // operation is over - go back to idle state
        ke_free(hogprh_env->env[conidx]->operation);
        hogprh_env->env[conidx]->operation = NULL;
        ke_state_set(dest_id, HOGPRH_IDLE);
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

    if(state != HOGPRH_FREE)
    {
        // HID Instance
        uint8_t hid_nb;
        uint8_t report_idx;
        uint8_t conidx = KE_IDX_GET(src_id);
        bool finished = false;
        // Get the address of the environment
        struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);

        // BOOT Report - HID instance is unknown.
        for (hid_nb = 0; (hid_nb < hogprh_env->env[conidx]->hids_nb) && !finished; hid_nb++)
        {
            for (report_idx = 0; (report_idx < HOGPRH_NB_REPORT_INST_MAX) && !finished; report_idx++)
            {
                if (param->handle == hogprh_env->env[conidx]->hids[hid_nb].chars[HOGPRH_CHAR_REPORT + report_idx].val_hdl)
                {

                    // Check if size of the data is lower than [ATT_MTU-3]
                    if (param->length < (gattc_get_mtu(conidx) - 3))
                    {
                        // send report indication
                        struct hogprh_report_ind* ind = KE_MSG_ALLOC_DYN(HOGPRH_REPORT_IND,
                                prf_dst_task_get(&(hogprh_env->prf_env), conidx), dest_id,
                                hogprh_report_ind, param->length);

                        ind->hid_idx       = hid_nb;
                        ind->report_idx    = report_idx;
                        ind->report.length = param->length;
                        memcpy(ind->report.value, param->value, param->length);

                        ke_msg_send(ind);
                    }
                    else
                    {
                        // data is maybe incomplete so perform an attribute read
                        struct hogprh_read_info_req* req = KE_MSG_ALLOC(HOGPRH_READ_INFO_REQ,
                                dest_id, dest_id, hogprh_read_info_req);

                        req->info          = HOGPRH_REPORT;
                        req->hid_idx       = hid_nb;
                        req->report_idx    = report_idx;

                        ke_msg_send(req);
                    }

                    finished = true;
                }
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
KE_MSG_HANDLER_TAB(hogprh)
{
    {HOGPRH_ENABLE_REQ,                     (ke_msg_func_t)hogprh_enable_req_handler},
    {GATTC_SDP_SVC_IND,                     (ke_msg_func_t)gattc_sdp_svc_ind_handler},

    {HOGPRH_READ_INFO_REQ,                  (ke_msg_func_t)hogprh_read_info_req_handler},
    {HOGPRH_WRITE_REQ,                      (ke_msg_func_t)hogprh_write_req_handler},

    {GATTC_READ_IND,                        (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                       (ke_msg_func_t)gattc_event_ind_handler},

    {GATTC_CMP_EVT,                         (ke_msg_func_t)gattc_cmp_evt_handler},
};

void hogprh_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);

    task_desc->msg_handler_tab = hogprh_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(hogprh_msg_handler_tab);
    task_desc->state           = hogprh_env->state;
    task_desc->idx_max         = HOGPRH_IDX_MAX;
}

#endif /* (BLE_HID_REPORT_HOST) */

/// @} HOGPRHTASK
