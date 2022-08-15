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
#ifndef __REG_RTC_H__
#define __REG_RTC_H__

#include "plat_types.h"

// PL031 Registers
struct RTC_T
{
    __I  uint32_t RTCDR;            // 0x000
    __IO uint32_t RTCMR;            // 0x004
    __IO uint32_t RTCLR;            // 0x008
    __IO uint32_t RTCCR;            // 0x00C
    __IO uint32_t RTCIMSC;          // 0x010
    __I  uint32_t RTCRIS;           // 0x014
    __I  uint32_t RTCMIS;           // 0x018
    __O  uint32_t RTCICR;           // 0x01C
};

#define RTC_CR_EN    (1 << 0)    /* counter enable bit */

#define RTC_BIT_AI    (1 << 0) /* Alarm interrupt bit */

#endif
