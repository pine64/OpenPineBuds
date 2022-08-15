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
#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

void *copy_mem32(void *dst, const void *src, unsigned int size);

void *copy_mem16(void *dst, const void *src, unsigned int size);

void *copy_mem(void *dst, const void *src, unsigned int size);

void *zero_mem32(void *dst, unsigned int size);

void *zero_mem16(void *dst, unsigned int size);

void *zero_mem(void *dst, unsigned int size);

#endif

