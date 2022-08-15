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
#include "bluetooth.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "apps.h"
#include "stdbool.h"
#include "app_ble_cmd_handler.h"
#include "app_ble_custom_cmd.h"
#include "rwapp_config.h"


#ifdef BTIF_BLE_APP_DATAPATH_SERVER


#define BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT	8

#define BLE_RAW_DATA_XFER_BUF_SIZE					80
/**
 * @brief waiting response timeout supervision data structure
 *
 */
typedef struct
{
    uint16_t		entryIndex; 		/**< The command waiting for the response */
	uint16_t		msTillTimeout;	/**< run-time timeout left milliseconds */
} BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T;

/**
 * @brief Custom command handling environment
 *
 */
typedef struct
{
    uint8_t		isInRawDataXferStage; 		/**< true if the received data is raw data, 
    											false if the received data is in the format of BLE_CUSTOM_CMD_PAYLOAD_T*/
	uint16_t	lengthOfRawDataXferToReceive;
	uint16_t	lengthOfReceivedRawDataXfer;
	uint8_t*	ptrRawXferDstBuf;
	BLE_RawDataReceived_Handler_t	rawDataHandler;
	BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T	waitingRspTimeoutInstance[BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT];
	uint32_t	lastSysTicks;
	uint8_t		timeoutSupervisorCount;
	osTimerId	supervisor_timer_id;
	osMutexId	mutex;
												
} BLE_CUSTOM_CMD_ENV_T;

static uint8_t rawDataXferBuf[BLE_RAW_DATA_XFER_BUF_SIZE];
static BLE_CUSTOM_CMD_ENV_T ble_custom_cmd_env;

osMutexDef(app_ble_cmd_mutex);

static void ble_custom_cmd_rsp_supervision_timer_cb(void const *n);
osTimerDef (APP_CUSTOM_CMD_RSP_SUPERVISION_TIMER, ble_custom_cmd_rsp_supervision_timer_cb); 

static void BLE_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex);

extern void app_datapath_server_send_data_via_notification(uint8_t* ptrData, uint32_t length);
extern void app_datapath_server_send_data_via_indication(uint8_t* ptrData, uint32_t length);
extern void app_datapath_server_send_data_via_write_command(uint8_t* ptrData, uint32_t length);
extern void app_datapath_server_send_data_via_write_request(uint8_t* ptrData, uint32_t length);


/**
 * @brief Callback function of the waiting response supervisor timer.
 *
 */
static void ble_custom_cmd_rsp_supervision_timer_cb(void const *n)
{
	uint32_t entryIndex = ble_custom_cmd_env.waitingRspTimeoutInstance[0].entryIndex;

	BLE_remove_waiting_rsp_timeout_supervision(entryIndex);

	// it means time-out happens before the response is received from the peer device,
	// trigger the response handler
	CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex)->cmdRspHandler(TIMEOUT_WAITING_RESPONSE, NULL, 0);
}

void BLE_get_response_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
	// parameter length check
	if (paramLen > sizeof(BLE_CUSTOM_CMD_RSP_T))
	{
		return;
	}

	if (0 == ble_custom_cmd_env.timeoutSupervisorCount)
	{
		return;
	}

	BLE_CUSTOM_CMD_RSP_T* rsp = (BLE_CUSTOM_CMD_RSP_T *)ptrParam;

	BLE_CUSTOM_CMD_INSTANCE_T* ptCmdInstance = 
		BLE_custom_command_get_entry_pointer_from_cmd_code(rsp->cmdCodeToRsp);
	if (NULL == ptCmdInstance)
	{
		return;
	}
	
	// remove the function code from the time-out supervision chain
	BLE_remove_waiting_rsp_timeout_supervision(rsp->cmdCodeToRsp);	

	// call the response handler
	if (ptCmdInstance->cmdRspHandler)
	{
		ptCmdInstance->cmdRspHandler(rsp->cmdRetStatus, rsp->rspData, rsp->rspDataLen);
	}
}

