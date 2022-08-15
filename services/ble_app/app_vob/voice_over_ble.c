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
#if __VOICE_OVER_BLE_ENABLED__

#include "string.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_iomux.h"
#include "app_thread.h"
#include "app_utils.h"
#include "Pcm8k_Cvsd.h"
#include "app_audio.h"
#include "apps.h"
#include "app.h"
#include "audioflinger.h"
#include "voice_over_ble.h"
#include "app_ble_cmd_handler.h"
#include "app_ble_custom_cmd.h"
#include "app_datapath_server.h"


/**
 * @brief The states of the VOB state machine
 *
 */
typedef enum
{
    VOB_STATE_IDLE = 0,
    VOB_STATE_ADVERTISING,
    VOB_STATE_STOPPING_ADV,
    VOB_STATE_ADV_STOPPED,
    VOB_STATE_CONNECTING,
    VOB_STATE_STOPPING_CONNECTING,
    VOB_STATE_CONNECTED,
    VOB_STATE_DISCONNECTED,
    VOB_STATE_VOICE_STREAM,
} VOB_STATE_E;

/**
 * @brief Buffer management of the VOB
 *
 */
typedef struct
{
    /**< for SRC: the encoder will encode the PCM data into the audio data and fill them into encodedDataBuf 
         for DST: the received encoded data will be pushed into encodedDataBuf pending for decoding and playing */
    uint32_t    encodedDataIndexToFill;    

    /**< for SRC: the BLE will fetch data at this index from the encodedDataBuf and send them 
         for DST: the player will fetch data at this index from the encodedDataBuf, decode them and play */
    uint32_t    encodedDataIndexToFetch; 

	uint32_t    encodedDataLength;

	uint8_t		isCachingDone;
    
} VOB_DATA_BUF_MANAGEMENT_T;

/**
 * @brief Format of the audio encoder function
 *
 * @param pcmDataPtr		Pointer of the input PCM data to be encoded
 * @param pcmDataLen		Length of the input PCM data to be encoded
 * @param encodedDataPtr	Pointer of the output encoded data	
 * @param encodedDataLen	Length of the output encoded data
 * 
 */
typedef void (*PCM_Encoder_Handler_t)(uint8_t* pcmDataPtr, uint32_t pcmDataLen, encodedDataPtr, encodedDataLen);


/**< configure the voice settings   */
#define VOB_VOICE_BIT_NUMBER                    (AUD_BITS_16)
#define VOB_VOICE_SAMPLE_RATE                   (AUD_SAMPRATE_8000)
#define VOB_VOICE_VOLUME                        (10)

/**< signal of the VOB application thread */
#define VOB_SIGNAL_GET_RESPONSE                 0x80

/**< signal of the BLE thread */
#define BLE_SIGNAL_VOB_GET_ENCODED_AUDIO_DATA   0x01
#define BLE_SIGNAL_VOB_DATA_SENT_OUT            0x02
#define BLE_SIGNAL_VOB_RECEIVED_DATA            0x04
#define BLE_SIGNAL_VOB_CONNECTED                0x08
#define BLE_SIGNAL_VOB_DISCONNECTED             0x10


/**< for SRC, the buffer is used to save the collected PCM data via the MIC 
     for DST, the buffer is used to save the decoded PCM data pending for playing back*/
#define VOB_VOICE_PCM_DATA_CHUNK_SIZE					(2048)
#define VOB_VOICE_PCM_DATA_CHUNK_COUNT					(8)
#define VOB_VOICE_PCM_DATA_STORAGE_BUF_SIZE    	        (VOB_VOICE_PCM_DATA_CHUNK_SIZE*VOB_VOICE_PCM_DATA_CHUNK_SIZE)

#define VOB_ENCODED_DATA_CACHE_SIZE						(2048)

/**< for SRC, the buffer is used to save the encoded audio data,
     for DST, the buffer is used to save the received encoded audio data via the BLE*/
#define VOB_ENCODED_DATA_STORAGE_BUF_SIZE               (2048*2)

/**< timeout in ms of waiting for the response */
#define VOB_WAITING_RESPONSE_TIMEOUT_IN_MS      (5000)

/**< connection parameter */
#define VOB_CONNECTION_INTERVAL_IN_MS				5
#define VOB_CONNECTION_SUPERVISION_TIMEOUT_IN_MS	5000

/**< depends on the compression ratio of the codec algrithm */
/**< now the cvsd is used, so the ratio is 50% */
#define VOB_PCM_SIZE_TO_AUDIO_SIZE(size)        ((size)/2)
#define VOB_AUDIO_SIZE_TO_PCM_SIZE(size)        ((size)*2)


static osThreadId ble_app_tid;
static void ble_app_thread(void const *argument);
osThreadDef(ble_app_thread, osPriorityNormal, 1, 512, "ble_app");

/**< connecting supervisor timer, if timeout happened, the connecting try will be stopped */
#define VOB_CONNECTING_TIMEOUT_IN_MS            (10*1000)
static void connecting_supervisor_timer_cb(void const *n);
osTimerDef (APP_VOB_CONNECTING_SUPERVISOR_TIMER, connecting_supervisor_timer_cb);  
static osTimerId vob_connecting_supervisor_timer;

/**< state machine of the VOB */
static VOB_STATE_E vob_state;

