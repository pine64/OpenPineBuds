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


#ifndef __SCO_H__
#define __SCO_H__

#include "sco_i.h"

struct sco_item_t {
    struct list_node list;
    struct bdaddr_t remote_bdaddr;

    sco_notify_callback_t sco_notify_callback;

    void *priv;
};

#endif /* __SCO_H__ */