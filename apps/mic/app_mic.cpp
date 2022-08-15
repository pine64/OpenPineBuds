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
//#include "mbed.h"
#include <stdio.h>
#include <assert.h>

#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "hal_codec.h"
#include "apps.h"

#include "app_ring_merge.h"

#include "bt_drv.h"
#include "bt_xtal_sync.h"
#include "besbt.h"
#include "app_bt_func.h"
#include "app_mic.h"


#include "app_thread.h"
#include "cqueue.h"
#include "btapp.h"
#include "app_bt_media_manager.h"
#include "string.h"
#include "hal_location.h"
#include "hal_codec.h"
#include "hal_sleep.h"
#include "app_hfp.h"

extern bool app_hfp_siri_is_active(void);
extern int a2dp_volume_2_level_convert(uint8_t vol);
extern bool mic_is_already_on;

typedef enum {
	MIC_EVENT_START,
	MIC_EVENT_STOP,
	MIC_EVENT_CHECK,
}MIC_EVENT_TYPE;

static MIC_APP_TYPE current_mictype = MIC_APP_NONE;
static struct AF_STREAM_CONFIG_T mic_config[MIC_APP_MAX];
osMutexId app_mic_mutex_id = NULL;
osMutexDef(app_mic_mutex);


// flag of is first mic date, if true ,will delete to avoid POP voice
bool first_mic_in = false;

