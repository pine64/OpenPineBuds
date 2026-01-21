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
#ifndef __HSHF_I_H__
#define __HSHF_I_H__

#include "btlib_type.h"
#include "rfcomm_i.h"
#include "co_queue.h"
#include "bt_co_list.h"
#include "sdp.h"
#include "bt_sys_cfg.h"

//#define _THREE_WAY_CALL

//#define PRO_PCDEMO
struct hshf_control;

typedef void (*hfp_callback_t)(struct hshf_control *, uint8_t, void *);

enum hshf_tx_status {
    HFP_TX_IDLE = 0,
    HFP_TX_BUSY,
};

/* notify upper layer */
enum hshf_event_t {
    /* user command event*/
    HSHF_ENTER_PAIRING = 1,
    HSHF_EXIT_PAIRING,
    HF_DIAL_NUM_REQ,
    HF_ANSWER_REQ,
    HF_REJECT_REQ,
    HF_ENDCALL_REQ,
    HF_TRANSFER_REQ,
    HF_DIAL_LASTNUM_REQ,
    HF_TRANSMIT_DTMF,
    HF_VOICE_DIAL_REQ, //10
    HF_VOICE_DIAL_CANCEL_REQ,
    HSHF_CONNECT_REQ,
    HF_DISCONNECT_REQ,
    HSHF_SPK_VOL_UP,
    HSHF_SPK_VOL_DOWN,
    HSHF_TOGGLE_MIC,
    HSHF_TOGGLE_LED,
    HSHF_TOGGLE_VOLBTN,
    HF_ANSWER_ENDCALL_CONN,     //  answer @ ring, end @ talking, connect@idle
    HF_REJ_REDIAL_TRANS_CONN,   // reject @ ring, redial @ connected, transfer @ talking
    HF_RELEASE_HOLD_REJECT_WAIT_REQ, //21
    HF_RELEASE_ACTVIE_ACCEPT_OTHER_REQ,
    HF_HOLD_ACTIVE_ACCEPT_OTHER_REQ,
    HF_CONFERENCE_REQ,
    HSHF_SET_PB_STORAGE,
    HSHF_GET_PB_ITEM,

    HF_HCI_RXTX_IND,

    /* internal event */
    HSHF_EVNT_BEGIN,     //28
    HSHF_RFCOMM_OPENED, //29
    HSHF_CONN_OPENED, 
    HSHF_CONN_CLOSED,
    HSHF_CONN_REQ_FAIL,
    HF_AG_SUPPORTED_FEATURE_IND,
    HF_AG_SUPPORTED_INDICATOR_IND,
    HF_AG_CURRENT_INDICATOR_IND,
    HF_INDICATOR_EVENT_IND,
    HF_CIEV_CALL_IND,
    HF_CIEV_SERVICE_IND,
    HF_CIEV_CALLSETUP_IND,    //38
    HF_CALLER_ID_IND,
    HF_VOICE_IND,
    HSHF_RING_IND,  //41
    HSHF_AUDIOCONN_OPENED,
    HSHF_AUDIOCONN_CLOSED,
    HSHF_SPK_VOL_IND,
    HSHF_MIC_VOL_IND,
    HF_IN_BAND_RING_IND,
    HSHF_PAIR_OK,  //47
    HSHF_PAIR_TOUT,
    HSHF_PAIR_FAILED,
    //// NEW for three way call
    HF_CIEW_CALLHELD_IND,
    HF_CCWA_IND,
    HF_VOICE_REQ,
    // for enter pairing and test mode by combkey
    HSHF_ENTER_TESTMODE,
    HSHF_CALL_IND,
    HF_EVENT_AT_RESULT_DATA,
};

enum hshf_callsetup {
    CALL_SETUP_NONE,
    CALL_SETUP_INCOMING,
    CALL_SETUP_OUTGOING,
    CALL_SETUP_REMOTE_ALERT,
    CALL_SETUP_ESTABLISHED
};

enum hshf_call {
	CALL_NONE = 0,
	CALL_ESTABLISHED
};

enum hshf_profile {
    PRO_BOTH = 0,
    PRO_HEADSET,
    PRO_HANDSFREE,
    PRO_EXIT,
    PRO_SHOW
};

enum hshf_conn_state {
    STOP,
    STANDBY = 1,
    LISTENING ,  //ready
    QUERING,
    CONNECTING,
    AT_EXCHANGING,
    CONNECTED,
    SCOCONNECTED
};

enum hshf_pb_location {
    LOCATION_SM = 0,
    LOCATION_ME,
    LOCATION_MT,
    LOCATION_DC,
    LOCATION_RC,
    LOCATION_MC,
    LOCATION_LD
};

enum hshf_pb_action {
    ACTION_PREV = 0,
    ACTION_NEXT
};

