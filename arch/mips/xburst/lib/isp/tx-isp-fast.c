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
#include <linux/mm.h>
#include <linux/vmalloc.h>

#include "include/tx-isp-frame-channel.h"
#include "include/tx-isp-device.h"
#include "include/tx-isp-core-tuning.h"
#include "include/tx-isp-common.h"
#include "include/fast_start_common.h"
#include "include/fast_start_device.h"
#include "include/tx-isp-fast.h"

//#include "include/osd/imp_osd.h"
#include "include/osd/isp_osd.h"
#include "include/user/osd.h"

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
int gosd_enable = 0; /* 1: ipu osd, 2: isp osd, 3: ipu osd and isp osd */
int ipuosd_enable = 0;
int isposd_enable = 0;
int sensor_wdr_mode = 0;
uint32_t g_riscv_isp_ev = 0;
uint32_t g_riscv_sensor_again = 0;
uint32_t g_riscv_sensor_inttime = 0;
uint32_t g_riscv_isp_gain = 0;
uint32_t g_riscv_luma = 0;
static int g_is_night = IR_STATUS_DAY;

// Declare global variables
struct isp_buf_info g_isp_buf[VI_MAX];
struct isp_buf_info g_ipc_buf[VI_MAX];
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
IMP_Sample_OsdParam ipuosdparam;
IMP_Sample_OsdParam isposdparam;
IMPFSChnAttr g_chn[FS_CHN_MAX] = {0};
/* Quick Start Grasp Raw Image: 1. Build_ Tag_ Memory size for ispmem in t41.sh>(width * height * 2) 2. save_ Raw_ Nums corresponds to the frame of the fast start raw image, set to 0, and do not cache raw data 3 Echo "snap raw" 0>/proc/jz/isp/isp w02 View the production raw image in the tmp directory: kernel_ Save0 raw (fast start grab raw graph) snap0 raw (system stable raw graph) */
int save_raw_nums = 0;
static struct list_head dma_alloc_frame_list[FS_CHN_MAX];
extern struct tx_isp_sensor_fast_attr sensor0;
extern struct tx_isp_sensor_fast_attr sensor1;
extern struct tx_isp_sensor_fast_attr sensor2;
extern struct tx_isp_sensor_fast_attr sensor3;
/* To avoid generating isp overflow, it needs to be set to a default value,When sensor_num >= 2 && isp_cache_line = 0,*/
static int zrt_isp_cache_line = 1080;
extern unsigned long rmem_size;

#define GPIO_IRCUT_N -1//GPIO_PB(28)
#define GPIO_IRCUT_P -1//GPIO_PB(18)
#define SEC_GPIO_IRCUT_N -1//GPIO_PB(28)
#define SEC_GPIO_IRCUT_P -1//GPIO_PB(18)

int kernel_osd = 0;

#define KERNEL_SNAP_YUV	0
#define KERNEL_SNAP_RAW	0

#if KERNEL_SNAP_RAW
struct vic_snap_frame {
	int vinum;
	struct work_struct snap_work;
}snap_raw;
#endif
#if KERNEL_SNAP_YUV
enum{
	PRO_ZRT_IOCTL_DMA_GET_FRAME = 1,
	PRO_ZRT_IOCTL_FREE_DMA_FRAME,
	PRO_ZRT_IOCTL_INVALUE,
};
#define PROC_NAME "zeratul"
#define ZRT_IOCTL_TYPE 'Z'
#define ZRT_IOCTL_GET_DMA_FRAME 	_IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_DMA_GET_FRAME, struct fast_frame_info)
#define ZRT_IOCTL_FREE_DMA_FRAME 	_IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_FREE_DMA_FRAME, struct fast_frame_info)

static struct proc_dir_entry *zrt_proc_entry_frame;
int zrt_isp_dma_alloc(struct fast_frame_info *info)
{
	struct fast_frame_list *new_frame = NULL;
	struct list_head *pos, *n;
	struct fast_frame_list *frame;
	if(direct_mode){
		switch(info->channel){
		case 0:
		case 3:
		case 6:
		case 9:
			printk("In direct_mode, these channels do not support capturing YUV\n");
			return -ENOMEM;
		default:
			break;
		}
	}
	new_frame = kzalloc(sizeof(struct fast_frame_list), GFP_KERNEL);
	if (!new_frame) {
		printk(KERN_ERR "Failed to allocate memory for fastframe\n");
		return -ENOMEM;
	}

	/* new_frame->info.kvaddr = dma_alloc_coherent(NULL, info->alloc_size, (dma_addr_t *)&new_frame->info.kpaddr, GFP_KERNEL); */
	new_frame->info.kvaddr = kmalloc(info->alloc_size,GFP_KERNEL);
	new_frame->info.kpaddr = virt_to_phys(new_frame->info.kvaddr);
	if (!new_frame->info.kvaddr) {
		printk(KERN_ERR "Failed to allocate coherent DMA memory\n");
		kfree(new_frame);
		return -ENOMEM;
	}

	new_frame->info.vinum = info->vinum;
	new_frame->info.channel = info->channel;
	new_frame->info.frame_num = info->frame_num;
	new_frame->info.alloc_size = info->alloc_size;
	new_frame->info.width = info->width;
	new_frame->info.height = info->height;
	new_frame->info.src_kpaddr = info->src_kpaddr;
	new_frame->busy = false;

	info->kvaddr = new_frame->info.kvaddr;
	info->kpaddr = new_frame->info.kpaddr;
#if 0
	list_for_each_safe(pos, n, &dma_alloc_frame_list) {
		frame = list_entry(pos, struct fast_frame_list, flist);
		if (frame->info.frame_num == info->frame_num && frame->info.channel == info->channel){
			pr_warning("Duplicate frame request.\n");
			goto insert_failed;
		}
	}
#endif
	list_add_tail(&new_frame->flist, &dma_alloc_frame_list[info->vinum * 3 + info->channel]);
	printk("[%s %d]Allocate the buffer(%d) %d size for vinum:%d channel%d kpaddr:0x%x\n",__func__,__LINE__,
	       info->frame_num, info->alloc_size, info->vinum,info->channel,new_frame->info.kpaddr);

	return 0;

insert_failed:
	/* dma_free_coherent(NULL, new_frame->info.alloc_size, new_frame->info.kvaddr, new_frame->info.kpaddr); */
	kfree(new_frame->info.kvaddr);
	kfree(new_frame);
	return -EINVAL;
}

