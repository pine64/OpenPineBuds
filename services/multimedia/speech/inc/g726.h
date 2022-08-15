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
#ifndef __G726_H
#define __G726_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned long g726_Encode(unsigned char *speech,char *bitstream, uint32_t sampleCount, uint8_t isReset);
unsigned long g726_Decode(char *bitstream,unsigned char *speech, uint32_t sampleCount, uint8_t isReset);

#ifdef __cplusplus	
}
#endif

#endif

