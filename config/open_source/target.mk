# Target chip/SoC selection
CHIP        ?= best2300p

# Enable debug features and symbols (1=enabled, 0=disabled)
DEBUG       ?= 1

# Use Mbed OS framework (1=enabled, 0=disabled)
MBED        ?= 0

RTOS        ?= 1

#KERNEL      ?= FREERTOS
# We have powerkey input
NO_PWRKEY    = 0
# Use ROM-based libc functions to save RAM (1=enabled, 0=disabled)
LIBC_ROM    ?= 1

# Extas added by Open source community
CONNECTED_BLUE_LIGHT = 1 # if set to 1, the blue light will flash when connected
# end our extras

export USER_SECURE_BOOT	?= 0
# enable:1
# disable:0

# Enable hardware watchdog timer to detect and recover from system hangs (1=enabled, 0=disabled)
WATCHER_DOG ?= 0

DEBUG_PORT  ?= 1
# 0: usb
# 1: uart0
# 2: uart1

FLASH_CHIP	?= ALL
# GD25Q80C
# GD25Q32C
# ALL

# Disable timestamps in trace/log output to reduce log size (1=disabled, 0=enabled)
export NO_TRACE_TIME_STAMP ?=1

# Force Bluetooth signaling mode for testing (1=enabled, 0=disabled)
export FORCE_SIGNALINGMODE ?= 0

# Force Bluetooth non-signaling mode for testing (1=enabled, 0=disabled)
export FORCE_NOSIGNALINGMODE ?= 0

# Force maximum retransmission count for SCO connections (1=enabled, 0=disabled)
export FORCE_SCO_MAX_RETX ?= 0

# Enable Fast ACK RX gain control for Bluetooth (1=enabled, 0=disabled)
export FA_RX_GAIN_CTRL ?= 1

# Enable Bluetooth Fast ACK Error Correction Code (1=enabled, 0=disabled)
export BT_FA_ECC ?= 0

# Enable Bluetooth controller dump for debugging (1=enabled, 0=disabled)
export CONTROLLER_DUMP_ENABLE ?= 0

# Enable Bluetooth controller memory logging (1=enabled, 0=disabled)
export CONTROLLER_MEM_LOG_ENABLE ?= 0

# Enable inter-subsystem communication debugging (1=enabled, 0=disabled)
export INTERSYS_DEBUG ?= 0

# Enable Bluetooth profile debugging (1=enabled, 0=disabled)
export PROFILE_DEBUG ?= 0

# Enable Bluetooth packet dumping for debugging (1=enabled, 0=disabled)
export BTDUMP_ENABLE ?= 0

# Enable Bluetooth debug test ports (1=enabled, 0=disabled)
export BT_DEBUG_TPORTS ?= 0

# Allow test ports and key input to coexist (1=enabled, 0=disabled)
TPORTS_KEY_COEXIST ?= 0

# Enable checking and reporting of Bluetooth sniff mode status (1=enabled, 0=disabled)
export SNIFF_MODE_CHECK ?= 0
# Merge L+R audio channels down to mono output (1=enabled, 0=disabled)
AUDIO_OUTPUT_MONO ?= 0

# Enable differential audio output mode (1=enabled, 0=disabled)
AUDIO_OUTPUT_DIFF ?= 0

# Raise mic bias from 2.2V to 3.3V
DIGMIC_HIGH_VOLT ?= 0

#### ANC DEFINE START ######
export ANC_APP		    ?= 1
# Feed Forward  ANC configuration (external mic)
export ANC_FF_ENABLED	?= 1
# Feed Backward ANC configuration (internal mic)
export ANC_FB_ENABLED	?= 1
# Wind noise reduction mode
export ANC_WNR_ENABLED ?= 0

