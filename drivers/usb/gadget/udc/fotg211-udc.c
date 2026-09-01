// SPDX-License-Identifier: GPL-2.0
/*
 * FOTG210 UDC Driver supports Bulk transfer so far
 *
 * Copyright (C) 2013 Faraday Technology Corporation
 *
 * Author : Yuan-Hsin Chen <yhchen@faraday-tech.com>
 */

#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <plat/efuse_protected.h>

#include "fotg211.h"
#include <linux/usb/video.h>
#include <linux/usb/cdc.h>
#include "fotg211-EP.h"
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#include <plat/nvt-sramctl.h>

#define	DRIVER_DESC	"FOTG211 USB Device Controller Driver"
#define	DRIVER_VERSION	"1.00.022"

#define USE_TASKLET_FOR_IRQ 0


static bool is_cdc_class = 0;
u32 vbus_gpio_no = D_GPIO(7);
static const char udc_name[] = "fotg211_udc";
//static const char * const fotg210_ep_name[] = {
//	"ep0", "ep1", "ep2", "ep3", "ep4"};

static const struct {
	const char *name;
	const struct usb_ep_caps caps;
} ep_info[] = {
#define EP_INFO(_name, _caps) \
	{ \
		.name = _name, \
		.caps = _caps, \
	}

	EP_INFO("ep0",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_CONTROL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep1",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep2",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep3",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep4",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep5",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep6",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep7",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep8",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep9",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep10",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep11",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep12",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep13",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep14",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep15",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
#undef EP_INFO
};

unsigned int outslice = 0;
module_param_named(outslice, outslice, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(outslice, "Fot class(CDC-NCM) out size is unknown, the use of short packet to terminate out");

static unsigned debug_on = 0;
module_param(debug_on, uint, S_IRUGO);
MODULE_PARM_DESC(debug_on, "Whether to enable debug message. Default = 0");

static unsigned force_fs = 0;
module_param(force_fs, uint, S_IRUGO);
MODULE_PARM_DESC(force_fs, "Whether to force fullspeed. Default = 0");


static int do_test_packet = 0;

#define numsg		pr_info
#define itfnumsg	pr_info
#define devnumsg	pr_info
#define ep0numsg	pr_info

static int fotg210_cable_out_clear(struct fotg210_ep *ep)
{
	u32 value, iep_length;
	u32 dma_start;
	int length = 0;

	pr_info("cable_out\n");
	pr_info("ep is %d, fifo is %d\n", ep->epnum, gEPMap[ep->epnum - 1]);

	value = ioread32(ep->fotg210->reg + 0x1C4);
	value |= DMACPSR1_DMA_ABORT;
	iowrite32(value, ep->fotg210->reg + 0x1C4);

	if (ep->dir_in) {
		iep_length = ioread32(ep->fotg210->reg + FOTG210_INEPMPSR(ep->epnum));
		/* reset fifo */
		do {
			pr_info("IN case, remaing DMA len is 0x%x\n", ioread32(ep->fotg210->reg + 0x1D4));

			dma_start = ioread32(ep->fotg210->reg + FOTG210_DMACPSR1);

			if (ep->epnum) {
				value = ioread32(ep->fotg210->reg +
						FOTG210_FIBCR(gEPMap[ep->epnum - 1]));

				length = value&FIBCR_BCFX;

				pr_info("FIFO lenght = 0x%x, ep max length is 0x%x\n", length, iep_length);
				value = value|FIBCR_FFRST;
				iowrite32(value, ep->fotg210->reg +
						FOTG210_FIBCR(gEPMap[ep->epnum - 1]));
			} else {
				value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
				value |= DCFESR_CX_CLR;
				iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);
			}
		} while (dma_start & 0x1);
	} else {
		do {
			dma_start = ioread32(ep->fotg210->reg + FOTG210_DMACPSR1);
			pr_info("OUT case, wait dma_start clear\n");
		} while (dma_start & 0x1);
	}
	pr_info("cable_out pass\n");

	value = ioread32(ep->fotg210->reg + 0x1C4);
	value &= ~DMACPSR1_DMA_ABORT;
	iowrite32(value, ep->fotg210->reg + 0x1C4);

	return 1;
}

static void fotg210_disable_fifo_int(struct fotg210_ep *ep)
{
	u32 value;

	if (ep->dir_in)
	{
		value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);
		value |= DMISGR1_MF_IN_INT(gEPMap[ep->epnum - 1]);

		iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);
	}
	else
	{
		if(gEPMap[ep->epnum - 1] < 8)
			value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);
		else
			value = ioread32(ep->fotg210->reg + FOTG210_DMISGR2);

		value |= DMISGR1_MF_OUTSPK_INT(gEPMap[ep->epnum - 1]);

		if(gEPMap[ep->epnum - 1] < 8)
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);
		else
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR2);
	}
}

static void fotg210_enable_fifo_int(struct fotg210_ep *ep)
{
	u32 value;

	if (ep->dir_in)
	{
		value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);
		value &= ~DMISGR1_MF_IN_INT(gEPMap[ep->epnum - 1]);

		iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);
	}
	else
	{
		if(gEPMap[ep->epnum - 1] < 8)
			value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);
		else
			value = ioread32(ep->fotg210->reg + FOTG210_DMISGR2);

		value &= ~DMISGR1_MF_OUTSPK_INT(gEPMap[ep->epnum - 1]);

		if(gEPMap[ep->epnum - 1] < 8)
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);
		else
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR2);
	}
}

static void fotg210_set_cxdone(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DCFESR);

	value |= DCFESR_CX_DONE;
	iowrite32(value, fotg210->reg + FOTG210_DCFESR);
}

static void fotg210_done(struct fotg210_ep *ep, struct fotg210_request *req,
			int status)
{
	if (debug_on)
		numsg("fotg210_done\n");

	list_del_init(&req->queue);

	/* don't modify queue heads during completion callback */
	if (ep->fotg210->gadget.speed == USB_SPEED_UNKNOWN)
		req->req.status = -ESHUTDOWN;
	else
		req->req.status = status;

	if(req->req.complete != NULL) {
		spin_unlock(&ep->fotg210->lock);
		usb_gadget_giveback_request(&ep->ep, &req->req);
		spin_lock(&ep->fotg210->lock);
	}

	if (ep->epnum) {
		if (list_empty(&ep->queue))
			fotg210_disable_fifo_int(ep);
	} else {
		fotg210_set_cxdone(ep->fotg210);
	}
}

