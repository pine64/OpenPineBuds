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
#ifndef MAP_PROTOCOL_H_INCLUDED
#define MAP_PROTOCOL_H_INCLUDED

#include "obex_tlv.h"

#ifdef __cplusplus
extern "C" {
#endif

//------ MAP_SPEC_V10 5.3.2 Flags and Name ------//
typedef unsigned int map_obex_flag_t;
#define map_obex_flag_GoBackToRoot (0x02)
#define map_obex_flag_GoDown1Level (0x02)
#define map_obex_flag_GoUp1Level   (0x03)

//------ MAP_SPEC_V10 6.3.1 Application Parameters Header ------//
#define map_appparam_tlv_ID_Transparent     (0x0B)
#define map_appparam_tlv_ID_Retry           (0x0C)
#define map_appparam_tlv_ID_Charset         (0x14)
#define map_appparam_tlv_VAL_Charset_native (0x00)
#define map_appparam_tlv_VAL_Charset_UTF8   (0x01)

//------ INTERACES : all in one now -------/
#define map_appparam_tlv_add_Transparent(tlv,val) \
    obex_tlv_add_1Byte(tlv, map_appparam_tlv_ID_Transparent, val)

#define map_appparam_tlv_add_Retry(tlv,val) \
    obex_tlv_add_1Byte(tlv, map_appparam_tlv_ID_Retry, val)

#define map_appparam_tlv_add_Charset(tlv,val) \
    obex_tlv_add_1Byte(tlv, map_appparam_tlv_ID_Charset, val)

typedef enum {
    MAP_BMSG_TYPE_EMAIL = 0,
    MAP_BMSG_TYPE_SMS_GSM, // 1
    MAP_BMSG_TYPE_SMS_CDMA, // 2
    MAP_BMSG_TYPE_MMS, // 3
} map_bmessage_type_t;

#ifdef __cplusplus
}
#endif

#endif // MAP_PROTOCOL_H_INCLUDED
