file (GLOB headers "*.h")
file (GLOB sources "*.cpp")

add_library (apps_mic STATIC ${sources} ${headers})
