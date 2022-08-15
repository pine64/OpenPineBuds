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

#ifndef __A2DP_I_H__
#define __A2DP_I_H__

#include "btlib_type.h"
#include "btm_i.h"
#include "avdtp.h"
#include "avdtp_i.h"
#include "sdp.h"

#define A2DP_RECVDATA_BUF_SIZE 48

/* notify upper layer */
enum a2dp_event_t
{
    /* user command event*/
    AV_CONNECT_REQ = 1,
    AV_DISCONNECT_REQ, //210
    AV_TRANS_REQ,
    AV_SUSPEND_REQ,
    AV_START_REQ,
    AV_ABORT_REQ,
    AV_SET_CONFIG_REQ,
    AV_RECONF_REQ,

    /* internal event */
    //  AV_EVNT_BEGIN,
    AV_CONN_OPENED, //217
    AV_CONN_CLOSED,

    AV_MEDIA_GET_CAP_IND,
    AV_MEDIA_SET_CFG_IND,
    AV_MEDIA_GET_CFG_CFM,
    AV_MEDIA_STREAM_CLOSE,   //219
    AV_MEDIA_STREAM_ABORT, //219
    AV_MEDIA_RECONF_IND,
    AV_MEDIA_DISCOVERY_COMPLETE,

    AV_MEDIA_OPENED,         //219
    AV_MEDIA_STREAM_START,   //219
    AV_MEDIA_STREAM_DATA_IND,   //219
    AV_MEDIA_STREAM_SUSPEND, //219
    AV_MEDIA_FREQ_CHANGED,   //219
    AV_CONN_REQ_FAIL,
    AV_MEDIA_SECURITY_CONTROL_CMD,
    AV_MEDIA_SECURITY_CONTROL_CFM
};

enum av_conn_state_enum
{
    AV_STOP,
    AV_STANDBY = 1, //ready
    AV_QUERING, //sdp quering
    AV_CONNECTING,  //initializing
    AV_OPEN,        //AV_SIG_ACTIVE,
    AV_CONNECTED,
    AV_STREAMING
};


struct a2dp_control_t
{
    struct bdaddr_t remote;
    bool cp_enable;
    uint8 disc_reason;
    uint32 l2capSignalHandle;
    uint32 l2capMediaHandle;
    uint32 freq;
    uint16 avdtp_local_version;
    uint16 avdtp_remote_version;
    uint16 a2dp_remote_features;
    uint8 channel_mode;

    enum av_conn_state_enum state; /*check if it is connecting now, if it is connecting,the hf_connect_req will not work */

    struct a2dp_sep *sep; //bind to lsep
    struct avdtp_control_t *avdtp;
                          //struct avdtp_stream *stream;
    void *avdtp_codec_req;
    void *avdtp_cp_req;
    struct avdtp_media_codec_capability *avdtp_codec_cap;
    struct avdtp_media_content_protect_capability *avdtp_content_protect_cap;
    uint8 type;
    struct avdtp_local_sep *lsep;

    void (*indicate)(struct a2dp_control_t *stream, uint8 event, void *pData);
    void (*data_cb)(struct a2dp_control_t *stream,struct pp_buff *ppb);
    uint8 sep_prio;
    struct a2dp_control_t* next;
    avdtp_media_header_t media_header;
    struct sdp_request sdp_request;
    uint8 device_id;
};

struct a2dp_ctx_input {
    struct ctx_content ctx;
    struct bdaddr_t *remote;
    struct a2dp_control_t *a2dp_ctl;
    struct avdtp_control_t *avdtp_ctl;
    struct avdtp_local_sep *local_sep;
    uint32 l2capSignalHandle;
    uint32 l2capMediaHandle;
    void (*indicate_callback)(struct a2dp_control_t *stream, uint8 event, void *pData);
    void (*data_callback)(struct a2dp_control_t *stream, struct pp_buff *ppb);
};

struct a2dp_ctx_output {
};

extern struct a2dp_control_t *a2dp_ctl_list;

//int8 av_register_datacallback(struct a2dp_control_t *stream,void (*data_callback)(struct a2dp_control_t* stream, struct pp_buff *ppb));

#if defined(__cplusplus)
extern "C" {
#endif

void a2dp_stack_init(void);
/*---- a2dp_app.c ----*/
//void app_callback(struct a2dp_control_t* stream, uint8 event, void *pData);
void data_callback_av(struct a2dp_control_t *stream, struct pp_buff *ppb);

/*----a2dp.c----*/
int8 a2dp_register(struct a2dp_control_t *stream,
                   void (*indicate_callback)(struct a2dp_control_t *stream, uint8 event, void *pData),
                   void (*data_callback)(struct a2dp_control_t *stream, struct pp_buff *ppb));
int a2dp_unregister(struct a2dp_control_t *ctl);

enum av_conn_state_enum av_getState(struct  a2dp_control_t*stream) ;
void av_setState( struct  a2dp_control_t *stream, enum av_conn_state_enum state);
int8 av_turnOn(struct  a2dp_control_t *stream);
int8 av_turnOff(struct a2dp_control_t * stream);

int8 av_connectReq(struct a2dp_control_t *stream, struct bdaddr_t *peer);
int8 av_disconnectReq(struct  a2dp_control_t *stream);
//int8 av_sendData(uint8 *data, uint32 datalen);

void av_reset_a2dp_state(struct a2dp_control_t * stream);

const char *a2dp_state2str(enum av_conn_state_enum state);

int8 av_suspendReq(struct a2dp_control_t *stream);
int8 av_startReq(  struct  a2dp_control_t  *stream);
int8 av_abortReq( struct  a2dp_control_t  *stream);
int8 av_setConfReq( struct  a2dp_control_t  *stream); ///
int8 av_reconfReq( struct  a2dp_control_t  *stream );  ///

void av_discoverCap(struct a2dp_control_t *stream);
int8 av_setSinkDelay(struct a2dp_control_t *stream, uint16 delay_ms);
int8 av_security_control_req(struct a2dp_control_t *stream, uint8_t *data, uint16_t len);
int8 av_security_control_resp(struct a2dp_control_t *stream, uint8_t *data, uint16_t len, uint8 error);
uint32 av_getFreq( struct  a2dp_control_t  *stream);
void av_setFreq( struct a2dp_control_t *stream, uint32 frequency);
uint8 av_getChannelMode(struct  a2dp_control_t *stream);

struct avdtp_control_t *a2dp_get_avdtp_control(struct a2dp_control_t *stream);
uint32 a2dp_save_ctx(struct a2dp_control_t *a2dp_ctl, uint8_t *buf, uint32_t buf_len);
uint32 a2dp_restore_ctx(struct a2dp_ctx_input *input, struct a2dp_ctx_output *output);

#if defined(__cplusplus)
}
#endif

#endif /* __A2DP_I_H__ */
