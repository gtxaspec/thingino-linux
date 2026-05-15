#ifndef __FAST_START_COMMON_H__
#define __FAST_START_COMMON_H__

#include <linux/list.h>
#include "tx-isp-common.h"
#include "tx-isp-core-tuning.h"
#include "tx-isp-fast.h"

#define TX_ISP_NRVBS                2
#define TX_ISP_FRAME_BUFFER_IGNORE_NUM 1    // 顺序丢帧
#define FRAME_CHANNEL_DEF_WIDTH 1920
#define FRAME_CHANNEL_DEF_HEIGHT 1080
#define VI_MAX 4
#define FS_CHN_MAX 12
/* char buf[256]; */

#define DEBUG_TTFF(a) ISP_WARNING("TTFF %s:%d %s:%d\n", __func__, __LINE__, (a==NULL? " " : a), *(volatile u32 *)0xb0002078 * 1024 / 24000 )  //TCU3用于快起计时
//#define DEBUG_TIMESTAMP
#define ALIGN_BLOCK_SIZE (1 << 12)
#define ALIGN_ADDR(base) (((base)+((ALIGN_BLOCK_SIZE)-1))&~((ALIGN_BLOCK_SIZE)-1))
#define ALIGN_SIZE(size) (((size)+((ALIGN_BLOCK_SIZE)-1))&~((ALIGN_BLOCK_SIZE)-1))

#define ACCESS_MEMBER_DIRECT(struct_var, member) (struct_var.member)
#define XNAME1(n) sensor##n
#define XNAME2(n) &sensor ## n
#define SENSOR_DRIVER_CHECK(SN)					\
	do{							\
		if(ACCESS_MEMBER_DIRECT(XNAME1(SN),init_sensor) == NULL){				\
			ISP_ERROR("sensor%d not init\n",SN);	\
			return -1;				\
		}						\
	}							\
	while(0)

#define SENSOR_DRIVER_ATTR_INIT(SN)				\
	do{							\
		memcpy(&fast_sensors[SN],			\
		       XNAME2(SN),				\
		       sizeof(struct tx_isp_sensor_fast_attr));	\
	}while(0)

typedef enum {
	TISP_OPS_MODE_DISABLE,                  /**< DISABLE mode of the current module */
	TISP_OPS_MODE_ENABLE,                   /**< ENABLE mode of the current module */
	TISP_OPS_MODE_BUTT,                     /**< effect paramater, parameters have to be less than this value*/
} TISP_OPS_MODE;
/**
 * AE scence mode
 */
typedef enum {
	TISP_AE_SCENCE_AUTO,	     /**< auto mode */
	TISP_AE_SCENCE_DISABLE,	     /**< diable mode */
	TISP_AE_SCENCE_ROI_ENABLE,	     /**< enable mode */
	TISP_AE_SCENCE_GLOBAL_ENABLE,	     /**< enable mode */
	TISP_AE_SCENCE_BUTT,	     /**< effect paramater, parameters have to be less than this value */
} TISPAEScenceMode;

struct AE_start{
	TISPAEScenceMode AeHLCEn;         /**< AE high light depress enable */
	unsigned char AeHLCStrength;        /**< AE high light depress strength (0 ~ 10) */
	TISPAEScenceMode AeBLCEn;         /**< AE back light compensation */
	unsigned char AeBLCStrength;        /**< AE back light compensation strength (0 ~ 10) */
	TISPAEScenceMode AeTargetCompEn;  /**< AE luma target compensation enable */
	uint32_t AeTargetComp;              /**< AE luma target compensation strength（0 ~ 255）*/
	TISPAEScenceMode AeStartEn;       /**< AE start point enable */
	uint32_t AeStartEv;                 /**< AE start ev value */

	uint32_t luma;                                          /**< AE luma value */
	uint32_t luma_scence;                                          /**< AE luma value */
};

/**
 * awb mode
 */
typedef enum {
	ISP_CORE_WB_MODE_AUTO = 0,			/**< auto mode */
	ISP_CORE_WB_MODE_MANUAL,			/**< manual mode */
	ISP_CORE_WB_MODE_DAY_LIGHT,			/**< day light mode */
	ISP_CORE_WB_MODE_CLOUDY,			/**< cloudy mode */
	ISP_CORE_WB_MODE_INCANDESCENT,                  /**< incandescent mode */
	ISP_CORE_WB_MODE_FLOURESCENT,                   /**< flourescent mode */
	ISP_CORE_WB_MODE_TWILIGHT,			/**< twilight mode */
	ISP_CORE_WB_MODE_SHADE,				/**< shade mode */
	ISP_CORE_WB_MODE_WARM_FLOURESCENT,              /**< warm flourescent mode */
	ISP_CORE_WB_MODE_COLORTEND,			/**< Color Trend Mode */
} TISPAWBMode;

/**
 * awb gain
 */
typedef struct {
	uint32_t rgain;     /**< awb r-gain */
	uint32_t bgain;     /**< awb b-gain */
} tisp_awb_gain_t;

/**
 * awb custom mode attribution
 */
