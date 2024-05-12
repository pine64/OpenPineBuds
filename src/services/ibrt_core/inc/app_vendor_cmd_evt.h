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
#ifndef __APP_VENDOR_EVT__
#define __APP_VENDOR_EVT__

#define GET_HCI_HANDLE(ptr)    ((unsigned short)(((unsigned short) *((ptr)+1) << 8) | ((unsigned short) *(ptr))))



//Sub code of Vendor event
#define HCI_DBG_TRACE_WARNING_EVT_CODE                0x01
#define HCI_SCO_SNIFFER_STATUS_EVT_CODE               0x02
#define HCI_ACL_SNIFFER_STATUS_EVT_CODE               0x03
#define HCI_TWS_EXCHANGE_CMP_EVT_CODE                 0x04
#define HCI_NOTIFY_CURRENT_ADDR_EVT_CODE              0x05
#define HCI_START_SWITCH_EVT_CODE                     0x09
#define HCI_LL_MONITOR_EVT_CODE                       0x0A
#define HCI_DBG_LMP_MESSAGE_RECORD_EVT_SUBCODE        0x0B
#define HCI_GET_TWS_SLAVE_MOBILE_RSSI_CODE            0x0C
#define HCI_DBG_IBRT_SWITCH_COMPLETE_EVT_SUBCODE      0x0D
#define HCI_DBG_IBRT_CONNECTED_EVT_SUBCODE            0x0E
#define HCI_DBG_IBRT_DISCONNECTED_EVT_SUBCODE         0x0f
#define HCI_DBG_SNIFFER_INIT_CMP_EVT_SUBCODE          0x10
#define HCI_DBG_RX_SEQ_ERROR_EVT_SUBCODE              0x11
#define HCI_DBG_FA_TO_EVT_SUBCODE                     0x14
#define HCI_DBG_ECC_DATA_TEST_EVT_SUBCODE             0x15
#define HCI_DBG_FRMERRIND_EVT_SUBCODE                 0x16
#define HCI_DBG_PKTERRIND_EVT_SUBCODE                 0x17


void app_hci_vendor_event_callback_init(void);
void app_hci_vendor_event_handler(uint8_t* pbuf, uint32_t length);
void btapp_sniffer_sco_start(uint16_t conhdl,uint8_t sco_status,uint8_t airmode,uint32_t bandwidth);
void btapp_sniffer_ibrt_op(uint16_t conhdl, uint8_t op, uint8_t status, uint8_t reason);
void app_ibrt_ui_rx_seq_error_handling_timer_cb(void const *current_evt);
void app_hci_vender_ibrt_disconnected_handle(uint8_t disc_reason);
void app_hci_vender_ibrt_fastack_to_handle(uint16_t mobile_conhdl);
#endif
