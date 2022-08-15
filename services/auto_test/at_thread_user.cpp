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
#include "at_thread_user.h"

#if 1
#define AT_THREAD_TRACE    TRACE
#else
#define AT_THREAD_TRACE(str,...)
#endif

//static osThreadId at_tid_user1  = NULL;
static void at_thread_user0(void const *argument);
static void at_thread_user1(void const *argument);
static void at_thread_user2(void const *argument);
static void at_thread_user3(void const *argument);
static void at_thread_user4(void const *argument);
//static osMailQId at_mailbox_user0 = NULL;
//static osMailQId at_mailbox_user1 = NULL;
//static osMailQId at_mailbox_user2 = NULL;
//static osMailQId at_mailbox_user3 = NULL;
//static osMailQId at_mailbox_user4 = NULL;

osThreadDef(at_thread_user0, osPriorityLow, 1, 1024, "at_user0");
osMailQDef (at_mailbox_user0, AT_MAILBOX_MAX, AT_USER_MESSAGE);

osThreadDef(at_thread_user1, osPriorityLow, 1, 1024, "at_user1");
osMailQDef (at_mailbox_user1, AT_MAILBOX_MAX, AT_USER_MESSAGE);
osThreadDef(at_thread_user2, osPriorityLow, 1, 1024, "at_user2");
osMailQDef (at_mailbox_user2, AT_MAILBOX_MAX, AT_USER_MESSAGE);
osThreadDef(at_thread_user3, osPriorityNormal, 1, 512, "at_user3");
osMailQDef (at_mailbox_user3, AT_MAILBOX_MAX, AT_USER_MESSAGE);
osThreadDef(at_thread_user4, osPriorityNormal, 1, 512, "at user4");
osMailQDef (at_mailbox_user4, AT_MAILBOX_MAX, AT_USER_MESSAGE);

typedef struct{
    bool is_inited;
    osMailQId  mid;
    osThreadId til;
    uint32_t mail_count;
}AT_TREAD_INFO;

AT_TREAD_INFO at_thread_info[THREAD_USER_COUNT] =
    {
      {false,NULL,NULL,0},
      {false,NULL,NULL,0},
      {false,NULL,NULL,0},
      {false,NULL,NULL,0},
      {false,NULL,NULL,0},
    };

static int at_mailbox_user_put(enum THREAD_USER_ID user_id,AT_MESSAGE* msg_src)
{
	osStatus status;
	AT_MESSAGE *msg_p = NULL;

    AT_THREAD_TRACE(3,"%s,%d,user_id = %d.",__func__,__LINE__,user_id);
    if(at_thread_info[user_id].mail_count >= 1)
    {
        AT_THREAD_TRACE(3,"%s,%d mail_count  = %d.",
              __func__,__LINE__,at_thread_info[user_id].mail_count);
        return 0;
    }
	msg_p = (AT_MESSAGE*)osMailAlloc(at_thread_info[user_id].mid, 0);
	ASSERT(msg_p, "osMailAlloc error");
	msg_p->id = msg_src->id;
	msg_p->ptr = msg_src->ptr;
	msg_p->param0 = msg_src->param0;
	msg_p->param1 = msg_src->param1;

	status = osMailPut(at_thread_info[user_id].mid, msg_p);
	if (osOK == status)
		at_thread_info[user_id].mail_count ++;
    AT_THREAD_TRACE(3,"%s,%d,at_mailbox_cnt = %d.",__func__,__LINE__,at_thread_info[user_id].mail_count);
	return (int)status;
}

static int at_mailbox_user_free(enum THREAD_USER_ID user_id,AT_MESSAGE* msg_p)
{
	osStatus status;

    AT_THREAD_TRACE(3,"%s,%d,user_id = %d.",__func__,__LINE__,user_id);
	status = osMailFree(at_thread_info[user_id].mid, msg_p);
	if (osOK == status)
		at_thread_info[user_id].mail_count --;
    AT_THREAD_TRACE(3,"%s,%d,at_mailbox_cnt = %d.",__func__,__LINE__,at_thread_info[user_id].mail_count);
	return (int)status;
}