static void USB_setFIFOCfg(struct fotg210_ep *ep, USB_FIFO_NUM FIFOn, USB_EP_TYPE BLK_TYP, USB_EP_BLKNUM BLKNO, BOOL BLKSZ, USB_FIFO_DIR Dir)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 fifocfg = ioread32(fotg210->reg + FOTG210_FIFOCF_03);
	u32 fifocfg_dir = ioread32(fotg210->reg + FOTG210_FIFOMAP_03);

	u32 fifocfg2 = ioread32(fotg210->reg + FOTG210_FIFOCF_47);
	u32 fifocfg2_dir = ioread32(fotg210->reg + FOTG210_FIFOMAP_47);

	u32 fifocfg3 = ioread32(fotg210->reg + FOTG210_FIFOCF_811);
	u32 fifocfg3_dir = ioread32(fotg210->reg + FOTG210_FIFOMAP_811);

	u32 fifocfg4 = ioread32(fotg210->reg + FOTG210_FIFOCF_1215);
        u32 fifocfg4_dir = ioread32(fotg210->reg + FOTG210_FIFOMAP_1215);

	switch (FIFOn) {
	case USB_FIFO0: {
		if (!((BLKNO == BLKNUM_TRIPLE) && BLKSZ && (ep->type == USB_ENDPOINT_XFER_ISOC))) {
			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg |= FIFOCF_FIFO_EN_NVT(1, FIFOn);


			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				if (BLKSZ == 1) {
					// Block size : 512~1024 bytes
					fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}

			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);
			}
		} else {
			// Triplex1024: ping pong trigger.
			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-2, FIFOn);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg |= FIFOCF_FIFO_EN_NVT(1, FIFOn);
			fifocfg	|= FIFOCF_FIFO_PP_EN(1, FIFOn);

			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+1);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
			fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			fifocfg |= FIFOCF_FIFO_PP_EN(1, FIFOn+1);

			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+2);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
			fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);
			fifocfg |= FIFOCF_FIFO_PP_EN(1, FIFOn+2);

			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+3);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
			fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
			fifocfg |= FIFOCF_FIFO_PP_EN(1, FIFOn+3);

			//fifo4~fifo7
			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+4);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);
			fifocfg2 |= FIFOCF_FIFO_PP_EN(1, FIFOn+4);


			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+5);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
			fifocfg2 |= FIFOCF_FIFO_PP_EN(1, FIFOn+5);


			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+6);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+6);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+6);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+6);
			fifocfg2 |= FIFOCF_FIFO_PP_EN(1, FIFOn+6);


			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+6);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+6);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+6);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+6);
			fifocfg2 |= FIFOCF_FIFO_PP_EN(1, FIFOn+6);


			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+7);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+7);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+7);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+7);
			fifocfg2 |= FIFOCF_FIFO_PP_EN(1, FIFOn+7);

			// fifo8~fifo11
			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+8);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+8);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+8);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+8);
			fifocfg3 |= FIFOCF_FIFO_PP_EN(1, FIFOn+8);


			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+9);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+9);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+9);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+9);
			fifocfg3 |= FIFOCF_FIFO_PP_EN(1, FIFOn+9);


			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+10);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+10);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+10);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+10);
			fifocfg3 |= FIFOCF_FIFO_PP_EN(1, FIFOn+10);


			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+11);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-2, FIFOn+11);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+11);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+11);
			fifocfg3 |= FIFOCF_FIFO_PP_EN(1, FIFOn+11);
		}
	}
	break;

	case USB_FIFO1: {
			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				if (BLKSZ == 1) {
					fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					//FIFO4~
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					//FIFO4~
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;

	case USB_FIFO2: {
			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					//FIFO4~
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
                                fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
                                fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
                                fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
                                fifocfg |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				//FIFO4~
                                fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
                                fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
                                fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
                                fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

                                if (BLKSZ == 1) {
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
                                        fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
                                        fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
                                }

			}
		}
		break;

	case USB_FIFO3: {
			fifocfg |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				//FIFO4~
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				//FIFO4~
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				//FIFO4~
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					//FIFO8~
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
	}
	break;

	case USB_FIFO4: {
			//FIFO4~
			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg2_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg2_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg2_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					//FIFO8~
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;

	case USB_FIFO5: {
			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg2_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg2_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg2_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				//FIFO4~
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					//FIFO8~
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					//FIFO8
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;

	case USB_FIFO6: {
			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg2_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg2_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg2_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					//FIFO8~
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg2 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				//FIFO8~
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;

	case USB_FIFO7: {
			fifocfg2 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg2 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg2 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg2_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg2_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg2_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg2 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				//FIFO8~
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				//FIFO8~
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				//FIFO8~
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					//FIFO12~
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;

	case USB_FIFO8: {
			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg3_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg3_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg3_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					//FIFO12~
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;


	case USB_FIFO9: {
			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg3_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg3_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg3_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					//FIFO12~
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					//FIFO12~
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;

	case USB_FIFO10: {
			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg3_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg3_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg3_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					//FIFO12~
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg3 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				//FIFO12~
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+4);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+4);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+4);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+4);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+5);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+5);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+5);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+5);
				}
			}
		}
		break;

	case USB_FIFO11: {
			fifocfg3 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg3 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg3 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg3_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg3_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg3_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg3 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				//FIFO12~
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				//FIFO12~
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					pr_err("space not enough\n");
				}
			}
		}
		break;

	case USB_FIFO12: {
			fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg4_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg4_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg4_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg4 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

					fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+3);
					fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+3);
					fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+3);
					fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+3);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					pr_err("space not enough\n");
				}
			}
		}
		break;

	case USB_FIFO13: {
			fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg4_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg4_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg4_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg4 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					pr_err("space not enough\n");
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+2);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+2);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+2);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+2);

				if (BLKSZ == 1) {
					pr_err("space not enough\n");
				}
			}
		}
		break;


	case USB_FIFO14: {
			fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg4_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg4_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg4_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
			fifocfg4 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == 1)) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);
			} else if (BLKNO == BLKNUM_DOUBLE) {
				fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn+1);
				fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn+1);
				fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn+1);
				fifocfg4 |= FIFOCF_FIFO_EN_NVT(0, FIFOn+1);

				// Block size : 512~1024 bytes
				if (BLKSZ == 1) {
					pr_err("space not enough\n");
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				pr_err("space not enough\n");
			}
		}
		break;

	case USB_FIFO15: {
			fifocfg4 |= FIFOCF_TYPE_NVT(BLK_TYP, FIFOn);
			fifocfg4 |= FIFOCF_BLK_NO(BLKNO-1, FIFOn);
			fifocfg4 |= FIFOCF_BLKSZ_NVT(BLKSZ, FIFOn);
			fifocfg4_dir |= FIFOMAP_DIR(Dir, FIFOn);
			fifocfg4_dir &= ~FIFOMAP_EPNOMSK_NVT(FIFOn); // fifomap
			fifocfg4_dir |= FIFOMAP_EPNO_NVT(ep->epnum, FIFOn);
                        fifocfg4 |= FIFOCF_FIFO_EN_NVT(1, FIFOn);

			if ((BLKNO > BLKNUM_SINGLE) || (BLKSZ == 1)) {
				pr_err("space not enough\n");
			}
		}
		break;


	default:
		break;

	}

	iowrite32(fifocfg, fotg210->reg + FOTG210_FIFOCF_03);
	iowrite32(fifocfg_dir, fotg210->reg + FOTG210_FIFOMAP_03);
	iowrite32(fifocfg2, fotg210->reg + FOTG210_FIFOCF_47);
	iowrite32(fifocfg2_dir, fotg210->reg + FOTG210_FIFOMAP_47);
	iowrite32(fifocfg3, fotg210->reg + FOTG210_FIFOCF_811);
	iowrite32(fifocfg3_dir, fotg210->reg + FOTG210_FIFOMAP_811);
	iowrite32(fifocfg4, fotg210->reg + FOTG210_FIFOCF_1215);
	iowrite32(fifocfg4_dir, fotg210->reg + FOTG210_FIFOMAP_1215);

}
static void fotg210_fifo_ep_mapping(struct fotg210_ep *ep, u32 epnum,
                                u32 dir_in, USB_FIFO_NUM FIFOn)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	/* Driver should map an ep to a fifo and then map the fifo
	 * to the ep. What a brain-damaged design!
	 */

	/* map a fifo to an ep */
	//1. Update registers
	if (epnum <= 4) {
		val = ioread32(fotg210->reg + FOTG210_EPMAP_14);
		val &= ~ EPMAP_FIFONOMSK_NVT(epnum, dir_in);
		val |= EPMAP_FIFONO_NVT(epnum, FIFOn, dir_in);
		iowrite32(val, fotg210->reg + FOTG210_EPMAP_14);
	} else if (epnum <= 8){
		val = ioread32(fotg210->reg + FOTG210_EPMAP_58);
		val &= ~ EPMAP_FIFONOMSK_NVT(epnum, dir_in);
		val |= EPMAP_FIFONO_NVT(epnum, FIFOn, dir_in);
		iowrite32(val, fotg210->reg + FOTG210_EPMAP_58);
	} else if (epnum <= 12){
		val = ioread32(fotg210->reg + FOTG210_EPMAP_912);
		val &= ~ EPMAP_FIFONOMSK_NVT(epnum, dir_in);
		val |= EPMAP_FIFONO_NVT(epnum, FIFOn, dir_in);
		iowrite32(val, fotg210->reg + FOTG210_EPMAP_912);
	} else {
		val = ioread32(fotg210->reg + FOTG210_EPMAP_1315);
		val &= ~ EPMAP_FIFONOMSK_NVT(epnum, dir_in);
		val |= EPMAP_FIFONO_NVT(epnum, FIFOn, dir_in);
		iowrite32(val, fotg210->reg + FOTG210_EPMAP_1315);
	}
	//2. update mapping table
	gEPMap[epnum-1] = FIFOn;


	/* map the ep to the fifo */
	// 1. update mapping table
	if (dir_in == USB_FIFO_IN) {
		gFIFOInMap[FIFOn] = epnum;
	} else {
		gFIFOOutMap[FIFOn] = epnum;
	}

	// 2. set fifo config, and caculate the fifo no for next EP.
	USB_setFIFOCfg(ep, FIFOn, ep->type, gEPBlkNo[epnum-1], (gEPBlkSz[epnum-1] > 512), dir_in);
}

static void fotg210_set_mps(struct fotg210_ep *ep, u32 epnum, u32 mps,
				u32 dir_in)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;
	u32 offset = dir_in ? FOTG210_INEPMPSR(epnum) :
				FOTG210_OUTEPMPSR(epnum);

	val = ioread32(fotg210->reg + offset);
	val &= ~INOUTEPMPSR_MPS(0x7FF);
	val |= INOUTEPMPSR_MPS(mps);

	// select HBW(1/2/3)
	val &= ~(0x3<<13);
	if((ep->type == USB_ENDPOINT_XFER_ISOC) && (ep->dir_in)) { // modify hbm while iso_in case
		val |= (HBM_ISOI<<13);
	}

	iowrite32(val, fotg210->reg + offset);
}

#define CLEARMASK 0xFFFFFFFF
static int fotg210_config_ep(struct fotg210_ep *ep,
		     const struct usb_endpoint_descriptor *desc)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	int32_t FIFOn = 0;
	int i = 0;

	fotg210_set_mps(ep, ep->epnum, ep->ep.maxpacket, ep->dir_in); // set packet max size and HBM

	for (i = 0; i < ep->epnum-1; i++) {
		FIFOn += gEPBlkNo[i];

		if (gEPBlkSz[i] > 512)
			 FIFOn += gEPBlkNo[i];
	}

	fotg210_fifo_ep_mapping(ep, ep->epnum, ep->dir_in, FIFOn);
	fotg210->ep[ep->epnum] = ep;

	return 0;
}

