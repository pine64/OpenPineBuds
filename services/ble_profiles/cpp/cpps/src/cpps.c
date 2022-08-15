/**
 ****************************************************************************************
 * @addtogroup CPPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "rwip_config.h"
#if (BLE_CP_SENSOR)
#include "cpp_common.h"
#include "gap.h"
#include "gattc_task.h"
#include "gattc.h"
#include "cpps.h"
#include "cpps_task.h"
#include "prf_utils.h"
#include "co_math.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Full CPPS Database Description - Used to add attributes into the database
static const struct attm_desc cpps_att_db[CPS_IDX_NB] =
{
    // Cycling Power Service Declaration
    [CPS_IDX_SVC]                    =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // CP Measurement Characteristic Declaration
    [CPS_IDX_CP_MEAS_CHAR]           =  {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // CP Measurement Characteristic Value
    [CPS_IDX_CP_MEAS_VAL]            =   {ATT_CHAR_CP_MEAS, PERM(NTF, ENABLE), PERM(RI, ENABLE), CPP_CP_MEAS_NTF_MAX_LEN},
    // CP Measurement Characteristic - Client Characteristic Configuration Descriptor
    [CPS_IDX_CP_MEAS_NTF_CFG]        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
    // CP Measurement Characteristic - Server Characteristic Configuration Descriptor
    [CPS_IDX_CP_MEAS_BCST_CFG]       =   {ATT_DESC_SERVER_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // CP Feature Characteristic Declaration
    [CPS_IDX_CP_FEAT_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // CP Feature Characteristic Value
    [CPS_IDX_CP_FEAT_VAL]            =   {ATT_CHAR_CP_FEAT, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint32_t)},

    // Sensor Location Characteristic Declaration
    [CPS_IDX_SENSOR_LOC_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Sensor Location Characteristic Value
    [CPS_IDX_SENSOR_LOC_VAL]         =   {ATT_CHAR_SENSOR_LOC, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},

    // CP Vector Characteristic Declaration
    [CPS_IDX_VECTOR_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // CP Vector Characteristic Value
    [CPS_IDX_VECTOR_VAL]             =   {ATT_CHAR_CP_VECTOR, PERM(NTF, ENABLE), PERM(RI, ENABLE), CPP_CP_VECTOR_MAX_LEN},
    // CP Vector Characteristic - Client Characteristic Configuration Descriptor
    [CPS_IDX_VECTOR_NTF_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // CP Control Point Characteristic Declaration
    [CPS_IDX_CTNL_PT_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // CP Control Point Characteristic Value - The response has the maximal length
    [CPS_IDX_CTNL_PT_VAL]            =   {ATT_CHAR_CP_CNTL_PT, PERM(IND, ENABLE) | PERM(WRITE_REQ, ENABLE),
                                                               PERM(RI, ENABLE), CPP_CP_CNTL_PT_RSP_MAX_LEN},
    // CP Control Point Characteristic - Client Characteristic Configuration Descriptor
    [CPS_IDX_CTNL_PT_IND_CFG]        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the CPPS module.
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
static uint8_t cpps_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, struct cpps_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------
    // Service content flag
    uint32_t cfg_flag= CPPS_MANDATORY_MASK;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    /*
     * Check if Broadcaster role shall be added.
     */
    if(CPPS_IS_FEATURE_SUPPORTED(params->prfl_config, CPPS_BROADCASTER_SUPP_FLAG))
    {
        //Add configuration to the database
        cfg_flag |= CPPS_MEAS_BCST_MASK;
    }
    /*
     * Check if the CP Vector characteristic shall be added.
     * Mandatory if at least one Vector procedure is supported, otherwise excluded.
     */
    if ((CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_CRANK_REV_DATA_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_EXTREME_ANGLES_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_INSTANT_MEAS_DIRECTION_SUPP)))
    {
        cfg_flag |= CPPS_VECTOR_MASK;
    }
    /*
     * Check if the Control Point characteristic shall be added
     * Mandatory if server supports:
     *     - Wheel Revolution Data
     *     - Multiple Sensor Locations
     *     - Configurable Settings (CPP_CTNL_PT_SET codes)
     *     - Offset Compensation
     *     - Server allows to be requested for parameters (CPP_CTNL_PT_REQ codes)
     */
    if ((CPPS_IS_FEATURE_SUPPORTED(params->prfl_config, CPPS_CTNL_PT_CHAR_SUPP_FLAG)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_WHEEL_REV_DATA_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_MULT_SENSOR_LOC_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_CRANK_LENGTH_ADJ_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_CHAIN_LENGTH_ADJ_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_CHAIN_WEIGHT_ADJ_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_SPAN_LENGTH_ADJ_SUPP)) ||
        (CPPS_IS_FEATURE_SUPPORTED(params->cp_feature, CPP_FEAT_OFFSET_COMP_SUPP)))
    {
        cfg_flag |= CPPS_CTNL_PT_MASK;
    }

    // Add service in the database
    status = attm_svc_create_db(start_hdl, ATT_SVC_CYCLING_POWER, (uint8_t *)&cfg_flag,
            CPS_IDX_NB, NULL, env->task, &cpps_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate CPPS required environment variable
        struct cpps_env_tag* cpps_env =
                (struct cpps_env_tag* ) ke_malloc(sizeof(struct cpps_env_tag), KE_MEM_ATT_DB);

        // Initialize CPPS environment
        env->env           = (prf_env_t*) cpps_env;
        cpps_env->shdl     = *start_hdl;
        cpps_env->prfl_cfg = cfg_flag;
        cpps_env->features = params->cp_feature;
        cpps_env->sensor_loc = params->sensor_loc;
        cpps_env->cumul_wheel_rev = params->wheel_rev;
        cpps_env->operation = CPPS_RESERVED_OP_CODE;

        cpps_env->op_data = NULL;
        memset(cpps_env->env, 0, sizeof(cpps_env->env));

        cpps_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        cpps_env->prf_env.prf_task    = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_CPPS;
        cpps_task_init(&(env->desc));

        /*
         * Check if the Broadcaster role shall be added.
         */
        if(CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_MEAS_BCST_MASK))
        {
            // Optional Permissions
            uint16_t perm = PERM(NTF, ENABLE) | PERM(BROADCAST,ENABLE);
            //Add configuration to the database
            attm_att_set_permission(CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL), perm, 0);
        }

        /* Put CPS in Idle state */
        ke_state_set(env->task, CPPS_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the CPPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void cpps_destroy(struct prf_task_env* env)
{
    struct cpps_env_tag* cpps_env = (struct cpps_env_tag*) env->env;

    if (cpps_env->op_data != NULL)
    {
        ke_free(cpps_env->op_data->cmd);
        if(cpps_env->op_data->ntf_pending)
        {
            ke_free(cpps_env->op_data->ntf_pending);
        }
        ke_free(cpps_env->op_data);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(cpps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void cpps_create(struct prf_task_env* env, uint8_t conidx)
{
    struct cpps_env_tag* cpps_env = (struct cpps_env_tag*) env->env;

    memset(&(cpps_env->env[conidx]), 0, sizeof(struct cpps_cnx_env));
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
static void cpps_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct cpps_env_tag* cpps_env = (struct cpps_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    memset(&(cpps_env->env[conidx]), 0, sizeof(struct cpps_cnx_env));
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// CPPS Task interface required by profile manager
const struct prf_task_cbs cpps_itf =
{
        (prf_init_fnct) cpps_init,
        cpps_destroy,
        cpps_create,
        cpps_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* cpps_prf_itf_get(void)
{
   return &cpps_itf;
}

uint8_t cpps_pack_meas_ntf(struct cpp_cp_meas *param, uint8_t *pckd_meas)
{
    // Packed Measurement length
    uint8_t pckd_meas_len = CPP_CP_MEAS_NTF_MIN_LEN;
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    // Check provided flags
    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_PEDAL_POWER_BALANCE_SUPP))
        {
            //Pack Pedal Power Balance info
            pckd_meas[pckd_meas_len] = param->pedal_power_balance;
            pckd_meas_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_ACCUM_TORQUE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_ACCUM_TORQUE_SUPP))
        {
            //Pack Accumulated Torque info
            co_write16p(&pckd_meas[pckd_meas_len], param->accum_torque);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_ACCUM_TORQUE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_WHEEL_REV_DATA_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_WHEEL_REV_DATA_SUPP))
        {
            //Pack Wheel Revolution Data (Cumulative Wheel & Last Wheel Event Time)
            co_write32p(&pckd_meas[pckd_meas_len], cpps_env->cumul_wheel_rev);
            pckd_meas_len += 4;
            co_write16p(&pckd_meas[pckd_meas_len], param->last_wheel_evt_time);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_WHEEL_REV_DATA_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_CRANK_REV_DATA_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_CRANK_REV_DATA_SUPP))
        {
            //Pack Crank Revolution Data (Cumulative Crank & Last Crank Event Time)
            co_write16p(&pckd_meas[pckd_meas_len], param->cumul_crank_rev);
            pckd_meas_len += 2;
            co_write16p(&pckd_meas[pckd_meas_len], param->last_crank_evt_time);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_CRANK_REV_DATA_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_EXTREME_MAGNITUDES_SUPP)
                && CPPS_IS_CLEAR(cpps_env->features, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
            co_write16p(&pckd_meas[pckd_meas_len], param->max_force_magnitude);
            pckd_meas_len += 2;
            co_write16p(&pckd_meas[pckd_meas_len], param->min_force_magnitude);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_EXTREME_MAGNITUDES_SUPP)
                && CPPS_IS_SET(cpps_env->features, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
            co_write16p(&pckd_meas[pckd_meas_len], param->max_torque_magnitude);
            pckd_meas_len += 2;
            co_write16p(&pckd_meas[pckd_meas_len], param->min_torque_magnitude);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_EXTREME_ANGLES_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_EXTREME_ANGLES_SUPP))
        {
            //Pack Extreme Angles (Maximum Angle & Minimum Angle)
            //Force to 12 bits
            param->max_angle &= 0x0FFF;
            param->min_angle &= 0x0FFF;
            uint32_t angle = (uint32_t) (param->max_angle | (param->min_angle<<12));
            co_write24p(&pckd_meas[pckd_meas_len], angle);
            pckd_meas_len += 3;

        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_EXTREME_ANGLES_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_TOPBOT_DEAD_SPOT_ANGLES_SUPP))
        {
            //Pack Top Dead Spot Angle
            co_write16p(&pckd_meas[pckd_meas_len], param->top_dead_spot_angle);
            pckd_meas_len += 2;
        }
        else
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_TOPBOT_DEAD_SPOT_ANGLES_SUPP))
        {
            //Pack Bottom Dead Spot Angle
            co_write16p(&pckd_meas[pckd_meas_len], param->bot_dead_spot_angle);
            pckd_meas_len += 2;
        }
        else
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_MEAS_ACCUM_ENERGY_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_ACCUM_ENERGY_SUPP))
        {
            //Pack Accumulated Energy
            co_write16p(&pckd_meas[pckd_meas_len], param->accum_energy);
            pckd_meas_len += 2;
        }
        else
        {
            //Force to not supported
            param->flags &= ~CPP_MEAS_ACCUM_ENERGY_PRESENT;
        }
    }

    // Allow to use reserved flags according to PTS Test
    //param->flags &= CPP_MEAS_ALL_SUPP;
    // Flags value
    co_write16p(&pckd_meas[0], param->flags);
    //Instant Power (Mandatory)
    co_write16p(&pckd_meas[2], param->inst_power);

    return pckd_meas_len;
}

