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

#include "a2dp_codec_ldac.h"
#include "avdtp_api.h"

extern struct BT_DEVICE_T app_bt_device;

#if defined(A2DP_LDAC_ON)
static btif_avdtp_codec_t a2dp_ldac_avdtpcodec;

const unsigned char a2dp_codec_ldac_elements[A2DP_LDAC_OCTET_NUMBER] = {
    0x2d,
    0x01,
    0x00,
    0x00, // Vendor ID
    0xaa,
    0x00, // Codec ID
    (A2DP_LDAC_SR_96000 | A2DP_LDAC_SR_88200 | A2DP_LDAC_SR_48000 |
     A2DP_LDAC_SR_44100),
    //    (A2DP_LDAC_SR_48000 |A2DP_LDAC_SR_44100),
    (A2DP_LDAC_CM_MONO | A2DP_LDAC_CM_DUAL | A2DP_LDAC_CM_STEREO),
};

bt_status_t a2dp_codec_ldac_init(int index) {
  bt_status_t st;

  struct BT_DEVICE_T *bt_dev = &app_bt_device;

  a2dp_ldac_avdtpcodec.codecType = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
  a2dp_ldac_avdtpcodec.discoverable = 1;
  a2dp_ldac_avdtpcodec.elements = (U8 *)&a2dp_codec_ldac_elements;
  a2dp_ldac_avdtpcodec.elemLen = sizeof(a2dp_codec_ldac_elements);
  TRACE(1, "a2dp_ldac_avdtpcodec.elemLen = %d \n",
        a2dp_ldac_avdtpcodec.elemLen);
  TRACE(7,
        "a2dp_ldac_avdtpcodec.elements->[0]=0x%02x,[1]=0x%02x,[2]=0x%02x,[3]="
        "0x%02x,[4]=0x%02x,[5]=0x%02x,[6]=0x%02x,\n",
        a2dp_ldac_avdtpcodec.elements[0], a2dp_ldac_avdtpcodec.elements[1],
        a2dp_ldac_avdtpcodec.elements[2], a2dp_ldac_avdtpcodec.elements[3],
        a2dp_ldac_avdtpcodec.elements[4], a2dp_ldac_avdtpcodec.elements[5],
        a2dp_ldac_avdtpcodec.elements[6]);
  TRACE(1, "a2dp_ldac_avdtpcodec.elements->[7]=0x%02x,\n",
        a2dp_ldac_avdtpcodec.elements[7]);

  st = btif_a2dp_register(bt_dev->a2dp_ldac_stream[index]->a2dp_stream,
                          BTIF_A2DP_STREAM_TYPE_SINK, &a2dp_ldac_avdtpcodec,
                          NULL, 3, index, a2dp_callback);

  return st;
}
#endif /* A2DP_LDAC_ON */