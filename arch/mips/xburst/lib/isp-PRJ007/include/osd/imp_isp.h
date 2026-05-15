/*
 * IMP ISP header file.
 *
 * Copyright (C) 2022 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __IMP_ISP_H__
#define __IMP_ISP_H__

//#include <stdbool.h>
//#include "imp_common.h"
#include "../tx-isp-device.h"
#include "../tx-isp-common.h"
#include "../tx-isp-core-tuning.h"
#include "../fast_start_common.h"

#define OSD_LETTER_NUM 20
#define OSD_REGION_WIDTH        16
#define OSD_REGION_HEIGHT       34

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @file
 * The header file of ISP
 */

/**
 * fill data value of Mask parameters
 */
typedef struct color_value {
	struct {
		unsigned char r_value;	/**< R offset of RGB type */
		unsigned char g_value;	/**< G offset of RGB type */
		unsigned char b_value;	/**< B offset of RGB type */
	} argb;					/**< RGB type */
	struct {
		unsigned char y_value;	/**< Y offset of YUV type */
		unsigned char u_value;	/**< U offset of YUV type */
		unsigned char v_value;	/**< V offset of YUV type */
	} ayuv;					/**< YUV type */
} IMP_ISP_COLOR_VALUE;

/**
 * Draw range attribution
 */
typedef struct {
	uint8_t  enable;			  /**< draw range enable */
	uint16_t left;				  /**< range start pixel in horizental */
	uint16_t top;				  /**< range start pixel in vertical */
	uint16_t width;				  /**< range width */
	uint16_t height;			  /**< range height */
	IMP_ISP_COLOR_VALUE color;	  /**< range color */
	uint8_t  line_width;		  /**< range line width */
	uint8_t  alpha;				  /**< range alpha(3bit) */
	uint16_t extend;			  /**< range extend */
	uint8_t  msk_en;		   /**< mosaic enable */
} IMPISPDrawRangAttr;

/**
 * Draw line attribution
 */
typedef struct {
	uint8_t  enable;		   /**< draw line enable */
	uint16_t startx;		   /**< line start pixel in horizental */
	uint16_t starty;		   /**< line start pixel in vertical */
	uint16_t endx;			   /**< line width */
	uint16_t endy;			   /**< line height */
	IMP_ISP_COLOR_VALUE color; /**< line color */
	uint8_t  width;			   /**< line line width */
	uint8_t  alpha;			   /**< line alpha */
} IMPISPDrawLineAttr;

/**
 * Draw window attribution
 */
typedef struct {
	uint8_t  enable;		   /**< draw window enable */
	uint16_t left;			   /**< window start pixel in horizental */
	uint16_t top;			   /**< window start pixel in vertical */
	uint16_t width;			   /**< window width */
	uint16_t height;		   /**< window height */
	IMP_ISP_COLOR_VALUE color; /**< window color */
	uint8_t  line_width;		   /**< window line width */
	uint8_t  alpha;			   /**< window alpha */
	uint8_t  msk_en;		   /**< mosaic enable */
}IMPISPDrawWindAttr;

/**
 * fill data type of Mask parameters
 */
typedef enum {
	IMPISP_MASK_TYPE_RGB = 0, /**< RGB type */
	IMPISP_MASK_TYPE_YUV = 1, /**< YUV type */
} IMPISP_MASK_TYPE;


/**
 * Draw type
 */
typedef enum {
	IMP_ISP_DRAW_WIND,		/**< Draw window */
	IMP_ISP_DRAW_RANGE,		/**< Draw range */
	IMP_ISP_DRAW_LINE,		/**< Draw line */
} IMPISPDrawType;

/**
 * Draw unit Attribution
 */
typedef struct {
	uint8_t pinum;					   /**< Block NUMBER (range: 0 to 19) */
	uint8_t msk_size;				   /**< msk size(range：8～64) */
	IMPISPDrawType type;			   /**< draw type */
	IMPISP_MASK_TYPE color_type;		/**< mask type */
	union {
		IMPISPDrawWindAttr wind;   /**< draw window attr */
		IMPISPDrawRangAttr rang;   /**< draw range attr */
		IMPISPDrawLineAttr line;   /**< draw line attr */
	} cfg;						  /**< draw attr */
} IMPISPDrawBlockAttr;

/**
 * OSD picture atribution
 */
typedef struct {
	uint8_t pinum;				   /**< Block NUMBER (range: 0 to 7) */
	uint8_t  osd_enable;		   /**< osd enable */
	uint16_t osd_left;			   /**< osd area x value */
	uint16_t osd_top;			   /**< osd area y value */
	uint16_t osd_width;			   /**< osd area width */
	uint16_t osd_height;		   /**< osd area height */
	char *osd_image;			   /**< osd picture(file path/array) */
	uint16_t osd_stride;		   /**< osd stride, In bytes, for example, 320x240 RGBA8888 osd_stride=320*4. */
} IMPISPOSDBlockAttr;

/**
 * Mask parameters of each channel
 */
typedef struct isp_mask_block_par {
	uint8_t chx;			  /**< Channel Number (range: 0 to 2) */
	uint8_t pinum;			  /**< Block NUMBER (range: 0 to 3) */
	uint8_t mask_en;		  /**< mask enable */
	uint16_t mask_pos_top;	  /**< y of mask position */
	uint16_t mask_pos_left;   /**< x of mask position  */
	uint16_t mask_width;	  /**< mask block width */
	uint16_t mask_height;	  /**< mask block height */
	IMPISP_MASK_TYPE mask_type;		/**< mask type */
	IMP_ISP_COLOR_VALUE mask_value;  /**< mask value */
} IMPISPMaskBlockAttr;


/**
 * @defgroup IMP_ISP
 * @ingroup imp
 * @brief Image signal processing unit. It contains several key function, for example, image effects setting, night scene, sensor's operations and so on.
 *
 * ISP module is not related to the data flow, so no need to process Bind, Only used for effect parameters configuration and sensor controls.
 *
 * The ISP manipulation is as follow:
 * @code
 * int32_t ret = 0;
 * ret = IMP_ISP_Open(); // step.1	create ISP module
 * if(ret < 0){
 *	   printf("Failed to ISPInit\n");
 *	   return -1;
 * }
 * IMPSensorInfo sensor;
 * sensor.name = "xxx";
 * sensor.cbus_type = SENSOR_CONTROL_INTERFACE_I2C; // OR SENSOR_CONTROL_INTERFACE_SPI
 * sensor.i2c = {
 *	.type = "xxx", // I2C sets the name, this name has to be the same as the name of the sensor drivers in struct i2c_device_id.
 *	.addr = xx,	// the I2C address
 *	.i2c_adapter_id = xx, // The value is the I2C adapter ID.
 * }
 * OR
 * sensor.spi = {
 *	.modalias = "xx", // SPI sets the name, this name has to be the same as the name of the sensor drivers in struct i2c_device_id.
 *	.bus_num = xx, // It is the address of SPI bus.
 * }
 * ret = IMP_ISP_AddSensor(&sensor); //step.2, add a sensor. Before the function is called, the sensor driver has to be registered into kernel.
 * if (ret < 0) {
 *	   printf("Failed to Register sensor\n");
 *	   return -1;
 * }
 *
 * ret = IMP_ISP_EnableSensor(void); //step.3, Enable sensor and sensor starts to output image.
 * if (ret < 0) {
 *	   printf("Failed to EnableSensor\n");
 *	   return -1;
 * }
 *
 * ret = IMP_ISP_EnableTuning(); //step.4, Enable ISP tuning, then you can use ISP debug interface.
 * if (ret < 0) {
 *	   printf("Failed to EnableTuning\n");
 *	   return -1;
 * }
 *
 * Debug interface, please refer to the ISP debug interface documentation //step.5
 *
 * @endcode
 * The process which uninstall(disable)ISP is as follows:
 * @code
 * int32_t ret = 0;
 * IMPSensorInfo sensor;
 * sensor.name = "xxx";
 * ret = IMP_ISP_DisableTuning(); //step.1 Turn off ISP tuning
 * if (ret < 0) {
 *	   printf("Failed to disable tuning\n");
 *	   return -1;
 * }
 *
 * ret = IMP_ISP_DisableSensor(); //step.2, Turn off sensor, Note that sensor will stop output pictures, so that all FrameSource should be closed.
 * if (ret < 0) {
 *	   printf("Failed to disable sensor\n");
 *	   return -1;
 * }
 *
 * ret = IMP_ISP_DelSensor(&sensor); //step.3, Delete sensor, before that step, the sensor has to be stopped.
 * if (ret < 0) {
 *	   printf("Failed to disable sensor\n");
 *	   return -1;
 * }
 *
 * ret = IMP_ISP_Close(); //step.4, After deleting all sensors, you can run this interface to clean up the ISP module.
 * if (ret < 0) {
 *	   printf("Failed to disable sensor\n");
 *	   return -1;
 * }
 * @endcode
 * There are more examples in the samples.
 * @{
 */


/**
 * ISP Module Toggle
 */
typedef enum {
	IMPISP_OPS_MODE_DISABLE,		/**< DISABLE mode of the current module */
	IMPISP_OPS_MODE_ENABLE,		/**< ENABLE mode of the current module */
	IMPISP_OPS_MODE_BUTT,			/**< effect paramater, parameters have to be less than this value */
} IMPISPOpsMode;

typedef IMPISP_TUNING_OPS_MODE_E IMPISPTuningOpsMode;
#if 0
/**
 * ISP Function Toggle
 */
typedef enum {
	IMPISP_TUNING_OPS_MODE_DISABLE,		/**< DISABLE mode of the current module */
	IMPISP_TUNING_OPS_MODE_ENABLE,			/**< ENABLE mode of the current module */
	IMPISP_TUNING_OPS_MODE_BUTT,			/**< effect paramater, parameters have to be less than this value*/
} IMPISPTuningOpsMode;
#endif
/**
 * ISP Function Mode
 */
typedef enum {
	IMPISP_TUNING_OPS_TYPE_AUTO,			/**< AUTO mode of the current module*/
	IMPISP_TUNING_OPS_TYPE_MANUAL,			/**< MANUAL mode of the current module*/
	IMPISP_TUNING_OPS_TYPE_BUTT,			/**< effect paramater, parameters have to be less than this value*/
} IMPISPTuningOpsType;

/**
* Sensor Number Label
*/
typedef enum {
	IMPVI_MAIN = 0,		  /**< Main Sensor */
	IMPVI_SEC = 1,		  /**< Second Sensor */
	IMPVI_THR = 2,		  /**< Third Sensor */
	IMPVI_FOUR = 3,		  /**< Fourth Sensor */
	IMPVI_BUTT,			  /**< effect paramater, parameters have to be less than this value */
} IMPVI_NUM;

/**
* The enum is types of sensor control bus.
*/
typedef enum tx_sensor_control_bus_type IMPSensorControlBusType;

/**
* Defines I2C bus information
*/
typedef struct {
	char type[20];		/**< Set the name, the value must be match with sensor name in 'struct i2c_device_id' */
	int32_t addr;		/**< the I2C address */
	int32_t i2c_adapter_id;	/**< I2C adapter ID */
} IMPI2CInfo;

/**
* Defines SPI bus information
*/
typedef struct {
	char modalias[32];				/**< Set the name, the value must be match with sensor name in 'struct i2c_device_id' */
	int32_t bus_num;		/**< Address of SPI bus */
} IMPSPIInfo;

/**
* Defines Sensor data input interface
*/
typedef enum {
	IMPISP_SENSOR_VI_MIPI_CSI0 = 0,    /**< The MIPI CSI0 interface */
	IMPISP_SENSOR_VI_MIPI_CSI1 = 1,/**< The MIPI CSI1 interface */
	IMPISP_SENSOR_VI_DVP = 2,		   /**< The DVP interface */
	IMPISP_SENSOR_VI_BUTT = 3,		   /**< effect paramater, parameters have to be less than this value */
} IMPSensorVinType;

/**
* Defines Sensor mclk clock source
*/
typedef enum {
	IMPISP_SENSOR_MCLK0 = 0,	   /**< MCLK0 */
	IMPISP_SENSOR_MCLK1 = 1,	   /**< MCLK1 */
	IMPISP_SENSOR_MCLK2 = 2,	   /**< MCLK2 */
	IMPISP_SENSOR_MCLK_BUTT = 3,   /**< effect paramater, parameters have to be less than this value*/
} IMPSensorMclk;

/**
 * Sensor frame rate
 */
typedef struct {
	uint32_t num;  /**< The molecular parameter of the frame rate */
	uint32_t den;  /**< The denominator parameter for the frame rate */
} IMPISPSensorFps;

/**
* Defines the information of sensor
*/
typedef struct {
	char name[32];						/**< the sensor name */
	IMPSensorControlBusType cbus_type;	/**< the sensor control bus type */
	union {
		IMPI2CInfo i2c;					/**< I2C bus information */
		IMPSPIInfo spi;					/**< SPI bus information */
	};
	int rst_gpio;						/**< The reset pin of sensor. */
	int pwdn_gpio;						/**< The power down pin of sensor. */
	int power_gpio;						/**< The power pin of sensor, but it is invalid now. */
	int switch_gpio;					/**< GPIO of mipi_switch in multi-camera case */
	int switch_gpio_state;				/**< GPIO status of the current Sensor mipi_switch in multi-camera mode */
	unsigned short sensor_id;			/**< The Sensor ID */
	IMPSensorVinType video_interface;	/**< The Sensor data input interface */
	IMPSensorMclk mclk;					/**< The Sensor input mclk clock source */
	int default_boot;					/**< The Sensor default boot setting */
	IMPISPSensorFps fps;				/**< Initial camera frame rate (reserved) */
} IMPSensorInfo;

/**
 * @fn int32_t IMP_ISP_Open(void)
 *
 * Open the ISP module
 *
 * @param none
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark After calling the function,it first creates the ISP module, then prepares to add sensor to ISP, and starts the ISP effect debugging function.
 *
 * @attention Before adding sensor image, this function must be called firstly.
 * @attention Called after IMP_System_Init.
 */
int32_t IMP_ISP_Open(void);

/**
 * @fn int32_t IMP_ISP_Close(void)
 *
 * Close the ISP module
 *
 * @param none
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark After calling the function, ISP will stop working.
 *
 * @attention Before calling this function, make sure that all FrameSources and effect debugging functions are off(disabled), and all sensors are deleted.
 */
int32_t IMP_ISP_Close(void);

/**
 * @fn int32_t IMP_ISP_AddSensor(IMPVI_NUM num, IMPSensorInfo *pinfo)
 *
 * Add a sensor into ISP module.
 *
 * @param[in] num	The sensor num label you want to add.
 * @param[in] pinfo The pointer for the sensor information.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The sensor will be used to capture image.
 *
 * @attention Before using this function, you must ensure that the camera driver has been registered into the kernel.
 * @attention Must be AddSensor one by one(0-1-2), after add all sensor, then you can call other apis.
*/
int32_t IMP_ISP_AddSensor(IMPVI_NUM num, IMPSensorInfo *pinfo);

/**
 * @fn int32_t IMP_ISP_DelSensor(IMPVI_NUM num, IMPSensorInfo *pinfo)
 *
 * Delete a sensor from ISP module.
 *
 * @param[in] num	The sensor num label you want to delete.
 * @param[in] pinfo The pointer for the sensor information
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Delete a sensor(image sensor which would be a camera)
 *
 * @attention Before using this function, you must ensure that the sensor has been stopped working, use IMP_ISP_DisableSensor function to do so.
 */
int32_t IMP_ISP_DelSensor(IMPVI_NUM num, IMPSensorInfo *pinfo);

/**
 * @fn int32_t IMP_ISP_EnableSensor(IMPVI_NUM num, IMPSensorInfo *pinfo)
 *
 * Enable a sensor from ISP module.
 *
 * @param[in] num	The sensor num label you want to enable.
 * @param[in] pinfo The pointer for the sensor information
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Enable a sensor(image sensor which would be a camera)
 *
 * @attention Before using this function, you must ensure that the sensor has been added, use IMP_ISP_AddSensor function to do so.
 */
int32_t IMP_ISP_EnableSensor(IMPVI_NUM num, IMPSensorInfo *info);

/**
 * @fn int32_t IMP_ISP_DisableSensor(IMPVI_NUM num)
 *
 * Disable the running sensor.
 *
 * @param[in] num	 the sensor num label.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark	if a sensor is not used, FrameSource and ISP won't be working either.
 *
 * @attention Before using this function, you must ensure that the Framesource and ISP have stopped working.
 */
