cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y +=  \
    -Iutils/boot_struct \
    -Iutils/intersyshci \
    -Iutils/hwtimer_list \
    -Iservices/bt_app \
    -Iservices/nvrecord \
    -Iservices/wl_filter \
    -Iservices/bt_if/inc \
    -Iservices/resources \
    -Iservices/remoter \
    -Iservices/wl_filter \
    -Iservices/audio_dump/include \
    -Iservices/multimedia/wl_alg/gcc_plat/include \
    -Iservices/multimedia/wl_alg/vad_user/include \
    -Iservices/overlay \
    -Iservices/multimedia/wl_alg/nsx_main/include \
    -Iservices/multimedia/wl_alg/agc/include \
    -Iservices/multimedia/wl_alg/utility/include \
    -Iservices/multimedia/wl_alg/notch_filter/include \
    -Iservices/multimedia/algorithm/fft/include \
    -Iservices/multimedia/speech/inc \
    -Iapps/key \
    -Iapps/i2c_sensor \
    -Iplatform/drivers/bt \
    -Iplatform/drivers/ana \
	-Iapps/battery

ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_app_factory_audio.o += -D__AUDIO_RESAMPLE__
endif
ifeq ($(SW_CAPTURE_RESAMPLE),1)
CFLAGS_app_factory_audio.o += -DSW_CAPTURE_RESAMPLE
endif

ifeq ($(POWERKEY_I2C_SWITCH),1)
CFLAGS_app_factory.o += -DPOWERKEY_I2C_SWITCH
endif

