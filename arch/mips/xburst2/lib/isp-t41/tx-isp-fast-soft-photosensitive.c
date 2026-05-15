#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/fs.h>

#include "include/fast_start_common.h"
#include "include/tx-isp-frame-channel.h"
#include "include/tx-isp-device.h"
#include "include/tx-isp-tuning.h"

#define IR_STATUS_ON "on"
#define IR_STATUS_OFF "off"
extern int tisp_enable_tuning(void);
enum IR_STATUS{
    IR_STATUS_DAY = 0,  //day mode
    IR_STATUS_NIGHT = 1,//night mode
    IR_STATUS_LED = 2,  //black linght mode
    IR_STATUS_IGNORE=3,
};

static char *g_ir_status[] = {"day","night","led","ignore"};
static int g_is_night = IR_STATUS_DAY;
extern int sensor_number;

//Declare global variables
extern unsigned long rmem_base; /* rmem base address */
extern unsigned long high_framerate_kernel_mode_en;
struct isp_buf_info g_ncu_buf;
struct isp_buf_info g_ncu_1_buf;
struct isp_buf_info g_dual_sensor_buf;

/**
 * IRLED
 * -1: ignore
 * 0: high level lights on, low level lights off
 * 1: high level lights off, low level lights on
 */
#define LED_MODE_IGNORE -1
#define LED_MODE_HIGH_ON 0
#define LED_MODE_LOW_ON  1

/**
 * IRCUT
 * -1: ignore
 * 0: Level trigger mode, P High level and N Low level Switch night mode
 * 1: Level trigger mode, N High level and P Low level Switch night mode
 * 2: Edge trigger mode, Rising edge trigger Switch night mode
 * 3: Edge trigger mode, Falling edge trigger Switch night mode
 */
#define IRCUT_MODE_IGNORE -1
#define IRCUT_MODE_LEVEL_TRIGGER_HIGH 0
#define IRCUT_MODE_LEVEL_TRIGGER_LOW  1
#define IRCUT_MODE_EDGE_TRIGGER_RISING  2
#define IRCUT_MODE_EDGE_TRIGGER_FALLING 3

/**
 * ADC threshold direction
 * 0: less than the threshold is night mode 
 * 1: more than the threshold is night mode
 */
#define ADC_DIRECTION_0 0
#define ADC_DIRECTION_1 1

struct vol_start_value_table {
    unsigned int vol0;
    unsigned int value;
};

struct vol_start_value_table_info {
    unsigned int head;
    int len_day;
    int len_night;
    unsigned int crc;
    /**
     * IRLED
     * -1: ignore
     * 0: high level light on, low level light off
     * 1: high level light off, low level light on
     */
    int gpio_led_mode;
    int gpio_led;

    int gpio_ircut_mode;
    int gpio_ircut_p;
    int gpio_ircut_n;
    int gpio_ircut_edge;
    int adc_value; /* Day and night mode switching threshold, this value is less than 0, not controlled*/
    int adc_direction; /* 0: less than the threshold is night mode, 1: greater than the threshold is night mode*/
    int adc_reference; /* The reference voltage collected by ad, the default is 1800, it can be set according to the actual voltage value*/
    int gpio_white_led_status;
    int gpio_white_led;
    unsigned char data[0]; /* Just a way for get data */
};

// AE Table Param
extern unsigned long ir_switch_mode;
extern int sensor_start_ae_table_init(void);
extern int get_sensor_start_ae_table(void);
extern int get_sec_sensor_start_ae_table(void);
static struct vol_start_value_table_info *g_start_ae_table_p = NULL;
static int g_gpio_led_mode = LED_MODE_HIGH_ON;
static int g_gpio_white_led_status = -1;
static int g_gpio_white_led = -1;
static int g_gpio_led = -1;
static int g_gpio_ircut_mode = IRCUT_MODE_LEVEL_TRIGGER_HIGH;
static int g_gpio_ircut_p = -1;
static int g_gpio_ircut_n = -1;
static int g_gpio_ircut_edge = -1;
static int g_adc_value = -1;
static int g_adc_reference = -1;
static int g_adc_direction = ADC_DIRECTION_0;
//second camera
static struct vol_start_value_table_info *g_sec_start_ae_table_p = NULL;
static int g_sec_gpio_led_mode = LED_MODE_HIGH_ON;
static int g_sec_gpio_white_led_status = -1;
static int g_sec_gpio_white_led = -1;
static int g_sec_gpio_led = -1;
static int g_sec_gpio_ircut_mode = IRCUT_MODE_LEVEL_TRIGGER_HIGH;
static int g_sec_gpio_ircut_p = -1;
static int g_sec_gpio_ircut_n = -1;
static int g_sec_gpio_ircut_edge = -1;
static int g_sec_adc_value = -1;
static int g_sec_adc_reference = -1;
static int g_sec_adc_direction = ADC_DIRECTION_0;

static ssize_t ir_fops_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned int count = (unsigned int)size;
    unsigned int len = (strlen(g_ir_status[g_is_night] + *ppos) < size) ? (strlen(g_ir_status[g_is_night] + *ppos)) : size;
    ssize_t ret;

    ret = copy_to_user(buf, (void *)(g_ir_status[g_is_night] + *ppos), len);
    if(ret != 0) {
        return ret;
    }

    *ppos += len;

    return len;
}

static const struct file_operations ir_status_proc_fops ={
    .read = ir_fops_read,
};

