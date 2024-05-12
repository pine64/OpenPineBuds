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
#ifdef __GATT_OVER_BR_EDR__
#ifndef __BTGATT_H__
#define __BTGATT_H__
#include "cmsis_os.h"
#include "stdbool.h"
#include "bt_co_list.h"
#include "btlib_type.h"
#include "btm.h"

#define BTGATT_PACKET_COUNT (5)

typedef struct _BtgattCallbackParms BtgattCallbackParms;
typedef struct _BtgattChannel   BtgattChannel;
typedef struct _BtgattConn BtgattConn;
typedef struct _BtgattConnCallbackParms BtgattConnCallbackParms;

typedef void (*BtgattConnCallback)(BtgattConn *Conn, BtgattConnCallbackParms *Parms);

/* Connection State */
struct _BtgattConn {
    uint32              l2cap_handle;
    uint8               state;
    BtgattConnCallback  callback;
};

struct _BtgattConnCallbackParms {
    uint8         event;
    int8          status;
    uint16        dataLen;
    union {
        struct btm_conn_item_t *remDev;
        uint8             *data;
    } ptrs;
};

typedef uint16 BtgattEvent;

typedef struct
{
    struct list_node    node;
    uint8*              pBuf;
    uint16              pDataLength;
} BtgattPacket;

struct _BtgattCallbackParms{
    BtgattEvent    event;

    int8    status;
    int8    errCode;
    BtgattChannel *chnl;
    union{
        struct btm_conn_item_t *remDev;
        BtgattPacket   Packet;
    }p;
};

typedef struct _BtBtgattContext {
    struct list_node channelList;
    uint16 btgattcpsm;
} BtBtgattContext;

/*--------------------------------------------------------------------------
 * BTGATTChannelStates type
 *
 *     This type enumerates the possible BTGATT channel connection
 *     states.
 */
typedef uint8 BtgattChannelStates;

/* End of HfChannelStates */
typedef void (*BtgattCallback)(BtgattChannel *Chan, BtgattCallbackParms *Info);

struct _BtgattChannel{
    struct list_node        node;
    struct btm_conn_item_t  *remDev;
    BtgattCallback          callback;           /* Application callback*/
    BtgattConn              btgattc_conn;
    BtgattChannelStates     state;              /* Current connection state      */
    uint16                  flags;              /* Current connection flags      */
    struct list_node        freeTxPacketList;
    struct list_node        pendingTxPacketList;
    uint8                   initiator;
    uint8                   tx_state;
    BtgattPacket            *curr_tx_packet;
};

////btgatt channel state
#define BTGATT_STATE_DISCONNECTED   0
#define BTGATT_STATE_CONN_PENDING   1
#define BTGATT_STATE_CONN_INCOMING  2
#define BTGATT_STATE_DISC_PENDING   3
#define BTGATT_STATE_DISC_INCOMING  4
#define BTGATT_STATE_CONNECTED      5

////channel tx state
#define BTGATT_TX_STATE_IDLE 0
#define BTGATT_TX_STATE_IN_TX 1


#define BTGATT_EVENT_CONTROL_CONNECTED      0x21
#define BTGATT_EVENT_CONTROL_DISCONNECTED   0x22
#define BTGATT_EVENT_CONTROL_DATA_IND       0x23
#define BTGATT_EVENT_CONTROL_DATA_SENT      0x24

#define BTGATT(s) (btgattContext.s)

#if defined(__cplusplus)
extern "C" {
#endif
int8 Btgatt_init(void);
int8 Btgatt_Register(BtgattChannel *chnl, BtgattCallback callback);
int8 Btgatt_Connect(BtgattChannel *chnl, struct btm_conn_item_t *RemDev);
int8 Btgatt_Disconnect(BtgattChannel *chnl);
int8 Btgatt_Send_packet(BtgattChannel *chnl, char *buffer, uint16 nBytes);
int8 Btgatt_Send_cmd_packet(BtgattChannel *chnl, char *buffer, uint16 nBytes);
bool Btgatt_Is_Connected(BtgattConn *Conn);
void Btgatt_addsdp(uint16_t pServiceUUID, uint16_t startHandle, uint16_t endHandle);
#if defined(__cplusplus)
}
#endif

#endif /* __BTGATT_H__ */
#endif
