file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (platform_drivers_btpcm STATIC ${sources} ${headers})
target_include_directories(platform_drivers_btpcm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(platform_drivers_btpcm rtos)