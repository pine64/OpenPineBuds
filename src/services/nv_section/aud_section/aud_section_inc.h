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
#ifndef __aud_section_inc_h__
#define __aud_section_inc_h__
#ifdef __cplusplus
extern "C" {
#endif

#include "aud_section.h"

int anccfg_loadfrom_audsec(const struct_anc_cfg *list[], const struct_anc_cfg *list_44p1k[], uint32_t count);

#ifdef __cplusplus
}
#endif
#endif

