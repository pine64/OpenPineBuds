obj-y := ana/ codec/ norflash/ bt/ btpcm/ sbcacc/

subdir-ccflags-y += -Iplatform/drivers/ana  \
										-Iplatform/drivers/bt
										
VCO_TEST_TOOL ?= 1
ifeq ($(VCO_TEST_TOOL),1)
subdir-ccflags-y += -DVCO_TEST_TOOL
endif