int32_t IMP_ISP_DisableSensor(IMPVI_NUM num);

/**
* the sensor num you want to add on isp
*/
typedef enum {
	IMPISP_TOTAL_ONE = 1,						/**< add up to 1 sensor on isp */
	IMPISP_TOTAL_TWO,							/**< add up to 2 sensor on isp */
	IMPISP_TOTAL_THR,							/**< add up to 3 sensor on isp */
	IMPISP_TOTAL_FOU,							/**< add up to 4 sensor on isp */
	IMPISP_TOTAL_BUTT,							/**< effect paramater, parameters have to be less than this value */
} IMPISPSensorNum;

/**
* Multi Camera Cache mode struct
*/
typedef enum {
	IMPISP_INPUT_DIRECT_MODE,
	IMPISP_INPUT_CACHED_MODE,
	IMPISP_INPUT_BUTT,
} IMPISPMultiInputMode;

/**
 * Multi camera cache configuration
 */
typedef struct {
	IMPISPMultiInputMode mode;			/**< cache mode */
	uint32_t cache_line;				/**< Number of cache lines (the size of each line is the maximum width in the multicamera) */
} IMPISPMultiInput;

/**
* Multi camera joint mode
*/
typedef enum {
	IMPISP_NOT_JOINT = 0,           /**< disable joint mode when enable dual sensor mode */

	IMPISP_HORIZONTAL_ONLY_JOINT,	/**< Horizontal splicing only */
	IMPISP_VERTICAL_ONLY_JOINT,		/**< Vertical splicing only */

	IMPISP_MAIN_JOINT_BUTT,         /**< effect paramater, parameters have to be less than this value */
} IMPISPMultiSplitJointMode;

/**
 * Multi camera joint position
 */
typedef enum {
	IMPISP_JOINT_MODE_POSITION_0,
	IMPISP_JOINT_MODE_POSITION_1,
	IMPISP_JOINT_MODE_POSITION_2,
	IMPISP_JOINT_MODE_POSITION_3,
	IMPISP_JOINT_MODE_POSITION_BUTT,	/**< effect paramater, parameters have to be less than this value */
} IMPISPMultiSplitJointPosition;

/**
* Multi camera struct
*/
typedef struct {
	IMPISPMultiSplitJointMode mode;
	IMPISPMultiSplitJointPosition pos[IMPISP_TOTAL_BUTT];
} IMPISPMultiSplitJoint;

/**
* Multi Camera system parameters struct
*/
typedef struct	{
	IMPISPSensorNum sensor_num;
	IMPISPMultiInput input;
	IMPISPMultiSplitJoint joint;
} IMPISPCameraInputMode;

/**
 * @fn int32_t IMP_ISP_SetCameraInputMode(IMPISPCameraInputMode *mode)
 *
 * Set the parameters of multi camera system.
 *
 * @param[in] mode	The mode of camera input.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Set the parameters of multi camera system.
 *
 * @attention Have to call this function before IMP_ISP_AddSensor.
 */
int32_t IMP_ISP_SetCameraInputMode(IMPISPCameraInputMode *mode);

/**
 * @fn int32_t IMP_ISP_GetCameraInputMode(IMPISPCameraInputMode *mode)
 *
 * Get the parameters of multi camera system.
 *
 * @param[in] mode	The mode of camera input.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Get the parameters of multi camera system.
 *
 * @attention Have to call this function after IMP_ISP_SetCameraInputMode.
 */
int32_t IMP_ISP_GetCameraInputMode(IMPISPCameraInputMode *mode);

/**
 * @fn int32_t IMP_ISP_EnableTuning(void)
 *
 * Enable effect debugging of ISP
 *
 * @param[in] void	  none.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using this function, you must ensure that 'IMP_ISP_EnableSensor' is working.
 */
int32_t IMP_ISP_EnableTuning(void);

/**
 * @fn int32_t IMP_ISP_DisableTuning(void)
 *
 * Disable effect debugging of ISP
 *
 * @param void	  none
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention First you must ensure that ISP is no longer working, then stop the sensor, after that you can use this function.
 */
int32_t IMP_ISP_DisableTuning(void);

/**
 * Effect bin file mode
 */
typedef enum {
	IMPISP_BIN_MODE_DAY,
	IMPISP_BIN_MODE_NIGHT,
	IMPISP_BIN_MODE_DAY_WDR,
	IMPISP_BIN_MODE_NIGHT_WDR,
	IMPISP_BIN_MODE_BUTT,
} IMPISPBinMode;

/**
 * Effect bin file path
 */
typedef struct {
    IMPISPBinMode mode;				/**< Switch bin mode select */
	char bname[128];				/**< The absolute path to the bin file */
} IMPISPDefaultBinAttr;

/**
 * @fn int32_t IMP_ISP_SetDefaultBinPath(IMPVI_NUM num, IMPISPDefaultBinAttr *attr)
 *
 * Sets the default path to the ISP bin file.
 *
 * @param[in] num	The sensor num label you want to delete.
 * @param[in] attr  The bin attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Sets the absolute path to the Bin file when the user-defined ISP is started.
 *
 * @code
 * int ret = 0;
 * IMPISPDefaultBinAttr attr;
 *
 * attr.mode = IMPISP_BIN_MODE_DAY;
 * strcpy(attr.bname, "/etc/sensor/xxx-PRJ007.bin");
 *
 * ret = IMP_ISP_SetDefaultBinPath(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_SetDefaultBinPath error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Using range: IMP_ISP_AddSensor before.
 */
int32_t IMP_ISP_SetDefaultBinPath(IMPVI_NUM num, IMPISPDefaultBinAttr *attr);

/**
 * @fn int32_t IMP_ISP_GetDefaultBinPath(IMPVI_NUM num, IMPISPDefaultBinAttr *attr)
 *
 * Gets the default path to the ISP bin file.
 *
 * @param[in] num	The sensor num label you want to delete.
 * @param[out] attr The bin attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Get the absolute path to the Bin file when the user-defined ISP is started.
 *
 * @code
 * int ret = 0;
 * IMPISPDefaultBinAttr attr;
 *
 * attr.mode = IMPISP_BIN_MODE_DAY;
 * ret = IMP_ISP_GetDefaultBinPath(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_GetDefaultBinPath error !\n");
 *	return -1;
 * }
 * printf("default bin path:%s\n", attr.path);
 * @endcode
 *
 * @attention This function must be called after the sensor is added.
 * @attention Only bin file path attributes for a single ISP can be retrieved at a time.
 */
int32_t IMP_ISP_GetDefaultBinPath(IMPVI_NUM num, IMPISPDefaultBinAttr *attr);

/**
 * @fn int32_t IMP_ISP_SetISPBypass(IMPVI_NUM num, IMPISPTuningOpsMode *enable)
 *
 * Control ISP modules.
 *
 * @param[in] num		The sensor num label.
 * @param[in] enable	bypass output mode (yes / no)
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPOpsMode enable;
 *
 * enable = IMPISP_OPS_MODE_ENABLE;
 * ret = IMP_ISP_SetISPBypass(IMPVI_MAIN, &enable);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_SetISPBypass error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Must be call this api before AddSensor.
 */
int32_t IMP_ISP_SetISPBypass(IMPVI_NUM num, IMPISPOpsMode *enable);

/**
 * @fn int32_t IMP_ISP_GetISPBypass(IMPVI_NUM num, IMPISPOpsMode *enable)
 *
 * Get ISP modules state.
 *
 * @param[in] num		The sensor num label.
 * @param[out] enable	bypass output mode (yes / no)
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPTuningOpsMode enable;
 *
 * ret = IMP_ISP_GetISPBypass(IMPVI_MAIN, &enable);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_GetISPBypass error !\n");
 *	return -1;
 * }
 * printf("isp bypass:%d\n", enable);
 * @endcode
 *
 * @attention Must be call this api before AddSensor.
 */
int32_t IMP_ISP_GetISPBypass(IMPVI_NUM num, IMPISPOpsMode *enable);

/**
 * @fn int32_t IMP_ISP_WDR_ENABLE(IMPVI_NUM num, IMPISPTuningOpsMode *mode)
 *
 * Toggle ISP WDR Mode.
 *
 * @param[in] num		The sensor num label.
 * @param[out] mode		ISP WDR mode
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPTuningOpsMode mode;
 *
 * mode = IMPISP_TUNING_OPS_MODE_ENABLE;
 * ret = IMP_ISP_WDR_ENABLE(IMPVI_MAIN, &mode);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_WDR_ENABLE error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Have to call this function before IMP_ISP_AddSensor for the first time.
 */
int32_t IMP_ISP_WDR_ENABLE(IMPVI_NUM num, IMPISPTuningOpsMode *mode);

/**
 * @fn int32_t IMP_ISP_WDR_ENABLE_GET(IMPVI_NUM num, IMPISPTuningOpsMode *mode)
 *
 * Get ISP WDR Mode.
 *
 * @param[in] num		The sensor num label.
 * @param[out] mode		ISP WDR mode
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPTuningOpsMode mode;
 *
 * ret = IMP_ISP_WDR_ENABLE_GET(IMPVI_MAIN, &mode);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_WDR_ENABLE_GET error !\n");
 *	return -1;
 * }
 * printf("wdr enable:%d\n", mode);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_WDR_ENABLE_GET(IMPVI_NUM num, IMPISPTuningOpsMode *mode);

/**
 * Sensor register
 */
typedef struct {
		uint32_t addr;	 /**< The address of the register */
		uint32_t value;  /**< The value of the register */
} IMPISPSensorRegister;

/**
 * @fn int32_t IMP_ISP_SetSensorRegister(IMPVI_NUM num, IMPISPSensorRegister *reg)
 *
 * Set the value of a register of a sensor.
 *
 * @param[in] num		The sensor num label.
 * @param[in] reg	The attr of the register.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Value of a register can be directly set.
 *
 * @code
 * int ret = 0;
 * IMPISPSensorRegister reg;
 *
 * reg.addr = 0x00;
 * reg.value = 0x00;
 * ret = IMP_ISP_SetSensorRegister(IMPVI_MAIN, *reg);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_SetSensorRegister error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using this function, you must ensure that the sensor is working, so it will be able to be configured or set.
 */
int32_t IMP_ISP_SetSensorRegister(IMPVI_NUM num, IMPISPSensorRegister *reg);

/**
 * @fn int32_t IMP_ISP_GetSensorRegister(IMPVI_NUM num, IMPISPSensorRegister *reg)
 *
 * Obtain a value of the register of sensor.
 *
 * @param[in] num		The sensor num label.
 * @param[out] reg	The attr of the register.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark You can directly obtain the value of the sensor's register.
 *
 * @code
 * int ret = 0;
 * IMPISPSensorRegister reg;
 *
 * ret = IMP_ISP_GetSensorRegister(IMPVI_MAIN, *reg);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_GetSensorRegister error !\n");
 *	return -1;
 * }
 * printf("addr:0x%x, value:0x%x\n", reg.addr, reg.value);
 * @endcode
 *
 * @attention Before using this function, you must ensure that the sensor is working.
 */
int32_t IMP_ISP_GetSensorRegister(IMPVI_NUM num, IMPISPSensorRegister *reg);

/**
 * Sensor attr
 */
typedef struct {
	uint32_t hts;			/**< sensor hts */
	uint32_t vts;			/**< sensor vts */
	uint32_t fps;			/**< sensor frame rate */
	uint32_t width;			/**< sensor output width */
	uint32_t height;		/**< sensor output height */
} IMPISPSENSORAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_GetSensorAttr(IMPVI_NUM num, IMPISPSENSORAttr *attr)
 *
 * Get Sensor Attr.
 *
 * @param[in] num		The sensor num label.
 * @param[out] attr		Sensor attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPSENSORAttr attr;
 *
 * ret = IMP_ISP_Tuning_GetSensorAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetSensorAttr error !\n");
 *	return -1;
 * }
 * printf("hts:%d, vts:%d, fps:%d/%d, width:%d, height:%d\n",
 * attr.hts, attr.vts, attr.fps >> 16, attr.fps & 0xffff, attr.width, attr.height);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetSensorAttr(IMPVI_NUM num, IMPISPSENSORAttr *attr);

/**
 * @fn int32_t IMP_ISP_Tuning_SetSensorFPS(IMPVI_NUM num, IMPISPSensorFps *fps)
 *
 * Set the FPS of enabled sensor.
 *
 * @param[in] num		The sensor num label.
 * @param[in] fps	The sensor frame rate.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPSensorFps fps;
 *
 * fps.num = 15;
 * fps.den = 1;
 * ret = IMP_ISP_Tuning_SetSensorFPS(IMPVI_MAIN, &fps);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetSensorFPS error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using this function, make sure that 'IMP_ISP_EnableSensor' and 'IMP_ISP_EnableTuning' are working properly.
 */
int32_t IMP_ISP_Tuning_SetSensorFPS(IMPVI_NUM num, IMPISPSensorFps *fps);

/**
 * @fn int32_t IMP_ISP_Tuning_GetSensorFPS(IMPVI_NUM num, IMPISPSensorFps *fps)
 *
 * Get the FPS of enabled sensor.
 *
 * @param[in] num		The sensor num label.
 * @param[out] fps	The sensor frame rate.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPSensorFps fps;
 *
 * ret = IMP_ISP_Tuning_GetSensorFPS(IMPVI_MAIN, &fps);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetSensorFPS error !\n");
 *	return -1;
 * }
 * printf("fps:%f\n", (float)fps.num/fps.den);
 * @endcode
 *
 * @attention Before using this function, make sure that 'IMP_ISP_EnableSensor' and 'IMP_ISP_EnableTuning' are working properly.
 * @attention Before starting data transmission in a Channel, you must first call this function in order to obtain the sensor's default FPS.
 */
int32_t IMP_ISP_Tuning_GetSensorFPS(IMPVI_NUM num, IMPISPSensorFps *fps);

/**
 * @fn int32_t IMP_ISP_Tuning_SetVideoDrop(void (*cb)(void))
 *
 * Set the video loss function. When there is a problem with the connection line of the sensor board, the callback function will be executed.
 *
 * @param[in] cb	The pointer for callback function.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetVideoDrop(void (*cb)(void));

/**
 * ISP Wait Frame irq parameters
 */

typedef enum {
	IMPISP_IRQ_FD = 0,	/**< frame start */
	IMPISP_IRQ_FS = 1,	/**< frame end */
} IMPISPIrqType;

/**
 * ISP Wait Frame Parameters
 */
typedef struct {
	uint32_t timeout;		/**< timeout,unit is ms */
	uint64_t cnt;			/**< Frame count */
	IMPISPIrqType irqtype;			/**< Frame irq type*/
} IMPISPWaitFrameAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_WaitFrameDone(IMPVI_NUM num, IMPISPWaitFrameAttr *attr)
 *
 * Wait frame done
 *
 * @param[in]  num		The sensor num label.
 * @param[in]  attr		frame done parameters
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPWaitFrameAttr attr;
 *
 * attr.timeout = 100;
 * ret = IMP_ISP_Tuning_WaitFrameDone(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_WaitFrameDone error !\n");
 *	return -1;
 * }
 * printf("this total number of frames: %d, irq type:%s\n",
 * attr->cnt, attr->irqtype ? "frame start" : "frame done");
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_WaitFrameDone(IMPVI_NUM num, IMPISPWaitFrameAttr *attr);

/**
 * ISP Anti-flicker mode structure.
 */
typedef enum {
	IMPISP_ANTIFLICKER_DISABLE_MODE,		/**< Disable ISP antiflicker */
	IMPISP_ANTIFLICKER_NORMAL_MODE,			/**< Enable ISP antiflicker normal mode, and the min integration can not reach the sensor min integration time */
	IMPISP_ANTIFLICKER_AUTO_MODE,			/**< Enable ISP antiflicker auto mode, and the min integration can reach the sensor min integration time */
	IMPISP_ANTIFLICKER_BUTT,				/**< effect paramater, parameters have to be less than this value */
} IMPISPAntiflickerMode;

/**
 * ISP anti-flash frequency function frequency structure.
 */
typedef enum {
	IMPISP_ANTIFLICKER_FREQ_50HZ = 50,
	IMPISP_ANTIFLICKER_FREQ_60HZ = 60,
	IMPISP_ANTIFLICKER_FREQ_BUTT,
} IMPISPAntiflickerFreq;

