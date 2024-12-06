
add_if_exists = $(foreach d,$(1),$(if $(wildcard $(srctree)/$(d)),$(d) ,))

# -------------------------------------------
# Macro Default Value
# -------------------------------------------
# 1 for IBRT, 0 for stereo
export IBRT ?= 0

# 1 to enable BLE, 0 to disable BLE
export BLE ?= 0

# 1 to enable BLE security feature, 0 to disable it
export BLE_SECURITY_ENABLED ?= 0

export CTKD_ENABLE ?= 0

export TILE_DATAPATH_ENABLED ?= 0

export OTA_ENABLE ?= 0

export OTA_CODE_OFFSET ?= 0
export OTA_UPGRADE_LOG_SIZE ?= 0
export OTA_SUPPORT_SLAVE_BIN ?= 0
export OTA_BOOT_SIZE ?= 0

# is current system a tws system
# currently in our SDK, system include relay tws and IBRT
# of if relay tws or IBRT is enabled, this macro must set to 1
export TWS_SYSTEM_ENABLED ?= 0

# ai voice related configuration
export VOICE_DATAPATH_ENABLED ?= 0

# Google related feature
export GOOGLE_SERVICE_ENABLE ?= 0
export BISTO_ENABLE ?= 0
export GSOUND_HOTWORD_ENABLE ?= 0
export GSOUND_HOTWORD_EXTERNAL ?= 0
export GFPS_ENABLE ?= 0

# Amazon related feature
export AMA_VOICE ?= 0

# HUAWEI related feature
export INTERCONNECTION ?= 0

# ALI related feature
export GMA_VOICE ?= 0

# TENCENT related feature
export TENCENT_VOICE ?= 0

# BAIDU related feature
export DMA_VOICE ?= 0

# BES AI feature
export SMART_VOICE ?= 0

# other AI feature
export CUSTOMIZE_VOICE ?= 0

# mean to manage all AI features
export AI_VOICE ?= 0

# Dual mic recording feature
export DUAL_MIC_RECORDING ?= 0
export LOWER_BANDWIDTH ?= 0

# 1 to support mutiple AI, 0 to disable this feature
IS_MULTI_AI_ENABLE ?= 0

# NOTE: This size is 0 by default, used to save the hotword model file
HOTWORD_SECTION_SIZE ?= 0x0

# NOTE: This size cannot be changed so that audio section address is fixed.
#       This rule can be removed once audio tool can set audio section address dynamically.
FACTORY_SECTION_SIZE ?= 0x1000

# NOTE: This size cannot be changed so that audio section address is fixed.
#       This rule can be removed once audio tool can set audio section address dynamically.
RESERVED_SECTION_SIZE ?= 0x1000

# 1 to enable the VAD feature, 0 to disable the VAD feature
export VOICE_DETECTOR_EN ?= 0

# 1 to use 8K sample rate for VAD, 0 to use 16K sample rate for VAD
VAD_USE_8K_SAMPLE_RATE ?= 0

# 1 to enable the blue flashing light when connected, 0 to disable
CONNECTED_BLUE_LIGHT ?= 0

# -------------------------------------------
# Root Option Dependencies
# -------------------------------------------
# NOTE: the value of AMA_VOICE，DMA_VOICE, SMART_VOICE, TENCENT_VOICE, GMA_VOICE, CUSTOMIZE_VOICE must be confirmed above
ifneq ($(filter 1,$(AMA_VOICE) $(DMA_VOICE) $(SMART_VOICE) $(TENCENT_VOICE) $(GMA_VOICE) $(CUSTOMIZE_VOICE) $(DUAL_MIC_RECORDING)),)
export AI_VOICE := 1
endif

ifeq ($(BT_ANC),1)
export ANC_APP ?= 1
endif

export ANC_ASSIST_ENABLED ?= 0

ifeq ($(GOOGLE_SERVICE_ENABLE), 1)
export BISTO_ENABLE := 1
export GFPS_ENABLE := 1
KBUILD_CPPFLAGS += -DOS_DYNAMIC_MEM_SIZE=0x6800
endif

ifeq ($(BISTO_ENABLE),1)
OTA_ENABLE := 1
export VOICE_DATAPATH_ENABLED := 1
export CRASH_REBOOT ?= 1

export BLE_SECURITY_ENABLED := 1

ifeq ($(CHIP),best1400)
export DUMP_CRASH_LOG ?= 0
else
export DUMP_CRASH_LOG ?= 0
endif

export VOICE_DATAPATH_TYPE ?= gsound
#export TRACE_DUMP2FLASH ?= 1
export FLASH_SUSPEND := 1
export BLE_ONLY_ENABLED ?= 0

ifeq ($(GSOUND_HOTWORD_ENABLE),1)
# used to store hotword model, currently 240KB
# this value is used in link file
HOTWORD_SECTION_SIZE ?= 0x3C000
endif
endif # ifeq ($(BISTO_ENABLE),1)

ifeq ($(MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED),1)
AUDIO_OUTPUT_SW_GAIN ?= 1
endif # ifeq ($(MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED),1)

# NOTE: variable value in BLE_SWITCH must already confirmed above here
BLE_SWITCH := \
    $(BISTO_ENABLE) \
    $(GFPS_ENABLE) \
    $(AMA_VOICE) \
    $(DMA_VOICE) \
    $(GMA_VOICE) \
    $(SMART_VOICE) \
    $(TENCENT_VOICE) \
	$(CUSTOMIZE_VOICE) \
    $(TILE_DATAPATH_ENABLED) \
    $(BLE_ONLY_ENABLED)

ifneq ($(filter 1, $(BLE_SWITCH)),)
export BLE := 1
endif

# NOTE: value of AI_VOICE and BISTO_ENABLE must already confirmed above here
ifeq ($(filter 0,$(AI_VOICE) $(BISTO_ENABLE)),)
IS_MULTI_AI_ENABLE := 1
endif

ifeq ($(VOICE_PROMPT),1)
KBUILD_CPPFLAGS += -DMEDIA_PLAYER_SUPPORT
endif

ifeq ($(INTERCONNECTION),1)
OTA_ENABLE := 1
endif

ifeq ($(GMA_VOICE),1)
OTA_ENABLE := 1
endif

ifeq ($(IBRT),1)
TWS_SYSTEM_ENABLED := 1
endif

MIX_MIC_DURING_MUSIC_ENABLED ?= 0

# make sure the value of GFPS_ENABLE and GMA_VOICE is confirmed above here
ifneq ($(filter 1,$(GFPS_ENABLE) $(GMA_VOICE) $(TOTA)),)
core-y += utils/encrypt/
endif

ifneq ($(filter 1,$(TOTA)),)
core-y += utils/sha256/
endif

ifneq ($(filter apps/ tests/speech_test/ tests/ota_boot/, $(core-y)),)
export BT_APP ?= 1
FULL_APP_PROJECT ?= 1
endif

ifeq ($(strip $(CONNECTED_BLUE_LIGHT)),0)
KBUILD_CPPFLAGS += -DDISABLE_CONNECTED_BLUE_LIGHT
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# OTA features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(OTA_ENABLE),1)

OTA_CODE_OFFSET := 0x18000

OTA_UPGRADE_LOG_SIZE := 0x1000
OTA_SUPPORT_SLAVE_BIN := 0

KBUILD_CPPFLAGS += -DOTA_ENABLED
KBUILD_CPPFLAGS += -DBES_OTA_BASIC

KBUILD_CPPFLAGS += -D__APP_IMAGE_FLASH_OFFSET__=$(OTA_CODE_OFFSET)
KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_OFFSET=0x200000
KBUILD_CPPFLAGS += -DFIRMWARE_REV

ifeq ($(IBRT), 1)
export FORCE_SCO_MAX_RETX := 0
KBUILD_CPPFLAGS += -DIBRT_OTA
endif
endif

# -------------------------------------------
# CHIP selection
# -------------------------------------------

export CHIP

ifneq (,)
else ifeq ($(CHIP),best1000)
KBUILD_CPPFLAGS += -DCHIP_BEST1000
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 2
export CHIP_HAS_DMA := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 1
export CHIP_FLASH_CTRL_VER := 1
export CHIP_PSRAM_CTRL_VER := 1
export CHIP_SPI_VER := 1
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best1400)
ifeq ($(CHIP_SUBTYPE),best1402)
SUBTYPE_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_BEST1402
export CHIP_FLASH_CTRL_VER := 3
else
KBUILD_CPPFLAGS += -DCHIP_BEST1400
export CHIP_FLASH_CTRL_VER := 2
endif
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 0
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 1
export CHIP_HAS_SPDIF := 0
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_SPI_VER := 3
export BTDUMP_ENABLE := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best2000)
KBUILD_CPPFLAGS += -DCHIP_BEST2000
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 1
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 2
export CHIP_HAS_SPDIF := 2
export CHIP_HAS_TRANSQ := 1
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 1
export CHIP_PSRAM_CTRL_VER := 1
export CHIP_SPI_VER := 1
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best2001)
KBUILD_CPPFLAGS += -DCHIP_BEST2001
ifeq ($(CHIP_SUBSYS),dsp)
KBUILD_CPPFLAGS += -DCHIP_BEST2001_DSP
CPU := a7
DSP_ENABLE ?= 1
else
CPU := m33
export CHIP_HAS_CP := 1
endif
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 1
export CHIP_HAS_PSRAMUHS := 1
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_SPIDPD := 1
export CHIP_HAS_I2C := 2
export CHIP_HAS_UART := 3
ifeq ($(DSP_ENABLE), 1)
export DSP_USE_AUDMA ?= 1
ifeq ($(LARGE_RAM), 1)
$(error LARGE_RAM conflicts with DSP_ENABLE)
endif
endif
ifeq ($(DSP_USE_AUDMA), 1)
export CHIP_HAS_DMA := 1
KBUILD_CPPFLAGS += -DDSP_USE_AUDMA
else
ifeq ($(CHIP_SUBSYS),dsp)
export CHIP_HAS_DMA := 0
else
export CHIP_HAS_DMA := 2
endif
endif
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 2
export CHIP_HAS_TRNG := 1
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 3
export CHIP_PSRAM_CTRL_VER := 2
export CHIP_SPI_VER := 4
export CHIP_CACHE_VER := 2
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
export FLASH_SIZE ?= 0x1000000
export OSC_26M_X4_AUD2BB := 0
export USB_USE_USBPLL := 1
export A7_DSP_SPEED ?= 1100
#780:780M, 1000:1G, 1100:1.1G
else ifeq ($(CHIP),best2300)
KBUILD_CPPFLAGS += -DCHIP_BEST2300
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2C := 2
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 2
export CHIP_HAS_I2S := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 2
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best2300a)
KBUILD_CPPFLAGS += -DCHIP_BEST2300A
CPU := m33
LIBC_ROM := 0
CRC32_ROM := 0
SHA256_ROM := 0
export LIBC_OVERRIDE ?= 1
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2C := 3
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 2
export CHIP_HAS_I2S := 2
export CHIP_HAS_TDM := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_CP := 1
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 3
export CHIP_SPI_VER := 4
export CHIP_CACHE_VER := 2
export CHIP_RAM_BOOT := 1
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_HAS_HW_SMOOTHING_GAIN := 1
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best2300p)
KBUILD_CPPFLAGS += -DCHIP_BEST2300P
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 1
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 1
export CHIP_HAS_SPIPHY := 1
export CHIP_HAS_I2C := 2
export CHIP_HAS_UART := 3
export CHIP_HAS_DMA := 2
export CHIP_HAS_I2S := 2
export CHIP_HAS_TDM := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_PSC := 1
export CHIP_HAS_EXT_PMU := 1
export CHIP_HAS_CP := 1
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 3
export CHIP_CACHE_VER := 2
export CHIP_HAS_EC_CODEC_REF := 1
export CHIP_HAS_SCO_DMA_SNAPSHOT := 1
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best3001)
ifeq ($(CHIP_SUBTYPE),best3005)
SUBTYPE_VALID := 1
KBUILD_CPPFLAGS += -DCHIP_BEST3005
export CHIP_CACHE_VER := 2
export CHIP_FLASH_CTRL_VER := 2
else
KBUILD_CPPFLAGS += -DCHIP_BEST3001
export CHIP_FLASH_CTRL_VER := 1
endif
CPU := m4
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 2
export CHIP_HAS_DMA := 1
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_SPI_VER := 3
export CHIP_HAS_EC_CODEC_REF := 0
export CHIP_HAS_SCO_DMA_SNAPSHOT := 0
export CHIP_ROM_UTILS_VER := 1
export NO_LPU_26M ?= 1
else ifeq ($(CHIP),best3003)
KBUILD_CPPFLAGS += -DCHIP_BEST3003
CPU := m33
export CHIP_HAS_FPU := 1
export CHIP_HAS_USB := 1
export CHIP_HAS_USBPHY := 1
export CHIP_HAS_SDMMC := 0
export CHIP_HAS_SDIO := 0
export CHIP_HAS_PSRAM := 0
export CHIP_HAS_SPI := 1
export CHIP_HAS_SPILCD := 0
export CHIP_HAS_SPIPHY := 0
export CHIP_HAS_I2C := 1
export CHIP_HAS_UART := 2
export CHIP_HAS_DMA := 2
export CHIP_HAS_SPDIF := 1
export CHIP_HAS_TRANSQ := 0
export CHIP_HAS_EXT_PMU := 0
export CHIP_HAS_AUDIO_CONST_ROM := 0
export CHIP_CACHE_VER := 2
export CHIP_FLASH_CTRL_VER := 2
export CHIP_SPI_VER := 4
export CHIP_HAS_DCO ?= 1
export NO_LPU_26M ?= 1
else
$(error Invalid CHIP: $(CHIP))
endif

