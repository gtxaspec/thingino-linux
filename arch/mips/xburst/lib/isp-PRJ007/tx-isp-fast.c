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

#include "include/tx-isp-frame-channel.h"
#include "include/tx-isp-device.h"
#include "include/tx-isp-core-tuning.h"
#include "include/tx-isp-common.h"
#include "include/fast_start_common.h"
#include "include/fast_start_device.h"
#include "include/tx-isp-fast.h"
#include <ingenic_vpu.h>

#define IR_STATUS_ON "on"
#define IR_STATUS_OFF "off"


enum IR_STATUS{
	IR_STATUS_DAY		= 0 << 0,//day mode
	IR_STATUS_NIGHT		= 1 << 0,//night mode
	IR_STATUS_LED_CLOSE	= 1 << 4,//White light, daytime
	IR_STATUS_LED_OPEN	= 2 << 4,//White light, nighttime
	IR_STATUS_IGNORE=IR_STATUS_NIGHT+IR_STATUS_LED_OPEN,
};
int sensor_wdr_mode = 0;
uint32_t g_riscv_isp_ev = 0;
uint32_t g_riscv_sensor_again = 0;
uint32_t g_riscv_sensor_inttime = 0;
uint32_t g_riscv_isp_gain = 0;
uint32_t g_riscv_luma = 0;
static int g_is_night = IR_STATUS_DAY;

// Declare global variables
struct isp_buf_info g_isp_buf[VI_MAX];
int riscv_is_run = 0;
int riscv_is_pass = 1;
unsigned long virYAddr0 = 0;
unsigned long virYAddr1 = 0;
int page_ch0_nums = 0; //12 1920 * 1080
int page_ch1_nums = 0; //10 1280 * 720
int kernel_save_ch0 = 0; //3
int kernel_save_ch1 = 0; //3
unsigned long rememAddrCh[12] = {0};
unsigned int ivdc_paddr = 0;
int sizeimage = 0;
/* Quick Start Grasp Raw Image: 1. Build_ Tag_ Memory size for ispmem in t41.sh>(width * height * 2) 2. save_ Raw_ Nums corresponds to the frame of the fast start raw image, set to 0, and do not cache raw data 3 Echo "snap raw" 0>/proc/jz/isp/isp w02 View the production raw image in the tmp directory: kernel_ Save0 raw (fast start grab raw graph) snap0 raw (system stable raw graph) */
int save_raw_nums = 0;
extern struct tx_isp_sensor_fast_attr sensor0;
extern struct tx_isp_sensor_fast_attr sensor1;
extern struct tx_isp_sensor_fast_attr sensor2;
extern struct tx_isp_sensor_fast_attr sensor3;

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
	char status[20]={"0"};
	unsigned int len = 0;
	ssize_t ret = 0;
	sprintf(status,"day:%d,wl_mode:%d\n",g_is_night&0x1,(g_is_night&0x30)>>4);
	len = (strlen(status + *ppos) < size) ? (strlen(status + *ppos)): size;

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

int enc_qbuf(int chnNum,FrameInfo *frame)
{
	int ret = 0;
    static int frame_num[FS_CHN_MAX] = {0};
	struct tisp_buffer *buf = (struct tisp_buffer*)frame->isp_priv;

//	printk("Enc Qbuf: %p buf_id = %d buf_ptr = %x buf_len = %d\n",buf, buf->index, buf->m.userptr, buf->length);
#ifdef DEBUG_TIMESTAMP
    if(frame_num[chnNum] < 10){
        printk("TTLV End fsChn = %d  num: %d index = %d  TS: %d @@\n",chnNum,frame_num[chnNum],buf->index,frame->timestamp);
    }
#endif
	ret = frame_channel_unlocked_ioctl_first(chnNum,TISP_VIDIOC_QBUF,(unsigned long)buf);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
	}
    frame_num[chnNum] ++;

	return ret;
}
int get_isp_bufsize(void)
{
    return sizeof(struct tisp_buffer);
}
int enc_dqbuf(int chnNum,FrameInfo *frame)
{
	int ret = 0;
	struct tisp_buffer *buf = (struct tisp_buffer *)frame->isp_priv;

	buf->type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	ret = frame_channel_unlocked_ioctl_first(chnNum,TISP_VIDIOC_DQBUF,(unsigned long)buf);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vb2_dqbuf failed\n");
	}