# Music cancel mode. Conflicts with audio resampling
export AUDIO_ANC_FB_MC ?= 0
# Audio section in flash for calibration data
export AUDIO_SECTION_SUPPT ?= 0
export AUD_SECTION_STRUCT_VERSION ?= 2
# Music cancel hardware?, dont think we have it
export AUDIO_ANC_FB_MC_HW ?=0
# Key to control anc
export APP_ANC_KEY ?= 1
# Feedback check for feedforward mic. Locked on due to blobs
export ANC_FB_CHECK ?= 1
# Build in ANC testing app (closed source)
APP_ANC_TEST ?= 0
export ANC_ASSIST_ENABLED ?= 0
##### ANC DEFINE END ######

# Allow test commands via bluetooth
TEST_OVER_THE_AIR ?= 0

# Enable hardware FIR equalizer processing (1=enabled, 0=disabled)
HW_FIR_EQ_PROCESS ?= 0

# Enable software IIR equalizer processing (1=enabled, 0=disabled)
SW_IIR_EQ_PROCESS ?= 1

HW_DAC_IIR_EQ_PROCESS ?= 0

HW_IIR_EQ_PROCESS ?= 0

HW_DC_FILTER_WITH_IIR ?= 0

# Enable Dynamic Range Compression for audio output (1=enabled, 0=disabled)
AUDIO_DRC ?= 0

AUDIO_DRC2 ?= 0

# Enable PC command interface via UART (1=enabled, 0=disabled)
PC_CMD_UART ?= 0

# Enable audio section configuration storage (1=enabled, 0=disabled)
AUDIO_SECTION_ENABLE ?= 0

# Enable audio sample rate resampling (1=enabled, 0=disabled)
AUDIO_RESAMPLE ?= 0

# Allow resampling to any sample rate (1=enabled, 0=disabled)
RESAMPLE_ANY_SAMPLE_RATE ?= 1

# Enable 26MHz x4 oscillator for audio to baseband sync (1=enabled, 0=disabled)
OSC_26M_X4_AUD2BB ?= 1

AUDIO_OUTPUT_VOLUME_DEFAULT ?= 12
# range:1~16

# Enable capless mode for audio input (no DC blocking capacitor) (1=enabled, 0=disabled)
AUDIO_INPUT_CAPLESSMODE ?= 0

# Enable large gain mode for audio input (1=enabled, 0=disabled)
AUDIO_INPUT_LARGEGAIN ?= 0

# Enable asynchronous codec close to avoid blocking (1=enabled, 0=disabled)
AUDIO_CODEC_ASYNC_CLOSE ?= 0

# SCO audio BTPCM channel configuration (channel number)
AUDIO_SCO_BTPCM_CHANNEL ?= 1

# Enable A2DP codec acceleration using coprocessor (1=enabled, 0=disabled)
export A2DP_CP_ACCEL ?= 1

# Enable SCO codec acceleration using coprocessor (1=enabled, 0=disabled)
export SCO_CP_ACCEL ?= 1

# Enable SCO trace acceleration using coprocessor (1=enabled, 0=disabled)
export SCO_TRACE_CP_ACCEL ?= 0

# For TWS SCO DMA snapshot and low delay
export PCM_FAST_MODE ?= 1

# Bypass CVSD codec processing in controller (1=enabled, 0=disabled)
export CVSD_BYPASS ?= 1

export LOW_DELAY_SCO ?= 0

# Enable DC filter for speech transmission (1=enabled, 0=disabled)
SPEECH_TX_DC_FILTER ?= 1

# Enable AEC2 float processing for speech transmission (1=enabled, 0=disabled)
SPEECH_TX_AEC2FLOAT ?= 0

# Enable NS3 noise suppression for speech transmission (1=enabled, 0=disabled)
SPEECH_TX_NS3 ?= 0

# Enable 2-microphone NS2 noise suppression for speech transmission (1=enabled, 0=disabled)
SPEECH_TX_2MIC_NS2 ?= 0

# Enable compressor/expander for speech transmission (1=enabled, 0=disabled)
SPEECH_TX_COMPEXP ?= 1

