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
#ifndef MAP_BMESSAGE_BUILDER_H_INCLUDED
#define MAP_BMESSAGE_BUILDER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//------ 20191129 MAP_SPEC_V11 ------//
//------ This is simple version : ASCII as UTF8 ------//

//------ MAP_SPEC_V11 3.1.3 Message format (x-bt/message) ------//
//------ VAR - VARIABLE, VAL - VALUE ------//
#define MAP_BMSG_CRLF                               "\r\n"
#define MAP_BMSG_object_BEGIN                       "BEGIN:BMSG\r\n"
#define MAP_BMSG_object_END                         "END:BMSG\r\n"
#define MAP_BMSG_version_VAR                        "VERSION:"
#define MAP_BMSG_readstatus_VAR                     "STATUS:"
#define MAP_BMSG_type_VAR                           "TYPE:"
#define MAP_BMSG_folder_VAR                         "FOLDER:"
#define MAP_BMSG_envelope_BEGIN                     "BEGIN:BENV\r\n"
#define MAP_BMSG_envelope_END                       "END:BENV\r\n"
#define MAP_BMSG_content_BEGIN                      "BEGIN:BBODY\r\n"
#define MAP_BMSG_content_END                        "END:BBODY\r\n"
#define MAP_BMSG_body_part_id_VAR                   "PARTID:"
#define MAP_BMSG_body_encoding_property_VAR         "ENCODING:"
#define MAP_BMSG_body_charset_property_VAR          "CHARSET:"
#define MAP_BMSG_body_language_property_VAR         "LANGUAGE:"
#define MAP_BMSG_body_content_length_property_VAR   "LENGTH:"
#define MAP_BMSG_body_content_BEGIN         "BEGIN:MSG\r\n"
#define MAP_BMSG_body_content_END           "END:MSG\r\n"

#define MAP_BMSG_version_VAL_10                     "1.0\r\n"
#define MAP_BMSG_readstatus_VAL_READ                "READ\r\n"
#define MAP_BMSG_readstatus_VAL_UNREAD              "UNREAD\r\n"
#define MAP_BMSG_type_VAL_EMAIL                     "EMAIL\r\n"
#define MAP_BMSG_type_VAL_SMS_GSM                   "SMS_GSM\r\n"
#define MAP_BMSG_type_VAL_SMS_CDMA                  "SMS_CDMA\r\n"
#define MAP_BMSG_type_VAL_MMS                       "MMS\r\n"
#define MAP_BMSG_body_encoding_VAL_8BIT             "8BIT\r\n"
#define MAP_BMSG_body_encoding_VAL_G7BIT            "G-7BIT\r\n"

#define MAP_BMSG_vcard_BEGIN                        "BEGIN:VCARD\r\n"
#define MAP_BMSG_vcard_END                          "END:VCARD\r\n"
#define MAP_BMSG_vcard_version_VAR                  "VERSION:"
#define MAP_BMSG_vcard_name_VAR                     "N:"
#define MAP_BMSG_vcard_tel_VAR                      "TEL:"
#define MAP_BMSG_vcard_email_VAR                    "EMAIL:"

typedef struct {
    uint8 *buff;
    uint32 buff_len;
    uint32 msg_len;
} map_bmsg_t;

#define BMSG_BEGIN(bmsg) \
    { \
        map_bmsg_t *__bmsg = bmsg;

#define BMSG_ADD(str) \
    map_bmsg_builder_add(__bmsg,(const char *)str,strlen((char *)str))

#define BMSG_ADD_BUFF(buff,buff_len) \
    map_bmsg_builder_add(__bmsg,buff,buff_len)

#define BMSG_END() \
    }

int32 map_bmsg_builder_init(map_bmsg_t *bmsg, uint8 *buff, uint32 buff_len);
int32 map_bmsg_builder_add(map_bmsg_t *bmsg, const char *buff, uint32 buff_len);
uint32 map_bmsg_builder_get_length(map_bmsg_t *bmsg);

#ifdef __cplusplus
}
#endif

#endif // MAP_BMESSAGE_BUILDER_H_INCLUDED
