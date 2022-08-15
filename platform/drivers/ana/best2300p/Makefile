cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(CODEC_HIGH_QUALITY),1)
VCODEC_VOLT ?= 1.95V
else
VCODEC_VOLT ?= 1.7V
endif
ifneq ($(filter-out 1.6V 1.7V 1.8V 1.9V 1.95V,$(VCODEC_VOLT)),)
$(error Invalid VCODEC_VOLT=$(VCODEC_VOLT))
endif
ifneq ($(VCODEC_VOLT),)
ANA_CFG_FLAGS += -DVCODEC_VOLT=$(subst V,,$(VCODEC_VOLT))
PMU_CFG_FLAGS += -DVCODEC_VOLT=$(subst V,,$(VCODEC_VOLT))
ifneq ($(filter 1.6V 1.7V 1.8V 1.9V 1.95V,$(VCODEC_VOLT)),)
VCODEC_OFF ?= 1
ifeq ($(VHPPA_VOLT),)
VHPPA_VOLT := $(VCODEC_VOLT)
endif
ifneq ($(VCODEC_VOLT),$(VHPPA_VOLT))
$(error Incompatible VCODEC_VOLT=$(VCODEC_VOLT) with VHPPA_VOLT=$(VHPPA_VOLT))
endif
endif
endif

ifeq ($(VCODEC_OFF),1)
PMU_CFG_FLAGS += -DVCODEC_OFF
endif

ifeq ($(VHPPA2VCODEC),1)
PMU_CFG_FLAGS += -DVHPPA2VCODEC
endif

ifeq ($(HPPA_LDO_ON),1)
PMU_CFG_FLAGS += -DHPPA_LDO_ON
endif

VHPPA_VOLT ?= 1.8V
ifneq ($(filter-out 1.6V 1.7V 1.8V 1.9V 1.95V,$(VHPPA_VOLT)),)
$(error Invalid VHPPA_VOLT=$(VHPPA_VOLT))
endif
ifneq ($(VHPPA_VOLT),)
PMU_CFG_FLAGS += -DVHPPA_VOLT=$(subst V,,$(VHPPA_VOLT))
endif

HIGH_VCORE ?= 1
ifeq ($(HIGH_VCORE),1)
PMU_CFG_FLAGS += -DHIGH_VCORE
endif

ifeq ($(USE_CYBERON),1)
PMU_CFG_FLAGS += -DUSE_CYBERON
endif

ifneq ($(EFUSE_BLOCK_NUM),)
PMU_CFG_FLAGS += -DEFUSE_BLOCK_NUM=$(EFUSE_BLOCK_NUM)
endif

ifeq ($(LBRT),1)
VANA_VOLT ?= 1.4V
else
VANA_VOLT ?= 1.3V
endif
ifneq ($(filter-out 1.2V 1.3V 1.35V 1.4V 1.5V 1.6V,$(VANA_VOLT)),)
$(error Invalid VANA_VOLT=$(VANA_VOLT))
endif
ifneq ($(VANA_VOLT),)
VANA_STEM := $(subst .,P,$(VANA_VOLT))
PMU_CFG_FLAGS += -DVANA_$(VANA_STEM)
endif

LOW_POWER_ADC ?= 1
ifeq ($(LOW_POWER_ADC),1)
ANA_CFG_FLAGS += -DLOW_POWER_ADC
endif

ifeq ($(DAC_DRE_GAIN_DC_UPDATE),1)
ANA_CFG_FLAGS += -DDAC_DRE_GAIN_DC_UPDATE
endif

ifeq ($(AUDIO_USE_BBPLL),1)
ANA_CFG_FLAGS += -DAUDIO_USE_BBPLL
PMU_CFG_FLAGS += -DAUDIO_USE_BBPLL
endif

ifeq ($(USB_USE_USBPLL),1)
PMU_CFG_FLAGS += -DUSB_USE_USBPLL
USBPHY_CFG_FLAGS += -DUSB_USE_USBPLL
endif

CFLAGS_pmu_$(CHIP).o += $(PMU_CFG_FLAGS)

CFLAGS_analog_$(CHIP).o += $(ANA_CFG_FLAGS)

CFLAGS_usbphy_$(CHIP).o += $(USBPHY_CFG_FLAGS)

