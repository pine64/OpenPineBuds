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
#ifndef __A2DP_CODEC_SCALABLE_H__
#define __A2DP_CODEC_SCALABLE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "avdtp_api.h"

#define A2DP_SCALABLE_OCTET_NUMBER                (7)
#define A2DP_SCALABLE_VENDOR_ID                   0x00000075
#define A2DP_SCALABLE_CODEC_ID                    0x0103

//To indicate bits per sample.
#define A2DP_SCALABLE_HQ	                      0x08
#define A2DP_SCALABLE_ADD_INFO1                   0x00
#define A2DP_SCALABLE_ADD_INFO2                   0x00
#define A2DP_SCALABLE_ADD_INFO3                   0x00
#define A2DP_SCALABLE_INFO(X)                 	  (X & (A2DP_SCALABLE_HQ | A2DP_SCALABLE_ADD_INFO1 | A2DP_SCALABLE_ADD_INFO2 | A2DP_SCALABLE_ADD_INFO3))

//To indicate Sampling Rate.
#define A2DP_SCALABLE_SR_96000                    0x80
#define A2DP_SCALABLE_SR_32000                    0x40
#define A2DP_SCALABLE_SR_44100                    0x20
#define A2DP_SCALABLE_SR_48000                    0x10
#define A2DP_SCALABLE_SR_DATA(X)                  (X & (A2DP_SCALABLE_SR_96000 | A2DP_SCALABLE_SR_32000 | A2DP_SCALABLE_SR_44100 | A2DP_SCALABLE_SR_48000))

//To indicate bits per sample.
#define A2DP_SCALABLE_FMT_24                      0x08
#define A2DP_SCALABLE_FMT_16                      0x00
#define A2DP_SCALABLE_FMT_DATA(X)                 (X & (A2DP_SCALABLE_FMT_24 | A2DP_SCALABLE_FMT_16))

extern btif_avdtp_codec_t a2dp_scalable_avdtpcodec;
extern const unsigned char a2dp_codec_scalable_elements[];
bt_status_t a2dp_codec_scalable_init(int index);

#if defined(__cplusplus)
}
#endif

#endif /* __A2DP_CODEC_SCALABLE_H__ */
