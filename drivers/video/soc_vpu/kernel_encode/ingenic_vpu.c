#ifdef CONFIG_JZ_VPU_KERN_ENCODE
#include <asm/delay.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include "ingenic_vpu.h"
#include "vpu_common.h"
#include "vpu_core.h"
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H265
#include "h265_core.h"
#endif
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H264
#include "h264_core.h"
#endif
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_JPEG
#include "jpeg_core.h"
#endif

#define DEVICE_NAME "kern_enc"
#ifdef VPU_FILE_INPUT_TEST
#define VPU_CHN0_MEMORY_START 0x4000000
#define VPU_CHN0_FILE_INPUT_START 0x6000000
#endif

extern int hera_irq_for_kernEnc;
extern int enc_dqbuf(int chnNum, FrameInfo *frame);
extern int enc_qbuf(int chnNum, FrameInfo *frame);
extern int get_isp_bufsize(void);
extern int gosd_enable;

struct vpu_work_handle {
	struct list_head list;
	int chn_id;
	FrameInfo frame;
};

struct vpu_inst {
	int index;
	bool is_start;
	VPUPayloadType en_type;
	unsigned int buf_size;
	unsigned int *stream_buffer;
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int i_qp;
	unsigned int p_qp;
	unsigned int gop;
	bool is_ivdc;
	bool is_req_idr;

	unsigned int *tot_viraddr;
	unsigned int tot_phyaddr;
	unsigned int bs_phyaddr; //bitstream
	unsigned int *bs_viraddr;
	unsigned int raw_y_phyaddr; //src nv12
	unsigned int raw_c_phyaddr;
	unsigned int ref_y_phyaddr; //ref
	unsigned int ref_c_phyaddr;
	unsigned int dec_y_phyaddr; //dec
	unsigned int dec_c_phyaddr;
	unsigned int rds_y_phyaddr; //rdshare
	unsigned int rds_c_phyaddr;
	unsigned int des_phyaddr; //vdma
	unsigned int *des_viraddr; //vdma vir addr
	unsigned int vpu_phyaddr; //memory after vdma,used by vpu

	struct vpu_io_info io;
	struct kern_enc_str_info str_info[NUM_MAX_STREAM];

	bool is_joint;
	int joint_mask;
	int joint_main_chn; //main channel, all stream will write to this channel

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H265
	h265_t h265_inst;
	struct h265_param h265_info;
#endif

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H264
	h264_t h264_inst;
	struct h264_param h264_info;
#endif

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_JPEG
	jpeg_t jpeg_inst;
	struct jpeg_param jpeg_info;
#endif
};

struct vpu_module {
	int is_work_thread_on;
	int is_module_open;
	int is_module_start;
	spinlock_t module_lock;
	struct task_struct *file_read_task;
	struct task_struct *vpu_work_task;
	struct semaphore chain_sem;
	spinlock_t chain_lock;
	struct list_head free_list;
	struct list_head busy_list;
	struct vpu_inst vpu_chn[NUM_MAX_ENCODER_CHN];
	struct vpu_work_handle work_list[NUM_MAX_CHAIN];
};

static struct vpu_module *g_module = NULL;
static struct miscdevice g_vpu_dev;
static struct file_operations g_vpu_fops;
static unsigned int g_bs_size = 0;
static unsigned int g_bs_phyaddr = 0;
static unsigned int g_bsfull_en = 1;

#ifdef VPU_FILE_INPUT_TEST
int file_read_thread_fun(void *data)
{
	struct vpu_module *module = (struct vpu_module *)data;
	struct vpu_inst *vpu_chn = module->vpu_chn;
	int i = 0,j = 0;
	VPUEncoderAttr chnAttr;
	FrameInfo frame;
	int width = 1920;
	int height = 1080;
	int num_frames = 10;
	int vpubs = 500*1024;
	int stream_out_len = 1*1024*1024;

	VPU_Encoder_SetVpuBsSize(VPU_CHN0_MEMORY_START, vpubs);
	VPU_Encoder_SetMultiSection(0);

	chnAttr.enType = PT_H264;
	chnAttr.picWidth = width;
	chnAttr.picHeight = height;
	chnAttr.fps = 15;
	chnAttr.Iqp = 30;
	chnAttr.Pqp = 30;
	chnAttr.bEnableIvdc = 0;
	chnAttr.encPhyaddr = VPU_CHN0_MEMORY_START + vpubs;
	chnAttr.strLen = stream_out_len;

	VPU_Encoder_CreateChn(0, &chnAttr);

	for (i = 0; i < NUM_MAX_ENCODER_CHN; i ++) {
		if (vpu_chn[i].index >= 0) {
			if (i == 0) {
				for (j = 0; j < num_frames; j ++) {
					frame.index = j;
					frame.width = width;
					frame.height = height;
					frame.phyAddr = VPU_CHN0_FILE_INPUT_START + j * width * height * 3 / 2;
					frame.virAddr = (unsigned int)vpu_wrap_phy2vir(frame.phyAddr);
					frame.sensor_id = 0;
					frame.isp_priv = NULL;
					VPU_Encoder_SendFrame(0, &frame);
					VPU_DEBUG("kernEnc send one frame success\n");
				}
			}
		}
	}
	return 0;
}
#endif

int joint_check_update_list(struct vpu_inst *curr_chn, struct list_head *curr_list)
{
	int i = 0;
	bool all_exist = true;
	bool chn_found = false;
	int target_chns[NUM_MAX_ENCODER_CHN];
	int target_count = 0;
	struct vpu_work_handle *handles_to_remove[NUM_MAX_ENCODER_CHN];
	int remove_count = 0;
	struct vpu_work_handle *handle;
	struct vpu_module *module = g_module;

	//Find the corresponding channels
	for (i = 0; i < NUM_MAX_ENCODER_CHN; i++) {
		if (i == curr_chn->index) {
			continue;
		}
		if (curr_chn->joint_mask & (0x1 << i)) {
			target_chns[target_count++] = i;
		}
	}

	if (target_count == 0) {
		VPU_ERROR("Joint mode is setting but joint_mask is err\n");
		list_add_tail(curr_list, &module->busy_list);
		return -1;
	}

	//Traverse the linked list to find the target node
	for (i = 0; i < target_count; i ++) {
		list_for_each_entry(handle, &module->busy_list, list) {
			if (handle->chn_id == target_chns[i]) {
				chn_found = true;
				handles_to_remove[remove_count++] = handle;
				break;
			}
		}

		if (!chn_found) {
			all_exist = false;
			break;
		}
	}

	//If all the channel nodes that need joint exist, encoding can be started
	if (all_exist) {
		for (i = 0; i < remove_count; i++) {
			list_del(&handles_to_remove[i]->list);
			list_add_tail(&handles_to_remove[i]->list, &module->free_list);
			down_interruptible(&module->chain_sem);
		}
	} else {
		list_add_tail(curr_list, &module->busy_list);
		return -1;
	}

	return 0;
}

