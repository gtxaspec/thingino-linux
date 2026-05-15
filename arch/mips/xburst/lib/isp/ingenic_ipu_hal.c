//#include <stdio.h>
//#include <stdlib.h>
#include <linux/string.h>

//#include <unistd.h>
//#include <fcntl.h>
//#include <sys/mman.h>
//#include <fb.h>
//#include <sys/ioctl.h>
#include "include/osd/ingenic_ipu_hal.h"

#define TAG "OSD"
#define IS_SOC_SERIES(ID) ((SOC_##ID##_START <= get_cpu_id()) && (get_cpu_id() <= SOC_##ID##_END))

#//define DEBUG
#ifdef  DEBUG
#define DBG(format, ...) {printk(format, ## __VA_ARGS__);}
#else
#define DBG(format, ...) do{ } while(0)
#endif
static int ipu_inited = 0;
//static int ipu_ipufd = 0;
//static int ipu_memfd = 0;
static struct ipu_buf_info ipu_pbuf;
static void * ipu_vbuf = 0;
extern int osd_mode;

int ipu_osd_init( unsigned int addr)
{
	int ret = 0;
	if (1 == ipu_inited) {
		ret = 0;
		goto warn_ipu_has_inited;
	}
    ret = ipu_open(NULL, NULL);
    if(ret < 0)
		goto err_ipu_open;

	ipu_pbuf.size = OSD_MMAP_SIZES;
	ipu_pbuf.paddr = addr;
	ipu_pbuf.paddr_align = addr;

	ipu_vbuf = ioremap(ipu_pbuf.paddr,ipu_pbuf.size);

	printk(" osd ipubuf paddr = 0x%08x\n", ipu_pbuf.paddr);
	printk(" osd ipubuf vbuf = 0x%08x\n", (unsigned int)ipu_vbuf);
	printk(" osd ipubuf size = %d\n", ipu_pbuf.size);

	ipu_inited = 1;
	return 0;
err_ipu_open:
warn_ipu_has_inited:
	return ret;
}

int ipu_deinit(void)
{
	int ret = 0;
	if (0 == ipu_inited) {

		printk("ipu: warning ipu has deinited\n");
		ret = 0;
		goto warn_ipu_has_deinited;
	}
	printk("%s\n",__func__);
	iounmap(ipu_vbuf);
	ipu_inited = 0;
	return 0;

warn_ipu_has_deinited:
	return ret;
}

int ipu_buf_lock(void)
{
	/* do not lock ipu buf, make sure only one process  */
	return 0;
}

int ipu_buf_unlock(void)
{
	return 0;
}

static unsigned int _ipu_get_align_addr(unsigned int *start, unsigned int size, unsigned int align)
{
	unsigned int start_align = (*start + (align - 1))&(~(align - 1));
	*start = start_align + size;
	return start_align;
}

#define IPU_OSD_CHX_PARA_OSDM_SET(p, x) ((p)=(((p)&(~(0xf<<19)))|((x&0xf)<<19)))
#define IPU_OSD_CHX_PARA_ALPHA_SET(p, x) ((p)=(((p)&(~(0xff<<3)))|((x&0xff)<<3)))
#define IPU_OSD_CHX_PARA_CSCCTL_SET(p, x) ((p)=(((p)&(~(0x3<<24)))|((x&0x3)<<24)))
#define IPU_OSD_CHX_PARA_PICTYPE_SET(p, x) ((p)=(((p)&(~(0x7<<11)))|((x&0x7)<<11)))
#define IPU_OSD_CHX_PARA_MASK_SET(p, x) ((p)=(((p)&(~(0x1<<23)))|((x&0x1)<<23)))
#define IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, x) ((p)=(((p)&(~(0xf<<14)))|((x&0xf)<<14)))

