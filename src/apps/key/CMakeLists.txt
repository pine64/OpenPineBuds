file (GLOB headers "*.h")
file (GLOB sources "*.cpp")

add_library (apps_key STATIC ${sources} ${headers})
target_link_libraries(apps_key platform_hal)