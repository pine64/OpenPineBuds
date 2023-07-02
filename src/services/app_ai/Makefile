cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)/src/*.cpp))

ifeq ($(BISTO_ENABLE),0)
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)voice_sbc/*.cpp))
endif

ifeq ($(ANC_APP),1)
CFLAGS_app_ai_if.o += -DANC_APP
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += -Iapps/audioplayers \
                    -Iapps/common \
                    -Iapps/key \
                    -Iapps/main \
                    -Iplatform/drivers/bt \
                    -Iplatform/hal \
                    -Iservices/ai_voice/audio \
                    -Iservices/ai_voice/transport \
                    -Iservices/ai_voice/manager \
                    -Iservices/ai_voice/protocol/ama/ama_stream \
                    -Iservices/audio_dump/include \
                    -Iservices/app_ai/inc \
                    -Iservices/app_ibrt/inc \
                    -Iservices/app_tws/inc \
                    -Iservices/ble_app/app_main \
                    -Iservices/ble_stack/hl/api \
                    -Iservices/ble_stack/ble_ip \
                    $(BT_IF_INCLUDES) \
                    -Iservices/bt_app \
                    -Iservices/bt_app/a2dp_codecs/include \
                    -Iservices/multimedia/audio/codec/sbc/inc \
                    -Iservices/multimedia/speech/inc \
                    -Iservices/norflash_api \
                    -Iservices/nvrecord \
                    -Iservices/ibrt_core/inc \
                    -Iservices/ibrt_ui/inc \
                    -Iservices/ble_stack/common/api \
                    -Iservices/ble_stack/hl/inc \
                    -Iservices/ble_stack/ke/api \
                    -Iservices/cp_accel \
                    -Iservices/resources \
                    -Iservices/through_put \
                    -Iservices/voicepath \
                    -Iservices/voicepath/gsound/gsound_custom/inc \
                    -Iservices/voicepath/gsound/gsound_target \
                    -Iservices/voicepath/gsound/gsound_target_api_read_only \
                    -Ithirdparty/userapi \
                    -Iutils/cqueue \
                    -Iutils/heap \
                    -Iapps/anc/inc
