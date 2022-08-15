/**
 ****************************************************************************************
 * @addtogroup PRF
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PROFILES)
#include "prf.h"
#include "att.h"


#if (BLE_HT_THERMOM)
extern const struct prf_task_cbs* htpt_prf_itf_get(void);
#endif // (BLE_HT_THERMOM)

#if (BLE_HT_COLLECTOR)
extern const struct prf_task_cbs* htpc_prf_itf_get(void);
#endif // (BLE_HT_COLLECTOR)

#if (BLE_DIS_SERVER)
extern const struct prf_task_cbs* diss_prf_itf_get(void);
#endif // (BLE_HT_THERMOM)

#if (BLE_DIS_CLIENT)
extern const struct prf_task_cbs* disc_prf_itf_get(void);
#endif // (BLE_DIS_CLIENT)

#if (BLE_BP_SENSOR)
extern const struct prf_task_cbs* blps_prf_itf_get(void);
#endif // (BLE_BP_SENSOR)

#if (BLE_BP_COLLECTOR)
extern const struct prf_task_cbs* blpc_prf_itf_get(void);
#endif // (BLE_BP_COLLECTOR)

#if (BLE_TIP_SERVER)
extern const struct prf_task_cbs* tips_prf_itf_get(void);
#endif // (BLE_TIP_SERVER)

#if (BLE_TIP_CLIENT)
extern const struct prf_task_cbs* tipc_prf_itf_get(void);
#endif // (BLE_TIP_CLIENT)

#if (BLE_HR_SENSOR)
extern const struct prf_task_cbs* hrps_prf_itf_get(void);
#endif // (BLE_HR_SENSOR)

#if (BLE_HR_COLLECTOR)
extern const struct prf_task_cbs* hrpc_prf_itf_get(void);
#endif // (BLE_HR_COLLECTOR)

#if (BLE_FINDME_LOCATOR)
extern const struct prf_task_cbs* findl_prf_itf_get(void);
#endif // (BLE_FINDME_LOCATOR)

#if (BLE_FINDME_TARGET)
extern const struct prf_task_cbs* findt_prf_itf_get(void);
#endif // (BLE_FINDME_TARGET)

#if (BLE_PROX_MONITOR)
extern const struct prf_task_cbs* proxm_prf_itf_get(void);
#endif // (BLE_PROX_MONITOR)

#if (BLE_PROX_REPORTER)
extern const struct prf_task_cbs* proxr_prf_itf_get(void);
#endif // (BLE_PROX_REPORTER)

#if (BLE_SP_CLIENT)
extern const struct prf_task_cbs* scppc_prf_itf_get(void);
#endif // (BLE_SP_CLENT)

#if (BLE_SP_SERVER)
extern const struct prf_task_cbs* scpps_prf_itf_get(void);
#endif // (BLE_SP_SERVER)

#if (BLE_BATT_CLIENT)
extern const struct prf_task_cbs* basc_prf_itf_get(void);
#endif // (BLE_BATT_CLIENT)

#if (BLE_BATT_SERVER)
extern const struct prf_task_cbs* bass_prf_itf_get(void);
#endif // (BLE_BATT_SERVER)

#if (BLE_HID_DEVICE)
extern const struct prf_task_cbs* hogpd_prf_itf_get(void);
#endif // (BLE_HID_DEVICE)

#if (BLE_HID_BOOT_HOST)
extern const struct prf_task_cbs* hogpbh_prf_itf_get(void);
#endif // (BLE_HID_BOOT_HOST)

#if (BLE_HID_REPORT_HOST)
extern const struct prf_task_cbs* hogprh_prf_itf_get(void);
#endif // (BLE_HID_REPORT_HOST)

#if (BLE_GL_COLLECTOR)
extern const struct prf_task_cbs* glpc_prf_itf_get(void);
#endif // (BLE_GL_COLLECTOR)

#if (BLE_GL_SENSOR)
extern const struct prf_task_cbs* glps_prf_itf_get(void);
#endif // (BLE_GL_SENSOR)

#if (BLE_RSC_COLLECTOR)
extern const struct prf_task_cbs* rscpc_prf_itf_get(void);
#endif // (BLE_RSC_COLLECTOR)

#if (BLE_RSC_SENSOR)
extern const struct prf_task_cbs* rscps_prf_itf_get(void);
#endif // (BLE_RSC_COLLECTOR)

#if (BLE_CSC_COLLECTOR)
extern const struct prf_task_cbs* cscpc_prf_itf_get(void);
#endif // (BLE_CSC_COLLECTOR)

#if (BLE_CSC_SENSOR)
extern const struct prf_task_cbs* cscps_prf_itf_get(void);
#endif // (BLE_CSC_COLLECTOR)

#if (BLE_AN_CLIENT)
extern const struct prf_task_cbs* anpc_prf_itf_get(void);
#endif // (BLE_AN_CLIENT)

#if (BLE_AN_SERVER)
extern const struct prf_task_cbs* anps_prf_itf_get(void);
#endif // (BLE_AN_SERVER)

#if (BLE_PAS_CLIENT)
extern const struct prf_task_cbs* paspc_prf_itf_get(void);
#endif // (BLE_PAS_CLIENT)

#if (BLE_PAS_SERVER)
extern const struct prf_task_cbs* pasps_prf_itf_get(void);
#endif // (BLE_PAS_SERVER)

#if (BLE_CP_COLLECTOR)
extern const struct prf_task_cbs* cppc_prf_itf_get(void);
#endif //(BLE_CP_COLLECTOR)

#if (BLE_CP_SENSOR)
extern const struct prf_task_cbs* cpps_prf_itf_get(void);
#endif //(BLE_CP_SENSOR)

#if (BLE_LN_COLLECTOR)
extern const struct prf_task_cbs* lanc_prf_itf_get(void);
#endif //(BLE_CP_COLLECTOR)

#if (BLE_LN_SENSOR)
extern const struct prf_task_cbs* lans_prf_itf_get(void);
#endif //(BLE_CP_SENSOR)

#if (BLE_IPS_SERVER)
extern const struct prf_task_cbs* ipss_prf_itf_get(void);
#endif //(BLE_IPS_SERVER)

#if (BLE_IPS_CLIENT)
extern const struct prf_task_cbs* ipsc_prf_itf_get(void);
#endif //(BLE_IPS_CLIENT)

#if (BLE_ENV_SERVER)
extern const struct prf_task_cbs* envs_prf_itf_get(void);
#endif //(BLE_ENV_SERVER)

#if (BLE_ENV_CLIENT)
extern const struct prf_task_cbs* envc_prf_itf_get(void);
#endif //(BLE_ENV_CLIENT

#if (BLE_WSC_SERVER)
extern const struct prf_task_cbs* wscs_prf_itf_get(void);
#endif //(BLE_WSC_SERVER)

#if (BLE_WSC_CLIENT)
extern const struct prf_task_cbs* wscc_prf_itf_get(void);
#endif //(BLE_WSC_CLIENT

#if (BLE_BCS_SERVER)
extern const struct prf_task_cbs* bcss_prf_itf_get(void);
#endif //(BLE_BCS_SERVER)

#if (BLE_BCS_CLIENT)
extern const struct prf_task_cbs* bcsc_prf_itf_get(void);
#endif //(BLE_BCS_CLIENT)

#ifdef BLE_AM0_HEARING_AID_SERV
extern const struct prf_task_cbs* am0_has_prf_itf_get(void);
#endif // BLE_AM0_HEARING_AID_SERV

#if (BLE_UDS_SERVER)
extern const struct prf_task_cbs* udss_prf_itf_get(void);
#endif //(BLE_UDS_SERVER)

#if (BLE_UDS_CLIENT)
extern const struct prf_task_cbs* udsc_prf_itf_get(void);
#endif //(BLE_UDS_CLIENT)

#if (BLE_VOICEPATH)
extern const struct prf_task_cbs* voicepath_prf_itf_get(void);
#endif //(BLE_VOICEPATH)

#if (BLE_DATAPATH_SERVER)
extern const struct prf_task_cbs* datapathps_prf_itf_get(void);
#endif //(BLE_DATAPATH_SERVER)

#if (BLE_OTA)
extern const struct prf_task_cbs* ota_prf_itf_get(void);
#endif //(BLE_OTA)

#if (BLE_TOTA)
extern const struct prf_task_cbs* tota_prf_itf_get(void);
#endif //(BLE_TOTA)

#if (BLE_BMS)
extern const struct prf_task_cbs* bms_prf_itf_get(void);
#endif //(BLE_BMS)


#if (BLE_ANC_CLIENT)
extern const struct prf_task_cbs* ancc_prf_itf_get(void);
#endif //(BLE_ANC_CLIENT)

#if (BLE_AMS_CLIENT)
extern const struct prf_task_cbs* amsc_prf_itf_get(void);
#endif //(BLE_AMS_CLIENT)

#if (BLE_GFPS_PROVIDER)
extern const struct prf_task_cbs* gfpsp_prf_itf_get(void);
#endif //(BLE_GFPS_PROVIDER)

#ifdef BLE_AI_VOICE
extern const struct prf_task_cbs* ai_prf_itf_get(void);
#endif

#if (ANCS_PROXY_ENABLE)
extern const struct prf_task_cbs* ancs_proxy_prf_itf_get(void);
extern const struct prf_task_cbs* ams_proxy_prf_itf_get(void);
#endif

#if (BLE_TILE)
extern const struct prf_task_cbs* tile_prf_itf_get(void);
#endif //(BLE_TILE)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * MACROS
 ****************************************************************************************
 */


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct prf_env_tag prf_env;

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve profile interface
 ****************************************************************************************
 */
