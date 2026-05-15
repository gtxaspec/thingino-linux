#ifndef __FAST_START_DEVICE_H__
#define __FAST_START_DEVICE_H__
typedef struct tisp_awb_start{
	unsigned int rgain;
	unsigned int bgain;
} awb_start_t;

// #define CONFIG_USER_AE
#ifdef CONFIG_USER_AE
typedef enum {
    IMPVI_MAIN = 0,
    IMPVI_SEC = 1,
    IMPVI_THR = 2,
    IMPVI_BUTT,
} IMPVI_NUM;

typedef struct tisp_zone_info{
	unsigned int zone[15][15];
}  __attribute__((packed, aligned(1))) tisp_zone_info_t;

typedef enum {
        TISP_OPS_MODE_DISABLE,                  /**< DISABLE mode of the current module */
        TISP_OPS_MODE_ENABLE,                   /**< ENABLE mode of the current module */
        TISP_OPS_MODE_BUTT,                     /**< effect paramater, parameters have to be less than this value*/
} TISP_OPS_MODE;

typedef enum {
	ISP_CORE_EXPR_UNIT_LINE_t,			/**< The unit is integration line */
	ISP_CORE_EXPR_UNIT_US_t,				/**< The unit is millisecond */
} TISPAEIntegrationTimeUnit;

typedef struct {
	unsigned int start_h;   /**< start pixel in the horizontal */
	unsigned int start_v;   /**< start pixel in the vertical */
	unsigned char node_h;   /**< the statistics node num in the horizontal */
	unsigned char node_v;   /**< the statistics node num in the vertical */
} tisp_statis_location_t;

typedef enum {
	IMP_ISP_HIST_ON_RAW,   /**< Raw Domain */
	IMP_ISP_HIST_ON_YUV,   /**< YUV Domain */
} TISPHistDomain;

typedef struct {
	TISP_OPS_MODE ae_statis_en;
	tisp_statis_location_t local;   /**< AE statistics location */	TISPHistDomain hist_domain;   /**< AE Hist statistics color domain */
	unsigned char histThresh[4];    /**< AE Hist Thresh */
} tisp_ae_statis_attr_t;

struct isp_core_sensor_attr{
	unsigned int hts;/* sensor hts */
	unsigned int vts;/* sensor vts */
	unsigned int fps;/* sensor fps: */
	unsigned int width;/* sensor width*/
	unsigned int height;/* sensor height*/
};

typedef enum {
	IMPISP_AE_NOTIFY_FPS_CHANGE,
} tisp_ae_algo_notify_t;

typedef struct {
	TISPAEIntegrationTimeUnit AeIntegrationTimeUnit;  /**< AE曝光时间单位 */
	uint32_t AeIntegrationTime;                         /**< AE手动模式下的曝光值 */
	uint32_t AeAGain;                                   /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
	uint32_t AeDGain;                                   /**< AE Sensor数字增益值，单位是倍数 x 1024 */
	uint32_t AeIspDGain;                                /**< AE ISP 数字增益值，单位倍数 x 1024*/

	uint32_t AeMinIntegrationTime;                      /**< AE最小曝光时间 */
	uint32_t AeMinAGain;                                /**< AE最小sensor模拟增益 */
	uint32_t AeMinDgain;                                /**< AE最小sensor数字增益 */
	uint32_t AeMinIspDGain;                             /**< AE最小ISP数字增益 */
	uint32_t AeMaxIntegrationTime;                      /**< AE最大曝光时间 */
	uint32_t AeMaxAGain;                                /**< AE最大sensor模拟增益 */
	uint32_t AeMaxDgain;                                /**< AE最大sensor数字增益 */
	uint32_t AeMaxIspDGain;                             /**< AE最大ISP数字增益 */

	/* WDR模式下短帧的AE 手动模式属性*/
	uint32_t AeShortIntegrationTime;                    /**< AE手动模式下的曝光值 */
	uint32_t AeShortAGain;                              /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
	uint32_t AeShortDGain;                              /**< AE Sensor数字增益值，单位是倍数 x 1024 */
	uint32_t AeShortIspDGain;                           /**< AE ISP 数字增益值，单位倍数 x 1024*/

	uint32_t AeShortMinIntegrationTime;                 /**< AE最小曝光时间 */
	uint32_t AeShortMinAGain;                           /**< AE最小sensor模拟增益 */
	uint32_t AeShortMinDgain;                           /**< AE最小sensor数字增益 */
	uint32_t AeShortMinIspDGain;                        /**< AE最小ISP数字增益 */
	uint32_t AeShortMaxIntegrationTime;                 /**< AE最大曝光时间 */
	uint32_t AeShortMaxAGain;                           /**< AE最大sensor模拟增益 */
	uint32_t AeShortMaxDgain;                           /**< AE最大sensor数字增益 */
	uint32_t AeShortMaxIspDGain;                        /**< AE最大ISP数字增益 */
	uint32_t fps;       /**< sensor 帧率 */
	tisp_ae_statis_attr_t AeStatis;
} tisp_ae_algo_init_t;

