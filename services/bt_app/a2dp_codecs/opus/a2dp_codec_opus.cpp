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

#include "a2dp_codec_opus.h"
#include "avdtp_api.h"

extern struct BT_DEVICE_T  app_bt_device;

#if defined(MASTER_USE_OPUS) || defined(ALL_USE_OPUS)
static btif_avdtp_codec_t a2dp_opus_avdtpcodec;

const unsigned char a2dp_codec_opus_elements[] = {
    A2D_SBC_IE_SAMP_FREQ_48 | A2D_SBC_IE_SAMP_FREQ_44 | A2D_SBC_IE_CH_MD_STEREO | A2D_SBC_IE_CH_MD_JOINT,
    A2D_SBC_IE_BLOCKS_16 | A2D_SBC_IE_BLOCKS_12 | A2D_SBC_IE_SUBBAND_8 | A2D_SBC_IE_ALLOC_MD_L,
    A2D_SBC_IE_MIN_BITPOOL,
    BTA_AV_CO_SBC_MAX_BITPOOL
};

bt_status_t a2dp_codec_opus_init(struct BT_DEVICE_T *bt_dev, int index)
{
    return BT_STS_SUCCESS;
}

#endif /* MASTER_USE_OPUS || ALL_USE_OPUS */