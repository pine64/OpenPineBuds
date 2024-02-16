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
#include "app_status_ind.h"
#include "app_pwl.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "stdbool.h"
#include "string.h"

static APP_STATUS_INDICATION_T app_status = APP_STATUS_INDICATION_NUM;
static APP_STATUS_INDICATION_T app_status_ind_filter =
    APP_STATUS_INDICATION_NUM;

const char *app_status_indication_str[] = {
    "[POWERON]",
    "[INITIAL]",
    "[PAGESCAN]",
    "[POWEROFF]",
    "[CHARGENEED]",
    "[CHARGING]",
    "[FULLCHARGE]",
    /* repeatable status: */
    "[BOTHSCAN]",
    "[CONNECTING]",
    "[CONNECTED]",
    "[DISCONNECTED]",
    "[CALLNUMBER]",
    "[INCOMINGCALL]",
    "[PAIRSUCCEED]",
    "[PAIRFAIL]",
    "[HANGUPCALL]",
    "[REFUSECALL]",
    "[ANSWERCALL]",
    "[CLEARSUCCEED]",
    "[CLEARFAIL]",
    "[WARNING]",
    "[ALEXA_START]",
    "[ALEXA_STOP]",
    "[GSOUND_MIC_OPEN]",
    "[GSOUND_MIC_CLOSE]",
    "[GSOUND_NC]",
    "[INVALID]",
    "[MUTE]",
    "[TESTMODE]",
    "[TESTMODE1]",
    "[RING_WARNING]",
#ifdef __INTERACTION__
    "[FINDME]",
#endif
    "[MY_BUDS_FIND]",
    "[TILE_FIND]",
};

const char *status2str(uint16_t status) {
  const char *str = NULL;

  if (status >= 0 && status < APP_STATUS_INDICATION_NUM) {
    str = app_status_indication_str[status];
  } else {
    str = "[UNKNOWN]";
  }

  return str;
}

int app_status_indication_filter_set(APP_STATUS_INDICATION_T status) {
  app_status_ind_filter = status;
  return 0;
}

APP_STATUS_INDICATION_T app_status_indication_get(void) { return app_status; }

int app_status_indication_set(APP_STATUS_INDICATION_T status) {
  struct APP_PWL_CFG_T cfg0;
  struct APP_PWL_CFG_T cfg1;

  if (app_status == status)
    return 0;

  if (app_status_ind_filter == status)
    return 0;

  TRACE(2, "%s %d", __func__, status);

  app_status = status;
  memset(&cfg0, 0, sizeof(struct APP_PWL_CFG_T));
  memset(&cfg1, 0, sizeof(struct APP_PWL_CFG_T));
  app_pwl_stop(APP_PWL_ID_0);
  app_pwl_stop(APP_PWL_ID_1);
  switch (status) {
  case APP_STATUS_INDICATION_POWERON:
    cfg0.part[0].level = 1;
    cfg0.part[0].time = (3000);
    cfg0.part[1].level = 0;
    cfg0.part[1].time = (200);
    cfg0.parttotal = 2;
    cfg0.startlevel = 1;
    cfg0.periodic = false;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);

    break;
  case APP_STATUS_INDICATION_INITIAL:
    break;
  case APP_STATUS_INDICATION_PAGESCAN:
    cfg0.part[0].level = 1;
    cfg0.part[0].time = (300);
    cfg0.part[1].level = 0;
    cfg0.part[1].time = (8000);
    cfg0.parttotal = 2;
    cfg0.startlevel = 1;
    cfg0.periodic = true;
    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    break;
  case APP_STATUS_INDICATION_BOTHSCAN:
    cfg0.part[0].level = 0;
    cfg0.part[0].time = (300);
    cfg0.part[1].level = 1;
    cfg0.part[1].time = (300);
    cfg0.parttotal = 2;
    cfg0.startlevel = 0;
    cfg0.periodic = true;

    cfg1.part[0].level = 1;
    cfg1.part[0].time = (300);
    cfg1.part[1].level = 0;
    cfg1.part[1].time = (300);
    cfg1.parttotal = 2;
    cfg1.startlevel = 1;
    cfg1.periodic = true;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    break;
  case APP_STATUS_INDICATION_CONNECTING:
    // LED's alternating Red/Blue
    cfg0.part[0].level = 1;
    cfg0.part[0].time = (300);
    cfg0.part[1].level = 0;
    cfg0.part[1].time = (300);
    cfg0.parttotal = 2;
    cfg0.startlevel = 0;
    cfg0.periodic = true;

    cfg1.part[0].level = 1;
    cfg1.part[0].time = (300);
    cfg1.part[1].level = 0;
    cfg1.part[1].time = (300);
    cfg1.parttotal = 2;
    cfg1.startlevel = 1;
    cfg1.periodic = true;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    break;
  case APP_STATUS_INDICATION_CONNECTED:
#ifdef DISABLE_CONNECTED_BLUE_LIGHT
    cfg0.part[0].level = 0;
    cfg0.part[0].time = (500);
    cfg0.parttotal = 1;
    cfg0.startlevel = 0;
    cfg0.periodic = false;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg0);
    app_pwl_start(APP_PWL_ID_1);
