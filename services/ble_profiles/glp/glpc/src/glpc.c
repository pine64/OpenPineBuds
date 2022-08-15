/**
 ****************************************************************************************
 * @addtogroup GLPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_GL_COLLECTOR)
#include "glpc.h"
#include "glpc_task.h"
#include "gap.h"

#include "ke_mem.h"
#include "co_utils.h"


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the GLPC module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 *
 * @param[out]    env        Collector or Service allocated environment data.
 * @param[in|out] start_hdl  Service start handle (0 - dynamically allocated), only applies for services.
 * @param[in]     app_task   Application task number.
 * @param[in]     sec_lvl    Security level (AUTH, EKS and MI field of @see enum attm_value_perm_mask)
 * @param[in]     param      Configuration parameters of profile collector or service (32 bits aligned)
 *
 * @return status code to know if profile initialization succeed or not.
 ****************************************************************************************
 */
static uint8_t glpc_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct glpc_env_tag* glpc_env =
            (struct glpc_env_tag* ) ke_malloc(sizeof(struct glpc_env_tag), KE_MEM_ATT_DB);

    // allocate GLPC required environment variable
    env->env = (prf_env_t*) glpc_env;

    glpc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    glpc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_GLPC;
    glpc_task_init(&(env->desc));

    for(idx = 0; idx < GLPC_IDX_MAX ; idx++)
    {
        glpc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), GLPC_FREE);
    }

    return GAP_ERR_NO_ERROR;
}

/**
 ****************************************************************************************
 * @brief Destruction of the GLPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void glpc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct glpc_env_tag* glpc_env = (struct glpc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < GLPC_IDX_MAX ; idx++)
    {
        if(glpc_env->env[idx] != NULL)
        {
            ke_free(glpc_env->env[idx]);
        }
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(glpc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void glpc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put GLP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), GLPC_IDLE);
}

/**
 ****************************************************************************************
 * @brief Handles Disconnection
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void glpc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct glpc_env_tag* glpc_env = (struct glpc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(glpc_env->env[conidx] != NULL)
    {
        ke_free(glpc_env->env[conidx]);
        glpc_env->env[conidx] = NULL;
    }

    /* Put GLP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), GLPC_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// GLPC Task interface required by profile manager
const struct prf_task_cbs glpc_itf =
{
        glpc_init,
        glpc_destroy,
        glpc_create,
        glpc_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* glpc_prf_itf_get(void)
{
   return &glpc_itf;
}


void glpc_enable_rsp_send(struct glpc_env_tag *glpc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on GLPS
    struct glpc_enable_rsp * rsp = KE_MSG_ALLOC(GLPC_ENABLE_RSP,
                                                prf_dst_task_get(&(glpc_env->prf_env), conidx),
                                                prf_src_task_get(&(glpc_env->prf_env), conidx),
                                                glpc_enable_rsp);
    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->gls = glpc_env->env[conidx]->gls;
        // Register GLPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(glpc_env->prf_env), conidx, &(glpc_env->env[conidx]->gls.svc));
        // Go to IDLE state
        ke_state_set(prf_src_task_get(&(glpc_env->prf_env), conidx), GLPC_IDLE);
    }

    ke_msg_send(rsp);
}

uint8_t glpc_unpack_meas_value(uint8_t *packed_meas, struct glp_meas* meas_val,
                               uint16_t *seq_num)
{
    uint8_t cursor = 0;

    // Flags
    meas_val->flags = packed_meas[cursor];
    cursor += 1;

    // Sequence Number
    *seq_num = co_read16p(packed_meas + cursor);
    cursor += 2;

    // Base Time
    cursor += prf_unpack_date_time(packed_meas + cursor, &(meas_val->base_time));

    //Time Offset
    if((meas_val->flags & GLP_MEAS_TIME_OFF_PRES) != 0)
    {
        meas_val->time_offset = co_read16p(packed_meas + cursor);
        cursor += 2;
    }

    // Glucose Concentration, type and location
    if((meas_val->flags & GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES) != 0)
    {
        meas_val->concentration = co_read16p(packed_meas + cursor);
        cursor += 2;

        /* type and location are 2 nibble values */
        meas_val->location = packed_meas[cursor] >> 4;
        meas_val->type     = packed_meas[cursor] & 0xF;

        cursor += 1;
    }

    // Sensor Status Annunciation
    if((meas_val->flags & GLP_MEAS_SENS_STAT_ANNUN_PRES) != 0)
    {
        meas_val->status = co_read16p(packed_meas + cursor);
        cursor += 2;
    }

    return cursor;}


