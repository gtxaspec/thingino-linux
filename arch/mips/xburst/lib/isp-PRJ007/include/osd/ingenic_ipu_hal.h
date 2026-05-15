/*
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Hal head file for Ingenic IPU device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __JZ_IPU_HAL_H__
#define __JZ_IPU_HAL_H__

#include <linux/fb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPU_DEVNAME    "/dev/ipu"

#define uint8_t unsigned char
#define uint32_t unsigned int

#define OSD_MMAP_OFFSET (0x4000000)
#define OSD_MMAP_SIZES  (500 * 1024)
#define CSC_MMAP_OFFSET (0xA000000)
#define CSC_MMAP_SIZES  (0x2000000)




struct ipu_param
{
	unsigned int		cmd;			/* IPU command */

	unsigned int 		bg_w;			/* background weight */
	unsigned int 		bg_h;			/* background hight */
	unsigned int 		bg_fmt;			/* background format */
	unsigned int		bg_buf_p;       /* background buffer physical addr */

	unsigned int 		out_fmt;		/* out format */

	unsigned int		osd_ch0_fmt;	/* osd ch0 para */
	unsigned int		osd_ch0_para;
	unsigned int		osd_ch0_bak_argb;
	unsigned int		osd_ch0_pos_x;
	unsigned int		osd_ch0_pos_y;
	unsigned int		osd_ch0_src_w;
	unsigned int		osd_ch0_src_h;
	unsigned int        osd_ch0_buf_p;
	unsigned int            osd_ch0_2bit_bit0_fmt;               /* new format */
	unsigned int            osd_ch0_2bit_bit1_fmt;               /* new format */
	unsigned int            osd_ch0_2bit_mask;					 /* new format */

	unsigned int		osd_ch1_fmt;	/* osd ch1 para */
	unsigned int		osd_ch1_para;
	unsigned int		osd_ch1_bak_argb;
	unsigned int		osd_ch1_pos_x;
	unsigned int		osd_ch1_pos_y;
	unsigned int		osd_ch1_src_w;
	unsigned int		osd_ch1_src_h;
	unsigned int        osd_ch1_buf_p;
	unsigned int            osd_ch1_2bit_bit0_fmt;               /* new format */
	unsigned int            osd_ch1_2bit_bit1_fmt;               /* new format */
	unsigned int            osd_ch1_2bit_mask;					 /* new format */

	unsigned int		osd_ch2_fmt;	/* osd ch2 para */
	unsigned int		osd_ch2_para;
	unsigned int		osd_ch2_bak_argb;
	unsigned int		osd_ch2_pos_x;
	unsigned int		osd_ch2_pos_y;
	unsigned int		osd_ch2_src_w;
	unsigned int		osd_ch2_src_h;
	unsigned int        osd_ch2_buf_p;
	unsigned int            osd_ch2_2bit_bit0_fmt;               /* new format */
	unsigned int            osd_ch2_2bit_bit1_fmt;               /* new format */
	unsigned int            osd_ch2_2bit_mask;					 /* new format */

	unsigned int		osd_ch3_fmt;	/* osd ch3 para */
	unsigned int		osd_ch3_para;
	unsigned int		osd_ch3_bak_argb;
	unsigned int		osd_ch3_pos_x;
	unsigned int		osd_ch3_pos_y;
	unsigned int		osd_ch3_src_w;
	unsigned int		osd_ch3_src_h;
	unsigned int        osd_ch3_buf_p;
	unsigned int            osd_ch3_2bit_bit0_fmt;               /* new format */
	unsigned int            osd_ch3_2bit_bit1_fmt;               /* new format */
	unsigned int            osd_ch3_2bit_mask;					 /* new format */
};


struct ipu_flush_cache_para
{
	void *addr;
	unsigned int size;
};

#define JZIPU_IOC_MAGIC  'I'

#define IOCTL_IPU_START         _IO(JZIPU_IOC_MAGIC, 106)
#define IOCTL_IPU_RES_PBUFF     _IO(JZIPU_IOC_MAGIC, 114)
#define IOCTL_IPU_GET_PBUFF     _IO(JZIPU_IOC_MAGIC, 115)
#define IOCTL_IPU_BUF_LOCK      _IO(JZIPU_IOC_MAGIC, 116)
#define IOCTL_IPU_BUF_UNLOCK    _IO(JZIPU_IOC_MAGIC, 117)
#define IOCTL_IPU_BUF_FLUSH_CACHE    _IO(JZIPU_IOC_MAGIC, 118)


