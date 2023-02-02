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

#include "a2dp_codec_aac.h"
#include "avdtp_api.h"

extern struct BT_DEVICE_T app_bt_device;

#if defined(A2DP_AAC_ON)

#ifdef __A2DP_AVDTP_CP__
btif_avdtp_content_prot_t a2dp_aac_avdtpCp[BT_DEVICE_NUM];
U8 a2dp_avdtpCp_aac_securityData[BT_DEVICE_NUM][BTIF_AVDTP_MAX_CP_VALUE_SIZE] =
    {};
#endif

btif_avdtp_codec_t a2dp_aac_avdtpcodec;

const unsigned char a2dp_codec_aac_elements[A2DP_AAC_OCTET_NUMBER] = {
    A2DP_AAC_OCTET0_MPEG2_AAC_LC, A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100,
    A2DP_AAC_OCTET2_CHANNELS_1 | A2DP_AAC_OCTET2_CHANNELS_2 |
        A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000,
    A2DP_AAC_OCTET3_VBR_SUPPORTED | ((MAX_AAC_BITRATE >> 16) & 0x7f),
    /* left bit rate 0 for unkown */
    (MAX_AAC_BITRATE >> 8) & 0xff, (MAX_AAC_BITRATE)&0xff};

btif_avdtp_codec_t *app_a2dp_codec_get_aac_avdtp_codec() {
  return (btif_avdtp_codec_t *)&a2dp_aac_avdtpcodec;
}

bt_status_t a2dp_codec_aac_init(int index) {
  bt_status_t st;
  struct BT_DEVICE_T *bt_dev = &app_bt_device;

#ifdef __A2DP_AVDTP_CP__
  a2dp_aac_avdtpCp[index].cpType = BTIF_AVDTP_CP_TYPE_SCMS_T;
  a2dp_aac_avdtpCp[index].data = (U8 *)&a2dp_avdtpCp_aac_securityData[index][0];
  a2dp_aac_avdtpCp[index].dataLen = 0;
#endif

  a2dp_aac_avdtpcodec.codecType = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
  a2dp_aac_avdtpcodec.discoverable = 1;
  a2dp_aac_avdtpcodec.elements = (U8 *)&a2dp_codec_aac_elements;
  a2dp_aac_avdtpcodec.elemLen = sizeof(a2dp_codec_aac_elements);

#ifdef __A2DP_AVDTP_CP__
  st = btif_a2dp_register(bt_dev->a2dp_aac_stream[index]->a2dp_stream,
                          BTIF_A2DP_STREAM_TYPE_SINK, &a2dp_aac_avdtpcodec,
                          &a2dp_aac_avdtpCp[index], 1, index, a2dp_callback);
  btif_a2dp_add_content_protection(bt_dev->a2dp_aac_stream[index]->a2dp_stream,
                                   &a2dp_aac_avdtpCp[index]);
#else
  st = btif_a2dp_register(bt_dev->a2dp_aac_stream[index]->a2dp_stream,
                          BTIF_A2DP_STREAM_TYPE_SINK, &a2dp_aac_avdtpcodec,
                          NULL, 1, index, a2dp_callback);
#endif

  return st;
}

#endif /* A2DP_AAC_ON */