uint8_t cpps_split_meas_ntf(uint8_t conidx, struct gattc_send_evt_cmd *meas_ntf1)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
    // Extract flags info
    uint16_t flags = co_read16p(&meas_ntf1->value[0]);
    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *meas_ntf2 = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&cpps_env->prf_env, conidx),
            gattc_send_evt_cmd, CPP_CP_MEAS_NTF_MAX_LEN);

    // Fill in the parameter structure
    meas_ntf2->operation = GATTC_NOTIFY;
    meas_ntf2->handle = CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL);
    meas_ntf2->length = CPP_CP_MEAS_NTF_MIN_LEN;

    // Copy status flags
    co_write16p(&meas_ntf2->value[0], (flags & (CPP_MEAS_PEDAL_POWER_BALANCE_REFERENCE |
                                                CPP_MEAS_ACCUM_TORQUE_SOURCE |
                                                CPP_MEAS_OFFSET_COMPENSATION_INDICATOR)));
    // Copy Instantaneous power
    memcpy(&meas_ntf2->value[2], &meas_ntf1->value[2], 2);

    // Current position
    uint8_t len = 0;

    for (uint16_t feat = CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT;
            feat <= CPP_MEAS_OFFSET_COMPENSATION_INDICATOR;
            feat <<= 1)
    {
        // First message fits within the MTU
        if (meas_ntf1->length <= gattc_get_mtu(conidx) - 3)
        {
            // Stop splitting
            break;
        }

        if (CPPS_IS_PRESENT(flags, feat))
        {
            switch (feat)
            {
                case CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT:
                    // Copy uint8
                    meas_ntf2->value[meas_ntf2->length] = meas_ntf1->value[CPP_CP_MEAS_NTF_MIN_LEN];
                    len = 1;
                    break;

                case CPP_MEAS_ACCUM_TORQUE_PRESENT:
                case CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT:
                case CPP_MEAS_ACCUM_ENERGY_PRESENT:
                case CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT:
                    // Copy uint16
                    memcpy(&meas_ntf2->value[meas_ntf2->length], &meas_ntf1->value[CPP_CP_MEAS_NTF_MIN_LEN], 2);
                    len = 2;
                    break;

                case CPP_MEAS_EXTREME_ANGLES_PRESENT:
                    // Copy uint24
                    memcpy(&meas_ntf2->value[meas_ntf2->length], &meas_ntf1->value[CPP_CP_MEAS_NTF_MIN_LEN], 3);
                    len = 3;
                    break;

                case CPP_MEAS_CRANK_REV_DATA_PRESENT:
                case CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT:
                case CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT:
                    // Copy uint16 + uint16
                    memcpy(&meas_ntf2->value[meas_ntf2->length], &meas_ntf1->value[CPP_CP_MEAS_NTF_MIN_LEN], 4);
                    len = 4;
                    break;

                case CPP_MEAS_WHEEL_REV_DATA_PRESENT:
                    // Copy uint32 + uint16
                    memcpy(&meas_ntf2->value[meas_ntf2->length], &meas_ntf1->value[CPP_CP_MEAS_NTF_MIN_LEN], 6);
                    len = 6;
                    break;

                default:
                    len = 0;
                    break;
            }

            if (len)
            {
                // Update values
                meas_ntf2->length += len;
                // Remove field and flags from the first ntf
                meas_ntf1->length -= len;
                memcpy(&meas_ntf1->value[CPP_CP_MEAS_NTF_MIN_LEN],
                        &meas_ntf1->value[CPP_CP_MEAS_NTF_MIN_LEN + len],
                        meas_ntf1->length);
                // Update flags
                meas_ntf1->value[0] &= ~feat;
                meas_ntf2->value[0] |= feat;
            }
        }
    }

    // store the pending notification to send
    cpps_env->op_data->ntf_pending = meas_ntf2;

    return meas_ntf2->length;
}