static VOB_ROLE_E vob_role;

/**< SRC mic stream environment */
AUDIO_STREAM_ENV_T src_mic_stream_env;

/**< DST speaker stream environment */
AUDIO_STREAM_ENV_T dst_speaker_stream_env;

VOB_DATA_BUF_MANAGEMENT_T vob_data_management;

/**< mutex of the voice over ble data buffer environment */
osMutexId vob_env_mutex_id;
osMutexDef(vob_env_mutex);


#define STREAM_A2DP_SPEAKER	0


#define STREAM_A2DP_PCM_DATA_BUF_SIZE		1024
#define STREAM_A2DP_ENCODED_DATA_BUF_SIZE	1024
static const VOB_FUNCTIONALITY_CONFIG_T	vob_func_config[] = 
{
	{
		STREAM_A2DP_PCM_DATA_BUF_SIZE,
		STREAM_A2DP_ENCODED_DATA_BUF_SIZE

	},
};

static VOB_DATA_BUF_MANAGEMENT_T 	stream_a2dp_data_management;

#define STREAM_DATA_PROCESSING_UNIT_IN_BYTES	1024

struct gap_bdaddr DST_BLE_BdAddr = {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66}, 0};

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)

#define RESAMPLE_ITER_LEN                   (128 * 2 * 2)

static struct APP_RESAMPLE_T *ble_resample;

static int ble_resample_iter(uint8_t *buf, uint32_t len)
{
    uint32_t acquiredPCMDataLength = decode_sbc_frame(buf, len);

    if (acquiredPCMDataLength < len)
    {
        TRACE(0,"Start encoded data caching again.");
        stream_a2dp_data_management.isCachingDone = false;
        return 1;
    }
    return 0;
}

#endif

uint32_t a2dp_audio_prepare_pcm_data(uint8_t *buf, uint32_t len)
{
	if (stream_a2dp_data_management.isCachingDone)
	{
        // the caching stage has been done, decode the encoded data and feed the PCM
#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
	    if (allow_resample)
        {
            app_playback_resample_run(ble_resample, buf, len);
	    }
        else
#endif
        {
    		uint32_t acquiredPCMDataLength = decode_sbc_frame(buf, len);
    		// the queued encoded data does't meet the PCM data requirement, 
    		// need to cache again
    		if (acquiredPCMDataLength < len)
    		{
    			TRACE(2,"Need %d PCM data, but the queue encoded data can only provide %d bytes", 
    				len, acquiredPCMDataLength);
    			
    			memset(buf+acquiredPCMDataLength, 0, len-acquiredPCMDataLength);

    			TRACE(0,"Start encoded data caching again.");
    			stream_a2dp_data_management.isCachingDone = false;
    		}
        }
	}
	else
	{
		memset(buf, 0, len);
	}
		
    return len;
}

static uint32_t stream_a2dp_speaker_more_pcm_data(uint8_t* buf, uint32_t len)
{
#ifdef BT_XTAL_SYNC
#ifndef __TWS__
    bt_xtal_sync(BT_XTAL_SYNC_MODE_MUSIC);
#endif	// #ifndef __TWS__
#endif	// #ifdef BT_XTAL_SYNC

#ifdef __TWS__
    tws_audout_pcm_more_data(buf, len);
#else	// __TWS__

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
    app_playback_resample_run(ble_resample, buf, len);
#else
    a2dp_audio_more_data(buf, len);
#endif

    app_ring_merge_more_data(buf, len);

#ifdef __AUDIO_OUTPUT_MONO_MODE__
    merge_stereo_to_mono_16bits((int16_t *)buf, (int16_t *)buf, len/2);
#endif	// #ifdef __AUDIO_OUTPUT_MONO_MODE__

#endif  // #ifdef __TWS__

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) || defined(__AUDIO_DRC__)
    // app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);

    audio_process_run(buf, len);

    // app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
#endif	// #if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) || defined(__AUDIO_DRC__)

    return len;
}

int encoded_data_received(unsigned char *buf, unsigned int len)
{
	int32_t ret, size;
	LOCK_APP_AUDIO_QUEUE();

	// push the encoded data into the queue
	ret = APP_AUDIO_EnCQueueByForce(&sbc_queue, buf, len);
	size = APP_AUDIO_LengthOfCQueue(&sbc_queue);
	UNLOCK_APP_AUDIO_QUEUE();

	// the queue is full, overwrite the oldest data
	if (CQ_ERR == ret)
	{
		TRACE(0,"Encoded data cache is overflow.");
	}
	
	if ((!(stream_a2dp_data_management.isCachingDone)) && 
			(size >= stream_a2dp_data_management.initialCachedEncodedDataSize))
	{
		// the cached encoded data is ready, time to start decoding and feeding to the codec
		stream_a2dp_data_management.isCachingDone = true;
	}
	
    return 0;
}


/**
 * @brief Start the A2DP speaker output stream
 *
 */
