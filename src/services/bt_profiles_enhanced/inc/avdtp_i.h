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

#ifndef __AVDTP_I_H__
#define __AVDTP_I_H__

#include "btlib.h"
#include "co_ppbuff.h"
#include "bt_co_list.h"
#include "bt_common.h"
#include "avdtp.h"

enum avdtp_event_enum
{
    AVDTP_OPENED,
    AVDTP_ACCEPT_OPENED,
    AVDTP_MEDIA_OPENED,
    AVDTP_CLOSED
};

typedef enum
{
    AVDTP_STATE_IDLE,
    AVDTP_STATE_DISCOVER,
    AVDTP_STATE_GETCAP,
    AVDTP_STATE_SETCONFIG,
    AVDTP_STATE_CONFIGURED,
    AVDTP_STATE_OPEN,
    AVDTP_STATE_STREAMING,
    AVDTP_STATE_CLOSING,
    AVDTP_STATE_ABORTING
} avdtp_state_t;

/* SEP types definitions */
#define AVDTP_SEP_TYPE_SOURCE 0x00
#define AVDTP_SEP_TYPE_SINK 0x01

/* Media types definitions */
#define AVDTP_MEDIA_TYPE_AUDIO 0x00
#define AVDTP_MEDIA_TYPE_VIDEO 0x01
#define AVDTP_MEDIA_TYPE_MULTIMEDIA 0x02

/* SEP capability categories */
#define AVDTP_MEDIA_TRANSPORT 0x01
#define AVDTP_REPORTING 0x02
#define AVDTP_RECOVERY 0x03
#define AVDTP_CONTENT_PROTECTION 0x04
#define AVDTP_HEADER_COMPRESSION 0x05
#define AVDTP_MULTIPLEXING 0x06
#define AVDTP_MEDIA_CODEC 0x07
#define AVDTP_DELAY_REPORTING 0x08

#define AVDTP_CFG_SERVER_CHANNEL 0x07

/* AVDTP Content Protection Type */
#define AVDTP_CP_TYPE_DTCP      0x0001
#define AVDTP_CP_TYPE_SCMS_T    0x0002

struct seid_info
{
    uint32 rfa0 : 1;
    uint32 inuse : 1;
    uint32 seid : 6;
    uint32 rfa2 : 3;
    uint32 type : 1;
    uint32 media_type : 4;
    //  uint32 unused:16;
} __attribute__((packed));
struct avdtp_service_capability
{
    uint8 category;
    uint8 length;
    uint8 data[1];
} __attribute__((packed));

struct avdtp_media_codec_capability
{
    uint8 media_codec_type;
    uint8 len;
    uint8 *data;
} __attribute__((packed));


struct avdtp_media_content_protect_capability
{
    uint16 protect_type;
    uint8 len;
    uint8 *data;
} __attribute__((packed));


struct avdtp_media_delay_report_capability
{
    uint8 len;
} __attribute__((packed));

struct avdtp_service_cap_req
{
    struct avdtp_service_capability *cap;
    uint8  parse_error;
}__attribute__((packed));



struct avdtp_config_request
{
    struct avdtp_service_cap_req *trans_cap;
    struct avdtp_service_cap_req *codec_cap;
    struct avdtp_service_cap_req *cp_cap;
    struct avdtp_service_cap_req *dr_cap;
    struct avdtp_service_cap_req *rp_cap;
    struct avdtp_service_cap_req *mp_cap;
    struct avdtp_service_cap_req *hc_cap;
    struct avdtp_service_cap_req *rc_cap;

    struct avdtp_service_cap_req *unknown_cap;
};

struct avdtp_local_sep
{
    struct seid_info info;
    uint8 codec;
    struct avdtp_sep_ind *ind;
    struct avdtp_sep_cfm *cfm;
    void *user_data;
    void *a2dp_stream;
    uint8 delay_reporting_support;
    uint16 dr_delay_ms;
    uint8 device_id;
    struct avdtp_local_sep *next;
};

struct avdtp_remote_sep
{
    uint8 seid;
    uint8 transact_id;
    uint8 delay_reporting_support;
    uint16 avdtp_cp_type;
    struct avdtp_service_capability *codec;
    struct avdtp_remote_sep *next;
};

struct avdtp_control_t
{
    int8 avdtp_channel;
    struct bdaddr_t remote;
    uint8 remote_seid;
    uint8 same_sepid_workaround_for_redmi5;
    uint8 delay_reporting_enabled_on_the_stream;

    void (*notify_callback)(uint8 event, uint32 l2cap_channel, void *pdata, uint8 reason);
    void (*datarecv_callback)(uint32 l2cap_channel, struct pp_buff *ppb);
    void (*discover_cb)(struct avdtp_control_t *avdtp_ctl, struct avdtp_remote_sep *);
    uint8 discover_cnt;

    avdtp_state_t state;

    struct avdtp_session *signal_session;
    struct avdtp_session *media_session;
    void *a2dp_stream;
    struct avdtp_local_sep *local_sep;
    BOOLEAN initiator;
    struct avdtp_control_t *next;
    void * cur_op;

    uint8 delay_resp_discover_req;
    uint8 discover_req_transaction;
};

