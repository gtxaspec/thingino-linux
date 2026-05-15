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

#if 1

enum IR_STATUS{
    IR_STATUS_DAY = 0,  //白天模式
    IR_STATUS_NIGHT = 1,//晚上模式
    IR_STATUS_LED = 2,  //白光灯模式
    IR_STATUS_IGNORE=3,
};

static char *g_ir_status[] = {"day","night","led","ignore"};

extern int sensor_number;
// 声明全局变量
extern unsigned long rmem_base; /* rmem 内存基地址 */
extern unsigned long high_framerate_kernel_mode_en;
static int g_is_night = IR_STATUS_DAY;
struct isp_buf_info g_ncu_buf;
struct isp_buf_info g_ncu_1_buf;
struct isp_buf_info g_dual_buf;
struct isp_buf_info g_dual_1_buf;

#define GPIO_IRCUT_N -1//GPIO_PB(28)
#define GPIO_IRCUT_P -1//GPIO_PB(18)

static int ircut_gpio_init(void)
{
	int ret = 0;

	if(GPIO_IRCUT_N > 0) {
		ret = gpio_request_one(GPIO_IRCUT_N, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_N");
		if(ret < 0) {
			printk("gpio_request_one GPIO_IRCUT_N failed\n");
			return -1;
		}
		gpio_direction_output(GPIO_IRCUT_N, 0);
	}

	if(GPIO_IRCUT_P > 0) {
		ret = gpio_request_one(GPIO_IRCUT_P, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "IR_P");
		if(ret < 0) {
			printk("gpio_request_one GPIO_IRCUT_P failed\n");
			return -1;
		}

		gpio_direction_output(GPIO_IRCUT_P, 0);
	}

	return 0;
}

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
	printk("TTFF %s %d W:%d H:%d N:%d\n", __func__, __LINE__, (int)frame_channel_width, (int)frame_channel_height, (int)frame_channel_nrvbs);


	if (sensor_number == 2) {
		// 打开新的通道
		ret = frame_channel3_open(NULL, NULL);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_open failed\n");
		}

		chan = IS_ERR_OR_NULL(g_f3_mdev) ? NULL : miscdev_to_frame_chan(g_f3_mdev);
		if(IS_ERR_OR_NULL(chan)){
			printk(KERN_ERR "chan is null\n");
		}

		// 设置通道格式，分辨率，裁剪缩放等

		memset(&format, 0x0, sizeof(struct frame_image_format));

		format.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
		format.pix.field = TISP_FIELD_ANY;
		if(frame_channel_width) {
			format.pix.width = frame_channel_width;
			format.crop_width = frame_channel_width;
			format.scaler_out_width = frame_channel_width;
		} else {
			format.pix.width = FRAME_CHANNEL_DEF_WIDTH;
			format.crop_width = FRAME_CHANNEL_DEF_WIDTH;
			format.scaler_out_width = FRAME_CHANNEL_DEF_WIDTH;
		}

		if(frame_channel_height) {
			format.pix.height = frame_channel_height;
			format.crop_height = frame_channel_height;
			format.scaler_out_height = frame_channel_height;
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
		// 设置通道buffer

		memset(&req, 0, sizeof(struct tisp_requestbuffers));
		if(frame_channel_nrvbs) {
			req.count = frame_channel_nrvbs; /*nrVBs*/
		} else {
			req.count = TX_ISP_NRVBS; /*nrVBs*/
		}
		req.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = TISP_MEMORY_USERPTR;

		//chan->index = 3;
		ret = frame_channel_reqbufs(chan, (unsigned long)&req, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_reqbufs failed\n");
		}

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
			struct tisp_buffer buf1;
			memset(&buf1, 0, sizeof(struct tisp_buffer));

			buf1.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
			buf1.memory = TISP_MEMORY_USERPTR;
			buf1.index = buf_i;
			buf1.m.userptr = addr + (count + buf_i) * (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 3 / 2);
			buf1.length = (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 3 / 2);

			chan->index = 3;
			ret = frame_channel_vb2_qbuf(chan, (unsigned long)&buf1, K_MODE);
			if(ret != 0) {
				printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
			}
		}

		type = TISP_BUF_TYPE_VIDEO_CAPTURE;

		// 启动出流
		ret = frame_channel_vb2_streamon(chan, (unsigned long)&type, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vb2_streamon failed\n");
		}
	}

	return 0;
}

#if 1
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


	// 打开新的通道
	ret = frame_channel_open(NULL, NULL);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_open failed\n");
	}

	chan = IS_ERR_OR_NULL(g_f0_mdev) ? NULL : miscdev_to_frame_chan(g_f0_mdev);
	if(IS_ERR_OR_NULL(chan)){
		printk(KERN_ERR "chan is null\n");
	}

	// 设置通道格式，分辨率，裁剪缩放等

	memset(&format, 0x0, sizeof(struct frame_image_format));

	format.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	format.pix.field = TISP_FIELD_ANY;
	if(frame_channel_width) {
		format.pix.width = frame_channel_width;
		format.crop_width = frame_channel_width;
		format.scaler_out_width = frame_channel_width;
	} else {
		format.pix.width = FRAME_CHANNEL_DEF_WIDTH;
		format.crop_width = FRAME_CHANNEL_DEF_WIDTH;
		format.scaler_out_width = FRAME_CHANNEL_DEF_WIDTH;
	}

	if(frame_channel_height) {
		format.pix.height = frame_channel_height;
		format.crop_height = frame_channel_height;
		format.scaler_out_height = frame_channel_height;
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
	// 设置通道buffer
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

	// 启动出流
	ret = frame_channel_vb2_streamon(chan, (unsigned long)&type, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vb2_streamon failed\n");
	}

	return 0;
}
#endif


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
	struct tisp_input inputt;
	struct tisp_input inputt1;

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
		s_mode.joint_mode = 0;
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

		//set main sensor Dual Sensor buf
		g_dual_buf.vinum = 0;
		ret = tx_isp_dualsensor_get_buf(module, (unsigned long)&g_dual_buf, K_MODE);
		if (ret < 0) {
			printk("tx_isp_get_dual_sensor_buf failed\n");
			return ret;
		}
		g_dual_buf.paddr = rmem_base + ALIGN_SIZE(g_ncu_buf.size);
		printk("Main sensor Dual Sensor: size = %d paddr = 0x%x\n", g_dual_buf.size, g_dual_buf.paddr);	
		sprintf(dual_buf_len,"len=%d", g_dual_buf.size);
		ret = tx_isp_dualsensor_set_buf(module, (unsigned long)&g_dual_buf, K_MODE);
		if(ret < 0) {
			printk("tx_isp_set_dual_sensor_buf failed\n");
			return ret;
		}

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

