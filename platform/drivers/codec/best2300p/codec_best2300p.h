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
#ifndef __CODEC_BEST2300P_H__
#define __CODEC_BEST2300P_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CODEC_ANC_BOOST_DELAY_FUNC)(uint32_t ms);

void codec_set_anc_boost_delay_func(CODEC_ANC_BOOST_DELAY_FUNC delay_func);

#ifdef __cplusplus
}
#endif

#endif
