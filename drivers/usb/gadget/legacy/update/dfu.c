/*
 * g_dnl.c -- USB Downloader Gadget
 *
 * Copyright (C) 2012 Samsung Electronics
 * Lukasz Majewski  <l.majewski@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/usb/composite.h>
#include <linux/module.h>

/*#include "flash_operate.c"*/
#include "f_dfu.h"
#include "usb_update.h"

#include "../gadget_chips.h"

#define CONFIG_G_DNL_VENDOR_NUM 	0x9337
#define CONFIG_G_DNL_PRODUCT_NUM	0x05a8
#define CONFIG_G_DNL_BCD	0x0010

USB_GADGET_COMPOSITE_OPTIONS();

/*#define DRIVER_VERSION		"usb_dnl 2.0"*/

static const char serialname[] = "usb_dfu";
static const char product[] = "dfu download gadget";
static const char manufacturer[] = "USB DFU";

static struct usb_device_descriptor device_desc = {
	.bLength 		= sizeof device_desc,
	.bDescriptorType 	= USB_DT_DEVICE,
	.bcdUSB 		= cpu_to_le16(0x0200),
	.bDeviceClass 		= 0x00,
	.bDeviceSubClass 	= 0x00, /*0x02:CDC-modem , 0x00:CDC-serial*/
	.bDeviceProtocol 	= 0,
	.idVendor 		= cpu_to_le16(CONFIG_G_DNL_VENDOR_NUM),
	.idProduct 		= cpu_to_le16(CONFIG_G_DNL_PRODUCT_NUM),
	.bcdDevice		= cpu_to_le16(CONFIG_G_DNL_BCD),
	.iManufacturer 		= 0,
	.iProduct 		= 0,
	.iSerialNumber 		= 0,
	.bNumConfigurations 	= 1,
};

/* static strings, in UTF-8 */
static struct usb_string g_dnl_string_defs[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = manufacturer,
	[USB_GADGET_PRODUCT_IDX].s = product,
	[USB_GADGET_SERIAL_IDX].s = serialname,
	{  } /* end of list */
};

static struct usb_gadget_strings g_dnl_string_tab = {
	.language	= 0x0409,	/* en-us */
	.strings	= g_dnl_string_defs,
};

static struct usb_gadget_strings *g_dnl_composite_strings[] = {
	&g_dnl_string_tab,
	NULL,
};

static int g_dnl_unbind(struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;

	usb_gadget_disconnect(gadget);

	return 0;
}

static int dfu_config_bind(struct usb_configuration *c)
{
	const char *s = c->cdev->driver->name;
	int ret = -1;
	printk("GADGET DRIVER: %s\n", s);
	ret = dfu_add(c);

	return ret;
}

static struct usb_configuration dfu_config_driver = {
	.label 			= "usb_dfuload",
	.bConfigurationValue 	= 1,
	.iConfiguration 	= 2,
	.bmAttributes 		= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.MaxPower       	= 0xc8
};


static int g_dnl_bind(struct usb_composite_dev *cdev)
{
	int status;
	status = usb_string_ids_tab(cdev, g_dnl_string_defs);
	if (status < 0)
		return status;
	device_desc.iManufacturer = g_dnl_string_defs[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = g_dnl_string_defs[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber = g_dnl_string_defs[USB_GADGET_SERIAL_IDX].id;

	/* register our configuration */
	status = usb_add_config(cdev, &dfu_config_driver, dfu_config_bind);
	if (status < 0)
		return status;
	usb_composite_overwrite_options(cdev, &coverwrite);

	return 0;

}

static struct usb_composite_driver g_dnl_driver = {
	.name		= "g_dfu",
	.dev 		= &device_desc,
	.strings 	= g_dnl_composite_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind 		= g_dnl_bind,
	.unbind 	= g_dnl_unbind,
};

static int __init g_dnl_register(void)
{
	int ret;
	ret = usb_composite_probe(&g_dnl_driver);

	if (ret) {
		printk("%s: failed!, error: %d\n", __func__, ret);
		return ret;
	}
	printk("################222222####\n");

	return 0;
}

static void __exit g_dnl_unregister(void)
{
	usb_composite_unregister(&g_dnl_driver);
}

module_init(g_dnl_register);
module_exit(g_dnl_unregister);
