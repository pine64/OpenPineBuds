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
#include "at_thread.h"

static void at_thread(void const *argument);
osThreadDef(at_thread, osPriorityNormal, 1, 1024, "AT_cmd");

osMailQDef (at_mailbox, AT_MAILBOX_MAX, AT_MESSAGE);
static osMailQId at_mailbox = NULL;
static uint8_t at_mailbox_cnt = 0;
#define AT_DEBUG TRACE
static int at_mailbox_init(void)
{
    AT_DEBUG("%s,%d",__func__,__LINE__);
	at_mailbox = osMailCreate(osMailQ(at_mailbox), NULL);
	if (at_mailbox == NULL)  {
        AT_DEBUG("Failed to Create at_mailbox\n");
		return -1;
	}
	at_mailbox_cnt = 0;
	return 0;
}

static int at_mailbox_put(AT_MESSAGE* msg_src)
{
	osStatus status;
	AT_MESSAGE *msg_p = NULL;

    AT_DEBUG("%s,%d",__func__,__LINE__);
    if(at_mailbox_cnt >= 1)
    {
        AT_DEBUG("%s,%d at_mailbox_cnt  = %d.",
              __func__,__LINE__,at_mailbox_cnt);
        return 0;
    }
	msg_p = (AT_MESSAGE*)osMailAlloc(at_mailbox, 0);
	ASSERT(msg_p, "osMailAlloc error");
	msg_p->id = msg_src->id;
	msg_p->ptr = msg_src->ptr;
	msg_p->param0 = msg_src->param0;
	msg_p->param1 = msg_src->param1;

	status = osMailPut(at_mailbox, msg_p);
	if (osOK == status)
		at_mailbox_cnt++;
    AT_DEBUG("%s,%d,at_mailbox_cnt = %d.",__func__,__LINE__,at_mailbox_cnt);
	return (int)status;
}

static int at_mailbox_free(AT_MESSAGE* msg_p)
{
	osStatus status;

    AT_DEBUG("%s,%d",__func__,__LINE__);
	status = osMailFree(at_mailbox, msg_p);
	if (osOK == status)
		at_mailbox_cnt--;
    AT_DEBUG("%s,%d,at_mailbox_cnt = %d.",__func__,__LINE__,at_mailbox_cnt);
	return (int)status;
}

static int at_mailbox_get(AT_MESSAGE **msg_p)
{
	osEvent evt;
	evt = osMailGet(at_mailbox, osWaitForever);
	if (evt.status == osEventMail) {
		*msg_p = (AT_MESSAGE*)evt.value.p;
		return 0;
	}
	return -1;
}

static void at_thread(void const *argument)
{
    AT_FUNC_T pfunc;
    // USB_FUNC_T usb_funcp;
    AT_DEBUG("%s,%d",__func__,__LINE__);
	while(1){
		AT_MESSAGE *msg_p = NULL;
		if (!at_mailbox_get(&msg_p)) {
            AT_DEBUG("_debug: %s,%d",__func__,__LINE__);
            AT_DEBUG("at_thread: id = 0x%x, ptr = 0x%x,param0 = 0x%x,param1 = 0x%x.",
                msg_p->id,msg_p->ptr,msg_p->param0,msg_p->param1);
            pfunc = (AT_FUNC_T)msg_p->ptr;
            pfunc(msg_p->param0,msg_p->param1);
            at_mailbox_free(msg_p);
		}
	}
}

int at_os_init(void)
{
    osThreadId at_tid;

    AT_DEBUG("%s,%d",__func__,__LINE__);
	if (at_mailbox_init())	{
        AT_DEBUG("_debug: %s,%d",__func__,__LINE__);
		return -1;
	}
	at_tid = osThreadCreate(osThread(at_thread), NULL);
	if (at_tid == NULL)  {
        AT_DEBUG("Failed to Create at_thread\n");
		return -2;
	}
	return 0;
}

int at_enqueue_cmd(uint32_t cmd_id, uint32_t param,uint32_t pfunc)
{
    AT_MESSAGE at_msg;
    int32_t ret;

    at_msg.id = AT_MESSAGE_ID_CMD;
    at_msg.param0 = param;
    at_msg.param1 = cmd_id;
    at_msg.ptr = pfunc;
    ret = at_mailbox_put(&at_msg);
    return ret;
}



