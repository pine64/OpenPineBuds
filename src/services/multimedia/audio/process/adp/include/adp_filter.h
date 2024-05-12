/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: fft transform
 last mod: $Id: smallft.h,v 1.3 2003/09/16 18:35:45 jm Exp $

 ********************************************************************/
/**
   @file adp_filter.h
   @brief Discrete Rotational Fourier Transform (DRFT)
*/
 
#ifndef _ADP_FILTER_H_
#define _ADP_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "adp_config.h"



struct adpparam
{
	void *fft_lookup;
	int M;
	float w_fft[ADPFILTER_NUM*2];
	float w[ADPFILTER_NUM*2];
	float u;
};
extern void lms_block_fft(short *in, short *dest, short *out, struct adpparam *param,int upadat_flag);
extern void *adp_filter_init(int size);
#ifdef __cplusplus
}
#endif

#endif
