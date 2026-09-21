# Chip/target/kernel selection. Included from the top-level CMakeLists.txt
# (not src/CMakeLists.txt) so that both src/ and test/smoke/ — which need
# the same CHIP/FLASH_SIZE/etc. compile definitions — see the same values;
# directory-scoped set()/add_compile_definitions() only cascade to child
# directories, and test/smoke/ is a sibling of src/, not a child of it.
#
# TODO(Phase 2): split into cmake/chips/<chip>.cmake (selected via an
# OPB_CHIP cache variable) and cmake/targets/<target>.cmake (OPB_TARGET),
# once more than one chip/target is ported. For now this only models
# CHIP=best2300p / T=open_source, matching what CI builds today.
set(CHIP best2300p)
set(CHIP_BEST2300P 1)
set(CHIP_ID_LITERAL best2300p)
set(CPU m4) # src/config/common.mk: CHIP=best2300p -> CPU := m4
set(VCODEC_VOLT 1.8V) # src/config/open_source/target.mk (non-CURRENT_TEST default)

set(OPB_TARGET_CFG_DIR open_source CACHE STRING "Per-target config override directory under src/config")
set(ROM_UTILS_ON 0)      # open_source/target.mk: ROM_UTILS_ON ?= 0
set(FLASH_SIZE 0x400000) # open_source/target.mk: FLASH_SIZE ?= 0x400000

add_compile_definitions(
    CHIP=${CHIP}
    CHIP_BEST2300P=${CHIP_BEST2300P}
    CHIP_ID_LITERAL=${CHIP_ID_LITERAL}
    # src/config/common.mk: RTOS=1 (default), CPU=m4 -> KERNEL ?= RTX
    RTOS
    KERNEL_RTX
    __RTX_CPU_STATISTICS__=1
    # src/config/common.mk CHIP=best2300p capability table. These gate
    # whole driver files (e.g. hal_uart.c/hal_i2c.c compile to nothing
    # without CHIP_HAS_UART/CHIP_HAS_I2C defined), so they're not optional.
    CHIP_HAS_UART=3
    CHIP_HAS_I2C=2
    CHIP_HAS_USB
    CHIP_HAS_CP
    # src/config/common.mk: FULL_APP_PROJECT=1 (core-y includes apps/) ->
    # NEW_NV_RECORD_ENABLED ?= 1
    NEW_NV_RECORD_ENABLED
    # open_source/target.mk: used across several services/ Makefiles as a
    # whole-directory ccflags-y, not just one file — kept global here.
    AUDIO_OUTPUT_VOLUME_DEFAULT=12
    FLASH_SUSPEND
    # src/config/common.mk: CHIP not in {best1000,best2000} -> version 2
    AUD_SECTION_STRUCT_VERSION=2
    # open_source/target.mk: export A2DP_DECODER_VER := 2 (applied globally
    # via src/Makefile's KBUILD_CPPFLAGS, not per-file)
    A2DP_DECODER_VER=2
    SHOW_RSSI # src/Makefile: always added to KBUILD_CPPFLAGS
    # open_source/target.mk: DEBUG ?= 1
    DEBUG
    # common.mk "BT features" (BT_APP=1) block — these translate `?=`-style
    # Make variables into unconditional -D flags, and were missed because
    # they were only tracked here as CMake variables for target naming
    # (e.g. IMPORTED-library filenames), not also passed to the compiler.
    ENHANCED_STACK       # ENHANCED_STACK=1
    __2M_PACK__          # BT_RF_PREFER=2M -> -D__$(RF_PREFER)_PACK__
    _SCO_BTPCM_CHANNEL_  # AUDIO_SCO_BTPCM_CHANNEL=1
    __A2DP_PLAYER_USE_BT_TRIGGER__ # A2DP_PLAYER_USE_BT_TRIGGER ?= 1
    A2DP_CP_ACCEL        # A2DP_CP_ACCEL=1
    SCO_CP_ACCEL         # SCO_CP_ACCEL=1
    USE_CMSIS_F32_FFT    # SCO_CP_ACCEL=1 implies this too
    BT_XTAL_SYNC_NEW_METHOD FIXED_BIT_OFFSET_TARGET # BT_XTAL_SYNC=1
    IBRT_BLOCKED IBRT_NOT_USE __A2DP_AUDIO_SYNC_FIX_DIFF_NOPID__ # IBRT=1
    TWS_SYSTEM_ENABLED   # derived from IBRT=1
    SUPPORT_BATTERY_REPORT SUPPORT_HF_INDICATORS SUPPORT_SIRI
    # common.mk FULL_APP_PROJECT=1 block
    MEDIA_PLAYER_SUPPORT # VOICE_PROMPT=1 (also matches resources.cpp's own copy)
    MULTIPOINT_DUAL_SLAVE
    SPEECH_PROCESS_FRAME_MS=16
    SPEECH_SCO_FRAME_MS=15
    # common.mk SPEECH_LIB=1 block (SPEECH_LIB derived true: FULL_APP_PROJECT=1)
    SPEECH_CODEC_CAPTURE_CHANNEL_NUM=1
    # apps/main/Makefile: APP_ANC_KEY=1 (open_source/target.mk)
    __BT_ANC_KEY__
)
include_directories(${CMAKE_SOURCE_DIR}/src/services/nv_section/userdata_section)
# ble_stack isn't built (BLE=0 for open_source — see src/services/Makefile),
# but several non-BLE files pull small shared headers (e.g. co_math.h) out
# of its common/api tree regardless, matching Makefiles that -I it even
# when the rest of ble_stack is never compiled for this target.
include_directories(${CMAKE_SOURCE_DIR}/src/services/ble_stack/common/api)
include_directories(${CMAKE_SOURCE_DIR}/src/services/ble_stack/ble_ip)
# same story for services/ble_app/app_main/app.h, referenced from bt_app
include_directories(${CMAKE_SOURCE_DIR}/src/services/ble_app/app_main)
