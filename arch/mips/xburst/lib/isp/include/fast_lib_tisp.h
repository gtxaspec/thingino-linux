#ifndef __FAST_LIB_TISP_H__
#define __FAST_LIB_TISP_H__
/****************** ISP LDC FUNC ********************/
typedef struct tisp_ldc {
	double intrinsicMat[4];
	double distortionCeff[4];
	uint8_t valid;
} tisp_ldc_t;

typedef struct tisp_ldc_par_cfg {
	tisp_ldc_t ldcPar;
	tisp_ldc_t hldcPar;
} tisp_ldc_par_cfg_t;

typedef enum {
    TISP_LDC = 0,
    TISP_HLDC = 1,
    TISP_I2D = 2,
} ldc_mode_type;

typedef enum {
    LDC_I2D_0 = 0,
    LDC_I2D_90,
    LDC_I2D_180,
    LDC_I2D_270,
    LDC_I2D_MIRR,
    LDC_I2D_FLIP,
} ldc_i2d_angle;
typedef struct tisp_i2d_attr {
    ldc_mode_type ldcMode;
    ldc_i2d_angle ldcI2dAngle;
} tisp_ldc_mode_attr_t;
typedef enum {
    ONLINE_LDC  = 0,
    OFFLINE_LDC = 1,
} ldc_type;
typedef struct {
	ldc_type ldcType;
	union{
		struct {
			uint16_t width;
			uint16_t height;
			ldc_mode_type ldcMode;
			uint32_t BlockSize;
			ldc_i2d_angle ldcI2dAngle;
			uint32_t dmaInStride;
			dma_addr_t dmaIn_Y_Paddr;
			dma_addr_t dmaIn_C_Paddr;
			uint32_t dmaOutStride;
			dma_addr_t dmaOut_Y_Paddr;
			dma_addr_t dmaOut_C_Paddr;
			dma_addr_t dmaLutPaddr;
		}offline;
		struct {
			uint16_t width;
			uint16_t height;
			ldc_mode_type ldcMode;
			uint32_t BlockSize;
			ldc_i2d_angle ldcI2dAngle;
			dma_addr_t dmaLutPaddr;
		}online;
	};
} tisp_ldc_param_t;
#endif /* __FAST_LIB_TISP_H__ */