typedef struct {	unsigned short ae_hist_5bin[5];     /**< AE hist bin value [0 ~ 65535] */
	uint32_t ae_hist_256bin[256];       /**< AE hist bin value, is the true value of pixels num each bin */
	tisp_zone_info_t ae_statis;         /**< AE statistics info */
}  __attribute__((packed, aligned(1))) tisp_ae_statis_info_t;

typedef struct {
	tisp_ae_statis_info_t ae_info;
	TISPAEIntegrationTimeUnit AeIntegrationTimeUnit;  /**< AE曝光时间单位 */
	uint32_t AeIntegrationTime;                         /**< AE手动模式下的曝光值 */
	uint32_t AeAGain;                                   /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
	uint32_t AeDGain;                                   /**< AE Sensor数字增益值，单位是倍数 x 1024 */
	uint32_t AeIspDGain;                                /**< AE ISP 数字增益值，单位倍数 x 1024*/
	uint32_t AeShortIntegrationTime;                    /**< AE手动模式下的曝光值 */
	uint32_t AeShortAGain;                              /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
	uint32_t AeShortDGain;                              /**< AE Sensor数字增益值，单位是倍数 x 1024 */
	uint32_t AeShortIspDGain;                           /**< AE ISP 数字增益值，单位倍数 x 1024*/

	uint32_t Wdr_mode;
	struct isp_core_sensor_attr sensor_attr;
}  __attribute__((packed, aligned(1))) tisp_ae_algo_info_t;

typedef struct {
	uint32_t change;
	TISPAEIntegrationTimeUnit AeIntegrationTimeUnit;  /**< AE曝光时间单位 */
	uint32_t AeIntegrationTime;                         /**< AE手动模式下的曝光值 */
	uint32_t AeAGain;                                   /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
	uint32_t AeDGain;                                   /**< AE Sensor数字增益值，单位是倍数 x 1024 */

	uint32_t AeIspDGain;                                /**< AE ISP 数字增益值，单位倍数 x 1024*/
	uint32_t AeShortIntegrationTime;                    /**< AE手动模式下的曝光值 */
	uint32_t AeShortAGain;                              /**< AE Sensor 模拟增益值，单位是倍数 x 1024 */
	uint32_t AeShortDGain;                              /**< AE Sensor数字增益值，单位是倍数 x 1024 */
	uint32_t AeShortIspDGain;                           /**< AE ISP 数字增益值，单位倍数 x 1024*/

	uint32_t luma;                         /**< AE Luma值 */
	uint32_t luma_scence;                  /**< AE 场景Luma值 */
} tisp_ae_algo_attr_t;


