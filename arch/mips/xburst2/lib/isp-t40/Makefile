
SENSOR_HEX = $(ZRT_ENV_SENSOR).hex

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
SENSOR1_HEX = $(ZRT_ENV_SENSOR1).hex
endif

EXTRA_CFLAGS += -I$(PWD)/include



all: modules

obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t40.hex
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast.o
obj-$(CONFIG_VIDEO_TX_ISP) += $(SENSOR_HEX) 
ifeq ($(ZRT_ENV_SENSOR_NUM), two)
obj-$(CONFIG_VIDEO_TX_ISP) += $(SENSOR1_HEX) 
endif
$(obj)/$(SENSOR_HEX):$(obj)/lib.a

$(obj)/lib.a:$(obj)/libtxisp-t40.hex
	cp $(obj)/libtxisp-t40.hex $(obj)/lib.a

