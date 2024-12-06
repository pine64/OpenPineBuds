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
#ifndef __HSHF_H__
#define __HSHF_H__

#include "btlib_type.h"

#include "rfcomm_i.h"
#include "sco_i.h"

#define HS_CFG_SERVER_CHANNEL 0x12
#define HF_CFG_SERVER_CHANNEL 0x13

#define HF_CFG_MAX_RX_CREDIT 207
#define HF_CFG_CREDIT_GIVE_LIMIT 20

/* Functions */
int8 hs_init ( void (*indicate_callback) ( uint8 event, void *pdata ));
int8 hs_close(void);

void hs_rfcomm_datarecv_callback(uint32 rfcomm_handle, struct pp_buff *ppb, void *priv);
void hs_rfcomm_notify_callback(enum rfcomm_event_enum event,
                                uint32 rfcomm_handle, void *pData,
                                uint8 reason, void *priv);

#endif /* __HSHF_H__ */