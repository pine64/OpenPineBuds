# apps/Makefile: obj-y := audioplayers/ common/ main/ key/ pwl/ battery/
# factory/ (unconditional), anc/ only if ANC_APP=1, apptester/ only if
# APP_TEST_AUDIO=1 (unset for open_source), cmd/ only if PC_CMD_UART=1
# (0 for open_source).
add_subdirectory (anc)
add_subdirectory (audioplayers)
add_subdirectory (battery)
add_subdirectory (common)
add_subdirectory (factory)
add_subdirectory (key)
add_subdirectory (main)
add_subdirectory (pwl)
# add_subdirectory (apptester) # APP_TEST_AUDIO=0 for open_source
# add_subdirectory (cmd)       # PC_CMD_UART=0 for open_source
