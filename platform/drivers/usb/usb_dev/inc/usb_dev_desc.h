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
#ifndef __USB_DEV_DESC_H__
#define __USB_DEV_DESC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

#ifdef USB_AUDIO_32BIT
#if defined(USB_AUDIO_192K) && !defined(USB_HIGH_SPEED)
#error "192K 32BIT stream can run on USB_HIGH_SPEED only"
#endif
#if defined(USB_AUDIO_176_4K) && !defined(USB_HIGH_SPEED)
#error "176.4K 32BIT stream can run on USB_HIGH_SPEED only"
#endif
#elif defined(USB_AUDIO_24BIT)
#if defined(USB_AUDIO_192K) && !defined(USB_HIGH_SPEED)
#error "192K 24BIT stream can run on USB_HIGH_SPEED only"
#endif
#if defined(USB_AUDIO_176_4K) && !defined(USB_HIGH_SPEED)
#error "176.4K 24BIT stream can run on USB_HIGH_SPEED only"
#endif
#endif

#if defined(USB_AUDIO_384K) && !(defined(USB_HIGH_SPEED) && defined(USB_AUDIO_UAC2))
#error "384K stream can run on USB_HIGH_SPEED and USB_AUDIO_UAC2 only"
#endif
#if defined(USB_AUDIO_352_8K) && !(defined(USB_HIGH_SPEED) && defined(USB_AUDIO_UAC2))
#error "352.8K stream can run on USB_HIGH_SPEED and USB_AUDIO_UAC2 only"
#endif

#if defined(USB_AUDIO_UAC2) && !defined(USB_HIGH_SPEED)
#error "USB_AUDIO_UAC2 should run on USB_HIGH_SPEED"
#endif

const uint8_t *cdc_dev_desc(uint8_t type);

const uint8_t *cdc_string_desc(uint8_t index);

const uint8_t *uaud_dev_desc(uint8_t type);

const uint8_t *uaud_string_desc(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif

