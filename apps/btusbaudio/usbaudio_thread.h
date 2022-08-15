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
#ifndef __USB_THREAD_H__
#define __USB_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#define USB_MAILBOX_MAX (20)

typedef struct {
	uint32_t id;
	uint32_t ptr;
	uint32_t param0;
	uint32_t param1;
} USB_MESSAGE;


//typedef int (*USB_MOD_HANDLER_T)(USB_MESSAGE_BODY *);
typedef void (*USB_FUNC_T)(uint32_t, uint32_t);

int usb_mailbox_put(USB_MESSAGE* msg_src);

int usb_mailbox_free(USB_MESSAGE* msg_p);

int usb_mailbox_get(USB_MESSAGE** msg_p);

int usb_os_init(void);

#ifdef __cplusplus
	}
#endif

#endif // __USB_THREAD_H__