//	printk("Enc DQbuf:%p buf_id = %d buf_ptr = %x buf_len = %d\n", buf,buf->index, buf->m.userptr, buf->length);
    frame->width = frame_channel_width[chnNum];
    frame->height = frame_channel_height[chnNum];
    frame->phyAddr = buf->m.userptr;
    frame->virAddr = frame->phyAddr + 0xA0000000;
    frame->sensor_id = chnNum / 3;
    frame->timestamp = (int64_t)((int64_t)buf->timestamp.tv_sec * 1000000 + (int64_t)buf->timestamp.tv_usec);

	return ret;
}
void enc_video(unsigned int addr)
{
	struct tx_isp_frame_channel *chan=NULL;
    int i = 0;
    int chnNum = 0;
	VPUEncoderAttr chnAttr;
    unsigned int bs_addr = addr;
    int enc_size = 0;

	VPU_Kern_Init();
    vpuBs_size = frame_channel_width[0] * frame_channel_height[0];
    printk("VpuBsaddr:0x%x vpuBs_size:%d\n",bs_addr,vpuBs_size);
    VPU_Encoder_SetVpuBsSize(bs_addr,vpuBs_size);
	for( i=0; i < FS_CHN_MAX; i++){
        chan = IS_ERR_OR_NULL(g_f_mdev[i]) ? NULL : miscdev_to_frame_chan(g_f_mdev[i]);
        if( ! IS_ERR_OR_NULL(chan)){
            memset(&chnAttr,0,sizeof(VPUEncoderAttr));
            if(chan->state == TX_ISP_MODULE_INIT){
                chnNum = chan->index;
                chnAttr.enType = PT_H265;
                chnAttr.picWidth = frame_channel_width[chnNum];
                chnAttr.picHeight = frame_channel_height[chnNum];
                chnAttr.fps = 15;
                chnAttr.qp = 30;
                chnAttr.strLen = (frame_channel_width[chnNum] * ((frame_channel_height[chnNum] + 0xF) & ~0xF) /4);//save kernel stream buf size
                if(direct_mode >= 1 && (chnNum%3)==0)
                    chnAttr.bEnableIvdc = true;
                if(chnNum == 0){
                    chnAttr.encPhyaddr = bs_addr + vpuBs_size;//offset: ispbuf + vb
                }else{
                    chnAttr.encPhyaddr = bs_addr + vpuBs_size + enc_size;//offset: ispbuf + vb + bs + enc0
                }
                enc_size += VPU_Encoder_GetChannelBufferSize(&chnAttr);
                printk("chnNum:%d encPhyaddr:0x%x enc_size:%d\n",chan->index, chnAttr.encPhyaddr,enc_size);
                VPU_Encoder_CreateChn(chnNum, &chnAttr);
            }
        }
    }
}
static int set_ivdc_addr(unsigned int addr)
{
	int ret = 0;
    int stride = 0;
    ivdc_paddr = addr;
    if(direct_mode != 0){
        if(direct_mode == 1){
            stride = (frame_channel_width[0] + 63) & (~63);
            if (0 == ivdc_mem_line) {
                sizeimage = stride * frame_channel_height[0] * 3 / 4;
            } else {
                sizeimage = stride * ivdc_mem_line * 3 / 2;
            }
        }
        if(direct_mode > 1){
            stride = (ivdc_mem_stride + 63) & (~63);
            if (0 == ivdc_mem_line) {
                sizeimage = stride * frame_channel_height[0] * 3 / 4;
            } else {
                sizeimage = stride * ivdc_mem_line * 3 / 2;
            }
        }
        sprintf(ivdc_buf_len,"ivdcbuf_len=%d",(sizeimage + 0xF) & ~0xF);
        printk("[%s][%d] sizeimage:%d\n", __func__, __LINE__,(sizeimage + 0xF) & ~0xF);
        printk("[%s][%d] ivdc_paddr:0x%x\n", __func__, __LINE__,ivdc_paddr);
        ret =ivdc_misc_unlocked_ioctl (NULL,TISP_VIDIOC_SET_PADDR,(unsigned long)&ivdc_paddr);
        if(ret != 0) {
            printk(KERN_ERR "TISP_VIDIOC_SET_PADDR failed\n");
        }
        return ivdc_paddr + ((sizeimage + 0xFF) & ~0xFF);
    }
    return ivdc_paddr;
}
static int get_frame_format(int chnNum,struct frame_image_format *format)
{
    switch(chnNum) {
        case 0:
            format->type = TISP_BUF_TYPE_VIDEO_CAPTURE;
            format->pix.pixelformat = TISP_VO_FMT_YUV_SEMIPLANAR_420;//NV12
            format->pix.colorspace = TISP_COLORSPACE_SRGB;
            format->pix.field = TISP_FIELD_ANY;
            format->pix.width = frame_channel_width[chnNum];
            format->pix.height = frame_channel_height[chnNum];
            format->crop_width = frame_channel_width[chnNum];
            format->crop_height = frame_channel_height[chnNum];
            format->scaler_out_width = frame_channel_width[chnNum];
            format->scaler_out_height = frame_channel_height[chnNum];
            format->crop_enable = 1;
            format->crop_top = 0;
            format->crop_left = 0;
            format->scaler_enable = 1;
            format->rate_bits = 0;
            format->rate_mask = 1;
            break;
        case 3:
            format->type = TISP_BUF_TYPE_VIDEO_CAPTURE;
            format->pix.pixelformat = TISP_VO_FMT_YUV_SEMIPLANAR_420;//NV12
            format->pix.colorspace = TISP_COLORSPACE_SRGB;
            format->pix.field = TISP_FIELD_ANY;
            format->pix.width = frame_channel_width[chnNum];
            format->pix.height = frame_channel_height[chnNum];
            format->crop_width = frame_channel_width[chnNum];
            format->crop_height = frame_channel_height[chnNum];
            format->scaler_out_width = frame_channel_width[chnNum];
            format->scaler_out_height = frame_channel_height[chnNum];
            format->crop_enable = 0;
            format->crop_top = 0;
            format->crop_left = 0;
            format->scaler_enable = 1;
            format->rate_bits = 0;
            format->rate_mask = 1;
            break;
        case 6:
            format->type = TISP_BUF_TYPE_VIDEO_CAPTURE;
            format->pix.pixelformat = TISP_VO_FMT_YUV_SEMIPLANAR_420;//NV12
            format->pix.colorspace = TISP_COLORSPACE_SRGB;
            format->pix.field = TISP_FIELD_ANY;
            format->pix.width = frame_channel_width[chnNum];
            format->pix.height = frame_channel_height[chnNum];
            format->crop_width = frame_channel_width[chnNum];
            format->crop_height = frame_channel_height[chnNum];
            format->scaler_out_width = frame_channel_width[chnNum];
            format->scaler_out_height = frame_channel_height[chnNum];
            format->crop_enable = 0;
            format->crop_top = 0;
            format->crop_left = 0;
            format->scaler_enable = 1;
            format->rate_bits = 0;
            format->rate_mask = 1;
            break;
        case 9:
            format->type = TISP_BUF_TYPE_VIDEO_CAPTURE;
            format->pix.pixelformat = TISP_VO_FMT_YUV_SEMIPLANAR_420;//NV12
            format->pix.colorspace = TISP_COLORSPACE_SRGB;
            format->pix.field = TISP_FIELD_ANY;
            format->pix.width = frame_channel_width[chnNum];
            format->pix.height = frame_channel_height[chnNum];
            format->crop_width = frame_channel_width[chnNum];
            format->crop_height = frame_channel_height[chnNum];
            format->scaler_out_width = frame_channel_width[chnNum];
            format->scaler_out_height = frame_channel_height[chnNum];
            format->crop_enable = 0;
            format->crop_top = 0;
            format->crop_left = 0;
            format->scaler_enable = 1;
            format->rate_bits = 0;
            format->rate_mask = 1;
            break;
        default:
            format->type = TISP_BUF_TYPE_VIDEO_CAPTURE;
            format->pix.pixelformat = TISP_VO_FMT_YUV_SEMIPLANAR_420;//NV12
            format->pix.colorspace = TISP_COLORSPACE_SRGB;
            format->pix.field = TISP_FIELD_ANY;
            format->pix.width = frame_channel_width[chnNum];
            format->pix.height = frame_channel_height[chnNum];
            format->crop_width = frame_channel_width[chnNum];
            format->crop_height = frame_channel_height[chnNum];
            format->scaler_out_width = frame_channel_width[chnNum];
            format->scaler_out_height = frame_channel_height[chnNum];
            format->crop_enable = 0;
            format->crop_top = 0;
            format->crop_left = 0;
            format->scaler_enable = 1;
            format->rate_bits = 0;
            format->rate_mask = 1;
    }
    return 0;
}
static int frame_channel_fast_start(int chn_num, unsigned int addr)
{
	int ret = 0;
	struct tx_isp_frame_channel *chan=NULL;
	struct frame_image_format format;
	struct tisp_requestbuffers req;
	int count = 0;
	int buf_i = 0;

	if(frame_channel_width[chn_num] <= 0 || frame_channel_height[chn_num] <= 0 || frame_channel_nrvbs[chn_num] <= 0){
		return addr;
	}
	printk("TTFF %s %d W:%d H:%d N:%d\n", __func__, __LINE__, (int)frame_channel_width[chn_num], (int)frame_channel_height[chn_num], (int)frame_channel_nrvbs[chn_num]);
	rememAddrCh[chn_num] = addr;
	/* open new channel */
	ret = frame_channel_open_fast(chn_num);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_open failed\n");
	}

	chan = IS_ERR_OR_NULL(g_f_mdev[chn_num]) ? NULL : miscdev_to_frame_chan(g_f_mdev[chn_num]);
	if(IS_ERR_OR_NULL(chan)){
		printk(KERN_ERR "chan is null\n");
	}
	/* Set channel format, resolution, cropping and scaling, etc */
	memset(&format, 0x0, sizeof(struct frame_image_format));

    ret = get_frame_format(chan->index,&format);
    if(ret !=0){
		printk(KERN_ERR "get_frame_format failed\n");
    }
	ret = frame_channel_unlocked_ioctl_first(chan->index,TISP_VIDIOC_SET_FRAME_FORMAT,(unsigned long)&format);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vidioc_set_fmt failed\n");
	}

	/* set channel buffer */
	memset(&req, 0, sizeof(struct tisp_requestbuffers));
	if(frame_channel_nrvbs[chn_num]) {
		req.count = frame_channel_nrvbs[chn_num]; /*nrVBs*/
	} else {
		req.count = TX_ISP_NRVBS; /*nrVBs*/
	}
	req.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = TISP_MEMORY_USERPTR;

	ret = frame_channel_unlocked_ioctl_first(chan->index,TISP_VIDIOC_REQBUFS,(unsigned long)&req);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_reqbufs failed\n");
	}

	if(frame_channel_nrvbs[chn_num]) {
		count = frame_channel_nrvbs[chn_num]; /*nrVBs*/
	} else {
		count = TX_ISP_NRVBS; /*nrVBs*/
	}
	ret = frame_channel_unlocked_ioctl_first(chan->index,TISP_VIDIOC_DEFAULT_CMD_SET_BANKS,(unsigned long)&count);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_set_channel_banks failed\n");
	}

	struct tisp_buffer buf;
	for(buf_i = 0; buf_i < count; buf_i++) {
		memset(&buf, 0, sizeof(struct tisp_buffer));

		buf.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = TISP_MEMORY_USERPTR;
		buf.index = buf_i;
        buf.length = (((frame_channel_width[chn_num] + 0xF) & ~0xF) * ((frame_channel_height[chn_num] + 0xF) & ~0xF) * 3 / 2);
        if(direct_mode >= 1 && (chn_num % 3) ==0)
            buf.m.userptr = 0x80000000;
        else{
            buf.m.userptr = addr + buf_i * buf.length;
        }
		printk("qbuf: buf_id = %d buf_ptr = 0x%lx buf_len = %d\n", buf.index, buf.m.userptr, buf.length);
		ret = frame_channel_unlocked_ioctl_first(chan->index,TISP_VIDIOC_QBUF,(unsigned long)&buf);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
		}
	}
    if(direct_mode >= 1 && (chn_num%3) == 0)
        addr = ivdc_paddr + ((sizeimage + 0xFF) & ~0xFF);
    else
        addr = addr + count*buf.length;
	return addr;
}

