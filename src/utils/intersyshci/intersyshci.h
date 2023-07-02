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

#ifndef __INTERSYSHCI_H__
#define __INTERSYSHCI_H__

#include "trans_adapt.h"

#if defined(__cplusplus)
extern "C" {
#endif

void BESHCI_BufferAvai(void *packet);
void BESHCI_Open(void);
void BESHCI_Close(void);
void BESHCI_Poll(void);
void BESHCI_LockBuffer(void);
void BESHCI_UNLockBuffer(void);
void BESHCI_SCO_Data_Start(void);
void BESHCI_SCO_Data_Stop(void);
void uartrxtx(void const *argument);

bool BESHCI_Controller_Log_Handler(const unsigned char *p_buff, uint32_t length);
void BESHCI_Dump_A2DP_Seq(const unsigned char *p_buff, uint32_t length);
typedef bool (*intersys_hci_cmd_filter_handler_func)(uint8_t* pbuf, uint32_t length);
void intersys_register_hci_cmd_filter_handler_callback(intersys_hci_cmd_filter_handler_func func);

#if defined(__cplusplus)
}
#endif

#endif /* __INTERSYSHCI_H__ */