static const struct prf_task_cbs * prf_itf_get(uint16_t task_id)
{
    const struct prf_task_cbs* prf_cbs = NULL;

    BLE_DBG(">>>>>> prf_itf_get task_id:  %d task_id %d<<<<<<\n",  KE_TYPE_GET(task_id), task_id);
    switch(KE_TYPE_GET(task_id))
    {
        #if (BLE_HT_THERMOM)
        case TASK_ID_HTPT:
            prf_cbs = htpt_prf_itf_get();
            break;
        #endif // (BLE_HT_THERMOM)

        #if (BLE_HT_COLLECTOR)
        case TASK_ID_HTPC:
            prf_cbs = htpc_prf_itf_get();
            break;
        #endif // (BLE_HT_COLLECTOR)

        #if (BLE_DIS_SERVER)
        case TASK_ID_DISS:
            prf_cbs = diss_prf_itf_get();
            break;
        #endif // (BLE_DIS_SERVER)

        #if (BLE_DIS_CLIENT)
        case TASK_ID_DISC:
            prf_cbs = disc_prf_itf_get();
            break;
        #endif // (BLE_DIS_CLIENT)

        #if (BLE_BP_SENSOR)
        case TASK_ID_BLPS:
            prf_cbs = blps_prf_itf_get();
            break;
        #endif // (BLE_BP_SENSOR)

        #if (BLE_BP_COLLECTOR)
        case TASK_ID_BLPC:
            prf_cbs = blpc_prf_itf_get();
            break;
        #endif // (BLE_BP_COLLECTOR)

        #if (BLE_TIP_SERVER)
        case TASK_ID_TIPS:
            prf_cbs = tips_prf_itf_get();
            break;
        #endif // (BLE_TIP_SERVER)

        #if (BLE_TIP_CLIENT)
        case TASK_ID_TIPC:
            prf_cbs = tipc_prf_itf_get();
            break;
        #endif // (BLE_TIP_CLIENT)

        #if (BLE_HR_SENSOR)
        case TASK_ID_HRPS:
            prf_cbs = hrps_prf_itf_get();
            break;
        #endif // (BLE_HR_SENSOR)

        #if (BLE_HR_COLLECTOR)
        case TASK_ID_HRPC:
            prf_cbs = hrpc_prf_itf_get();
            break;
        #endif // (BLE_HR_COLLECTOR)

        #if (BLE_FINDME_LOCATOR)
        case TASK_ID_FINDL:
            prf_cbs = findl_prf_itf_get();
            break;
        #endif // (BLE_FINDME_LOCATOR)

        #if (BLE_FINDME_TARGET)
        case TASK_ID_FINDT:
            prf_cbs = findt_prf_itf_get();
            break;
        #endif // (BLE_FINDME_TARGET)

        #if (BLE_PROX_MONITOR)
        case TASK_ID_PROXM:
            prf_cbs = proxm_prf_itf_get();
            break;
        #endif // (BLE_PROX_MONITOR)

        #if (BLE_PROX_REPORTER)
        case TASK_ID_PROXR:
            prf_cbs = proxr_prf_itf_get();
            break;
        #endif // (BLE_PROX_REPORTER)

        #if (BLE_SP_SERVER)
        case TASK_ID_SCPPS:
            prf_cbs = scpps_prf_itf_get();
            break;
        #endif // (BLE_SP_SERVER)

        #if (BLE_SP_CLIENT)
        case TASK_ID_SCPPC:
            prf_cbs = scppc_prf_itf_get();
            break;
        #endif // (BLE_SP_CLIENT)

        #if (BLE_BATT_SERVER)
        case TASK_ID_BASS:
            prf_cbs = bass_prf_itf_get();
            break;
        #endif // (BLE_BATT_SERVER)

        #if (BLE_BATT_CLIENT)
        case TASK_ID_BASC:
            prf_cbs = basc_prf_itf_get();
            break;
        #endif // (BLE_BATT_CLIENT)

        #if (BLE_HID_DEVICE)
        case TASK_ID_HOGPD:
            prf_cbs = hogpd_prf_itf_get();
            break;
        #endif // (BLE_HID_DEVICE)

        #if (BLE_HID_BOOT_HOST)
        case TASK_ID_HOGPBH:
            prf_cbs = hogpbh_prf_itf_get();
            break;
        #endif // (BLE_HID_BOOT_HOST)

        #if (BLE_HID_REPORT_HOST)
        case TASK_ID_HOGPRH:
            prf_cbs = hogprh_prf_itf_get();
            break;
        #endif // (BLE_HID_REPORT_HOST)

        #if (BLE_GL_COLLECTOR)
        case TASK_ID_GLPC:
            prf_cbs = glpc_prf_itf_get();
            break;
        #endif // (BLE_GL_COLLECTOR)

        #if (BLE_GL_SENSOR)
        case TASK_ID_GLPS:
            prf_cbs = glps_prf_itf_get();
            break;
        #endif // (BLE_GL_SENSOR)

        #if (BLE_RSC_COLLECTOR)
        case TASK_ID_RSCPC:
            prf_cbs = rscpc_prf_itf_get();
            break;
        #endif // (BLE_RSC_COLLECTOR)

        #if (BLE_RSC_SENSOR)
        case TASK_ID_RSCPS:
            prf_cbs = rscps_prf_itf_get();
            break;
        #endif // (BLE_RSC_SENSOR)

        #if (BLE_CSC_COLLECTOR)
        case TASK_ID_CSCPC:
            prf_cbs = cscpc_prf_itf_get();
            break;
        #endif // (BLE_CSC_COLLECTOR)

        #if (BLE_CSC_SENSOR)
        case TASK_ID_CSCPS:
            prf_cbs = cscps_prf_itf_get();
            break;
        #endif // (BLE_CSC_SENSOR)

        #if (BLE_CP_COLLECTOR)
        case TASK_ID_CPPC:
            prf_cbs = cppc_prf_itf_get();
            break;
        #endif // (BLE_CP_COLLECTOR)

        #if (BLE_CP_SENSOR)
        case TASK_ID_CPPS:
            prf_cbs = cpps_prf_itf_get();
            break;
        #endif // (BLE_CP_SENSOR)

        #if (BLE_LN_COLLECTOR)
        case TASK_ID_LANC:
            prf_cbs = lanc_prf_itf_get();
            break;
        #endif // (BLE_LN_COLLECTOR)

        #if (BLE_LN_SENSOR)
        case TASK_ID_LANS:
            prf_cbs = lans_prf_itf_get();
            break;
        #endif // (BLE_LN_SENSOR)

        #if (BLE_AN_CLIENT)
        case TASK_ID_ANPC:
            prf_cbs = anpc_prf_itf_get();
            break;
        #endif // (BLE_AN_CLIENT)

        #if (BLE_AN_SERVER)
        case TASK_ID_ANPS:
            prf_cbs = anps_prf_itf_get();
            break;
        #endif // (BLE_AN_SERVER)

        #if (BLE_PAS_CLIENT)
        case TASK_ID_PASPC:
            prf_cbs = paspc_prf_itf_get();
            break;
        #endif // (BLE_PAS_CLIENT)

        #if (BLE_PAS_SERVER)
        case TASK_ID_PASPS:
            prf_cbs = pasps_prf_itf_get();
            break;
        #endif // (BLE_PAS_SERVER)

        #ifdef BLE_AM0_HEARING_AID_SERV
        case TASK_ID_AM0_HAS:
            prf_cbs = am0_has_prf_itf_get();
            break;
        #endif // defined(BLE_AM0_HEARING_AID_SERV)

        #if (BLE_IPS_SERVER)
        case TASK_ID_IPSS:
            prf_cbs = ipss_prf_itf_get();
            break;
        #endif //(BLE_IPS_SERVER)

        #if (BLE_IPS_CLIENT)
        case TASK_ID_IPSC:
            prf_cbs = ipsc_prf_itf_get();
            break;
        #endif //(BLE_IPS_CLIENT)

        #if (BLE_ENV_SERVER)
        case TASK_ID_ENVS:
            prf_cbs = envs_prf_itf_get();
            break;
        #endif //(BLE_ENV_SERVER)

        #if (BLE_ENV_CLIENT)
        case TASK_ID_ENVC:
            prf_cbs = envc_prf_itf_get();
            break;
        #endif //(BLE_ENV_CLIENT

        #if (BLE_WSC_SERVER)
        case TASK_ID_WSCS:
            prf_cbs = wscs_prf_itf_get();
            break;
        #endif //(BLE_WSC_SERVER)

        #if (BLE_WSC_CLIENT)
        case TASK_ID_WSCC:
            prf_cbs = wscc_prf_itf_get();
            break;
        #endif //(BLE_WSC_CLIENT

        #if (BLE_BCS_SERVER)
        case TASK_ID_BCSS:
            prf_cbs = bcss_prf_itf_get();
            break;
        #endif //(BLE_BCS_SERVER)

        #if (BLE_BCS_CLIENT)
        case TASK_ID_BCSC:
            prf_cbs = bcsc_prf_itf_get();
            break;
        #endif //(BLE_BCS_CLIENT)

        #if (BLE_UDS_SERVER)
        case TASK_ID_UDSS:
            prf_cbs = udss_prf_itf_get();
            break;
        #endif //(BLE_UDS_SERVER)

        #if (BLE_UDS_CLIENT)
        case TASK_ID_UDSC:
            prf_cbs = udsc_prf_itf_get();
            break;
        #endif //(BLE_UDS_CLIENT)

        #if (BLE_VOICEPATH)
        case TASK_ID_VOICEPATH:
            prf_cbs = voicepath_prf_itf_get();
            break;
        #endif //(TASK_ID_VOICEPATH)

        #if (BLE_OTA)
        case TASK_ID_OTA:
            prf_cbs = ota_prf_itf_get();
            break;
        #endif //(BLE_OTA)

        #if (BLE_TOTA)
        case TASK_ID_TOTA:
            prf_cbs = tota_prf_itf_get();
            break;
        #endif //(BLE_TOTA)

    #if (BLE_BMS)
        case TASK_ID_BMS:
          prf_cbs = bms_prf_itf_get();
          break;
    #endif //(BLE_BMS)

        #if (BLE_ANC_CLIENT)
        case TASK_ID_ANCC:
            prf_cbs = ancc_prf_itf_get();
            break;
        #endif //(BLE_ANC_CLIENT)

        #if (BLE_AMS_CLIENT)
        case TASK_ID_AMSC:
            prf_cbs = amsc_prf_itf_get();
            break;
        #endif //(BLE_AMS_CLIENT)

        #if (BLE_TILE)
        case TASK_ID_TILE:
            prf_cbs = tile_prf_itf_get();
            break;
        #endif //(TASK_ID_TILE)

        #if (ANCS_PROXY_ENABLE)
        case TASK_ID_ANCSP:
            prf_cbs = ancs_proxy_prf_itf_get();
            break;
        case TASK_ID_AMSP:
            prf_cbs = ams_proxy_prf_itf_get();
            break;
        #endif //(ANCS_PROXY_ENABLE)

        #if (BLE_GFPS_PROVIDER)
        case TASK_ID_GFPSP:
            prf_cbs = gfpsp_prf_itf_get();
            break;
        #endif //(BLE_GFPS_PROVIDER) 
        
        #if (BLE_AI_VOICE)
        case TASK_ID_AI:
            prf_cbs = ai_prf_itf_get();
            break;
        #endif //(BLE_AMA)

        #if (BLE_DATAPATH_SERVER)
        case TASK_ID_DATAPATHPS:
            prf_cbs = datapathps_prf_itf_get();
            break;
        #endif //(BLE_DATAPATH_SERVER)

        default: /* Nothing to do */
            break;
    }

    return prf_cbs;
}

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
void prf_init(bool reset)
{
    uint8_t i;

    BLE_FUNC_ENTER();

    BLE_DBG(">>>>>> prf_create reset %d<<<<<<\n", reset);
    if (!reset)
    {
        // FW boot profile initialization
        for(i = 0; i < BLE_NB_PROFILES ; i++)
        {
            prf_env.prf[i].env  = NULL;
            prf_env.prf[i].task = TASK_GAPC + i +1;
            prf_env.prf[i].id   = TASK_ID_INVALID;

            // Initialize Task Descriptor
            prf_env.prf[i].desc.msg_handler_tab = NULL;
            prf_env.prf[i].desc.state           = NULL;
            prf_env.prf[i].desc.idx_max         = 0;
            prf_env.prf[i].desc.msg_cnt         = 0;

            ke_task_create(prf_env.prf[i].task, &(prf_env.prf[i].desc));
            BLE_DBG("prf_init prf_env.prf[%d].task: %d\n", i, prf_env.prf[i].task);
        }
    }
    else
    {
        // FW boot profile destruction
        for(i = 0; i < BLE_NB_PROFILES ; i++)
        {
            // Get Profile API
            const struct prf_task_cbs * cbs = prf_itf_get(prf_env.prf[i].id);
            if(cbs != NULL)
            {
                // request to destroy profile
                cbs->destroy(&(prf_env.prf[i]));
            }
            // unregister profile
            prf_env.prf[i].id   = TASK_ID_INVALID;
            prf_env.prf[i].desc.msg_handler_tab = NULL;
            prf_env.prf[i].desc.state           = NULL;
            prf_env.prf[i].desc.idx_max         = 0;
            prf_env.prf[i].desc.msg_cnt         = 0;

            // Request kernel to flush task messages
            ke_task_msg_flush(KE_TYPE_GET(prf_env.prf[i].task));
        }
    }

    BLE_FUNC_LEAVE();
}