# Enable equalizer for speech transmission (1=enabled, 0=disabled)
SPEECH_TX_EQ ?= 0

# Enable post-gain adjustment for speech transmission (1=enabled, 0=disabled)
SPEECH_TX_POST_GAIN ?= 0

# Enable NS2 float noise suppression for speech reception (1=enabled, 0=disabled)
SPEECH_RX_NS2FLOAT ?= 0

# Enable equalizer for speech reception (1=enabled, 0=disabled)
SPEECH_RX_EQ ?= 0

# Enable post-gain adjustment for speech reception (1=enabled, 0=disabled)
SPEECH_RX_POST_GAIN ?= 0

# Enable large RAM configuration for more memory (1=enabled, 0=disabled)
LARGE_RAM ?= 1

# Enable Headset Profile support (1=enabled, 0=disabled)
HSP_ENABLE ?= 0

# Enable Hands-Free Profile 1.6 specification support (1=enabled, 0=disabled)
HFP_1_6_ENABLE ?= 1

# Enable Packet Loss Concealment for mSBC codec (1=enabled, 0=disabled)
MSBC_PLC_ENABLE ?= 1

MSBC_PLC_ENCODER ?= 1

# Enable 16kHz sample rate for mSBC codec (1=enabled, 0=disabled)
MSBC_16K_SAMPLE_RATE ?= 1

# Are SBC codec functions in ROM to save RAM (1=enabled, 0=disabled)
SBC_FUNC_IN_ROM ?= 0

# Enable ROM utility functions (1=enabled, 0=disabled)
ROM_UTILS_ON ?= 0

# Enable line-in as A2DP source (1=enabled, 0=disabled)
APP_LINEIN_A2DP_SOURCE ?= 0

# Enable I2S input as A2DP source (1=enabled, 0=disabled)
APP_I2S_A2DP_SOURCE ?= 0

# Enable voice prompt playback support (1=enabled, 0=disabled)
VOICE_PROMPT ?= 1

# Enable throughput testing mode (1=enabled, 0=disabled)
export THROUGH_PUT ?= 0

#### Google related feature ####
# the overall google service switch
# currently, google service includes BISTO and GFPS
export GOOGLE_SERVICE_ENABLE ?= 0

# BISTO is a GVA service on Bluetooth audio device
# BISTO is an isolated service relative to GFPS
export BISTO_ENABLE ?= 0

# macro switch for reduced_guesture
export REDUCED_GUESTURE_ENABLE ?= 0

# GSOUND_HOTWORD is a hotword library running on Bluetooth audio device
# GSOUND_HOTWORD is a subset of BISTO
export GSOUND_HOTWORD_ENABLE ?= 0

# this is a subset choice for gsound hotword
export GSOUND_HOTWORD_EXTERNAL ?= 0

# GFPS is google fastpair service
# GFPS is an isolated service relative to BISTO
export GFPS_ENABLE ?= 0
#### Google related feature ####

# Enable wireless UI features (1=enabled, 0=disabled)
export WL_UI ?= 1

# Enable Bluetooth Low Energy support (1=enabled, 0=disabled)
BLE ?= 0

# Enable Test Over The Air functionality (1=enabled, 0=disabled)
TOTA ?= 0

# Enable GATT over BR/EDR (1=enabled, 0=disabled)
GATT_OVER_BR_EDR ?= 0

# Enable Over-The-Air firmware update support (1=enabled, 0=disabled)
OTA_ENABLE ?= 0

# Enable Tile datapath support (1=enabled, 0=disabled)
TILE_DATAPATH_ENABLED ?= 0

# Enable custom Tile information support (1=enabled, 0=disabled)
CUSTOM_INFORMATION_TILE_ENABLE ?= 0

# Enable device interconnection features (1=enabled, 0=disabled)
INTERCONNECTION ?= 0
# Looks like Find-My-Device support?
INTERACTION ?= 0

