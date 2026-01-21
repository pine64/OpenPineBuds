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
#ifndef __APP_MIC_ALG_H__
#define __APP_MIC_ALG_H__

#include "app_utils.h"

int app_mic_alg_audioloop(bool on, enum APP_SYSFREQ_FREQ_T freq);
void vol_state_process(uint32_t db_val);

#endif