uint8_t prf_add_profile(struct gapm_profile_task_add_cmd * params, ke_task_id_t* prf_task)
{
    uint8_t i;
    uint8_t status = GAP_ERR_NO_ERROR;

    BLE_DBG(">>>>>> prf_add_profile <<<<<<\n");

    // retrieve profile callback
    const struct prf_task_cbs * cbs = prf_itf_get(params->prf_task_id);
    if(cbs == NULL)
    {
        // profile API not available
        status = GAP_ERR_INVALID_PARAM;
    }

    // check if profile not already present in task list
    if(status == GAP_ERR_NO_ERROR)
    {
        for(i = 0; i < BLE_NB_PROFILES ; i++)
        {
            if(prf_env.prf[i].id == params->prf_task_id)
            {
                status = GAP_ERR_NOT_SUPPORTED;
                break;
            }
        }
    }

    if(status == GAP_ERR_NO_ERROR)
    {
        // find first available task
        for(i = 0; i < BLE_NB_PROFILES ; i++)
        {
            // available task found
            if(prf_env.prf[i].id == TASK_ID_INVALID)
            {
                // initialize profile
                status = cbs->init(&(prf_env.prf[i]), &(params->start_hdl), params->app_task, params->sec_lvl, params->param);

                // initialization succeed
                if(status == GAP_ERR_NO_ERROR)
                {
                    // register profile
                    prf_env.prf[i].id = params->prf_task_id;
                    *prf_task = prf_env.prf[i].task;
                }
                break;
            }
        }

        if(i == BLE_NB_PROFILES)
        {
            status = GAP_ERR_INSUFF_RESOURCES;
        }
    }

    return (status);
}



