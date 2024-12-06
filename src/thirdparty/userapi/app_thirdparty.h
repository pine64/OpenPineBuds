#ifndef __APP_THIRDPARTY_H__
#define __APP_THIRDPARTY_H__

#define INVALID_THIRDPARTY_ENTRY_INDEX      0xFFFF
#define APP_THIRDPARTY_DISABLE              0x5A5A

#define ALEXA_WWE_LIB_NAME  alexa_wwe
#define KWS_ALEXA_LIB_NAME  kws_demo
#define CYBERON_LIB_NAME    cyberon
#define KNOWLES_LIB_NAME    knowles
#define NOISE_TRACKER_LIB_NAME nt_demo
#define VOICESPOT_LIB_NAME  kws_engine
#define DEMO_LIB_NAME       demo
#define BIXBY_NAME          bixby


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef enum
{
    THIRDPARTY_FUNC_DEMO,         // Each library has a unique id
    THIRDPARTY_FUNC_NO1,          // KWS
    THIRDPARTY_FUNC_NO2,          // knowles
    THIRDPARTY_FUNC_NO3,          // Noise tracker for anc
    THIRDPARTY_FUNC_KWS,          // key words
    THIRDPARTY_FUNC_NUMBER,
} THIRDPARTY_FUNC_ID;

typedef enum
{
    THIRDPARTY_ID_DEMO,             // Each library has a unique id
    THIRDPARTY_ID_BES_ALEXA,        // key words
    THIRDPARTY_ID_ALEXA_WWE,        // AMA wakeup word engine
    THIRDPARTY_ID_BIXBY,            // Bixby wakeup word engine
    THIRDPARTY_ID_NUMBER,
} THIRDPARTY_LIB_ID;

typedef enum
{
    THIRDPARTY_INIT = 0x00,
    THIRDPARTY_START = 0x01,
    THIRDPARTY_STOP = 0x02,
    THIRDPARTY_START2MIC = 0x03,
    THIRDPARTY_STOP2MIC = 0x04,
    THIRDPARTY_DEINIT = 0x05,
    THIRDPARTY_BT_CONNECTABLE = 0x06,
    THIRDPARTY_BT_DISCOVERABLE = 0x07,
    THIRDPARTY_BT_CONNECTED = 0x08,
    THIRDPARTY_A2DP_STREAMING = 0x09,
    THIRDPARTY_HFP_SETUP = 0x0A,
    THIRDPARTY_MIC_OPEN = 0x0B,
    THIRDPARTY_MIC_CLOSE = 0x0C,
    THIRDPARTY_BURSTING = 0x0D,
    THIRDPARTY_AI_PROVIDE_SPEECH = 0x0E,
    THIRDPARTY_AI_STOP_SPEECH = 0x0F,
    THIRDPARTY_AI_CONNECT = 0x10,
    THIRDPARTY_AI_DISCONNECT = 0x11,
    THIRDPARTY_CALL_START = 0x12,
    THIRDPARTY_CALL_STOP = 0x13,
    THIRDPARTY_OTHER_EVENT = 0x14,
    THIRDPARTY_EVENT_NUMBER,
}THIRDPARTY_EVENT_TYPE;

typedef enum
{
    THIRDPARTY_DATA_COME_CALLBACK = 0x0,
    THIRDPARTY_WAKE_UP_CALLBACK,
    THIRDPARTY_START_SPEECH_CALLBACK,
    THIRDPARTY_STOP_SPEECH_CALLBACK,
}THIRDPARTY_CALLBACK_TYPE;

//the ai wake up type
typedef enum {
    THIRDPARTY_TYPE__NONE,
    THIRDPARTY_TYPE__PRESS_AND_HOLD,
    THIRDPARTY_TYPE__TAP,
    THIRDPARTY_TYPE__KEYWORD_WAKEUP
} THIRDPARTY_WAKE_UP_TYPE_E;

typedef int (*APP_THIRDPARTY_HANDLE_CB_T)(unsigned char, void *param);
typedef uint32_t (*APP_THIRDPARTY_CUSTOM_CB_T)(void*, uint32_t);

