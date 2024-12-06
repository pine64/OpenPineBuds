cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)


subdir-ccflags-y +=  \
	-Ithirdparty/noise_tracker_lib \
	-Iapps/audioplayers \
	-Iapps/common \
	-Iapps/key \
	-Iutils/heap \
	-Irtos/rtx/TARGET_CORTEX_M \
	-Iservices/bt_app \
	-Iservices/audio_dump/include \
	-Iplatform/drivers/ana \
	-Iservices/resources \
	-Iutils/cqueue \
	-Ithirdparty/userapi \
	$(BT_IF_INCLUDES) \
	-Iservices/multimedia/speech/inc \
	$(BT_PROFILES_INCLUDES) \

ifeq ($(VOICE_DETECTOR_EN),1)
subdir-ccflags-y += -Iapps/voice_detector
endif
