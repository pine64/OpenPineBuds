cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*/*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))
obj_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*/*.cpp))

A2DPCODECS_LIB_NAME := lib_a2dpcodecs

obj-y += $(A2DPCODECS_LIB_NAME).a

$(A2DPCODECS_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

CFLAGS_a2dp_codec_ldac.o += -Ithirdparty/audio_codec_lib/ldac/inc

subdir-ccflags-y += \
	-Iservices/audio_process \
	-Iservices/hw_dsp/inc \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/fs/fat/ChaN \
	$(BT_IF_INCLUDES) \
	-Iservices/multimedia/speech/inc \
	-Iservices/bone_sensor/lis25ba \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iservices/multimedia/rbcodec \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/multimedia/audio/process/filters/include \
	-Iservices/multimedia/audio/process/drc/include \
	-Iservices/multimedia/audio/process/anc/include\
	-Iservices/nv_section/aud_section \
	-Iservices/nv_section/userdata_section \
  	-Iservices/nv_section/include   \
	-Iservices/voicepath/$(VOICE_DATAPATH_TYPE) \
	-Iservices/voicepath/gsound \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/cmsis \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iutils/heap \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iapps/anc/inc \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/audioplayers/a2dp_decoder \
	-Iapps/battery \
	-Iapps/common \
	-Iapps/factory \
	-Iservices/app_ibrt/inc \
	-Iservices/ble_app \
	-Iutils/hwtimer_list \
	-Iservices/ibrt_ui/inc \
	-Iservices/ibrt_core/inc \
	-Iservices/ble_stack/ble_ip \
	-Iservices/ble_stack/hl/api \
	-Iservices/ble_stack/app/api/  \
	-Iservices/ble_stack/common/api/ \
	-Iservices/ble_stack/hl/inc/ \
	-Iservices/ble_stack/ke/api \
	-Iservices/voicepath \
	-Ithirdparty/userapi \
	-Iservices/ble_app/app_gfps \
	-Iservices/ble_app/app_main \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
	-Iservices/ai_voice/manager \
	-Iservices/ai_voice/audio \
	-Iservices/ai_voice/transport \
	-Iservices/app_ai/inc \
	-Iservices/interconnection/red \
	-Iservices/interconnection/green \
	-Iservices/interconnection/umm_malloc \
	-Iservices/bt_app \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/bt_app/a2dp_codecs/include
