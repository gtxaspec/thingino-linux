SENSOR_HEX = $(ZRT_ENV_SENSOR).hex

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
SENSOR1_HEX = $(ZRT_ENV_SENSOR1).hex
EXTRA_CFLAGS += -I$(PWD)/include_double
else
EXTRA_CFLAGS += -I$(PWD)/include
endif

ifeq ($(ZRT_ENV_IVDC_MODE), direct)
$(shell find -name "fast_start_common.h" | xargs sed -i 's/Zeratul_Platform 1/Zeratul_Platform 0/g')
else
$(shell find -name "fast_start_common.h" | xargs sed -i 's/Zeratul_Platform 0/Zeratul_Platform 1/g')
endif

all: modules

obj-$(CONFIG_VIDEO_TX_ISP) += $(SENSOR_HEX)
ifeq ($(ZRT_ENV_SENSOR_NUM), two)
obj-$(CONFIG_VIDEO_TX_ISP) += $(SENSOR1_HEX)
endif

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast-double.o
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t23-double.hex
else
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t23-double-direct.hex
endif
else
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast.o
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t23.hex
else
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t23-direct.hex
endif
endif

$(obj)/$(SENSOR_HEX):$(obj)/lib.a

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
	cp $(obj)/libtxisp-t23-double.hex $(obj)/lib.a
else
	cp $(obj)/libtxisp-t23-double-direct.hex $(obj)/lib.a
endif
else
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
	cp $(obj)/libtxisp-t23.hex $(obj)/lib.a
else
	cp $(obj)/libtxisp-t23-direct.hex $(obj)/lib.a
endif
endif
$(obj)/lib.a:$(obj)/libtxisp-t23.hex
