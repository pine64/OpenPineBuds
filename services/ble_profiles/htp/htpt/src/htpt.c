/**
 ****************************************************************************************
 * @addtogroup HTPT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HT_THERMOM)
#include "attm.h"
#include "htpt.h"
#include "htpt_task.h"
#include "co_utils.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * HTPT PROFILE ATTRIBUTES
 ****************************************************************************************
 */
/// Full HTS Database Description - Used to add attributes into the database
const struct attm_desc htpt_att_db[HTS_IDX_NB] =
{
    // Health Thermometer Service Declaration
    [HTS_IDX_SVC]                   =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // Temperature Measurement Characteristic Declaration
    [HTS_IDX_TEMP_MEAS_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Temperature Measurement Characteristic Value
    [HTS_IDX_TEMP_MEAS_VAL]         =   {ATT_CHAR_TEMPERATURE_MEAS, PERM(IND, ENABLE), PERM(RI, ENABLE), 0},
    // Temperature Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HTS_IDX_TEMP_MEAS_IND_CFG]     =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},

    // Temperature Type Characteristic Declaration
    [HTS_IDX_TEMP_TYPE_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Temperature Type Characteristic Value
    [HTS_IDX_TEMP_TYPE_VAL]         =   {ATT_CHAR_TEMPERATURE_TYPE, PERM(RD, ENABLE), PERM(RI, ENABLE), 0},

    // Intermediate Measurement Characteristic Declaration
    [HTS_IDX_INTERM_TEMP_CHAR]      =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Intermediate Measurement Characteristic Value
    [HTS_IDX_INTERM_TEMP_VAL]       =   {ATT_CHAR_INTERMED_TEMPERATURE, PERM(NTF, ENABLE), PERM(RI, ENABLE), 0},
    // Intermediate Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HTS_IDX_INTERM_TEMP_CFG]       =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},

    // Measurement Interval Characteristic Declaration
    [HTS_IDX_MEAS_INTV_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Measurement Interval Characteristic Value
    [HTS_IDX_MEAS_INTV_VAL]         =   {ATT_CHAR_MEAS_INTERVAL, PERM(RD, ENABLE), PERM(RI, ENABLE), HTPT_MEAS_INTV_MAX_LEN},
    // Measurement Interval Characteristic - Client Characteristic Configuration Descriptor
    [HTS_IDX_MEAS_INTV_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
    // Measurement Interval Characteristic - Valid Range Descriptor
    [HTS_IDX_MEAS_INTV_VAL_RANGE]   =   {ATT_DESC_VALID_RANGE, PERM(RD, ENABLE), PERM(RI, ENABLE), 0},
};

static uint16_t htpt_compute_att_table(uint16_t features);

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static uint8_t htpt_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task,
                            uint8_t sec_lvl, struct htpt_db_cfg* params)
{
    // Service content flag
    uint16_t cfg_flag;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    BLE_PRF_HP_FUNC_ENTER();

    cfg_flag = htpt_compute_att_table(params->features);

    status = attm_svc_create_db(start_hdl, ATT_SVC_HEALTH_THERMOM, (uint8_t *)&cfg_flag,
               HTS_IDX_NB, NULL, env->task, &htpt_att_db[0],
              (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE) );

    if( status == ATT_ERR_NO_ERROR )
    {
        //-------------------- allocate memory required for the profile  ---------------------
        struct htpt_env_tag* htpt_env =
                (struct htpt_env_tag* ) ke_malloc(sizeof(struct htpt_env_tag), KE_MEM_ATT_DB);

        // allocate PROXR required environment variable
        env->env = (prf_env_t*) htpt_env;

        htpt_env->shdl     = *start_hdl;
        htpt_env->prf_env.app_task = app_task
                        | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        htpt_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_HTPT;
        htpt_task_init(&(env->desc));

        //Save features on the environment
        htpt_env->features      = params->features;
        htpt_env->meas_intv     = params->meas_intv;
        htpt_env->meas_intv_min = params->valid_range_min;
        htpt_env->meas_intv_max = params->valid_range_max;
        htpt_env->temp_type     = params->temp_type;
        htpt_env->operation     = NULL;
        memset(htpt_env->ntf_ind_cfg, 0 , sizeof(htpt_env->ntf_ind_cfg));

        // Update measurement interval permissions
        if (HTPT_IS_FEATURE_SUPPORTED(params->features, HTPT_MEAS_INTV_CHAR_SUP))
        {
            uint16_t perm = PERM(RD, ENABLE);

            //Check if Measurement Interval Char. supports indications
            if (HTPT_IS_FEATURE_SUPPORTED(params->features, HTPT_MEAS_INTV_IND_SUP))
            {
                perm |= PERM(IND, ENABLE);
            }

            //Check if Measurement Interval Char. is writable
            if (HTPT_IS_FEATURE_SUPPORTED(params->features, HTPT_MEAS_INTV_WR_SUP))
            {
                perm |= PERM(WP, UNAUTH)|PERM(WRITE_REQ, ENABLE);
            }

            attm_att_set_permission(HTPT_HANDLE(HTS_IDX_MEAS_INTV_VAL), perm, 0);
        }

        // service is ready, go into an Idle state
        ke_state_set(env->task, HTPT_IDLE);
    }

    BLE_PRF_HP_FUNC_LEAVE();

    return (status);
}

static void htpt_destroy(struct prf_task_env* env)
{
    struct htpt_env_tag* htpt_env = (struct htpt_env_tag*) env->env;

    BLE_PRF_HP_FUNC_ENTER();

    // free profile environment variables
    if(htpt_env->operation != NULL)
    {
        ke_free(htpt_env->operation);
    }

    env->env = NULL;
    ke_free(htpt_env);

    BLE_PRF_HP_FUNC_LEAVE();
}

static void htpt_create(struct prf_task_env* env, uint8_t conidx)
{
    BLE_PRF_HP_FUNC_ENTER();

    /* Clear configuration for this connection */
    struct htpt_env_tag* htpt_env = (struct htpt_env_tag*) env->env;
    htpt_env->ntf_ind_cfg[conidx] = 0;

    BLE_PRF_HP_FUNC_LEAVE();
}

static void htpt_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    BLE_PRF_HP_FUNC_ENTER();

    /* Clear configuration for this connection */
    struct htpt_env_tag* htpt_env = (struct htpt_env_tag*) env->env;
    htpt_env->ntf_ind_cfg[conidx] = 0;

    BLE_PRF_HP_FUNC_LEAVE();
}

/**
 ****************************************************************************************
 * @brief Compute a flag allowing to know attributes to add into the database
 *
 * @return a 16-bit flag whose each bit matches an attribute. If the bit is set to 1, the
 * attribute will be added into the database.
 ****************************************************************************************
 */
static uint16_t htpt_compute_att_table(uint16_t features)
{
    BLE_PRF_HP_FUNC_ENTER();

    //Temperature Measurement Characteristic is mandatory
    uint16_t att_table = HTPT_TEMP_MEAS_MASK;

    //Check if Temperature Type Char. is supported
    if (HTPT_IS_FEATURE_SUPPORTED(features, HTPT_TEMP_TYPE_CHAR_SUP))
    {
        att_table |= HTPT_TEMP_TYPE_MASK;
    }

    //Check if Intermediate Temperature Char. is supported
    if (HTPT_IS_FEATURE_SUPPORTED(features, HTPT_INTERM_TEMP_CHAR_SUP))
    {
        att_table |= HTPT_INTM_TEMP_MASK;
    }

    //Check if Measurement Interval Char. is supported
    if (HTPT_IS_FEATURE_SUPPORTED(features, HTPT_MEAS_INTV_CHAR_SUP))
    {
        att_table |= HTPT_MEAS_INTV_MASK;

        //Check if Measurement Interval Char. supports indications
        if (HTPT_IS_FEATURE_SUPPORTED(features, HTPT_MEAS_INTV_IND_SUP))
        {
            att_table |= HTPT_MEAS_INTV_CCC_MASK;
        }

        //Check if Measurement Interval Char. is writable
        if (HTPT_IS_FEATURE_SUPPORTED(features, HTPT_MEAS_INTV_WR_SUP))
        {
            att_table |= HTPT_MEAS_INTV_VALID_RGE_MASK;
        }
    }

    BLE_PRF_HP_FUNC_LEAVE();

    return att_table;
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// HTPT Task interface required by profile manager
const struct prf_task_cbs htpt_itf =
{
    (prf_init_fnct) htpt_init,
    htpt_destroy,
    htpt_create,
    htpt_cleanup,
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

uint8_t htpt_get_valid_rge_offset(uint16_t features)
{
    uint8_t offset = 0;

    if (HTPT_IS_FEATURE_SUPPORTED(features, HTPT_MEAS_INTV_WR_SUP))
    {
        offset += 1;

        if (HTPT_IS_FEATURE_SUPPORTED(features, HTPT_MEAS_INTV_IND_SUP))
        {
            offset += 1;
        }
    }

    return offset;
}

uint8_t htpt_pack_temp_value(uint8_t *packed_temp, struct htp_temp_meas temp_meas)
{
    uint8_t cursor = 0;

    *(packed_temp + cursor) = temp_meas.flags;
    cursor += 1;

    co_write32p(packed_temp + cursor, temp_meas.temp);
    cursor += 4;

    //Time Flag Set
    if ((temp_meas.flags & HTP_FLAG_TIME) == HTP_FLAG_TIME)
    {
        cursor += prf_pack_date_time(packed_temp + cursor, &(temp_meas.time_stamp));
    }

    //Type flag set
    if ((temp_meas.flags & HTP_FLAG_TYPE) == HTP_FLAG_TYPE)
    {
        *(packed_temp + cursor) = temp_meas.type;
        cursor += 1;
    }

    //Clear unused packet data
    if(cursor < HTPT_TEMP_MEAS_MAX_LEN)
    {
        memset(packed_temp + cursor, 0, (HTPT_TEMP_MEAS_MAX_LEN - cursor));
    }

    return cursor;
}

void htpt_exe_operation(void)
{
    BLE_PRF_HP_FUNC_ENTER();

    struct htpt_env_tag* htpt_env = PRF_ENV_GET(HTPT, htpt);

    ASSERT_ERR(htpt_env->operation != NULL);

    bool finished = true;

    while(htpt_env->operation->cursor < BLE_CONNECTION_MAX)
    {
        // check if this type of event is enabled
        if(((htpt_env->ntf_ind_cfg[htpt_env->operation->cursor] & htpt_env->operation->op) != 0)
            // and event not filtered on current connection
            && (htpt_env->operation->conidx != htpt_env->operation->cursor))
        {
            // trigger the event
            struct gattc_send_evt_cmd * evt = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                    KE_BUILD_ID(TASK_GATTC , htpt_env->operation->cursor), prf_src_task_get(&htpt_env->prf_env, 0),
                    gattc_send_evt_cmd, htpt_env->operation->length);

            evt->operation = (htpt_env->operation->op != HTPT_CFG_INTERM_MEAS_NTF) ? GATTC_INDICATE : GATTC_NOTIFY;
            evt->length    = htpt_env->operation->length;
            evt->handle    = htpt_env->operation->handle;
            memcpy(evt->value, htpt_env->operation->data, evt->length);

            ke_msg_send(evt);

            finished = false;
            htpt_env->operation->cursor++;
            break;
        }
        htpt_env->operation->cursor++;
    }


    // check if operation is finished
    if(finished)
    {
        // do not send response if operation has been locally requested
        if(htpt_env->operation->dest_id != prf_src_task_get(&htpt_env->prf_env, 0))
        {
            // send response to requester
            struct htpt_meas_intv_upd_rsp * rsp =
                    KE_MSG_ALLOC(((htpt_env->operation->op == HTPT_CFG_MEAS_INTV_IND) ? HTPT_MEAS_INTV_UPD_RSP : HTPT_TEMP_SEND_RSP),
                                 htpt_env->operation->dest_id, prf_src_task_get(&htpt_env->prf_env, 0),
                                 htpt_meas_intv_upd_rsp);
            rsp->status = GAP_ERR_NO_ERROR;
            ke_msg_send(rsp);
        }

        // free operation
        ke_free(htpt_env->operation);
        htpt_env->operation = NULL;
        // go back to idle state
        ke_state_set(prf_src_task_get(&(htpt_env->prf_env), 0), HTPT_IDLE);
    }

    BLE_PRF_HP_FUNC_LEAVE();
}


uint8_t htpt_update_ntf_ind_cfg(uint8_t conidx, uint8_t cfg, uint16_t valid_val, uint16_t value)
{
    struct htpt_env_tag* htpt_env = PRF_ENV_GET(HTPT, htpt);
    uint8_t status = GAP_ERR_NO_ERROR;

    BLE_PRF_HP_FUNC_ENTER();

    if((value != valid_val) && (value != PRF_CLI_STOP_NTFIND))
    {
        status = PRF_APP_ERROR;

    }
    else if (value == valid_val)
    {
        htpt_env->ntf_ind_cfg[conidx] |= cfg;
    }
    else
    {
        htpt_env->ntf_ind_cfg[conidx] &= ~cfg;
    }

    if(status == GAP_ERR_NO_ERROR)
    {
        // inform application that notification/indication configuration has changed
        struct htpt_cfg_indntf_ind * ind = KE_MSG_ALLOC(HTPT_CFG_INDNTF_IND,
                prf_dst_task_get(&htpt_env->prf_env, conidx), prf_src_task_get(&htpt_env->prf_env, conidx),
                htpt_cfg_indntf_ind);
        ind->conidx      = conidx;
        ind->ntf_ind_cfg = htpt_env->ntf_ind_cfg[conidx];
        ke_msg_send(ind);
    }

    BLE_PRF_HP_FUNC_LEAVE();

    return (status);
}


const struct prf_task_cbs* htpt_prf_itf_get(void)
{
   return &htpt_itf;
}


uint16_t htpt_att_hdl_get(struct htpt_env_tag* htpt_env, uint8_t att_idx)
{
    uint16_t handle = htpt_env->shdl;

    do
    {
        // Mandatory attribute handle
        if(att_idx > HTS_IDX_TEMP_MEAS_IND_CFG)
        {
            handle += HTPT_TEMP_MEAS_ATT_NB;
        }
        else
        {
            handle += att_idx;
            break;
        }

        // Temperature Type
        if((HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_TEMP_TYPE_CHAR_SUP)) && (att_idx > HTS_IDX_TEMP_TYPE_VAL))
        {
            handle += HTPT_TEMP_TYPE_ATT_NB;
        }
        else if(!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_TEMP_TYPE_CHAR_SUP))
        {
            handle = ATT_INVALID_HANDLE;
            break;
        }
        else
        {
            handle += att_idx - HTS_IDX_TEMP_TYPE_CHAR;
            break;
        }

        // Intermediate Temperature Measurement
        if((HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_INTERM_TEMP_CHAR_SUP)) && (att_idx > HTS_IDX_INTERM_TEMP_CFG))
        {
            handle += HTPT_INTERM_MEAS_ATT_NB;
        }
        else if(!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_INTERM_TEMP_CHAR_SUP))
        {
            handle = ATT_INVALID_HANDLE;
            break;
        }
        else
        {
            handle += att_idx - HTS_IDX_INTERM_TEMP_CHAR;
            break;
        }

        // Measurement Interval
        if(!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_CHAR_SUP) || (att_idx >= HTS_IDX_NB))
        {
            handle = ATT_INVALID_HANDLE;
            break;
        }

        if(att_idx <= HTS_IDX_MEAS_INTV_VAL)
        {
            handle += att_idx - HTS_IDX_MEAS_INTV_CHAR;
            break;
        }
        else
        {
            handle += HTPT_MEAS_INTV_ATT_NB;
        }

        // Measurement Interval Indication
        if(att_idx == HTS_IDX_MEAS_INTV_CFG)
        {
            if(!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_IND_SUP))
            {
                handle = ATT_INVALID_HANDLE;
                break;
            }
        }
        // Measurement Interval Write permission
        else if(HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_WR_SUP))
        {
            handle += HTPT_MEAS_INTV_CCC_ATT_NB;

            if(!HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_WR_SUP))
            {
                handle = ATT_INVALID_HANDLE;
                break;
            }
        }
    } while (0);

    return handle;
}

