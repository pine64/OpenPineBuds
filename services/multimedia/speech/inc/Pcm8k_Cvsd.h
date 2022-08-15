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
/////////////////////////////////////////////////////////////////////////
//PCM8K <=> CVSD                                      
/////////////////////////////////////////////////////////////////////////
#ifndef _PCM8K_CVSD_H_
#define _PCM8K_CVSD_H_



//max num of input samples
#define MAXNUMOFSAMPLES 128


//resample delay 
#define RESAMPLE_DELAY 32 


void Pcm8k_CvsdInit(void);
void Pcm8kToCvsd(short *PcmInBuf, unsigned char *CvsdOutBuf, int numsample);
void CvsdToPcm8k(unsigned char *CvsdInBuf, short *PcmOutBuf, int numsample, int LossFlag);


#endif












