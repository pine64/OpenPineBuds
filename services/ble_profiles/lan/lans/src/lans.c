/**
 ****************************************************************************************
 * @addtogroup LANS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_LN_SENSOR)
#include "lan_common.h"

#include "gap.h"
#include "gattc_task.h"
#include "gattc.h"
#include "attm.h"
#include "lans.h"
#include "lans_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 *  CYCLING POWER SERVICE ATTRIBUTES
 ****************************************************************************************
 */

/// Full LANS Database Description - Used to add attributes into the database
static const struct attm_desc lans_att_db[LNS_IDX_NB] =
{
    // Location and Navigation Service Declaration
    [LNS_IDX_SVC]                  =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // LN Feature Characteristic Declaration
    [LNS_IDX_LN_FEAT_CHAR]         =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // LN Feature Characteristic Value
    [LNS_IDX_LN_FEAT_VAL]          =   {ATT_CHAR_LN_FEAT, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint32_t)},

    // Location and Speed Characteristic Declaration
    [LNS_IDX_LOC_SPEED_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Location and Speed Characteristic Value
    [LNS_IDX_LOC_SPEED_VAL]        =   {ATT_CHAR_LOC_SPEED, PERM(NTF, ENABLE), PERM(RI, ENABLE), LANP_LAN_LOC_SPEED_MAX_LEN},
    // Location and Speed Characteristic - Client Characteristic Configuration Descriptor
    [LNS_IDX_LOC_SPEED_NTF_CFG]    =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Position Quality Characteristic Declaration
    [LNS_IDX_POS_Q_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Position Quality Characteristic Value
    [LNS_IDX_POS_Q_VAL]            =   {ATT_CHAR_POS_QUALITY, PERM(RD, ENABLE), PERM(RI, ENABLE), LANP_LAN_POSQ_MAX_LEN},

    // LN Control Point Characteristic Declaration
    [LNS_IDX_LN_CTNL_PT_CHAR]      =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // LN Control Point Characteristic Value - The response has the maximal length
    [LNS_IDX_LN_CTNL_PT_VAL]       =   {ATT_CHAR_LN_CNTL_PT, PERM(IND, ENABLE) | PERM(WRITE_REQ, ENABLE),
                                                             PERM(RI, ENABLE), LANP_LAN_LN_CNTL_PT_RSP_MAX_LEN},
    // LN Control Point Characteristic - Client Characteristic Configuration Descriptor
    [LNS_IDX_LN_CTNL_PT_IND_CFG]   =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Navigation Characteristic Declaration
    [LNS_IDX_NAVIGATION_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Navigation Characteristic Value
    [LNS_IDX_NAVIGATION_VAL]        =   {ATT_CHAR_NAVIGATION, PERM(NTF, ENABLE), PERM(RI, ENABLE), LANP_LAN_NAVIGATION_MAX_LEN},
    // Navigation Characteristic - Client Characteristic Configuration Descriptor
    [LNS_IDX_NAVIGATION_NTF_CFG]    =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the LANS module.
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
static uint8_t lans_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, struct lans_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------
    // Service Configuration Flag
    uint16_t cfg_flag = LANS_MANDATORY_MASK;
    // Database Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    /*
     * Check if Position Quality shall be added.
     */
    if((LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP)) ||
       (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_NUMBER_OF_BEACONS_IN_VIEW_SUPP)) ||
       (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_TIME_TO_FIRST_FIX_SUPP)) ||
       (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_ESTIMATED_HOR_POSITION_ERROR_SUPP)) ||
       (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_ESTIMATED_VER_POSITION_ERROR_SUPP)) ||
       (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_HOR_DILUTION_OF_PRECISION_SUPP)) ||
       (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_VER_DILUTION_OF_PRECISION_SUPP)) ||
       (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_POSITION_STATUS_SUPP)))
    {
        //Add configuration to the database
        cfg_flag |= LANS_POS_Q_MASK;
    }

    /*
     * Check if the Navigation characteristic shall be added.
     */
    if ((LANS_IS_FEATURE_SUPPORTED(params->prfl_config, LANS_NAVIGATION_SUPP_FLAG)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_REMAINING_DISTANCE_SUPP)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_REMAINING_VERTICAL_DISTANCE_SUPP)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_ESTIMATED_TIME_OF_ARRIVAL_SUPP)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_POSITION_STATUS_SUPP)))
    {
        cfg_flag |= LANS_NAVI_MASK;
    }
    /*
     * Check if the LN Control Point characteristic shall be added
     */
    if ((LANS_IS_FEATURE_SUPPORTED(cfg_flag, LANS_NAVI_MASK)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->prfl_config, LANS_CTNL_PT_CHAR_SUPP_FLAG)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_TOTAL_DISTANCE_SUPP)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_LSPEED_CHAR_CT_MASKING_SUPP)) ||
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_FIX_RATE_SETTING_SUPP)) ||
        ((LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_ELEVATION_SUPP)) &&
        (LANS_IS_FEATURE_SUPPORTED(params->ln_feature, LANP_FEAT_ELEVATION_SETTING_SUPP))))
    {
        cfg_flag |= LANS_LN_CTNL_PT_MASK;
    }

    // Add service in the database
    status = attm_svc_create_db(start_hdl, ATT_SVC_LOCATION_AND_NAVIGATION, (uint8_t *)&cfg_flag,
            LNS_IDX_NB, NULL, env->task, &lans_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate LANS required environment variable
        struct lans_env_tag* lans_env =
                (struct lans_env_tag* ) ke_malloc(sizeof(struct lans_env_tag), KE_MEM_ATT_DB);

        // Initialize LANS environment
        env->env           = (prf_env_t*) lans_env;
        lans_env->shdl     = *start_hdl;
        lans_env->prfl_cfg = cfg_flag;
        lans_env->features = params->ln_feature;
        lans_env->operation = LANS_RESERVED_OP_CODE;
        lans_env->posq = (LANS_IS_FEATURE_SUPPORTED(cfg_flag, LANS_POS_Q_MASK)) ?
                (struct lanp_posq* ) ke_malloc(sizeof(struct lanp_posq), KE_MEM_ATT_DB) : NULL;

        lans_env->op_data = NULL;
        memset(lans_env->env, 0, sizeof(lans_env->env));

        lans_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        lans_env->prf_env.prf_task    = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_LANS;
        lans_task_init(&(env->desc));

        /* Put CPS in Idle state */
        ke_state_set(env->task, LANS_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the LANS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void lans_destroy(struct prf_task_env* env)
{
    struct lans_env_tag* lans_env = (struct lans_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;

    if (lans_env->posq != NULL)
    {
        ke_free(lans_env->posq);
    }

    if (lans_env->op_data != NULL)
    {
        ke_free(lans_env->op_data->cmd);
        if(lans_env->op_data->ntf_pending)
        {
            ke_free(lans_env->op_data->ntf_pending);
        }
        ke_free(lans_env->op_data);
    }

    ke_free(lans_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void lans_create(struct prf_task_env* env, uint8_t conidx)
{
    struct lans_env_tag* lans_env = (struct lans_env_tag*) env->env;

    memset(&(lans_env->env[conidx]), 0, sizeof(struct lans_cnx_env));
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
static void lans_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct lans_env_tag* lans_env = (struct lans_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    memset(&(lans_env->env[conidx]), 0, sizeof(struct lans_cnx_env));
}

/// LANS Task interface required by profile manager
const struct prf_task_cbs lans_itf =
{
        (prf_init_fnct) lans_init,
        lans_destroy,
        lans_create,
        lans_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* lans_prf_itf_get(void)
{
   return &lans_itf;
}

void lans_send_cmp_evt(uint8_t conidx, uint8_t src_id, uint8_t dest_id, uint8_t operation, uint8_t status)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);

    // Go back to the Connected state if the state is busy
    if (ke_state_get(src_id) == LANS_BUSY)
    {
        ke_state_set(src_id, LANS_IDLE);
    }

    // Set the operation code
    lans_env->operation = LANS_RESERVED_OP_CODE;

    // Send the message
    struct lans_cmp_evt *evt = KE_MSG_ALLOC(LANS_CMP_EVT,
                                             dest_id, src_id,
                                             lans_cmp_evt);

    evt->conidx     = conidx;
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

uint8_t lans_pack_loc_speed_ntf(struct lanp_loc_speed *param, uint8_t *pckd_loc_speed)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);
    // Packed Measurement length
    uint8_t pckd_loc_speed_len = LANP_LAN_LOC_SPEED_MIN_LEN;

    // Check provided flags
    if (LANS_IS_PRESENT(param->flags, LANP_LSPEED_INST_SPEED_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_INSTANTANEOUS_SPEED_SUPP))
        {
            //Pack instantaneous speed
            co_write16p(&pckd_loc_speed[pckd_loc_speed_len], param->inst_speed);
            pckd_loc_speed_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_LSPEED_INST_SPEED_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_LSPEED_TOTAL_DISTANCE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_TOTAL_DISTANCE_SUPP))
        {
            //Pack total distance (24bits)
            co_write24p(&pckd_loc_speed[pckd_loc_speed_len], param->total_dist);
            pckd_loc_speed_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_LSPEED_TOTAL_DISTANCE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_LSPEED_LOCATION_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_LOCATION_SUPP))
        {
            //Pack Location
            co_write32p(&pckd_loc_speed[pckd_loc_speed_len], param->latitude);
            pckd_loc_speed_len += 4;
            co_write32p(&pckd_loc_speed[pckd_loc_speed_len], param->longitude);
            pckd_loc_speed_len += 4;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_LSPEED_LOCATION_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_LSPEED_ELEVATION_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ELEVATION_SUPP))
        {
            //Pack elevation (24 bits)
            co_write24p(&pckd_loc_speed[pckd_loc_speed_len], param->elevation);
            pckd_loc_speed_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_LSPEED_ELEVATION_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_LSPEED_HEADING_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_HEADING_SUPP))
        {
            //Pack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
            co_write16p(&pckd_loc_speed[pckd_loc_speed_len], param->heading);
            pckd_loc_speed_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_LSPEED_HEADING_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_LSPEED_ROLLING_TIME_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ROLLING_TIME_SUPP))
        {
            //Pack rolling time
            pckd_loc_speed[pckd_loc_speed_len] = param->rolling_time;
            pckd_loc_speed_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_LSPEED_ROLLING_TIME_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_LSPEED_UTC_TIME_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_UTC_TIME_SUPP))
        {
            // Pack UTC time
            pckd_loc_speed_len += prf_pack_date_time(&pckd_loc_speed[pckd_loc_speed_len], &(param->date_time));

        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_LSPEED_UTC_TIME_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
//    param->flags &= LANP_LSPEED_ALL_PRESENT;
    // Flags value
    co_write16p(&pckd_loc_speed[0], param->flags);

    return pckd_loc_speed_len;
}