void kick_off_a2dp_speaker_stream(void)
{
	// clean up the data management structure
	memset((uint8_t *)&stream_a2dp_data_management, 0, sizeof(stream_a2dp_data_management));

	// prepare the buffer
    app_audio_mempool_init();
	
	app_audio_mempool_get_buff(&(stream_a2dp_data_management.pcmDataBuf), 
		vob_func_config[STREAM_A2DP_SPEAKER].pcmDataBufSize);
    app_audio_mempool_get_buff(&(stream_a2dp_data_management.encodedDataBuf),   
		vob_func_config[STREAM_A2DP_SPEAKER].encodedDataBufSize);


    // acuqire the sufficient system clock
    app_sysfreq_req(APP_SYSFREQ_VOICE_OVER_BLE, APP_SYSFREQ_52M);

    // create the audio flinger stream
    struct AF_STREAM_CONFIG_T stream_cfg;
    
    stream_cfg.bits         = stream_a2dp_data_management.pcmBitNumber;
    stream_cfg.channel_num  = AUD_CHANNEL_NUM_2;
    stream_cfg.sample_rate  = stream_a2dp_data_management.pcmSampleRate;
    stream_cfg.vol          = VOB_VOICE_VOLUME;

    stream_cfg.device       = AUD_STREAM_USE_OPTIMIZED_STREAM;

    stream_cfg.io_path      = AUD_OUTPUT_PATH_SPEAKER;
    stream_cfg.handler      = vob_func_config[STREAM_A2DP_SPEAKER].dataCallback;

    stream_cfg.data_ptr     = BT_AUDIO_CACHE_2_UNCACHE(vob_data_management.pcmDataBufSize);
	
    stream_cfg.data_size    = STREAM_DATA_PROCESSING_UNIT_IN_BYTES;
	stream_cfg.data_chunk_count = 2;
    
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
}


static osThreadId vob_ThreadId;

/**< The negotiated MTU size between SRC and DST */
static uint16_t negotiatedMTUSize;

/**
 * @brief API to send data via BLE
 *
 * @param ptrBuf Pointer of the data to send
 * @param length Length of the data in bytes
 * 
 * @return ture if the data is successfully sent out.
 */
bool ble_send_data(uint8_t* ptrBuf, uint32_t length)
{
    app_datapath_server_send_data(ptrBuf, length);
	return true;
}

/**
 * @brief API to receive data via BLE
 *
 * @param ptrBuf Pointer of the data to send
 * @param length Length of the data in bytes
 * 
 * @return ture if the data is successfully sent out.
 */
bool ble_receive_data(uint8_t* ptrBuf, uint32_t length)
{
    
}

/**
 * @brief Stop the MIC voice input stream
 *
 */
void vob_stop_mic_input_stream(void)
{
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    
    // release the acquired system clock
    app_sysfreq_req(APP_SYSFREQ_VOICE_OVER_BLE, APP_SYSFREQ_32K);
}

/**
 * @brief Stop the speaker output stream
 *
 */
void vob_stop_speaker_output_stream(void)
{
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    
    // release the acquired system clock
    app_sysfreq_req(APP_SYSFREQ_VOICE_OVER_BLE, APP_SYSFREQ_32K);
}

/**
 * @brief Stop the voice stream
 *
 */
static void vob_stop_voice_stream(void)
{
    if (VOB_ROLE_SRC == vob_role)
    {
        // stop the MIC voice input stream
        vob_stop_mic_input_stream();
    }
    else
    {
        // stop the speaker output stream
        vob_stop_speaker_output_stream();
    }
    
    VOB_CMD_RSP_T cmd;

    // send start voice stream command to the DST
    cmd.magicCode = VOB_MAGICCODE_OF_CMD_RSP;
    cmd.cmdRspCode = VOB_CMD_STOP_VOICE_STREAM;

    bool ret = ble_send_data((uint8_t *)&cmd, &(((VOB_CMD_RSP_T *)0)->lengthOfParm))
    ASSERT_SIMPLIFIED(ret);

    // waiting until it recieves the response from the other side or timeout happens
    osEvent event = osSignalWait(VOB_SIGNAL_GET_RESPONSE, VOB_WAITING_RESPONSE_TIMEOUT_IN_MS);
    if (osEventSignal != event.status)
    {
       TRACE(0,"Time-out happens when waiting the response of stopping voice stream from DST.");
    }

    // reset the state anyway
    vob_state = VOB_STATE_CONNECTED;
}

/**
 * @brief Handler when receiving the stop the voice stream command
 *
 */
static void vob_receiving_stop_voice_stream_handler(void)
{
    if (VOB_ROLE_SRC == vob_role)
    {
        // stop the MIC voice input stream
        vob_stop_mic_input_stream();
    }
    else
    {
        // stop the speaker output stream
        vob_stop_speaker_output_stream();
    }
    
    VOB_CMD_RSP_T cmd;

    // send start voice stream command to the DST

	cmd.magicCode = VOB_MAGICCODE_OF_CMD_RSP;
    cmd.cmdRspCode = VOB_RSP_READY_TO_STOP_VOICE_STREAM;

    bool ret = ble_send_data((uint8_t *)&cmd, &(((VOB_CMD_RSP_T *)0)->lengthOfParm))
    ASSERT_SIMPLIFIED(ret);

    // reset the state anyway
    vob_state = VOB_STATE_CONNECTED;
}


/**
 * @brief Start advertising
 *
 */
static void vob_start_advertising(void)
{
    // start advertising
    appm_start_advertising();

    vob_state = VOB_STATE_ADVERTISING;
}

