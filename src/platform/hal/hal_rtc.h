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
#ifndef __HAL_RTC_H__
#define __HAL_RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

struct rtc_time {
        int tm_sec;
        int tm_min;
        int tm_hour;
        int tm_mday;
        int tm_mon;
        int tm_year;
};

typedef void (*HAL_RTC_IRQ_HANDLER_T)(struct rtc_time *time);

int hal_rtc_get(struct rtc_time *time);

int hal_rtc_set(struct rtc_time *time);

int hal_rtc_set_alarm(struct rtc_time *time);

int hal_rtc_clear_alarm(void);

HAL_RTC_IRQ_HANDLER_T hal_rtc_set_irq_handler(HAL_RTC_IRQ_HANDLER_T handler);

#ifdef __cplusplus
}
#endif

#endif