uint8_t lans_split_loc_speed_ntf(uint8_t conidx, struct gattc_send_evt_cmd *loc_speed_ntf1)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);
    // Extract flags info
    uint16_t flags = co_read16p(&loc_speed_ntf1->value[0]);
    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *loc_speed_ntf2 = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(lans_env->prf_env), conidx),
            gattc_send_evt_cmd, LANP_LAN_LOC_SPEED_MAX_LEN);

    // Fill in the parameter structure
    loc_speed_ntf2->operation = GATTC_NOTIFY;
    loc_speed_ntf2->handle = LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL);
    loc_speed_ntf2->length = LANP_LAN_LOC_SPEED_MIN_LEN;

    // Copy status flags
    co_write16p(&loc_speed_ntf2->value[0], (flags & (LANP_LSPEED_POSITION_STATUS_LSB |
                                    LANP_LSPEED_POSITION_STATUS_MSB |
                                    LANP_LSPEED_SPEED_AND_DISTANCE_FORMAT |
                                    LANP_LSPEED_ELEVATION_SOURCE_LSB |
                                    LANP_LSPEED_ELEVATION_SOURCE_MSB |
                                    LANP_LSPEED_HEADING_SOURCE)));
    // Current position
    uint8_t len = 0;

    for (uint16_t feat = LANP_LSPEED_INST_SPEED_PRESENT;
            feat <= LANP_LSPEED_POSITION_STATUS_LSB;
            feat <<= 1)
    {
        // First message fits within the MTU
        if (loc_speed_ntf1->length <= gattc_get_mtu(conidx) - 3)
        {
            // Stop splitting
            break;
        }

        if (LANS_IS_PRESENT(flags, feat))
        {
            switch (feat)
            {
                case LANP_LSPEED_ROLLING_TIME_PRESENT:
                    // Copy uint8
                    loc_speed_ntf2->value[loc_speed_ntf2->length] =
                            loc_speed_ntf1->value[LANP_LAN_LOC_SPEED_MIN_LEN];
                    len = 1;
                    break;

                case LANP_LSPEED_INST_SPEED_PRESENT:
                case LANP_LSPEED_HEADING_PRESENT:
                    // Copy uint16
                    memcpy(&loc_speed_ntf2->value[loc_speed_ntf2->length],
                            &loc_speed_ntf1->value[LANP_LAN_LOC_SPEED_MIN_LEN],
                            2);
                    len = 2;
                    break;

                case LANP_LSPEED_TOTAL_DISTANCE_PRESENT:
                case LANP_LSPEED_ELEVATION_PRESENT:
                    // Copy uint24
                    memcpy(&loc_speed_ntf2->value[loc_speed_ntf2->length],
                            &loc_speed_ntf1->value[LANP_LAN_LOC_SPEED_MIN_LEN],
                            3);
                    len = 3;
                    break;

                case LANP_LSPEED_LOCATION_PRESENT:
                    // Copy latitude and longitude
                    memcpy(&loc_speed_ntf2->value[loc_speed_ntf2->length],
                            &loc_speed_ntf1->value[LANP_LAN_LOC_SPEED_MIN_LEN],
                            8);
                    len = 8;
                    break;

                case LANP_LSPEED_UTC_TIME_PRESENT:
                    // Copy time
                    memcpy(&loc_speed_ntf2->value[loc_speed_ntf2->length],
                            &loc_speed_ntf1->value[LANP_LAN_LOC_SPEED_MIN_LEN],
                            7);
                    len = 7;
                    break;

                default:
                    len = 0;
                    break;
            }

            if (len)
            {
                // Update values
                loc_speed_ntf2->length += len;
                // Remove field and flags from the first ntf
                loc_speed_ntf1->length -= len;
                memcpy(&loc_speed_ntf1->value[LANP_LAN_LOC_SPEED_MIN_LEN],
                        &loc_speed_ntf1->value[LANP_LAN_LOC_SPEED_MIN_LEN + len],
                        loc_speed_ntf1->length);
                // Update flags
                loc_speed_ntf1->value[0] &= ~feat;
                loc_speed_ntf2->value[0] |= feat;
            }
        }
    }

    // store the pending notification to send
    lans_env->op_data->ntf_pending = loc_speed_ntf2;

    return loc_speed_ntf2->length;
}