void prf_create(uint8_t conidx)
{
    uint8_t i;
    /* simple connection creation handler, nothing to do. */

    BLE_DBG(">>>>>> prf_create <<<<<<\n");
    // execute create function of each profiles
    for(i = 0; i < BLE_NB_PROFILES ; i++)
    {
        // Get Profile API
        const struct prf_task_cbs * cbs = prf_itf_get(prf_env.prf[i].id);
        if(cbs != NULL)
        {
            // call create callback
            cbs->create(&(prf_env.prf[i]), conidx);
        }
    }
}


void prf_cleanup(uint8_t conidx, uint8_t reason)
{
    uint8_t i;
    /* simple connection creation handler, nothing to do. */

    BLE_DBG(">>>>>> prf_cleanup <<<<<<\n");
    // execute create function of each profiles
    for(i = 0; i < BLE_NB_PROFILES ; i++)
    {
        // Get Profile API
        const struct prf_task_cbs * cbs = prf_itf_get(prf_env.prf[i].id);
        if(cbs != NULL)
        {
            // call cleanup callback
            cbs->cleanup(&(prf_env.prf[i]), conidx, reason);
        }
    }
}


prf_env_t* prf_env_get(uint16_t prf_id)
{
    prf_env_t* env = NULL;
    uint8_t i;
    // find if profile present in profile tasks
    BLE_DBG("%s prf_id %d", __func__, prf_id);
    for(i = 0; i < BLE_NB_PROFILES ; i++)
    {
        // check if profile identifier is known
        if(prf_env.prf[i].id == prf_id)
        {
            env = prf_env.prf[i].env;
            break;
        }
    }

    return env;
}

