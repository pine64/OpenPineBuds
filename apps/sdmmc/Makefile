cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

subdir-ccflags-y += -Iservices/fs/sd \
					-Iservices/fs/fat \
					-Iservices/fs/fat/ChaN

ifeq ($(APP_TEST_SDMMC),1)
ccflags-y += -D__APP_TEST_SDMMC__
endif
