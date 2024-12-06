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

#ifndef __PBAP_I_H__
#define __PBAP_I_H__

#include "obex_i.h"

#define PULL_PHONEBOOK_TYPE		"x-bt/phonebook"
#define PULL_LISTING_TYPE			"x-bt/vcard-listing"
#define PULL_VCARD_ENTRY_TYPE		"x-bt/vcard"

typedef uint8 vcard_sort_type;
#define VCARD_SORT_ORDER_INDEXED		0x00       /* Indexed sorting */
#define VCARD_SORT_ORDER_ALPHA		0x01       /* Alphabetical sorting */
#define VCARD_SORT_ORDER_PHONETICAL	0x02       /* Phonetical sorting */

typedef uint8 vcard_search_attribute;
#define VCARD_SEARCH_ATTRIB_NAME		0x00        /* Search by Name */
#define VCARD_SEARCH_ATTRIB_NUMBER	0x01        /* Search by Number */
#define VCARD_SEARCH_ATTRIB_SOUND		0x02        /* Search by Sound */

struct vcard_filter{
	uint32 low;
	uint32 high;
};

#define VCARD_FILTER_VERSION		0    /* Version (Bit 0) */
#define VCARD_FILTER_FN			1    /* Formatted Name (Bit 1) */
#define VCARD_FILTER_N				2    /* Structured Presentation of Name (Bit 2) */
#define VCARD_FILTER_PHOTO		3    /* Associated Image or Photo (Bit 3) */
#define VCARD_FILTER_BDAY			4    /* Birthday (Bit 4) */
#define VCARD_FILTER_ADR			5    /* Delivery Address (Bit 5) */
#define VCARD_FILTER_LABEL		6    /* Delivery (Bit 6) */
#define VCARD_FILTER_TEL			7    /* Telephone (Bit 7) */
#define VCARD_FILTER_EMAIL		8    /* Electronic Mail Address (Bit 8) */
#define VCARD_FILTER_MAILER		9    /* Electronic Mail (Bit 9) */
#define VCARD_FILTER_TZ			10   /* Time Zone (Bit 10) */
#define VCARD_FILTER_GEO			11   /* Geographic Position (Bit 11) */
#define VCARD_FILTER_TITLE		12   /* Job (Bit 12) */
#define VCARD_FILTER_ROLE			13   /* Role within the Organization (Bit 13) */
#define VCARD_FILTER_LOGO			14   /* Organization Logo (Bit 14) */
#define VCARD_FILTER_AGENT		15   /* vCard of Person Representing (Bit 15) */
#define VCARD_FILTER_ORG			16   /* Name of Organization (Bit 16) */
#define VCARD_FILTER_NOTE			17   /* Comments (Bit 17) */
#define VCARD_FILTER_REV			18   /* Revision (Bit 18) */
#define VCARD_FILTER_SOUND		19   /* Pronunciation of Name (Bit 19) */
#define VCARD_FILTER_URL			20   /* Uniform Resource Locator (Bit 20) */
#define VCARD_FILTER_UID			21   /* Unique ID (Bit 21) */
#define VCARD_FILTER_KEY			22   /* Public Encryption Key (Bit 22) */
#define VCARD_FILTER_NICKNAME		23   /* Nickname (Bit 23) */
#define VCARD_FILTER_CATEGORIES	24   /* Categories (Bit 24) */
#define VCARD_FILTER_PRODID		25   /* Product Id (Bit 25) */
#define VCARD_FILTER_CLASS		26   /* Class Information (Bit 26) */
#define VCARD_FILTER_SORT_STRING	27   /* Sort string (Bit 27) */
#define VCARD_FILTER_TIMESTAMP	28   /* Time stamp (Bit 28) */
/* Bits 29-38 Reserved for future use */
#define VCARD_FILTER_PROPRIETARY	39   /* Use of a proprietary filter (Bit 39) */ 
/* Bits 40-63 Reserved for proprietary filter usage */

typedef uint8 vcard_format;
#define VCARD_FORMAT_21        0x00       /* Version 2.1 format */
#define VCARD_FORMAT_30        0x01       /* Version 3.0 format */