static int at_mailbox_user_get(enum THREAD_USER_ID user_id,AT_MESSAGE **msg_p)
{
	osEvent evt;
	evt = osMailGet(at_thread_info[user_id].mid, osWaitForever);
	if (evt.status == osEventMail) {
		*msg_p = (AT_MESSAGE*)evt.value.p;
		return 0;
	}
	return -1;
}

static void at_thread_user0(const void *argument)
{
    AT_FUNC_T pfunc;
    //enum THREAD_USER_ID user_id = THREAD_USER_COUNT;
    //uint32_t arg;

    AT_THREAD_TRACE(2,"%s,%d.",__func__,__LINE__);
    //arg = (uint32_t)argument;

    //AT_THREAD_TRACE(3,"%s,%d,user_id = %d.",__func__,__LINE__,user_id);
	while(1){
	    AT_THREAD_TRACE(2,"%s,%d.",__func__,__LINE__);
		AT_MESSAGE *msg_p = NULL;
		if (!at_mailbox_user_get(THREAD_USER0,&msg_p)) {
            AT_THREAD_TRACE(2,"_debug: %s,%d",__func__,__LINE__);
            AT_THREAD_TRACE(4,"at_thread_user1: id = 0x%x, ptr = 0x%x,param0 = 0x%x,param1 = 0x%x.",
                msg_p->id,msg_p->ptr,msg_p->param0,msg_p->param1);
            pfunc = (AT_FUNC_T)msg_p->ptr;
            pfunc(msg_p->param0,msg_p->param1);
            at_mailbox_user_free(THREAD_USER0,msg_p);
		}
		AT_THREAD_TRACE(2,"%s,%d.",__func__,__LINE__);
	}
}

static void at_thread_user1(void const *argument)
{
    AT_FUNC_T pfunc;

    AT_THREAD_TRACE(2,"%s,%d",__func__,__LINE__);
	while(1){
		AT_MESSAGE *msg_p = NULL;
		if (!at_mailbox_user_get(THREAD_USER1,&msg_p)) {
            AT_THREAD_TRACE(2,"_debug: %s,%d",__func__,__LINE__);
            AT_THREAD_TRACE(4,"at_thread_user1: id = 0x%x, ptr = 0x%x,param0 = 0x%x,param1 = 0x%x.",
                msg_p->id,msg_p->ptr,msg_p->param0,msg_p->param1);
            pfunc = (AT_FUNC_T)msg_p->ptr;
            pfunc(msg_p->param0,msg_p->param1);
            at_mailbox_user_free(THREAD_USER1,msg_p);
		}
	}
}
#if 1
static void at_thread_user2(void const *argument)
{
    AT_FUNC_T pfunc;
    AT_THREAD_TRACE(2,"%s,%d",__func__,__LINE__);
	while(1){
		AT_MESSAGE *msg_p = NULL;
		if (!at_mailbox_user_get(THREAD_USER2,&msg_p)) {
            AT_THREAD_TRACE(2,"_debug: %s,%d",__func__,__LINE__);
            AT_THREAD_TRACE(4,"at_thread_user1: id = 0x%x, ptr = 0x%x,param0 = 0x%x,param1 = 0x%x.",
                msg_p->id,msg_p->ptr,msg_p->param0,msg_p->param1);
            pfunc = (AT_FUNC_T)msg_p->ptr;
            pfunc(msg_p->param0,msg_p->param1);
            at_mailbox_user_free(THREAD_USER2,msg_p);
		}
	}
}
static void at_thread_user3(void const *argument)
{
    AT_FUNC_T pfunc;
    AT_THREAD_TRACE(2,"%s,%d",__func__,__LINE__);
	while(1){
		AT_MESSAGE *msg_p = NULL;
		if (!at_mailbox_user_get(THREAD_USER3,&msg_p)) {
            AT_THREAD_TRACE(2,"_debug: %s,%d",__func__,__LINE__);
            AT_THREAD_TRACE(4,"at_thread_user1: id = 0x%x, ptr = 0x%x,param0 = 0x%x,param1 = 0x%x.",
                msg_p->id,msg_p->ptr,msg_p->param0,msg_p->param1);
            pfunc = (AT_FUNC_T)msg_p->ptr;
            pfunc(msg_p->param0,msg_p->param1);
            at_mailbox_user_free(THREAD_USER3,msg_p);
		}
	}
}

