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
#ifndef __VAD_SENSOR_H__
#define __VAD_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

int vad_sensor_open(void);
int vad_sensor_engine_start(void);
int vad_sensor_engine_stop(void);
int vad_sensor_close(void);

#ifdef __cplusplus
}
#endif

#endif
