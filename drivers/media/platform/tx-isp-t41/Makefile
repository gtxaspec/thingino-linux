# Makefile for tx-isp video driver

ifeq ($(ZRT_ENV_SENSOR_NUM), two)
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-t41-device-double.o
else
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-t41-device.o
endif
