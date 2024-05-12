cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj-y := $(obj-y:.c=.o)

ifeq ($(CRC32_ROM),1)
ccflags-y += -Iutils/rom_utils
EXCLUDE_OBJS := crc32.o
else
EXCLUDE_OBJS := crc32_rom.o
endif

obj-y := $(filter-out $(EXCLUDE_OBJS),$(obj-y))

ifeq ($(MAKE_CRC32_TABLE),1)
ccflags-y += -DMAKE_CRC32_TABLE
endif