uint8_t lans_update_characteristic_config(uint8_t conidx, uint8_t prfl_config, struct gattc_write_req_ind const *param)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    // Get the value
    uint16_t ntf_cfg = co_read16p(&param->value[0]);

    // Check if the value is correct
    if (prfl_config == LANS_PRF_CFG_FLAG_LN_CTNL_PT_IND)
    {
        if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND))
        {
            // Save the new configuration in the environment
            (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                    LANS_DISABLE_NTF_IND(conidx, prfl_config) : LANS_ENABLE_NTF_IND(conidx, prfl_config);
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
        {
            // Save the new configuration in the environment
            (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                    LANS_DISABLE_NTF_IND(conidx, prfl_config) : LANS_ENABLE_NTF_IND(conidx, prfl_config);
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }

    if (status == GAP_ERR_NO_ERROR)
    {
        // Allocate message to inform application
        struct lans_cfg_ntfind_ind *ind = KE_MSG_ALLOC(LANS_CFG_NTFIND_IND,
                                                         prf_dst_task_get(&(lans_env->prf_env), conidx), prf_src_task_get(&(lans_env->prf_env), conidx),
                                                         lans_cfg_ntfind_ind);
        // Inform APP of configuration change
        ind->char_code = prfl_config;
        ind->ntf_cfg   = ntf_cfg;
        ind->conidx = conidx;
        ke_msg_send(ind);

        // Enable Bonded Data
        LANS_ENABLE_NTF_IND(conidx, LANS_PRF_CFG_PERFORMED_OK);
    }

    return (status);
}

uint8_t lans_pack_navigation_ntf(struct lanp_navigation *param, uint8_t *pckd_navigation)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);
    // Packed Measurement length
    uint8_t pckd_navigation_len = LANP_LAN_NAVIGATION_MIN_LEN;

    // Check provided flags
    if (LANS_IS_PRESENT(param->flags, LANP_NAVI_REMAINING_DIS_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_REMAINING_DISTANCE_SUPP))
        {
            //Pack distance (24bits)
            co_write24p(&pckd_navigation[pckd_navigation_len], param->remaining_distance);
            pckd_navigation_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_NAVI_REMAINING_DIS_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_NAVI_REMAINING_VER_DIS_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_REMAINING_VERTICAL_DISTANCE_SUPP))
        {
            //Pack vertical distance (24bits)
            co_write24p(&pckd_navigation[pckd_navigation_len], param->remaining_ver_distance);
            pckd_navigation_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_NAVI_REMAINING_VER_DIS_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->flags, LANP_NAVI_ESTIMATED_TIME_OF_ARRIVAL_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ESTIMATED_TIME_OF_ARRIVAL_SUPP))
        {
            //Pack time
            pckd_navigation_len += prf_pack_date_time(
                    &pckd_navigation[pckd_navigation_len], &(param->estimated_arrival_time));

        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->flags &= ~LANP_NAVI_ESTIMATED_TIME_OF_ARRIVAL_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
//    param->flags &= LANP_NAVI_ALL_PRESENT;
    // Flags value
    co_write16p(&pckd_navigation[0], param->flags);
    // Bearing value
    co_write16p(&pckd_navigation[2], param->bearing);
    // heading value
    co_write16p(&pckd_navigation[4], param->heading);

    return pckd_navigation_len;
}