static unsigned int _bgfmt_is_YUV(int bg_fmt)
{
	unsigned int is_YUV = 0;
	switch (bg_fmt) {
		case HAL_PIXEL_FORMAT_ARGB_8888:
		case HAL_PIXEL_FORMAT_ARBG_8888:
		case HAL_PIXEL_FORMAT_AGRB_8888:
		case HAL_PIXEL_FORMAT_AGBR_8888:
		case HAL_PIXEL_FORMAT_ABRG_8888:
		case HAL_PIXEL_FORMAT_ABGR_8888:
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RBGA_8888:
		case HAL_PIXEL_FORMAT_GRBA_8888:
		case HAL_PIXEL_FORMAT_GBRA_8888:
		case HAL_PIXEL_FORMAT_BRGA_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
		case HAL_PIXEL_FORMAT_2BIT:
			is_YUV = 0;
			break;
		case HAL_PIXEL_FORMAT_NV21:
		case HAL_PIXEL_FORMAT_NV12:
			is_YUV = 1;
			break;
	default:
		is_YUV = 0;
		break;
	}

	return is_YUV;
}

//The interface _ipu_set_osdx_para was used for the background format must be YUV,
//if was RGB should be change CSCCTL_SET
static unsigned int _ipu_set_osdx_para(unsigned int *para, unsigned int fmt, unsigned int alpha, unsigned int bg_fmt)
{
	int ret = 0;
	unsigned int is_yuv = 0;
	unsigned int p = 0x020347f9;
	if (fmt == HAL_PIXEL_FORMAT_NV12 || fmt == HAL_PIXEL_FORMAT_NV21)
		p = 0x020347fb;         //use global alpha
	else {
		p = 0x020347f9;         //use pixel alpha
		/*p = 0x020347f9;         //use pixel alpha*/
		/*
		alpha = alpha & 0xff;
		p = 0x020347fd;
		*/                      //use pixsel alpha * (global alpha/256)
	}

	is_yuv = _bgfmt_is_YUV(bg_fmt);
	switch(fmt) {
		case HAL_PIXEL_FORMAT_RGBA_8888:
			if(is_yuv)
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
			else
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
			IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
			IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 8);
			break;
		case HAL_PIXEL_FORMAT_BGRA_8888:
			if(is_yuv)
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
			else
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
			IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
			IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0xd);
			break;
		case HAL_PIXEL_FORMAT_ARGB_8888:
			if(is_yuv)
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
			else
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
			IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
			IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0);
			break;
		case HAL_PIXEL_FORMAT_ABGR_8888:
			if(is_yuv)
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
			else
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
			IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
			IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 5);
			break;
		case HAL_PIXEL_FORMAT_NV12:
			if(is_yuv)
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
			else
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 1);
			IPU_OSD_CHX_PARA_PICTYPE_SET(p, 2);
			break;
		case HAL_PIXEL_FORMAT_NV21:
			if(is_yuv)
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
			else
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 1);
			IPU_OSD_CHX_PARA_PICTYPE_SET(p, 3);
			break;
		case HAL_PIXEL_FORMAT_2BIT:
			if(is_yuv)
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
			else
				IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
			IPU_OSD_CHX_PARA_PICTYPE_SET(p, 5);
			IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0);
			break;
		default:
			printk("ipu: err osd fmt not support \n");
			return -1;
			break;
	}
	IPU_OSD_CHX_PARA_OSDM_SET(p,0);//argv[14] set,default 0
	IPU_OSD_CHX_PARA_ALPHA_SET(p, alpha);
    //p |= 0x300000;   // set osd mode (OSD_CHX_PARA)
	*para = p;
	return ret;
}

static unsigned int _ipu_set_osdx_mask(unsigned int *para, unsigned int alpha)
{
	int ret = 0;
	unsigned int p = 0x020347f9;
	p = 0x028347fb;
//	IPU_OSD_CHX_PARA_OSDM_SET(p,4);
	IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
	IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
	IPU_OSD_CHX_PARA_ALPHA_SET(p, alpha);
	IPU_OSD_CHX_PARA_MASK_SET(p, 1);
	IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0);
    //p |= 0x300000;   // set osd mode (OSD_CHX_PARA)
	*para = p;
	return ret;
}

int chx_share_osd_mem(struct ipu_osdx_para *firstch, struct ipu_osdx_para *secondch)
{
	int ret = 1;
	if (firstch->osd_chx_fmt != secondch->osd_chx_fmt)	ret = 0;
	if (firstch->osd_chx_src_w != secondch->osd_chx_src_w)	ret = 0;
	if (firstch->osd_chx_src_h != secondch->osd_chx_src_h)	ret = 0;
	if (firstch->osd_chx_buf != secondch->osd_chx_buf)	ret = 0;
	return ret;
}