/**
 * @brief Process the recevied audio data from BLE, encode them and feed them to the speaker
 *
 * @param ptrBuf 	Pointer of the PCM data buffer to feed.
 * @param length	Length of the asked PCM data.
 *
 * @return uint32_t 0 means no error happens
 */
static uint32_t vob_need_more_voice_data(uint8_t* ptrBuf, uint32_t length)
{
	if (vob_data_management.isCachingDone)
	{
		// caching has been done, can decoded the received encoded data to PCM and feed them to the buffer
    	uint32_t remainingPCMBytesToFeed = length;
    	uint32_t pcmBytesToDecode;
    	do
    	{
        	pcmBytesToDecode = (remainingPCMBytesToFeed > (2*MAXNUMOFSAMPLES*src_mic_stream_env.bitNumber/AUD_BITS_8))?\
            	(2*MAXNUMOFSAMPLES*src_mic_stream_env.bitNumber/AUD_BITS_8):remainingPCMBytesToFeed;

	        // cvsd's compression ratio is 50%
	        if ((VOB_PCM_SIZE_TO_AUDIO_SIZE(pcmBytesToDecode) + vob_data_management.encodedDataIndexToFetch) > 
	            src_mic_stream_env.encodedDataBufSize)
	        {
	            pcmBytesToDecode = VOB_AUDIO_SIZE_TO_PCM_SIZE(src_mic_stream_env.encodedDataBufSize - 
	                vob_data_management.encodedDataIndexToFetch);
	        }

        	// decode to generate the PCM data        
        	CvsdToPcm8k(vob_data_management.encodedDataBuf + vob_data_management.encodedDataIndexToFetch, (short *)ptrBuf, 
            	pcmBytesToDecode / sizeof(short));

        	// update the index
        	ptrBuf += pcmBytesToDecode;
        	remainingPCMBytesToFeed -= pcmBytesToDecode;
        
        	vob_data_management.encodedDataIndexToFetch += VOB_PCM_SIZE_TO_AUDIO_SIZE(pcmBytesToDecode);
        
	        if (src_mic_stream_env.encodedDataBufSize == vob_data_management.encodedDataIndexToFetch)
	        {
	            vob_data_management.encodedDataIndexToFetch = 0;
	        }
        
    	} while (remainingPCMBytesToFeed > 0);

		osMutexWait(vob_env_mutex_id, osWaitForever);

    	// update the bytes to decode and feed to codec
    	vob_data_management.encodedDataLength -= VOB_PCM_SIZE_TO_AUDIO_SIZE(length);   

		osMutexRelease(vob_env_mutex_id);
	}
	else
	{
		// caching is not done yet, just fill all ZERO
		memset(ptrBuf, 0, length);
	}
	    
    return len;
}

/**
 * @brief Start the speaker output stream
 *
 */
void vob_kick_off_speaker_output_stream(void)
{
	// prepare the memory buffer
    app_audio_mempool_init();
	// PCM data buffer
	app_audio_mempool_get_buff(&(dst_speaker_stream_env.pcmDataBuf),       
		src_mic_stream_env.pcmDataChunkCount*dst_speaker_stream_env.pcmDataChunkSize);
	
	// encoded data buffer
   	app_audio_mempool_get_buff(&(dst_speaker_stream_env.encodedDataBuf),   
   		dst_speaker_stream_env.encodedDataBufSize);

	// encoded data queue
	APP_AUDIO_InitCQueue(&(dst_speaker_stream_env.queue), dst_speaker_stream_env.encodedDataBufSize, 
		dst_speaker_stream_env.encodedDataBuf);	
	
    // acuqire the sufficient system clock
    app_sysfreq_req(APP_SYSFREQ_VOICE_OVER_BLE, APP_SYSFREQ_26M);

    // create the audio flinger stream
    struct AF_STREAM_CONFIG_T stream_cfg;
    
    stream_cfg.bits         = dst_speaker_stream_env.bitNumber;
	stream_cfg.sample_rate  = dst_speaker_stream_env.sampleRate;
    stream_cfg.channel_num  = dst_speaker_stream_env.channelCount;    
    stream_cfg.vol          = VOB_VOICE_VOLUME;

    stream_cfg.device       = AUD_STREAM_USE_OPTIMIZED_STREAM;

    stream_cfg.io_path      = AUD_OUTPUT_PATH_SPEAKER;
    stream_cfg.handler      = dst_speaker_stream_env.morePcmHandler;

    stream_cfg.data_ptr     = BT_AUDIO_CACHE_2_UNCACHE(dst_speaker_stream_env.pcmDataBuf);
    stream_cfg.data_size    = dst_speaker_stream_env.pcmDataChunkSize;
    
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
}

/**
 * @brief Send the encoded audio data to DST via BLE
 *
 */