static int fotg210_ep_enable(struct usb_ep *_ep,
			  const struct usb_endpoint_descriptor *desc)
{
	struct fotg210_ep *ep;

	if (debug_on)
		numsg("fotg210_ep_enable\n");

	ep = container_of(_ep, struct fotg210_ep, ep);

	ep->desc = desc;
	ep->epnum = usb_endpoint_num(desc);
	ep->type = usb_endpoint_type(desc);
	ep->dir_in = usb_endpoint_dir_in(desc);
	ep->ep.maxpacket = usb_endpoint_maxp(desc);

	if (debug_on) {
		itfnumsg("fotg210_ep_enable\n");
		itfnumsg("ep-desc len=0x%X type=0x%X epaddr=0x%X attr=0x%X MaxPkt=0x%X intval=0x%X\n",desc->bLength,desc->bDescriptorType
			,desc->bEndpointAddress,desc->bmAttributes,desc->wMaxPacketSize,desc->bInterval);
	}

	return fotg210_config_ep(ep, desc);
}

static void fotg210_reset_tseq(struct fotg210_udc *fotg210, u8 epnum)
{
	struct fotg210_ep *ep = fotg210->ep[epnum];
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(epnum);

	/* Note: Driver needs to set and clear INOUTEPMPSR_RESET_TSEQ
	 *	 bit. Controller wouldn't clear this bit. WTF!!!
	 */

	value = ioread32(reg);
	value |= INOUTEPMPSR_RESET_TSEQ;
	iowrite32(value, reg);

	value = ioread32(reg);
	value &= ~INOUTEPMPSR_RESET_TSEQ;
	iowrite32(value, reg);
}

static int fotg210_ep_release(struct fotg210_ep *ep)
{
	if (!ep->epnum)
		return 0;
	ep->epnum = 0;
	ep->stall = 0;
	ep->wedged = 0;

	fotg210_reset_tseq(ep->fotg210, ep->epnum);

	return 0;
}

static int fotg210_ep_disable(struct usb_ep *_ep)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;

	BUG_ON(!_ep);

	ep = container_of(_ep, struct fotg210_ep, ep);

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next,
			struct fotg210_request, queue);
		spin_lock_irqsave(&ep->fotg210->lock, flags);
		fotg210_done(ep, req, -ECONNRESET);
		spin_unlock_irqrestore(&ep->fotg210->lock, flags);
	}

	return fotg210_ep_release(ep);
}

static struct usb_request *fotg210_ep_alloc_request(struct usb_ep *_ep,
						gfp_t gfp_flags)
{
	struct fotg210_request *req;

	req = kzalloc(sizeof(struct fotg210_request), gfp_flags);
	if (!req)
		return NULL;

	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void fotg210_ep_free_request(struct usb_ep *_ep,
					struct usb_request *_req)
{
	struct fotg210_request *req;

	req = container_of(_req, struct fotg210_request, req);
	kfree(req);
}

#define BC_STUCK_CHK_CNT 300
static void fotg210_enable_dma(struct fotg210_ep *ep,
			      dma_addr_t d, u32 len)
{
	u32 value;
	struct fotg210_udc *fotg210 = ep->fotg210;
	bool cable_out_ret = 0;
	u32 pre_bc = 0;
	u32 cur_bc = 0;
	u32 count = 0;
	u32 pre_dma_remain = 0;
	u32 cur_dma_remain = 0;

	if (debug_on)
		numsg("fotg210_enable_dma ep%d len=%d\n", ep->epnum, len);

	do {
		if (debug_on) {
			pr_info("fotg210_enable_dma- wait dma not start\n");
		}
		value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	}while(value & DMACPSR1_DMA_START);

	/* check if fifo empty */
	if (is_cdc_class) {
		if ((ep->epnum)&&(ep->dir_in)) {
			do {
				if (debug_on) {
					pr_info("cdc case check fifo reset\n");
				}

				if(gEPMap[ep->epnum -1] < 8 )
				{
					value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
				}
				else
				{
					value = ioread32(ep->fotg210->reg + FOTG210_DICR);
				}
			} while (!(value & DCFESR_FIFO_EMPTY(gEPMap[ep->epnum -1])));
		}
	} else {
		if ((ep->epnum)&&(ep->dir_in)) {
			do {
				udelay(50);
				// check whether fifo stuck normally during DMA
				cur_bc = (ioread32(ep->fotg210->reg + FOTG210_FIBCR(gEPMap[ep->epnum -1])) & FIBCR_BCFX);
				pre_dma_remain = ioread32(ep->fotg210->reg + 0x1D4);
				if (debug_on) {
					pr_info("[fifo reset]cur_bc is 0x%x, pre_bc is 0x%x\n", cur_bc, pre_bc);
					pr_info("[fifo reset]cur_dma_remain is 0x%x, pre_dma_remain is 0x%x\n", cur_dma_remain, pre_dma_remain);
				}

				if ((cur_bc == pre_bc) && (cur_dma_remain == pre_dma_remain)) {
					count++;
					if (count == BC_STUCK_CHK_CNT) {
						pr_info("count is %d times\n", count);
						cable_out_ret = fotg210_cable_out_clear(ep);
						return;
					}
				} else {
					pre_bc = cur_bc;
					pre_dma_remain = cur_dma_remain;
					count = 0;
				}

				if(gEPMap[ep->epnum -1] < 8 )
				{
					value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
				}
				else
				{
					value = ioread32(ep->fotg210->reg + FOTG210_DICR);
				}
			} while (!(value & DCFESR_FIFO_EMPTY(gEPMap[ep->epnum -1])));
		}
	}

	/* set transfer length and direction */
	value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	value &= ~(DMACPSR1_DMA_LEN(0x7FFFFF) | DMACPSR1_DMA_TYPE(1));
	value |= DMACPSR1_DMA_LEN(len) | DMACPSR1_DMA_TYPE(ep->dir_in);
	iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);

	/* set device DMA target FIFO number */
	value = ioread32(fotg210->reg + FOTG210_DMATFNR);
	if (ep->epnum) {
		value = DMATFNR_ACC_FN_NVT(gEPMap[ep->epnum -1]);
	} else {
		value = DMATFNR_ACC_CXF;
	}
	//value = DMATFNR_ACC_FN(ep->epnum - 1);
	iowrite32(value, fotg210->reg + FOTG210_DMATFNR);

	/* set DMA memory address */
	iowrite32(d, fotg210->reg + FOTG210_DMACPSR2);

	/* enable MDMA_EROR and MDMA_CMPLT interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMISGR2);
	value &= ~(DMISGR2_MDMA_CMPLT | DMISGR2_MDMA_ERROR);
	iowrite32(value, fotg210->reg + FOTG210_DMISGR2);

	/* start DMA */
	value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	value |= DMACPSR1_DMA_START;
	iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);
}

static void fotg210_disable_dma(struct fotg210_ep *ep)
{
	//iowrite32(DMATFNR_DISDMA, ep->fotg210->reg + FOTG210_DMATFNR);
}

static void fotg210_wait_dma_done(struct fotg210_ep *ep)
{
	u32 value;
	unsigned long timeout = 0;

	bool cable_out_ret = 0;
	u32 vbus_gpio = 0;
	u32 pre_bc = 0;
	u32 cur_bc = 0;
	u32 count = 0;
	u32 pre_dma_remain = 0;
	u32 cur_dma_remain = 0;

	timeout = jiffies + msecs_to_jiffies(1000);

	if (debug_on)
		numsg("fotg210_wait_dma_done\n");

	if (is_cdc_class) {
		do {
			if (debug_on) {
				pr_info("cdc case check dma cmplt\n");
			}
			value = ioread32(ep->fotg210->reg + FOTG210_DISGR2);
			if ((value & DISGR2_USBRST_INT) ||
					(value & DISGR2_DMA_ERROR))
				goto dma_reset;

			if (time_after(jiffies, timeout)) {
				pr_err("%s timeout\n", __func__);
				goto dma_reset;
			}
			cpu_relax();

		} while (!(value & DISGR2_DMA_CMPLT));

		value |= DISGR2_DMA_CMPLT;
		iowrite32(value, ep->fotg210->reg + FOTG210_DISGR2);

	} else {
		do {
			udelay(50);
			// check whether plugout cable during DMA
			vbus_gpio = gpio_get_value(vbus_gpio_no);
			if (!vbus_gpio) {
				if (debug_on) {
					pr_info("[dma complete]vbus_gpio is %d\n", vbus_gpio);

				}
			}

			// check whether fifo stuck normally during DMA
			cur_bc = (ioread32(ep->fotg210->reg + FOTG210_FIBCR(gEPMap[ep->epnum -1])) & FIBCR_BCFX);
			cur_dma_remain = ioread32(ep->fotg210->reg + 0x1D4);
			if (debug_on) {
				pr_info("[dma complt]cur_bc is 0x%x, pre_bc is 0x%x\n", cur_bc, pre_bc);
				pr_info("[dma complt]cur_dma_remain is 0x%x, pre_dma_remain is 0x%x\n", cur_dma_remain, pre_dma_remain);
			}

			if ((cur_bc == pre_bc) && (cur_dma_remain == pre_dma_remain)) {
				count++;
				if (count == BC_STUCK_CHK_CNT) {
					pr_info("count is %d times\n", count);
					cable_out_ret = fotg210_cable_out_clear(ep);
					value |= DISGR2_DMA_CMPLT;
					iowrite32(value, ep->fotg210->reg + FOTG210_DISGR2);
					return;
				}
			} else {
				pre_bc = cur_bc;
				pre_dma_remain = cur_dma_remain;
				count = 0;
			}

			value = ioread32(ep->fotg210->reg + FOTG210_DISGR2);
			if ((value & DISGR2_USBRST_INT) ||
					(value & DISGR2_DMA_ERROR))
				goto dma_reset;

			if (time_after(jiffies, timeout)) {
				pr_err("%s timeout\n", __func__);
				goto dma_reset;
			}
			cpu_relax();

		} while (!(value & DISGR2_DMA_CMPLT));

		value |= DISGR2_DMA_CMPLT;
		iowrite32(value, ep->fotg210->reg + FOTG210_DISGR2);
	}

	if (debug_on) {
		pr_info("successful dma wait return\n");
	}
	return;

dma_reset:
	value = ioread32(ep->fotg210->reg + FOTG210_DMACPSR1);
	value |= DMACPSR1_DMA_ABORT;
	iowrite32(value, ep->fotg210->reg + FOTG210_DMACPSR1);

	/* reset fifo */
	if (ep->epnum) {
		value = ioread32(ep->fotg210->reg +
				FOTG210_FIBCR(gEPMap[ep->epnum -1]));
		value |= FIBCR_FFRST;
		iowrite32(value, ep->fotg210->reg +
				FOTG210_FIBCR(gEPMap[ep->epnum -1]));
	} else {
		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value |= DCFESR_CX_CLR;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);
	}
}

