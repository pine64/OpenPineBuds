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
#ifndef __APP_IBRT_IF_INTERNAL__
#define __APP_IBRT_IF_INTERNAL__
#ifdef __cplusplus
extern "C" {
#endif

bool app_ibrt_if_false_trigger_protect(ibrt_event_type evt_type);
bool app_ibrt_if_tws_reconnect_allowed(void);
void app_ibrt_if_sniff_event_callback(const btif_event_t *event);
bool app_ibrt_if_tws_sniff_allowed(void);
void app_ibrt_if_audio_mute(void);
void app_ibrt_if_audio_recover(void);
void app_ibrt_if_pairing_mode_refresh(void);
#ifdef __cplusplus
}
#endif

#endif
