/*
 * f_uac_mic.c -- Minimal UAC1 microphone-only USB gadget function
 *
 * No ALSA dependency. Userspace writes raw PCM to /dev/uac_mic,
 * kernel sends it to the host via isochronous IN endpoint.
 *
 * 16-bit PCM, mono, 16kHz.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/usb/audio.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/uaccess.h>

/* Audio parameters */
#define UAC_MIC_SAMPLE_RATE	16000
#define UAC_MIC_CHANNELS	1
#define UAC_MIC_BIT_RES		16
#define UAC_MIC_SUBFRAME_SIZE	2	/* bytes per sample */

/* Samples per USB frame (1ms): 16000/1000 = 16 */
#define UAC_MIC_SAMPLES_PER_FRAME (UAC_MIC_SAMPLE_RATE / 1000)
/* Bytes per USB frame: 16 samples × 2 bytes × 1 channel = 32 */
#define UAC_MIC_FRAME_SIZE	(UAC_MIC_SAMPLES_PER_FRAME * UAC_MIC_SUBFRAME_SIZE * UAC_MIC_CHANNELS)

#define UAC_MIC_NUM_REQS	4	/* ISO request double-buffering */
#define UAC_MIC_RING_SIZE	(UAC_MIC_FRAME_SIZE * 256) /* ~256ms buffer = 8192 bytes */

/* Terminal IDs */
#define UAC_MIC_IT_ID		1	/* Input Terminal (microphone) */
#define UAC_MIC_OT_ID		2	/* Output Terminal (USB streaming) */

struct uac_mic {
	struct usb_function func;
	struct usb_ep *ep;
	struct usb_request *req[UAC_MIC_NUM_REQS];

	/* Ring buffer: userspace writes, ISO callback reads */
	uint8_t *ring_buf;
	size_t ring_size;
	size_t write_pos;
	size_t read_pos;
	spinlock_t lock;

	/* Misc char device */
	struct miscdevice mdev;
	bool mdev_registered;

	bool streaming;
	wait_queue_head_t wq;

	unsigned int ac_intf;
	unsigned int as_intf;
};

static struct uac_mic *g_uac_mic; /* singleton for misc device access */

/* --------------------------------------------------------------------------
 * Char device: /dev/uac_mic
 */

static int uac_mic_dev_open(struct inode *inode, struct file *f)
{
	if (!g_uac_mic)
		return -ENODEV;
	f->private_data = g_uac_mic;
	return 0;
}

static ssize_t uac_mic_dev_write(struct file *f, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct uac_mic *mic = f->private_data;
	unsigned long flags;
	size_t space, len, first, second;

	if (!mic || !mic->ring_buf)
		return -ENODEV;

	spin_lock_irqsave(&mic->lock, flags);
	space = mic->ring_size - ((mic->write_pos + mic->ring_size - mic->read_pos) % mic->ring_size) - 1;
	spin_unlock_irqrestore(&mic->lock, flags);

	len = min(count, space);
	if (len == 0) {
		if (f->f_flags & O_NONBLOCK)
			return -EAGAIN;
		/* Block until space available */
		if (wait_event_interruptible(mic->wq, ({
			spin_lock_irqsave(&mic->lock, flags);
			space = mic->ring_size - ((mic->write_pos + mic->ring_size - mic->read_pos) % mic->ring_size) - 1;
			spin_unlock_irqrestore(&mic->lock, flags);
			space > 0;
		})))
			return -ERESTARTSYS;
		len = min(count, space);
	}

	/* Handle wrap-around copy */
	first = min(len, mic->ring_size - mic->write_pos);
	if (copy_from_user(mic->ring_buf + mic->write_pos, buf, first))
		return -EFAULT;
	second = len - first;
	if (second > 0) {
		if (copy_from_user(mic->ring_buf, buf + first, second))
			return -EFAULT;
	}

	spin_lock_irqsave(&mic->lock, flags);
	mic->write_pos = (mic->write_pos + len) % mic->ring_size;
	spin_unlock_irqrestore(&mic->lock, flags);

	return len;
}