void lans_upd_posq(struct lanp_posq param)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);

    // Check provided flags
    if (LANS_IS_PRESENT(param.flags, LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP))
        {
            //Pack beacons in solution
            lans_env->posq->n_beacons_solution = param.n_beacons_solution;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param.flags &= ~LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param.flags, LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_NUMBER_OF_BEACONS_IN_VIEW_SUPP))
        {
            //Pack beacons in view
            lans_env->posq->n_beacons_view = param.n_beacons_view;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param.flags &= ~LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param.flags, LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_TIME_TO_FIRST_FIX_SUPP))
        {
            //Pack time to fix
            lans_env->posq->time_first_fix = param.time_first_fix;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param.flags &= ~LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param.flags, LANP_POSQ_EHPE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ESTIMATED_HOR_POSITION_ERROR_SUPP))
        {
            //Pack ehpe
            lans_env->posq->ehpe = param.ehpe;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param.flags &= ~LANP_POSQ_EHPE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param.flags, LANP_POSQ_EVPE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ESTIMATED_VER_POSITION_ERROR_SUPP))
        {
            //Pack evpe
            lans_env->posq->evpe = param.evpe;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param.flags &= ~LANP_POSQ_EVPE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param.flags, LANP_POSQ_HDOP_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_HOR_DILUTION_OF_PRECISION_SUPP))
        {
            //Pack hdop
            lans_env->posq->hdop = param.hdop;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param.flags &= ~LANP_POSQ_HDOP_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param.flags, LANP_POSQ_VDOP_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_VER_DILUTION_OF_PRECISION_SUPP))
        {
            //Pack vdop
            lans_env->posq->vdop = param.vdop;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param.flags &= ~LANP_POSQ_VDOP_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
//    param.flags &= LANP_POSQ_ALL_PRESENT;
    // Flags value
    lans_env->posq->flags = param.flags;
}

