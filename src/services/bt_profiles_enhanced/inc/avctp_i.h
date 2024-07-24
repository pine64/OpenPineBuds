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

#ifndef __AVCTP_I_H__
#define __AVCTP_I_H__

#include "btlib.h"
#include "co_ppbuff.h"
#include "bt_co_list.h"
#include "btm_i.h"
#include "l2cap_i.h"

enum avctp_event_enum {
    AVCTP_CHANNEL_CONN_REQ,
    AVCTP_CHANNEL_OPEN,
    AVCTP_CHANNEL_NEW_OPENED,
    AVCTP_CHANNEL_TX_HANDLED,
    AVCTP_CHANNEL_CLOSED
};

enum avctp_ctl_state_enum {
    AVCTP_CTL_FREE,
    AVCTP_CTL_INUSE
};

enum avctp_frame_type_enum {
    AVCTP_FRAME_ADV_CMD,
    AVCTP_FRAME_ADV_RSP,
    AVCTP_FRAME_PNL_CMD,
    AVCTP_FRAME_PNL_RSP,
    AVCTP_FRAME_UNITINFO_CMD,
    AVCTP_FRAME_UNITINFO_RSP,
    AVCTP_FRAME_SUBUNITINFO_RSP,
};

enum avctp_tx_status_enum {
    AVCTP_TX_IDLE,
    AVCTP_TX_BUSY,
};

enum avctp_role_enum {
    AVCTP_MASTER,
    AVCTP_SLAVE
};

#define AVCTP_CFG_SERVER_CHANNEL 0x08

/*
struct avctp_session {
    struct list_node list;
    
    uint32 l2cap_handle;
    
    struct bdaddr_t remote;
    
    void (*notify_callback) (uint8 event, struct avctp_session *s, void *pdata);
    void (*datarecv_callback) (struct avctp_session *s, struct pp_buff *ppb);
};
*/

#define AVCTP_PKT_HEADER_PROFILE_ID 0x110E

#define AVCTP_PKT_HEADER_VALID_PID 0
#define AVCTP_PKT_HEADER_INVALID_PID 1

#define AVCTP_PKT_HEADER_IS_CMD 0
#define AVCTP_PKT_HEADER_IS_RESPONSE 1

enum avctp_packet_type
{
    AVCTP_PACKET_TYPE_SINGLE = 0,
    AVCTP_PACKET_TYPE_START = 1,
    AVCTP_PACKET_TYPE_CONTINUE = 2,
    AVCTP_PACKET_TYPE_END = 3,
};

struct avctp_header {
    uint32
    ipid_ind    : 1,
    com_or_resp : 1,
    packet_type : 2,
    trans_label : 4;
    uint8 profile_ind[2];
}__attribute__ ((packed));

struct avctp_frame_t {
    struct list_node node;
    struct avctp_header header;
    int8 frame_type;
    bool need_free;
    uint8 *data;
    uint32 data_len;
    uint32 data_offset;
};

struct avctp_control_t {
    struct list_node node;
    struct list_node tx_list;
    
    struct bdaddr_t remote;
    struct avctp_header header;
    int8 server_channel;
    uint32 l2cap_handle;

    int8 tx_status;

    enum avctp_ctl_state_enum ctl_state;
    
    enum avctp_role_enum role;
    int (*notify_callback)(struct avctp_control_t *avctp_ctl, uint8 event, uint32 handle, void *pdata);
    void (*datarecv_callback)(struct avctp_control_t *avctp_ctl, uint32 handle, struct pp_buff *ppb);
};

struct avctp_ctx_input {
    struct ctx_content ctx;
    struct bdaddr_t *remote;
    uint32 l2cap_handle;
    struct avctp_control_t *avctp_ctl;
};

struct avctp_ctx_output {
};

#if defined(__cplusplus)
extern "C" {
#endif

int avctp_l2cap_notify_callback(enum l2cap_event_enum event, uint32 l2cap_handle, void *pdata, uint8 reason);
void avctp_l2cap_datarecv_callback(uint32 l2cap_handle, struct pp_buff *ppb);

int8 avctp_init(struct avctp_control_t *avctp_ctl);
int8 avctp_init_service(void);

int8 avctp_register_server (struct avctp_control_t *avctp_ctl, int8 server_channel,
                            int (*avctp_notify_callback)(struct avctp_control_t *avctp_ctl, uint8 event, uint32 handle, void *pdata),
                            void (*avctp_datarecv_callback)(struct avctp_control_t *avctp_ctl, uint32 handle, struct pp_buff *ppb)
                            );
                            
int8 avctp_unregister_server (struct avctp_control_t *avctp_ctl, int8 server_channel);

void avctp_init_packet_header(struct avctp_control_t *avctp_ctl, struct avctp_frame_t *frame, 
    int8 is_cmd, int8 ipid_ind, uint16 profile_ind);
int8 avctp_send_message(struct avctp_control_t *avctp_ctl, struct avctp_frame_t *frame);
int8 avctp_close(struct avctp_control_t *avctp_ctl);
int8 avctp_open_i(struct avctp_control_t *avctp_ctl, struct bdaddr_t *remote,
                  int (*avctp_notify_callback)(struct avctp_control_t *avctp_ctl, uint8 event, uint32 handle, void *pdata),
                  void (*avctp_datarecv_callback)(struct avctp_control_t *avctp_ctl, uint32 handle, struct pp_buff *ppb),
                  uint32 *avctp_handle);
int8 avctp_disconnectReq (struct avctp_control_t *avctp_ctl);

uint32 avctp_get_l2cap_handle(struct avctp_control_t *avctp_ctl);
uint16 avctp_get_conn_handle(struct avctp_control_t *avctp_ctl);
uint32 avctp_save_ctx(struct avctp_control_t *avctp_ctl, uint8_t *buf, uint32_t buf_len);
uint32 avctp_restore_ctx(struct avctp_ctx_input *input, struct avctp_ctx_output *output);

#if defined(__cplusplus)
}
#endif

#endif /* __AVCTP_I_H__ */