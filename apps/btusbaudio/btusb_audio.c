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
#include "stdio.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "app_audio.h"

#include "a2dp_api.h"
#include "app_bt.h"
#include "btapp.h"
#include "usb_audio_app.h"
#include "btusb_audio.h"

extern void btusbaudio_entry(void);
extern void btusbaudio_exit(void);
extern a2dp_stream_t* app_bt_get_steam(enum BT_DEVICE_ID_T id);
extern int app_bt_get_bt_addr(enum BT_DEVICE_ID_T id,bt_bdaddr_t *bdaddr);
extern bool app_bt_a2dp_service_is_connected(void);
int app_bt_A2DP_OpenStream(a2dp_stream_t *Stream, bt_bdaddr_t *Addr);
int app_bt_A2DP_CloseStream(a2dp_stream_t *Stream);

extern bool btapp_hfp_is_call_active(void);
static bool btusb_usb_is_on = false;
static enum BTUSB_MODE btusb_mode = BTUSB_MODE_INVALID;
static bool btusb_bt_audio_is_suspend = false;

#define BT_USB_DEBUG() //TRACE(2,"_debug: %s,%d",__func__,__LINE__)
extern struct BT_DEVICE_T  app_bt_device;

static void _btusb_stream_open(unsigned int timeout_ms)
{
    a2dp_stream_t *stream = NULL;
    bt_bdaddr_t bdaddr;
    uint32_t stime = 0;
    uint32_t etime = 0;

    stime = hal_sys_timer_get();
    //BT_USB_DEBUG();
    stream = (a2dp_stream_t*)app_bt_get_steam(BT_DEVICE_ID_1);

    app_bt_get_bt_addr(BT_DEVICE_ID_1,&bdaddr);
    if(stream)
    {
        //struct BT_DEVICE_T *bt_dev = &app_bt_device;
        //A2DP_Register((a2dp_stream_t *)bt_dev->a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream, &a2dp_avdtpcodec, NULL, (A2dpCallback) a2dp_callback);
        //AVRCP_Register((AvrcpChannel *)bt_dev->avrcp_channel[BT_DEVICE_ID_1]->avrcp_channel_handle, (AvrcpCallback)avrcp_callback_CT, BTIF_AVRCP_CT_CATEGORY_1 | BTIF_AVRCP_CT_CATEGORY_2 | BTIF_AVRCP_TG_CATEGORY_2);
        BT_USB_DEBUG();
        osDelay(10);
        app_bt_A2DP_OpenStream(stream,&bdaddr);
    }
    else
    {
        BT_USB_DEBUG();
        return;
    }
    while(1)
    {
        if(app_bt_a2dp_service_is_connected()){
            etime = hal_sys_timer_get();
            TRACE(1,"_debug: a2dp service connected, wait time = 0x%x.",TICKS_TO_MS(etime - stime));
            break;
        }
        else
        {
            etime = hal_sys_timer_get();
            if(TICKS_TO_MS(etime - stime) >= timeout_ms)
            {
                TRACE(1,"_debug: a2dp service connect timeout = 0x%x.",
                       TICKS_TO_MS(etime - stime));
                break;
            }
            osDelay(10);
        }
    }
    //BT_USB_DEBUG();
}

static void _btusb_stream_close(unsigned int timeout_ms)
{
    a2dp_stream_t *stream = NULL;
    uint32_t stime = 0;
    uint32_t etime = 0;

    stime = hal_sys_timer_get();
    BT_USB_DEBUG();
    stream = (a2dp_stream_t*)app_bt_get_steam(BT_DEVICE_ID_1);
    if(stream)
    {
        BT_USB_DEBUG();
        app_bt_A2DP_CloseStream(stream);
    }
    else
    {
        BT_USB_DEBUG();
        return;
    }
    stime = hal_sys_timer_get();
    while(1)
    {
        if(!app_bt_a2dp_service_is_connected()){
            //struct BT_DEVICE_T *bt_dev = &app_bt_device;
            //AVRCP_Deregister(bt_dev->avrcp_channel[BT_DEVICE_ID_1]->avrcp_channel_handle);
            //A2DP_Deregister(stream);
            etime = hal_sys_timer_get();
            TRACE(1,"a2dp service diconnected, wait time = 0x%x.",
                  TICKS_TO_MS(etime - stime));
            break;
        }
        else
        {
            etime = hal_sys_timer_get();
            if(TICKS_TO_MS(etime - stime) >= timeout_ms)
            {
                TRACE(1,"a2dp service diconnect timeout = 0x%x.",
                       TICKS_TO_MS(etime - stime));
                break;
            }
            osDelay(10);
        }
    }
    BT_USB_DEBUG();
}

