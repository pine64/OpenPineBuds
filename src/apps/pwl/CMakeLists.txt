file (GLOB headers "*.h")
file (GLOB sources "*.cpp")

add_library (apps_pwl STATIC ${sources} ${headers})
