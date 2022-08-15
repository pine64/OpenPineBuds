/**
 ****************************************************************************************
 * @addtogroup HOGPD
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
#include "attm.h"
#include "hogpd.h"
#include "hogpd_task.h"
#include "prf_utils.h"

#include "ke_mem.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define HIDS_CFG_FLAG_MANDATORY_MASK    ((uint32_t)0x000FD)
#define HIDS_MANDATORY_ATT_NB           (7)
#define HIDS_CFG_FLAG_MAP_EXT_MASK      ((uint32_t)0x00102)
#define HIDS_MAP_EXT_ATT_NB             (2)
#define HIDS_CFG_FLAG_PROTO_MODE_MASK   ((uint32_t)0x00600)
#define HIDS_PROTO_MODE_ATT_NB          (2)
#define HIDS_CFG_FLAG_KEYBOARD_MASK     ((uint32_t)0x0F800)
#define HIDS_KEYBOARD_ATT_NB            (5)
#define HIDS_CFG_FLAG_MOUSE_MASK        ((uint32_t)0x70000)
#define HIDS_MOUSE_ATT_NB               (3)

#define HIDS_CFG_REPORT_MANDATORY_MASK  ((uint32_t)0x7)
#define HIDS_REPORT_MANDATORY_ATT_NB    (3)
#define HIDS_CFG_REPORT_IN_MASK         ((uint32_t)0x8)
#define HIDS_REPORT_IN_ATT_NB           (1)
// number of attribute index for a report
#define HIDS_REPORT_NB_IDX              (4)

/*
 * HIDS ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Full HIDS Database Description - Used to add attributes into the database
const struct attm_desc hids_att_db[HOGPD_IDX_NB] =
{
    // HID Service Declaration
    [HOGPD_IDX_SVC]                             = {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // HID Service Declaration
    [HOGPD_IDX_INCL_SVC]                        = {ATT_DECL_INCLUDE, PERM(RD, ENABLE), 0, 0},

    // HID Information Characteristic Declaration
    [HOGPD_IDX_HID_INFO_CHAR]                   = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),0, 0},
    // HID Information Characteristic Value
    [HOGPD_IDX_HID_INFO_VAL]                    = {ATT_CHAR_HID_INFO, PERM(RD, ENABLE), 0, sizeof(struct hids_hid_info)},

    // HID Control Point Characteristic Declaration
    [HOGPD_IDX_HID_CTNL_PT_CHAR]                = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // HID Control Point Characteristic Value
    [HOGPD_IDX_HID_CTNL_PT_VAL]                 = {ATT_CHAR_HID_CTNL_PT, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE),  sizeof(uint8_t)},

    // Report Map Characteristic Declaration
    [HOGPD_IDX_REPORT_MAP_CHAR]                 = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Report Map Characteristic Value
    [HOGPD_IDX_REPORT_MAP_VAL]                  = {ATT_CHAR_REPORT_MAP, PERM(RD, ENABLE), PERM(RI, ENABLE), HOGPD_REPORT_MAP_MAX_LEN},
    // Report Map Characteristic - External Report Reference Descriptor
    [HOGPD_IDX_REPORT_MAP_EXT_REP_REF]          = {ATT_DESC_EXT_REPORT_REF, PERM(RD, ENABLE), 0, sizeof(uint16_t)},

    // Protocol Mode Characteristic Declaration
    [HOGPD_IDX_PROTO_MODE_CHAR]                 = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Protocol Mode Characteristic Value
    [HOGPD_IDX_PROTO_MODE_VAL]                  = {ATT_CHAR_PROTOCOL_MODE, (PERM(RD, ENABLE) | PERM(WRITE_COMMAND, ENABLE)), PERM(RI, ENABLE), sizeof(uint8_t)},

    // Boot Keyboard Input Report Characteristic Declaration
    [HOGPD_IDX_BOOT_KB_IN_REPORT_CHAR]          = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Boot Keyboard Input Report Characteristic Value
    [HOGPD_IDX_BOOT_KB_IN_REPORT_VAL]           = {ATT_CHAR_BOOT_KB_IN_REPORT, (PERM(RD, ENABLE) | PERM(NTF, ENABLE)), PERM(RI, ENABLE), HOGPD_BOOT_REPORT_MAX_LEN},
    // Boot Keyboard Input Report Characteristic - Client Characteristic Configuration Descriptor
    [HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG]       = {ATT_DESC_CLIENT_CHAR_CFG,  PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},

    // Boot Keyboard Output Report Characteristic Declaration
    [HOGPD_IDX_BOOT_KB_OUT_REPORT_CHAR]         = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Boot Keyboard Output Report Characteristic Value
    [HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL]          = {ATT_CHAR_BOOT_KB_OUT_REPORT, (PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE)), PERM(RI, ENABLE), HOGPD_BOOT_REPORT_MAX_LEN},

    // Boot Mouse Input Report Characteristic Declaration
    [HOGPD_IDX_BOOT_MOUSE_IN_REPORT_CHAR]       = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Boot Mouse Input Report Characteristic Value
    [HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL]        = {ATT_CHAR_BOOT_MOUSE_IN_REPORT, (PERM(RD, ENABLE) | PERM(NTF, ENABLE)), PERM(RI, ENABLE), HOGPD_BOOT_REPORT_MAX_LEN,},
    // Boot Mouse Input Report Characteristic - Client Characteristic Configuration Descriptor
    [HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG,  PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), 0, 0},

    // Report Characteristic Declaration
    [HOGPD_IDX_REPORT_CHAR]                     = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Report Characteristic Value
    [HOGPD_IDX_REPORT_VAL]                      = {ATT_CHAR_REPORT, PERM(RD, ENABLE), PERM(RI, ENABLE), HOGPD_REPORT_MAX_LEN},
    // Report Characteristic - Report Reference Descriptor
    [HOGPD_IDX_REPORT_REP_REF]                  = {ATT_DESC_REPORT_REF, PERM(RD, ENABLE), 0, sizeof(struct hids_report_ref)},
    // Report Characteristic - Client Characteristic Configuration Descriptor
    [HOGPD_IDX_REPORT_NTF_CFG]                  = {ATT_DESC_CLIENT_CHAR_CFG,  PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), 0, 0},
};


/**
 ****************************************************************************************
 * @brief Initialization of the HOGPD module.
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
static uint8_t hogpd_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct hogpd_db_cfg* params)
{
    struct hogpd_env_tag* hogpd_env = NULL;
    // Service content flag - Without Report Characteristics
    uint32_t cfg_flag[HOGPD_NB_HIDS_INST_MAX][(HOGPD_ATT_MAX/32) +1];
    // Start handle used to allocate service.
    uint16_t shdl;

    // Total number of attributes
    uint8_t tot_nb_att = 0;

    // array of service description used to allocate the service
    struct attm_desc * hids_db[HOGPD_NB_HIDS_INST_MAX];
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    // Service Instance Counter, Counter
    uint8_t svc_idx, report_idx;
    // Report Char. Report Ref value
    struct hids_report_ref report_ref;

    //-------------------- allocate memory required for the profile  ---------------------
    hogpd_env = (struct hogpd_env_tag* ) ke_malloc(sizeof(struct hogpd_env_tag), KE_MEM_ATT_DB);

    // ensure that everything is initialized
    memset(hids_db, 0, sizeof(hids_db));
    memset(cfg_flag, 0, sizeof(cfg_flag));
    memset(hogpd_env, 0, sizeof(struct hogpd_env_tag));


    // Check number of HID instances
    if ((params->hids_nb == 0) || (params->hids_nb > HOGPD_NB_HIDS_INST_MAX))
    {
        // Invalid number of service instances
        status = PRF_ERR_INVALID_PARAM;
    }

    // For each required HIDS instance
    for (svc_idx = 0; ((svc_idx < params->hids_nb) && (status == GAP_ERR_NO_ERROR)); svc_idx++)
    {
        // Check number of Report Char. instances
        if (params->cfg[svc_idx].report_nb > HOGPD_NB_REPORT_INST_MAX)
        {
            // Too many Report Char. Instances
            status = PRF_ERR_INVALID_PARAM;
            break;
        }

        // retrieve features
        hogpd_env->svcs[svc_idx].features  = params->cfg[svc_idx].svc_features & HOGPD_CFG_MASK;
        hogpd_env->svcs[svc_idx].nb_report = params->cfg[svc_idx].report_nb;

        hids_db[svc_idx] = (struct attm_desc *) ke_malloc(HOGPD_ATT_MAX * sizeof(struct attm_desc), KE_MEM_NON_RETENTION);

        cfg_flag[svc_idx][0]             = HIDS_CFG_FLAG_MANDATORY_MASK;
        hogpd_env->svcs[svc_idx].nb_att += HIDS_MANDATORY_ATT_NB;

        // copy default definition of the HID attribute database
        memcpy(hids_db[svc_idx], hids_att_db, sizeof(struct attm_desc) * HOGPD_ATT_UNIQ_NB);

        //--------------------------------------------------------------------
        // Compute cfg_flag[i] without Report Characteristics
        //--------------------------------------------------------------------
        if ((params->cfg[svc_idx].svc_features & HOGPD_CFG_MAP_EXT_REF) == HOGPD_CFG_MAP_EXT_REF)
        {
            cfg_flag[svc_idx][0]            |= HIDS_CFG_FLAG_MAP_EXT_MASK;
            hogpd_env->svcs[svc_idx].nb_att += HIDS_MAP_EXT_ATT_NB;
            // store reference handle in database
            hids_db[svc_idx][HOGPD_IDX_INCL_SVC].max_size = params->cfg[svc_idx].ext_ref.inc_svc_hdl;
        }

        if ((params->cfg[svc_idx].svc_features & HOGPD_CFG_PROTO_MODE) == HOGPD_CFG_PROTO_MODE)
        {
            cfg_flag[svc_idx][0]            |= HIDS_CFG_FLAG_PROTO_MODE_MASK;
            hogpd_env->svcs[svc_idx].nb_att += HIDS_PROTO_MODE_ATT_NB;
        }

        if ((params->cfg[svc_idx].svc_features & HOGPD_CFG_KEYBOARD) == HOGPD_CFG_KEYBOARD)
        {
            cfg_flag[svc_idx][0]            |= HIDS_CFG_FLAG_KEYBOARD_MASK;
            hogpd_env->svcs[svc_idx].nb_att += HIDS_KEYBOARD_ATT_NB;

            if ((params->cfg[svc_idx].svc_features & HOGPD_CFG_BOOT_KB_WR) == HOGPD_CFG_BOOT_KB_WR)
            {
                // Adds write permissions on report
                hids_db[svc_idx][HOGPD_IDX_BOOT_KB_IN_REPORT_VAL].perm |= (PERM(WRITE_REQ, ENABLE));
            }
        }

        if ((params->cfg[svc_idx].svc_features & HOGPD_CFG_MOUSE) == HOGPD_CFG_MOUSE)
        {
            cfg_flag[svc_idx][0]            |= HIDS_CFG_FLAG_MOUSE_MASK;
            hogpd_env->svcs[svc_idx].nb_att += HIDS_MOUSE_ATT_NB;

            if ((params->cfg[svc_idx].svc_features & HOGPD_CFG_BOOT_MOUSE_WR) == HOGPD_CFG_BOOT_MOUSE_WR)
            {
                // Adds write permissions on report
                hids_db[svc_idx][HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL].perm |= (PERM(WRITE_REQ, ENABLE));
            }
        }

        // set report handle offset
        hogpd_env->svcs[svc_idx].report_hdl_offset = hogpd_env->svcs[svc_idx].nb_att;

        //--------------------------------------------------------------------
        // Update cfg_flag_rep[i] with Report Characteristics
        //--------------------------------------------------------------------
        for (report_idx = 0; report_idx < params->cfg[svc_idx].report_nb; report_idx++)
        {
            uint16_t perm = 0;
            uint16_t report_offset = HIDS_REPORT_NB_IDX*report_idx;

            // update config for current report
            cfg_flag[svc_idx][(HOGPD_IDX_REPORT_CHAR + report_offset) / 32]    |= (1 << ((HOGPD_IDX_REPORT_CHAR + report_offset) % 32));
            cfg_flag[svc_idx][(HOGPD_IDX_REPORT_VAL + report_offset) / 32]     |= (1 << ((HOGPD_IDX_REPORT_VAL + report_offset) % 32));
            cfg_flag[svc_idx][(HOGPD_IDX_REPORT_REP_REF + report_offset) / 32] |= (1 << ((HOGPD_IDX_REPORT_REP_REF + report_offset) % 32));
            hogpd_env->svcs[svc_idx].nb_att += HIDS_REPORT_MANDATORY_ATT_NB;

            // copy default definition of the HID report database
            memcpy(&(hids_db[svc_idx][HOGPD_IDX_REPORT_CHAR + report_offset]), &(hids_att_db[HOGPD_IDX_REPORT_CHAR]), sizeof(struct attm_desc) * HIDS_REPORT_NB_IDX);

            // according to the report type, update value property
            switch (params->cfg[svc_idx].report_char_cfg[report_idx] & HOGPD_CFG_REPORT_FEAT)
            {
                // Input Report
                case HOGPD_CFG_REPORT_IN:
                {
                    // add notification permission on report
                    perm = PERM(RD, ENABLE) | PERM(NTF, ENABLE);
                    // Report Char. supports NTF => Client Characteristic Configuration Descriptor
                    cfg_flag[svc_idx][(HOGPD_IDX_REPORT_NTF_CFG + report_offset) / 32] |= (1 << ((HOGPD_IDX_REPORT_NTF_CFG + report_offset) % 32));
                    hogpd_env->svcs[svc_idx].nb_att += HIDS_REPORT_IN_ATT_NB;

                    // update feature flag
                    hogpd_env->svcs[svc_idx].features |= (HOGPD_CFG_REPORT_NTF_EN << report_idx);

                    // check if attribute value could be written
                    if ((params->cfg[svc_idx].report_char_cfg[report_idx] & HOGPD_CFG_REPORT_WR) == HOGPD_CFG_REPORT_WR)
                    {
                        perm |= PERM(WRITE_REQ, ENABLE);
                    }
                } break;

                // Output Report
                case HOGPD_CFG_REPORT_OUT:
                {
                    perm = PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE);
                } break;

                // Feature Report
                case HOGPD_CFG_REPORT_FEAT:
                {
                    perm = PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE);
                } break;

                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                } break;
            }

            hids_db[svc_idx][HOGPD_IDX_REPORT_VAL + (HIDS_REPORT_NB_IDX*report_idx)].perm  = perm;
        }

        // increment total number of attributes to allocate.
        tot_nb_att += hogpd_env->svcs[svc_idx].nb_att;
    }

    // Check that attribute list can be allocated.
    if(status == ATT_ERR_NO_ERROR)
    {
        status = attm_reserve_handle_range(start_hdl, tot_nb_att);
    }

    // used start handle calculated when handle range reservation has been performed
    shdl = *start_hdl;
    hogpd_env->start_hdl = *start_hdl;
    hogpd_env->hids_nb   = params->hids_nb;
    // Initialize the Report ID
    report_ref.report_id = 0;

    // allocate services
    for (svc_idx = 0; ((svc_idx < params->hids_nb) && (status == GAP_ERR_NO_ERROR)); svc_idx++)
    {
        uint16_t handle;
        status = attm_svc_create_db(&shdl, ATT_SVC_HID, (uint8_t *)&(cfg_flag[svc_idx][0]),
                                    HOGPD_ATT_MAX, NULL, env->task, hids_db[svc_idx],
                                    (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)));
        // update start handle for next service
        shdl += hogpd_env->svcs[svc_idx].nb_att;

        // Set HID Information Char. Value
        if(status == GAP_ERR_NO_ERROR)
        {
            handle = hogpd_get_att_handle(hogpd_env, svc_idx, HOGPD_IDX_HID_INFO_VAL, 0);
            ASSERT_ERR(handle != ATT_INVALID_HDL);
            status = attm_att_set_value(handle, sizeof(struct hids_hid_info), 0, (uint8_t *)&params->cfg[svc_idx].hid_info);
        }

        // Set Report Map Char. External Report Ref value
        if((status == GAP_ERR_NO_ERROR)
                && ((params->cfg[svc_idx].svc_features & HOGPD_CFG_MAP_EXT_REF) == HOGPD_CFG_MAP_EXT_REF))
        {
            handle = hogpd_get_att_handle(hogpd_env, svc_idx, HOGPD_IDX_REPORT_MAP_EXT_REP_REF, 0);
            ASSERT_ERR(handle != ATT_INVALID_HDL);
            status = attm_att_set_value(handle, sizeof(uint16_t), 0, (uint8_t *)&params->cfg[svc_idx].ext_ref.rep_ref_uuid);
        }

        // Set Report External Ref value
        for(report_idx = 0 ; (status == GAP_ERR_NO_ERROR)
                              && (report_idx < params->cfg[svc_idx].report_nb) ; report_idx++)
        {
            // Save the Report ID
            report_ref.report_id = params->cfg[svc_idx].report_id[report_idx];
            // Set Report Type
            report_ref.report_type = (params->cfg[svc_idx].report_char_cfg[report_idx] & HOGPD_CFG_REPORT_FEAT);

            handle = hogpd_get_att_handle(hogpd_env, svc_idx, HOGPD_IDX_REPORT_REP_REF, report_idx);
            ASSERT_ERR(handle != ATT_INVALID_HDL);
            // Set value in the database
            status = attm_att_set_value(handle, sizeof(struct hids_report_ref), 0, (uint8_t *)&report_ref);
        }


        // by default in Report protocol mode.
        hogpd_env->svcs[svc_idx].proto_mode = HOGP_REPORT_PROTOCOL_MODE;

    }

    //-------------------- Update profile task information  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // allocate HOGPD required environment variable
        env->env = (prf_env_t*) hogpd_env;
        hogpd_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        hogpd_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);
        // initialize environment variable
        env->id                     = TASK_ID_HOGPD;
        hogpd_task_init(&(env->desc));

        // service is ready, go into an Idle state
        ke_state_set(env->task, HOGPD_IDLE);
    }
    else if(hogpd_env != NULL)
    {
        ke_free(hogpd_env);
    }

    // ensure that temporary allocated databases description is correctly free
    for (svc_idx = 0; svc_idx < HOGPD_NB_HIDS_INST_MAX; svc_idx++)
    {
        if(hids_db[svc_idx] != NULL)
        {
            ke_free(hids_db[svc_idx]);
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the HOGPD module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void hogpd_destroy(struct prf_task_env* env)
{
    struct hogpd_env_tag* hogpd_env = (struct hogpd_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(hogpd_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void hogpd_create(struct prf_task_env* env, uint8_t conidx)
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
static void hogpd_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct hogpd_env_tag* hogpd_env = (struct hogpd_env_tag*) env->env;
    uint8_t svc_idx;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // Reset the notification configuration to ensure that no notification will be sent on
    // a disconnected link
    for (svc_idx = 0; svc_idx < hogpd_env->hids_nb; svc_idx++)
    {
        hogpd_env->svcs[svc_idx].ntf_cfg[conidx] = 0;
    }
}



/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// HOGPD Task interface required by profile manager
const struct prf_task_cbs hogpd_itf =
{
        (prf_init_fnct) hogpd_init,
        hogpd_destroy,
        hogpd_create,
        hogpd_cleanup,
};


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* hogpd_prf_itf_get(void)
{
   return &hogpd_itf;
}


uint16_t hogpd_get_att_handle(struct hogpd_env_tag* hogpd_env, uint8_t svc_idx, uint8_t att_idx, uint8_t report_idx)
{
    uint16_t handle  = ATT_INVALID_HDL;
    uint8_t i = 0;

    // Sanity check
    if((svc_idx < hogpd_env ->hids_nb) && (att_idx < HOGPD_IDX_NB)
           && ((att_idx < HOGPD_ATT_UNIQ_NB) || (report_idx < hogpd_env->svcs[svc_idx].nb_report)))
    {
        handle = hogpd_env->start_hdl;

        for(i = 0 ; i < svc_idx ; i++)
        {
            // update start handle for next service - only useful if multiple service, else not used.
            handle +=  hogpd_env->svcs[i].nb_att;
        }

        // increment index according to expected index
        if(att_idx < HOGPD_ATT_UNIQ_NB)
        {
            handle += att_idx;

            // check if Keyboard feature active
            if((hogpd_env->svcs[svc_idx].features & HOGPD_CFG_KEYBOARD) == 0)
            {
               if(att_idx > HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL)
               {
                   handle -= HIDS_KEYBOARD_ATT_NB;
               }
               // Error Case
               else if ((att_idx >= HOGPD_IDX_BOOT_KB_IN_REPORT_CHAR)
                       && (att_idx <= HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL))
               {
                   handle = ATT_INVALID_HANDLE;
               }
            }

            // check if Mouse feature active
            if((hogpd_env->svcs[svc_idx].features & HOGPD_CFG_MOUSE) == 0)
            {
               if(att_idx > HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG)
               {
                   handle -= HIDS_MOUSE_ATT_NB;
               }
               // Error Case
               else if ((att_idx >= HOGPD_IDX_BOOT_MOUSE_IN_REPORT_CHAR)
                       && (att_idx <= HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG))
               {
                   handle = ATT_INVALID_HANDLE;
               }
            }

            // check if Protocol Mode feature active
            if((hogpd_env->svcs[svc_idx].features & HOGPD_CFG_PROTO_MODE) == 0)
            {
               if(att_idx > HOGPD_IDX_PROTO_MODE_VAL)
               {
                   handle -= HIDS_PROTO_MODE_ATT_NB;
               }
               // Error Case
               else if ((att_idx >= HOGPD_IDX_PROTO_MODE_CHAR)
                       && (att_idx <= HOGPD_IDX_PROTO_MODE_VAL))
               {
                   handle = ATT_INVALID_HANDLE;
               }
            }

            // check if Ext Ref feature active
            if((hogpd_env->svcs[svc_idx].features & HOGPD_CFG_MAP_EXT_REF) == 0)
            {
               if(att_idx > HOGPD_IDX_REPORT_MAP_EXT_REP_REF)
               {
                   handle -= HIDS_MAP_EXT_ATT_NB;
               }
               else if(att_idx > HOGPD_IDX_INCL_SVC)
               {
                   handle -= 1;
               }
               // Error Case
               else if ((att_idx == HOGPD_IDX_INCL_SVC)
                       || (att_idx == HOGPD_IDX_REPORT_MAP_EXT_REP_REF))
               {
                   handle = ATT_INVALID_HANDLE;
               }
            }
        }
        else
        {
            handle += hogpd_env->svcs[svc_idx].report_hdl_offset;

            // increment attribute handle with other reports
            for(i = 0 ; i < report_idx; i++)
            {
                handle += HIDS_REPORT_MANDATORY_ATT_NB;
                // check if it's a Report input
                if((hogpd_env->svcs[svc_idx].features & (HOGPD_CFG_REPORT_NTF_EN << i)) != 0)
                {
                    handle += HIDS_REPORT_IN_ATT_NB;
                }
            }

            // Error check
            if((att_idx == HOGPD_IDX_REPORT_NTF_CFG)
                    && ((hogpd_env->svcs[svc_idx].features & (HOGPD_CFG_REPORT_NTF_EN << report_idx)) != 0))
            {
                handle = ATT_INVALID_HANDLE;
            }
            else
            {
                // update handle cursor
                handle += att_idx - HOGPD_ATT_UNIQ_NB;
            }
        }
    }

    return handle;
}

uint8_t hogpd_get_att_idx(struct hogpd_env_tag* hogpd_env, uint16_t handle, uint8_t *svc_idx, uint8_t *att_idx, uint8_t *report_idx)
{
    uint16_t hdl_cursor = hogpd_env->start_hdl;
    uint8_t status = PRF_APP_ERROR;

    // invalid index
    *att_idx = HOGPD_IDX_NB;

    // Browse list of services
    // handle must be greater than current index
    for(*svc_idx = 0 ; (*svc_idx < hogpd_env->hids_nb) && (handle >= hdl_cursor) ; (*svc_idx)++)
    {
        // check if handle is on current service
        if(handle >= (hdl_cursor + hogpd_env->svcs[*svc_idx].nb_att))
        {
            hdl_cursor += hogpd_env->svcs[*svc_idx].nb_att;
            continue;
        }

        // if we are here, we are sure that handle is valid
        status = GAP_ERR_NO_ERROR;
        *report_idx = 0;

        // check if handle is in reports or not
        if(handle < (hdl_cursor + hogpd_env->svcs[*svc_idx].report_hdl_offset))
        {
            if(handle == hdl_cursor)
            {
                *att_idx = HOGPD_IDX_SVC;
                break;
            }

            // check if Ext Ref feature active
            if((hogpd_env->svcs[*svc_idx].features & HOGPD_CFG_MAP_EXT_REF) != 0)
            {
                hdl_cursor += 1;
                if(handle == hdl_cursor)
                {
                    *att_idx = HOGPD_IDX_INCL_SVC;
                    break;
                }
            }

            // check if handle is in mandatory range
            hdl_cursor += HIDS_MANDATORY_ATT_NB;
            if(handle <= hdl_cursor)
            {
                *att_idx = HOGPD_IDX_REPORT_MAP_VAL - (hdl_cursor - handle - 1);
                break;
            }

            // check if Ext Ref feature active
            if((hogpd_env->svcs[*svc_idx].features & HOGPD_CFG_MAP_EXT_REF) != 0)
            {
                hdl_cursor += 1;
                if(handle == hdl_cursor)
                {
                    *att_idx = HOGPD_IDX_REPORT_MAP_EXT_REP_REF;
                    break;
                }
            }

            // check if Protocol Mode feature active
            if((hogpd_env->svcs[*svc_idx].features & HOGPD_CFG_PROTO_MODE) != 0)
            {
                hdl_cursor += HIDS_PROTO_MODE_ATT_NB;
                if(handle <= hdl_cursor)
                {
                    *att_idx = HOGPD_IDX_PROTO_MODE_VAL - (hdl_cursor - handle - 1);
                    break;
                }
            }

            // check if Keyboard feature active
            if((hogpd_env->svcs[*svc_idx].features & HOGPD_CFG_KEYBOARD) != 0)
            {
                hdl_cursor += HIDS_KEYBOARD_ATT_NB;
                if(handle <= hdl_cursor)
                {
                    *att_idx = HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL - (hdl_cursor - handle - 1);
                    break;
                }
            }

            // check if Mouse feature active
            if((hogpd_env->svcs[*svc_idx].features & HOGPD_CFG_MOUSE) != 0)
            {
                hdl_cursor += HIDS_MOUSE_ATT_NB;
                if(handle <= hdl_cursor)
                {
                    *att_idx = HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG - (hdl_cursor - handle - 1);
                    break;
                }
            }

            // not expected
            ASSERT_ERR(0);
        }
        else
        {
            // add handle offset
            hdl_cursor += hogpd_env->svcs[*svc_idx].report_hdl_offset;

            for(*report_idx = 0 ; (*report_idx < hogpd_env->svcs[*svc_idx].nb_report) ; (*report_idx)++)
            {
                hdl_cursor += HIDS_REPORT_MANDATORY_ATT_NB;
                if(handle <= hdl_cursor)
                {
                    *att_idx = HOGPD_IDX_REPORT_REP_REF - (hdl_cursor - handle - 1);
                    break;
                }

                if((hogpd_env->svcs[*svc_idx].features & (HOGPD_CFG_REPORT_NTF_EN << *report_idx)) != 0)
                {
                    hdl_cursor += HIDS_REPORT_IN_ATT_NB;
                    if(handle == hdl_cursor)
                    {
                        *att_idx = HOGPD_IDX_REPORT_NTF_CFG;
                        break;
                    }
                }
            }

            // not expected
            ASSERT_ERR(*att_idx != HOGPD_IDX_NB);
        }
        // loop not expected here
        break;
    }

    return (status);
}


uint8_t hogpd_ntf_send(uint8_t conidx, const struct hogpd_report_info* report)
{
    struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);
    uint8_t  status = GAP_ERR_NO_ERROR;
    uint16_t handle = ATT_INVALID_HANDLE;
    uint8_t  att_idx = HOGPD_IDX_NB;
    uint16_t max_report_len= 0;
    uint16_t feature_mask = 0;
    uint8_t exp_prot_mode = 0;

    // According to the report type retrieve:
    // - Attribute index
    // - Attribute max length
    // - Feature to use
    // - Expected protocol mode
    switch(report->type)
    {
        // An Input Report
        case HOGPD_REPORT:
        {
            att_idx = HOGPD_IDX_REPORT_VAL;
            max_report_len = HOGPD_REPORT_MAX_LEN;
            feature_mask = HOGPD_CFG_REPORT_NTF_EN << report->idx;
            exp_prot_mode = HOGP_REPORT_PROTOCOL_MODE;
        }break;
        // Boot Keyboard input report
        case HOGPD_BOOT_KEYBOARD_INPUT_REPORT:
        {
            att_idx = HOGPD_IDX_BOOT_KB_IN_REPORT_VAL;
            max_report_len = HOGPD_BOOT_REPORT_MAX_LEN;
            feature_mask = HOGPD_CFG_KEYBOARD;
            exp_prot_mode = HOGP_BOOT_PROTOCOL_MODE;
        }break;
        // Boot Mouse input report
        case HOGPD_BOOT_MOUSE_INPUT_REPORT:
        {
            att_idx = HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL;
            max_report_len = HOGPD_BOOT_REPORT_MAX_LEN;
            feature_mask = HOGPD_CFG_MOUSE;
            exp_prot_mode = HOGP_BOOT_PROTOCOL_MODE;
        }break;

        default: /* Nothing to do */ break;
    }

    handle = hogpd_get_att_handle(hogpd_env, report->hid_idx, att_idx, report->idx);

    // check if attribute is found
    if(handle == ATT_INVALID_HANDLE)
    {
        // check if it's an unsupported feature
        if((feature_mask != 0) && (report->hid_idx < hogpd_env->hids_nb)
                && (report->idx < hogpd_env->svcs[report->hid_idx].nb_report)
                && ((hogpd_env->svcs[report->hid_idx].features & feature_mask) == 0))
        {
            status = PRF_ERR_FEATURE_NOT_SUPPORTED;
        }
        // or an invalid param
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    // check if length is valid
    else if(report->length > max_report_len)
    {
        status = PRF_ERR_UNEXPECTED_LEN;
    }
    // check if notification is enabled
    else if ((hogpd_env->svcs[report->hid_idx].ntf_cfg[conidx] & feature_mask) == 0)
    {
        status = PRF_ERR_NTF_DISABLED;
    }
    // check if protocol mode is valid
    else if((hogpd_env->svcs[report->hid_idx].proto_mode != exp_prot_mode)
            && ((hogpd_env->svcs[report->hid_idx].features & HOGPD_CFG_PROTO_MODE) != 0))
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if(status == GAP_ERR_NO_ERROR)
    {
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&(hogpd_env->prf_env), conidx),
                gattc_send_evt_cmd, report->length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_NOTIFY;
        report_ntf->handle    = handle;
        // pack measured value in database
        report_ntf->length    = report->length;
        memcpy(report_ntf->value, report->value, report->length);
        // send notification to peer device
        ke_msg_send(report_ntf);
    }

    return (status);
}

