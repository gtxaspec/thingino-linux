/*
 *	ucam_driver.c -- USB camera gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/usb/composite.h>
#include <linux/module.h>
#include <linux/poll.h>

#include "u_serial_update.h"

USB_GADGET_COMPOSITE_OPTIONS();
/* --------------------------------------------------------------------------
 * Device descriptor
 */

static unsigned int CDC_VENDOR_ID   =  0x291a;	/* Linux Foundation */
static unsigned int CDC_PRODUCT_ID  =  0x3369;	/* Ucam A/V gadget */
static unsigned int CDC_DEVICE_BCD  =  0x0100;	/* 0.10 */


#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static char cdc_vendor_label[64] = "CDC Serial Device";
static char cdc_product_label[64] = "CDC Serial Device";
static char cdc_serial_label[64] = "CDC Serial C100";
static char cdc_config_label[] = "C100";

/* string IDs are assigned dynamically */

static struct usb_string cdc_strings[] = {
	[USB_GADGET_PRODUCT_IDX].s = cdc_product_label,
	[USB_GADGET_MANUFACTURER_IDX].s = cdc_vendor_label,
	[USB_GADGET_SERIAL_IDX].s = cdc_serial_label,
	{  }
};

static struct usb_gadget_strings cdc_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = cdc_strings,
};

static struct usb_gadget_strings *cdc_device_strings[] = {
	&cdc_stringtab,
	NULL,
};

static struct usb_device_descriptor cdc_device_descriptor = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	.bcdUSB			= cpu_to_le16(0x0200),
	.bDeviceClass		= USB_CLASS_MISC,
	.bDeviceSubClass	= 0x02,
	.bDeviceProtocol	= 0x01,
	.bMaxPacketSize0	= 0, /* dynamic */
	/* .idVendor		= cpu_to_le16(CDC_VENDOR_ID), */
	/* .idProduct		= cpu_to_le16(CDC_PRODUCT_ID), */
	/* .bcdDevice		= cpu_to_le16(CDC_DEVICE_BCD), */
	.iManufacturer		= 0, /* dynamic */
	.iProduct		= 0, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations	= 0, /* dynamic */
};

/* --------------------------------------------------------------------------
 * USB configuration
 */


static int
cdc_config_bind(struct usb_configuration *c)
{
	int ret;

	ret = ucam_bind_acm(c);
		if(ret)
			printk("cdc bind acm func failed\n");

	return ret;
}

static struct usb_configuration cdc_config_driver = {
	.label			= cdc_config_label,
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= 0xC8,
};

static int /* __init_or_exit */
cdc_unbind(struct usb_composite_dev *cdev)
{
	acm_cleanup();
	return 0;
}

static int
cdc_bind(struct usb_composite_dev *cdev)
{
	int ret;

	/* Allocate string descriptor numbers ... note that string contents
	 * can be overridden by the composite_dev glue.
	 */
	ret = usb_string_ids_tab(cdev, cdc_strings);
	if (ret < 0)
		goto error;
	cdc_device_descriptor.iManufacturer =
		cdc_strings[USB_GADGET_MANUFACTURER_IDX].id;
	cdc_device_descriptor.iProduct =
		cdc_strings[USB_GADGET_PRODUCT_IDX].id;
	cdc_device_descriptor.iSerialNumber =
		cdc_strings[USB_GADGET_SERIAL_IDX].id;


	ret =  acm_mod_init();
		if (ret)
			printk("%s: Failed to init acm", __func__);

	/* Register our configuration. */
	if ((ret = usb_add_config(cdev, &cdc_config_driver,
					cdc_config_bind)) < 0)
		goto error;

	usb_composite_overwrite_options(cdev, &coverwrite);
	return 0;

error:
	cdc_unbind(cdev);
	return ret;
}

/* --------------------------------------------------------------------------
 * Driver
 */

static __refdata struct usb_composite_driver cdc_driver = {
	.name		= "g_cdc_serial",
	.dev		= &cdc_device_descriptor,
	/* .dev		= &adb_device_desc, */
	.strings	= cdc_device_strings,
	.max_speed	= USB_SPEED_SUPER,
	.bind		= cdc_bind,
	.unbind		= cdc_unbind,
};

static int
cdc_driver_init(void)
{
	int status;

	cdc_device_descriptor.idVendor = cpu_to_le16(CDC_VENDOR_ID);
	cdc_device_descriptor.idProduct = cpu_to_le16(CDC_PRODUCT_ID);
	cdc_device_descriptor.bcdDevice = cpu_to_le16(CDC_DEVICE_BCD);

	status = usb_composite_probe(&cdc_driver);
	if (status < 0)
		return status;

	return status;
}

static void
cdc_driver_cleanup(void)
{
	usb_composite_unregister(&cdc_driver);
}

static int __init cdc_serial_init(void)
{
	int ret = 0;
	ret = cdc_driver_init();
	if(ret < 0){
		printk("ERROR:cdc driver init is failed\n");
		return ret;
	}
	printk("#########3cdc serial init ########\n");

	return ret;
}

static void __exit cdc_serial_exit(void)
{
	cdc_driver_cleanup();
}

module_init(cdc_serial_init);
module_exit(cdc_serial_exit);

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Ucamera Video Gadget");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.0");

