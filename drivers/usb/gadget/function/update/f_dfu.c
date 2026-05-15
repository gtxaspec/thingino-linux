/*
 * f_dfu.c -- Device Firmware Update USB function
 *
 * Copyright (C) 2012 Samsung Electronics
 * authors: Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *          Lukasz Majewski <l.majewski@samsung.com>
 *
 * Based on OpenMoko u-boot: drivers/usb/usbdfu.c
 * (C) 2007 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * based on existing SAM7DFU code from OpenPCD:
 * (C) Copyright 2006 by Harald Welte <hwelte at hmw-consulting.de>
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>

#include <asm/byteorder.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <asm/unaligned.h>

#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/reboot.h>
#include <linux/gpio.h>

#include "f_dfu.h"
#include "usb_update.h"

struct f_dfu_buf {
	char *buf;
	int actual;
	uint64_t partition_address;
	uint64_t partition_size;
};


struct f_dfu {
	struct usb_function		usb_function;

	struct usb_descriptor_header	**function;
	struct usb_string		*strings;
	struct f_dfu_buf *copy_buf;
	struct work_struct write_flash_work;
	/*struct work_struct erase_flash_work;*/

	/* when configured, we have one config */
	u8				config;
	u8				altsetting;
	enum dfu_state			dfu_state;
	unsigned int			dfu_status;

	/* Send/received block number is handy for data integrity check */
	int                             blk_seq_num;
};

typedef int (*dfu_state_fn) (struct f_dfu *,
			     const struct usb_ctrlrequest *,
			     struct usb_gadget *,
			     struct usb_request *);

/*
 *struct erase_info {
 *        struct mtd_info *mtd;
 *        uint64_t addr;
 *        uint64_t len;
 *        uint64_t fail_addr;
 *        u_long time;
 *        u_long retries;
 *        unsigned dev;
 *        unsigned cell;
 *        void (*callback) (struct erase_info *self);
 *        u_long priv;
 *        u_char state;
 *        struct erase_info *next;
 *};
 */


static inline struct f_dfu *func_to_dfu(struct usb_function *f)
{
	return container_of(f, struct f_dfu, usb_function);
}

static const struct dfu_function_descriptor dfu_func = {
	.bLength =		sizeof dfu_func,
	.bDescriptorType =	DFU_DT_FUNC,
	.bmAttributes =		DFU_BIT_WILL_DETACH |
				DFU_BIT_MANIFESTATION_TOLERANT |
				DFU_BIT_CAN_UPLOAD |
				DFU_BIT_CAN_DNLOAD,
	.wDetachTimeOut =	0,
	.wTransferSize =	DFU_USB_BUFSIZ,
	.bcdDFUVersion =	__constant_cpu_to_le16(0x0110),
};

static struct usb_interface_descriptor dfu_intf_runtime = {
	.bLength =		sizeof dfu_intf_runtime,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_APP_SPEC,
	.bInterfaceSubClass =	1,
	.bInterfaceProtocol =	1,
	/* .iInterface = DYNAMIC */
};

/*
 *static struct usb_interface_descriptor dfu_intf_runtime_uboot = {
 *        .bLength =		sizeof dfu_intf_runtime,
 *        .bDescriptorType =	USB_DT_INTERFACE,
 *        .bNumEndpoints =	0,
 *        .bInterfaceClass =	USB_CLASS_APP_SPEC,
 *        .bInterfaceSubClass =	1,
 *        .bInterfaceProtocol =	1,
 *        [> .iInterface = DYNAMIC <]
 *};
 */
static const struct usb_qualifier_descriptor dev_qualifier = {
	.bLength 	         = sizeof dev_qualifier,
	.bDescriptorType 	 = USB_DT_DEVICE_QUALIFIER,
	.bcdUSB 	  	 = __constant_cpu_to_le16(0x0200),
	.bDeviceClass 		 = USB_CLASS_VENDOR_SPEC,
	.bDeviceSubClass 	 = 0,
	.bDeviceProtocol 	 = 0,
	.bMaxPacketSize0 	 = __constant_cpu_to_le16(0x0040),
	.bNumConfigurations	 =	1,
	.bRESERVED		 =	0,
};

static struct usb_descriptor_header *dfu_runtime_descs[] = {
	(struct usb_descriptor_header *) &dfu_intf_runtime,
	NULL,
};

/*
 * static strings, in UTF-8
 *
 * dfu_generic configuration
 */