uint8_t cpps_update_characteristic_config(uint8_t conidx, uint8_t prfl_config, struct gattc_write_req_ind const *param)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    // Get the value
    uint16_t ntf_cfg = co_read16p(&param->value[0]);

    switch (prfl_config)
    {
        case CPP_PRF_CFG_FLAG_CTNL_PT_IND:
            // Check CCC configuration
            if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND))
            {
                // Save the new configuration in the environment
                (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                        CPPS_DISABLE_NTF_IND_BCST(conidx, prfl_config) : CPPS_ENABLE_NTF_IND_BCST(conidx, prfl_config);
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
            break;

        case CPP_PRF_CFG_FLAG_VECTOR_NTF:
            // Do not save until confirmation
            break;

        case CPP_PRF_CFG_FLAG_SP_MEAS_NTF:
            // Check CCC configuration
            if ((ntf_cfg == PRF_SRV_STOP_BCST) || (ntf_cfg == PRF_SRV_START_BCST))
            {
                // Save the new configuration in the environment
                cpps_env->broadcast_enabled = (ntf_cfg == PRF_SRV_STOP_BCST) ?
                        false : true;

                // Update value for every connection (useful for bond data)
                for (uint8_t i=0; i<BLE_CONNECTION_MAX; i++)
                {
                    // Save the new configuration in the environment
                    (ntf_cfg == PRF_SRV_STOP_BCST) ?
                            CPPS_DISABLE_NTF_IND_BCST(conidx, prfl_config) : CPPS_ENABLE_NTF_IND_BCST(conidx, prfl_config);
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
            break;

        case CPP_PRF_CFG_FLAG_CP_MEAS_NTF:

            // Check CCC configuration
            if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
            {
                // Save the new configuration in the environment
                (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                        CPPS_DISABLE_NTF_IND_BCST(conidx, prfl_config) : CPPS_ENABLE_NTF_IND_BCST(conidx, prfl_config);
            }
            else
            {
                status = PRF_APP_ERROR;
            }
            break;

        default:
            status = ATT_ERR_INVALID_HANDLE;
            break;
    }


    if (status == GAP_ERR_NO_ERROR)
    {
        if (prfl_config == CPP_PRF_CFG_FLAG_VECTOR_NTF)
        {
            // Allocate message to inform application
            struct cpps_vector_cfg_req_ind *ind = KE_MSG_ALLOC(CPPS_VECTOR_CFG_REQ_IND,
                                                             prf_dst_task_get(&cpps_env->prf_env, conidx), prf_src_task_get(&cpps_env->prf_env, conidx),
                                                             cpps_vector_cfg_req_ind);
            //Inform APP of configuration change
            ind->char_code = prfl_config;
            ind->ntf_cfg   = ntf_cfg;
            ind->conidx = conidx;
            ke_msg_send(ind);
        }
        else
        {
            // Allocate message to inform application
            struct cpps_cfg_ntfind_ind *ind = KE_MSG_ALLOC(CPPS_CFG_NTFIND_IND,
                                                             prf_dst_task_get(&cpps_env->prf_env, conidx), prf_src_task_get(&cpps_env->prf_env, conidx),
                                                             cpps_cfg_ntfind_ind);
            //Inform APP of configuration change
            ind->char_code = prfl_config;
            ind->ntf_cfg   = ntf_cfg;
            ind->conidx = conidx;
            ke_msg_send(ind);
        }
        // Enable Bonded Data
        CPPS_ENABLE_NTF_IND_BCST(conidx, CPP_PRF_CFG_PERFORMED_OK);
    }
    return (status);
}

uint8_t cpps_pack_vector_ntf(struct cpp_cp_vector *param, uint8_t *pckd_vector)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
    // Packed Measurement length
    uint8_t pckd_vector_len = CPP_CP_VECTOR_MIN_LEN;

    // Check provided flags
    if (CPPS_IS_PRESENT(param->flags, CPP_VECTOR_CRANK_REV_DATA_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_CRANK_REV_DATA_SUPP))
        {
            //Pack Crank Revolution Data (Cumulative Crank & Last Crank Event Time)
            co_write16p(&pckd_vector[pckd_vector_len], param->cumul_crank_rev);
            pckd_vector_len += 2;
            co_write16p(&pckd_vector[pckd_vector_len], param->last_crank_evt_time);
            pckd_vector_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_VECTOR_CRANK_REV_DATA_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_VECTOR_FIRST_CRANK_MEAS_ANGLE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_EXTREME_ANGLES_SUPP))
        {
            //Pack First Crank Measurement Angle
            co_write16p(&pckd_vector[pckd_vector_len], param->first_crank_meas_angle);
            pckd_vector_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_VECTOR_FIRST_CRANK_MEAS_ANGLE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_VECTOR_INST_FORCE_MAGNITUDE_ARRAY_PRESENT))
    {
        if (CPPS_IS_CLEAR(cpps_env->features, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Instantaneous Force Magnitude Array
            if ((param->nb > CPP_CP_VECTOR_MIN_LEN) &&
                    (param->nb <= CPP_CP_VECTOR_MAX_LEN - pckd_vector_len))
            {
                for(int j=0; j<param->nb; j++)
                {
                    co_write16p(&pckd_vector[pckd_vector_len], param->force_torque_magnitude[j]);
                    pckd_vector_len += 2;
                }
            }
            else
            {
                return 0;
            }
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_VECTOR_INST_FORCE_MAGNITUDE_ARRAY_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->flags, CPP_VECTOR_INST_TORQUE_MAGNITUDE_ARRAY_PRESENT))
    {
        if (CPPS_IS_SET(cpps_env->features, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Instantaneous Torque Magnitude Array
            if ((param->nb > CPP_CP_VECTOR_MIN_LEN) &&
                    (param->nb <= CPP_CP_VECTOR_MAX_LEN - pckd_vector_len))
            {
                for(int j=0; j<param->nb; j++)
                {
                    co_write16p(&pckd_vector[pckd_vector_len], param->force_torque_magnitude[j]);
                    pckd_vector_len += 2;
                }
            }
            else
            {
                return 0;
            }
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~CPP_VECTOR_INST_TORQUE_MAGNITUDE_ARRAY_PRESENT;
        }
    }

    // Allow to use reserved flags
    //param->flags &= CPP_VECTOR_ALL_SUPP;
    // Flags value
    pckd_vector[0] = param->flags;

    return pckd_vector_len;
}

void cpps_send_rsp_ind(uint8_t conidx, uint8_t req_op_code, uint8_t status)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *ctl_pt_rsp = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx),
            prf_src_task_get(&cpps_env->prf_env, conidx),
            gattc_send_evt_cmd, CPP_CP_CNTL_PT_RSP_MIN_LEN);

    // Fill in the parameter structure
    ctl_pt_rsp->operation = GATTC_INDICATE;
    ctl_pt_rsp->handle = CPPS_HANDLE(CPS_IDX_CTNL_PT_VAL);
    // Pack Control Point confirmation
    ctl_pt_rsp->length = CPP_CP_CNTL_PT_RSP_MIN_LEN;
    // Response Code
    ctl_pt_rsp->value[0] = CPP_CTNL_PT_RSP_CODE;
    // Request Operation Code
    ctl_pt_rsp->value[1] = req_op_code;
    // Response value
    ctl_pt_rsp->value[2] = status;

    // Send the event
    ke_msg_send(ctl_pt_rsp);
}

