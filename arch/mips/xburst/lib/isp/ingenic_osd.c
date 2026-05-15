#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/err.h>
//#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include<linux/string.h>

#include "include/osd/imp_osd.h"
#include "include/osd/ingenic_ipu_hal.h"
#include "include/osd/logodata_100x100_bgra.h"
#include "include/user/osd.h"

static IMPOsd g_osd_group;
extern IMP_Sample_OsdParam ipuosdparam;
//extern uint8_t logodata_100x100_bgra[40000];
#if 0
#define SECONDS_IN_MINUTE 60
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_DAY 86400
#define DAYS_IN_COMMON_YEAR 365
#define DAYS_IN_LEAP_YEAR 366
#define MONTHS_IN_YEAR 12
#endif

int osd_thread_create(IMP_Sample_OsdParam *ipuosdparam)
{
	int err;
	struct task_struct *osd_task;
	ipuosdparam->osd_task = kthread_run(update_thread, ipuosdparam, "ipu_osd_update");
	osd_task = ipuosdparam->osd_task;
	if(IS_ERR(osd_task)){
		printk("Unable to start kernel thread. ");
		err = PTR_ERR(osd_task);
		osd_task = NULL;
		return err;
	}

	return 0;
}

static unsigned int application_to_hal_fmt(IMPPixelFormat app_fmt)
{
	unsigned int hal_fmt = HAL_PIXEL_FORMAT_BGRA_8888;
	switch (app_fmt) {
		case PIX_FMT_BGRA:
			hal_fmt = HAL_PIXEL_FORMAT_BGRA_8888;
			break;
		case PIX_FMT_RGBA:
			hal_fmt = HAL_PIXEL_FORMAT_RGBA_8888;
			break;
		case PIX_FMT_ABGR:
			hal_fmt = HAL_PIXEL_FORMAT_ABGR_8888;
			break;
		case PIX_FMT_ARGB:
			hal_fmt = HAL_PIXEL_FORMAT_ARGB_8888;
			break;
		case PIX_FMT_BGR555LE:
			hal_fmt = HAL_PIXEL_FORMAT_BGRA_5551;
			break;
		case PIX_FMT_RGB555LE:
			hal_fmt = HAL_PIXEL_FORMAT_RGBA_5551;
			break;
		case PIX_FMT_NV12:
			hal_fmt = HAL_PIXEL_FORMAT_NV12;
			break;
		case PIX_FMT_NV21:
			hal_fmt = HAL_PIXEL_FORMAT_NV21;
			break;
		case PIX_FMT_YUV420P:
			hal_fmt = HAL_PIXEL_FORMAT_YCbCr_420_P;
			break;
		case PIX_FMT_2BIT:
			hal_fmt = HAL_PIXEL_FORMAT_2BIT;
			break;
		default:
			hal_fmt = HAL_PIXEL_FORMAT_BGRA_8888;
			break;
	}
	return hal_fmt;
}