static void vob_send_encoded_audio_data(void)
{
	uint32_t dataBytesToSend = 0;
	uint32_t offsetInEncodedDatabuf = vob_data_management.indexToFetch;
	
	osMutexWait(vob_env_mutex_id, osWaitForever);

    if (vob_data_management.encodedDataLength > 0)
    {
        if (vob_data_management.encodedDataLength > negotiatedMTUSize)
        {
            dataBytesToSend = negotiatedMTUSize;
        }
        else
        {
            dataBytesToSend = vob_data_management.encodedDataLength;
        }

        if ((vob_data_management.indexToFetch + dataBytesToSend) > src_mic_stream_env.encodedDataBufSize)
        {
            dataBytesToSend = src_mic_stream_env.encodedDataBufSize - vob_data_management.indexToFetch;
        }

        // update the index
        vob_data_management.encodedDataLength += dataBytesToSend;
        if (src_mic_stream_env.encodedDataBufSize == (vob_data_management.indexToFetch + dataBytesToSend))
        {
            vob_data_management.indexToFetch = 0;
        }        
		else
		{
			vob_data_management.indexToFetch += dataBytesToSend;
    	}
    }

	osMutexRelease(vob_env_mutex_id);

	if (dataBytesToSend > 0)
	{
        // send out the data via the BLE
        ble_send_data((vob_data_management.encodedDataBuf + offsetInEncodedDatabuf), 
        	dataBytesToSend);
	}
}

/**
 * @brief Hanlder of the BLE data sent out event
 *
 */
void vob_data_sent_out_handler(void)
{
	vob_send_encoded_audio_data();
}

/**
 * @brief Hanlder of the response to start VOB voice stream command, called on the SRC side
 *
 */
void vob_start_voice_stream_rsp_handler(BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
	
	if (NO_ERROR == retStatus)
	{
		TRACE(0,"voice stream is successfully started!"

		// kick off the voice input stream from MIC
        vob_kick_off_mic_input_stream();

        // update the state
        vob_state = VOB_STATE_VOICE_STREAM;
	}
	else
	{
		TRACE(0,"starting voice stream failed!"
	}
}

void vob_audio_data_reiceived_handler(uint8_t* ptrBuf, uint32_t length)
{
    // only for DST, receive the encoded audio data
    if (VOB_ROLE_DST == vob_role)
    {
        // push into the encoded data buffer
        ASSERT((vob_data_management.encodedDataLength + length) <= dst_speaker_stream_env.encodedDataBufSize,
            "The left voice over ble buffer space is not suitable for the coming audio data.");

        uint32_t bytesToTheEnd = dst_speaker_stream_env.encodedDataBufSize - 
			vob_data_management.encodedDataIndexToFill;
        if (length > bytesToTheEnd)
        {
            memcpy((dst_speaker_stream_env.encodedDataBuf + vob_data_management.encodedDataIndexToFill), 
				ptrBuf, length);
        }
        else
        {            
            memcpy((dst_speaker_stream_env.encodedDataBuf + vob_data_management.encodedDataIndexToFill), 
				ptrBuf, bytesToTheEnd);
            memcpy(dst_speaker_stream_env.encodedDataBuf, ptrBuf + bytesToTheEnd, length - bytesToTheEnd);
        }

		osMutexWait(vob_env_mutex_id, osWaitForever);
		
        // update the bytes to transmit via BLE
        vob_data_management.encodedDataLength += length;    

		osMutexRelease(vob_env_mutex_id);

		if ((!vob_data_management.isCachingDone) && 
				(vob_data_management.encodedDataLength >= src_mic_stream_env.cachedEncodedDataSize))
		{
			vob_data_management.isCachingDone = true;
		}
    }
}

/**
 * @brief Hanlder of the start VOB voice stream command, called on the DST side
 *
 */
void vob_start_voice_stream_cmd_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
	BLE_CUSTOM_CMD_RET_STATUS_E retStatus = NO_ERROR;

    if (VOB_STATE_CONNECTED == vob_state)
    {
    	// update the state
    	vob_state = VOB_STATE_VOICE_STREAM;

		// start raw data xfer
		BLE_control_raw_data_xfer(true);

		// configure raw data handler
		BLE_set_raw_data_xfer_received_callback(vob_audio_data_reiceived_handler);

	    // kick off the speaker stream
	    vob_kick_off_speaker_output_stream();
	}
	else
	{
		retStatus = HANDLING_FAILED;
	}

	BLE_send_response_to_command(funcCode, retStatus, NULL, TRANSMISSION_VIA_WRITE_CMD);
}

/**
 * @brief Hanlder of the response to stop VOB voice stream command, called on the SRC side
 *
 */
void vob_stop_voice_stream_rsp_handler(BLE_CUSTOM_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
	// stop the MIC voice input stream
    vob_stop_mic_input_stream();

	vob_state = VOB_STATE_CONNECTED;
}

/**
 * @brief Hanlder of the stop VOB voice stream command, called on the DST side
 *
 */
void vob_stop_voice_stream_cmd_handler(uint32_t funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
	BLE_CUSTOM_CMD_RET_STATUS_E retStatus = NO_ERROR;

    if (VOB_STATE_VOICE_STREAM == vob_state)
    {
    	// update the state
    	vob_state = VOB_STATE_CONNECTED;

		// stop raw data xfer
		BLE_control_raw_data_xfer(false);

	    // stop the speaker output stream
        vob_stop_speaker_output_stream();
	}
	else
	{
		retStatus = HANDLING_FAILED;
	}

	BLE_send_response_to_command(funcCode, retStatus, NULL, TRANSMISSION_VIA_WRITE_CMD);
}

static void vob_encoded_data_sent_done(void)
{
	if ((VOB_STATE_VOICE_STREAM == vob_state) && 
			(vob_data_management.encodedDataLength > 0))
	{	
		// inform the ble application thread to continue sending the encoded data if any in the buffer
		osSignalSet(ble_app_tid, BLE_SIGNAL_VOB_DATA_SENT_OUT);
	}
}

/**
 * @brief Hanlder of the BLE connected event
 *
 */
void vob_connected_evt_handler(void)
{
	if (VOB_ROLE_SRC == vob_role)
	{
		l2cap_update_param(VOB_CONNECTION_INTERVAL_IN_MS, 
			VOB_CONNECTION_INTERVAL_IN_MS, VOB_CONNECTION_SUPERVISION_TIMEOUT_IN_MS);
		
		vob_data_management.encodedDataIndexToFetch = 0;
		vob_data_management.encodedDataIndexToFill = 0;
		vob_data_management.encodedDataLength = 0;
		
		// register the BLE tx done callback function 
		app_datapath_server_register_tx_done(vob_encoded_data_sent_done);
	
    	// delete the timer
    	osTimerDelete(vob_connecting_supervisor_timer);
	}

	vob_data_management.isCachingDone = false;
	
    // play the connected sound
    app_voice_report(APP_STATUS_INDICATION_CONNECTED, 0);

    // update the state
    vob_state = VOB_STATE_CONNECTED;
}

/**
 * @brief Hanlder of the BLE disconnected event
 *
 */
void vob_disconnected_evt_handler(void)
{
    ASSERT_SIMPLIFIED((VOB_STATE_VOICE_STREAM == vob_state) || (VOB_STATE_CONNECTED == vob_state));
        
    vob_state = VOB_STATE_DISCONNECTED;

	// stop raw data xfer
	BLE_control_raw_data_xfer(false);

    // play the disconnected sound
    app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, 0);

    // stop the voice stream if it's in progress
    if (VOB_STATE_VOICE_STREAM == vob_state)
    {
        if (VOB_ROLE_SRC == vob_role)
        {
            vob_stop_mic_input_stream();
        }
        else
        {
            vob_stop_speaker_output_stream();
        }
    }

    if (VOB_ROLE_SRC == vob_role)
    {
        // try to re-connect
        vob_start_connecting();
    }
    else
    {
        // re-start advertising
        vob_start_advertising();
    }
}