uint8_t lans_pack_posq(uint8_t *pckd_posq)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);

    // Packed length
    uint8_t pckd_posq_len = LANP_LAN_POSQ_MIN_LEN;

    // Check provided flags
    if (LANS_IS_PRESENT(lans_env->posq->flags, LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP))
        {
            //Pack beacons in solution
            pckd_posq[pckd_posq_len] = lans_env->posq->n_beacons_solution;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            lans_env->posq->flags &= ~LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(lans_env->posq->flags, LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_NUMBER_OF_BEACONS_IN_VIEW_SUPP))
        {
            //Pack beacons in view
            pckd_posq[pckd_posq_len] = lans_env->posq->n_beacons_view;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            lans_env->posq->flags &= ~LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(lans_env->posq->flags, LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_TIME_TO_FIRST_FIX_SUPP))
        {
            //Pack time to fix
            co_write16p(&pckd_posq[pckd_posq_len], lans_env->posq->time_first_fix);
            pckd_posq_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            lans_env->posq->flags &= ~LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(lans_env->posq->flags, LANP_POSQ_EHPE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ESTIMATED_HOR_POSITION_ERROR_SUPP))
        {
            //Pack ehpe
            co_write32p(&pckd_posq[pckd_posq_len], lans_env->posq->ehpe);
            pckd_posq_len += 4;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            lans_env->posq->flags &= ~LANP_POSQ_EHPE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(lans_env->posq->flags, LANP_POSQ_EVPE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ESTIMATED_VER_POSITION_ERROR_SUPP))
        {
            //Pack ehpe
            co_write32p(&pckd_posq[pckd_posq_len], lans_env->posq->evpe);
            pckd_posq_len += 4;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            lans_env->posq->flags &= ~LANP_POSQ_EVPE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(lans_env->posq->flags, LANP_POSQ_HDOP_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_HOR_DILUTION_OF_PRECISION_SUPP))
        {
            //Pack ehpe
            pckd_posq[pckd_posq_len] = lans_env->posq->hdop;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            lans_env->posq->flags &= ~LANP_POSQ_HDOP_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(lans_env->posq->flags, LANP_POSQ_VDOP_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_VER_DILUTION_OF_PRECISION_SUPP))
        {
            //Pack ehpe
            pckd_posq[pckd_posq_len] = lans_env->posq->vdop;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            lans_env->posq->flags &= ~LANP_POSQ_VDOP_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
//    lans_env->posq->flags &= LANP_POSQ_ALL_PRESENT;
    // Flags value
    co_write16p(&pckd_posq[0], lans_env->posq->flags);

    return pckd_posq_len;
}

