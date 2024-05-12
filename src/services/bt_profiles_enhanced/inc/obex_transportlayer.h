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
#ifndef OBEX_TRANSPORTLAYER_H_INCLUDED
#define OBEX_TRANSPORTLAYER_H_INCLUDED

#include "bt_common.h"
#include "bt_co_list.h"
#include "rfcomm_i.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OBEX_TRANSPORT_TYPE_RFCOMM = 0,
    OBEX_TRANSPORT_TYPE_L2CAP, // 1
} obex_transport_type_t;

typedef enum {
    OBEX_TRANSPORT_ROLE_CLIENT = 0,
    OBEX_TRANSPORT_ROLE_SERVER, // 1
} obex_transport_role_t;

typedef enum {
    OBEX_TRANSPORT_CONN_STATE_CLOSE = 0,
    OBEX_TRANSPORT_CONN_STATE_CONNECTING, // 1
    OBEX_TRANSPORT_CONN_STATE_CONNECTED, // 2
    OBEX_TRANSPORT_CONN_STATE_DISCONNECTING, // 3
} obex_transport_conn_state_t;

typedef enum {
    OBEX_TRANSPORT_LISTEN_STATE_IDLE = 0,
    OBEX_TRANSPORT_LISTEN_STATE_LISTENING, // 1
} obex_transport_listen_state_t;

typedef struct {
    obex_transport_type_t type;
    union {
        struct {
            uint32 remote_channel;
            uint32 local_channel;
            uint32 rfcomm_handle;
        } rfcomm;
        struct {
            uint32 psm;
            uint32 channel;
        } l2cap;
    } u;
    void *priv;
} obex_transport_config_t;

typedef enum {
    OBEX_TRANSPORT_EVENT_OPEN_IND = 0,
    OBEX_TRANSPORT_EVENT_OPEN, // 1
    OBEX_TRANSPORT_EVENT_TX_HANDLED, // 2
    OBEX_TRANSPORT_EVENT_CLOSE,// 3
    OBEX_TRANSPORT_EVENT_DATA, // 4
} obex_transport_event_t;

typedef struct {
    union {
        struct {
        } open_ind;
        struct {
        } open;
        struct {
        } tx_handled;
        struct {
        } close;
        struct {
            uint8 *buff;
            uint32 len;
        } data;
    } u;
} obex_transport_cb_param_t;

struct _obex_transport_t;

typedef int32 (*obex_transport_connect_t) (struct _obex_transport_t *transport, struct bdaddr_t *remote);
typedef int32 (*obex_transport_listen_t) (struct _obex_transport_t *transport);
typedef int32 (*obex_transport_disconnect_t) (struct _obex_transport_t *transport);
typedef uint32 (*obex_transport_send_t)(struct _obex_transport_t *transport, uint8 *buff, uint32 buff_len);
typedef int32 (*obex_transport_event_handler_t)(struct _obex_transport_t *transport, obex_transport_event_t event, obex_transport_cb_param_t *param);

typedef struct _obex_transport_t{
    struct list_node node;
    obex_transport_conn_state_t conn_state;
    obex_transport_listen_state_t listen_state;
    obex_transport_config_t config;

    obex_transport_send_t send;
    obex_transport_listen_t listen;
    obex_transport_connect_t connect;
    obex_transport_disconnect_t disconnect;
    obex_transport_event_handler_t event_handler;
} obex_transport_t;

const char *obex_transport_conn_state_2_str(obex_transport_conn_state_t state);
const char *obex_transport_event_2_str(obex_transport_event_t event);
int32 obex_transport_init(obex_transport_t *transport, obex_transport_config_t *config, obex_transport_event_handler_t event_handler);
int32 obex_transport_restore(obex_transport_t *transport,uint32 server_chnl);
void obex_rfcomm_notify_cb(enum rfcomm_event_enum event,uint32 rfcomm_handle,void *data, uint8 reason, void *priv);
void obex_rfcomm_data_recv_cb(uint32 rfcomm_handle,struct pp_buff *ppb, void *priv);

#ifdef __cplusplus
}
#endif

#endif // OBEX_TRANSPORTLAYER_H_INCLUDED