static unsigned int uac_mic_dev_poll(struct file *f, poll_table *wait)
{
	struct uac_mic *mic = f->private_data;
	unsigned long flags;
	unsigned int mask = 0;
	size_t space;

	poll_wait(f, &mic->wq, wait);

	spin_lock_irqsave(&mic->lock, flags);
	space = mic->ring_size - ((mic->write_pos + mic->ring_size - mic->read_pos) % mic->ring_size) - 1;
	spin_unlock_irqrestore(&mic->lock, flags);

	if (space > 0)
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

static const struct file_operations uac_mic_fops = {
	.owner		= THIS_MODULE,
	.open		= uac_mic_dev_open,
	.write		= uac_mic_dev_write,
	.poll		= uac_mic_dev_poll,
};

/* --------------------------------------------------------------------------
 * ISO IN completion callback
 */

static void uac_mic_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uac_mic *mic = req->context;
	unsigned long flags;
	size_t avail, len, first, second;

	if (req->status == -ESHUTDOWN)
		return;

	spin_lock_irqsave(&mic->lock, flags);
	avail = (mic->write_pos + mic->ring_size - mic->read_pos) % mic->ring_size;
	len = min(avail, (size_t)UAC_MIC_FRAME_SIZE);

	if (len > 0) {
		/* Copy from ring to USB request, handle wrap */
		first = min(len, mic->ring_size - mic->read_pos);
		memcpy(req->buf, mic->ring_buf + mic->read_pos, first);
		second = len - first;
		if (second > 0)
			memcpy(req->buf + first, mic->ring_buf, second);
		mic->read_pos = (mic->read_pos + len) % mic->ring_size;
	} else {
		/* Underrun: send silence */
		memset(req->buf, 0, UAC_MIC_FRAME_SIZE);
		len = UAC_MIC_FRAME_SIZE;
	}
	spin_unlock_irqrestore(&mic->lock, flags);

	req->length = len;
	if (usb_ep_queue(ep, req, GFP_ATOMIC) < 0)
		printk(KERN_WARNING "uac_mic: ep_queue failed\n");

	wake_up_interruptible(&mic->wq);
}

/* --------------------------------------------------------------------------
 * USB Audio Class descriptors
 */

/* Audio Control Interface */
static struct usb_interface_descriptor uac_mic_ac_intf = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_AUDIO,
	.bInterfaceSubClass	= USB_SUBCLASS_AUDIOCONTROL,
	.bInterfaceProtocol	= 0,
};

/* AC Header: 9 bytes (8 + 1 interface) */
static struct {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubtype;
	__le16 bcdADC;
	__le16 wTotalLength;
	__u8  bInCollection;
	__u8  baInterfaceNr;
} __attribute__((packed)) uac_mic_ac_header = {
	.bLength		= 9,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_HEADER,
	.bcdADC			= cpu_to_le16(0x0100),
	.wTotalLength		= cpu_to_le16(9 + 12 + 9), /* header + IT + OT */
	.bInCollection		= 1,
	.baInterfaceNr		= 0, /* filled at bind */
};

/* Input Terminal: Microphone */
static struct uac_input_terminal_descriptor uac_mic_it = {
	.bLength		= UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_INPUT_TERMINAL,
	.bTerminalID		= UAC_MIC_IT_ID,
	.wTerminalType		= cpu_to_le16(UAC_INPUT_TERMINAL_MICROPHONE),
	.bAssocTerminal		= 0,
	.bNrChannels		= UAC_MIC_CHANNELS,
	.wChannelConfig		= cpu_to_le16(0x0001), /* mono: left front */
	.iChannelNames		= 0,
	.iTerminal		= 0,
};

/* Output Terminal: USB Streaming */
static struct uac1_output_terminal_descriptor uac_mic_ot = {
	.bLength		= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_OUTPUT_TERMINAL,
	.bTerminalID		= UAC_MIC_OT_ID,
	.wTerminalType		= cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal		= 0,
	.bSourceID		= UAC_MIC_IT_ID,
	.iTerminal		= 0,
};

