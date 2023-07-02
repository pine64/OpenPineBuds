cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y += GCC/

obj-y += rtx_config/

CFLAGS_rtx_lib.o += -DOS_IDLE_THREAD_NAME=\"idle\" \
                    -DOS_TIMER_THREAD_NAME=\"timer\"

subdir-ccflags-y += -DOS_CLOCK_NOMINAL=$(OS_CLOCK_NOMINAL)

ifneq ($(OS_IDLESTKSIZE),)
subdir-ccflags-y += -DOS_IDLE_THREAD_STACK_SIZE=$(OS_IDLESTKSIZE)
endif

ifneq ($(OS_FIFOSZ),)
subdir-ccflags-y += -DOS_ISR_FIFO_QUEUE=$(OS_FIFOSZ)
endif

ifneq ($(OS_SCHEDULERSTKSIZE),)
CFLAGS_rtx_lib.o += -DOS_SCHEDULERSTKSIZE=$(OS_SCHEDULERSTKSIZE)
endif

ifneq ($(DEBUG_SLEEP),)
subdir-ccflags-y += -DDEBUG_SLEEP=$(DEBUG_SLEEP)
endif

subdir-ccflags-y += -Irtos/rtx5/rtx_config