uint8_t lans_pack_ln_ctnl_point_cfm (uint8_t conidx, struct lans_ln_ctnl_pt_cfm *param, uint8_t *rsp)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);
    // Response Length (At least 3)
    uint8_t rsp_len = LANP_LAN_LN_CNTL_PT_RSP_MIN_LEN;

    // Set the Response Code
    rsp[0] = LANP_LN_CTNL_PT_RESPONSE_CODE;
    // Set the Response Value
    rsp[2] = (param->status > LANP_LN_CTNL_PT_RESP_FAILED) ? LANP_LN_CTNL_PT_RESP_FAILED : param->status;

    switch (lans_env->operation)
    {
        case (LANS_SET_CUMUL_VALUE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SET_CUMUL_VALUE;
        } break;

        case (LANS_MASK_LSPEED_CHAR_CT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_MASK_LSPEED_CHAR_CT;
            if (param->status == LANP_LN_CTNL_PT_RESP_SUCCESS)
            {
                lans_env->env[conidx].mask_lspeed_content =
                        param->value.mask_lspeed_content & 0x007F;
            }
        } break;

        case (LANS_NAVIGATION_CONTROL_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_NAVIGATION_CONTROL;
            if (param->status == LANP_LN_CTNL_PT_RESP_SUCCESS)
            {
                switch(LANS_GET_NAV_MODE(conidx))
                {
                    // Disable notifications
                    case LANP_LN_CTNL_STOP_NAVI:
                    case LANP_LN_CTNL_PAUSE_NAVI:
                        LANS_SET_NAV_EN(conidx, false);
                        break;

                    // Enable notifications
                    case LANP_LN_CTNL_START_NAVI:
                    case LANP_LN_CTNL_RESUME_NAVI:
                    case LANP_LN_CTNL_START_NST_WPT:
                        LANS_SET_NAV_EN(conidx, true);
                        break;

                    // Do nothing
                    case LANP_LN_CTNL_SKIP_WPT:
                    default:
                        break;
                }
            }
        } break;

        case (LANS_REQ_NUMBER_OF_ROUTES_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_REQ_NUMBER_OF_ROUTES;
            if (param->status == LANP_LN_CTNL_PT_RESP_SUCCESS)
            {
                co_write16p(&rsp[rsp_len], param->value.number_of_routes);
                rsp_len += 2;
            }
        } break;

        case (LANS_REQ_NAME_OF_ROUTE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_REQ_NAME_OF_ROUTE;
            if (param->status == LANP_LN_CTNL_PT_RESP_SUCCESS)
            {
                // pack name of route
                for (int i=0; i<param->value.route.length; i++)
                {
                    rsp[i + 3] = param->value.route.name[i];
                    rsp_len++;
                }
            }
        } break;

        case (LANS_SELECT_ROUTE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SELECT_ROUTE;
        } break;

        case (LANS_SET_FIX_RATE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SET_FIX_RATE;
        } break;

        case (LANS_SET_ELEVATION_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SET_ELEVATION;
        } break;

        default:
        {
            param->status = LANP_LN_CTNL_PT_RESP_NOT_SUPP;
        } break;
    }

    return rsp_len;
}

