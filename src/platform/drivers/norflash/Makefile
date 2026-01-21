cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y := -I$(obj)/inc -Itests/programmer/inc -Iutils/boot_struct

CFLAGS_norflash_drv.o += -DCHIP_FLASH_CTRL_VER=$(CHIP_FLASH_CTRL_VER)

ifeq ($(FLASH_HPM),1)
ccflags-y += -DFLASH_HPM
endif

ifeq ($(FLASH_CALIB_DEBUG),1)
CFLAGS_norflash_drv.o += -DFLASH_CALIB_DEBUG
endif
ifeq ($(FLASH_SUSPEND),1)
CFLAGS_norflash_drv.o += -DFLASH_SUSPEND
endif
ifeq ($(FLASH_SECURITY_REGISTER),1)
CFLAGS_norflash_drv.o += -DFLASH_SECURITY_REGISTER
endif

ifeq ($(PSRAM_ENABLE),1)
CFLAGS_norflash_drv.o += -DPSRAM_ENABLE
endif
ifeq ($(PSRAMUHS_ENABLE),1)
CFLAGS_norflash_drv.o += -DPSRAMUHS_ENABLE
endif

ifeq ($(FLASH_CHIP),)
ifeq ($(PROGRAMMER),1)
FLASH_CHIP := ALL
else
FLASH_CHIP := GD25Q32C
endif
endif
ifeq ($(FLASH_CHIP),)
$(error *** Error: FLASH_CHIP is NOT defined!)
endif

CFLAGS_norflash_drv.o += $(foreach flash,$(FLASH_CHIP),-D__NORFLASH_$(flash)__)

