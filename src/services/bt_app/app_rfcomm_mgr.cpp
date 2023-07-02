#include "app_rfcomm_mgr.h"
#include "string.h"
extern "C" {
#include "app_bt.h"
#include "bt_if.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include "hal_trace.h"
#include "os_api.h"
#include "spp_api.h"
}

typedef struct {
  uint8_t instanceIndex;

  struct spp_device *sppDev;

#if SPP_SERVER == XA_ENABLED
  struct spp_service *sppService;
  btif_sdp_record_t *sppSdpRecord;
#endif

  rfcomm_callback_func callback;
  osMutexId service_mutex_id;
} RfcommService_t;

#define RFCOMM_SERVER_MAX_CHANNEL_CNT 4

static RfcommService_t RfcommServiceInstance[RFCOMM_SERVER_MAX_CHANNEL_CNT];
static uint8_t rfcomm_service_used_instance_cnt = 0;
#define RFCOMM_RECV_BUFFER_SIZE 2048
#define RFCOMM_RECV_BUFFER_POOL_SIZE                                           \
  (RFCOMM_SERVER_MAX_CHANNEL_CNT * RFCOMM_RECV_BUFFER_SIZE)

static uint8_t rfcomm_rx_buf[RFCOMM_RECV_BUFFER_POOL_SIZE];
static uint32_t rfcomm_rx_buf_allocated_offset = 0;

/****************************************************************************
 * SPP SDP Entries
 ****************************************************************************/
static const uint8_t RFCOMM_NULL_UUID_128[16] = {
    0x00,
};

static const U8 rfcommClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),            /* Data Element Sequence, 17 bytes */
    SDP_UUID_128BIT(RFCOMM_NULL_UUID_128), /* 128 bit UUID in Big Endian */
};

static uint8_t rfcommClassIdArray[RFCOMM_SERVER_MAX_CHANNEL_CNT *
                                  sizeof(rfcommClassId)] = {

};

static const U8 RfcommProtoDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(12), /* Data element sequence, 12 bytes */

    /* Each element of the list is a Protocol descriptor which is a
     * data element sequence. The first element is L2CAP which only
     * has a UUID element.
     */
    SDP_ATTRIB_HEADER_8BIT(3), /* Data element sequence for L2CAP, 3
                                * bytes
                                */

    SDP_UUID_16BIT(PROT_L2CAP), /* Uuid16 L2CAP */

    /* Next protocol descriptor in the list is RFCOMM. It contains two
     * elements which are the UUID and the channel. Ultimately this
     * channel will need to filled in with value returned by RFCOMM.
     */

    /* Data element sequence for RFCOMM, 5 bytes */
    SDP_ATTRIB_HEADER_8BIT(5),

    SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

    /* Uint8 RFCOMM channel number - value can vary */
    SDP_UINT_8BIT(0)};

static uint8_t RfcommProtoDescListArray[RFCOMM_SERVER_MAX_CHANNEL_CNT *
                                        sizeof(RfcommProtoDescList)] = {

};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 ProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8), /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT), /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)          /* As per errata 2239 */
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t rfcommAttributes[] = {
    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, rfcommClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, RfcommProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, ProfileDescList),
};

static sdp_attribute_t rfcommAttributesArray[RFCOMM_SERVER_MAX_CHANNEL_CNT]
                                            [ARRAY_SIZE(rfcommAttributes)] = {

};

osThreadId app_rfcomm_get_rx_thread_id(uint8_t serviceIndex) {
  if (serviceIndex < rfcomm_service_used_instance_cnt) {
    return RfcommServiceInstance[serviceIndex].sppDev->reader_thread_id;
  } else {
    ASSERT(false, "Wrong rfcomm service index %d", serviceIndex);
  }
}

uint8_t app_rfcomm_get_instance_index(void *dev) {
  for (uint8_t index = 0; index < rfcomm_service_used_instance_cnt; index++) {
    if ((struct spp_device *)dev == (RfcommServiceInstance[index].sppDev)) {
      return index;
    }
  }

  ASSERT(false, "invalid rfcomm callback triggered!");
  return 0;
}

