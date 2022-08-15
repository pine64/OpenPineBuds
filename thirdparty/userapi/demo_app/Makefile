cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)


subdir-ccflags-y +=  \
    -Ithirdparty/demo_lib \
	-Iapps/audioplayers \
	-Iapps/common \
	-Iservices/bt_app \
	-Iservices/resources \
	-Iutils/cqueue \
	-Ithirdparty/userapi