int ipu_osd(struct ipu_osd_param *ipu_osd_param)
{
	int ret = 0;
	unsigned int ipubuf_v = 0;
	unsigned int ipubuf_p = 0;
	unsigned int ipubuf_size = 0;
	unsigned int flags = 0;
	struct ipu_buf_info pbuf;
	struct ipu_param ip;
	struct ipu_osd_param *osdp = ipu_osd_param;
	unsigned int start_paddr = 0;
	unsigned int start_offset = 0;
	unsigned int offset = 0;
	unsigned int start_vaddr = 0;
	struct ipu_flush_cache_para fcache_para;
/*
	if ((ret = ipu_init()) != 0) {
		printk("ipu: error ipu_init ret = %d\n", ret);
		goto err_ipu_init;
	}
*/
	DBG(" osd_test pbuf.vaddr_alloc = 0x%08x\npbuf.paddr_align = 0x%08x\npbuf.paddr = 0x%08x\npbuf.size = 0x%x\n"
			, ipu_pbuf.vaddr_alloc, ipu_pbuf.paddr_align, ipu_pbuf.paddr, ipu_pbuf.size);

	pbuf = ipu_pbuf;
	ipubuf_size = pbuf.size;
	ipubuf_p = pbuf.paddr_align;
	ipubuf_v = (unsigned int)ipu_vbuf;

	//	printk("---------osd_test1--------\n");
	DBG(" osd_test  ipubuf_p = 0x%08x\n", ipubuf_p);
	DBG(" osd_test  ipubuf_v = 0x%08x\n", ipubuf_v);
	DBG(" osd_test  ipubuf_size = 0x%08x\n", ipubuf_size);
	//	printk("---------osd_test2--------\n");

	/* avoid two thread use buf */
	if(0 != ipu_buf_lock())
		goto err_ipu_lock_buf;
	//	printk("---------osd_test3--------\n");
	flags = osdp->osd_flags;
	memset(&ip, 0, sizeof(ip));
	//	printk("---------osd_test4--------\n");
	ip.cmd = 0xf&osdp->osd_flags;
	ip.bg_w = osdp->bg_w;
	ip.bg_h = osdp->bg_h;
	ip.bg_fmt = ip.out_fmt = osdp->bg_fmt;
	//	printk("---------osd_test5--------\n");
	if (flags&IPU_OSD_FLAGS_BG_BUF_V) {
		start_offset =  _ipu_get_align_addr(&offset, ip.bg_w*ip.bg_h*3/2, 4);
		start_paddr = ipubuf_p + start_offset;
		start_vaddr = ipubuf_v + start_offset;
		printk("start_offset:%d offset:%d\n",start_offset,offset);
		printk("---------osd_test6--------\n");
		if (offset > ipubuf_size) {
			printk("----offset = 0x%08x----\n", offset);

			printk("ipu: %s,%d error ipu buffer too small\n", __func__, __LINE__);
			goto err_ipu_buf_small;
			printk("---------osd_test7--------\n");
		}
		ip.bg_buf_p = start_paddr;
		osdp->bg_buf_v = start_vaddr;
		memcpy((void *)start_vaddr, (void *)osdp->bg_buf, ip.bg_w*ip.bg_h*3/2);
		printk("---------osd_test8--------\n");
	} else {
		//        printk("---------osd_test8--------\n");
		ip.bg_buf_p = osdp->bg_buf;
	}

	if (flags&IPU_OSD_FLAGS_OSD0) {
		ip.osd_ch0_fmt = osdp->ch[0].osd_chx_fmt;
		ip.osd_ch0_pos_x = osdp->ch[0].osd_chx_pos_x;
		ip.osd_ch0_pos_y = osdp->ch[0].osd_chx_pos_y;
		ip.osd_ch0_src_w = osdp->ch[0].osd_chx_src_w;
		ip.osd_ch0_src_h = osdp->ch[0].osd_chx_src_h;
		ip.osd_ch0_2bit_bit0_fmt = osdp->ch[0].osd_chx_2bit_bit0_fmt;
		ip.osd_ch0_2bit_bit1_fmt = osdp->ch[0].osd_chx_2bit_bit1_fmt;
		ip.osd_ch0_2bit_mask = osdp->ch[0].osd_chx_2bit_mask;
		if (0 == osdp->ch[0].osd_chx_buf) {
			_ipu_set_osdx_mask(&ip.osd_ch0_para, osdp->ch[0].osd_chx_alpha);
			ip.osd_ch0_bak_argb = osdp->osd_mask_para;
		} else {
			unsigned int size = 0;
			if (osdp->ch[0].osd_chx_fmt == HAL_PIXEL_FORMAT_NV12) {
				size = osdp->ch[0].osd_chx_src_w*osdp->ch[0].osd_chx_src_h*3/2;
			} else if(osdp->ch[0].osd_chx_fmt == HAL_PIXEL_FORMAT_2BIT) {
				size = osdp->ch[0].osd_chx_src_w*osdp->ch[0].osd_chx_src_h/4;
			} else {
				size = osdp->ch[0].osd_chx_src_w*osdp->ch[0].osd_chx_src_h*4;
			}
			ret = _ipu_set_osdx_para(&ip.osd_ch0_para, osdp->ch[0].osd_chx_fmt, osdp->ch[0].osd_chx_alpha, osdp->bg_fmt);
			if (ret < 0) {
				printk("ipu: %s,%d error _ipu_set_osdx_para\n", __func__, __LINE__);
				goto err_ipu_set_osdx_para;
			}
			start_offset =  _ipu_get_align_addr(&offset, size, 4);
//            printk("start_offset:%d offset:%d\n",start_offset,offset);
			start_paddr = ipubuf_p + start_offset;
			start_vaddr = ipubuf_v + start_offset;
			if (offset > ipubuf_size) {
				printk("ipu: %s,%d error ipu buffer too small\n", __func__, __LINE__);
				goto err_ipu_buf_small;
			}
			ip.osd_ch0_buf_p = start_paddr;
			memcpy((void *)start_vaddr, (void *)osdp->ch[0].osd_chx_buf, size);
        }
	}

	if (flags&IPU_OSD_FLAGS_OSD1) {
		ip.osd_ch1_fmt = osdp->ch[1].osd_chx_fmt;
		ip.osd_ch1_pos_x = osdp->ch[1].osd_chx_pos_x;
		ip.osd_ch1_pos_y = osdp->ch[1].osd_chx_pos_y;
		ip.osd_ch1_src_w = osdp->ch[1].osd_chx_src_w;
		ip.osd_ch1_src_h = osdp->ch[1].osd_chx_src_h;
		ip.osd_ch1_2bit_bit0_fmt = osdp->ch[1].osd_chx_2bit_bit0_fmt;
		ip.osd_ch1_2bit_bit1_fmt = osdp->ch[1].osd_chx_2bit_bit1_fmt;
		ip.osd_ch1_2bit_mask = osdp->ch[1].osd_chx_2bit_mask;
		if (0 == osdp->ch[1].osd_chx_buf) {
			_ipu_set_osdx_mask(&ip.osd_ch1_para, osdp->ch[1].osd_chx_alpha);
			ip.osd_ch1_bak_argb = osdp->osd_mask_para;
		} else {
			/* ch1 share ch0 osd mem */
			if (chx_share_osd_mem(&osdp->ch[0], &osdp->ch[1])) {
				ip.osd_ch1_para = ip.osd_ch0_para;
				ip.osd_ch1_buf_p = ip.osd_ch0_buf_p;
			} else {
				unsigned int size = 0;
				ret = _ipu_set_osdx_para(&ip.osd_ch1_para, osdp->ch[1].osd_chx_fmt, osdp->ch[1].osd_chx_alpha, osdp->bg_fmt);
				if (ret < 0) {
					printk("ipu: %s,%d error _ipu_set_osdx_para\n", __func__, __LINE__);
					goto err_ipu_set_osdx_para;
				}
				if (osdp->ch[1].osd_chx_fmt == HAL_PIXEL_FORMAT_NV12) {
					size = osdp->ch[1].osd_chx_src_w*osdp->ch[1].osd_chx_src_h*3/2;
				} else if(osdp->ch[1].osd_chx_fmt == HAL_PIXEL_FORMAT_2BIT) {
					size = osdp->ch[1].osd_chx_src_w*osdp->ch[1].osd_chx_src_h/4;
				} else {
					size = osdp->ch[1].osd_chx_src_w*osdp->ch[1].osd_chx_src_h*4;
				}
				start_offset =  _ipu_get_align_addr(&offset, size, 4);
				start_paddr = ipubuf_p + start_offset;
				start_vaddr = ipubuf_v + start_offset;
				if (offset > ipubuf_size) {
					printk("ipu: %s,%d error ipu buffer too small\n", __func__, __LINE__);
					goto err_ipu_buf_small;
				}
				ip.osd_ch1_buf_p = start_paddr;
				memcpy((void *)start_vaddr, (void *)osdp->ch[1].osd_chx_buf, size);
			}
		}
	}
	if (flags&IPU_OSD_FLAGS_OSD2) {
		ip.osd_ch2_fmt = osdp->ch[2].osd_chx_fmt;
		ip.osd_ch2_pos_x = osdp->ch[2].osd_chx_pos_x;
		ip.osd_ch2_pos_y = osdp->ch[2].osd_chx_pos_y;
		ip.osd_ch2_src_w = osdp->ch[2].osd_chx_src_w;
		ip.osd_ch2_src_h = osdp->ch[2].osd_chx_src_h;
		ip.osd_ch2_2bit_bit0_fmt = osdp->ch[2].osd_chx_2bit_bit0_fmt;
		ip.osd_ch2_2bit_bit1_fmt = osdp->ch[2].osd_chx_2bit_bit1_fmt;
		ip.osd_ch2_2bit_mask = osdp->ch[2].osd_chx_2bit_mask;
		if (0 == osdp->ch[2].osd_chx_buf) {
			_ipu_set_osdx_mask(&ip.osd_ch2_para, osdp->ch[2].osd_chx_alpha);
			ip.osd_ch2_bak_argb = osdp->osd_mask_para;
		} else {
			if (chx_share_osd_mem(&osdp->ch[2], &osdp->ch[0])) {
				ip.osd_ch2_para = ip.osd_ch0_para;
				ip.osd_ch2_buf_p = ip.osd_ch0_buf_p;
			} else if (chx_share_osd_mem(&osdp->ch[2], &osdp->ch[1])) {
				ip.osd_ch2_para = ip.osd_ch1_para;
				ip.osd_ch2_buf_p = ip.osd_ch1_buf_p;
			} else {
				unsigned int size = 0;
				if (osdp->ch[2].osd_chx_fmt == HAL_PIXEL_FORMAT_NV12) {
					size = osdp->ch[2].osd_chx_src_w*osdp->ch[2].osd_chx_src_h*3/2;
				} else if(osdp->ch[2].osd_chx_fmt == HAL_PIXEL_FORMAT_2BIT) {
					size = osdp->ch[2].osd_chx_src_w*osdp->ch[2].osd_chx_src_h/4;
				} else {
					size = osdp->ch[2].osd_chx_src_w*osdp->ch[2].osd_chx_src_h*4;
				}
				ret = _ipu_set_osdx_para(&ip.osd_ch2_para, osdp->ch[2].osd_chx_fmt, osdp->ch[2].osd_chx_alpha, osdp->bg_fmt);
				if (ret < 0) {
					printk("ipu: %s,%d error _ipu_set_osdx_para\n", __func__, __LINE__);
					goto err_ipu_set_osdx_para;
				}
				start_offset =  _ipu_get_align_addr(&offset, size, 4);
				start_paddr = ipubuf_p + start_offset;
				start_vaddr = ipubuf_v + start_offset;
				if (offset > ipubuf_size) {
					printk("ipu: %s,%d error ipu buffer too small\n", __func__, __LINE__);
					goto err_ipu_buf_small;
				}
				ip.osd_ch2_buf_p = start_paddr;
				memcpy((void *)start_vaddr, (void *)osdp->ch[2].osd_chx_buf, size);
			}
		}
	}

	if (flags&IPU_OSD_FLAGS_OSD3) {
		ip.osd_ch3_fmt = osdp->ch[3].osd_chx_fmt;
		ip.osd_ch3_pos_x = osdp->ch[3].osd_chx_pos_x;
		ip.osd_ch3_pos_y = osdp->ch[3].osd_chx_pos_y;
		ip.osd_ch3_src_w = osdp->ch[3].osd_chx_src_w;
		ip.osd_ch3_src_h = osdp->ch[3].osd_chx_src_h;
		ip.osd_ch3_2bit_bit0_fmt = osdp->ch[3].osd_chx_2bit_bit0_fmt;
		ip.osd_ch3_2bit_bit1_fmt = osdp->ch[3].osd_chx_2bit_bit1_fmt;
		ip.osd_ch3_2bit_mask = osdp->ch[3].osd_chx_2bit_mask;
		if (0 == osdp->ch[3].osd_chx_buf) {
			_ipu_set_osdx_mask(&ip.osd_ch3_para, osdp->ch[3].osd_chx_alpha);
			ip.osd_ch3_bak_argb = osdp->osd_mask_para;
		} else {
			if (chx_share_osd_mem(&osdp->ch[3], &osdp->ch[0])) {
				ip.osd_ch3_para = ip.osd_ch0_para;
				ip.osd_ch3_buf_p = ip.osd_ch0_buf_p;
			} else if (chx_share_osd_mem(&osdp->ch[3], &osdp->ch[1])) {
				ip.osd_ch3_para = ip.osd_ch1_para;
				ip.osd_ch3_buf_p = ip.osd_ch1_buf_p;
			} else if (chx_share_osd_mem(&osdp->ch[3], &osdp->ch[2])) {
				ip.osd_ch3_para = ip.osd_ch2_para;
				ip.osd_ch3_buf_p = ip.osd_ch2_buf_p;
			} else {
				unsigned int size = 0;
				if (osdp->ch[3].osd_chx_fmt == HAL_PIXEL_FORMAT_NV12) {
					size = osdp->ch[3].osd_chx_src_w*osdp->ch[3].osd_chx_src_h*3/2;
				} else if(osdp->ch[3].osd_chx_fmt == HAL_PIXEL_FORMAT_2BIT) {
					size = osdp->ch[3].osd_chx_src_w*osdp->ch[3].osd_chx_src_h/4;
				} else {
					size = osdp->ch[3].osd_chx_src_w*osdp->ch[3].osd_chx_src_h*4;
				}
				ret = _ipu_set_osdx_para(&ip.osd_ch3_para, osdp->ch[3].osd_chx_fmt, osdp->ch[3].osd_chx_alpha, osdp->bg_fmt);
				if (ret < 0) {
					printk("ipu: %s,%d error _ipu_set_osdx_para\n", __func__, __LINE__);
					goto err_ipu_set_osdx_para;
				}
				start_offset =  _ipu_get_align_addr(&offset, size, 4);
				start_paddr = ipubuf_p + start_offset;
				start_vaddr = ipubuf_v + start_offset;
				if (offset > ipubuf_size) {
					printk("ipu: %s,%d error ipu buffer too small\n", __func__, __LINE__);
					goto err_ipu_buf_small;
				}
				ip.osd_ch3_buf_p = start_paddr;
				memcpy((void *)start_vaddr, (void *)osdp->ch[3].osd_chx_buf, size);
			}
		}
	}

	fcache_para.addr = (void *)ipu_vbuf;
	/* flash cache all */
	//fcache_para.size = 2*1024*1024;
	fcache_para.size = offset;

#if 1
	//printk("cache: addr(0x%x) size(0x%x)\n", (unsigned int)fcache_para.addr, fcache_para.size);
	if (fcache_para.size != 0) {
        ret = ipu_ioctl(NULL,IOCTL_IPU_BUF_FLUSH_CACHE,(unsigned long)&fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}
#endif

//	printk("*!!!!!!!!!!!!!ipu_param->cmd = %d, ipu_param->osd_ch0_fmt = %x osd_ch01_fmt = %x, osd_ch2_fmt = %x, osd_ch3_fmt = %x \n", ip.cmd, 
//			            ip.osd_ch0_fmt, ip.osd_ch1_fmt, ip.osd_ch2_fmt, ip.osd_ch3_fmt);
    ret = ipu_ioctl(NULL,IOCTL_IPU_START,(unsigned long)&ip);
	if (ret != 0) {
		printk("ioctl error ret = %d\n", ret);
#if 0
		{
			FILE *fp;
			char *fname = "ipu_test_dump_err.nv12";
			printk("##### SaveNv12Data, save data : %s\n", fname);

			if ((fp = fopen(fname, "wb")) == NULL) {
				printk("##### err open file %s\n", fname);
				return -1;
			}
			fwrite((void *)osdp->bg_buf_v, 1, osdp->bg_w*osdp->bg_h*3/2 , fp);
			fclose(fp);
		}
#endif
		goto err_ioctl_ipu_start;
	}
#if 0
	//printk("cache: addr(0x%x) size(0x%x)\n", (unsigned int)fcache_para.addr, fcache_para.size);
	if (fcache_para.size != 0) {
		ret = ioctl(ipu_ipufd, IOCTL_IPU_BUF_FLUSH_CACHE, &fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}
#endif
#if 0
	if (flags&IPU_OSD_FLAGS_BG_BUF_V) {
		memcpy((void *)osdp->bg_buf, (void *)osdp->bg_buf_v, osdp->bg_w*osdp->bg_h*3/2);
	}
#endif

#if 0
	{
		FILE *fp;
		char *fname = "ipu_test_dump.nv12";
		printk("##### SaveNv12Data, save data : %s\n", fname);

		if ((fp = fopen(fname, "wb")) == NULL) {
			printk("##### err open file %s\n", fname);
			return -1;
		}
		fwrite((void *)osdp->bg_buf_v, 1, osdp->bg_w*osdp->bg_h*3/2 , fp);
		fclose(fp);
	}
#endif
	ipu_buf_unlock();
	return 0;

err_ioctl_ipu_start:
err_cache_flush:
err_ipu_buf_small:
err_ipu_set_osdx_para:
	ipu_buf_unlock();
	ipu_deinit();
err_ipu_lock_buf:
	return -1;
}

int ipu_csc_nv12_to_hsv(uint8_t *psrc, uint8_t *pdst, uint32_t width, uint32_t height)
{
	int ret = 0;
    struct ipu_param cscp;
	struct ipu_flush_cache_para fcache_para;
/*
	if ((ret = ipu_init()) != 0) {
		printk("ipu: error ipu_init ret = %d\n", ret);
		return -1;
	}
*/

    cscp.cmd             = 0x10;
    cscp.bg_w            = width;
    cscp.bg_h            = height;
    cscp.bg_fmt          = HAL_PIXEL_FORMAT_NV12;
    cscp.bg_buf_p        = (unsigned int)psrc;
    cscp.out_fmt         = HAL_PIXEL_FORMAT_HSV;
    cscp.osd_ch0_buf_p   = (unsigned int)pdst;

	DBG(" csc_test  inputbuf_p = 0x%08x outbufp = 0x%08x\n", cscp.bg_buf_p, cscp.osd_ch0_buf_p);
	fcache_para.addr = (void *)ipu_vbuf;
	fcache_para.size = 2*1024*1024;

	if (fcache_para.size != 0) {
        ret = ipu_ioctl(NULL,IOCTL_IPU_BUF_FLUSH_CACHE,(unsigned long)&fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}

    ret = ipu_ioctl(NULL,IOCTL_IPU_START,(unsigned long)&cscp);
    if (ret < 0) {
        printk("ioctl test error: ret = %d\n", ret);
        goto err_ioctl_ipu_start;
    }

	fcache_para.addr = (void *)ipu_vbuf;
	fcache_para.size = 2*1024*1024;

	if (fcache_para.size != 0) {
        ret = ipu_ioctl(NULL,IOCTL_IPU_BUF_FLUSH_CACHE,(unsigned long)&fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}

    return 0;
err_cache_flush:
err_ioctl_ipu_start:
    return -1;
}

int ipu_csc_nv12_to_bgra(uint8_t *psrc, uint8_t *pdst, uint32_t width, uint32_t height)
{
	int ret = 0;
    struct ipu_param cscp;
	struct ipu_flush_cache_para fcache_para;
/*
	if ((ret = ipu_init()) != 0) {
		printk("ipu: error ipu_init ret = %d\n", ret);
		return -1;
	}
*/

#if 0
	int out_fmt = HAL_PIXEL_FORMAT_BGRA;
	switch(cmdline) {
		case 0:
			out_fmt = HAL_PIXEL_FORMAT_ARGB;
		case 1:
			out_fmt = HAL_PIXEL_FORMAT_ARBG;
		case 2:
			out_fmt = HAL_PIXEL_FORMAT_AGRB;
		case 3:
			out_fmt = HAL_PIXEL_FORMAT_AGBR;
		case 4:
			out_fmt = HAL_PIXEL_FORMAT_ABRG;
		case 5:
			out_fmt = HAL_PIXEL_FORMAT_ABGR;
		case 6:
			out_fmt = HAL_PIXEL_FORMAT_RGBA;
		case 7:
			out_fmt = HAL_PIXEL_FORMAT_RBGA;
		case 8:
			out_fmt = HAL_PIXEL_FORMAT_GRBA;
		case 9:
			out_fmt = HAL_PIXEL_FORMAT_GBRA;
		case 10:
			out_fmt = HAL_PIXEL_FORMAT_BRGA;
		case 11:
			out_fmt = HAL_PIXEL_FORMAT_BGRA;
			break;
	}
#endif

    cscp.cmd         = 0x10;
    cscp.bg_w        = width;
    cscp.bg_h        = height;
    cscp.bg_fmt      = HAL_PIXEL_FORMAT_NV12;
    cscp.bg_buf_p    = (unsigned int)psrc;
    cscp.out_fmt     = HAL_PIXEL_FORMAT_RGBA_8888;
    cscp.osd_ch0_buf_p    = (unsigned int)pdst;

	DBG(" csc_test  inputbuf_p = 0x%08x outbufp = 0x%08x\n", cscp.bg_buf_p, cscp.osd_ch0_buf_p);
	fcache_para.addr = (void *)ipu_vbuf;
	fcache_para.size = 1*1024*1024;

	if (fcache_para.size != 0) {
        ret = ipu_ioctl(NULL,IOCTL_IPU_BUF_FLUSH_CACHE,(unsigned long)&fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}

    ret = ipu_ioctl(NULL,IOCTL_IPU_START,(unsigned long)&cscp);
    if (ret < 0) {
        printk("ioctl test error: ret = %d\n", ret);
        goto err_ioctl_ipu_start;
    }

	fcache_para.addr = (void *)ipu_vbuf;
	fcache_para.size = 1*1024*1024;

	if (fcache_para.size != 0) {
        ret = ipu_ioctl(NULL,IOCTL_IPU_BUF_FLUSH_CACHE,(unsigned long)&fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}

    return 0;
err_cache_flush:
err_ioctl_ipu_start:
    return -1;
}

int ipu_csc_nv12_to_nv21(uint8_t *psrc, uint8_t *pdst, uint32_t width, uint32_t height)
{
	int ret = 0;
    struct ipu_param cscp;
	struct ipu_flush_cache_para fcache_para;
/*
	if ((ret = ipu_init()) != 0) {
		printk("ipu: error ipu_init ret = %d\n", ret);
		return -1;
	}
*/

    cscp.cmd         = 0x10;
    cscp.bg_w        = width;
    cscp.bg_h        = height;
    cscp.bg_fmt      = HAL_PIXEL_FORMAT_NV12;
    cscp.bg_buf_p    = (unsigned int)psrc;
    cscp.out_fmt     = HAL_PIXEL_FORMAT_NV21;
    cscp.osd_ch0_buf_p    = (unsigned int)pdst;

	DBG(" csc_test  inputbuf_p = 0x%08x outbufp = 0x%08x\n", cscp.bg_buf_p, cscp.osd_ch0_buf_p);
	fcache_para.addr = (void *)ipu_vbuf;
	fcache_para.size = 1*1024*1024;

	if (fcache_para.size != 0) {
        ret = ipu_ioctl(NULL,IOCTL_IPU_BUF_FLUSH_CACHE,(unsigned long)&fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}

    ret = ipu_ioctl(NULL,IOCTL_IPU_START,(unsigned long)&cscp);
    if (ret < 0) {
        printk("ioctl test error: ret = %d\n", ret);
        goto err_ioctl_ipu_start;
    }

	fcache_para.addr = (void *)ipu_vbuf;
	fcache_para.size = 1*1024*1024;

	if (fcache_para.size != 0) {
        ret = ipu_ioctl(NULL,IOCTL_IPU_BUF_FLUSH_CACHE,(unsigned long)&fcache_para);
		if (ret < 0) {
			printk("ioctl IOCTL_IPU_BUF_FLUSH_CACHE error ret = %d\n", ret);
			goto err_cache_flush;
		}
	}
    return 0;
err_cache_flush:
err_ioctl_ipu_start:
    return -1;
}