/**
 * ISP Anti-flicker attrbution structure.
 */
typedef struct {
	IMPISPAntiflickerMode mode;			/**< ISP antiflicker mode */
	IMPISPAntiflickerFreq freq;			/**< ISP antiflicker frequence */
} IMPISPAntiflickerAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SetAntiFlickerAttr(IMPVI_NUM num, IMPISPAntiflickerAttr *pattr)
 *
 * Set the antiflicker parameter.
 *
 * @param[in] num		The sensor num label.
 * @param[in] attr	The value for antiflicker attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPAntiflickerAttr attr;
 *
 * attr.mode = IMPISP_ANTIFLICKER_NORMAL_MODE;
 * attr.freq = IMPISP_ANTIFLICKER_FREQ_50HZ;
 * ret = IMP_ISP_Tuning_SetAntiFlickerAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetAntiFlickerAttr error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before calling this function, make sure that ISP debugging function is working.
 */
int32_t IMP_ISP_Tuning_SetAntiFlickerAttr(IMPVI_NUM num, IMPISPAntiflickerAttr *pattr);

/**
 * @fn int32_t IMP_ISP_Tuning_GetAntiFlickerAttr(IMPVI_NUM num, IMPISPAntiflickerAttr *pattr)
 *
 * Get the mode of antiflicker
 *
 * @param[in] num		The sensor num label.
 * @param[in] pattr		The pointer for antiflicker mode.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPAntiflickerAttr attr;
 *
 * ret = IMP_ISP_Tuning_GetAntiFlickerAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAntiFlickerAttr error !\n");
 *	return -1;
 * }
 * printf("antiflicker mode:%d, antiflicker freq:%d\n", attr.mode, attr.freq);
 * @endcode
 *
 * @attention Before calling this function, make sure that ISP debugging function is working.
 */
int32_t IMP_ISP_Tuning_GetAntiFlickerAttr(IMPVI_NUM num, IMPISPAntiflickerAttr *pattr);

/**
 * HV Flip mode struct
 */
typedef enum {
	IMPISP_FLIP_NORMAL_MODE = 0,	/**< normal mode */
	IMPISP_FLIP_H_MODE,				/**< ISP mirror mode */
	IMPISP_FLIP_V_MODE,				/**< ISP flip mode */
	IMPISP_FLIP_HV_MODE,			/**< ISP mirror and flip mode */
	IMPISP_FLIP_MODE_BUTT,			/**< effect paramater, parameters have to be less than this value */
} IMPISPHVFLIP;

typedef struct {
	IMPISPHVFLIP sensor_mode;	/**< Flip mode corresponding to sensor */
	IMPISPHVFLIP isp_mode[3];	/**< Flip mode for each ISP channel */
} IMPISPHVFLIPAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SetHVFLIP(IMPVI_NUM num, IMPISPHVFLIPAttr *attr)
 *
 * Set HV Flip mode.
 *
 * @param[in] num		The sensor num label.
 * @param[in] attr	flip attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPHVFLIPAttr attr;
 *
 * ret = IMP_ISP_Tuning_GetHVFLIP(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetHVFLIP error !\n");
 *	return -1;
 * }
 * attr.sensor_mode = IMPISP_FLIP_H_MODE;
 * attr.isp_mode[0] = IMPISP_FLIP_V_MODE;
 * ret = IMP_ISP_Tuning_SetHVFLIP(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetHVFLIP error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @remark When the IVDC pass through module is used, channel 0 does not have the ISP Vflip function.
 * @remark Sensor HVFLip and ISP HVFLip are in a front-to-back relationship, and Sensor and ISP
 *			cannot mirror or flip at the same time.
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetHVFLIP(IMPVI_NUM num, IMPISPHVFLIPAttr *attr);

/**
 * @fn int32_t IMP_ISP_Tuning_GetHVFLIP(IMPVI_NUM num, IMPISPHVFLIPAttr *attr)
 *
 * Get HV Flip mode.
 *
 * @param[in] num		The sensor num label.
 * @param[out] attr	flip attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPHVFLIPAttr attr;
 *
 * ret = IMP_ISP_Tuning_GetHVFLIP(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetHVFLIP error !\n");
 *	return -1;
 * }
 * printf("hvflip mode:...\n", ...);
 # @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetHVFLIP(IMPVI_NUM num, IMPISPHVFLIPAttr *attr);

/**
 * Defines the enumeration of ISP working mode.
 */
typedef enum {
	IMPISP_RUNNING_MODE_DAY = 0,		/**< ISP day mode */
	IMPISP_RUNNING_MODE_NIGHT = 1,		/**< ISP night mode */
	IMPISP_RUNNING_MODE_BUTT,			/**< effect paramater, parameters have to be less than this value */
} IMPISPRunningMode;

/**
 * @fn int32_t IMP_ISP_Tuning_SetISPRunningMode(IMPVI_NUM num, IMPISPRunningMode *mode)
 *
 * Set ISP running mode, normal mode or night vision mode; default mode: normal mode.
 *
 * @param[in] num		The sensor num label.
 * @param[in] mode		running mode parameter
 *
 * @remark ISP running mode. If enable custom mode, must add extra custom bin file.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPRunningMode mode;
 *
 * if( it is during a night now){
 *		mode = IMPISP_RUNNING_MODE_NIGHT
 * }else{
 *	mode = IMPISP_RUNNING_MODE_DAY;
 * }
 * ret = IMP_ISP_Tuning_SetISPRunningMode(IMPVI_MAIN, &mode);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetISPRunningMode error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetISPRunningMode(IMPVI_NUM num, IMPISPRunningMode *mode);

/**
 * @fn int32_t IMP_ISP_Tuning_GetISPRunningMode(IMPVI_NUM num, IMPISPRunningMode *pmode)
 *
 * Get ISP running mode, normal mode or night vision mode;
 *
 * @param[in] num		The sensor num label.
 * @param[in] pmode		The pointer of the running mode.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPRunningMode mode;
 *
 * mode = IMPISP_RUNNING_MODE_DAY;
 * ret = IMP_ISP_Tuning_GetISPRunningMode(IMPVI_MAIN, &mode);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetISPRunningMode error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetISPRunningMode(IMPVI_NUM num, IMPISPRunningMode *pmode);

/**
 * @fn int32_t IMP_ISP_Tuning_SetBrightness(IMPVI_NUM num, unsigned char *bright)
 *
 * Set the brightness of image effect.
 *
 * @param[in] num		The sensor num label.
 * @param[in] bright	The value for brightness.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 that means increase brightness, and less than 128 that means decrease brightness.\n
 *
 * @code
 * int ret = 0;
 * unsigned char bright = 128;
 *
 * ret = IMP_ISP_Tuning_SetBrightness(IMPVI_MAIN, &bright);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetBrightness error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetBrightness(IMPVI_NUM num, unsigned char *bright);

/**
 * @fn int32_t IMP_ISP_Tuning_GetBrightness(IMPVI_NUM num, unsigned char *pbright)
 *
 * Get the brightness of image effect.
 *
 * @param[in] num		The sensor num label.
 * @param[in] pbright	The pointer for brightness value.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 (increase brightness), and less than 128 (decrease brightness).\n
 *
 * @code
 * int ret = 0;
 * unsigned char bright;
 *
 * ret = IMP_ISP_Tuning_GetBrightness(IMPVI_MAIN, &bright);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetBrightness error !\n");
 *	return -1;
 * }
 * printf("brightness:%d\n", bright);
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetBrightness(IMPVI_NUM num, unsigned char *pbright);

/**
 * @fn int32_t IMP_ISP_Tuning_SetContrast(IMPVI_NUM num, unsigned char *contrast)
 *
 * Set the contrast of image effect.
 *
 * @param[in] num			The sensor num label.
 * @param[in] contrast		The value for contrast.
 *
 * @retval 0 means success.
 * @retval Other value means failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 (increase contrast), and less than 128 (decrease contrast).
 *
 * @code
 * int ret = 0;
 * unsigned char contrast = 128;
 *
 * ret = IMP_ISP_Tuning_SetContrast(IMPVI_MAIN, &contrast);
 * if(ret){
 *	IMP_LOG_ERR(TAG, " error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetContrast(IMPVI_NUM num, unsigned char *contrast);

/**
 * @fn int32_t IMP_ISP_Tuning_GetContrast(IMPVI_NUM num, unsigned char *pcontrast)
 *
 * Get the contrast of image effect.
 *
 * @param[in] num				The sensor num label.
 * @param[in] pcontrast		The pointer for contrast.
 *
 * @retval 0 means success.
 * @retval Other value means failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 (increase contrast), and less than 128 (decrease contrast).
 *
 * @code
 * int ret = 0;
 * unsigned char contrast;
 *
 * ret = IMP_ISP_Tuning_GetContrast(IMPVI_MAIN, &contrast);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetContrast error !\n");
 *	return -1;
 * }
 * printf("contrast:%d\n", contrast);
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetContrast(IMPVI_NUM num, unsigned char *pcontrast);

 /**
  * @fn int32_t IMP_ISP_Tuning_SetSharpness(IMPVI_NUM num, unsigned char *sharpness)
 *
 * Set the sharpness of image effect.
 *
 * @param[in] num				The sensor num label.
 * @param[in] sharpness		The value for sharpening strength.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 (increase sharpening), and less than 128 (decrease sharpening).
 *
 * @code
 * int ret = 0;
 * unsigned char sharpness = 128;
 *
 * IMP_ISP_Tuning_SetSharpness(IMPVI_MAIN, &sharpness);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetSharpness error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetSharpness(IMPVI_NUM num, unsigned char *sharpness);

/**
 * @fn int32_t IMP_ISP_Tuning_GetSharpness(IMPVI_NUM num, unsigned char *psharpness)
 *
 * Get the sharpness of image effect.
 *
 * @param[in] num				The sensor num label.
 * @param[in] psharpness	The pointer for sharpness strength.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 (increase sharpening), and less than 128 (decrease sharpening).
 *
 * @code
 * int ret = 0;
 * unsigned char sharpness;
 *
 * IMP_ISP_Tuning_GetSharpness(IMPVI_MAIN, &sharpness);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetSharpness error !\n");
 *	return -1;
 * }
 * printf("sharpness:%d\n", sharpness);
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetSharpness(IMPVI_NUM num, unsigned char *psharpness);

/**
 * @fn int32_t IMP_ISP_Tuning_SetSaturation(IMPVI_NUM num, unsigned char *saturation)
 *
 * Set the saturation of image effect.
 *
 * @param[in] num			The sensor num label.
 * @param[in] saturation	The value for saturation strength.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark	The default value is 128, more than 128 (increase saturation), and less than 128 (decrease saturation).
 *
 * @code
 * int ret = 0;
 * unsigned char saturation = 128;
 *
 * ret = IMP_ISP_Tuning_SetSaturation(IMPVI_MAIN, &saturation);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetSaturation error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetSaturation(IMPVI_NUM num, unsigned char *saturation);

/**
 * @fn int32_t IMP_ISP_Tuning_GetSaturation(IMPVI_NUM num, unsigned char *psaturation)
 *
 * Get the saturation of image effect.
 *
 * @param[in] num					The sensor num label.
 * @param[in] psaturation			The pointer for saturation strength.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark	The default value is 128, more than 128 (increase saturation), and less than 128 (decrease saturation).
 *
 * @code
 * int ret = 0;
 * unsigned char saturation;
 *
 * ret = IMP_ISP_Tuning_GetSaturation(IMPVI_MAIN, &saturation);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetSaturation error !\n");
 *	return -1;
 * }
 * printf("saturation:%d\n", saturation);
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetSaturation(IMPVI_NUM num, unsigned char *psaturation);

/**
 * @fn int32_t IMP_ISP_Tuning_SetBcshHue(IMPVI_NUM num, unsigned char *hue)
 *
 * Set the hue of image color.
 *
 * @param[in] num		The sensor num label.
 * @param[in] hue		The value of hue, range from 0 to 255, default 128.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 that means increase hue, and less than 128 that means decrease hue.
 *
 * @code
 * int ret = 0;
 * unsigned char hue = 128;
 *
 * ret = IMP_ISP_Tuning_SetBcshHue(IMPVI_MAIN, &hue);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetBcshHue error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetBcshHue(IMPVI_NUM num, unsigned char *hue);

/**
 * @fn int32_t IMP_ISP_Tuning_GetBcshHue(IMPVI_NUM num, unsigned char *hue)
 *
 * Get the hue of image color.
 *
 * @param[in] num		The sensor num label.
 * @param[in] hue		The pointer for hue value.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The default value is 128, more than 128 (increase hue), and less than 128 (decrease hue).
 *
 * @code
 * int ret = 0;
 * unsigned char hue;
 *
 * ret = IMP_ISP_Tuning_GetBcshHue(IMPVI_MAIN, &hue);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetBcshHue error !\n");
 *	return -1;
 * }
 * printf("bcsh hue:%d\n", hue);
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetBcshHue(IMPVI_NUM num, unsigned char *hue);

/**
 * ISP Module Control
 */
typedef union {
	uint32_t key;							/**< module ctrl key */
	struct {
		unsigned int bitBypassDPC	: 1; /* [0]  */
		unsigned int bitBypassGIB	: 1; /* [1]  */
		unsigned int bitBypassLSC	: 1; /* [2]  */
		unsigned int bitBypassAWBG	: 1; /* [3]  */
		unsigned int bitBypassTMO	: 1; /* [4]  */
		unsigned int bitBypassDMSC	: 1; /* [5]	 */
		unsigned int bitBypassCCM	: 1; /* [6]  */
		unsigned int bitBypassGAMMA : 1; /* [7]	 */
		unsigned int bitBypassDEFOG : 1; /* [8]	 */
		unsigned int bitBypassCSC	: 1; /* [9]  */
		unsigned int bitBypassLCE	: 1; /* [10] */
		unsigned int bitBypassMDNS	: 1; /* [11] */
		unsigned int bitBypassYDNS	: 1; /* [12] */
		unsigned int bitBypassBCSH	: 1; /* [13] */
		unsigned int bitBypassCLM	: 1; /* [14] */
		unsigned int bitBypassYSP	: 1; /* [15] */
		unsigned int bitBypassSDNS	: 1; /* [16] */
		unsigned int bitBypassCDNS	: 1; /* [17] */
		unsigned int bitBypassCSCCR : 1; /* [18] */
		unsigned int bitBypassRDNS	: 1; /* [19] */
		unsigned int bitBypassDECMP : 1; /* [20] */
		unsigned int bitRsv0 : 1;		 /* [21] */
		unsigned int bitBypassWDR	: 1; /* [22] */
		unsigned int bitRsv1 : 9; /* [23 ~ 31]	*/
	};
} IMPISPModuleCtl;

/**
 * @fn int32_t IMP_ISP_Tuning_SetModuleControl(IMPVI_NUM num, IMPISPModuleCtl *ispmodule)
 *
 * Set ISP Module control
 *
 * @param[in] num		The sensor num label.
 * @param[in] ispmodule ISP Module control.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPModuleCtl ctl;
 *
 * ret = IMP_ISP_Tuning_GetModuleControl(IMPVI_MAIN, &ctl);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetModuleControl error !\n");
 *	return -1;
 * }
 *
 * ctl.bitBypassBLC = 1;
 * //...
 *
 * ret = IMP_ISP_Tuning_SetModuleControl(IMPVI_MAIN, &ctl);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetModuleControl error !\n");
 *	return -1;
 * }
 * @endcode
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetModuleControl(IMPVI_NUM num, IMPISPModuleCtl *ispmodule);

/**
 * @fn int32_t IMP_ISP_Tuning_GetModuleControl(IMPVI_NUM num, IMPISPModuleCtl *ispmodule)
 *
 * Get ISP Module control.
 *
 * @param[in] num			The sensor num label.
 * @param[out] ispmodule	ISP Module control
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPModuleCtl ctl;
 *
 * ret = IMP_ISP_Tuning_GetModuleControl(IMPVI_MAIN, &ctl);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetModuleControl error !\n");
 *	return -1;
 * }
 * printf("module conctrol:0x%x\n", ctl.key);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetModuleControl(IMPVI_NUM num, IMPISPModuleCtl *ispmodule);

/**
 * ISP module ratio set array list label
 */
typedef enum {
	IMP_ISP_MODULE_SINTER = 0,	/**< sinter denoise array list label */
	IMP_ISP_MODULE_TEMPER,		/**< temper denoise array list label */
	IMP_ISP_MODULE_DRC,			/**< DRC denoise array list label(reserve) */
	IMP_ISP_MODULE_DPC,			/**< DPC denoise array list label */
	IMP_ISP_MODULE_DEFOG,	   /**< DEFOG denoise array list label */
	IMP_ISP_MODULE_BUTT,		/**< effect paramater, parameters have to be less than this value */
} IMPISPModuleRatioArrayList;

/**
 * ISP module ratio array unit
 */
typedef struct {
	IMPISPTuningOpsMode en;   /**< module ratio enable */
	uint8_t ratio;			  /**< module ratio value. The default value is 128, more than 128 (increase strength), and less than 128 (decrease strength) */
} IMPISPRatioUnit;

/**
 * ISP module ratio Attrbution
 */
typedef struct {
	IMPISPRatioUnit ratio_attr[16]; /**< module ratio attr */
} IMPISPModuleRatioAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SetModule_Ratio(IMPVI_NUM num, IMPISPModuleRatioAttr *ratio)
 *
 * set isp modules ratio.
 *
 * @param[in] num	The sensor num label.
 * @param[in] ratio module ratio set attrbution.
 *
 * @retval 0 means success.
 * @retval Other value means failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPModuleRatioAttr ratio;
 *
 * ret = IMP_ISP_Tuning_GetModule_Ratio(IMPVI_MAIN, &ratio);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetModule_Ratio error !\n");
 *	return -1;
 * }
 *
 * ratio.ratio_attr[IMP_ISP_MODULE_SINTER].en = IMPISP_TUNING_OPS_MODE_ENABLE;
 * ratio.ratio_attr[IMP_ISP_MODULE_SINTER].ratio = 128;
 * //...
 *
 * ret = IMP_ISP_Tuning_SetModule_Ratio(IMPVI_MAIN, &ratio);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetModule_Ratio error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetModule_Ratio(IMPVI_NUM num, IMPISPModuleRatioAttr *ratio);