void BLE_control_raw_data_xfer(bool isStartXfer)
{
	ble_custom_cmd_env.isInRawDataXferStage = isStartXfer;

	if (true == isStartXfer)
	{
		ble_custom_cmd_env.lengthOfReceivedRawDataXfer = 0;
		// default configuration, can be customized by BLE_config_raw_data_xfer
		ble_custom_cmd_env.lengthOfRawDataXferToReceive = sizeof(rawDataXferBuf);
		ble_custom_cmd_env.ptrRawXferDstBuf = (uint8_t *)&rawDataXferBuf;
		ble_custom_cmd_env.rawDataHandler = NULL;
	}
}

void BLE_set_raw_data_xfer_received_callback(BLE_RawDataReceived_Handler_t callback)
{
	ble_custom_cmd_env.rawDataHandler = callback;
}

void BLE_raw_data_xfer_control_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
	bool isStartXfer = false;
	
	if (OP_START_RAW_DATA_XFER == funcCode)
	{
		isStartXfer = true;
	}

	BLE_control_raw_data_xfer(isStartXfer);

	BLE_send_response_to_command(funcCode, NO_ERROR, NULL, 0, TRANSMISSION_VIA_NOTIFICATION);
}

void BLE_start_raw_data_xfer_control_rsp_handler(BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
	if (NO_ERROR == retStatus)
	{
		/**< now the sending data api won't wrap BLE_CUSTOM_CMD_PAYLOAD_T but will directly send the raw data */ 
		ble_custom_cmd_env.isInRawDataXferStage = true;
	}
}

void BLE_stop_raw_data_xfer_control_rsp_handler(BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
	if (NO_ERROR == retStatus)
	{
		ble_custom_cmd_env.isInRawDataXferStage = false;
	}
}

/**
 * @brief Refresh the waiting response supervisor list
 *
 */
static void ble_custom_cmd_refresh_supervisor_env(void)
{
	// do nothing if no supervisor was added
	if (ble_custom_cmd_env.timeoutSupervisorCount > 0)
	{	
		uint32_t currentTicks = GET_CURRENT_TICKS();
		uint32_t passedTicks;
		if (currentTicks >= ble_custom_cmd_env.lastSysTicks)
		{
			passedTicks = (currentTicks - ble_custom_cmd_env.lastSysTicks);
		}
		else
		{
			passedTicks = (hal_sys_timer_get_max() - ble_custom_cmd_env.lastSysTicks + 1) + currentTicks;
		}
		
		uint32_t deltaMs = TICKS_TO_MS(passedTicks);
		
		BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T* pRspSupervisor = &(ble_custom_cmd_env.waitingRspTimeoutInstance[0]);
		for (uint32_t index = 0;index < ble_custom_cmd_env.timeoutSupervisorCount;index++)
		{
			ASSERT(pRspSupervisor[index].msTillTimeout > deltaMs, 
				"the waiting command response supervisor timer is missing!!!, \
				%d ms passed but the ms to trigger is %d", deltaMs, pRspSupervisor[index].msTillTimeout);
			pRspSupervisor[index].msTillTimeout -= deltaMs;
		}
	}

	ble_custom_cmd_env.lastSysTicks = GET_CURRENT_TICKS();
}

/**
 * @brief Remove the time-out supervision of waiting response
 *
 * @param entryIndex 	Entry index of the command table
 * 
 */
static void BLE_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex)
{
	ASSERT(ble_custom_cmd_env.timeoutSupervisorCount > 0,
		"%s The BLE custom command time-out supervisor is already empty!!!", __FUNCTION__);

	osMutexWait(ble_custom_cmd_env.mutex, osWaitForever);

	uint32_t index;
	for (index = 0;index < ble_custom_cmd_env.timeoutSupervisorCount;index++)
	{
		if (ble_custom_cmd_env.waitingRspTimeoutInstance[index].entryIndex == entryIndex)
		{
			memcpy(&(ble_custom_cmd_env.waitingRspTimeoutInstance[index]), 
				&(ble_custom_cmd_env.waitingRspTimeoutInstance[index + 1]), 
				(ble_custom_cmd_env.timeoutSupervisorCount - index - 1)*sizeof(BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T));
			break;
		}
	}

	// cannot find it, directly return
	if (index == ble_custom_cmd_env.timeoutSupervisorCount)
	{
		goto exit;
	}
	
	ble_custom_cmd_env.timeoutSupervisorCount--;	

	if (ble_custom_cmd_env.timeoutSupervisorCount > 0)
	{
		// refresh supervisor environment firstly
		ble_custom_cmd_refresh_supervisor_env();
			
		// start timer, the first entry is the most close one
		osTimerStart(ble_custom_cmd_env.supervisor_timer_id, ble_custom_cmd_env.waitingRspTimeoutInstance[0].msTillTimeout);
	}
	else
	{
		// no supervisor, directly stop the timer
		osTimerStop(ble_custom_cmd_env.supervisor_timer_id);
	}

exit:
	osMutexRelease(ble_custom_cmd_env.mutex);
}

