
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)TARGET_M4/TOOLCHAIN_GCC/*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y += -DOS_CLOCK_NOMINAL=$(OS_CLOCK_NOMINAL)

ifneq ($(OS_TASKCNT),)
CFLAGS_RTX_Conf_CM.o += -DOS_TASKCNT=$(OS_TASKCNT)
endif

ifneq ($(OS_SCHEDULERSTKSIZE),)
CFLAGS_RTX_Conf_CM.o += -DOS_SCHEDULERSTKSIZE=$(OS_SCHEDULERSTKSIZE)
endif

ifneq ($(OS_IDLESTKSIZE),)
CFLAGS_RTX_Conf_CM.o += -DOS_IDLESTKSIZE=$(OS_IDLESTKSIZE)
endif

ifneq ($(OS_FIFOSZ),)
CFLAGS_RTX_Conf_CM.o += -DOS_FIFOSZ=$(OS_FIFOSZ)
endif

ifneq ($(DEBUG_SLEEP),)
CFLAGS_RTX_Conf_CM.o += -DDEBUG_SLEEP=$(DEBUG_SLEEP)
CFLAGS_rt_System.o += -DDEBUG_SLEEP=$(DEBUG_SLEEP)
endif

ifeq ($(OS_API_IN_INTLOCK),1)
CFLAGS_rt_CMSIS.o += -DOS_API_IN_INTLOCK
endif