#if 1
		// Set second sensor NCU buf
		g_ncu_1_buf.vinum = 1;
		ret = tx_isp_get_mdns_buf(module, (unsigned long)&g_ncu_1_buf, K_MODE);
		if(ret < 0) {
			printk("tx_isp_get_buf failed\n");
			return ret;
		}
		g_ncu_1_buf.paddr = g_dual_buf.paddr + ALIGN_SIZE(g_dual_buf.size);

		printk("Second sensor NCU: size = %d  paddr = 0x%x\n", g_ncu_1_buf.size, g_ncu_1_buf.paddr);
		sprintf(ncu_1_buf_len,"len=%d", g_ncu_1_buf.size);
		ret = tx_isp_set_mdns_buf(module, (unsigned long)&g_ncu_1_buf, K_MODE);
		if(ret < 0) {
			printk("tx_isp_set_buf failed\n");
			return ret;
		}


		//set second sensor Dual Sensor buf
		g_dual_1_buf.vinum = 1;
		ret = tx_isp_dualsensor_get_buf(module, (unsigned long)&g_dual_1_buf, K_MODE);
		if (ret < 0) {
			printk("tx_isp_get_dualsensor_buf failed\n");
			return ret;
		}
		g_dual_1_buf.paddr = g_ncu_1_buf.paddr + ALIGN_SIZE(g_ncu_1_buf.size);
		printk("Second sensor Dual Sensor: size = %d paddr = 0x%x\n", g_dual_1_buf.size, g_dual_1_buf.paddr);	
		sprintf(dual_1_buf_len,"len=%d", g_dual_1_buf.size);
		ret = tx_isp_dualsensor_set_buf(module, (unsigned long)&g_dual_1_buf, K_MODE);
		if(ret < 0) {
			printk("tx_isp_set_dualsensor_buf failed\n");
			return ret;
		}
#endif

	}
//	if(g_riscv_isp_gain > 1024) {
	//	tx_isp_tuning_set_sharpness(50); /* 有效 */
	//	tx_isp_tuning_set_sinter_strength(255); /* 有效 */
//	}

	// set start AWB
	//tx_isp_set_awb_start(g_awb_start); /* 有效 */

	//tx_isp_tuning_set_brightness(255); /* 有效 */
	//tx_isp_tuning_set_contrast(255); /* 有效 */
	//tx_isp_tuning_set_sharpness(50); /* 有效 */
	//tx_isp_tuning_set_saturation(255); /* 有效 */
	//tx_isp_tuning_set_ae_it_max(100); /* SetFPS 之后被刷新 */
	//tx_isp_tuning_set_hv_flip(ISP_CORE_FLIP_HV_MODE); /* 有效 */
	//tx_isp_tuning_set_max_again(1024);/* SetFPS 之后被刷新 */
	//tx_isp_tuning_set_max_dgain(1024); /* 有效 */

	//tx_isp_tuning_set_max_dgain(126); /* 有效 */

	// 切换白天夜视模式
	//isp_core_tuning_switch_day_or_night(0); /* 有效 */

	//tx_isp_tuning_set_sinter_strength(255); /* 有效 */

	// 切换白天夜视模式
	if(g_is_night == IR_STATUS_NIGHT) {
//		isp_core_tuning_switch_day_or_night(0);
	}

	// IRCUT 切回idle状态
//	ircut_gpio_init();
	// 创建 ir_status 节点
	proc_create("ir_status", S_IRUGO, NULL, &ir_status_proc_fops);

	// Stream on
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
	
	if (sensor_number == 2) {
		frame_channel_fast_start(g_dual_1_buf.paddr + ALIGN_SIZE(g_dual_1_buf.size));
	} else {
		frame_channel_fast_start(g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size));
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

		frame_channel3_fast_start(g_dual_1_buf.paddr + ALIGN_SIZE(g_dual_1_buf.size));
	}

	
	return 0;
}

static int tx_isp_frame_done_int_handler(int count)
{
	if(count == 1) {
	//	tx_isp_tuning_set_fps(15, 1); /* 最终实现在中断下文中实现 */
	}

	return 0;
}

#endif

struct tx_isp_callback_ops g_tx_isp_callback_ops = {
	//.tx_isp_riscv_hf_prepare = tx_isp_riscv_hf_prepare,
	.tx_isp_riscv_hf_resize = tx_isp_riscv_hf_resize,
	.tx_isp_frame_done_int_handler = tx_isp_frame_done_int_handler,
};
