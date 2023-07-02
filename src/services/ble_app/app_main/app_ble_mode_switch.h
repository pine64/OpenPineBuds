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

#ifndef __APP_BLE_MODE_SWITCH_H__
#define __APP_BLE_MODE_SWITCH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************header include********************************/
#include "bluetooth.h"
#include "co_bt_defines.h"

/******************************macro defination*****************************/
#define BLE_ADV_DATA_STRUCT_HEADER_LEN (2)

#ifndef BLE_CONNECTION_MAX
#define BLE_CONNECTION_MAX (1)
#endif

// the default interval is 160ms, note that for Bisto user case, to
// let GVA iOS version pop-out notification smoothly, the maximum interval should be this value
#define BLE_ADVERTISING_INTERVAL (160)
#define BLE_FAST_ADVERTISING_INTERVAL (48)

#define BLE_ADV_SVC_FLAG  0x16
#define BLE_ADV_MANU_FLAG 0xFF

// Maximal length of the Device Name value
#define APP_DEVICE_NAME_MAX_LEN      (24)

/******************************type defination******************************/
/**
 * @brief The state type of the ble
 *
 */
enum BLE_STATE_E {
    STATE_IDLE       = 0,
    ADVERTISING      = 1,
    STARTING_ADV     = 2,
    STOPPING_ADV     = 3,
    SCANNING         = 4,
    STARTING_SCAN    = 5,
    STOPPING_SCAN    = 6,
    CONNECTING       = 7,
    STARTING_CONNECT = 8,
    STOPPING_CONNECT = 9,
};

/**
 * @brief The operation type of the ble
 *
 */
enum BLE_OP_E {
    OP_IDLE       = 0,
    START_ADV     = 1,
    START_SCAN    = 2,
    START_CONNECT = 3,
    STOP_ADV      = 4,
    STOP_SCAN     = 5,
    STOP_CONNECT  = 6,
};

enum BLE_ADV_USER_E {
    USER_STUB            = 0,
    USER_GFPS            = 1,
    USER_GSOUND          = 2,
    USER_AI              = 3,
    USER_INTERCONNECTION = 4,
    USER_TILE            = 5,
    USER_OTA             = 6,
    BLE_ADV_USER_NUM,
};

enum BLE_ADV_SWITCH_USER_E {
    BLE_SWITCH_USER_RS          = 0, // used for role switch
    BLE_SWITCH_USER_BOX         = 1, // used for box open/close
    BLE_SWITCH_USER_AI          = 2, // used for ai
    BLE_SWITCH_USER_BT_CONNECT  = 3, // used for bt connect
    BLE_SWITCH_USER_SCO         = 4, // used for sco
    BLE_SWITCH_USER_IBRT        = 5, // used for ibrt

    BLE_SWITCH_USER_NUM,
};

//Scan filter policy
enum BLE_SCAN_FILTER_POLICY {
    ///Allow advertising packets from anyone
    BLE_SCAN_ALLOW_ADV_ALL            = 0x00,
    ///Allow advertising packets from White List devices only
    BLE_SCAN_ALLOW_ADV_WLST,
    ///Allow advertising packets from anyone and Direct adv using RPA in InitA
    BLE_SCAN_ALLOW_ADV_ALL_AND_INIT_RPA,
    ///Allow advertising packets from White List devices only and Direct adv using RPA in InitA
    BLE_SCAN_ALLOW_ADV_WLST_AND_INIT_RPA,
};

typedef struct {
    bool withFlag;
    uint8_t advType;
    uint16_t advInterval;
    uint8_t advDataLen;
    uint8_t advData[BLE_DATA_LEN];
    uint8_t scanRspDataLen;
    uint8_t scanRspData[BLE_DATA_LEN];
} BLE_ADV_PARAM_T;

typedef void (*BLE_DATA_FILL_FUNC_T)(void *advParam);

typedef struct {
    uint8_t scanType;
    uint16_t scanWindow;
    uint16_t scanInterval;
} BLE_SCAN_PARAM_T;

typedef struct {
    uint32_t advSwitch;
    uint8_t state;
    uint8_t op;
    uint8_t bleAddrToConnect[BTIF_BD_ADDR_SIZE];
    uint32_t adv_user_register; //one bit represent one user
    uint32_t adv_user_enable;   //one bit represent one user

    BLE_DATA_FILL_FUNC_T bleDataFillFunc[BLE_ADV_USER_NUM];

    // param used for BLE adv
    BLE_ADV_PARAM_T advInfo;

    // prarm used for BLE scan
    BLE_SCAN_PARAM_T scanInfo;

    // pointer of @see app_env
    struct app_env_tag *bleEnv;
} __attribute__((__packed__)) BLE_MODE_ENV_T;


/*---------------------------------------------------------------------------
 *            app_ble_mode_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    init the bleModeEnv
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_mode_init(void);

/*---------------------------------------------------------------------------
 *            app_ble_register_data_fill_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    register a BLE advertisement and scan response data fill handler for a
 *    specific user(see @BLE_ADV_USER_E), so that the adv/scan response data
 *    could present in BLE adv/scan response data
 *
 * Parameters:
 *    user - see the defination in BLE_ADV_USER_E
 *    func - adv/scan response data fill handler for specific user
 *
 * Return:
 *    void
 */
