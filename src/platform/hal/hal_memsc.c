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
#include "hal_memsc.h"
#include "hal_cmu.h"
#include "plat_addr_map.h"

int hal_memsc_lock(enum HAL_MEMSC_ID_T id) {
  if (id >= HAL_MEMSC_ID_QTY) {
    return 0;
  }

  return (hal_cmu_get_memsc_addr())[id];
}

void hal_memsc_unlock(enum HAL_MEMSC_ID_T id) {
  if (id >= HAL_MEMSC_ID_QTY) {
    return;
  }

  (hal_cmu_get_memsc_addr())[id] = 1;
}

bool hal_memsc_avail(enum HAL_MEMSC_ID_T id) {
  if (id >= HAL_MEMSC_ID_QTY) {
    return false;
  }

  return !!((hal_cmu_get_memsc_addr())[4] & (1 << id));
}
