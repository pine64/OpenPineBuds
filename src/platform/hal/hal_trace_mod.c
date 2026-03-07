/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "hal_trace_mod.h"
#include "plat_types.h"

#undef _LOG_MODULE_DEF_A
#define _LOG_MODULE_DEF_A(p, m) #m

static const char *mod_desc[] = {_LOG_MODULE_LIST};

const char *hal_trace_get_log_module_desc(enum LOG_MODULE_T module) {
  if (module < ARRAY_SIZE(mod_desc)) {
    return mod_desc[module];
  }
  return NULL;
}
