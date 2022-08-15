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

#ifndef __AVDTP_H__
#define __AVDTP_H__

#include "avdtp_i.h"
#include "l2cap_i.h"
#include "btm.h"

#define MAX_SEID 0x3E

#define AVDTP_DISCOVER 0x01
#define AVDTP_GET_CAPABILITIES 0x02
#define AVDTP_SET_CONFIGURATION 0x03
#define AVDTP_GET_CONFIGURATION 0x04
#define AVDTP_RECONFIGURE 0x05
#define AVDTP_OPEN 0x06
#define AVDTP_START 0x07
#define AVDTP_CLOSE 0x08
#define AVDTP_SUSPEND 0x09
#define AVDTP_ABORT 0x0A
#define AVDTP_SECURITY_CONTROL 0x0B
#define AVDTP_GET_ALL_CAPABILITIES 0x0C
#define AVDTP_DELAYREPORT 0x0D

#define AVDTP_PKT_TYPE_SINGLE 0x00
#define AVDTP_PKT_TYPE_START 0x01
#define AVDTP_PKT_TYPE_CONTINUE 0x02
#define AVDTP_PKT_TYPE_END 0x03

#define AVDTP_MSG_TYPE_COMMAND 0x00
#define AVDTP_MSG_TYPE_GENERAL_REJECT 0x01
#define AVDTP_MSG_TYPE_ACCEPT 0x02
#define AVDTP_MSG_TYPE_REJECT 0x03

/* AVDTP error definitions */
#define AVDTP_BAD_HEADER_FORMAT 0x01
#define AVDTP_BAD_LENGTH 0x11
#define AVDTP_BAD_ACP_SEID 0x12
#define AVDTP_SEP_IN_USE 0x13
#define AVDTP_SEP_NOT_IN_USE 0x14
#define AVDTP_BAD_SERV_CATEGORY 0x17
#define AVDTP_BAD_PAYLOAD_FORMAT 0x18
#define AVDTP_NOT_SUPPORTED_COMMAND 0x19
#define AVDTP_INVALID_CAPABILITIES 0x1A
#define AVDTP_BAD_RECOVERY_TYPE 0x22
#define AVDTP_BAD_MEDIA_TRANSPORT_FORMAT 0x23
#define AVDTP_BAD_RECOVERY_FORMAT 0x25
#define AVDTP_BAD_REPORT_FORMAT 0x65
#define AVDTP_BAD_ROHC_FORMAT 0x26
#define AVDTP_BAD_CP_FORMAT 0x27
#define AVDTP_BAD_MULTIPLEXING_FORMAT 0x28
#define AVDTP_UNSUPPORTED_CONFIGURATION 0x29
#define AVDTP_BAD_STATE 0x31

#define AVDTP_CONN_TIMEOUT //(HZ * 30)
#define AVDTP_DISC_TIMEOUT //(HZ * 20)
#define AVDTP_AUTH_TIMEOUT //(HZ * 25)

#define AVDTP_DEFAULT_MTU 127
#define AVDTP_DEFAULT_CREDITS 7

#define AVDTP_MAX_L2CAP_MTU 1013
#define AVDTP_MAX_CREDITS 40

#define AVDTP_PPB_HEAD_RESERVE 8
#define AVDTP_PPB_TAIL_RESERVE 2
#define AVDTP_PPB_RESERVE (AVDTP_PPB_HEAD_RESERVE + AVDTP_PPB_TAIL_RESERVE)

typedef struct
{
	U8 version; /* RTP Version */

	U8 padding; /* If the padding bit is set, the packet contains 
                         * one or more additional padding octets at the end, 
                         * which are not parts of the payload.  The last 
                         * octet of the padding contains a count of how many 
                         * padding octets should be ignored.  
                         */

	U8 marker; /* Profile dependent.  Used to mark significant 
                         * events such as frame boundaries in the packet 
                         * stream.  
                         */

	U8 payloadType; /* Profile dependent.  Identifies the RTP payload 
                         * type.  
                         */

	U16 sequenceNumber; /* Incremented by one for each packet sent */

	U32 timestamp; /* Time stamp of the sample */

	U32 ssrc; /* Synchronization source */

	U8 csrcCount; /* The number of CSRC (Contributing Source) 
                         * identifiers that follow the fixed header.  
                         */

	U32 csrcList[15]; /* List of CSRC identifiers */

} avdtp_media_header_t;

struct avdtp_header
{
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;
	//	uint32 unused:16;
} __attribute__((packed));

