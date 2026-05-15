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

#if !Zeratul_Platform
/* 0：非直通（默认配置） 1：全直通 */
static int zrt_direct_mode = 1;
/* 配置 IVDC 中 DDR 存储的行数（一行表示 width，256 对齐）Y 配置 ivdc_mem_line，UV 配置 ivdc_mem_line/2  一般ivdc_mem_line = height/2 */
static int zrt_ivdc_mem_line = 1080/4;//1440/2;
/* 0：配置为 height/2，表示阈值为半帧（默认值 */
static int zrt_ivdc_threshold_line = 0;
#endif




extern int tisp_enable_tuning(void);
enum IR_STATUS{
    IR_STATUS_DAY = 0 << 0,  //白天模式
    IR_STATUS_NIGHT = 1 << 0,//晚上模式
    IR_STATUS_LED_CLOSE = 1 << 4,  //白光灯，白天
    IR_STATUS_LED_OPEN = 2 << 4, //白光灯，晚上
    IR_STATUS_IGNORE=IR_STATUS_NIGHT+IR_STATUS_LED_OPEN,
};
int fast_wdr_mode = 0;
int sec_fast_wdr_mode = 0;
static char *g_ir_status[] = {"day","night","wl_open","wl_close","ignore"};

uint32_t g_riscv_isp_ev = 0;
uint32_t g_riscv_sensor_again = 0;
uint32_t g_riscv_sensor_inttime = 0;
uint32_t g_riscv_isp_gain = 0;
uint32_t g_riscv_luma = 0;
static int g_is_night = IR_STATUS_DAY;

extern int isp_clk;
extern int direct_mode;
extern int ivdc_mem_line;
extern int ivdc_threshold_line;
extern int sensor_number;
// 声明全局变量
extern unsigned long rmem_base; /* rmem 内存基地址 */
extern unsigned long high_framerate_kernel_mode_en;
struct isp_buf_info g_ncu_buf;
struct isp_buf_info g_wdr_buf;
int riscv_is_run = 1;
int riscv_is_pass = 1;


#define GPIO_IRCUT_N -1//GPIO_PB(28)
#define GPIO_IRCUT_P -1//GPIO_PB(18)
#define SEC_GPIO_IRCUT_N -1//GPIO_PB(28)
#define SEC_GPIO_IRCUT_P -1//GPIO_PB(18)

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
    char status[20]={"0"};
    sprintf(status,"day:%d,wl_mode:%d\n",g_is_night&0x1,(g_is_night&0x30)>>4);
    unsigned int len = (strlen(status + *ppos) < size) ? (strlen(status + *ppos)): size;
    ssize_t ret;

    ret = copy_to_user(buf, (void *)(status + *ppos), len);
    if(ret != 0) {
        return ret;
    }

    *ppos += len;

    return len;
}

static const struct file_operations ir_status_proc_fops ={
    .read = ir_fops_read,
};


static int frame_channel_fast_start(unsigned int addr)
{
#if Zeratul_Platform
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

	ret = frame_channel_vidioc_set_fmt(chan, (unsigned long)&format, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vidioc_set_fmt failed\n");
	}

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
#endif
	return 0;
}


