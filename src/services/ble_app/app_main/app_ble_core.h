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

#ifndef __APP_BLE_CORE_H__
#define __APP_BLE_CORE_H__

/*****************************header include********************************/

/******************************macro defination*****************************/

/******************************type defination******************************/
/**
 * @brief The event type of the ble
 *
 */
typedef enum{
    BLE_CONNECT_EVENT       = 0,
    BLE_DISCONNECT_EVENT,
    BLE_CONN_PARAM_UPDATE_REQ_EVENT,
    BLE_SET_RANDOM_BD_ADDR_EVENT,

    BLE_EVENT_NUM_MAX,
} ble_evnet_type_e;

/**
 * @brief The event type of other module
 *
 */
typedef enum{
    BLE_CALLBACK_RS_START = 0,
    BLE_CALLBACK_RS_COMPLETE,
    BLE_CALLBACK_ROLE_UPDATE,
    BLE_CALLBACK_IBRT_EVENT_ENTRY,

    BLE_CALLBACK_EVENT_NUM_MAX,
} ble_callback_evnet_type_e;

typedef struct {
    ble_evnet_type_e evt_type;
    union {
        struct {
            uint8_t conidx;
            const uint8_t *peer_bdaddr;
        } connect_handled;
        struct {
            uint8_t conidx;
        } disconnect_handled;
        struct {
            /// Connection interval minimum
            uint16_t intv_min;
            /// Connection interval maximum
            uint16_t intv_max;
            /// Latency
            uint16_t latency;
            /// Supervision timeout
            uint16_t time_out;
        } conn_param_update_req_handled;
        struct {
            uint8_t *new_bdaddr;
        } set_random_bd_addr_handled;
    } p;
} ble_evnet_t;

typedef struct {
    ble_callback_evnet_type_e evt_type;
    union {
        struct {
            uint8_t newRole;
        } rs_complete_handled;
        struct {
            uint8_t newRole;
        } role_update_handled;
        struct {
            uint8_t event;
        } ibrt_event_entry_handled;
    } p;
} ble_callback_evnet_t;

typedef void (*APP_BLE_CORE_GLOBAL_HANDLER_FUNC)(ble_evnet_t *, void *);
typedef void (*APP_BLE_CORE_GLOBAL_CALLBACK_HANDLER_FUNC)(ble_callback_evnet_t *, void *);


// Element of a message handler table.
typedef struct {
    // evt_type of the ble event
    ble_evnet_type_e evt_type;
    // Pointer to the handler function for the ble event above.
    APP_BLE_CORE_GLOBAL_HANDLER_FUNC func;
} ble_event_handler_t;

// Element of a message handler table.
typedef struct {
    // evt_type of the ble callback event
    ble_callback_evnet_type_e evt_type;
    // Pointer to the handler function for the ble callback event above.
    APP_BLE_CORE_GLOBAL_CALLBACK_HANDLER_FUNC func;
} ble_callback_event_handler_t;

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *            app_ble_core_register_global_handler_ind
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to register custom handler function
 *
 * Parameters:
 *    handler -- custom handler function
 *
 * Return:
 *    void
 */
void app_ble_core_register_global_handler_ind(APP_BLE_CORE_GLOBAL_HANDLER_FUNC handler);

/*---------------------------------------------------------------------------
 *            app_ble_core_register_global_callback_handle_ind
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to register custom callback handler function
 *
 * Parameters:
 *    handler -- custom callback handler function
 *
 * Return:
 *    void
 */
void app_ble_core_register_global_callback_handle_ind(APP_BLE_CORE_GLOBAL_CALLBACK_HANDLER_FUNC handler);

/*---------------------------------------------------------------------------
 *            app_ble_core_global_handle
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble core to handle event
 *
 * Parameters:
 *    *event -- the event ble core need to handle
 *
 * Return:
 *    uint32_t
 */
void app_ble_core_global_handle(ble_evnet_t *event, void *output);

/*---------------------------------------------------------------------------
 *            app_ble_core_global_callback_event
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble core to handle callback event
 *
 * Parameters:
 *    *event -- the event ble core need to handle
 *
 * Return:
 *    void
 */
void app_ble_core_global_callback_event(ble_callback_evnet_t *event, void *output);

/*---------------------------------------------------------------------------
 *            app_ble_stub_user_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    None
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_stub_user_init(void);

/****************************function declearation**************************/
/*---------------------------------------------------------------------------
 *            app_ble_sync_ble_info
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for tws sync ble info
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_sync_ble_info(void);

/*---------------------------------------------------------------------------
 *            app_ble_mode_tws_sync_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    tws related environment initialization for ble module
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_mode_tws_sync_init(void);

/****************************function declearation**************************/
/*---------------------------------------------------------------------------
 *            ble_adv_data_parse
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for ble core to parse adv data
 *
 * Parameters:
 *    bleBdAddr -- ble address
 *    rssi      -- ble rssi
 *    adv_buf   -- adv data
 *    len       -- adv data length
 *
 * Return:
 *    void
 */
void ble_adv_data_parse(uint8_t *bleBdAddr,
                               int8_t rssi,
                               unsigned char *adv_buf,
                               unsigned char len);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_BLE_CORE_H__ */