static struct usb_string strings_dfu_generic[] = {
	[0].s = "control",
	[1].s = "uboot",
	[2].s = "kernel",
	[3].s = "rootfs",
	[4].s = "appfs",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dfu_generic = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dfu_generic,
};

static struct usb_gadget_strings *dfu_generic_strings[] = {
	&stringtab_dfu_generic,
	NULL,
};


/*-------------------------------------------------------------------------*/

/*
 *extern int recovery_norflash_erase(struct erase_info *instr);
 *extern int recovery_norflash_read(loff_t from, size_t len, unsigned char *buf);
 *extern int recovery_norflash_write(loff_t to, size_t len, const unsigned char *buf);
 *
 *static int read_flash(void *buf, int offset, int size)
 *{
 *        recovery_norflash_read((loff_t)offset, (size_t)size, (unsigned char *)buf);
 *        return 0;
 *}
 *
 *static int erase_flash(int offset, int size)
 *{
 *        struct erase_info instr;
 *        instr.addr = (unsigned long long)offset;
 *        instr.len = (unsigned long long)size;
 *        recovery_norflash_erase(&instr);
 *
 *        return 0;
 *}
 *
 *static int write_flash(void *buf, int deviation, int size)
 *{
 *        int offset = 0;
 *        offset = deviation;
 *        recovery_norflash_write((loff_t)offset, (size_t)size, (unsigned char *)buf);
 *        return 0;
 *}
 */

static struct f_dfu_buf *f_dfu_buffer_alloc(int buf_size)
{
	struct f_dfu_buf *copy_buf;

	copy_buf = kzalloc(sizeof *copy_buf, GFP_KERNEL);
	if(!copy_buf){
		printk("kzalloc if copy_buf is failed\n");
		return ERR_PTR(-ENOMEM);
	}

	copy_buf->buf = kmalloc(buf_size, GFP_KERNEL);
	if(!copy_buf->buf){
		kfree(copy_buf);
		printk("kzalloc if copy_buf->buf is failed\n");
		return ERR_PTR(-ENOMEM);
	}
	copy_buf->actual = 0;

	return copy_buf;
}

void write_flash_work_func(struct work_struct *work)
{
	struct f_dfu *dfu = container_of(work, struct f_dfu, write_flash_work);

	struct f_dfu_buf *play_buf;

	play_buf = dfu->copy_buf;

	erase_flash(play_buf->partition_address, play_buf->partition_size);

	write_flash(play_buf->buf, play_buf->partition_address, play_buf->actual);
	play_buf->actual = 0;
	memset(play_buf->buf, 0, BUF_SIZE);
}

static void dnload_request_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_dfu *f_dfu = req->context;
	struct f_dfu_buf *copy_buf = f_dfu->copy_buf ;
	if(copy_buf->actual > copy_buf->partition_size){
		printk("write data exceeds partition size total_len =%d,partition_size=%llu\n",copy_buf->actual, copy_buf->partition_size);
		return;
	}
	memcpy(copy_buf->buf + copy_buf->actual, req->buf, req->actual);
	copy_buf->actual += req->actual;
	f_dfu->copy_buf = copy_buf;

	if(req->actual == 0) {
		if(!work_pending(&f_dfu->write_flash_work)){
			printk("schedule_work is success\n");
			schedule_work(&f_dfu->write_flash_work);
		} else {
			printk("schedule_work is failed\n");
		}


	}
}

static void handle_getstatus(struct usb_request *req)
{
	struct dfu_status *dstat = (struct dfu_status *)req->buf;
	struct f_dfu *f_dfu = req->context;

	switch (f_dfu->dfu_state) {
	case DFU_STATE_dfuDNLOAD_SYNC:
	case DFU_STATE_dfuDNBUSY:
		f_dfu->dfu_state = DFU_STATE_dfuDNLOAD_IDLE;
		break;
	case DFU_STATE_dfuMANIFEST_SYNC:
		break;
	default:
		break;
	}

	/* send status response */
	dstat->bStatus = f_dfu->dfu_status;
	dstat->bwPollTimeout[0] = 0;
	dstat->bwPollTimeout[1] = 0;
	dstat->bwPollTimeout[2] = 0;
	dstat->bState = f_dfu->dfu_state;
	dstat->iString = 0;
}

static void handle_getstate(struct usb_request *req)
{
	struct f_dfu *f_dfu = req->context;

	((u8 *)req->buf)[0] = f_dfu->dfu_state;
	req->actual = sizeof(u8);
}

