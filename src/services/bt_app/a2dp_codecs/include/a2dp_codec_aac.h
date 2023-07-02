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
#ifndef __A2DP_CODEC_AAC_H__
#define __A2DP_CODEC_AAC_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "avdtp_api.h"
#include "tgt_hardware.h"

#ifndef MAX_AAC_BITRATE
#if defined(CHIP_BEST1400)
#define MAX_AAC_BITRATE (96000)
#else
#define MAX_AAC_BITRATE (264630)
#endif
#endif

#define A2DP_AAC_OCTET0_MPEG2_AAC_LC              0x80
#define A2DP_AAC_OCTET0_MPEG4_AAC_LC              0x40
#define A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100  0x01
#define A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000  0x80
#define A2DP_AAC_OCTET2_CHANNELS_1                0x08
#define A2DP_AAC_OCTET2_CHANNELS_2                0x04
#define A2DP_AAC_OCTET3_VBR_SUPPORTED             0x80

#define A2DP_AAC_OCTET_NUMBER                     (6)

#if defined(A2DP_AAC_ON)
extern const unsigned char a2dp_codec_aac_elements[A2DP_AAC_OCTET_NUMBER];
bt_status_t a2dp_codec_aac_init(int index);
#endif /* A2DP_AAC_ON */

#if defined(__cplusplus)
}
#endif

#endif /* __A2DP_CODEC_AAC_H__ */