uint8_t htpt_att_idx_get(struct htpt_env_tag* htpt_env, uint16_t handle)
{
    uint16_t handle_ref = htpt_env->shdl;
    uint8_t att_idx = ATT_INVALID_IDX;

    do
    {
        // not valid hande
        if(handle < handle_ref)
        {
            break;
        }

        // Mandatory attribute handle
        handle_ref += HTPT_TEMP_MEAS_ATT_NB;

        if(handle < handle_ref)
        {
            att_idx = HTS_IDX_TEMP_TYPE_CHAR - (handle_ref - handle);
            break;
        }

        // Temperature Type
        if(HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_TEMP_TYPE_CHAR_SUP))
        {
            handle_ref += HTPT_TEMP_TYPE_ATT_NB;

            if(handle < handle_ref)
            {
                att_idx = HTS_IDX_INTERM_TEMP_CHAR - (handle_ref - handle);
                break;
            }
        }

        // Intermediate Temperature Measurement
        if(HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_INTERM_TEMP_CHAR_SUP))
        {
            handle_ref += HTPT_INTERM_MEAS_ATT_NB;

            if(handle < handle_ref)
            {
                att_idx = HTS_IDX_MEAS_INTV_CHAR - (handle_ref - handle);
                break;
            }
        }

        // Measurement Interval
        if(HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_CHAR_SUP))
        {
            handle_ref += HTPT_MEAS_INTV_ATT_NB;

            if(handle < handle_ref)
            {
                att_idx = HTS_IDX_MEAS_INTV_CFG - (handle_ref - handle);
                break;
            }

            if(HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_IND_SUP))
            {
                if(handle == handle_ref)
                {
                    att_idx = HTS_IDX_MEAS_INTV_CFG;
                    break;
                }

                handle_ref += HTPT_MEAS_INTV_CCC_ATT_NB;
            }

            if(HTPT_IS_FEATURE_SUPPORTED(htpt_env->features, HTPT_MEAS_INTV_WR_SUP))
            {
                if(handle == handle_ref)
                {
                    att_idx = HTS_IDX_MEAS_INTV_VAL_RANGE;
                    break;
                }
            }
        }
    } while (0);

    return att_idx;
}



#endif //BLE_HT_THERMOM

/// @} HTPT