static void app_rfcomm_org_callback(struct spp_device *locDev,
                                    struct spp_callback_parms *info) {
  uint8_t index = app_rfcomm_get_instance_index(locDev);
  RfcommService_t *pService = &(RfcommServiceInstance[index]);
  RFCOMM_EVENT_E event;
  uint16_t sentDataLen = 0;
  uint8_t *sentDataBuf = NULL;

  if (BTIF_SPP_EVENT_REMDEV_CONNECTED_IND == info->event) {
    event = RFCOMM_INCOMING_CONN_REQ;
  } else if (BTIF_SPP_EVENT_REMDEV_CONNECTED == info->event) {
    event = RFCOMM_CONNECTED;
  } else if (BTIF_SPP_EVENT_REMDEV_DISCONNECTED == info->event) {
    event = RFCOMM_DISCONNECTED;
  } else if (BTIF_SPP_EVENT_DATA_SENT == info->event) {
    struct spp_tx_done *pTxDone = (struct spp_tx_done *)(info->p.other);
    sentDataBuf = pTxDone->tx_buf;
    sentDataLen = pTxDone->tx_data_length;
    event = RFCOMM_TX_DONE;
  } else {
    event = RFCOMM_UNKNOWN_EVENT;
  }

  uint8_t *pBtAddr = NULL;
  if (info->p.remDev) {
    pBtAddr = btif_me_get_remote_device_bdaddr(info->p.remDev)->address;

    if (pService->callback) {
      bool ret = pService->callback(
          event, index, btif_me_get_remote_device_hci_handle(info->p.remDev),
          pBtAddr, sentDataBuf, sentDataLen);

      if (!ret) {
        if (RFCOMM_INCOMING_CONN_REQ == event) {
          // reject the incoming connection request
          info->status = BT_STS_CANCELLED;
        }
      }
    }
  }
}

osMutexId app_rfcomm_service_get_mutex(uint8_t instanceIndex) {
  return RfcommServiceInstance[instanceIndex].service_mutex_id;
}

static bt_status_t app_rfcomm_service_init(
    RfcommService_t *service, sdp_attribute_t *sdpAttributes,
    int attributeCount, int serviceId, uint32_t app_id,
    spp_handle_data_event_func_t dataRecCallback, int txPacketCount,
    osMutexId mutexId, osMutexId creditMutexId) {
  service->sppService = btif_create_spp_service();
  service->sppDev = btif_create_spp_device();

  service->sppDev->portType = BTIF_SPP_SERVER_PORT;
  uint8_t *ptrRxBuf = &rfcomm_rx_buf[rfcomm_rx_buf_allocated_offset];
  uint32_t rxBufSize = RFCOMM_RECV_BUFFER_SIZE;

  rfcomm_rx_buf_allocated_offset += rxBufSize;

  ASSERT(rfcomm_rx_buf_allocated_offset <= RFCOMM_RECV_BUFFER_POOL_SIZE,
         "RFComm rx buffer overflow! Need %d actual %d",
         rfcomm_rx_buf_allocated_offset, RFCOMM_RECV_BUFFER_POOL_SIZE);

  btif_spp_init_rx_buf(service->sppDev, ptrRxBuf, rxBufSize);

  service->sppSdpRecord = btif_sdp_create_record();

  btif_sdp_record_param_t param;
  param.attrs = sdpAttributes, param.attr_count = attributeCount;
  param.COD = BTIF_COD_MAJOR_PERIPHERAL;
  btif_sdp_record_setup(service->sppSdpRecord, &param);

  service->sppService->rf_service.serviceId = serviceId;
  service->sppService->numPorts = 0;
  service->sppDev->app_id = app_id;
  service->sppDev->spp_handle_data_event_func = dataRecCallback;
  // TODO: add more in other files
  service->sppDev->creditMutex = creditMutexId;

  btif_spp_service_setup(service->sppDev, service->sppService,
                         service->sppSdpRecord);
  btif_spp_init_device(service->sppDev, txPacketCount, mutexId);

  return btif_spp_open(service->sppDev, NULL, app_rfcomm_org_callback);
}

void app_rfcomm_read(uint8_t instanceIndex, uint8_t *pBuf, uint16_t *ptrLen) {
  if (instanceIndex >= rfcomm_service_used_instance_cnt) {
    return;
  }

  uint16_t lenToRead = *ptrLen;

  btif_spp_read(RfcommServiceInstance[instanceIndex].sppDev, (char *)pBuf,
                &lenToRead);
  *ptrLen = lenToRead;
}