static int tx_isp_riscv_hf_prepare(void)
{
	int vinum;
	int enable;
	int ret = 0;
	int timeout = 200;
    uint32_t frame_count = 0;

#if !Zeratul_Platform  /*直通模式下内核不缓存数据*/
direct_mode = zrt_direct_mode;
ivdc_mem_line = zrt_ivdc_mem_line;
ivdc_threshold_line = zrt_ivdc_threshold_line;
#endif

	isp_clk = 300000000;
	printk("t41 set isp clk is %d\n", isp_clk);
#if TX_ISP_FAST_START
	DEBUG_TTFF("tx_isp_stop_riscv start");
	ret = tx_isp_riscv_is_run();
    if (ret < 0) {
        riscv_is_run = 0;
    } else {
#if 1
		//等待riscv固定帧数
        timeout = 1000;
        while(timeout--) {
            ret = tx_isp_get_riscv_framecount(&frame_count);
            if(ret < 0) {
                printk("Error: %s tx_isp_get_riscv_framecount failed ret:%d\n", __func__, ret);
                return ret;
            }

            if((frame_count >= 2) && (frame_count != -1))
                break;

            mdelay(1);
        }
#endif
		DEBUG_TTFF("wait riscv frame done");
#if 1
        ret = tx_isp_stop_riscv();
        if(ret < 0) {
            printk("Error: %s tx_isp_stop_riscv failed ret:%d\n", __func__, ret);
            return ret;
        }
#endif
        timeout = 1000;
        while(!tx_isp_riscv_is_stop() && timeout--) {
            mdelay(1);
        }
        DEBUG_TTFF("tx_isp_stop_riscv done");

        if(timeout <= 0) {
            printk("stop riscv timeout\n");
            riscv_is_pass = 0;
        }
    }
    ret = tx_isp_get_sensor_again(&g_riscv_sensor_again);
    if(ret < 0) {
        printk("Error: %s tx_isp_get_sensor_again failed\n", __func__);
        return ret;
    }
    ret = tx_isp_get_sensor_inttime(&g_riscv_sensor_inttime);
    if(ret < 0) {
        printk("Error: %s tx_isp_get_sensor_inttime failed\n", __func__);
        return ret;
    }
    ret = tx_isp_get_isp_gain(&g_riscv_isp_gain);
    if(ret < 0) {
        printk("Error: %s tx_isp_get_isp_gain failed\n", __func__);
        return ret;
    }
    ret = tx_isp_get_ae_luma(&g_riscv_luma);
    if(ret < 0) {
        printk("Error: %s tx_isp_get_ae_luma failed\n", __func__);
        return ret;
    }
	vinum = 0;
	ret = tx_isp_get_ev_start(vinum, &g_riscv_isp_ev);
	if(ret < 0) {
		printk("Error: %s g_riscv_isp_ev failed\n", __func__);
		return ret;
	}
	printk("Current sensor again:%d isp gain:%d inttime:%d luma:%d ev:%d\n", g_riscv_sensor_again, g_riscv_isp_gain, g_riscv_sensor_inttime, g_riscv_luma, g_riscv_isp_ev);

    /*
     *硬光敏传递状态:
     *g_is_night:
     *bit0 :0代表判断白天，1是晚上
     *bit4 :0代表判断白天关闭白光灯，1是晚上打开白光灯
     * */
	g_is_night = tx_isp_get_riscv_night_mode();
    if(g_is_night > IR_STATUS_IGNORE){
        g_is_night = IR_STATUS_IGNORE;
    }
    printk("Tuning mode is:%#x\n", g_is_night);
	//	tx_isp_discard_frame(0xfffffffc); /* 主摄丢弃前两帧 有效*/
    //tx_isp_set_ev_start(g_riscv_isp_ev);
    //tx_isp_set_sensor_again(g_riscv_sensor_again);
    //tx_isp_set_sensor_inttime(1700);
    //tx_isp_set_isp_gain(4096);

	// enable debug info per frame
	vinum = 0;
	//tx_isp_enable_debug(vinum, 1, 20);
#endif
	return 0;
}

