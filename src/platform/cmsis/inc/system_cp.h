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
#ifndef __SYSTEM_CP_H__
#define __SYSTEM_CP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

void system_cp_reset_handler(void);

void system_cp_init(int load_code);

void system_cp_term(void);

#ifdef __cplusplus
}
#endif

#endif