int8_t app_rfcomm_write(uint8_t instanceIndex, const uint8_t *pBuf,
                        uint16_t length) {
  if (instanceIndex >= rfcomm_service_used_instance_cnt) {
    return -1;
  }

  uint16_t len = (uint16_t)length;
  if (BT_STS_SUCCESS !=
      btif_spp_write(RfcommServiceInstance[instanceIndex].sppDev, (char *)pBuf,
                     &len)) {
    return -1;
  } else {
    return 0;
  }
}

bool app_rfcomm_removesdpservice(uint8_t instanceIndex) {
  if (instanceIndex >= rfcomm_service_used_instance_cnt) {
    return false;
  }

  return (BT_STS_SUCCESS ==
          btif_sdp_remove_record(
              &(RfcommServiceInstance[instanceIndex].sppSdpRecord)));
}

bool app_rfcomm_addsdpservice(uint8_t instanceIndex) {
  if (instanceIndex >= rfcomm_service_used_instance_cnt) {
    return false;
  }

  return (BT_STS_SUCCESS ==
          btif_sdp_add_record(
              &(RfcommServiceInstance[instanceIndex].sppSdpRecord)));
}

void app_rfcomm_close(uint8_t instanceIndex) {
  if (instanceIndex >= rfcomm_service_used_instance_cnt) {
    return;
  }
  btif_spp_close(RfcommServiceInstance[instanceIndex].sppDev);
}

int8_t app_rfcomm_open(RFCOMM_CONFIG_T *ptConfig) {
  if (rfcomm_service_used_instance_cnt >= RFCOMM_SERVER_MAX_CHANNEL_CNT) {
    return -1;
  }

  RfcommService_t *sppServiceInstance =
      &RfcommServiceInstance[rfcomm_service_used_instance_cnt];

  sppServiceInstance->callback = ptConfig->callback;

  memcpy(&rfcommClassIdArray[rfcomm_service_used_instance_cnt *
                             sizeof(rfcommClassId)],
         rfcommClassId, sizeof(rfcommClassId));
  for (int32_t index = 0; index < 16; index++) {
    rfcommClassIdArray[rfcomm_service_used_instance_cnt *
                           sizeof(rfcommClassId) +
                       3 + index] = ptConfig->rfcomm_128bit_uuid[15 - index];
  }

  memcpy(&RfcommProtoDescListArray[rfcomm_service_used_instance_cnt *
                                   sizeof(RfcommProtoDescList)],
         RfcommProtoDescList, sizeof(RfcommProtoDescList));
  memcpy(&RfcommProtoDescListArray[rfcomm_service_used_instance_cnt *
                                       sizeof(RfcommProtoDescList) +
                                   13],
         &(ptConfig->rfcomm_ch), 1);

  memcpy(&rfcommAttributesArray[rfcomm_service_used_instance_cnt],
         rfcommAttributes, sizeof(rfcommAttributes));

  rfcommAttributesArray[rfcomm_service_used_instance_cnt][0].value =
      &rfcommClassIdArray[rfcomm_service_used_instance_cnt *
                          sizeof(rfcommClassId)];
  rfcommAttributesArray[rfcomm_service_used_instance_cnt][1].value =
      &RfcommProtoDescListArray[rfcomm_service_used_instance_cnt *
                                sizeof(RfcommProtoDescList)];

  sppServiceInstance->instanceIndex = rfcomm_service_used_instance_cnt;

  bt_status_t status = app_rfcomm_service_init(
      sppServiceInstance,
      rfcommAttributesArray[rfcomm_service_used_instance_cnt],
      ARRAY_SIZE(rfcommAttributes), ptConfig->rfcomm_ch, ptConfig->app_id,
      ptConfig->spp_handle_data_event_func, ptConfig->tx_pkt_cnt,
      ptConfig->mutexId, ptConfig->creditMutexId);

  if (status != BT_STS_SUCCESS) {
    return -1;
  } else {
    return rfcomm_service_used_instance_cnt++;
  }
}

void app_rfcomm_mgr_init(void) {
  memset(&RfcommServiceInstance, 0, sizeof(RfcommServiceInstance));
  rfcomm_service_used_instance_cnt = 0;
}

// Add services to sdp database, currently invoked on connection
//
void app_rfcomm_services_add_sdp(void) {}

// Remove services from sdp database, currently invoked on disconnection
//
void app_rfcomm_services_remove_sdp(void) {}
