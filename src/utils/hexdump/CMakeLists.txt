file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_hexdump STATIC ${sources} ${headers})

if(NOT __ARM_EABI__)
target_link_libraries(util_hexdump platform_hal)
endif(NOT __ARM_EABI__)

target_include_directories(util_hexdump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util_hexdump PRIVATE ${CMAKE_SOURCE_DIR}/hal/)