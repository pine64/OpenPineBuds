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
#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_utils.h"
#include "usb_audio_app.h"
#include "usbaudio_thread.h"

static void usb_thread(void const *argument);
osThreadDef(usb_thread, osPriorityHigh, 1, 2048, "usb");

osMailQDef (usb_mailbox, USB_MAILBOX_MAX, USB_MESSAGE);
static osMailQId usb_mailbox = NULL;
static uint8_t usb_mailbox_cnt = 0;
#define USBAUDIO_DEBUG TRACE
static int usb_mailbox_init(void)
{
    USBAUDIO_DEBUG("%s,%d",__func__,__LINE__);
	usb_mailbox = osMailCreate(osMailQ(usb_mailbox), NULL);
	if (usb_mailbox == NULL)  {
        USBAUDIO_DEBUG("Failed to Create usb_mailbox\n");
		return -1;
	}
	usb_mailbox_cnt = 0;
	return 0;
}

int usb_mailbox_put(USB_MESSAGE* msg_src)
{
	osStatus status;
	USB_MESSAGE *msg_p = NULL;

    USBAUDIO_DEBUG("%s,%d",__func__,__LINE__);
    if(usb_mailbox_cnt >= 1)
    {
        USBAUDIO_DEBUG("%s,%d usb_mailbox_cnt  = %d.",
              __func__,__LINE__,usb_mailbox_cnt);
        return 0;
    }
	msg_p = (USB_MESSAGE*)osMailAlloc(usb_mailbox, 0);
	ASSERT(msg_p, "osMailAlloc error");
	msg_p->id = msg_src->id;
	msg_p->ptr = msg_src->ptr;
	msg_p->param0 = msg_src->param0;
	msg_p->param1 = msg_src->param1;

	status = osMailPut(usb_mailbox, msg_p);
	if (osOK == status)
		usb_mailbox_cnt++;
    USBAUDIO_DEBUG("%s,%d,usb_mailbox_cnt = %d.",__func__,__LINE__,usb_mailbox_cnt);
	return (int)status;
}

int usb_mailbox_free(USB_MESSAGE* msg_p)
{
	osStatus status;

    USBAUDIO_DEBUG("%s,%d",__func__,__LINE__);
	status = osMailFree(usb_mailbox, msg_p);
	if (osOK == status)
		usb_mailbox_cnt--;
    USBAUDIO_DEBUG("%s,%d,usb_mailbox_cnt = %d.",__func__,__LINE__,usb_mailbox_cnt);
	return (int)status;
}

int usb_mailbox_get(USB_MESSAGE **msg_p)
{
	osEvent evt;
	evt = osMailGet(usb_mailbox, osWaitForever);
	if (evt.status == osEventMail) {
		*msg_p = (USB_MESSAGE*)evt.value.p;
		return 0;
	}
	return -1;
}

static void usb_thread(void const *argument)
{
    // USB_FUNC_T usb_funcp;
    USBAUDIO_DEBUG("%s,%d",__func__,__LINE__);
	while(1){
		USB_MESSAGE *msg_p = NULL;

		if (!usb_mailbox_get(&msg_p)) {
            //TRACE(2,"_debug: %s,%d",__func__,__LINE__);
            USBAUDIO_DEBUG("usb_thread: id = 0x%x, ptr = 0x%x,param0 = 0x%x,param1 = 0x%x.",
                msg_p->id,msg_p->ptr,msg_p->param0,msg_p->param1);
            usb_mailbox_free(msg_p);
            usb_audio_app_loop();

		}
	}
}

static void usb_enqueue_cmd(uint32_t data)
{
    USB_MESSAGE usb_msg;

    usb_msg.id = 0;
    usb_msg.param0 = 0;
    usb_msg.param1 = 0;
    usb_msg.ptr = 0;
    usb_mailbox_put(&usb_msg);
}

int usb_os_init(void)
{
    osThreadId usb_tid;

    USBAUDIO_DEBUG("%s,%d",__func__,__LINE__);
	if (usb_mailbox_init())	{
        USBAUDIO_DEBUG("_debug: %s,%d",__func__,__LINE__);
		return -1;
	}
	usb_tid = osThreadCreate(osThread(usb_thread), NULL);
	if (usb_tid == NULL)  {
        USBAUDIO_DEBUG("Failed to Create usb_thread\n");
		return 0;
	}

    usb_audio_set_enqueue_cmd_callback(usb_enqueue_cmd);

	return 0;
}


