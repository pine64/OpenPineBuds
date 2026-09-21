# platform/hal/Makefile: obj-y += $(CHIP)/ — hal_cmu_*/hal_analogif_*/
# hal_iomux_*/hal_codec_*/hal_psc_*/hal_sensor_eng_* live in the per-chip
# subdirectory, matching platform/drivers/{ana,codec,norflash}.
file (GLOB headers "*.h" "${CHIP}/*.h")
file (GLOB sources "*.c*" "${CHIP}/*.c")

add_library (platform_hal STATIC ${sources} ${headers})
target_include_directories(platform_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${CHIP})
# TODO(Phase 3): util_crash_catcher needs services/nv_section/log_section
# (coredump_section.h) which isn't ported yet; hal_trace.c calls into it, so
# re-add once that's converted (the final image link will otherwise report
# undefined CrashCatcher references, matching CORE_DUMP=1's core-y addition).
target_link_libraries(platform_hal platform_drivers_ana platform_cmsis platform_drivers_norflash util_boot_struct)
target_include_directories(platform_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(platform_hal PRIVATE
    ${CMAKE_SOURCE_DIR}/platform/drivers/spi_norflash/
    ${CMAKE_SOURCE_DIR}/tests/programmer/inc/
)

# platform/hal/Makefile: per-file -Iutils/hwtimer_list (not a whole-target
# dependency — hal doesn't link against util_hwtimer_list, which itself
# links against platform_hal, so this must stay source-scoped to avoid a
# circular target dependency)
foreach(_f hal_key.c hal_usbhost.c)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_f})
        set_property(SOURCE ${_f} APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/src/utils/hwtimer_list)
    endif()
endforeach()

# platform/hal/Makefile: per-file -DFLASH_SIZE=$(FLASH_SIZE)
foreach(_f hal_norflash.c hal_trace.c)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_f})
        set_property(SOURCE ${_f} APPEND PROPERTY COMPILE_DEFINITIONS FLASH_SIZE=${FLASH_SIZE})
    endif()
endforeach()