static int frame_channel3_fast_start(unsigned int addr)
{
	int ret = 0;
	struct tx_isp_frame_channel *chan=NULL;
	struct frame_image_format format;
	struct tisp_requestbuffers req;
	int count = 0;
	int buf_i = 0;
	enum tisp_buf_type type;
	addr = addr + frame_channel_nrvbs * (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 3 / 2);
	printk("TTFF %s %d W:%d H:%d N:%d\n", __func__, __LINE__, (int)frame_channel3_width, (int)frame_channel3_height, (int)frame_channel3_nrvbs);

	if (sensor_number == 2) {
		// open new frame 3 channel
		ret = frame_channel3_open(NULL, NULL);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_open failed\n");
		}

		chan = IS_ERR_OR_NULL(g_f3_mdev) ? NULL : miscdev_to_frame_chan(g_f3_mdev);
		if(IS_ERR_OR_NULL(chan)){
			printk(KERN_ERR "chan is null\n");
		}

		//Set channel format, resolution, crop zoom, etc.

		memset(&format, 0x0, sizeof(struct frame_image_format));

		format.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
		format.pix.field = TISP_FIELD_ANY;
		if(frame_channel3_width) {
			format.pix.width = frame_channel3_width;
			format.crop_width = frame_channel3_width;
			format.scaler_out_width = frame_channel3_width;
		} else {
			format.pix.width = FRAME_CHANNEL_DEF_WIDTH;
			format.crop_width = FRAME_CHANNEL_DEF_WIDTH;
			format.scaler_out_width = FRAME_CHANNEL_DEF_WIDTH;
		}

		if(frame_channel3_height) {
			format.pix.height = frame_channel3_height;
			format.crop_height = frame_channel3_height;
			format.scaler_out_height = frame_channel3_height;
		} else {
			format.pix.height = FRAME_CHANNEL_DEF_HEIGHT;
			format.crop_height = FRAME_CHANNEL_DEF_HEIGHT;
			format.scaler_out_height = FRAME_CHANNEL_DEF_HEIGHT;
		}
		format.pix.pixelformat = TISP_VO_FMT_YUV_SEMIPLANAR_420;//NV12
		format.crop_enable = 0;
		format.crop_top = 0;
		format.crop_left = 0;
		format.scaler_enable = 1;
		format.pix.colorspace = TISP_COLORSPACE_SRGB;
		format.rate_bits = 0;
		format.rate_mask = 1;

		printk("chan-index = %d\n", chan->index);
		ret = frame_channel_vidioc_set_fmt(chan, (unsigned long)&format, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vidioc_set_fmt failed\n");
		}
		/*printk("set_fmt after sizesize = %d\n", chan->vbq.format.fmt.pix.sizeimage);*/
		
		//set framesource buffer
		memset(&req, 0, sizeof(struct tisp_requestbuffers));
		if(frame_channel3_nrvbs) {
			req.count = frame_channel3_nrvbs; /*nrVBs*/
		} else {
			req.count = TX_ISP_NRVBS; /*nrVBs*/
		}
		req.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = TISP_MEMORY_USERPTR;

		ret = frame_channel_reqbufs(chan, (unsigned long)&req, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_reqbufs failed\n");
		}

		if(frame_channel3_nrvbs) {
			count = frame_channel3_nrvbs; /*nrVBs*/
		} else {
			count = TX_ISP_NRVBS; /*nrVBs*/
		}

		ret = frame_channel_set_channel_banks(chan, (unsigned long)&count, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_set_channel_banks failed\n");
		}

		for(buf_i = 0; buf_i < count; buf_i++) {
			struct tisp_buffer buf1;
			memset(&buf1, 0, sizeof(struct tisp_buffer));

			buf1.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
			buf1.memory = TISP_MEMORY_USERPTR;
			buf1.index = buf_i;
			buf1.m.userptr = addr + buf_i * (frame_channel3_width * ((frame_channel3_height + 0xF) & ~0xF) * 3 / 2);
			buf1.length = (frame_channel3_width * ((frame_channel3_height + 0xF) & ~0xF) * 3 / 2);

			chan->index = 3;
			ret = frame_channel_vb2_qbuf(chan, (unsigned long)&buf1, K_MODE);
			if(ret != 0) {
				printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
			}
		}
		type = TISP_BUF_TYPE_VIDEO_CAPTURE;

		//Start stream on
		ret = frame_channel_vb2_streamon(chan, (unsigned long)&type, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vb2_streamon failed\n");
		}
	}

	return 0;
}

