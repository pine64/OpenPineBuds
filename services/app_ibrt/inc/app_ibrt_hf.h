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
#ifndef __APP_IBRT_HF__
#define __APP_IBRT_HF__

#ifdef __cplusplus
extern "C" {
#endif

int hfp_ibrt_service_connected_mock(void);

int hfp_ibrt_sync_get_status(ibrt_hfp_status_t *hfp_status);

int hfp_ibrt_sync_set_status(ibrt_hfp_status_t *hfp_status);

int hfp_ibrt_sco_audio_disconnected(void);

int hfp_ibrt_sco_audio_connected(hfp_sco_codec_t codec, uint16_t sco_connhdl);

bool btapp_hfp_is_sco_active(void);

uint8_t btapp_hfp_get_call_state(void);

uint8_t btapp_hfp_get_call_setup(void);

#if defined(__cplusplus)
}
#endif

#endif
