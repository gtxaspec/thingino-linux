EXTRA_CFLAGS += -I$(src)/include

obj-$(CONFIG_SOUND) += audio_dsp.o audio_debug.o

#ifeq (${CONFIG_JZ_TS_DMIC}, y)
obj-$(CONFIG_SOUND) += host/t40/audio_aic.o host/t40/audio_dmic.o
obj-$(CONFIG_SOUND) += inner_codecs/t40/codec.o
obj-$(CONFIG_SOUND) += boards/t40_platform.o

#endif