void *zrt_isp_dma_getframe(int vinum, int channel, int frame_num)
{
	/* printk("===%s vinum:%d channel:%d\n",__func__,vinum,channel); */
	struct fast_frame_list *frame;
	struct list_head *pos, *n;
	list_for_each_safe(pos, n, &dma_alloc_frame_list[vinum * 3 + channel]) {
		frame = list_entry(pos, struct fast_frame_list, flist);
		if (frame->info.frame_num == frame_num && frame->info.vinum == vinum && frame->info.channel == channel){
			return frame;
		}
	}
	return NULL;
}
int zrt_isp_copyframe(int frame_channel, unsigned int frame_virAddr)
{
	struct fast_frame_list *frame;
	struct list_head *pos, *n;
	int ret, vinum, channel;
	void *src_vaddr = NULL;

	vinum = frame_channel / 3;
	channel = frame_channel % 3;
	printk("%s start vinum:%d channel:%d frame_virAddr:0x%x\n",__func__,vinum,channel, frame_virAddr);
	list_for_each_safe(pos, n, &dma_alloc_frame_list[vinum * 3 + channel]) {
		frame = list_entry(pos, struct fast_frame_list, flist);
		if(frame->info.vinum == vinum && frame->info.channel == channel && !frame->busy) {
			memcpy(frame->info.kvaddr, frame_virAddr, frame->info.alloc_size);
			printk("%s success\n",__func__);
			frame->busy = true;
			break;
		}
	}
	return 0;
}

int zrt_isp_dma_copyframe(int frame_channel)
{
	struct fast_frame_list *frame;
	struct list_head *pos, *n;
	int ret, vinum, channel;
	void *src_vaddr = NULL;

	vinum = frame_channel / 3;
	channel = frame_channel % 3;
	printk("%s vinum:%d channel:%d\n",__func__,vinum,channel);
	list_for_each_safe(pos, n, &dma_alloc_frame_list[vinum * 3 + channel]) {
		frame = list_entry(pos, struct fast_frame_list, flist);
		if(frame->info.vinum == vinum && frame->info.channel == channel && !frame->busy) {
			tisp_nv12_attr_t snap_nv12_attr;
			snap_nv12_attr.vinum = frame->info.vinum;
			snap_nv12_attr.channel = frame->info.channel;
			snap_nv12_attr.snapnum = 1;
			snap_nv12_attr.imagesize = (((frame->info.width + 0xF) & ~0xF) * ((frame->info.height + 0xF) & ~0xF) * 3 / 2);
			snap_nv12_attr.firstflag = 0;
			snap_nv12_attr.savepaddr = frame->info.kpaddr;
			printk("[%s %d]get nv12 vinum:%d channel:%d imagesize:%d savepaddr:0x%x\n",__func__,__LINE__, frame->info.vinum, frame->info.channel, snap_nv12_attr.imagesize,frame->info.kpaddr);
			ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_GET_NV12_CONTROL,(unsigned long)&snap_nv12_attr);
			if(ret < 0) {
				printk("tx_isp_unlocked_ioctl_first failed\n");
			}
			frame->busy = true;
			//break;
		}
	}
	return 0;
}


int zrt_isp_dma_free(int vinum, int channel, int frame_num)
{
	struct fast_frame_list *frame;
	struct list_head *pos, *n;

	printk("===%s vinum:%d channel:%d\n",__func__,vinum,channel);
	list_for_each_safe(pos, n, &dma_alloc_frame_list[vinum * 3 + channel]) {
		frame = list_entry(pos, struct fast_frame_list, flist);
		if (frame->info.frame_num == frame_num && frame->info.vinum == vinum && frame->info.channel == channel){
			/* dma_free_coherent(NULL, frame->info.alloc_size, frame->info.kvaddr, frame->info.kpaddr); */
			kfree(frame->info.kvaddr);
			list_del(pos);
			kfree(frame);
			printk(KERN_INFO "Deleted fastframe with Frame Num: %d\n", frame_num);
			return 0;
		}
	}
	return -EFAULT;
}
static long zeratul_frame_proc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case ZRT_IOCTL_GET_DMA_FRAME:
		{
			struct fast_frame_info user_frame_info;
			struct fast_frame_info *kernel_frame_info = NULL;
			if (copy_from_user(&user_frame_info, (struct fast_frame_info __user *)arg, sizeof(struct fast_frame_info)))
				return -EFAULT;
			kernel_frame_info = (struct fast_frame_info*)zrt_isp_dma_getframe(user_frame_info.vinum,user_frame_info.channel, user_frame_info.frame_num);
			if(kernel_frame_info){
				if (copy_to_user((struct fast_frame_info __user *)arg, kernel_frame_info, sizeof(struct fast_frame_info)))
					return -EFAULT;
			}else{
				return -EFAULT;
			}
		}
		break;
	case ZRT_IOCTL_FREE_DMA_FRAME:
		{
			struct fast_frame_info user_frame_info;
			if (copy_from_user(&user_frame_info, (struct fast_frame_info __user *)arg, sizeof(struct fast_frame_info)))
				return -EFAULT;
			return zrt_isp_dma_free(user_frame_info.vinum,user_frame_info.channel, user_frame_info.frame_num);
		}
		break;
	default:
		printk("Proc zeratul dir cmd invalue :%x, %x %x\n",cmd,ZRT_IOCTL_GET_DMA_FRAME, ZRT_IOCTL_FREE_DMA_FRAME);
		return -ENOTTY;
	}

	return 0;
}
static const struct file_operations zrt_fast_frame_proc_fops = {
	.owner = NULL,
	.unlocked_ioctl = zeratul_frame_proc_ioctl,
};
struct channel_snap_frame snap_yuv0;
struct channel_snap_frame snap_yuv1;
void channel_irq_snap_frame_work0(struct work_struct *work)
{
	struct channel_snap_frame *data = container_of(work, struct channel_snap_frame, snap_work);
	zrt_isp_dma_copyframe(data->channel);
}

void channel_irq_snap_frame_work1(struct work_struct *work)
{
	struct channel_snap_frame *data = container_of(work, struct channel_snap_frame, snap_work);
	zrt_isp_dma_copyframe(data->channel);
}
#endif/*KERNEL_SNAP_YUV*/
#if KERNEL_SNAP_RAW
void vic_irq_snap_raw_work(struct work_struct *work)
{
	struct vic_snap_frame *data = container_of(work, struct vic_snap_frame, snap_work);
	lib_tisp_save_raw_t info = {
		.rawType = TISP_RAW_16,
		.paddr = ispmem_base, /* ispmem */
		.vaddr = 0x0,
		.frameNum = 1,
		.wdrMode = 0,
		.frameIndex = 1,/* 1:first frame  range:1-10 */
		.back0 = 0,
		.back1 = 0,
		.back2 = 0,
		.back3 = 0,
		.back4 = 0,
		.back5 = 0
	};
	printk("start snap raw\n");
	lib_tisp_save_specified_frame_raw(data->vinum, &info);
}
#endif

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