static void at_thread_user4(void const *argument)
{
    AT_FUNC_T pfunc;
    AT_THREAD_TRACE(2,"%s,%d",__func__,__LINE__);
	while(1){
		AT_MESSAGE *msg_p = NULL;
		if (!at_mailbox_user_get(THREAD_USER4,&msg_p)) {
            AT_THREAD_TRACE(2,"_debug: %s,%d",__func__,__LINE__);
            AT_THREAD_TRACE(4,"at_thread_user1: id = 0x%x, ptr = 0x%x,param0 = 0x%x,param1 = 0x%x.",
                msg_p->id,msg_p->ptr,msg_p->param0,msg_p->param1);
            pfunc = (AT_FUNC_T)msg_p->ptr;
            pfunc(msg_p->param0,msg_p->param1);
            at_mailbox_user_free(THREAD_USER4,msg_p);
		}
	}
}
#endif

int at_thread_user_init(enum THREAD_USER_ID user_id)
{
    AT_THREAD_TRACE(3,"%s,%d,user_id = %d.",__func__,__LINE__,user_id);

	if(user_id == THREAD_USER0)
	{
	    at_thread_info[user_id].mid = osMailCreate(osMailQ(at_mailbox_user0), NULL);
	    at_thread_info[user_id].til = osThreadCreate(osThread(at_thread_user0), (void*)user_id);
	}
	if(user_id == THREAD_USER1)
	{
	    at_thread_info[user_id].mid = osMailCreate(osMailQ(at_mailbox_user1), NULL);
	    at_thread_info[user_id].til = osThreadCreate(osThread(at_thread_user1), (void*)user_id);
	}

	if(user_id == THREAD_USER2)
	{
	    at_thread_info[user_id].mid = osMailCreate(osMailQ(at_mailbox_user2), NULL);
	    at_thread_info[user_id].til = osThreadCreate(osThread(at_thread_user2), (void*)user_id);
	}
	if(user_id == THREAD_USER3)
	{
	    at_thread_info[user_id].mid = osMailCreate(osMailQ(at_mailbox_user3), NULL);
	    at_thread_info[user_id].til = osThreadCreate(osThread(at_thread_user3), (void*)user_id);
	}
	if(user_id == THREAD_USER4)
	{
	    at_thread_info[user_id].mid = osMailCreate(osMailQ(at_mailbox_user4), NULL);
	    at_thread_info[user_id].til = osThreadCreate(osThread(at_thread_user4), (void*)user_id);
	}

	if (at_thread_info[user_id].mid == NULL)  {
        AT_THREAD_TRACE(1,"Failed to Create mailbox, user_id = %d.",user_id);
		return -1;
	}
	if (at_thread_info[user_id].til == NULL)  {
        AT_THREAD_TRACE(1,"Failed to Create thread for user. user_id = %d",user_id);
		return -2;
	}
    at_thread_info[user_id].mail_count = 0;
	at_thread_info[user_id].is_inited = true;
	return 0;
}

bool at_thread_user_is_inited(enum THREAD_USER_ID user_id)
{
    return at_thread_info[user_id].is_inited;
}

int at_thread_user_enqueue_cmd(enum THREAD_USER_ID user_id,
        uint32_t cmd_id,
        uint32_t param0,
        uint32_t param1,
        uint32_t pfunc
        )
{
    AT_MESSAGE at_msg;
    int32_t ret;

    at_msg.id = cmd_id;
    at_msg.param0 = param0;
    at_msg.param1 = param1;
    at_msg.ptr = pfunc;
    ret = at_mailbox_user_put(user_id,&at_msg);
    return ret;
}
