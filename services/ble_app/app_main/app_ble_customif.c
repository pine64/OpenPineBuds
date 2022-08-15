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
#include "string.h"
#include "co_math.h" // Common Maths Definition
#include "cmsis_os.h"
#include "ble_app_dbg.h"
#include "stdbool.h"
#include "app_thread.h"
#include "app_utils.h"
#include "apps.h"
#include "app.h"
#include "app_sec.h"
#include "app_ble_include.h"
#include "nvrecord.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "rwprf_config.h"
#include "nvrecord_ble.h"
#include "app_sec.h"
#ifdef IBRT
#include "app_ibrt_ui.h"
#include "app_tws_if.h"
#endif

static void app_ble_customif_connect_event_handler(ble_evnet_t *event, void *output)
{
}

static void app_ble_customif_disconnect_event_handler(ble_evnet_t *event, void *output)
{
}

static void app_ble_customif_conn_param_update_req_event_handler(ble_evnet_t *event, void *output)
{
}

static void app_ble_customif_set_random_bd_addr_event_handler(ble_evnet_t *event, void *output)
{
    // Indicate that a new random BD address set in lower layers
}

static const ble_event_handler_t app_ble_customif_event_handler_tab[] =
{
    {BLE_CONNECT_EVENT, app_ble_customif_connect_event_handler},
    {BLE_DISCONNECT_EVENT, app_ble_customif_disconnect_event_handler},
    {BLE_CONN_PARAM_UPDATE_REQ_EVENT, app_ble_customif_conn_param_update_req_event_handler},
    {BLE_SET_RANDOM_BD_ADDR_EVENT, app_ble_customif_set_random_bd_addr_event_handler},
};

//handle the event that from ble lower layers
void app_ble_customif_global_handler_ind(ble_evnet_t *event, void *output)
{
    uint8_t evt_type = event->evt_type;
    uint16_t index = 0;
    const ble_event_handler_t *p_ble_event_hand = NULL;

    for (index=0; index<BLE_EVENT_NUM_MAX; index++)
    {
        p_ble_event_hand = &app_ble_customif_event_handler_tab[index];
        if (p_ble_event_hand->evt_type == evt_type)
        {
            p_ble_event_hand->func(event, output);
            break;
        }
    }
}

static void app_ble_customif_callback_roleswitch_start_handler(ble_callback_evnet_t *event, void *output)
{
}

static void app_ble_customif_callback_roleswitch_complete_handler(ble_callback_evnet_t *event, void *output)
{
}

static void app_ble_customif_callback_role_update_handler(ble_callback_evnet_t *event, void *output)
{
}

static void app_ble_customif_callback_ibrt_event_entry_handler(ble_callback_evnet_t *event, void *output)
{
}

static const ble_callback_event_handler_t app_ble_customif_callback_event_handler_tab[] =
{
    {BLE_CALLBACK_RS_START, app_ble_customif_callback_roleswitch_start_handler},
    {BLE_CALLBACK_RS_COMPLETE, app_ble_customif_callback_roleswitch_complete_handler},
    {BLE_CALLBACK_ROLE_UPDATE, app_ble_customif_callback_role_update_handler},
    {BLE_CALLBACK_IBRT_EVENT_ENTRY, app_ble_customif_callback_ibrt_event_entry_handler},
};

//handle the event that from other module
void app_ble_customif_global_callback_handler_ind(ble_callback_evnet_t *event, void *output)
{
   uint8_t evt_type = event->evt_type;
   uint16_t index = 0;
   const ble_callback_event_handler_t *p_ble_callback_event_hand = NULL;
   
   for (index=0; index<BLE_EVENT_NUM_MAX; index++)
   {
       p_ble_callback_event_hand = &app_ble_customif_callback_event_handler_tab[index];
       if (p_ble_callback_event_hand->evt_type == evt_type)
       {
           p_ble_callback_event_hand->func(event, output);
           break;
       }
   }
}

void app_ble_customif_init(void)
{
    LOG_I("%s", __func__);
    app_ble_core_register_global_handler_ind(app_ble_customif_global_handler_ind);
    app_ble_core_register_global_callback_handle_ind(app_ble_customif_global_callback_handler_ind);
}

