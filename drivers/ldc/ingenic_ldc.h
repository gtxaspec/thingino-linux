#ifndef __LDC__H__
#define __LDC__H__

/*
#define SAMPLE_IOC_MAGIC  'S'
#define IOCTL_SAMPLE_GET			_IO(SAMPLE_IOC_MAGIC, 100)
#define IOCTL_SAMPLE_SET		    _IO(SAMPLE_IOC_MAGIC, 101)
#define IOCTL_LDC_INIT		        _IO(SAMPLE_IOC_MAGIC, 102)
#define IOCTL_LDC_SET_DMA_ADDR      _IO(SAMPLE_IOC_MAGIC, 103)
*/

#define JZLDC_IOC_MAGIC  'D'
#define IOCTL_LDC_START			_IO(JZLDC_IOC_MAGIC, 106)
#define IOCTL_LDC_LISTEN		_IO(JZLDC_IOC_MAGIC, 116)
#define IOCTL_LDC_RES_PBUFF		_IO(JZLDC_IOC_MAGIC, 114)
#define IOCTL_LDC_GET_PBUFF		_IO(JZLDC_IOC_MAGIC, 115)
#define IOCTL_LDC_BUF_UNLOCK	_IO(JZLDC_IOC_MAGIC, 117)
#define IOCTL_LDC_BUF_FLUSH_CACHE	_IO(JZLDC_IOC_MAGIC, 118)

#define LDC_BASE	0x13040000

#define LDC_ADDR_VERSION	                0x00
#define LDC_ADDR_LDC_WORK_EN	            0x04
#define LDC_ADDR_RES_SIZE	                0x10
#define LDC_ADDR_X_CORF	                    0x14
#define LDC_ADDR_BROCF	                    0x1c
#define LDC_ADDR_R2_REP	                    0x20
#define LDC_ADDR_UDIS_R	                    0x24
#define LDC_ADDR_Y_DMAIN_ADDR	            0x30
#define LDC_ADDR_Y_DMAIN_STRIDE	            0x34
#define LDC_ADDR_Y_DMAOT_ADDR	            0x38
#define LDC_ADDR_Y_DMAOT_STRIDE	            0x3c
#define LDC_ADDR_UV_DMAIN_ADDR	            0x40
#define LDC_ADDR_UV_DMAIN_STRIDE	        0x44
#define LDC_ADDR_UV_DMAOT_ADDR	            0x48
#define LDC_ADDR_UV_DMAOT_STRIDE	        0x4c
#define LDC_ADDR_LEN_COEF	                0x50
#define LDC_ADDR_CFG_BUS	                0x60
#define LDC_ADDR_RESET	                    0x64
#define LDC_ADDR_INT_STATE	                0x68
#define LDC_ADDR_INT_CLR	                0x6c
#define LDC_ADDR_INT_MASK	                0x70
#define LDC_ADDR_TIME_OUT	                0x78
#define LDC_ADDR_DEBUG	                    0xf84

#define u32 unsigned int

struct ldc_reg_struct {
	char *name;
	unsigned int addr;
};

struct ldc_buf_info {
	unsigned int vaddr_alloc;
	unsigned int paddr;
	unsigned int paddr_align;
	unsigned int size;
};

struct jz_ldc {

	int irq;
	char name[16];

	struct clk *clk;
	struct clk *ahb0_gate; /* T31 IPU mount at AHB1*/
	void __iomem *iomem;
	struct device *dev;
	struct resource *res;
	struct miscdevice misc_dev;

	struct mutex mutex;
	struct mutex irq_mutex;
	struct completion done_ldc;
	struct completion done_buf;
	struct ldc_buf_info pbuf;

	int streaming;
};

#define LDC_Enable 0x1
#define LDC_Disable 0x0

struct ldc_flush_cache_para
{
	void *addr;
	unsigned int size;
};

struct ldc_param
{
	unsigned int 		src_w;
	unsigned int 		src_h;
	unsigned int 		data_type;

    unsigned int		in_addr;
	unsigned int		out_addr;

	unsigned int		ldc_default_params_num;
};

typedef struct _ldc_opt_ {
	uint32_t width;
	uint32_t height;
	uint32_t y_str;
	uint32_t uv_str;
	uint32_t k1_x;
	uint32_t k2_x;
	uint32_t k1_y;
	uint32_t k2_y;
	uint32_t p1_val_x;
	uint32_t p1_val_y;
	uint32_t r2_rep;
	uint32_t len_cofe;
	uint32_t wr_side_len;
	uint32_t rd_len;
	uint32_t rd_side_len;
	uint32_t y_fill;
	uint32_t u_fill;
	uint32_t v_fill;
	uint32_t view_mode;
	uint32_t udis_r;
	int16_t y_shift_lut[256];
	int16_t uv_shift_lut[256];
} tx_isp_ldc_opt;

struct ldc_initarg{
    int buf;
    int on;
};

/* match HAL_PIXEL_FORMAT_ in system/core/include/system/graphics.h */

/**
 * IMP图像格式定义.
 */
