file (GLOB headers "*.h")
file (GLOB sources "*.cpp")

add_library (apps_mic_alg STATIC ${sources} ${headers})