int fast_framesource_createjoint(IMPFSJointAttr *joint_attr)
{
	IMPFSChnAttr *chan = NULL;
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t chn_cnt = 0, k = 0;
	uint16_t line_height = 0;

	IMPFSJointManageAttr *joint_manage_attr = (IMPFSJointManageAttr *)kmalloc(sizeof(IMPFSJointManageAttr),GFP_KERNEL);
	if (!joint_manage_attr) {
		pr_err("[%s][%d] malloc joint manage attr failed\n", __func__, __LINE__);
		return -1;
	}
	memset(joint_manage_attr, 0, sizeof(IMPFSJointManageAttr));
	/* 获取拼接的通道号，分辨率的宽高信息，计算每行的宽度，每行最大高度，每行的通道个数，行数，所有拼接通道的个数 */
	joint_manage_attr->chn_flag = 0;
	for (i = 0; ((i < 12) && (joint_attr->position[i][j] != 0xff)); i++) {
		joint_manage_attr->line_width[i] = 0;
		for (j = 0; ((j < 12) && (joint_attr->position[i][j] != 0xff)); j++) {
			chan = &g_chn[joint_attr->position[i][j]];
			if (chan->joint_handler == NULL) {
				chan->joint_handler = joint_manage_attr;
			} else {
				kfree(joint_manage_attr);
				pr_err("The channel already joint (%d).\n", joint_attr->position[i][j]);
				return -1;
			}
			joint_manage_attr->chn_flag |= (1 << joint_attr->position[i][j]);
			joint_manage_attr->base[chn_cnt].chn = joint_attr->position[i][j];
			joint_manage_attr->base[chn_cnt].width = chan->picWidth;
			joint_manage_attr->base[chn_cnt].height = chan->picHeight;
			joint_manage_attr->line_width[i] += chan->picWidth;

			if ((j == 0) || (chan->picHeight > joint_manage_attr->line_height_max[i])) {
				joint_manage_attr->line_height_max[i] = chan->picHeight;
			}
			chn_cnt++;
		}
		joint_manage_attr->chans_per_row[i] = j;
		j = 0;
	}
	joint_manage_attr->chn_num = chn_cnt;
	joint_manage_attr->rows_num = i;
	/* 遍历最大宽度和最大高度 */
	joint_manage_attr->height_max = 0;
	for(i = 0; i < joint_manage_attr->rows_num; i++) {
		if ((i == 0) || (joint_manage_attr->line_width[i] > joint_manage_attr->width_max)) {
			joint_manage_attr->width_max = joint_manage_attr->line_width[i];
		}
		joint_manage_attr->height_max += joint_manage_attr->line_height_max[i];
	}
	joint_manage_attr->stride = joint_manage_attr->width_max;

	/* 计算Y/UV的偏移地址 */
	chn_cnt = 0;
	for (i = 0; ((i < 12) && (joint_attr->position[i][j] != 0xff)); i++) {
		for (j = 0; ((j < 12) && (joint_attr->position[i][j] != 0xff)); j++) {
			if (j == 0) {
				line_height = 0;
				for (k = 0; k < i; k++) {
					line_height += joint_manage_attr->line_height_max[k];
				}
				joint_manage_attr->base[chn_cnt].addr_offset = line_height * joint_manage_attr->width_max;
				joint_manage_attr->base[chn_cnt].addr_uv_offset = line_height * joint_manage_attr->width_max / 2;

				joint_manage_attr->chn[joint_attr->position[i][j]].addr_offset = line_height * joint_manage_attr->width_max;
				joint_manage_attr->chn[joint_attr->position[i][j]].addr_uv_offset = line_height * joint_manage_attr->width_max / 2;
			} else {
				joint_manage_attr->base[chn_cnt].addr_offset = joint_manage_attr->base[chn_cnt-1].addr_offset +joint_manage_attr->base[chn_cnt-1].width;
				joint_manage_attr->base[chn_cnt].addr_uv_offset = joint_manage_attr->base[chn_cnt-1].addr_uv_offset +joint_manage_attr->base[chn_cnt-1].width;

				joint_manage_attr->chn[joint_attr->position[i][j]].addr_offset = joint_manage_attr->base[chn_cnt-1].addr_offset +joint_manage_attr->base[chn_cnt-1].width;
				joint_manage_attr->chn[joint_attr->position[i][j]].addr_uv_offset = joint_manage_attr->base[chn_cnt-1].addr_uv_offset +joint_manage_attr->base[chn_cnt-1].width;
			}
			chn_cnt++;
			g_chn[joint_attr->position[i][j]].joint_handler = joint_manage_attr;
		}
		j = 0;
	}

	return 0;
}

