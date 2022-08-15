
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s   := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c   := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y :=  $(src_obj)

subdir-ccflags-y += \
    -Iapps/key \
    -Iplatform/hal \
    -Iplatform/drivers/bt \
    -Iservices/ble_stack/ble_ip \
    -Iservices/ble_stack/ke/api \
    -Iservices/ble_stack/hl/api \
    -Iservices/ble_stack/hl/inc \
    -Iservices/ble_stack/common/api \
    -Iservices/ai_voice/manager \
    -Iservices/app_ibrt/inc \
    -Iservices/app_tws/inc \
    -Iservices/ble_app/app_gfps \
    -Iservices/ble_app/app_main \
    -Iservices/ble_app/app_sec \
    -Iservices/bt_if_enhanced/inc \
    -Iservices/bt_app \
    -Iservices/bt_app/a2dp_codecs/include \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/ibrt_core/inc \
    -Iservices/ibrt_ui/inc \
    -Iservices/voicepath \
    -Iservices/voicepath/gsound/gsound_target_api_read_only \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iservices/voicepath/gsound/gsound_target \
    -Ithirdparty/userapi \
    -Iservices/app_ai/inc \
    -Iutils/cqueue \
    -Iservices/ota\
    -Iservices/ibrt_ota/inc \
	-Iservices/ai_voice/protocol/gma/gma_manager