# Enable Fast Pair interaction support (1=enabled, 0=disabled)
INTERACTION_FASTPAIR ?= 0

# Enable simultaneous connection to two devices (1=enabled, 0=disabled)
BT_ONE_BRING_TWO ?= 0

# Enable DSD (Direct Stream Digital) audio format support (1=enabled, 0=disabled)
DSD_SUPPORT ?= 0

# Enable 24-bit processing for A2DP equalizer (1=enabled, 0=disabled)
A2DP_EQ_24BIT ?= 1

# Enable AAC codec for A2DP audio streaming (1=enabled, 0=disabled)
A2DP_AAC_ON ?= 1

# Enable Scalable codec for A2DP (1=enabled, 0=disabled)
A2DP_SCALABLE_ON ?= 0

# Enable LHDC codec for A2DP high-quality audio (1=enabled, 0=disabled)
A2DP_LHDC_ON ?= 0
ifeq ($(A2DP_LHDC_ON),1)
A2DP_LHDC_V3 ?= 1
A2DP_LHDC_LARC ?= 1
export FLASH_UNIQUE_ID ?= 1
endif

# Enable LDAC codec for A2DP high-quality audio (1=enabled, 0=disabled)
A2DP_LDAC_ON ?= 1

# Enable TX/RX PCM masking (1=enabled, 0=disabled)
export TX_RX_PCM_MASK ?= 0

A2DP_SCALABLE_ON ?= 0

# Enable factory test mode (1=enabled, 0=disabled)
FACTORY_MODE ?= 0

# Enable engineering/developer mode (1=enabled, 0=disabled)
ENGINEER_MODE ?= 0

# Enable ultra-low-power mode to reduce power consumption (1=enabled, 0=disabled)
ULTRA_LOW_POWER	?= 1

# Enable Class-G DAC for improved power efficiency (1=enabled, 0=disabled)
DAC_CLASSG_ENABLE ?= 1

# Disable sleep mode to keep system always active (1=no sleep, 0=sleep allowed)
NO_SLEEP ?= 0

# Enable core dump generation on crashes for debugging (1=enabled, 0=disabled)
CORE_DUMP ?= 0

# Store core dumps to flash memory (1=enabled, 0=disabled)
CORE_DUMP_TO_FLASH ?= 0

# Use enhanced Bluetooth stack implementation (1=enabled, 0=disabled)
ENHANCED_STACK ?= 1

# Synchronize Bluetooth controller and profile timing (1=enabled, 0=disabled)
export SYNC_BT_CTLR_PROFILE ?= 0

# Enable A2DP AVDTP content protection (1=enabled, 0=disabled)
export A2DP_AVDTP_CP ?= 0

# A2DP decoder version selection
export A2DP_DECODER_VER := 2

# Enable IBRT (In-Band Radio Technology) for TWS earbuds (1=enabled, 0=disabled)
export IBRT = 1

# Enable IBRT search UI for device pairing (1=enabled, 0=disabled)
export IBRT_SEARCH_UI ?= 1

# Enable BES audio features (1=enabled, 0=disabled)
export BES_AUD ?= 1

# Power mode configuration (DIG_DCDC=digital DC-DC converter)
export POWER_MODE   ?= DIG_DCDC

# Bluetooth RF preference (2M=2Mbps PHY rate)
export BT_RF_PREFER ?= 2M

# Enable speech codec support (1=enabled, 0=disabled)
export SPEECH_CODEC ?= 1

# Synchronize voice prompts between TWS earbuds (1=enabled, 0=disabled)
export TWS_PROMPT_SYNC ?= 0
# Mix audio prompts with A2DP media playback (1=enabled, 0=disabled)
export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED ?= 0
# Enable iOS MFi (Made for iPhone) authentication (1=enabled, 0=disabled)
export IOS_MFI ?= 0