typedef struct {
	TISP_OPS_MODE customEn;  /**< awb custom enable */
	tisp_awb_gain_t gainH;           /**< awb gain on high ct */
	tisp_awb_gain_t gainM;           /**< awb gain on medium ct */
	tisp_awb_gain_t gainL;           /**< awb gain on low ct */
	uint32_t ct_node[4];           /**< awb custom mode nodes */
} tisp_awb_custom_attr_t;

struct AWB_start{
	TISPAWBMode mode;                     /**< awb mode */
	tisp_awb_gain_t gain_val;			/**< awb gain on manual mode */
	TISP_OPS_MODE frz;
	unsigned int ct;                        /**< awb current ct value */
	tisp_awb_custom_attr_t custom;         /**< awb custom attribution */
	TISP_OPS_MODE awb_start_en;       /**< awb algo start function enable */
	tisp_awb_gain_t awb_start;                /**< awb algo start point */
};




struct add_sensor {
	const char *name;
	const char *wdr_mode;
	const int h_id;
	const int l_id;
	int (*sensor_init)(void);
	char (*add_sensor_get_name)(void);
	char (*add_sensor_get_i2c_addr)(void);
	struct list_head list;
};
typedef struct {
	int en;
	uint32_t switch_con;
 	uint32_t switch_con_num;
}dual_mode_switch;

struct msensor_mode{
	int sensor_num;// = 2,
	int dual_mode;// = 1,//DUALSENSOR_MODE,
	dual_mode_switch dmode_switch;
	int joint_mode;// = 0,//IMPISP_NOT_JOINT,
};

struct wdr_open_attr{
    uint8_t vinum;
    enum tx_sensor_data_type mode;
};

typedef struct {
	uint8_t vinum;
	enum tx_sensor_data_type mode;
} tx_wdr_open_attr;

struct tx_isp_callback_ops {

	/**
	 * 内核期间启动ISP工作
	 * 返回值：0 成功， 1 失败
	 */
	int (*tx_isp_start_device)(struct tx_isp_module *module);

	/**
	 * 内核高帧率小图模式,切换流程
	 * 返回值：0 成功， 1 失败
	 */
	int (*tx_isp_kernel_hf_resize)(void);

	/**
	 * Riscv高帧率小图模式,切换准备
	 * 返回值：0 成功， 1 失败
	 */
	int (*tx_isp_riscv_hf_prepare)(void);

	/**
	 * Riscv高帧率小图模式,切换流程
	 * 返回值：0 成功， 1 失败
	 */
	int (*tx_isp_riscv_hf_resize)(void);

	/**
	 * 帧完成中断回调函数
	 * count: 帧数
	 * 返回值：0 成功， 1 失败
	 * 说明：该函数为中断函数，切勿作复杂处理
	 */
	int (*tx_isp_frame0_done_int_handler)(int count);
};

extern int zeratul_open;
extern void* setting_mem;
extern unsigned long rmem_base; /* rmem memory base address */
extern unsigned long frame_channel_width[12];
extern unsigned long frame_channel_height[12];
extern unsigned long frame_channel_nrvbs[12];
extern unsigned long direct_mode_save_kernel_ch1;
extern int direct_mode;
extern int ivdc_mem_line;
extern int ivdc_mem_stride;
extern int ivdc_threshold_line;
extern int sensor_number;
extern int use_num_sensor;
extern unsigned int user_wdr_mode;
extern struct tx_isp_sensor_fast_attr fast_sensors[VI_MAX];
extern struct tx_isp_callback_ops g_tx_isp_callback_ops;
extern struct add_sensor  *sensors[8];
extern unsigned long bufferSize0;
extern unsigned long bufferSize1;
extern char isp_buf_len[64 * VI_MAX];
extern char wdr_buf_len[255];
extern char ivdc_buf_len[64];
extern int vpuBs_size;
extern struct tx_isp_module *g_isp_module;
extern uint32_t g_start_sensor_inttime;
extern uint32_t g_start_sensor_again;
extern uint32_t g_start_isp_dgain;
extern uint32_t g_start_isp_ev;
extern uint32_t g_sec_start_isp_ev;
extern uint32_t g_discard_frame;
extern int g_debug_per_frame;
extern int g_debug_frame_count;
extern int g_sensor_expo_switch;
extern struct miscdevice *g_f_mdev[12];
extern struct tx_isp_frame_sources *global_fs;
extern unsigned long Joint_mode;
extern int tisp_enable_tuning(void);
int sensor_setting_init(void);
int add_sensor_chose(void);

long isp_core_tunning_unlocked_ioctl_fast(int vinum, unsigned int get_or_set, unsigned int cmd, unsigned long arg);
long frame_channel_unlocked_ioctl_first(int chn, unsigned int cmd, unsigned long arg);
long tx_isp_unlocked_ioctl_first(unsigned int cmd, unsigned long arg);
long isp_core_tunning_default_ioctl(image_tuning_vdrv_t *tuning, unsigned int cmd, unsigned long arg);
long ivdc_misc_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

//zeratul
int frame_channel_open_fast(uint32_t chn_num);
/* riscv */
void tiziano_fastpara_init(void); /* For Zeratul */
void tiziano_fastpara_exit(void); /* For Zeratul */
int tx_isp_get_tiziano_para(uint32_t vinum);
#endif /* __FAST_START_COMMON_H__  */
