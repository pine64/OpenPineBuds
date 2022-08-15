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
#include <stdio.h>
#include "hal_trace.h"
#include "customparam_section.h"
#include <string.h>

extern uint32_t __custom_parameter_start[];

static uint8_t isCustomParamSectionValid = false;

void nv_custom_parameter_section_init(void)
{
	isCustomParamSectionValid = false;
	
	if (CUSTOMPARAM_MAGIC_CODE ==
		((CUSTOM_PARAM_SECTION_HEADER_T *)__custom_parameter_start)->magic_code)
	{
		// check whether the length is correct
		uint8_t* startLogicAddr = (uint8_t *)__custom_parameter_start;
		
		CUSTOM_PARAM_SECTION_HEADER_T* pSectoinHeader = 
			(CUSTOM_PARAM_SECTION_HEADER_T *)startLogicAddr;

		uint32_t totalDataLen = OFFSETOF(CUSTOM_PARAM_SECTION_HEADER_T, entryCount) +
			pSectoinHeader->length;

		if (totalDataLen > CUSTOMPARAM_SECTION_SIZE)
		{
			return;
		}
		
		uint32_t offset = sizeof(CUSTOM_PARAM_SECTION_HEADER_T);
		for (uint32_t entry = 0;entry < pSectoinHeader->entryCount;entry++)
		{
			CUSTOM_PARAM_ENTRY_HEADER_T* pEntryHeader = 
				(CUSTOM_PARAM_ENTRY_HEADER_T *)((uint8_t *)__custom_parameter_start + offset);
			offset += (sizeof(CUSTOM_PARAM_ENTRY_HEADER_T) + pEntryHeader->paramLen);
			if (offset > CUSTOMPARAM_SECTION_SIZE)
			{
				return;
			}
		}
		
		isCustomParamSectionValid = true;

		uint8_t serialNumber[CUSTOM_PARAM_SERIAL_NUM_LEN];
		uint32_t serialNumParamLen = 0;
		bool isSuccessfullyLoaded = nv_custom_parameter_section_get_entry(CUSTOM_PARAM_SERIAL_NUM_INDEX, serialNumber, 
			&serialNumParamLen);
		if (isSuccessfullyLoaded)
		{
			TRACE(0,"Serial number is:");
			DUMP8("%02x ", serialNumber, sizeof(serialNumber));
		}		
	}
}

bool nv_custom_parameter_section_get_entry(uint16_t paramIndex, uint8_t* pParamVal, uint32_t* pParamLen)
{
	if (isCustomParamSectionValid)
	{
		uint8_t* startLogicAddr = (uint8_t *)__custom_parameter_start;
		
		CUSTOM_PARAM_SECTION_HEADER_T* pSectoinHeader = 
			(CUSTOM_PARAM_SECTION_HEADER_T *)startLogicAddr;

		uint32_t offset = sizeof(CUSTOM_PARAM_SECTION_HEADER_T);
		for (uint32_t entry = 0;entry < pSectoinHeader->entryCount;entry++)
		{
			CUSTOM_PARAM_ENTRY_HEADER_T* pEntryHeader = 
				(CUSTOM_PARAM_ENTRY_HEADER_T *)((uint8_t *)__custom_parameter_start + offset);
			if (paramIndex == pEntryHeader->paramIndex)
			{
				memcpy(pParamVal, ((uint8_t *)pEntryHeader)+sizeof(CUSTOM_PARAM_ENTRY_HEADER_T), 
					pEntryHeader->paramLen);
				*pParamLen = pEntryHeader->paramLen;
				return true;
			}
			offset += (sizeof(CUSTOM_PARAM_ENTRY_HEADER_T) + pEntryHeader->paramLen);
		}
	}

	return false;
}

#if defined(BISTO_ENABLED)||defined(GFPS_ENABLED)
uint32_t Get_ModelId(void)
{
    uint8_t Model_ID[CUSTOM_PARAM_Model_ID_LEN];
    uint32_t ModelIdValue, Model_ID_ParamLen = 0;
    if(nv_custom_parameter_section_get_entry( CUSTOM_PARAM_Mode_ID_INDEX,
                                             Model_ID, &Model_ID_ParamLen))
    {
        ModelIdValue = (Model_ID[0]<<16) | (Model_ID[1]<<8) | Model_ID[2];
        TRACE(1,"Model id is :0x%08x\n",ModelIdValue);
        return ModelIdValue;
    }
    return 0;
}
#endif
