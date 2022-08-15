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
#ifndef __CUSTOMPARAM_SECTION_H__
#define __CUSTOMPARAM_SECTION_H__

#ifdef __cplusplus
extern "C" {
#endif

// Could be customized
#define CUSTOMPARAM_MAGIC_CODE	0x54534542
#define CUSTOMPARAM_VERSION		1

#define CUSTOMPARAM_SECTION_SIZE	4096	// one flash page

typedef struct 
{
	uint32_t 	magic_code;	// fixed value as CUSTOMPARAM_MAGIC_CODE
	uint16_t	version;
	uint16_t	length;		// length in bytes of the following data in the section
	uint16_t	entryCount;
	// following are parameter entries
	
} __attribute__((packed)) CUSTOM_PARAM_SECTION_HEADER_T;

typedef struct
{
	uint16_t	paramIndex;
	uint16_t	paramLen;
	// following are the parameter content with length paramLen
} __attribute__((packed)) CUSTOM_PARAM_ENTRY_HEADER_T;

#define CUSTOM_PARAM_Mode_ID_INDEX      0
#define CUSTOM_PARAM_Model_ID_LEN       3

#define CUSTOM_PARAM_SERIAL_NUM_INDEX   0
#define CUSTOM_PARAM_SERIAL_NUM_LEN     16
typedef struct
{
    uint8_t sn[CUSTOM_PARAM_SERIAL_NUM_LEN];
} CUSTOM_PARAM_SERIAL_NUM_T;

// TODO:
// Add your own custom parameters here


void nv_custom_parameter_section_init(void);
bool nv_custom_parameter_section_get_entry(
	uint16_t paramIndex, uint8_t* pParamVal, uint32_t* pParamLen);
uint32_t Get_ModelId(void);

#ifdef __cplusplus
}
#endif
#endif

