add_library(util_crc16 STATIC "crc16.c" "crc16.h")
target_include_directories(util_crc16 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})