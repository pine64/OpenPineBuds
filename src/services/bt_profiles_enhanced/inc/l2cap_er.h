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



#ifndef __L2CAP_ER_H__
#define __L2CAP_ER_H__

#include "l2cap.h"
#include "co_ppbuff.h"

#if SUPPORT_L2CAP_ENHANCED_RETRANS==1

/* retransmission and flow control options for enhanced retransmission mode */
#define L2CAPE_RFC_TXWINDOW					5 //in enhanced retransmission mode, this value should be between 1 and 63. 10 is just a reference, need to be modified.
#define L2CAPE_RFC_MAXTRANSMIT				3
#define L2CAPE_RFC_RETRANSMISSION_TIMEOUT	2000
#define L2CAPE_RFC_MONITOR_TIMEOUT			12000
#define L2CAPE_RFC_MPS							666//666L2CAP_CFG_MTU - L2CAPE_CONTROL_LEN - L2CAPE_SDULEN_LEN - L2CAPE_FCS_LEN

#define L2CAPE_CONTROL_LEN	2
#define L2CAPE_SDULEN_LEN		2
#define L2CAPE_FCS_LEN			2
#define L2CAPE_PPB_RESERVE	(L2CAPE_CONTROL_LEN+L2CAP_PPB_RESERVE)	//in our design SAR is always 00, so L2CAPE_SDULEN_LEN is useless

#define L2CAPE_STATE_XMIT			0x00
#define L2CAPE_STATE_WAIT_F		0x01

#define L2CAPE_STATE_RECV			0x00
#define L2CAPE_STATE_REJ_SEND		0x01
#define L2CAPE_STATE_SREJ_SEND	0x02

#define L2CAPE_FLAG_REMOTE_BUSY	(1<<0)
#define L2CAPE_FLAG_LOCAL_BUSY	(1<<1)
#define L2CAPE_FLAG_RNR_SENT		(1<<2)
/* After S-frame with P-bit set, a rej frame with F-bit is 0 was received before receive a frame with F-bit is set, this flag will be set. */
#define L2CAPE_FLAG_REJ_ACTIONED	(1<<3)
#define L2CAPE_FLAG_SREJ_ACTIONED	(1<<4)
#define L2CAPE_FLAG_SEND_REJ		(1<<5)
#define L2CAPE_FLAG_F_BIT_SET		(1<<6)	//indicate the bit in received frame
#define L2CAPE_FLAG_P_BIT_SET		(1<<7)
#define L2CAPE_FLAG_FCS_USED		(1<<8)
#define L2CAPE_FLAG_SET_F_BIT		(1<<9)	//indicate the bit will be set or not in sending frame
#define L2CAPE_FLAG_SET_P_BIT		(1<<10)
#define L2CAPE_FLAG_SEND_ACK		(1<<11)	//when receive SAR_UNSEG or SAR_END, this bit is set

#define L2CAPE_SREJ_SUPPORTED		FALSE

#define L2CAPE_EVENT_DATA_REQUEST			0x00
#define L2CAPE_EVENT_LOCAL_BUSY_DETECTED	0x01
#define L2CAPE_EVENT_LOCAL_BUSY_CLEAR		0x02
#define L2CAPE_EVENT_RECV_REQSEQ_F			0x03
#define L2CAPE_EVENT_RECV_F					0x04
#define L2CAPE_EVENT_RETRANSMIT_EXPIRED	0x05
#define L2CAPE_EVENT_MONITOR_EXPIRED		0x06
#define L2CAPE_EVENT_RECV_I_FRAME			0x07
#define L2CAPE_EVENT_RECV_RR				0x08
#define L2CAPE_EVENT_RECV_REJ				0x09
#define L2CAPE_EVENT_RECV_RNR				0x0A
#define L2CAPE_EVENT_RECV_SREJ				0x0B

#define L2CAPE_SAR_UNSEG		0x00
#define L2CAPE_SAR_START		0x01
#define L2CAPE_SAR_END			0x02
#define L2CAPE_SAR_CONTINUE	0x03

