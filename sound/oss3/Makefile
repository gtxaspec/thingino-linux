ifeq ($(CONFIG_SOC_PRJ007),y)
EXTRA_CFLAGS += -I$(src)/include
ifeq ($(CONFIG_COMPILE_JZSOUND_OSS3_INTO_KO),m)
obj-m     += audio.o

audio-objs := audio_dsp.o audio_debug.o
audio-objs += host/PRJ007/audio_aic.o
audio-objs += inner_codecs/PRJ007/codec.o
audio-objs += host/dmic/as-fmtcov/as-fmtcov.o
audio-objs += host/PRJ007/dmic/as-dbus/as-dbus.o
audio-objs += host/PRJ007/dmic/as-dma/as-dma.o
audio-objs += host/PRJ007/dmic/as-dmic/as-dmic.o
audio-objs += boards/PRJ007_platform.o
else ifeq ($(CONFIG_COMPILE_JZSOUND_OSS3_INTO_KO),y)
obj-y += audio_dsp.o audio_debug.o
obj-y += host/PRJ007/audio_aic.o
obj-y += inner_codecs/PRJ007/codec.o
obj-y += host/PRJ007/dmic/as-fmtcov/as-fmtcov.o
obj-y += host/PRJ007/dmic/as-dbus/as-dbus.o
obj-y += host/PRJ007/dmic/as-dma/as-dma.o
obj-y += host/PRJ007/dmic/as-dmic/as-dmic.o
obj-y += boards/PRJ007_platform.o
endif
else
EXTRA_CFLAGS += -I$(src)/include
ifeq ($(CONFIG_COMPILE_JZSOUND_OSS3_INTO_KO),m)
obj-m     += audio.o

audio-objs := audio_dsp.o audio_debug.o
audio-objs += host/PRJ008/audio_aic.o
audio-objs += inner_codecs/PRJ008/codec.o
audio-objs += boards/PRJ008_platform.o
else ifeq ($(CONFIG_COMPILE_JZSOUND_OSS3_INTO_KO),y)
obj-y += audio_dsp.o audio_debug.o
obj-y += host/PRJ008/audio_aic.o
obj-y += inner_codecs/PRJ008/codec.o
obj-y += boards/PRJ008_platform.o
endif
endif
