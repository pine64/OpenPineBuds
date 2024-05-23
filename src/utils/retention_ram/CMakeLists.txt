file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_retention_ram STATIC ${sources} ${headers})
target_link_libraries(util_retention_ram platform_hal)
target_include_directories(util_retention_ram PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})