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

#ifndef __SPP_TASK_H__
#define __SPP_TASK_H__

#include "cmsis_os.h"
#include "cqueue.h"
#include "rfcomm_api.h"
#include "sdp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

osThreadId create_spp_read_thread(void);

void close_spp_read_thread(void);

int spp_mailbox_put(struct spp_device *dev, uint8_t spp_dev_num, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __SPP_TASK_H__ */
