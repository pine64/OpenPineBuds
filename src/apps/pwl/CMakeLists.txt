file (GLOB headers "*.h")
file (GLOB sources "*.cpp")

add_library (apps_pwl STATIC ${sources} ${headers})
target_link_libraries(apps_pwl platform_hal platform_drivers_ana)
