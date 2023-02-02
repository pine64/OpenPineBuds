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
#include "hal_sysfreq.h"
#include "cmsis.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "plat_types.h"
#ifndef ROM_BUILD
#include "pmu.h"
#endif

static uint32_t BOOT_BSS_LOC sysfreq_bundle[(HAL_SYSFREQ_USER_QTY + 3) / 4];

static uint8_t *const sysfreq_per_user = (uint8_t *)&sysfreq_bundle[0];

static enum HAL_SYSFREQ_USER_T BOOT_DATA_LOC top_user = HAL_SYSFREQ_USER_QTY;

static enum HAL_CMU_FREQ_T BOOT_DATA_LOC min_sysfreq = HAL_CMU_FREQ_26M;

static enum HAL_CMU_FREQ_T hal_sysfreq_revise_freq(enum HAL_CMU_FREQ_T freq) {
  return freq > min_sysfreq ? freq : min_sysfreq;
}

void hal_sysfreq_set_min_freq(enum HAL_CMU_FREQ_T freq) {
  uint32_t lock;

  lock = int_lock();

  if (min_sysfreq < freq) {
    min_sysfreq = freq;
    if (min_sysfreq > hal_sysfreq_get()) {
      hal_cmu_sys_set_freq(min_sysfreq);
    }
  }

  int_unlock(lock);
}

int hal_sysfreq_req(enum HAL_SYSFREQ_USER_T user, enum HAL_CMU_FREQ_T freq) {
  uint32_t lock;
  enum HAL_CMU_FREQ_T cur_sys_freq;
  int i;

  if (user >= HAL_SYSFREQ_USER_QTY) {
    return 1;
  }
  if (freq >= HAL_CMU_FREQ_QTY) {
    return 2;
  }

  lock = int_lock();

  cur_sys_freq = hal_sysfreq_get();

  sysfreq_per_user[user] = freq;

  if (freq == cur_sys_freq) {
    top_user = user;
  } else if (freq > cur_sys_freq) {
    top_user = user;
    freq = hal_sysfreq_revise_freq(freq);
#ifndef ROM_BUILD
    pmu_sys_freq_config(freq);
#ifdef ULTRA_LOW_POWER
    // Enable PLL if required
    hal_cmu_low_freq_mode_disable(hal_sysfreq_revise_freq(cur_sys_freq), freq);
#endif
#endif
    hal_cmu_sys_set_freq(freq);
  } else /* if (freq < cur_sys_freq) */ {
    if (top_user == user || top_user == HAL_SYSFREQ_USER_QTY) {
      if (top_user == user) {
        freq = sysfreq_per_user[0];
        user = 0;
        for (i = 1; i < HAL_SYSFREQ_USER_QTY; i++) {
          if (freq < sysfreq_per_user[i]) {
            freq = sysfreq_per_user[i];
            user = i;
          }
        }
      }
      top_user = user;
      if (freq != cur_sys_freq) {
        freq = hal_sysfreq_revise_freq(freq);
        hal_cmu_sys_set_freq(freq);
#ifndef ROM_BUILD
#ifdef ULTRA_LOW_POWER
        // Disable PLL if capable
        hal_cmu_low_freq_mode_enable(hal_sysfreq_revise_freq(cur_sys_freq),
                                     freq);
#endif
        pmu_sys_freq_config(freq);
#endif
      }
    }
  }

  int_unlock(lock);

  return 0;
}

enum HAL_CMU_FREQ_T hal_sysfreq_get(void) {
  if (top_user < HAL_SYSFREQ_USER_QTY) {
    return sysfreq_per_user[top_user];
  } else {
    return hal_cmu_sys_get_freq();
  }
}

enum HAL_CMU_FREQ_T hal_sysfreq_get_hw_freq(void) {
  if (top_user < HAL_SYSFREQ_USER_QTY) {
    return hal_sysfreq_revise_freq(sysfreq_per_user[top_user]);
  } else {
    return hal_cmu_sys_get_freq();
  }
}

int hal_sysfreq_busy(void) {
  int i;

  for (i = 0; i < ARRAY_SIZE(sysfreq_bundle); i++) {
    if (sysfreq_bundle[i] != 0) {
      return 1;
    }
  }

  return 0;
}

void hal_sysfreq_print(void) {
  int i;

  for (i = 0; i < HAL_SYSFREQ_USER_QTY; i++) {
    if (sysfreq_per_user[i] != 0) {
      TRACE(2, "*** SYSFREQ user=%d freq=%d", i, sysfreq_per_user[i]);
    }
  }
  TRACE(1, "*** SYSFREQ top_user=%d", top_user);
}
