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


#ifndef __A2DP_H__
#define __A2DP_H__

#include "btlib_type.h"
#include "a2dp_i.h"
#include "avdtp_i.h"

#define AVDTP_CODEC_CAP_MAX_LENGTH (32)

#define AVDTP_VER_1_2 0x0102
#define AVDTP_VER_1_3 0x0103
#define AVDTP_LOCAL_VERSION AVDTP_VER_1_3

#define A2DP_VER_1_2 0x0102
#define A2DP_VER_1_3 0x0103
#define A2DP_LOCAL_VERSION A2DP_VER_1_3

#define A2DP_MEDIA_TYPE_AUDIO 	    0x00

#define A2DP_CODEC_SBC              0x00
#define A2DP_CODEC_MPEG12_AUDIO     0x01
#define A2DP_CODEC_MPEG24_AAC       0x02
#define A2DP_CODEC_ATRAC            0x03
#define A2DP_CODEC_VENDOR_SPECIFIC  0xff

#define A2DP_SAMPLING_FREQ_16000	(1 << 3)
#define A2DP_SAMPLING_FREQ_32000	(1 << 2)
#define A2DP_SAMPLING_FREQ_44100	(1 << 1)
#define A2DP_SAMPLING_FREQ_48000	1

#define A2DP_CHANNEL_MODE_MONO		(1 << 3)
#define A2DP_CHANNEL_MODE_DUAL_CHANNEL	(1 << 2)
#define A2DP_CHANNEL_MODE_STEREO	(1 << 1)
#define A2DP_CHANNEL_MODE_JOINT_STEREO	1

#define A2DP_BLOCK_LENGTH_4		(1 << 3)
#define A2DP_BLOCK_LENGTH_8		(1 << 2)
#define A2DP_BLOCK_LENGTH_12		(1 << 1)
#define A2DP_BLOCK_LENGTH_16		1

#define A2DP_SUBBANDS_4			(1 << 1)
#define A2DP_SUBBANDS_8			1

#define A2DP_ALLOCATION_SNR		(1 << 1)
#define A2DP_ALLOCATION_LOUDNESS	1

struct a2dp_sep {
    uint8 type;   //src or snk
    struct avdtp_local_sep *sep;
};

struct get_codec_cap_t {
    uint8 ** cap;
    uint16 * cap_len; 
    bool     done;
};

struct sbc_codec_cap {
    uint32 rfa0:4;
    uint32 media_type:4;
    uint32 media_codec_type:8;
    uint32 channel_mode:4;
    uint32 frequency:4;
    uint32 allocation_method:2;
    uint32 subbands:2;
    uint32 block_length:4;
    uint8 min_bitpool;
    uint8 max_bitpool;
} __attribute__ ((packed));

#define AAC_OCTECT0_MPEG2_AAC_LC     0x80
#define AAC_OCTECT1_SAMP_FREQ_44100  0x01
#define AAC_OCTECT2_SAMP_FREQ_48000  0x8
#define AAC_OCTECT2_CHAN_MODE_MONO   0x08
#define AAC_OCTECT2_CHAN_MODE_STEREO 0x04

struct aac_codec_cap {
    uint32 media_type:8;                //Audio            0x00
    uint32 media_codec_type:8;          //MEPG-2,4 AAC     0x02
    uint32 object_types_support:8;      //MPEG-2 AAC LC    0x80
    uint32 samp_freq_441:8;             //44100            0x01
    uint32 channel_mode:4;              //1(0x8) 2(0x4) 1 2(0xc)
    uint32 samp_freq_48k:4;             //48000            0x8-
    uint32 max_peak_bitrate_high:7;
    uint32 vbr_supported:1;
    uint8 max_peak_bitrate_low2;
    uint8 max_peak_bitrate_low1;
} __attribute__ ((packed));

