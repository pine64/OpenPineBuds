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


#ifndef __L2CAP_H__
#define __L2CAP_H__

#include "btlib_type.h"
#include "co_ppbuff.h"
#include "bt_co_list.h"
#include "l2cap_i.h"
#include "btm_i.h"

typedef void (*l2cap_sdp_disconnect_callback)(const void *para);
typedef uint8 (*btm_get_ibrt_role_callback)(const void *para);
typedef uint8 (*btm_get_tss_state_callback)(const void *para);
/* base frame */
#define L2CAP_SIG_CID 0x0001
#define L2CAP_CONNECTIONLESS_CID 0x0002

#define L2CAP_CHANNEL_NUM_PER_LINK     (9) /* rfcomm (1) + avrcp (1) + avdtp (2) + sdp (4) + extra (1) */
#define CID_INDEX_OFFSET               (0x40)
#define l2cap_cid_to_index(cid)        (cid - CID_INDEX_OFFSET)
#define l2cap_index_to_cid(index)      (index + CID_INDEX_OFFSET)

struct l2cap_hdr {
    uint16 len;
    uint16 cid;
}__attribute__ ((packed));

/* command code */
#define L2CAP_SIG_REJ 0x01
#define L2CAP_SIG_CONN_REQ 0x02
#define L2CAP_SIG_CONN_RSP 0x03
#define L2CAP_SIG_CFG_REQ 0x04
#define L2CAP_SIG_CFG_RSP 0x05
#define L2CAP_SIG_DISCONN_REQ 0x06
#define L2CAP_SIG_DISCONN_RSP 0x07
#define L2CAP_SIG_ECHO_REQ 0x08
#define L2CAP_SIG_ECHO_RSP 0x09
#define L2CAP_SIG_INFO_REQ 0x0A
#define L2CAP_SIG_INFO_RSP 0x0B

//Not used since inter-operation with special BT insturement
#if 0
/* indentifier id */
#define L2C_SIG_ID_REMAP_OFFSET    (100)
#define L2C_SIG_CONN_REQ_ID        (1)
#define L2C_SIG_DISCONN_REQ_ID     (21)
#define L2C_SIG_CONFIG_REQ_ID      (41)
#define L2C_SIG_ECHO_REQ_ID        (61)
#define L2CAP_SIG_INFO_REQ_ID      (81)
#define L2CAP_SIG_DEFAULT_ID       (1)
#endif

struct l2cap_sig_hdr {
    byte code;
    byte id;
    uint16 len;
}__attribute__ ((packed));


#define L2CAP_SIG_REASON_NOT_UNDERSTOOD 0x0
#define L2CAP_SIG_REASON_MTU_EXCEED 0x1
#define L2CAP_SIG_REASON_INVALID_CID 0x2
struct l2cap_sig_rej {

    uint16 reason;
/*data*/
    uint16 scid;            /*the data len is 0 - 4*/
    uint16 dcid;
}__attribute__ ((packed));

struct l2cap_sig_conn_req {
    uint16 psm;
    uint16 scid;
}__attribute__ ((packed));

#define L2CAP_SIG_RESULT_SUCCESS 0x0
#define L2CAP_SIG_RESULT_PENDING 0x1
#define L2CAP_SIG_RESULT_REFUSE_PSM 0x2
#define L2CAP_SIG_RESULT_REFUSE_SECURITY 0x3
#define L2CAP_SIG_RESULT_REFUSE_RESOURCE 0x4    

#define L2CAP_SIG_RESULT_PENDING_NOINFO 0x00    
#define L2CAP_SIG_RESULT_PENDING_AUTHEN 0x01
#define L2CAP_SIG_RESULT_PENDING_AUTHOR 0x02
struct l2cap_sig_conn_rsp {
    uint16 dcid;
    uint16 scid;

    uint16 result;
    uint16 status;   /*only defined when result = pending */
}__attribute__ ((packed));

struct l2cap_sig_cfg_req {
    uint16 dcid;
    uint16 flags;           /* bit0=1:continue  bit0=0:complete  */
}__attribute__ ((packed));

#define L2CAP_CFGRSP_SUCCESS                        0x0000
#define L2CAP_CFGRSP_UNACCEPT_PARAMS       0x0001
#define L2CAP_CFGRSP_REJ                                 0x0002
#define L2CAP_CFGRSP_UNKNOWN                       0x0003
struct l2cap_sig_cfg_rsp {
    uint16 scid;
    uint16 flags;
    uint16 result;
}__attribute__ ((packed));
    
#define L2CAP_CFG_TYPE_MTU		0x01
#define L2CAP_CFG_TYPEF_FLUSH_TO	0x02
#define L2CAP_CFG_TYPE_QOS		0x03
#define L2CAP_CFG_TYPE_RFC		0x04	//retransmission and flow control
#define L2CAP_CFG_TYPE_FCS		0x05
#define L2CAP_CFG_TYPE_EFS		0x05	//extended flow specification
#define L2CAP_CFG_TYPE_EWS		0x06	//extended window size
struct l2cap_sig_cfg_opt_hdr{
    byte type;
    byte len;
}__attribute__ ((packed));

