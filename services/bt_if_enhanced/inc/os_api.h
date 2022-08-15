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

#ifndef __OS_API__H__
#define __OS_API__H__

#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif
    typedef void (*osapi_timer_notify) (void);

    void osapi_stop_hardware(void);

    void osapi_resume_hardware(void);

    void osapi_memcopy(U8 * dest, const U8 * source, U32 numBytes);

    void osapi_lock_stack(void);

    void osapi_unlock_stack(void);

    void osapi_notify_evm(void);
	
	uint8_t osapi_lock_is_exist(void);

#ifdef __cplusplus
}
#endif
#endif/*__OS_API__H__*/
