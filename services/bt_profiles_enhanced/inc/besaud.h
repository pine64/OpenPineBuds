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

#ifndef __BESAUD_H__
#define __BESAUD_H__

#include "bluetooth.h"
#include "bt_co_list.h"
#include "btm.h"
#include "btlib_type.h"

#define BESAUD_PACKET_COUNT (5)

typedef struct _BesaudCallbackParms BesaudCallbackParms;
typedef struct _BesaudChannel   BesaudChannel;

typedef struct _BesaudConn BesaudConn;
typedef struct _BesaudConnCallbackParms BesaudConnCallbackParms;

typedef void (*BesaudConnCallback)(BesaudConn *Conn, BesaudConnCallbackParms *Parms);

/* Connection State */
struct _BesaudConn {
    uint32 l2cap_handle;
    uint8                state;
    BesaudConnCallback callback;
};

struct _BesaudConnCallbackParms {
    uint8         event;
    int8          status;
    uint16        dataLen;
    union {
        struct btm_conn_item_t *remDev;
        uint8             *data;
    } ptrs;
};

int8 BesaudDisconnect(struct btm_conn_item_t *RemDev);
BOOL BesaudIsConnected(BesaudConn *Conn);
int8 BesaudConnect(BesaudChannel *chnl, struct btm_conn_item_t *RemDev);

typedef uint16 BesaudEvent;

typedef struct
{
    struct list_node node;
	uint8* 		pBuf;
	uint16		pDataLength;
} BesaudPacket;

struct _BesaudCallbackParms{
    BesaudEvent    event;

    int8    status;
    int8    errCode;
    BesaudChannel *chnl;
    union{
        struct btm_conn_item_t *remDev;
        BesaudPacket   Packet;
    }p;
};

/*--------------------------------------------------------------------------
 * BESAUDChannelStates type
 *
 *     This type enumerates the possible BESAUD channel connection
 *     states.
 */
typedef uint8 BesaudChannelStates;

/* End of HfChannelStates */

typedef void (*BesaudCallback)(BesaudChannel *Chan, BesaudCallbackParms *Info);


struct _BesaudChannel{
    struct list_node node;
    struct btm_conn_item_t *remDev;
    BesaudCallback          callback;         /* Application callback*/
    BesaudConn          besaudc_conn;
    BesaudChannelStates     state;           /* Current connection state      */
    uint16                 flags;           /* Current connection flags      */
    struct list_node freeTxPacketList;
    struct list_node pendingTxPacketList;
    uint8    initiator;
    uint8    tx_state;
    BesaudPacket *curr_tx_packet;
};

#ifndef BESAUDC_MAX_MTU
#define BESAUDC_MAX_MTU L2CAP_MTU
#endif


#ifndef BESAUDI_MAX_MTU
#define BESAUDI_MAX_MTU L2CAP_MTU
#endif


////besaud channel state
#define BESAUD_STATE_DISCONNECTED   0
#define BESAUD_STATE_CONN_PENDING   1
#define BESAUD_STATE_CONN_INCOMING  2
#define BESAUD_STATE_DISC_PENDING   3
#define BESAUD_STATE_DISC_INCOMING  4
#define BESAUD_STATE_CONNECTED      5

////channel tx state
#define BESAUD_TX_STATE_IDLE 0
#define BESAUD_TX_STATE_IN_TX 1

#define BESAUD_REPORT_TYPE_INPUT     1
#define BESAUD_REPORT_TYPE_OUTPUT     2
#define BESAUD_REPORT_TYPE_FEATURE     3



#define BESAUD_TRANS_TYPE_HANDSHAKE    0
#define BESAUD_TRANS_TYPE_CONTROL        1
#define BESAUD_TRANS_TYPE_GET_REPORT        4
#define BESAUD_TRANS_TYPE_SET_REPORT        5
#define BESAUD_TRANS_TYPE_GET_PROTOCAL        6
#define BESAUD_TRANS_TYPE_SET_PROTOCAL        7
#define BESAUD_TRANS_TYPE_GET_IDLE        8
#define BESAUD_TRANS_TYPE_SET_IDLE        9
#define BESAUD_TRANS_TYPE_DATA        10
#define BESAUD_TRANS_TYPE_DATAC        11


#define BESAUD_HANDSHAKE_SUCCESS    0
#define BESAUD_HANDSHAKE_NOT_READY    1
#define BESAUD_HANDSHAKE_INVALID_REPORTID    2
#define BESAUD_HANDSHAKE_UNSUPPORT_REQUEST    3
#define BESAUD_HANDSHAKE_INVALID_PARAM   4
#define BESAUD_HANDSHAKE_ERROR_UNKNOWN    0xe
#define BESAUD_HANDSHAKE_ERROR_FATAL   0xF


#define BESAUD_CONTROL_NOP        0
#define BESAUD_CONTROL_HARD_RESET   1
#define BESAUD_CONTROL_SOFT_RESET   2
#define BESAUD_CONTROL_SUSPEND   3
#define BESAUD_CONTROL_EXIT_SUSPEND   4
#define BESAUD_CONTROL_VIRTUAL_CABLE_UNPLUG   5

#define BESAUD_CHANNEL_TYPE_INTERRUPT     0
#define BESAUD_CHANNEL_TYPE_CONTROL     1



#define BESAUD_DATA_TXSTATE_IDLE            0
#define BESAUD_DATA_TXSTATE_SEND            1



#define BESAUD_EVENT_CONTROL_CONNECTED               0x21

#define BESAUD_EVENT_CONTROL_DISCONNECTED         0x22

#define BESAUD_EVENT_CONTROL_DATA_IND                0x23

#define BESAUD_EVENT_CONTROL_DATA_SENT              0x24


#define BESAUD_EVENT_CONTROL_SET_IDLE              0x30

#if defined(__cplusplus)
extern "C" {
#endif

int8 BESAUD_Init(void);
int8 Besaud_Register(BesaudChannel *chnl,
                        BesaudCallback    callback
                        );
int8 Besaud_Connect(BesaudChannel *chnl, struct btm_conn_item_t *RemDev);
int8 Besaud_Disconnect(BesaudChannel *chnl);
int8 Besaud_Send_packet(BesaudChannel *chnl, char *buffer, uint16 nBytes);
int8 Besaud_Send_cmd_packet(BesaudChannel *chnl, char *buffer, uint16 nBytes);

#if defined(__cplusplus)
}
#endif

#endif /* __BESAUD_H__ */