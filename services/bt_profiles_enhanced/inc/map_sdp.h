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
#ifndef MAP_SDP_H_INCLUDED
#define MAP_SDP_H_INCLUDED

#include "bt_common.h"
#include "bt_co_list.h"
#include "btlib_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAP_SDP_EVT_SUCCESS = 0,
    MAP_SDP_EVT_NOT_FOUND, // 1
    MAP_SDP_EVT_CONNECT_FAIL, // 2
    MAP_SDP_EVT_CONNECT_CLOSE, // 3
} map_sdp_event_t;

typedef struct {
    uint32 rfcomm_channel;
    uint32 l2cap_psm;
    uint16 profile_version;
    uint8 mas_instance_id;
    uint8 supported_message_types;
    uint32 map_supported_features;
} map_sdp_server_property_t;

typedef struct {
    union {
        struct {
            map_sdp_server_property_t property;
        } success;
        struct {
            uint8 reason;
        } connect_fail;
    } u;
} map_sdp_callback_param;

typedef int32 (*map_sdp_callback_t)(map_sdp_event_t event, map_sdp_callback_param *cb_param, void *priv);

int32 map_sdp_request(struct bdaddr_t *remote, void *priv);
int32 map_sdp_register(map_sdp_server_property_t *property);

#ifdef __cplusplus
}
#endif

#endif // MAP_SDP_H_INCLUDED