/**
 * @brief Add the time-out supervision of waiting response
 *
 * @param entryIndex 	Index of the command entry
 * 
 */
static void BLE_add_waiting_rsp_timeout_supervision(uint16_t entryIndex)
{
	ASSERT(ble_custom_cmd_env.timeoutSupervisorCount < BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT,
		"%s The BLE custom command time-out supervisor is full!!!", __FUNCTION__);

	osMutexWait(ble_custom_cmd_env.mutex, osWaitForever);

	// refresh supervisor environment firstly
	ble_custom_cmd_refresh_supervisor_env();

	BLE_CUSTOM_CMD_INSTANCE_T* pInstance = CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);

	BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T	waitingRspTimeoutInstance[BLE_CUSTOM_CMD_WAITING_RSP_TIMEOUT_COUNT];

	uint32_t index = 0, insertedIndex = 0;
	for (index = 0;index < ble_custom_cmd_env.timeoutSupervisorCount;index++)
	{
		uint32_t msTillTimeout = ble_custom_cmd_env.waitingRspTimeoutInstance[index].msTillTimeout;

		// in the order of low to high
		if ((ble_custom_cmd_env.waitingRspTimeoutInstance[index].entryIndex != entryIndex) &&
			(pInstance->timeoutWaitingRspInMs >= msTillTimeout))
		{
			waitingRspTimeoutInstance[insertedIndex++] = ble_custom_cmd_env.waitingRspTimeoutInstance[index];
		}
		else if (pInstance->timeoutWaitingRspInMs < msTillTimeout)
		{
			waitingRspTimeoutInstance[insertedIndex].entryIndex = entryIndex;
			waitingRspTimeoutInstance[insertedIndex].msTillTimeout = pInstance->timeoutWaitingRspInMs;

			insertedIndex++;
		}
	}

	// biggest one? then put it at the end of the list
	if (ble_custom_cmd_env.timeoutSupervisorCount == index)
	{
		waitingRspTimeoutInstance[insertedIndex].entryIndex = entryIndex;
		waitingRspTimeoutInstance[insertedIndex].msTillTimeout = pInstance->timeoutWaitingRspInMs;
		
		insertedIndex++;
	}

	// copy to the global variable
	memcpy((uint8_t *)&(ble_custom_cmd_env.waitingRspTimeoutInstance), (uint8_t *)&waitingRspTimeoutInstance,
		insertedIndex*sizeof(BLE_CUSTOM_CMD_WAITING_RSP_SUPERVISOR_T));

	ble_custom_cmd_env.timeoutSupervisorCount = insertedIndex;	

	// start timer, the first entry is the most close one
	osTimerStart(ble_custom_cmd_env.supervisor_timer_id, ble_custom_cmd_env.waitingRspTimeoutInstance[0].msTillTimeout);

	osMutexRelease(ble_custom_cmd_env.mutex);
}

/**
 * @brief Return the pointer of the received raw data
 *
 * @return uint8_t*	Pointer of the raw data buffer
 */
uint8_t* BLE_custom_command_raw_data_buffer_pointer(void)
{
	return ble_custom_cmd_env.ptrRawXferDstBuf;
}

/**
 * @brief Return the size of the received raw data
 *
 * @return uint16_t	Pointer of the raw data buffer
 */
uint16_t BLE_custom_command_received_raw_data_size(void)
{
	return ble_custom_cmd_env.lengthOfReceivedRawDataXfer;
}

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData 		Pointer of the received data
 * @param dataLength	Length of the received data
 * 
 * @return BLE_CUSTOM_CMD_RET_STATUS_E
 */
