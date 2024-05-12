set(sources "sha256.c")
if(SHA256_ROM)
    set(sources "sha256_rom.c")
endif(SHA256_ROM)

add_library (util_sha256 STATIC ${sources} "sha256.h" "hash-internal.h")
target_include_directories(util_sha256 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(util_sha256 platform_cmsis platform_hal)

if(SHA256_ROM)
    target_link_libraries(util_sha256 util_rom_utils)
endif(SHA256_ROM)