typedef struct {
	void *priv_data;
	int (*open)(void *priv_data, tisp_ae_algo_init_t AeInitAttr);
	void (*close)(void *priv_data);
	void (*handle)(void *priv_data, const tisp_ae_algo_info_t *AeInfo, tisp_ae_algo_attr_t *AeAttr);
	int (*notify)(void *priv_data, tisp_ae_algo_notify_t notify, void* data);
} tisp_ae_algo_func_t;
#endif


#ifdef CONFIG_USER_AE
int IMP_ISP_SetAeAlgoFunc(IMPVI_NUM num, tisp_ae_algo_func_t *ae_func);
int IMP_ISP_SetAeAlgoFunc_internal(struct tx_isp_module *module, IMPVI_NUM num, tisp_ae_algo_func_t *ae_func);
int IMP_ISP_SetAeAlgoFunc_close(struct tx_isp_module *module,IMPVI_NUM num);
#endif
/**
 * @fn int isp_core_tuning_switch_day_or_night(int day, int vinum);
 *
 * 设置ISP的白天夜视模式
 *
 * @param[in] day  1：白天模式  0：夜晚模式
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 在使用这个函数之前，必须保证ISP效果调试功能已使能.
 */
int isp_core_tuning_switch_day_or_night(int day, int vinum);

/**
 * @fn int tx_isp_get_ev_start(int vinum, int *value);
 *
 * 获取ISP的EV信息，返回 riscv 收敛后的最终结果
 *
 * @param[out] value riscv收敛后的ev参数结果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数可在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_get_ev_start(int vinum, int *value);

/**
 * @fn int tx_isp_set_ev_start(int vinum, int enable, int value);
 *
 * 设置ISP的起始EV信息
 *
 * @param[in] value 起始ev
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数建议在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_set_ev_start(int vinum, int enable, int value);

/**
 * @fn int tx_isp_get_ae_luma(uint32_t *luma);
 *
 * 获取图片亮度信息，返回 riscv 收敛后的最终结果
 *
 * @param[out] luma riscv收敛后的亮度结果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数可在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_get_ae_luma(uint32_t *luma);

/**
 * @fn int tx_isp_get_sensor_again(uint32_t *value);
 *
 * 获取sensor的again信息，返回 riscv 收敛后的最终结果
 *
 * @param[out] value riscv收敛后的最终结果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数可在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_get_sensor_again(uint32_t *value);

/**
 * @fn int tx_isp_get_sensor_again(uint32_t *value);
 *
 * 设置 ISP 起始的sensor again参数
 *
 * @param[in] value  sensor again 参数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数建议在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_set_sensor_again(uint32_t value);

/**
 * @fn int tx_isp_get_sensor_inttime(uint32_t *value);
 *
 * 获取sensor的曝光信息，返回 riscv 收敛后的最终结果
 *
 * @param[out] value riscv收敛后的最终结果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数可在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_get_sensor_inttime(uint32_t *value);

/**
 * @fn int tx_isp_set_sensor_inttime(uint32_t value);
 *
 * 设置 ISP 起始的sensor 曝光参数
 *
 * @param[in] value  sensor inttime 参数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数建议在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_set_sensor_inttime(uint32_t value);

/**
 * @fn int tx_isp_get_isp_gain(uint32_t *value);
 *
 * 获取ISP的dgain信息，返回 riscv 收敛后的最终结果
 *
 * @param[out] value riscv收敛后的最终结果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数可在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_get_isp_gain(uint32_t *value);

/**
 * @fn int tx_isp_set_isp_gain(uint32_t value);
 *
 * 设置 ISP 起始dgain 参数
 *
 * @param[in] value  isp dgain 参数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数建议在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_set_isp_gain(uint32_t value);

/**
 * @fn int tx_isp_get_awb_start(awb_start_t *awb_start);
 *
 * 获取AWB信息，返回 riscv 收敛后的最终结果
 *
 * @param[out] awb_start riscv收敛后的最终结果
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 该函数可在tx_isp_riscv_hf_prepare函数中调用
 */
