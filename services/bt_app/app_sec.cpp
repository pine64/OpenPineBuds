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
#include "apps.h"
#include "hal_trace.h"
#include "me_api.h"
#include "bt_if.h"
#include "nvrecord.h"
#if defined(IBRT)
#include "app_ibrt_if.h"
#endif

void pair_handler_func(enum pair_event evt, const btif_event_t *event)
{
    switch(evt) {
    case PAIR_EVENT_NUMERIC_REQ:
        break;
    case PAIR_EVENT_COMPLETE:
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("Pairing ok.");
#endif

#ifndef FPGA
#ifdef MEDIA_PLAYER_SUPPORT
        if (btif_me_get_callback_event_err_code(event) == BTIF_BEC_NO_ERROR) {
#if defined(IBRT)
            app_voice_report(APP_STATUS_INDICATION_PAIRSUCCEED,0);
#endif
        } else {
            app_voice_report(APP_STATUS_INDICATION_PAIRFAIL,0);
        }
#endif
#endif
#if defined(IBRT)
    if (app_ibrt_if_is_audio_active()){
        TRACE(0,"!!!!!!!!!! flash_touch");
        nv_record_execute_async_flush();
    }else{
        TRACE(0,"!!!!!!!!!! flash_flush");
        nv_record_flash_flush();
    }
#elif !defined(__BT_ONE_BRING_TWO__)
        nv_record_flash_flush();
#endif
        break;
    default:
        break;
    }
}

void auth_handler_func(void)
{
    /*currently do nothing*/
    return;
}