static int tx_isp_riscv_hf_prepare(void)
{
	int ret = 0;
	int timeout = 200;
	memset(&fast_sensors,0,sizeof(fast_sensors));

	switch(sensor_number){
	case 4:
		SENSOR_DRIVER_CHECK(3);
		SENSOR_DRIVER_ATTR_INIT(3);

	case 3:
		SENSOR_DRIVER_CHECK(2);
		SENSOR_DRIVER_ATTR_INIT(2);

	case 2:
		SENSOR_DRIVER_CHECK(1);
		SENSOR_DRIVER_ATTR_INIT(1);

	case 1:
		SENSOR_DRIVER_CHECK(0);
		SENSOR_DRIVER_ATTR_INIT(0);
		break;
	default:
		SENSOR_DRIVER_CHECK(0);
		SENSOR_DRIVER_ATTR_INIT(0);
		break;
	}

	DEBUG_TTFF("tx_isp_stop_riscv start");
	ret = tx_isp_riscv_is_run();
	/* return 0; */
	if (ret < 0) {
		riscv_is_run = 0;
	} else {
		riscv_is_run = 1;
		DEBUG_TTFF("wait riscv frame done");
		timeout = 3000;
		while(!tx_isp_riscv_is_stop() && timeout--) {
			mdelay(1);
		}
		DEBUG_TTFF("tx_isp_stop_riscv done");
		if(timeout <= 0) {
			printk("stop riscv timeout\n");
			riscv_is_pass = 0;
		}

		ret = tx_isp_get_tiziano_para(0);
		if(ret < 0) {
			printk("tx_isp_get_tiziano_para failed\n");
			return ret;
		}
		if (sensor_number == 2) {
			ret = tx_isp_get_tiziano_para(1);
			if(ret < 0) {
				printk("tx_isp_get_tiziano_para failed\n");
				return ret;
			}
		}
	}
	tiziano_fastpara_exit();

	/*
	 * Hard photosensitive transmission state:
	 *g_is_night:
	 *bit0 :0 is day time，1 is night
	 *bit4 :0 represents the judgment of turning off white light during the day，1 turn on white light at night
	 * */
	/* g_is_night = tx_isp_get_riscv_night_mode(); */
	/* if(g_is_night > IR_STATUS_IGNORE){ */
	/* 	g_is_night = IR_STATUS_IGNORE; */
	/* } */
	/* printk("Tuning mode is:%#x\n", g_is_night); */
	/* tx_isp_set_tiziano_param(vinum, g_riscv_isp_ev); */

	//enable debug info per frame
	//tx_isp_enable_debug(vinum, 1, 20);

	return 0;
}