BLE_CUSTOM_CMD_RET_STATUS_E BLE_custom_command_receive_data(uint8_t* ptrData, uint32_t dataLength)
{
	TRACE(1,"Receive length %d data: ", dataLength);
	DUMP8("0x%02x ", ptrData, dataLength);
	BLE_CUSTOM_CMD_PAYLOAD_T* pPayload = (BLE_CUSTOM_CMD_PAYLOAD_T *)ptrData;
	
	if ((OP_START_RAW_DATA_XFER == pPayload->cmdCode) || 
		(OP_STOP_RAW_DATA_XFER == pPayload->cmdCode) ||
		(!(ble_custom_cmd_env.isInRawDataXferStage)))
	{
		// check command code
		if (pPayload->cmdCode >= OP_COMMAND_COUNT)
		{
			return INVALID_CMD_CODE;
		}

		// check parameter length
		if (pPayload->paramLen > sizeof(pPayload->param))
		{
			return PARAMETER_LENGTH_OUT_OF_RANGE;
		}

		BLE_CUSTOM_CMD_INSTANCE_T* pInstance = 
			BLE_custom_command_get_entry_pointer_from_cmd_code(pPayload->cmdCode);

		// execute the command handler
		if(!pInstance)
		    pInstance->cmdHandler(pPayload->cmdCode, pPayload->param, pPayload->paramLen);
	}
	else
	{
		// the payload of the raw data xfer is 2 bytes cmd code + raw data
		if (dataLength < sizeof(pPayload->cmdCode))
		{
			return PARAMETER_LENGTH_TOO_SHORT;
		}

		dataLength -= sizeof(pPayload->cmdCode);
		ptrData += sizeof(pPayload->cmdCode);

		if (NULL == ble_custom_cmd_env.rawDataHandler)
		{
			// default handler
		
			// save the received raw data into raw data buffer
			uint32_t bytesToSave;
			if ((dataLength + ble_custom_cmd_env.lengthOfReceivedRawDataXfer) > \
				ble_custom_cmd_env.lengthOfRawDataXferToReceive)
			{
				bytesToSave = ble_custom_cmd_env.lengthOfRawDataXferToReceive - \
					ble_custom_cmd_env.lengthOfReceivedRawDataXfer;
			}
			else
			{
				bytesToSave = dataLength;
			}
			memcpy((uint8_t *)&ble_custom_cmd_env.ptrRawXferDstBuf[ble_custom_cmd_env.lengthOfReceivedRawDataXfer], \
				ptrData, bytesToSave);

			
			ble_custom_cmd_env.lengthOfReceivedRawDataXfer += bytesToSave;		
		}
		else
		{
			// custom handler that is set by BLE_set_raw_data_xfer_received_callback
			ble_custom_cmd_env.rawDataHandler(ptrData, dataLength);
		}
	}

	return NO_ERROR;
}

