cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

ifeq ($(SHA256_ROM),1)
ccflags-y += -Iutils/rom_utils
EXCLUDE_OBJS := sha256.o
else
EXCLUDE_OBJS := sha256_rom.o
endif

obj-y := $(filter-out $(EXCLUDE_OBJS),$(obj-y))

