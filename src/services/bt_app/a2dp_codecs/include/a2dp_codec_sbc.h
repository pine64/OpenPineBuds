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
#ifndef __A2DP_CODEC_SBC_H__
#define __A2DP_CODEC_SBC_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "avdtp_api.h"

extern btif_avdtp_codec_t a2dp_avdtpcodec;
extern const unsigned char a2dp_codec_elements[];
bt_status_t a2dp_codec_sbc_init(int index);

#if defined(__cplusplus)
}
#endif

#endif /* __A2DP_CODEC_SBC_H__ */