int register_sensor(int vi_num, char *sensor_name, int i2c_addr, int i2c_adaptor_id, int reset_gpio, int vin_type, int vin_mclk, int default_boot)
{
	int ret = 0;
	struct tx_isp_sensor_register_info sensor_register_info;
	struct tx_isp_initarg init;
	struct tisp_input inputt;
	if(vi_num < 0 || vi_num >= VI_MAX){
		printk("register sensor error(vinum out of range)\n");
		return -1;
	}
	// Register sensor
	memset(&sensor_register_info, 0, sizeof(struct tx_isp_sensor_register_info));
	sensor_register_info.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
	strcpy(sensor_register_info.name, sensor_name);
	strcpy(sensor_register_info.i2c.type, sensor_name);
	sensor_register_info.i2c.addr = i2c_addr;

	sensor_register_info.i2c.i2c_adapter_id = i2c_adaptor_id;
	sensor_register_info.rst_gpio = reset_gpio;//PC27
	sensor_register_info.pwdn_gpio = -1;
	sensor_register_info.power_gpio = -1;
    sensor_register_info.video_interface =  vin_type;//TISP_SENSOR_VI_MIPI_CSI0
	sensor_register_info.mclk = vin_mclk;
	sensor_register_info.default_boot = default_boot;//need match sensor driver
	sensor_register_info.sensor_id = vi_num;
	printk("sensor[%d]  name = %s i2c = 0x%x default_boot=%d\n",vi_num,sensor_register_info.name,sensor_register_info.i2c.addr,sensor_register_info.default_boot);
	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_REGISTER_SENSOR,(unsigned long)&sensor_register_info);
	if(ret < 0) {
		printk("tx_isp_sensor_register_sensor failed\n");
		return ret;
	}

	inputt.index = vi_num;
	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_ENUMINPUT,(unsigned long)&inputt);
	if(ret < 0) {
		printk("tx_isp_sensor_enum_input failed\n");
		return ret;
	}

	// Set Input
	init.vinum = vi_num;
	init.enable = 1;
	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_S_INPUT,(unsigned long)&init);
	if(ret < 0) {
		printk("tx_isp_sensor_set_input failed\n");
		return ret;
	}

	return 0;
}

