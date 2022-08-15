/**
 ****************************************************************************************
 * @addtogroup BLPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BP_SENSOR)
#include "gap.h"
#include "gattc_task.h"
#include "blps.h"
#include "blps_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * BLPS PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// Full BLPS Database Description - Used to add attributes into the database
const struct attm_desc blps_att_db[BPS_IDX_NB] =
{
    // Blood Pressure Service Declaration
    [BPS_IDX_SVC]                         =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // Blood Pressure Measurement Characteristic Declaration
    [BPS_IDX_BP_MEAS_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Blood Pressure Measurement Characteristic Value
    [BPS_IDX_BP_MEAS_VAL]                 =   {ATT_CHAR_BLOOD_PRESSURE_MEAS, PERM(IND, ENABLE), PERM(RI, ENABLE), BLPS_BP_MEAS_MAX_LEN},
    // Blood Pressure Measurement Characteristic - Client Characteristic Configuration Descriptor
    [BPS_IDX_BP_MEAS_IND_CFG]             =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Blood Pressure Feature Characteristic Declaration
    [BPS_IDX_BP_FEATURE_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Blood Pressure Feature Characteristic Value
    [BPS_IDX_BP_FEATURE_VAL]              =   {ATT_CHAR_BLOOD_PRESSURE_FEATURE, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint16_t)},

    // Intermediate Cuff Pressure Characteristic Declaration
    [BPS_IDX_INTM_CUFF_PRESS_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Intermediate Cuff Pressure Characteristic Value
    [BPS_IDX_INTM_CUFF_PRESS_VAL]         =   {ATT_CHAR_INTERMEDIATE_CUFF_PRESSURE, PERM(NTF, ENABLE), PERM(RI, ENABLE), BLPS_BP_MEAS_MAX_LEN},
    // Intermediate Cuff Pressure Characteristic - Client Characteristic Configuration Descriptor
    [BPS_IDX_INTM_CUFF_PRESS_NTF_CFG]     =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BLPS module.
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
static uint8_t blps_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, struct blps_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------
    // Service content flag
    uint32_t cfg_flag= BLPS_MANDATORY_MASK;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    //Set Configuration Flag Value
    if (params->prfl_cfg & BLPS_INTM_CUFF_PRESS_SUP)
    {
        cfg_flag |= BLPS_INTM_CUFF_PRESS_MASK;
    }

    // Add service in the database
    status = attm_svc_create_db(start_hdl, ATT_SVC_BLOOD_PRESSURE, (uint8_t *)&cfg_flag,
            BPS_IDX_NB, NULL, env->task, &blps_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate BLPS required environment variable
        struct blps_env_tag* blps_env =
                (struct blps_env_tag* ) ke_malloc(sizeof(struct blps_env_tag), KE_MEM_ATT_DB);

        // Initialize BLPS environment
        env->env           = (prf_env_t*) blps_env;
        blps_env->shdl     = *start_hdl;
        blps_env->features = params->features;
        blps_env->prfl_cfg = params->prfl_cfg;

        blps_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Multi Instantiated task
        blps_env->prf_env.prf_task    = env->task | PERM(PRF_MI, ENABLE);

        // initialize environment variable
        env->id                     = TASK_ID_BLPS;
        blps_task_init(&(env->desc));

        memset(blps_env->prfl_ntf_ind_cfg, 0, BLE_CONNECTION_MAX);

        /* Put BLS in disabled state */
        ke_state_set(env->task, BLPS_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the BLPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void blps_destroy(struct prf_task_env* env)
{
    struct blps_env_tag* blps_env = (struct blps_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(blps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void blps_create(struct prf_task_env* env, uint8_t conidx)
{
    struct blps_env_tag* blps_env = (struct blps_env_tag*) env->env;
    blps_env->prfl_ntf_ind_cfg[conidx] = 0;

    /* Put BLS in idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), BLPS_IDLE);
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
static void blps_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct blps_env_tag* blps_env = (struct blps_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    blps_env->prfl_ntf_ind_cfg[conidx] = 0;
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// BLPS Task interface required by profile manager
const struct prf_task_cbs blps_itf =
{
        (prf_init_fnct) blps_init,
        blps_destroy,
        blps_create,
        blps_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* blps_prf_itf_get(void)
{
   return &blps_itf;
}

uint8_t blps_pack_meas_value(uint8_t *packed_bp, const struct bps_bp_meas* pmeas_val)
{
    uint8_t cursor;

    *(packed_bp) = pmeas_val->flags;

    // Blood Pressure Measurement Compound Value - Systolic
    co_write16p(packed_bp + 1, pmeas_val->systolic);

    // Blood Pressure Measurement Compound Value - Diastolic (mmHg)
    co_write16p(packed_bp + 3, pmeas_val->diastolic);

    //  Blood Pressure Measurement Compound Value - Mean Arterial Pressure (mmHg)
    co_write16p(packed_bp + 5, pmeas_val->mean_arterial_pressure);

    cursor = 7;

    // time flag set
    if ((pmeas_val->flags & BPS_FLAG_TIME_STAMP_PRESENT) == BPS_FLAG_TIME_STAMP_PRESENT)
    {
        cursor += prf_pack_date_time(packed_bp + cursor, &(pmeas_val->time_stamp));
    }

    // Pulse rate flag set
    if ((pmeas_val->flags & BPS_FLAG_PULSE_RATE_PRESENT) == BPS_FLAG_PULSE_RATE_PRESENT)
    {
        co_write16p(packed_bp + cursor, pmeas_val->pulse_rate);
        cursor += 2;
    }

    // User ID flag set
    if ((pmeas_val->flags & BPS_FLAG_USER_ID_PRESENT) == BPS_FLAG_USER_ID_PRESENT)
    {
        *(packed_bp + cursor) = pmeas_val->user_id;
        cursor += 1;
    }

    // Measurement status flag set
    if ((pmeas_val->flags & BPS_FLAG_MEAS_STATUS_PRESENT) == BPS_FLAG_MEAS_STATUS_PRESENT)
    {
        co_write16p(packed_bp + cursor, pmeas_val->meas_status);
        cursor += 2;
    }

    // clear unused packet data
    if(cursor < BLPS_BP_MEAS_MAX_LEN)
    {
        memset(packed_bp + cursor, 0, BLPS_BP_MEAS_MAX_LEN - cursor);
    }

    return cursor;
}

void blps_meas_send_rsp_send(uint8_t conidx, uint8_t status)
{
    // Get the address of the environment
    struct blps_env_tag *blps_env = PRF_ENV_GET(BLPS, blps);

    struct blps_meas_send_rsp * rsp = KE_MSG_ALLOC(BLPS_MEAS_SEND_RSP,
            prf_dst_task_get(&blps_env->prf_env, conidx),
            prf_src_task_get(&blps_env->prf_env, conidx),
            blps_meas_send_rsp);

    rsp->conidx = conidx;
    rsp->status = status;

    ke_msg_send(rsp);
}

#endif /* BLE_BP_SENSOR */

/// @} BLPS
