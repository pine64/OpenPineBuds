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

#ifndef OBEX_TRANSMISSION_H_INCLUDED
#define OBEX_TRANSMISSION_H_INCLUDED

#include "bt_common.h"
#include "bt_co_list.h"
#include "obex_protocol.h"

/*------ What Is A Transmission ------*/
/* An user (application) fires a operation (CONNECT/DISCONNECT/PUT/GET/SETPATH/ABORT) to server to do something */
/* The operation is in fact a packet of data which is called Transmission here */
/* A Transmission is composed of fixed 'Field' plus some optional 'HEADER' */
/* For the fixed 'Field', here using obex_transmisson_prepare to reserve the buffer in Transmission */
/* For the optional 'Header', here using obex_transmisson_add_XXX to add it in Transmission */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16 len;
    uint8 *text;
    uint8 valid;
} obex_transmission_header_Unicode_t;

typedef struct {
    uint16 len;
    uint8 *buff;
    uint8 valid;
} obex_transmission_header_ByteSequence_t;

typedef struct {
    uint8 value;
    uint8 valid;
} obex_transmission_header_1ByteQuality;

typedef struct {
    uint32 value;
    uint8 valid;
} obex_transmission_header_4ByteQuality;

typedef struct {
    obex_transmission_header_4ByteQuality Count;
    obex_transmission_header_Unicode_t Name;
    obex_transmission_header_ByteSequence_t Type;
    obex_transmission_header_4ByteQuality Length;
    obex_transmission_header_ByteSequence_t Time_ISO8601;
    obex_transmission_header_4ByteQuality Time_4Bytes;
    obex_transmission_header_Unicode_t Description;
    obex_transmission_header_ByteSequence_t Target;
    obex_transmission_header_ByteSequence_t HTTP;
    obex_transmission_header_ByteSequence_t Body;
    obex_transmission_header_ByteSequence_t EndOfBody;
    obex_transmission_header_ByteSequence_t Who;
    obex_transmission_header_4ByteQuality ConnectionID;
    obex_transmission_header_ByteSequence_t AppParameters;
    obex_transmission_header_ByteSequence_t AuthChallenge;
    obex_transmission_header_ByteSequence_t AuthResponse;
    obex_transmission_header_ByteSequence_t ObjectClass;
} obex_transmission_headers_t;

typedef struct {
    uint8 *buff;
    uint16 packet_len;
    uint16 max_buff_len;
    bool is_final;
    uint8 flag;
} obex_transmission_t;

//------ Initialize a transmission in a give buffer ------//
int32 obex_transmission_init(obex_transmission_t *trasmission, uint8 *buff, uint32 len);
int32 obex_transmission_prepare(obex_transmission_t *trasmission, obex_operation_opcode_t operation_code);

int32 obex_transmission_parse_headers(uint8 *buff, uint32 len, obex_transmission_headers_t *headers);

bool obex_is_final_transmission(obex_transmission_t *transmission);
void obex_transmission_set_final(obex_transmission_t *transmission, bool final);
uint8 obex_transmission_get_flag(obex_transmission_t *transmission);
void obex_transmission_set_flag(obex_transmission_t *transmission, uint8 flag);

#define obex_transmission_add_Count(tr,Count) \
    obex_transmission_add_4ByteQuantity(tr,obex_header_id_Count,Count)

#define obex_transmission_add_Name(tr,name,name_len_without_null) \
    obex_transmission_add_Unicode(tr,obex_header_id_Name,name,name_len_without_null)

#define obex_transmission_add_Type(tr,type,type_len_with_null) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_Type,type,type_len_with_null)

#define obex_transmission_add_Length(tr,Length) \
    obex_transmission_add_4ByteQuantity(tr,obex_header_id_Length,Length)

#define obex_transmission_add_Time_ISO8601t(tr,time,time_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_Time_ISO8601,time,time_len)

#define obex_transmission_add_Time_4Byte(tr,Time) \
    obex_transmission_add_4ByteQuantity(tr,obex_header_id_Time_4byte,Time)

#define obex_transmission_add_Description(tr,desc,desc_len_without_null) \
    obex_transmission_add_Unicode(tr,obex_header_id_Description,desc,desc_len_without_null)

#define obex_transmission_add_Target(tr,uuid,uuid_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_Target,uuid,uuid_len)

#define obex_transmission_add_HTTP(tr,http,http_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_HTTP,http,http_len)

#define obex_transmission_add_Body(tr,body,body_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_Body,body,body_len)

#define obex_transmission_add_EndOfBody(tr,body,body_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_EndofBody,body,body_len)

#define obex_transmission_add_Who(tr,who,who_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_Who,who,who_len)

#define obex_transmission_add_ConnectionID(tr,ConnectionID) \
    obex_transmission_add_4ByteQuantity(tr,obex_header_id_ConnectionID,ConnectionID)

#define obex_transmission_add_ApplicationParameters(tr,tlv,tlv_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_ApplicationParameters,tlv,tlv_len)

#define obex_transmission_add_AuthenticateChallenge(tr,ac,ac_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_AuthenticateChallenge,ac,ac_len)

#define obex_transmission_add_AuthenticateResponse(tr,as,as_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_AuthenticateResponse,as,as_len)

#define obex_transmission_add_ObjectClass(tr,oc,oc_len) \
    obex_transmission_add_ByteSequence(tr,obex_header_id_ObjectClass,oc,oc_len)

//------ null terminated Unicode text, length prefixed with 2 byte unsigned integer ------//
int32 obex_transmission_add_Unicode(obex_transmission_t *tr, obex_header_id_t id, uint8 *ascii, uint32 ascii_len_without_null);
//------ byte sequence, length prefixed with 2 byte unsigned integer ------//
int32 obex_transmission_add_ByteSequence(obex_transmission_t *tr, obex_header_id_t id, uint8 *bytes, uint32 bytes_len);
//------ 1 byte quantity ------//
int32 obex_transmission_add_1ByteQuantity(obex_transmission_t *tr, obex_header_id_t id, uint8 value);
//------ 4 byte quantity ------//
int32 obex_transmission_add_4ByteQuantity(obex_transmission_t *tr, obex_header_id_t id, uint32 value);


#ifdef __cplusplus
}
#endif

#endif // OBEX_TRANSMISSION_H_INCLUDED
