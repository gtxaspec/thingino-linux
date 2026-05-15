SENSOR_HEX = $(ZRT_ENV_SENSOR).hex

EXTRA_CFLAGS += -I$(PWD)/include


all: modules
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t41.hex
else
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t41-direct.hex
endif

obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast.o
obj-$(CONFIG_VIDEO_TX_ISP) += $(SENSOR_HEX)
$(obj)/$(SENSOR_HEX):$(obj)/lib.a
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
	cp $(obj)/libtxisp-t41.hex $(obj)/lib.a
else
	cp $(obj)/libtxisp-t41-direct.hex $(obj)/lib.a
endif

$(obj)/lib.a:$(obj)/libtxisp-t41.hex