unsigned int set_isp_buf(int vinum, unsigned int rmem_addr)
{
	int ret = 0;
	// Set ISP buf
    char str_buf[64] = {0};
	g_isp_buf[vinum].vinum = vinum;
	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_GET_ISP_MEM_INFO,(unsigned long)&g_isp_buf[vinum]);
	if(ret < 0) {
		printk("tx_isp_get_buf failed\n");
		return ret;
	}
    g_isp_buf[vinum].paddr = rmem_addr;
    printk("Sensor%d ISP Buf: size = %d  paddr = 0x%x\n", vinum, (g_isp_buf[vinum].size + 0xF) & ~0xF, g_isp_buf[vinum].paddr);
    sprintf(str_buf," len%d=%d ",vinum, (g_isp_buf[vinum].size + 0xF) & ~0xF);
    strcat(isp_buf_len,str_buf);

	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_ISP_MEM_INFO,(unsigned long)&g_isp_buf[vinum]);
	if(ret < 0) {
		printk("tx_isp_set_buf failed\n");
		return ret;
	}
	return rmem_addr + ((g_isp_buf[vinum].size + 0xF) & ~0xF);
}

int sensor_stream_on(int vi_num)
{
	int ret = 0;
	struct tx_isp_initarg init;
	init.vinum = vi_num;
	init.enable = 1;

	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_G_INPUT,(unsigned long)&init);
	if(ret < 0) {
		printk("tx_isp_sensor_get_input failed\n");
		return ret;
	}
	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_STREAMON,(unsigned long)&init);
	if(ret < 0) {
		printk("tx_isp_video_s_stream failed\n");
		return ret;
	}

	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_CREATE_SUBDEV_LINKS,(unsigned long)&init);
	if(ret < 0) {
		printk("tx_isp_video_link_setup failed\n");
		return ret;
	}

	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_LINKS_STREAMON,(unsigned long)&init);
	if(ret < 0) {
		printk("TISP_VIDIOC_LINKS_STREAMON failed\n");
		return ret;
	}
	return 0;
}

