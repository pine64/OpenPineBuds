
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

mmlib_obj :=

mmlib_obj += algorithm/fft/src/

mmlib_obj += audio/process/adp/src/
mmlib_obj += audio/process/anc/src/
mmlib_obj += audio/process/fir2iir/src/
mmlib_obj += audio/process/common/src/
mmlib_obj += audio/process/drc/src/
mmlib_obj += audio/process/filters/src/
mmlib_obj += audio/process/limiter/src/
mmlib_obj += audio/process/resample/src/
mmlib_obj += audio/process/integer_resampling/src/
mmlib_obj += audio/process/floatlimiter/src/
mmlib_obj += audio/process/adj_mc/src/

ifeq ($(FM_ENABLE),1)
mmlib_obj += fm/
endif

ifeq ($(A2DP_OPUS_ON),1)
#mmlib_obj += opus121/src/
endif

ifeq ($(BT_APP),1)
mmlib_obj += audio/codec/sbc/src/
endif

ifeq ($(A2DP_AAC_ON),1)
aac_obj :=
ifeq ($(FDKAAC_VERSION),2)
aac_obj += audio/codec/fdkaac_codec_2_0_1/libAACdec/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libAACenc/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libFDK/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libMpegTPDec/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libMpegTPEnc/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libPCMutils/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libSBRdec/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libSBRenc/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libSACdec/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libSACenc/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libDRCdec/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libArithCoding/src/
aac_obj += audio/codec/fdkaac_codec_2_0_1/libSYS/src/
else
aac_obj += audio/codec/fdkaac_codec/libAACdec/src/
aac_obj += audio/codec/fdkaac_codec/libAACenc/src/
aac_obj += audio/codec/fdkaac_codec/libFDK/src/
aac_obj += audio/codec/fdkaac_codec/libMpegTPDec/src/
aac_obj += audio/codec/fdkaac_codec/libMpegTPEnc/src/
aac_obj += audio/codec/fdkaac_codec/libPCMutils/src/
aac_obj += audio/codec/fdkaac_codec/libSBRdec/src/
aac_obj += audio/codec/fdkaac_codec/libSBRenc/src/
aac_obj += audio/codec/fdkaac_codec/libSYS/src/
endif
ifeq ($(A2DP_CP_ACCEL),1)
fdkaac_codec-y := $(aac_obj)
mmlib_obj += fdkaac_codec.o
else
mmlib_obj += $(aac_obj)
endif
ifeq ($(ROM_UTILS_ON),1)
subdir-ccflags-y += -DFDK_FUNC_ATTR=WEAK
else
subdir-ccflags-y += -DFDK_FUNC_ATTR=
endif
endif

#mmlib_obj += rbcodec/src/

ifeq ($(SPEECH_LIB),1)
mmlib_obj += speech/src/
endif

MULTIMEDIA_LIB_NAME := $(CHIP)_libmultimedia

ifeq ($(ROM_UTILS_ON),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_romaac
endif

ifeq ($(A2DP_CP_ACCEL),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_cp
endif

ifeq ($(ANC_APP),1)
MULTIMEDIA_LIB_NAME := $(MULTIMEDIA_LIB_NAME)_anc
endif

$(MULTIMEDIA_LIB_NAME)-y := $(mmlib_obj)

rel_src_obj :=
rel_src_obj += audio/process/anc/cfg/
rel_src_obj += audio/process/filters/cfg/
rel_src_obj += audio/process/resample/coef/

obj-y := $(MULTIMEDIA_LIB_NAME).a $(rel_src_obj)

subdir-ccflags-y += \
	-Iutils/heap \

	