/* match HAL_PIXEL_FORMAT_ in system/core/include/system/graphics.h */
enum {
	HAL_PIXEL_FORMAT_RGBA_8888    = 1,
	HAL_PIXEL_FORMAT_RGBX_8888    = 2,
	HAL_PIXEL_FORMAT_RGB_888      = 3,
	HAL_PIXEL_FORMAT_RGB_565      = 4,
	HAL_PIXEL_FORMAT_BGRA_8888    = 5,
	//HAL_PIXEL_FORMAT_BGRX_8888    = 0x8000, /* Add BGRX_8888, Wolfgang, 2010-07-24 */
	HAL_PIXEL_FORMAT_BGRX_8888      = 0x1ff, /* 2012-10-23 */
	HAL_PIXEL_FORMAT_RGBA_5551    = 6,
	HAL_PIXEL_FORMAT_RGBA_4444    = 7,
	HAL_PIXEL_FORMAT_ABGR_8888    = 8,
	HAL_PIXEL_FORMAT_ARGB_8888    = 9,
	HAL_PIXEL_FORMAT_YCbCr_422_SP = 0x10,
	HAL_PIXEL_FORMAT_YCbCr_420_SP = 0x11,
	HAL_PIXEL_FORMAT_YCbCr_422_P  = 0x12,
	HAL_PIXEL_FORMAT_YCbCr_420_P  = 0x13,
	HAL_PIXEL_FORMAT_YCbCr_420_B  = 0x24,
	HAL_PIXEL_FORMAT_YCbCr_422_I  = 0x14,
	HAL_PIXEL_FORMAT_YCbCr_420_I  = 0x15,
	HAL_PIXEL_FORMAT_CbYCrY_422_I = 0x16,
	HAL_PIXEL_FORMAT_CbYCrY_420_I = 0x17,
	HAL_PIXEL_FORMAT_NV12         = 0x18,
	HAL_PIXEL_FORMAT_NV21         = 0x19,
	HAL_PIXEL_FORMAT_BGRA_5551    = 0x1a,
	HAL_PIXEL_FORMAT_HSV          = 0x1b,/*Add HSV*/
	HAL_PIXEL_FORMAT_2BIT         = 0x1c,/*新增格式*/
	HAL_PIXEL_FORMAT_ARBG_8888    = 0x1d,
	HAL_PIXEL_FORMAT_AGRB_8888    = 0x1e,
	HAL_PIXEL_FORMAT_AGBR_8888    = 0x20,
	HAL_PIXEL_FORMAT_ABRG_8888    = 0x21,
	HAL_PIXEL_FORMAT_RBGA_8888    = 0x22,
	HAL_PIXEL_FORMAT_GRBA_8888    = 0x23,
	HAL_PIXEL_FORMAT_GBRA_8888    = 0x25,
	HAL_PIXEL_FORMAT_BRGA_8888    = 0x26,

	/* suport for YUV420 */
	HAL_PIXEL_FORMAT_JZ_YUV_420_P       = 0x47700001, // YUV_420_P
	HAL_PIXEL_FORMAT_JZ_YUV_420_B       = 0x47700002, // YUV_420_P BLOCK MODE

};

struct ipu_buf_info {
	unsigned int vaddr_alloc;
	unsigned int paddr;
	unsigned int paddr_align;
	unsigned int size;
};

struct ipu_osdx_para
{
	unsigned int		osd_chx_fmt;
	unsigned int		osd_chx_alpha;
	unsigned int		osd_chx_pos_x;
	unsigned int		osd_chx_pos_y;
	unsigned int		osd_chx_src_w;
	unsigned int		osd_chx_src_h;
	unsigned int        osd_chx_buf;
	unsigned int        osd_chx_2bit_bit0_fmt;
	unsigned int        osd_chx_2bit_bit1_fmt;
	unsigned int        osd_chx_2bit_mask;
};

struct ipu_osd_param
{
	unsigned int		osd_flags;

	unsigned int 		bg_w;			/* background weight */
	unsigned int 		bg_h;			/* background hight */
	unsigned int 		bg_fmt;			/* background format */
	unsigned int		bg_buf;
	unsigned int		bg_buf_v;

	unsigned int		osd_mask_para;

	struct ipu_osdx_para ch[4];

};

#define IPU_OSD_FLAGS_OSD0		(1<<0)
#define IPU_OSD_FLAGS_OSD1		(1<<1)
#define IPU_OSD_FLAGS_OSD2		(1<<2)
#define IPU_OSD_FLAGS_OSD3		(1<<3)
#define IPU_OSD_FLAGS_BG_BUF_V	(1<<4) /* background buf virtual */
int ipu_open(struct inode *inode, struct file *filp);
int ipu_osd(struct ipu_osd_param *ipu_osd_param);
long ipu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

int ipu_csc_nv12_to_hsv(uint8_t *psrc, uint8_t *pdst, uint32_t width, uint32_t height);
int ipu_csc_nv12_to_bgra(uint8_t *psrc, uint8_t *pdst, uint32_t width, uint32_t height);
int ipu_csc_nv12_to_nv21(uint8_t *psrc, uint8_t *pdst, uint32_t width, uint32_t height);
int ipu_buf_lock(void);
int ipu_buf_unlock(void);

int ipu_deinit(void);

#ifdef __cplusplus
}
#endif

#endif  //__JZ_IPU_HAL_H__
