cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_eq_cfg.o += -D__AUDIO_RESAMPLE__
endif

ccflags-y := \
	-Iservices/multimedia/audio/process/filters/include \
