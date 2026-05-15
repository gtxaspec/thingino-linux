ifeq (${CONFIG_JZ_TS_DMIC}, y)
obj-$(CONFIG_SOUND) += devices/ex_codecs/codec_i2c_dev.o devices/ex_codecs/es8374_codec.o devices/codecs/jz_t10_codec.o devices/xb47xx_i2s_v12.o	\
					   devices/dmic/mic.o devices/dmic/mic_sys.o devices/dmic/mic_ops.o devices/dmic/mic_dma.o devices/dmic/mic_hrtimer.o\
					   devices/dmic/dmic_hal.o\
					   interface/xb_snd_dsp.o xb_snd_card.o
else
obj-$(CONFIG_SOUND) += devices/ex_codecs/codec_i2c_dev.o devices/ex_codecs/es8374_codec.o devices/codecs/jz_t10_codec.o devices/xb47xx_i2s_v12.o	\
						interface/xb_snd_dsp.o xb_snd_card.o
endif