struct l2cap_sig_cfg_opt_mtu {

    uint16 mtu;
}__attribute__ ((packed));
struct l2cap_sig_cfg_opt_flushto {
    
    uint16 flushto;
}__attribute__ ((packed));

#define L2CAP_QOS_NO_TRAFFIC		0x00
#define L2CAP_QOS_BEST_EFFORT		0x01
#define L2CAP_QOS_GUARANTEED		0x02    
struct l2cap_sig_cfg_opt_qos {
   
    byte	 flags;
    byte     service_type;
    uint32  token_rate;
    uint32  token_size;
    uint32  bandwidth;
    uint32  latency;
    uint32  delay_variation;    
}__attribute__ ((packed));

#define L2CAP_MODE_BASE 0
#define L2CAP_MODE_RETRANSMISSION 1
#define L2CAP_MODE_FLOWCONTROL  2   
#define L2CAP_MODE_ENHANCED_RETRANSMISSION	3
#define L2CAP_MODE_STREAMING	4
struct l2cap_sig_cfg_opt_rfc {

    byte     mode;
    byte      txwindow;
    byte      maxtransmit;
    uint16   retransmission_timeout;
    uint16   monitor_timeout;
    uint16   mps;
}__attribute__ ((packed));

#define L2CAP_FCS_TYPE_NONE		0x00
#define L2CAP_FCS_TYPE_16_BIT		0x01
struct l2cap_sig_cfg_opt_fcs {
	byte type;
}__attribute__ ((packed));

struct l2cap_sig_disconn_req {

    uint16 dcid;
    uint16 scid;
}__attribute__ ((packed));

struct l2cap_sig_disconn_rsp {
    uint16 dcid;
    uint16 scid;
}__attribute__ ((packed));

#define L2CAP_INFOTYPE_CONNLESS_MTU         0x01
#define L2CAP_INFOTYPE_EXTENED_FEATURE   0x02
struct l2cap_sig_info_req {
    uint16 infotype;
}__attribute__ ((packed));

#define L2CAP_INFOTYPE_SUCCESS              0x00
#define L2CAP_INFOTYPE_NOT_SUPPORT      0x01

#define L2CAP_INFOTYPE_SUPPORT_FLOWCONTROL_MASK         0x01
#define L2CAP_INFOTYPE_SUPPORT_RETRANSMISSION_MASK     0x02
#define L2CAP_INFOTYPE_SUPPORT_BIQOS_MASK                       0x04
struct l2cap_sig_info_rsp {
    uint16 infotype;
    uint16 result;
    /*if result == success, data: mtu(2 bytes), feature mask(4 bytes) */
    uint32 mask;
}__attribute__ ((packed));

//supported extern features by l2cap
#define L2CAP_EXFEATURE_FLOW_CONTROL        (1<<0)
#define L2CAP_EXFEATURE_RETRANS_MODE        (1<<1)
#define L2CAP_EXFEATURE_BIDIRECT_QOS        (1<<2)
#define L2CAP_EXFEATURE_ENHANCED_RETRANS    (1<<3)
#define L2CAP_EXFEATURE_STREAMING_MODE      (1<<4)
#define L2CAP_EXFEATURE_FCS_OPTIONS         (1<<5)
#define L2CAP_EXFEATURE_EXT_FLOW_SPEC       (1<<6)
#define L2CAP_EXFEATURE_FIXED_CHANNELS      (1<<7)
#define L2CAP_EXFEATURE_EXT_WINDOW_SIZE     (1<<8)
#define L2CAP_EXFEATURE_UCD_RECEPTION       (1<<9)

#define L2CAP_SIG_CFG_MTU_MASK                  (1<<0)
#define L2CAP_SIG_CFG_FLUSHTO_MASK          (1<<1)
#define L2CAP_SIG_CFG_QOS_MASK                  (1<<2)
#define L2CAP_SIG_CFG_RFC_MASK                   (1<<3)
#define L2CAP_SIG_CFG_FCS_MASK                   (1<<4)

struct config_in_t {
    uint8  cfgin_flag;
    struct l2cap_sig_cfg_opt_mtu mtu_local;
    struct l2cap_sig_cfg_opt_flushto flushto_remote;
    struct l2cap_sig_cfg_opt_qos qos_remote;
    struct l2cap_sig_cfg_opt_rfc rfc_local;
    struct l2cap_sig_cfg_opt_fcs fcs_remote;
};

struct config_out_t {
    uint8 cfgout_flag;
    struct l2cap_sig_cfg_opt_mtu mtu_remote;
    struct l2cap_sig_cfg_opt_flushto flushto_local;
    struct l2cap_sig_cfg_opt_qos qos_local;
    struct l2cap_sig_cfg_opt_rfc rfc_remote;
    struct l2cap_sig_cfg_opt_fcs fcs_local;
};