/**
 * @fn int32_t IMP_ISP_Tuning_GetModule_Ratio(IMPVI_NUM num, IMPISPModuleRatioAttr *ratio)
 *
 * set isp modules ratio.
 *
 * @param[in] num		The sensor num label.
 * @param[out] ratio	 module ratio set attrbution.
 *
 * @retval 0 means success.
 * @retval Other value means failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPModuleRatioAttr ratio;
 *
 * ret = IMP_ISP_Tuning_GetModule_Ratio(IMPVI_MAIN, &ratio);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetModule_Ratio error !\n");
 *	return -1;
 * }
 * printf("sinter en:%d, sinter ratio:%d\n", ratio.ratio_attr[IMP_ISP_MODULE_SINTER].en,
 * ratio.ratio_attr[IMP_ISP_MODULE_SINTER].ratio);
 * @endcode
 *
 * @attention Before using it, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetModule_Ratio(IMPVI_NUM num, IMPISPModuleRatioAttr *ratio);

/**
 * ISP CSC Matrix Standard
 */
typedef enum {
	IMP_ISP_CG_BT601_FULL,	   /**< BT601 full range */
	IMP_ISP_CG_BT601_LIMITED,  /**< BT601 not full range */
	IMP_ISP_CG_BT709_FULL,	   /**< BT709 full range */
	IMP_ISP_CG_BT709_LIMITED,  /**< BT709 not full range */
	IMP_ISP_CG_BT2020_FULL,	   /**< BT2020 full range */
	IMP_ISP_CG_BT2020_LIMITED, /**< BT2020 not full range */
	IMP_ISP_CG_USER,		   /**< CUSTOM mode. Only use this mode, the IMPISPCscMatrix parameters is valid. */
	IMP_ISP_CG_BUTT,		   /**< effect paramater, parameters have to be less than this value */
} IMPISPCSCColorGamut;

/**
 * ISP CSC Matrix struct
 */
typedef struct {
	float CscCoef[9];				/**< 3x3 matrix */
	unsigned char CscOffset[2];		/**< [0]:UV offset [1]:Y offset */
	unsigned char CscClip[4];		/**< Y max, Y min, UV max, UV min */
} IMPISPCscMatrix;

/**
 * ISP CSC Attribution
 */
typedef struct {
	IMPISPCSCColorGamut ColorGamut;		/**< RGB to YUV Matrix Standard */
	IMPISPCscMatrix Matrix;				/**< Custom Matrix */
        IMPISPCscMatrix MatrixIn;           /**< Custom MatrixIn */
} IMPISPCSCAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SetISPCSCAttr(IMPVI_NUM num, IMPISPCSCAttr *csc)
 *
 * set csc attr
 *
 * @param[in] num	The sensor num label.
 * @param[out] csc	csc attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPCSCAttr csc;
 *
 * memset(&csc, 0, sizeof(csc));
 * csc.ColorGamut = mode;
 *
 * if (csc.ColorGamut == IMP_ISP_CG_USER) {
 *		float CscCoef[] = {...};
 *		unsigned char CscOffset[] = {...};
 *		unsigned char CscClip[] = {...};
 *
 *		memcpy(csc.CscCoef, CscCoef, sizeof(CscCoef));
 *		memcpy(csc.CscOffset, CscOffset, sizeof(CscOffset));
 *		memcpy(csc.CscClip, CscClip, sizeof(CscClip));
 * }
 * ret = IMP_ISP_Tuning_SetISPCSCAttr(IMPVI_MAIN, &csc);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetISPCSCAttr error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetISPCSCAttr(IMPVI_NUM num, IMPISPCSCAttr *csc);

/**
 * @fn int32_t IMP_ISP_Tuning_GetISPCSCAttr(IMPVI_NUM num, IMPISPCSCAttr *csc)
 *
 * Get CCM Attr.
 *
 * @param[in] num	The sensor num label.
 * @param[out] csc	csc attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPCSCAttr csc;
 *
 * memset(&csc, 0, sizeof(csc));
 *
 * ret = IMP_ISP_Tuning_GetISPCSCAttr(IMPVI_MAIN, &csc);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetISPCSCAttr error !\n");
 *	return -1;
 * }
 * printf("csc color gamut:%d\n", csc.ColorGamut);
 * if (csc.ColorGamut == IMP_ISP_CG_USER) {
 *		printf("CscCoef:%f ..., CscOffset:%d ..., CscClip:%d ...\n", ...);
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetISPCSCAttr(IMPVI_NUM num, IMPISPCSCAttr *csc);

/**
 * ISP CCM Attr
 */
typedef struct {
	IMPISPTuningOpsMode ManualEn;		/**< CCM Manual enable ctrl */
	IMPISPTuningOpsMode SatEn;			/**< CCM Saturation enable ctrl */
	float ColorMatrix[9];				/**< color matrix on manual mode */
} IMPISPCCMAttr;
/**
 * @fn int32_t IMP_ISP_Tuning_SetCCMAttr(IMPVI_NUM num, IMPISPCCMAttr *ccm)
 *
 * set ccm attr
 *
 * @param[in] num	The sensor num label.
 * @param[out] ccm	ccm attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPCCMAttr attr;
 * float ColorMatrix[9] = {...};
 *
 * attr.ManualEn = IMPISP_TUNING_OPS_MODE_ENABLE;
 * attr.SatEn = IMPISP_TUNING_OPS_MODE_ENABLE;
 * memcpy(attr.ColorMatrix, ColorMatrix, sizeof(ColorMatrix));
 *
 * ret = IMP_ISP_Tuning_SetCCMAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetCCMAttr error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetCCMAttr(IMPVI_NUM num, IMPISPCCMAttr *ccm);

/**
 * @fn int32_t IMP_ISP_Tuning_GetCCMAttr(IMPVI_NUM num, IMPISPCCMAttr *ccm)
 *
 * Get CCM Attr.
 *
 * @param[in] num	The sensor num label.
 * @param[out] ccm	ccm attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPCCMAttr attr;
 *
 * memset(&attr, 0x0, sizeof(IMPISPCCMAttr));
 * ret = IMP_ISP_Tuning_GetCCMAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetCCMAttr error !\n");
 *	return -1;
 * }
 * printf("manual_en:%d, sat_en:%d\n", attr.ManualEn, attr.SatEn);
 * printf("color matrix:...", ...);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetCCMAttr(IMPVI_NUM num, IMPISPCCMAttr *ccm);

/**
 * statistics color domain
 */
typedef enum {
	IMP_ISP_GAMMA_CURVE_DEFAULT,
	IMP_ISP_GAMMA_CURVE_SRGB,
	IMP_ISP_GAMMA_CURVE_REC709,
	IMP_ISP_GAMMA_CURVE_HDR,
	IMP_ISP_GAMMA_CURVE_USER,
	IMP_ISP_GAMMA_CURVE_BUTT,
} IMPISPGammaCurveType;

typedef struct {
	IMPISPGammaCurveType Curve_type;
	uint16_t gamma[129];
} IMPISPGammaAttr;

int32_t IMP_ISP_Tuning_SetGammaAttr(IMPVI_NUM num, IMPISPGammaAttr *gamma);

int32_t IMP_ISP_Tuning_GetGammaAttr(IMPVI_NUM num, IMPISPGammaAttr *gamma);

typedef enum {
	IMP_ISP_HIST_ON_RAW,    /**< Raw Domain */
	IMP_ISP_HIST_ON_YUV,    /**< YUV Domain */
} IMPISPHistDomain;

/**
 * statistics hist area struct
 */
typedef struct {
	unsigned int start_h;   /**< start pixel in the horizontal */
	unsigned int start_v;   /**< start pixel in the vertical */
	unsigned char node_h;   /**< the statistics node num in the horizontal [12 ~ 15] */
	unsigned char node_v;   /**< the statistics node num in the vertical [12 ~ 15]*/
} IMPISP3AStatisLocation;

/**
 * AE statistics attribution
 */
typedef struct {
	IMPISPTuningOpsMode ae_sta_en;  /**< AE enable*/
	IMPISP3AStatisLocation local;   /**< AE statistics location */
	IMPISPHistDomain hist_domain;   /**< AE Hist statistics color domain */
	unsigned char histThresh[4];    /**< AE Hist Thresh */
} IMPISPAEStatisAttr;

/**
 * AWB statistics value attribution
 */
typedef enum {
	IMP_ISP_AWB_ORIGIN,    /**< Original value */
	IMP_ISP_AWB_LIMITED,   /**< Limited statistics value */
} IMPISPAWBStatisMode;

/**
 * AWB statistics attribution
 */
typedef struct {
	IMPISPTuningOpsMode awb_sta_en;	/**< AWB enable*/
	IMPISP3AStatisLocation local;	/**< AWB Statistic area */
	IMPISPAWBStatisMode mode;	/**< AWB Statistic mode */
} IMPISPAWBStatisAttr;

/**
 * AF statistics attribution
 */
typedef struct {
	IMPISPTuningOpsMode af_sta_en;          /**< AF enable*/
	IMPISP3AStatisLocation local;           /**< AF statistics area */
	unsigned char af_metrics_shift;         /**< Metrics scaling factor 0x0 is default*/
	unsigned short af_delta;                /**< AF statistics low pass fliter weight [0 ~ 64]*/
	unsigned short af_theta;                /**< AF statistics high pass fliter weight [0 ~ 64]*/
	unsigned short af_hilight_th;           /**< AF high light threshold [0 ~ 255]*/
	unsigned short af_alpha_alt;            /**< AF statistics H and V direction weight [0 ~ 64]*/
	unsigned short af_belta_alt;            /**< AF statistics H and V direction weight [0 ~ 64]*/
} IMPISPAFStatisAttr;

/**
 * Statistics info attribution
 */
typedef struct {
	IMPISPAEStatisAttr ae;      /**< AE statistics info attr */
	IMPISPAWBStatisAttr awb;    /**< AWB statistics info attr */
	IMPISPAFStatisAttr af;      /**< AF statistics info attr */
} IMPISPStatisConfig;

/**
 * @fn int32_t IMP_ISP_Tuning_SetStatisConfig(IMPVI_NUM num, IMPISPStatisConfig *statis_config)
 *
 * set statistics attribution
 *
 * @param[in] num               The sensor num label.
 * @param[in] statis_config 	statistics configuration.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPStatisConfig config;
 * unsigned char histThresh[4] = {...};
 *
 * ret = IMP_ISP_Tuning_GetStatisConfig(IMPVI_MAIN, &config);
 * if(ret){
 * 	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetStatisConfig error !\n");
 * 	return -1;
 * }
 *
 * config.ae.ae_sta_en = ae_mode;
 * if (config.ae.ae_sta_en == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *      memcpy(config.ae.histThresh, histThresh, sizeof(histThresh));
 * }
 *
 * config.awb.awb_sta_en = awb_mode;
 * if (config.awb.awb_sta_en == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *      config.awb.local.start_h = 1;
 *      config.awb.local.start_v = 1;
 *      config.awb.local.node_h = 15;
 *      config.awb.local.node_v = 15;
 * }
 *
 * config.af.af_sta_en = af_mode;
 * if (config.af.af_sta_en == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *      config.af.local.start_h = 1;
 *      config.af.local.start_v = 3;
 *      config.af.local.node_h = 15;
 *      config.af.local.node_v = 15;
 *      config.af.af_metrics_shift = 0;
 *      config.af.af_delta = 1;
 *      config.af.af_theta = 1;
 *      config.af.af_hilight_th = 1;
 *      config.af.af_alpha_alt = 1;
 *      config.af.af_belta_alt = 1;
 * }
 *
 * ret = IMP_ISP_Tuning_SetStatisConfig(IMPVI_MAIN, &config);
 * if(ret){
 * 	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetStatisConfig error !\n");
 * 	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetStatisConfig(IMPVI_NUM num, IMPISPStatisConfig *statis_config);

/**
 * @fn int32_t IMP_ISP_Tuning_GetStatisConfig(IMPVI_NUM num, IMPISPStatisConfig *statis_config)
 *
 * set statistics attribution
 *
 * @param[in] num               The sensor num label.
 * @param[out] statis_config 	statistics configuration.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPStatisConfig config;
 *
 * ret = IMP_ISP_Tuning_GetStatisConfig(IMPVI_MAIN, &config);
 * if(ret){
 * 	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetStatisConfig error !\n");
 * 	return -1;
 * }
 *
 * printf("ae config mode:%s\n", config.ae.ae_sta_en ? "enable" : "disable");
 * if (config.ae.ae_sta_en == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *      printf("ae config:...", ...);
 * }
 *
 * printf("awb config mode:%s\n", config.awb.awb_sta_en ? "enable" : "disable");
 * if (config.awb.awb_sta_en == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *      printf("awb config:...", ...);
 * }
 *
 * printf("af config mode:%s\n", config.af.af_sta_en ? "enable" : "disable");
 * if (config.af.af_sta_en == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *      printf("af config:...", ...);
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetStatisConfig(IMPVI_NUM num, IMPISPStatisConfig *statis_config);

/**
 * ISP Gamma mode enumeration
 */
typedef enum {
	ISP_CORE_EXPR_UNIT_LINE,
	ISP_CORE_EXPR_UNIT_US,
} IMPISPAEIntegrationTimeUnit;

/**
 * Defines the attribute of gamma.
 */