int vpu_work_thread_fun(void *data)
{
	struct vpu_module *module = (struct vpu_module *)data;
	struct vpu_inst *vpu_chn = module->vpu_chn;
	struct vpu_work_handle *handle;
	struct list_head *list = NULL;
	FrameInfo *frame = NULL;
	FrameInfo frame_array[NUM_MAX_ENCODER_CHN];
	struct vpu_inst *chn = NULL;
	int isp_bufsize = 0;
	char *isp_buf_addr = NULL;
	//unsigned int ref_y_tmp = 0;
	//unsigned int ref_c_tmp = 0;
	int ret = 0;
	int i = 0;
	bool is_chn_exist = 0;
	unsigned long flags;
	isp_bufsize = get_isp_bufsize();
	isp_buf_addr = vpu_wrap_malloc(isp_bufsize * NUM_MAX_ENCODER_CHN);
	for (i = 0; i < NUM_MAX_ENCODER_CHN; i++) {
		frame_array[i].isp_priv = isp_buf_addr + i * isp_bufsize;
	}

	module->is_work_thread_on = true;
	while (!kthread_should_stop()) {
		if (down_interruptible(&module->chain_sem) == -EINTR) {
			VPU_ERROR("down() recieve exit signal\n");
			return -1;
		}
		spin_lock_irqsave(&module->chain_lock, flags);
		if (list_empty(&module->busy_list)) {
			spin_unlock_irqrestore(&module->chain_lock, flags);
			break;
		}
		list = module->busy_list.next;
		list_del(list);
		handle = list_entry(list, struct vpu_work_handle, list);
		spin_unlock_irqrestore(&module->chain_lock, flags);

		frame = &handle->frame;
		chn = vpu_chn + handle->chn_id;

		if (!chn->is_start && chn->is_joint) {
			spin_lock_irqsave(&module->chain_lock, flags);
			list_add_tail(list, &module->free_list);
			spin_unlock_irqrestore(&module->chain_lock, flags);
			continue;
		}

		if (chn->is_joint) {
			spin_lock_irqsave(&module->chain_lock, flags);
			if (joint_check_update_list(chn, list) < 0) {
				up(&module->chain_sem);
				spin_unlock_irqrestore(&module->chain_lock, flags);
				continue;
			}
			spin_unlock_irqrestore(&module->chain_lock, flags);
		}
#ifndef VPU_FILE_INPUT_TEST
		if (chn->is_joint) {
			//Dq all joint channels
			for (i = 0; i < NUM_MAX_ENCODER_CHN; i++) {
				if (chn->joint_mask & (0x1 << i)) {
					enc_dqbuf(i, &frame_array[i]);
				}
			}
			frame = &frame_array[chn->joint_main_chn];
		} else {
			enc_dqbuf(chn->index, frame);
		}
#endif
		if (chn->is_joint) {
			//change to main channel
			chn = vpu_chn + chn->joint_main_chn;
		}
		chn->io.width = chn->width;
		chn->io.height = chn->height;
		chn->io.y_stride = chn->width;
		chn->io.c_stride = chn->width;
		chn->io.gop = chn->gop;
		chn->io.is_ivdc = chn->is_ivdc;
		chn->io.sensor_id = frame->sensor_id;
		if (chn->is_req_idr) {
			chn->io.frame_num = 0;
			chn->is_req_idr = 0;
		}
		chn->io.frm_num_gop = chn->io.frame_num % chn->io.gop;
		chn->io.bsfull_en = g_bsfull_en;
		chn->io.bsfull_size = C_ALIGN_DOWN(g_bs_size - (C_ALIGN(g_bs_phyaddr, 256) - g_bs_phyaddr), 2) / 2 / 1024;

		if (chn->en_type == PT_H265) {
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H265
			chn->io.frame_type = chn->io.frm_num_gop ? H265_SLICE_TYPE_P : H265_SLICE_TYPE_IDR;
			if (chn->io.frame_type == H265_SLICE_TYPE_IDR) {
				chn->io.qp = chn->i_qp;
			} else {
				chn->io.qp = chn->p_qp;
			}
			//bitstream
			chn->io.vpu_stream_buffer = chn->stream_buffer;
			chn->io.vpu_stream_buffer_len = chn->buf_size;
			chn->io.bs_phyaddr = chn->bs_phyaddr;
			chn->io.bs_viraddr = chn->bs_viraddr;
			//raw
			chn->io.raw_y_phyaddr = frame->phyAddr;
#ifndef VPU_FILE_INPUT_TEST
			chn->io.raw_c_phyaddr = frame->phyAddr + C_ALIGN(frame->width, 16) * C_ALIGN(frame->height,16);
#else
			chn->io.raw_c_phyaddr = frame->phyAddr + C_ALIGN(frame->width, 16) * frame->height;
#endif
#if 0
			if (chn->io.frame_type == H265_SLICE_TYPE_IDR) {
				//reference
				chn->io.ref_y_phyaddr = 0;
				chn->io.ref_c_phyaddr = 0;
				//decoder
				chn->io.dec_y_phyaddr = chn->dec_y_phyaddr;
				chn->io.dec_c_phyaddr = chn->dec_c_phyaddr;
			} else {
				if (chn->io.ref_y_phyaddr == 0) {
					chn->io.ref_y_phyaddr = chn->io.dec_y_phyaddr;
					chn->io.ref_c_phyaddr = chn->io.dec_c_phyaddr;
					chn->io.dec_y_phyaddr = chn->ref_y_phyaddr;
					chn->io.dec_c_phyaddr = chn->ref_c_phyaddr;
				} else {
					ref_y_tmp = chn->io.ref_y_phyaddr;
					ref_c_tmp = chn->io.ref_c_phyaddr;
					chn->io.ref_y_phyaddr = chn->io.dec_y_phyaddr;
					chn->io.ref_c_phyaddr = chn->io.dec_c_phyaddr;
					chn->io.dec_y_phyaddr = ref_y_tmp;
					chn->io.dec_c_phyaddr = ref_c_tmp;
				}
			}
#else
			chn->io.rds_y_phyaddr = chn->rds_y_phyaddr;
			chn->io.rds_c_phyaddr = chn->rds_c_phyaddr;
#endif
			//vdma
			chn->io.des_phyaddr = chn->des_phyaddr;
			chn->io.des_viraddr = chn->des_viraddr;
			//vpu
			chn->io.vpu_phyaddr = chn->vpu_phyaddr;
			//ktime_t start, end;
			//start = ktime_get();
			ret = h265_encode(&chn->h265_inst, &chn->io);
			//end = ktime_get();
			//printk("h265_encode cost %lld ms\n", (long long int)ktime_to_ms(end) - (long long int)ktime_to_ms(start));
			if (ret < 0) {
				VPU_ERROR("h265_encode failed\n");
				goto err_handle;
			}
#else
			VPU_ERROR("h265 code not compiled\n");
			ret = VPU_NOTREADY;
			goto err_handle;
#endif
		} else if (chn->en_type == PT_H264) {
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H264
			chn->io.frame_type = chn->io.frm_num_gop ? H264_SLICE_TYPE_P : H264_SLICE_TYPE_IDR;
			if (chn->io.frame_type == H264_SLICE_TYPE_IDR) {
				chn->io.qp = chn->i_qp;
			} else {
				chn->io.qp = chn->p_qp;
			}
			//bitstream
			chn->io.vpu_stream_buffer = chn->stream_buffer;
			chn->io.vpu_stream_buffer_len = chn->buf_size;
			chn->io.bs_phyaddr = chn->bs_phyaddr;
			chn->io.bs_viraddr = chn->bs_viraddr;
			//raw
			chn->io.raw_y_phyaddr = frame->phyAddr;
#ifndef VPU_FILE_INPUT_TEST
			chn->io.raw_c_phyaddr = frame->phyAddr + C_ALIGN(frame->width, 16) * C_ALIGN(frame->height,16);
#else
			chn->io.raw_c_phyaddr = frame->phyAddr + C_ALIGN(frame->width, 16) * frame->height;
#endif
#if 0
			if (chn->io.frame_type == H264_SLICE_TYPE_IDR) {
				//reference
				chn->io.ref_y_phyaddr = 0;
				chn->io.ref_c_phyaddr = 0;
				//decoder
				chn->io.dec_y_phyaddr = chn->dec_y_phyaddr;
				chn->io.dec_c_phyaddr = chn->dec_c_phyaddr;
			} else {
				chn->io.ref_y_phyaddr = chn->io.dec_y_phyaddr;
				chn->io.ref_c_phyaddr = chn->io.dec_c_phyaddr;
				if (!chn->io.ref_y_phyaddr && !chn->io.ref_c_phyaddr) {
					chn->io.dec_y_phyaddr = chn->io.ref_y_phyaddr;
					chn->io.dec_c_phyaddr = chn->io.ref_c_phyaddr;
				} else {
					chn->io.dec_y_phyaddr = chn->ref_y_phyaddr;
					chn->io.dec_c_phyaddr = chn->ref_c_phyaddr;
				}
			}
#else
			chn->io.rds_y_phyaddr = chn->rds_y_phyaddr;
			chn->io.rds_c_phyaddr = chn->rds_c_phyaddr;
#endif
			//vdma
			chn->io.des_phyaddr = chn->des_phyaddr;
			chn->io.des_viraddr = chn->des_viraddr;
			//vpu
			chn->io.vpu_phyaddr = chn->vpu_phyaddr;
			ret = h264_encode(&chn->h264_inst, &chn->io);
			if (ret < 0) {
				VPU_ERROR("h264_encode failed\n");
				goto err_handle;
			}
#else
			VPU_ERROR("h264 code not compiled\n");
			ret = VPU_NOTREADY;
			goto err_handle;
#endif
		} else { //PT_JPEG
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_JPEG
			chn->io.qp = chn->i_qp;
			//bitstream
			chn->io.vpu_stream_buffer = chn->stream_buffer;
			chn->io.vpu_stream_buffer_len = chn->buf_size;
			chn->io.bs_phyaddr = chn->bs_phyaddr;
			chn->io.bs_viraddr = chn->bs_viraddr;
			//raw
			chn->io.raw_y_phyaddr = frame->phyAddr;
#ifndef VPU_FILE_INPUT_TEST
			chn->io.raw_c_phyaddr = frame->phyAddr + C_ALIGN(frame->width, 16) * C_ALIGN(frame->height,16);
#else
			chn->io.raw_c_phyaddr = frame->phyAddr + C_ALIGN(frame->width, 16) * frame->height;
#endif
			ret = jpeg_encode(&chn->jpeg_inst, &chn->io);
			if (ret < 0) {
				VPU_ERROR("jpeg_encode failed\n");
				goto err_handle;
			}
#else
			VPU_ERROR("jpeg code not compiled\n");
			ret = VPU_NOTREADY;
			goto err_handle;
#endif
		}

		i = chn->io.tot_frame_num;
		if (i >= NUM_MAX_STREAM) {
			VPU_ERROR("vpu chn only have %d stream buffers\n", NUM_MAX_STREAM);
			ret = VPU_BUFFULL;
			goto err_handle;
		}
		chn->str_info[i].str_offset = chn->io.vpu_stream_buffer_occupancy;
		chn->str_info[i].str_len = chn->io.vpu_curr_stream_len;
		chn->str_info[i].timestamp = frame->timestamp;
		if (chn->io.frm_num_gop == 0) {
			chn->str_info[i].type = FSTYPE_IDR;
		} else {
			chn->str_info[i].type = FSTYPE_SBASE;
		}
		chn->io.frame_num ++;
		chn->io.tot_frame_num ++;
		chn->io.vpu_stream_buffer_occupancy += chn->io.vpu_curr_stream_len;

#ifndef VPU_FILE_INPUT_TEST
		if (!chn->is_ivdc) {
			if (chn->is_joint) {
				for (i = 0; i < NUM_MAX_ENCODER_CHN; i++) {
					if (chn->joint_mask & (0x1 << i)) {
						enc_qbuf(i, &frame_array[i]);
					}
				}
			} else {
				enc_qbuf(chn->index, frame);
			}
		}
#endif
		spin_lock_irqsave(&module->chain_lock, flags);
		list_add_tail(list, &module->free_list);
		spin_unlock_irqrestore(&module->chain_lock, flags);

		continue;
err_handle:
		if (ret == VPU_NOTREADY) {
			chn->is_start = 0;
		}else if (ret == VPU_BUFFULL) {
			for(i = 0; i < NUM_MAX_ENCODER_CHN; i++) {
				g_module->vpu_chn[i].is_start  = 0;
			}
		}
		chn->is_req_idr = 1;
#ifndef VPU_FILE_INPUT_TEST
		if (!chn->is_ivdc) {
			if (chn->is_joint) {
				for (i = 0; i < NUM_MAX_ENCODER_CHN; i++) {
					if (chn->joint_mask & (0x1 << i)) {
						enc_qbuf(i, &frame_array[i]);
					}
				}
			} else {
				enc_qbuf(chn->index, frame);
			}
		}
#endif
		spin_lock_irqsave(&module->chain_lock, flags);
		list_add_tail(list, &module->free_list);
		spin_unlock_irqrestore(&module->chain_lock, flags);
	}
	vpu_wrap_free(isp_buf_addr);
	module->is_work_thread_on = false;
	return 0;
}

