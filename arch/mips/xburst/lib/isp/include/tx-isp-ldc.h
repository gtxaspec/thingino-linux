#ifndef __TX_ISP_LDC_H__
#define __TX_ISP_LDC_H__

#define JZ_ISP_ONLINE_LDC_LUT_MODE 0x01
#define JZ_ISP_ONLINE_LDC_PARAM_MODE 0x02
#define LDC_LUT_OFFSET_SIZE  128
enum ldc_vi_num {
	LDC_VI_MAIN = 0,
	LDC_VI_SEC = 1,
	LDC_VI_THR = 2,
	LDC_VI_FOUR = 3,
	LDC_VI_BUTT,
};

enum ldc_func_mode {
	LDC_ISP_I2D_MODE  = 1,
	LDC_ISP_HLDC_MODE = 2,
	LDC_ISP_LDC_MODE  = 4,
	LDC_ISP_LDC_FUNC_BUTT,
};

enum ldc_lut_blockszie {
	LDC_LUT_BLOCKSIZE_8x8   = 8,
	LDC_LUT_BLOCKSIZE_16x16 = 16,
	LDC_LUT_BLOCKSIZE_32x32 = 32,
	LDC_LUT_BLOCKSIZE_64x64 = 64,
	LDC_LUT_BLOCKSIZE_BUTT,
};

enum ldc_i2d_angle{
	LDC_ISP_I2D_0 = 0,
	LDC_ISP_I2D_90,
	LDC_ISP_I2D_180,
	LDC_ISP_I2D_270,
	LDC_ISP_I2D_MIRROR,
	LDC_ISP_I2D_FLIP,
	LDC_ISP_I2D_BUTT,
};

enum ldc_isp_ops_mode{
	LDC_OPS_MODE_DISABLE = 0,               //不使能该模块功能
	LDC_OPS_MODE_ENABLE,            //使能该模块功能
	LDC_OPS_MODE_BUTT,                      //用于判断参数的有效性，参数大小必须小于这个值
};

//i2d
struct t32_ldc_online_init {
	enum ldc_vi_num vinum;;
	uint16_t width;
	uint16_t height;
	uint16_t ldc_enable;
	uint16_t api_par_enable;
	uint32_t lut_offset;
	uint32_t lut_size;
	void    *lut_addr;
};
struct ldc_online_init {
	enum ldc_vi_num         vinum;
	enum ldc_func_mode      func;
	enum ldc_lut_blockszie  size;
	enum ldc_isp_ops_mode   state;
	enum ldc_i2d_angle      angle;
	uint16_t                width;
	uint16_t                height;
	uint16_t                lut_mode;
	uint32_t                lut_addr;
	uint32_t                lut_offset;
	uint32_t                lut_size;
};

enum tx_isp_ldc_mode{
	LDC_ONLINE_MODE = 0,
	LDC_OFFLINE_MODE,
	LDC_MODE_BUTT,                  /**< 用于判断参数的有效性，参数大小必须小于这个值 */
};

enum tx_isp_offline_ldc_ioctl {
	LDC_OFFLINE_CMD_CREATE_JOB = 0x00,
	LDC_OFFLINE_CMD_DESTROY_JOB,
	LDC_OFFLINE_CMD_SUBMIT_TASK,
};

struct tx_isp_offline_ldc_job {
	uint16_t  width;  //当前输入的宽度
	uint16_t  height; //当前输入的高度
	uint32_t  ldc_lut_addr;
	uint32_t  ldc_lut_size;
	enum ldc_func_mode      func;
	enum ldc_lut_blockszie  size;
	uint32_t	job_hander;
};

struct tx_isp_offline_ldc_task {
	uint16_t width;     //当前输入的宽度
	uint16_t height;    //当前输入的高度
	uint32_t addr_in;
	uint32_t addr_uv_in;
	uint32_t addr_out;
	uint32_t addr_uv_out;
	uint16_t stride_in;
	uint16_t stride_out;
	uint32_t lut_addr;
	enum ldc_func_mode      func;
	enum ldc_lut_blockszie  size;
	enum ldc_i2d_angle      angle;      //使用I2D时需要配置
};


typedef struct {
	int blocksize;
	double strength;
	double d0;
	double d1;
	double d2;
	double d3;
	double fx;
	double fy;
	double cx;
	double cy;
} ISPLDCCalibrationParam;  /**<Parameters used for generating the LUT table */

#define SOC_TYPE_SIZE 8
typedef struct tisp_ldc_par_header {
	unsigned int size;
	unsigned int crc;
	char soc[SOC_TYPE_SIZE];
} tisp_ldc_par_header_t;

typedef struct tisp_ldc_lut_par {
	double intrinsicMat[4];
	double distortionCeff[4];
	double ratio;
	unsigned int inWidth;
	unsigned int inHeight;
	unsigned int outWidth;
	unsigned int outHeight;
	unsigned int blockSize;
	char version[8];
	unsigned int *lut;
} tisp_ldc_lut_par_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t y_str;
    uint32_t uv_str;
    uint32_t k1_x;
    uint32_t k2_x;
    uint32_t k1_y;
    uint32_t k2_y;
    uint32_t r2_rep;
    uint32_t len_cofe;
    int16_t y_shift_lut[256];
    int16_t uv_shift_lut[256];
} JZ_ISP_ldc_param_cfg;

typedef struct {
    uint16_t ldc_haf_en;
    uint16_t width;
    uint16_t height;
    uint16_t k1;
    uint16_t k2;
    uint16_t ldc_back;
    JZ_ISP_ldc_param_cfg st_ldc_par_cfg;
} JZ_ISP_ldc_param;

#endif /* __TX_ISP_LDC_H__ */