typedef struct {
	IMPISPAEIntegrationTimeUnit AeIntegrationTimeUnit;
	IMPISPTuningOpsType AeMode;
	IMPISPTuningOpsType AeIntegrationTimeMode;
	IMPISPTuningOpsType AeAGainManualMode;
	IMPISPTuningOpsType AeDGainManualMode;
	IMPISPTuningOpsType AeIspDGainManualMode;
	uint32_t AeIntegrationTime;
	uint32_t AeAGain;
	uint32_t AeDGain;
	uint32_t AeIspDGain;

	IMPISPTuningOpsType AeMinIntegrationTimeMode;
	IMPISPTuningOpsType AeMinAGainMode;
	IMPISPTuningOpsType AeMinDgainMode;
	IMPISPTuningOpsType AeMinIspDGainMode;
	IMPISPTuningOpsType AeMaxIntegrationTimeMode;
	IMPISPTuningOpsType AeMaxAGainMode;
	IMPISPTuningOpsType AeMaxDgainMode;
	IMPISPTuningOpsType AeMaxIspDGainMode;
	uint32_t AeMinIntegrationTime;
	uint32_t AeMinAGain;
	uint32_t AeMinDgain;
	uint32_t AeMinIspDGain;
	uint32_t AeMaxIntegrationTime;
	uint32_t AeMaxAGain;
	uint32_t AeMaxDgain;
	uint32_t AeMaxIspDGain;

	/* WDR模式下短帧的AE 手动模式属性*/
	IMPISPTuningOpsType AeShortMode;
	IMPISPTuningOpsType AeShortIntegrationTimeMode;
	IMPISPTuningOpsType AeShortAGainManualMode;
	IMPISPTuningOpsType AeShortDGainManualMode;
	IMPISPTuningOpsType AeShortIspDGainManualMode;
	uint32_t AeShortIntegrationTime;
	uint32_t AeShortAGain;
	uint32_t AeShortDGain;
	uint32_t AeShortIspDGain;

	IMPISPTuningOpsType AeShortMinIntegrationTimeMode;
	IMPISPTuningOpsType AeShortMinAGainMode;
	IMPISPTuningOpsType AeShortMinDgainMode;
	IMPISPTuningOpsType AeShortMinIspDGainMode;
	IMPISPTuningOpsType AeShortMaxIntegrationTimeMode;
	IMPISPTuningOpsType AeShortMaxAGainMode;
	IMPISPTuningOpsType AeShortMaxDgainMode;
	IMPISPTuningOpsType AeShortMaxIspDGainMode;
	uint32_t AeShortMinIntegrationTime;
	uint32_t AeShortMinAGain;
	uint32_t AeShortMinDgain;
	uint32_t AeShortMinIspDGain;
	uint32_t AeShortMaxIntegrationTime;
	uint32_t AeShortMaxAGain;
	uint32_t AeShortMaxDgain;
	uint32_t AeShortMaxIspDGain;
} IMPISPAEExprInfo;

int32_t IMP_ISP_Tuning_SetAeExprInfo(IMPVI_NUM num, IMPISPAEExprInfo *exprinfo);

int32_t IMP_ISP_Tuning_GetAeExprInfo(IMPVI_NUM num, IMPISPAEExprInfo *exprinfo);

/**
 * AE read-only attribute
 */
typedef struct {
	uint32_t TotalGainDb;			/**< Ae total gain, Unit is db */
	uint32_t TotalGainDbShort;		/**< Ae total gain of short frames, Unit is db */
	uint64_t ExposureValue;			/**< Ae exposure value, integration time x again x dgain */
	uint32_t EVLog2;				/**< Ae exposure value, This value is log-operated */
	uint32_t luma;					/**< Ae luma */
	uint32_t luma_scence;			/**< Ae scene */
	int stable;						/**< Ae State of convergence */
	uint32_t target;				/**< Current target brightness */
	uint32_t ae_mean;				/**< The current statistical average of AE after superimposing the weights */
    uint32_t bv;
} IMPISPAeOnlyReadAttr;

 /**
 * @fn int32_t IMP_ISP_Tuning_GetAeOnlyReadAttr(IMPVI_NUM num, IMPISPAeOnlyReadAttr *OnlyAttr)
 *
 * Get the AE read-only attribute
 *
 * @param[in] num		The sensor num label.
 * @param[out] Attr		AE read-only attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAeOnlyReadAttr(IMPVI_NUM num, IMPISPAeOnlyReadAttr *OnlyAttr);

/**
 * Target luminance meter
 */
typedef struct {
	IMPISPTuningOpsMode enable;		/**< Enables the target luminance meter */
	uint64_t at[15];				/**< Target brightness Indicates the target brightness of the table */
} IMPISPAeAtList;

/**
 * Exposure gain meter
 */
typedef struct {
	IMPISPTuningOpsMode enable;		/**< Enable the exposure gain meter */
	uint64_t list[15];				/**< Exposure gain meter table */
} IMPISPAeEvList;

/**
 * AE target attribute
 */
typedef struct {
	IMPISPTuningOpsMode atCompEn;		/**< AE target brightness compensation is enabled */
	uint32_t atComp;					/**< AE Target brightness adjustment intensity (0 ~ 255, less than 128 becomes dark, greater than 128 becomes bright) */
	IMPISPAeAtList at;					/**< Target luminance meter */
	IMPISPAeEvList ev;					/**< Exposure gain meter */
} IMPISPAeTargetAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SetAeTargetAttr(IMPVI_NUM num, IMPISPAeTargetAttr *attr)
 *
 * Set AE target attribute
 *
 * @param[in] num		The sensor num label.
 * @param[in] attr		AE target attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetAeTargetAttr(IMPVI_NUM num, IMPISPAeTargetAttr *attr);

 /**
 * @fn int32_t IMP_ISP_Tuning_GetAeTargetAttr(IMPVI_NUM num, IMPISPAeTargetAttr *attr)
 *
 * Get AE target attribute
 *
 * @param[in] num		The sensor num label.
 * @param[out] attr		AE target attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAeTargetAttr(IMPVI_NUM num, IMPISPAeTargetAttr *attr);

/**
 * AE start point attribute
 */
typedef struct {
	IMPISPTuningOpsMode enable;			/**< Enable the start point of AE */
	uint32_t value;						/**< EV value of AE starting point */
} IMPISPAeStartAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SetAeStartAttr(IMPVI_NUM num, IMPISPAeStartAttr *attr)
 *
 * Set the AE start point property.
 *
 * @param[in] num		The sensor num label.
 * @param[in] attr		AE start point attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetAeStartAttr(IMPVI_NUM num, IMPISPAeStartAttr *attr);

 /**
 * @fn int32_t IMP_ISP_Tuning_GetAeStartAttr(IMPVI_NUM num, IMPISPAeStartAttr *attr)
 *
 * Get the AE start point property.
 *
 * @param[in] num		The sensor num label.
 * @param[out] attr		AE start point attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAeStartAttr(IMPVI_NUM num, IMPISPAeStartAttr *attr);

/**
 * Highlight Compensation or backlight compensation mode
 */
typedef enum {
    IMPISP_HBLC_DISABLE,
    IMPISP_HLC_ENABLE,
    IMPISP_BLC_ENABLE,
} IMPISPAeLightCompensationMode;

/**
 * Highlight Compensation or backlight compensation attribution
 */
typedef struct {
    IMPISPAeLightCompensationMode mode;
    uint8_t strength;                     /* strength range: 1~128 */
}IMPISPAeLightCompensationAttr;
/**
 * @fn int32_t IMP_ISP_Tuning_SetAeLightCompensation(IMPVI_NUM num, IMPISPAeStartAttr *attr)
 *
 * Set AE Highlight Compensation or backlight compensation attribution.
 *
 * @param[in] num		The sensor num label.
 * @param[out] attr		AE highlight Compensation or backlight compensation attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetAeLightCompensation(IMPVI_NUM num, IMPISPAeLightCompensationAttr *attr);

 /**
 * @fn int32_t IMP_ISP_Tuning_GetAeLightCompensation(IMPVI_NUM num, IMPISPAeStartAttr *attr)
 *
 * Get AE Highlight Compensation or backlight compensation attribution.
 *
 * @param[in] num		The sensor num label.
 * @param[out] attr		AE highlight Compensation or backlight compensation attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAeLightCompensation(IMPVI_NUM num, IMPISPAeLightCompensationAttr *attr);

/**
 * awb gain
 */
typedef struct {
	uint32_t rgain;		/**< awb r-gain */
	uint32_t bgain;		/**< awb b-gain */
} IMPISPAWBGain;

typedef TISPAWBMode IMPISPAWBMode;
#if 0
/**
 * awb mode
 */
typedef enum {
	ISP_CORE_WB_MODE_AUTO = 0,			/**< auto mode */
	ISP_CORE_WB_MODE_MANUAL,			/**< manual mode */
	ISP_CORE_WB_MODE_DAY_LIGHT,			/**< day light mode */
	ISP_CORE_WB_MODE_CLOUDY,			/**< cloudy mode */
	ISP_CORE_WB_MODE_INCANDESCENT,		/**< incandescent mode */
	ISP_CORE_WB_MODE_FLOURESCENT,		/**< flourescent mode */
	ISP_CORE_WB_MODE_TWILIGHT,			/**< twilight mode */
	ISP_CORE_WB_MODE_SHADE,				/**< shade mode */
	ISP_CORE_WB_MODE_WARM_FLOURESCENT,	/**< warm flourescent mode */
	ISP_CORE_WB_MODE_COLORTEND,			/**< Color Trend Mode */
} IMPISPAWBMode;
#endif
/**
 * awb custom mode attribution
 */
typedef struct {
	IMPISPTuningOpsMode customEn;  /**< awb custom enable */
	IMPISPAWBGain gainH;		   /**< awb gain on high ct */
	IMPISPAWBGain gainM;		   /**< awb gain on medium ct */
	IMPISPAWBGain gainL;		   /**< awb gain on low ct */
	uint32_t ct_node[4];		   /**< awb custom mode nodes */
} IMPISPAWBCustomModeAttr;

/**
 * awb attribution
 */
typedef struct isp_core_wb_attr{
	IMPISPAWBMode mode;						/**< awb mode */
	IMPISPAWBGain gain_val;			/**< awb gain on manual mode */
	IMPISPTuningOpsMode awb_frz;			/**< awb frz enable */
	IMPISPAWBCustomModeAttr custom;			/**< awb custom attribution */
} IMPISPWBAttr;

/**
* @fn int32_t IMP_ISP_Tuning_SetAwbAttr(IMPVI_NUM num, IMPISPWBAttr *attr)
*
* set awb attribution
*
* @param[in]  num					  The sensor num label.
* @param[in]  attr					  awb attribution.
*
* @retval 0 means success.
* @retval Other values mean failure, its value is an error code.
*
 * @code
 * int ret = 0;
 * IMPISPWBAttr attr;
 * uint32_t ct_node[4] = {...};
 *
 * ret = IMP_ISP_Tuning_GetAwbAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAwbAttr error !\n");
 *	return -1;
 * }
 *
 * attr.mode = ISP_CORE_WB_MODE_MANUAL;
 * atrr.gain_val.rgain = 200;
 * atrr.gain_val.bgain = 200;
 *
 * attr.awb_frz = IMPISP_TUNING_OPS_MODE_ENABLE;
 *
 * attr.custom.customEn = IMPISP_TUNING_OPS_MODE_ENABLE;
 * attr.custom.gainH.rgain = 100;
 * attr.custom.gainH.bgain = 100;
 * attr.custom.gainM.rgain = 100;
 * attr.custom.gainM.bgain = 100;
 * attr.custom.gainL.rgain = 100;
 * attr.custom.gainL.bgain = 100;
 * memcpy(attr.custom.ct_node, ct_node, sizeof(ct_node));
 *
 * ret = IMP_ISP_Tuning_SetAwbAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetAwbAttr error !\n");
 *	return -1;
 * }
 * @endcode
 *
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
*/
int32_t IMP_ISP_Tuning_SetAwbAttr(IMPVI_NUM num, IMPISPWBAttr *attr);

/**
* @fn int32_t IMP_ISP_Tuning_GetAwbAttr(IMPVI_NUM num, IMPISPWBAttr *attr)
*
* get awb attribution
*
* @param[in]   num					   The sensor num label.
* @param[out]  attr					   awb attribution.
*
* @retval 0 means success.
* @retval Other values mean failure, its value is an error code.
*
 * @code
 * int ret = 0;
 * IMPISPWBAttr attr;
 *
 * ret = IMP_ISP_Tuning_GetAwbAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAwbAttr error !\n");
 *	return -1;
 * }
 * printf("awb mode:%d, rgain:%d, bgain:%d\n", attr.mode, attr.gain_val.rgain, attr.gain_val.bgain);
 * printf("awb frzzen:%d\n", attr.awb_frz);
 * printf("awb custom en:%d", attr.custom.customEn);
 * if (attr.custom.customEn == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *		printf("awb custom:...", ...);
 * }
 * @endcode
 *
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
*/
int32_t IMP_ISP_Tuning_GetAwbAttr(IMPVI_NUM num, IMPISPWBAttr *attr);

typedef struct {
	unsigned char weight[15][15];
} IMPISPWeight;

/**
 * @fn int32_t IMP_ISP_Tuning_SetAfWeight(IMPVI_NUM num, IMPISPWeight *af_weight)
 *
 * set zone weighting for AF
 *
 * @param[in] num			The sensor num label.
 * @param[in] af_weight		af weight
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPWeight af;
 * unsigned char weight[15*15] = {...};
 *
 * memcpy(af.weight, weight, sizeof(weight));
 * ret = IMP_ISP_Tuning_SetAfWeight(IMPVI_MAIN, &af);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetAfWeight error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetAfWeight(IMPVI_NUM num, IMPISPWeight *af_weight);

/**
 * @fn int32_t IMP_ISP_Tuning_GetAfWeight(IMPVI_NUM num, IMPISPWeight *af_weight)
 *
 * get zone weighting for AF
 *
 * @param[in] num			 The sensor num label.
 * @param[out] af_weight	 af weight
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPWeight af;
 *
 * ret = IMP_ISP_Tuning_GetAfWeight(IMPVI_MAIN, &af);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAfWeight error !\n");
 *	return -1;
 * }
 * printf("af weight:...\n", ...);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetAfWeight(IMPVI_NUM num, IMPISPWeight *af_weight);

typedef struct {
	unsigned char weight[32][32];
} IMPISPAwbWeight;
/**
 * @fn int32_t IMP_ISP_Tuning_SetAwbWeight(IMPVI_NUM num, IMPISPAwbWeight *awb_weight)
 *
 * set awb weight
 *
 * @param[in] num					  The sensor num label.
 * @param[in] awb_weight			  awb weight.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPAwbWeight weight;
 * unsigned char w[15*15] = {...};
 *
 * memcpy(weight.weight, w, sizeof(w));
 * ret = IMP_ISP_Tuning_SetAwbWeight(IMPVI_MAIN, &weight);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetAwbWeight error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetAwbWeight(IMPVI_NUM num, IMPISPAwbWeight *awb_weight);

/**
 * @fn int32_t IMP_ISP_Tuning_GetAwbWeight(IMPVI_NUM num, IMPISPAwbWeight *awb_weight)
 *
 * get awb weight
 *
 * @param[in]	 num					 The sensor num label.
 * @param[out]	 awb_weight				 awb weight.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPAwbWeight weight;
 *
 * ret = IMP_ISP_Tuning_GetAwbWeight(IMPVI_MAIN, &weight);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAwbWeight error !\n");
 *	return -1;
 * }
 * printf("awb weight:...\n", ...);
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAwbWeight(IMPVI_NUM num, IMPISPAwbWeight *awb_weight);

typedef struct {
	unsigned int zone[32][32];
}IMPISPAwbStatisZone;
/**
 * AWB statistics info
 */
typedef struct {
	IMPISPAwbStatisZone awb_r;	  /**< AWB R channel statistics value */
	IMPISPAwbStatisZone awb_g;	  /**< AWB G channel statistics value */
	IMPISPAwbStatisZone awb_b;	  /**< AWB B channel statistics value */
} IMPISPAWBStatisInfo;

/**
* @fn int32_t IMP_ISP_Tuning_GetAwbStatistics(IMPVI_NUM num, IMPISPAWBStatisInfo *awb_statis)
*
* get awb statistics
*
* @param[in]   num					   The sensor num label.
* @param[out]  awb_statis			   awb statistics.
*
* @retval 0 means success.
* @retval Other values mean failure, its value is an error code.
*
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
*/
int32_t IMP_ISP_Tuning_GetAwbStatistics(IMPVI_NUM num, IMPISPAWBStatisInfo *awb_statis);

/**
 * AWB Global statistics
 */
typedef struct {
	IMPISPAWBGain statis_weight_gain;	/**< awb global weight gain */
	IMPISPAWBGain statis_gol_gain;		/**< awb global gain */
} IMPISPAWBGlobalStatisInfo;

/**
* @fn int32_t IMP_ISP_Tuning_GetAwbGlobalStatistics(IMPVI_NUM num, IMPISPAWBGlobalStatisInfo *awb_statis)
*
* get awb global statistics
*
* @param[in]   num					   The sensor num label.
* @param[out]  awb_statis			   awb statistics.
*
* @retval 0 means success.
* @retval Other values mean failure, its value is an error code.
*
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
*/
int32_t IMP_ISP_Tuning_GetAwbGlobalStatistics(IMPVI_NUM num, IMPISPAWBGlobalStatisInfo *awb_statis);

/**
 * statistics info each area
 */
typedef struct {
	uint32_t statis[15][15];
}  __attribute__((packed, aligned(1))) IMPISPStatisZone;

