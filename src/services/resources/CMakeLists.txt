file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_resources STATIC ${sources} ${headers})
target_link_libraries(services_resources util_crc32 platform_hal)
target_include_directories(services_resources PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# open_source/target.mk: VOICE_PROMPT ?= 1
set_property(SOURCE resources.cpp APPEND PROPERTY COMPILE_DEFINITIONS MEDIA_PLAYER_SUPPORT)
