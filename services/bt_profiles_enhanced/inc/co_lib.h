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
#ifndef __CO_LIB_H__
#define __CO_LIB_H__

char ascii_char2val(const char c);
int ascii_str2val( const char str[], char base);
int ascii_strn2val( const char str[], char base, char n);

#define  __set_mask(var, mask)   {var |= mask;}
#define  __test_mask(var, mask)   (var & mask)
//#define  __test_and_clear_mask(var, mask)  ((var & mask)?(var &= ~mask,1):0)

#endif /* __CO_LIB_H__ */