static int frame_channel_fast_start(unsigned int addr)
{
	int ret = 0;
	struct tx_isp_frame_channel *chan=NULL;
	struct frame_image_format format;
	struct tisp_requestbuffers req;
	int count = 0;
	int buf_i = 0;
	enum tisp_buf_type type;
	printk("TTFF %s %d W:%d H:%d N:%d\n", __func__, __LINE__, (int)frame_channel_width, (int)frame_channel_height, (int)frame_channel_nrvbs);

	//open new frame 0 channel
	ret = frame_channel_open(NULL, NULL);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_open failed\n");
	}

	chan = IS_ERR_OR_NULL(g_f0_mdev) ? NULL : miscdev_to_frame_chan(g_f0_mdev);
	if(IS_ERR_OR_NULL(chan)){
		printk(KERN_ERR "chan is null\n");
	}

	//Set channel format, resolution, crop zoom, etc.
	memset(&format, 0x0, sizeof(struct frame_image_format));

	format.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	format.pix.field = TISP_FIELD_ANY;
	if(frame_channel_width) {
		format.pix.width = frame_channel_width;
		format.crop_width = frame_channel_width;
		format.scaler_out_width = main_scaler_width;//frame_channel_width;
	} else {
		format.pix.width = FRAME_CHANNEL_DEF_WIDTH;
		format.crop_width = FRAME_CHANNEL_DEF_WIDTH;
		format.scaler_out_width = FRAME_CHANNEL_DEF_WIDTH;
	}

	if(frame_channel_height) {
		format.pix.height = frame_channel_height;
		format.crop_height = frame_channel_height;
		format.scaler_out_height = main_scaler_height;//frame_channel_height;
	} else {
		format.pix.height = FRAME_CHANNEL_DEF_HEIGHT;
		format.crop_height = FRAME_CHANNEL_DEF_HEIGHT;
		format.scaler_out_height = FRAME_CHANNEL_DEF_HEIGHT;
	}
	format.pix.pixelformat = TISP_VO_FMT_YUV_SEMIPLANAR_420;//NV12
	format.crop_enable = 0;
	format.crop_top = 0;
	format.crop_left = 0;
	format.scaler_enable = 1;
	format.pix.colorspace = TISP_COLORSPACE_SRGB;
	format.rate_bits = 0;
	format.rate_mask = 1;

	printk("chan-index = %d\n", chan->index);
	ret = frame_channel_vidioc_set_fmt(chan, (unsigned long)&format, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vidioc_set_fmt failed\n");
	}

	/*printk("set_fmt after sizesize = %d\n", chan->vbq.format.fmt.pix.sizeimage);*/
	// set frame channel buffer
	memset(&req, 0, sizeof(struct tisp_requestbuffers));
	if(frame_channel_nrvbs) {
		req.count = frame_channel_nrvbs; /*nrVBs*/
	} else {
		req.count = TX_ISP_NRVBS; /*nrVBs*/
	}
	req.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = TISP_MEMORY_USERPTR;

	ret = frame_channel_reqbufs(chan, (unsigned long)&req, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_reqbufs failed\n");
	}
	/*printk("reqbuf after sizesize = %d\n", chan->vbq.format.fmt.pix.sizeimage);*/

	if(frame_channel_nrvbs) {
		count = frame_channel_nrvbs; /*nrVBs*/
	} else {
		count = TX_ISP_NRVBS; /*nrVBs*/
	}
	ret = frame_channel_set_channel_banks(chan, (unsigned long)&count, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_set_channel_banks failed\n");
	}

	for(buf_i = 0; buf_i < count; buf_i++) {
		struct tisp_buffer buf;
		memset(&buf, 0, sizeof(struct tisp_buffer));

		buf.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = TISP_MEMORY_USERPTR;
		buf.index = buf_i;
		buf.m.userptr = addr + buf_i * (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 3 / 2);
		buf.length = (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 3 / 2);

		printk("buf_id = %d  buf_ptr = %x  buf_len = %d\n",buf.index, buf.m.userptr, buf.length);
		ret = frame_channel_vb2_qbuf(chan, (unsigned long)&buf, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
		}
	}
	type = TISP_BUF_TYPE_VIDEO_CAPTURE;

	//Start stream on
	ret = frame_channel_vb2_streamon(chan, (unsigned long)&type, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vb2_streamon failed\n");
	}

	return 0;
}


static int tx_isp_riscv_hf_prepare(void)
{
	int vinum;
	int enable;
	int ret = 0;

	tx_isp_discard_frame(0xfffffffc); /* main camera discards the first two frames*/
	if(sensor_number == 2)
		sec_tx_isp_discard_frame(0xfffffffc); /*second camera discards the first two frames*/

	// enable debug info per frame
	vinum = 0;
	//tx_isp_enable_debug(vinum, 1, 50);
	if (sensor_number == 2){
		vinum = 1;
		//tx_isp_enable_debug(vinum, 1, 50);
	}

	// prepare AE table in advance
	sensor_start_ae_table_init();

	//Soft photosensitive operation needs to turn off the sensor exposure parameter settings for the first few frames.
	tx_isp_disable_sensor_expo();
	
	return 0;
}