static void btusb_usbaudio_entry(void)
{
    BT_USB_DEBUG();
    btusbaudio_entry();
    btusb_usb_is_on = true ;
}

void btusb_usbaudio_open(void)
{
    BT_USB_DEBUG();
    if(!btusb_usb_is_on)
    {
        btusb_usbaudio_entry();
        BT_USB_DEBUG();
    }
    else
    {
        usb_audio_app(1);
    }
    BT_USB_DEBUG();
}

void btusb_usbaudio_close(void)
{
    BT_USB_DEBUG();
    if(btusb_usb_is_on)
    {
        usb_audio_app(0);
        BT_USB_DEBUG();
    }
    else
    {
        BT_USB_DEBUG();
    }
}

void btusb_btaudio_close(bool is_wait)
{
    BT_USB_DEBUG();
    //if(!btusb_bt_audio_is_suspend)
    {
        BT_USB_DEBUG();
        if(is_wait)
        {
            app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSEALL, 0);
            _btusb_stream_close(BTUSB_OUTTIME_MS);
        }
        else
        {
            _btusb_stream_close(0);
        }
        btusb_bt_audio_is_suspend = true;
    }
}

void btusb_btaudio_open(bool is_wait)
{
    BT_USB_DEBUG();
    //if(btusb_bt_audio_is_suspend)
    {
        TRACE(2,"%s: %d.",__func__,__LINE__);
        if(is_wait)
        {
            _btusb_stream_open(BTUSB_OUTTIME_MS);
            app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
            app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_SETUP, 0);
        }
        else
        {
            _btusb_stream_open(0);
        }
        TRACE(2,"%s: %d.",__func__,__LINE__);
        btusb_bt_audio_is_suspend = false;
    }
}

void btusb_switch(enum BTUSB_MODE mode)
{
    //BT_USB_DEBUG();
    if(mode != BTUSB_MODE_BT && mode != BTUSB_MODE_USB)
    {
        ASSERT(0, "%s:%d, mode = %d.",__func__,__LINE__,mode);
    }

    if(btusb_mode == mode) {
        BT_USB_DEBUG();
        return;
    }

    if(btusb_mode == BTUSB_MODE_INVALID)
    {
        if(mode == BTUSB_MODE_BT) {
            TRACE(1,"%s: switch to BT mode.",__func__);
            btusb_mode = BTUSB_MODE_BT;
        }
        else {
            TRACE(1,"%s: switch to USB mode.",__func__);
            //btusb_btaudio_close(true);
            osDelay(500);
            btusb_usbaudio_open();
            btusb_mode = BTUSB_MODE_USB;
        }
    }
    else
    {
        if(mode == BTUSB_MODE_BT) {
            TRACE(1,"%s: switch to BT mode.",__func__);
            if(btusb_usb_is_on)
            {
                TRACE(1,"%s: btusb_usbaudio_close.",__func__);
                btusb_usbaudio_close();
                TRACE(1,"%s: btusb_usbaudio_close done.",__func__);
                osDelay(500);
            }
            btusb_mode = BTUSB_MODE_BT;
            btusb_btaudio_open(true);
            TRACE(1,"%s: switch to BT mode done.",__func__);
        }
        else {
            if(btapp_hfp_is_call_active() == 1)
            {
                TRACE(1,"%s: hfp is call active.",__func__);
                return;
            }
            TRACE(1,"%s: switch to USB mode.",__func__);
            btusb_btaudio_close(true);
            TRACE(1,"%s: btusb_btaudio_close done.",__func__);
            osDelay(500);
            btusb_usbaudio_open();
            btusb_mode = BTUSB_MODE_USB;
            TRACE(1,"%s: switch to USB mode done.",__func__);
        }
    }
}

bool btusb_is_bt_mode(void)
{
    BT_USB_DEBUG();
    return btusb_mode == BTUSB_MODE_BT ? true : false;
}

bool btusb_is_usb_mode(void)
{
    return btusb_mode == BTUSB_MODE_USB ? true : false;
}


#if defined(BT_USB_AUDIO_DUAL_MODE_TEST)
void test_btusb_switch(void)
{
    if(btusb_mode == BTUSB_MODE_BT)
    {
        btusb_switch(BTUSB_MODE_USB);
    }
    else
    {
        btusb_switch(BTUSB_MODE_BT);
    }
}

void test_btusb_switch_to_bt(void)
{
     btusb_switch(BTUSB_MODE_BT);
}

void test_btusb_switch_to_usb(void)
{
     btusb_switch(BTUSB_MODE_USB);
}
#endif