ifneq ($(CHIP_SUBTYPE),)
ifneq ($(SUBTYPE_VALID),1)
$(error Invalid CHIP_SUBTYPE=$(CHIP_SUBTYPE) for CHIP=$(CHIP))
endif
export CHIP_SUBTYPE
endif

ifeq ($(CPU),)
CPU := m33
endif
export CPU

ifneq ($(filter a%,$(CPU)),)
# Override lds file
LDS_FILE := armca.lds

ifeq ($(GEN_BOOT_SECTION),1)
CPPFLAGS_${LDS_FILE} += -DGEN_BOOT_SECTION
endif

ifeq ($(EXEC_IN_RAM),1)
CPPFLAGS_${LDS_FILE} += -DEXEC_IN_RAM
else ifeq ($(EXEC_IN_PSRAM),1)
CPPFLAGS_${LDS_FILE} += -DEXEC_IN_PSRAM
endif
endif

export FLASH_SIZE ?= 0x100000
ifeq ($(CHIP_HAS_PSRAM),1)
export PSRAM_SIZE ?= 0x400000
export PSRAM_ENABLE ?= 0
export PSRAM_SPEED ?= 200
#166:166M, 200:200M
endif
ifeq ($(CHIP_HAS_PSRAMUHS),1)
export PSRAMUHS_ENABLE ?= 0
export PSRAMUHS_SPEED ?= 1000
#400:400M, 600:600M, 800:800M, 900:900M, 1000:1G
ifeq ($(PSRAMUHS_DUAL_8BIT), 1)
export PSRAMUHS_SIZE ?= 0x1000000
ifeq ($(PSRAMUHS_DUAL_SWITCH), 1)
$(error PSRAMUHS_DUAL_8BIT conflicts with PSRAMUHS_DUAL_SWITCH)
endif
endif
export PSRAMUHS_SIZE ?= 0x800000
endif

KBUILD_CPPFLAGS += -DCHIP_HAS_UART=$(CHIP_HAS_UART)
KBUILD_CPPFLAGS += -DCHIP_HAS_I2C=$(CHIP_HAS_I2C)