static int tx_isp_riscv_hf_resize(void)
{
    struct tx_isp_module *module = g_isp_module;
	struct tx_isp_frame_channel *chan=NULL;
	struct tx_isp_module * submod = NULL;
	struct tx_isp_subdev *sd;
	int ret = 0, index = 0;
	struct tx_isp_device *ispdev = NULL;
	struct tx_isp_subdev *subdev = NULL;
	struct msensor_mode s_mode;
	int vi_num = 0, i = 0,default_boot = 0, i2c_adaptor_id;
	unsigned int channel_buf_addr = 0;
	struct wdr_open_attr wdr_attr;
	int get_or_set;//0 set 1 get

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
	sensor_wdr_mode = fast_sensors[0].wdr;
	printk("sensor_wdr_mode = %d user_wdr_mode = %d\n", sensor_wdr_mode, user_wdr_mode);

	if (sensor_wdr_mode) {
		if(user_wdr_mode) {
			vi_num = 0;
            tx_isp_unlocked_ioctl_first(TISP_VIDIOC_ISP_WDR_ENABLE,(unsigned long)&vi_num);
		} else {
			wdr_attr.vinum = 0;
			wdr_attr.mode = sensor_wdr_mode;
//            tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_ISP_WDR_OPEN,(unsigned long)&wdr_attr);
		}
	}

	//set dual sensor mode
	if (sensor_number > 1) {
		s_mode.sensor_num = sensor_number;
		s_mode.dual_mode = 0;
		s_mode.joint_mode = 0;//IMPISP_MAIN_ON_THE_ABOVE;
		s_mode.dmode_switch.en = 0;
		ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_MULTISENSOR_MODE,(unsigned long)&s_mode);
		if(ret < 0) {
			printk("set dualsensor_mode failed\n");
			return ret;
		}
	}


	/*Set night mode,ensure successful first frame transition. Before adding the sensor*/
	if(g_is_night == IR_STATUS_NIGHT){
		vi_num = 0;
		tx_isp_unlocked_ioctl_first(TISP_VIDIOC_START_NIGHT_MODE,(unsigned long)&vi_num);
		if(sensor_number > 1) {
			vi_num = 1;
            tx_isp_unlocked_ioctl_first(TISP_VIDIOC_START_NIGHT_MODE,(unsigned long)&vi_num);
		}
	}
	vi_num = 0;
	default_boot = 0;
	i2c_adaptor_id = 0;
	ret = register_sensor(0, fast_sensors[0].name, fast_sensors[0].i2c_addr, i2c_adaptor_id, GPIO_PA(20), TISP_SENSOR_VI_MIPI_CSI0, TISP_SENSOR_MCLK0, default_boot);
	if(ret < 0){
		return ret;
	}
	//set isp buf
    channel_buf_addr = rmem_base;
    channel_buf_addr = set_isp_buf(0, channel_buf_addr);
	//Dual Sensor mode
	if (sensor_number == 2) {
		vi_num = 1;
		default_boot = 0;
		i2c_adaptor_id = 1;
		ret = register_sensor(vi_num, fast_sensors[1].name, fast_sensors[1].i2c_addr, 1, GPIO_PA(21), TISP_SENSOR_VI_MIPI_CSI1, TISP_SENSOR_MCLK1, default_boot);
		if(ret < 0){
			return ret;
		}
        channel_buf_addr = set_isp_buf(1, channel_buf_addr);
	}

	tisp_enable_tuning();

	vi_num = 0;
	printk("fs0 start ev is %d\n", g_riscv_isp_ev);

