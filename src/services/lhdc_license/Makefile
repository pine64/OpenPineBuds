cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

LHDC_LICENSE_LIB_NAME := lib_lhdc_license
$(LHDC_LICENSE_LIB_NAME)-y := $(src_obj)
obj-y += $(LHDC_LICENSE_LIB_NAME).a

subdir-ccflags-y += -Iplatform/drivers/ana \
					-Iplatform/hal \
					-Iservices/audioflinger \
					-Iutils/lockcqueue \
					-Iservices/bt_app/a2dp_codecs/include\
					-Iservices/bt_if_enhanced/inc
					



					
