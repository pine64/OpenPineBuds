#include "cmsis_os.h"
#include "hal_trace.h"
#include <stdlib.h>
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_ai_manager_api.h"
#include "app_ai_if_config.h"
#include "app_ai_if_thirdparty.h"
#include "app_ai_if_thirdparty.h"
#include "app_ai_if_custom_ui.h"
#include "app_through_put.h"

#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_thread.h"
#include "app_ai_voice.h"
#endif

static THROUGHPUT_TEST_ENV_T throughputTestEnv;
static THROUGHPUT_TEST_CONFIG_T throughputTestConfig;

static uint32_t app_throughput_test_transmission_handler(void *param1, uint32_t param2);

static APP_THROUGHPUT_CMD_INSTANCE_T *find_throughput_instance_by_code(uint16_t cmdCode)
{
    for (uint32_t index = 0;
        index < APP_THROUGHPUT_CMD_INSTANCE_NUMBER;
        index++)
    {
        if (through_put_table[index].cmdCode == cmdCode)
        {
            return &through_put_table[index];
        }
    }

    return NULL;
}

bool app_throughput_send_command(APP_THROUGHPUT_CMD_CODE_E cmdCode, 
    uint8_t *ptrParam, uint32_t paramLen)
{
    APP_THROUGHPUT_CMD_PAYLOAD_T payload;
    uint16_t cmd_len = 0;

    if (THROUGHPUT_DATA_MAX_SIZE < paramLen)
    {
        TRACE(1,"%s error ", __func__);
        return false;
    }

    payload.cmdCode = cmdCode;
    payload.paramLen = paramLen;
    memcpy(payload.param, ptrParam, paramLen);

    cmd_len = (uint32_t)THROUGHPUT_CMD_PAYLOAD_HEADER_LEN + payload.paramLen;
    app_ai_if_custom_ui_send_cmd((uint8_t *)&payload, cmd_len);

    return true;
}

uint32_t app_throughput_cmd_send_done(void *param1, uint32_t param2)
{
    if (throughputTestEnv.isThroughputTestOn && \
            ((app_ai_get_transport_type() == AI_TRANSPORT_BLE) ||\
            (WITHOUT_RSP == throughputTestConfig.responseModel)))
    {
        app_throughput_test_transmission_handler(NULL, 0);
    }

    return 0;
}

uint32_t app_throughput_receive_cmd(void *param1, uint32_t param2)
{
    uint8_t *ptrbuf = NULL;
    uint32_t data_length = 0;
    APP_THROUGHPUT_CMD_PAYLOAD_T* pPayload = NULL;

    TRACE(2,"%s data len %d", __func__, param2);
    //DUMP8("0x%02x ", param1, param2);

    data_length = param2;
    while(data_length) {
        ptrbuf = (uint8_t *)((uint32_t)param1 + param2 - data_length);

        pPayload = (APP_THROUGHPUT_CMD_PAYLOAD_T *)ptrbuf;
        data_length -= THROUGHPUT_CMD_PAYLOAD_HEADER_LEN;
        if(data_length < pPayload->paramLen) {
            TRACE(3,"%s error data_length %d paramLen %d", __func__, data_length, pPayload->paramLen);
            return 2;
        }
        TRACE(3,"%s data_length %d  paramLen %d", __func__, data_length, pPayload->paramLen);
        data_length -= pPayload->paramLen;

        // check parameter length
        if (pPayload->paramLen > sizeof(pPayload->param)) {
            TRACE(0,"SV COMMAND PARAM ERROR LENGTH");
            return 4;
        }

        APP_THROUGHPUT_CMD_INSTANCE_T *pInstance = find_throughput_instance_by_code(pPayload->cmdCode);

        // execute the command handler
        if (pInstance)
        {
            pInstance->cmdHandler((APP_THROUGHPUT_CMD_CODE_E)(pPayload->cmdCode), pPayload->param, pPayload->paramLen);
        }
    }
    
    return 0;
}

#define APP_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS    2000
static void app_throughput_pre_config_pending_timer_cb(void const *n);
osTimerDef (APP_THROUGHPUT_PRE_CONFIG_PENDING_TIMER, app_throughput_pre_config_pending_timer_cb);
osTimerId   app_throughput_pre_config_pending_timer_id = NULL;

static void app_throughput_test_data_xfer_lasting_timer_cb(void const *n);
osTimerDef (APP_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER, app_throughput_test_data_xfer_lasting_timer_cb);
osTimerId   app_throughput_test_data_xfer_lasting_timer_id = NULL;


static uint8_t app_throughput_datapattern[THROUGHPUT_DATA_MAX_SIZE];

static void app_throughput_test_data_xfer_lasting_timer_cb(void const *n)
{
    app_throughput_send_command(THROUGHPUT_OP_THROUGHPUT_TEST_DONE, NULL, 0);

    app_stop_throughput_test();
}

static void app_throughput_pre_config_pending_timer_cb(void const *n)
{
    // inform the configuration
    app_throughput_send_command(THROUGHPUT_OP_INFORM_THROUGHPUT_TEST_CONFIG,
        (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig));

    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_throughput_test_transmission_handler(NULL, 0);
    #ifndef SLAVE_ADV_BLE
        osTimerStart(app_throughput_test_data_xfer_lasting_timer_id,
            throughputTestConfig.lastTimeInSecond*1000);
    #endif
    }
}

void app_throughput_test_init(void)
{
    memset(&throughputTestEnv, 0x00, sizeof(throughputTestEnv));
    memset(&throughputTestConfig, 0x00, sizeof(throughputTestConfig));

    app_throughput_pre_config_pending_timer_id =
        osTimerCreate(osTimer(APP_THROUGHPUT_PRE_CONFIG_PENDING_TIMER),
        osTimerOnce, NULL);

    app_throughput_test_data_xfer_lasting_timer_id =
        osTimerCreate(osTimer(APP_THROUGHPUT_TEST_DATA_XFER_LASTING_TIMER),
        osTimerOnce, NULL);
}