#define L2C_NOTIFY_RESULT_ACCEPT (0)
#define L2C_NOTIFY_RESULT_REJECT (1)
#define L2C_NOTIFY_RESULT_UPPER_LAYER_HANDLED (2)

struct l2cap_registered_psm_item_t {
    struct list_node list;
    uint16 psm;
    int8 conn_count;    /*how many conn can be created*/
    int (*l2cap_notify_callback)(enum l2cap_event_enum event, uint32 l2cap_handle, void *pdata, uint8 reason);
    void (*l2cap_datarecv_callback)(uint32 l2cap_handle, struct pp_buff *ppb);
};

enum l2cap_channel_state_enum {
    L2CAP_CLOSE,                /*baseband connection closed, wait for hci conn openning, and then can send out conn request signal*/
    L2CAP_WAIT_DISCONNECT,
    L2CAP_WAITING,              /* waitf for the baseband connection to send out conn req signal */
    L2CAP_AUTH_PENDING, /* waiting for baseband authentication or encryption */
    L2CAP_WAIT_CONNECTION_RSP,
    L2CAP_WAIT_CONFIG,
    L2CAP_WAIT_CONFIG_REQ_RSP,
    L2CAP_WAIT_CONFIG_RSP, 
    L2CAP_WAIT_CONFIG_REQ,
    L2CAP_OPEN
};

struct l2cap_channel{
    struct list_node list;

    struct l2cap_conn *conn;


    uint32 l2cap_handle;

    uint16 scid;
    uint16 dcid;
    uint16 psm_remote;

    uint8 used;         /* channel used or not*/
    uint8 initiator;    /* local or peer initate l2cap channel*/
    uint8 sigid_last_send;   /*to save our last request signal id*/
    uint8 sigid_last_recv;  /*to save the last remote's request signal id*/

    //max co timer num is less than 255 in our stack,so just one byte is ok
    uint8 disconnect_req_timeout_timer;/*to avoid disconnect req not response,so we need to give a timeout flag*/
    uint8 close_delay_timer;
    uint8 wait_conn_req_timer;
    uint8 wait_config_req_timer;

    uint8 disconnect_req_reason;
    /* for config req and resp */
    uint8 wait_cfg_req_done;
    struct config_in_t cfgin;
    struct config_out_t cfgout;


    enum l2cap_channel_state_enum state;
    
    int (*l2cap_notify_callback)(enum l2cap_event_enum event, uint32 l2cap_handle, void *pdata, uint8 reason);
    void (*l2cap_datarecv_callback)(uint32 l2cap_handle, struct pp_buff *ppb);
#if SUPPORT_L2CAP_ENHANCED_RETRANS==1
    struct l2cap_enhanced_channel *eChannel;
    uint8 channel_mode;
#endif
};

struct l2cap_conn {
    struct list_node list;
    struct bdaddr_t remote;
    uint16 conn_handle;
    uint8 sigid;
    uint8 inuse;

    uint8 disconnect_by_acl;
    uint8 disconnect_reason;
    uint8 delay_free_conn_timer;

    struct list_node channels;
    struct l2cap_channel l2c_channel_array[L2CAP_CHANNEL_NUM_PER_LINK];
};

struct l2cap_conn_req_param_t {
    uint16 conn_handle;
    uint16 remote_scid;
    uint16 psm;
    uint8 identifier;
    struct bdaddr_t remote_addr;
};

#if defined(__cplusplus)
extern "C" {
#endif

struct l2cap_channel * l2cap_accept_conn_req(struct l2cap_conn_req_param_t* req);
void l2cap_reject_conn_req(struct l2cap_conn_req_param_t* req, uint16 reason);

int8 l2cap_send_frame_done(uint16 conn_handle, struct pp_buff *ppb);
uint8* l2cap_make_sig_req(struct l2cap_channel *channel,uint8 sig_code,uint16 sig_datalen,struct pp_buff *ppb);
struct l2cap_conn *l2cap_conn_search_conn_handle(uint16 conn_handle);
struct l2cap_channel *l2cap_channel_search_l2cap_conn(struct l2cap_conn * conn, uint16 psm);
uint16 l2cap_get_conn_handle(struct bdaddr_t *bdaddr);
void l2cap_channel_add_new(struct l2cap_conn *conn,struct l2cap_channel *channel);
struct l2cap_channel *l2cap_channel_search_scid(struct l2cap_conn *conn, uint16 scid);
struct l2cap_channel *l2cap_channel_search_psm(struct l2cap_conn *conn, uint16 psm);
//uint32 l2cap_save_channel_ctx(uint8 *ctxs_buffer, uint16 mobile_handle);
//uint32 l2cap_set_channels_ctx(uint8 *ctxs_buffer, uint8 dev_tbl_idx);
struct l2cap_channel *l2cap_channel_malloc(struct l2cap_conn *conn,uint16 psm,uint16 scid);
void l2cap_channel_free(struct l2cap_channel *channel);
void l2cap_find_and_free_pending_avdtp_channel(struct bdaddr_t* remote);
#if defined(__cplusplus)
}
#endif

#endif /* __L2CAP_H__ */
