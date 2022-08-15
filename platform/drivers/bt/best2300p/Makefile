cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

CFLAGS_bt_drv_uart_bridge_intsys.o += $(BT_DRV_UART_BRIDGE_FLAGS)

CFLAGS_bt_drv.o += $(BT_DRV_FLAGS)

CFLAGS_bt_drv_patch.o += $(BT_DRV_PATCH_FLAGS)

CFLAGS_bt_drv_config.o += $(BT_DRV_CONFIG_FLAGS)

TX_IQ_CAL ?= 0
ifeq ($(TX_IQ_CAL),1)
subdir-ccflags-y += -DTX_IQ_CAL
endif

subdir-ccflags-y += -Iutils/crash_catcher/include
subdir-ccflags-y += -Iservices/nv_section/userdata_section
ccflags-y += $(BT_IF_INCLUDES)