# Flash memory size in bytes
export FLASH_SIZE ?= 0x400000
# Enable flash suspend feature during erase/write operations (1=enabled, 0=disabled)
export FLASH_SUSPEND ?= 1

ifeq ($(DSD_SUPPORT),1)
export BTUSB_AUDIO_MODE     ?= 1
export AUDIO_INPUT_MONO     ?= 1
export USB_ISO              ?= 1
export USB_AUDIO_DYN_CFG    ?= 1
export DELAY_STREAM_OPEN    ?= 0
export KEEP_SAME_LATENCY    ?= 1
export HW_FIR_DSD_PROCESS   ?= 1
ifeq ($(HW_FIR_DSD_PROCESS),1)
ifeq ($(CHIP),best2300)
export HW_FIR_DSD_BUF_MID_ADDR  ?= 0x200A0000
export DATA_BUF_START           ?= 0x20040000
endif
endif
export USB_AUDIO_UAC2 ?= 1
export USB_HIGH_SPEED ?= 1
KBUILD_CPPFLAGS += \
    -DHW_FIR_DSD_BUF_MID_ADDR=$(HW_FIR_DSD_BUF_MID_ADDR) \
    -DDATA_BUF_START=$(DATA_BUF_START)
endif

# Use third-party audio processing libraries (1=enabled, 0=disabled)
USE_THIRDPARTY ?= 0
# Use Knowles audio processing algorithms/chips (1=enabled, 0=disabled)
export USE_KNOWLES ?= 0

ifeq ($(CURRENT_TEST),1)
# Codec voltage for current testing mode
export VCODEC_VOLT ?= 1.6V
# Analog voltage for current testing mode
export VANA_VOLT ?= 1.35V
else
# Codec voltage for normal operation
export VCODEC_VOLT ?= 1.8V
# Analog voltage for normal operation
export VANA_VOLT ?= 1.35V
endif

# Enable Laurent algorithm for audio processing (1=enabled, 0=disabled)
export LAURENT_ALGORITHM ?= 0

# Enable TX IQ calibration for Bluetooth radio (1=enabled, 0=disabled)
export TX_IQ_CAL ?= 0

# Enable Bluetooth crystal oscillator synchronization (1=enabled, 0=disabled)
export BT_XTAL_SYNC ?= 1

# Use fixed Bluetooth address for debugging (1=enabled, 0=disabled)
export BTADDR_FOR_DEBUG ?= 0

# Enable power key I2C switch functionality (1=enabled, 0=disabled)
export POWERKEY_I2C_SWITCH ?=0

# Enable wireless detection (1=enabled, 0=disabled)
export WL_DET ?= 0

# Enable audio loopback for testing (1=enabled, 0=disabled)
export AUDIO_LOOPBACK ?= 0

# Enable automatic testing mode (1=enabled, 0=disabled)
AUTO_TEST ?= 0

# Enable BES automated testing (1=enabled, 0=disabled)
BES_AUTOMATE_TEST ?= 0

# Dump normal logs for debugging (1=enabled, 0=disabled)
export DUMP_NORMAL_LOG ?= 0

# Enable battery level reporting to connected devices (1=enabled, 0=disabled)
SUPPORT_BATTERY_REPORT ?= 1

# Enable HFP indicator support (1=enabled, 0=disabled)
SUPPORT_HF_INDICATORS ?= 1

# Enable Siri voice assistant support (1=enabled, 0=disabled)
SUPPORT_SIRI ?= 1

# BES audio dev board 9v0 hardware configuration (1=enabled, 0=disabled)
BES_AUDIO_DEV_Main_Board_9v0 ?= 0

# Use LED to indicate IBRT status (1=enabled, 0=disabled)
APP_USE_LED_INDICATE_IBRT_STATUS ?= 0

