ifeq ($(CONFIG_SOC_PRJ008),y)
SOC_TYPE = PRJ008
else
ifeq ($(CONFIG_SOC_PRJ007),y)
SOC_TYPE = PRJ007
endif
endif

ifeq ($(CONFIG_PM), y)
SENSOR_HEX = $(ZRT_ENV_SENSOR)_pm.hex
SENSOR1_HEX = $(ZRT_ENV_SENSOR1)_pm.hex
SENSOR2_HEX = $(ZRT_ENV_SENSOR2)_pm.hex
SENSOR3_HEX = $(ZRT_ENV_SENSOR3)_pm.hex
else
SENSOR_HEX = $(ZRT_ENV_SENSOR).hex
SENSOR1_HEX = $(ZRT_ENV_SENSOR1).hex
SENSOR2_HEX = $(ZRT_ENV_SENSOR2).hex
SENSOR3_HEX = $(ZRT_ENV_SENSOR3).hex
endif

obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-funcs.o
obj-$(CONFIG_VIDEO_TX_ISP) += isp_osd.o
obj-$(CONFIG_VIDEO_TX_ISP) += ingenic_ipu_hal.o
obj-$(CONFIG_VIDEO_TX_ISP) += ingenic_osd.o
obj-$(CONFIG_VIDEO_TX_ISP) += osd_common.o
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast.o

obj-$(CONFIG_VIDEO_TX_ISP) += sensor_$(SOC_TYPE)/$(SENSOR_HEX)
ifeq ($(ZRT_ENV_SENSOR_NUM), 2)
    obj-$(CONFIG_VIDEO_TX_ISP) += sensor_$(SOC_TYPE)/$(SENSOR1_HEX)
endif
ifeq ($(ZRT_ENV_SENSOR_NUM), 3)
    obj-$(CONFIG_VIDEO_TX_ISP) += sensor_$(SOC_TYPE)/$(SENSOR1_HEX)
    obj-$(CONFIG_VIDEO_TX_ISP) += sensor_$(SOC_TYPE)/$(SENSOR2_HEX)
endif
ifeq ($(ZRT_ENV_SENSOR_NUM), 4)
    obj-$(CONFIG_VIDEO_TX_ISP) += sensor_$(SOC_TYPE)/$(SENSOR1_HEX)
    obj-$(CONFIG_VIDEO_TX_ISP) += sensor_$(SOC_TYPE)/$(SENSOR2_HEX)
    obj-$(CONFIG_VIDEO_TX_ISP) += sensor_$(SOC_TYPE)/$(SENSOR3_HEX)
endif

$(obj)/sensor_$(SOC_TYPE)/$(SENSOR_HEX):$(obj)/lib.a

$(obj)/lib.a : FORCE
ifeq ($(CONFIG_PM),y)
ifeq ($(CONFIG_SOC_PRJ008),y)
	@echo "现在正在链接 PRJ008 pm isp."
	cp  $(obj)/libPRJ008-firmware-pm.hex $(obj)/lib.a
else
ifeq ($(CONFIG_SOC_PRJ007),y)
	@echo "现在正在链接 PRJ007 pm isp."
	cp  $(obj)/libPRJ007-firmware-pm.hex $(obj)/lib.a
endif
endif
else
ifeq ($(CONFIG_SOC_PRJ008),y)
	@echo "现在正在链接 PRJ008 isp."
	cp  $(obj)/libPRJ008-firmware.hex $(obj)/lib.a
else
ifeq ($(CONFIG_SOC_PRJ007),y)
	@echo "现在正在链接 PRJ007 isp."
	cp  $(obj)/libPRJ007-firmware.hex $(obj)/lib.a
endif
endif
endif

.PHONY : FORCE