#if 0
static void ivot_usbdev_start_ep0_data(struct fotg210_ep *ep,
			struct fotg210_request *req)
{
	u32 *buffer;
	u32 value,length,i=0;
	s32	opsize;

	buffer = (u32 *)(req->req.buf + req->req.actual);

	if (req->req.length - req->req.actual > 64)  {
		length = 64;

		if (ep->dir_in) {
			value = ioread32(ep->fotg210->reg +
						FOTG210_DMISGR0);
			value &= ~DMISGR0_MCX_IN_INT;
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
		}

	} else {
		length = req->req.length - req->req.actual;

		if (ep->dir_in) {
			value = ioread32(ep->fotg210->reg +
						FOTG210_DMISGR0);
			value |= DMISGR0_MCX_IN_INT;
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
		}
	}

	value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
	if(value & DCFESR_CX_DATAPORT_EN)
		pr_err("DATAPORT EN ERROR!!!\n");

	if (ep->dir_in) {
		if (debug_on)
			ep0numsg("ivot_usbdev_start_ep0_data IN 0x%X  act=0x%X\n",req->req.length,req->req.actual);

		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value &= ~DCFESR_CX_FNT_IN;
		value |= (length<<16);
		value |= DCFESR_CX_DATAPORT_EN;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);


		opsize = length;
		while(opsize>0)
		{
			iowrite32(buffer[i++], ep->fotg210->reg + FOTG210_CXDATAPORT);
			opsize-=4;
		}
	} else {
		if (debug_on)
			ep0numsg("ivot_usbdev_start_ep0_data OUT 0x%X  act=0x%X\n",req->req.length,req->req.actual);

		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value |= DCFESR_CX_DATAPORT_EN;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);

		opsize = (ioread32(ep->fotg210->reg + FOTG210_DCFESR) & DCFESR_CX_FNT_OUT)>>24;
		while(!opsize) {
			msleep(1);
			opsize = (ioread32(ep->fotg210->reg + FOTG210_DCFESR) & DCFESR_CX_FNT_OUT)>>24;
		}

		while(opsize>0)
		{
			value = ioread32(ep->fotg210->reg + FOTG210_CXDATAPORT);

			if(opsize>=4) {
				buffer[i++] = value;
			} else if (opsize==3) {
				buffer[i] &= ~0xFFFFFF;
				value &= 0xFFFFFF;
				buffer[i] += value;
			} else if (opsize==2) {
				buffer[i] &= ~0xFFFF;
				value &= 0xFFFF;
				buffer[i] += value;
			} else if (opsize==1) {
				buffer[i] &= ~0xFF;
				value &= 0xFF;
				buffer[i] += value;
			}
			opsize-=4;
		}


	}

	value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
	value &= ~DCFESR_CX_DATAPORT_EN;
	iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);

	/* update actual transfer length */
	req->req.actual += length;

}
#endif

static void fotg210_start_dma(struct fotg210_ep *ep,
			struct fotg210_request *req)
{
	dma_addr_t d;
	u8 *buffer;
	u32 length,slice,szOp;

	if (debug_on)
		numsg("fotg210_start_dma\n");

	if (ep->epnum) {
		if (ep->dir_in) {
			buffer = req->req.buf + req->req.actual;
			length = req->req.length - req->req.actual;
		} else {
			buffer = req->req.buf + req->req.actual;
			length = ioread32(ep->fotg210->reg +
					FOTG210_FIBCR(gEPMap[ep->epnum - 1]));

			length &= FIBCR_BCFX;

			if(length < 512)
				length = length;//short packet
			else
				length = req->req.length - req->req.actual;

			if (outslice && (ep->type == USB_ENDPOINT_XFER_BULK)) {
				if(length > ep->ep.maxpacket)
					length = ep->ep.maxpacket;
			}
		}
	} else {
		buffer = req->req.buf + req->req.actual;
		length = req->req.length - req->req.actual;

		if(length > 64)
			length = 64;
	}

	szOp = 0;

	while(szOp<length) {

		//if((length - szOp) > 4096)
		//	slice = 4096;
		//else
			slice = (length - szOp);

		d = dma_map_single(NULL, (u8 *)(buffer+szOp), slice,
				ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

		if (dma_mapping_error(NULL, d)) {
			pr_err("dma_mapping_error\n");
			return;
		}

		dma_sync_single_for_device(NULL, d, slice,
					   ep->dir_in ? DMA_TO_DEVICE :
						DMA_FROM_DEVICE);

		fotg210_enable_dma(ep, d, slice);

		/* check if dma is done */
		fotg210_wait_dma_done(ep);

		fotg210_disable_dma(ep);

		szOp+=slice;
		dma_unmap_single(NULL, d, slice, DMA_TO_DEVICE);
	}

	/* update actual transfer length */
	req->req.actual += length;
}

static void fotg210_ep0_queue(struct fotg210_ep *ep,
				struct fotg210_request *req)
{
	if (!req->req.length) {
		fotg210_done(ep, req, 0);
		return;
	}
	if (ep->dir_in) { /* if IN */
		if (req->req.length) {
			//ivot_usbdev_start_ep0_data(ep, req);
			fotg210_start_dma(ep, req);
		} else {
			pr_err("%s : req->req.length = 0x%x\n",
			       __func__, req->req.length);
		}
		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket))
			fotg210_done(ep, req, 0);
	} else { /* OUT */
		/* For set_feature(TEST_PACKET) */
		if (do_test_packet && req->req.length == 53) {
			ep->dir_in = 1;
			fotg210_start_dma(ep, req);
			do_test_packet = 0;
		} else {
			u32 value = ioread32(ep->fotg210->reg + FOTG210_DMISGR0);

			value &= ~DMISGR0_MCX_OUT_INT;
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
		}
	}
}

static int fotg210_ep_queue(struct usb_ep *_ep, struct usb_request *_req,
				gfp_t gfp_flags)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;
	int request = 0;

	ep = container_of(_ep, struct fotg210_ep, ep);
	req = container_of(_req, struct fotg210_request, req);

	if (ep->fotg210->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	spin_lock_irqsave(&ep->fotg210->lock, flags);

	if (list_empty(&ep->queue))
		request = 1;

	list_add_tail(&req->queue, &ep->queue);

	req->req.actual = 0;
	req->req.status = -EINPROGRESS;

	if (!ep->epnum) /* ep0 */
		fotg210_ep0_queue(ep, req);
	else if (request && !ep->stall)
		fotg210_enable_fifo_int(ep);

	spin_unlock_irqrestore(&ep->fotg210->lock, flags);

	return 0;
}

static int fotg210_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;

	ep = container_of(_ep, struct fotg210_ep, ep);
	req = container_of(_req, struct fotg210_request, req);

	spin_lock_irqsave(&ep->fotg210->lock, flags);
	if (!list_empty(&ep->queue))
		fotg210_done(ep, req, -ECONNRESET);
	spin_unlock_irqrestore(&ep->fotg210->lock, flags);

	return 0;
}

static void fotg210_set_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	/* check if IN FIFO is empty before stall */
	if (ep->dir_in) {
		do {
			value = ioread32(fotg210->reg + FOTG210_DCFESR);
		} while (!(value & DCFESR_FIFO_EMPTY(gEPMap[ep->epnum - 1])));
	}
	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	value |= INOUTEPMPSR_STL_EP;
	iowrite32(value, reg);
}

static void fotg210_clear_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	value &= ~INOUTEPMPSR_STL_EP;
	iowrite32(value, reg);
}

