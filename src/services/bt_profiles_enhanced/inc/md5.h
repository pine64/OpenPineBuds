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

#ifndef __MD5_H__
#define __MD5_H__

#include "obex_i.h"
#include "btlib_type.h"

#if OBEX_AUTHENTICATION == 1

typedef struct _xMD5Context {
    uint32     buf[4];
    uint32     bytes[2];
    uint32     in[16];
} xMD5Context;

void MD5(void *dest, void *orig, uint16 len);
void xMD5Init(xMD5Context * ctx);
void xMD5Update(xMD5Context * ctx, const uint8 * buf, uint16 len);
void xMD5Final(uint8 digest [ AUTH_NONCE_LEN ], xMD5Context * ctx);

#endif

#endif /* __MD5_H__ */