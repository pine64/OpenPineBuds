file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (apps_voice_detector STATIC ${sources} ${headers})