#else
    cfg0.part[0].level = 1;
    cfg0.part[0].time = (500);
    cfg0.part[1].level = 0;
    cfg0.part[1].time = (3000);
    cfg0.parttotal = 2;
    cfg0.startlevel = 1;
    cfg0.periodic = true;

    cfg1.part[0].level = 0;
    cfg1.part[0].time = (500);
    cfg1.part[1].level = 0;
    cfg1.part[1].time = (3000);
    cfg1.parttotal = 2;
    cfg1.startlevel = 0;
    cfg1.periodic = true;
    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
#endif
    break;
  case APP_STATUS_INDICATION_CHARGING:
    cfg1.part[0].level = 1;
    cfg1.part[0].time = (5000);
    cfg1.parttotal = 1;
    cfg1.startlevel = 1;
    cfg1.periodic = true;
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    break;
  case APP_STATUS_INDICATION_FULLCHARGE:
    cfg0.part[0].level = 0;
    cfg0.part[0].time = (5000);
    cfg0.parttotal = 1;
    cfg0.startlevel = 1;
    cfg0.periodic = false;
    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg0);
    app_pwl_start(APP_PWL_ID_1);
    break;
  case APP_STATUS_INDICATION_POWEROFF:
    cfg1.part[0].level = 0;
    cfg1.part[0].time = (100);
    cfg1.parttotal = 1;
    cfg1.startlevel = 1;
    cfg1.periodic = false;
    cfg0.part[0].level = 0;
    cfg0.part[0].time = (100);
    cfg0.parttotal = 1;
    cfg0.startlevel = 1;
    cfg0.periodic = false;

    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    break;
  case APP_STATUS_INDICATION_CHARGENEED:
    cfg1.part[0].level = 1;
    cfg1.part[0].time = (500);
    cfg1.part[1].level = 0;
    cfg1.part[1].time = (2000);
    cfg1.parttotal = 2;
    cfg1.startlevel = 1;
    cfg1.periodic = true;
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    break;
  case APP_STATUS_INDICATION_TESTMODE:
    cfg0.part[0].level = 0;
    cfg0.part[0].time = (300);
    cfg0.part[1].level = 1;
    cfg0.part[1].time = (300);
    cfg0.parttotal = 2;
    cfg0.startlevel = 0;
    cfg0.periodic = true;

    cfg1.part[0].level = 0;
    cfg1.part[0].time = (300);
    cfg1.part[1].level = 1;
    cfg1.part[1].time = (300);
    cfg1.parttotal = 2;
    cfg1.startlevel = 1;
    cfg1.periodic = true;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    break;
  case APP_STATUS_INDICATION_TESTMODE1:
    cfg0.part[0].level = 0;
    cfg0.part[0].time = (1000);
    cfg0.part[1].level = 1;
    cfg0.part[1].time = (1000);
    cfg0.parttotal = 2;
    cfg0.startlevel = 0;
    cfg0.periodic = true;

    cfg1.part[0].level = 0;
    cfg1.part[0].time = (1000);
    cfg1.part[1].level = 1;
    cfg1.part[1].time = (1000);
    cfg1.parttotal = 2;
    cfg1.startlevel = 1;
    cfg1.periodic = true;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    break;
  default:
    break;
  }
  return 0;
}