static long vpu_stop(void)
{
	long ret = 0;
	int i = 0;
	int retry = 250000; //5s
	struct vpu_inst *vpu_chn = NULL;
	struct list_head *list = NULL;
	unsigned long flags;

	if (!g_module) {
		VPU_ERROR("vpu has been released\n");
		return 0;
	}

	vpu_chn = g_module->vpu_chn;
	spin_lock(&g_module->module_lock);
	if (!g_module->is_module_start) {
		spin_unlock(&g_module->module_lock);
		VPU_ERROR("vpu has been stopped\n");
		return 0;
	} else {
		g_module->is_module_start = 0;
		spin_unlock(&g_module->module_lock);
		for (i = 0; i < NUM_MAX_ENCODER_CHN; i ++) {
			if (vpu_chn[i].index >= 0) {
				vpu_chn[i].is_start = 0;
			}
		}

		while (retry --) {
			spin_lock_irqsave(&g_module->chain_lock, flags);
			i = 0;
			list_for_each(list, &g_module->free_list) {
				i ++;
			}
			if (i == NUM_MAX_CHAIN) {
				spin_unlock_irqrestore(&g_module->chain_lock, flags);
				break;
			} else {
				spin_unlock_irqrestore(&g_module->chain_lock, flags);
				vpu_wrap_usleep(20);
			}
		}
		disable_irq_nosync(hera_irq_for_kernEnc);
	}

	return ret;
}

static long vpu_channel_get_stream(long usr_arg)
{
	long ret = 0;
	struct kern_enc_node enc_node;
	struct vpu_inst *chn = NULL;

	if (!g_module) {
		VPU_ERROR("vpu has been released\n");
		return -1;
	}

	if (copy_from_user(&enc_node, (void *)usr_arg, sizeof(struct kern_enc_node))) {
		ret = -EINVAL;
		VPU_ERROR("copy_from_user failed\n");
		goto err_copy_from_user;
	}

	chn = &g_module->vpu_chn[enc_node.chn_id];
	if (chn->index < 0) {
		VPU_ERROR("vpu chn%d not enable\n", enc_node.chn_id);
		return -1;
	}

	enc_node.type = chn->en_type;
	enc_node.output_len = chn->io.vpu_stream_buffer_occupancy;
	enc_node.output_cnt = chn->io.tot_frame_num;
	enc_node.output_addr = vpu_wrap_vir2phy(chn->stream_buffer);
	memcpy(enc_node.str_info, chn->str_info, sizeof(struct kern_enc_str_info) * enc_node.output_cnt);

	if (copy_to_user((void *)usr_arg, &enc_node, sizeof(struct kern_enc_node))) {
		VPU_ERROR("copy_to_user failed\n");
		ret = -EINVAL;
		goto err_copy_to_user;
	}

err_copy_to_user:
err_copy_from_user:
	return ret;
}