/**
 * @brief BLE application thread handler
 *
 * @param argument 	Parameter imported during the thread creation.
 *
 */
static void ble_app_thread(void const *argument)
{
	while(1) 
    {
        osEvent evt;
        // wait any signal
        evt = osSignalWait(0x0, osWaitForever);

        // get role from signal value
        if (evt.status == osEventSignal)
        {
            if (evt.value.signals & BLE_SIGNAL_VOB_GET_ENCODED_AUDIO_DATA)
            {
                vob_send_encoded_audio_data();
                break;
            }
            else if (evt.value.signals & BLE_SIGNAL_VOB_DATA_SENT_OUT)
            {
                vob_data_sent_out_handler();
                break;
            }
            else if (evt.value.signals & BLE_SIGNAL_VOB_RECEIVED_DATA)
            {
                vob_data_reiceived_handler();
                break;
            }       
            else if (evt.value.signals & BLE_SIGNAL_VOB_CONNECTED)
            {
                vob_connected_evt_handler();
                break;
            }  
            else if (evt.value.signals & BLE_SIGNAL_VOB_DISCONNECTED)
            {
                vob_disconnected_evt_handler();
                break;
            }  
        }
	}
}

/**
 * @brief The role of the voice over BLE
 *
 * @return 0 if successful, -1 if failed
 *
 */
int ble_app_init(void)
{
	ble_app_tid = osThreadCreate(osThread(ble_app_thread), NULL);
	if (ble_app_tid == NULL)  {
        TRACE(0,"Failed to Create ble_app_thread\n");
		return 0;
	}
	return 0;
}

/**
 * @brief Call back function of the connecting supervisor timer
 *
 * @param n Parameter imported during the timer creation osTimerCreate.
 *
 */
static void connecting_supervisor_timer_cb(void const *n)
{
    if (VOB_STATE_CONNECTING == vob_state)
    {
        // time-out happens at the time of connecting.
        // it means the slave is not present or cannot be connected, so just stop connecting
        appm_stop_connecting();

        vob_state = VOB_STATE_STOPPING_CONNECTING;
    }    

    osTimerDelete(vob_connecting_supervisor_timer);
}

/**
 * @brief Start connecting to DST
 *          this can happen for two cases: 
 *          1. Key press to start connecting
 *          2. Re-connecting when disconnection happened
 *
 */
void vob_start_connecting(void)
{
    // play the connecting sound
    app_voice_report(APP_STATUS_INDICATION_CONNECTING, 0);
    
    // start connecting
    appm_start_connecting(&DST_BLE_BdAddr);

    // create and start a timer to supervise the connecting procedure
    vob_connecting_supervisor_timer = 
        osTimerCreate(osTimer(APP_VOB_CONNECTING_SUPERVISOR_TIMER), osTimerOnce, NULL);

    osTimerStart(vob_connecting_supervisor_timer, VOB_CONNECTING_TIMEOUT_IN_MS);

    vob_state = VOB_STATE_CONNECTING;
}

