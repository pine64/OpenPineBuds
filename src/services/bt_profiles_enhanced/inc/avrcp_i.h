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


#ifndef __AVRCP_I_H__
#define __AVRCP_I_H__

#include "bt_co_list.h"
#include "btlib_type.h"
#include "btm_i.h"
#include "avctp_i.h"
#include "sdp.h"

#define AVRCP_RECVDATA_BUF_SIZE 48

#define AVRCP_UINT_INFO_IND      0x30
#define AVRCP_SUBUNIT_INFO_IND   0x31
#define AVRCP_PASSTHROUGH_IND    0x7C
#define AVRCP_VENDOR_DEP_IND     0x00

#define AVRCP_RESP_NOT_IMPLEMENTED  0x08
#define AVRCP_RESP_ACCEPT           0x09
#define AVRCP_RESP_REJECT           0x0A
#define AVRCP_RESP_STABLE           0x0C
#define AVRCP_RESP_CHANGED          0x0D
#define AVRCP_RESP_INTERIM          0x0F

#define AVRCP_OP_GET_CAPABILITIES               0x10
#define AVRCP_OP_LIST_PLAYER_SETTING_ATTRIBS    0x11
#define AVRCP_OP_LIST_PLAYER_SETTING_VALUES     0x12
#define AVRCP_OP_GET_PLAYER_SETTING_VALUE       0x13
#define AVRCP_OP_SET_PLAYER_SETTING_VALUE       0x14
#define AVRCP_OP_GET_PLAYER_SETTING_ATTR_TEXT   0x15
#define AVRCP_OP_GET_PLAYER_SETTING_VALUE_TEXT  0x16
#define AVRCP_OP_INFORM_DISP_CHAR_SET           0x17
#define AVRCP_OP_INFORM_BATT_STATUS             0x18
#define AVRCP_OP_GET_MEDIA_INFO                 0x20
#define AVRCP_OP_GET_PLAY_STATUS                0x30
#define AVRCP_OP_REGISTER_NOTIFY                0x31
#define AVRCP_OP_REQUEST_CONT_RESP              0x40
#define AVRCP_OP_ABORT_CONT_RESP                0x41
#define AVRCP_OP_SET_ABSOLUTE_VOLUME            0x50
#define AVRCP_OP_SET_ADDRESSED_PLAYER           0x60
#define AVRCP_OP_SET_BROWSED_PLAYER             0x70
#define AVRCP_OP_GET_FOLDER_ITEMS               0x71
#define AVRCP_OP_CHANGE_PATH                    0x72
#define AVRCP_OP_GET_ITEM_ATTRIBUTES            0x73
#define AVRCP_OP_PLAY_ITEM                      0x74
#define AVRCP_OP_SEARCH                         0x80
#define AVRCP_OP_ADD_TO_NOW_PLAYING             0x90
#define AVRCP_OP_GENERAL_REJECT                 0xA0
#define AVRCP_OP_CUSTOM_CMD						0xF0

enum avrcp_role_enum {
    AVRCP_MASTER,
    AVRCP_SLAVE
};
enum avrcp_event_t {
//    AVRCP_TURN_ON = 1,
//    AVRCP_TURN_OFF,
    AVRCP_PLAY_REQ = 1, //241
    AVRCP_PAUSE_REQ,
    AVRCP_STOP_REQ,
    AVRCP_RECORD_REQ,
    AVRCP_FORWARD_REQ,
    AVRCP_BACKWARD_REQ,
    AVRCP_FAST_FORWARD_START_REQ,
    AVRCP_FAST_BACKWARD_START_REQ,
    AVRCP_FF_FB_STOP_REQ,
    AVRCP_CONN_REQ, //250
    AVRCP_DISCONN_REQ,

//   AV_C_EVNT_BEGIN,
    AVRCP_CHANNEL_OPENED,
    AVRCP_CHANNEL_NEW_OPEN,
    AVRCP_CHANNEL_CLOSED,
    AVRCP_CHANNEL_TX_HANDLED,
    AVRCP_CHANNEL_RESPONSE,
    AVRCP_CHANNEL_COMMAND,
    AVRCP_CHANNEL_DATA_IND,
    AVRCP_RESPONSE            //no more than 15
};


enum avrcp_state_enum {
    AVRCP_STOP,
    AVRCP_STANDBY = 1,  //ready
    AVRCP_QUERING,
    AVRCP_CONNECTING,
    AVRCP_CONNECTED
 //   AVRCP_BUSY
};

enum music_state_enum {
    MUSIC_NO_ACTION,
    MUSIC_PLAYING,
    MUSIC_PAUSE,
    MUSIC_RESUME,
    MUSIC_STOP
};