static long vpu_release(long usr_arg)
{
	long ret = 0;
	int i = 0;
	struct kern_enc_node enc_node;
	struct vpu_inst *vpu_chn = NULL;

	if (!g_module) {
		VPU_ERROR("vpu has been released\n");
		return -1;
	}

	vpu_chn = g_module->vpu_chn;
	if (copy_from_user(&enc_node, (void *)usr_arg, sizeof(struct kern_enc_node))) {
		ret = -EINVAL;
		VPU_ERROR("copy_from_user failed\n");
		goto err_copy_from_user;
	}

	if (vpu_chn[enc_node.chn_id].index < 0) {
		VPU_ERROR("vpu chn%d not enable\n", enc_node.chn_id);
		return -1;
	}

	VPU_Encoder_DestroyChn(enc_node.chn_id);
	spin_lock(&g_module->module_lock);
	for (i = 0; i < NUM_MAX_ENCODER_CHN; i ++) {
		if (vpu_chn[i].index >= 0) {
			break;
		}
	}
	if (g_module->is_module_open && i == NUM_MAX_ENCODER_CHN) {
		g_module->is_module_open = 0;
		//if vpu_stop is not called, we should disable irq here.
		if (g_module->is_module_start) {
			g_module->is_module_start = 0;
			spin_unlock(&g_module->module_lock);
			disable_irq_nosync(hera_irq_for_kernEnc);
		} else {
			spin_unlock(&g_module->module_lock);
		}
		VPU_Kern_Exit();
#ifndef VPU_FILE_INPUT_TEST
		if( gosd_enable )
			_jz_osd_exit();
#endif
	} else {
		spin_unlock(&g_module->module_lock);
	}

err_copy_from_user:
	return ret;
}

static long vpu_channel_get_status(long usr_arg)
{
	long ret = 0;
	struct kern_enc_node enc_node;
	struct vpu_inst *chn = NULL;

	if (!g_module) {
		enc_node.chn_status = 0;
		if (copy_to_user((void *)usr_arg, &enc_node, sizeof(struct kern_enc_node))) {
			VPU_ERROR("copy_to_user failed\n");
			ret = -EINVAL;
			goto err_copy_to_user;
		}
		return ret;
	}

	if (copy_from_user(&enc_node, (void *)usr_arg, sizeof(struct kern_enc_node))) {
		ret = -EINVAL;
		VPU_ERROR("copy_from_user failed\n");
		goto err_copy_from_user;
	}

	chn = &g_module->vpu_chn[enc_node.chn_id];
	enc_node.chn_status = chn->index < 0 ? 0 : 1;

	if (copy_to_user((void *)usr_arg, &enc_node, sizeof(struct kern_enc_node))) {
		VPU_ERROR("copy_to_user failed\n");
		ret = -EINVAL;
		goto err_copy_to_user;
	}

err_copy_to_user:
err_copy_from_user:
	return ret;
}


static long fops_vpu_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = -1;

	switch (cmd) {
		case IOCTL_KERN_ENC_STOP:
			ret = vpu_stop();
			break;
		case IOCTL_KERN_ENC_GET_STREAM:
			ret = vpu_channel_get_stream(arg);
			break;
		case IOCTL_KERN_ENC_RELEASE:
			ret = vpu_release(arg);
			break;
		case IOCTL_KERN_ENC_GET_STATUS:
			ret = vpu_channel_get_status(arg);
			break;
		default:
			ret = -1;
	}

	return ret;
}

static int fops_vpu_release(struct inode *inode, struct file *file)
{
	//VPU_Kern_Exit();
	return 0;
}

