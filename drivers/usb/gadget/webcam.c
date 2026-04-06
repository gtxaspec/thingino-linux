/*
 *	webcam.c -- USB webcam gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	Modified for thingino Raptor Webcam (MJPEG + H.264)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/usb/video.h>

#include "f_uvc.h"

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "uvc_queue.c"
#include "uvc_video.c"
#include "uvc_v4l2.c"
#include "f_uvc.c"

USB_GADGET_COMPOSITE_OPTIONS();

/* --------------------------------------------------------------------------
 * Device descriptor
 */

#define WEBCAM_VENDOR_ID		0x1d6b	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0102	/* Webcam A/V gadget */
#define WEBCAM_DEVICE_BCD		0x0010	/* 0.10 */

static char webcam_vendor_label[] = "thingino";
static char webcam_product_label[] = "Raptor Webcam";
static char webcam_config_label[] = "Video";

/* string IDs are assigned dynamically */

#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string webcam_strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = webcam_vendor_label,
	[USB_GADGET_PRODUCT_IDX].s = webcam_product_label,
	[USB_GADGET_SERIAL_IDX].s = "",
	[STRING_DESCRIPTION_IDX].s = webcam_config_label,
	{  }
};

static struct usb_gadget_strings webcam_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = webcam_strings,
};

static struct usb_gadget_strings *webcam_device_strings[] = {
	&webcam_stringtab,
	NULL,
};

static struct usb_device_descriptor webcam_device_descriptor = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	.bcdUSB			= cpu_to_le16(0x0200),
	.bDeviceClass		= USB_CLASS_MISC,
	.bDeviceSubClass	= 0x02,
	.bDeviceProtocol	= 0x01,
	.bMaxPacketSize0	= 0, /* dynamic */
	.idVendor		= cpu_to_le16(WEBCAM_VENDOR_ID),
	.idProduct		= cpu_to_le16(WEBCAM_PRODUCT_ID),
	.bcdDevice		= cpu_to_le16(WEBCAM_DEVICE_BCD),
	.iManufacturer		= 0, /* dynamic */
	.iProduct		= 0, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations	= 0, /* dynamic */
};

/* --------------------------------------------------------------------------
 * UVC control descriptors
 */

DECLARE_UVC_HEADER_DESCRIPTOR(1);

static const struct UVC_HEADER_DESCRIPTOR(1) uvc_control_header = {
	.bLength		= UVC_DT_HEADER_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC			= cpu_to_le16(0x0100),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};

static const struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength		= UVC_DT_CAMERA_TERMINAL_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= 1,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal		= 0,
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	.bmControls[0]		= 0,
	.bmControls[1]		= 0,
	.bmControls[2]		= 0,
};

static const struct uvc_processing_unit_descriptor uvc_processing = {
	.bLength		= UVC_DT_PROCESSING_UNIT_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_PROCESSING_UNIT,
	.bUnitID		= 2,
	.bSourceID		= 1,
	.wMaxMultiplier		= cpu_to_le16(16*1024),
	.bControlSize		= 2,
	.bmControls[0]		= 0,
	.bmControls[1]		= 0,
	.iProcessing		= 0,
};

static const struct uvc_output_terminal_descriptor uvc_output_terminal = {
	.bLength		= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= 3,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID		= 2,
	.iTerminal		= 0,
};

/* --------------------------------------------------------------------------
 * UVC streaming descriptors — MJPEG (format 1) + H.264 framebased (format 2)
 */

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 2);

static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 2) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 2,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo			= 0,
	.bTerminalLink		= 3,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0,
};

/* --- Format 1: MJPEG --- */

static const struct uvc_format_mjpeg uvc_format_mjpg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 3,
	.bmFlags		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

DECLARE_UVC_FRAME_MJPEG(3);

static const struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_1080p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1080),
	.dwMinBitRate		= cpu_to_le32(3000000),
	.dwMaxBitRate		= cpu_to_le32(40000000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1920 * 1080 * 2),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= cpu_to_le32(333333),	/* 30 fps */
	.dwFrameInterval[1]	= cpu_to_le32(400000),	/* 25 fps */
	.dwFrameInterval[2]	= cpu_to_le32(666666),	/* 15 fps */
};

static const struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_720p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(2000000),
	.dwMaxBitRate		= cpu_to_le32(20000000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1280 * 720 * 2),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= cpu_to_le32(333333),	/* 30 fps */
	.dwFrameInterval[1]	= cpu_to_le32(400000),	/* 25 fps */
	.dwFrameInterval[2]	= cpu_to_le32(666666),	/* 15 fps */
};

static const struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_360p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(360),
	.dwMinBitRate		= cpu_to_le32(1000000),
	.dwMaxBitRate		= cpu_to_le32(10000000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(640 * 360 * 2),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= cpu_to_le32(333333),	/* 30 fps */
	.dwFrameInterval[1]	= cpu_to_le32(400000),	/* 25 fps */
	.dwFrameInterval[2]	= cpu_to_le32(666666),	/* 15 fps */
};

/* --- Format 2: H.264 (Frame-Based) --- */

/*
 * UVC 1.5 frame-based format/frame descriptors for H.264.
 * The kernel headers define UVC_VS_FORMAT_FRAME_BASED (0x10) and
 * UVC_VS_FRAME_FRAME_BASED (0x11) but lack the corresponding structs.
 */

#define UVC_DT_FORMAT_FRAMEBASED_SIZE			28
#define UVC_DT_FRAME_FRAMEBASED_SIZE(n)			(26 + 4 * (n))

struct uvc_format_framebased {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bNumFrameDescriptors;
	__u8  guidFormat[16];
	__u8  bBitsPerPixel;
	__u8  bDefaultFrameIndex;
	__u8  bAspectRatioX;
	__u8  bAspectRatioY;
	__u8  bmInterfaceFlags;
	__u8  bCopyProtect;
	__u8  bVariableSize;
} __attribute__((packed));

