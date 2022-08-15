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

#ifndef __RFCOMM_H__
#define __RFCOMM_H__

#include "rfcomm_i.h"
#include "bt_co_list.h"

#define RFCOMM_CONN_TIMEOUT //(HZ * 30)
#define RFCOMM_DISC_TIMEOUT //(HZ * 20)
#define RFCOMM_AUTH_TIMEOUT //(HZ * 25)

#define RFCOMM_DEFAULT_MTU      672
#define RFCOMM_DEFAULT_CREDITS  7

#define RFCOMM_MAX_CREDITS	40

#define RFCOMM_PPB_HEAD_RESERVE	8
#define RFCOMM_PPB_TAIL_RESERVE	2
#define RFCOMM_PPB_RESERVE  (RFCOMM_PPB_HEAD_RESERVE + RFCOMM_PPB_TAIL_RESERVE)

#define RFCOMM_SABM	0x2f
#define RFCOMM_DISC	0x43
#define RFCOMM_UA	0x63
#define RFCOMM_DM	0x0f
#define RFCOMM_UIH	0xef

#define RFCOMM_TEST	0x08
#define RFCOMM_FCON	0x28
#define RFCOMM_FCOFF	0x18
#define RFCOMM_MSC	0x38
#define RFCOMM_RPN	0x24
#define RFCOMM_RLS	0x14
#define RFCOMM_PN	0x20
#define RFCOMM_NSC	0x04

#define RFCOMM_V24_FC	0x02
#define RFCOMM_V24_RTC	0x04
#define RFCOMM_V24_RTR	0x08
#define RFCOMM_V24_IC	0x40
#define RFCOMM_V24_DV	0x80

#define RFCOMM_RPN_BR_2400	0x0
#define RFCOMM_RPN_BR_4800	0x1
#define RFCOMM_RPN_BR_7200	0x2
#define RFCOMM_RPN_BR_9600	0x3
#define RFCOMM_RPN_BR_19200	0x4
#define RFCOMM_RPN_BR_38400	0x5
#define RFCOMM_RPN_BR_57600	0x6
#define RFCOMM_RPN_BR_115200	0x7
#define RFCOMM_RPN_BR_230400	0x8

#define RFCOMM_RPN_DATA_5	0x0
#define RFCOMM_RPN_DATA_6	0x1
#define RFCOMM_RPN_DATA_7	0x2
#define RFCOMM_RPN_DATA_8	0x3

#define RFCOMM_RPN_STOP_1	0
#define RFCOMM_RPN_STOP_15	1

#define RFCOMM_RPN_PARITY_NONE	0x0
#define RFCOMM_RPN_PARITY_ODD	0x1
#define RFCOMM_RPN_PARITY_EVEN	0x3
#define RFCOMM_RPN_PARITY_MARK	0x5
#define RFCOMM_RPN_PARITY_SPACE	0x7

#define RFCOMM_RPN_FLOW_NONE	0x00

#define RFCOMM_RPN_XON_CHAR	0x11
#define RFCOMM_RPN_XOFF_CHAR	0x13

#define RFCOMM_RPN_PM_BITRATE		0x0001
#define RFCOMM_RPN_PM_DATA		0x0002
#define RFCOMM_RPN_PM_STOP		0x0004
#define RFCOMM_RPN_PM_PARITY		0x0008
#define RFCOMM_RPN_PM_PARITY_TYPE	0x0010
#define RFCOMM_RPN_PM_XON		0x0020
#define RFCOMM_RPN_PM_XOFF		0x0040
#define RFCOMM_RPN_PM_FLOW		0x3F00

#define RFCOMM_RPN_PM_ALL		0x3F7F

#define RFCOMM_HANDLE_UNUSED        0xffffffff
#define RFCOMM_HANDLE_MAX_VALUE     0xffffff00


struct rfcomm_hdr {
	uint8 addr;
	uint8 ctrl;
	uint8 len;    // Actual size can be 2 bytes
}__attribute__ ((packed));