uint8_t cpps_unpack_ctnl_point_ind (uint8_t conidx, struct gattc_write_req_ind const *param)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    // Indication Status
    uint8_t ind_status = CPP_CTNL_PT_RESP_NOT_SUPP;

    // Allocate a request indication message for the application
    struct cpps_ctnl_pt_req_ind *req_ind = KE_MSG_ALLOC(CPPS_CTNL_PT_REQ_IND,
            prf_dst_task_get(&cpps_env->prf_env, conidx), prf_src_task_get(&cpps_env->prf_env, conidx), cpps_ctnl_pt_req_ind);
    // Operation Code
    req_ind->op_code   = param->value[0];
    req_ind->conidx = conidx;

    // Operation Code
    switch(req_ind->op_code)
    {
        case (CPP_CTNL_PT_SET_CUMUL_VAL):
        {
            // Check if the Wheel Revolution Data feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_WHEEL_REV_DATA_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 5)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env->operation = CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE;
                    // Cumulative value
                    req_ind->value.cumul_val = co_read32p(&param->value[1]);
                }
            }
        } break;

        case (CPP_CTNL_PT_UPD_SENSOR_LOC):
        {
            // Check if the Multiple Sensor Location feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_MULT_SENSOR_LOC_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 2)
                {
                    // Check the sensor location value
                    if (param->value[1] < CPP_LOC_MAX)
                    {
                        // The request can be handled
                        ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                        // Update the environment
                        cpps_env->operation = CPPS_CTNL_PT_UPD_SENSOR_LOC_OP_CODE;
                        // Sensor Location
                        req_ind->value.sensor_loc = param->value[1];
                    }
                }
            }
        } break;

        case (CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC):
        {
            // Check if the Multiple Sensor Location feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_MULT_SENSOR_LOC_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env->operation = CPPS_CTNL_PT_REQ_SUPP_SENSOR_LOC_OP_CODE;
            }
        } break;

        case (CPP_CTNL_PT_SET_CRANK_LENGTH):
        {
            // Check if the Crank Length Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_CRANK_LENGTH_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env->operation = CPPS_CTNL_PT_SET_CRANK_LENGTH_OP_CODE;
                    // Crank Length
                    req_ind->value.crank_length = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_CRANK_LENGTH):
        {
            // Optional even if feature not supported
            ind_status = CPP_CTNL_PT_RESP_SUCCESS;
            // Update the environment
            cpps_env->operation = CPPS_CTNL_PT_REQ_CRANK_LENGTH_OP_CODE;
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_LENGTH):
        {
            // Check if the Chain Length Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_CHAIN_LENGTH_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env->operation = CPPS_CTNL_PT_SET_CHAIN_LENGTH_OP_CODE;
                    // Chain Length
                    req_ind->value.crank_length = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_CHAIN_LENGTH):
        {
            // Optional even if feature not supported
            ind_status = CPP_CTNL_PT_RESP_SUCCESS;
            // Update the environment
            cpps_env->operation = CPPS_CTNL_PT_REQ_CHAIN_LENGTH_OP_CODE;
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_WEIGHT):
        {
            // Check if the Chain Weight Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_CHAIN_WEIGHT_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env->operation = CPPS_CTNL_PT_SET_CHAIN_WEIGHT_OP_CODE;
                    // Chain Weight
                    req_ind->value.chain_weight = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_CHAIN_WEIGHT):
        {
            // Optional even if feature not supported
            ind_status = CPP_CTNL_PT_RESP_SUCCESS;
            // Update the environment
            cpps_env->operation = CPPS_CTNL_PT_REQ_CHAIN_WEIGHT_OP_CODE;
        } break;

        case (CPP_CTNL_PT_SET_SPAN_LENGTH):
        {
            // Check if the Span Length Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_SPAN_LENGTH_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env->operation = CPPS_CTNL_PT_SET_SPAN_LENGTH_OP_CODE;
                    // Span Length
                    req_ind->value.span_length = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_SPAN_LENGTH):
        {
            // Optional even if feature not supported
            ind_status = CPP_CTNL_PT_RESP_SUCCESS;
            // Update the environment
            cpps_env->operation = CPPS_CTNL_PT_REQ_SPAN_LENGTH_OP_CODE;
        } break;

        case (CPP_CTNL_PT_START_OFFSET_COMP):
        {
            // Check if the Offset Compensation feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_OFFSET_COMP_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env->operation = CPPS_CTNL_PT_START_OFFSET_COMP_OP_CODE;
            }
        } break;

        case (CPP_CTNL_MASK_CP_MEAS_CH_CONTENT):
        {
            // Check if the CP Masking feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_CP_MEAS_CH_CONTENT_MASKING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env->operation = CPPS_CTNL_MASK_CP_MEAS_CH_CONTENT_OP_CODE;
                    // Mask content
                    req_ind->value.mask_content = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (CPP_CTNL_REQ_SAMPLING_RATE):
        {
            // Optional even if feature not supported
            ind_status = CPP_CTNL_PT_RESP_SUCCESS;
            // Update the environment
            cpps_env->operation = CPPS_CTNL_REQ_SAMPLING_RATE_OP_CODE;
        } break;

        case (CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE):
        {
            // Optional even if feature not supported
            ind_status = CPP_CTNL_PT_RESP_SUCCESS;
            // Update the environment
            cpps_env->operation = CPPS_CTNL_REQ_FACTORY_CALIBRATION_DATE_OP_CODE;
        } break;

        default:
        {
            // Operation Code is invalid, status is already CPP_CTNL_PT_RESP_NOT_SUPP
        } break;
    }

    // If no error raised, inform the application about the request
    if (ind_status == CPP_CTNL_PT_RESP_SUCCESS)
    {
        // Send the request indication to the application
        ke_msg_send(req_ind);
        // Go to the Busy status
        ke_state_set(prf_src_task_get(&cpps_env->prf_env, conidx), CPPS_BUSY);
    }
    else
    {
        // Free the allocated message
        ke_msg_free(ke_param2msg(req_ind));
    }

    return ind_status;
}