#define L2CAPE_SFRAME_TYPE_RR		0x00
#define L2CAPE_SFRAME_TYPE_REJ	0x01
#define L2CAPE_SFRAME_TYPE_RNR	0x02
#define L2CAPE_SFRAME_TYPE_SREJ	0x03

struct l2cap_enhanced_control_i {
	uint32 i_bit:1;
	uint32 txSeq:6;
	uint32 f_bit:1;
	uint32 reqSeq:6;
	uint32 sar:2;
}__attribute__ ((packed));

struct l2cap_enhanced_control_s {
	uint32 s_bit:1;
	uint32 reserve1:1;
	uint32 type:2;
	uint32 p_bit:1;
	uint32 reserve2:2;
	uint32 f_bit:1;
	uint32 reqSeq:6;
	uint32 reserve3:2;
}__attribute__ ((packed));

/* this struct is used to store sending data, we don't support segment, so sequence is always 0 */
struct l2cap_enhanced_packet {
	struct pp_buff *ppb;		//a whole frame before segmented is stored in this ppb
	struct l2cap_enhanced_packet *next;
	struct l2cap_enhanced_control_i *i_frame;
	//uint8 sequence;				//the position of this packet in the origion frame
};

union l2cap_enhanced_control {
	void *arg;
	struct l2cap_enhanced_control_i *i_frame;
	struct l2cap_enhanced_control_s *s_frame;
};

struct l2cap_enhanced_channel {
	uint32 flags;
	uint8 tx_state;
	uint8 rx_state;

	uint8 maxRxWindow;
	uint8 maxTransmit;
	uint16 retransTimeout;
	uint16 monitorTimeout;
	uint16 sendAckTimeout;
	uint8 retransTimer;
	uint8 monitorTimer;
	uint8 sendAckTimer;

	/* sending peer variables and sequence numbers */
	uint8 maxTxWindow;
	uint8 nextTxSeq;		//the next I-frame to be transmitted
	uint8 expectedAckSeq;	//the next I-frame expected to be acknowledged by receiving peer
	uint8 unackedFrames;	//holds the number of unacknowledged I-frames
	struct l2cap_enhanced_packet *sendingList;		//hold the unacknowledged I-frames and pending I-frames
	struct l2cap_enhanced_packet *pendingList;		//hold the pending I-frames, it is a member of sendingList
	uint8 retryIframes[64];	//holds a retry counter for each I-frame that is sent within the receiving device's TxWindow
	//uint8 pendingFrames;	//holds the number of pending I-frames
	//struct pp_buff *send_buf;	//it's used as a queue, and store encapsulated data can be sent directly by __l2cap_send_data_ppb
	uint8 retryCount;		//holds the number of times an S-frame operation is retried
	//uint8 framesSent;

	/* receiving peer variables and sequence numbers */
	uint8 txSeqRx;			//the sequence of new I-frame
	uint8 reqSeqRx;			//the sequence number in an acknowledgement frame to request I-frame with TxSeq=ReqSeq and acknowledge receipt of I-frames up to and including (ReqSeq-1)
	uint8 expectedTxSeq; 	//the value of TxSeq expected in the next I-frame
	uint8 lastAckSeq;
	//uint8 bufferSeq;		//in this design, bufferSeq-1 is used as lastAckSeq
	uint8 currentRxWindow;
	uint8 bufferSeqSrej;
	uint8 srejSaveReqSeq;
	struct pp_buff *recv_buf;	//the latest received ppb buffer
	struct pp_buff *seg_buf;	//when the SDU is segmented, we use this buffer to do reassemble

	union l2cap_enhanced_control control_field;
};

struct pp_buff *l2cap_enre_data_ppb_alloc(struct l2cap_channel *channel, uint32 len);
void l2cap_enre_free_channel(struct l2cap_enhanced_channel *eCh);
int8 l2cap_enre_send_data_ppb(struct l2cap_channel *channel, struct pp_buff *ppb);
void l2cap_enre_receive_data(struct l2cap_channel *channel, struct pp_buff *ppb);
uint8 l2cap_enre_channel_add_new(struct l2cap_channel *channel);

#endif	//if SUPPORT_L2CAP_ENHANCED_RETRANS==1

#endif /* __L2CAP_ER_H__ */