uint8_t lans_unpack_ln_ctnl_point_ind (uint8_t conidx, struct gattc_write_req_ind const *param)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);

    // Indication Status
    uint8_t ind_status = LANP_LN_CTNL_PT_RESP_NOT_SUPP;

    // Allocate a request indication message for the application
    struct lans_ln_ctnl_pt_req_ind *req_ind = KE_MSG_ALLOC(LANS_LN_CTNL_PT_REQ_IND,
                                                            prf_dst_task_get(&(lans_env->prf_env), conidx), prf_src_task_get(&(lans_env->prf_env), conidx),
                                                            lans_ln_ctnl_pt_req_ind);
    // Operation Code
    req_ind->op_code   = param->value[0];
    req_ind->conidx = conidx;

    // Operation Code
    switch(req_ind->op_code)
    {
        case (LANP_LN_CTNL_PT_SET_CUMUL_VALUE):
        {
            // Check if total distance feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_TOTAL_DISTANCE_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 4)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env->operation = LANS_SET_CUMUL_VALUE_OP_CODE;
                    // Cumulative value
                    req_ind->value.cumul_val = co_read24p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_MASK_LSPEED_CHAR_CT):
        {
            // Check if the LN Masking feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_LSPEED_CHAR_CT_MASKING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env->operation = LANS_MASK_LSPEED_CHAR_CT_OP_CODE;
                    // Mask content
                    req_ind->value.mask_content = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_NAVIGATION_CONTROL):
        {
            // Check if navigation feature is supported
            if ((LANS_IS_FEATURE_SUPPORTED(lans_env->prfl_cfg, LANS_NAVI_MASK)) &&
                 (LANS_IS_NTF_IND_ENABLED(conidx, LANS_PRF_CFG_FLAG_NAVIGATION_NTF)))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 2)
                {
                    if (param->value[1] <= 0x05)
                    {
                        // The request can be handled
                        ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                        // Update the environment
                        lans_env->operation = LANS_NAVIGATION_CONTROL_OP_CODE;
                        // Control value
                        req_ind->value.control_value = param->value[1];
                        // Store value in the environment
                        LANS_SET_NAV_MODE(conidx, req_ind->value.control_value);
                    }
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_REQ_NUMBER_OF_ROUTES):
        {
            // Check if navigation feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env->prfl_cfg, LANS_NAVI_MASK))
            {
                // The request can be handled
                ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                lans_env->operation = LANS_REQ_NUMBER_OF_ROUTES_OP_CODE;
            }
        } break;

        case (LANP_LN_CTNL_PT_REQ_NAME_OF_ROUTE):
        {
            // Check if navigation feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env->prfl_cfg, LANS_NAVI_MASK))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env->operation = LANS_REQ_NAME_OF_ROUTE_OP_CODE;
                    // Route number
                    req_ind->value.route_number = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_SELECT_ROUTE):
        {
            // Check if navigation feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env->prfl_cfg, LANS_NAVI_MASK))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env->operation = LANS_SELECT_ROUTE_OP_CODE;
                    // route number
                    req_ind->value.route_number = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_SET_FIX_RATE):
        {
            // Check if the LN Masking feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_FIX_RATE_SETTING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 2)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env->operation = LANS_SET_FIX_RATE_OP_CODE;
                    // fix rate
                    req_ind->value.fix_rate = param->value[1];
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_SET_ELEVATION):
        {
            // Check if the Chain Weight Adjustment feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_ELEVATION_SETTING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 4)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env->operation = LANS_SET_ELEVATION_OP_CODE;
                    // elevation
                    req_ind->value.elevation = co_read24p(&param->value[1]);
                }
            }
        } break;

        default:
        {
            // Operation Code is invalid, status is already LAN_CTNL_PT_RESP_NOT_SUPP
        } break;
    }

    // If no error raised, inform the application about the request
    if (ind_status == LANP_LN_CTNL_PT_RESP_SUCCESS)
    {
        // Send the request indication to the application
        ke_msg_send(req_ind);
        // Go to the Busy status
        ke_state_set(prf_src_task_get(&(lans_env->prf_env), conidx), LANS_BUSY);
    }
    else
    {
        // Free the allocated message
        ke_msg_free(ke_param2msg(req_ind));
    }

    return ind_status;
}




