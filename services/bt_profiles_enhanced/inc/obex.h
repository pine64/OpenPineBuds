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

#ifndef __OBEX_H__
#define __OBEX_H__

#include "rfcomm_i.h"

enum obex_event_t{
	OBEX_EVENT_TP_CONNECTED,
	OBEX_EVENT_TP_DISCONNECTED,
	OBEX_EVENT_CONNECTED,
	OBEX_EVENT_DISCONNECTED,
	OBEX_EVENT_GET,
	OBEX_EVENT_SET_PATH,
	OBEX_EVENT_ABORT,
	OBEX_EVENT_SUCCESS,
	OBEX_EVENT_FAILED,
	OBEX_EVENT_TIMEOUT
};

#define OBEX_FINAL_BIT				0x80

#define OBEX_OPCODE_CONNECT		0x00
#define OBEX_OPCODE_DISCONNECT	0x01
#define OBEX_OPCODE_PUT			0x02
#define OBEX_OPCODE_GET			0x03
#define OBEX_OPCODE_SET_PATH		0x05
#define OBEX_OPCODE_ABORT			0xFF

typedef uint8 obex_respcode;

#define OBEX_RESP_CONTINUE				0x10 /* Continue */
#define OBEX_RESP_SUCCESS					0x20 /* OK, Success */

#define OBEX_RESP_CREATED					0x21 /* Created */
#define OBEX_RESP_ACCEPTED				0x22 /* Accepted */
#define OBEX_RESP_NON_AUTHOR_INFO			0x23 /* Non-Authoritative Information */
#define OBEX_RESP_NO_CONTENT				0x24 /* No Content */
#define OBEX_RESP_RESET_CONTENT			0x25 /* Reset Content */
#define OBEX_RESP_PARTIAL_CONTENT			0x26 /* Partial Content */

#define OBEX_RESP_MULTIPLE_CHOICES		0x30 /* Multiple Choices */
#define OBEX_RESP_MOVED_PERMANENT			0x31 /* Moved Permanently */
#define OBEX_RESP_MOVED_TEMPORARY			0x32 /* Moved Temporarily */
#define OBEX_RESP_SEE_OTHER				0x33 /* See Other */
#define OBEX_RESP_NOT_MODIFIED			0x34 /* Not Modified */
#define OBEX_RESP_USE_PROXY				0x35 /* Use Proxy */

#define OBEX_RESP_BAD_REQUEST				0x40 /* Bad Request */
#define OBEX_RESP_UNAUTHORIZED			0x41 /* Unauthorized */
#define OBEX_RESP_PAYMENT_REQUIRED		0x42 /* Payment Required */
#define OBEX_RESP_FORBIDDEN				0x43 /* Forbidden - operation is understood but refused */
#define OBEX_RESP_NOT_FOUND				0x44 /* Not Found */
#define OBEX_RESP_METHOD_NOT_ALLOWED		0x45 /* Method Not Allowed */
#define OBEX_RESP_NOT_ACCEPTABLE			0x46 /* Not Acceptable */
#define OBEX_RESP_PROXY_AUTHEN_REQ		0x47 /* Proxy Authentication Required */
#define OBEX_RESP_REQUEST_TIME_OUT		0x48 /* Request Timed Out */
#define OBEX_RESP_CONFLICT				0x49 /* Conflict */

#define OBEX_RESP_GONE						0x4a /* Gone */
#define OBEX_RESP_LENGTH_REQUIRED			0x4b /* Length Required */
#define OBEX_RESP_PRECONDITION_FAILED	0x4c /* Precondition Failed */
#define OBEX_RESP_REQ_ENTITY_TOO_LARGE	0x4d /* Requested entity is too large */
#define OBEX_RESP_REQ_URL_TOO_LARGE		0x4e /* Requested URL is too large */
#define OBEX_RESP_UNSUPPORT_MEDIA_TYPE	0x4f /* Unsupported Media Type */

#define OBEX_RESP_INTERNAL_SERVER_ERR	0x50 /* Internal Server Error */
#define OBEX_RESP_NOT_IMPLEMENTED			0x51 /* Not Implemented */
#define OBEX_RESP_BAD_GATEWAY				0x52 /* Bad Gateway */
#define OBEX_RESP_SERVICE_UNAVAILABLE	0x53 /* Service Unavailable */
#define OBEX_RESP_GATEWAY_TIMEOUT			0x54 /* Gateway Timeout */
#define OBEX_RESP_HTTP_VER_NO_SUPPORT	0x55 /* HTTP version not supported */

#define OBEX_RESP_DATABASE_FULL			0x60 /* Database Full */
#define OBEX_RESP_DATABASE_LOCKED			0x61 /* Database Locked */

typedef uint8 obex_header;

#define OBEX_HEADER_COUNT			0xC0 /* (4-byte) Number of objects */
#define OBEX_HEADER_NAME			0x01 /* (Unicode) Object name */
#define OBEX_HEADER_TYPE			0x42 /* (ByteSeq) MIME type of object */
#define OBEX_HEADER_LENGTH		0xC3 /* (4-byte) Object length */
#define OBEX_HEADER_TIME_ISO		0x44 /* (ByteSeq, ISO 8601 format) Creation or modification time for object (preferred format). */
#define OBEX_HEADER_TIME_COMPAT	0xC4 /* (4-byte) Creation or modification time for object for backward-compatibility. */
#define OBEX_HEADER_DESCRIPTION	0x05 /* (Unicode) Text description of object */
#define OBEX_HEADER_TARGET		0x46 /* (ByteSeq) Target ID for operation */
#define OBEX_HEADER_HTTP			0x47 /* (ByteSeq) An HTTP 1.x header (URL for object) */
#define OBEX_HEADER_BODY			0x48 /* Not for use by OBEX applications */
#define OBEX_HEADER_END_BODY		0x49 /* Not for use by OBEX applications */
#define OBEX_HEADER_WHO			0x4A /* (ByteSeq) Who ID identifies service providing the object */
#define OBEX_HEADER_CONNID		0xCB /* (4-byte) Identifies the connection for which the operation is directed */
#define OBEX_HEADER_APP_PARAMS	0x4C /* (ByteSeq) Application parameters */
#define OBEX_HEADER_AUTH_CHAL		0x4D /* (ByteSeq) Authentication challenge */
#define OBEX_HEADER_AUTH_RESP		0x4E /* (ByteSeq) Authentication response */
#define OBEX_HEADER_OBJECT_CLASS	0x4F /* (ByteSeq)  OBEX Object class of object */

#define MAX_OBEX_TX_BUF_LEN	128		//need to be confirm
#define MAX_OBEX_PACKET_SIZE	0x0200	//need to be confirm, 512

#define OBEX_CFG_SERVER_CHANNEL	0x14	//need to be confirm

int8 obex_init(void (*indicate_callback)(enum obex_event_t event));
int8 obex_close(void);
void obex_rfcomm_notifyCallback(enum rfcomm_event_enum event, uint32 rfcomm_handle, void *pData, uint8 reason, void *priv);
void obex_rfcomm_dataRecvCallback(uint32 rfcomm_handle, struct pp_buff *ppb, void *priv);
//int8 obex_sendData(struct obex_client *client);

#endif /* __OBEX_H__ */