typedef enum {
	PIX_FMT_YUV420P,   /**< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples) */
	PIX_FMT_YUYV422,   /**< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr */
	PIX_FMT_UYVY422,   /**< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1 */
	PIX_FMT_YUV422P,   /**< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples) */
	PIX_FMT_YUV444P,   /**< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples) */
	PIX_FMT_YUV410P,   /**< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples) */
	PIX_FMT_YUV411P,   /**< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples) */
	PIX_FMT_GRAY8,     /**<	   Y	    ,  8bpp */
	PIX_FMT_MONOWHITE, /**<	   Y	    ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb */
	PIX_FMT_MONOBLACK, /**<	   Y	    ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb */

	PIX_FMT_NV12,      /**< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V) */
	PIX_FMT_NV21,      /**< as above, but U and V bytes are swapped */

	PIX_FMT_RGB24,     /**< packed RGB 8:8:8, 24bpp, RGBRGB... */
	PIX_FMT_BGR24,     /**< packed RGB 8:8:8, 24bpp, BGRBGR... */

	PIX_FMT_ARGB,      /**< packed ARGB 8:8:8:8, 32bpp, ARGBARGB... */
	PIX_FMT_RGBA,	   /**< packed RGBA 8:8:8:8, 32bpp, RGBARGBA... */
	PIX_FMT_ABGR,	   /**< packed ABGR 8:8:8:8, 32bpp, ABGRABGR... */
	PIX_FMT_BGRA,	   /**< packed BGRA 8:8:8:8, 32bpp, BGRABGRA... */

	PIX_FMT_RGB565BE,  /**< packed RGB 5:6:5, 16bpp, (msb)	  5R 6G 5B(lsb), big-endian */
	PIX_FMT_RGB565LE,  /**< packed RGB 5:6:5, 16bpp, (msb)	  5R 6G 5B(lsb), little-endian */
	PIX_FMT_RGB555BE,  /**< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian, most significant bit to 0 */
	PIX_FMT_RGB555LE,  /**< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian, most significant bit to 0 */

	PIX_FMT_BGR565BE,  /**< packed BGR 5:6:5, 16bpp, (msb)	 5B 6G 5R(lsb), big-endian */
	PIX_FMT_BGR565LE,  /**< packed BGR 5:6:5, 16bpp, (msb)	 5B 6G 5R(lsb), little-endian */
	PIX_FMT_BGR555BE,  /**< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian, most significant bit to 1 */
	PIX_FMT_BGR555LE,  /**< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian, most significant bit to 1 */

	PIX_FMT_0RGB,      /**< packed RGB 8:8:8, 32bpp, 0RGB0RGB... */
	PIX_FMT_RGB0,	   /**< packed RGB 8:8:8, 32bpp, RGB0RGB0... */
	PIX_FMT_0BGR,	   /**< packed BGR 8:8:8, 32bpp, 0BGR0BGR... */
	PIX_FMT_BGR0,	   /**< packed BGR 8:8:8, 32bpp, BGR0BGR0... */

	PIX_FMT_BAYER_BGGR8,    /**< bayer, BGBG..(odd line), GRGR..(even line), 8-bit samples */
	PIX_FMT_BAYER_RGGB8,    /**< bayer, RGRG..(odd line), GBGB..(even line), 8-bit samples */
	PIX_FMT_BAYER_GBRG8,    /**< bayer, GBGB..(odd line), RGRG..(even line), 8-bit samples */
	PIX_FMT_BAYER_GRBG8,    /**< bayer, GRGR..(odd line), BGBG..(even line), 8-bit samples */

	PIX_FMT_RAW,

	PIX_FMT_HSV,

	PIX_FMT_NB,
	PIX_FMT_YUV422,
	PIX_FMT_YVU422,
	PIX_FMT_UVY422,
	PIX_FMT_VUY422,
	PIX_FMT_RAW8,
	PIX_FMT_RAW16,
} IMPPixelFormat;


#define JZLDC_IOC_MAGIC  'D'
#define IOCTL_LDC_START			_IO(JZLDC_IOC_MAGIC, 106)
#define IOCTL_LDC_LISTEN		_IO(JZLDC_IOC_MAGIC, 116)
#define IOCTL_LDC_RES_PBUFF		_IO(JZLDC_IOC_MAGIC, 114)
#define IOCTL_LDC_GET_PBUFF		_IO(JZLDC_IOC_MAGIC, 115)
#define IOCTL_LDC_BUF_UNLOCK	_IO(JZLDC_IOC_MAGIC, 117)
#define IOCTL_LDC_BUF_FLUSH_CACHE	_IO(JZLDC_IOC_MAGIC, 118)


static inline unsigned int reg_read(struct jz_ldc *jzldc, int offset)
{
	return readl(jzldc->iomem + offset);
}

static inline void reg_write(struct jz_ldc *jzldc, int offset, unsigned int val)
{
	writel(val, jzldc->iomem + offset);
}

#define __ldc_mask_irq()		reg_bit_set(ldc, LDC_IRQ_MASK, (LDC_IRQ_TIMEOUT_MASK | LDC_IRQ_FRAME_DONE_MASK))
#define __ldc_irq_clear(value)		reg_bit_set(ldc, LDC_ADDR_INT_CLR, value)

#define __start_ldc()			reg_bit_set(ldc, 0, LDC_START)
//#define __reset_safe_ldc()		reg_bit_set(ldc, 0, LDC_SAFE_RESET)
#define __reset_ldc(value)			reg_bit_set(ldc, LDC_ADDR_RESET, value)

#endif
