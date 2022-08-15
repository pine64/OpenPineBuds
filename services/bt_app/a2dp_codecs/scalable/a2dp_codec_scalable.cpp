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
#include <stdio.h>
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "app.h"
#include "bt_drv.h"
#include "app_audio.h"
#include "bluetooth.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#include "hal_location.h"
#include "a2dp_api.h"
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif

#include "a2dp_api.h"
#include "avrcp_api.h"
#include "besbt.h"

#include "cqueue.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "resources.h"
#include "app_bt_media_manager.h"
#include "tgt_hardware.h"
#include "bt_drv_interface.h"
#include "hci_api.h"

#include "a2dp_codec_scalable.h"
#include "avdtp_api.h"

extern struct BT_DEVICE_T  app_bt_device;

#if defined(A2DP_SCALABLE_ON)
btif_avdtp_codec_t a2dp_scalable_avdtpcodec;

//Vendor Specific value (8bit) : 0x78 (not support UHQ)  0xF8  (support UHQ)
// - bit 7 : 96kHz   sampling frequency supported
// - bit 6 : 32kHz   sampling frequency supported
// - bit 5 : 44.1kHz sampling frequency supported
// - bit 4 : 48kHz   sampling frequency supported
// - bit 3 : high quality supported 
// - bit 2 : additional information (current 0)
// - bit 1 : additional information (current 0)
// - bit 0 : additional information (current 0)
//<1byte Vendor Specific values for Scalable codec> 
const unsigned char a2dp_codec_scalable_elements[A2DP_SCALABLE_OCTET_NUMBER] =
{
    0x75,0x0,0x0,0x0, // vendor id
    0x03,0x01,          // vendor specific codec id
#if defined(A2DP_SCALABLE_UHQ_SUPPORT)
    0xf8,                 // vendor specific value
#else
    0x78,                // vendor specific value
#endif
};

bt_status_t a2dp_codec_scalable_init(int index)
{
    bt_status_t st;

    struct BT_DEVICE_T *bt_dev = &app_bt_device;

    a2dp_scalable_avdtpcodec.codecType = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
    a2dp_scalable_avdtpcodec.discoverable = 1;
    a2dp_scalable_avdtpcodec.elements = (U8 *)&a2dp_codec_scalable_elements;
    a2dp_scalable_avdtpcodec.elemLen  = sizeof(a2dp_codec_scalable_elements);

    st = btif_a2dp_register(bt_dev->a2dp_scalable_stream[index]->a2dp_stream, BTIF_A2DP_STREAM_TYPE_SINK, &a2dp_scalable_avdtpcodec, NULL, 4,index,a2dp_callback);

    return st;
}
#endif /* A2DP_SCALABLE_ON */
