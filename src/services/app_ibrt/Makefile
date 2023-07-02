
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

IBRT_LIB_NAME := libtws_ibrt_ui

$(IBRT_LIB_NAME)-y := $(src_obj)

obj-y :=  $(IBRT_LIB_NAME).a

subdir-ccflags-y += \
	-Iservices/osif \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/fs/fat/ChaN \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iservices/multimedia/audio/process/resample/include \
	$(BT_IF_INCLUDES) \
	-Iservices/nv_section/factory_section \
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
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/audioplayers/a2dp_decoder \
	-Iapps/anc/inc \
	-Iapps/factory \
	-Iservices/ble_app \
	-Iservices/ble_app \
	-Iservices/interconnection/green \
	-Iutils/hwtimer_list \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
	$(BT_PROFILES_INCLUDES) \
	-Iservices/ble_stack/common/api \
	-Iservices/ble_app/app_tws \
	-Iservices/nv_section/log_section \
	-Iservices/ai_voice/manager \
	-Iservices/app_ai/inc \
	-Iapps/battery/ \
	-Iutils/crc16 \
	-Iutils/crc32 \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/bt_app/a2dp_codecs/include \
	-Iservices/voicepath/gsound/gsound_target \
	-Iservices/ble_app/app_main \
	-Iservices/app_tws/inc \
	-Iservices/ai_voice/protocol/gma/gma_manager \
	-Iservices/ai_voice/protocol/gma/gma_crypto  \
	-Iservices/ai_voice/protocol/smartvoice/smartvoice_ble\
	-Iservices/ota \
	-Iservices/tota \
	-Iservices/ble_stack/ble_ip \
	-Iservices/ble_stack/hl/api \
	-Iservices/ble_stack/app/api/ \
	-Iservices/ble_stack/common/api/ \
	-Iservices/ble_stack/hl/inc/ \
	-Iservices/ble_stack/ke/api \
	-Iservices/norflash_api \
	-Iservices/ai_voice/transport  \
	-Iservices/ai_voice/manager	\
	-Iservices/ai_voice/protocol/dual_mic_recording/voice_manager \
	-Iapps/anc/inc 

	
ifeq ($(IBRT),1)
subdir-ccflags-y += \
	-Iservices/ibrt_core/inc \
	-Iservices/ibrt_ui/inc \
	-Iservices/app_ibrt/inc \
	-Iservices/bt_app
endif

ifeq ($(OTA_ENABLE),1)
subdir-ccflags-y += \
    -Iservices/ibrt_ota/inc
endif

ifeq ($(ANC_APP),1)
CFLAGS_app_ibrt_ui_test.o += -DANC_APP
endif

ifeq ($(VOICE_DATAPATH_ENABLED),1)
subdir-ccflags-y += \
    -Iservices/voicepath/gsound/gsound_custom/inc \
	-Iservices/voicepath/gsound/gsound_target_api_read_only
endif