/**
 * AE statistics info
 */
typedef struct {
	unsigned short ae_hist_5bin[5];			/**< AE hist bin value [0 ~ 65535] */
	uint32_t ae_hist_256bin[256];			/**< AE hist bin value, is the true value of pixels num each bin */
	IMPISPStatisZone ae_statis;				/**< AE statistics info */
}  __attribute__((packed, aligned(1))) IMPISPAEStatisInfo;

/**
* @fn int32_t IMP_ISP_Tuning_GetAeStatistics(IMPVI_NUM num, IMPISPAEStatisInfo *ae_statis)
*
* set ae weight
*
* @param[in]   num				   The sensor num label.
* @param[out]  ae_statis		   ae statistics.
*
* @retval 0 means success.
* @retval Other values mean failure, its value is an error code.
*
 * @code
 * int ret = 0;
 * IMPISPAEStatisInfo info;
 *
 * ret = IMP_ISP_Tuning_GetAeStatistics(IMPVI_MAIN, &statis);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAeStatistics error !\n");
 *	return -1;
 * }
 *
 * printf("ae hist 5bin:...\n", info.ae_hist_5bin[...]);
 * printf("ae hist 256bin:...\n, info.ae_hist_256bin[...]");
 * printf("ae statis:...\n", info.ae_statis.statis[...][...]);
 * @endcode
 *
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
*/
int32_t IMP_ISP_Tuning_GetAeStatistics(IMPVI_NUM num, IMPISPAEStatisInfo *ae_statis);

/**
 * AF statistics info each area
 */
typedef struct {
	IMPISPStatisZone Af_Fir0;
	IMPISPStatisZone Af_Fir1;
	IMPISPStatisZone Af_Iir0;
	IMPISPStatisZone Af_Iir1;
	IMPISPStatisZone Af_YSum;
	IMPISPStatisZone Af_HighLumaCnt;
} IMPISPAFStatisInfo;
/**
 * @fn int32_t IMP_ISP_Tuning_GetAfStatistics(IMPVI_NUM num, IMPISPAFStatisInfo *af_statis)
 *
 * get awb weight
 *
 * @param[in]	 num					 The sensor num label.
 * @param[out]	 af_statis				 af statistics.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPAFStatisInfo info;
 *
 * ret = IMP_ISP_Tuning_GetAfStatistics(IMPVI_MAIN, &info);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAfStatistics error !\n");
 *	return -1;
 * }
 * printf("af statis:...\n", ...);
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAfStatistics(IMPVI_NUM num, IMPISPAFStatisInfo *af_statis);

typedef struct {
	unsigned char weight[15][15];
} IMPISPAeWeight;
/**
 * AE weight attribute
 */
typedef struct {
	IMPISPTuningOpsMode weight_enable; /**< global weight set enable */
	IMPISPAeWeight ae_weight;			 /**< global weight value (0 ~ 8)*/
} IMPISPAEWeightAttr;

/**
* @fn int32_t IMP_ISP_Tuning_SetAeWeight(IMPVI_NUM num, IMPISPAEWeightAttr *ae_weight)
*
* set ae weight
*
* @param[in] num			   The sensor num label.
* @param[in] ae_weight	ae weight.
*
* @retval 0 means success.
* @retval Other values mean failure, its value is an error code.
*
 * @code
 * int ret = 0;
 * IMPISPAEWeightAttr attr;
 * unsigned char weight[15*15] = {...};
 *
 * attr.weight_enable = IMPISP_TUNING_OPS_MODE_ENABLE;
 * memcpy(attr.ae_weight.weight, weight, sizeof(weight));
 *
 * ret = IMP_ISP_Tuning_SetAeWeight(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetAeWeight error !\n");
 *	return -1;
 * }
 * @endcode
 *
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
*/
int32_t IMP_ISP_Tuning_SetAeWeight(IMPVI_NUM num, IMPISPAEWeightAttr *ae_weight);

/**
* @fn int32_t IMP_ISP_Tuning_GetAeWeight(IMPVI_NUM num, IMPISPAEWeightAttr *ae_weight)
*
* set ae weight
*
* @param[in] num			   The sensor num label.
* @param[out] ae_weight		ae weight.
*
* @retval 0 means success.
* @retval Other values mean failure, its value is an error code.
*
 * @code
 * int ret = 0;
 * IMPISPAEWeightAttr attr;
 *
 * ret = IMP_ISP_Tuning_GetAeWeight(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAeWeight error !\n");
 *	return -1;
 * }
 *
 * if (attr.weight_enable == IMPISP_TUNING_OPS_MODE_ENABLE) {
 *		printf("ae global weight:...\n", attr.ae_weight.weight[...][...]);
 * }
 * @endcode
 *
* @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
*/
int32_t IMP_ISP_Tuning_GetAeWeight(IMPVI_NUM num, IMPISPAEWeightAttr *ae_weight);

/**
 * AF metrics
 */
 typedef struct {
	 uint32_t af_metrics;		/**< AF Main metrics */
	 uint32_t af_metrics_alt;	/**< AF second metrics */
	 uint8_t af_frame_num;		/**< AF frame num */
 } IMPISPAFMetricsInfo;

/**
 * @fn int32_t IMP_ISP_Tuning_GetAFMetricesInfo(IMPVI_NUM num, IMPISPAFMetricsInfo *metric)
 *
 * get af metrics
 *
 * @param[in]	 num						The sensor num label.
 * @param[out]	 metric						af metrics.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPAFMetricsInfo info;
 *
 * ret = IMP_ISP_Tuning_GetAFMetricesInfo(IMPVI_MAIN, &info);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAFMetricesInfo error !\n");
 *	return -1;
 * }
 * printf("af metrics:%d, af metrics alt:%d, frame num:%d\n", info.af_metrics, info.af_metrics_alt, info.af_frame_num);
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAFMetricesInfo(IMPVI_NUM num, IMPISPAFMetricsInfo *metric);

/**
 * AE exposure list attr
 */
typedef struct {
	IMPISPTuningOpsMode mode;
	uint32_t elist[5*16];	/**< Column order instruction: reserve | exposure time(us) | again | dgain | reserve */
} IMPISPAeExpListAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SetAeExpList(IMPVI_NUM num, IMPISPAeExpListAttr *attr)
 *
 * Set AE exposure list.
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetAeExpList(IMPVI_NUM num, IMPISPAeExpListAttr *attr);

/**
 * @fn int32_t IMP_ISP_Tuning_GetAeExpList(IMPVI_NUM num, IMPISPAeExpListAttr *attr)
 *
 * Get AE exposure list.
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAeExpList(IMPVI_NUM num, IMPISPAeExpListAttr *attr);

typedef struct {
		int flag;
} IMPISPFlickerFlag;

/**
 * AWB read-only attribute.
 */
typedef struct {
	unsigned int ct;			/**< AWB Current color temperature */
} IMPISPAwbOnlyReadAttr;

 /**
 * @fn int32_t IMP_ISP_Tuning_GetAwbOnlyAttr(IMPVI_NUM num, IMPISPAwbOnlyReadAttr *Attr)
 *
 * Get AWB read-only attribute.
 *
 * @param[in]	num						The sensor num label.
 * @param[out]	attr				   AWB read-only attribute.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetAwbOnlyAttr(IMPVI_NUM num, IMPISPAwbOnlyReadAttr *Attr);

/**
 * ISP AutoZoom Attribution
 */
typedef struct {
	int32_t zoom_chx_en[3];		/**< auto zoom enable for each channel */
	int32_t zoom_left[3];		/**< the start pixel in horizen on zoom area */
	int32_t zoom_top[3];		/**< the start pixel in vertical on zoom are */
	int32_t zoom_width[3];		/**< zoom area width */
	int32_t zoom_height[3];		/**< zoom area height */
} IMPISPAutoZoom;

/**
 * @fn int32_t IMP_ISP_Tuning_SetAutoZoom(IMPVI_NUM num, IMPISPAutoZoom *ispautozoom)
 *
 * set auto zoom attribution
 *
 * @param[in] num				The sensor num label.
 * @param[in] ispautozoom		auto zoom attribution
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPAutoZoom zoom;
 *
 * zoom.zoom_chx_en[0] = 1;
 * zoom.zoom_left[0] = 10;
 * zoom.zoom_top[0] = 10;
 * zoom.zoom_width[0] = 640;
 * zoom.zoom_height[0] = 480;
 *
 * ret = IMP_ISP_Tuning_SetAutoZoom(IMPVI_MAIN, &zoom);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetAutoZoom error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetAutoZoom(IMPVI_NUM num, IMPISPAutoZoom *ispautozoom);

/**
 * @fn int32_t IMP_ISP_Tuning_GetAutoZoom(IMPVI_NUM num, IMPISPAutoZoom *ispautozoom)
 *
 * set auto zoom attribution
 *
 * @param[in]  num				 The sensor num label.
 * @param[out] ispautozoom		 auto zoom attribution
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int i;
 * int ret = 0;
 * IMPISPAutoZoom zoom;
 *
 * ret = IMP_ISP_Tuning_GetAutoZoom(IMPVI_MAIN, &zoom);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetAutoZoom error !\n");
 *	return -1;
 * }
 * for(i=0; i<3; i++){
 *		if (zoom.zoom_chx_en[i]) {
 *				printf("ch:%d, left:%d, top:%d, width:%d, height:%d\n", i,
 *				zoom.zoom_left[i], zoom.zoom_top[i], zoom.zoom_width[i], zoom.zoom_height[i]);
 *		}
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetAutoZoom(IMPVI_NUM num, IMPISPAutoZoom *ispautozoom);

/**
 * @fn int32_t IMP_ISP_Tuning_SetMaskBlock(IMPVI_NUM num, IMPISPMaskBlockAttr *mask)
 *
 * Set Mask Attr.
 *
 * @param[in] num		The sensor num label.
 * @param[in] mask		Mask attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPMaskBlockAttr block;
 *
 * if (en) {
 *		block.chx = 0;
 *		block.pinum = 0;
 *		block.mask_en = 1;
 *		block.mask_pos_top = 10;
 *		block.mask_pos_left = 100;
 *		block.mask_width = 200;
 *		block.mask_height = 200;
 *		block.mask_type = IMPISP_MASK_TYPE_YUV;
 *		block.mask_value.ayuv.y_value = 100;
 *		block.mask_value.ayuv.u_value = 100;
 *		block.mask_value.ayuv.v_value = 100;
 * } else {
 *		block.mask_en = 0;
 * }
 *
 * ret = IMP_ISP_Tuning_SetMaskBlock(IMPVI_MAIN, &block);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetMaskBlock error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetMaskBlock(IMPVI_NUM num, IMPISPMaskBlockAttr *mask);

/**
 * @fn int32_t IMP_ISP_Tuning_GetMaskBlock(IMPVI_NUM num, IMPISPMaskBlockAttr *mask)
 *
 * Get Mask Attr.
 *
 * @param[in]	num			The sensor num label.
 * @param[out]	mask		Mask attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPMaskBlockAttr attr;
 *
 * attr.chx = 0;
 * attr.pinum = 0;
 * ret = IMP_ISP_Tuning_GetMaskBlock(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetMaskBlock error !\n");
 *	return -1;
 * }
 * printf("chx:%d, pinum:%d, en:%d\n", attr.chx, attr.pinum, attr.mask_en);
 * if (attr.mask_en) {
 *		printf("top:%d, left:%d ...\n", ...);
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */

/**
 * OSD picture type
 */
typedef enum {
	IMP_ISP_PIC_ARGB_8888,	/**< ARGB8888 */
	IMP_ISP_PIC_ARGB_1555,	/**< ARBG1555 */
	IMP_ISP_PIC_ARGB_1100,	/**< AC 2bit */
} IMPISPPICTYPE;

/**
 * OSD type
 */
typedef enum {
	IMP_ISP_ARGB_TYPE_BGRA = 0,
	IMP_ISP_ARGB_TYPE_GBRA,
	IMP_ISP_ARGB_TYPE_BRGA,
	IMP_ISP_ARGB_TYPE_RBGA,
	IMP_ISP_ARGB_TYPE_GRBA,
	IMP_ISP_ARGB_TYPE_RGBA,

	IMP_ISP_ARGB_TYPE_ABGR = 8,
	IMP_ISP_ARGB_TYPE_AGBR,
	IMP_ISP_ARGB_TYPE_ABRG,
	IMP_ISP_ARGB_TYPE_AGRB,
	IMP_ISP_ARGB_TYPE_ARBG,
	IMP_ISP_ARGB_TYPE_ARGB,
} IMPISPARGBType;


/**
 * OSD 2bit RGB value
 */
typedef struct {
	uint8_t osd_value_r[4];
	uint8_t osd_value_g[4];
	uint8_t osd_value_b[4];
	uint8_t osd_value_alpha[4];
} tisp_osd_2bit_value;

/**
 * OSD channel attribution
 */
typedef struct {
	IMPISPPICTYPE osd_type;							/**< OSD picture type */
	IMPISPARGBType osd_argb_type;					/**< OSD argb type */
	IMPISPTuningOpsMode osd_pixel_alpha_disable;	/**< OSD pixel alpha disable function enable */
	tisp_osd_2bit_value osd_rgb_value;
	uint8_t osd_overlap;
	uint8_t osd_group;							    /**< Two groups in total [0~1] */
} IMPISPOSDAttr;