struct rfcomm_cmd {
	uint8 addr;
	uint8 ctrl;
	uint8 len;
	uint8 fcs;
}__attribute__ ((packed));

struct rfcomm_mcc {
	uint8 type;
	uint8 len;
}__attribute__ ((packed));

struct rfcomm_pn {
	uint8  dlci;
	uint8  flow_ctrl;
	uint8  priority;
	uint8  ack_timer;
	uint16 mtu;
	uint8  max_retrans;
	uint8  credits;
}__attribute__ ((packed));

struct rfcomm_rpn {
	uint8  dlci;
	uint8  bit_rate;
	uint8  line_settings;
	uint8  flow_ctrl;
	uint8  xon_char;
	uint8  xoff_char;
	uint16 param_mask;
}__attribute__ ((packed));

struct rfcomm_rls {
	uint8  dlci;
	uint8  status;
}__attribute__ ((packed));

struct rfcomm_msc {
	uint8  dlci;
	uint8  v24_sig;
}__attribute__ ((packed));

enum rfcomm_session_state_enum {
    SESSION_CLOSE,          /*l2cap connection closed, wait for openning, and then can send out sabm request*/
    SESSION_CONNECT,      /*l2cap channel created*/
    SESSION_WAITING_OPEN, /*has sent sabm of dlci 0, wait ack*/
    SESSION_OPEN,            /* rfcomm session created, means dlci 0 created ok */
    SESSION_CLOSING       /* all dlc disced, dlci 0 wait to close */
};

struct rfcomm_session {
    struct list_node list;

    uint8              initiator;
    uint32          l2cap_handle;

    enum rfcomm_session_state_enum state;

    struct bdaddr_t remote;
    struct list_node dlcs;
};


enum rfcomm_dlc_state_enum {
    DLC_CLOSE,
    DLC_CONFIG,     /*in dlc parameter config process*/
    DLC_CONNECTING, /* config passed, then send sabm,waiting for ack */
    DLC_OPEN,
    DLC_DISCONNECT /*in dlc disconnection process*/
};

struct rfcomm_dlc {

    struct list_node      list;
    struct rfcomm_session *session;
    struct pp_buff_head   tx_queue;

    uint32          rfcomm_handle;
    uint8            dlci;
    uint8            addr;
    bool           local_trx_ready;
    bool           peer_trx_ready;

    uint16          mtu;
    uint8            priority;
    uint8            v24_sig;
    uint8            cfc;   /* 0: no flow control; other: the credits we give remote;*/

    uint8          rx_credits;  /*the remote device's tx credits now*/
    uint8          tx_credits;  /*our tx credits*/
    uint8          credit_give_limit;
    osMutexId      creditMutex; //!< used to manage the remote device's tx credit

    enum rfcomm_dlc_state_enum state;

    rfcomm_notify_callback_t rfcomm_notify_callback;
    rfcomm_datarecv_callback_t rfcomm_datarecv_callback;

    /*
     * for used by uplayer to it's own private info, mainly used for
     * hfp channel address, hsp channel, spp channel, etc...
     */
    void *priv;
    struct {
        void *priv;
    } context;
};

struct rfcomm_registered_server_item_t {
    struct list_node list;
    int8 server_channel;
    osMutexId creditMutex;
    uint8 initial_credits;
    uint8 credit_give_limit;
    rfcomm_notify_callback_t rfcomm_notify_callback;
    rfcomm_datarecv_callback_t rfcomm_datarecv_callback;
    struct rfcomm_dlc *dlc;
    /*
     * used for server's private info
     */
    void *priv;
};

/* flow control states */
#define RFCOMM_CFC_DISABLED 0
#define RFCOMM_CFC_ENABLED  RFCOMM_DEFAULT_CREDITS

#define RFCOMM_CFG_SESSIONS_MAX   5    /* means how many l2cap channel */

#endif /* __RFCOMM_H__ */