uint8_t hogpd_ntf_cfg_ind_send(uint8_t conidx, uint8_t svc_idx, uint8_t att_idx, uint8_t report_idx, uint16_t ntf_cfg)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    uint16_t mask = 0;

    struct hogpd_env_tag* hogpd_env = PRF_ENV_GET(HOGPD, hogpd);

    // set notification config update mask
    switch(att_idx)
    {
        // An Input Report
        case HOGPD_IDX_REPORT_NTF_CFG:
        {
            mask = HOGPD_CFG_REPORT_NTF_EN << report_idx;
        }break;
        // Boot Keyboard input report
        case HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG:
        {
            mask = HOGPD_CFG_KEYBOARD;
        }break;
        // Boot Mouse input report
        case HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG:
        {
            mask = HOGPD_CFG_MOUSE;
        }break;

        default:
        {
            ASSERT_ERR(0);
        }break;
    }

    // Stop notification
    if (ntf_cfg == PRF_CLI_STOP_NTFIND)
    {
        hogpd_env->svcs[svc_idx].ntf_cfg[conidx] &= ~mask;
    }
    // Start notification
    else if(ntf_cfg == PRF_CLI_START_NTF)
    {
        hogpd_env->svcs[svc_idx].ntf_cfg[conidx] |= mask;
    }
    // Provided value is incorrect
    else
    {
        status = PRF_APP_ERROR;
    }


    // inform application about updated notification configuration
    if(status == GAP_ERR_NO_ERROR)
    {
        // Allocate the GATT notification message
        struct hogpd_ntf_cfg_ind *ntf_cfg_ind = KE_MSG_ALLOC(HOGPD_NTF_CFG_IND,
                                                    prf_dst_task_get(&(hogpd_env->prf_env), conidx),
                                                    prf_src_task_get(&(hogpd_env->prf_env), conidx),
                                                    hogpd_ntf_cfg_ind);

        // Fill in the parameter structure
        ntf_cfg_ind->conidx = conidx;

        for(svc_idx = 0 ; svc_idx < HOGPD_NB_HIDS_INST_MAX; svc_idx++)
        {
            ntf_cfg_ind->ntf_cfg[svc_idx] = hogpd_env->svcs[svc_idx].ntf_cfg[conidx];
        }

        // send indication to application
        ke_msg_send(ntf_cfg_ind);
    }

    return (status);
}

#endif /* BLE_HID_DEVICE */

/// @} HOGPD
