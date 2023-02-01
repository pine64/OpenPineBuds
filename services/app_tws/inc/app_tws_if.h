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

#ifndef __APP_TWS_IF_H__
#define __APP_TWS_IF_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************header include********************************/

/******************************macro defination*****************************/
#define TWS_SYNC_BUF_SIZE 300

/******************************type defination******************************/
typedef enum {
  EAR_SIDE_UNKNOWN = 0,
  EAR_SIDE_LEFT = 1,
  EAR_SIDE_RIGHT = 2,
  EAR_SIDE_NUM,
} APP_TWS_SIDE_T;

enum {
  DISCONNECTED = 0,
  CONNECTED = 1,

  CONNECTION_STATE_NUM,
};

typedef enum {
  TWS_SYNC_USER_BLE_INFO = 0,
  TWS_SYNC_USER_OTA = 1,
  TWS_SYNC_USER_AI_CONNECTION = 2,
  TWS_SYNC_USER_GFPS_INFO = 3,
  TWS_SYNC_USER_AI_INFO = 4,
  TWS_SYNC_USER_AI_MANAGER = 5,
  TWS_SYNC_USER_DIP = 6,

  TWS_SYNC_USER_NUM,
} TWS_SYNC_USER_E;

typedef void (*TWS_SYNC_INFO_PREPARE_FUNC_T)(uint8_t *buf, uint16_t *len);
typedef void (*TWS_INFO_SYNC_FUNC_T)(uint8_t *buf, uint16_t len);
typedef struct {
  TWS_SYNC_INFO_PREPARE_FUNC_T sync_info_prepare_handler;
  TWS_INFO_SYNC_FUNC_T sync_info_received_handler;
  TWS_SYNC_INFO_PREPARE_FUNC_T sync_info_prepare_rsp_handler;
  TWS_INFO_SYNC_FUNC_T sync_info_rsp_received_handler;
  TWS_INFO_SYNC_FUNC_T sync_info_rsp_timeout_handler;
} TWS_SYNC_USER_T;

typedef struct {
  TWS_SYNC_USER_T syncUser[TWS_SYNC_USER_NUM];
} TWS_ENV_T;

typedef struct {
  TWS_SYNC_USER_E userId;
  uint8_t info[TWS_SYNC_BUF_SIZE - 1];
} TWS_SYNC_DATA_T;

/****************************function declearation**************************/
/*---------------------------------------------------------------------------
 *            app_tws_if_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    initialize the tws interface related parameter
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_init(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_role_switch_started_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler for role switch started event of tws system
 *
 * Parameters:
 *    void
 *
 * Return:
 *    voide
 */
void app_tws_if_role_switch_started_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_tws_role_switch_complete_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    callback function of role switch complete event for tws system
 *    NOTE: tws system include relay_tws and IBRT
 *
 * Parameters:
 *    newRole - current role of device after role switch complete
 *
 * Return:
 *    void
 */
void app_tws_if_tws_role_switch_complete_handler(uint8_t newRole);

/*---------------------------------------------------------------------------
 *            app_tws_if_tws_role_updated_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    callback function of tws role updated event for tws system
 *    NOTE: tws system include relay_tws and IBRT
 *
 * Parameters:
 *    newRole - current role of device after role updated
 *
 * Return:
 *    void
 */
void app_tws_if_tws_role_updated_handler(uint8_t newRole);

/*---------------------------------------------------------------------------
 *            app_tws_if_sync_info_received_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler for tws common info received event
 *    NOTE: see @ to get more info for common info received
 *
 * Parameters:
 *    rsp_seq - sequence number of response
 *    p_buff - pointer for received data
 *    lenght - length of received data
 *
 * Return:
 *    void
 */
void app_tws_if_sync_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                           uint16_t length);

/*---------------------------------------------------------------------------
 *            app_tws_if_common_info_rsp_recieved_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    shared common info response received handler
 *
 * Parameters:
 *    rsp_seq - sequence number of response
 *    p_buff - pointer for received data
 *    lenght - length of received data
 *
 * Return:
 *    void
 */
void app_tws_if_sync_info_rsp_received_handler(uint16_t rsp_seq,
                                               uint8_t *p_buff,
                                               uint16_t length);

/*---------------------------------------------------------------------------
 *            app_tws_if_sync_info_rsp_timeout_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    shared common info response timeout handler
 *
 * Parameters:
 *    rsp_seq - sequence number of response
 *    p_buff - pointer for received data
 *    lenght - length of received data
 *
 * Return:
 *    void
 */
