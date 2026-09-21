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
)
