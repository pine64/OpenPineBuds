cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
ifeq ($(ANC_APP),1)
obj-y += ../../tests/anc_usb/anc_usb_app.c
endif
obj-y += ../../tests/anc_usb/usb_audio_app.c
obj-y += ../../tests/anc_usb/safe_queue.c
obj-y += ../../tests/anc_usb/memutils.c
obj-y += ../../tests/anc_usb/speech_process.c
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

subdir-ccflags-y += \
	-Iutils/cqueue \
	-Iplatform/drivers/ana \
	-Iservices/audio_process \
	-Iservices/multimedia/rbcodec/inc \
	-Iservices/multimedia/audio/process/anc/include \
	-Iservices/multimedia/audio/process/filters/include \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/multimedia/speech/inc \
    -Iservices/nv_section/include \
    -Iservices/nv_section/aud_section \
	-Iplatform/drivers/usb/usb_dev/inc \
	-Itests/anc_usb \
	-Iutils/hwtimer_list

CFLAGS_usb_audio_app.o += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)
CFLAGS_adda_loop_app.o += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)

ifeq ($(APP_TEST_AUDIO),1)
ccflags-y += -DAPP_TEST_AUDIO
endif

ANC_USB_CFG_FLAGS :=

# ANC option
ifeq ($(ANC_APP),1)
ANC_USB_CFG_FLAGS += -DANC_APP
endif

# USB audio option
ifeq ($(USB_AUDIO_APP),1)
ANC_USB_CFG_FLAGS += -DUSB_AUDIO_APP
endif

ifeq ($(USB_HIGH_SPEED),1)
ANC_USB_CFG_FLAGS += -DUSB_HIGH_SPEED
endif

ifeq ($(AUDIO_RESAMPLE),1)
ANC_USB_CFG_FLAGS += -D__AUDIO_RESAMPLE__
endif

ifeq ($(ADC_CH_SEP_BUFF),1)
ANC_USB_CFG_FLAGS += -DADC_CH_SEP_BUFF
endif

include platform/drivers/usb/usb_dev/uaud_cfg_flags.mk

platform/drivers/usb/usb_dev/uaud_cfg_flags.mk: ;

ANC_USB_CFG_FLAGS += $(UAUD_CFG_FLAGS)

# USB audio configuration
ifeq ($(USB_AUDIO_DYN_CFG),1)
ifneq ($(AUDIO_RESAMPLE),1)
SW_CAPTURE_RESAMPLE ?= 1
endif
endif

ifeq ($(AUDIO_PLAYBACK_24BIT),1)
ANC_USB_CFG_FLAGS += -DAUDIO_PLAYBACK_24BIT
endif

# DSD configuration
ifeq ($(HW_FIR_DSD_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__HW_FIR_DSD_PROCESS__
endif

# EQ configuration
ifeq ($(HW_FIR_EQ_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__HW_FIR_EQ_PROCESS__
endif

ifeq ($(HW_IIR_EQ_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__HW_IIR_EQ_PROCESS__
endif

ifeq ($(SW_IIR_EQ_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__SW_IIR_EQ_PROCESS__
endif

ifeq ($(SW_CAPTURE_RESAMPLE),1)
ANC_USB_CFG_FLAGS += -DSW_CAPTURE_RESAMPLE
endif

CFLAGS_app_audtest.o += $(ANC_USB_CFG_FLAGS)
CFLAGS_usb_audio_app.o += $(ANC_USB_CFG_FLAGS)
CFLAGS_anc_usb_app.o += $(ANC_USB_CFG_FLAGS)

ifeq ($(ANC_KEY_DOUBLE_CLICK_ON_OFF),1)
CFLAGS_anc_usb_app.o += -DANC_KEY_DOUBLE_CLICK_ON_OFF
endif

ifeq ($(ANC_FF_ENABLED),1)
CFLAGS_anc_usb_app.o += -DANC_FF_ENABLED
endif

ifeq ($(ANC_FB_ENABLED),1)
CFLAGS_anc_usb_app.o += -DANC_FB_ENABLED
endif

ifeq ($(AUDIO_SECTION_SUPPT),1)
CFLAGS_anc_usb_app.o += -D__AUDIO_SECTION_SUPPT__
endif

ifeq ($(ANC_INIT_OFF),1)
CFLAGS_anc_usb_app.o += -DANC_INIT_OFF
endif

ifeq ($(PC_CMD_UART),1)
CFLAGS_app_audtest.o += -D__PC_CMD_UART__
endif

ifeq ($(DELAY_STREAM_OPEN),1)
CFLAGS_usb_audio_app.o += -DDELAY_STREAM_OPEN
endif

ifeq ($(NOISE_GATING),1)
CFLAGS_usb_audio_app.o += -DNOISE_GATING
endif

ifeq ($(NOISE_REDUCTION),1)
CFLAGS_usb_audio_app.o += -DNOISE_REDUCTION
endif

ifeq ($(ANC_L_R_MISALIGN_WORKAROUND),1)
CFLAGS_usb_audio_app.o += -DANC_L_R_MISALIGN_WORKAROUND
endif

ifeq ($(ANDROID_ACCESSORY_SPEC),1)
CFLAGS_usb_audio_app.o += -DANDROID_ACCESSORY_SPEC
ifeq ($(ANDROID_VOICE_CMD_KEY),1)
CFLAGS_usb_audio_app.o += -DANDROID_VOICE_CMD_KEY
endif
endif

ifeq ($(DUAL_AUX_MIC_MORE_FILTER),1)
CFLAGS_usb_audio_app.o += -DDUAL_AUX_MIC_MORE_FILTER
endif

ifeq ($(FREQ_RESP_EQ),1)
CFLAGS_usb_audio_app.o += -DFREQ_RESP_EQ
endif

ifeq ($(KEEP_SAME_LATENCY),1)
CFLAGS_usb_audio_app.o += -DKEEP_SAME_LATENCY
endif

ifeq ($(USB_AUDIO_PWRKEY_TEST),1)
CFLAGS_usb_audio_app.o += -DUSB_AUDIO_PWRKEY_TEST
endif

ifeq ($(AUDIO_RESAMPLE),1)
# If neither best1000 nor best2000
ifeq ($(filter best1000 best2000,$(CHIP)),)
PLL_TUNE_SAMPLE_RATE ?= 1
endif
ifeq ($(PLL_TUNE_SAMPLE_RATE),1)
CFLAGS_usb_audio_app.o += -DPLL_TUNE_SAMPLE_RATE
endif
endif

