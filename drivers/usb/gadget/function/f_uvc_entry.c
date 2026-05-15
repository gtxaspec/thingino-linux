/*
 *	uvc_gadget.c  --  USB Video Class Gadget driver
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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>

#include "u_uvc.h"
#include "f_uvc.h"

static struct usb_function_instance *uvc_alloc_inst(void)
{
	return uvc_alloc_inst_callback();
}


static struct usb_function *uvc_alloc(struct usb_function_instance *fi)
{
	return uvc_alloc_callback(fi);
}

#ifdef CONFIG_DUAL_VIDEO
static struct usb_function_instance *uvc2_alloc_inst(void)
{
	return uvc2_alloc_inst_callback();
}


static struct usb_function *uvc2_alloc(struct usb_function_instance *fi)
{
	return uvc2_alloc_callback(fi);
}
#endif

DECLARE_USB_FUNCTION_INIT(uvc, uvc_alloc_inst, uvc_alloc);
#ifdef CONFIG_DUAL_VIDEO
DECLARE_USB_FUNCTION_INIT(uvc2, uvc2_alloc_inst, uvc2_alloc);
#endif
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laurent Pinchart");
