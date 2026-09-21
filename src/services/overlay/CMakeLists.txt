file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_overlay STATIC ${sources} ${headers})
target_link_libraries(services_overlay platform_hal)
target_include_directories(services_overlay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