/**
 * @fn int32_t IMP_ISP_GetOSDAttr(IMPVI_NUM num, IMPISPOSDAttr *attr)
 *
 * Get osd Attr.
 *
 * @param[in]	num			The sensor num label.
 * @param[out]	attr		osd attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPOSDAttr attr;
 *
 * ret = IMP_ISP_Tuning_GetOSDAttr(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetOSDAttr error !\n");
 *	return -1;
 * }
 * printf("type:%d, argb_type:%d, mode:%d\n", attr.osd_type,
 * attr.osd_argb_type, attr.osd_pixel_alpha_disable);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_GetOSDAttr(IMPVI_NUM num, IMPISPOSDAttr *attr);

/**
 * @fn int32_t IMP_ISP_SetOSDAttr(IMPVI_NUM num, IMPISPOSDAttr *attr)
 *
 * Set osd Attr.
 *
 * @param[in]	num			The sensor num label.
 * @param[in]	attr		osd attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPOSDAttr attr;
 *
 * attr.osd_type = IMP_ISP_PIC_ARGB_8888;
 * attr.osd_argb_type = IMP_ISP_ARGB_TYPE_BGRA;
 * attr.osd_pixel_alpha_disable = IMPISP_TUNING_OPS_MODE_ENABLE;
 *
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_SetOSDAttr error !\n");
 *	return -1;
 * }
 * printf("type:%d, argb_type:%d, mode:%d\n", attr.osd_type,
 * attr.osd_argb_type, attr.osd_pixel_alpha_disable);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_SetOSDAttr(IMPVI_NUM num, IMPISPOSDAttr *attr);

/**
 * @fn int32_t IMP_ISP_SetOSDBlock(IMPVI_NUM num, IMPISPOSDBlockAttr *attr)
 *
 * Set osd Attr.
 *
 * @param[in]	num			The sensor num label.
 * @param[in]	attr		osd block attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPOSDBlockAttr block;
 *
 * block.pinum = pinum;
 * block.osd_enable = enable;
 * block.osd_left = left / 2 * 2;
 * block.osd_top = top / 2 * 2;
 * block.osd_width = width;
 * block.osd_height = height;
 * block.osd_image = image;
 * block.osd_stride = stride;
 *
 * ret = IMP_ISP_Tuning_SetOSDBlock(IMPVI_MAIN, &block);
 * if(ret){
 *	imp_log_err(tag, "imp_isp_tuning_setosdblock error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetOSDBlock(IMPVI_NUM num, IMPISPOSDBlockAttr *attr);

/**
 * @fn int32_t IMP_ISP_GetOSDBlock(IMPVI_NUM num, IMPISPOSDBlockAttr *attr)
 *
 * Get osd Attr.
 *
 * @param[in]	num			The sensor num label.
 * @param[out]	attr		osd block attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPOSDBlockAttr attr;
 *
 * attr.pinum = 0;
 * ret = IMP_ISP_Tuning_GetOSDBlock(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetOSDBlock error !\n");
 *	return -1;
 * }
 * printf("pinum:%d, en:%d\n", attr.pinum, attr.osd_enable);
 * if (attr.osd_enable) {
 *		printf("top:%d, left:%d ...\n", ...);
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetOSDBlock(IMPVI_NUM num, IMPISPOSDBlockAttr *attr);


/**
 * @fn int32_t IMP_ISP_Tuning_SetDrawBlock(IMPVI_NUM num, IMPISPDrawBlockAttr *attr)
 *
 * Get draw Attr.
 *
 * @param[in]	num			The sensor num label.
 * @param[out]	attr		draw attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * IMPISPDrawBlockAttr block;
 *
 * block.pinum = pinum;
 * block.type = (IMPISPDrawType)2;
 * block.color_type = (IMPISP_MASK_TYPE)ctype;
 * block.cfg.line.enable = en;
 * block.cfg.line.startx = left / 2 * 2;
 * block.cfg.line.starty = top / 2 * 2;
 * block.cfg.line.endx = w / 2 * 2;
 * block.cfg.line.endy = h / 2 * 2;
 * block.cfg.line.color.ayuv.y_value = y;
 * block.cfg.line.color.ayuv.u_value = u;
 * block.cfg.line.color.ayuv.v_value = v;
 * block.cfg.line.width = lw / 2 * 2;
 * block.cfg.line.alpha = alpha;
 * IMP_ISP_Tuning_SetDrawBlock(IMPVI_MAIN, &block);
 *
 * IMPISPDrawBlockAttr block;
 *
 * block.pinum = pinum;
 * block.type = (IMPISPDrawType)0;
 * block.color_type = (IMPISP_MASK_TYPE)ctype;
 * block.cfg.wind.enable = en;
 * block.cfg.wind.left = left / 2 * 2;
 * block.cfg.wind.top = top / 2 * 2;
 * block.cfg.wind.width = w / 2 * 2;
 * block.cfg.wind.height = h / 2 * 2;
 * block.cfg.wind.color.ayuv.y_value = y;
 * block.cfg.wind.color.ayuv.u_value = u;
 * block.cfg.wind.color.ayuv.v_value = v;
 * block.cfg.wind.line_width = lw / 2 * 2;
 * block.cfg.wind.alpha = alpha;
 *
 * IMP_ISP_Tuning_SetDrawBlock(IMPVI_MAIN, &block);
 * IMPISPDrawBlockAttr block;
 *
 * block.pinum = pinum;
 * block.type = (IMPISPDrawType)1;
 * block.color_type = (IMPISP_MASK_TYPE)ctype;
 * block.cfg.rang.enable = en;
 * block.cfg.rang.left = left / 2 * 2;
 * block.cfg.rang.top = top / 2 * 2;
 * block.cfg.rang.width = w / 2 * 2;
 * block.cfg.rang.height = h / 2 * 2;
 * block.cfg.rang.color.ayuv.y_value = y;
 * block.cfg.rang.color.ayuv.u_value = u;
 * block.cfg.rang.color.ayuv.v_value = v;
 * block.cfg.rang.line_width = lw / 2 * 2;
 * block.cfg.rang.alpha = alpha;
 * block.cfg.rang.extend = extend / 2 * 2;
 *
 * IMP_ISP_Tuning_SetDrawBlock(IMPVI_MAIN, &block);
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SetDrawBlock(IMPVI_NUM num, IMPISPDrawBlockAttr *attr);

/**
 * @fn int32_t IMP_ISP_Tuning_GetDrawBlock(IMPVI_NUM num, IMPISPDrawBlockAttr *attr)
 *
 * Set draw Attr.
 *
 * @param[in]	num			The sensor num label.
 * @param[in]	attr		draw attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPDrawBlockAttr attr;
 *
 * attr.pinum = 0;
 * ret = IMP_ISP_Tuning_GetDrawBlock(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetDrawBlock error !\n");
 *	return -1;
 * }
 * printf("pinum:%d, type:%d, color type:%d\n", attr.pinum, attr.type, attr.color_type);
 * switch (attr.type) {
 *		case IMP_ISP_DRAW_WIND:
 *			printf("enable:%d\n", attr.wind.enable);
 *			if (attr.wind.enable) {
 *				printf("left:%d, ...\n", ...);
 *			}
 *			break;
 *		case IMP_ISP_DRAW_RANGE:
 *			printf("enable:%d\n", attr.rang.enable);
 *			if (attr.rang.enable) {
 *				printf("left:%d, ...\n", ...);
 *			}
 *			break;
 *		case IMP_ISP_DRAW_LINE:
 *			printf("enable:%d\n", attr.line.enable);
 *			if (attr.line.enable) {
 *				printf("left:%d, ...\n", ...);
 *			}
 *			break;
 *		default:
 *			break;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_GetDrawBlock(IMPVI_NUM num, IMPISPDrawBlockAttr *attr);

/**
 * Tuning bin file function properties.
 */
typedef struct {
	IMPISPTuningOpsMode enable;	 /**< Switch bin function switch */
    IMPISPBinMode mode;          /**< Switch bin mode selection */
	char bname[128];			 /**< The absolute path to the bin file */
} IMPISPBinAttr;

/**
 * @fn int32_t IMP_ISP_Tuning_SwitchBin(IMPVI_NUM num, IMPISPBinAttr *attr)
 *
 * Switch to bin file.
 *
 * @param[in] num	   The label corresponding to the sensor.
 * @param[in] attr	   The bin file properties to switch.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPBinAttr attr;
 * char name[] = "/etc/sensor/xxx-PRJ007-xxx.bin"
 *
 * attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
 * attr.mode = 0;
 * memcpy(attr.bname, name, sizeof(name));
 * ret = IMP_ISP_Tuning_SwitchBin(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SwitchBin error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using that function, make sure that ISP is working properly.
 */
int32_t IMP_ISP_Tuning_SwitchBin(IMPVI_NUM num, IMPISPBinAttr *attr);

/**
 * WDR output mode.
 */
typedef enum {
	IMPISP_WDR_OUTPUT_MODE_FUS_12BIT_FRAME,
	IMPISP_WDR_OUTPUT_MODE_LONG_FRAME,
	IMPISP_WDR_OUTPUT_MODE_SHORT_FRAME,
	IMPISP_WDR_OUTPUT_MODE_FUS_16BIT_FRAME,
	IMPISP_WDR_OUTPUT_MODE_BUTT,
} IMPISPWdrOutputMode;

/**
 * @fn int32_t IMP_ISP_Tuning_SetWdrOutputMode(IMPVI_NUM num, IMPISPWdrOutputMode *mode)
 *
 * Set the WDR image output mode.
 *
 * @param[in] num	The label corresponding to the sensor.
 * @param[in] mode	output mode.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPWdrOutputMode mode;
 *
 * mode = IMPISP_WDR_OUTPUT_MODE_FUS_FRAME;
 * ret = IMP_ISP_Tuning_SetWdrOutputMode(IMPVI_MAIN, &mode);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetWdrOutputMode error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetWdrOutputMode(IMPVI_NUM num, IMPISPWdrOutputMode *mode);

/**
 * @fn int32_t IMP_ISP_Tuning_GetWdrOutputMode(IMPVI_NUM num, IMPISPWdrOutputMode *mode)
 *
 * Get the WDR image output mode.
 *
 * @param[in] num	The label corresponding to the sensor.
 * @param[out] mode	output mode.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPWdrOutputMode mode;
 *
 * ret = IMP_ISP_Tuning_GetWdrOutputMode(IMPVI_MAIN, &mode);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_GetWdrOutputMode error !\n");
 *	return -1;
 * }
 * printf("wdr output mode:%d\n", mode);
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_GetWdrOutputMode(IMPVI_NUM num, IMPISPWdrOutputMode *mode);

/**
 * frame drop parameter.
 */
typedef struct {
	IMPISPTuningOpsMode enable;	/**< enbale mark */
		uint8_t lsize;			/**< sum (range:0~31) */
		uint32_t fmark;			/**< bit mark(1 output,0 drop) */
} IMPISPFrameDrop;

/**
 * frame drop attr.
 */
typedef struct {
	IMPISPFrameDrop fdrop[3];	/**< frame drop parameters for each channel */
} IMPISPFrameDropAttr;

/**
 * @fn int32_t IMP_ISP_SetFrameDrop(IMPVI_NUM num, IMPISPFrameDropAttr *attr)
 *
 * Set frame drop attr.
 *
 * @param[in] num	The label corresponding to the sensor.
 * @param[in] attr	Frame drop attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Every time (lsize+1) is accepted, (fmark invalid figure) is lost.
 * @remark Example:lsize=3,fmark=0x5(Frames 2 and 4 are lost every 4).
 *
 * @code
 * int i;
 * int ret = 0;
 * IMPISPFrameDropAttr attr;
 *
 * for (i=0; i<3; i++) {
 *	   if (en[i]) {
 *		   attr.fdrop[i].enable = IMPISP_TUNING_OPS_MODE_ENABLE;
 *		   attr.fdrop[i].lsize = 3;
 *		   attr.fdrop[i].fmark = 0x5;
 *	   } else {
 *		   attr.fdrop[i].enable = IMPISP_TUNING_OPS_MODE_DISABLE;
 *	   }
 *
 * }
 * ret = IMP_ISP_SetFrameDrop(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_SetFrameDrop error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_Open' is working properly.
 */
int32_t IMP_ISP_SetFrameDrop(IMPVI_NUM num, IMPISPFrameDropAttr *attr);

/**
 * @fn int32_t IMP_ISP_GetFrameDrop(IMPVI_NUM num, IMPISPFrameDropAttr *attr)
 *
 * Get frame drop attr.
 *
 * @param[in] num	The label corresponding to the sensor.
 * @param[out] attr	Frame drop attr.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark Every time (lsize+1) is accepted, (fmark invalid figure) is lost.
 * @remark Example:lsize=3,fmark=0x5(Frames 2 and 4 are lost every 4).
 *
 * @code
 * int i;
 * int ret = 0;
 * IMPISPFrameDropAttr attr;
 *
 * ret = IMP_ISP_GetFrameDrop(IMPVI_MAIN, &attr);
 * if(ret){
 *	   IMP_LOG_ERR(TAG, "IMP_ISP_GetFrameDrop error !\n");
 *	   return -1;
 * }
 * for (i=0; i<3; i++) {
 *	   printf("ch:%d, enable:%d\n", i, attr.fdrop[i].enable);
 *	   if (attr.fdrop[i].enable) {
 *		   printf("lsize:%d, fmark:0x%x\n", ...);
 *	   }
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_Open' is working properly.
 */
int32_t IMP_ISP_GetFrameDrop(IMPVI_NUM num, IMPISPFrameDropAttr *attr);

/**
 * Scaler mode
 */
typedef enum {
		IMPISP_SCALER_FITTING_CRUVE,
		IMPISP_SCALER_FIXED_WEIGHT,
		IMPISP_SCALER_BUTT,
} IMPISPScalerMode;

/**
 * Scaler effect params
 */
typedef struct {
		uint8_t chx;		/*channel 0~2*/
		IMPISPScalerMode mode;	/*scaler method*/
		uint8_t level;		/*scaler level range 0~128*/
} IMPISPScalerLvAttr;

/**
 * @fn IMP_ISP_Tuning_SetScalerLv(IMPVI_NUM num, IMPISPScalerLvAttr *attr)
 *
 * Set Scaler method and level.
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] mscaler opt.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 * IMPISPScalerLvAttr attr;
 *
 * attr.chx = 0;
 * attr.mode = IMPISP_SCALER_FIXED_WEIGHT;
 * attr.level = 64;
 * ret = IMP_ISP_Tuning_SetScalerLv(IMPVI_MAIN, &attr);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_Tuning_SetScalerLv error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_Tuning_SetScalerLv(IMPVI_NUM num, IMPISPScalerLvAttr *attr);

/**
 * @fn IMP_ISP_StartNightMode(IMPVI_NUM num)
 *
 * Start night mode.
 *
 * @param[in] num  The label corresponding to the sensor.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * int ret = 0;
 *
 * ret = IMP_ISP_StartNightMode(IMPVI_MAIN);
 * if(ret){
 *	IMP_LOG_ERR(TAG, "IMP_ISP_StartNightMode error !\n");
 *	return -1;
 * }
 * @endcode
 *
 * @attention Using range: IMP_ISP_AddSensor before.
 */
int32_t IMP_ISP_StartNightMode(IMPVI_NUM num);

/**
 * ISP internal channel type
 */
typedef enum {
	TX_ISP_INTERNAL_CHANNEL_ISP_DMA0,	/**< ISP channel 0 (Obtain only. Setting is invalid) */
	TX_ISP_INTERNAL_CHANNEL_ISP_DMA1,	/**< ISP channel 1 (Obtain only. Setting is invalid) */
	TX_ISP_INTERNAL_CHANNEL_ISP_DMA2,	/**< ISP channel 2 (Obtain only. Setting is invalid) */
	TX_ISP_INTERNAL_CHANNEL_ISP_IR,		/**< ISP IR channel */
	TX_ISP_INTERNAL_CHANNEL_VIC_DMA0,	/**< ISP bypass channel 0 */
	TX_ISP_INTERNAL_CHANNEL_VIC_DMA1,	/**< ISP bypass channel 1 */
	TX_ISP_INTERNAL_CHANNEL_VIC_BUTT,	/**< effect paramater, parameters have to be less than this value */
} IMPISPInternalChnType;

/**
 * ISP internal channel
 */
typedef struct {
	IMPISPInternalChnType type;	/**< ISP internal channel selection */
	uint8_t vc_index;			/**< MIPI virtual channel in ISP bypass mode */
} IMPISPInternalChn;

/**
 * ISP internal channel attr
 */
typedef struct {
	IMPISPInternalChn ch[3];	/**< Internal channel attributes corresponding to each ISP output channel */
} IMPISPInternalChnAttr;

/**
 * @fn int32_t IMP_ISP_SetInternalChnAttr(IMPVI_NUM num, IMPISPInternalChnAttr *attr)
 *
 * Set channel attr
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr	channel attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The ISP DMA channel type can be obtained only. The setting does not take effect.
 * @remark An internal channel cannot be bound to multiple external channels.
 * @remark Please refer to 'sample-ISP-InternalChn.c' for use.
 *
 * @attention Before using it, use 'IMP_ISP_GetInternalChnAttr' to get the attribute.
 */
int32_t IMP_ISP_SetInternalChnAttr(IMPVI_NUM num, IMPISPInternalChnAttr *attr);

/**
 * @fn int32_t IMP_ISP_GetInternalChnAttr(IMPVI_NUM num, IMPISPInternalChnAttr *attr)
 *
 * Get channel attr.
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[out] attr	channel attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark The ISP DMA channel type can be obtained only. The setting does not take effect.
 * @remark Please refer to 'sample-ISP-InternalChn.c' for use.
 *
 * @attention Before using it, make sure that 'IMP_ISP_Open' is working properly.
 */
int32_t IMP_ISP_GetInternalChnAttr(IMPVI_NUM num, IMPISPInternalChnAttr *attr);

#define MAXSUPCHNMUN 4	/*ISPOSD supports up to four channel*/
#define MAXISPOSDPIC 8	/*ISPOSD supports the maximum number of pictures drawn on each channel. Currently, it only supports 8 pictures at most**/

/**
 * Regional status
 */
typedef enum {
	IMP_ISP_OSD_RGN_FREE,	/*ISPOSD region not created or released*/
	IMP_ISP_OSD_RGN_BUSY,	/*ISPOSD region created*/
}IMPIspOsdRngStat;

/**
 *Mode selection
 */
typedef enum {
		ISP_OSD_REG_INV		  = 0, /**< undefined */
			ISP_OSD_REG_PIC		  = 1, /**< ISP draws pictures*/
}IMPISPOSDType;
typedef struct IMPISPOSDNode IMPISPOSDNode;

/**
 * Fill Function Channel Properties
 */
typedef struct {
		IMPISPOSDAttr chnOSDAttr; /**< Fill Function Channel Properties, There are two groups in total (PINUM 0-3 in PIC is one group, 4-7 is one group, distinguished by the group attribute of this parameter) */
		IMPISPOSDBlockAttr pic;  /**< Fill image attribute, each channel can fill up to 8 images */
} IMPISPOSDSingleAttr;

/**
 *ISPOSD attribute set
 */
typedef struct {
	IMPISPOSDType type;
	union {
		IMPISPOSDSingleAttr stsinglepicAttr;/*PIC type ISPOSD*/
	};
}IMPIspOsdAttrAsm;