#if 0
    /* set fps */
	get_or_set = 0;
    int32_t tmp = 0;
    int32_t fps = 15;
    tmp = (fps << 16) | 1;
	isp_core_tunning_unlocked_ioctl_fast(vi_num, get_or_set, IMAGE_TUNING_CID_CONTROL_FPS, (unsigned long)tmp);
    if( sensor_number == 2 ){
        isp_core_tunning_unlocked_ioctl_fast(1, get_or_set, IMAGE_TUNING_CID_CONTROL_FPS, (unsigned long)tmp);
    }

    /* enable discard frame */
    tisp_frame_drop_attr_t tfd[VI_MAX];
    for ( i=0; i < sensor_number; i++) {
        tfd[i].vinum = i;
        for ( j=0; j < 3; j++) {
            if( frame_channel_width[ i*3+j ] && frame_channel_height[ i*3+j ] && frame_channel_nrvbs[ i*3+j ] ){
                tfd[i].fdrop[j].enable = IMPISP_TUNING_OPS_MODE_ENABLE;
                tfd[i].fdrop[j].lsize = 14;
                tfd[i].fdrop[j].fmark = 0xfffffffc;
            }
        }
        ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_FRAME_DROP,(unsigned long)&tfd[i]);
        if(ret != 0) {
            printk(KERN_ERR "TISP_VIDIOC_SET_FRAME_DROP enable failed\n");
        }
    }
#endif

