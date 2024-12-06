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
#ifndef __DIP_H__
#define __DIP_H__

#include "sdp.h"
#include "btlib_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// customize
#if !defined(DIP_VENDOR_ID)
#define DIP_VENDOR_ID  0x02B0 //BesTechnic(Shanghai),Ltd
#endif
#if !defined(DIP_PRODUCT_ID)
#define DIP_PRODUCT_ID 0x0000
#endif
#if !defined(DIP_PRODUCT_VERSION)
#define DIP_PRODUCT_VERSION 0x001F  //0.1.F
#endif

#define DIP_SPECIFICATION              0x0103

// values should keep untouched
#define DIP_VENDID_SOURCE_BTSIG        0x0001
#define DIP_VENDID_SOURCE_USBIF        0x0002

#define ATTRID_SPECIFICATION_ID        0x0200
#define ATTRID_VENDOR_ID               0x0201
#define ATTRID_PRODUCT_ID              0x0202
#define ATTRID_PRODUCT_VERSION         0x0203
#define ATTRID_PRIMARY_RECORD          0x0204
#define ATTRID_VENDOR_ID_SOURCE        0x0205

enum dip_event {
    DIP_EVENT_SDP_REQ_FAIL,
    DIP_EVENT_SDP_REQ_SUCCESS,
};

enum dip_ctrl_state {
    DIP_CTRL_ST_IDLE = 0,
    DIP_CTRL_ST_SDP_REQUESTING,
};

struct dip_device_info
{
    uint16 spec_id;
    uint16 vend_id;
    uint16 prod_id;
    uint16 prod_ver;
    uint8  prim_rec;
    uint16 vend_id_source;
};

enum dip_sdp_req_fail_reason {
    DIP_FAIL_REASON_NO_ERROR = 0,
    DIP_FAIL_REASON_SDP_ERROR,
    DIP_FAIL_REASON_SDP_CHANNEL_CLOSE,
};

struct dip_sdp_req_fail {
    enum dip_sdp_req_fail_reason reason;
};

struct dip_callback_param {
    enum dip_event event;
    union {
        struct dip_sdp_req_fail fail;
    }p;
};

struct dip_control_t;
typedef void (*dip_callback_t)(struct dip_control_t *ctrl, struct dip_callback_param *param);

struct dip_control_t {
    struct bdaddr_t remote;
    enum dip_ctrl_state state;
    struct dip_device_info device_info;
    uint8 device_info_valid;
    dip_callback_t callback;
    struct sdp_request sdp_request;
};

int8 dip_register_sdp(void);
int8 dip_deregister_sdp(void);
int8 dip_init(struct dip_control_t *ctrl, dip_callback_t callback);
int8 dip_send_sdp_request(struct dip_control_t *ctrl, struct bdaddr_t *remote);
int8 dip_is_idle(struct dip_control_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif /* __DIP_H__ */