int enc_qbuf(int chnNum,FrameInfo *frame)
{
	int ret = 0;
	static int frame_num[FS_CHN_MAX] = {0};
	struct tisp_buffer *buf = (struct tisp_buffer*)frame->isp_priv;

	/* printk("Enc Qbuf: %p buf_id = %d buf_ptr = %x buf_len = %d\n",buf, buf->index, buf->m.userptr, buf->length); */
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
	static int frame_num[FS_CHN_MAX] = {0}, snap_num[FS_CHN_MAX] = {0};
	struct tisp_buffer *buf = (struct tisp_buffer *)frame->isp_priv;
	int buffers = 0;

	buf->type = TISP_BUF_TYPE_VIDEO_CAPTURE;
	ret = frame_channel_unlocked_ioctl_first(chnNum,TISP_VIDIOC_DEFAULT_CMD_LISTEN_BUF,(unsigned long)buffers);
	if(ret != 0) {
		printk(KERN_ERR "TISP_VIDIOC_DEFAULT_CMD_LISTEN_BUF failed\n");
	}

	ret = frame_channel_unlocked_ioctl_first(chnNum,TISP_VIDIOC_DQBUF,(unsigned long)buf);
	if(ret != 0) {
		printk(KERN_ERR "frame_channel_vb2_dqbuf failed\n");
	}
	/* printk("Enc DQbuf:%p buf_id = %d buf_ptr = %x buf_len = %d\n", buf,buf->index, buf->m.userptr, buf->length); */
	if((g_chn[chnNum].joint_handler) != NULL){
		buf->joint.uv_addr = g_chn[chnNum].joint_handler->buf[buf->index].paddr + (g_chn[chnNum].joint_handler->sizeimage * 2/3) + g_chn[chnNum].joint_handler->chn[chnNum].addr_uv_offset;
		buf->joint.stride = ((g_chn[chnNum].joint_handler->stride + 0xF) & ~0xF);
		buf->joint.state = 1;
		buf->joint.width = frame_channel_width[chnNum];
		buf->joint.height = frame_channel_height[chnNum];
	}else{
		buf->joint.uv_addr = buf->m.userptr + ((frame_channel_width[chnNum] + 0xF) & ~0xF) * ((frame_channel_height[chnNum] + 0xF) & ~0xF);
		buf->joint.stride = ((frame_channel_width[chnNum] + 0xF) & ~0xF);
		buf->joint.state = 1;
		buf->joint.width = frame_channel_width[chnNum];
		buf->joint.height = frame_channel_height[chnNum];
	}
	if((g_chn[chnNum].joint_handler) != NULL){
		frame->width = g_chn[chnNum].joint_handler->width_max;
		frame->height = g_chn[chnNum].joint_handler->height_max;
		frame->phyAddr = g_chn[chnNum].joint_handler->buf[buf->index].paddr;
	}else {
		frame->width = frame_channel_width[chnNum];
		frame->height = frame_channel_height[chnNum];
		frame->phyAddr = buf->m.userptr;
	}
	frame->virAddr = frame->phyAddr + 0xA0000000;
	frame->sensor_id = chnNum / 3;
	frame->timestamp = (int64_t)((int64_t)buf->timestamp.tv_sec * 1000000 + (int64_t)buf->timestamp.tv_usec);

	if( ipuosd_enable ){
		if(( direct_mode == 0 ) || (direct_mode >= 1 && (chnNum%3 != 0))){
			ret = ipu_osd_set(chnNum, buf->m.userptr,frame_channel_width[chnNum],(frame_channel_height[chnNum] + 0xF) & ~0xF);
			if(ret != 0) {
				printk(KERN_ERR "osd_init failed\n");
			}
		}
	}
	frame_num[chnNum] ++;
#if KERNEL_SNAP_YUV
//	if(chnNum == 2 || chnNum == 5)
	if(chnNum == 2)
	{
		if(frame_num[chnNum] >= (SNAP_YUV_FRAMENUM)){
			if(snap_num[chnNum] < SNAP_YUV_MAX){
				zrt_isp_copyframe(chnNum, frame->virAddr);
				snap_num[chnNum] ++;
			}
		}
	}
#endif
	return ret;
}
#ifdef CONFIG_JZ_VPU_KERN_ENCODE
static int enc_video(unsigned int addr)
{
	struct tx_isp_frame_channel *chan=NULL;
	int i = 0,ret = 0;
	int chnNum = 0;
	VPUEncoderAttr chnAttr;
	unsigned int bs_addr = addr;
	int enc_size = 0;
	int joint_enc_chn[12] = {0,3};

	VPU_Kern_Init();
#ifdef JOINT_MODE
	ret = VPU_Encoder_SetJointAttr(joint_enc_chn, 2);
	if(ret < 0){
		printk(KERN_ERR "fast set encoder joint faield\n");
		return -1;
	}
#endif
	vpuBs_size = 500 * 1024;
	printk("VpuBsaddr:0x%x vpuBs_size:%d\n",bs_addr,vpuBs_size);
	VPU_Encoder_SetVpuBsSize(bs_addr,vpuBs_size);
	for( i=0; i < FS_CHN_MAX; i++){
		chan = IS_ERR_OR_NULL(g_f_mdev[i]) ? NULL : miscdev_to_frame_chan(g_f_mdev[i]);
		if( ! IS_ERR_OR_NULL(chan)){
			memset(&chnAttr,0,sizeof(VPUEncoderAttr));
			if(chan->state == TX_ISP_MODULE_RUNNING){
				chnNum = chan->index;
				chnAttr.enType = PT_H265;
				if((g_chn[chnNum].joint_handler) != NULL){
					chnAttr.picWidth = g_chn[chnNum].joint_handler->width_max;
					chnAttr.picHeight = g_chn[chnNum].joint_handler->height_max;
				}else{
					chnAttr.picWidth = frame_channel_width[chnNum];
					chnAttr.picHeight = frame_channel_height[chnNum];
				}
				chnAttr.fps = 15;
				chnAttr.Iqp = 40;
				chnAttr.Pqp = 35;
				//				chnAttr.maxPicSize = 0; //Kbits
				chnAttr.strLen = (frame_channel_width[chnNum] * ((frame_channel_height[chnNum] + 0xF) & ~0xF) /4);//save kernel stream buf size
				if( (direct_mode == 1 && (chnNum == 0)) || \
				    (direct_mode == 2 && (chnNum == 0 || chnNum == 3)) || \
				    (direct_mode == 3 && (chnNum == 0 || chnNum == 3 || chnNum == 6)) || \
				    (direct_mode == 4 && (chnNum == 0 || chnNum == 3 || chnNum == 6 || chnNum == 9)))
				{
					chnAttr.bEnableIvdc = true;
				}
				if(chnNum == 0){
					chnAttr.encPhyaddr = bs_addr + vpuBs_size;//offset: ispbuf + vb
				}else{
					chnAttr.encPhyaddr = bs_addr + vpuBs_size + enc_size;//offset: ispbuf + vb + bs + enc0
				}
				enc_size += VPU_Encoder_GetChannelBufferSize(&chnAttr);
				printk("chnNum:%d encPhyaddr:0x%x enc_size all:%d\n",chan->index, chnAttr.encPhyaddr,enc_size);
				ret = VPU_Encoder_CreateChn(chnNum, &chnAttr);
				if(ret < 0){
					printk(KERN_ERR "fast encoder create chn%d faield\n",chnNum);
					return -1;
				}
			}
		}
	}
	addr = bs_addr + vpuBs_size + enc_size;
	return addr;
}
#endif

static int find_max_height(void) {
	int max_height = frame_channel_height[0];
	int i = 0,height = 0;;
	for (i = 0; i < sensor_number; i++) {
		height = frame_channel_height[i * 3];
		if (height > max_height) {
			max_height = height;
		}
	}
	return max_height;
}