void lans_exe_operation(void)
{
    struct lans_env_tag* lans_env = PRF_ENV_GET(LANS, lans);

    ASSERT_ERR(lans_env->op_data != NULL);

    bool finished = true;

    while((lans_env->op_data->cursor < BLE_CONNECTION_MAX) && finished)
    {
        uint8_t conidx = lans_env->op_data->cursor;

        switch(lans_env->operation)
        {
            case LANS_NTF_LOC_SPEED_OP_CODE:
            {
                // notification is pending, send it first
                if(lans_env->op_data->ntf_pending != NULL)
                {
                    ke_msg_send(lans_env->op_data->ntf_pending);
                    lans_env->op_data->ntf_pending = NULL;
                    finished = false;
                }
                // Check if sending of notifications has been enabled
                else if (LANS_IS_NTF_IND_ENABLED(conidx, LANS_PRF_CFG_FLAG_LOC_SPEED_NTF))
                {
                    struct lans_ntf_loc_speed_req *speed_cmd =
                            (struct lans_ntf_loc_speed_req *) ke_msg2param(lans_env->op_data->cmd);

                    // Save flags value
                    uint16_t flags = speed_cmd->parameters.flags;

                    // Mask unwanted fields if supported
                    if (LANS_IS_FEATURE_SUPPORTED(lans_env->features, LANP_FEAT_LSPEED_CHAR_CT_MASKING_SUPP))
                    {
                        speed_cmd->parameters.flags &= ~lans_env->env[conidx].mask_lspeed_content;
                    }

                    // Allocate the GATT notification message
                    struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(lans_env->prf_env),conidx),
                            gattc_send_evt_cmd, LANP_LAN_LOC_SPEED_MAX_LEN);

                    // Fill in the parameter structure
                    meas_val->operation = GATTC_NOTIFY;
                    meas_val->handle = LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL);
                    // pack measured value in database
                    meas_val->length = lans_pack_loc_speed_ntf(&speed_cmd->parameters, meas_val->value);

                    if (meas_val->length > gattc_get_mtu(conidx) - 3)
                    {
                        // Split (if necessary)
                        lans_split_loc_speed_ntf(conidx, meas_val);
                    }

                    // Restore flags value
                    speed_cmd->parameters.flags = flags;

                    // Send the event
                    ke_msg_send(meas_val);

                    finished = false;
                }

                // update cursor only if all notification has been sent
                if(lans_env->op_data->ntf_pending == NULL)
                {
                    lans_env->op_data->cursor++;
                }
            }
            break;

            case LANS_NTF_NAVIGATION_OP_CODE:
            {
                struct lans_ntf_navigation_req *nav_cmd =
                            (struct lans_ntf_navigation_req *) ke_msg2param(lans_env->op_data->cmd);

                // Check if sending of notifications has been enabled
                if (LANS_IS_NTF_IND_ENABLED(conidx, LANS_PRF_CFG_FLAG_NAVIGATION_NTF)
                        // and if navigation is enabled
                        && LANS_IS_NAV_EN(conidx))
                {
                    // Allocate the GATT notification message
                    struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                            KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(lans_env->prf_env),conidx),
                            gattc_send_evt_cmd, LANP_LAN_NAVIGATION_MAX_LEN);

                    // Fill in the parameter structure
                    meas_val->operation = GATTC_NOTIFY;
                    meas_val->handle = LANS_HANDLE(LNS_IDX_NAVIGATION_VAL);
                    // pack measured value in database
                    meas_val->length = lans_pack_navigation_ntf(&nav_cmd->parameters, meas_val->value);

                    // Send the event
                    ke_msg_send(meas_val);

                    finished = false;
                }

                lans_env->op_data->cursor++;
            }
            break;
            default: ASSERT_ERR(0); break;
        }
    }


    // check if operation is finished
    if(finished)
    {
        // send response to requester
        struct lans_ntf_navigation_rsp * rsp =
                KE_MSG_ALLOC(((lans_env->operation == LANS_NTF_NAVIGATION_OP_CODE) ?
                        LANS_NTF_NAVIGATION_RSP : LANS_NTF_LOC_SPEED_RSP),
                        lans_env->op_data->cmd->src_id,
                        lans_env->op_data->cmd->dest_id,
                        lans_ntf_navigation_rsp);
        rsp->status = GAP_ERR_NO_ERROR;
        ke_msg_send(rsp);

        // free operation data
        ke_msg_free(lans_env->op_data->cmd);
        ke_free(lans_env->op_data);
        lans_env->op_data = NULL;
        // Set the operation code
        lans_env->operation = LANS_RESERVED_OP_CODE;
        // go back to idle state
        ke_state_set(prf_src_task_get(&(lans_env->prf_env), 0), LANS_IDLE);
    }
}

void lans_send_rsp_ind(uint8_t conidx, uint8_t req_op_code, uint8_t status)
{
    // Get the address of the environment
    struct lans_env_tag *lans_env = PRF_ENV_GET(LANS, lans);

    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *ctl_pt_rsp = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx),
            prf_src_task_get(&lans_env->prf_env, conidx),
            gattc_send_evt_cmd, LANP_LAN_LN_CNTL_PT_RSP_MIN_LEN);

    // Fill in the parameter structure
    ctl_pt_rsp->operation = GATTC_INDICATE;
    ctl_pt_rsp->handle = LANS_HANDLE(LNS_IDX_LN_CTNL_PT_VAL);
    // Pack Control Point confirmation
    ctl_pt_rsp->length = LANP_LAN_LN_CNTL_PT_RSP_MIN_LEN;
    // Response Code
    ctl_pt_rsp->value[0] = LANP_LN_CTNL_PT_RESPONSE_CODE;
    // Request Operation Code
    ctl_pt_rsp->value[1] = req_op_code;
    // Response value
    ctl_pt_rsp->value[2] = status;

    // Send the event
    ke_msg_send(ctl_pt_rsp);
}


#endif //(BLE_LN_SENSOR)

/// @} LANS
