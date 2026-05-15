#ifndef __TX_ISP_CORE_TUNING_H__
#define __TX_ISP_CORE_TUNING_H__

#include "tx-isp-common.h"
//#include "tx-isp-core.h"
//#include "lib_tisp_api.h"

#define WEIGHT_ZONE_NUM (15*15)

typedef enum isp_core_module_ops_mode {
        ISPCORE_MODULE_DISABLE,
        ISPCORE_MODULE_ENABLE,
        ISPCORE_MODULE_BUTT,
} ISPMODULE_OPS_MODE_E;

typedef enum isp_core_module_ops_type {
        ISPCORE_MODULE_AUTO,
        ISPCORE_MODULE_MANUAL,
} ISPMODULE_OPS_TYPE_E;
/*
 * select which zones are used to gather isp's statistics.
 * the region of interest is defined as rectangle with top-left coordinates(startx, starty)
 * and bottom-right coordinates(endx, endy).
 */

typedef enum imp_isp_tuning_modules_ops_mode {
        IMPISP_TUNING_OPS_MODE_DISABLE,
        IMPISP_TUNING_OPS_MODE_ENABLE,
        IMPISP_TUNING_OPS_MODE_BUTT,
} IMPISP_TUNING_OPS_MODE_E;

/*
 * it is defined as follows that is v4l2-ctrls.
 */

/* isp core tuning */
#define TISP_TUNING_CID_PRIVATE_BASE  0x08000000
enum isp_image_tuning_private_cmd_id {
        IMAGE_TUNING_CID_STATIS_CONFIG = TISP_TUNING_CID_PRIVATE_BASE,
        IMAGE_TUNING_CID_AWB_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10,
        IMAGE_TUNING_CID_AWB_STATIS,
        IMAGE_TUNING_CID_AWB_WEIGHT,
        IMAGE_TUNING_CID_AWB_GLOBAL_STATIS,
        IMAGE_TUNING_CID_FACE_AWB_CONTROL,
        IMAGE_TUNING_CID_AE_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 2,
        IMAGE_TUNING_CID_AE_WEIGHT,
        IMAGE_TUNING_CID_AE_STATIS,
        IMAGE_TUNING_CID_AE_EXPR_INFO,
        IMAGE_TUNING_CID_AE_SCENCE_ATTR,
        IMAGE_TUNING_CID_GAMMA_ATTR,
        IMAGE_TUNING_CID_AE_ANTIFLICKER_ATTR,
        IMAGE_TUNING_CID_AF_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 3,
        IMAGE_TUNING_CID_AF_STATIS,
        IMAGE_TUNING_CID_AF_WEIGHT,
        IMAGE_TUNING_CID_SENSOR_ATTR_CONTROL,
        IMAGE_TUNING_CID_AF_METRIC_INFO,
        IMAGE_TUNING_CID_FACE_AE_CONTROL,
        IMAGE_TUNING_CID_DYNAMIC_DP_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 4,
        IMAGE_TUNING_CID_STATIC_DP_ATTR,
        IMAGE_TUNING_CID_WDR_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 5,
        IMAGE_TUNING_CID_ENABLE_DRC,
        IMAGE_TUNING_CID_ENABLE_DEFOG,
        IMAGE_TUNING_CID_CUSTOM_ANTI_FOG,
        IMAGE_TUNING_CID_WDR_OUTPUT_MODE,
        IMAGE_TUNING_CID_SHARP_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 6,
        IMAGE_TUNING_CID_DEMO_ATTR,
        IMAGE_TUNING_CID_CONTROL_FPS= TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 7,
        IMAGE_TUNING_CID_DAY_OR_NIGHT,
        IMAGE_TUNING_CID_MODULE_CONTROL,
        IMAGE_TUNING_CID_HV_FLIP,
        IMAGE_TUNING_CID_MASK_BLOCK_ATTR,
        IMAGE_TUNING_CID_EV_START,
        IMAGE_TUNING_CID_ISP_CUST_MODE,
        IMAGE_TUNING_CID_AUTOZOOM_CONTROL,
        IMAGE_TUNING_CID_CCM_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 8,
        IMAGE_TUNING_CID_BCSH_HUE,
        IMAGE_TUNING_CID_DIS_STAINFO = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 9,
        IMAGE_TUNING_CID_ISP_WAIT_FRAME_ATTR,
        IMAGE_TUNING_CID_BRIGHTNESS,
        IMAGE_TUNING_CID_SHARPNESS,
        IMAGE_TUNING_CID_SATURATION,
        IMAGE_TUNING_CID_CONTRAST,
        IMAGE_TUNING_CID_CSC_ATTR,