/**
 * @brief Process the collected PCM data from MIC
 *
 * @param ptrBuf 	Pointer of the PCM data buffer to access.
 * @param length	Length of the PCM data in the buffer in bytes.
 *
 * @return uint32_t 0 means no error happens
 */
static uint32_t vob_voice_data_come(uint8_t* ptrBuf, uint32_t length)
{
    // encode the voice PCM data into cvsd
    uint32_t remainingBytesToEncode = length;
    uint32_t pcmBytesToEncode;
    do
    {
        pcmBytesToEncode = (remainingBytesToEncode > (MAXNUMOFSAMPLES*src_mic_stream_env.bitNumber/AUD_BITS_8))?\
            (MAXNUMOFSAMPLES*src_mic_stream_env.bitNumber/AUD_BITS_8):remainingBytesToEncode;

        // cvsd's compression ratio is 50%
        if ((VOB_PCM_SIZE_TO_AUDIO_SIZE(pcmBytesToEncode) + vob_data_management.encodedDataIndexToFill) > 
            src_mic_stream_env.encodedDataBufSize)
        {
            pcmBytesToEncode = VOB_AUDIO_SIZE_TO_PCM_SIZE(src_mic_stream_env.encodedDataBufSize - 
                vob_data_management.encodedDataIndexToFill);
        }

        // encode the PCM data        
        Pcm8kToCvsd((short *)ptrBuf, vob_data_management.encodedDataBuf + vob_data_management.encodedDataIndexToFill, 
            pcmBytesToEncode / sizeof(short));

        // update the index
        ptrBuf += pcmBytesToEncode;
        remainingBytesToEncode -= pcmBytesToEncode;
        
        vob_data_management.encodedDataIndexToFill += VOB_PCM_SIZE_TO_AUDIO_SIZE(pcmBytesToEncode);
        
        if (src_mic_stream_env.encodedDataBufSize == vob_data_management.encodedDataIndexToFill)
        {
            vob_data_management.encodedDataIndexToFill = 0;
        }
        
    } while (remainingBytesToEncode > 0);

	osMutexWait(vob_env_mutex_id, osWaitForever);

    // update the bytes to transmit via BLE
    vob_data_management.encodedDataLength += VOB_PCM_SIZE_TO_AUDIO_SIZE(length);   

	osMutexRelease(vob_env_mutex_id);

	if ((!vob_data_management.isCachingDone) && 
		(vob_data_management.encodedDataLength >= src_mic_stream_env.cachedEncodedDataSize))
	{
		vob_data_management.isCachingDone = true;
		
		// inform the ble application thread to send out the encoded data
		osSignalSet(ble_app_tid, BLE_SIGNAL_VOB_GET_ENCODED_AUDIO_DATA)
	}
    return 0;
}

/**
 * @brief Init the MIC and Speaker audio streams
 *
 */
static void vob_init_audio_streams(void)
{
	src_mic_stream_env.bitNumber 					= VOB_VOICE_BIT_NUMBER;
	src_mic_stream_env.sampleRate 					= VOB_VOICE_SAMPLE_RATE;
	src_mic_stream_env.channelCount 				= AUD_CHANNEL_NUM_1;
	src_mic_stream_env.morePcmHandler 				= vob_voice_data_come;
	src_mic_stream_env.pcmDataChunkCount 			= VOB_VOICE_PCM_DATA_CHUNK_COUNT;
	src_mic_stream_env.pcmDataChunkSize 			= VOB_VOICE_PCM_DATA_CHUNK_SIZE;
	src_mic_stream_env.cachedEncodedDataSize 		= VOB_ENCODED_DATA_CACHE_SIZE;
	src_mic_stream_env.encodedDataBufSize 			= VOB_ENCODED_DATA_STORAGE_BUF_SIZE;

	dst_speaker_stream_env.bitNumber 				= VOB_VOICE_BIT_NUMBER;
	dst_speaker_stream_env.sampleRate 				= VOB_VOICE_SAMPLE_RATE;
	dst_speaker_stream_env.channelCount 			= AUD_CHANNEL_NUM_2;
	dst_speaker_stream_env.morePcmHandler 			= vob_need_more_voice_data;
	dst_speaker_stream_env.pcmDataChunkCount 		= VOB_VOICE_PCM_DATA_CHUNK_COUNT;
	dst_speaker_stream_env.pcmDataChunkSize 		= VOB_VOICE_PCM_DATA_CHUNK_SIZE;
	dst_speaker_stream_env.cachedEncodedDataSize 	= VOB_ENCODED_DATA_CACHE_SIZE;
	dst_speaker_stream_env.encodedDataBufSize 		= VOB_ENCODED_DATA_STORAGE_BUF_SIZE;
}

/**
 * @brief Start the voice input stream from the MIC
 *
 */
