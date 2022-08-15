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
#ifndef OBEX_TLV_H_INCLUDED
#define OBEX_TLV_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char obex_tlv_id_t;
typedef unsigned char obex_tlv_length_t;

typedef struct {
    uint8 *buff;
    uint32 buff_max_len;
    uint32 packet_len;
} obex_tlv_t;

int32 obex_tlv_init(obex_tlv_t *tlv, uint8 *buff, uint32 buff_len);
int32 obex_tlv_add_1Byte(obex_tlv_t *tlv, obex_tlv_id_t id, uint8 value);
int32 obex_tlv_add_2Bytes(obex_tlv_t *tlv, obex_tlv_id_t id, uint16 value, bool is_le);
int32 obex_tlv_add_4Bytes(obex_tlv_t *tlv, obex_tlv_id_t id, uint32 value, bool is_le);
int32 obex_tlv_add_buffer(obex_tlv_t *tlv, obex_tlv_id_t id, obex_tlv_length_t lenght, uint8 *value);
uint32 obex_tlv_get_length(obex_tlv_t *tlv);

#ifdef __cplusplus
}
#endif

#endif // OBEX_TLV_H_INCLUDED