static int fotg210_set_halt_and_wedge(struct usb_ep *_ep, int value, int wedge)
{
	struct fotg210_ep *ep;
	struct fotg210_udc *fotg210;
	unsigned long flags;
	int ret = 0;

	ep = container_of(_ep, struct fotg210_ep, ep);

	fotg210 = ep->fotg210;

	spin_lock_irqsave(&ep->fotg210->lock, flags);

	if (value) {
		fotg210_set_epnstall(ep);
		ep->stall = 1;
		if (wedge)
			ep->wedged = 1;
	} else {
		fotg210_reset_tseq(fotg210, ep->epnum);
		fotg210_clear_epnstall(ep);
		ep->stall = 0;
		ep->wedged = 0;
		if (!list_empty(&ep->queue))
			fotg210_enable_fifo_int(ep);

	}

	spin_unlock_irqrestore(&ep->fotg210->lock, flags);
	return ret;
}

static int fotg210_ep_set_halt(struct usb_ep *_ep, int value)
{
	return fotg210_set_halt_and_wedge(_ep, value, 0);
}

static int fotg210_ep_set_wedge(struct usb_ep *_ep)
{
	return fotg210_set_halt_and_wedge(_ep, 1, 1);
}

static void fotg210_ep_fifo_flush(struct usb_ep *_ep)
{
}

static const struct usb_ep_ops fotg210_ep_ops = {
	.enable		= fotg210_ep_enable,
	.disable	= fotg210_ep_disable,

	.alloc_request	= fotg210_ep_alloc_request,
	.free_request	= fotg210_ep_free_request,

	.queue		= fotg210_ep_queue,
	.dequeue	= fotg210_ep_dequeue,

	.set_halt	= fotg210_ep_set_halt,
	.fifo_flush	= fotg210_ep_fifo_flush,
	.set_wedge	= fotg210_ep_set_wedge,
};

static void fotg210_set_tx0byte(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;
	u32 offset = (ep->dir_in) ? FOTG210_INEPMPSR(ep->epnum) :
				FOTG210_OUTEPMPSR(ep->epnum);
	bool cable_out_ret = 0;
	u32 vbus_gpio = 0;
	u32 pre_bc = 0;
	u32 cur_bc = 0;
	u32 count = 0;
	u32 pre_dma_remain = 0;
	u32 cur_dma_remain = 0;

	val = ioread32(fotg210->reg + offset);
	val |= INOUTEPMPSR_TX0BYTE_IEP;
	iowrite32(val, fotg210->reg + offset);

	do{
		udelay(50);
		// check whether plugout cable during DMA
		vbus_gpio = gpio_get_value(vbus_gpio_no);
		if (!vbus_gpio) {
			if (debug_on) {
				pr_info("[fifo reset]vbus_gpio is %d\n", vbus_gpio);
			}
		}

		// check whether fifo stuck normally during DMA
		cur_bc = (ioread32(ep->fotg210->reg + FOTG210_FIBCR(gEPMap[ep->epnum -1])) & FIBCR_BCFX);
		pre_dma_remain = ioread32(ep->fotg210->reg + 0x1D4);
		if (debug_on) {
			pr_info("[set_tx0byte] cur_bc is 0x%x, pre_bc is 0x%x\n", cur_bc, pre_bc);
			pr_info("[set_tx0byte(] cur_dma_remain is 0x%x, pre_dma_remain is 0x%x\n", cur_dma_remain, pre_dma_remain);
		}

		if ((cur_bc == pre_bc) && (cur_dma_remain == pre_dma_remain)) {
			count++;
			if (count == BC_STUCK_CHK_CNT) {
				pr_info("count is %d times\n", count);
				cable_out_ret = fotg210_cable_out_clear(ep);
				return;
			}
		} else {
			pre_bc = cur_bc;
			pre_dma_remain = cur_dma_remain;
			count = 0;
		}

		val = ioread32(fotg210->reg + offset);
	}while(val & INOUTEPMPSR_TX0BYTE_IEP);

	return;
}

static void fotg210_clear_tx0byte(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_TX0BYTE);

	value &= (TX0BYTE_EP1 | TX0BYTE_EP2 | TX0BYTE_EP3
		   | TX0BYTE_EP4 | TX0BYTE_EP5 | TX0BYTE_EP6 | TX0BYTE_EP7 | TX0BYTE_EP8);
	iowrite32(value, fotg210->reg + FOTG210_TX0BYTE);
}

static void fotg210_clear_rx0byte(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_RX0BYTE);

	value &= ~(RX0BYTE_EP1 | RX0BYTE_EP2 | RX0BYTE_EP3
		   | RX0BYTE_EP4 | RX0BYTE_EP5 | RX0BYTE_EP6 | RX0BYTE_EP7 | RX0BYTE_EP8);
	iowrite32(value, fotg210->reg + FOTG210_RX0BYTE);
}

/* read 8-byte setup packet only */
static void fotg210_rdsetupp(struct fotg210_udc *fotg210,
		   u8 *buffer)
{
#if 1
	u32 *tmp32;
	tmp32 = (u32 *)buffer;
	*tmp32++ = ioread32(fotg210->reg + FOTG210_CXPORT);
	*tmp32   = ioread32(fotg210->reg + FOTG210_CXPORT);

	if (debug_on)
		ep0numsg("SETUP 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n"
			,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);

#else
	int i = 0;
	u8 *tmp = buffer;
	u32 data;
	u32 length = 8;

	iowrite32(DMATFNR_ACC_CXF, fotg210->reg + FOTG210_DMATFNR);

	for (i = (length >> 2); i > 0; i--) {
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		*(tmp + 3) = (data >> 24) & 0xFF;
		tmp = tmp + 4;
	}

	switch (length % 4) {
	case 1:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		break;
	case 2:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		break;
	case 3:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		break;
	default:
		break;
	}

	iowrite32(DMATFNR_DISDMA, fotg210->reg + FOTG210_DMATFNR);
#endif

}

static void fotg210_set_configuration(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DAR);

	value |= DAR_AFT_CONF;
	iowrite32(value, fotg210->reg + FOTG210_DAR);
}

static void fotg210_set_dev_addr(struct fotg210_udc *fotg210, u32 addr)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DAR);

	value |= (addr & 0x7F);
	iowrite32(value, fotg210->reg + FOTG210_DAR);
}

static void fotg210_set_cxstall(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DCFESR);

	value |= DCFESR_CX_STL;
	iowrite32(value, fotg210->reg + FOTG210_DCFESR);
}

static void fotg210_request_error(struct fotg210_udc *fotg210)
{
	fotg210_set_cxstall(fotg210);
	pr_err("request error!!\n");
}

static void fotg210_set_address(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	if (ctrl->wValue >= 0x0100) {
		fotg210_request_error(fotg210);
	} else {
		fotg210_set_dev_addr(fotg210, ctrl->wValue);
		fotg210_set_cxdone(fotg210);
	}
}

static void gen_test_packet(u8 *tst_packet)
{
	u8 *tp = tst_packet;

	int i;
	for (i = 0; i < 9; i++)/*JKJKJKJK x 9*/
		*tp++ = 0x00;

	for (i = 0; i < 8; i++) /* 8*AA */
		*tp++ = 0xAA;

	for (i = 0; i < 8; i++) /* 8*EE */
		*tp++ = 0xEE;

	*tp++ = 0xFE;

	for (i = 0; i < 11; i++) /* 11*FF */
		*tp++ = 0xFF;

	*tp++ = 0x7F;
	*tp++ = 0xBF;
	*tp++ = 0xDF;
	*tp++ = 0xEF;
	*tp++ = 0xF7;
	*tp++ = 0xFB;
	*tp++ = 0xFD;
	*tp++ = 0xFC;
	*tp++ = 0x7E;
	*tp++ = 0xBF;
	*tp++ = 0xDF;
	*tp++ = 0xEF;
	*tp++ = 0xF7;
	*tp++ = 0xFB;
	*tp++ = 0xFD;
	*tp++ = 0x7E;
}