ifeq ($(CHIP_HAS_USB),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_USB
endif

ifneq ($(CHIP_HAS_TRANSQ),0)
KBUILD_CPPFLAGS += -DCHIP_HAS_TRANSQ=$(CHIP_HAS_TRANSQ)
endif

ifeq ($(CHIP_HAS_CP),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_CP
endif

ifeq ($(CHIP_HAS_AUDIO_CONST_ROM),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_AUDIO_CONST_ROM
endif

ifeq ($(CHIP_HAS_HW_SMOOTHING_GAIN),1)
KBUILD_CPPFLAGS += -DHW_SUPPORT_SMOOOTHING_GAIN
endif

ifeq ($(MCU_SLEEP_POWER_DOWN),1)
KBUILD_CPPFLAGS += -DMCU_SLEEP_POWER_DOWN
endif

ifeq ($(USB_AUDIO_APP),1)
ifneq ($(BTUSB_AUDIO_MODE),1)
NO_OVERLAY ?= 1
endif
endif
export NO_OVERLAY
ifeq ($(NO_OVERLAY),1)
KBUILD_CPPFLAGS +=  -DNO_OVERLAY
endif

ifneq ($(ROM_SIZE),)
KBUILD_CPPFLAGS += -DROM_SIZE=$(ROM_SIZE)
endif

ifneq ($(RAM_SIZE),)
KBUILD_CPPFLAGS += -DRAM_SIZE=$(RAM_SIZE)
endif

ifeq ($(AUDIO_SECTION_ENABLE),1)
KBUILD_CPPFLAGS += -DAUDIO_SECTION_ENABLE
# depend on length of (ANC + AUDIO + SPEECH) in aud_section.c
AUD_SECTION_SIZE ?= 0x8000
ifeq ($(ANC_APP),1)
$(error Can not enable AUDIO_SECTION_ENABLE and ANC_APP together)
endif
endif

ifeq ($(ANC_APP),1)
ifeq ($(CHIP),best1000)
AUD_SECTION_SIZE ?= 0x8000
else
AUD_SECTION_SIZE ?= 0x10000
endif
ifeq ($(ANC_FB_CHECK),1)
KBUILD_CPPFLAGS += -DANC_FB_CHECK
endif
else
AUD_SECTION_SIZE ?= 0
endif

ifeq ($(TWS),1)
LARGE_RAM ?= 1
endif

USERDATA_SECTION_SIZE ?= 0x1000

FACTORY_SECTION_SIZE ?= 0x1000

LHDC_LICENSE_SECTION_SIZE ?= 0x1000

export DUMP_NORMAL_LOG ?= 0
ifeq ($(DUMP_NORMAL_LOG),1)
ifeq ($(FLASH_SIZE),0x40000) # 2M bits
LOG_DUMP_SECTION_SIZE ?= 0x4000
endif
ifeq ($(FLASH_SIZE),0x80000) # 4M bits
LOG_DUMP_SECTION_SIZE ?= 0x8000
endif
ifeq ($(FLASH_SIZE),0x100000) # 8M bits
LOG_DUMP_SECTION_SIZE ?= 0x10000
endif
ifeq ($(FLASH_SIZE),0x200000) # 16M bits
LOG_DUMP_SECTION_SIZE ?= 0x80000
endif
ifeq ($(FLASH_SIZE),0x400000) # 32M bits
LOG_DUMP_SECTION_SIZE ?= 0x200000
endif
ifeq ($(FLASH_SIZE),0x800000) # 64M bits
LOG_DUMP_SECTION_SIZE ?= 0x400000
endif
KBUILD_CPPFLAGS += -DDUMP_LOG_ENABLE
else
LOG_DUMP_SECTION_SIZE ?= 0
endif

APP_USE_LED_INDICATE_IBRT_STATUS ?= 0
ifeq ($(APP_USE_LED_INDICATE_IBRT_STATUS),1)
KBUILD_CPPFLAGS += -D__APP_USE_LED_INDICATE_IBRT_STATUS__
endif

ifeq ($(DUMP_CRASH_LOG),1)
CRASH_DUMP_SECTION_SIZE ?= 0x4000
KBUILD_CPPFLAGS += -DDUMP_CRASH_ENABLE
else
CRASH_DUMP_SECTION_SIZE ?= 0
endif

export CORE_DUMP_TO_FLASH ?= 0
ifeq ($(CORE_DUMP_TO_FLASH),1)
CORE_DUMP_SECTION_SIZE ?= 0x100000
KBUILD_CPPFLAGS += -DCORE_DUMP_TO_FLASH
else
CORE_DUMP_SECTION_SIZE ?= 0
endif

CUSTOM_PARAMETER_SECTION_SIZE ?= 0x1000

export LDS_SECTION_FLAGS := \
	-DHOTWORD_SECTION_SIZE=$(HOTWORD_SECTION_SIZE) \
	-DCORE_DUMP_SECTION_SIZE=$(CORE_DUMP_SECTION_SIZE) \
	-DOTA_UPGRADE_LOG_SIZE=$(OTA_UPGRADE_LOG_SIZE) \
	-DLOG_DUMP_SECTION_SIZE=$(LOG_DUMP_SECTION_SIZE) \
	-DCRASH_DUMP_SECTION_SIZE=$(CRASH_DUMP_SECTION_SIZE) \
	-DCUSTOM_PARAMETER_SECTION_SIZE=$(CUSTOM_PARAMETER_SECTION_SIZE) \
	-DLHDC_LICENSE_SECTION_SIZE=$(LHDC_LICENSE_SECTION_SIZE) \
	-DUSERDATA_SECTION_SIZE=$(USERDATA_SECTION_SIZE) \
	-DAUD_SECTION_SIZE=$(AUD_SECTION_SIZE) \
	-DRESERVED_SECTION_SIZE=$(RESERVED_SECTION_SIZE) \
	-DFACTORY_SECTION_SIZE=$(FACTORY_SECTION_SIZE)

CPPFLAGS_${LDS_FILE} += \
	-DLINKER_SCRIPT \
	-DFLASH_SIZE=$(FLASH_SIZE) \
	-Iplatform/hal

CPPFLAGS_${LDS_FILE} += $(LDS_SECTION_FLAGS)

ifneq ($(PSRAM_SIZE),)
CPPFLAGS_${LDS_FILE} +=-DPSRAM_SIZE=$(PSRAM_SIZE)
endif

ifneq ($(PSRAMUHS_SIZE),)
CPPFLAGS_${LDS_FILE} +=-DPSRAMUHS_SIZE=$(PSRAMUHS_SIZE)
endif

ifneq ($(OTA_BOOT_SIZE), 0)
CPPFLAGS_${LDS_FILE} += -DOTA_BOOT_SIZE=$(OTA_BOOT_SIZE)
endif

CPPFLAGS_${LDS_FILE} += -DOTA_CODE_OFFSET=$(OTA_CODE_OFFSET)

ifneq ($(OTA_REMAP_OFFSET),)
CPPFLAGS_${LDS_FILE} += -DOTA_REMAP_OFFSET=$(OTA_REMAP_OFFSET)
endif

ifneq ($(FLASH_REGION_SIZE),)
CPPFLAGS_${LDS_FILE} += -DFLASH_REGION_SIZE=$(FLASH_REGION_SIZE)
endif

ifneq ($(SLAVE_BIN_FLASH_OFFSET),)
export SLAVE_BIN_FLASH_OFFSET
CPPFLAGS_${LDS_FILE} += -DSLAVE_BIN_FLASH_OFFSET=$(SLAVE_BIN_FLASH_OFFSET)
endif

ifeq ($(BOOT_CODE_IN_RAM),1)
CPPFLAGS_${LDS_FILE} += -DBOOT_CODE_IN_RAM
endif

ifeq ($(GSOUND_HOTWORD_EXTERNAL),1)
CPPFLAGS_${LDS_FILE} += -DGSOUND_HOTWORD_EXTERNAL
endif

ifeq ($(LARGE_RAM),1)
KBUILD_CPPFLAGS += -DLARGE_RAM
endif

ifeq ($(CHIP_HAS_EXT_PMU),1)
export PMU_IRQ_UNIFIED ?= 1
endif

# -------------------------------------------
# Standard C library
# -------------------------------------------

export NOSTD
export LIBC_ROM

ifeq ($(NOSTD),1)

ifeq ($(MBED),1)
$(error Invalid configuration: MBED needs standard C library support)
endif
ifeq ($(RTOS),1)
$(error Invalid configuration: RTOS needs standard C library support)
endif

ifneq ($(NO_LIBC),1)
core-y += utils/libc/
endif

SPECS_CFLAGS :=

LIB_LDFLAGS := $(filter-out -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys,$(LIB_LDFLAGS))

KBUILD_CPPFLAGS += -ffreestanding -Iutils/libc/inc
ifeq ($(TOOLCHAIN),armclang)
# 1) Avoid -nostdinc
#    CMSIS header files need arm_compat.h, which is one of toolchain's standard header files
# 2) Always -nostdlib for compiling C/C++ files
#    Never convert standard API calls to non-standard library calls, but just emit standard API calls
# 3) Avoid -nostdlib for linking final image
#    Some 64-bit calculations and math functions need toolchain's standard library
KBUILD_CPPFLAGS += -nostdlib
else
KBUILD_CPPFLAGS += -nostdinc
CFLAGS_IMAGE += -nostdlib
endif

KBUILD_CPPFLAGS += -DNOSTD

else # NOSTD != 1

ifeq ($(LIBC_ROM),1)
core-y += utils/libc/
endif

ifeq ($(TOOLCHAIN),armclang)
LIB_LDFLAGS := $(filter-out -lsupc++,$(LIB_LDFLAGS))
else
SPECS_CFLAGS := --specs=nano.specs

LIB_LDFLAGS += -lm -lc -lgcc -lnosys
endif

endif # NOSTD != 1

# -------------------------------------------
# RTOS library
# -------------------------------------------

export RTOS

ifeq ($(RTOS),1)

ifeq ($(CPU),m4)
KERNEL ?= RTX
else
KERNEL ?= RTX5
ifeq ($(KERNEL),RTX)
$(error RTX doesn't support $(CPU))
endif
endif

export KERNEL

VALID_KERNEL_LIST := RTX RTX5 FREERTOS

ifeq ($(filter $(VALID_KERNEL_LIST),$(KERNEL)),)
$(error Bad KERNEL=$(KERNEL). Valid values are: $(VALID_KERNEL_LIST))
endif

core-y += rtos/

KBUILD_CPPFLAGS += -DRTOS
KBUILD_CPPFLAGS += -DKERNEL_$(KERNEL)

ifeq ($(KERNEL),RTX)
KBUILD_CPPFLAGS += \
	-Iinclude/rtos/rtx/
KBUILD_CPPFLAGS += -D__RTX_CPU_STATISTICS__=1
#KBUILD_CPPFLAGS += -DTASK_HUNG_CHECK_ENABLED=1
else ifeq ($(KERNEL),RTX5)
OS_IDLESTKSIZE ?= 1024
KBUILD_CPPFLAGS += \
	-Iinclude/rtos/rtx5/
KBUILD_CPPFLAGS += -D__RTX_CPU_STATISTICS__=1
#KBUILD_CPPFLAGS += -DTASK_HUNG_CHECK_ENABLED=1
else #!rtx
ifeq ($(KERNEL),FREERTOS)
KBUILD_CPPFLAGS += \
    -Iinclude/rtos/freertos/
endif #freertos
endif #rtx

ifeq ($(BLE),0)
KBUILD_CPPFLAGS += -DBESBT_STACK_SIZE=1024*5+512
else
KBUILD_CPPFLAGS += -DBESBT_STACK_SIZE=1024*8+512
endif

ifeq ($(BLE_SECURITY_ENABLED), 1)
KBUILD_CPPFLAGS += -DCFG_APP_SEC
endif

ifeq ($(TWS),1)
OS_TASKCNT ?= 12
OS_SCHEDULERSTKSIZE ?= 768
OS_IDLESTKSIZE ?= 512
else
OS_TASKCNT ?= 20
OS_SCHEDULERSTKSIZE ?= 512
OS_IDLESTKSIZE ?= 256
endif

ifeq ($(CPU),m33)
OS_CLOCK_NOMINAL ?= 16000
else
OS_CLOCK_NOMINAL ?= 32000
endif
OS_FIFOSZ ?= 24

export OS_TASKCNT
export OS_SCHEDULERSTKSIZE
export OS_IDLESTKSIZE
export OS_CLOCK_NOMINAL
export OS_FIFOSZ

endif

# -------------------------------------------
# MBED library
# -------------------------------------------

export MBED

ifeq ($(MBED),1)

core-y += mbed/

KBUILD_CPPFLAGS += -DMBED

KBUILD_CPPFLAGS += \
	-Imbed/api \
	-Imbed/common \

endif

# -------------------------------------------
# DEBUG functions
# -------------------------------------------

export DEBUG

ifeq ($(CHIP),best1400)
OPT_LEVEL ?= s
endif

ifneq ($(OPT_LEVEL),)
KBUILD_CFLAGS	+= -O$(OPT_LEVEL)
else
KBUILD_CFLAGS	+= -O2
endif

ifeq ($(DEBUG),1)

KBUILD_CPPFLAGS	+= -DDEBUG

ifneq ($(NOSTD),1)
KBUILD_CFLAGS  	+= -fstack-protector-strong
endif

else

KBUILD_CPPFLAGS	+= -DNDEBUG

REL_TRACE_ENABLE ?= 1
ifeq ($(REL_TRACE_ENABLE),1)
KBUILD_CPPFLAGS	+= -DREL_TRACE_ENABLE
endif

endif

ifeq ($(NO_CHK_TRC_FMT),1)
KBUILD_CPPFLAGS	+= -DNO_CHK_TRC_FMT
else
# Typedef int32_t to int, and typedef uint32_t to unsigned int
KBUILD_CPPFLAGS	+= -U__INT32_TYPE__ -D__INT32_TYPE__=int -U__UINT32_TYPE__
endif

ifeq ($(MERGE_CONST),1)
ifeq ($(TOOLCHAIN),armclang)
$(error MERGE_CONST is not supported in $(TOOLCHAIN))
else
KBUILD_CPPFLAGS += -fmerge-constants -fmerge-all-constants
endif
endif

export CORE_DUMP ?= 0
ifeq ($(CORE_DUMP),1)
core-y += utils/crash_catcher/ utils/xyzmodem/
endif

# -------------------------------------------
# SIMU functions
# -------------------------------------------

export SIMU

ifeq ($(SIMU),1)

KBUILD_CPPFLAGS += -DSIMU

endif

# -------------------------------------------
# ROM_BUILD functions
# -------------------------------------------

export ROM_BUILD

ifeq ($(ROM_BUILD),1)

KBUILD_CPPFLAGS += -DROM_BUILD

endif

# Limit the length of REVISION_INFO if ROM_BUILD or using rom.lds
ifneq ($(filter 1,$(ROM_BUILD))$(filter rom.lds,$(LDS_FILE)),)
ifeq ($(CHIP),best1000)
REVISION_INFO := x
else
REVISION_INFO := $(GIT_REVISION)
endif
endif

# -------------------------------------------
# PROGRAMMER functions
# -------------------------------------------

export PROGRAMMER

ifeq ($(PROGRAMMER),1)

KBUILD_CPPFLAGS += -DPROGRAMMER

endif

# -------------------------------------------
# ROM_UTILS functions
# -------------------------------------------

export ROM_UTILS_ON ?= 0
ifeq ($(ROM_UTILS_ON),1)
KBUILD_CPPFLAGS += -DROM_UTILS_ON
core-y += utils/rom_utils/
endif

# -------------------------------------------
# Features
# -------------------------------------------

export DEBUG_PORT ?= 1

ifneq ($(filter best1000 best2000,$(CHIP)),)
export AUD_SECTION_STRUCT_VERSION ?= 1
else
export AUD_SECTION_STRUCT_VERSION ?= 2
endif

ifneq ($(AUD_SECTION_STRUCT_VERSION),)
KBUILD_CPPFLAGS += -DAUD_SECTION_STRUCT_VERSION=$(AUD_SECTION_STRUCT_VERSION)
endif

export FLASH_CHIP
ifneq ($(FLASH_CHIP),)
VALID_FLASH_CHIP_LIST := ALL \
	GD25LQ64C GD25LQ32C GD25LQ16C GD25Q32C GD25Q80C GD25Q40C GD25Q20C \
	P25Q64L P25Q32L P25Q16L P25Q80H P25Q21H \
	XT25Q08B \
	EN25S80B
ifneq ($(filter-out $(VALID_FLASH_CHIP_LIST),$(FLASH_CHIP)),)
$(error Invalid FLASH_CHIP: $(filter-out $(VALID_FLASH_CHIP_LIST),$(FLASH_CHIP)))
endif
endif

NV_REC_DEV_VER ?= 2

export NO_SLEEP ?= 0

export FAULT_DUMP ?= 1

export USE_TRACE_ID ?= 0
ifeq ($(USE_TRACE_ID),1)
export TRACE_STR_SECTION ?= 1
endif

export CRASH_BOOT ?= 0

export OSC_26M_X4_AUD2BB ?= 0
ifeq ($(OSC_26M_X4_AUD2BB),1)
export ANA_26M_X4_ENABLE ?= 1
export FLASH_LOW_SPEED ?= 0
endif

export AUDIO_CODEC_ASYNC_CLOSE ?= 0

# Enable the workaround for BEST1000 version C & earlier chips
export CODEC_PLAY_BEFORE_CAPTURE ?= 0

export AUDIO_INPUT_CAPLESSMODE ?= 0

export AUDIO_INPUT_LARGEGAIN ?= 0

export AUDIO_INPUT_MONO ?= 0

export AUDIO_OUTPUT_MONO ?= 0

export AUDIO_OUTPUT_VOLUME_DEFAULT ?= 10

export AUDIO_OUTPUT_INVERT_RIGHT_CHANNEL ?= 0

export AUDIO_OUTPUT_CALIB_GAIN_MISSMATCH ?= 0

ifeq ($(USB_AUDIO_APP),1)
export CODEC_HIGH_QUALITY ?= 1
endif

ifeq ($(ANC_APP),1)
export CODEC_HIGH_QUALITY ?= 1
endif

ifeq ($(CHIP),best1000)
AUDIO_OUTPUT_DIFF ?= 1
AUDIO_OUTPUT_DC_CALIB ?= $(AUDIO_OUTPUT_DIFF)
AUDIO_OUTPUT_SMALL_GAIN_ATTN ?= 1
AUDIO_OUTPUT_SW_GAIN ?= 1
ANC_L_R_MISALIGN_WORKAROUND ?= 1
else ifeq ($(CHIP),best2000)
ifeq ($(CODEC_HIGH_QUALITY),1)
export VCODEC_VOLT ?= 2.5V
else
export VCODEC_VOLT ?= 1.6V
endif
AUDIO_OUTPUT_DIFF ?= 0
ifeq ($(VCODEC_VOLT),2.5V)
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
else
AUDIO_OUTPUT_DC_CALIB ?= 1
AUDIO_OUTPUT_DC_CALIB_ANA ?= 0
endif
ifneq ($(AUDIO_OUTPUT_DIFF),1)
# Class-G module still needs improving
#DAC_CLASSG_ENABLE ?= 1
endif
else ifeq ($(CHIP),best2001)
export VCODEC_VOLT ?= 1.8V
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
else ifneq ($(filter best3001 best3003 best3005,$(CHIP)),)
export VCODEC_VOLT ?= 2.5V
AUDIO_OUTPUT_DC_CALIB ?= 1
AUDIO_OUTPUT_DC_CALIB_ANA ?= 0
else
AUDIO_OUTPUT_DC_CALIB ?= 0
AUDIO_OUTPUT_DC_CALIB_ANA ?= 1
endif

ifeq ($(AUDIO_OUTPUT_DC_CALIB)-$(AUDIO_OUTPUT_DC_CALIB_ANA),1-1)
$(error AUDIO_OUTPUT_DC_CALIB and AUDIO_OUTPUT_DC_CALIB_ANA cannot be enabled at the same time)
endif

export AUDIO_OUTPUT_DIFF

export AUDIO_OUTPUT_DC_CALIB

export AUDIO_OUTPUT_DC_CALIB_ANA

export AUDIO_OUTPUT_SMALL_GAIN_ATTN

export AUDIO_OUTPUT_SW_GAIN

export ANC_L_R_MISALIGN_WORKAROUND

export DAC_CLASSG_ENABLE

export AF_DEVICE_I2S ?= 0

export AF_DEVICE_TDM ?= 0

export AF_ADC_I2S_SYNC ?= 0
ifeq ($(AF_ADC_I2S_SYNC),1)
KBUILD_CPPFLAGS += -DAF_ADC_I2S_SYNC
export AF_DEVICE_I2S = 1
export INT_LOCK_EXCEPTION ?= 1
endif

export BONE_SENSOR_TDM ?= 0
ifeq ($(BONE_SENSOR_TDM),1)
KBUILD_CPPFLAGS += -DBONE_SENSOR_TDM
KBUILD_CPPFLAGS += -DI2C_TASK_MODE
export AF_DEVICE_I2S = 1
export AF_DEVICE_TDM = 1
KBUILD_CPPFLAGS += -DI2S_MCLK_FROM_SPDIF
KBUILD_CPPFLAGS += -DI2S_MCLK_IOMUX_INDEX=13
KBUILD_CPPFLAGS += -DCLKOUT_IOMUX_INDEX=13
endif

export PLAYBACK_USE_I2S ?= 0
ifeq ($(PLAYBACK_USE_I2S),1)
KBUILD_CPPFLAGS += -DPLAYBACK_USE_I2S
export AF_DEVICE_I2S = 1
export A2DP_EQ_24BIT = 0
endif

ifeq ($(ANC_APP),1)
export ANC_FF_ENABLED ?= 1
ifeq ($(ANC_FB_CHECK),1)
KBUILD_CPPFLAGS += -DANC_FB_CHECK
endif
endif

ifeq ($(CHIP),best1400)
export AUDIO_RESAMPLE ?= 1
export PMU_IRQ_UNIFIED ?= 1
else ifeq ($(CHIP),best2001)
export AUDIO_RESAMPLE ?= 1
else
export AUDIO_RESAMPLE ?= 0
endif

ifeq ($(AUDIO_RESAMPLE),1)
ifeq ($(CHIP),best1000)
export SW_PLAYBACK_RESAMPLE ?= 1
export SW_CAPTURE_RESAMPLE ?= 1
export NO_SCO_RESAMPLE ?= 1
endif # CHIP is best1000
ifeq ($(CHIP),best2000)
export SW_CAPTURE_RESAMPLE ?= 1
export SW_SCO_RESAMPLE ?= 1
export NO_SCO_RESAMPLE ?= 0
endif # CHIP is best2000
ifeq ($(BT_ANC),1)
ifeq ($(NO_SCO_RESAMPLE),1)
$(error BT_ANC and NO_SCO_RESAMPLE cannot be enabled at the same time)
endif
endif # BT_ANC

KBUILD_CPPFLAGS += -D__AUDIO_RESAMPLE__
endif # AUDIO_RESAMPLE

export HW_FIR_DSD_PROCESS ?= 0

export HW_FIR_EQ_PROCESS ?= 0

export SW_IIR_EQ_PROCESS ?= 0
ifeq ($(SW_IIR_EQ_PROCESS),1)
export A2DP_EQ_24BIT = 1
endif

export HW_IIR_EQ_PROCESS ?= 0

export HW_DAC_IIR_EQ_PROCESS ?= 0

export AUDIO_DRC ?= 0

export AUDIO_DRC2 ?= 0

export HW_DC_FILTER_WITH_IIR ?= 0
ifeq ($(HW_DC_FILTER_WITH_IIR),1)
KBUILD_CPPFLAGS += -DHW_DC_FILTER_WITH_IIR
export HW_FILTER_CODEC_IIR ?= 1
endif

ifeq ($(USB_AUDIO_APP),1)
export ANDROID_ACCESSORY_SPEC ?= 1
export FIXED_CODEC_ADC_VOL ?= 0

ifneq ($(BTUSB_AUDIO_MODE),1)
NO_PWRKEY ?= 1
NO_GROUPKEY ?= 1
endif
endif

export NO_PWRKEY

export NO_GROUPKEY

ifneq ($(CHIP),best1000)
ifneq ($(CHIP)-$(TWS),best2000-1)
# For bt
export A2DP_EQ_24BIT ?= 1
# For usb audio
export AUDIO_PLAYBACK_24BIT ?= 1
endif
endif

ifeq ($(CHIP),best1000)

ifeq ($(AUD_PLL_DOUBLE),1)
KBUILD_CPPFLAGS += -DAUD_PLL_DOUBLE
endif

ifeq ($(DUAL_AUX_MIC),1)
ifeq ($(AUDIO_INPUT_MONO),1)
$(error Invalid talk mic configuration)
endif
KBUILD_CPPFLAGS += -D_DUAL_AUX_MIC_
endif

endif # best1000

ifeq ($(CAPTURE_ANC_DATA),1)
KBUILD_CPPFLAGS += -DCAPTURE_ANC_DATA
endif

ifeq ($(AUDIO_ANC_TT_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_TT_HW
endif

ifeq ($(AUDIO_ANC_FB_MC_HW),1)
KBUILD_CPPFLAGS += -DAUDIO_ANC_FB_MC_HW
endif

ifeq ($(AUDIO_ANC_FB_MC),1)
ifeq ($(AUDIO_RESAMPLE),1)
$(error AUDIO_ANC_FB_MC conflicts with AUDIO_RESAMPLE)
endif
KBUILD_CPPFLAGS += -DAUDIO_ANC_FB_MC
endif

ifeq ($(BT_ANC),1)
KBUILD_CPPFLAGS += -D__BT_ANC__
endif

export ANC_NOISE_TRACKER ?= 0
ifeq ($(ANC_NOISE_TRACKER),1)
ifeq ($(IBRT),1)
KBUILD_CPPFLAGS += -DANC_NOISE_TRACKER_CHANNEL_NUM=1
else
KBUILD_CPPFLAGS += -DANC_NOISE_TRACKER_CHANNEL_NUM=2
endif
endif

ifeq ($(AUDIO_ANC_FB_ADJ_MC),1)
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

ifeq ($(ANC_WNR_ENABLED),1)
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

ifeq ($(KWS_ALEXA),1)
KBUILD_CPPFLAGS += -DKWS_BES
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_CMSIS_FFT
endif

export BTUSB_AUDIO_MODE ?= 0
ifeq ($(BTUSB_AUDIO_MODE),1)
KBUILD_CPPFLAGS += -DBTUSB_AUDIO_MODE
endif

export BT_USB_AUDIO_DUAL_MODE ?= 0
ifeq ($(BT_USB_AUDIO_DUAL_MODE),1)
KBUILD_CPPFLAGS += -DBT_USB_AUDIO_DUAL_MODE
endif

ifeq ($(WATCHER_DOG),1)
KBUILD_CPPFLAGS += -D__WATCHER_DOG_RESET__
endif

export ULTRA_LOW_POWER ?= 0
ifeq ($(ULTRA_LOW_POWER),1)
export FLASH_LOW_SPEED ?= 1
export PSRAM_LOW_SPEED ?= 1
endif

export USB_HIGH_SPEED ?= 0
ifeq ($(CHIP),best2000)
ifeq ($(USB_HIGH_SPEED),1)
export AUDIO_USE_BBPLL ?= 1
endif
ifeq ($(AUDIO_USE_BBPLL),1)
ifeq ($(MCU_HIGH_PERFORMANCE_MODE),1)
$(error MCU_HIGH_PERFORMANCE_MODE conflicts with AUDIO_USE_BBPLL)
endif
else # !AUDIO_USE_BBPLL
ifeq ($(USB_HIGH_SPEED),1)
$(error AUDIO_USE_BBPLL must be used with USB_HIGH_SPEED)
endif
endif # !AUDIO_USE_BBPLL
endif # best2000

ifeq ($(SIMPLE_TASK_SWITCH),1)
KBUILD_CPPFLAGS += -DSIMPLE_TASK_SWITCH
endif

ifeq ($(ASSERT_SHOW_FILE_FUNC),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FILE_FUNC
else
ifeq ($(ASSERT_SHOW_FILE),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FILE
else
ifeq ($(ASSERT_SHOW_FUNC),1)
KBUILD_CPPFLAGS += -DASSERT_SHOW_FUNC
endif
endif
endif

ifeq ($(CALIB_SLOW_TIMER),1)
KBUILD_CPPFLAGS += -DCALIB_SLOW_TIMER
endif

ifeq ($(INT_LOCK_EXCEPTION),1)
KBUILD_CPPFLAGS += -DINT_LOCK_EXCEPTION
endif

export APP_ANC_TEST ?= 0
ifeq ($(APP_ANC_TEST),1)
KBUILD_CPPFLAGS += -DAPP_ANC_TEST
endif

USER_REBOOT_PLAY_MUSIC_AUTO ?= 0
ifeq ($(USER_REBOOT_PLAY_MUSIC_AUTO),1)
KBUILD_CPPFLAGS += -DUSER_REBOOT_PLAY_MUSIC_AUTO
export USER_REBOOT_PLAY_MUSIC_AUTO ?= 1
endif

export TEST_OVER_THE_AIR ?= 0
ifeq ($(TEST_OVER_THE_AIR),1)
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED=1
endif

ifeq ($(USE_TRACE_ID),1)
KBUILD_CPPFLAGS += -DUSE_TRACE_ID
endif

ifeq ($(TRACE_STR_SECTION),1)
KBUILD_CPPFLAGS += -DTRACE_STR_SECTION
CPPFLAGS_${LDS_FILE} += -DTRACE_STR_SECTION
endif

USE_THIRDPARTY ?= 0
ifeq ($(USE_THIRDPARTY),1)
KBUILD_CPPFLAGS += -D__THIRDPARTY
core-y += thirdparty/
endif

export PC_CMD_UART ?= 0
ifeq ($(PC_CMD_UART),1)
KBUILD_CPPFLAGS += -D__PC_CMD_UART__
endif

ifeq ($(VOICE_DETECTOR_EN),1)
KBUILD_CPPFLAGS += -DVOICE_DETECTOR_EN
endif

ifeq ($(VAD_USE_8K_SAMPLE_RATE),1)
KBUILD_CPPFLAGS += -DVAD_USE_8K_SAMPLE_RATE
endif

ifeq ($(USB_ANC_MC_EQ_TUNING),1)
KBUILD_CPPFLAGS += -DUSB_ANC_MC_EQ_TUNING -DANC_PROD_TEST
endif

export AUTO_TEST ?= 0
ifeq ($(AUTO_TEST),1)
KBUILD_CFLAGS += -D_AUTO_TEST_
endif

export BES_AUTOMATE_TEST ?= 0
ifeq ($(BES_AUTOMATE_TEST),1)
KBUILD_CFLAGS += -DBES_AUTOMATE_TEST
KBUILD_CFLAGS += -DHAL_TRACE_RX_ENABLE
endif

ifeq ($(RB_CODEC),1)
CPPFLAGS_${LDS_FILE} += -DRB_CODEC
endif

ifneq ($(DATA_BUF_START),)
CPPFLAGS_${LDS_FILE} += -DDATA_BUF_START=$(DATA_BUF_START)
endif

ifeq ($(USER_SECURE_BOOT),1)
core-y += utils/user_secure_boot/ \
               utils/system_info/
endif

ifeq ($(MAX_DAC_OUTPUT),-60db)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_M60DB
else
ifeq ($(MAX_DAC_OUTPUT),3.75mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_3P75MW
else
ifeq ($(MAX_DAC_OUTPUT),5mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_5MW
else
ifeq ($(MAX_DAC_OUTPUT),10mw)
MAX_DAC_OUTPUT_FLAGS := -DMAX_DAC_OUTPUT_10MW
else
ifneq ($(MAX_DAC_OUTPUT),30mw)
ifneq ($(MAX_DAC_OUTPUT),)
$(error Invalid MAX_DAC_OUTPUT value: $(MAX_DAC_OUTPUT) (MUST be one of: -60db 3.75mw 5mw 10mw 30mw))
endif
endif
endif
endif
endif
endif
export MAX_DAC_OUTPUT_FLAGS

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BT features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BT_APP),1)

export BT_IF_INCLUDES ?=
export BT_PROFILES_INCLUDES ?=
export ENHANCED_STACK ?= 1

export INTERSYS_NO_THREAD ?= 0

export INTERSYS_DEBUG ?= 0
ifeq ($(INTERSYS_DEBUG),1)
	KBUILD_CPPFLAGS += -DINTERSYS_DEBUG=1
endif

export BT_DEBUG_TPORTS ?= 0
ifneq ($(BT_DEBUG_TPORTS),0)
	KBUILD_CPPFLAGS += -D__BT_DEBUG_TPORTS__
endif

export SNOOP_DATA_EXCHANGE_VIA_BLE ?= 0
ifeq ($(SNOOP_DATA_EXCHANGE_VIA_BLE),1)
	KBUILD_CPPFLAGS += -DSNOOP_DATA_EXCHANGE_VIA_BLE
endif

export SYNC_BT_CTLR_PROFILE ?= 0
ifeq ($(SYNC_BT_CTLR_PROFILE),1)
	KBUILD_CPPFLAGS += -DSYNC_BT_CTLR_PROFILE
endif

export PROFILE_DEBUG ?= 0
ifeq ($(PROFILE_DEBUG),1)
	KBUILD_CPPFLAGS += -DXA_DEBUG=1
endif

ifeq ($(ENHANCED_STACK),1)
BT_IF_INCLUDES += \
	-Iservices/bt_if_enhanced/inc
BT_PROFILES_INCLUDES += \
	-Iservices/bt_profiles_enhanced/inc
else
BT_IF_INCLUDES += \
	-Iservices/bt_if/inc
BT_PROFILES_INCLUDES += \
	-Iservices/bt_profiles/inc \
	-Iservices/bt_profiles/inc/sys
endif

ifeq ($(ENHANCED_STACK),1)
KBUILD_CFLAGS += -DENHANCED_STACK
#KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__ -D__A2DP_AVDTP_DR__
#KBUILD_CPPFLAGS += -D__A2DP_AVDTP_DR__
KBUILD_CPPFLAGS += -D__BLE_TX_USE_BT_TX_QUEUE__
ifeq ($(BISTO_ENABLE),1)
ifneq ($(IBRT),1)
#disbled before IBRT MAP role switch feature is ready
KBUILD_CPPFLAGS += -D__BTMAP_ENABLE__
endif
endif
endif

ifeq ($(A2DP_AVDTP_CP),1)
KBUILD_CPPFLAGS += -D__A2DP_AVDTP_CP__
endif

ifneq ($(filter-out 2M 3M,$(BT_RF_PREFER)),)
$(error Invalid BT_RF_PREFER=$(BT_RF_PREFER))
endif
ifneq ($(BT_RF_PREFER),)
RF_PREFER := $(subst .,P,$(BT_RF_PREFER))
KBUILD_CPPFLAGS += -D__$(RF_PREFER)_PACK__
endif

export AUDIO_SCO_BTPCM_CHANNEL ?= 1
ifeq ($(AUDIO_SCO_BTPCM_CHANNEL),1)
KBUILD_CPPFLAGS += -D_SCO_BTPCM_CHANNEL_
endif

export BT_ONE_BRING_TWO ?= 0
ifeq ($(BT_ONE_BRING_TWO),1)
KBUILD_CPPFLAGS += -D__BT_ONE_BRING_TWO__
endif

export A2DP_PLAYER_USE_BT_TRIGGER ?= 1
ifeq ($(A2DP_PLAYER_USE_BT_TRIGGER),1)
KBUILD_CPPFLAGS += -D__A2DP_PLAYER_USE_BT_TRIGGER__
endif

export BT_SELECT_PROF_DEVICE_ID ?= 0
ifeq ($(BT_ONE_BRING_TWO),1)
ifeq ($(BT_SELECT_PROF_DEVICE_ID),1)
KBUILD_CPPFLAGS += -D__BT_SELECT_PROF_DEVICE_ID__
endif
endif

export SBC_FUNC_IN_ROM ?= 0
ifeq ($(SBC_FUNC_IN_ROM),1)

KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM__

ifeq ($(CHIP),best2000)
UNALIGNED_ACCESS ?= 1
KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM_VBEST2000_ONLYSBC__
KBUILD_CPPFLAGS += -D__SBC_FUNC_IN_ROM_VBEST2000__
endif
endif

export HFP_1_6_ENABLE ?= 1
ifeq ($(HFP_1_6_ENABLE),1)
KBUILD_CPPFLAGS += -DHFP_1_6_ENABLE
ifeq ($(MSBC_16K_SAMPLE_RATE),0)
KBUILD_CPPFLAGS += -DMSBC_8K_SAMPLE_RATE
export DSP_LIB ?= 1
endif
endif

export A2DP_AAC_ON ?= 0
ifeq ($(A2DP_AAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_AAC_ON
KBUILD_CPPFLAGS += -D__ACC_FRAGMENT_COMPATIBLE__
endif

export FDKAAC_VERSION ?= 2

ifneq ($(FDKAAC_VERSION),)
KBUILD_CPPFLAGS += -DFDKAAC_VERSION=$(FDKAAC_VERSION)
endif

export A2DP_LHDC_ON ?= 0
ifeq ($(A2DP_LHDC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_ON
export A2DP_LHDC_V3 ?= 0
ifeq ($(A2DP_LHDC_V3),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_V3
endif
export A2DP_LHDC_LARC ?= 0
ifeq ($(A2DP_LHDC_LARC),1)
KBUILD_CPPFLAGS += -DA2DP_LHDC_LARC
endif

core-y += thirdparty/audio_codec_lib/liblhdc-dec/
endif
ifeq ($(USER_SECURE_BOOT),1)
KBUILD_CPPFLAGS += -DUSER_SECURE_BOOT
endif

export A2DP_SCALABLE_ON ?= 0
ifeq ($(A2DP_SCALABLE_ON),1)
KBUILD_CPPFLAGS += -DA2DP_SCALABLE_ON
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT
#KBUILD_CPPFLAGS += -DA2DP_SCALABLE_UHQ_SUPPORT
core-y += thirdparty/audio_codec_lib/scalable/
endif

export A2DP_LDAC_ON ?= 0
ifeq ($(A2DP_LDAC_ON),1)
KBUILD_CPPFLAGS += -DA2DP_LDAC_ON
core-y += thirdparty/audio_codec_lib/ldac/
endif

export A2DP_CP_ACCEL ?= 0
ifeq ($(A2DP_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DA2DP_CP_ACCEL
endif

export SCO_CP_ACCEL ?= 0
ifeq ($(SCO_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DSCO_CP_ACCEL
# spx fft will share buffer which is not fit for dual cores.
KBUILD_CPPFLAGS += -DUSE_CMSIS_F32_FFT
endif

export SCO_TRACE_CP_ACCEL ?= 0
ifeq ($(SCO_TRACE_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DSCO_TRACE_CP_ACCEL
endif

ifeq ($(BT_XTAL_SYNC),1)
KBUILD_CPPFLAGS += -DBT_XTAL_SYNC_NEW_METHOD
KBUILD_CPPFLAGS += -DFIXED_BIT_OFFSET_TARGET
endif

ifeq ($(BT_XTAL_SYNC_SLOW),1)
KBUILD_CPPFLAGS += -DBT_XTAL_SYNC_SLOW
endif

ifeq ($(BT_XTAL_SYNC_NO_RESET),1)
KBUILD_CPPFLAGS += -DBT_XTAL_SYNC_NO_RESET
endif


ifeq ($(APP_LINEIN_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_LINEIN_A2DP_SOURCE
endif

ifeq ($(HSP_ENABLE),1)
KBUILD_CPPFLAGS += -D__HSP_ENABLE__
endif

ifeq ($(APP_I2S_A2DP_SOURCE),1)
KBUILD_CPPFLAGS += -DAPP_I2S_A2DP_SOURCE
endif

export TX_RX_PCM_MASK ?= 0
ifeq ($(TX_RX_PCM_MASK),1)
KBUILD_CPPFLAGS += -DTX_RX_PCM_MASK
endif

export PCM_FAST_MODE ?= 0
ifeq ($(PCM_FAST_MODE),1)
KBUILD_CPPFLAGS += -DPCM_FAST_MODE
endif

export LOW_DELAY_SCO ?= 0
ifeq ($(LOW_DELAY_SCO),1)
KBUILD_CPPFLAGS += -DLOW_DELAY_SCO
endif

export CVSD_BYPASS ?= 0
ifeq ($(CVSD_BYPASS),1)
KBUILD_CPPFLAGS += -DCVSD_BYPASS
endif

export SCO_DMA_SNAPSHOT ?= 0
ifeq ($(CHIP_HAS_SCO_DMA_SNAPSHOT),1)
export SCO_DMA_SNAPSHOT := 1
KBUILD_CPPFLAGS += -DSCO_DMA_SNAPSHOT
endif

export SCO_OPTIMIZE_FOR_RAM ?= 0
ifeq ($(SCO_OPTIMIZE_FOR_RAM),1)
KBUILD_CPPFLAGS += -DSCO_OPTIMIZE_FOR_RAM
endif

export AAC_TEXT_PARTIAL_IN_FLASH ?= 0
ifeq ($(AAC_TEXT_PARTIAL_IN_FLASH),1)
KBUILD_CPPFLAGS += -DAAC_TEXT_PARTIAL_IN_FLASH
endif

ifeq ($(SUPPORT_BATTERY_REPORT),1)
KBUILD_CPPFLAGS += -DSUPPORT_BATTERY_REPORT
endif

ifeq ($(SUPPORT_HF_INDICATORS),1)
KBUILD_CPPFLAGS += -DSUPPORT_HF_INDICATORS
endif

ifeq ($(SUPPORT_SIRI),1)
KBUILD_CPPFLAGS += -DSUPPORT_SIRI
endif

export BQB_PROFILE_TEST ?= 0
ifeq ($(BQB_PROFILE_TEST),1)
KBUILD_CPPFLAGS += -D__BQB_PROFILE_TEST__
endif

export AUDIO_SPECTRUM ?= 0
ifeq ($(AUDIO_SPECTRUM),1)
KBUILD_CPPFLAGS += -D__AUDIO_SPECTRUM__
KBUILD_CPPFLAGS += -DGLOBAL_SRAM_KISS_FFT
endif

export INTERCONNECTION ?= 0
ifeq ($(INTERCONNECTION),1)
KBUILD_CPPFLAGS += -D__INTERCONNECTION__
endif

export INTERACTION ?= 0
ifeq ($(INTERACTION),1)
KBUILD_CPPFLAGS += -D__INTERACTION__
endif


export WL_DET ?= 0
ifeq ($(WL_DET),1)
KBUILD_CPPFLAGS += -DWL_DET
endif

export AUDIO_LOOPBACK ?= 0
ifeq ($(AUDIO_LOOPBACK),1)
KBUILD_CPPFLAGS += -DAUDIO_LOOPBACK
endif


export INTERACTION_FASTPAIR ?= 0
ifeq ($(INTERACTION_FASTPAIR),1)
KBUILD_CPPFLAGS += -D__INTERACTION_FASTPAIR__
KBUILD_CPPFLAGS += -D__INTERACTION_CUSTOMER_AT_COMMAND__
endif

ifeq ($(SUSPEND_ANOTHER_DEV_A2DP_STREAMING_WHEN_CALL_IS_COMING),1)
KBUILD_CPPFLAGS += -DSUSPEND_ANOTHER_DEV_A2DP_STREAMING_WHEN_CALL_IS_COMING
endif

export TWS_PROMPT_SYNC ?= 0
ifeq ($(TWS_PROMPT_SYNC), 1)
export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED = 1
KBUILD_CPPFLAGS += -DTWS_PROMPT_SYNC
endif

export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED ?= 0
ifeq ($(MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED), 1)
KBUILD_CPPFLAGS += -DMIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
export RESAMPLE_ANY_SAMPLE_RATE ?= 1
endif

#ifeq ($(AUDIO_RESAMPLE),0)
export RESAMPLE_ANY_SAMPLE_RATE ?= 1
KBUILD_CPPFLAGS += -DRESAMPLE_ANY_SAMPLE_RATE
#endif

export MEDIA_PLAY_24BIT ?= 0

export LBRT ?= 0
ifeq ($(LBRT),1)
KBUILD_CPPFLAGS += -DLBRT
endif

export IBRT ?= 0
ifeq ($(IBRT),1)
KBUILD_CPPFLAGS += -DIBRT
KBUILD_CPPFLAGS += -DIBRT_BLOCKED
KBUILD_CPPFLAGS += -DIBRT_NOT_USE
KBUILD_CPPFLAGS += -D__A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__
endif


export IBRT_TESTMODE ?= 0
ifeq ($(IBRT_TESTMODE),1)
KBUILD_CPPFLAGS += -D__IBRT_IBRT_TESTMODE__
endif

ifeq ($(TWS_SYSTEM_ENABLED),1)
KBUILD_CPPFLAGS += -DTWS_SYSTEM_ENABLED
endif

export BES_AUD ?= 0
ifeq ($(BES_AUD),1)
KBUILD_CPPFLAGS += -DBES_AUD
endif

export IBRT_SEARCH_UI ?= 0
ifeq ($(IBRT_SEARCH_UI),1)
KBUILD_CPPFLAGS += -DIBRT_SEARCH_UI
endif

endif # BT_APP

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BLE features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BLE),1)

KBUILD_CPPFLAGS += -DBLE_ENABLE

KBUILD_CPPFLAGS += -D__IAG_BLE_INCLUDE__

IS_USE_BLE_DUAL_CONNECTION ?= 1

ifeq ($(IS_USE_BLE_DUAL_CONNECTION),1)
KBUILD_CPPFLAGS += -DBLE_CONNECTION_MAX=2
else
KBUILD_CPPFLAGS += -DBLE_CONNECTION_MAX=1
endif

ifeq ($(IS_ENABLE_DEUGGING_MODE),1)
KBUILD_CPPFLAGS += -DIS_ENABLE_DEUGGING_MODE
endif

endif # BLE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Speech features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
export SPEECH_TX_24BIT ?= 0
ifeq ($(SPEECH_TX_24BIT),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_24BIT
endif

export SPEECH_TX_DC_FILTER ?= 0
ifeq ($(SPEECH_TX_DC_FILTER),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_DC_FILTER
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_MIC_CALIBRATION ?= 0
ifeq ($(SPEECH_TX_MIC_CALIBRATION),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_MIC_CALIBRATION
endif

export SPEECH_TX_MIC_FIR_CALIBRATION ?= 0
ifeq ($(SPEECH_TX_MIC_FIR_CALIBRATION),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_MIC_FIR_CALIBRATION
endif

export SPEECH_TX_AEC_CODEC_REF ?= 0

export SPEECH_TX_AEC ?= 0
ifeq ($(SPEECH_TX_AEC),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

export SPEECH_TX_AEC2 ?= 0
ifeq ($(SPEECH_TX_AEC2),1)
$(error SPEECH_TX_AEC2 is not supported now, use SPEECH_TX_AEC2FLOAT instead)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

export SPEECH_TX_AEC3 ?= 0
ifeq ($(SPEECH_TX_AEC3),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

export SPEECH_TX_AEC2FLOAT ?= 0
ifeq ($(SPEECH_TX_AEC2FLOAT),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AEC2FLOAT
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

ifeq ($(SCO_DMA_SNAPSHOT),0)
export SPEECH_TX_AEC_CODEC_REF := 0
endif

export SPEECH_TX_NS ?= 0
ifeq ($(SPEECH_TX_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_NS2 ?= 0
ifeq ($(SPEECH_TX_NS2),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
KBUILD_CPPFLAGS += -DLC_MMSE_FRAME_LENGTH=$(LC_MMSE_FRAME_LENGTH)
endif

export SPEECH_TX_NS2FLOAT ?= 0
ifeq ($(SPEECH_TX_NS2FLOAT),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS2FLOAT
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_TX_NS3 ?= 0
ifeq ($(SPEECH_TX_NS3),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NS3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_WNR ?= 0
ifeq ($(SPEECH_TX_WNR),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_WNR
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_CS_VAD ?= 0
ifeq ($(SPEECH_CS_VAD),1)
KBUILD_CPPFLAGS += -DSPEECH_CS_VAD
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 1

export SPEECH_TX_2MIC_NS ?= 0
ifeq ($(SPEECH_TX_2MIC_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_2MIC_NS2 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS2),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
KBUILD_CPPFLAGS += -DCOH_FRAME_LENGTH=$(COH_FRAME_LENGTH)
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_2MIC_NS3 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS3),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_2MIC_NS4 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS4),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS4
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC

ifeq ($(BONE_SENSOR_TDM),1)
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 1
else
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

endif

export SPEECH_TX_2MIC_NS5 ?= 0
ifeq ($(SPEECH_TX_2MIC_NS5),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_2MIC_NS5
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
endif

export SPEECH_TX_3MIC_NS ?= 0
ifeq ($(SPEECH_TX_3MIC_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_3MIC_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
ifeq ($(BONE_SENSOR_TDM),1)
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
else
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 3
endif
endif

export SPEECH_TX_3MIC_NS3 ?= 0
ifeq ($(SPEECH_TX_3MIC_NS3),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_3MIC_NS3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 3
endif

export SPEECH_TX_THIRDPARTY_ALANGO ?= 0
ifeq ($(SPEECH_TX_THIRDPARTY_ALANGO),1)
export SPEECH_TX_THIRDPARTY := 1
core-y += thirdparty/alango_lib/
endif

export SPEECH_TX_THIRDPARTY ?= 0
ifeq ($(SPEECH_TX_THIRDPARTY),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_THIRDPARTY
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
# Get 1 channel from sensor
export SPEECH_CODEC_CAPTURE_CHANNEL_NUM = 2
ifeq ($(CHIP_HAS_EC_CODEC_REF),1)
export SPEECH_TX_AEC_CODEC_REF := 1
endif
endif

export SPEECH_TX_NOISE_GATE ?= 0
ifeq ($(SPEECH_TX_NOISE_GATE),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_NOISE_GATE
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_COMPEXP ?= 0
ifeq ($(SPEECH_TX_COMPEXP),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_COMPEXP
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_AGC ?= 0
ifeq ($(SPEECH_TX_AGC),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_AGC
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_TX_EQ ?= 0
ifeq ($(SPEECH_TX_EQ),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_EQ
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_TX_POST_GAIN ?= 0
ifeq ($(SPEECH_TX_POST_GAIN),1)
KBUILD_CPPFLAGS += -DSPEECH_TX_POST_GAIN
endif

export SPEECH_RX_NS ?= 0
ifeq ($(SPEECH_RX_NS),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_RX_NS2 ?= 0
ifeq ($(SPEECH_RX_NS2),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS2
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_RX_NS2FLOAT ?= 0
ifeq ($(SPEECH_RX_NS2FLOAT),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS2FLOAT
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_RX_NS3 ?= 0
ifeq ($(SPEECH_RX_NS3),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_NS3
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_RX_AGC ?= 0
ifeq ($(SPEECH_RX_AGC),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_AGC
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
endif

export SPEECH_RX_EQ ?= 0
ifeq ($(SPEECH_RX_EQ),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_EQ
KBUILD_CPPFLAGS += -DHFP_DISABLE_NREC
export DSP_LIB ?= 1
endif

export SPEECH_RX_POST_GAIN ?= 0
ifeq ($(SPEECH_RX_POST_GAIN),1)
KBUILD_CPPFLAGS += -DSPEECH_RX_POST_GAIN
endif

export SPEECH_RX_24BIT ?= 0
ifeq ($(SPEECH_RX_EQ),1)
# enable 24bit to fix low level signal distortion
export SPEECH_RX_24BIT = 1
endif

export WL_UI ?= 0
ifeq ($(WL_UI),1)
KBUILD_CPPFLAGS += -DWL_UI
endif


export SPEECH_PROCESS_FRAME_MS 	?= 16
ifeq ($(SPEECH_CODEC_CAPTURE_CHANNEL_NUM),1)
export SPEECH_PROCESS_FRAME_MS = 15
endif
ifeq ($(SPEECH_TX_2MIC_NS2),1)
export SPEECH_PROCESS_FRAME_MS = 15
endif
ifeq ($(SPEECH_TX_THIRDPARTY),1)
export SPEECH_PROCESS_FRAME_MS = 15
endif

export SPEECH_SCO_FRAME_MS 		?= 15

export SPEECH_SIDETONE ?= 0
ifeq ($(SPEECH_SIDETONE),1)
KBUILD_CPPFLAGS += -DSPEECH_SIDETONE
ifeq ($(CHIP),best2000)
# Disable SCO resample
export SW_SCO_RESAMPLE := 0
export NO_SCO_RESAMPLE := 1
endif
endif

ifeq ($(THIRDPARTY_LIB),aispeech)
export DSP_LIB ?= 1
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Features for full application projects
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(FULL_APP_PROJECT),1)

export BESLIB_INFO := $(GIT_REVISION)

export SPEECH_LIB ?= 1

export FLASH_PROTECTION ?= 1

export APP_TEST_AUDIO ?= 0

export APP_TEST_MODE ?= 0
ifeq ($(APP_TEST_MODE),1)
KBUILD_CPPFLAGS += -DAPP_TEST_MODE
endif

export VOICE_PROMPT ?= 1

export AUDIO_QUEUE_SUPPORT ?= 1

export VOICE_RECOGNITION ?= 0

export ENGINEER_MODE ?= 1
ifeq ($(ENGINEER_MODE),1)
FACTORY_MODE := 1
endif
ifeq ($(FACTORY_MODE),1)
KBUILD_CPPFLAGS += -D__FACTORY_MODE_SUPPORT__
endif

export NEW_NV_RECORD_ENABLED ?= 1
ifeq ($(NEW_NV_RECORD_ENABLED),1)
KBUILD_CPPFLAGS += -DNEW_NV_RECORD_ENABLED
KBUILD_CPPFLAGS += -Iservices/nv_section/userdata_section
endif

ifeq ($(HEAR_THRU_PEAK_DET),1)
KBUILD_CPPFLAGS += -D__HEAR_THRU_PEAK_DET__
endif

KBUILD_CPPFLAGS += -DSPEECH_PROCESS_FRAME_MS=$(SPEECH_PROCESS_FRAME_MS)
KBUILD_CPPFLAGS += -DSPEECH_SCO_FRAME_MS=$(SPEECH_SCO_FRAME_MS)

KBUILD_CPPFLAGS += -DMULTIPOINT_DUAL_SLAVE

endif # FULL_APP_PROJECT

ifeq ($(SPEECH_LIB),1)

export DSP_LIB ?= 1

ifeq ($(USB_AUDIO_APP),1)
ifneq ($(USB_AUDIO_SEND_CHAN),$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM))
$(info )
$(info CAUTION: Change USB_AUDIO_SEND_CHAN($(USB_AUDIO_SEND_CHAN)) to SPEECH_CODEC_CAPTURE_CHANNEL_NUM($(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)))
$(info )
export USB_AUDIO_SEND_CHAN := $(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)
ifneq ($(USB_AUDIO_SEND_CHAN),$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM))
$(error ERROR: Failed to change USB_AUDIO_SEND_CHAN($(USB_AUDIO_SEND_CHAN)))
endif
endif
endif

KBUILD_CPPFLAGS += -DSPEECH_CODEC_CAPTURE_CHANNEL_NUM=$(SPEECH_CODEC_CAPTURE_CHANNEL_NUM)
KBUILD_CPPFLAGS += -DSPEECH_ASR_CAPTURE_CHANNEL_NUM=$(SPEECH_ASR_CAPTURE_CHANNEL_NUM)
endif # SPEECH_LIB

export USB_AUDIO_SPEECH ?= 0
ifeq ($(USB_AUDIO_SPEECH),1)
export USB_AUDIO_DYN_CFG := 0
export KEEP_SAME_LATENCY := 1
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BISTO feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BISTO_ENABLE),1)

KBUILD_CFLAGS += -DBISTO_ENABLED

KBUILD_CPPFLAGS += -DCFG_SW_KEY_LPRESS_THRESH_MS=1000

KBUILD_CPPFLAGS += -DDEBUG_BLE_DATAPATH=0

KBUILD_CFLAGS += -DCRC32_OF_IMAGE

#export OPUS_CODEC ?= 1
#ENCODING_ALGORITHM_OPUS                2
#ENCODING_ALGORITHM_SBC                 3
#KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=3

# As MIX_MIC_DURING_MUSIC uses the isolated audio stream, if define MIX_MIC_DURING_MUSIC,
# the isolated audio stream must be enabled
KBUILD_CPPFLAGS += -DISOLATED_AUDIO_STREAM_ENABLED=1

ASSERT_SHOW_FILE_FUNC ?= 1

#KBUILD_CPPFLAGS += -DSAVING_AUDIO_DATA_TO_SD_ENABLED=1

KBUILD_CPPFLAGS += -DIS_GSOUND_BUTTION_HANDLER_WORKAROUND_ENABLED

ifeq ($(GSOUND_HOTWORD_ENABLE), 1)
KBUILD_CFLAGS += -DGSOUND_HOTWORD_ENABLED
ifeq ($(GSOUND_HOTWORD_EXTERNAL), 1)
KBUILD_CFLAGS += -DGSOUND_HOTWORD_EXTERNAL
endif
endif

REDUCED_GUESTURE_ENABLE ?= 0

BISTO_USE_TWS_API ?= 1
ifeq ($(BISTO_USE_TWS_API), 1)
KBUILD_CPPFLAGS += -DBISTO_USE_TWS_API
endif

ifeq ($(REDUCED_GUESTURE_ENABLE), 1)
KBUILD_CFLAGS += -DREDUCED_GUESTURE_ENABLED
endif
endif # BISTO_ENABLE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# GFPS feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(GFPS_ENABLE),1)

KBUILD_CPPFLAGS += -DGFPS_ENABLED
export BLE_SECURITY_ENABLED := 1

# this macro is used to determain if the resolveable private address is used for BLE
KBUILD_CPPFLAGS += -DBLE_USE_RPA

endif # GFPS

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# AMA feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(AMA_VOICE),1)

KBUILD_CPPFLAGS += -D__AMA_VOICE__
KBUILD_CPPFLAGS += -DBLE_USE_RPA

ifeq ($(CHIP),best1400)
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=3
export OPUS_CODEC ?= 0
else
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC := 1
endif
endif # AMA_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# DMA feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(DMA_VOICE),1)
KBUILD_CPPFLAGS += -D__DMA_VOICE__

KBUILD_CPPFLAGS += -D__BES__
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
KBUILD_CPPFLAGS += -DNVREC_BAIDU_DATA_SECTION
KBUILD_CPPFLAGS += -DBAIDU_DATA_RAND_LEN=8
KBUILD_CPPFLAGS += -DFM_MIN_FREQ=875
KBUILD_CPPFLAGS += -DFM_MAX_FREQ=1079
KBUILD_CPPFLAGS += -DBAIDU_DATA_DEF_FM_FREQ=893
KBUILD_CPPFLAGS += -DBAIDU_DATA_SN_LEN=16
export OPUS_CODEC := 1
export SHA256_ROM ?= 1
export LIBC_ROM := 0
ifeq ($(LIBC_ROM),1)
$(error LIBC_ROM should be 0 when DMA_VOICE=1)
endif
endif # DMA_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# GMA feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(GMA_VOICE),1)
export AI_VOICE ?= 1
KBUILD_CPPFLAGS += -D__GMA_VOICE__

#KBUILD_CPPFLAGS += -DKEYWORD_WAKEUP_ENABLED=0
#KBUILD_CPPFLAGS += -DPUSH_AND_HOLD_ENABLED=1
#KBUILD_CPPFLAGS += -DAI_32KBPS_VOICE=0
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC := 1
KBUILD_CPPFLAGS += -D__GMA_OTA_TWS__
#KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
KBUILD_CPPFLAGS += -DGMA_HOLD_ACTIVE_ENABLE
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# SMART_VOICE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(SMART_VOICE),1)
KBUILD_CPPFLAGS += -D__SMART_VOICE__
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC := 1
#SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 2
#KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
#KBUILD_CPPFLAGS += -DSPEECH_CAPTURE_TWO_CHANNEL
endif # SMART_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# TENCENT_VOICE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(TENCENT_VOICE),1)
KBUILD_CPPFLAGS += -D__TENCENT_VOICE__
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC := 1
endif # TENCENT_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# CUSTOMIZE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(CUSTOMIZE_VOICE),1)
KBUILD_CPPFLAGS += -D__CUSTOMIZE_VOICE__
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
export OPUS_CODEC := 1
#SPEECH_CODEC_CAPTURE_CHANNEL_NUM ?= 2
#KBUILD_CPPFLAGS += -DMCU_HIGH_PERFORMANCE_MODE
#KBUILD_CPPFLAGS += -DSPEECH_CAPTURE_TWO_CHANNEL
endif # CUSTOMIZE_VOICE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Dual MIC recording feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(DUAL_MIC_RECORDING),1)
KBUILD_CPPFLAGS += -D__DUAL_MIC_RECORDING__
KBUILD_CPPFLAGS += -DVOB_ENCODING_ALGORITHM=2
ifeq ($(LOWER_BANDWIDTH),1)
KBUILD_CPPFLAGS += -D__LOWER_BANDWIDTH__
endif # LOWER_BANDWIDTH
export OPUS_CODEC := 1
endif # DUAL_MIC_RECORDING

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# AI_VOICE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(AI_VOICE),1)
KBUILD_CPPFLAGS += -D__AI_VOICE__
KBUILD_CPPFLAGS += -DNO_ENCODING=0
KBUILD_CPPFLAGS += -DENCODING_ALGORITHM_ADPCM=1
KBUILD_CPPFLAGS += -DENCODING_ALGORITHM_OPUS=2
KBUILD_CPPFLAGS += -DENCODING_ALGORITHM_SBC=3
KBUILD_CPPFLAGS += -DOS_DYNAMIC_MEM_SIZE=0x6800
ifeq ($(OPUS_CODEC),1)
KBUILD_CPPFLAGS += -DOPUS_IN_OVERLAY
endif

ifeq ($(filter -DISOLATED_AUDIO_STREAM_ENABLED=1,$(KBUILD_CPPFLAGS)),)
KBUILD_CPPFLAGS += -DISOLATED_AUDIO_STREAM_ENABLED=1
endif

ifneq ($(filter -DVOB_ENCODING_ALGORITHM=2,$(KBUILD_CPPFLAGS)),)
ifneq ($(USE_KNOWLES),1)
ifneq ($(ALEXA_WWE),1)
export AF_STACK_SIZE ?= 16384
endif
endif
endif
endif # AI_VOICE

ifeq ($(THROUGH_PUT),1)
KBUILD_CPPFLAGS += -D__THROUGH_PUT__
endif # THROUGH_PUT

ifeq ($(USE_KNOWLES),1)
KBUILD_CPPFLAGS += -D__KNOWLES
KBUILD_CPPFLAGS += -DIDLE_ALEXA_KWD
export THIRDPARTY_LIB := knowles_uart
endif

AI_CAPTURE_CHANNEL_NUM ?= 0
ifneq ($(AI_CAPTURE_CHANNEL_NUM),0)
KBUILD_CPPFLAGS += -DAI_CAPTURE_CHANNEL_NUM=$(AI_CAPTURE_CHANNEL_NUM)
endif

AI_CAPTURE_DATA_AEC ?= 0
ifeq ($(AI_CAPTURE_DATA_AEC),1)
KBUILD_CPPFLAGS += -DAI_CAPTURE_DATA_AEC
KBUILD_CPPFLAGS += -DAI_ALGORITHM_ENABLE
KBUILD_CPPFLAGS += -DAEC_STERE_ON
endif

export AI_AEC_CP_ACCEL ?= 0
ifeq ($(AI_AEC_CP_ACCEL),1)
KBUILD_CPPFLAGS += -DAI_AEC_CP_ACCEL
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# MULTI_AI feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(IS_MULTI_AI_ENABLE),1)
export SHA256_ROM ?= 1
KBUILD_CPPFLAGS += -DIS_MULTI_AI_ENABLED
endif # MULTI_AI

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# VOICE_DATAPATH feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(VOICE_DATAPATH_ENABLED),1)
KBUILD_CPPFLAGS += -DVOICE_DATAPATH
endif # VOICE_DATAPATH

export SLAVE_ADV_BLE_ENABLED ?= 0
ifeq ($(SLAVE_ADV_BLE_ENABLED),1)
KBUILD_CPPFLAGS += -DSLAVE_ADV_BLE
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# TILE feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(TILE_DATAPATH_ENABLED),1)
KBUILD_CPPFLAGS += -DTILE_DATAPATH
endif

export CUSTOM_INFORMATION_TILE_ENABLE ?= 0
ifeq ($(CUSTOM_INFORMATION_TILE_ENABLE),1)
KBUILD_CPPFLAGS += -DCUSTOM_INFORMATION_TILE=1
endif # TILE

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# MFI feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(IOS_MFI),1)
KBUILD_CPPFLAGS += -DIOS_IAP2_PROTOCOL
endif # IOS_MFI

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# MIX_MIC_DURING_MUSIC feature
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(MIX_MIC_DURING_MUSIC_ENABLED),1)
KBUILD_CPPFLAGS += -DMIX_MIC_DURING_MUSIC
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Put customized features above
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Obsoleted features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
OBSOLETED_FEATURE_LIST := EQ_PROCESS RB_CODEC AUDIO_EQ_PROCESS MEDIA_PLAYER_RBCODEC
USED_OBSOLETED_FEATURE := $(strip $(foreach f,$(OBSOLETED_FEATURE_LIST),$(if $(filter 1,$($f)),$f)))
ifneq ($(USED_OBSOLETED_FEATURE),)
$(error Obsoleted features: $(USED_OBSOLETED_FEATURE))
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# Flash suspend features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(FLASH_SUSPEND), 1)
KBUILD_CPPFLAGS += -DFLASH_SUSPEND
endif

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# BLE only enable features
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ifeq ($(BLE_ONLY_ENABLED),1)
KBUILD_CPPFLAGS += -DBLE_ONLY_ENABLED
KBUILD_CPPFLAGS += -DBLE_POWER_LEVEL_0
endif

# -------------------------------------------
# General
# -------------------------------------------

ifneq ($(NO_CONFIG),1)
core-y += config/
endif

ifneq ($(NO_BOOT_STRUCT),1)
core-y += $(call add_if_exists,utils/boot_struct/)
endif

export DEFAULT_CFG_SRC ?= _default_cfg_src_

ifneq ($(wildcard $(srctree)/config/$(T)/tgt_hardware.h $(srctree)/config/$(T)/res/),)
KBUILD_CPPFLAGS += -Iconfig/$(T)
endif
KBUILD_CPPFLAGS += -Iconfig/$(DEFAULT_CFG_SRC)

CPU_CFLAGS := -mthumb
ifeq ($(CPU),a7)
CPU_CFLAGS += -march=armv7-a
else ifeq ($(CPU),m33)
ifeq ($(CPU_NO_DSP),1)
CPU_CFLAGS += -mcpu=cortex-m33+nodsp
else
CPU_CFLAGS += -mcpu=cortex-m33
endif
else
CPU_CFLAGS += -mcpu=cortex-m4
endif

export UNALIGNED_ACCESS ?= 1
ifeq ($(UNALIGNED_ACCESS),1)
KBUILD_CPPFLAGS += -DUNALIGNED_ACCESS
else
CPU_CFLAGS += -mno-unaligned-access
endif

ifeq ($(CHIP_HAS_FPU),1)
ifeq ($(CPU),a7)
CPU_CFLAGS += -mfpu=vfpv3-d16
else ifeq ($(CPU),m33)
CPU_CFLAGS += -mfpu=fpv5-sp-d16
else
CPU_CFLAGS += -mfpu=fpv4-sp-d16
endif
ifeq ($(SOFT_FLOAT_ABI),1)
CPU_CFLAGS += -mfloat-abi=softfp
else
CPU_CFLAGS += -mfloat-abi=hard
endif
else
CPU_CFLAGS += -mfloat-abi=soft
endif

# ifneq ($(ALLOW_WARNING),1)
# KBUILD_CPPFLAGS += -Werror
# endif

ifeq ($(PIE),1)
ifneq ($(TOOLCHAIN),armclang)
ifneq ($(NOSTD),1)
$(error PIE can only work when NOSTD=1)
endif
KBUILD_CPPFLAGS += -msingle-pic-base
endif
KBUILD_CPPFLAGS += -fPIE
# -pie option will generate .dynamic section
#LDFLAGS += -pie
#LDFLAGS += -z relro -z now
endif

KBUILD_CPPFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS)
LINK_CFLAGS += $(CPU_CFLAGS) $(SPECS_CFLAGS)
CFLAGS_IMAGE += $(CPU_CFLAGS) $(SPECS_CFLAGS)

# Save 100+ bytes by filling less alignment holes
# TODO: Array alignment?
#LDFLAGS += --sort-common --sort-section=alignment

ifeq ($(CTYPE_PTR_DEF),1)
ifeq ($(TOOLCHAIN),armclang)
$(error CTYPE_PTR_DEF is not supported in $(TOOLCHAIN))
else
LDFLAGS_IMAGE += --defsym __ctype_ptr__=0
endif
endif

export RAND_FROM_MIC ?= 0
ifeq ($(RAND_FROM_MIC),1)
KBUILD_CPPFLAGS += -D__RAND_FROM_MIC__
endif

export RSSI_GATHERING_ENABLED ?= 0
ifeq ($(RSSI_GATHERING_ENABLED),1)
KBUILD_CPPFLAGS += -DRSSI_GATHERING_ENABLED
endif

export TOTA ?= 0
ifeq ($(TOTA),1)
KBUILD_CPPFLAGS += -DTOTA
KBUILD_CPPFLAGS += -DRSSI_GATHERING_ENABLED
endif

export POWER_ON_ENTER_TWS_PAIRING_ENABLED ?= 0
ifeq ($(POWER_ON_ENTER_TWS_PAIRING_ENABLED),1)
KBUILD_CPPFLAGS += -DPOWER_ON_ENTER_TWS_PAIRING_ENABLED
endif