void vob_kick_off_mic_input_stream(void)
{
	// prepare the memory buffer
    app_audio_mempool_init();
	// PCM data buffer
	app_audio_mempool_get_buff(&(src_mic_stream_env.pcmDataBuf),       
		src_mic_stream_env.pcmDataChunkCount*src_mic_stream_env.pcmDataChunkSize);
	
	// encoded data buffer
   	app_audio_mempool_get_buff(&(src_mic_stream_env.encodedDataBuf),   
   		src_mic_stream_env.encodedDataBufSize);

	// encoded data queue
	APP_AUDIO_InitCQueue(&(src_mic_stream_env.queue), src_mic_stream_env.encodedDataBufSize, 
		src_mic_stream_env.encodedDataBuf);	
	
    // acuqire the sufficient system clock
    app_sysfreq_req(APP_SYSFREQ_VOICE_OVER_BLE, APP_SYSFREQ_26M);

    // create the audio flinger stream
    struct AF_STREAM_CONFIG_T stream_cfg;
    
    stream_cfg.bits         = src_mic_stream_env.bitNumber;
	stream_cfg.sample_rate  = src_mic_stream_env.sampleRate;
    stream_cfg.channel_num  = src_mic_stream_env.channelCount;    
    stream_cfg.vol          = VOB_VOICE_VOLUME;

    stream_cfg.device       = AUD_STREAM_USE_OPTIMIZED_STREAM;

    stream_cfg.io_path      = AUD_INPUT_PATH_MAINMIC;
    stream_cfg.handler      = src_mic_stream_env.morePcmHandler;

    stream_cfg.data_ptr     = BT_AUDIO_CACHE_2_UNCACHE(src_mic_stream_env.pcmDataBuf);
    stream_cfg.data_size    = src_mic_stream_env.pcmDataChunkSize;
    
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
}

/**
 * @brief Start the voice stream, this can only be triggered by SRC
 *
 */
static void vob_start_voice_stream(void)
{
	BLE_send_custom_command(OP_VOB_CMD_START_VOICE_STREAM, NULL, 0);
}

/**
 * @brief Callback function called when the BLE activity(adv/connecting/scanning is stopped)
 *
 */
static void vob_ble_activity_stopped(void)
{
	if (VOB_STATE_STOPPING_ADV == vob_state)
	{
		vob_state = VOB_STATE_ADV_STOPPED;

		// should switch to connecting state
		vob_switch_state_handler();
	}
	else if (VOB_STATE_STOPPING_CONNECTING == vob_state)
	{
		// start advertising
		vob_start_advertising();
	}
}

/**
 * @brief The handler of switching VOB state
 *
 */
void vob_switch_state_handler(void)
{
    switch (vob_state)
    {
        case VOB_STATE_ADV_STOPPED:
        {
            if (VOB_ROLE_SRC == vob_role)
            {
                // start connecting
                vob_start_connecting();  
            }
            break;
        }
        case VOB_STATE_CONNECTED:
        {
            // start voice stream
            vob_start_voice_stream();
            break;
        } 
        case VOB_STATE_VOICE_STREAM:
        {
            // stop voice stream
            vob_stop_voice_stream();
            break;            
        }
    }
}

/**
 * @brief Process the message sent to the VOB application thread
 *
 */
static int voice_over_ble_handler(APP_MESSAGE_BODY *msg_body)
{
    switch (msg_body->message_id)
    {
        case VOB_START_AS_SRC:
			// change role
            vob_role = VOB_ROLE_SRC;
            // stop advertising, and start connecting when adv is stopped
            appm_stop_advertising();
			// change state
			vob_state = VOB_STATE_STOPPING_ADV;
            break;
        case VOB_SWITCH_STATE:
	        vob_switch_state_handler();
            break;           
        default:
            break;
    }
    return 0;
}

/**
 * @brief Notify VOB that it should switch to the next state:
 *        VOB_IDLE -> VOB_CONNECTED -> VOB_VOICE_STREAM -> VOB_CONNECTED
 *
 * @param ptrParam 	Pointer of the parameter
 * @param paramLen  Length of the parameter in bytes
 *
 */
void notify_vob(VOB_MESSAGE_ID_E message, uint8_t* ptrParam, uint32_t paramLen)
{
    APP_MESSAGE_BLOCK msg;

    msg.mod_id = APP_MODUAL_VOB;

    msg.msg_body.message_id = message;    

    /**< reserved for future usage */
    // ASSERT(paramLen <= 8, "The parameter length %d exceeds the maximum supported length 8!", paramLen);
    // memcpy((uint8_t *)&(msg.msg_body.message_Param0), ptrParam, paramLen);
    
    app_mailbox_put(&msg);
}

/**
 * @brief Initialize the voice over BLE system
 *
 */
void voice_over_ble_init(void)
{
	// create the mutext
	vob_env_mutex_id = osMutexCreate((osMutex(vob_env_mutex)));

	// initialize the audio streams
	vob_init_audio_streams();
	
    // initialize the state machine
    vob_state = VOB_STATE_IDLE;

    // thread id used for os signal communication between VOB thread and BLE thread
    vob_ThreadId = osThreadGetId();
        
    // add the voice over ble handler into the application thread
    app_set_threadhandle(APP_MODUAL_VOB, voice_over_ble_handler);	
    
    // initialize the cvsd library
    Pcm8k_CvsdInit();
    
    // default role is DST 
    vob_role = VOB_ROLE_DST;

	// register the BLE adv and connecting activity stopped callback function
	app_datapath_server_register_activity_stopped_cb(vob_ble_activity_stopped);
    
    // start advertising if DST
    vob_start_advertising();    
}

#endif // #if __VOICE_OVER_BLE_ENABLED__

