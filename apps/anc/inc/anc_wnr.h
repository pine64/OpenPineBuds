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
#ifndef __ANC_WNR_H__
#define __ANC_WNR_H__

// #include "plat_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ANC_WNR_OPEN_MODE_STANDALONE = 0,
  ANC_WNR_OPEN_MODE_CONFIGURE,

  ANC_WNR_OPEN_MODE_QTY
} anc_wnr_open_mode_t;

void anc_release_gain(void);

typedef enum {
  APP_WNR_NOTIFY_DETECT_RESULT,
  APP_WNR_REQUEST_DETECT_RESULT,
  APP_WNR_RESPONSE_DETECT_RESULT,
  APP_WNR_PROCESS_DETECT_RESULT,
  APP_WNR_SET_TRIGGER,
  APP_WNR_EXCUTE_TRIGGER,
  APP_WNR_SHARE_MODULE_INFO,
} anc_wnr_sync_ctrl_internal_event_e;

int32_t anc_wnr_ctrl(int32_t sample_rate, int32_t frame_len);
int32_t anc_wnr_open(anc_wnr_open_mode_t mode);
int32_t anc_wnr_close(void);
int32_t anc_wnr_process(void *pcm_buf, uint32_t pcm_len);
void app_wnr_sync_state(void);
void app_wnr_cmd_receive_process(uint8_t *p_buff, uint16_t length);

#ifdef __cplusplus
}
#endif
#endif
