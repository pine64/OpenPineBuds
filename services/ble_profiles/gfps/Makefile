cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gfps_crypto/*.c))

#CRYPTO_LIB_NAME := libgfps_crypto
#$(CRYPTO_LIB_NAME)-y := $(obj_c:.c=.o)

libgfps_crypto-y := $(obj_c:.c=.o)
obj-y := libgfps_crypto.a
subdir-ccflags-y += \
	-Igfps_crypto/ \
	-Iutils/encrypt \
	-Iapi/