        IMAGE_TUNING_CID_DRAW_BLOCK_ATTR = TISP_TUNING_CID_PRIVATE_BASE + 0x10 * 10,
        IMAGE_TUNING_CID_OSD_ATTR,
        IMAGE_TUNING_CID_OSD_BLOCK_ATTR,
        IMAGE_TUNING_CID_CUSTOM_WDR,
        IMAGE_TUNING_CID_MODULE_RATIO,
        IMAGE_TUNING_CID_SWITCH_BIN,
        IMAGE_TUNING_CID_SCALER_LEVEL,
};
typedef enum tisp_mode_day_and_night {
	TISP_RUNING_MODE_DAY_MODE,
	TISP_RUNING_MODE_NIGHT_MODE,
	TISP_RUNING_MODE_CUSTOM_MODE,
	TISP_RUNING_MODE_BUTT,
} TISP_MODE_DN_E;

typedef enum {
	ISP_FLIP_NORMAL_MODE = 0,
	ISP_FLIP_ISP_H_MODE,
	ISP_FLIP_ISP_V_MODE,
	ISP_FLIP_ISP_HV_MODE,
	ISP_FLIP_MODE_BUTT,
} ISP_CORE_HVFLIP;

typedef struct {
	ISP_CORE_HVFLIP sensor_mode;
	ISP_CORE_HVFLIP isp_mode[3];
} tisp_hv_flip_t;

struct image_tuning_ctrls {

        /* Enable - vertically flip */
        unsigned int vflip;
        /* Enable - horizontally flip */
        unsigned int hflip;

        /* Enable Wide Dynamic Range module */
        unsigned int wdr;

        /* used and inited */
        unsigned char contrast;
        unsigned char saturation;
        unsigned char brightness;
        unsigned char sharpness;
        unsigned char hue;

        /* sensor output fps */
        unsigned int fps;
        ISP_CORE_HVFLIP hvflip;

        /* The mode of isp day and night */
        TISP_MODE_DN_E daynight;
};

/**
 * struct fimc_isp - FIMC-IS ISP data structure
 * @parent: pointer to ISP CORE device
 * @video: the ISP block image tuning device
 * @ctrl: v4l2 controls handler
 * @mlock: mutex serializing video device and the subdev operations
 * @state: driver state flags
 */
typedef struct isp_core_tuning_driver {
        struct tx_isp_subdev *parent;           // which is that the driver belongs to.
        struct image_tuning_ctrls       ctrls[3];

        spinlock_t                      slock;
        struct mutex                    mlock;
        int                     state;
        struct file_operations *fops;
        int (*event)(struct isp_core_tuning_driver *tuning, unsigned int event, void *data);
} image_tuning_vdrv_t;

#define ctrl_to_image_tuning(_ctrl)                                     \
        container_of(ctrl->handler, image_tuning_vdrv_t, ctrls.handler)

image_tuning_vdrv_t *isp_core_tuning_init(struct tx_isp_subdev *parent);
void isp_core_tuning_deinit(image_tuning_vdrv_t *tuning);
void isp_core_tuning_clear(int vinum, image_tuning_vdrv_t *tuning);


#endif //__TX_ISP_CORE_TUNING_H__