static void BLE_send_out_data(BLE_CUSTOM_CMD_TRANSMISSION_PATH_E path, BLE_CUSTOM_CMD_PAYLOAD_T* ptPayLoad)
{
	switch (path)
	{
		case TRANSMISSION_VIA_NOTIFICATION:			
			app_datapath_server_send_data_via_notification((uint8_t *)ptPayLoad, 
				(uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
			break;
		case TRANSMISSION_VIA_INDICATION:			
			app_datapath_server_send_data_via_indication((uint8_t *)ptPayLoad, 
				(uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
			break;			
		case TRANSMISSION_VIA_WRITE_CMD:
			app_datapath_server_send_data_via_write_command((uint8_t *)ptPayLoad, 
				(uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
			break;
		case TRANSMISSION_VIA_WRITE_REQ:
			app_datapath_server_send_data_via_write_request((uint8_t *)ptPayLoad, 
				(uint32_t)(&(((BLE_CUSTOM_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
			break;
		default:
			break;
	}	
}

/**
 * @brief Send response to the command request 
 *
 * @param responsedCmdCode 	Command code of the responsed command request
 * @param returnStatus		Handling result
 * @param rspData 			Pointer of the response data
 * @param rspDataLen		Length of the response data
 * @param path				Path of the data transmission
 * 
 * @return BLE_CUSTOM_CMD_RET_STATUS_E
 */
BLE_CUSTOM_CMD_RET_STATUS_E BLE_send_response_to_command
	(uint32_t responsedCmdCode, BLE_CUSTOM_CMD_RET_STATUS_E returnStatus, 
	uint8_t* rspData, uint32_t rspDataLen, BLE_CUSTOM_CMD_TRANSMISSION_PATH_E path)
{
	// check responsedCmdCode's validity
	if (responsedCmdCode >= OP_COMMAND_COUNT)
	{
		return INVALID_CMD_CODE;
	}

	BLE_CUSTOM_CMD_PAYLOAD_T payload;

	BLE_CUSTOM_CMD_RSP_T* pResponse = (BLE_CUSTOM_CMD_RSP_T *)&(payload.param);

	// check parameter length
	if (rspDataLen > sizeof(pResponse->rspData))
	{
		return PARAMETER_LENGTH_OUT_OF_RANGE;
	}

	pResponse->cmdCodeToRsp = responsedCmdCode;
	pResponse->cmdRetStatus = returnStatus;
	pResponse->rspDataLen = rspDataLen;
	memcpy(pResponse->rspData, rspData, rspDataLen);

	payload.paramLen = 3*sizeof(uint16_t) + rspDataLen;

	payload.cmdCode = OP_RESPONSE_TO_CMD;

	BLE_send_out_data(path, &payload);

	return NO_ERROR;
}

/**
 * @brief Send the custom command to the peer device
 *
 * @param cmdCode 	Command code
 * @param ptrParam 	Pointer of the output parameter
 * @param paramLen	Length of the output parameter
 * @param path		Path of the data transmission
 * 
 * @return BLE_CUSTOM_CMD_RET_STATUS_E
 */
BLE_CUSTOM_CMD_RET_STATUS_E BLE_send_custom_command(uint32_t cmdCode, 
	uint8_t* ptrParam, uint32_t paramLen, BLE_CUSTOM_CMD_TRANSMISSION_PATH_E path)
{
	// check cmdCode's validity
	if (cmdCode >= OP_COMMAND_COUNT)
	{
		return INVALID_CMD_CODE;
	}

	BLE_CUSTOM_CMD_PAYLOAD_T payload;

	// check parameter length
	if (paramLen > sizeof(payload.param))
	{
		return PARAMETER_LENGTH_OUT_OF_RANGE;
	}
	
	 uint16_t entryIndex = BLE_custom_command_get_entry_index_from_cmd_code(cmdCode);
	 BLE_CUSTOM_CMD_INSTANCE_T* pInstance = CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);

	// wrap the command payload	
	payload.cmdCode = cmdCode;
	payload.paramLen = paramLen;
	memcpy(payload.param, ptrParam, paramLen);

	// send out the data
	BLE_send_out_data(path, &payload);

	// insert into time-out supervison
	if (pInstance->isNeedResponse)
	{
		BLE_add_waiting_rsp_timeout_supervision(cmdCode);
	}

	return NO_ERROR;
}

BLE_CUSTOM_CMD_INSTANCE_T* BLE_custom_command_get_entry_pointer_from_cmd_code(uint16_t cmdCode)
{
	for (uint32_t index = 0;
		index < ((uint32_t)__custom_handler_table_end-(uint32_t)__custom_handler_table_start)/sizeof(BLE_CUSTOM_CMD_INSTANCE_T);index++)
	{
		if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode)
		{
			return CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index);
		}			
	}

	return NULL;
}

uint16_t BLE_custom_command_get_entry_index_from_cmd_code(uint16_t cmdCode)
{
	for (uint32_t index = 0;
		index < (__custom_handler_table_end-__custom_handler_table_start)/sizeof(BLE_CUSTOM_CMD_INSTANCE_T);index++)
	{
		if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode)
		{
			return index;
		}			
	}

	return INVALID_CUSTOM_ENTRY_INDEX;
}

/**
 * @brief Initialize the BLE custom command framework
 *
 */
void BLE_custom_command_init(void)
{
	memset((uint8_t *)&ble_custom_cmd_env, 0, sizeof(ble_custom_cmd_env));

	ble_custom_cmd_env.supervisor_timer_id = 
		osTimerCreate(osTimer(APP_CUSTOM_CMD_RSP_SUPERVISION_TIMER), osTimerOnce, NULL);

	ble_custom_cmd_env.mutex = osMutexCreate((osMutex(app_ble_cmd_mutex)));
}
#endif