void app_ble_register_data_fill_handle(enum BLE_ADV_USER_E user, BLE_DATA_FILL_FUNC_T func, bool enable);

/*---------------------------------------------------------------------------
 *            app_ble_data_fill_enable
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    enable/disable specific user to fill the adv/scan response data
 *
 * Parameters:
 *    user : user to enable/disable fill data
 *    enable : true - enable user
 *             false - disable user
 *
 * Return:
 *    void
 */
void app_ble_data_fill_enable(enum BLE_ADV_USER_E user, bool enable);

/*---------------------------------------------------------------------------
 *            app_ble_get_data_fill_enable
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get if specific user is enabled to fill adv/scan response data
 *
 * Parameters:
 *    user : user
 *
 * Return:
 *    true - user is enabled to fill data
 *    false - user is disabled to fill data
 */
bool app_ble_get_data_fill_enable(enum BLE_ADV_USER_E user);

/*---------------------------------------------------------------------------
 *            app_ble_start_connectable_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start connetable BLE advertise
 *
 * Parameters:
 *    advertisement interval in ms
 *
 * Return:
 *    None
 */
void app_ble_start_connectable_adv(uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            app_ble_refresh_adv_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    refresh adv state
 *
 * Parameters:
 *    advertisement interval in ms
 *
 * Return:
 *    None
 */
void app_ble_refresh_adv_state(uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            app_ble_force_switch_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    enable/disable all BLE adv request for specific UI user
 *
 * Parameters:
 *    user : UI user
 *    onOff : true - enable BLE adv for specific UI user
 *            false -  disable BLE adv for specific UI user
 *
 * Return:
 *    void
 */
void app_ble_force_switch_adv(uint8_t user, bool onOff);

/*---------------------------------------------------------------------------
 *            app_ble_start_scan
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE scan
 *
 * Parameters:
 *    scanFilterPolicy : Scan filter policy
 *    scanWindow : BLE scan window size(in ms)
 *    scanWnInterval : BLE scan window interval(in ms)
 *
 * Return:
 *    void
 */
void app_ble_start_scan(enum BLE_SCAN_FILTER_POLICY scanFilterPolicy, uint16_t scanWindow, uint16_t scanWnterval);

/*---------------------------------------------------------------------------
 *            app_ble_start_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start connect BLE with the given address
 *
 * Parameters:
 *    bdAddrToConnect : BLE address to connnect
 *
 * Return:
 *    void
 */
void app_ble_start_connect(uint8_t *bdAddrToConnect);

/*---------------------------------------------------------------------------
 *            app_ble_is_connection_on
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    is a specific BLE connection exist
 *
 * Parameters:
 *    index: Index of the BLE connection to check
 *
 * Return:
 *    true - BLE connection exists
 *    false - BLE connection doesn't exist
 */
bool app_ble_is_connection_on(uint8_t index);

/*---------------------------------------------------------------------------
 *            app_ble_is_any_connection_exist
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    is there any BLE connection exist
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - at least one BLE connection exist
 *    false - no BLE connection exists
 */
bool app_ble_is_any_connection_exist();

/*---------------------------------------------------------------------------
 *            app_ble_start_disconnect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    disconnect the BLE connection with given connection index
 *
 * Parameters:
 *    conIdx: connection index to disconnect
 *
 * Return:
 *    void
 */
void app_ble_start_disconnect(uint8_t conIdx);

/*---------------------------------------------------------------------------
 *            app_ble_disconnect_all
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    disconnect all BLE connections
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_disconnect_all(void);

/*---------------------------------------------------------------------------
 *            app_ble_stop_activities
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop all BLE activities
 *    NOTE: will not disconnect the existed connections
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_stop_activities(void);

/*---------------------------------------------------------------------------
 *            app_ble_is_in_advertising_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    is BLE in advertising progress
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - BLE adv is in progress
 *    false - BLE adv is not in progress
 */
bool app_ble_is_in_advertising_state(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_user_register
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get adv user register
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint32_t -- adv user register
 */
uint32_t app_ble_get_user_register(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_current_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get current ble state
 *
 * Parameters:
 *    void
 *
 * Return:
 *    enum BLE_STATE_E -- ble state
 */
enum BLE_STATE_E app_ble_get_current_state(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_current_operation
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get current ble operation
 *
 * Parameters:
 *    void
 *
 * Return:
 *    enum BLE_OP_E -- ble operation
 */
enum BLE_OP_E app_ble_get_current_operation(void);

/*---------------------------------------------------------------------------
 *            app_ble_get_runtime_adv_param
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get current ble advertising parameters
 *
 * Parameters:
 *    pAdvType: Output pointer of adv type
 *    pAdvIntervalMs: Output pointer of adv internal in ms
 *
 * Return:
 *    void
 */
void app_ble_get_runtime_adv_param(uint8_t* pAdvType, uint16_t* pAdvIntervalMs);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_BLE_MODE_SWITCH_H__ */
