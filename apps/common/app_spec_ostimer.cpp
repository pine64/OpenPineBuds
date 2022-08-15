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
#include "stdint.h"
#include "app_spec_ostimer.h"
#include "hal_trace.h"


/// Create timer
osStatus app_spec_timer_create (SPEC_TIMER_CTX_T *spec_timer_ctx, const osTimerDef_t *timer_def, os_timer_type type, void *argument)
{    
    spec_timer_ctx->type     = type;
    spec_timer_ctx->argument = argument;
    spec_timer_ctx->timerid  = osTimerCreate(timer_def, type, spec_timer_ctx);
    return spec_timer_ctx->timerid ? osOK: osErrorOS;
}

/// Start or restart timer
osStatus app_spec_timer_start (SPEC_TIMER_CTX_T *spec_timer_ctx, uint32_t millisec)
{
    osStatus status;

    //TRACE(1,"%s", __func__);
    if (millisec > UINT16_MAX){
        spec_timer_ctx->interval = millisec;
        spec_timer_ctx->ctx = millisec;
        status = osTimerStart(spec_timer_ctx->timerid, UINT16_MAX);
    }else{        
        spec_timer_ctx->interval = millisec;
        spec_timer_ctx->ctx = millisec;
        status = osTimerStart(spec_timer_ctx->timerid, (uint32_t)millisec);
    }

    return status;
}

/// Stop timer
osStatus app_spec_timer_stop (SPEC_TIMER_CTX_T *spec_timer_ctx)
{
  return osTimerStop(spec_timer_ctx->timerid);
}

/// Delete timer
osStatus app_spec_timer_delete (SPEC_TIMER_CTX_T *spec_timer_ctx)
{
  return osTimerDelete(spec_timer_ctx->timerid);
}

void app_spec_timer_handler(void const *para)
{
    SPEC_TIMER_CTX_T *spec_timer_ctx = (SPEC_TIMER_CTX_T *)para;

    if (spec_timer_ctx->ctx > UINT16_MAX){
        spec_timer_ctx->ctx -= UINT16_MAX;
        if (spec_timer_ctx->ctx > UINT16_MAX){
            osTimerStart(spec_timer_ctx->timerid, UINT16_MAX);
        }else{
            osTimerStart(spec_timer_ctx->timerid, spec_timer_ctx->ctx);
        }
    }else{
        (*spec_timer_ctx->ptimer)(spec_timer_ctx->argument);
        if (spec_timer_ctx->type == osTimerPeriodic){
            app_spec_timer_start(spec_timer_ctx, spec_timer_ctx->interval);
        }
    }
}

#if 0
//tester
uint32_t ibrt_test_arg;
void app_tws_ibrt_test_timer_cb(void const *para);
specTimerDef (IBRT_TEST_TIMER, app_tws_ibrt_test_timer_cb);

void app_tws_ibrt_test_timer_cb(void const *para)
{
    TRACE(2,"%s %08x", __func__, para);
}

void app_tws_ibrt_test_timer(void)
{
    app_spec_timer_create(specTimerCtx(IBRT_TEST_TIMER), specTimer(IBRT_TEST_TIMER), osTimerOnce, &ibrt_test_arg);
    app_spec_timer_start(specTimerCtx(IBRT_TEST_TIMER), 0x10100);
}
#endif
