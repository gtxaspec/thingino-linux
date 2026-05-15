
SENSOR_HEX = $(ZRT_ENV_SENSOR).hex

obj-$(CONFIG_VIDEO_TX_ISP) += $(SENSOR_HEX)
$(obj)/$(SENSOR_HEX):$(obj)/lib.a

$(obj)/lib.a:$(obj)/libtxisp-t31.hex
ifeq ($(ZRT_ENV_TOOL_CHAIN), 5.4.0)
	cp $(obj)/libtxisp-t31-540.hex $(obj)/lib.a
else
	cp $(obj)/libtxisp-t31.hex $(obj)/lib.a
endif

