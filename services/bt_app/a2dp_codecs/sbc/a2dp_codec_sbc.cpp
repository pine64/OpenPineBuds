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
#include "a2dp_api.h"
#include "analog.h"
#include "app.h"
#include "app_audio.h"
#include "audioflinger.h"
#include "bluetooth.h"
#include "bt_drv.h"
#include "cmsis_os.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "lockcqueue.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include <stdio.h>
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif

#include "a2dp_api.h"
#include "avrcp_api.h"
#include "besbt.h"

#include "app_bt.h"
#include "app_bt_media_manager.h"
#include "apps.h"
#include "bt_drv_interface.h"
#include "btapp.h"
#include "cqueue.h"
#include "hci_api.h"
#include "resources.h"
#include "tgt_hardware.h"

#include "a2dp_codec_sbc.h"
#include "avdtp_api.h"

extern struct BT_DEVICE_T app_bt_device;

#ifdef __A2DP_AVDTP_CP__
btif_avdtp_content_prot_t a2dp_avdtpCp[BT_DEVICE_NUM];
U8 a2dp_avdtpCp_securityData[BT_DEVICE_NUM][BTIF_AVDTP_MAX_CP_VALUE_SIZE] = {};
#endif /* __A2DP_AVDTP_CP__ */

btif_avdtp_codec_t a2dp_avdtpcodec;

const unsigned char a2dp_codec_elements[] = {
    A2D_SBC_IE_SAMP_FREQ_48 | A2D_SBC_IE_SAMP_FREQ_44 | A2D_SBC_IE_CH_MD_MONO |
        A2D_SBC_IE_CH_MD_STEREO | A2D_SBC_IE_CH_MD_JOINT |
        A2D_SBC_IE_CH_MD_DUAL,
    A2D_SBC_IE_BLOCKS_16 | A2D_SBC_IE_BLOCKS_12 | A2D_SBC_IE_BLOCKS_8 |
        A2D_SBC_IE_BLOCKS_4 | A2D_SBC_IE_SUBBAND_8 | A2D_SBC_IE_ALLOC_MD_L |
        A2D_SBC_IE_ALLOC_MD_S,
    A2D_SBC_IE_MIN_BITPOOL, BTA_AV_CO_SBC_MAX_BITPOOL};

bt_status_t a2dp_codec_sbc_init(int index) {
  struct BT_DEVICE_T *bt_dev = &app_bt_device;

  bt_status_t st;
#ifdef __A2DP_AVDTP_CP__
  a2dp_avdtpCp[index].cpType = BTIF_AVDTP_CP_TYPE_SCMS_T;
  a2dp_avdtpCp[index].data = (U8 *)&a2dp_avdtpCp_securityData[index][0];
  a2dp_avdtpCp[index].dataLen = 0;
#endif /* __A2DP_AVDTP_CP__ */

  a2dp_avdtpcodec.codecType = BTIF_AVDTP_CODEC_TYPE_SBC;
  a2dp_avdtpcodec.discoverable = 1;
  a2dp_avdtpcodec.elements = (U8 *)&a2dp_codec_elements;
  a2dp_avdtpcodec.elemLen = 4;

#ifdef __A2DP_AVDTP_CP__
  st = btif_a2dp_register(bt_dev->a2dp_stream[index]->a2dp_stream,
                          BTIF_A2DP_STREAM_TYPE_SINK, &a2dp_avdtpcodec,
                          &a2dp_avdtpCp[index], 0, index, a2dp_callback);
  btif_a2dp_add_content_protection(bt_dev->a2dp_stream[index]->a2dp_stream,
                                   &a2dp_avdtpCp[index]);
#else
  st = btif_a2dp_register(bt_dev->a2dp_stream[index]->a2dp_stream,
                          BTIF_A2DP_STREAM_TYPE_SINK, &a2dp_avdtpcodec, NULL, 0,
                          index, a2dp_callback);
#endif /* __A2DP_AVDTP_CP__ */

  return st;
}