static int set_ivdc_addr(unsigned int addr)
{
	int ret = 0;
	int stride = 0;
	ivdc_paddr = addr;
	int max_height = 0;

	if(sensor_number > 1){
		max_height = find_max_height();
	}
	if(direct_mode != 0){
		if(direct_mode == 1){
			stride = (frame_channel_width[0] + 63) & (~63);
			if (0 == ivdc_mem_line) {
				/* default 1/2 height */
				sizeimage = stride * frame_channel_height[0] * 3 / 4;
				ivdc_mem_line = frame_channel_height[0] / 2;
			} else {
				sizeimage = stride * ivdc_mem_line * 3 / 2;
			}
		}
		if(direct_mode > 1){
			stride = (ivdc_mem_stride + 63) & (~63);
			if (0 == ivdc_mem_line) {
				/* default 1/2 max height */
				sizeimage = stride * max_height * 3 / 4;
				ivdc_mem_line = max_height / 2;
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
	int buf_i = 0,snap_i = 0;
	struct tisp_buffer buf;
	static int j_first_alloc = 0;

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
#if KERNEL_SNAP_YUV
//	if(chn_num == 2  || chn_num == 5){
	if(chn_num == 2){
		struct fast_frame_info info;
		for( snap_i=0; snap_i < SNAP_YUV_MAX; snap_i++){
			info.vinum = chan->index / 3;
			info.channel = chan->index % 3;
			info.frame_num = snap_i;
			info.width = ((frame_channel_width[chan->index] + 0xF) & ~0xF);
			info.height = ((frame_channel_height[chan->index] + 0xF) & ~0xF);
			info.alloc_size = (((frame_channel_width[chan->index] + 0xF) & ~0xF) * ((frame_channel_height[chan->index] + 0xF) & ~0xF) * 3 / 2);
			info.src_kpaddr = (void *)addr;
			ret = zrt_isp_dma_alloc(&info);
			if(ret){
				printk("alloc mem%d on CMA failed\n",snap_i);
			}
		}
	}
#if 0
	if(chn_num == 2){
		snap_yuv0.channel = chn_num;
		INIT_WORK(&snap_yuv0.snap_work, channel_irq_snap_frame_work0);
		schedule_work(&snap_yuv0.snap_work);
	}
	if(chn_num == 5){
		snap_yuv1.channel = chn_num;
		INIT_WORK(&snap_yuv1.snap_work, channel_irq_snap_frame_work1);
		schedule_work(&snap_yuv1.snap_work);
	}
#endif
#endif
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

	uint32_t temp_align = 0;
	for(buf_i = 0; buf_i < count; buf_i++) {
		memset(&buf, 0, sizeof(struct tisp_buffer));

		buf.type = TISP_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = TISP_MEMORY_USERPTR;
		buf.index = buf_i;
		buf.length = frame_channel_width[chn_num] * frame_channel_height[chn_num] * 3 / 2;
		if((g_chn[chn_num].joint_handler) != NULL){
			if(j_first_alloc == 0){
				g_chn[chn_num].joint_handler->sizeimage = g_chn[chn_num].joint_handler->width_max*((g_chn[chn_num].joint_handler->height_max+15)&~15)* 3 / 2;
				joint_sizeimage = g_chn[chn_num].joint_handler->sizeimage;
				joint_vb_chn = chn_num;
				printk("joint_vb_chn:%d width_max:%d height_max%d joint_sizeimage:%d\n", joint_vb_chn, g_chn[chn_num].joint_handler->width_max, (g_chn[chn_num].joint_handler->height_max+15)&~15, joint_sizeimage);
			}
			temp_align = g_chn[chn_num].joint_handler->chn[chn_num].addr_offset;/* offset */
		}else{
			temp_align = (((frame_channel_width[chn_num] + 0xF) & ~0xF) * ((frame_channel_height[chn_num] + 0xF) & ~0xF) * 3 / 2);
		}
		switch(direct_mode){
			case 4:
				if(chn_num == 0 || chn_num == 3 || chn_num == 6 || chn_num == 9)
					buf.m.userptr = 0x80000000;
				else
					buf.m.userptr = addr + buf_i * temp_align;
				break;
			case 3:
				if(chn_num == 0 || chn_num == 3 || chn_num == 6)
					buf.m.userptr = 0x80000000;
				else
					buf.m.userptr = addr + buf_i * temp_align;
				break;
			case 2:
				if(chn_num == 0 || chn_num == 3)
					buf.m.userptr = 0x80000000;
				else
					buf.m.userptr = addr + buf_i * temp_align;
				break;
			case 1:
				if(chn_num == 0)
					buf.m.userptr = 0x80000000;
				else
					buf.m.userptr = addr + buf_i * temp_align;
				break;
			case 0:
				if((g_chn[chn_num].joint_handler) != NULL){
					if(j_first_alloc == 0){
						g_chn[chn_num].joint_handler->buf[buf_i].paddr = addr + buf_i * joint_sizeimage;
						g_chn[chn_num].joint_handler->buf[buf_i].vaddr = ioremap(g_chn[chn_num].joint_handler->buf[buf_i].paddr,joint_sizeimage);
						memset(g_chn[chn_num].joint_handler->buf[buf_i].vaddr, 0 , joint_sizeimage*2/3);
						memset(g_chn[chn_num].joint_handler->buf[buf_i].vaddr +  joint_sizeimage*2/3 , 128, joint_sizeimage/ 3);
						iounmap(g_chn[chn_num].joint_handler->buf[buf_i].vaddr);
					}
					buf.m.userptr = g_chn[chn_num].joint_handler->buf[buf_i].paddr + temp_align;/* offset */
				}else{
					buf.m.userptr = addr + buf_i * temp_align;
				}
				break;
		}
		printk("qbuf: buf_id = %d buf_ptr = 0x%lx buf_len = %d temp_align = %d\n", buf.index, buf.m.userptr, buf.length, temp_align);
		if((g_chn[chn_num].joint_handler) != NULL){
			buf.joint.uv_addr = g_chn[chn_num].joint_handler->buf[buf_i].paddr + (g_chn[chn_num].joint_handler->sizeimage * 2/3) + g_chn[chn_num].joint_handler->chn[chn_num].addr_uv_offset;
			buf.joint.stride = ((g_chn[chn_num].joint_handler->stride + 0xF) & ~0xF);//16BYTE
			buf.joint.state = 1;
			buf.joint.width = frame_channel_width[chn_num];
			buf.joint.height = frame_channel_height[chn_num];
		}else{
			buf.joint.uv_addr = buf.m.userptr + ((frame_channel_width[chn_num] + 0xF) & ~0xF) * ((frame_channel_height[chn_num] + 0xF) & ~0xF);
			buf.joint.stride = ((frame_channel_width[chn_num] + 0xF) & ~0xF);//16BYTE
			buf.joint.state = 1;
			buf.joint.width = frame_channel_width[chn_num];
			buf.joint.height = frame_channel_height[chn_num];
		}
		ret = frame_channel_unlocked_ioctl_first(chan->index,TISP_VIDIOC_QBUF,(unsigned long)&buf);
		if(ret != 0) {
			printk(KERN_ERR "frame_channel_vb2_qbuf failed\n");
		}
	}


	if( (direct_mode == 1 && chn_num == 0) || \
	    (direct_mode == 2 && (chn_num == 0 || chn_num == 3)) || \
	    (direct_mode == 3 && (chn_num == 0 || chn_num == 3 || chn_num == 6)) || \
	    (direct_mode == 4 && (chn_num == 0 || chn_num == 3 || chn_num == 6 || chn_num == 9)))
	{
		;
	}else{
		//undirect
		if((g_chn[chn_num].joint_handler) != NULL){
			if(j_first_alloc == 0){
				addr = addr + count * joint_sizeimage;
				j_first_alloc = 1;
			}
		}else{
			addr = addr + count * temp_align;
		}
	}
	return addr;
}

int riscv_printf(void)
{
	unsigned char* log_head;
	int log_offset = 0;

	log_head = (unsigned char *)(rmem_base + rmem_size - 32*1024);
	log_head = (unsigned char *)(0xa0000000 | (unsigned long)log_head);

	const int max_log_size = 32 * 1024;
	char buf[256];
	int buf_pos = 0;
	int ff_run = 0;  // 连续 0xFF 计数

	printk(KERN_INFO "\n[risc-v] RISC-V core Log start:\n");

	while (log_offset < max_log_size) {
		unsigned char c = log_head[log_offset++];
		if (c == 0x00)
			break;

		// 如果连续多个字节都是0xFF，可能是无效数据区域
		if (c == 0xFF) {
			ff_run++;
			if (ff_run >= 8) {
				break;
			}
		} else {
			ff_run = 0;
		}

		buf[buf_pos++] = c;

		if (buf_pos >= sizeof(buf)-1 || c == '\n') {
			buf[buf_pos] = '\0';
			printk(KERN_INFO "[risc-v] %s", buf);
			buf_pos = 0;
		}
	}

	if (buf_pos > 0) {
		buf[buf_pos] = '\0';
		printk(KERN_INFO "[risc-v] %s\n", buf);
	}

	return 0;
}

extern int32_t tisp_s_mdns_riscvEn(uint8_t MdnsEn);
extern int tx_isp_get_riscv_night_mode(void);
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
	if(sensor_number >= 2 && isp_cache_line == 0){
		ISP_WARNING("use default zrt_isp_cache_line:%d\n",zrt_isp_cache_line);
		isp_cache_line = zrt_isp_cache_line;
	}
	DEBUG_TTFF("tx_isp_stop_riscv start");
	ret = tx_isp_riscv_is_run();
	/* return 0; */
	if (ret < 0) {
		riscv_is_run = 0;
	} else {
		riscv_is_run = 1;
		DEBUG_TTFF("wait riscv frame done");
		timeout = 2000;
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
		if (sensor_number == 3) {
			ret = tx_isp_get_tiziano_para(1);
			if(ret < 0) {
				printk("tx_isp_get_tiziano_para failed\n");
				return ret;
			}
			ret = tx_isp_get_tiziano_para(2);
			if(ret < 0) {
				printk("tx_isp_get_tiziano_para failed\n");
				return ret;
			}
		}
		if (sensor_number == 4) {
			ret = tx_isp_get_tiziano_para(1);
			if(ret < 0) {
				printk("tx_isp_get_tiziano_para failed\n");
				return ret;
			}
			ret = tx_isp_get_tiziano_para(2);
			if(ret < 0) {
				printk("tx_isp_get_tiziano_para failed\n");
				return ret;
			}
			ret = tx_isp_get_tiziano_para(3);
			if(ret < 0) {
				printk("tx_isp_get_tiziano_para failed\n");
				return ret;
			}
		}
	}
	if(mdns_en == 1)
		tisp_s_mdns_riscvEn(0);
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

	if(riscv_is_run == 1){
		(void)riscv_printf();
	}
	tiziano_fastpara_exit();

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
	if(vi_num == 0 || vi_num == 2){
		/* sensor_register_info.switch_gpio = GPIO_PB(06); */
		sensor_register_info.switch_gpio = GPIO_PC(02);
	}else if(vi_num == 1 || vi_num == 3){
		/* sensor_register_info.switch_gpio = GPIO_PB(07); */
		sensor_register_info.switch_gpio = GPIO_PC(04);
	}
	/* sensor_register_info.switch_gpio_state = -1; */
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

int set_ipc_buf(unsigned int rmem_addr)
{
	int i = 0, ret = 0;
	char str_buf[64] = {0};
	unsigned int ipcX_start_addr = 0, ipcX_end_addr = 0;
	ipcX_end_addr = rmem_base + rmem_size - TISP_RISCV_MEM_SIZE;

	for(i=0; i < sensor_number; i++){
		//get ipc size
		g_ipc_buf[i].vinum = i;
		ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_GET_ISP_IPC_MEM_INFO,(unsigned long)&g_ipc_buf[i]);
		if(ret < 0) {
			printk("GET_ISP_IPC_MEM_INFO failed\n");
			return ret;
		}
		if(g_ipc_buf[i].size == 0){
			g_ipc_buf[i].paddr = 0;
		}else{
			ipcX_start_addr = ipcX_end_addr -  g_ipc_buf[i].size;
			if(ipcX_start_addr < rmem_addr){
				printk("IPC buffer start paddr error\n");
				return ret;
			}
			ipcX_end_addr = ipcX_start_addr;
			g_ipc_buf[i].paddr = ipcX_start_addr;
		}
		printk("Sensor%d IPC Buf: size = %d  paddr = 0x%x\n", i, g_ipc_buf[i].size, g_ipc_buf[i].paddr);

		ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_ISP_IPC_MEM_INFO,(unsigned long)&g_ipc_buf[i]);
		if(ret < 0) {
			printk("SET_ISP_IPC_MEM_INFO failed\n");
			return ret;
		}
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

void _jz_osd_exit()
{
	struct task_struct *osd_task;
	if( ipuosd_enable ){
		osd_task = ipuosdparam.osd_task;
		if (osd_task && !IS_ERR(osd_task)) {
			kthread_stop(osd_task);
			ipuosdparam.osd_task = NULL; // 防止重复释放
		}
		ipu_deinit();
	}

	if( isposd_enable ){
		osd_task = isposdparam.osd_task;
		if (osd_task && !IS_ERR(osd_task)) {
			kthread_stop(osd_task);
			isposdparam.osd_task = NULL; // 防止重复释放
		}
	}
	return;
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
	int vi_num = 0, i = 0,j = 0,default_boot = 0, i2c_adaptor_id, iq_index = 0;
	unsigned int channel_buf_addr = 0;
	struct wdr_open_attr wdr_attr;
	static uint32_t *timeStampData_ipu,*timeStampData_isp;
	int get_or_set;//0 set 1 get
	enum tisp_buf_type type;
	struct tx_isp_version v_info;

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

	memset(&v_info, 0, sizeof(struct tx_isp_version));
	ret = tx_isp_unlocked_ioctl_first(TISP_VIDIOC_DRIVER_VERSION,(unsigned long)&v_info);
	if(ret < 0) {
		printk("get isp driver version failed\n");
		return ret;
	}

#ifdef CONFIG_SOC_PRJ008
	if(strcmp(TISP_FAST_PRJ008_VERSION, v_info.version) != 0){
		pr_err("ISP FAST version(%s) is not match isp driver version(%s) !\n", TISP_FAST_PRJ008_VERSION, v_info.version);
		return -1;
	}
#endif
#ifdef CONFIG_SOC_PRJ007
	if(strcmp(TISP_FAST_PRJ007_VERSION, v_info.version) != 0){
		pr_err("ISP FAST version(%s) is not match isp driver version(%s) !\n", TISP_FAST_PRJ007_VERSION, v_info.version);
		return -1;
	}
#endif

	//set wdr mode
	sensor_wdr_mode = fast_sensors[0].wdr;
	printk("sensor_wdr_mode = %d user_wdr_mode = %d\n", sensor_wdr_mode, user_wdr_mode);
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

	tisp_switch_bin_t st_bin_attr;
	memset(&st_bin_attr, 0, sizeof(tisp_switch_bin_t));
	if (sensor_wdr_mode) {
		if(user_wdr_mode == 1) {
			printk("only WDR mode\n");
			st_bin_attr.vinum = vi_num;
			st_bin_attr.updateCsccr = 1;
			st_bin_attr.wdr_mode = TISP_WDR_MODE;
			iq_index = get_sensor_start_iq_index(vi_num);
			if(g_is_night == IR_STATUS_NIGHT){
				st_bin_attr.dn_mode = TISP_DN_NIGHT;
				st_bin_attr.paddr = virt_to_phys((void *)get_sensor_setting(st_bin_attr.vinum,iq_index,TISP_WDR_NIGHT));
				st_bin_attr.size = get_sensor_setting_len(st_bin_attr.vinum,iq_index,TISP_WDR_NIGHT);
			}
			else{
				st_bin_attr.dn_mode = TISP_DN_DAY;
				st_bin_attr.paddr = virt_to_phys((void *)get_sensor_setting(st_bin_attr.vinum,iq_index,TISP_WDR_DAY));
				st_bin_attr.size = get_sensor_setting_len(st_bin_attr.vinum,iq_index,TISP_WDR_DAY);
			}
            tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_TIZIANO_PAR,(unsigned long)&st_bin_attr);
		} else {
			printk("LINEAR mode based on WDR OPEN\n");
//          tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_ISP_WDR_OPEN,.....);
			st_bin_attr.vinum = vi_num;
			st_bin_attr.updateCsccr = 1;
			st_bin_attr.wdr_mode = TISP_LINEAR_MODE;
			iq_index = get_sensor_start_iq_index(vi_num);
			if(g_is_night == IR_STATUS_NIGHT){
				st_bin_attr.dn_mode = TISP_DN_NIGHT;
				st_bin_attr.paddr = virt_to_phys((void *)get_sensor_setting(st_bin_attr.vinum,iq_index,TISP_LINEAR_NIGHT));
				st_bin_attr.size = get_sensor_setting_len(st_bin_attr.vinum,iq_index,TISP_LINEAR_NIGHT);
			}
			else{
				st_bin_attr.dn_mode = TISP_DN_DAY;
				st_bin_attr.paddr = virt_to_phys((void *)get_sensor_setting(st_bin_attr.vinum,iq_index,TISP_LINEAR_DAY));
				st_bin_attr.size = get_sensor_setting_len(st_bin_attr.vinum,iq_index,TISP_LINEAR_DAY);
			}
            tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_TIZIANO_PAR,(unsigned long)&st_bin_attr);
		}
	}else{
			printk("only LINEAR mode\n");
			for ( i=0; i < sensor_number; i++) {
				st_bin_attr.vinum = i;
				st_bin_attr.updateCsccr = 1;
				st_bin_attr.wdr_mode = TISP_LINEAR_MODE;
				iq_index = get_sensor_start_iq_index(st_bin_attr.vinum);
				if(g_is_night == IR_STATUS_NIGHT){
					st_bin_attr.dn_mode = TISP_DN_NIGHT;
					st_bin_attr.paddr = virt_to_phys((void *)get_sensor_setting(st_bin_attr.vinum,iq_index,TISP_LINEAR_NIGHT));
					st_bin_attr.size = get_sensor_setting_len(st_bin_attr.vinum,iq_index,TISP_LINEAR_NIGHT);
				}
				else{
					st_bin_attr.dn_mode = TISP_DN_DAY;
					st_bin_attr.paddr = virt_to_phys((void *)get_sensor_setting(st_bin_attr.vinum,iq_index,TISP_LINEAR_DAY));
					st_bin_attr.size = get_sensor_setting_len(st_bin_attr.vinum,iq_index,TISP_LINEAR_DAY);
				}
				tx_isp_unlocked_ioctl_first(TISP_VIDIOC_SET_TIZIANO_PAR,(unsigned long)&st_bin_attr);
			}
	}
	vi_num = 0;
	default_boot = 0;
	i2c_adaptor_id = 0;
	ret = register_sensor(0, fast_sensors[0].name, fast_sensors[0].i2c_addr, i2c_adaptor_id, -1, TISP_SENSOR_VI_MIPI_CSI0, TISP_SENSOR_MCLK0, default_boot);
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
		if(i2c_num == 2)
			i2c_adaptor_id = 1;
		else
			i2c_adaptor_id = 0;
		ret = register_sensor(vi_num, fast_sensors[1].name, fast_sensors[1].i2c_addr, i2c_adaptor_id, -1, TISP_SENSOR_VI_MIPI_CSI1, TISP_SENSOR_MCLK1, default_boot);
		if(ret < 0){
			return ret;
		}
		channel_buf_addr = set_isp_buf(1, channel_buf_addr);
	}
	if (sensor_number == 3) {
		vi_num = 1;
		default_boot = 0;
		if(i2c_num == 2)
			i2c_adaptor_id = 1;
		else
			i2c_adaptor_id = 0;
		ret = register_sensor(vi_num, fast_sensors[1].name, fast_sensors[1].i2c_addr, i2c_adaptor_id, -1, TISP_SENSOR_VI_MIPI_CSI1, TISP_SENSOR_MCLK1, default_boot);
		if(ret < 0){
			return ret;
		}
		channel_buf_addr = set_isp_buf(1, channel_buf_addr);
		printk("sensor1 channel_buf_addr %x \n",channel_buf_addr);
		//sensor_number = 3 ;
		vi_num = 2;
		default_boot = 0;
		if(i2c_num == 2)
			i2c_adaptor_id = 0;
		else
			i2c_adaptor_id = 0;
		ret = register_sensor(vi_num, fast_sensors[2].name, fast_sensors[2].i2c_addr, i2c_adaptor_id, -1, TISP_SENSOR_VI_MIPI_CSI0, TISP_SENSOR_MCLK0, default_boot);
		if(ret < 0){
			return ret;
		}
		channel_buf_addr = set_isp_buf(2, channel_buf_addr);
		printk("sensor2 channel_buf_addr %x \n",channel_buf_addr);
	}
	if (sensor_number == 4) {
		vi_num = 1;
		default_boot = 0;
		if(i2c_num == 2)
			i2c_adaptor_id = 1;
		else
			i2c_adaptor_id = 0;
		ret = register_sensor(vi_num, fast_sensors[1].name, fast_sensors[1].i2c_addr, i2c_adaptor_id, -1, TISP_SENSOR_VI_MIPI_CSI0, TISP_SENSOR_MCLK0, default_boot);
		if(ret < 0){
			return ret;
		}
		channel_buf_addr = set_isp_buf(1, channel_buf_addr);
		//sensor_number = 3 ;
		vi_num = 2;
		default_boot = 0;
		if(i2c_num == 2)
			i2c_adaptor_id = 0;
		else
			i2c_adaptor_id = 0;
		ret = register_sensor(vi_num, fast_sensors[2].name, fast_sensors[2].i2c_addr, i2c_adaptor_id, -1, TISP_SENSOR_VI_MIPI_CSI1, TISP_SENSOR_MCLK1, default_boot);
		if(ret < 0){
			return ret;
		}
		channel_buf_addr = set_isp_buf(2, channel_buf_addr);

		vi_num = 3;
		default_boot = 0;
		if(i2c_num == 2)
			i2c_adaptor_id = 1;
		else
			i2c_adaptor_id = 0;
		ret = register_sensor(vi_num, fast_sensors[3].name, fast_sensors[3].i2c_addr, i2c_adaptor_id, -1, TISP_SENSOR_VI_MIPI_CSI1, TISP_SENSOR_MCLK1, default_boot);
		if(ret < 0){
			return ret;
		}
		channel_buf_addr = set_isp_buf(3, channel_buf_addr);
	}

    //set ipc buf
	ret = set_ipc_buf(rmem_base);
	if(ret < 0){
		pr_err("set ipc buf failed\n");
		return  -1;
	}

#if KERNEL_SNAP_RAW
	snap_raw.vinum= 0;
	INIT_WORK(&snap_raw.snap_work, vic_irq_snap_raw_work);
	schedule_work(&snap_raw.snap_work);
#endif
	tisp_enable_tuning();

	vi_num = 0;
	printk("fs0 start ev is %d\n", g_riscv_isp_ev);

#if 0
	//enable debug info per frame
	lib_tisp_api_get_3a_info(0,50);
	if(sensor_number ==2 )
		lib_tisp_api_get_3a_info(1,50);
#endif
#if 0
	tisp_hv_flip_t flip;
	memset(&flip,0,sizeof(tisp_hv_flip_t));
	flip.isp_mode[0] = ISP_FLIP_ISP_HV_MODE;

	ret = isp_core_tunning_unlocked_ioctl_fast(vi_num, 0, IMAGE_TUNING_CID_HV_FLIP, (unsigned long)&flip);
	if (ret) {
		printk(KERN_ERR "isp_core_tunning_unlocked_ioctl_fast HV_FLIP failed for sensor: %d\n", vi_num);
	}
#endif
#if 1
	/* set fps */
	get_or_set = 0;
	int32_t tmp = 0;
	int32_t fps = 15;
	tmp = (fps << 16) | 1;
	isp_core_tunning_unlocked_ioctl_fast(vi_num, get_or_set, IMAGE_TUNING_CID_CONTROL_FPS, (unsigned long)tmp);
	if( sensor_number == 2 ){
		isp_core_tunning_unlocked_ioctl_fast(1, get_or_set, IMAGE_TUNING_CID_CONTROL_FPS, (unsigned long)tmp);
	}
#endif
#ifdef ISP_DISCARD_ENABLE
	/* enable discard frame */
	tisp_frame_drop_attr_t tfd[VI_MAX];
	memset(&tfd,0,sizeof(tisp_frame_drop_attr_t));
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
#if KERNEL_SNAP_YUV
	struct proc_dir_entry *p = NULL;
	for( i=0; i < FS_CHN_MAX; i++ ){
		INIT_LIST_HEAD(&dma_alloc_frame_list[i]);
	}
	p = create_proc_jz_zeratul(PROC_NAME);
	if (!p) {
		pr_warning("[%s %d]create_proc_entry for common reset failed.\n",__func__,__LINE__);
		return -ENODEV;
	}else{
		zrt_proc_entry_frame = proc_create_data("dma_frame", 0444, p, &zrt_fast_frame_proc_fops, NULL);
		if (!zrt_proc_entry_frame){
			pr_warning("Failed create zeratul dma_frame\n");
		}
	}
#endif
	memset(&g_chn, 0, sizeof(IMPFSChnAttr));
	for(i=0; i < FS_CHN_MAX; i++){
		if(frame_channel_width[i] > 0 && frame_channel_height[i] > 0 && frame_channel_nrvbs[i] > 0){
			g_chn[i].picWidth = frame_channel_width[i];
			g_chn[i].picHeight = frame_channel_height[i];
			g_chn[i].nrVBs = frame_channel_nrvbs[i];
		}
	}
#ifdef JOINT_MODE
	IMPFSJointAttr user_joint;
	memset(&user_joint, 0xff, sizeof(IMPFSJointAttr));
	user_joint.position[0][0] = 0;
	user_joint.position[1][0] = 3;
	ret = fast_framesource_createjoint(&user_joint);
	if(ret < 0){
		pr_err("create joint failed\n");
		return -1;
	}
#endif
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
	/* ISPOSD and IPUOSD init */
	/*create rgn*/
	if( gosd_enable != 0) {
		/*timeStamp buffer prepare*/
		if( gosd_enable == 1){
#ifdef CONFIG_SOC_PRJ007
			ipuosd_enable = 1;
#endif
		}else if( gosd_enable == 2){
			isposd_enable = 1;
		}else if( gosd_enable ==3 ){
#ifdef CONFIG_SOC_PRJ007
			ipuosd_enable = 1;
#endif
			isposd_enable = 1;
		}
		if( ipuosd_enable ) {
			printk("========================== ipu osd\n");
			timeStampData_ipu = kzalloc(OSD_LETTER_NUM * OSD_REGION_HEIGHT * OSD_REGION_WIDTH * sizeof(uint32_t), GFP_KERNEL);
			ipuosdparam.ptimestamps = timeStampData_ipu;
			ipu_osd_init(channel_buf_addr);
			ret = OSD_CreateRgn(0, timeStampData_ipu, TX_ISP_IPU_OSD_MODE);
			if (ret < 0) {
				printk("create ipuosd rgn failed\n");
			}

		}
		if( isposd_enable ) {
			printk("========================== isp osd\n");
			timeStampData_isp = kzalloc(OSD_LETTER_NUM * OSD_REGION_HEIGHT * OSD_REGION_WIDTH * sizeof(uint32_t), GFP_KERNEL);
			isposdparam.ptimestamps = timeStampData_isp;
			isp_osd_init();
			ret = OSD_CreateRgn(0, NULL, TX_ISP_ISP_OSD_MODE);
			if (ret < 0) {
				printk("create isposd rgn failed\n");
			}

		}
		printk("create osd rgn end\n");
	}
	if( gosd_enable != 0 ){
		if( ipuosd_enable ){
			ret = osd_thread_create(&ipuosdparam);
			if(ret < 0){
				printk("ipuosd_thread_create err!\n");
				return ret;
			}
		}
		if( isposd_enable){
			ISPOSD(&isposdparam);
		}
		printk("timestamp thread create end\n");
		msleep(2);
	}
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
#ifdef CONFIG_JZ_VPU_KERN_ENCODE
    /* enc video  */
    if(kernel_encode){
		ret = enc_video(channel_buf_addr);
		if(ret < 0){
			printk(KERN_ERR "enc_video faield\n");
			return -1;
		}
	}
#endif
	return 0;
}

static int tx_isp_channel_done_int_handler(int channel, int buffer_index)
{
	static int channel_done[FS_CHN_MAX] = {0};
	int ret = 0;
	if(!channel_done[channel]){
		channel_done[channel] = 1;
	}
	return 0;
}

static int tx_isp_frame0_done_int_handler(int count)
{
	/* disable discard frame */
#ifdef ISP_DISCARD_ENABLE
	if( count == 14){
		int i = 0, j = 0;
		tisp_frame_drop_attr_t tfd[VI_MAX];
		memset(&tfd,0,sizeof(tisp_frame_drop_attr_t));
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
	.tx_isp_channel_done_int_handler = tx_isp_channel_done_int_handler,
};

