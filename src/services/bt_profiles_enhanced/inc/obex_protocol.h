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
#ifndef OBEX_PROTOCOL_H_INCLUDED
#define OBEX_PROTOCOL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define OBEX_HOST16_TO_BE(value,ptr) \
    *(ptr+0) = ((value)>>8)&0xFF; \
    *(ptr+1) = ((value))&0xFF

#define OBEX_HOST16_TO_LE(value,ptr) \
    *(ptr+0) = ((value))&0xFF; \
    *(ptr+1) = ((value)>>8)&0xFF

#define OBEX_BE_TO_HOST16(ptr) \
    (*(ptr+0))<<8|(*(ptr+1))

#define OBEX_HOST32_TO_BE(value,ptr) \
    *(ptr+0) = ((value)>>24)&0xFF; \
    *(ptr+1) = ((value)>>16)&0xFF; \
    *(ptr+2) = ((value)>>8)&0xFF; \
    *(ptr+3) = ((value))&0xFF \

#define OBEX_HOST32_TO_LE(value,ptr) \
    *(ptr+0) = ((value))&0xFF; \
    *(ptr+1) = ((value)>>8)&0xFF; \
    *(ptr+2) = ((value)>>16)&0xFF; \
    *(ptr+3) = ((value)>>24)&0xFF \

#define OBEX_BE_TO_HOST32(ptr) \
    (*(ptr+0))<<24|(*(ptr+1))<<16|(*(ptr+2))<<8|(*(ptr+3))

//----- Max 128 ASCII for 'Name' ... Unicode String ------//
#define OBEX_UNICODE_STRING_SIZE_MAX (256)

//------ GOEP_SPEC_V1.1 5.2 OBEX HEADERS ------//
typedef unsigned char obex_header_type_t;
#define obex_header_type_UnicodeText          0x00
#define obex_header_type_ByteSequence         0x40
#define obex_header_type_1ByteQuality         0x80
#define obex_header_type_4ByteQuality         0xC0

typedef unsigned char obex_header_id_t;

#define obex_header_id_Count                  0xC0
#define obex_header_id_Name                   0x01
#define obex_header_id_Type                   0x42
#define obex_header_id_Length                 0xC3
#define obex_header_id_Time_ISO8601           0x44
#define obex_header_id_Time_4byte             0xC4
#define obex_header_id_Description            0x05
#define obex_header_id_Target                 0x46
#define obex_header_id_HTTP                   0x47
#define obex_header_id_Body                   0x48
#define obex_header_id_EndofBody              0x49
#define obex_header_id_Who                    0x4A
#define obex_header_id_ConnectionID           0xCB
#define obex_header_id_ApplicationParameters  0x4C
#define obex_header_id_AuthenticateChallenge  0x4D
#define obex_header_id_AuthenticateResponse   0x4E
#define obex_header_id_ObjectClass            0x4F

//------ IrDA Object Exchange Protocol 3.3 OBEX Operations and Opcode definitions ------//
typedef unsigned char obex_operation_opcode_t;

#define obex_opcode_CONNECT         0x80
#define obex_opcode_DISCONNECT      0x81
#define obex_opcode_PUT             0x02
#define obex_opcode_GET             0x03
#define obex_opcode_RESERVED        0x04
#define obex_opcode_SETPATH         0x85
#define obex_opcode_ABORT           0xFF
#define obex_opcode_FINAL           0x80

//------ IrDA Object Exchange Protocol 3.2.1 Response Code values ------//
typedef unsigned char obex_operation_response_code_t;

#define obex_response_code_Continue                      0x10
#define obex_response_code_OK_Success                    0x20
#define obex_response_code_Created                       0x21
#define obex_response_code_Accepted                      0x22
#define obex_response_code_Non_Authoritative_Information 0x23
#define obex_response_code_No_Content                    0x24
#define obex_response_code_Reset_Content                 0x25
#define obex_response_code_Partial_Content               0x26
#define obex_response_code_Multiple_Choices              0x30
#define obex_response_code_Moved_Permanently             0x31
#define obex_response_code_Moved_temporarily             0x32
#define obex_response_code_See_Other                     0x33
#define obex_response_code_Not_modified                  0x34
#define obex_response_code_Use_Proxy                     0x35
#define obex_response_code_Bad_Request                   0x40
#define obex_response_code_Unauthorized                  0x41
#define obex_response_code_Payment_required              0x42
#define obex_response_code_Forbidden                     0x43
#define obex_response_code_Not_Found                     0x44
#define obex_response_code_Method_not_allowed            0x45
#define obex_response_code_Not_Acceptable                0x46
#define obex_response_code_Proxy_Authentication_required 0x47
#define obex_response_code_Request_Time_Out              0x48
#define obex_response_code_Conflict                      0x49
#define obex_response_code_Gone                          0x4A
#define obex_response_code_Length_Required               0x4B
#define obex_response_code_Precondition_failed           0x4C
#define obex_response_code_Requeste_dentity_too_large    0x4D
#define obex_response_code_Request_URL_too_large         0x4E
#define obex_response_code_Unsupported_media_type        0x4F
#define obex_response_code_Internal_Server_Error         0x50
#define obex_response_code_Not_Implemented               0x51
#define obex_response_code_Bad_Gateway                   0x52
#define obex_response_code_Service_Unavailable           0x53
#define obex_response_code_Gateway_Timeout               0x54
#define obex_response_code_HTTP_version_not_supported    0x55
#define obex_response_code_Database_Full                 0x60
#define obex_response_code_Database_Locked               0x61

#ifdef __cplusplus
}
#endif

#endif // OBEX_PROTOCOL_H_INCLUDED
