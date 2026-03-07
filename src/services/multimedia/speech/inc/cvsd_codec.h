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
/*******************************************************************************
** namer£ºCVSD CODEC header
** description£ºCVSD encoder and CVSD decoder header
** version£ºV1.0
** author: xuml
** modify£º2015.8.21.
*******************************************************************************/

#ifndef _CVSD_CODEC_H_
#define _CVSD_CODEC_H_


//Q fixed
#define CVSD_Q 10

struct CvsdEncSt
{
	unsigned short CvsdEncJudgeAlpha;
	int   CvsdEncLastData;
	int   CvsdEncDelta;
	int CvsdEncFirstFrameFlag;
	int FilterState0[3];
	int FilterState1[3];
	int FilterState2[3];
};
//decoder
struct CvsdDecSt
{
	unsigned short CvsdDecJudgeAlpha;
	int   CvsdDecLastData;
	int   CvsdDecDelta;
	int FilterState0[3];
	int FilterState1[3];
	int FilterState2[3];
};

#ifdef __cplusplus
extern "C" {
#endif

//encoder
void CvsdEncInit(struct CvsdEncSt *CvsdEnc);
int CvsdEncode(struct CvsdEncSt *CvsdEnc, short *pInPutData, short *pOutPutdata, int iEncSize);

//decoder
void CvsdDecInit(struct CvsdDecSt *CvsdDec);
int CvsdDecode(struct CvsdDecSt *CvsdDec, short *pInPutData, short *pOutPutdata, int iDecSize);

#ifdef __cplusplus
}
#endif

#endif












