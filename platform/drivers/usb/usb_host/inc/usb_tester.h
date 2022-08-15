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
#ifndef USB_TESTER_H
#define USB_TESTER_H

#include "FATFileSystem.h"
#include <stdint.h>


class USBFileSystem : public FATFileSystem
{

public:
    USBFileSystem();
    
    void SetDevice(int device);

    int GetDevice(void);

    virtual int disk_initialize();
    
    virtual int disk_write(const uint8_t * buffer, uint64_t sector, uint8_t count);
    
    virtual int disk_read(uint8_t * buffer, uint64_t sector, uint8_t count);
        
    virtual uint64_t disk_sectors();

protected:	
	int _device;
	u32 _blockSize;
	u32 _blockCount;
	
};


#endif
