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
#include "app_overlay.h"
#include "cmsis_os.h"
#include "hal_location.h"
#include "hal_trace.h"

osMutexDef(app_overlay_mutex);

static osMutexId app_overlay_mutex_id = NULL;
static APP_OVERLAY_ID_T app_overlay_id = APP_OVERLAY_ID_QTY;

APP_OVERLAY_ID_T app_get_current_overlay(void) { return app_overlay_id; }

void app_overlay_select(enum APP_OVERLAY_ID_T id) {
  TRACE(3, "%s id:%d:%d", __func__, id, app_overlay_id);

  osMutexWait(app_overlay_mutex_id, osWaitForever);
  if (app_overlay_id == APP_OVERLAY_ID_QTY) {
    app_overlay_load(id);
  } else if (app_overlay_id != APP_OVERLAY_ID_QTY) {
    if (app_overlay_id != id) {
      app_overlay_unload(app_overlay_id);
      app_overlay_load(id);
    }
  }
  app_overlay_id = id;
  osMutexRelease(app_overlay_mutex_id);
}

void app_overlay_unloadall(void) {
  osMutexWait(app_overlay_mutex_id, osWaitForever);
  if (app_overlay_id != APP_OVERLAY_ID_QTY) {
    app_overlay_unload(app_overlay_id);
  }
  app_overlay_id = APP_OVERLAY_ID_QTY;
  osMutexRelease(app_overlay_mutex_id);
}

void app_overlay_open(void) {
  if (app_overlay_mutex_id == NULL) {
    app_overlay_mutex_id = osMutexCreate(osMutex(app_overlay_mutex));
  }
}

void app_overlay_close(void) {
  app_overlay_unloadall();
  if (app_overlay_mutex_id != NULL) {
    osMutexDelete(app_overlay_mutex_id);
    app_overlay_mutex_id = NULL;
  }
}
