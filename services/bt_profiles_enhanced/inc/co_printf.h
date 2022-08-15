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

#ifndef __CO_PRINTF_H__
#define __CO_PRINTF_H__

int co_strlen( const char *s );
int co_printf(const char *format, ...);
char *co_strstr(const char *s1, const char *s2);
int co_sprintf(char *out, const char *format, ...);

#endif /* __CO_PRINTF_H__ */
