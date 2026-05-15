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
#include "include/ldclut.h"

#define IR_STATUS_ON "on"
#define IR_STATUS_OFF "off"

#if !Zeratul_Platform
/* 0：un-direct(default configuration)  1: direct */
static int zrt_direct_mode = 1;
/* Configure the number of rows stored in DDR in IVDC (one row represents width, 256 aligned) Y Configure IVDC_ Mem_ Line, UV configuration ivdc_ Mem_ Line/2 General ivdc_ Mem_ Line=height/2 */
static int zrt_ivdc_mem_line = 1080/4;//1440/2;
/* 0: configured as height/2, indicating a threshold of half a frame (default value) */
static int zrt_ivdc_threshold_line = 0;
#endif




extern int tisp_enable_tuning(void);
enum IR_STATUS{
    IR_STATUS_DAY = 0 << 0,  //day mode
    IR_STATUS_NIGHT = 1 << 0,//night mode
    IR_STATUS_LED_CLOSE = 1 << 4,  //White light, daytime
    IR_STATUS_LED_OPEN = 2 << 4, //White light, nighttime
    IR_STATUS_IGNORE=IR_STATUS_NIGHT+IR_STATUS_LED_OPEN,
};
int sensor_wdr_mode = 0;
extern int use_num_sensor;
static char *g_ir_status[] = {"day","night","wl_open","wl_close","ignore"};

uint32_t g_riscv_isp_ev = 0;
uint32_t g_riscv_sensor_again = 0;
uint32_t g_riscv_sensor_inttime = 0;
uint32_t g_riscv_isp_gain = 0;
uint32_t g_riscv_luma = 0;
static int g_is_night = IR_STATUS_DAY;

extern char *clk_name;
extern int isp_clk;
extern int direct_mode;
extern int ivdc_mem_line;
extern int ivdc_threshold_line;
extern int sensor_number;
extern int ldc_mode;
// Declare global variables
extern unsigned long rmem_base; /* rmem memory base address */
extern unsigned long high_framerate_kernel_mode_en;
struct isp_buf_info g_ncu_buf;
struct isp_buf_info g_wdr_buf;
int riscv_is_run = 1;
int riscv_is_pass = 1;
unsigned long virYAddr0 = 0;
unsigned long virYAddr1 = 0;
int page_ch0_nums = 0; //12 1920 * 1080
int page_ch1_nums = 0; //10 1280 * 720
int kernel_save_ch0 = 0; //3
int kernel_save_ch1 = 0; //3
unsigned long rememAddrCh0 = 0;
unsigned long rememAddrCh1 = 0;
/* Quick Start Grasp Raw Image: 1. Build_ Tag_ Memory size for ispmem in t41.sh>(width * height * 2) 2. save_ Raw_ Nums corresponds to the frame of the fast start raw image, set to 0, and do not cache raw data 3 Echo "snap raw" 0>/proc/jz/isp/isp w02 View the production raw image in the tmp directory: kernel_ Save0 raw (fast start grab raw graph) snap0 raw (system stable raw graph) */
int save_raw_nums = 0;

//#define NRVBS_DYNAMIC_SWITCH
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

