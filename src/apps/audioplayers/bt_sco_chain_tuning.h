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
#ifndef __BT_SCO_CHAIN_TUNING_H__
#define __BT_SCO_CHAIN_TUNING_H__

#ifdef __cplusplus
extern "C" {
#endif

// Initialize this module when platform setup
int speech_tuning_init(void);

// Open this module when speech stream open
int speech_tuning_open(void);

// Close this module when speech stream close
int speech_tuning_close(void);

bool speech_tuning_get_status(void);
int speech_tuning_set_status(bool en);


#ifdef __cplusplus
}
#endif

#endif