/**
 * @fn int IMP_ISP_Tuning_SetOsdPoolSize(int size)
 *
 * The size of rmem memory used to create ISPOSD
 *
 * @param[in]
 *
 * @retval 0 means success.
 * @retval None 0 failure.
 *
 * @remarks None
 *
 * @attention None
 */
int IMP_ISP_Tuning_SetOsdPoolSize(int size);

/**
 * @fn int IMP_ISP_Tuning_CreateOsdRgn(int chn,IMPIspOsdAttrAsm *pIspOsdAttr)
 *
 * Create ISPOSD Region
 *
 * @param[in] Chn channel number, IMPIspOsdAttrAsm structure pointer
 *
 * @retval 0 means success.
 * @retval None 0 failure.
 *
 * @remarks None
 *
 * @attention None
 */
int IMP_ISP_Tuning_CreateOsdRgn(int chn,IMPIspOsdAttrAsm *pIspOsdAttr);

/**
 * @fn int IMP_ISP_Tuning_SetOsdRgnAttr(int chn,int handle,IMPIspOsdAttrAsm *pIspOsdAttr)
 *
 * Set the properties of the ISPOSD channel area
 *
 * @param[in] Chn channel number, IMPIspOsdAttrAsm structure pointer
 *
 * @retval 0 means success.
 * @retval None 0 failure.
 *
 * @remarks None
 *
 * @attention None
 */
int IMP_ISP_Tuning_SetOsdRgnAttr(int chn,int handle, IMPIspOsdAttrAsm *pIspOsdAttr);

/**
 * @fn int IMP_ISP_Tuning_GetOsdRgnAttr(int chn,int handle,IMPIspOsdAttrAsm *pIspOsdAttr)
 *
 * Get the properties of the ISPOSD channel area
 *
 * @param[in] Chn channel number, IMPIspOsdAttrAsm structure pointer
 *
 * @retval 0 means success.
 * @retval None 0 failure.
 *
 * @remarks None
 *
 * @attention None
 */
int IMP_ISP_Tuning_GetOsdRgnAttr(int chn,int handle, IMPIspOsdAttrAsm *pIspOsdAttr);

/**
 * @fn int IMP_ISP_Tuning_ShowOsdRgn( int chn,int handle, int showFlag)
 *
 * Set the display status of the handle in the ISPOSD channel number
 *
 * @param[in] chn channel ID, handle ID, showFlag Display status (0: display off, 1: display on)
 *
 * @retval 0 means success.
 * @retval None 0 failure.
 *
 * @remarks None
 *
 * @attention None
 */
int IMP_ISP_Tuning_ShowOsdRgn(int chn,int handle, int showFlag);

/**
 * @fn int IMP_ISP_Tuning_DestroyOsdRgn(int chn,int handle)
 *
 * Destroy the corresponding handle node in the channel
 *
 * @param[in] chn channel number, handle number
 *
 * @retval 0 means success.
 * @retval None 0 failure.
 *
 * @remarks None
 *
 * @attention None
 * */
int IMP_ISP_Tuning_DestroyOsdRgn(int chn,int handle);

/**
 * @fn int32_t IMP_ISP_SetVicDoneCbFunc(void (*cb)(void))
 *
 * When a frame of data is accepted, the callback function will be executed.
 *
 * @param[in] cb	The pointer for callback function.
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_SetVicDoneCbFunc(void (*cb)(void));

typedef enum {
	IMPISP_TUNING_CSCCR_DEFAULT_MODE,	 /**< Default mode*/
	IMPISP_TUNING_CSCCR_STRETCH_MODE,	 /**< STRETCH Mode*/
	IMPISP_TUNING_CSCCR_COMPRESS_MODE,	 /**< COMPRESS Mode*/
	IMPISP_TUNING_CSCCR_CUSTOMER_MODE,	 /**< Custom mode*/
} IMPISPCsccrMode;

typedef struct{
	uint8_t input_y_min;
	uint8_t input_y_max;
	uint8_t output_y_min;
	uint8_t output_y_max;
	uint8_t input_uv_min;
	uint8_t input_uv_max;
	uint8_t output_uv_min;
	uint8_t output_uv_max;
}IMPISPCsccrValue;

typedef struct {
	IMPISPCsccrMode mode;
	IMPISPCsccrValue parameters;
}IMPISPCsccrModeAttr;

/**
 * @fn int32_t IMP_ISP_SetCsccrMode(IMPVI_NUM, IMPISPCscrModeAttr *attr);
 *
 * Set the working mode of CSCR (note: parameters only need to be configured in custom mode, other modes do not need to set parameters).
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr	csccr attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_SetCsccrMode(IMPVI_NUM num, IMPISPCsccrModeAttr *attr);

/**
 * @fn int32_t IMP_ISP_GetCsccrMode(IMPVI_NUM, IMPISPCscrModeAttr *attr);
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr	csccr attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @attention Before using it, make sure that 'IMP_ISP_EnableTuning' is working properly.
 */
int32_t IMP_ISP_GetCsccrMode(IMPVI_NUM num, IMPISPCsccrModeAttr *attr);

/**
 * Wdr init mode
 */
typedef enum {
	IMPISP_TYPE_WDR_FS = 1,
	IMPISP_TYPE_WDR_DOL,
	IMPISP_TYPE_WDR_NATIVE,
} IMPISPWdrInitMode;

/**
 * Run mode
 */
typedef enum {
	IMPISP_TYPE_RUN_LINEAR,
	IMPISP_TYPE_RUN_WDR,
} IMPISPWdrRunMode;

/**
 * Wdr open attribute
 */
typedef struct {
	IMPISPWdrRunMode rmode;
	IMPISPWdrInitMode imode;
} IMPISPWdrOpenAttr;

/**
 * @fn int32_t IMP_ISP_WDR_OPEN(IMPVI_NUM num, IMPISPWdrOpenAttr *attr)
 *
 * The WDR mode can also be enabled based on the linear sensor selection configuration.
 *
 * @param[in] num	The sensor num label.
 * @param[in] attr	ISP WDR attr
 *
 * @retval means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @remark imode is used to configure the WDR mode, and rmode is used to select the boot mode.
 *
 * @attention The first call of this function must precede IMP ISP AddSensor, that is, call this function first, and then call IMP ISP AddSensor to add sensors one by one
 * @attention When booting in linear mode, the wdr cache property must be configured under the linear branch of the sensor driver check function.
 *
 * @code
 * IMPISPWdrOpenAttr wdrattr;
 *
 * memset(&wdrattr, 0, sizeof(IMPISPWdrOpenAttr));
 * wdrattr.rmode = IMPISP_TYPE_RUN_LINEAR;
 * wdrattr.imode = IMPISP_TYPE_WDR_DOL;
 * ret = IMP_ISP_WDR_OPEN(IMPVI_MAIN, &wdrattr);
 * printf("\n==> sdk IMP_ISP_WDR_OPEN\n");
 */
int32_t IMP_ISP_WDR_OPEN(IMPVI_NUM num, IMPISPWdrOpenAttr *attr);

/**
 * @fn int32_t IMP_ISP_Tuning_GetAeBv(IMPVI_NUM num, int *bv)
 *
 * get AE BV.
 *
 * @param[in] num Indicates the label of the sensor
 * @param[out] bv	BV value
 *
 * @retval 0 Success
 * @retval non-0 fails, returns an error code
 *
 * @attentionBefore this function is used, IMP_ISP_EnableTuning is called.
 */
int32_t IMP_ISP_Tuning_GetAeBv(IMPVI_NUM num, int *bv);

/**
 * AWB color temperature tendency adjustment.
 */
typedef struct {
	uint16_t thres_hm;	/**< The medium-high CT threshold of AWB color temperature tendency adjustment, greater than the threshold for high color temperature, lower than the threshold for medium color temperature */
	uint16_t thres_lm;	/**< The medium and low CT threshold of AWB color temperature tendency adjustment, greater than the threshold is the medium color temperature, lower than the threshold is the low color temperature */
	IMPISPAWBGain gain_h; /**< AWB color temperature tendency to adjust the high color temperature range */
	IMPISPAWBGain gain_m; /**< AWB color temperature tendency to adjust the middle color temperature segment */
	IMPISPAWBGain gain_l; /**< AWB color temperature tendency to adjust the low color temperature segment */
} IMPISPAwbCtTrendOffset;

/**
 * @fn int32_t IMP_ISP_Tuning_SetAwbCtTrendOffset(IMPVI_NUM num, IMPISPAwbCtTrendOffset *offset)
 *
 * Set AWB color temperature tendency adjustment.
 *
 * @param[in] num Indicates the label of the sensor
 * @param[in] offset offset parameter
 *
 * @retval 0 Success
 * @retval non-0 fails, returns an error code
 *
 * @attentionBefore this function is used, IMP_ISP_EnableTuning is called.
 */
int32_t IMP_ISP_Tuning_SetAwbCtTrendOffset(IMPVI_NUM num, IMPISPAwbCtTrendOffset *offset);

/**
 * @fn int32_t IMP_ISP_Tuning_GetAwbCtTrendOffset(IMPVI_NUM num, IMPISPAwbCtTrendOffset *offset)
 *
 * Get AWB color temperature tendency adjustment.
 *
 * @param[in] num Indicates the label of the sensor
 * @param[out] offset offset parameter
 *
 * @retval 0 Success
 * @retval non-0 fails, returns an error code
 *
 * @attentionBefore this function is used, IMP_ISP_EnableTuning is called.
 */
int32_t IMP_ISP_Tuning_GetAwbCtTrendOffset(IMPVI_NUM num, IMPISPAwbCtTrendOffset *offset);

/**
 * LDC param
 */
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
	uint32_t reserve[32];
} IMPISPLDCParams;

/**
 * LDC priority
 */
typedef enum {
	IMPISP_LDC_PRIOY_FIRST,		/**< high priority */
	IMPISP_LDC_PRIOY_SECOND,	/**< moderate priority */
	IMPISP_LDC_PRIOY_THIRD,		/**< low priority */
	IMPISP_LDC_PRIOY_BUTT,		/**< effect paramater, parameters have to be less than this value*/
} IMPISPLDCPriorityType;

/**
 * LDC init channel attr
 */
typedef struct {
	IMPISPOpsMode mode;				/**< function switch */
	IMPISPOpsMode enable;			/**< enable switch */
	IMPISPLDCPriorityType prioy;	/**< priority */
	IMPISPLDCParams params;			/**< params */
} IMPISPLDCInitChAttr;

/**
 * LDC init attr
 */
typedef struct {
	IMPISPLDCInitChAttr cattr[3];
} IMPISPLDCInitAttr;

/**
 * @fn int32_t IMP_ISP_LDC_INIT(IMPVI_NUM num, IMPISPLDCInitAttr *attr)
 *
 * LDC module init
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * IMPISPLDCInitAttr attr;
 * int32_t ret = 0;
 *
 * attr.cattr[0].mode = IMPISP_OPS_MODE_ENABLE;
 * attr.cattr[0].enable = IMPISP_OPS_MODE_ENABLE;
 * attr.cattr[0].prioy = IMPISP_LDC_PRIOY_FIRST;
 * memcpy(&attr.cattr[0].params, &ldc_default_params[0], sizeof(IMPISPLDCParams));
 *
 * attr.cattr[1].mode = IMPISP_OPS_MODE_ENABLE;
 * attr.cattr[1].enable = IMPISP_OPS_MODE_ENABLE;
 * attr.cattr[1].prioy = IMPISP_LDC_PRIOY_FIRST;
 * memcpy(&attr.cattr[1].params, &ldc_default_params[0], sizeof(IMPISPLDCParams));
 *
 * ret = IMP_ISP_LDC_INIT(IMPVI_MAIN, &attr);
 * @endcode
 *
 * @attention Called before IMP_ISP_Open.
 */
int32_t IMP_ISP_LDC_INIT(IMPVI_NUM num, IMPISPLDCInitAttr *attr);

/**
 * LDC channel attr
 */
typedef struct {
	IMPISPOpsMode enable;
	IMPISPLDCPriorityType prioy;
	IMPISPLDCParams params;
} IMPISPLDCChannelAttr;

/**
 * LDC attr
 */
typedef struct {
	IMPISPLDCChannelAttr cattr[3];
} IMPISPLDCAttr;

/**
 * @fn int32_t int32_t IMP_ISP_LDC_GetAttr(IMPVI_NUM num, IMPISPLDCAttr *attr)
 *
 * Get LDC attr.
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * IMPISPLDCAttr attr;
 * int32_t ret = 0;
 *
 * ret = IMP_ISP_LDC_GetAttr(IMPVI_MAIN, &attr);
 * @endcode
 *
 * @attention Called after IMP_ISP_Open.
 */
int32_t IMP_ISP_LDC_GetAttr(IMPVI_NUM num, IMPISPLDCAttr *attr);

/**
 * @fn int32_t int32_t IMP_ISP_LDC_SetAttr(IMPVI_NUM num, IMPISPLDCAttr *attr)
 *
 * Set LDC attr.
 *
 * @param[in] num  The label corresponding to the sensor.
 * @param[in] attr attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * IMPISPLDCAttr attr;
 * int32_t ret = 0;
 *
 * ret = IMP_ISP_LDC_GetAttr(IMPVI_MAIN, &attr);
 *
 * attr.cattr[0].enable = IMPISP_OPS_MODE_ENABLE;
 * attr.cattr[0].prioy = IMPISP_LDC_PRIOY_FIRST;
 * memcpy(&attr.cattr[0].params, &ldc_default_params[0], sizeof(IMPISPLDCParams));
 *
 * attr.cattr[1].enable = IMPISP_OPS_MODE_ENABLE;
 * attr.cattr[1].prioy = IMPISP_LDC_PRIOY_FIRST;
 * memcpy(&attr.cattr[1].params, &ldc_default_params[0], sizeof(IMPISPLDCParams));
 *
 * ret = IMP_ISP_LDC_SetAttr(IMPVI_MAIN, &attr);
 * @endcode
 *
 * @attention Called after IMP_ISP_Open.
 */
int32_t IMP_ISP_LDC_SetAttr(IMPVI_NUM num, IMPISPLDCAttr *attr);


typedef  tisp_mode_t IMPISPPMAttr;

/**
 * @fn int32_t IMP_ISP_PM_SetMode(IMPVI_NUM num, IMPISPPMAttr *attr);
 *
 * Set the PM mode.
 *
 * @param[in] num	The label corresponding to the sensor.
 * @param[in] attr	attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * @endcode
 *
 * @attention Single and dual cameras only need to be set up once, and in the case of dual cameras, num only needs to be configured as 0
 */
int32_t IMP_ISP_PM_SetMode(IMPVI_NUM num, IMPISPPMAttr *attr);

/**
 * @fn int32_t IMP_ISP_PM_GetMode(IMPVI_NUM num, IMPISPPMAttr *attr);
 *
 * Get the PM mode.
 *
 * @param[in] num	The label corresponding to the sensor.
 * @param[in] attr	attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * @endcode Single and dual cameras only need to be set up once, and in the case of dual cameras, num only needs to be configured as 0
 *
 * @attention
 */
int32_t IMP_ISP_PM_GetMode(IMPVI_NUM num, IMPISPPMAttr *attr);

/**
 * @fn int32_t IMP_ISP_PM_WaitAllDone(void);
 *
 * Waiting for the signal indicating that "all done" has been reached.
 *
 * @param[in] attr	attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * @endcode
 *
 * @attention
 */
int32_t IMP_ISP_PM_WaitAllDone(IMPISPOpsMode *enable);

/**
 * @fn int32_t IMP_ISP_HB_GetAttr(IMPVI_NUM num, uint16_t *time);
 *
 * Get the hb size between primary and secondary shots in the case of double shots
 *
 * @param[in] num   The label corresponding to the sensor.
 * @param[in] attr  attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * @endcode
 *
 * @attention
 */
int32_t IMP_ISP_HB_GetAttr(IMPVI_NUM num, uint16_t *time);

/**
 * @fn int32_t IMP_ISP_HB_SetAttr(IMPVI_NUM num, uint16_t *time);
 *
 * Set the hb size between primary and secondary shots in the case of double shots
 *
 * @param[in] num   The label corresponding to the sensor.
 * @param[in] attr  attr
 *
 * @retval 0 means success.
 * @retval Other values mean failure, its value is an error code.
 *
 * @code
 * @endcode
 *
 * @attention
 */
int32_t IMP_ISP_HB_SetAttr(IMPVI_NUM num, uint16_t *time);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* __IMP_ISP_H__ */


