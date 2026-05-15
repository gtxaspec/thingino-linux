#
# USB peripheral controller drivers
#
subdir-ccflags-$(CONFIG_USB_GADGET_DEBUG)	:= -DDEBUG
subdir-ccflags-$(CONFIG_USB_GADGET_VERBOSE)	+= -DVERBOSE_DEBUG
ccflags-y				+= -I$(srctree)/drivers/usb/gadget/udc

obj-$(CONFIG_SUPPORT_WINHELLO)	+= libwin_hello.o
libwin_hello-y			:= lib_winhello.a
$(obj)/lib_winhello.a: $(obj)/lib_winhello.hex
	cp $(obj)/lib_winhello.hex $(obj)/lib_winhello.a

obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o configfs.o u_f.o

obj-$(CONFIG_USB_GADGET)	+= udc/ function/ legacy/
ifeq ($(CONFIG_SUPPORT_WINHELLO), y)
libcomposite-y			+= lib_winhello.a
endif
