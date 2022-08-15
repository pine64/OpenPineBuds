/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include "cmsis_os.h"
#include "rwip_config.h"
#include "ke_msg.h"
#include "att.h"
#include "gapm_task.h"
#include "app_voicepath_ble.h"

#ifdef BISTO_ENABLED
#include "gsound_gatt_server.h"
#include "gsound_custom_ble.h"
#endif

/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/

/****************************function defination****************************/
void app_ble_voicepath_init(void)
{
    app_gsound_ble_init();
}

const struct ke_state_handler *app_voicepath_ble_get_msg_handler_table(void)
{
    return gsound_ble_get_msg_handler_table();
}

void app_ble_voicepath_add_svc(void)
{
    return app_gsound_ble_add_svc();
}

const struct prf_task_cbs *voicepath_prf_itf_get(void)
{
    return gsound_prf_itf_get();
}

void app_ble_bms_add_svc(void)
{

    // Register the BMS service
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON);
#endif
    req->prf_task_id = TASK_ID_BMS;
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    ke_msg_send(req);
}

#if (ANCS_PROXY_ENABLE)
void app_ble_ancsp_add_svc(void)
{
    TRACE(0,"Registering ANCS Proxy GATT Service");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM,
                         TASK_APP,
                         gapm_profile_task_add_cmd,
                         0);

    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON);
#endif
    req->prf_task_id = TASK_ID_ANCSP;
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    ke_msg_send(req);
}

void app_ble_amsp_add_svc(void)
{
    TRACE(0,"Registering AMS Proxy GATT Service");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM,
                         TASK_APP,
                         gapm_profile_task_add_cmd,
                         0);

    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, ENABLE) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, ENABLE);
#endif
    req->prf_task_id = TASK_ID_AMSP;
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    ke_msg_send(req);
}
#endif

void app_voicepath_mtu_exchanged_handler(uint8_t conidx, uint16_t MTU)
{
    app_gsound_ble_mtu_exchanged_handler(conidx, MTU);
}

void app_voicepath_disconnected_evt_handler(uint8_t conidx)
{
    if (app_gsound_ble_get_connection_index() == conidx)
    {
        app_gsound_ble_disconnected_evt_handler(conidx);
    }
    else
    {
        TRACE(2,"disconnect idx:%d, gsound idx:%d", conidx, app_gsound_ble_get_connection_index());
    }
}

void app_voicepath_ble_conn_parameter_updated(uint8_t conidx,
                                              uint16_t connInterval, uint16_t connSlavelatency)
{
    gsound_ble_conn_parameter_updated(conidx, connInterval, connSlavelatency);
}