static inline void to_dfu_mode(struct f_dfu *f_dfu)
{
	f_dfu->usb_function.strings = dfu_generic_strings;
	f_dfu->usb_function.hs_descriptors = f_dfu->function;
	f_dfu->dfu_state = DFU_STATE_dfuIDLE;
}

static inline void to_runtime_mode(struct f_dfu *f_dfu)
{
	f_dfu->usb_function.strings = NULL;
	f_dfu->usb_function.hs_descriptors = dfu_runtime_descs;
}

static int handle_upload(struct usb_request *req, u16 len)
{
	/*struct f_dfu *f_dfu = req->context;*/

	/*
	 *return dfu_read(dfu_get_entity(f_dfu->altsetting), req->buf,
	 *                req->length, f_dfu->blk_seq_num);
	 */
	return 0;
}

static int handle_dnload(struct usb_gadget *gadget, u16 len)
{
	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	struct usb_request *req = cdev->req;
	struct f_dfu *f_dfu = req->context;

	if (len == 0)
		f_dfu->dfu_state = DFU_STATE_dfuMANIFEST_SYNC;

	req->complete = dnload_request_complete;

	return len;
}

/*-------------------------------------------------------------------------*/
/* DFU state machine  */
static int state_app_idle(struct f_dfu *f_dfu,
			  const struct usb_ctrlrequest *ctrl,
			  struct usb_gadget *gadget,
			  struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	case USB_REQ_DFU_DETACH:
		f_dfu->dfu_state = DFU_STATE_appDETACH;
		to_dfu_mode(f_dfu);
		value = RET_ZLP;
		break;
	default:
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_app_detach(struct f_dfu *f_dfu,
			    const struct usb_ctrlrequest *ctrl,
			    struct usb_gadget *gadget,
			    struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_appIDLE;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_idle(struct f_dfu *f_dfu,
			  const struct usb_ctrlrequest *ctrl,
			  struct usb_gadget *gadget,
			  struct usb_request *req)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 len = le16_to_cpu(ctrl->wLength);
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_DNLOAD:
		if (len == 0) {
			f_dfu->dfu_state = DFU_STATE_dfuERROR;
			value = RET_STALL;
			break;
		}
		f_dfu->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
		f_dfu->blk_seq_num = w_value;
		value = handle_dnload(gadget, len);
		break;
	case USB_REQ_DFU_UPLOAD:
		f_dfu->dfu_state = DFU_STATE_dfuUPLOAD_IDLE;
		f_dfu->blk_seq_num = 0;
		value = handle_upload(req, len);
		break;
	case USB_REQ_DFU_ABORT:
		/* no zlp? */
		value = RET_ZLP;
		break;
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	case USB_REQ_DFU_DETACH:
		/*
		 * Proprietary extension: 'detach' from idle mode and
		 * get back to runtime mode in case of USB Reset.  As
		 * much as I dislike this, we just can't use every USB
		 * bus reset to switch back to runtime mode, since at
		 * least the Linux USB stack likes to send a number of
		 * resets in a row :(
		 */
		f_dfu->dfu_state =
			DFU_STATE_dfuMANIFEST_WAIT_RST;
		to_runtime_mode(f_dfu);
		f_dfu->dfu_state = DFU_STATE_appIDLE;
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_dnload_sync(struct f_dfu *f_dfu,
				 const struct usb_ctrlrequest *ctrl,
				 struct usb_gadget *gadget,
				 struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_dnbusy(struct f_dfu *f_dfu,
			    const struct usb_ctrlrequest *ctrl,
			    struct usb_gadget *gadget,
			    struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_dnload_idle(struct f_dfu *f_dfu,
				 const struct usb_ctrlrequest *ctrl,
				 struct usb_gadget *gadget,
				 struct usb_request *req)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 len = le16_to_cpu(ctrl->wLength);
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_DNLOAD:
		f_dfu->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
		f_dfu->blk_seq_num = w_value;
		value = handle_dnload(gadget, len);
		break;
	case USB_REQ_DFU_ABORT:
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		value = RET_ZLP;
		break;
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_manifest_sync(struct f_dfu *f_dfu,
				   const struct usb_ctrlrequest *ctrl,
				   struct usb_gadget *gadget,
				   struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		/* We're MainfestationTolerant */
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		handle_getstatus(req);
		f_dfu->blk_seq_num = 0;
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_upload_idle(struct f_dfu *f_dfu,
				 const struct usb_ctrlrequest *ctrl,
				 struct usb_gadget *gadget,
				 struct usb_request *req)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 len = le16_to_cpu(ctrl->wLength);
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_UPLOAD:
		/* state transition if less data then requested */
		f_dfu->blk_seq_num = w_value;
		value = handle_upload(req, len);
		if (value >= 0 && value < len)
			f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		break;
	case USB_REQ_DFU_ABORT:
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		/* no zlp? */
		value = RET_ZLP;
		break;
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_error(struct f_dfu *f_dfu,
				 const struct usb_ctrlrequest *ctrl,
				 struct usb_gadget *gadget,
				 struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(req);
		break;
	case USB_REQ_DFU_CLRSTATUS:
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		f_dfu->dfu_status = DFU_STATUS_OK;
		/* no zlp? */
		value = RET_ZLP;
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static dfu_state_fn dfu_state[] = {
	state_app_idle,          /* DFU_STATE_appIDLE */
	state_app_detach,        /* DFU_STATE_appDETACH */
	state_dfu_idle,          /* DFU_STATE_dfuIDLE */
	state_dfu_dnload_sync,   /* DFU_STATE_dfuDNLOAD_SYNC */
	state_dfu_dnbusy,        /* DFU_STATE_dfuDNBUSY */
	state_dfu_dnload_idle,   /* DFU_STATE_dfuDNLOAD_IDLE */
	state_dfu_manifest_sync, /* DFU_STATE_dfuMANIFEST_SYNC */
	NULL,                    /* DFU_STATE_dfuMANIFEST */
	NULL,                    /* DFU_STATE_dfuMANIFEST_WAIT_RST */
	state_dfu_upload_idle,   /* DFU_STATE_dfuUPLOAD_IDLE */
	state_dfu_error          /* DFU_STATE_dfuERROR */
};

static int
dfu_handle(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_gadget *gadget = f->config->cdev->gadget;
	struct usb_request *req = f->config->cdev->req;
	struct f_dfu *f_dfu = f->config->cdev->req->context;
	u16 len = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	int value = 0;
	u8 req_type = ctrl->bRequestType & USB_TYPE_MASK;

	if (req_type == USB_TYPE_STANDARD) {
		if (ctrl->bRequest == USB_REQ_GET_DESCRIPTOR &&
		    (w_value >> 8) == DFU_DT_FUNC) {
			value = min(len, (u16) sizeof(dfu_func));
			memcpy(req->buf, &dfu_func, value);
		}
	} else {/* DFU specific request */
		value = dfu_state[f_dfu->dfu_state] (f_dfu, ctrl, gadget, req);
		req->context = f_dfu;
	}

	if (value >= 0) {
		req->length = value;
		req->zero = value < len;
		value = usb_ep_queue(gadget->ep0, req, 0);
		if (value < 0) {
			req->status = 0;
		}
	}

	return value;
}

/*-------------------------------------------------------------------------*/

static int
dfu_prepare_strings(struct f_dfu *f_dfu, int n)
{
	int i = 0;

	f_dfu->strings = kcalloc(sizeof(struct usb_string), n + 1, GFP_KERNEL);
	if (!f_dfu->strings)
		goto enomem;

	for (i = 0; i < n; ++i) {
		f_dfu->strings[i].id = i;
		f_dfu->strings[i].s = strings_dfu_generic[i].s;
	}


	return 0;

enomem:
	while (i)
		f_dfu->strings[--i].s = NULL;

	kfree(f_dfu->strings);

	return -ENOMEM;
}

static int dfu_prepare_function(struct f_dfu *f_dfu, int n)
{
	struct usb_interface_descriptor *d;
	int i = 0;

	f_dfu->function = kcalloc(sizeof(struct usb_descriptor_header *), n + 2, GFP_KERNEL);
	if (!f_dfu->function)
		goto enomem;

	for (i = 0; i < n; ++i) {
		d = kcalloc(sizeof(*d), 1, GFP_KERNEL);
		if (!d)
			goto enomem;

		d->bLength =		sizeof(*d);
		d->bDescriptorType =	USB_DT_INTERFACE;
		d->bAlternateSetting =	i;
		d->bNumEndpoints =	0;
		d->bInterfaceClass =	USB_CLASS_APP_SPEC;
		d->bInterfaceSubClass =	1;
		d->bInterfaceProtocol =	2;

		f_dfu->function[i] = (struct usb_descriptor_header *)d;
	}
	f_dfu->function[i] = (struct usb_descriptor_header *)&dfu_func;
	f_dfu->function[i+1] = NULL;

	return 0;

enomem:
	while (i) {
		kfree(f_dfu->function[--i]);
		f_dfu->function[i] = NULL;
	}
	kfree(f_dfu->function);

	return -ENOMEM;
}

static int dfu_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_dfu *f_dfu = func_to_dfu(f);
	int alt_num = 5;
	int rv, id, i;

	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	dfu_intf_runtime.bInterfaceNumber = id;

	f_dfu->dfu_state = DFU_STATE_appIDLE;
	f_dfu->dfu_status = DFU_STATUS_OK;

	rv = dfu_prepare_function(f_dfu, alt_num);
	if (rv)
		goto error;

	rv = dfu_prepare_strings(f_dfu, alt_num);
	if (rv)
		goto error;
	for (i = 0; i < alt_num; i++) {
		id = usb_string_id(cdev);
		if (id < 0)
			return id;
		f_dfu->strings[i].id = id;
		((struct usb_interface_descriptor *)f_dfu->function[i])
			->iInterface = id;
	}

	to_dfu_mode(f_dfu);

	stringtab_dfu_generic.strings = f_dfu->strings;

	cdev->req->context = f_dfu;
	INIT_WORK(&f_dfu->write_flash_work, write_flash_work_func);
	f_dfu->copy_buf = f_dfu_buffer_alloc(BUF_SIZE);

error:
	return rv;
}

static void dfu_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_dfu *f_dfu = func_to_dfu(f);
	int alt_num = 1;
	int i;

	if (f_dfu->strings) {
		i = alt_num;
		while (i)
			f_dfu->strings[--i].s = NULL;

		kfree(f_dfu->strings);
	}

	if (f_dfu->function) {
		i = alt_num;
		while (i) {
			kfree(f_dfu->function[--i]);
			f_dfu->function[i] = NULL;
		}
		kfree(f_dfu->function);
	}

	kfree(f_dfu);
}

static int dfu_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_dfu *f_dfu = func_to_dfu(f);
	struct f_dfu_buf *dfu_buf;

	printk("%s: intf:%d alt:%d\n", __func__, intf, alt);

	f_dfu->altsetting = alt;
	dfu_buf = f_dfu->copy_buf;

	switch(alt){
		case CONTROL:
			break;
		case UBOOT:
			dfu_buf->partition_address = UBOOT_ADDRESS;
			dfu_buf->partition_size = UBOOT_SIZE;
			break;
		case KERNEL:
			dfu_buf->partition_address = KERNEL_ADDRESS;
			dfu_buf->partition_size = KERNEL_SIZE;
			break;
		case ROOTFS:
			dfu_buf->partition_address = ROOTFS_ADDRESS;
			dfu_buf->partition_size = ROOTFS_SIZE;
			break;
		case APPFS:
			dfu_buf->partition_address = APPFS_ADDRESS;
			dfu_buf->partition_size = APPFS_SIZE;
			break;
		default:
			printk("There is no interface = %d\n",alt);
			break;
	}

	return 0;
}

/* TODO: is this really what we need here? */
static void dfu_disable(struct usb_function *f)
{
	struct f_dfu *f_dfu = func_to_dfu(f);
	if (f_dfu->config == 0)
		return;


	f_dfu->config = 0;
}

static int dfu_bind_config(struct usb_configuration *c)
{
	struct f_dfu *f_dfu;
	int status;

	f_dfu = kcalloc(sizeof(*f_dfu), 1, GFP_KERNEL);
	if (!f_dfu)
		return -ENOMEM;
	f_dfu->usb_function.name = "dfu";
	f_dfu->usb_function.bind = dfu_bind;
	f_dfu->usb_function.unbind = dfu_unbind;
	f_dfu->usb_function.set_alt = dfu_set_alt;
	f_dfu->usb_function.disable = dfu_disable;
	f_dfu->usb_function.strings = dfu_generic_strings,
	f_dfu->usb_function.setup = dfu_handle,
	status = usb_add_function(c, &f_dfu->usb_function);
	if (status)
		kfree(f_dfu);

	return status;
}

int dfu_add(struct usb_configuration *c)
{
	int id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	strings_dfu_generic[0].id = id;
	dfu_intf_runtime.iInterface = id;

	return dfu_bind_config(c);
}