void app_tws_if_sync_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                              uint16_t length);

/*---------------------------------------------------------------------------
 *            app_tws_if_mobile_connected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of tws connected with mobile phone
 *
 * Parameters:
 *    addr - connected mobile address
 *
 * Return:
 *    void
 */
void app_tws_if_mobile_connected_handler(uint8_t *addr);

/*---------------------------------------------------------------------------
 *            app_tws_if_mobile_disconnected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of tws disconnected with mobile phone
 *
 * Parameters:
 *    addr - disconnected mobile phone
 *
 * Return:
 *    void
 */
void app_tws_if_mobile_disconnected_handler(uint8_t *addr);

/*---------------------------------------------------------------------------
 *            app_tws_if_ibrt_connected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of ibrt connected
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_ibrt_connected_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_ibrt_disconnected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of ibrt disconnected
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_ibrt_disconnected_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_ble_connected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of ble connected event for tws system
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_ble_connected_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_ble_disconnected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    ble disconnected handler for tws system
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_ble_disconnected_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_tws_connected_sync_info
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    sync BLE\BISTO\AI info after tws connected
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_tws_connected_sync_info(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_tws_connected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of tws connected event
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_tws_connected_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_tws_disconnected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of tws disconnected event
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_tws_disconnected_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_trigger_role_switch
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    role switch trigger for tws system
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_trigger_role_switch(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_handle_click
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handle the power key click event
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_handle_click(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_master_prepare_rs
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    master prepare for role switch for BISTO and AI
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_master_prepare_rs(uint8_t *p_buff, uint16_t length);

#ifdef IBRT
/*---------------------------------------------------------------------------
 *            app_tws_if_slave_continue_rs
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    slave continue role switch progress
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_slave_continue_rs(uint8_t *p_buff, uint16_t length);
#endif

/*---------------------------------------------------------------------------
 *            app_tws_if_register_sync_user
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    register info sync user for tws system
 *
 * Parameters:
 *    id - user ID, see @TWS_SYNC_USER_E
 *    user - user related functions
 *
 * Return:
 *    void
 */
void app_tws_if_register_sync_user(uint8_t id, TWS_SYNC_USER_T *user);

/*---------------------------------------------------------------------------
 *            app_tws_if_deregister_sync_user
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    deregister info sync user for tws system
 *
 * Parameters:
 *   id - id of user to deregister, see @TWS_SYNC_USER_E
 *
 * Return:
 *    void
 */
void app_tws_if_deregister_sync_user(uint8_t id);

/*---------------------------------------------------------------------------
 *            app_tws_if_sync_info
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    sync tws if
 *
 * Parameters:
 *    id - user ID
 *
 * Return:
 *    void
 */
void app_tws_if_sync_info(TWS_SYNC_USER_E id);

void app_tws_if_ai_send_cmd_to_peer(uint8_t *p_buff, uint16_t length);
void app_tws_if_ai_rev_peer_cmd_hanlder(uint16_t rsp_seq, uint8_t *p_buff,
                                        uint16_t length);
void app_tws_if_ai_send_cmd_with_rsp_to_peer(uint8_t *p_buff, uint16_t length);
void app_tws_if_ai_send_cmd_rsp_to_peer(uint8_t *p_buff, uint16_t rsp_seq,
                                        uint16_t length);
void app_tws_if_ai_rev_peer_cmd_with_rsp_hanlder(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length);
void app_tws_if_ai_rev_cmd_rsp_from_peer_hanlder(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length);
void app_tws_if_ai_rev_cmd_rsp_timeout_hanlder(uint16_t rsp_seq,
                                               uint8_t *p_buff,
                                               uint16_t length);

#ifdef GFPS_ENABLED
void app_ibrt_share_fastpair_info(uint8_t *p_buff, uint16_t length);
void app_tws_send_fastpair_info_to_slave(void);
void app_ibrt_shared_fastpair_info_received_handler(uint16_t rsp_seq,
                                                    uint8_t *p_buff,
                                                    uint16_t length);
#endif

bool app_tws_is_master_mode(void);
bool app_tws_is_slave_mode(void);
bool app_tws_is_freeman_mode(void);
void app_ibrt_sync_volume_info(void);
bool app_tws_is_left_side(void);
bool app_tws_is_right_side(void);
bool app_tws_is_unknown_side(void);
void app_tws_set_side_from_addr(uint8_t *addr);
bool app_tws_nv_is_master_role(void);
void app_tws_set_side_from_gpio(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_TWS_IF_H__ */