static int tx_isp_riscv_hf_resize(void)
{
	struct tx_isp_module *module = g_isp_module;
	struct tx_isp_module * submod = NULL;
	struct tx_isp_subdev *sd;
	int ret = 0;
	int mode = 0;
	struct tx_isp_device *ispdev = NULL;
	struct tx_isp_subdev *subdev = NULL;
	struct tx_isp_sensor_register_info sensor_register_info;
	struct tx_isp_initarg init;
	int index = 0;
	int input = 0;
	int link = 0;
	struct msensor_mode s_mode;
	struct tisp_anfiflicker_attr_t flicker_attr;
	struct tisp_input inputt;
	struct tisp_input inputt1;
	int vi_num;

	if(g_isp_module == NULL){
		printk("Error: %s module is NULL\n", __func__);
		return 0;
	}
	submod = module->submods[0];
	sd = module_to_subdev(submod);
	ispdev = module_to_ispdev(g_isp_module);

	ispdev->active_link[0] = 0;
	ispdev->active_link[1] = 0;
	ispdev->active_link[2] = 0;
	for(index = 0; index < TX_ISP_ENTITY_ENUM_MAX_DEPTH; index++){
		submod = module->submods[index];
		if(submod){
			subdev = module_to_subdev(submod);
			ret = tx_isp_subdev_call(subdev, internal, activate_module);
			if(ret && ret != -ENOIOCTLCMD)
				break;
		}
	}
	
	//set dual sensor mode
	if (sensor_number == 2) {
		s_mode.sensor_num = 2;
		s_mode.dual_mode = 4;
		s_mode.joint_mode = Joint_mode;//IMPISP_MAIN_ON_THE_ABOVE;
		s_mode.dmode_switch.en = 0;
		ret = tx_isp_dualsensor_mode(module, (unsigned long)&s_mode , K_MODE);
		if(ret < 0) {
			printk("set dualsensor_mode failed\n");
			return ret;
		}
	}

	// Register sensor
	memset(&sensor_register_info, 0, sizeof(struct tx_isp_sensor_register_info));
	strcpy(sensor_register_info.name, get_sensor_name());
	sensor_register_info.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
	strcpy(sensor_register_info.i2c.type, get_sensor_name());
	sensor_register_info.i2c.addr = get_sensor_i2c_addr();
	sensor_register_info.i2c.i2c_adapter_id = 1;
	sensor_register_info.rst_gpio = 91;//PC27
	sensor_register_info.pwdn_gpio = -1;
	sensor_register_info.power_gpio = -1;
	sensor_register_info.video_interface = TISP_SENSOR_VI_MIPI_CSI0;
	sensor_register_info.mclk = TISP_SENSOR_MCLK1;
	sensor_register_info.default_boot = 0;//need match sensor driver
	sensor_register_info.sensor_id = 0;
	printk("Main sensor name = %s   i2c = 0x%x\n",sensor_register_info.name,sensor_register_info.i2c.addr);
	ret = tx_isp_sensor_register_sensor(module,(unsigned long)&sensor_register_info, K_MODE);
	if(ret < 0) {
		printk("tx_isp_sensor_register_sensor failed\n");
		return ret;
	}

	inputt.index = 0;
	ret = tx_isp_sensor_enum_input(module, (unsigned long)&inputt, K_MODE);
	if(ret < 0) {
		printk("tx_isp_sensor_enum_input failed\n");
		return ret;
	}

	// Set Input
	init.vinum = 0;
	init.enable = 1;
	ret = tx_isp_sensor_set_input(module, (unsigned long)&init, K_MODE);
	if(ret < 0) {
		printk("tx_isp_sensor_set_input failed\n");
		return ret;
	}

	// Set NCU buf
	g_ncu_buf.vinum = 0;
	ret = tx_isp_get_mdns_buf(module, (unsigned long)&g_ncu_buf, K_MODE);
	if(ret < 0) {
		printk("tx_isp_get_buf failed\n");
		return ret;
	}
	g_ncu_buf.paddr = rmem_base;

	printk("Main sensor NCU: size = %d  paddr = 0x%x\n", g_ncu_buf.size, g_ncu_buf.paddr);
	sprintf(ncu_buf_len,"len=%d", g_ncu_buf.size);
	ret = tx_isp_set_mdns_buf(module, (unsigned long)&g_ncu_buf, K_MODE);
	if(ret < 0) {
		printk("tx_isp_set_buf failed\n");
		return ret;
	}

	//Dual Sensor mode
	if (sensor_number == 2) {
		// Register second sensor
		memset(&sensor_register_info, 0, sizeof(struct tx_isp_sensor_register_info));
		strcpy(sensor_register_info.name, get_sensor1_name());
		sensor_register_info.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
		strcpy(sensor_register_info.i2c.type, get_sensor1_name());
		sensor_register_info.i2c.addr = get_sensor1_i2c_addr();
		sensor_register_info.i2c.i2c_adapter_id = 3;
		sensor_register_info.rst_gpio = 92;//PC28
		sensor_register_info.pwdn_gpio = -1;
		sensor_register_info.power_gpio = -1;
		sensor_register_info.video_interface = TISP_SENSOR_VI_MIPI_CSI1;
		sensor_register_info.mclk = TISP_SENSOR_MCLK2;
		sensor_register_info.default_boot = 0;//need match sensor driver
		sensor_register_info.sensor_id = 1;
		printk("Second sensor name = %s   i2c = 0x%x\n",sensor_register_info.name,sensor_register_info.i2c.addr);

		sprintf(sub_sensor_w,"len=%d", get_sensor1_width());
		sprintf(sub_sensor_h,"len=%d", get_sensor1_height());
		ret = tx_isp_sensor_register_sensor(module,(unsigned long)&sensor_register_info, K_MODE);
		if(ret < 0) {
			printk("tx_isp_sensor_register_sensor failed\n");
			return ret;
		}
		inputt1.index = 1;
		ret = tx_isp_sensor_enum_input(module, (unsigned long)&inputt1, K_MODE);
		if(ret < 0) {
			printk("tx_isp_sensor_enum_input failed\n");
			return ret;
		}

		// Set second sensor Input
		init.vinum = 1;//second sensor
		init.enable = 1;
		ret = tx_isp_sensor_set_input(module, (unsigned long)&init, K_MODE);
		if(ret < 0) {
			printk("tx_isp_sensor_set_input failed\n");
			return ret;
		}
		
		// Set second sensor NCU buf
		g_ncu_1_buf.vinum = 1;
		ret = tx_isp_get_mdns_buf(module, (unsigned long)&g_ncu_1_buf, K_MODE);
		if(ret < 0) {
			printk("tx_isp_get_buf failed\n");
			return ret;
		}
		//g_ncu_1_buf.paddr = g_dual_buf.paddr + ALIGN_SIZE(g_dual_buf.size);
		g_ncu_1_buf.paddr = g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size);

		printk("Second sensor NCU: size = %d  paddr = 0x%x\n", g_ncu_1_buf.size, g_ncu_1_buf.paddr);
		sprintf(ncu_1_buf_len,"len=%d", g_ncu_1_buf.size);
		ret = tx_isp_set_mdns_buf(module, (unsigned long)&g_ncu_1_buf, K_MODE);
		if(ret < 0) {
			printk("tx_isp_set_buf failed\n");
			return ret;
		}

		//set second sensor Dual Sensor buf
		g_dual_sensor_buf.vinum = 1;
		ret = tx_isp_dualsensor_get_buf(module, (unsigned long)&g_dual_sensor_buf, K_MODE);
		if (ret < 0) {
			printk("tx_isp_get_dualsensor_buf failed\n");
			return ret;
		}
		g_dual_sensor_buf.paddr = g_ncu_1_buf.paddr + ALIGN_SIZE(g_ncu_1_buf.size);
		printk("sensor Dual Sensor: size = %d paddr = 0x%x\n", g_dual_sensor_buf.size, g_dual_sensor_buf.paddr);
		sprintf(dual_sensor_buf_len,"len=%d", g_dual_sensor_buf.size);
		ret = tx_isp_dualsensor_set_buf(module, (unsigned long)&g_dual_sensor_buf, K_MODE);
		if(ret < 0) {
			printk("tx_isp_set_dualsensor_buf failed\n");
			return ret;
		}
	}

	/*enable isp tunning, can set some function*/
	tisp_enable_tuning();
	
	//set anfiflicker
