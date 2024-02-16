/**
 ****************************************************************************************
 *
 * @file app_tota_data_handler.c
 *
 * @date 24th April 2018
 *
 * @brief The framework of the tota data handler
 *
 * Copyright (C) 2017
 *
 *
 ****************************************************************************************
 */
#include "app_tota_data_handler.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "apps.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "stdbool.h"
#include "string.h"
//#include "rwapp_config.h"
#include "app_spp_tota.h"
#include "app_spp_tota_general_service.h"
#include "app_tota_conn.h"
#include "crc32.h"
#include "tota_buffer_manager.h"
#include "tota_stream_data_transfer.h"
/**
 * @brief tota data handling environment
 *
 */

typedef struct {
  uint8_t isDataXferInProgress;
  uint8_t isCrcCheckEnabled;
  uint32_t wholeDataXferLen;
  uint32_t dataXferLenUntilLastSegVerification;
  uint32_t currentWholeCrc32;
  uint32_t wholeCrc32UntilLastSeg;
  uint32_t crc32OfCurrentSeg;
} APP_TOTA_DATA_HANDLER_ENV_T;

#define APP_TOTA_TX_BUF_SIZE 2046
static uint8_t app_tota_tmpDataXferBuf[APP_TOTA_TX_BUF_SIZE];

/*static receive_data_callback recervice_data_cb = NULL;
void app_tota_data_received_callback_handler_register(receive_data_callback cb )
{
    recervice_data_cb = cb;
}
__attribute__((weak)) void app_tota_data_received_callback(uint8_t* ptrData,
uint32_t dataLength)
{
    if(recervice_data_cb != NULL){
        recervice_data_cb(ptrData,dataLength);
    }
}
*/

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData 		Pointer of the received data
 * @param dataLength	Length of the received data
 *
 * @return APP_TOTA_CMD_RET_STATUS_E
 */
APP_TOTA_CMD_RET_STATUS_E app_tota_data_received(uint8_t *ptrData,
                                                 uint32_t dataLength) {
  TOTA_LOG_DBG(0, ">>> app_tota_data_received");
  if ((OP_TOTA_STREAM_DATA !=
       (APP_TOTA_CMD_CODE_E)(((uint16_t *)ptrData)[0])) ||
      (dataLength < TOTA_CMD_CODE_SIZE)) {
    return TOTA_INVALID_DATA_PACKET;
  }
  APP_TOTA_TRANSMISSION_PATH_E dataPath = app_tota_get_datapath();

  APP_TOTA_DATA_ACK_T tAck = {0};
  app_tota_send_command(OP_TOTA_SPP_DATA_ACK, (uint8_t *)&tAck, sizeof(tAck),
                        dataPath);
  return TOTA_NO_ERROR;
}

void app_tota_send_data(APP_TOTA_TRANSMISSION_PATH_E path, uint8_t *ptrData,
                        uint32_t dataLength) {
  if (path < APP_TOTA_TRANSMISSION_PATH_COUNT) {
    ((uint16_t *)app_tota_tmpDataXferBuf)[0] = OP_TOTA_STREAM_DATA;
    memcpy(app_tota_tmpDataXferBuf + TOTA_CMD_CODE_SIZE, ptrData, dataLength);

    switch (path) {
    case APP_TOTA_VIA_SPP:
      app_tota_send_data_via_spp(app_tota_tmpDataXferBuf,
                                 dataLength + TOTA_CMD_CODE_SIZE);
      break;
    case APP_TOTA_GEN_VIA_SPP:
      app_tota_gen_send_data_via_spp(app_tota_tmpDataXferBuf,
                                     dataLength + TOTA_CMD_CODE_SIZE);
      break;
    default:
      break;
    }
  }
}

#if defined(APP_ANC_TEST)
void app_anc_tota_send_data(APP_TOTA_TRANSMISSION_PATH_E path, uint8_t *ptrData,
                            uint32_t dataLength) {
  if (path < APP_TOTA_TRANSMISSION_PATH_COUNT) {
    switch (path) {
    case APP_TOTA_VIA_SPP:
      app_tota_send_data_via_spp(ptrData, dataLength);
      break;
    default:
      break;
    }
  }
}
#endif

void app_tota_handle_received_data(uint8_t *buffer, uint16_t maxBytes) {
  TOTA_LOG_DBG(2, "[%s]data receive data length = %d", __func__, maxBytes);
  uint8_t *_buf = buffer;
  uint32_t _bufLen = maxBytes;

  if (OP_TOTA_STREAM_DATA == *(uint16_t *)buffer) {
    TOTA_LOG_DBG(0, "APP TOTA DATA RECEIVED");
    app_tota_data_received(_buf, _bufLen);
  } else {
    if (OP_TOTA_STRING == *(uint16_t *)buffer) {
      TOTA_LOG_DBG(0, "APP TOTA STRING RECEIVED");
    } else {
#if defined(TOTA_ENCODE) && TOTA_ENCODE
      if (is_tota_connected()) {
        TOTA_LOG_DBG(0, "APP TOTA ENCRYPT DATA RECEIVED");
        _buf = tota_decrypt_packet(buffer, maxBytes, &_bufLen);
      } else {
        if (*(uint16_t *)buffer <= OP_TOTA_CONN_CONFIRM) {
          TOTA_LOG_DBG(0, "APP TOTA UNCRYPT CONN COMMAND");
        } else {
          // TODO
          TOTA_LOG_DBG(0, "TOTA UNCONNECT: COMMAND NOT SUPPORT");
          return;
        }
      }
      app_tota_cmd_received(_buf, _bufLen);
#else
      TOTA_LOG_DBG(0, "APP TOTA CMD RECEIVED");
      app_tota_cmd_received(_buf, _bufLen);
#endif
    }
  }
}