int VPU_Kern_Init(void)
{
	int i = 0;
	int ret = 0;
	int isp_bufsize = 0;
	void *isp_priv = NULL;

	if (!g_module) {
		g_module = vpu_wrap_malloc(sizeof(struct vpu_module));
		if (!g_module) {
			VPU_ERROR("g_module alloc failed!\n");
			goto err_vpu_chn_alloc;
		}
		memset((void *)g_module, 0, sizeof(struct vpu_module));
		for (i = 0; i < NUM_MAX_ENCODER_CHN; i++) {
			g_module->vpu_chn[i].index = -1;
		}

		spin_lock_init(&g_module->module_lock);
		spin_lock_init(&g_module->chain_lock);
		INIT_LIST_HEAD(&g_module->free_list);
#ifndef VPU_FILE_INPUT_TEST
		isp_bufsize = get_isp_bufsize();
		isp_priv = vpu_wrap_malloc(isp_bufsize * NUM_MAX_CHAIN);
		for (i = 0; i < NUM_MAX_CHAIN; i ++) {
			list_add_tail(&g_module->work_list[i].list, &g_module->free_list);
			g_module->work_list[i].frame.isp_priv = (char *)isp_priv + isp_bufsize * i;
			if (!g_module->work_list[i].frame.isp_priv) {
				VPU_ERROR("isp_priv alloc failed!\n");
				goto err_isp_priv_alloc;
			}
		}
#else
		for (i = 0; i < NUM_MAX_CHAIN; i ++) {
			list_add_tail(&g_module->work_list[i].list, &g_module->free_list);
		}
#endif
		INIT_LIST_HEAD(&g_module->busy_list);
		sema_init(&g_module->chain_sem, 0);

		g_vpu_fops.owner = THIS_MODULE;
		g_vpu_fops.unlocked_ioctl = fops_vpu_ioctl;
		g_vpu_fops.release = fops_vpu_release;
#ifdef CONFIG_ZERATUL
		g_vpu_dev.minor = 57;
#else
		g_vpu_dev.minor = MISC_DYNAMIC_MINOR;
#endif
		g_vpu_dev.name = DEVICE_NAME;
		g_vpu_dev.fops = &g_vpu_fops;
		ret = misc_register(&g_vpu_dev);
		if (ret < 0) {
			VPU_ERROR("kern enc device register failed! ret = %d\n", ret);
			goto err_kern_enc_dev;;
		}

		enable_irq(hera_irq_for_kernEnc);

#ifndef CONFIG_FPGA_TEST
		//set vpu clk div
		vpu_wrap_write_reg(REG_CPM_VPU_DIV, (vpu_wrap_read_reg(REG_CPM_VPU_DIV) & (~(0x1 << 27))) | (0x1 << 29));
		while (vpu_wrap_read_reg(REG_CPM_VPU_DIV) & (0x1 << 28));


#ifdef CONFIG_SOC_PRJ007
		/* bit-31:30: VPUCS, 00:VPU clock source is SCLK_A, 01: VPU clock source is MPLL, 10/11: VPU clock source is VPLL
		   bit-3:0: VPUCDR, Divider for VPU Frequency.division ratio = VPUCDR + 1 */
		vpu_wrap_write_reg(REG_CPM_VPU_DIV, (vpu_wrap_read_reg(REG_CPM_VPU_DIV) & 0x3ffffff0) | (0x2 << 30) | (0x1 << 0));
#elif defined(CONFIG_SOC_PRJ008)
		/* bit31:30: VPUCS, 00:VPU clock source is SCLK_A, 01: VPU clock source is MPLL
		   bit-3:0: VPUCDR, Divider for VPU Frequency.division ratio = VPUCDR + 1 */
		vpu_wrap_write_reg(REG_CPM_VPU_DIV, (vpu_wrap_read_reg(REG_CPM_VPU_DIV) & 0x3ffffff0) | (0x0 << 30) | (0x2 << 0));
#endif

		vpu_wrap_write_reg(REG_CPM_VPU_DIV, vpu_wrap_read_reg(REG_CPM_VPU_DIV) & ~(0x1 << 29));
		while (vpu_wrap_read_reg(REG_CPM_VPU_DIV) & (0x1 << 28));

		//set ispa clk div
		vpu_wrap_write_reg(REG_CPM_ISPA_DIV, (vpu_wrap_read_reg(REG_CPM_ISPA_DIV) & ~(0x1 << 27)) | (0x1 << 29));
		while (vpu_wrap_read_reg(REG_CPM_ISPA_DIV) & (0x1 << 28));

#ifdef CONFIG_SOC_PRJ007
		/* bit-31:30: ISPACS, 00:ISPA clock source is SCLK_A, 01: IPSA clock source is MPLL, 10/11: ISPA clock source is VPLL
		   bit-3:0: ISPACDR, Divider for ISPA Frequency.division ratio = ISPACDR + 1 */
		vpu_wrap_write_reg(REG_CPM_ISPA_DIV, (vpu_wrap_read_reg(REG_CPM_ISPA_DIV) & 0x3ffffff0) | (0x2 << 30) | (0x1 << 0));
#elif defined(CONFIG_SOC_PRJ008)
		/* bit-31:30: ISPACS, 00:ISPA clock source is SCLK_A, 01: IPSA clock source is MPLL
		   bit-3:0: ISPACDR, Divider for ISPA Frequency.division ratio = ISPACDR + 1 */
		vpu_wrap_write_reg(REG_CPM_ISPA_DIV, (vpu_wrap_read_reg(REG_CPM_ISPA_DIV) & 0x3ffffff0) | (0x0 << 30) | (0x2 << 0));
#endif
		vpu_wrap_write_reg(REG_CPM_ISPA_DIV, vpu_wrap_read_reg(REG_CPM_ISPA_DIV) & ~(0x1 << 29));
		while (vpu_wrap_read_reg(REG_CPM_ISPA_DIV) & (0x1 << 28));

		//open clk gates
		vpu_wrap_write_reg(REG_CPM_CLKGATE0, vpu_wrap_read_reg(REG_CPM_CLKGATE0) & (~(1 << BIT_CPM_VPU_CLOCKGATE)));
		vpu_wrap_write_reg(REG_CPM_CLKGATE1, vpu_wrap_read_reg(REG_CPM_CLKGATE1) & (~(1 << BIT_CPM_IVDC_CLOCKGATE)));
		vpu_wrap_write_reg(REG_CPM_CLKGATE1, vpu_wrap_read_reg(REG_CPM_CLKGATE1) & (~(1 << BIT_CPM_AHB1_CLOCKGATE)));

#if 0
		//debug clk
		VPU_INFO("0xb0000020=%x\n", *(volatile unsigned int*)(0xb0000020));//vpu clk gate
		VPU_INFO("0xb0000028=%x\n", *(volatile unsigned int*)(0xb0000028));//ispa clk gate
		VPU_INFO("0xb0000030=%x\n", *(volatile unsigned int*)(0xb0000030));//vpu div
		VPU_INFO("0xb0000094=%x\n", *(volatile unsigned int*)(0xb0000094));//ispa div
		VPU_INFO("0xb00000e0=%x\n", *(volatile unsigned int*)(0xb00000e0));//ispa div
#endif
#endif

		g_module->vpu_work_task = kthread_run(vpu_work_thread_fun, (void *)g_module, "kern_enc_vpu_work");
		if (IS_ERR(g_module->vpu_work_task)) {
			VPU_ERROR("vpu_work_task create failed\n");
			goto err_vpu_work_task;
		}
#ifdef VPU_FILE_INPUT_TEST
		g_module->file_read_task = kthread_run(file_read_thread_fun, (void *)g_module, "kern_enc_file_read");
		if (IS_ERR(g_module->file_read_task)) {
			VPU_ERROR("file_read_task create failed\n");
			goto err_file_read_task;
		}
#endif
		g_module->is_module_open = 1;
		g_module->is_module_start = 1;

		VPU_INFO("KernEnc Init success!\n");
	}

	return 0;
#ifdef VPU_FILE_INPUT_TEST
	kthread_stop(g_module->vpu_work_task);
err_file_read_task:
#endif
err_vpu_work_task:
	misc_deregister(&g_vpu_dev);
err_kern_enc_dev:
#ifndef VPU_FILE_INPUT_TEST
	if (g_module->work_list[0].frame.isp_priv) {
		vpu_wrap_free(g_module->work_list[0].frame.isp_priv);
	}
err_isp_priv_alloc:
#endif
	if (g_module)
		vpu_wrap_free(g_module);
err_vpu_chn_alloc:
	return -1;
}

int VPU_Kern_Exit(void)
{
	int i = 0;
	unsigned long flags = 0;
	struct vpu_module *tmp = g_module;
	//misc_deregister(&g_vpu_dev);
	up(&g_module->chain_sem);
	while (g_module->is_work_thread_on)
		vpu_wrap_usleep(20);
#ifndef VPU_FILE_INPUT_TEST
	if (g_module->work_list[0].frame.isp_priv) {
		vpu_wrap_free(g_module->work_list[0].frame.isp_priv);
	}
#endif
	spin_lock_irqsave(&tmp->module_lock, flags);
	g_module = NULL;
	spin_unlock_irqrestore(&tmp->module_lock, flags);
	vpu_wrap_free(tmp);
	return 0;
}

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H265
int vpu_h265_buf_init(struct vpu_inst *chn)
{
	unsigned int y_rdsize = 0;
	unsigned int c_rdsize = 0;
	//unsigned int vpu_size = 0;
	unsigned int w_align = 0;
	unsigned int h_align = 0;

	chn->stream_buffer = (unsigned int *)vpu_wrap_malloc_linear(chn->buf_size);
	if (!chn->stream_buffer) {
		VPU_ERROR("h265 stream mem alloc failed!\n");
		goto err_stm_mem_alloc;
	}

	w_align = C_ALIGN(chn->width, 64);
	h_align = C_ALIGN(chn->height, 64);
#ifdef CONFIG_SOC_PRJ007
	y_rdsize = C_ALIGN(w_align * (h_align + (2 + 1) * 64), 256);
	c_rdsize = C_ALIGN(w_align * (h_align / 2 + (2 + 1) * 32), 256);
#elif defined(CONFIG_SOC_PRJ008)
	//y_rdsize = C_ALIGN(w_align * (C_ALIGN(chn->height, 16) + 64), 256) + 4*64*64;
	//c_rdsize = C_ALIGN(w_align * (C_ALIGN(chn->height, 16) / 2 + 32), 256) + 4*32*64;
	y_rdsize = C_ALIGN(w_align * (h_align + (1 + 1) * 64), 256);
	c_rdsize = C_ALIGN(w_align * (h_align / 2 + (1 + 1) * 32), 256);
#endif

	//vpu_size = C_ALIGN(VPU_VDMA_SIZE + 256 + (C_ALIGN(chn->width, 32) / 32) * (C_ALIGN(chn->height, 32) / 32) * (77 * 4) + 20 + 2*1024 + 14*256, 256);

	chn->tot_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->tot_phyaddr);
	if (chn->tot_viraddr == NULL) {
		VPU_ERROR("h265 linear mem alloc failed!\n");
		goto err_265_mem_alloc;
	}
	//bitstream
	chn->bs_phyaddr = C_ALIGN(g_bs_phyaddr, 256);
	chn->bs_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->bs_phyaddr);
#if 0
	//ref
	chn->ref_y_phyaddr = C_ALIGN(chn->tot_phyaddr, 256);
	chn->ref_c_phyaddr = C_ALIGN(chn->ref_y_phyaddr + y_rdsize, 256);
	//dec
	chn->dec_y_phyaddr = C_ALIGN(chn->ref_y_phyaddr + y_rdsize * 3 / 2 + 256, 256);
	chn->dec_c_phyaddr = C_ALIGN(chn->dec_y_phyaddr + y_rdsize, 256);