#if 0 /*Set up as required, not set by default*/
	vi_num = 0;
	flicker_attr.mode = ISP_ANTIFLICKER_AUTO_MODE;
	flicker_attr.freq = 50;
	tx_isp_tuning_set_flicker(vi_num, &flicker_attr);
	if(sensor_number == 2){
		vi_num = 1;
		flicker_attr.mode = ISP_ANTIFLICKER_AUTO_MODE;
		flicker_attr.freq = 50;
		tx_isp_tuning_set_flicker(vi_num, &flicker_attr);
	}
#endif

	/** 
	 * soft photosensitive operation needs to close the dgain operation. 
	 * Prevent the influence of dgain.
	 */
	tx_isp_tunning_main_set_max_dgain(0);
	if(sensor_number == 2)
		tx_isp_tunning_sec_set_max_dgain(0);

	vi_num = 0;//main sensor
	//set flip mirror
	//tx_isp_tuning_set_hv_flip(ISP_CORE_FLIP_HV_MODE, vi_num); 
	// set day or night mode
	//isp_core_tuning_switch_day_or_night(0, vi_num); /* 0：night  1：day */

	if (sensor_number == 2) { //second sensor
		vi_num = 1;//second sensor
		//set flip mirror
		//tx_isp_tuning_set_hv_flip(ISP_CORE_FLIP_V_MODE, vi_num);
		// set day and night mode
		//isp_core_tuning_switch_day_or_night(0, vi_num); /* 0：night  1：day*/
	}

	// create ir_status node
	proc_create("ir_status", S_IRUGO, NULL, &ir_status_proc_fops);

	// Main sensor stream on
	init.vinum = 0;
	init.enable = 1;
	ret = tx_isp_sensor_get_input(module, (unsigned long)&init, K_MODE);
	if(ret < 0) {
		printk("tx_isp_sensor_get_input failed\n");
		return ret;
	}

	ret = tx_isp_video_s_stream(module, (unsigned int)&init, K_MODE);
	if(ret < 0) {
		printk("tx_isp_video_s_stream failed\n");
		return ret;
	}

	ret = tx_isp_video_link_setup(module, (unsigned long)&init, K_MODE);
	if(ret < 0) {
		printk("tx_isp_video_link_setup failed\n");
		return ret;
	}

	ret = tx_isp_video_link_stream(module, (unsigned int)&init, K_MODE);
	if(ret < 0) {
		printk("tx_isp_video_link_setup failed\n");
		return ret;
	}

	//Second sensor stream on
	if (sensor_number == 2){
		init.vinum = 1;
		init.enable = 1;

		ret = tx_isp_sensor_get_input(module, (unsigned long)&init, K_MODE);
		if(ret < 0) {
			printk("tx_isp_sensor_get_input failed\n");
			return ret;
		}

		ret = tx_isp_video_s_stream(module, (unsigned int)&init, K_MODE);
		if(ret < 0) {
			printk("tx_isp_video_s_stream failed\n");
			return ret;
		}

		ret = tx_isp_video_link_setup(module, (unsigned long)&init, K_MODE);
		if(ret < 0) {
			printk("tx_isp_video_link_setup failed\n");
			return ret;
		}

		ret = tx_isp_video_link_stream(module, (unsigned int)&init, K_MODE);
		if(ret < 0) {
			printk("tx_isp_video_link_setup failed\n");
			return ret;
		}
	}

	return 0;
}

static int get_ae_table_info(void)
{
    struct vol_start_value_table_info *ae_start_table = get_sensor_start_ae_table();
    uint32_t ae_table_head;
    uint8_t *ae_table_head_p = (uint8_t *)&ae_table_head;
    ae_table_head_p[0] = 'A';
    ae_table_head_p[1] = 'T';
    ae_table_head_p[2] = 'A';
    ae_table_head_p[3] = 'B';
    if (ae_start_table->head == ae_table_head) {
        g_start_ae_table_p = ae_start_table;
        g_gpio_white_led_status = ae_start_table->gpio_white_led_status;
        g_gpio_white_led = ae_start_table->gpio_white_led;
        //g_gpio_led_mode = ae_start_table->gpio_led_mode;
        g_gpio_led = ae_start_table->gpio_led;
        //g_gpio_ircut_mode = ae_start_table->gpio_ircut_mode;
        g_gpio_ircut_p = ae_start_table->gpio_ircut_p;
        g_gpio_ircut_n = ae_start_table->gpio_ircut_n;
        g_gpio_ircut_edge = ae_start_table->gpio_ircut_edge;
        g_adc_value = ae_start_table->adc_value;
        g_adc_reference = ae_start_table->adc_reference;
        g_adc_direction = ae_start_table->adc_direction;
    } else {
        printk("Invalid ae table\n");
        return -1;
    }

	return 0;
}

