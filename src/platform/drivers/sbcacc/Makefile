cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(wildcard $(cur_dir)$(CHIP)/),)
obj-y += sbcaac_dummy.o

$(obj)/sbcaac_dummy.o :
	$(call if_changed,cc_dummy)
else
obj-y += $(CHIP)/
endif

subdir-ccflags-y += -Iservices/nvrecord

