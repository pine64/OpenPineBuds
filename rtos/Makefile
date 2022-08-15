ifeq ($(KERNEL),RTX)
rtos_lib-y := rtx/TARGET_CORTEX_M/
else ifeq ($(KERNEL),RTX5)
rtos_lib-y := rtx5/
else ifeq ($(KERNEL),FREERTOS)
rtos_lib-y := freertos/
endif

ifeq ($(MBED),1)
rtos_lib-y += rtos/
endif

obj-y := rtos_lib.o

subdir-ccflags-y += -Iplatform/cmsis/inc -Iplatform/hal -Iutils/hwtimer_list