static uint32_t app_throughput_test_transmission_handler(void *param1, uint32_t param2)
{
    if (UP_STREAM == throughputTestConfig.direction)
    {
        app_throughput_send_command(THROUGHPUT_OP_THROUGHPUT_TEST_DATA,
            app_throughput_datapattern, throughputTestConfig.dataPacketSize - 4);
    }

    return 0;
}

static void app_throughput_test_data_handler(APP_THROUGHPUT_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    if ((WITH_RSP == throughputTestConfig.responseModel) &&
        (AI_TRANSPORT_SPP == app_ai_get_transport_type()))
    {
        app_throughput_send_command(THROUGHPUT_OP_THROUGHPUT_TEST_DATA_ACK, NULL, 0);
    }
}

static void app_throughput_test_data_ack_handler(APP_THROUGHPUT_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    if (throughputTestEnv.isThroughputTestOn &&
        (WITH_RSP == throughputTestConfig.responseModel))
    {
        app_throughput_test_transmission_handler(NULL, 0);
    }
}

static void app_throughput_test_done_signal_handler(APP_THROUGHPUT_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    app_stop_throughput_test();
}

void app_stop_throughput_test(void)
{
    throughputTestEnv.isThroughputTestOn = false;
    osTimerStop(app_throughput_pre_config_pending_timer_id);
    osTimerStop(app_throughput_test_data_xfer_lasting_timer_id);
}

static void app_throughput_test_config_handler(APP_THROUGHPUT_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    throughputTestConfig = *(THROUGHPUT_TEST_CONFIG_T *)ptrParam;

    TRACE(4,"%s patter %d time %d response %d", __func__, \
            throughputTestConfig.dataPattern, \
            throughputTestConfig.lastTimeInSecond, \
            throughputTestConfig.responseModel);

    // generate the data pattern
    switch (throughputTestConfig.dataPattern)
    {
        case PATTERN_RANDOM:
        {
            for (uint32_t index = 0;index < THROUGHPUT_DATA_MAX_SIZE;index++)
            {
                app_throughput_datapattern[index] = (uint8_t)rand();
            }
            break;
        }
        case PATTERN_11110000:
        {
            memset(app_throughput_datapattern, 0xF0, THROUGHPUT_DATA_MAX_SIZE);
            break;
        }
        case PATTERN_10101010:
        {
            memset(app_throughput_datapattern, 0xAA, THROUGHPUT_DATA_MAX_SIZE);
            break;
        }
        case PATTERN_ALL_1:
        {
            memset(app_throughput_datapattern, 0xFF, THROUGHPUT_DATA_MAX_SIZE);
            break;
        }
        case PATTERN_ALL_0:
        {
            memset(app_throughput_datapattern, 0, THROUGHPUT_DATA_MAX_SIZE);
            break;
        }
        case PATTERN_00001111:
        {
            memset(app_throughput_datapattern, 0x0F, THROUGHPUT_DATA_MAX_SIZE);
            break;
        }
        case PATTERN_0101:
        {
            memset(app_throughput_datapattern, 0x55, THROUGHPUT_DATA_MAX_SIZE);
            break;
        }
        default:
            throughputTestConfig.dataPattern = 0;
            break;
    }

    throughputTestEnv.isThroughputTestOn = true;
    throughputTestEnv.conidx = app_ai_if_get_ble_connection_index();
    TRACE(2, "conidx 0x%x useSpecificConnParameter %d", throughputTestEnv.conidx, throughputTestConfig.isToUseSpecificConnParameter);

    // check whether need to use the new conn parameter
    if (AI_TRANSPORT_BLE == app_ai_get_transport_type()
        && throughputTestConfig.isToUseSpecificConnParameter)
    {
        if (throughputTestEnv.conidx != 0xFF)
        {
            l2cap_update_param(throughputTestEnv.conidx, throughputTestConfig.minConnIntervalInMs,
                throughputTestConfig.maxConnIntervalInMs,
                THROUGHPUT_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS,
                THROUGHPUT_BLE_CONNECTION_SLAVELATENCY);
        }

        osTimerStart(app_throughput_pre_config_pending_timer_id,
            APP_THROUGHPUT_PRE_CONFIG_PENDING_TIME_IN_MS);
    }
    else
    {
        app_throughput_send_command(THROUGHPUT_OP_INFORM_THROUGHPUT_TEST_CONFIG,
            (uint8_t *)&throughputTestConfig, sizeof(throughputTestConfig));

        if (UP_STREAM == throughputTestConfig.direction)
        {
            app_throughput_test_transmission_handler(NULL, 0);
            osTimerStart(app_throughput_test_data_xfer_lasting_timer_id,
                throughputTestConfig.lastTimeInSecond*1000);
        }
    }
}


APP_THROUGHPUT_CMD_INSTANCE_T through_put_table[APP_THROUGHPUT_CMD_INSTANCE_NUMBER] =
{
    {THROUGHPUT_OP_INFORM_THROUGHPUT_TEST_CONFIG, app_throughput_test_config_handler},
    {THROUGHPUT_OP_THROUGHPUT_TEST_DATA, app_throughput_test_data_handler},
    {THROUGHPUT_OP_THROUGHPUT_TEST_DATA_ACK, app_throughput_test_data_ack_handler},
    {THROUGHPUT_OP_THROUGHPUT_TEST_DONE, app_throughput_test_done_signal_handler}
};

