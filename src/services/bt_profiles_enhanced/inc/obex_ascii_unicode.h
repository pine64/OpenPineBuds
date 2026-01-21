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
#ifndef OBEX_ASCII_UNICODE_H_INCLUDED
#define OBEX_ASCII_UNICODE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* IrDA Object Exchange Protocol IrOBEX 2.1 OBEX Headers */
/*
For Unicode text, the length field (immediately following the header ID) includes the 2 bytes of the null
terminator (0x00, 0x00). Therefore the length of the string ¡±Jumar¡± would be 12 bytes; 5 visible
characters plus the null terminator, each two bytes in length.
*/

/* It is not a real UNICODE. It is 2 bytes presentation of ASCII. */

uint32 obex_ascii_to_unicode(uint8 *ascii, uint32 ascii_len_without_null, uint8 *unicode, uint32 unicode_len_with_null);

#ifdef __cplusplus
}
#endif

#endif // OBEX_ASCII_UNICODE_H_INCLUDED
