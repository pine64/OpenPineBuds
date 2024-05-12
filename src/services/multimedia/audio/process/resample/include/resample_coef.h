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
#ifndef __RESAMPLE_COEF_H__
#define __RESAMPLE_COEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "audio_resample_ex.h"

extern const struct RESAMPLE_COEF_T resample_coef_50p7k_to_48k;
extern const struct RESAMPLE_COEF_T resample_coef_50p7k_to_44p1k;
extern const struct RESAMPLE_COEF_T resample_coef_44p1k_to_48k;
extern const struct RESAMPLE_COEF_T resample_coef_48k_to_44p1k;

extern const struct RESAMPLE_COEF_T resample_coef_32k_to_50p7k;
extern const struct RESAMPLE_COEF_T resample_coef_44p1k_to_50p7k;
extern const struct RESAMPLE_COEF_T resample_coef_48k_to_50p7k;
extern const struct RESAMPLE_COEF_T resample_coef_8k_to_8p4k;
extern const struct RESAMPLE_COEF_T resample_coef_8p4k_to_8k;

extern const struct RESAMPLE_COEF_T resample_coef_16k_to_48k;

extern const struct RESAMPLE_COEF_T resample_coef_any_up64;
extern const struct RESAMPLE_COEF_T resample_coef_any_up256;
extern const struct RESAMPLE_COEF_T resample_coef_any_up512_32;
extern const struct RESAMPLE_COEF_T resample_coef_any_up512_36;

#ifdef __cplusplus
}
#endif

#endif