struct avrcp_control_t {
    struct list_node node;
    struct avctp_control_t avctp_ctl;
    enum avrcp_role_enum role;
    struct bdaddr_t remote;
    enum avrcp_state_enum state;
    uint32 handle;
    uint8 op_code;
    uint8 op_id;
    struct avctp_frame_t pnl_cmd;
    struct avctp_frame_t adv_cmd;
    struct avctp_frame_t adv_rsp;
    struct avctp_frame_t unitinfo_cmd;
    struct avctp_frame_t subunitinfo_cmd;
    uint8 pnl_cmd_buff[64];
    uint8 adv_cmd_buff[128];
    uint8 adv_rsp_buff[128];
    uint8 unitinfo_cmd_buff[64];
    uint8 is_src_playing;
    uint8 is_volume_sync;
   
    void (*indicate) (struct avrcp_control_t *avrcp_ctl, uint8 event, void *pdata);
    void (*data_cb) (struct avrcp_control_t *avrcp_ctl, struct pp_buff *ppb);

    struct sdp_request sdp_request;
    uint16 remote_avctp_version;
    uint16 remote_avrcp_version;
    uint16 remote_support_features;
    bool conn_avctp_after_sdp;
};

struct avrcp_advanced_cmd_pdu {
    struct list_node node;
    uint8 op;
    U16 parm_len;
    U8 *parms;
    U8  trans_id;

    BOOL more;
    U16  cur_len;
    U16  bytes_to_send;
    U8   cont_op;
    BOOL abort;

    BOOL  internal;
    U8  response;
    U8 error;

    BOOL is_cmd;
    U8  ctype;          /* 4 bits */
};

struct avrcp_adv_cmd_rsp_cb_parms {
    uint8 rsp;
    uint8 subunit_type;
    uint8 subunit_id;
    uint8 op_code;
    uint8 *origin_data;
    uint8 *op_data;
    uint16 op_data_len;
};

struct avrcp_adv_cmd_cb_parms {
    uint8 ctype;
    uint8 subunit_type;
    uint8 subunit_id;
    uint8 op_code;
    uint8 *origin_data;
    uint8 *op_data;
    uint16 op_data_len;
};

struct avrcp_ctx_input {
    struct ctx_content ctx;
    struct bdaddr_t *remote;
    struct avrcp_control_t *avrcp_ctl;
};

struct avrcp_ctx_output {
};

#if defined(__cplusplus)
extern "C" {
#endif

/* AVRCP APP */
void app_callback(uint8 event, void *pdata);

/* AVRCP */
int8 avrcp_init_inst(struct avrcp_control_t *avrcp_ctl, void (*indicate) (struct avrcp_control_t *avrcp_ctl, uint8 event, void *pdata),
                void (*datarecv_callback) (struct avrcp_control_t *avrcp_ctl, struct pp_buff *ppb));
                
enum avrcp_state_enum avrcp_getState(struct avrcp_control_t *avrcp_ctl);
void avrcp_setState(struct avrcp_control_t *avrcp_ctl, enum avrcp_state_enum state);
int8 avrcp_turnOn(struct avrcp_control_t *avrcp_ctl);
int8 avrcp_turnOff(struct avrcp_control_t *avrcp_ctl);
int8 avrcp_connectReq(struct avrcp_control_t *avrcp_ctl, struct bdaddr_t *peer);
int8 avrcp_disconnectReq(struct avrcp_control_t *avrcp_ctl);
int8 avrcp_send_cmd(struct avrcp_control_t *avrcp_ctl, int op);

/* Used by Controller to Send Command */
bool avrcp_is_advanced_command_can_send(struct avrcp_control_t * avrcp_ctl, bool is_cmd);
int8 avrcp_send_advanced_command(struct avrcp_control_t *avrcp_ctl, struct avrcp_advanced_cmd_pdu *pdu);
int8 avrcp_send_advanced_response(struct avrcp_control_t *avrcp_ctl, struct avrcp_advanced_cmd_pdu *pdu);
int8 avrcp_send_panel_key(struct avrcp_control_t *avrcp_ctl, uint16 op, uint8 press);
bool avrcp_is_control_channel_connected(struct avrcp_control_t *avrcp_ctl);
int8 avrcp_is_panel_cmd_can_send(struct avrcp_control_t *avrcp_ctl);
int8 avrcp_send_panel_response(struct avrcp_control_t *avrcp_ctl, uint16 op, uint8 press, uint8 response);
int8 avrcp_send_unit_info_cmd(struct avrcp_control_t *avrcp_ctl);
int8 avrcp_send_unit_info_response(struct avrcp_control_t *avrcp_ctl);
int8 avrcp_send_subunit_info_response(struct avrcp_control_t *avrcp_ctl);
void avrcp_register_panel_command_tx_handled_callback(void (*cb)(struct avrcp_control_t *avrcp_ctl, void *pdata));

struct avctp_control_t *avrcp_get_avctp_control(struct avrcp_control_t *avrcp_ctl);
uint16 avrcp_get_conn_handle(struct avrcp_control_t* avrcp_ctl);
uint32 avrcp_save_ctx(struct avrcp_control_t *avrcp_ctl, uint8_t *buf, uint32_t buf_len);
uint32 avrcp_restore_ctx(struct avrcp_ctx_input *input, struct avrcp_ctx_output *output);

#if defined(__cplusplus)
}
#endif

#endif /* __AVRCP_I_H__ */