enum hfp_indicator {
	HFP_INDICATOR_SERVICE = 0,
	HFP_INDICATOR_CALL,
	HFP_INDICATOR_CALLSETUP,
	HFP_INDICATOR_CALLHELD,
	HFP_INDICATOR_SIGNAL,
	HFP_INDICATOR_ROAM,
	HFP_INDICATOR_BATTCHG,
	HFP_INDICATOR_LAST
};

typedef void (*ciev_func_t)(struct hshf_control *chan, uint8_t val);

struct indicator {
    uint8_t index;
    uint8_t min;
    uint8_t max;
    uint8_t val;
    bool disable;
    ciev_func_t cb;
};

struct indicator;

#define HFP_HF_IND_ENHANCED_SAFETY 1
#define HFP_HF_IND_BATTERY_LEVEL   2

struct hf_ind_enhanced_safety {
    bool local_support;
    bool remote_support;
    bool enabled;
    uint8_t value; /* 0 or 1 */
};

struct hf_ind_battery_level {
    bool local_support;
    bool remote_support;
    bool enabled;
    uint8_t value; /* 0 ~ 100 */
};

struct hf_indicator {
    struct hf_ind_enhanced_safety enhanced_safety;
    struct hf_ind_battery_level battery_level;
};

#define MAX_DIAL_NUM_SIZE     0x10
#define MAX_SAVED_CALL_NUM    4

#define CODEC_ID_CVSD 0x01
#define CODEC_ID_MSBC 0x02
struct hfp_codec {
	uint8_t type;
	bool local_supported;
	bool remote_supported;
};

#if defined(HFP_MOBILE_AG_ROLE)
struct hfp_ag_call_info
{
    uint8_t direction; // 0 outgoing, 1 incoming
    uint8_t state; // 0 active, 1 held, 2 outgoing dialing, 3 outgoing alerting, 4 incoming, 5 waiting, 6 held by Response and Hold
    uint8_t mode; // 0 voice, 1 data, 2 fax
    uint8_t multiparty; // 0 is not one of multiparty call parties, 1 is one of.
    const char* number; // calling number, optional
};

typedef int (*hfp_mobile_handler)(void* hfp_chan);
typedef int (*hfp_mobile_handler_int)(void* hfp_chan, int n);
typedef int (*hfp_mobile_handler_str)(void* hfp_chan, const char* s);
typedef int (*hfp_mobile_iterate_call_handler)(void* hfp_chan, struct hfp_ag_call_info* out);
typedef const char* (*hfp_mobile_query_operator_handler)(void* hfp_chan);

struct hfp_mobile_module_handler
{
    hfp_mobile_handler answer_call;
    hfp_mobile_handler hungup_call;
    hfp_mobile_handler dialing_last_number;
    hfp_mobile_handler release_held_calls;
    hfp_mobile_handler release_active_and_accept_calls;
    hfp_mobile_handler hold_active_and_accept_calls;
    hfp_mobile_handler add_held_call_to_conversation;
    hfp_mobile_handler connect_remote_two_calls;
    hfp_mobile_handler disable_mobile_nrec;
    hfp_mobile_handler_int release_specified_active_call;
    hfp_mobile_handler_int hold_all_calls_except_specified_one;
    hfp_mobile_handler_int hf_battery_change; /* battery level 0 ~ 100 */
    hfp_mobile_handler_int hf_spk_gain_change; /* speaker gain 0 ~ 15 */
    hfp_mobile_handler_int hf_mic_gain_change; /* mic gain 0 ~ 15 */
    hfp_mobile_handler_int transmit_dtmf_code;
    hfp_mobile_handler_int memory_dialing_call;
    hfp_mobile_handler_str dialing_call;
    hfp_mobile_handler_str handle_at_command;
    hfp_mobile_query_operator_handler query_current_operator;
    hfp_mobile_iterate_call_handler iterate_current_call;
};
#endif

struct hshf_control {
    struct list_node hfp_node;
    struct bdaddr_t remote;
    uint32_t rfcomm_handle;
    uint8 listen_channel;
    uint8 disc_reason;
    uint8 audio_up;
	struct coqueue *cmd_queue;

#if HFP_CMD_FLOW_CONTROL_ENABLE==1
    unsigned int tx_time;
    uint8_t tx_timeout_timer;
    enum hshf_tx_status tx_status;
#endif

	struct coqueue *event_handlers;
	uint8_t negotiated_codec;

	struct hfp_codec codecs[2];

	struct indicator ag_ind[HFP_INDICATOR_LAST];

    struct hf_indicator hf_ind;

	uint32_t chld_features;

    uint8 bsir_enable;
    uint8 status_call;             /* Phone status info - call */
    uint8 status_service;          /* Phone status info - service */
    uint8 status_callsetup;		  /* Phone status info - callsetup*/
    uint8 status_callheld;		  /* Phone status info - callheld*/

#if defined(HFP_MOBILE_AG_ROLE)
    uint32_t slc_completed: 1;
    uint32_t ag_status_report_enable: 1;
    uint32_t calling_line_notify: 1;
    uint32_t call_waiting_notify: 1;
    uint32_t extended_error_enable: 1;
    struct hfp_mobile_module_handler* mobile_module;
#endif

