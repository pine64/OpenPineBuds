cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y := \
	-Iservices/multimedia/audio/process/anc/include \
	-Iservices/multimedia/audio/process/filters/include \
	-Iservices/nv_section/aud_section \
	-Iservices/nv_section/include \
	-Iutils/crc32 \
	-Iservices/multimedia/speech/src/process/ns_and_ec/inc \
	-Iservices/multimedia/speech/inc \
	-Iservices/multimedia/speech/src/process/common/inc \
	-Iservices/multimedia/speech/src/process/inc

ccflags-y += -DUSE_CMSIS_F32_FFT

ifeq ($(HW_DAC_IIR_EQ_PROCESS),1)
CFLAGS_anc_cfg.o += -D__HW_DAC_IIR_EQ_PROCESS__
endif
ifeq ($(HW_IIR_EQ_PROCESS),1)
CFLAGS_anc_cfg.o += -D__HW_IIR_EQ_PROCESS__
endif
ifeq ($(AUDIO_RESAMPLE),1)
CFLAGS_anc_cfg.o += -D__AUDIO_RESAMPLE__
endif

CFLAGS_fftfilt2.o += -DUSE_MED_MALLOC
