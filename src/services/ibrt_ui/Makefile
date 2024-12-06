cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

IBRT_LIB_NAME := libtws_ibrt

ifeq ($(ENHANCED_STACK),1)
IBRT_LIB_NAME := $(IBRT_LIB_NAME)_enhanced_stack
endif

ifeq ($(BLE), 1)
IBRT_LIB_NAME := $(IBRT_LIB_NAME)_ble
endif

ifeq ($(KERNEL),RTX)
IBRT_LIB_NAME := $(IBRT_LIB_NAME)_RTX
endif
ifeq ($(KERNEL),RTX5)
IBRT_LIB_NAME := $(IBRT_LIB_NAME)_RTX5
endif

$(IBRT_LIB_NAME)-y := $(src_obj)

obj-y :=  $(IBRT_LIB_NAME).a

subdir-ccflags-y += \
    -Iservices/fs/fat \
    -Iservices/fs/sd \
    -Iservices/fs/fat/ChaN \
    -Iservices/overlay \
    -Iservices/nvrecord \
    -Iservices/resources \
    -Iservices/multimedia/audio/process/resample/include \
    $(BT_IF_INCLUDES) \
    -Iplatform/drivers/uarthci \
    -Iplatform/drivers/ana \
    -Iplatform/drivers/bt \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iservices/audioflinger \
    -Iutils/lockcqueue \
    -Iutils/intersyshci \
    -Iapps/key \
    -Iapps/main \
    -Iapps/battery \
    -Iapps/common \
    -Iapps/audioplayers \
    -Iapps/audioplayers/a2dp_decoder \
    -Iapps/factory \
    -Iservices/ai_voice/manager \
    -Iservices/ble_stack/ble_ip \
    -Iservices/ble_stack/common/api \
    -Iservices/ble_stack/hl/api \
    -Iservices/ble_stack/hl/inc \
    -Iservices/ble_stack/ke/api \
    -Iservices/ble_app \
    -Iservices/ble_app/app_datapath \
    -Iservices/ble_app/app_tws \
    -Iservices/ble_app/app_main \
    $(BT_PROFILES_INCLUDES) \
    -Iservices/osif \
    -Iservices/nv_section/factory_section \
    -Iservices/nv_section/log_section \
    -Iservices/app_ibrt/inc \
    -Iservices/bt_app/a2dp_codecs/include \
    -Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/multimedia/audio/codec/sbc/src/inc \
    -Iutils/hwtimer_list \
    -Iservices/voicepath \
    -Iservices/app_tws/inc \
    -Iservices/app_ai/inc \
    -Iservices/ai_voice/protocol/gma/gma_crypto \
    -Iservices/ibrt_ota/inc

ifeq ($(IBRT),1)
subdir-ccflags-y += \
    -Iservices/ibrt_core/inc \
    -Iservices/ibrt_ui/inc \
    -Iservices/bt_app
endif