# Enable external LNA and PA for Bluetooth (1=enabled, 0=disabled)
export BT_EXT_LNA_PA ?=0
# Enable external LNA (Low Noise Amplifier) for Bluetooth RX (1=enabled, 0=disabled)
export BT_EXT_LNA ?=0
# Enable external PA (Power Amplifier) for Bluetooth TX (1=enabled, 0=disabled)
export BT_EXT_PA ?=0

ifeq ($(A2DP_LHDC_ON),1)
AUDIO_BUFFER_SIZE := 140*1024
else ifeq ($(A2DP_LDAC_ON),1)
AUDIO_BUFFER_SIZE := 140*1024
else
AUDIO_BUFFER_SIZE := 100*1024
endif

# Trace/debug buffer size in bytes
export TRACE_BUF_SIZE := 16*1024
# Trace UART baud rate for debug output
export TRACE_BAUD_RATE := 2000000

init-y :=
core-y := platform/ services/ apps/ utils/cqueue/ utils/list/ services/multimedia/ utils/intersyshci/

KBUILD_CPPFLAGS += \
    -Iplatform/cmsis/inc \
    -Iservices/audioflinger \
    -Iplatform/hal \
    -Iservices/fs/ \
    -Iservices/fs/sd \
    -Iservices/fs/fat \
    -Iservices/fs/fat/ChaN

KBUILD_CPPFLAGS += \
    -DAPP_AUDIO_BUFFER_SIZE=$(AUDIO_BUFFER_SIZE) \
    -DCHARGER_PLUGINOUT_RESET=1 \


ifeq ($(APP_ANC_KEY),1)
KBUILD_CPPFLAGS += -D__BT_ANC_KEY__
endif

ifeq ($(BES_AUDIO_DEV_Main_Board_9v0),1)
KBUILD_CPPFLAGS += -DBES_AUDIO_DEV_Main_Board_9v0
endif

ifeq ($(TPORTS_KEY_COEXIST),1)
KBUILD_CPPFLAGS += -DTPORTS_KEY_COEXIST
endif

#-DIBRT_LINK_LOWLAYER_MONITOR

#-D_AUTO_SWITCH_POWER_MODE__
#-D__APP_KEY_FN_STYLE_A__
#-D__APP_KEY_FN_STYLE_B__
#-D__EARPHONE_STAY_BOTH_SCAN__
#-D__POWERKEY_CTRL_ONOFF_ONLY__
#-DAUDIO_LINEIN

ifeq ($(CURRENT_TEST),1)
INTSRAM_RUN ?= 1
endif
ifeq ($(INTSRAM_RUN),1)
LDS_FILE := best1000_intsram.lds
else
LDS_FILE := best1000.lds
endif

ifeq ($(GATT_OVER_BR_EDR),1)
export GATT_OVER_BR_EDR ?= 1
KBUILD_CPPFLAGS += -D__GATT_OVER_BR_EDR__
endif

ifeq ($(TOTA),1)
ifeq ($(BLE),1)
KBUILD_CPPFLAGS += -DBLE_TOTA_ENABLED
endif
KBUILD_CPPFLAGS += -DSHOW_RSSI
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED
export TEST_OVER_THE_AIR ?= 1
endif

KBUILD_CPPFLAGS += -DSHOW_RSSI
ifneq ($(A2DP_DECODER_VER), )
KBUILD_CPPFLAGS += -DA2DP_DECODER_VER=$(A2DP_DECODER_VER)
endif

KBUILD_CPPFLAGS += \
    # -DHAL_TRACE_RX_ENABLE

KBUILD_CFLAGS +=

LIB_LDFLAGS += -lstdc++ -lsupc++

# Enable Bluetooth HID (Human Interface Device) support (1=enabled, 0=disabled)
export BTIF_HID_DEVICE ?= 0
ifeq ($(BTIF_HID_DEVICE),1)
KBUILD_CPPFLAGS += -DBTIF_HID_DEVICE
endif

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float

#LDFLAGS_IMAGE += --wrap main
