cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

obj-y += $(CHIP)/

subdir-ccflags-y += -Iplatform/drivers/codec

export CODEC_INT_CFG_FLAGS :=

CODEC_INT_CFG_FLAGS += -Iplatform/drivers/ana

ifeq ($(AUDIO_CODEC_ASYNC_CLOSE),1)
CODEC_INT_CFG_FLAGS += -D__CODEC_ASYNC_CLOSE__
endif

ifeq ($(AUDIO_RESAMPLE),1)
CODEC_INT_CFG_FLAGS += -D__AUDIO_RESAMPLE__
endif

ifeq ($(ANC_L_R_MISALIGN_WORKAROUND),1)
CODEC_INT_CFG_FLAGS += -DANC_L_R_MISALIGN_WORKAROUND
endif

ifeq ($(AUDIO_OUTPUT_DC_CALIB),1)
CODEC_INT_CFG_FLAGS += -DAUDIO_OUTPUT_DC_CALIB
endif

ifeq ($(CODEC_ANC_BOOST),1)
CODEC_INT_CFG_FLAGS += -DCODEC_ANC_BOOST
endif

