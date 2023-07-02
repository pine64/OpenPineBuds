obj-y := userapi/

ifeq ($(TILE_DATAPATH_ENABLED), 1)
obj-y += tile
endif

ifeq ($(THIRDPARTY_LIB),)
THIRDPARTY_LIB := demo
$(info demo)
else ifeq ($(THIRDPARTY_LIB),voicespot)
KBUILD_CPPFLAGS += -D_VOICESPOT_
endif

obj-y += $(foreach lib,$(THIRDPARTY_LIB),$(lib)_lib/)