static void fotg210_set_feature(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u32 val;
	u8 tst_packet[53];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		switch(w_value) {
		case USB_DEVICE_TEST_MODE:
			switch (w_index >> 8) {
			case TEST_J:
				pr_info("\n...TEST_J...\n");
				iowrite32(PHYTMSR_TST_JSTA,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);
				break;
			case TEST_K:
				pr_info("\n...TEST_K...\n");
				iowrite32(PHYTMSR_TST_KSTA,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);
				break;
			case TEST_SE0_NAK:
				pr_info("\n...TEST_SE0_NAK...\n");
				iowrite32(PHYTMSR_TST_SE0NAK,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);
				break;
			case TEST_PACKET:
				pr_info("\n...TEST_PACKET1...\n");
				iowrite32(PHYTMSR_TST_PKT,
					fotg210->reg + FOTG210_PHYTMSR);
				fotg210_set_cxdone(fotg210);

				gen_test_packet(tst_packet);

				fotg210->ep0_req->buf = tst_packet;
				fotg210->ep0_req->length = 53;
				do_test_packet = 1;
				pr_info("\n...TEST_PACKET2...\n");
				//spin_unlock(&fotg210->lock);
				fotg210_ep_queue(fotg210->gadget.ep0,
						fotg210->ep0_req, GFP_KERNEL);
				//spin_lock(&fotg210->lock);

				//Test Packet Done set
				val = ioread32(fotg210->reg + FOTG210_DCFESR);
				val |= DCFESR_TST_PKDONE;
				iowrite32(val, fotg210->reg + FOTG210_DCFESR);
				pr_info("\n...TEST_PACKET3...\n");
				break;
			case TEST_FORCE_EN:
				fotg210_set_cxdone(fotg210);
				break;
			default:
				fotg210_request_error(fotg210);
				break;
			}
			break;

		default:
			fotg210_set_cxdone(fotg210);
			break;
		}
		break;
	case USB_RECIP_INTERFACE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_ENDPOINT: {
		u8 epnum;
		epnum = le16_to_cpu(ctrl->wIndex) & USB_ENDPOINT_NUMBER_MASK;
		if (epnum) {
			fotg210->ep[epnum]->stall = 1;
			fotg210_set_epnstall(fotg210->ep[epnum]);
		}
		else
			fotg210_set_cxstall(fotg210);

		fotg210_set_cxdone(fotg210);
		}
		break;
	default:
		fotg210_request_error(fotg210);
		break;
	}
}

static void fotg210_clear_feature(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	struct fotg210_ep *ep =
		fotg210->ep[ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_INTERFACE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_ENDPOINT:
		if (ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK) {
			if (ep->wedged) {
				fotg210_set_cxdone(fotg210);
				break;
			}
			if (ep->stall)
				fotg210_set_halt_and_wedge(&ep->ep, 0, 0);
		}
		fotg210_set_cxdone(fotg210);
		break;
	default:
		fotg210_request_error(fotg210);
		break;
	}
}

static int fotg210_is_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	return value & INOUTEPMPSR_STL_EP ? 1 : 0;
}

static int fotg210_is_cxstall(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DCFESR);
	return value & DCFESR_CX_STL ? 1 : 0;
}

static void fotg210_get_status(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u8 epnum;

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		fotg210->ep0_data = 1 << USB_DEVICE_SELF_POWERED;
		break;
	case USB_RECIP_INTERFACE:
		fotg210->ep0_data = 0;
		break;
	case USB_RECIP_ENDPOINT:
		epnum = ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK;
		if (epnum)
			fotg210->ep0_data =
				fotg210_is_epnstall(fotg210->ep[epnum])
				<< USB_ENDPOINT_HALT;
		else
			fotg210->ep0_data =
				fotg210_is_cxstall(fotg210)
				<< USB_ENDPOINT_HALT;
		break;

	default:
		fotg210_request_error(fotg210);
		return;		/* exit */
	}

	fotg210->ep0_req->buf = &fotg210->ep0_data;
	fotg210->ep0_req->length = 2;

	spin_unlock(&fotg210->lock);
	fotg210_ep_queue(fotg210->gadget.ep0, fotg210->ep0_req, GFP_ATOMIC);
	spin_lock(&fotg210->lock);
}

static int fotg210_setup_packet(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u8 *p = (u8 *)ctrl;
	u8 ret = 0;

	fotg210_rdsetupp(fotg210, p);

	fotg210->ep[0]->dir_in = ctrl->bRequestType & USB_DIR_IN;

	if (fotg210->gadget.speed == USB_SPEED_UNKNOWN) {
		u32 value = ioread32(fotg210->reg + FOTG210_DMCR);
		fotg210->gadget.speed = value & DMCR_HS_EN ?
				USB_SPEED_HIGH : USB_SPEED_FULL;
	}

	/* check request */
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (ctrl->bRequest) {
		case USB_REQ_GET_STATUS:
			fotg210_get_status(fotg210, ctrl);
			break;
		case USB_REQ_CLEAR_FEATURE:
			fotg210_clear_feature(fotg210, ctrl);
			break;
		case USB_REQ_SET_FEATURE:
			fotg210_set_feature(fotg210, ctrl);
			break;
		case USB_REQ_SET_ADDRESS:
			fotg210_set_address(fotg210, ctrl);
			break;
		case USB_REQ_SET_CONFIGURATION:
			fotg210_set_configuration(fotg210);
			ret = 1;
			break;
		default:
			ret = 1;
			break;
		}
	} else {
		if (debug_on)
			pr_info("none standard request, ctrl->bRequestType is 0x%x, ctrl->bRequest is 0x%x\n",  ctrl->bRequestType, ctrl->bRequest);

		if ((ctrl->bRequestType & 0xFF) == USB_CDC_REQ_GET_LINE_CODING) {
			if (debug_on) {
				pr_info("CDC case\n");
			}
			is_cdc_class = 1;
		}

		ret = 1;
	}

	return ret;
}

static void fotg210_ep0out(struct fotg210_udc *fotg210)
{
	struct fotg210_ep *ep = fotg210->ep[0];

	if (!list_empty(&ep->queue) && !ep->dir_in) {
		struct fotg210_request *req;

		req = list_first_entry(&ep->queue,
			struct fotg210_request, queue);

		if (req->req.length)
			//ivot_usbdev_start_ep0_data(ep, req);
			fotg210_start_dma(ep, req);

		if ((req->req.length - req->req.actual) < ep->ep.maxpacket)
			fotg210_done(ep, req, 0);
	} else {
		pr_err("%s : empty queue\n", __func__);
	}
}

static void fotg210_ep0in(struct fotg210_udc *fotg210)
{
	struct fotg210_ep *ep = fotg210->ep[0];

	if ((!list_empty(&ep->queue)) && (ep->dir_in)) {
		struct fotg210_request *req;

		req = list_entry(ep->queue.next,
				struct fotg210_request, queue);

		if (req->req.length)
			//ivot_usbdev_start_ep0_data(ep, req);
			fotg210_start_dma(ep, req);

		if (req->req.length == req->req.actual)
			fotg210_done(ep, req, 0);
	} else {
		fotg210_set_cxdone(fotg210);
	}
}

static void fotg210_clear_comabt_int(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DISGR0);

	value &= ~DISGR0_CX_COMABT_INT;
	iowrite32(value, fotg210->reg + FOTG210_DISGR0);
}

static void fotg210_in_fifo_handler(struct fotg210_ep *ep)
{
	struct fotg210_request *req = list_entry(ep->queue.next,
					struct fotg210_request, queue);

	if (req->req.length)
	{
		fotg210_start_dma(ep, req);
	}
	else
	{
		fotg210_set_tx0byte(ep);
	}
	if (req->req.length == req->req.actual)
		fotg210_done(ep, req, 0);
}

static void fotg210_out_fifo_handler(struct fotg210_ep *ep)
{
	struct fotg210_request *req = list_entry(ep->queue.next,
						 struct fotg210_request, queue);

	fotg210_start_dma(ep, req);

	/* finish out transfer */
	if (req->req.length == req->req.actual ||
	    (req->req.actual & (ep->ep.maxpacket-1))) {
		fotg210_done(ep, req, 0);
	}
}