#if 0 /* set start ev awb */
	struct AWB_start awb_attr;
	get_or_set = 1;//0 set 1 get
	isp_core_tunning_unlocked_ioctl_fast(vi_num, get_or_set, IMAGE_TUNING_CID_AWB_ATTR, (unsigned long)&awb_attr);
	awb_attr.awb_start.rgain = 274;
	awb_attr.awb_start.bgain = 256;
	awb_attr.awb_start_en = TISP_OPS_MODE_ENABLE;
	get_or_set = 0;//0 set 1 get
	isp_core_tunning_unlocked_ioctl_fast(vi_num, get_or_set, IMAGE_TUNING_CID_AWB_ATTR, (unsigned long)&awb_attr);


	struct AE_start ae_attr;
	get_or_set = 1;//0 set 1 get
	isp_core_tunning_unlocked_ioctl_fast(vi_num, get_or_set, IMAGE_TUNING_CID_AE_SCENCE_ATTR, (unsigned long)&ae_attr);
	//printk("0 get ae target = %d en = %d\n",ae_attr.AeTargetComp, ae_attr.AeTargetCompEn);
	ae_attr.AeStartEv = 2024;
	ae_attr.AeStartEn = TISP_AE_SCENCE_GLOBAL_ENABLE;

	get_or_set = 0;//0 get 1 get
	isp_core_tunning_unlocked_ioctl_fast(vi_num, get_or_set, IMAGE_TUNING_CID_AE_SCENCE_ATTR, (unsigned long)&ae_attr);
	get_or_set = 1;//0 set 1 get
	isp_core_tunning_unlocked_ioctl_fast(vi_num, get_or_set, IMAGE_TUNING_CID_AE_SCENCE_ATTR, (unsigned long)&ae_attr);
	//printk("1 get ae target = %d en = %d\n",ae_attr.AeTargetComp, ae_attr.AeTargetCompEn);
#endif

	//set anfiflicker
#if 0 /*Set up as required, not set by default*/
	struct tisp_anfiflicker_attr_t flicker_attr;
	vi_num = 0;
	flicker_attr.mode = ISP_ANTIFLICKER_AUTO_MODE;
	flicker_attr.freq = 50;
	tx_isp_tuning_set_flicker(vi_num, &flicker_attr);
#endif

	// IRCUT switch back to idle state
	ircut_gpio_init();

	// create ir_status node
	proc_create("ir_status", S_IRUGO, NULL, &ir_status_proc_fops);

	//sensor Stream on
	for(i=0; i < sensor_number; i++)
	{
		ret = sensor_stream_on(i);
		if(ret < 0){
			return ret;
		}
	}
    channel_buf_addr = set_ivdc_addr(channel_buf_addr);
	for(i=0; i < sensor_number; i++){
        channel_buf_addr = frame_channel_fast_start(i*3, channel_buf_addr);
        channel_buf_addr = frame_channel_fast_start(i*3+1, channel_buf_addr);
        channel_buf_addr = frame_channel_fast_start(i*3+2, channel_buf_addr);
    }
    /* enc video  */
    if(kernel_encode)
        enc_video(channel_buf_addr);
	enum tisp_buf_type type;
	type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	/* stream on */
    for( i=0 ;i < FS_CHN_MAX ; i++){
        chan = IS_ERR_OR_NULL(g_f_mdev[i]) ? NULL : miscdev_to_frame_chan(g_f_mdev[i]);
        if( ! IS_ERR_OR_NULL(chan)){
            if(chan->state == TX_ISP_MODULE_INIT){
                ret = frame_channel_unlocked_ioctl_first(chan->index,TISP_VIDIOC_FRAME_STREAMON,(unsigned long)&type);
                if(ret != 0) {
                    printk(KERN_ERR "frame_channel_vb2_streamon failed\n");
                }
            }
        }
    }
	return 0;
}

static int tx_isp_frame0_done_int_handler(int count)
{
#if 0
	int vi_num,enable;
	if(count == 1 ) {
		vi_num = 0;
		tx_isp_tuning_set_fps(vi_num, 15, 1); /* Set frame rate */
	}
#endif
    /* disable discard frame */
#if 0
    if( count == 14){
        int i = 0, j = 0;
        tisp_frame_drop_attr_t tfd[VI_MAX];
        for ( i=0; i < sensor_number; i++) {
            tfd[i].vinum = i;
            for ( j=0; j < 3; j++) {
                if( frame_channel_width[ i*3+j ] && frame_channel_height[ i*3+j ] && frame_channel_nrvbs[ i*3+j ] ){
                    tfd[i].fdrop[j].enable = IMPISP_TUNING_OPS_MODE_DISABLE;
                }
            }
            tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_FRAME_DROP,(unsigned long)&tfd[i]);
        }

    }
#endif
	return 0;
}

struct tx_isp_callback_ops g_tx_isp_callback_ops = {
	.tx_isp_riscv_hf_prepare = tx_isp_riscv_hf_prepare,
	.tx_isp_riscv_hf_resize = tx_isp_riscv_hf_resize,
	.tx_isp_frame0_done_int_handler = tx_isp_frame0_done_int_handler,
};