/* Audio Streaming Interface Alt 0: inactive */
static struct usb_interface_descriptor uac_mic_as_intf_alt0 = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_AUDIO,
	.bInterfaceSubClass	= USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol	= 0,
};

/* Audio Streaming Interface Alt 1: active */
static struct usb_interface_descriptor uac_mic_as_intf_alt1 = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bAlternateSetting	= 1,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_AUDIO,
	.bInterfaceSubClass	= USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol	= 0,
};

/* AS General */
static struct uac1_as_header_descriptor uac_mic_as_general = {
	.bLength		= UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_AS_GENERAL,
	.bTerminalLink		= UAC_MIC_OT_ID,
	.bDelay			= 1,
	.wFormatTag		= cpu_to_le16(UAC_FORMAT_TYPE_I_PCM),
};

/* Format Type I: 16-bit mono 16kHz */
static struct {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubtype;
	__u8  bFormatType;
	__u8  bNrChannels;
	__u8  bSubframeSize;
	__u8  bBitResolution;
	__u8  bSamFreqType;
	__u8  tSamFreq[3]; /* 16000 = 0x003E80 → LE: 0x80, 0x3E, 0x00 */
} __attribute__((packed)) uac_mic_format = {
	.bLength		= 11,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_FORMAT_TYPE,
	.bFormatType		= UAC_FORMAT_TYPE_I,
	.bNrChannels		= UAC_MIC_CHANNELS,
	.bSubframeSize		= UAC_MIC_SUBFRAME_SIZE,
	.bBitResolution		= UAC_MIC_BIT_RES,
	.bSamFreqType		= 1,
	.tSamFreq		= { 0x80, 0x3E, 0x00 }, /* 16000 Hz */
};

/* ISO IN endpoint */
/* FS ISO endpoint: bInterval=1 → every 1ms frame */
static struct usb_endpoint_descriptor uac_mic_ep_fs = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE,
	.wMaxPacketSize		= cpu_to_le16(UAC_MIC_FRAME_SIZE),
	.bInterval		= 1,
};

/* HS ISO endpoint: bInterval=4 → 2^(4-1)=8 microframes = 1ms */
static struct usb_endpoint_descriptor uac_mic_ep_hs = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE,
	.wMaxPacketSize		= cpu_to_le16(UAC_MIC_FRAME_SIZE),
	.bInterval		= 4,
};

/* Class-specific ISO endpoint */
static struct uac_iso_endpoint_descriptor uac_mic_ep_cs = {
	.bLength		= UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType	= USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype	= UAC_EP_GENERAL,
	.bmAttributes		= 0,
	.bLockDelayUnits	= 0,
	.wLockDelay		= 0,
};

/* --------------------------------------------------------------------------
 * USB function callbacks
 */

static int
uac_mic_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct uac_mic *mic = container_of(f, struct uac_mic, func);
	int i, ret;

	if (intf == mic->ac_intf) {
		/* Audio control interface: only alt 0 */
		return alt ? -EINVAL : 0;
	}

	if (intf != mic->as_intf)
		return -EINVAL;

	if (alt == 0) {
		/* Stop streaming */
		if (mic->streaming) {
			for (i = 0; i < UAC_MIC_NUM_REQS; i++)
				if (mic->req[i])
					usb_ep_dequeue(mic->ep, mic->req[i]);
			usb_ep_disable(mic->ep);
			mic->streaming = false;
			printk(KERN_INFO "uac_mic: streaming stopped\n");
		}
		return 0;
	}

	if (alt == 1) {
		/* Start streaming */
		ret = config_ep_by_speed(f->config->cdev->gadget, f, mic->ep);
		if (ret)
			return ret;
		ret = usb_ep_enable(mic->ep);
		if (ret)
			return ret;

		/* Reset ring buffer */
		spin_lock_irq(&mic->lock);
		mic->read_pos = 0;
		mic->write_pos = 0;
		spin_unlock_irq(&mic->lock);

		/* Queue ISO requests */
		for (i = 0; i < UAC_MIC_NUM_REQS; i++) {
			mic->req[i]->length = UAC_MIC_FRAME_SIZE;
			memset(mic->req[i]->buf, 0, UAC_MIC_FRAME_SIZE);
			ret = usb_ep_queue(mic->ep, mic->req[i], GFP_ATOMIC);
			if (ret) {
				printk(KERN_ERR "uac_mic: ep_queue[%d] failed: %d\n", i, ret);
				usb_ep_disable(mic->ep);
				return ret;
			}
		}

		mic->streaming = true;
		printk(KERN_INFO "uac_mic: streaming started (%d Hz, %d-bit, %d ch)\n",
		       UAC_MIC_SAMPLE_RATE, UAC_MIC_BIT_RES, UAC_MIC_CHANNELS);
		return 0;
	}

	return -EINVAL;
}