typedef struct {
    int      score;
    uint32_t start_index;
    uint32_t end_index;
    THIRDPARTY_WAKE_UP_TYPE_E wake_up_type;
}APP_THIRDPARTY_WAKE_WORD_T;

typedef struct {
    THIRDPARTY_FUNC_ID func_id;
    THIRDPARTY_LIB_ID lib_id;
    THIRDPARTY_EVENT_TYPE event;
}APP_THIRDPARTY_SIGN;

typedef struct {
    APP_THIRDPARTY_SIGN thirdparty_sign;
    APP_THIRDPARTY_HANDLE_CB_T function;
    unsigned char status;
    void *param;
} APP_THIRDPARTY_HANDLE;

typedef struct
{
    // Pointer to the thirdparty handler table
    const APP_THIRDPARTY_HANDLE *thirdparty_handler_tab;
    // Number of thirdparty handler
    uint16_t thirdparty_handler_cnt;
    THIRDPARTY_FUNC_ID thirdparty_func_id;
    THIRDPARTY_LIB_ID thirdparty_lib_id;

    APP_THIRDPARTY_CUSTOM_CB_T _app_thirdparty_data_come_callback;
    APP_THIRDPARTY_CUSTOM_CB_T _app_thirdparty_wake_up_callback;
    APP_THIRDPARTY_CUSTOM_CB_T _app_thirdparty_start_callback;
    APP_THIRDPARTY_CUSTOM_CB_T _app_thirdparty_stop_callback;
}APP_THIRDPARTY_HANDLE_TAB_T;


#define _THIRDPARTY_HANDLER_TAB(id_name)   const APP_THIRDPARTY_HANDLE id_name##_handler_tab[] =
#define _THIRDPARTY_GET_HANDLER_TAB(id_name)   id_name##_handler_tab
#define _THIRDPARTY_HANDLER_TAB_SIZE(id_name)   const uint32_t id_name##_handler_tab_size = ARRAY_SIZE(id_name##_handler_tab);
#define _THIRDPARTY_GET_HANDLER_TAB_SIZE(id_name)  id_name##_handler_tab_size
#define _EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(id_name)     extern const APP_THIRDPARTY_HANDLE id_name##_handler_tab[]; \
                                                            extern const uint32_t id_name##_handler_tab_size;

#define THIRDPARTY_HANDLER_TAB(id_name) _THIRDPARTY_HANDLER_TAB(id_name)
#define THIRDPARTY_GET_HANDLER_TAB(id_name) _THIRDPARTY_GET_HANDLER_TAB(id_name)
#define THIRDPARTY_HANDLER_TAB_SIZE(id_name) _THIRDPARTY_HANDLER_TAB_SIZE(id_name)
#define THIRDPARTY_GET_HANDLER_TAB_SIZE(id_name) _THIRDPARTY_GET_HANDLER_TAB_SIZE(id_name)
#define EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(id_name) _EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(id_name)

EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(ALEXA_WWE_LIB_NAME)
EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(KWS_ALEXA_LIB_NAME)
EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(CYBERON_LIB_NAME)
EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(KNOWLES_LIB_NAME)
EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(NOISE_TRACKER_LIB_NAME)
EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(VOICESPOT_LIB_NAME)
EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(DEMO_LIB_NAME)
EXTERN_THIRDPARTY_HANDLER_TAB_AND_SIZE(BIXBY_NAME)

#ifdef __cplusplus
extern "C" {
#endif

void app_thirdparty_callback_init(THIRDPARTY_CALLBACK_TYPE type, APP_THIRDPARTY_CUSTOM_CB_T cb);
uint32_t app_thirdparty_callback_handble(THIRDPARTY_CALLBACK_TYPE type, void* param1, uint32_t param2);
int app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_ID funcId,THIRDPARTY_EVENT_TYPE event_type);
void app_thirdparty_init(void);


#ifdef __cplusplus
    }
#endif

#endif
