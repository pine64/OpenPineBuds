file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_norflash_api STATIC ${sources} ${headers})
target_link_libraries(services_norflash_api platform_hal util_heap platform_drivers_ana)
target_include_directories(services_norflash_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# services/norflash_api/Makefile
set_property(SOURCE norflash_api.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    FLASH_SUSPEND    # open_source/target.mk: FLASH_SUSPEND ?= 1
    OTA_CODE_OFFSET=${OPB_LDS_OTA_CODE_OFFSET}
    FLASH_API_NORMAL # default (FLASH_API_SIMPLE/HIGHPERFORMANCE unset)
)