static int frame_channel1_fast_start(unsigned int addr)
{
	int ret = 0;
	struct tx_isp_frame_channel *chan=NULL;
	struct frame_image_format format;
	struct tisp_requestbuffers req;
	int count = 0;
	int buf_i = 0;
	enum tisp_buf_type type;
    int sensor_orginal_width,sensor_orginal_height;
#ifndef CONFIG_INGENIC_NUM_SENSOR
    sensor_orginal_width = get_sensor_width();
    sensor_orginal_height = get_sensor_height();
#else
    if(use_num_sensor == 0){
        sensor_orginal_width = get_sensor_width_one();
        sensor_orginal_height = get_sensor_height_one();
    }else if(use_num_sensor == 1){
        sensor_orginal_width = get_sensor_width_two();
        sensor_orginal_height = get_sensor_height_two();
    }else{
		printk("[%s][%d]====== NO SENSOR NUM ======\n", __func__, __LINE__);
    }
#endif

#if Zeratul_Platform
    rememAddrCh1 = addr = addr + frame_channel_nrvbs * (sensor_orginal_width * ((sensor_orginal_height + 0xF) & ~0xF) * 3 / 2);
#endif
    printk("TTFF %s %d W:%d H:%d N:%d\n", __func__, __LINE__, (int)frame_channel1_width, (int)frame_channel1_height, (int)frame_channel1_nrvbs);

	// open new channel
	ret = frame_channel1_open(NULL, NULL);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel1_open failed\n");
	}

	chan = IS_ERR_OR_NULL(g_f1_mdev) ? NULL : miscdev_to_frame_chan(g_f1_mdev);
	if(IS_ERR_OR_NULL(chan)){
		printk(KERN_ERR "chan is null\n");
	}

	/* Set channel format, resolution, cropping and scaling, etc */
	memset(&format, 0x0, sizeof(struct frame_image_format));

	format.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	format.pix.field = TISP_FIELD_ANY;
	if(frame_channel1_width) {
		format.pix.width = frame_channel1_width;
		format.crop_width = frame_channel1_width;
		format.scaler_out_width = frame_channel1_width;
	} else {
		format.pix.width = FRAME_CHANNEL_DEF_WIDTH;
		format.crop_width = FRAME_CHANNEL_DEF_WIDTH;
		format.scaler_out_width = FRAME_CHANNEL_DEF_WIDTH;
	}

	if(frame_channel1_height) {
		format.pix.height = frame_channel1_height;
		format.crop_height = frame_channel1_height;
		format.scaler_out_height = frame_channel1_height;
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
	// set channel buffer

	memset(&req, 0, sizeof(struct tisp_requestbuffers));
	if(frame_channel1_nrvbs) {
		req.count = frame_channel1_nrvbs;
	} else {
		req.count = TX_ISP_NRVBS; /*nrVBs*/
	}
	req.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = TISP_MEMORY_USERPTR;

	// chan->index = 1;
	ret = frame_channel_reqbufs(chan, (unsigned long)&req, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_reqbufs failed\n");
	}

	if(frame_channel1_nrvbs) {
		count = frame_channel1_nrvbs; /*nrVBs*/
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
		buf1.m.userptr = addr + buf_i * (frame_channel1_width * ((frame_channel1_height + 0xF) & ~0xF) * 3 / 2);
		buf1.length = (frame_channel1_width * ((frame_channel1_height + 0xF) & ~0xF) * 3 / 2);
		chan->index = 1;
		printk("buf_id = %d  buf_ptr = %x  buf_len = %d\n",buf1.index, (unsigned int)buf1.m.userptr, buf1.length);
		ret = frame_channel_vb2_qbuf(chan, (unsigned long)&buf1, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
		}
	}

	type = TISP_BUF_TYPE_VIDEO_CAPTURE;

	// stream on
	ret = frame_channel_vb2_streamon(chan, (unsigned long)&type, K_MODE);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vb2_streamon failed\n");
	}

	return 0;
}

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
    rememAddrCh0 = addr;
	/* open new channel */
	ret = frame_channel_open(NULL, NULL);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_open failed\n");
	}

	chan = IS_ERR_OR_NULL(g_f0_mdev) ? NULL : miscdev_to_frame_chan(g_f0_mdev);
	if(IS_ERR_OR_NULL(chan)){
		printk(KERN_ERR "chan is null\n");
	}

	/* Set channel format, resolution, cropping and scaling, etc */
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

	/* set channel buffer */
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
		if(ldc_mode){
			buf.m.userptr = addr + buf_i * (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 2);
			buf.length = (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 2);
		}else{
			buf.m.userptr = addr + buf_i * (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 3 / 2);
			buf.length = (frame_channel_width * ((frame_channel_height + 0xF) & ~0xF) * 3 / 2);
		}

		printk("ldc_mode:%d buf_id = %d buf_ptr = %x buf_len = %d\n",ldc_mode, buf.index, buf.m.userptr, buf.length);
		ret = frame_channel_vb2_qbuf(chan, (unsigned long)&buf, K_MODE);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
		}
	}
	type = TISP_BUF_TYPE_VIDEO_CAPTURE;

	/* stream on */
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