uint8_t cpps_pack_ctnl_point_cfm (uint8_t conidx, struct cpps_ctnl_pt_cfm *param, uint8_t *rsp)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
    // Response Length (At least 3)
    uint8_t rsp_len = CPP_CP_CNTL_PT_RSP_MIN_LEN;

    // Set the Response Code
    rsp[0] = CPP_CTNL_PT_RSP_CODE;
    // Set the Response Value
    rsp[2] = (param->status > CPP_CTNL_PT_RESP_FAILED) ? CPP_CTNL_PT_RESP_FAILED : param->status;

    switch (cpps_env->operation)
    {
        case (CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CUMUL_VAL;

            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Save in the environment
                cpps_env->cumul_wheel_rev = param->value.cumul_wheel_rev;
            }
        } break;

        case (CPPS_CTNL_PT_UPD_SENSOR_LOC_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_UPD_SENSOR_LOC;

            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Store the new value in the environment
                cpps_env->sensor_loc = param->value.sensor_loc;
            }
        } break;

        case (CPPS_CTNL_PT_REQ_SUPP_SENSOR_LOC_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC;

            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the list of supported location
                for (uint8_t counter = 0; counter < CPP_LOC_MAX; counter++)
                {
                    if ((param->value.supp_sensor_loc >> counter) & 0x0001)
                    {
                        rsp[rsp_len] = counter;
                        rsp_len++;
                    }
                }
            }
        } break;

        case (CPPS_CTNL_PT_SET_CRANK_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CRANK_LENGTH;
        } break;

        case (CPPS_CTNL_PT_REQ_CRANK_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_CRANK_LENGTH;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.crank_length);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_SET_CHAIN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CHAIN_LENGTH;
        } break;

        case (CPPS_CTNL_PT_REQ_CHAIN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_CHAIN_LENGTH;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.chain_length);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_SET_CHAIN_WEIGHT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CHAIN_WEIGHT;
        } break;


        case (CPPS_CTNL_PT_REQ_CHAIN_WEIGHT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_CHAIN_WEIGHT;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.chain_weight);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_SET_SPAN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_SPAN_LENGTH;
        } break;

        case (CPPS_CTNL_PT_REQ_SPAN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_SPAN_LENGTH;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.span_length);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_START_OFFSET_COMP_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_START_OFFSET_COMP;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.offset_comp);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_MASK_CP_MEAS_CH_CONTENT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_MASK_CP_MEAS_CH_CONTENT;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                uint16_t cpp_mask_cp_meas_flags [] =
                {
                    CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT,
                    CPP_MEAS_ACCUM_TORQUE_PRESENT,
                    CPP_MEAS_WHEEL_REV_DATA_PRESENT,
                    CPP_MEAS_CRANK_REV_DATA_PRESENT,
                    CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT |
                        CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT,
                    CPP_MEAS_EXTREME_ANGLES_PRESENT,
                    CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT,
                    CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT,
                    CPP_MEAS_ACCUM_ENERGY_PRESENT,
                };

                uint16_t mask = 0;
                for (uint8_t count = 0; count < 9; count++)
                {
                    if ((param->value.mask_meas_content >> count) & 0x0001)
                    {
                        mask |= cpp_mask_cp_meas_flags[count];
                    }
                }
                cpps_env->env[conidx].mask_meas_content = mask;
            }
        } break;

        case (CPPS_CTNL_REQ_SAMPLING_RATE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_REQ_SAMPLING_RATE;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                rsp[rsp_len] = param->value.sampling_rate;
                rsp_len ++;
            }
        } break;

        case (CPPS_CTNL_REQ_FACTORY_CALIBRATION_DATE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                rsp_len += prf_pack_date_time(&rsp[rsp_len], &(param->value.factory_calibration));
            }
        } break;

        default:
        {
            rsp[2] = CPP_CTNL_PT_RESP_NOT_SUPP;
        } break;
    }

    return rsp_len;
}


