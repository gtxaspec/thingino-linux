# Makefile for APICAL ISP video driver

CFLAGS_KERNEL += -I$(PWD)/include/linux/tx-isp/apical-isp

obj-$(CONFIG_APICAL_ISP) += tx-isp-core-tuning.o tx-isp-core.o apical_dynamic_calibrations.o apical_static_calibrations.o \
				system_isp_io.o log.o system_io.o apical_calibrations.o apical_calibrations_init.o tx-isp-load-parameters.o \
				apical_custom_initialization.o ApicalDefault.o \
				system_i2c.o system_spi.o system_timer.o system_uart.o system_semaphore.o sensor_drv.o system_chardev.o