#define LHDC_CODEC_SAMP_RATE_96000 0x01
#define LHDC_CODEC_SAMP_RATE_88200 0x02
#define LHDC_CODEC_SAMP_RATE_48000 0x04
#define LHDC_CODEC_SAMP_RATE_44100 0x08
#define LHDC_CODEC_BITS_PER_SAMP_24 0x10
#define LHDC_CODEC_BITS_PER_SAMP_16 0x20

#define LHDC_CODEC_VERSION_V3 0x01
#define LHDC_CODEC_VERSION_LOWER 0x00
#define LHDC_CODEC_VERSION_MASK 0x0f
#define LHDC_CODEC_MAX_SR_900 0x00
#define LHDC_CODEC_MAX_SR_500 0x10
#define LHDC_CODEC_MAX_SR_400 0x20
#define LHDC_CODEC_MAX_LLC_EN 0x40
#define LHDC_CODEC_MAX_SR_MASK 0xf0

#define LHDC_CODEC_COF_CSC_DISABLE 0x01
#define LHDC_CODEC_COF_CSC 0x02
#define LHDC_CODEC_COF_CSC_PRE 0x04
#define LHDC_CODEC_COF_CSC_RFU 0x08

struct lhdc_codec_cap {
    uint8 media_type; //Audio 0x00
    uint8 media_codec_type; //Vendor-Specific Codec 0xff
    uint8 vendor_id[4]; //3a 05 00 00
    uint8 codec_id[2];  //32 4c (Lower Ver.) or 33 4c (V3)
    uint8 sample_rate;
    uint8 max_sr_ver;
    uint8 cof_csc;
} __attribute__ ((packed));

#define LDAC_CODEC_SAMP_FREQ_96000 0x04
#define LDAC_CODEC_SAMP_FREQ_88200 0x08
#define LDAC_CODEC_SAMP_FREQ_48000 0x10
#define LDAC_CODEC_SAMP_FREQ_44100 0x20

#define LDAC_CODEC_CHAN_MODE_STEREO 0x01
#define LDAC_CODEC_CHAN_MODE_DUAL   0x02
#define LDAC_CODEC_CHAN_MODE_MONO   0x04

struct ldac_codec_cap {
    uint8 media_type; //Audio 0x00
    uint8 media_codec_type; //Vendor-Specific Codec 0xff
    uint8 vendor_id[4]; //2d 01 00 00
    uint8 codec_id[2];  //aa 00
    uint8 sample_freq;
    uint8 chan_mode;
} __attribute__ ((packed));

#define SCALABLE_CODEC_BITS_16 0x00
#define SCALABLE_CODEC_BITS_24 0x08
#define SCALABLE_CODEC_SAMP_48000 0x10
#define SCALABLE_CODEC_SAMP_44100 0x20
#define SCALABLE_CODEC_SAMP_32000 0x40
#define SCALABLE_CODEC_SAMP_96000 0x80

struct scalable_codec_cap {
    uint8 media_type; //Audio 0x00
    uint8 media_codec_type; //Vendor-Specific Codec 0xff
    uint8 vendor_id[4]; //75 00 00 00
    uint8 codec_id[2];  //03 01
    uint8 sample_rate;
} __attribute__ ((packed));

U16 a2dp_MediaPacketSize(struct a2dp_control_t *Stream);
#define a2dp_MediaPacketSize(s) (l2cap_get_tx_mtu((s)->l2capSignalHandle))

#if defined(__cplusplus)
extern "C" {
#endif

U16 a2dp_create_media_header(struct a2dp_control_t *a2dp_ctl, U8 *buffer);
void a2dp_notifyCallback(uint8 event, uint32 l2cap_channel, void *pData, uint8 reason);
void a2dp_dataRecvCallback(uint32 l2cap_handle, struct pp_buff *ppb);
int8 a2dp_close(struct a2dp_control_t * stream);
void doDisconnect(void);
#if defined(__cplusplus)
}
#endif

#endif /* __A2DP_H__ */