#if !Zeratul_Platform  /* does not cache data in direct mode*/
direct_mode = zrt_direct_mode;
ivdc_mem_line = zrt_ivdc_mem_line;
ivdc_threshold_line = zrt_ivdc_threshold_line;
#endif

	/*set isp clk and clk_name*/
	/*{"mpll", "vpll", "sclka"}*/
	strcpy(clk_name, "mpll");
	isp_clk = 320000000;
	printk("t41 set isp clk is %d\n", isp_clk);

#if TX_ISP_FAST_START
	DEBUG_TTFF("tx_isp_stop_riscv start");
	ret = tx_isp_riscv_is_run();
    if (ret < 0) {
        riscv_is_run = 0;
    } else {
#if 1
		/* Waiting for riscv fixed frames */
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
        printk("riscv count = %d\n", frame_count);
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
     * Hard photosensitive transmission state:
     *g_is_night:
     *bit0 :0 is day time，1 is night
     *bit4 :0 represents the judgment of turning off white light during the day，1 turn on white light at night
     * */
	g_is_night = tx_isp_get_riscv_night_mode();
    if(g_is_night > IR_STATUS_IGNORE){
        g_is_night = IR_STATUS_IGNORE;
    }
    printk("Tuning mode is:%#x\n", g_is_night);
		tx_isp_discard_frame(0xfffffffc); /* The main camera discards the first two frames*/
    tx_isp_set_ev_start(vinum, 1, g_riscv_isp_ev);
    //tx_isp_set_sensor_again(g_riscv_sensor_again);
    //tx_isp_set_sensor_inttime(1700);
    //tx_isp_set_isp_gain(4096);

	// enable debug info per frame
	/* tx_isp_enable_debug(vinum, 1, 20); */
#endif
	return 0;
}

#ifdef CONFIG_USER_AE
static int ae_open(void *priv_data, tisp_ae_algo_init_t AeInitAttr)
{
	printk("%s[%d]: ae_open \n",__FUNCTION__,__LINE__);
	return 0;
}

static int ae_close(void *priv_data)
{
	printk("%s[%d]: ae_close \n",__FUNCTION__,__LINE__);
	return 0;
}
static int ae_handle(void *priv_data, const tisp_ae_algo_info_t *AeInfo, tisp_ae_algo_attr_t *AeAttr)
{
	printk("%s[%d]: ae_handle \n",__FUNCTION__,__LINE__);msleep(1000);
	return 0;
}
#endif

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
	struct wdr_open_attr wdr_attr;
	char *seneor_use_name;
	char *sensor_use_i2c_addr;

#ifdef NRVBS_DYNAMIC_SWITCH
    virYAddr0 = __get_free_pages(GFP_KERNEL, page_ch0_nums);
	virYAddr1 = __get_free_pages(GFP_KERNEL, page_ch1_nums);

	if (!virYAddr0)
	{
		free_pages(virYAddr0, page_ch0_nums);
	    kernel_save_ch0 = 0;
	}
	if (!virYAddr1)
	{
        free_pages(virYAddr1, page_ch1_nums);
        kernel_save_ch1 = 0;
    }
#endif
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


	/* Quick start supports wide dynamic wdr mode reference: Ingenic_ Zeratul_ T41_ Instructions for using fast start wide dynamic WDR_ 20230630_ CN */
	
	/* set wdr mode */
#ifndef CONFIG_INGENIC_NUM_SENSOR
	sensor_wdr_mode = get_sensor_wdr_mode();
#else
    if(use_num_sensor == 0){
        sensor_wdr_mode = get_sensor_wdr_mode_one();
    }else if(use_num_sensor == 1){
        sensor_wdr_mode = get_sensor_wdr_mode_two();
    }