int cpps_update_cfg_char_value (uint8_t conidx, uint8_t con_type, uint8_t handle, uint16_t ntf_cfg, uint16_t cfg_flag)
{
    uint8_t status = ATT_ERR_NO_ERROR;
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    if (con_type != PRF_CON_DISCOVERY)
    {
        // Save the new configuration in the environment
        ((ntf_cfg & cfg_flag) == 0) ?
                CPPS_DISABLE_NTF_IND_BCST(conidx, cfg_flag) :
                CPPS_ENABLE_NTF_IND_BCST(conidx, cfg_flag);
    }

    return (status);
}

void cpps_send_cmp_evt(uint8_t conidx, uint8_t src_id, uint8_t dest_id, uint8_t operation, uint8_t status)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
    // Go back to the IDLE if the state is busy
    if (ke_state_get(src_id) == CPPS_BUSY)
    {
        ke_state_set(src_id, CPPS_IDLE);
    }

    // Set the operation code
    cpps_env->operation = CPPS_RESERVED_OP_CODE;

    // Send the message
    struct cpps_cmp_evt *evt = KE_MSG_ALLOC(CPPS_CMP_EVT,
                                             dest_id, src_id,
                                             cpps_cmp_evt);

    evt->conidx = conidx;
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

void cpps_exe_operation(void)
{
    struct cpps_env_tag* cpps_env = PRF_ENV_GET(CPPS, cpps);

    ASSERT_ERR(cpps_env->op_data != NULL);

    bool finished = true;

    while((cpps_env->op_data->cursor < BLE_CONNECTION_MAX) && finished)
    {
        uint8_t conidx = cpps_env->op_data->cursor;

        switch(cpps_env->operation)
        {
            case CPPS_NTF_MEAS_OP_CODE:
            {
                // notification is pending, send it first
                if(cpps_env->op_data->ntf_pending != NULL)
                {
                    ke_msg_send(cpps_env->op_data->ntf_pending);
                    cpps_env->op_data->ntf_pending = NULL;
                    finished = false;
                }
                // Check if sending of notifications has been enabled
                else if (CPPS_IS_NTF_IND_BCST_ENABLED(conidx, CPP_PRF_CFG_FLAG_CP_MEAS_NTF))
                {
                    struct cpps_ntf_cp_meas_req *meas_cmd =
                            (struct cpps_ntf_cp_meas_req *) ke_msg2param(cpps_env->op_data->cmd);
                    // Save flags value
                    uint16_t flags = meas_cmd->parameters.flags;

                    // Mask unwanted fields if supported
                    if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_CP_MEAS_CH_CONTENT_MASKING_SUPP))
                    {
                        meas_cmd->parameters.flags &= ~cpps_env->env[conidx].mask_meas_content;
                    }

                    // Allocate the GATT notification message
                    struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(cpps_env->prf_env),conidx),
                            gattc_send_evt_cmd, CPP_CP_MEAS_NTF_MAX_LEN);

                    // Fill in the parameter structure
                    meas_val->operation = GATTC_NOTIFY;
                    meas_val->handle = CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL);
                    // pack measured value in database
                    meas_val->length = cpps_pack_meas_ntf(&meas_cmd->parameters, meas_val->value);

                    if (meas_val->length > gattc_get_mtu(conidx) - 3)
                    {
                        // Split (if necessary)
                        cpps_split_meas_ntf(conidx, meas_val);
                    }

                    // Restore flags value
                    meas_cmd->parameters.flags = flags;

                    // Send the event
                    ke_msg_send(meas_val);

                    finished = false;
                }
                // update cursor only if all notification has been sent
                if(cpps_env->op_data->ntf_pending == NULL)
                {
                    cpps_env->op_data->cursor++;
                }
            } break;

            case CPPS_NTF_VECTOR_OP_CODE:
            {
                struct cpps_ntf_cp_vector_req *vector_cmd =
                            (struct cpps_ntf_cp_vector_req *) ke_msg2param(cpps_env->op_data->cmd);

                // Check if sending of notifications has been enabled
                if (CPPS_IS_NTF_IND_BCST_ENABLED(conidx, CPP_PRF_CFG_FLAG_VECTOR_NTF))
                {
                    // Allocate the GATT notification message
                    struct gattc_send_evt_cmd *vector_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(cpps_env->prf_env),conidx),
                            gattc_send_evt_cmd, CPP_CP_VECTOR_MAX_LEN);

                    // Fill in the parameter structure
                    vector_val->operation = GATTC_NOTIFY;
                    vector_val->handle = CPPS_HANDLE(CPS_IDX_VECTOR_VAL);
                    // pack measured value in database
                    vector_val->length = cpps_pack_vector_ntf(&vector_cmd->parameters, vector_val->value);

                    // Send the event
                    ke_msg_send(vector_val);

                    finished = false;
                }

                cpps_env->op_data->cursor++;
            } break;

            default: ASSERT_ERR(0); break;
        }
    }

    // check if operation is finished
    if(finished)
    {
        if (cpps_env->operation == CPPS_NTF_MEAS_OP_CODE)
        {
            // send response to requester
            struct cpps_ntf_cp_meas_rsp * rsp = KE_MSG_ALLOC(
                    CPPS_NTF_CP_MEAS_RSP,
                    cpps_env->op_data->cmd->src_id,
                    cpps_env->op_data->cmd->dest_id,
                    cpps_ntf_cp_meas_rsp);

            rsp->status = GAP_ERR_NO_ERROR;
            ke_msg_send(rsp);
        }
        else if (cpps_env->operation == CPPS_NTF_VECTOR_OP_CODE)
        {
            // send response to requester
            struct cpps_ntf_cp_vector_rsp * rsp = KE_MSG_ALLOC(
                    CPPS_NTF_CP_VECTOR_RSP,
                    cpps_env->op_data->cmd->src_id,
                    cpps_env->op_data->cmd->dest_id,
                    cpps_ntf_cp_vector_rsp);

            rsp->status = GAP_ERR_NO_ERROR;
            ke_msg_send(rsp);
        }

        // free operation data
        ke_msg_free(cpps_env->op_data->cmd);
        ke_free(cpps_env->op_data);
        cpps_env->op_data = NULL;
        // Set the operation code
        cpps_env->operation = CPPS_RESERVED_OP_CODE;
        // go back to idle state
        ke_state_set(prf_src_task_get(&(cpps_env->prf_env), 0), CPPS_IDLE);
    }
}

#endif //(BLE_CP_SENSOR)

/// @} CPPS