static irqreturn_t fotg210_irq(int irq, void *_fotg210)
{
	struct fotg210_udc *fotg210 = _fotg210;
	u32 int_grp = ioread32(fotg210->reg + FOTG210_DIGR);
	u32 int_msk = ioread32(fotg210->reg + FOTG210_DMIGR);
	int EPn;

	int_grp &= ~int_msk;

	spin_lock(&fotg210->lock);

	if (int_grp & DIGR_INT_G2) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR2;
		u32 int_grp2 = ioread32(reg);
		u32 int_msk2 = ioread32(fotg210->reg + FOTG210_DMISGR2);
		u32 value;

		int_grp2 &= ~int_msk2;

		if (int_grp2 & DISGR2_USBRST_INT) {
			iowrite32(DISGR2_USBRST_INT, reg);

			// reset releated settings when bus reset got
			iowrite32(0x00420000, fotg210->reg + FOTG210_DAR);

			iowrite32(0x00000200, fotg210->reg + 0x160);
			iowrite32(0x00000200, fotg210->reg + 0x164);
			iowrite32(0x00000200, fotg210->reg + 0x168);
			iowrite32(0x00000200, fotg210->reg + 0x16C);
			iowrite32(0x00000200, fotg210->reg + 0x170);
			iowrite32(0x00000200, fotg210->reg + 0x174);
			iowrite32(0x00000200, fotg210->reg + 0x178);
			iowrite32(0x00000200, fotg210->reg + 0x17C);
			iowrite32(0x00000200, fotg210->reg + 0x180);
			iowrite32(0x00000200, fotg210->reg + 0x184);
			iowrite32(0x00000200, fotg210->reg + 0x188);
			iowrite32(0x00000200, fotg210->reg + 0x18C);
			iowrite32(0x00000200, fotg210->reg + 0x190);
			iowrite32(0x00000200, fotg210->reg + 0x194);
			iowrite32(0x00000200, fotg210->reg + 0x198);
			iowrite32(0x00000200, fotg210->reg + 0x19C);

			iowrite32(0xFFFFFFFF, fotg210->reg + 0x1A0);
			iowrite32(0xFFFFFFFF, fotg210->reg + 0x1A4);
			iowrite32(0x0F0F0F0F, fotg210->reg + 0x1A8);
			iowrite32(0x00000000, fotg210->reg + 0x1AC);
			iowrite32(0x0F0F0F0F, fotg210->reg + 0x1D8);
			iowrite32(0x00000000, fotg210->reg + 0x1DC);

			if (debug_on)
				devnumsg("fotg210 udc reset\n");
		}
		if (int_grp2 & DISGR2_SUSP_INT) {
			iowrite32(DISGR2_SUSP_INT, reg);
			if (debug_on)
				devnumsg("fotg210 udc suspend\n");
		}
		if (int_grp2 & DISGR2_RESM_INT) {
			iowrite32(DISGR2_RESM_INT, reg);
			if (debug_on)
				devnumsg("fotg210 udc resume\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ERR_INT) {
			iowrite32(DISGR2_ISO_SEQ_ERR_INT, reg);
			if (debug_on)
				devnumsg("fotg210 iso sequence error\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ABORT_INT) {
			iowrite32(DISGR2_ISO_SEQ_ABORT_INT, reg);
			if (debug_on)
				devnumsg("fotg210 iso sequence abort\n");
		}
		if (int_grp2 & DISGR2_TX0BYTE_INT) {
			iowrite32(DISGR2_TX0BYTE_INT, reg);
			fotg210_clear_tx0byte(fotg210);

			if (debug_on)
				numsg("fotg210 transferred 0 byte\n");
		}
		if (int_grp2 & DISGR2_RX0BYTE_INT) {
			iowrite32(DISGR2_RX0BYTE_INT, reg);
			fotg210_clear_rx0byte(fotg210);
			if (debug_on)
				numsg("fotg210 received 0 byte\n");
		}
		if (int_grp2 & DISGR2_DMA_ERROR) {
			value = ioread32(reg);
			value &= ~DISGR2_DMA_ERROR;
			iowrite32(value, reg);
			if (debug_on)
				numsg("fotg210 DISGR2_DMA_ERROR\n");
		}

		for (EPn = 0; EPn < (FOTG210_MAX_NUM_EP -1); EPn++) {
			if (gEPMap[EPn] == USB_FIFO_NOT_USE || gEPMap[EPn] < 8 )
				continue;

			if (((int_grp2&0xFFFF0000) & DISGR2_OUT_INT(gEPMap[EPn])) ||
					((int_grp2&0xFFFF0000) & DISGR2_SPK_INT(gEPMap[EPn])))
				fotg210_out_fifo_handler(fotg210->ep[EPn+1]);
		}
	}

	if (int_grp & DIGR_INT_G0) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR0;
		u32 int_grp0 = ioread32(reg);
		u32 int_msk0 = ioread32(fotg210->reg + FOTG210_DMISGR0);
		struct usb_ctrlrequest ctrl;

		int_grp0 &= ~int_msk0;
		if (debug_on)
			numsg("G0 0x%08X\n",int_grp0);

		/* the highest priority in this source register */
		if (int_grp0 & DISGR0_CX_COMABT_INT) {
			fotg210_clear_comabt_int(fotg210);
			pr_info("fotg210 CX command abort\n");
		}

		if (int_grp0 & DISGR0_CX_SETUP_INT) {
			if (fotg210_setup_packet(fotg210, &ctrl)) {
				spin_unlock(&fotg210->lock);
				if (fotg210->driver->setup(&fotg210->gadget,
							   &ctrl) < 0) {
					if (ctrl.bRequestType & USB_DIR_IN) {
						fotg210_set_cxstall(fotg210);
					} else {
						fotg210_set_cxstall(fotg210);
						fotg210_set_cxdone(fotg210);
					}
				}
				spin_lock(&fotg210->lock);
			}
		}
		if (int_grp0 & DISGR0_CX_COMEND_INT)
			pr_info("fotg210 cmd end\n");

		if (int_grp0 & DISGR0_CX_IN_INT)
			fotg210_ep0in(fotg210);

		if (int_grp0 & DISGR0_CX_OUT_INT)
			fotg210_ep0out(fotg210);

		if (int_grp0 & DISGR0_CX_COMFAIL_INT) {
			fotg210_set_cxstall(fotg210);
			pr_info("fotg210 ep0 fail\n");
		}
	}

	if (int_grp & DIGR_INT_G1) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR1;
		u32 int_grp1 = ioread32(reg);
		u32 int_msk1 = ioread32(fotg210->reg + FOTG210_DMISGR1);

		if (debug_on)
			numsg("G1 0x%08X\n",int_grp1);

		int_grp1 &= ~int_msk1;

		for (EPn = 0; EPn < (FOTG210_MAX_NUM_EP -1); EPn++) {
			if (gEPMap[EPn] == USB_FIFO_NOT_USE )
				continue;

			if (int_grp1 & DISGR1_IN_INT(gEPMap[EPn]))
				fotg210_in_fifo_handler(fotg210->ep[EPn+1]);

			if(gEPMap[EPn] < 8)
			{
				if ((int_grp1 & DISGR1_OUT_INT(gEPMap[EPn])) ||
					(int_grp1 & DISGR1_SPK_INT(gEPMap[EPn])))
					fotg210_out_fifo_handler(fotg210->ep[EPn+1]);
			}
		}
	}

	spin_unlock(&fotg210->lock);

	return IRQ_HANDLED;
}

#if USE_TASKLET_FOR_IRQ
static irqreturn_t fotg210_isr(int irq, void *_fotg210)
{
	u32 value;
	struct fotg210_udc *fotg210 = _fotg210;

	spin_lock(&fotg210->lock);

	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value &= ~DMCR_GLINT_EN;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	spin_unlock(&fotg210->lock);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t fotg210_ist(int irq, void *_fotg210)
{
	u32 value;
	struct fotg210_udc *fotg210 = (struct fotg210_udc *)_fotg210;

	fotg210_irq(0,(void *)_fotg210);

	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value |= DMCR_GLINT_EN;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	return IRQ_HANDLED;
}
#endif

static void fotg210_disable_unplug(struct fotg210_udc *fotg210)
{
	u32 reg = ioread32(fotg210->reg + FOTG210_PHYTMSR);

	reg &= ~PHYTMSR_UNPLUG;
	iowrite32(reg, fotg210->reg + FOTG210_PHYTMSR);
}

static int fotg210_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct fotg210_udc *fotg210 = gadget_to_fotg210(g);
	u32 value;

	/* hook up the driver */
	driver->driver.bus = NULL;
	fotg210->driver = driver;

	fotg210_disable_unplug(fotg210);

	/* enable device global interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value |= DMCR_GLINT_EN;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	return 0;
}

static void fotg210_init(struct fotg210_udc *fotg210)
{
	u32 value;

	iowrite32((ioread32(fotg210->reg+0x400))|(0x3<<20), fotg210->reg + 0x400);

	/* make device exit suspend */
	value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	value &= ~DMACPSR1_DEVSUSPEND;
	iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);

	udelay(200);

	#if 1
	/* Configure PHY related settings below */
	{
		u16 data=0;
		s32 result=0;
		u32 temp;
		u8 u2_trim_swctrl=4, u2_trim_sqsel=4, u2_trim_resint=8;

		result= efuse_readParamOps(EFUSE_USBC_TRIM_DATA, &data);
		if(result == 0) {
			u2_trim_swctrl = data&0x7;
			u2_trim_sqsel  = (data>>3)&0x7;
			u2_trim_resint = (data>>6)&0x1F;
		}

		iowrite32(0x20, (void __iomem *)(0xFD601000+(0x51<<2)));
		iowrite32(0x30, (void __iomem *)(0xFD601000+(0x50<<2)));

		temp = ioread32((void __iomem *)(0xFD601000+(0x06<<2)));
		temp &= ~(0x7<<1);
		temp |= (u2_trim_swctrl<<1);
		iowrite32(temp,(void __iomem *)(0xFD601000+(0x06<<2)));

		temp = ioread32((void __iomem *)(0xFD601000+(0x05<<2)));
		temp &= ~(0x7<<2);
		temp |= (u2_trim_sqsel<<2);
		iowrite32(temp,(void __iomem *)(0xFD601000+(0x05<<2)));

		iowrite32(0x60+u2_trim_resint, (void __iomem *)(0xFD601000+(0x52<<2)));
		iowrite32(0x00, (void __iomem *)(0xFD601000+(0x51<<2)));

		iowrite32(0x100+u2_trim_resint, fotg210->reg + 0x30C);
	}
	#endif


	/* VBUS debounce time */
	iowrite32(ioread32(fotg210->reg + FOTG210_OTGCTRLSTS)|0x400, fotg210->reg + FOTG210_OTGCTRLSTS);


////////////////////////////////////////////////////////////////////////////////

	/* disable global interrupt and set int polarity to active high */
	iowrite32(GMIR_MHC_INT | GMIR_MOTG_INT | GMIR_INT_POLARITY,
		  fotg210->reg + FOTG210_GMIR);

	/* disable device global interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value &= ~DMCR_GLINT_EN;
	#ifdef CONFIG_NVT_FPGA_EMULATION
	value |= DMCR_HALF_SPEED;
	#endif

	if(force_fs)
		value |= DMCR_FORCE_FS;

	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	/* disable all fifo interrupt */
	iowrite32(~(u32)0, fotg210->reg + FOTG210_DMISGR1);

	/* disable cmd end */
	value = ioread32(fotg210->reg + FOTG210_DMISGR0);
	value |= DMISGR0_MCX_COMEND;
	iowrite32(value, fotg210->reg + FOTG210_DMISGR0);
}

