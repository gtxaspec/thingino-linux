SENSOR_HEX = $(ZRT_ENV_SENSOR).hex
SENSOR1_HEX = $(ZRT_ENV_SENSOR1).hex
SENSOR2_HEX = $(ZRT_ENV_SENSOR2).hex
SENSOR3_HEX = $(ZRT_ENV_SENSOR3).hex
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-funcs.o

obj-$(CONFIG_VIDEO_TX_ISP) += isp_osd.o

obj-$(CONFIG_VIDEO_TX_ISP) += ingenic_ipu_hal.o
obj-$(CONFIG_VIDEO_TX_ISP) += ingenic_osd.o

obj-$(CONFIG_VIDEO_TX_ISP) += osd_common.o

obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-fast.o
obj-$(CONFIG_VIDEO_TX_ISP) += sensor/$(SENSOR_HEX)
ifeq ($(ZRT_ENV_SENSOR_NUM), 2)
obj-$(CONFIG_VIDEO_TX_ISP) += sensor/$(SENSOR1_HEX)
endif
ifeq ($(ZRT_ENV_SENSOR_NUM), 3)
obj-$(CONFIG_VIDEO_TX_ISP) += sensor/$(SENSOR1_HEX)
obj-$(CONFIG_VIDEO_TX_ISP) += sensor/$(SENSOR2_HEX)
endif
ifeq ($(ZRT_ENV_SENSOR_NUM), 4)
obj-$(CONFIG_VIDEO_TX_ISP) += sensor/$(SENSOR1_HEX)
obj-$(CONFIG_VIDEO_TX_ISP) += sensor/$(SENSOR2_HEX)
obj-$(CONFIG_VIDEO_TX_ISP) += sensor/$(SENSOR3_HEX)
endif
$(obj)/sensor/$(SENSOR_HEX):$(obj)/lib.a


$(obj)/lib.a : FORCE
	@echo "现在正在链接isp."
	cp $(obj)/libPRJ007-firmware.hex $(obj)/lib.a

.PHONY : FORCE
