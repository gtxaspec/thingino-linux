#ifndef _VPU_CORE_H_
#define _VPU_CORE_H_
#include <linux/types.h>

#define VPU_VDMA_SIZE 0x2000
struct vpu_io_info {
	unsigned short width;
	unsigned short height;
	unsigned short y_stride;
	unsigned short c_stride;
	unsigned short qp;
	unsigned short gop;
	unsigned short fps;
	unsigned short fps_den;
	bool is_ivdc;
	int sensor_id;
	int frame_num;
	int tot_frame_num;
	char frame_type;
	int frm_num_gop;
	unsigned int cur_frame_phyaddr;
	unsigned int *vpu_stream_buffer;
	unsigned int vpu_stream_buffer_len;
	unsigned int vpu_curr_stream_len;
	unsigned int vpu_stream_buffer_occupancy;
	unsigned int bs_phyaddr;
	unsigned int *bs_viraddr;
	unsigned int raw_y_phyaddr;
	unsigned int raw_c_phyaddr;
	unsigned int ref_y_phyaddr;
	unsigned int ref_c_phyaddr;
	unsigned int dec_y_phyaddr;
	unsigned int dec_c_phyaddr;
	unsigned int rds_y_phyaddr;
	unsigned int rds_c_phyaddr;
	unsigned int des_phyaddr;
	unsigned int *des_viraddr;
	unsigned int vpu_phyaddr;
	unsigned int bslen;
	//tmp value
	unsigned char *ref1_addr;
	unsigned char *ref_addr;
	unsigned char *cps_addr;
	unsigned int s_last_y_ofst_addr;
	unsigned int s_last_c_ofst_addr;
	int bsfull_en;
	unsigned int bsfull_size;
};

#endif
