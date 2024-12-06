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

#ifndef __OBEX_I_H__
#define __OBEX_I_H__

#include "obex.h"
#include "sdp.h"

#define OBEX_AUTHENTICATION	0
#define AUTH_NONCE_LEN			16

typedef uint8 obex_opcode;

enum obex_conn_state_enum{
	OBEX_TP_CONNECTING,
	OBEX_TP_CONNECTED,
	OBEX_TP_DISCONNECTING,
	OBEX_TP_DISCONNECTED
};

enum obex_state_enum{
	OBEX_IDLE,
	OBEX_STANDBY,
	OBEX_CONNECTING,
	OBEX_CONNECTED,
	OBEX_DISCONNECTING,
	//OBEX_DISCONNECTED,
	OBEX_GET,
	OBEX_ABORT,
	OBEX_SET_PATH
};

#define OBEX_FLAG_CONTINUE	0x01
#define OBEX_FLAG_CHALLENGE	0x02

struct obex_client{
	uint8 flags;
	obex_opcode opcode;
	
	void (*indicate_cb)(enum obex_event_t event);
	void (*data_cb)(uint8 *data, uint16 len);

	uint8 *tx_buffer;
	uint16 tx_length;
	
	uint8 rfcomm_handle;
};

struct obex_control_t{
	uint8_t timer_handle;
	struct bdaddr_t remote;
	//uint8 rfcomm_handle;

	enum obex_conn_state_enum conn_state;
	enum obex_state_enum state;

	/*uint16 sendlen;
	char sendData[OBEX_SENDDATA_BUF_SIZE];*/

	void (*indicate_cb)(enum obex_event_t event);
	void (*data_cb)(struct pp_buff *ppb);

	struct obex_client *pbap_client;
};

int8 obex_turnOn(struct obex_client *client);
int8 obex_turnOff(void);
int8 obex_parse_tx(struct obex_client *client);

#endif /* __OBEX_I_H__ */