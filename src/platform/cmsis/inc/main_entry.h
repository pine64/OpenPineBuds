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
#ifndef __MAIN_ENTRY_H__
#define __MAIN_ENTRY_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NOSTD
#define MAIN_ENTRY(...)                 _start(__VA_ARGS__)
#else
#define MAIN_ENTRY(...)                 main(__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif
