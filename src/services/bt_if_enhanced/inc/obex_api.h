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

#ifndef __OBEX_API_H__
#define __OBEX_API_H__

#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTIF_OBEX_SESSION_ROLE_CLIENT = 0,
    BTIF_OBEX_SESSION_ROLE_SERVER, // 1
} btif_obex_session_role_t;

#ifdef __cplusplus
}
#endif

#endif /* __OBEX_API_H__ */