#define DECLARE_UVC_FRAME_FRAMEBASED(n)				\
	struct UVC_FRAME_FRAMEBASED_##n {			\
		__u8  bLength;					\
		__u8  bDescriptorType;				\
		__u8  bDescriptorSubType;			\
		__u8  bFrameIndex;				\
		__u8  bmCapabilities;				\
		__u16 wWidth;					\
		__u16 wHeight;					\
		__u32 dwMinBitRate;				\
		__u32 dwMaxBitRate;				\
		__u32 dwDefaultFrameInterval;			\
		__u8  bFrameIntervalType;			\
		__u32 dwBytesPerLine;				\
		__u32 dwFrameInterval[];			\
	} __attribute__((packed))

DECLARE_UVC_FRAME_FRAMEBASED(3);

static const struct uvc_format_framebased uvc_format_h264 = {
	.bLength		= UVC_DT_FORMAT_FRAMEBASED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex		= 2,
	.bNumFrameDescriptors	= 3,
	.guidFormat		=
		{ 'H',  '2',  '6',  '4', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
	.bVariableSize		= 1,
};

static const struct UVC_FRAME_FRAMEBASED_3 uvc_frame_h264_1080p = {
	.bLength		= UVC_DT_FRAME_FRAMEBASED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1080),
	.dwMinBitRate		= cpu_to_le32(500000),
	.dwMaxBitRate		= cpu_to_le32(8000000),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 3,
	.dwBytesPerLine		= 0,
	.dwFrameInterval	= {
		cpu_to_le32(333333),	/* 30 fps */
		cpu_to_le32(400000),	/* 25 fps */
		cpu_to_le32(666666),	/* 15 fps */
	},
};

static const struct UVC_FRAME_FRAMEBASED_3 uvc_frame_h264_720p = {
	.bLength		= UVC_DT_FRAME_FRAMEBASED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(300000),
	.dwMaxBitRate		= cpu_to_le32(4000000),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 3,
	.dwBytesPerLine		= 0,
	.dwFrameInterval	= {
		cpu_to_le32(333333),	/* 30 fps */
		cpu_to_le32(400000),	/* 25 fps */
		cpu_to_le32(666666),	/* 15 fps */
	},
};

static const struct UVC_FRAME_FRAMEBASED_3 uvc_frame_h264_360p = {
	.bLength		= UVC_DT_FRAME_FRAMEBASED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(360),
	.dwMinBitRate		= cpu_to_le32(100000),
	.dwMaxBitRate		= cpu_to_le32(2000000),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 3,
	.dwBytesPerLine		= 0,
	.dwFrameInterval	= {
		cpu_to_le32(333333),	/* 30 fps */
		cpu_to_le32(400000),	/* 25 fps */
		cpu_to_le32(666666),	/* 15 fps */
	},
};

/* --- Color matching --- */

static const struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};

/* --------------------------------------------------------------------------
 * Descriptor arrays
 */

static const struct uvc_descriptor_header * const uvc_fs_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_fs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1080p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_360p,
	(const struct uvc_descriptor_header *) &uvc_format_h264,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_1080p,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_360p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_hs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1080p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_360p,
	(const struct uvc_descriptor_header *) &uvc_format_h264,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_1080p,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_360p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1080p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_360p,
	(const struct uvc_descriptor_header *) &uvc_format_h264,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_1080p,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_360p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

/* --------------------------------------------------------------------------
 * USB configuration
 */

static int __init
webcam_config_bind(struct usb_configuration *c)
{
	return uvc_bind_config(c, uvc_fs_control_cls, uvc_ss_control_cls,
		uvc_fs_streaming_cls, uvc_hs_streaming_cls,
		uvc_ss_streaming_cls);
}

static struct usb_configuration webcam_config_driver = {
	.label			= webcam_config_label,
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= CONFIG_USB_GADGET_VBUS_DRAW,
};

static int /* __init_or_exit */
webcam_unbind(struct usb_composite_dev *cdev)
{
	return 0;
}

static int __init
webcam_bind(struct usb_composite_dev *cdev)
{
	int ret;

	ret = usb_string_ids_tab(cdev, webcam_strings);
	if (ret < 0)
		goto error;
	webcam_device_descriptor.iManufacturer =
		webcam_strings[USB_GADGET_MANUFACTURER_IDX].id;
	webcam_device_descriptor.iProduct =
		webcam_strings[USB_GADGET_PRODUCT_IDX].id;
	webcam_config_driver.iConfiguration =
		webcam_strings[STRING_DESCRIPTION_IDX].id;

	if ((ret = usb_add_config(cdev, &webcam_config_driver,
					webcam_config_bind)) < 0)
		goto error;

	usb_composite_overwrite_options(cdev, &coverwrite);
	INFO(cdev, "Raptor Webcam Gadget\n");
	return 0;

error:
	webcam_unbind(cdev);
	return ret;
}

/* --------------------------------------------------------------------------
 * Driver
 */

static __refdata struct usb_composite_driver webcam_driver = {
	.name		= "g_webcam",
	.dev		= &webcam_device_descriptor,
	.strings	= webcam_device_strings,
	.max_speed	= USB_SPEED_SUPER,
	.bind		= webcam_bind,
	.unbind		= webcam_unbind,
};

static int __init
webcam_init(void)
{
	return usb_composite_probe(&webcam_driver);
}

static void __exit
webcam_cleanup(void)
{
	usb_composite_unregister(&webcam_driver);
}

module_init(webcam_init);
module_exit(webcam_cleanup);

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Raptor Webcam Gadget");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2.0");