int ipu_osd_set(int grpNum, unsigned int ipu_bg_paddr,unsigned int bg_width, unsigned int bd_height)
{
	int ret = 0;
	struct ipu_osd_param osdp;
	IMPRgnHandle rHanderLogo = 0;
	IMPRgnHandle rHanderFont = 1;
	IMPOSDRgnAttr *rAttrFont;
	IMPOSDRgnAttr *rAttrLogo;
	int picw,pich;

	memset(&osdp, 0, sizeof(osdp));
	//背景准备
	osdp.bg_w = bg_width;
	osdp.bg_h = bd_height;
	osdp.bg_fmt =  HAL_PIXEL_FORMAT_NV12;
	osdp.bg_buf = ipu_bg_paddr;
	osdp.osd_mask_para = 0xffff8080;
	/* Font */
	rAttrFont = &g_osd_group.group[grpNum].rAttr[rHanderFont];
	osdp.ch[0].osd_chx_fmt = application_to_hal_fmt(rAttrFont->fmt);
	osdp.osd_flags |= IPU_OSD_FLAGS_OSD0;
	osdp.ch[0].osd_chx_alpha = 0xff;

	osdp.ch[0].osd_chx_pos_x = rAttrFont->rect.p0.x;
	osdp.ch[0].osd_chx_pos_y = rAttrFont->rect.p0.y;
	osdp.ch[0].osd_chx_src_w = rAttrFont->rect.p1.x - rAttrFont->rect.p0.x + 1;
	osdp.ch[0].osd_chx_src_h = rAttrFont->rect.p1.y - rAttrFont->rect.p0.y + 1;
	osdp.ch[0].osd_chx_buf = (unsigned int)rAttrFont->data.picData.pData;
	if (HAL_PIXEL_FORMAT_2BIT == osdp.ch[0].osd_chx_fmt) {
		osdp.ch[0].osd_chx_2bit_mask = 0x0;
		osdp.ch[0].osd_chx_2bit_bit0_fmt = 0x7F00FF00;
		osdp.ch[0].osd_chx_2bit_bit1_fmt = 0x7FFF0000;
	}

	/* Logo */
	rAttrLogo = &g_osd_group.group[grpNum].rAttr[rHanderLogo];
	osdp.ch[1].osd_chx_fmt = application_to_hal_fmt(rAttrFont->fmt);
	osdp.osd_flags |= IPU_OSD_FLAGS_OSD1;
	osdp.ch[1].osd_chx_alpha = 0xff;

	osdp.ch[1].osd_chx_pos_x = rAttrLogo->rect.p0.x;
	osdp.ch[1].osd_chx_pos_y = rAttrLogo->rect.p0.y;

	osdp.ch[1].osd_chx_src_w = rAttrLogo->rect.p1.x - rAttrLogo->rect.p0.x + 1;
	osdp.ch[1].osd_chx_src_h = rAttrLogo->rect.p1.y - rAttrLogo->rect.p0.y + 1;
	picw = osdp.ch[1].osd_chx_src_w - osdp.ch[1].osd_chx_pos_x + 1;
	pich = osdp.ch[1].osd_chx_src_h - osdp.ch[1].osd_chx_pos_y + 1;
	osdp.ch[1].osd_chx_buf = (unsigned int)rAttrLogo->data.picData.pData;
	if (HAL_PIXEL_FORMAT_2BIT == osdp.ch[1].osd_chx_fmt) {
		osdp.ch[1].osd_chx_2bit_mask = 0x0;
		osdp.ch[1].osd_chx_2bit_bit0_fmt = 0x7F00FF00;
		osdp.ch[1].osd_chx_2bit_bit1_fmt = 0x7FFF0000;
	}

	ret = ipu_osd(&osdp);	//osd进行叠加
	if (ret != 0) {
		printk("ipu_osd error ret = %d\n", ret);
		return -1;
	}
	memset(&osdp, 0, sizeof(struct ipu_osd_param));

	return 0;
}
int *ipu_osd_create_rgn(int grpNum, uint32_t *timeStampData_ipu)
{
	IMPRgnHandle *prHander = NULL;
	IMPRgnHandle rHanderLogo = 0;
	IMPRgnHandle rHanderFont = 1;
	IMPOSDRgnAttr rAttrFont;
	IMPOSDRgnAttr rAttrLogo;
	int picw = 100;
	int pich = 100;

	prHander = kmalloc(2 * sizeof(IMPRgnHandle),GFP_KERNEL);

    /* LOGO */
	memset(&rAttrLogo, 0, sizeof(IMPOSDRgnAttr));
	rAttrLogo.type = OSD_REG_PIC;
	rAttrLogo.rect.p0.x = 50;
	rAttrLogo.rect.p0.y = 50;

	/* p0 is start，and p1 well be epual p0+width(or heigth)-1 */
	rAttrLogo.rect.p1.x = rAttrLogo.rect.p0.x+picw-1;
	rAttrLogo.rect.p1.y = rAttrLogo.rect.p0.y+pich-1;
	rAttrLogo.fmt = PIX_FMT_BGRA;
	rAttrLogo.data.picData.pData = logodata_100x100_bgra;
//    printk("===%s %d LOGO:0x%x\n",__func__,__LINE__,*(unsigned char *)rAttrLogo.data.picData.pData);

    // TODO
#if 0
	IMPOSDGrpRgnAttr grAttrLogo;
	memset(&grAttrLogo, 0, sizeof(IMPOSDGrpRgnAttr));
	grAttrLogo.show = 1;
	/* Set Logo global alpha to 0x7f, it is semi-transparent. */
	grAttrLogo.gAlphaEn = 0;
	grAttrLogo.fgAlhpa = 0x7f;
	grAttrLogo.layer = 2;
#endif
	prHander[0] = rHanderLogo;
	memcpy(&g_osd_group.group[grpNum].rAttr[rHanderLogo],&rAttrLogo,sizeof(IMPOSDRgnAttr));

	/* Font */;
	memset(&rAttrFont, 0, sizeof(IMPOSDRgnAttr));
	rAttrFont.type = OSD_REG_PIC;
//    rAttrFont.type = OSD_REG_BITMAP;
	rAttrFont.rect.p0.x = 300;
	rAttrFont.rect.p0.y = 50;
	rAttrFont.rect.p1.x = rAttrFont.rect.p0.x + OSD_LETTER_NUM * OSD_REGION_WIDTH - 1;	/* p0 is start，and p1 well be epual p0+width(or heigth)-1 */
	rAttrFont.rect.p1.y = rAttrFont.rect.p0.y + OSD_REGION_HEIGHT - 1;
#ifdef SUPPORT_RGB555LE
	rAttrFont.fmt = PIX_FMT_RGB555LE;
#else
	rAttrFont.fmt = PIX_FMT_BGRA;
#endif
	rAttrFont.data.picData.pData = timeStampData_ipu;

    //TODO
#if 0
	IMPOSDGrpRgnAttr grAttrFont;
	memset(&grAttrFont, 0, sizeof(IMPOSDGrpRgnAttr));
	grAttrFont.show = 1;
	grAttrFont.gAlphaEn = 0;
	grAttrFont.fgAlhpa = 0x7f;
	grAttrFont.layer = 2;
#endif
	prHander[1] = rHanderFont;
	memcpy(&g_osd_group.group[grpNum].rAttr[rHanderFont],&rAttrFont,sizeof(IMPOSDRgnAttr));

	return prHander;
}