#else
	chn->rds_y_phyaddr = C_ALIGN(chn->tot_phyaddr, 256);
	chn->rds_c_phyaddr = C_ALIGN(chn->rds_y_phyaddr + y_rdsize, 256);
#endif
	//vdma
	chn->des_phyaddr = C_ALIGN(chn->rds_y_phyaddr + y_rdsize + c_rdsize, 256);
	chn->des_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->des_phyaddr);
	//vpu
	chn->vpu_phyaddr = C_ALIGN(chn->des_phyaddr + VPU_VDMA_SIZE, 256);

	return 0;
err_265_mem_alloc:
	vpu_wrap_free(chn->stream_buffer);
err_stm_mem_alloc:
	return -1;
}
#endif

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H264
int vpu_h264_buf_init(struct vpu_inst *chn)
{
	unsigned int y_rdsize = 0;
	unsigned int c_rdsize = 0;
	//unsigned int vpu_size = 0;
	unsigned int w_align = 0;
	unsigned int h_align = 0;

	chn->stream_buffer = (unsigned int *)vpu_wrap_malloc_linear(chn->buf_size);
	if (!chn->stream_buffer) {
		VPU_ERROR("h264 stream mem alloc failed!\n");
		goto err_stm_mem_alloc;
	}

	w_align = C_ALIGN(chn->width, 64);
	h_align = C_ALIGN(chn->height, 16);
#ifdef CONFIG_SOC_PRJ007
	y_rdsize = C_ALIGN(w_align * (h_align + (2 + 1) * 64), 256);
	c_rdsize = C_ALIGN(w_align * (h_align / 2 + (2 + 1) * 32), 256);
#elif defined(CONFIG_SOC_PRJ008)
	//y_rdsize = C_ALIGN(w_align * (C_ALIGN(chn->height, 16) + 2*64), 256) + 4*64*64;
	//c_rdsize = C_ALIGN(w_align * (C_ALIGN(chn->height, 16) / 2 + 2*32), 256) + 4*32*64;
	y_rdsize = C_ALIGN(w_align * (C_ALIGN(chn->height, 16) + (1 + 1) * 64), 256);
	c_rdsize = C_ALIGN(w_align * (C_ALIGN(chn->height, 16) / 2 + (1 + 1) * 32), 256);
#endif
	//vpu_size = C_ALIGN(VPU_VDMA_SIZE + 256 + (C_ALIGN(chn->width, 16) / 16) * (C_ALIGN(chn->height, 16) / 16) * 70 + 73 + 2*1024 + 14 * 256, 256);

	chn->tot_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->tot_phyaddr);
	if (chn->tot_viraddr == NULL) {
		VPU_ERROR("h264 linear mem alloc failed!\n");
		goto err_264_mem_alloc;
	}
	//bitstream
	chn->bs_phyaddr = C_ALIGN(g_bs_phyaddr, 256);
	chn->bs_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->bs_phyaddr);
#if 0
	//ref
	chn->ref_y_phyaddr = C_ALIGN(chn->tot_phyaddr, 256);
	chn->ref_c_phyaddr = C_ALIGN(chn->ref_y_phyaddr + y_rdsize, 256);
	//dec
	chn->dec_y_phyaddr = C_ALIGN(chn->ref_y_phyaddr + y_rdsize * 3 / 2 + 256, 256);
	chn->dec_c_phyaddr = C_ALIGN(chn->dec_y_phyaddr + y_rdsize, 256);
#else
	chn->rds_y_phyaddr = C_ALIGN(chn->tot_phyaddr, 256);
	chn->rds_c_phyaddr = C_ALIGN(chn->rds_y_phyaddr + y_rdsize, 256);
#endif
	//vdma
	chn->des_phyaddr = C_ALIGN(chn->rds_y_phyaddr + y_rdsize + c_rdsize, 256);
	chn->des_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->des_phyaddr);
	//vpu
	chn->vpu_phyaddr = C_ALIGN(chn->des_phyaddr + VPU_VDMA_SIZE, 256);

	return 0;
err_264_mem_alloc:
	vpu_wrap_free(chn->stream_buffer);
err_stm_mem_alloc:
	return -1;
}
#endif

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_JPEG
int vpu_jpeg_buf_init(struct vpu_inst *chn)
{
	unsigned int bs_size = 0;
	unsigned int tot_size = 0;
	unsigned int w_align = 0;
	unsigned int h_align = 0;

	chn->stream_buffer = (unsigned int *)vpu_wrap_malloc_linear(chn->buf_size);
	if (!chn->stream_buffer) {
		VPU_ERROR("jpeg stream mem alloc failed!\n");
		goto err_stm_mem_alloc;
	}

	w_align = C_ALIGN(chn->width, 64);
	h_align = C_ALIGN(chn->height, 16);
	bs_size = w_align * h_align + 256;
	tot_size = bs_size;

#if 0
	chn->tot_viraddr = (unsigned int *)vpu_wrap_malloc_linear(tot_size);
	if (chn->tot_viraddr == NULL) {
		VPU_ERROR("h265 linear mem alloc failed!\n");
		goto err_265_mem_alloc;
	}
	chn->tot_phyaddr = (unsigned int)vpu_wrap_vir2phy(chn->tot_viraddr);
#else
	chn->tot_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->tot_phyaddr);
	if (chn->tot_viraddr == NULL) {
		VPU_ERROR("jpeg linear mem alloc failed!\n");
		goto err_jpeg_mem_alloc;
	}
#endif
	//bitstream
	chn->bs_phyaddr = chn->tot_phyaddr;
	chn->bs_viraddr = (unsigned int *)vpu_wrap_phy2vir(chn->bs_phyaddr);

	return 0;
err_jpeg_mem_alloc:
	vpu_wrap_free(chn->stream_buffer);
err_stm_mem_alloc:
	return -1;
}
#endif

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H265
int vpu_h265_buf_exit(struct vpu_inst *chn)
{
	vpu_wrap_free_linear(chn->stream_buffer, chn->buf_size);
	return 0;
}
#endif

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H264
int vpu_h264_buf_exit(struct vpu_inst *chn)
{
	vpu_wrap_free_linear(chn->stream_buffer, chn->buf_size);
	return 0;
}
#endif

#ifdef CONFIG_JZ_VPU_KERN_ENCODE_JPEG
int vpu_jpeg_buf_exit(struct vpu_inst *chn)
{
	vpu_wrap_free_linear(chn->stream_buffer, chn->buf_size);
	return 0;
}
#endif