static int fotg210_udc_stop(struct usb_gadget *g)
{
	struct fotg210_udc *fotg210 = gadget_to_fotg210(g);
	unsigned long	flags;

	spin_lock_irqsave(&fotg210->lock, flags);

	fotg210_init(fotg210);
	fotg210->driver = NULL;

	spin_unlock_irqrestore(&fotg210->lock, flags);

	return 0;
}

static const struct usb_gadget_ops fotg210_gadget_ops = {
	.udc_start		= fotg210_udc_start,
	.udc_stop		= fotg210_udc_stop,
};

static int fotg210_udc_remove(struct platform_device *pdev)
{
	struct fotg210_udc *fotg210 = platform_get_drvdata(pdev);
	int i;

	usb_del_gadget_udc(&fotg210->gadget);
	iounmap(fotg210->reg);
	free_irq(platform_get_irq(pdev, 0), fotg210);

	fotg210_ep_free_request(&fotg210->ep[0]->ep, fotg210->ep0_req);
	for (i = 0; i < FOTG210_MAX_NUM_EP; i++)
		kfree(fotg210->ep[i]);

	kfree(fotg210);

#if IS_ENABLED(CONFIG_NVT_IVOT_PLAT_NA51089)
	nvt_enable_sram_shutdown(USB_SD);
#endif
	{
		struct clk *source_clk;

		source_clk = clk_get(NULL, "f0600000.usb20");
		if (IS_ERR(source_clk)) {
			printk("faile to get clock f0600000.usb20\n");
		} else {
			clk_unprepare(source_clk);
			clk_put(source_clk);
		}
	}

	return 0;
}

static int fotg210_udc_probe(struct platform_device *pdev)
{
	struct resource *res, *ires;
	struct fotg210_udc *fotg210 = NULL;
	struct fotg210_ep *_ep[FOTG210_MAX_NUM_EP];
	int ret = 0;
	int i;
	UINT32 EPi, FIFOi;
	int val, val2;
	UINT32 epnum = FOTG210_DEFAULT_NUM_EP;

	if (debug_on)
		numsg("fotg210_udc_probe\r\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("platform_get_resource error.\n");
		return -ENODEV;
	}

	ires = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!ires) {
		pr_err("platform_get_resource IORESOURCE_IRQ error.\n");
		return -ENODEV;
	}
	of_property_read_u32(pdev->dev.of_node, "epnum", &epnum);

	ret = -ENOMEM;

	/* initialize udc */
	fotg210 = kzalloc(sizeof(struct fotg210_udc), GFP_KERNEL);
	if (fotg210 == NULL)
		goto err;

	for (i = 0; i < FOTG210_MAX_NUM_EP; i++) {
		_ep[i] = kzalloc(sizeof(struct fotg210_ep), GFP_KERNEL);
		if (_ep[i] == NULL)
			goto err_alloc;
		fotg210->ep[i] = _ep[i];
	}

	fotg210->reg = ioremap(res->start|0x0F000000, resource_size(res));
	if (fotg210->reg == NULL) {
		pr_err("ioremap error.\n");
		goto err_alloc;
	}

	{
		struct clk *source_clk;

		source_clk = clk_get(NULL, "f0600000.usb20");
		if (IS_ERR(source_clk)) {
			printk("faile to get clock f0600000.usb20\n");
		} else {
			clk_prepare(source_clk);
			clk_put(source_clk);
		}
	}

#if IS_ENABLED(CONFIG_NVT_IVOT_PLAT_NA51089)
	nvt_disable_sram_shutdown(USB_SD);
#endif
	spin_lock_init(&fotg210->lock);

	platform_set_drvdata(pdev, fotg210);

	fotg210->gadget.ops = &fotg210_gadget_ops;

	fotg210->gadget.max_speed = USB_SPEED_HIGH;
	fotg210->gadget.dev.parent = &pdev->dev;
	fotg210->gadget.dev.dma_mask = pdev->dev.dma_mask;
	fotg210->gadget.name = udc_name;

	INIT_LIST_HEAD(&fotg210->gadget.ep_list);

	for (i = 0; i < FOTG210_MAX_NUM_EP; i++) {
		struct fotg210_ep *ep = fotg210->ep[i];

		if (i) {
			INIT_LIST_HEAD(&fotg210->ep[i]->ep.ep_list);
			list_add_tail(&fotg210->ep[i]->ep.ep_list,
				      &fotg210->gadget.ep_list);
		}
		ep->fotg210 = fotg210;
		INIT_LIST_HEAD(&ep->queue);
		//ep->ep.name = fotg210_ep_name[i];
		ep->ep.name = ep_info[i].name;
		ep->ep.caps = ep_info[i].caps;
		ep->ep.ops = &fotg210_ep_ops;
		usb_ep_set_maxpacket_limit(&ep->ep, (unsigned short) ~0);

		if (i == 0) {
			ep->ep.caps.type_control = true;
		} else {
			ep->ep.caps.type_iso = true;
			ep->ep.caps.type_bulk = true;
			ep->ep.caps.type_int = true;
		}

		ep->ep.caps.dir_in = true;
		ep->ep.caps.dir_out = true;
	}
	usb_ep_set_maxpacket_limit(&fotg210->ep[0]->ep, 0x40);
	fotg210->gadget.ep0 = &fotg210->ep[0]->ep;
	INIT_LIST_HEAD(&fotg210->gadget.ep0->ep_list);

	fotg210->ep0_req = fotg210_ep_alloc_request(&fotg210->ep[0]->ep,
				GFP_KERNEL);
	if (fotg210->ep0_req == NULL)
		goto err_map;

	fotg210_init(fotg210);

	//fotg210_disable_unplug(fotg210);


#if USE_TASKLET_FOR_IRQ
	/* Using tasklet for command processing */
	ret =  request_threaded_irq(ires->start, fotg210_isr, fotg210_ist,
			IRQF_SHARED, udc_name, fotg210);
#else
	ret = request_irq(ires->start, fotg210_irq, IRQF_SHARED,
			  udc_name, fotg210);
#endif
	if (ret < 0) {
		pr_err("request_irq error (%d)\n", ret);
		goto err_req;
	}

	ret = usb_add_gadget_udc(&pdev->dev, &fotg210->gadget);
	if (ret)
		goto err_add_udc;

	dev_info(&pdev->dev, "version %s\n", DRIVER_VERSION);

	//clear 0x1A8, 0x1AC
	val |= ioread32(fotg210->reg + 0x1A8);
	val2 |= ioread32(fotg210->reg + 0x1AC);

	val &= ~CLEARMASK;
	val2 &= ~CLEARMASK;

	iowrite32(val, (fotg210->reg+0x1A8));
	iowrite32(val2, (fotg210->reg+0x1AC));

	// clear all EP & FIFO setting of EP-FIFO mapping table. -1 because excluding the EP0.
	for (EPi = 0 ; EPi < (FOTG210_MAX_NUM_EP - 1) ; EPi++) {
		gEPMap[EPi] = USB_FIFO_NOT_USE;
	}

	for (FIFOi = 0 ; FIFOi < FOTG210_MAX_FIFO_NUM ; FIFOi++) {
		gFIFOInMap[FIFOi]  = USB_EP_NOT_USE;
		gFIFOOutMap[FIFOi] = USB_EP_NOT_USE;
	}

	if(epnum > FOTG210_DEFAULT_NUM_EP)
	{
		dev_info(&pdev->dev, "more than 8 EPs!, switch to 1 EP 1 Fifo\n");
		for (EPi = 0 ; EPi < (FOTG210_MAX_NUM_EP - 1) ; EPi++) {
			gEPBlkNo[EPi] = BLKNUM_SINGLE;
		}
	}

	return 0;

err_add_udc:
	free_irq(ires->start, fotg210);

err_req:
	fotg210_ep_free_request(&fotg210->ep[0]->ep, fotg210->ep0_req);

err_map:
	iounmap(fotg210->reg);

err_alloc:
	for (i = 0; i < FOTG210_MAX_NUM_EP; i++)
		kfree(fotg210->ep[i]);
	kfree(fotg210);

err:
#if IS_ENABLED(CONFIG_NVT_IVOT_PLAT_NA51089)
	nvt_enable_sram_shutdown(USB_SD);
#endif
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id of_fotg210_match[] = {
	{
		.compatible = "nvt,fotg200_udc"
	},

	{ },
};
MODULE_DEVICE_TABLE(of, of_fotg210_match);
#endif

static struct platform_driver fotg210_driver = {
	.driver		= {
		.name =	(char *)udc_name,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(of_fotg210_match),
#endif
	},
	.probe		= fotg210_udc_probe,
	.remove		= fotg210_udc_remove,
};

module_platform_driver(fotg210_driver);

MODULE_AUTHOR("Klins Chen, klins_chen <klins_chen@novatek.com.tw>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