static int
uac_mic_get_alt(struct usb_function *f, unsigned intf)
{
	struct uac_mic *mic = container_of(f, struct uac_mic, func);

	if (intf == mic->ac_intf)
		return 0;
	if (intf == mic->as_intf)
		return mic->streaming ? 1 : 0;
	return -EINVAL;
}

static int
uac_mic_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	/* Stall all class-specific requests — we don't support volume/mute */
	return -EOPNOTSUPP;
}

static void
uac_mic_disable(struct usb_function *f)
{
	struct uac_mic *mic = container_of(f, struct uac_mic, func);
	int i;

	if (mic->streaming) {
		for (i = 0; i < UAC_MIC_NUM_REQS; i++)
			if (mic->req[i])
				usb_ep_dequeue(mic->ep, mic->req[i]);
		usb_ep_disable(mic->ep);
		mic->streaming = false;
	}
}

static int __init
uac_mic_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct uac_mic *mic = container_of(f, struct uac_mic, func);
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_ep *ep;
	int ret, i;

	/* Allocate AC interface */
	ret = usb_interface_id(c, f);
	if (ret < 0)
		return ret;
	mic->ac_intf = ret;
	uac_mic_ac_intf.bInterfaceNumber = ret;

	/* Allocate AS interface */
	ret = usb_interface_id(c, f);
	if (ret < 0)
		return ret;
	mic->as_intf = ret;
	uac_mic_as_intf_alt0.bInterfaceNumber = ret;
	uac_mic_as_intf_alt1.bInterfaceNumber = ret;
	uac_mic_ac_header.baInterfaceNr = ret;

	/* Allocate ISO IN endpoint */
	ep = usb_ep_autoconfig(cdev->gadget, &uac_mic_ep_hs);
	if (!ep) {
		printk(KERN_ERR "uac_mic: unable to allocate ISO IN ep\n");
		return -ENODEV;
	}
	mic->ep = ep;
	ep->driver_data = mic;
	uac_mic_ep_fs.bEndpointAddress = ep->address;

	/* Allocate ring buffer */
	mic->ring_size = UAC_MIC_RING_SIZE;
	mic->ring_buf = kzalloc(mic->ring_size, GFP_KERNEL);
	if (!mic->ring_buf)
		return -ENOMEM;

	/* Allocate USB requests */
	for (i = 0; i < UAC_MIC_NUM_REQS; i++) {
		mic->req[i] = usb_ep_alloc_request(ep, GFP_KERNEL);
		if (!mic->req[i]) {
			ret = -ENOMEM;
			goto err_req;
		}
		mic->req[i]->buf = kzalloc(UAC_MIC_FRAME_SIZE, GFP_KERNEL);
		if (!mic->req[i]->buf) {
			ret = -ENOMEM;
			goto err_req;
		}
		mic->req[i]->complete = uac_mic_complete;
		mic->req[i]->context = mic;
	}

	/* Register misc char device */
	mic->mdev.minor = MISC_DYNAMIC_MINOR;
	mic->mdev.name = "uac_mic";
	mic->mdev.fops = &uac_mic_fops;
	ret = misc_register(&mic->mdev);
	if (ret) {
		printk(KERN_ERR "uac_mic: misc_register failed: %d\n", ret);
		goto err_req;
	}
	mic->mdev_registered = true;

	INFO(cdev, "UAC Mic: %d Hz, %d-bit, %d ch → /dev/uac_mic\n",
	     UAC_MIC_SAMPLE_RATE, UAC_MIC_BIT_RES, UAC_MIC_CHANNELS);
	return 0;