static int tx_isp_riscv_hf_resize(void)
{
//#if Zeratul_Platform
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
	struct tisp_anfiflicker_attr_t flicker_attr;
	struct tisp_input inputt;
	struct tisp_input inputt1;
	int vi_num;
	int enable;

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

	//set wdr mode
	fast_wdr_mode = get_sensor_wdr_mode();
	printk("fast_wdr_mode = %d\n", fast_wdr_mode);

	if (fast_wdr_mode) {
		vi_num = 0;
		tx_isp_wdr_enable(module, (unsigned long)&vi_num, K_MODE);
	}


	// Register sensor
	memset(&sensor_register_info, 0, sizeof(struct tx_isp_sensor_register_info));
	strcpy(sensor_register_info.name, get_sensor_name());
	sensor_register_info.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
	strcpy(sensor_register_info.i2c.type, get_sensor_name());
	sensor_register_info.i2c.addr = get_sensor_i2c_addr();
	sensor_register_info.i2c.i2c_adapter_id = 0;
	sensor_register_info.rst_gpio = 18;//PC27
	sensor_register_info.pwdn_gpio = -1;
	sensor_register_info.power_gpio = -1;
	sensor_register_info.video_interface = TISP_SENSOR_VI_MIPI_CSI0;
	sensor_register_info.mclk = TISP_SENSOR_MCLK0;
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

	printk("NCU: size = %d  paddr = 0x%x\n", g_ncu_buf.size, g_ncu_buf.paddr);
	sprintf(ncu_buf_len,"len=%d", g_ncu_buf.size);
	ret = tx_isp_set_mdns_buf(module, (unsigned long)&g_ncu_buf, K_MODE);
	if(ret < 0) {
		printk("tx_isp_set_buf failed\n");
		return ret;
	}

	//set WDR buf
	if (fast_wdr_mode){
		g_wdr_buf.vinum = 0;
		ret = tx_isp_wdr_get_buf(module, (unsigned long)&g_wdr_buf, K_MODE);
		if(ret < 0){
			printk("tx_isp_wdr_get_buf failed\n");
			return ret;
		}

		g_wdr_buf.paddr = g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size);

		printk("main sensor wdr buf: size = %d paddr = 0x%x\n", g_wdr_buf.size, g_wdr_buf.paddr);
		sprintf(wdr_buf_len, "len=%d", g_wdr_buf.size);
		ret = tx_isp_wdr_set_buf(module, (unsigned long)&g_wdr_buf, K_MODE);
		if(ret < 0) {
			printk("main sensor tx_isp_wdr_set_buf failed\n");
			return ret;
		}
	}

	tisp_enable_tuning();

	//set start ev
	vi_num = 0;
	enable = 1;
	//g_riscv_isp_ev = 200;
	printk("fs0 start ev is %d\n", g_riscv_isp_ev);
    g_riscv_isp_ev = 100;
//	tx_isp_set_ev_start(vi_num, enable, g_riscv_isp_ev);//设置主摄起始EV

	//set anfiflicker
#if 0 /*Set up as required, not set by default*/
	vi_num = 0;
	flicker_attr.mode = ISP_ANTIFLICKER_AUTO_MODE;
	flicker_attr.freq = 50;
	tx_isp_tuning_set_flicker(vi_num, &flicker_attr);
#endif

	//set flip mirror
	vi_num = 0;//main sensor
	//tx_isp_tuning_set_hv_flip(ISP_CORE_FLIP_HV_MODE, vi_num); /* 有效 */

	// IRCUT 切回idle状态
	ircut_gpio_init();


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

#if Zeratul_Platform
	if (fast_wdr_mode) {
		frame_channel_fast_start(g_wdr_buf.paddr + ALIGN_SIZE(g_wdr_buf.size));
	}else{
		frame_channel_fast_start(g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size));
	}
#endif
	return 0;
}

static int tx_isp_frame0_done_int_handler(int count)
{
	int vi_num,enable;
	if(count == 1) {
		vi_num = 0;
		//tx_isp_tuning_set_fps(vi_num, 15, 1); /* 设置主摄帧率 */
	}

	return 0;
}

struct tx_isp_callback_ops g_tx_isp_callback_ops = {
	.tx_isp_riscv_hf_prepare = tx_isp_riscv_hf_prepare,
	.tx_isp_riscv_hf_resize = tx_isp_riscv_hf_resize,
	.tx_isp_frame0_done_int_handler = tx_isp_frame0_done_int_handler,
};
