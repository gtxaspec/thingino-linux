# Makefile for tx-isp video driver

CFLAGS_KERNEL += -I$(PWD)/include/linux/tx-isp/

ifeq ($(ZRT_ENV_SENSOR),ov9732)
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_OV9732
export CONFIG_SENSOR_DVP_OV9732=y

else ifeq ($(ZRT_ENV_SENSOR),sc1135)
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_SC1135
export CONFIG_SENSOR_DVP_SC1135=y

else ifeq ($(ZRT_ENV_SENSOR),ps5250)
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_PS5250
export CONFIG_SENSOR_DVP_PS5250=y

else ifeq ($(ZRT_ENV_SENSOR),jxh62)
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_JXH62
export CONFIG_SENSOR_DVP_JXH62=y

else ifeq ($(ZRT_ENV_SENSOR),imx291)
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_IMX291
export CONFIG_SENSOR_DVP_IMX291=y

else ifeq ($(ZRT_ENV_SENSOR),sc2235)
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_SC2235
export CONFIG_SENSOR_DVP_SC2235=y

else ifeq ($(ZRT_ENV_SENSOR),gc2033)
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_GC2033
export CONFIG_SENSOR_DVP_GC2033=y

else # default
CFLAGS_KERNEL += -DCONFIG_SENSOR_DVP_OV9732
export CONFIG_SENSOR_DVP_OV9732=y

endif



obj-$(CONFIG_VIDEO_TX_ISP) += sensors/
obj-$(CONFIG_APICAL_ISP) += apical-isp/ tx-isp-vic.o
obj-$(CONFIG_VIDEO_TX_ISP) += tx-isp-videobuf.o tx-isp-video-in.o tx-isp-frame-channel.o \
				tx-isp-csi.o tx-isp-interrupt.o tx-isp-debug.o tx-isp-device.o