struct avdtp_ctx_input {
    struct ctx_content ctx;
    struct bdaddr_t *remote;
    struct avdtp_control_t *avdtp_ctl;
    avdtp_state_t state;
    uint32_t seid;
    uint32 sig_handle;
    uint32 med_handle;
    void (*avdtp_notify_callback)(uint8 event, uint32 l2cap_channel, void *pdata, uint8 reason);
    void (*avdtp_datarecv_callback)(uint32 l2cap_handle, struct pp_buff *ppb);
};

struct avdtp_ctx_output {
    struct avdtp_control_t *avdtp_ctl;
    struct avdtp_local_sep *local_sep;
};

//callback functions for dealing with remote response to local command
struct avdtp_sep_cfm
{
    void (*setconf)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*getconf)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*open)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*start)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*suspend)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*close)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*abort)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*reconf)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep );
    void (*security_control)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep , struct avdtp_media_content_protect_capability* cp);
};
//callback functions to deal with remote command
struct avdtp_sep_ind
{
    uint8 *(*getcap)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep , uint16 *len, uint8 sig_type);
    int8 (*setconf)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep , struct avdtp_config_request* cfg_req);
    int8 (*open)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep);
    int8 (*start)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep);
    int8 (*suspend)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep);
    int8 (*close)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep);
    int8 (*abort)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep);
    int8 (*reconf)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep, struct avdtp_config_request* cfg_req);
    int8 (*security_control)(struct avdtp_control_t *avdtp_ctl,struct avdtp_local_sep *local_sep , struct avdtp_media_content_protect_capability* cp);
};

#if defined(__cplusplus)
extern "C" {
#endif

int8 avdtp_init(int8 server_channel,
                void (*avdtp_notify_callback)(uint8 event, uint32 l2cap_channel, void *pdata, uint8 reason),
                void (*avdtp_datarecv_callback)(uint32 l2cap_handle, struct pp_buff *ppb));
uint32 avdtp_open_i(void *a2dp_ctl, struct bdaddr_t *remote);
int8 avdtp_close_i(struct bdaddr_t *remote);

int8 avdtp_send_delay_report(struct avdtp_control_t * avdtp_ctl, uint16 delay_ms);

int8 avdtp_send_security_control_req(struct avdtp_control_t *stream, uint8_t *data, uint16_t len);

int8 avdtp_send_security_control_resp(struct avdtp_control_t *stream,
                                      uint8_t *data, uint16_t len, uint8 error);

struct avdtp_local_sep *avdtp_register_sep(uint8 device_id,
                                           uint8 type,
                                           struct avdtp_sep_ind *indi,
                                           struct avdtp_sep_cfm *cnfm);
int8 avdtp_unregister_sep(struct avdtp_local_sep *sep);
int8 avdtp_discover(void (*discover_cb)(struct avdtp_control_t *avdtp_ctl, struct avdtp_remote_sep *),uint32_t l2csig_handle);
int8 avdtp_force_close(uint32_t l2csig_handle);
void avdtp_discover_cap(uint32_t l2csig_handle);
int8 avdtp_set_configuration(struct avdtp_control_t *avdtp_ctl, struct avdtp_remote_sep *rsep, uint8 local_sepid, uint8 *conf_data, uint16 caps_len);

int8 avdtp_send_command_with_param(struct avdtp_control_t *avdtp_ctl, uint32 signal_id, uint8 *params, uint32 param_len);

#define avdtp_send_command_len3(c,s) \
    avdtp_send_command_with_param(c,s,NULL,0)

#define avdtp_open(avdtp_ctl) \
    avdtp_send_command_len3(avdtp_ctl, AVDTP_OPEN)

#define avdtp_start(avdtp_ctl) \
    avdtp_send_command_len3(avdtp_ctl, AVDTP_START)

#define avdtp_close(avdtp_ctl) \
    avdtp_send_command_len3(avdtp_ctl, AVDTP_CLOSE)

#define avdtp_suspend(avdtp_ctl) \
    avdtp_send_command_len3((avdtp_ctl), AVDTP_SUSPEND)

#define avdtp_abort(avdtp_ctl) \
    avdtp_send_command_len3((avdtp_ctl), AVDTP_ABORT)

//int8 avdtp_discover(struct avdtp_session *session, void (*discover_cb)(struct avdtp_session *s, GSList *sep, uint8 err, void *user_data));

uint32 avdtp_get_signal_l2cap_handle(struct avdtp_control_t *avdtp_ctl);
uint32 avdtp_get_media_l2cap_handle(struct avdtp_control_t *avdtp_ctl);
uint32 avdtp_save_ctx(struct avdtp_control_t *avdtp_ctl, uint8_t *buf, uint32_t buf_len);
uint32 avdtp_restore_ctx(struct avdtp_ctx_input *input, struct avdtp_ctx_output *output);
void avdtp_set_synced_streaming_state(struct bdaddr_t *addr);

void pts_send_discover_cmd(void);
void pts_send_get_capability_cmd(void);
void pts_send_set_configuration_cmd(void);
void pts_send_get_configuration_cmd(void);
void pts_send_reconfigure_cmd(void);
void pts_send_open_cmd(void);
void pts_send_close_cmd(void);
void pts_send_abort_cmd(void);
void pts_send_get_all_capability_cmd(void);
void pts_send_suspend_cmd(void);
void pts_send_start_cmd(void);

const char *avdtp_state2str(avdtp_state_t state);
const char *avdtp_event2str(enum avdtp_event_enum event);


#if defined(__cplusplus)
}
#endif

#endif /* __AVDTP_I_H__ */