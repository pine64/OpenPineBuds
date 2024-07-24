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

#ifndef __PACKER_H__
#define __PACKER_H__

#define WRITE_INT8(offset, p, data)                        \
  ( *(INT8U *)(((INT8U *)p) + offset) = (INT8U)data )

#define WRITE_INT16(offset, p, data)                       \
  WRITE_INT8(offset,     p, (((INT16U)data) & 0xFF));              \
  WRITE_INT8((offset+1), p, ((((INT16U)data) >> 8) & 0xFF))

/* convert parameters into single data*/
#define READ_INT8(offset, p)                              \
  ( *(INT8U *)(((INT8U *)p) + offset) )

#define READ_INT16(offset, p)                             \
  (INT16U )(READ_INT8(offset, p) +                        \
         (READ_INT8(offset+1, p) << 8))
         
#endif /* __PACKER_H__ */