typedef uint8 app_parameter_tag;
#define PBAP_TAG_ORDER				0x01  /* 1-byte, 0x00 (indexed), 0x01 (alpha), or 0x02 (phonetic) */
#define PBAP_TAG_SEARCH_VALUE		0x02  /* Variable length text string */
#define PBAP_TAG_SEARCH_ATTRIB	0x03  /* 1-byte, 0x00 (Name), 0x01 (Number), or 0x02 (Sound) */
#define PBAP_TAG_MAX_LIST_COUNT	0x04  /* 2-bytes, 0x0000 to 0xFFFF */
#define PBAP_TAG_LIST_OFFSET		0x05  /* 2-bytes, 0x0000 to 0xFFFF */
#define PBAP_TAG_FILTER			0x06  /* 8-bytes, 64 bit mask */
#define PBAP_TAG_FORMAT			0x07  /* 1-byte, 0x00 = 2.1, 0x01 = 3.0 */
#define PBAP_TAG_PHONEBOOK_SIZE	0x08  /* 2-bytes, 0x0000 to 0xFFFF */
#define PBAP_TAG_MISSED_CALLS		0x09  /* 1-byte, 0x00 to 0xFF */

enum pbap_state{
	PBAP_TP_CONNECTING,
	PBAP_TP_DISCONNECTING,
	PBAP_IDLE,				//transport connection idle, connection idle
	PBAP_DISCONNECTING,	//transport connection established, connection discarding
	PBAP_TP_CONNECTED,		//transport connection established, connection idle
	PBAP_CONNECTING,		//transport connection established, connection establishing
	PBAP_CONNECTED,		//transport connection established, connection established
	PBAP_GET,
	PBAP_ABORT,
	PBAP_SET_PATH
};

enum pbap_event{
	PBAP_EVENT_IDLE,
	PBAP_EVENT_TP_CONNECTED,
	PBAP_EVENT_CONNECTED,
	PBAP_EVENT_CONTINUE,
	PBAP_EVENT_COMPLETE,
#if OBEX_AUTHENTICATION == 1
	PBAP_EVENT_AUTH,
#endif
	PBAP_EVENT_NO_CONNECTION,
	PBAP_EVENT_FAILED,
	PBAP_EVENT_TIMEOUT
};

typedef uint8 pbap_opcode;
#define PBAP_OP_NULL			0x00
#define PBAP_OP_CONNECT		0x01
#define PBAP_PULL_PHONEBOOK	0x02
#define PBAP_PULL_VCARDLIST	0x03
#define PBAP_PULL_VCARDENTRY	0x04
#define PBAP_OP_SET_PATH		0x05
#define PBAP_OP_ABORT			0x06
#define PBAP_OP_DISCONNECT	0x07

struct pbap_pull_phonebook_parms{
	char name[30];
	struct vcard_filter filter;
	uint8 format;
	uint16 maxListCount;
	uint16 listStartOffset;
};

struct pbap_pull_list_parms{
	char folder[10];
	uint8 order;
	char searchValue[20];
	uint8 searchAttribute;
	uint16 maxListCount;
	uint16 listStartOffset;
};

struct pbap_pull_entry_parms{
	char name[10];
	struct vcard_filter filter;
};

struct pbap_set_path_parms{
	char path[10];
};

struct pbap_client{
	pbap_opcode opcode;
	void (*pbap_client_callback)(enum pbap_event event);

	/*struct pbap_pull_phonebook_parms *pb_parms;
	struct pbap_pull_list_parms *list_parms;
	struct pbap_pull_entry_parms *entry_parms;
	struct pbap_set_path_parms *setPath_parms;*/
	void *parms;

	uint8 *recData;
	uint16 recLen;
};

#if OBEX_AUTHENTICATION == 1
#include "md5.h"
#endif

struct pbap_control_t{
	enum pbap_state state;
	
	uint8 connectID[4];
#if OBEX_AUTHENTICATION == 1
	uint8 challenge[AUTH_NONCE_LEN];
	uint8 response[AUTH_NONCE_LEN];
	uint8 challenge_length;
#endif
	struct pbap_client *pbap_client_app;
};

#define PBAP_SEM_TIMEOUT	100

int8 pbap_connectReq(void);
int8 pbap_disconnectReq(void);
//void pbap_pull_phonebook(struct pbap_pull_phonebook_parms *parms);
//void pbap_set_path(char *pathName);
//void pbap_pull_vCardListing(struct pbap_pull_list_parms *parms);
//void pbap_pull_vCardEntry(struct pbap_pull_entry_parms *parms);
int8 pbap_pull_phonebook(void);
int8 pbap_set_path(void);
int8 pbap_pull_vCardListing(void);
int8 pbap_pull_vCardEntry(void);
int8 pbap_abort(void);

void pbap_client_init(struct pbap_client *client);
void pbap_client_exit(void);
void vcard_filter_set_bit(struct vcard_filter *filter, uint8 bit);
void vcard_filter_clear_bit(struct vcard_filter *filter, uint8 bit);
enum pbap_state pbap_getState(void);
int8 pbap_get_continue(void);

#endif /* __PBAP_I_H__ */