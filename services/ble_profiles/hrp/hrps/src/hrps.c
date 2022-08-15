/**
 ****************************************************************************************
 * @addtogroup HRPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HR_SENSOR)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "gapc_task.h"
#include "hrps.h"
#include "hrps_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * HTPT PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// Full HRS Database Description - Used to add attributes into the database
const struct attm_desc hrps_att_db[HRS_IDX_NB] =
{
    // Heart Rate Service Declaration
    [HRS_IDX_SVC]                      =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // Heart Rate Measurement Characteristic Declaration
    [HRS_IDX_HR_MEAS_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Heart Rate Measurement Characteristic Value
    [HRS_IDX_HR_MEAS_VAL]             =   {ATT_CHAR_HEART_RATE_MEAS, PERM(NTF, ENABLE), PERM(RI, ENABLE), HRPS_HT_MEAS_MAX_LEN},
    // Heart Rate Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HRS_IDX_HR_MEAS_NTF_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Body Sensor Location Characteristic Declaration
    [HRS_IDX_BODY_SENSOR_LOC_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Body Sensor Location Characteristic Value
    [HRS_IDX_BODY_SENSOR_LOC_VAL]   =   {ATT_CHAR_BODY_SENSOR_LOCATION, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},

    // Heart Rate Control Point Characteristic Declaration
    [HRS_IDX_HR_CTNL_PT_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Heart Rate Control Point Characteristic Value
    [HRS_IDX_HR_CTNL_PT_VAL]         =   {ATT_CHAR_HEART_RATE_CNTL_POINT, PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
};

/**
 ****************************************************************************************
 * @brief Initialization of the HRPS module.
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
static uint8_t hrps_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, struct hrps_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------
    //Service Configuration Flag
    uint8_t cfg_flag = HRPS_MANDATORY_MASK;
    //Database Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    //Set Configuration Flag Value
    if (HRPS_IS_SUPPORTED(params->features, HRPS_BODY_SENSOR_LOC_CHAR_SUP))
    {
        cfg_flag |= HRPS_BODY_SENSOR_LOC_MASK;
    }
    if (HRPS_IS_SUPPORTED(params->features, HRPS_ENGY_EXP_FEAT_SUP))
    {
        cfg_flag |= HRPS_HR_CTNL_PT_MASK;
    }

    //Add Service Into Database
    status = attm_svc_create_db(start_hdl, ATT_SVC_HEART_RATE, (uint8_t *)&cfg_flag,
            HRS_IDX_NB, NULL, env->task, &hrps_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))| PERM(SVC_MI, DISABLE));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate HRPS required environment variable
        struct hrps_env_tag* hrps_env =
                (struct hrps_env_tag* ) ke_malloc(sizeof(struct hrps_env_tag), KE_MEM_ATT_DB);

        // Initialize HRPS environment
        env->env           = (prf_env_t*) hrps_env;
        hrps_env->shdl     = *start_hdl;
        hrps_env->features = params->features;
        hrps_env->sensor_location = params->body_sensor_loc;
        hrps_env->operation = NULL;

        hrps_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        hrps_env->prf_env.prf_task    = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_HRPS;
        hrps_task_init(&(env->desc));

        /* Put HRS in Idle state */
        ke_state_set(env->task, HRPS_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the HRPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void hrps_destroy(struct prf_task_env* env)
{
    struct hrps_env_tag* hrps_env = (struct hrps_env_tag*) env->env;

    // free profile environment variables
    if(hrps_env->operation != NULL)
    {
        ke_free(hrps_env->operation);
    }
    env->env = NULL;
    ke_free(hrps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void hrps_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Nothing to do */
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
static void hrps_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// HRPS Task interface required by profile manager
const struct prf_task_cbs hrps_itf =
{
        (prf_init_fnct) hrps_init,
        hrps_destroy,
        hrps_create,
        hrps_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* hrps_prf_itf_get(void)
{
   return &hrps_itf;
}


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void hrps_meas_send_rsp_send(uint8_t status)
{
    // Get the address of the environment
    struct hrps_env_tag *hrps_env = PRF_ENV_GET(HRPS, hrps);

    // Send CFM to APP that value has been sent or not
    struct hrps_meas_send_rsp * cfm = KE_MSG_ALLOC(
            HRPS_MEAS_SEND_RSP,
            prf_dst_task_get(&hrps_env->prf_env, 0),
            prf_src_task_get(&hrps_env->prf_env, 0),
            hrps_meas_send_rsp);

    cfm->status = status;

    ke_msg_send(cfm);
}

uint8_t hrps_pack_meas_value(uint8_t *packed_hr, const struct hrs_hr_meas* pmeas_val)
{
    uint8_t cursor = 0;
    uint8_t i = 0;

    // Heart Rate measurement flags
    *(packed_hr) = pmeas_val->flags;

    if ((pmeas_val->flags & HRS_FLAG_HR_16BITS_VALUE) == HRS_FLAG_HR_16BITS_VALUE)
    {
        // Heart Rate Measurement Value 16 bits
        co_write16p(packed_hr + 1, pmeas_val->heart_rate);
        cursor += 3;
    }
    else
    {
        // Heart Rate Measurement Value 8 bits
        *(packed_hr + 1) = pmeas_val->heart_rate;
        cursor += 2;
    }

    if ((pmeas_val->flags & HRS_FLAG_ENERGY_EXPENDED_PRESENT) == HRS_FLAG_ENERGY_EXPENDED_PRESENT)
    {
        // Energy Expended present
        co_write16p(packed_hr + cursor, pmeas_val->energy_expended);
        cursor += 2;
    }

    if ((pmeas_val->flags & HRS_FLAG_RR_INTERVAL_PRESENT) == HRS_FLAG_RR_INTERVAL_PRESENT)
    {
        for(i = 0 ; (i < (pmeas_val->nb_rr_interval)) && (i < (HRS_MAX_RR_INTERVAL)) ; i++)
        {
            // RR-Intervals
            co_write16p(packed_hr + cursor, pmeas_val->rr_intervals[i]);
            cursor += 2;
        }
    }

    // Clear unused packet data
    if(cursor < HRPS_HT_MEAS_MAX_LEN)
    {
        memset(packed_hr + cursor, 0, HRPS_HT_MEAS_MAX_LEN - cursor);
    }

    return cursor;
}


void hrps_exe_operation(void)
{
    struct hrps_env_tag *hrps_env = PRF_ENV_GET(HRPS, hrps);

    ASSERT_ERR(hrps_env->operation != NULL);

    bool finished = true;

    while(hrps_env->operation->cursor < BLE_CONNECTION_MAX)
    {
        // Check if notifications are enabled
        if((hrps_env->hr_meas_ntf[hrps_env->operation->cursor] & ~HRP_PRF_CFG_PERFORMED_OK) == PRF_CLI_START_NTF)
        {
            // Allocate the GATT notification message
            struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                    KE_BUILD_ID(TASK_GATTC, hrps_env->operation->cursor),
                    prf_src_task_get(&(hrps_env->prf_env), 0),
                    gattc_send_evt_cmd, hrps_env->operation->length);

            // Fill in the parameter structure
            meas_val->operation = GATTC_NOTIFY;
            meas_val->handle = hrps_env->shdl + HRS_IDX_HR_MEAS_VAL;
            meas_val->length = hrps_env->operation->length;
            memcpy(meas_val->value, hrps_env->operation->data, hrps_env->operation->length);

            // Send the event
            ke_msg_send(meas_val);

            finished = false;
            hrps_env->operation->cursor++;
            break;
        }

        hrps_env->operation->cursor++;
    }

    // check if operation is finished
    if(finished)
    {
        // Send confirmation
        hrps_meas_send_rsp_send(GAP_ERR_NO_ERROR);

        // free operation
        ke_free(hrps_env->operation);
        hrps_env->operation = NULL;
        // go back to idle state
        ke_state_set(prf_src_task_get(&(hrps_env->prf_env), 0), HRPS_IDLE);
    }
}

#endif /* BLE_HR_SENSOR */

/// @} HRPS