    uint32_t hf_features;    /*hf supported feature bitmap:            */
                                /* bit 0 - EC/NR function                */
                                /*     1 - Call waiting and 3-way calling */
                                /*     2 - CLI presentation capability   */
                                /*     3 - Voice recognition activation   */
                                /*     4 - Remote volume control         */
                                /*     5 - Enhance call status            */
                                /*     6 - Enhanced call control         */
    uint32_t ag_features;   /* AG supported feature bitmap  */
                                /* bit0 - 3-way calling      */
                                /*    1 - EC/NR function    */
                                /*    2 - Voice recognition  */
                                /*    3 - In-band ring tone */
                                /*    4 - Attach a number to a voice tag*/
                                /*    5 - Ablility to reject a call */
                                /*    6 - Enhanced call status     */
                                /*    7 - Enhanced call control     */
                                /*    8 - extended error result codes*/

    enum hshf_conn_state state;   /*check if it is connecting now,
                                   if it is connecting,the hf_connect_req
                                   will not work
                                   */
    uint8 speak_volume;
    uint8 mic_gain;
    uint8 voice_rec;
    uint8 voice_rec_param;
    bool client_enabled;
    char caller_id[MAX_DIAL_NUM_SIZE+1]; /* incoming caller id */

    struct sdp_request sdp_request;
    char *ptr;
};

struct hfp_ctx_input {
    struct ctx_content ctx;
    struct bdaddr_t *remote;
    uint32 rfcomm_handle;
    struct hshf_control *hfp_ctl;
};

struct hfp_ctx_output {
    uint32 rfcomm_handle;
};

#if defined(__cplusplus)
extern "C" {
#endif
/*----hshf.c----*/
enum hshf_conn_state hshf_get_state(struct hshf_control *chan);
void hshf_set_state(struct hshf_control *chan, enum hshf_conn_state state);
const char *hshf_state2str(enum hshf_conn_state state);
const char *hshf_event2str(enum hshf_event_t event);

typedef struct {
    uint8 length;
    char *caller_id;
} hf_caller_id_ind_t;

int8 hf_release_sco(struct bdaddr_t *bdaddr, uint8 reason);

/*----connect.c----*/
int8 hf_disconnect_req(struct hshf_control *chan);
int8 hshf_exit_sniff(struct hshf_control *chan);
int8 hshf_connect_req (struct hshf_control *hshf_ctl, struct bdaddr_t *remote);

/* - hfp.c - */
int8 hshf_create_codec_connection(struct bdaddr_t *bdaddr, struct hshf_control *chan);
int8 hf_createSCO(struct bdaddr_t *bdaddr, void *chan);
int hfp_init(hfp_callback_t callback);

int hf_open_chan(struct hshf_control *chan);

int8 hf_close_chan(struct hshf_control *chan);

bool hshf_disable_nrec(struct hshf_control *chan);

bool hshf_report_speaker_volume(struct hshf_control *chan, uint8_t gain);

bool hshf_send_custom_cmd(struct hshf_control *chan, const char *cmd);

bool hshf_hungup_call(struct hshf_control *chan);

bool hshf_dial_number(struct hshf_control *chan, uint8 *number, uint16 len);

bool hshf_answer_call(struct hshf_control *chan);

bool hshf_redial_call(struct hshf_control *chan);

bool hshf_batt_report(struct hshf_control *chan, uint8_t level);

bool hshf_call_hold(struct hshf_control *chan, int8 action, int8 index);

void hshf_set_hf_indicator_enabled(bool enable);
bool hshf_report_enhanced_safety(struct hshf_control *chan, uint8_t value);
bool hshf_report_battery_level(struct hshf_control* chan, uint8_t value);

bool hshf_update_indicators_value(struct hshf_control *chan, uint8_t assigned_num, uint8_t level);
bool hshf_hf_indicators_1(struct hshf_control *chan);
bool hshf_hf_indicators_2(struct hshf_control *chan);
bool hshf_hf_indicators_3(struct hshf_control *chan);
bool hshf_codec_conncetion(struct hshf_control *chan);
bool hshf_list_current_calls(struct hshf_control *chan);

bool hshf_enable_voice_recognition(struct hshf_control *chan, uint8_t en);

bool hshf_is_voice_recognition_active(struct hshf_control *chan);

uint32 hfp_get_rfcomm_handle(struct hshf_control *hfp_ctl);

uint32 hfp_save_ctx(struct hshf_control *hfp_ctl, uint8_t *buf, uint32_t buf_len);

uint32 hfp_restore_ctx(struct hfp_ctx_input *input, struct hfp_ctx_output *output);

#if defined(__cplusplus)
}
#endif

#endif /* __HSHF_I_H__ */