int tx_isp_get_awb_start(awb_start_t *awb_start);

/**
 * @fn int tx_isp_set_awb_start(awb_start_t awb_start);
 *
 * 设置AWB信息
 *
 * @param[in] awb_start awb 结构体信息
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention 在使用这个函数之前，必须保证ISP效果调试功能已使能.
 */
int tx_isp_set_awb_start(awb_start_t awb_start);


/**
 * @fn int32_t lib_tisp_api_get_3a_info(uint8_t vinum, uint32_t frameCnt);
 *
 * 是否开启debug模式
 *
 * @param [in] vinum 0:主摄  1：副摄
 * @param [in] en 0:关闭debug  1：开启debug
 * @param [in] count :需要打印的帧数
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 开启debug模式后，每帧的AE、AWB信息将会通过dmesg打印出来，方便调试
 *
 * @attention 该函数在sensor出流前调用
 */
int lib_tisp_api_get_3a_info(uint8_t vinum, uint32_t frameCnt);

/**
 * @fn int tx_isp_discard_frame(uint32_t frame);
 *
 * 内核丢帧接口
 *
 * @param [frame] :需要丢弃的帧格式
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 最大可控帧数为16帧，支持任意丢帧，frame 填充方式，对应 bit 为 1 则不丢弃，对应bit为0，则丢弃。如 丢弃前面两帧 frame = 0xfffffffc, 如果想丢弃 第一帧和第三帧，frame = 0xfffffffa
 *
 * @attention 该函数建议在tx_isp_riscv_hf_prepare函数中调用或在stream on之前调用即可
 */
int tx_isp_discard_frame(uint32_t frame);
int sec_tx_isp_discard_frame(uint32_t frame);

/**
 * @fn int tx_isp_get_riscv_night_mode(void);
 *
 * 获取小核白天夜视模式
 *
 * @retval 0 白天 1 夜视
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention
 */
int tx_isp_get_riscv_night_mode(void);

/**
 * @fn int tx_isp_disable_sensor_expo(void);
 *
 * 关闭sensor曝光参数设置，ISP统计结果不会通过I2C设置给Sensor
 *
 * @retval 非0 失败，返回错误码
 *
 * @remark
 *
 * @attention
 */
int tx_isp_disable_sensor_expo(void);

/**
 * @fn int tx_isp_enable_sensor_expo(void);
 *
 * 开启sensor曝光参数设置，ISP统计结果通过I2C设置给Sensor
 *
 * @retval 非0 失败，返回错误码
 *
 * @remark 此类接口不调用默认值是开启状态
 *
 * @attention
 */
int tx_isp_enable_sensor_expo(void);
/**
 * @fn int tx_isp_get_riscv_framecount(void);
 *
 * 获取小核riscv跑的帧数
 *
 * @retval
 *
 * @remark 返回值无意义，结果通过出参value传出
 *
 * @attention
 */
int tx_isp_get_riscv_framecount(uint32_t *value); /* For Zeratul */
/**
 * @fn int tx_isp_riscv_is_run(void);
 *
 * 判断小核riscv是否运行
 *
 * @retval 非0表示没有运行，0表示运行
 *
 * @remark
 *
 * @attention
 */
int tx_isp_riscv_is_run(void); /* For Zeratul */
/**
 * @fn int tx_isp_stop_riscv(void);
 *
 * 停止riscv
 *
 * @retval 无意义
 *
 * @remark 调用一次即可，无需判断返回值
 *
 * @attention
 */
int tx_isp_stop_riscv(void);
/**
 * @fn int tx_isp_enable_sensor_expo(void);
 *
 * 判断riscv是否停止
 *
 * @retval 1 已停止，0未停止
 *
 * @remark
 *
 * @attention
 */
int tx_isp_riscv_is_stop(void);

#endif/*__FAST_START_DEVICE_H__*/