static int internal_mic_start(MIC_APP_TYPE new_mictype)
{
    TRACE(1,"MIC_EVENT_START,current_mictype=%d",current_mictype);
    assert(new_mictype != MIC_APP_NONE);
    if (current_mictype != MIC_APP_NONE) {
        TRACE(0,"MIC START ERROR################");
        return false;
    }
    if (new_mictype == MIC_APP_SOC_CALL)
    {
        if (btapp_hfp_get_call_state() || app_hfp_siri_is_active())
        {
            TRACE(2,"[%s] tws_mic_start_telephone_call: %d", __func__, mic_config[new_mictype].sample_rate);
        	if (mic_config[new_mictype].data_ptr != NULL)
            {
            }
            else
        	{
                TRACE(1,"[%s] Warning sco play not started",__func__);
        	}
            current_mictype = MIC_APP_SOC_CALL;
        }
    }
    else if (new_mictype == MIC_APP_SPEECH_RECO)
    {
    }
    else if (new_mictype == MIC_APP_CSPOTTER)
    {
        first_mic_in = true;
        current_mictype = MIC_APP_CSPOTTER;
    }
    else if (new_mictype == MIC_APP_MICRECORD)
    {
        current_mictype = MIC_APP_MICRECORD;
    }
    else if (new_mictype == MIC_APP_OTHER)
    {
        TRACE(0,"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    }

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &mic_config[new_mictype]);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    return false;
}

static int internal_mic_stop(MIC_APP_TYPE new_mictype)
{
    TRACE(1,"MIC_EVENT_STOP,current_mictype=%d",current_mictype);
    //assert(currentMicStauts == currentStatus);
    if (new_mictype != current_mictype) {
        TRACE(0,"MIC STOP ERROR ################");
        return false;
    }
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    first_mic_in = false;
    current_mictype = MIC_APP_NONE;
    app_sysfreq_req(APP_SYSFREQ_USER_APP_3, APP_SYSFREQ_32K);
    return true;
}

static int app_mic_process(APP_MESSAGE_BODY *msg_body)
{
    MIC_EVENT_TYPE mic_event = (MIC_EVENT_TYPE)msg_body->message_id;
    MIC_APP_TYPE new_mictype = (MIC_APP_TYPE)msg_body->message_ptr;
    int ret = -1;
    TRACE(4,"%s mic_event:%d new_mictype:%d current_mictype:%d",__func__,mic_event, new_mictype, current_mictype);

    osMutexWait(app_mic_mutex_id, osWaitForever);
    if (mic_event == MIC_EVENT_START)
        ret = internal_mic_start(new_mictype);
    else if (mic_event == MIC_EVENT_STOP)
        ret = internal_mic_stop(new_mictype);
    else if (mic_event == MIC_EVENT_CHECK)
    {
        TRACE(1,"MIC_EVENT_CHECK,current_mictype=%d",current_mictype);
        if (current_mictype != new_mictype)
        {
            if (current_mictype != MIC_APP_NONE)
                internal_mic_stop(current_mictype);
            if (new_mictype != MIC_APP_CSPOTTER)
                internal_mic_start(new_mictype);
            ret = 0;
        }
    }
    else
        assert(0);
    osMutexRelease(app_mic_mutex_id);
    return ret;
}

void app_mic_init()
{
    app_mic_mutex_id = osMutexCreate((osMutex(app_mic_mutex)));
    app_set_threadhandle(APP_MODUAL_MIC, app_mic_process);
}

int app_mic_register(MIC_APP_TYPE mic_type, struct AF_STREAM_CONFIG_T *newStream)
{
    TRACE(2,"app_mic_registration mic_type:%d,newStream=%p\n",mic_type,newStream);
    if (mic_type > MIC_APP_NONE && mic_type < MIC_APP_MAX)
    {
        osMutexWait(app_mic_mutex_id, osWaitForever);
        if (memcmp(&mic_config[mic_type],newStream,sizeof(struct AF_STREAM_CONFIG_T)) != 0)
        {
            TRACE(0,"app_mic_registration Warning mic stream config changed!!!");
        }
        memcpy(&mic_config[mic_type],newStream,sizeof(struct AF_STREAM_CONFIG_T));
        osMutexRelease(app_mic_mutex_id);
        return 0;
    }
    return -1;
}

int app_mic_deregister(MIC_APP_TYPE mic_type)
{
    TRACE(1,"app_mic_deregister mic_type:%d\n",mic_type);
    if (mic_type > MIC_APP_NONE && mic_type < MIC_APP_MAX)
    {
        osMutexWait(app_mic_mutex_id, osWaitForever);
        memset(&mic_config[mic_type],0,sizeof(struct AF_STREAM_CONFIG_T));
        osMutexRelease(app_mic_mutex_id);
        return 0;
    }
    return -1;
}

bool app_mic_is_registed(MIC_APP_TYPE mic_type)
{
    TRACE(1,"app_mic_is_registed mic_type:%d\n",mic_type);
    bool ret = false;
    if (mic_type > MIC_APP_NONE && mic_type < MIC_APP_MAX)
    {
        osMutexWait(app_mic_mutex_id, osWaitForever);
        ret = mic_config[mic_type].data_ptr != NULL;
        osMutexRelease(app_mic_mutex_id);
    }
    return ret;
}

bool app_mic_start(MIC_APP_TYPE mic_type)
{
    APP_MESSAGE_BLOCK msg;
    msg.mod_id = APP_MODUAL_MIC;
    msg.msg_body.message_id = MIC_EVENT_START;
    msg.msg_body.message_ptr = mic_type;
    app_mailbox_put(&msg);
    return true;
}

bool app_mic_stop(MIC_APP_TYPE mic_type)
{
    APP_MESSAGE_BLOCK msg;
    msg.mod_id = APP_MODUAL_MIC;
    msg.msg_body.message_id = MIC_EVENT_STOP;
    msg.msg_body.message_ptr = mic_type;
    app_mailbox_put(&msg);
    return true;
}

void app_mic_check(MIC_APP_TYPE mic_type)
{
    APP_MESSAGE_BLOCK msg;
    msg.mod_id = APP_MODUAL_MIC;
    msg.msg_body.message_id = MIC_EVENT_CHECK;
    msg.msg_body.message_ptr = mic_type;
    app_mailbox_put(&msg);
}

MIC_APP_TYPE app_mic_status(void)
{
    MIC_APP_TYPE ret;
    osMutexWait(app_mic_mutex_id, osWaitForever);
    ret= current_mictype;
    osMutexRelease(app_mic_mutex_id);
    return ret;
}

