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

#if defined(__BQB_PROFILE_TEST__) && !defined(ENHANCED_STACK)
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_cmu.h"
#include "analog.h"
#include "bt_drv.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"


#include "cqueue.h"
#include "apps.h"
#include "tgt_hardware.h"

#include "stdio.h"
#include "cmsis_os.h"
#include "string.h"

#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_bootmode.h"

#include "audioflinger.h"
#include "apps.h"
#include "app_thread.h"
#include "app_key.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "app_utils.h"
#include "app_status_ind.h"
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory.h"
#include "app_factory_bt.h"
#endif
#include "bt_drv_interface.h"
#include "besbt.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"


#include "me_api.h"
#include "a2dp_api.h"
#include "avdtp_api.h"
#include "avctp_api.h"
#include "avrcp_api.h"
#include "btapp.h"
#include "app_bt.h"

#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#include "app_bt_media_manager.h"
#include "hal_sleep.h"


extern struct BT_DEVICE_T  app_bt_device;
void A2dp_pts_Set_Sink_Delay(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!A2dp_pts_Set_Sink_Delay\n");
    btif_a2dp_set_sink_delay(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream,10);
}

void A2dp_pts_Create_Avdtp_Signal_Channel(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!A2dp_pts_Create_Avdtp_Signal_Channel\n");
    bt_bdaddr_t bdAddr;

    //PTS addr:13 71 da 7d 1a 0
    bdAddr.address[0] = 0x13;
    bdAddr.address[1] = 0x71;
    bdAddr.address[2] = 0xda;
    bdAddr.address[3] = 0x7d;
    bdAddr.address[4] = 0x1a;
    bdAddr.address[5] = 0x00;
    btif_a2dp_open_stream(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream, &bdAddr);
}

void Hfp_pts_create_service_level_channel(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_create_service_level_channel\n");
    bt_bdaddr_t   bdAddr;

    //PTS addr:13 71 da 7d 1a 0
    bdAddr.address[0] = 0x13;
    bdAddr.address[1] = 0x71;
    bdAddr.address[2] = 0xda;
    bdAddr.address[3] = 0x7d;
    bdAddr.address[4] = 0x1a;
    bdAddr.address[5] = 0x00;
    btif_hf_create_service_link(app_bt_device.hf_channel[BT_DEVICE_ID_1],&bdAddr);
}

#if 0
typedef struct {
    AvrcpAdvancedPdu pdu;
    uint8_t para_buf[10];
}APP_A2DP_AVRCPADVANCEDPDU;
extern osPoolId   app_a2dp_avrcpadvancedpdu_mempool;
#define app_a2dp_avrcpadvancedpdu_mempool_calloc(buf)  do{ \
                                                        APP_A2DP_AVRCPADVANCEDPDU * avrcpadvancedpdu; \
                                                        avrcpadvancedpdu = (APP_A2DP_AVRCPADVANCEDPDU *)osPoolCAlloc(app_a2dp_avrcpadvancedpdu_mempool); \
                                                        buf = &(avrcpadvancedpdu->pdu); \
                                                        buf->parms = avrcpadvancedpdu->para_buf; \
                                                     }while(0);


#endif

void Avrcp_pts_volume_change_notify(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Avrcp_pts_volume_change_notify\n");
    if (app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1] == NULL)
        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1]);
    btif_avrcp_ct_register_notification(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1], BTIF_AVRCP_EID_VOLUME_CHANGED,0);
}

//extern int a2dp_volume_get(void);
#if 0
void Avrcp_pts_set_absolute_volume(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"app_avrcp_set_absolute_volume\n");
    if (app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1] == NULL)
        btif_app_a2dp_avrcpadvancedpdu_mempool_calloc(&app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1]);

    int vol = app_bt_stream_volume_get_ptr()->a2dp_vol;
    vol = 8*vol-1;
    if (vol > (0x7f-1))
        vol = 0x7f;

    btif_avrcp_ct_set_absolute_volume(app_bt_device.avrcp_channel[BT_DEVICE_ID_1],(avrcp_advanced_pdu_t*)app_bt_device.avrcp_notify_rsp[BT_DEVICE_ID_1],vol);
}
#endif
#endif