#endif
	printk("sensor_wdr_mode = %d\nuser_wdr_mode = %d\n", sensor_wdr_mode,user_wdr_mode);

	if (sensor_wdr_mode) {
		if(user_wdr_mode) {
			vi_num = 0;
			tx_isp_wdr_enable(module, (unsigned long)&vi_num, K_MODE);
		} else {
			wdr_attr.vinum = 0;
			wdr_attr.mode = sensor_wdr_mode;
			tx_isp_wdr_open(module, (unsigned long)&wdr_attr, K_MODE);
		}
	}

	if(ldc_mode){
		/* ldc init */
		ldc_init_attr ldc;
		memset(&ldc, 0, sizeof(ldc_init_attr));

		ldc.cattr[0].mode = IMPISP_OPS_MODE_ENABLE;
		ldc.cattr[0].enable = IMPISP_OPS_MODE_DISABLE;
		ldc.cattr[0].priority = 0;
		/* 2560 * 1440 */
		memcpy(&ldc.cattr[0].params, &ldc_default_params[7], sizeof(IMPISPLDCParams));

		ldc.cattr[1].mode = IMPISP_OPS_MODE_ENABLE;
		ldc.cattr[1].enable = IMPISP_OPS_MODE_DISABLE;
		ldc.cattr[1].priority = 0;
		/* 640 * 360 */
		memcpy(&ldc.cattr[1].params, &ldc_default_params[2], sizeof(IMPISPLDCParams));

		tx_isp_set_ldc_enable(module, (unsigned long)&ldc, K_MODE);
	}


	/*Set night mode,ensure successful first frame transition. Before adding the sensor*/
    if(g_is_night == IR_STATUS_NIGHT){
		vi_num = 0;
		tx_isp_start_night_mode(module, (unsigned long)&vi_num, K_MODE);
    }


#ifdef CONFIG_INGENIC_NUM_SENSOR
	if(use_num_sensor == 0) {
		seneor_use_name = get_sensor_name_one();
		sensor_use_i2c_addr = get_sensor_i2c_addr_one();
	} else if(use_num_sensor == 1) {
		seneor_use_name = get_sensor_name_two();
		sensor_use_i2c_addr = get_sensor_i2c_addr_two();
	} 
/*	else if(use_num_sensor == 2) {
		seneor_use_name = get_sensor_name_third();
		sensor_use_i2c_addr = get_sensor_i2c_addr_third();
	}
	else if(use_num_sensor == 3) {
		seneor_use_name = get_sensor_name_four();
		sensor_use_i2c_addr = get_sensor_i2c_addr_four();
	} else if(use_num_sensor == 4) {
		seneor_use_name = get_sensor_name_five();
		sensor_use_i2c_addr = get_sensor_i2c_addr_five();
	} else if(use_num_sensor == 5) {
		seneor_use_name = get_sensor_name_six();
		sensor_use_i2c_addr = get_sensor_i2c_addr_six();
	} else if(use_num_sensor == 6) {
		seneor_use_name = get_sensor_name_seven();
		sensor_use_i2c_addr = get_sensor_i2c_addr_seven();
	} else if(use_num_sensor == 7) {
		seneor_use_name = get_sensor_name_eight();
		sensor_use_i2c_addr = get_sensor_i2c_addr_eight();
	}
*/
	else {
		printk("[%s][%d]====== NO SENSOR NUM ======\n", __func__, __LINE__);
	}
#endif    
	// Register sensor
	memset(&sensor_register_info, 0, sizeof(struct tx_isp_sensor_register_info));
	sensor_register_info.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
#ifndef CONFIG_INGENIC_NUM_SENSOR
	strcpy(sensor_register_info.name, get_sensor_name());
	strcpy(sensor_register_info.i2c.type, get_sensor_name());
	sensor_register_info.i2c.addr = get_sensor_i2c_addr();
#else
    printk("-------tx-isp-fast num sensor\n");
	strcpy(sensor_register_info.name, seneor_use_name);
	strcpy(sensor_register_info.i2c.type, seneor_use_name);
	sensor_register_info.i2c.addr = sensor_use_i2c_addr;
