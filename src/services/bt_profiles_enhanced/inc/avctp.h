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

#ifndef __AVCTP_H__
#define __AVCTP_H__

#include "avctp_i.h"
#include "l2cap_i.h"

#ifndef PSM_AVCTP
#endif

#define AVCTP_DEFAULT_MTU     127
#define AVCTP_DEFAULT_CREDITS 7

#define AVCTP_MAX_L2CAP_MTU 1013
#define AVCTP_MAX_CREDITS   40

/* flow control states */
#define AVCTP_CFC_DISABLED 0
#define AVCTP_CFC_ENABLED  AVCTP_DEFAULT_CREDITS

#define AVCTP_CFG_SESSIONS_MAX   2    /* means how many l2cap channel */

struct avctp_resp {
    struct avctp_header header;
    void *avrcp_resp;
} __attribute__ ((packed));

#endif /* __AVCTP_H__ */