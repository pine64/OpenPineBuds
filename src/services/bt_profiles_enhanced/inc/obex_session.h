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
#ifndef OBEX_SESSION_H_INCLUDED
#define OBEX_SESSION_H_INCLUDED

#include "bt_common.h"
#include "bt_co_list.h"
#include "obex_protocol.h"
#include "obex_transmission.h"
#include "obex_transportlayer.h"
#include "map_sdp.h"
#include "sdp.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    OBEX_SESSION_EVT_OPEN_IND = 0,
    OBEX_SESSION_EVT_OPEN, // 1
    OBEX_SESSION_EVT_REQUEST, // 2
    OBEX_SESSION_EVT_RESPONSE, // 3
    OBEX_SESSION_EVT_CLOSE, // 4
} obex_session_event_t;

typedef struct {
    uint8 opcode;
    uint8 *packet;
    uint16 packet_len;
    union {
        struct {
            struct {
                uint8 obex_version_num;
                uint8 flags;
                uint16 obex_max_packet_len;
            } connect;
            struct {
                uint8 flags;
                uint16 constants;
            } setpath;
        } request;
        struct {
            struct {
                uint8 obex_version_num;
                uint8 flags;
                uint16 obex_max_packet_len;
            } connect;
        } response;
    } p;
} obex_session_cb_param_t;

struct _obex_session_t;

typedef int32 (*obex_session_event_handler_t)(struct _obex_session_t *session, obex_session_event_t event, obex_session_cb_param_t *param);

typedef enum {
    OBEX_SESSION_ROLE_CLIENT = 0,
    OBEX_SESSION_ROLE_SERVER, // 1
} obex_session_role_t;

typedef enum {
    OBEX_SESSION_STATE_CLOSE = 0,
    OBEX_SESSION_STATE_CONNECTING, // 1
    OBEX_SESSION_STATE_OPEN, // 2
    OBEX_SESSION_STATE_DISCONNECTING, // 3
} obex_session_state_t;

typedef struct {
    obex_session_role_t role;
    uint32 remote_rfcomm_channel;
    uint32 remote_l2cap_psm;
    uint8 local_version;
    uint16 max_obex_packet_length;
    uint8 local_rfcomm_channel;
    uint8 local_l2cap_psm;
    obex_transport_type_t transport_type;
    void *priv;
} obex_session_config_t;

typedef struct _obex_session_t{
    obex_session_config_t config;
    obex_session_state_t state;
    obex_transport_t transport;
    obex_transmission_t *transmission;
    struct bdaddr_t remote;
    uint32 connectionID;
    obex_operation_opcode_t op;
    obex_session_event_handler_t event_handler;
} obex_session_t;

int32 obex_session_init(obex_session_t *session, obex_session_config_t *config, obex_session_event_handler_t event_handler);
int32 obex_session_open(obex_session_t *session, struct bdaddr_t *remote);
int32 obex_session_connect(obex_session_t *session, obex_transmission_t *transmission);
int32 obex_session_disconnect(obex_session_t *session, obex_transmission_t *transmission);
int32 obex_session_put(obex_session_t *session, obex_transmission_t *transmission);
int32 obex_session_get(obex_session_t *session, obex_transmission_t *transmission);
int32 obex_session_setpath(obex_session_t *session, obex_transmission_t *transmission);
int32 obex_session_abort(obex_session_t *session, obex_transmission_t *transmission);
int32 obex_session_response(obex_session_t *session, obex_transmission_t *transmission);
int32 obex_session_close(obex_session_t *session);
int32 obex_session_restore(obex_session_t *session,uint32 connectionID,uint32 server_chnl);

#ifdef __cplusplus
}
#endif

#endif // OBEX_SESSION_H_INCLUDED