ke_task_id_t prf_src_task_get(prf_env_t* env, uint8_t conidx)
{
    ke_task_id_t task = PERM_GET(env->prf_task, PRF_TASK);

    BLE_DBG("%s conidx %d", __func__, conidx);
    if(PERM_GET(env->prf_task, PRF_MI))
    {
        task = KE_BUILD_ID(task, conidx);
    }

    return task;
}

ke_task_id_t prf_dst_task_get(prf_env_t* env, uint8_t conidx)
{
    ke_task_id_t task = PERM_GET(env->app_task, PRF_TASK);

    BLE_DBG("%s app_task %d conidx %d", __func__, env->app_task, conidx);
    if(PERM_GET(env->app_task, PRF_MI))
    {
        task = KE_BUILD_ID(task, conidx);
    }

    return task;
}


ke_task_id_t prf_get_id_from_task(ke_msg_id_t task)
{
    ke_task_id_t id = TASK_ID_INVALID;
    uint8_t idx = KE_IDX_GET(task);
    uint8_t i;
    task = KE_TYPE_GET(task);

    BLE_DBG("%s task %d", __func__, task);
    // find if profile present in profile tasks
    for(i = 0; i < BLE_NB_PROFILES ; i++)
    {
        // check if profile identifier is known
        if(prf_env.prf[i].task == task)
        {
            id = prf_env.prf[i].id;
            break;
        }
    }

    return KE_BUILD_ID(id, idx);
}

ke_task_id_t prf_get_task_from_id(ke_msg_id_t id)
{
    ke_task_id_t task = TASK_NONE;
    uint8_t idx = KE_IDX_GET(id);
    uint8_t i;
    id = KE_TYPE_GET(id);

    BLE_DBG("%s id %d", __func__, id);
    // find if profile present in profile tasks
    for(i = 0; i < BLE_NB_PROFILES ; i++)
    {
        // check if profile identifier is known
        if(prf_env.prf[i].id == id)
        {
            task = prf_env.prf[i].task;
            break;
        }
    }

    return KE_BUILD_ID(task, idx);
}


#endif // (BLE_PROFILES)

/// @} PRF
