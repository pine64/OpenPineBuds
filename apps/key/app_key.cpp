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
#include "list.h"
#include "string.h"
#include "app_thread.h"
#include "app_key.h"
#include "hal_trace.h"

#define APP_KEY_TRACE(s,...) //TRACE(s, ##__VA_ARGS__)

#define KEY_EVENT_CNT_LIMIT (3)

typedef struct {
  list_t *key_list;
}APP_KEY_CONFIG;

APP_KEY_CONFIG app_key_conifg = {
    .key_list = NULL
};

osPoolDef (app_key_handle_mempool, 20, APP_KEY_HANDLE);
osPoolId   app_key_handle_mempool = NULL;
static uint8_t key_event_cnt = 0;

static int key_event_process(uint32_t key_code, uint8_t key_event)
{
    uint32_t app_keyevt;
    APP_MESSAGE_BLOCK msg;

    if (key_event_cnt>KEY_EVENT_CNT_LIMIT){
        return 0;
    }else{
        key_event_cnt++;
    }

    msg.mod_id = APP_MODUAL_KEY;
    APP_KEY_SET_MESSAGE(app_keyevt, key_code, key_event);
    msg.msg_body.message_id = app_keyevt;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_mailbox_put(&msg);

    return 0;
}

void app_key_simulate_key_event(uint32_t key_code, uint8_t key_event)
{
    key_event_process(key_code, key_event);
}

static void app_key_handle_free(void *key_handle)
{
    osPoolFree (app_key_handle_mempool, key_handle);
}

static APP_KEY_HANDLE *app_key_handle_find(const APP_KEY_STATUS *key_status)
{
    APP_KEY_HANDLE *key_handle = NULL;
    list_node_t *node = NULL;

    for (node = list_begin(app_key_conifg.key_list); node != list_end(app_key_conifg.key_list); node = list_next(node)) {
        key_handle = (APP_KEY_HANDLE *)list_node(node);
        if ((key_handle->key_status.code == key_status->code)&&(key_handle->key_status.event == key_status->event))
            return key_handle;
    }

    return NULL;
}

static int app_key_handle_process(APP_MESSAGE_BODY *msg_body)
{
    APP_KEY_STATUS key_status;
    APP_KEY_HANDLE *key_handle = NULL;

    APP_KEY_GET_CODE(msg_body->message_id, key_status.code);
    APP_KEY_GET_EVENT(msg_body->message_id, key_status.event);

    APP_KEY_TRACE(3,"%s code:%d event:%d",__func__,key_status.code, key_status.event);

    key_event_cnt--;

    key_handle = app_key_handle_find(&key_status);

    if (key_handle != NULL && key_handle->function!= NULL)
        ((APP_KEY_HANDLE_CB_T)key_handle->function)(&key_status,key_handle->param);

    return 0;
}

int app_key_handle_registration(const APP_KEY_HANDLE *key_handle)
{
    APP_KEY_HANDLE *dest_key_handle = NULL;
    APP_KEY_TRACE(1,"%s",__func__);
    dest_key_handle = app_key_handle_find(&(key_handle->key_status));

    APP_KEY_TRACE(2,"%s dest handle:0x%x",__func__,dest_key_handle);
    if (dest_key_handle == NULL){
        dest_key_handle = (APP_KEY_HANDLE *)osPoolCAlloc (app_key_handle_mempool);
        APP_KEY_TRACE(2,"%s malloc:0x%x",__func__,dest_key_handle);
        list_append(app_key_conifg.key_list, dest_key_handle);
    }
    if (dest_key_handle == NULL)
        return -1;
    APP_KEY_TRACE(5,"%s set handle:0x%x code:%d event:%d function:%x",__func__,dest_key_handle, key_handle->key_status.code, key_handle->key_status.event, key_handle->function);
    dest_key_handle->key_status.code = key_handle->key_status.code;
    dest_key_handle->key_status.event = key_handle->key_status.event;
    dest_key_handle->string = key_handle->string;
    dest_key_handle->function = key_handle->function;
    dest_key_handle->param = key_handle->param;;

    return 0;
}

void app_key_handle_clear(void)
{
    list_clear(app_key_conifg.key_list);
}

int app_key_open(int checkPwrKey)
{
    APP_KEY_TRACE(2,"%s %x",__func__, app_key_conifg.key_list);

    if (app_key_conifg.key_list == NULL)
        app_key_conifg.key_list = list_new(app_key_handle_free, NULL, NULL);

    if (app_key_handle_mempool == NULL)
        app_key_handle_mempool = osPoolCreate(osPool(app_key_handle_mempool));

    app_set_threadhandle(APP_MODUAL_KEY, app_key_handle_process);

    return hal_key_open(checkPwrKey, key_event_process);
}

int app_key_close(void)
{
    hal_key_close();
    if (app_key_conifg.key_list != NULL)
        list_free(app_key_conifg.key_list);
    app_set_threadhandle(APP_MODUAL_KEY, NULL);
    return 0;
}

uint32_t app_key_read_status(uint32_t code)
{
    return (uint32_t)hal_key_read_status((enum HAL_KEY_CODE_T)code);
}

#if defined(_AUTO_TEST_)
int simul_key_event_process(uint32_t key_code, uint8_t key_event)
{
    return key_event_process(key_code, key_event);
}
#endif

