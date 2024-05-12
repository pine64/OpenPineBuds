file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_main STATIC ${sources} ${headers})
target_link_libraries(apps_main platform_hal)