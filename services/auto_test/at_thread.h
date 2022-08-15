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
#ifndef __AT_THREAD_H__
#define __AT_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#define AT_MAILBOX_MAX (20)

typedef struct {
	uint32_t id;
	uint32_t ptr;
	uint32_t param0;
	uint32_t param1;
} AT_MESSAGE;

#define AT_MESSAGE_ID_CMD    0


typedef void (*AT_FUNC_T)(uint32_t, uint32_t);

typedef void (*AT_THREAD)(void*);
int at_os_init(void);
int at_enqueue_cmd(uint32_t cmd_id, uint32_t param,uint32_t pfunc);
#ifdef __cplusplus
	}
#endif

#endif // __AT_THREAD_H__

