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
#ifndef MAP_SESSION_H_INCLUDED
#define MAP_SESSION_H_INCLUDED

#include "obex_session.h"
#include "map_protocol.h"
#include "map_bmessage_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAP_SESSION_FOLDER_LEVEL_MAX (32)
#define MAP_MAX_PACKET_LENGTH (512)
#define MAP_OBEX_VERSION (0x10)

typedef enum {
    MAP_SESSION_TYPE_MAS = 0,
    MAP_SESSION_TYPE_MNS, // 1
} map_session_type_t;

typedef struct {
    map_session_type_t type;
    obex_session_role_t obex_role;
} map_session_config_t;

typedef enum {
    MAP_FUNC_NONE = 0,
    MAP_FUNC_SendEvent,
    MAP_FUNC_SetNotificationRegistration,
    MAP_FUNC_SetFolder,
    MAP_FUNC_GetFolderListing,
    MAP_FUNC_GetMessagesListing,
    MAP_FUNC_GetMessage,
    MAP_FUNC_SetMessageStatus,
    MAP_FUNC_PushMessage,
    MAP_FUNC_UpdateInbox,
} map_session_function_t;

typedef struct {
    union {
        struct {
            uint32 bmsg_offset;
        } PushMessage;
        struct {
            uint8 level;
        } SetFolder;
    } state;
} map_session_function_state_t;

typedef enum {
    MAP_SESSION_ERR_NO_ERROR = 0,
    MAP_SESSION_ERR_SDP_REQUEST_FAIL, // 1
    MAP_SESSION_ERR_CHANNEL_CLOSE, // 2
} map_session_error_t;

typedef struct {
    union {
        struct {
            map_session_error_t error;
            uint8 error_detail;
        } close;
        struct {
            uint8 *packet;
            uint32 packet_len;
        } data;
        struct {
            struct bdaddr_t *remote;
        } open;
    } p;
} map_session_cb_param_t;

typedef enum {
    MAP_SESSION_EVENT_OPEN_IND = 0,
    MAP_SESSION_EVENT_OPEN, // 1
    MAP_SESSION_EVENT_CLOSE, // 2
} map_session_event_t;

typedef enum {
    MAP_SESSION_STATE_CLOSE = 0,
    MAP_SESSION_STATE_SDP_REQUESTING, // 1
    MAP_SESSION_STATE_OPENING, // 2
    MAP_SESSION_STATE_CONNECTING, // 3
    MAP_SESSION_STATE_OPEN, // 4
    MAP_SESSION_STATE_CONNECTED, // 5
} map_session_state_t;

struct _map_session_t;

typedef int32 (*map_session_event_handler_t)(struct _map_session_t *, map_session_event_t event, map_session_cb_param_t *param);

typedef struct {
    union {
        struct
        {
        } SendEvent;
        struct
        {
        } SetNotificationRegistration;
        struct
        {
            char *folder[MAP_SESSION_FOLDER_LEVEL_MAX];
            uint8 folder_level;
            uint8 up_level;
        } SetFolder;
        struct
        {
        } GetFolderListing;
        struct
        {
        } GetMessagesListing;
        struct
        {
        } GetMessage;
        struct
        {
        } SetMessageStatus;
        struct
        {
            char *folder;
            uint32 folder_len;
            char *msg_body;
            uint32 msg_body_len;
            uint8 retry;
            uint8 transparent;
            map_bmessage_type_t type;
            char *originator_name;
            uint32 originator_name_len;
            char *originator_tel;
            uint32 originator_tel_len;
            char *receipt_name;
            uint32 receipt_name_len;
            char *receipt_tel;
            uint32 receipt_tel_len;
        } PushMessage;
        struct
        {
        } UpdateInbox;
    } p;
} map_session_function_param_t;

typedef enum {
    MAP_FUNC_ACTION_NONE = 0,
    MAP_FUNC_ACTION_START,
    MAP_FUNC_ACTION_STOP,
    MAP_FUNC_ACTION_REQUEST,
    MAP_FUNC_ACTION_RESPONSE,
    MAP_FUNC_ACTION_CONNECTED,
} map_session_function_action_t;

typedef struct {
    void *priv;
    union {
        struct {
            uint8 *packet;
            uint32 packet_len;
        } request;
        struct {
            uint8 *packet;
            uint32 packet_len;
        } response;
    } p;
} map_session_function_action_param_t;

typedef struct _map_session_t {
    map_session_config_t config;
    map_session_state_t state;
    obex_session_t obex_session;
    obex_transmission_t obex_transm;
    struct sdp_request sdp_request;
    map_sdp_callback_t sdp_callback;
    map_sdp_server_property_t remote_server_property;
    struct bdaddr_t remote;
    map_session_event_handler_t event_handler;
    uint8 transm_buffer[MAP_MAX_PACKET_LENGTH];
    uint8 tlv_buffer[MAP_MAX_PACKET_LENGTH];
    uint8 bmsg_buffer[MAP_MAX_PACKET_LENGTH];
    map_bmsg_t bmsg;
    map_session_function_t function;
    map_session_function_param_t function_param;
    map_session_function_state_t function_state;
} map_session_t;

typedef void (*map_callback_t) (obex_session_cb_param_t * param, map_session_t * map_session);

void map_callback_register(map_callback_t callback);
int32 map_session_init(map_session_t *session, map_session_config_t *cfg, map_session_event_handler_t event_handler);
int32 map_session_open(map_session_t *session, struct bdaddr_t *remote);
int32 map_session_close(map_session_t *session);
int32 map_session_exec_function(map_session_t *session, map_session_function_t func, map_session_function_param_t *param);
int32 map_session_restore(map_session_t *session, uint32 conn_id,uint32 server_chnl);
bool map_session_check_is_idle(map_session_t *map_session);

#ifdef __cplusplus
}
#endif

#endif // MAP_SESSION_H_INCLUDED