err_req:
	for (i = 0; i < UAC_MIC_NUM_REQS; i++) {
		if (mic->req[i]) {
			kfree(mic->req[i]->buf);
			usb_ep_free_request(ep, mic->req[i]);
			mic->req[i] = NULL;
		}
	}
	kfree(mic->ring_buf);
	mic->ring_buf = NULL;
	return ret;
}

static void
uac_mic_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct uac_mic *mic = container_of(f, struct uac_mic, func);
	int i;

	if (mic->mdev_registered) {
		misc_deregister(&mic->mdev);
		mic->mdev_registered = false;
	}

	for (i = 0; i < UAC_MIC_NUM_REQS; i++) {
		if (mic->req[i]) {
			kfree(mic->req[i]->buf);
			usb_ep_free_request(mic->ep, mic->req[i]);
			mic->req[i] = NULL;
		}
	}
	kfree(mic->ring_buf);
	mic->ring_buf = NULL;
	g_uac_mic = NULL;
}

/* --------------------------------------------------------------------------
 * Public: bind to composite configuration
 */

static struct usb_descriptor_header *uac_mic_fs_descs[] = {
	(struct usb_descriptor_header *) &uac_mic_ac_intf,
	(struct usb_descriptor_header *) &uac_mic_ac_header,
	(struct usb_descriptor_header *) &uac_mic_it,
	(struct usb_descriptor_header *) &uac_mic_ot,
	(struct usb_descriptor_header *) &uac_mic_as_intf_alt0,
	(struct usb_descriptor_header *) &uac_mic_as_intf_alt1,
	(struct usb_descriptor_header *) &uac_mic_as_general,
	(struct usb_descriptor_header *) &uac_mic_format,
	(struct usb_descriptor_header *) &uac_mic_ep_fs,
	(struct usb_descriptor_header *) &uac_mic_ep_cs,
	NULL,
};

static struct usb_descriptor_header *uac_mic_hs_descs[] = {
	(struct usb_descriptor_header *) &uac_mic_ac_intf,
	(struct usb_descriptor_header *) &uac_mic_ac_header,
	(struct usb_descriptor_header *) &uac_mic_it,
	(struct usb_descriptor_header *) &uac_mic_ot,
	(struct usb_descriptor_header *) &uac_mic_as_intf_alt0,
	(struct usb_descriptor_header *) &uac_mic_as_intf_alt1,
	(struct usb_descriptor_header *) &uac_mic_as_general,
	(struct usb_descriptor_header *) &uac_mic_format,
	(struct usb_descriptor_header *) &uac_mic_ep_hs,
	(struct usb_descriptor_header *) &uac_mic_ep_cs,
	NULL,
};

int __init uac_mic_bind_config(struct usb_configuration *c)
{
	struct uac_mic *mic;
	int ret;

	mic = kzalloc(sizeof(*mic), GFP_KERNEL);
	if (!mic)
		return -ENOMEM;

	spin_lock_init(&mic->lock);
	init_waitqueue_head(&mic->wq);

	mic->func.name = "uac_mic";
	mic->func.bind = uac_mic_bind;
	mic->func.unbind = uac_mic_unbind;
	mic->func.set_alt = uac_mic_set_alt;
	mic->func.get_alt = uac_mic_get_alt;
	mic->func.setup = uac_mic_setup;
	mic->func.disable = uac_mic_disable;

	/* Static descriptors for all speeds (same for FS/HS) */
	mic->func.fs_descriptors = uac_mic_fs_descs;
	mic->func.hs_descriptors = uac_mic_hs_descs;
	mic->func.ss_descriptors = uac_mic_hs_descs;

	g_uac_mic = mic;

	ret = usb_add_function(c, &mic->func);
	if (ret) {
		kfree(mic);
		g_uac_mic = NULL;
	}
	return ret;
}
