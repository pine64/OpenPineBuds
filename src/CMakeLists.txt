# CHIP/CPU/FLASH_SIZE/OPB_TARGET_CFG_DIR/etc. are set by
# cmake/target-defaults.cmake (included from the top-level CMakeLists.txt).

# include_directories() only affects subdirectories added *after* this point,
# so this must come before any add_subdirectory() call below.
include_directories (include)
# src/config/common.mk: KERNEL=RTX -> KBUILD_CPPFLAGS += -Iinclude/rtos/rtx/
include_directories (include/rtos/rtx)
# src/Makefile: KBUILD_CPPFLAGS += -Iconfig/$(DEFAULT_CFG_SRC) — applied
# globally since most drivers include tgt_hardware.h without linking the
# `config` target (Kbuild has no per-target include-path scoping). The
# per-target config dir (set above) takes precedence when it defines a file
# the default doesn't (see config/CMakeLists.txt's override-else-default).
include_directories (config/${OPB_TARGET_CFG_DIR})
include_directories (config/_default_cfg_src_)

if(OPB_ENABLE_RUST)
    add_subdirectory(rust)
endif()

add_subdirectory(apps)
add_subdirectory(config)
add_subdirectory(services)
add_subdirectory(utils)
add_subdirectory(platform)
add_subdirectory(rtos)
add_subdirectory(thirdparty)
