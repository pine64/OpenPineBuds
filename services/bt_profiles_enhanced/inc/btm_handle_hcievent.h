/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

#ifndef __BTM_HANDLE_HCIEVENT_H__
#define __BTM_HANDLE_HCIEVENT_H__

#include "btlib.h"
#include "btm_hci.h"
typedef bool (*ibrt_io_capbility_callback)(void *bdaddr);

void btm_event_handle(uint8 *param, uint8 *priv, uint8 *donot_free);
void btm_ble_acl_handle(uint16 conn_handle, uint8 *data_p, uint16 data_len, uint8 *priv, uint8 *donot_free);
void btm_acl_handle(uint16 conn_handle, uint8 *data_p, uint16 data_len, uint8 *priv, uint8 *donot_free);
void btm_sco_handle(uint16 conn_handle, uint8 *data_p, uint16 data_len, uint8 *priv, uint8 *donot_free);
void btm_process_conn_complete_evt(struct hci_evt_packet_t *pkt);
//void btm_process_conn_req_evt(struct hci_evt_packet_t *pkt);
int8 btm_create_acl_connection_fail_process(struct btm_conn_item_t *conn, uint8 status, struct bdaddr_t *bdaddr);

void btm_process_pin_code_req_evt(struct hci_evt_packet_t *pkt);
void btm_process_link_key_req_evt(struct hci_evt_packet_t *pkt);
void btm_process_link_key_notify_evt(struct hci_evt_packet_t *pkt);
void btm_process_authentication_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_simple_pairing_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_encryption_change_evt(struct hci_evt_packet_t *pkt);
void btm_process_remote_name_req_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_inquiry_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_inquiry_result_evt(struct hci_evt_packet_t *pkt, uint8 rssi, uint8 extinq);
void btm_process_mode_change_evt(struct hci_evt_packet_t *pkt);
void btm_process_acl_data_active_evt(void *conn, uint16_t len);
void btm_process_acl_data_not_active_evt(void *conn, uint16_t len);
void btm_process_num_of_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_read_remote_version_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_read_remote_supported_feature_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_read_remote_extended_feature_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_cmd_complete_inquiry_cancel (uint8 *data);
void btm_process_cmd_complete_remote_name_cancel(uint8 *data);
void btm_process_cmd_complete_evt(struct hci_evt_packet_t *pkt);
void btm_process_cmd_complete_read_buffer_size(uint8 *data);
void btm_process_return_linkkeys_evt (struct hci_evt_packet_t *pkt); 
void btm_acl_handle_nocopy(uint16 conn_handle, uint16 data_len, uint8 *data_p);
extern void hcile_acl_rx_data_received(uint16 conn_handle, uint16 data_len, uint8 *data_p);
extern int hci_no_operation_cmd_cmp_evt_handler(uint16_t opcode, void const *param);
extern uint8_t hcile_evt_received(uint8_t code, uint8_t length, uint8 *data_p);
void btm_process_vendor_evt(struct hci_evt_packet_t *pkt);

#endif /* __BTM_HANDLE_HCIEVENT_H__ */