int VPU_Encoder_CreateChn(int chnNum, const VPUEncoderAttr *chnAttr)
{
	int rd_num = 0, sup_en = 0;
	struct vpu_inst *chn = NULL;
	if (chnNum >= NUM_MAX_ENCODER_CHN || chnNum < 0) {
		VPU_ERROR("chnNum is out of range!\n");
		return -1;
	}

	if (!g_bs_size || !g_bs_phyaddr) {
		VPU_ERROR("Please set vpubs config firstly\n");
		return -1;
	}

	chn = &g_module->vpu_chn[chnNum];
	chn->en_type = chnAttr->enType;
	chn->width = chnAttr->picWidth;
	chn->height = chnAttr->picHeight;
	chn->buf_size = chnAttr->strLen;
	chn->fps = chnAttr->fps;
	chn->i_qp = chnAttr->Iqp;
	chn->p_qp = chnAttr->Pqp;
	chn->gop = 2 * chn->fps;
	chn->tot_phyaddr = chnAttr->encPhyaddr;
	chn->is_ivdc = chnAttr->bEnableIvdc;

	if (chn->is_ivdc && chn->is_joint) {
		VPU_ERROR("Ivdc channel don't support joint\n");
		return -1;
	}

	chn->index = chnNum;
	chn->is_start = 1;
	if (chn->en_type == PT_H265) {
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H265
		vpu_h265_buf_init(chn);
		chn->h265_info.width = chn->width;
		chn->h265_info.height = chn->height;
		chn->h265_info.y_stride = C_ALIGN(chn->h265_info.width, 16);
		chn->h265_info.c_stride = chn->h265_info.y_stride;
		chn->h265_info.i_qp = chn->i_qp;
		chn->h265_info.p_qp = chn->p_qp;
		chn->h265_info.gop = chn->gop;
		chn->h265_info.fps = chn->fps;
		chn->h265_info.fps_den = 1;
		h265_param_init(&chn->h265_inst, &chn->h265_info);
#else
		VPU_ERROR("h265 code not compiled\n");
		return -1;
#endif
	} else if (chn->en_type == PT_H264) {
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H264
		vpu_h264_buf_init(chn);
		chn->h264_info.width = chn->width;
		chn->h264_info.height = chn->height;
		chn->h264_info.y_stride = C_ALIGN(chn->h264_info.width, 16);
		chn->h264_info.c_stride = chn->h264_info.y_stride;
		chn->h264_info.i_qp = chn->i_qp;
		chn->h264_info.p_qp = chn->p_qp;
		chn->h264_info.gop = chn->gop;
		chn->h264_info.fps = chn->fps;
		chn->h264_info.fps_den = 1;
		h264_param_init(&chn->h264_inst, &chn->h264_info);
#else
		VPU_ERROR("h264 code not compiled\n");
		return -1;
#endif
	} else { //PT_JPEG
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_JPEG
		vpu_jpeg_buf_init(chn);
		chn->jpeg_info.width = chn->width;
		chn->jpeg_info.height = chn->height;
		chn->jpeg_info.y_stride = C_ALIGN(chn->jpeg_info.width, 16);
		chn->jpeg_info.c_stride = chn->jpeg_info.y_stride;
		chn->jpeg_info.qp = chn->i_qp;
		jpeg_param_init(&chn->jpeg_inst, &chn->jpeg_info);
#else
		VPU_ERROR("jpeg code not compiled\n");
		return -1;
#endif
	}

	if (chn->is_ivdc) {
		if(chn->index == 0){
			if (chn->en_type == PT_H265) {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 8);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb3100018) = C_ALIGN(chn->height, 8)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb31000c0) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else if (chn->en_type == PT_H264) {
				*(volatile int *)(0xb3100018) = C_ALIGN(chn->height, 16)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb31000c0) = 0;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 2);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb310001c) = C_ALIGN(chn->height, 2)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb31000c4) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x2<<2 | 0x2<<4;
			}
		}

		if(chn->index == 3){
			if (chn->en_type == PT_H265) {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 8);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb3100608) = C_ALIGN(chn->height, 8)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb3100624) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else if (chn->en_type == PT_H264) {
				*(volatile int *)(0xb3100608) = C_ALIGN(chn->height, 16)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb3100624) = 0;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 2);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb310060c) = C_ALIGN(chn->height, 2)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb3100628) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x2<<2 | 0x2<<4;
			}
		}
		if(chn->index == 6){
			if (chn->en_type == PT_H265) {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 8);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb3100648) = C_ALIGN(chn->height, 8)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb3100664) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else if (chn->en_type == PT_H264) {
				*(volatile int *)(0xb3100648) = C_ALIGN(chn->height, 16)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb3100664) = 0;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 2);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb310064c) = C_ALIGN(chn->height, 2)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb3100668) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x2<<2 | 0x2<<4;
			}
		}
		if(chn->index == 9){
			if (chn->en_type == PT_H265) {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 8);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb3100688) = C_ALIGN(chn->height, 8)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb31006a4) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else if (chn->en_type == PT_H264) {
				*(volatile int *)(0xb3100688) = C_ALIGN(chn->height, 16)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb31006a4) = 0;
				*(volatile int *)(0xb3100008) = 0x1<<2 | 0x2<<4;
			} else {
				rd_num = C_ALIGN(chn->height, 16) - C_ALIGN(chn->height, 2);
				if (rd_num == 0)
					sup_en = 0;
				else
					sup_en = 1;
				*(volatile int *)(0xb310068c) = C_ALIGN(chn->height, 2)<<16 | C_ALIGN(chn->width, 16);
				*(volatile int *)(0xb31006a8) = sup_en<<15 | (rd_num/2)<<8 | sup_en<<7 | rd_num;
				*(volatile int *)(0xb3100008) = 0x2<<2 | 0x2<<4;
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(VPU_Encoder_CreateChn);

int VPU_Encoder_DestroyChn(int chnNum)
{
	struct vpu_inst *chn = NULL;
	if (chnNum >= NUM_MAX_ENCODER_CHN || chnNum < 0) {
		VPU_ERROR("chnNum is out of range!\n");
		return -1;
	}

	chn = &g_module->vpu_chn[chnNum];
	if (chn->index < 0) {
		VPU_INFO("chn %d has not be created\n", chnNum);
		return 0;
	}

	if (chn->en_type == PT_H265) {
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H265
		h265_exit(&chn->h265_inst);
		vpu_h265_buf_exit(chn);
#else
		VPU_ERROR("h265 code not compiled\n");
		return -1;
#endif
	} else if (chn->en_type == PT_H264) {
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_H264
		h264_exit(&chn->h264_inst);
		vpu_h264_buf_exit(chn);
#else
		VPU_ERROR("h264 code not compiled\n");
		return -1;
#endif
	} else { //PT_JPEG
#ifdef CONFIG_JZ_VPU_KERN_ENCODE_JPEG
		jpeg_exit(&chn->jpeg_inst);
		vpu_jpeg_buf_exit(chn);
#else
		VPU_ERROR("jpeg code not compiled\n");
		return -1;
#endif
	}
	memset(chn, 0, sizeof(struct vpu_inst));
	chn->index = -1;

	return 0;
}

#ifdef VPU_FILE_INPUT_TEST
int VPU_Encoder_SendFrame(int chnNum, FrameInfo *frame)
#else
int VPU_Encoder_SendFrame(int chnNum)
#endif
{
	struct vpu_inst *vpu_chn = NULL;
	struct vpu_work_handle *handle;
	struct list_head *list = NULL;
	unsigned long flags = 0;

	if (!g_module) {
		return VPU_NOTREADY;
	}

	if (chnNum >= NUM_MAX_ENCODER_CHN || chnNum < 0) {
		VPU_ERROR("chnNum is out of range!\n");
		return VPU_ERRCHN;
	}

	vpu_chn = g_module->vpu_chn;
	if (vpu_chn[chnNum].index < 0) {
		VPU_ERROR("chn %d is not created!\n", chnNum);
		return VPU_ERRCHN;
	}

	if (vpu_chn[chnNum].is_start) {
		//Since this function is called in an interrupt context, all chain_locks use spin_lock_irqsave.
		spin_lock_irqsave(&g_module->chain_lock, flags);
		if (!vpu_chn[chnNum].is_start) {
			spin_unlock_irqrestore(&g_module->chain_lock, flags);
			return VPU_NOTREADY;
		}
		if (list_empty(&g_module->free_list)) {
			spin_unlock_irqrestore(&g_module->chain_lock, flags);
			VPU_ERROR("no free work list\n");
			return VPU_BUFFULL;
		}
		list = g_module->free_list.next;
		list_del(list);
		handle = list_entry(list, struct vpu_work_handle, list);

#ifdef VPU_FILE_INPUT_TEST
		handle->chn_id = chnNum;
		memcpy(&handle->frame, frame, sizeof(FrameInfo));
#else
		handle->chn_id = chnNum;
#endif
		list_add_tail(list, &g_module->busy_list);
		spin_unlock_irqrestore(&g_module->chain_lock, flags);
		up(&g_module->chain_sem);
	} else {
		//VPU_ERROR("vpu chn %d not start\n", chnNum);
		return VPU_NOTREADY;
	}
	return 0;
}
EXPORT_SYMBOL(VPU_Encoder_SendFrame);

int VPU_Encoder_GetChannelBufferSize(const VPUEncoderAttr *chnAttr)
{
	int tot_size = -1;
	unsigned int bs_size = 0;
	unsigned int y_rdsize = 0;
	unsigned int c_rdsize = 0;
	unsigned int vpu_size = 0;
	unsigned int w_align = 0;
	unsigned int h_align = 0;
	unsigned int mb_size = 0;
	unsigned int ttl_size = 0;

#ifdef CONFIG_SOC_PRJ007
	w_align = C_ALIGN(chnAttr->picWidth, 64);
	h_align = C_ALIGN(chnAttr->picHeight, 64);
	if (chnAttr->enType == PT_H265) {
		y_rdsize = C_ALIGN(w_align * (h_align + (2 + 1) * 64), 256);
		c_rdsize = C_ALIGN(w_align * (h_align / 2 + (2 + 1) * 32), 256);
		mb_size = C_ALIGN((C_ALIGN(chnAttr->picWidth, 32) / 32) * (C_ALIGN(chnAttr->picHeight, 32) / 32) * 4, 256);
		vpu_size = C_ALIGN(VPU_VDMA_SIZE + 256 + mb_size * 33 + 256, 256);

		tot_size = y_rdsize + c_rdsize + vpu_size;
		//Avoid the memory overflow because of the align of start addr.
		tot_size += 256;
	} else if (chnAttr->enType == PT_H264) {
		y_rdsize = C_ALIGN(w_align * (h_align + (2 + 1) * 64), 256);
		c_rdsize = C_ALIGN(w_align * (h_align / 2 + (2 + 1) * 32), 256);
		mb_size = C_ALIGN((C_ALIGN(chnAttr->picWidth, 16) / 16) * (C_ALIGN(chnAttr->picHeight, 16) / 16), 256);
		vpu_size = C_ALIGN(VPU_VDMA_SIZE + 256 + mb_size * 33 + 256, 256);

		tot_size = y_rdsize + c_rdsize + vpu_size;
		//Avoid the memory overflow because of the align of start addr.
		tot_size += 256;
	} else { //PT_JPEG
		bs_size = w_align * h_align + 256;
		tot_size = bs_size;
	}
#elif defined(CONFIG_SOC_PRJ008)
	w_align = C_ALIGN(chnAttr->picWidth, 64);
	h_align = C_ALIGN(chnAttr->picHeight, 64);
	if (chnAttr->enType == PT_H265) {
		//y_rdsize = C_ALIGN(w_align * (C_ALIGN(chnAttr->picHeight, 16) + 2*64), 256) + 4*64*64;
		//c_rdsize = C_ALIGN(w_align * (C_ALIGN(chnAttr->picHeight, 16) / 2 + 2*32), 256) + 4*32*64;
		y_rdsize = C_ALIGN(w_align * (h_align + (1 + 1) * 64), 256);
		c_rdsize = C_ALIGN(w_align * (h_align / 2 + (1 + 1) * 32), 256);
		mb_size = C_ALIGN((C_ALIGN(chnAttr->picWidth, 32) / 32) * (C_ALIGN(chnAttr->picHeight, 32) / 32) * 4, 256);
		ttl_size = C_ALIGN(mb_size*24, 256) + C_ALIGN(mb_size*4, 256) + C_ALIGN(mb_size/2, 256) + C_ALIGN(mb_size*4, 256) + C_ALIGN(mb_size*4, 256) + C_ALIGN(mb_size, 256);
		vpu_size = C_ALIGN(VPU_VDMA_SIZE + 256 + ttl_size + 256, 256);

		tot_size = y_rdsize + c_rdsize + vpu_size;
		//Avoid the memory overflow because of the align of start addr.
		tot_size += 256;
	} else if (chnAttr->enType == PT_H264) {
		//y_rdsize = C_ALIGN(w_align * (C_ALIGN(chnAttr->picHeight, 16) + 64), 256) + 4*64*64;
		//c_rdsize = C_ALIGN(w_align * (C_ALIGN(chnAttr->picHeight, 16) / 2 + 32), 256) + 4*32*64;
		y_rdsize = C_ALIGN(w_align * (C_ALIGN(chnAttr->picHeight, 16) + (1 + 1) * 64), 256);
		c_rdsize = C_ALIGN(w_align * (C_ALIGN(chnAttr->picHeight, 16) / 2 + (1 + 1) * 32), 256);
		mb_size = C_ALIGN((C_ALIGN(chnAttr->picWidth, 16) / 16) * (C_ALIGN(chnAttr->picHeight, 16) / 16), 256);
		ttl_size = C_ALIGN(mb_size*24, 256) + C_ALIGN(mb_size*4, 256) + C_ALIGN((mb_size+1)/2, 256) + C_ALIGN(mb_size*4, 256) + C_ALIGN(mb_size, 256);
		vpu_size = C_ALIGN(VPU_VDMA_SIZE + 256 + ttl_size + 256, 256);

		tot_size = y_rdsize + c_rdsize + vpu_size;
		//Avoid the memory overflow because of the align of start addr.
		tot_size += 256;
	} else { //PT_JPEG
		bs_size = w_align * h_align + 256;
		tot_size = bs_size;
	}
#endif

	return tot_size;
}
EXPORT_SYMBOL(VPU_Encoder_GetChannelBufferSize);

int VPU_Encoder_SetVpuBsSize(unsigned int bsPhyaddr, unsigned int bsSize)
{
	if (!bsPhyaddr || !bsSize) {
		VPU_ERROR("Wrong bsPhyaddr or bsSize\n");
		return -1;
	}

	g_bs_phyaddr = bsPhyaddr;
	g_bs_size = bsSize;

	return 0;
}
EXPORT_SYMBOL(VPU_Encoder_SetVpuBsSize);

int VPU_Encoder_SetMultiSection(int enable)
{
	if (!enable) {
		g_bsfull_en = 0;
		return 0;
	}

	return 0;
}
EXPORT_SYMBOL(VPU_Encoder_SetMultiSection);

/*
	channels: joint channel id array
	count: joint channel count
 */
int VPU_Encoder_SetJointAttr(const int *channels, int count)
{
	int i = 0;
	int new_mask = 0;
	struct vpu_inst *chn = NULL;

	if (channels == NULL || count < 2) {
		VPU_ERROR("Set joint attr needs at least 2 channels\n");
		return -1;
	}

	for (i = 0; i < count; i ++) {
		if (channels[i] < 0 || channels[i] >= NUM_MAX_ENCODER_CHN) {
			VPU_ERROR("Joint channel id err\n");
			return -1;
		}
	}

	for (i = 0; i < count; i ++) {
		new_mask |= (1 << channels[i]);
	}

	for (i = 0; i < count; i ++) {
		chn = &g_module->vpu_chn[channels[i]];
		chn->is_joint = true;
		chn->joint_mask = new_mask;
		chn->joint_main_chn = channels[0];
	}

	return 0;
}
EXPORT_SYMBOL(VPU_Encoder_SetJointAttr);

#if 0
static int __init kern_enc_init(void)
{
	VPU_Kern_Init();
}

static void __exit kern_enc_exit(void)
{
	VPU_Kern_Exit();
}
#endif
#else //CONFIG_JZ_VPU_KERN_ENCODE
#include "ingenic_vpu.h"
#include "vpu_common.h"

int VPU_Encoder_SendFrame(int chnNum)
{
	return VPU_NOTREADY;
}
EXPORT_SYMBOL(VPU_Encoder_SendFrame);
#endif //CONFIG_JZ_VPU_KERN_ENCODE
