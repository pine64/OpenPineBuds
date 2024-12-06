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
#ifndef __FAR_FIELD_SPEECH_ENHANCEMENT_H__
#define __FAR_FIELD_SPEECH_ENHANCEMENT_H__

#ifdef __cplusplus
extern "C" {
#endif

void far_field_speech_enhancement_init(void);
void far_field_speech_enhancement_start(void);
void far_field_speech_enhancement_process(short *in);
void far_field_speech_enhancement_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
