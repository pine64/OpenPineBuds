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
)
include_directories(${CMAKE_SOURCE_DIR}/src/services/nv_section/userdata_section)
# ble_stack isn't built (BLE=0 for open_source — see src/services/Makefile),
# but several non-BLE files pull small shared headers (e.g. co_math.h) out
# of its common/api tree regardless, matching Makefiles that -I it even
# when the rest of ble_stack is never compiled for this target.
include_directories(${CMAKE_SOURCE_DIR}/src/services/ble_stack/common/api)
include_directories(${CMAKE_SOURCE_DIR}/src/services/ble_stack/ble_ip)
