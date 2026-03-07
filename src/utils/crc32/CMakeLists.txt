set(sources "crc32.c")
if(CRC32_ROM)
    set(sources "crc32_rom.c")
endif(CRC32_ROM)


add_library (util_crc32 STATIC ${sources} "crc32.h")
target_include_directories(util_crc32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CRC32_ROM)
    target_link_libraries(util_crc32 util_rom_utils)
endif(CRC32_ROM)
