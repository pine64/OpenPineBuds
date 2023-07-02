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
#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#if !defined(ENHANCED_STACK)
#include "hci.h"
#endif

#include "ke_msg.h"

#if defined(ENHANCED_STACK)

typedef struct {
    uint8_t *buffer;
    uint16_t buffer_len;
    uint8_t *priv;
    uint16_t conn_handle_flags;
} BridgeBuffer;

typedef struct {
    U8 *param;
    U8 param_len;
    uint8_t event;
    BridgeBuffer *rx_buff;
} BridgeEvent;

#else

typedef HciEvent BridgeEvent;
typedef HciBuffer BridgeBuffer;

#endif /* ENHANCED_STACK */

void bridge_hcif_send_acl(struct ke_msg *msg);
void bridge_hcif_recv_acl(BridgeBuffer * pBuffer);
void bridge_free_rx_buffer(BridgeBuffer *pBuffer);
void bridge_free_tx_buffer(BridgeBuffer *pBuffer);
void bridge_hci_ble_event(const BridgeEvent* event);
uint8_t bridge_is_cmd_opcode_supported(uint16_t opcode);
void bridge_free_token(void * token);
U8 bridge_check_ble_handle_valid(U16 handle);
void bridge_hcif_send_cmd(struct ke_msg *msg);

#endif
