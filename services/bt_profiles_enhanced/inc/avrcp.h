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


#ifndef __AVRCP_H__
#define __AVRCP_H__

#include "btlib_type.h"
#include "avctp_i.h"

#define AVRCP_BT_COMPANY_ID "\x00\x19\x58"
 
 /* Define Opcode */
#define AVRCP_OPCODE_UNIT_INFO       0x30
#define AVRCP_OPCODE_SUBUNIT_INFO    0x31
#define AVRCP_OPCODE_PASS_THROUGH    0x7C
#define AVRCP_OPCODE_VENDOR_DEP      0x00

/* Define Command Type*/
#define AVRCP_CTYPE_CONTROL   0x00
#define AVRCP_CTYPE_STATUS    0x01
#define AVRCP_CTYPE_NOTIFY    0x03
#define AVCTP_CTYPE_GENERAL_INQUIRY       0x04

/* Response */
#define AVRCP_RESPONSE_STABLE    0x0C

/* Define PASSTHROUGH OP_ID */
#define AVRCP_OP_ID_PLAY    0x44
#define AVRCP_OP_ID_STOP    0x45
#define AVRCP_OP_ID_PAUSE   0x46
#define AVRCP_OP_ID_REC     0x47
#define AVRCP_OP_ID_FB      0x48
#define AVRCP_OP_ID_FF      0x49
#define AVRCP_OP_ID_FW      0x4B
#define AVRCP_OP_ID_BW      0x4C

#define AVRCP_OP_NEXT_GROUP  0x017E
#define AVRCP_OP_PREV_GROUP  0x027E

#define AVRCP_BTN_PUSHED	0x00
#define AVRCP_BTN_RELEASED	0x80
int avrcp_notify_callback(struct avctp_control_t *avctp_ctl, uint8 event, uint32 handle, void *pdata);
void avrcp_datarecv_callback(struct avctp_control_t *avctp_ctl, uint32 handle, struct pp_buff *ppb);

#endif /* __AVRCP_H__ */