struct seid_req
{
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;
	uint32 rfa1 : 2;
	uint32 acp_seid : 6;
	uint8 param[0];
	//	uint32 unused:8;
} __attribute__((packed));

struct discover_resp
{
	//	struct avdtp_header header;
	//	struct seid_info *seps;
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;

	uint32 rfa1 : 1;
	uint32 inuse : 1;
	uint32 seid : 6;
	uint32 rfa2 : 3;
	uint32 type : 1;
	uint32 media_type : 4;
} __attribute__((packed));

struct discover_rej
{
	//	struct avdtp_header header;
	//	struct seid_info *seps;
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;
    uint8  error_code;
} __attribute__((packed));



struct security_control_req
{
	//avdtp hrader
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;
	uint32 rfa2 : 2;
	uint32 acp_seid : 6;
	//Content Protection Method Dependent Data
	uint8 *data;
} __attribute__((packed));

struct security_control_resp
{
	struct avdtp_header header;
	uint8 *data;
} __attribute__((packed));

struct getcap_resp
{
	struct avdtp_header header;
	uint8 *caps;
} __attribute__((packed));

struct getcap_req
{
    uint32 message_type : 2;
    uint32 packet_type : 2;
    uint32 transaction : 4;
    uint32 signal_id : 6;
    uint32 rfa0 : 2;
    uint32 rfa2 : 2;
    uint32 ACP_seid : 6;
} __attribute__((packed));


struct start_req
{
	struct avdtp_header header;

	uint8 first_seid;
	uint8 *other_seids;
} __attribute__((packed));

struct suspend_req
{
	struct avdtp_header header;

	uint8 first_seid;
	uint8 *other_seids;
} __attribute__((packed));

struct setconf_req
{
	//	struct avdtp_header header;
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;

	uint32 rfa2 : 2;
	uint32 acp_seid : 6;
	uint32 rfa1 : 2;
	uint32 int_seid : 6;

	uint8 *caps;
} __attribute__((packed));

struct reconf_req
{
	//	struct avdtp_header header;
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;

	uint32 rfa2 : 2;
	uint32 acp_seid : 6;

	uint8 *caps;
} __attribute__((packed));
struct general_rej
{
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	//uint32 rfa0:8;
	uint32 signal_id : 6; //modified by owen.liu, for support version 1.3
	uint32 rfa0 : 2;
} __attribute__((packed));

struct seid_rej
{
	//	struct avdtp_header header;
	uint32 message_type : 2;
	uint32 packet_type : 2;
	uint32 transaction : 4;
	uint32 signal_id : 6;
	uint32 rfa0 : 2;

	uint32 error : 8;
	uint32 unused : 8;
} __attribute__((packed));

struct conf_rej
{
	struct avdtp_header header;
	uint8 category;
	uint8 error;
} __attribute__((packed));

struct stream_rej
{
	struct avdtp_header header;
	uint8 acp_seid;
	uint8 error;
} __attribute__((packed));

struct avdtp_session
{
	uint32 l2cap_handle;
	uint8 state;
} __attribute__((packed));

enum avdtp_session_state_enum
{
	AVDTP_SESSION_CLOSE,	 /* l2cap connection closed, wait for openning, and then can send out sabm request */
	AVDTP_SESSION_CONNECTED, /* l2cap channel created */
	AVDTP_SESSION_OPEN		 /* avdtp session open, ready for start stream */
};



//int8 avdtp_open(struct avdtp_session *session, struct avdtp_stream *stream);

/* flow control states */
#define AVDTP_CFC_DISABLED 0
#define AVDTP_CFC_ENABLED AVDTP_DEFAULT_CREDITS

#define AVDTP_CFG_SESSIONS_MAX 5 /* means how many l2cap channel */

#if defined(__cplusplus)
extern "C" {
#endif


int8 avdtp_send(uint32 avdtp_handle, uint8 *data, uint32 datalen);
int avdtp_l2cap_notify_callback(enum l2cap_event_enum event, uint32 l2cap_handle, void *pdata, uint8 reason);
void avdtp_l2cap_datarecv_callback(uint32 l2cap_handle, struct pp_buff *ppb);
struct avdtp_control_t *avdtp_ctl_search_l2caphandle(uint32 l2cap_handle);
void avdtp_free_remote_sep_list(void);
struct avdtp_local_sep* avdtp_find_same_device_other_sep(struct avdtp_local_sep *sep);
#if defined(__cplusplus)
}
#endif

#endif /* __AVDTP_H__ */