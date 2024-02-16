
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

ifeq ($(ANC_APP),1)
CFLAGS_app_anc.o += -DANC_APP
endif

ifeq ($(ANC_FF_ENABLED),1)
CFLAGS_app_anc.o += -DANC_FF_ENABLED
endif

ifeq ($(ANC_FB_ENABLED),1)
CFLAGS_app_anc.o += -DANC_FB_ENABLED
endif

ifeq ($(ANC_WNR_ENABLED),1)
CFLAGS_app_anc.o += -DANC_WNR_ENABLED
endif



ifeq ($(ANC_ASSIST_ENABLED),1)
CFLAGS_app_anc.o += -DANC_ASSIST_ENABLED

ifeq ($(ANC_ASSIST_WNR_ENABLED),1)
CFLAGS_anc_assist.o += -DANC_ASSIST_WNR_ENABLED
endif
ifeq ($(ANC_ASSIST_PILOT_ENABLED),1)
CFLAGS_anc_assist.o += -DANC_ASSIST_PILOT_ENABLED
endif
ifeq ($(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED),1)
CFLAGS_anc_assist.o += -DANC_ASSIST_NOISE_ADAPTIVE_ENABLED
endif
ifeq ($(ANC_ASSIST_HESS_ENABLED),1)
CFLAGS_anc_assist.o += -DANC_ASSIST_HESS_ENABLED
endif
ifeq ($(ANC_ASSIST_PNC_ENABLED),1)
CFLAGS_anc_assist.o += -DANC_ASSIST_PNC_ENABLED
endif
ifeq ($(ANC_ASSIST_DEHOWLING_ENABLED),1)
CFLAGS_anc_assist.o += -DANC_ASSIST_DEHOWLING_ENABLED
endif

else
obj_c := $(filter-out src/anc_assist.c ,$(obj_c))
endif 


ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_app_anc.o += -D__AUDIO_RESAMPLE__
endif
ifeq ($(SW_PLAYBACK_RESAMPLE),1)
CFLAGS_app_anc.o += -DSW_PLAYBACK_RESAMPLE
endif
ifeq ($(SW_CAPTURE_RESAMPLE),1)
CFLAGS_app_anc.o += -DSW_CAPTURE_RESAMPLE
endif
ifeq ($(AUDIO_SECTION_SUPPT),1)
CFLAGS_app_anc.o += -D__AUDIO_SECTION_SUPPT__
endif

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

subdir-ccflags-y += \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/fs/fat/ChaN \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iservices/multimedia/audio/process/resample/include \
    -Iservices/multimedia/audio/process/anc/include \
    -Iservices/multimedia/speech/inc \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iapps/anc/inc \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/factory \
	-Iservices/ble_app \
	-Iservices/bt_app \
	-Iservices/anc/inc \
	-Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/nv_section/aud_section \
    -Iservices/nv_section/include   \
    -Iutils/hwtimer_list \
    -Iservices/ibrt_core/inc \
	-Iservices/tota \
	$(BT_IF_INCLUDES) \
	-Iservices/app_ibrt/inc \
	-Iservices/ble_stack/common/api \
	-Iservices/ble_stack/ble_ip \
	-Iservices/ibrt_ui/inc \
	-Iservices/audio_dump/include
