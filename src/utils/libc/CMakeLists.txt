# src/utils/libc/Makefile: NOSTD=0 (default) -> obj-y := libc_rom.o only,
# built whenever LIBC_ROM=1 (open_source/target.mk default). ROM_UTILS_ON=0
# for open_source, so only export_fn_rom.h's declarations are needed, not a
# compiled utils/rom_utils library (that only exists when ROM_UTILS_ON=1).
add_library (util_libc STATIC libc_rom.c)
target_include_directories(util_libc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(util_libc PRIVATE
    ${CMAKE_SOURCE_DIR}/src/utils/rom_utils
    ${CMAKE_SOURCE_DIR}/src/platform/hal
)
target_compile_definitions(util_libc PRIVATE NO_STRTOL)