#endif

	sensor_register_info.i2c.i2c_adapter_id = 0;
	sensor_register_info.rst_gpio = 18;//PC27
	sensor_register_info.pwdn_gpio = -1;
	sensor_register_info.power_gpio = -1;
	sensor_register_info.video_interface = TISP_SENSOR_VI_MIPI_CSI0;
	sensor_register_info.mclk = TISP_SENSOR_MCLK0;
	if(user_wdr_mode == 1){
		sensor_register_info.default_boot = 1;//need match sensor driver
	} else {
		sensor_register_info.default_boot = 0;//need match sensor driver
	}
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
	if (sensor_wdr_mode){
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

	// tx_isp_tuning_set_sharpness(160);	//set sharpness
	//set start ev
	vi_num = 0;
	enable = 1;
	//g_riscv_isp_ev = 200;
	printk("fs0 start ev is %d\n", g_riscv_isp_ev);
    g_riscv_isp_ev = 100;
//	tx_isp_set_ev_start(vi_num, enable, g_riscv_isp_ev);//set start EV

	//set anfiflicker
#if 0 /*Set up as required, not set by default*/
	vi_num = 0;
	flicker_attr.mode = ISP_ANTIFLICKER_AUTO_MODE;
	flicker_attr.freq = 50;
	tx_isp_tuning_set_flicker(vi_num, &flicker_attr);
#endif

	// IRCUT switch back to idle state
	ircut_gpio_init();


	// create ir_status node
	proc_create("ir_status", S_IRUGO, NULL, &ir_status_proc_fops);

	// Stream on
	init.vinum = 0;
	init.enable = 1;

#ifdef CONFIG_USER_AE
	// ret = IMP_ISP_SetAeAlgoFunc_internal(module, (unsigned long)&init);
	// if(ret < 0) {
	// 	printk("IMP_ISP_SetAeAlgoFunc_internal failed\n");
	// 	return ret;
	// }
	static tisp_ae_algo_func_t ae_func = {
		.open = ae_open,
		.close = ae_close,
		.handle = ae_handle,
	};

	IMP_ISP_SetAeAlgoFunc(0, &ae_func);
	IMP_ISP_SetAeAlgoFunc_internal(module, IMPVI_MAIN, &ae_func);
#endif

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
	if (sensor_wdr_mode) {
		frame_channel_fast_start(g_wdr_buf.paddr + ALIGN_SIZE(g_wdr_buf.size));
	}else{
		frame_channel_fast_start(g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size));
	}
    if(frame_channel1_width && frame_channel1_height && frame_channel1_nrvbs)
        frame_channel1_fast_start(g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size));
#endif
#if !Zeratul_Platform
    if(frame_channel1_width && frame_channel1_height && frame_channel1_nrvbs && direct_mode_save_kernel_ch1)
        frame_channel1_fast_start(g_ncu_buf.paddr + ALIGN_SIZE(g_ncu_buf.size));
#endif
	return 0;
}

static int tx_isp_frame0_done_int_handler(int count)
{
	int vi_num,enable;
	if(count == 1 ) {
		vi_num = 0;
	//set flip mirror
 #if 0
    tisp_hv_flip_t flip_attr;
    flip_attr.sensor_mode = 0;
    flip_attr.isp_mode[0] = ISP_CORE_FLIP_HV_MODE;
	tx_isp_tuning_set_hv_flip(&flip_attr, vi_num); 
#endif   

	//tx_isp_tuning_set_fps(vi_num, 15, 1); /* Set frame rate */
	}

	return 0;
}

struct tx_isp_callback_ops g_tx_isp_callback_ops = {
	.tx_isp_riscv_hf_prepare = tx_isp_riscv_hf_prepare,
	.tx_isp_riscv_hf_resize = tx_isp_riscv_hf_resize,
	.tx_isp_frame0_done_int_handler = tx_isp_frame0_done_int_handler,
};

