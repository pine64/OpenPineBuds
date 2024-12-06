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
#ifndef RBPLAY_UTILS_H
#define RBPLAY_UTILS_H

#undef warn
#undef error

#define info(str, ...) TRACE(str, ##__VA_ARGS__)
#define warn(str, ...) TRACE("\033[31mwarn: \033[0m" str, ##__VA_ARGS__)
#define error(str, ...) TRACE("\033[31merr: \033[0m" str, ##__VA_ARGS__)

#endif