static int get_sec_ae_table_info(void)
{
    struct vol_start_value_table_info *ae_start_table = get_sec_sensor_start_ae_table();
    uint32_t ae_table_head;
    uint8_t *ae_table_head_p = (uint8_t *)&ae_table_head;
    ae_table_head_p[0] = 'A';
    ae_table_head_p[1] = 'T';
    ae_table_head_p[2] = 'A';
    ae_table_head_p[3] = '1';
    if (ae_start_table->head == ae_table_head) {
        g_sec_start_ae_table_p = ae_start_table;
        g_sec_gpio_white_led_status = ae_start_table->gpio_white_led_status;
        g_sec_gpio_white_led = ae_start_table->gpio_white_led;
        //g_gpio_led_mode = ae_start_table->gpio_led_mode;
        g_sec_gpio_led = ae_start_table->gpio_led;
        //g_gpio_ircut_mode = ae_start_table->gpio_ircut_mode;
        g_sec_gpio_ircut_p = ae_start_table->gpio_ircut_p;
        g_sec_gpio_ircut_n = ae_start_table->gpio_ircut_n;
        g_sec_gpio_ircut_edge = ae_start_table->gpio_ircut_edge;
        g_sec_adc_value = ae_start_table->adc_value;
        g_sec_adc_reference = ae_start_table->adc_reference;
        g_sec_adc_direction = ae_start_table->adc_direction;
    } else {
        printk("Invalid sec ae table\n");
        return -1;
    }

	return 0;
}

/**
 * get ae table param
 * addr: ae table para addr
 * vol: ADC sampled data
 * mode: 0 day、1 night
 */
static unsigned int get_ae_table_value(struct vol_start_value_table_info *addr, int vol, int mode)
{
    int i = 0;
    struct vol_start_value_table_info *info = (struct vol_start_value_table_info *)addr;
    struct vol_start_value_table *table_day = (struct vol_start_value_table *)(info->data);
    struct vol_start_value_table *table_night = (struct vol_start_value_table *)(info->data + sizeof(struct vol_start_value_table) * info->len_day);

    struct vol_start_value_table *table_tmp;
	if(mode == 0) {
		for (i = 0; i < info->len_day; i++)	{
			table_tmp = table_day + i;
			if (vol < table_tmp->vol0) {
				return table_tmp->value;
			}
		}
	} else {
		for (i = 0; i < info->len_night; i++)	{
			table_tmp = table_night + i;
			if (vol < table_tmp->vol0) {
				return table_tmp->value;
			}
		}
	}

	return table_tmp->value;
}

/**
 *check day and night mode
 * retval: 0 day mode 1 night mode 2 ignore default day mode
 */
