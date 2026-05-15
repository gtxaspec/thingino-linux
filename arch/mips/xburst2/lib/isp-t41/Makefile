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

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
obj-$(CONFIG_VIDEO_TX_ISP) += dual_sensor/$(SENSOR1_HEX)
obj-$(CONFIG_VIDEO_TX_ISP) += dual_sensor/$(SENSOR_HEX)
else
obj-$(CONFIG_VIDEO_TX_ISP) += single_sensor/$(SENSOR_HEX)
endif

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast-double.o
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t41-double.hex
else
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t41-double-direct.hex
endif
else
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast.o
obj-$(CONFIG_INGENIC_NUM_SENSOR) += ${ZRT_ENV_SENSOR_MULTI_0}.hex
obj-$(CONFIG_INGENIC_NUM_SENSOR) += ${ZRT_ENV_SENSOR_MULTI_1}.hex
#obj-$(CONFIG_INGENIC_NUM_SENSOR) += ${ZRT_ENV_SENSOR_MULTI_2}.hex
obj-$(CONFIG_INGENIC_NUM_SENSOR) += common.hex
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t41.hex
else
obj-$(CONFIG_VIDEO_TX_ISP) += libtxisp-t41-direct.hex
endif
endif

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
$(obj)/dual_sensor/$(SENSOR_HEX):$(obj)/lib.a
else
$(obj)/single_sensor/$(SENSOR_HEX):$(obj)/lib.a
endif

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
	cp $(obj)/libtxisp-t41-double.hex $(obj)/lib.a
else
	cp $(obj)/libtxisp-t41-double-direct.hex $(obj)/lib.a
endif#ZRT_ENV_IVDC_MODE
else
ifeq ($(ZRT_ENV_IVDC_MODE), un-direct)
	cp $(obj)/libtxisp-t41.hex $(obj)/lib.a
else
	cp $(obj)/libtxisp-t41-direct.hex $(obj)/lib.a
endif
endif
$(obj)/lib.a:$(obj)/libtxisp-t41.hex 
