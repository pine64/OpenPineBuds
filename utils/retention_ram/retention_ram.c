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
#include "string.h"
#include "retention_ram.h"
#include "hal_timer.h"
#include "hal_trace.h"

#define OTA_RANDOM_BLE_BD_ADDR_MAGIC_CODE	0x51544F55
#define BLE_BD_ADDR_LEN						6
typedef struct 
{
	uint32_t magicCode;	// 0x51544F55: "UOTA" means the OTA firmware should use the BD address
						// in this structure, otherwise it will generate a random BD address
	uint8_t  bleBdAddr[BLE_BD_ADDR_LEN];
} BLE_BD_ADDR_FOR_OTA_T;

// retained across the HW reset
BLE_BD_ADDR_FOR_OTA_T __attribute((section(".reboot_param")))	bleBdAddrForOta;
void fillBleBdAddrForOta(uint8_t* bdAddr)
{
	bleBdAddrForOta.magicCode = OTA_RANDOM_BLE_BD_ADDR_MAGIC_CODE;
	memcpy(&(bleBdAddrForOta.bleBdAddr), bdAddr, BLE_BD_ADDR_LEN);
}

extern uint8_t ble_addr[BLE_BD_ADDR_LEN];
static unsigned int seed = 1;

static void set_rand_seed(unsigned int init)
{
    seed = init;
}

static int get_rand(void)
{
    //Based on Knuth "The Art of Computer Programming"
    seed = seed * 1103515245 + 12345;
    return ( (unsigned int) (seed / 65536) % (32767+1) );
}

void otaConfigureBleBdAddr(void)
{
	if (OTA_RANDOM_BLE_BD_ADDR_MAGIC_CODE == bleBdAddrForOta.magicCode)
	{
		// use the BD address saved in bleBdAddrForOta
		memcpy(&ble_addr, &(bleBdAddrForOta.bleBdAddr), BLE_BD_ADDR_LEN);

		// clear the magic code
		bleBdAddrForOta.magicCode = 0;
	}
	else
	{
		// generate the random BD static address
		int randomData;
		
		for (uint32_t index = 0;index < BLE_BD_ADDR_LEN/2;index++)
		{
			set_rand_seed(hal_sys_timer_get());
			randomData = get_rand();
			TRACE(2,"randomData %d is 0x%x", index, randomData);
			memcpy(&(ble_addr[index*2]), (uint8_t *)&randomData, 2);
		}

		// top two bits of the random static BD address are 1
		ble_addr[5] |= 0xC0;
	}
}


