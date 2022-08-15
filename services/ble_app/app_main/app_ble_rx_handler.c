#include "cmsis_os.h"
#include "cmsis.h"
#include "ble_app_dbg.h"
#include "app_ble_rx_handler.h"
#include "cqueue.h"
#include "app_bt_func.h"

#ifdef BISTO_ENABLED
#include "gsound_service.h"
#include "gsound_custom_ble.h"
#endif

#define BLE_RX_EVENT_MAX_MAILBOX    16
#define BLE_RX_BUF_SIZE             (2048)

static uint8_t app_ble_rx_buf[BLE_RX_BUF_SIZE];
static CQueue app_ble_rx_cqueue;

#define VOICEPATH_COMMON_OTA_BUFF_SIZE 4096

uint8_t* app_voicepath_get_common_ota_databuf(void)
{
    static uint8_t voicepath_common_ota_buf[VOICEPATH_COMMON_OTA_BUFF_SIZE];
    return voicepath_common_ota_buf;
}

#ifdef BES_OTA_BASIC
extern void ota_bes_handle_received_data(uint8_t *otaBuf, bool isViaBle,uint16_t dataLenth);

#if defined(IBRT)
#ifdef IBRT_OTA
extern void ota_ibrt_handle_received_data(uint8_t *otaBuf, bool isViaBle, uint16_t len);
#endif
#endif

#endif

#if (BLE_APP_TOTA)
extern void app_tota_handle_received_data(uint8_t* buffer, uint16_t maxBytes);
#endif

static osThreadId app_ble_rx_thread = NULL;
static bool ble_rx_thread_init_done = false;

extern void gsound_send_control_rx_cfm(uint8_t conidx);
extern uint8_t app_ota_get_conidx(void);
extern void app_ota_send_rx_cfm(uint8_t conidx);

static void app_ble_rx_handler_thread(const void *arg);
osThreadDef(app_ble_rx_handler_thread, osPriorityNormal, 1, 2048, "ble_rx");

osMailQDef (ble_rx_event_mailbox, BLE_RX_EVENT_MAX_MAILBOX, BLE_RX_EVENT_T);
static osMailQId app_ble_rx_event_mailbox = NULL;
static int32_t app_ble_rx_event_mailbox_init(void)
{
    app_ble_rx_event_mailbox = osMailCreate(osMailQ(ble_rx_event_mailbox), NULL);
    if (app_ble_rx_event_mailbox == NULL) {
        LOG_I("Failed to Create app_ble_rx_event_mailbox");
        return -1;
    }
    return 0;
}

static void update_init_state(bool state)
{
    ble_rx_thread_init_done = state;
}

static bool get_init_state(void)
{
    return ble_rx_thread_init_done;
}

static void app_ble_rx_mailbox_free(BLE_RX_EVENT_T* rx_event)
{
    osStatus status;

    status = osMailFree(app_ble_rx_event_mailbox, rx_event);
    ASSERT(osOK == status, "Free ble rx event mailbox failed!");
}

void app_ble_rx_handler_init(void)
{
    if (!get_init_state())
    {
        InitCQueue(&app_ble_rx_cqueue, BLE_RX_BUF_SIZE, ( CQItemType * )app_ble_rx_buf);
        app_ble_rx_event_mailbox_init();

        app_ble_rx_thread = osThreadCreate(osThread(app_ble_rx_handler_thread), NULL);
        update_init_state(true);
    }
    else
    {
        LOG_I("rx already initialized");
    }
}

void app_ble_push_rx_data(uint8_t flag, uint8_t conidx, uint8_t* ptr, uint16_t len)
{
    uint32_t lock = int_lock();
    int32_t ret = EnCQueue(&app_ble_rx_cqueue, ptr, len);
    int_unlock(lock);
    ASSERT(CQ_OK == ret, "BLE rx buffer overflow! %d,%d",AvailableOfCQueue(&app_ble_rx_cqueue),len);

    BLE_RX_EVENT_T* event = (BLE_RX_EVENT_T*)osMailAlloc(app_ble_rx_event_mailbox, 0);
    event->flag = flag;
    event->conidx = conidx;
    event->ptr = ptr;
    event->len = len;

    osMailPut(app_ble_rx_event_mailbox, event);
}

static int32_t app_ble_rx_mailbox_get(BLE_RX_EVENT_T** rx_event)
{
    osEvent evt;
    evt = osMailGet(app_ble_rx_event_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *rx_event = (BLE_RX_EVENT_T *)evt.value.p;
        LOG_I("flag %d ptr %p len %d", (*rx_event)->flag,
            (*rx_event)->ptr, (*rx_event)->len);
        return 0;
    }
    return -1;
}

extern int app_ibrt_if_tws_sniff_block(uint32_t block_next_sec);
static void app_ble_rx_handler_thread(void const *argument)
{
    while (true)
    {
        BLE_RX_EVENT_T* rx_event = NULL;
        if (!app_ble_rx_mailbox_get(&rx_event))
        {
            uint8_t tmpData[512];
            uint32_t lock = int_lock();
            DeCQueue(&app_ble_rx_cqueue, tmpData, rx_event->len);
            int_unlock(lock);
            switch (rx_event->flag)
            {
            #ifdef BISTO_ENABLED
                case BLE_RX_DATA_GSOUND_CONTROL:
                #ifdef IBRT
                    app_ibrt_if_tws_sniff_block(5);
                #endif
                    LOG_I("gsound processes %d control data.", rx_event->len);
                    app_gsound_rx_control_data_handler(rx_event->ptr, rx_event->len);
                    break;
            #endif
                #if (BLE_APP_OTA)
                case BLE_RX_DATA_SELF_OTA:
                    #if defined(IBRT)
                    #ifdef IBRT_OTA
                        ota_ibrt_handle_received_data(tmpData, true, rx_event->len);
                        app_ota_send_rx_cfm(app_ota_get_conidx());
                    #endif
                    #else
                        ota_bes_handle_received_data(tmpData, true,rx_event->len);
                    #endif
                    break;
                #endif
                #if (BLE_APP_TOTA)
                case BLE_RX_DATA_SELF_TOTA:
                    app_tota_handle_received_data(tmpData, rx_event->len);
                    break;
                #endif
                default:
                    break;
            }
            app_ble_rx_mailbox_free(rx_event);
        }
    }
}