static unsigned int check_is_day_or_night(int vol)
{
    char *p = NULL;

	//accord env info to set ir_mode
	if(ir_switch_mode == 0) {
		return 0;
	} else if(ir_switch_mode == 1) {
		return 1;
	}

	// auto mode :accord adc value to judging day or night mode
    if(g_adc_value < 0)
        return 0;

    if(g_adc_direction == 0) {
        if(vol > g_adc_value) {
            return 0;
        } else {
            return 1;
        }
    } else {
        if(vol > g_adc_value) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}

static int irled_and_ircut_init(void)
{
	int ret = 0;

	if(g_gpio_ircut_n > 0) {
		ret = gpio_request_one(g_gpio_ircut_n, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_N");
		if(ret < 0) {
			printk("gpio_request_one g_gpio_ircut_n failed\n");
			return -1;
		}
		gpio_direction_output(g_gpio_ircut_n, 0);
	}

	if(g_gpio_ircut_p > 0) {
		ret = gpio_request_one(g_gpio_ircut_p, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_P");
		if(ret < 0) {
			printk("gpio_request_one g_gpio_ircut_p failed\n");
			return -1;
		}
		gpio_direction_output(g_gpio_ircut_p, 0);
	}

	if(g_gpio_led > 0) {
		ret = gpio_request_one(g_gpio_led, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_LED");
		if(ret < 0) {
			printk("gpio_request_one g_gpio_led failed\n");
			return -1;
		}
		gpio_direction_output(g_gpio_led, 1);
	}

	if(g_gpio_ircut_edge > 0) {
		ret = gpio_request_one(g_gpio_ircut_edge, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_E");
		if(ret < 0) {
			printk("gpio_request_one g_gpio_ircut_edge failed\n");
			return -1;
		}
		gpio_direction_output(g_gpio_ircut_edge, 0);
	}

	/*second camera ircut and irled process*/
	if(sensor_number == 2){
		if(g_sec_gpio_ircut_n > 0) {
			ret = gpio_request_one(g_sec_gpio_ircut_n, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_N");
			if(ret < 0) {
				printk("gpio_request_one g_sec_gpio_ircut_n failed\n");
				return -1;
			}
			gpio_direction_output(g_sec_gpio_ircut_n, 0);
		}

		if(g_sec_gpio_ircut_p > 0) {
			ret = gpio_request_one(g_sec_gpio_ircut_p, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_P");
			if(ret < 0) {
				printk("gpio_request_one g_sec_gpio_ircut_p failed\n");
				return -1;
			}
			gpio_direction_output(g_sec_gpio_ircut_p, 0);
		}

		if(g_sec_gpio_led > 0) {
			ret = gpio_request_one(g_sec_gpio_led, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_LED");
			if(ret < 0) {
				printk("gpio_request_one g_sec_gpio_led failed\n");
				return -1;
			}
			gpio_direction_output(g_sec_gpio_led, 1);
		}

		if(g_sec_gpio_ircut_edge > 0) {
			ret = gpio_request_one(g_sec_gpio_ircut_edge, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_E");
			if(ret < 0) {
				printk("gpio_request_one g_sec_gpio_ircut_edge failed\n");
				return -1;
			}
			gpio_direction_output(g_sec_gpio_ircut_edge, 0);
		}

	}
	
	return 0;
}

static int set_ircut_idle(void)
{
	if(g_gpio_ircut_n > 0) {
		gpio_direction_output(g_gpio_ircut_n, 0);
	}

	if(g_gpio_ircut_p > 0) {
		gpio_direction_output(g_gpio_ircut_p, 0);
	}

	if(sensor_number == 2){
		if(g_sec_gpio_ircut_n > 0) {
			gpio_direction_output(g_sec_gpio_ircut_n, 0);
		}

		if(g_sec_gpio_ircut_p > 0) {
			gpio_direction_output(g_sec_gpio_ircut_p, 0);
		}
	}
}

/**
 * Set infrared light and infrared polarizer according to day and night vision mode
 */
static unsigned int set_irled_and_ircut(int is_night)
{
	// deal with irled
	if(g_gpio_led_mode == LED_MODE_HIGH_ON) {
		if(is_night) {
			if(g_gpio_led > 0) {
				gpio_direction_output(g_gpio_led, 1);
			}
		} else {
			if(g_gpio_led > 0) {
				gpio_direction_output(g_gpio_led, 0);
			}
		}
	} else if(g_gpio_led_mode == LED_MODE_LOW_ON){
		if(is_night) {
			if(g_gpio_led > 0) {
				gpio_direction_output(g_gpio_led, 0);
			}
		} else {
			if(g_gpio_led > 0) {
				gpio_direction_output(g_gpio_led, 1);
			}
		}
	}

	// deal with IRCUT
	switch(g_gpio_ircut_mode) {
		case IRCUT_MODE_LEVEL_TRIGGER_HIGH:
			{
				if(g_gpio_ircut_p > 0 && g_gpio_ircut_n > 0) {
					if(is_night) {
						gpio_direction_output(g_gpio_ircut_p, 1);
						gpio_direction_output(g_gpio_ircut_n, 0);
					} else {
						gpio_direction_output(g_gpio_ircut_p, 0);
						gpio_direction_output(g_gpio_ircut_n, 1);
					}
				}
			}
			break;
		case IRCUT_MODE_LEVEL_TRIGGER_LOW:
			{
				if(g_gpio_ircut_p > 0 && g_gpio_ircut_n > 0) {
					if(is_night) {
						gpio_direction_output(g_gpio_ircut_p, 0);
						gpio_direction_output(g_gpio_ircut_n, 1);
					} else {
						gpio_direction_output(g_gpio_ircut_p, 1);
						gpio_direction_output(g_gpio_ircut_n, 0);
					}
				}
			}
			break;
		case IRCUT_MODE_EDGE_TRIGGER_RISING:
			{
				if(g_gpio_ircut_edge > 0) {
					gpio_direction_output(g_gpio_ircut_edge, 0);
					mdelay(1);
					gpio_direction_output(g_gpio_ircut_edge, 1);
				}
			}
			break;
		case IRCUT_MODE_EDGE_TRIGGER_FALLING:
			{
				if(g_gpio_ircut_edge > 0) {
					gpio_direction_output(g_gpio_ircut_edge, 1);
					mdelay(1);
					gpio_direction_output(g_gpio_ircut_edge, 0);
				}
			}
			break;
		default:
			break;
	}

	return 0;
}

static unsigned int set_sec_irled_and_ircut(int is_night)
{
	// deal with irled
	if(g_sec_gpio_led_mode == LED_MODE_HIGH_ON) {
		if(is_night) {
			if(g_sec_gpio_led > 0) {
				gpio_direction_output(g_sec_gpio_led, 1);
			}
		} else {
			if(g_sec_gpio_led > 0) {
				gpio_direction_output(g_sec_gpio_led, 0);
			}
		}
	} else if(g_sec_gpio_led_mode == LED_MODE_LOW_ON){
		if(is_night) {
			if(g_sec_gpio_led > 0) {
				gpio_direction_output(g_sec_gpio_led, 0);
			}
		} else {
			if(g_sec_gpio_led > 0) {
				gpio_direction_output(g_sec_gpio_led, 1);
			}
		}
	}

	// IRCUT
	switch(g_sec_gpio_ircut_mode) {
		case IRCUT_MODE_LEVEL_TRIGGER_HIGH:
			{
				if(g_sec_gpio_ircut_p > 0 && g_sec_gpio_ircut_n > 0) {
					if(is_night) {
						gpio_direction_output(g_sec_gpio_ircut_p, 1);
						gpio_direction_output(g_sec_gpio_ircut_n, 0);
					} else {
						gpio_direction_output(g_sec_gpio_ircut_p, 0);
						gpio_direction_output(g_sec_gpio_ircut_n, 1);
					}
				}
			}
			break;
		case IRCUT_MODE_LEVEL_TRIGGER_LOW:
			{
				if(g_sec_gpio_ircut_p > 0 && g_sec_gpio_ircut_n > 0) {
					if(is_night) {
						gpio_direction_output(g_sec_gpio_ircut_p, 0);
						gpio_direction_output(g_sec_gpio_ircut_n, 1);
					} else {
						gpio_direction_output(g_sec_gpio_ircut_p, 1);
						gpio_direction_output(g_sec_gpio_ircut_n, 0);
					}
				}
			}
			break;
		case IRCUT_MODE_EDGE_TRIGGER_RISING:
			{
				if(g_sec_gpio_ircut_edge > 0) {
					gpio_direction_output(g_sec_gpio_ircut_edge, 0);
					mdelay(1);
					gpio_direction_output(g_sec_gpio_ircut_edge, 1);
				}
			}
			break;
		case IRCUT_MODE_EDGE_TRIGGER_FALLING:
			{
				if(g_sec_gpio_ircut_edge > 0) {
					gpio_direction_output(g_sec_gpio_ircut_edge, 1);
					mdelay(1);
					gpio_direction_output(g_sec_gpio_ircut_edge, 0);
				}
			}
			break;
		default:
			break;
	}

	return 0;
}

static unsigned int g_isp_ev = 0;
static unsigned int g_sec_isp_ev = 0;
static void tx_isp_soft_photosensitive_process(struct work_struct *work)
{
	int ret = 0;
	unsigned char main_luma = 0;
	unsigned char sec_luma = 0;
	int vi_num = 0;
	int enable;

	ret = get_ae_table_info();
	if(ret < 0) {
		printk("get_ae_table_info failed\n");
		return;
	}
	
	if(sensor_number == 2){
		ret = get_sec_ae_table_info();
		if(ret < 0) {
			printk("get_sec_ae_table_info failed\n");
			return;
		}
	}

	// get luma value
	vi_num = 0;
	ret = tx_isp_tuning_get_ae_luma(vi_num, &main_luma);
	if(ret < 0) {
		printk("tx_isp_tuning_get_ae_luma failed\n");
		return;
	}
	if(sensor_number == 2){
		vi_num = 1;
		ret = tx_isp_tuning_get_ae_luma(vi_num, &sec_luma);
		if(ret < 0) {
			printk("tx_isp_tuning_get_ae_luma failed\n");
			return;
		}
	}

	// judging day and night
	g_is_night = check_is_day_or_night(main_luma);

	// switch ISP day_or_night mode
	if(g_is_night == IR_STATUS_NIGHT) {
		vi_num = 0;
		isp_core_tuning_switch_day_or_night(0, vi_num);
		
		if(sensor_number == 2){
			vi_num = 1;
			isp_core_tuning_switch_day_or_night(0, vi_num);
		}
	}

	// deal with IRCUT and IRLED 
	irled_and_ircut_init();
	set_irled_and_ircut(g_is_night);
	if(sensor_number == 2)
		set_sec_irled_and_ircut(g_is_night);

	/**
	 *Enable sensor exposure parameter setting, 
	 *ISP count results are set to Sensor through I2C
	 */
	tx_isp_enable_sensor_expo();

	//open dgain
	tx_isp_tunning_main_set_max_dgain(196);
	if(sensor_number == 2)
		tx_isp_tunning_sec_set_max_dgain(196);

	// get start AE parameter
	g_isp_ev = get_ae_table_value(g_start_ae_table_p, main_luma, g_is_night);
	printk("%s main_luma:%d g_is_night:%d main_ev:%d\n", __func__, main_luma, g_is_night, g_isp_ev);
	if(sensor_number == 2){
		g_sec_isp_ev = get_ae_table_value(g_sec_start_ae_table_p, sec_luma, g_is_night);
		printk("%s sec_luma:%d g_is_night:%d sec_ev:%d\n", __func__, sec_luma, g_is_night, g_sec_isp_ev);
	}

	// set start EV
	vi_num = 0;
	enable = 1;
	tx_isp_set_ev_start(vi_num, enable, g_isp_ev);//main sensor set start ev
	if(sensor_number == 2){
		vi_num = 0;
		tx_isp_set_ev_start(vi_num, enable, g_sec_isp_ev);//second sensor set start EV
	}

	// set frame buffer
	if (sensor_number == 2) {
		frame_channel_fast_start(g_dual_sensor_buf.paddr + ALIGN_SIZE(g_dual_sensor_buf.size));
		frame_channel3_fast_start(g_dual_sensor_buf.paddr + ALIGN_SIZE(g_dual_sensor_buf.size));
	} else {
		frame_channel_fast_start(g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size));
	}

	// set fps 15fps
	vi_num = 0;
	tx_isp_tuning_set_fps(vi_num, 15, 1); /* main sensor set fps */
	if(sensor_number == 2){
		vi_num = 1;
		tx_isp_tuning_set_fps(vi_num, 15, 1); /* second sensor set fps */
	}
	
	msleep(50);

	// IRCUT restores the idle state, avoid to power loss and chip burning
	set_ircut_idle();
}
DECLARE_WORK(soft_photosensitive_work, tx_isp_soft_photosensitive_process);

static int tx_isp_frame0_done_int_handler(int count)
{
#if 0
	// for debug
	if(count <= 3) {
		struct isp_core_ev_attr ev_attr;
		unsigned char main_luma = 0;
		unsigned char sec_luma = 0;
		int vi_num;

		vi_num = 0;	
		tx_isp_tuning_get_ae_luma(vi_num, &main_luma);
		tx_isp_tuning_get_ev_attr(&ev_attr);
		printk("soft photosensitive main camera luma:%d ev:%d expr:%d ev_log2:%d again:%d dgain:%d\n",
				main_luma, ev_attr.ev, ev_attr.expr_us, ev_attr.ev_log2, ev_attr.again, ev_attr.dgain);
		
		if(sensor_number == 2){
			vi_num = 1;	
			tx_isp_tuning_get_ae_luma(vi_num, &sec_luma);
			tx_isp_tuning_get_ev_attr(&ev_attr);
			printk("soft photosensitive second camera luma:%d ev:%d expr:%d ev_log2:%d again:%d dgain:%d\n",
					sec_luma, ev_attr.ev, ev_attr.expr_us, ev_attr.ev_log2, ev_attr.again, ev_attr.dgain);
		}
	}
#endif

	if(count == 3) {
		schedule_work(&soft_photosensitive_work);
	}

	return 0;
}


struct tx_isp_callback_ops g_tx_isp_callback_ops = {
	.tx_isp_riscv_hf_prepare = tx_isp_riscv_hf_prepare,
	.tx_isp_riscv_hf_resize = tx_isp_riscv_hf_resize,
	.tx_isp_frame0_done_int_handler = tx_isp_frame0_done_int_handler,
};