uint8_t glpc_unpack_meas_ctx_value(uint8_t *packed_meas_ctx,
                                   struct glp_meas_ctx* meas_ctx_val,
                                   uint16_t* seq_num)
{
    uint8_t cursor = 0;
    // Flags
    meas_ctx_val->flags = packed_meas_ctx[cursor];
    cursor += 1;

    // Sequence Number
    *seq_num = co_read16p(packed_meas_ctx + cursor);
    cursor += 2;

    // Extended Flags
    if((meas_ctx_val->flags & GLP_CTX_EXTD_F_PRES) != 0)
    {
        meas_ctx_val->ext_flags = packed_meas_ctx[cursor];
        cursor += 1;
    }

    // Carbohydrate ID And Carbohydrate Present
    if((meas_ctx_val->flags & GLP_CTX_CRBH_ID_AND_CRBH_PRES) != 0)
    {
        // Carbohydrate ID
        meas_ctx_val->carbo_id = packed_meas_ctx[cursor];
        cursor += 1;
        // Carbohydrate Present
        meas_ctx_val->carbo_val = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;
    }

    // Meal Present
    if((meas_ctx_val->flags & GLP_CTX_MEAL_PRES) != 0)
    {
        meas_ctx_val->meal = packed_meas_ctx[cursor];
        cursor += 1;
    }

    // Tester-Health Present
    if((meas_ctx_val->flags & GLP_CTX_TESTER_HEALTH_PRES) != 0)
    {
        // Tester and Health are 2 nibble values
        meas_ctx_val->health = packed_meas_ctx[cursor] >> 4;
        meas_ctx_val->tester = packed_meas_ctx[cursor] & 0xF;
        cursor += 1;
    }

    // Exercise Duration & Exercise Intensity Present
    if((meas_ctx_val->flags & GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES) != 0)
    {
        // Exercise Duration
        meas_ctx_val->exercise_dur = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;

        // Exercise Intensity
        meas_ctx_val->exercise_intens = packed_meas_ctx[cursor];
        cursor += 1;
    }

    // Medication ID And Medication Present
    if((meas_ctx_val->flags & GLP_CTX_MEDIC_ID_AND_MEDIC_PRES) != 0)
    {
        // Medication ID
        meas_ctx_val->med_id = packed_meas_ctx[cursor];
        cursor += 1;

        // Medication Present
        meas_ctx_val->med_val = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;
    }

    // HbA1c Present
    if((meas_ctx_val->flags & GLP_CTX_HBA1C_PRES) != 0)
    {
        // HbA1c
        meas_ctx_val->hba1c_val = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;
    }

    return cursor;
}

uint8_t glpc_pack_racp_req(uint8_t *packed_val,
                           const struct glp_racp_req* racp_req)
{
    uint8_t cursor = 0;

    // command op code
    packed_val[cursor] = racp_req->op_code;
    cursor++;

    // operator of the function
    packed_val[cursor] = racp_req->filter.operator;
    cursor++;

    // Abort operation don't require any other parameter
    if(racp_req->op_code == GLP_REQ_ABORT_OP)
    {
        return cursor;
    }

    // check if request requires operand (filter)
    if((racp_req->filter.operator >= GLP_OP_LT_OR_EQ)
            && (racp_req->filter.operator <= GLP_OP_WITHIN_RANGE_OF))
    {

        // command filter type
        packed_val[cursor] = racp_req->filter.filter_type;
        cursor++;

        // filter uses sequence number
        if(racp_req->filter.filter_type == GLP_FILTER_SEQ_NUMBER)
        {
            // minimum value
            if((racp_req->filter.operator == GLP_OP_GT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // minimum value
                co_write16p(packed_val + cursor,racp_req->filter.val.seq_num.min);
                cursor +=2;
            }

            // maximum value
            if((racp_req->filter.operator == GLP_OP_LT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // maximum value
                co_write16p(packed_val + cursor,racp_req->filter.val.seq_num.max);
                cursor +=2;
            }
        }
        // filter uses user facing time
        else
        {
            // retrieve minimum value
            if((racp_req->filter.operator == GLP_OP_GT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                 // retrieve minimum facing time
                cursor += prf_pack_date_time((packed_val + cursor),
                        &(racp_req->filter.val.time.facetime_min));
            }

            // retrieve maximum value
            if((racp_req->filter.operator == GLP_OP_LT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // retrieve maximum facing time
                cursor += prf_pack_date_time((packed_val + cursor),
                        &(racp_req->filter.val.time.facetime_max));
            }
        }
    }

    return cursor;
}

uint8_t glpc_unpack_racp_rsp(uint8_t *packed_val,
                             struct glp_racp_rsp* racp_rsp)
{
    uint8_t cursor = 0;

    // response op code
    racp_rsp->op_code = packed_val[cursor];
    cursor++;

    // operator (null)
    cursor++;

    // number of records
    if(racp_rsp->op_code == GLP_REQ_NUM_OF_STRD_RECS_RSP)
    {
        racp_rsp->operand.num_of_record = co_read16p(packed_val + cursor);
        cursor += 2;
    }
    else
    {
        // requested opcode
        racp_rsp->operand.rsp.op_code_req = packed_val[cursor];
        cursor++;
        // command status
        racp_rsp->operand.rsp.status = packed_val[cursor];
        cursor++;
    }

    return cursor;
}

uint8_t glpc_validate_request(struct glpc_env_tag *glpc_env, uint8_t conidx, uint8_t char_code)
{
    uint8_t status = GAP_ERR_NO_ERROR;
    // check if feature val characteristic exists
    if(glpc_env->env[conidx]->gls.chars[char_code].val_hdl == ATT_INVALID_HANDLE)
    {
        status = PRF_ERR_INEXISTENT_HDL;
    }

    return (status);
}

#endif /* (BLE_GL_COLLECTOR) */

/// @} GLPC
