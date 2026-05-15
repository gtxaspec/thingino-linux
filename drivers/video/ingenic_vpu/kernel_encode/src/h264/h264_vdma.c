#include "hera.h"
#include "h264_core.h"
#include <linux/kernel.h>

#define C_SPE_ADD_MB_NUM 3
static unsigned int lps_range[64] = {
	0xeeceaefc,  0xe1c3a5fc,  0xd6b99cfc,  0xcbb094f2,
	0xc1a78ce4,  0xb79e85da,  0xad967ece,  0xa48e78c4,
	0x9c8772ba,  0x94806cb0,  0x8c7966a6,  0x8573619e,
	0x7e6d5c96,  0x7867578e,  0x72625386,  0x6c5d4e80,
	0x66584a78,  0x61544672,  0x5c4f436c,  0x574b3f66,
	0x53473c62,  0x4e43395c,  0x4a403658,  0x463d3352,
	0x4339304e,  0x3f362e4a,  0x3c342b46,  0x39312942,
	0x362e273e,  0x332c253c,  0x30292338,  0x2e272136,
	0x2b251f32,  0x29231d30,  0x27211c2c,  0x251f1a2a,
	0x231e1928,  0x211c1826,  0x1f1b1624,  0x1d191522,
	0x1c181420,  0x1a17131e,  0x1915121c,  0x1714111a,
	0x16131018,  0x15120f18,  0x14110e16,  0x13100d14,
	0x120f0c14,  0x110e0c12,  0x100d0b12,  0x0f0d0a10,
	0x0e0c0a10,  0x0d0b090e,  0x0c0a090e,  0x0c0a080c,
	0x0b09070c,  0x0a09070a,  0x0a08070a,  0x0908060a,
	0x09070608,  0x08070508,  0x07060508,  0x00000000,
};

//#define OPEN_X264_EFE_CFG
#if 1
void BUF_SHARE_CFG(struct h264_slice_info *s, unsigned int *buf_addr_group, unsigned char *buf_ref_mby_size, unsigned char *buf_odma_spe_flag, unsigned char *buf_odma_alg_flag)
{
	int beyond_size = (s->buf_share_size + 1) * 64;

	//int c_pxl_space = (s->mb_width*16)*(s->mb_height*8);
	int y_space = (s->mb_width*16)*(s->mb_height*16+beyond_size);
	int c_space = (s->mb_width*16)*(s->mb_height*8+(beyond_size/2));
	int y_every_space = (s->mb_width*16)*beyond_size;
	int c_every_space = (s->mb_width*16)*(beyond_size/2);

	int mb_total = s->mb_width * s->mb_height;

	int spe_frm = 0;
	int spe_ad_flag = 0;
	int spe_mi_flag = 0;
	int last_spe_ad_flag = 0;
	int last_spe_mi_flag = 0;
	int c_ofst_addr_n = ((s->frame_idx)*c_every_space)/c_space;
	int last_c_ofst_addr_n = ((s->frame_idx-1)*c_every_space)/c_space;

	int tmp_beyond_caddr = 0;

	//calc last frame addr
	int last_y_ofst_addr = 0;
	int last_y_tmp_addr  = 0;

	int last_c_ofst_addr = 0;
	int last_c_tmp_addr  = 0;

	int y_ofst_addr   = 0;
	int y_tmp_addr    = 0;

	int c_ofst_addr   = 0;
	int c_tmp_addr    = 0;

	int spe_flag = 1;

	int alg_flag = 0;
	int c_pxl_space_row = 0;

	if(mb_total%2){
		//spe_frm = 1;
	}

	//buf_beyond_yaddr
	buf_addr_group[0] = s->fb[0][0] + y_space;
	//buf_beyond_caddr
	buf_addr_group[1] = spe_frm ? s->fb[0][1] + c_space + (128*C_SPE_ADD_MB_NUM) : s->fb[0][1] + c_space;
	tmp_beyond_caddr = spe_frm ? s->fb[0][1] + c_space + 128 : s->fb[0][1] + c_space;

	//calc last frame addr
	last_y_ofst_addr = s->frame_idx == 0 ? 0 : ((s->frame_idx-1)*y_every_space)%y_space;
	last_y_tmp_addr  = last_y_ofst_addr ? (buf_addr_group[0] - last_y_ofst_addr) : s->fb[0][0];

	last_c_ofst_addr = s->frame_idx == 0 ? 0 : ((s->frame_idx-1)*c_every_space)%c_space;
	last_c_tmp_addr  = last_c_ofst_addr ? (tmp_beyond_caddr - last_c_ofst_addr) : s->fb[0][1];

	if((spe_frm & last_c_tmp_addr) != s->fb[0][1]){
		if(last_c_ofst_addr_n%2){
			last_c_tmp_addr -= 256;
			last_spe_mi_flag = 1;
		}

		if(last_c_tmp_addr%256){
			last_c_tmp_addr += 128;
			last_spe_ad_flag = 1;
		}

		if((last_c_tmp_addr + c_every_space > buf_addr_group[1]) & last_spe_mi_flag){
			last_c_tmp_addr += 256;
		}
	}


	//refy_addr_ba0
	buf_addr_group[4]  = s->frame_idx == 0 ? s->fb[0][0] : last_y_tmp_addr;
	//refy_addr_ba1
	buf_addr_group[5]  = s->fb[0][0];
	//refc_addr_ba0
	buf_addr_group[6]  = s->frame_idx == 0 ? s->fb[0][1] : last_c_tmp_addr;
	//refc_addr_ba1
	buf_addr_group[7]  = s->fb[0][1];
	//ref_mby_size
	*buf_ref_mby_size  = !s->buf_share_en ? 0xff : ((buf_addr_group[0] - buf_addr_group[4])/(s->mb_width*16))/16 - 1;


	y_ofst_addr   = (s->frame_idx*y_every_space)%y_space;
	y_tmp_addr    = y_ofst_addr ? (buf_addr_group[0] - y_ofst_addr) : s->fb[0][0];
	//buf_start_yaddr
	buf_addr_group[2]   = s->frame_idx == 0 ? s->fb[0][0] : y_tmp_addr;

	c_ofst_addr   = (s->frame_idx*c_every_space)%c_space;
	c_tmp_addr    = c_ofst_addr ? (tmp_beyond_caddr - c_ofst_addr) : s->fb[0][1];
	//buf_start_caddr
	buf_addr_group[3]   = s->frame_idx == 0 ? s->fb[0][1] : c_tmp_addr;

	spe_flag = 1;
	if((spe_frm & buf_addr_group[3]) != s->fb[0][1]){
		if(c_ofst_addr_n%2){
			buf_addr_group[3] -= 256;
			spe_mi_flag = 1;
		}

		if(buf_addr_group[3]%256){
			buf_addr_group[3] += 128;
			spe_ad_flag = 1;
		}

		if((buf_addr_group[3] + c_every_space > buf_addr_group[1]) & spe_mi_flag){
			buf_addr_group[3] += 256;
			spe_flag = 0;
		}

	}

	*buf_odma_spe_flag = spe_flag;

	alg_flag = 0;
	c_pxl_space_row = s->mb_width*16*8;
	if(((buf_addr_group[1] - (128*C_SPE_ADD_MB_NUM)) - buf_addr_group[3])% c_pxl_space_row == 0 )
		alg_flag = 1;

	*buf_odma_alg_flag = alg_flag;
}
#endif

void H264E_T32V_SliceInit(struct h264_slice_info *s)
{
	unsigned int i, j;
	int checksum = 0;
	volatile unsigned int *chn = (volatile unsigned int *)s->des_va;

	//unsigned int RADIX_CFGC_BASE_VDMA =	 (RADIX_HID_M0 << 15);
	unsigned int RADIX_VDMA_BASE_VDMA =	 (RADIX_HID_M1 << 15);
	unsigned int RADIX_ODMA_BASE_VDMA =	 (RADIX_HID_M2 << 15);
	unsigned int RADIX_TMC_BASE_VDMA  =	 (RADIX_HID_M3 << 15);
	unsigned int RADIX_EFE_BASE_VDMA  =	 (RADIX_HID_M4 << 15);
	unsigned int RADIX_JRFD_BASE_VDMA =	 (RADIX_HID_M5 << 15);
	unsigned int RADIX_MCE_BASE_VDMA  =	 (RADIX_HID_M6 << 15);
	//unsigned int RADIX_TFM_BASE_VDMA  =    (RADIX_HID_M7 << 15);
	unsigned int RADIX_MD_BASE_VDMA   =	 (RADIX_HID_M8 << 15);
	//unsigned int RADIX_DT_BASE_VDMA   =    (RADIX_HID_M9 << 15);
	unsigned int RADIX_DBLK_BASE_VDMA =    (RADIX_HID_M10 << 15);
	//unsigned int RADIX_SAO_BASE_VDMA  =    (RADIX_HID_M11 << 15);
	//unsigned int RADIX_BC_BASE_VDMA   =    (RADIX_HID_M12 << 15);
	unsigned int RADIX_SDE_BASE_VDMA  =    (RADIX_HID_M13 << 15);
	//unsigned int RADIX_IPRED_BASE_VDMA=	 (RADIX_HID_M14 << 15);
	//unsigned int RADIX_STC_BASE_VDMA  =    (RADIX_HID_M15 << 15);
	unsigned int RADIX_VMAU_BASE_VDMA =    (RADIX_HID_M16 << 15);
	unsigned int RADIX_EMC_BASE_VDMA  =    (RADIX_HID_M18 << 15);

	/**************************************************
	  buf share cfg
	 *************************************************/
	unsigned int buf_beyond_yaddr;
	unsigned int buf_beyond_caddr;
	unsigned int buf_start_yaddr;
	unsigned int buf_start_caddr;
	unsigned int refy_addr_ba0;
	unsigned int refy_addr_ba1;
	unsigned int refc_addr_ba0;
	unsigned int refc_addr_ba1;
	unsigned char  ref_mby_size;
	unsigned char  odma_spe_flag;
	unsigned char  odma_alg_flag;

	unsigned int buf_addr_group[8];
	unsigned char buf_ref_mby_size;
	unsigned char buf_odma_spe_flag;
	unsigned char buf_odma_alg_flag;

	unsigned int is_264 = 1;
	unsigned int efe_extra_cfg0 = (((is_264 & 0x1) << 16) +  // 264 enable
			((1 & 0x1) << 12) +  // md/mce cfg parity check enable
			((0 & 0x3) << 5) +
			((1 & 0x1) << 4) +
			0);
	int efe_ctrl = (((0 & 0x1) << 0) |
			((1 & 0x1) << 1) |
			//((0 & 0x1) << 3) |
			((s->frame_type & 0x1) << 3) |
			((1 & 0x1) << 4)  |
			((1 & 0x1) << 11) |
			((s->mb_mode_use & 0x1) << 12) |
			((s->rotate_mode & 0x3) << 30)
			);
	unsigned int raw_avg_en       = 1;
	unsigned int emc_out_en[2]    = {1,1};
	unsigned int rrs1_en          = 0;
	unsigned int refresh_en       = 0;
	unsigned int rrs1_c_en        = 0;
	unsigned int sobel_16_or_14   = 1;

	/**************************************************
	  EFE configuration
	 *************************************************/
	int petbc_filter_valid = ((s->pet_filter_valid[3] & 0x1) |
			((s->pet_filter_valid[2] & 0x1)<< 1) |
			(s->pet_filter_valid[1] & 0x1) << 2 |
			(s->pet_filter_valid[0] & 0x1) << 3 );

	int ai_motion_smd_ofst0= ((s->qpg_ai_motion_smd_ofst[0][0][0] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[0][0][1] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[0][0][2] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[0][0][3] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[0][0][4] & 0x3f) <<  0 );
	int ai_motion_smd_ofst1= ((s->qpg_ai_motion_smd_ofst[0][0][5] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[0][0][6] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[0][0][7] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[0][1][0] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[0][1][1] & 0x3f) <<  0 );
	int ai_motion_smd_ofst2= ((s->qpg_ai_motion_smd_ofst[0][1][2] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[0][1][3] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[0][1][4] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[0][1][5] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[0][1][6] & 0x3f) <<  0 );
	int ai_motion_smd_ofst3= ((s->qpg_ai_motion_smd_ofst[0][1][7] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[1][0][0] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[1][0][1] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[1][0][2] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[1][0][3] & 0x3f) <<  0 );
	int ai_motion_smd_ofst4= ((s->qpg_ai_motion_smd_ofst[1][0][4] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[1][0][5] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[1][0][6] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[1][0][7] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[1][1][0] & 0x3f) <<  0 );
	int ai_motion_smd_ofst5= ((s->qpg_ai_motion_smd_ofst[1][1][1] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[1][1][2] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[1][1][3] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[1][1][4] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[1][1][5] & 0x3f) <<  0 );
	int ai_motion_smd_ofst6= ((s->qpg_ai_motion_smd_ofst[1][1][6] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[1][1][7] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[2][0][0] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[2][0][1] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[2][0][2] & 0x3f) <<  0 );
	int ai_motion_smd_ofst7= ((s->qpg_ai_motion_smd_ofst[2][0][3] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[2][0][4] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[2][0][5] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[2][0][6] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[2][0][7] & 0x3f) <<  0 );
	int ai_motion_smd_ofst8= ((s->qpg_ai_motion_smd_ofst[2][1][0] & 0x3f) << 24 |
			(s->qpg_ai_motion_smd_ofst[2][1][1] & 0x3f) << 18 |
			(s->qpg_ai_motion_smd_ofst[2][1][2] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[2][1][3] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[2][1][4] & 0x3f) <<  0 );
	int ai_motion_smd_ofst9= ((s->qpg_ai_motion_smd_ofst[2][1][5] & 0x3f) << 12 |
			(s->qpg_ai_motion_smd_ofst[2][1][6] & 0x3f) <<  6 |
			(s->qpg_ai_motion_smd_ofst[2][1][7] & 0x3f) <<  0 );

	int ifa_cfg = (s->ifa_en |
			(s->cps_en << 1)   |
			(s->cps_mode << 2) |
			(s->ery_en << 3) |
			(raw_avg_en << 5) |
			(s->smd_en << 6) |
			(s->sobel_en << 7) |
			(emc_out_en[0] << 8) |
			(emc_out_en[1] << 9) |
			(s->skin_cnt_thrd << 10) |
			(s->sobel_edge_thrd << 16) |
			(s->skin_lvl_en[0] << 24) |
			(s->skin_lvl_en[1] << 25) |
			(s->skin_lvl_en[2] << 26) |
			(s->petbc_en << 27)       |
			(sobel_16_or_14 << 31)
			//(s->var_flat_sub_size << 30)
			);
	int ifa_cfg1 = (0       |
			(s->pet_mode   << 1) |
			(rrs1_en    << 2));
	int rfh_cfg1 = refresh_en;
	int qpg_cfg  = (s->qpg_en |
			(s->qpg_table_en << 1) |
			(s->qpg_skin_en << 2)  |
			(s->qpg_ery_en << 3)   |
			(s->qpg_sobel_en << 4) |
			(s->qpg_smd_en << 5)   |
			(s->crp_thrd << 6)   |
			(s->qpg_skin_qp_ofst[0] & 0xff) << 8   |
			(s->qpg_skin_qp_ofst[1] & 0xff) << 16  |
			(s->qpg_skin_qp_ofst[2] & 0xff) << 24  );
	int qpg_cfg1 = ((s->qpg_petbc_en    & 0x1) |
			((s->qpg_filte_en   & 0x1) << 2) |
			((s->qpg_roi_en     & 0x1) << 3) |
			((s->ai_mark_type   & 0xf) << 4) |
			((s->ai_mark_en     & 0x1) << 8)  |
			((s->rc_dly_en      & 0x1) << 9)  |
			((s->rc_max_qp      & 0x3f) << 10)|
			((s->rc_min_qp      & 0x3f) << 16)|
			((s->rc_en          & 0x1)  << 22));

	int roi_ctrl_cfg[4];
	int roi_pos_cfg[16];
	int qpg_roi_area_en;

	int cplx_qp0  = ((s->qpg_cplx_qp_ofst[0]  & 0xff) |
			((s->qpg_cplx_qp_ofst[1] & 0xff) << 8)  |
			((s->qpg_cplx_qp_ofst[2] & 0xff) << 16) |
			((s->qpg_cplx_qp_ofst[3] & 0xff) << 24) );
	int cplx_qp1  = ((s->qpg_cplx_qp_ofst[4]  & 0xff) |
			((s->qpg_cplx_qp_ofst[5] & 0xff) << 8)  |
			((s->qpg_cplx_qp_ofst[6] & 0xff) << 16) |
			((s->qpg_cplx_qp_ofst[7] & 0xff) << 24) );

	int smd_cplx_thrd0 = s->qpg_smd_cplx_thrd[0] | (s->qpg_smd_cplx_thrd[1] << 16);
	int smd_cplx_thrd1 = s->qpg_smd_cplx_thrd[2] | (s->qpg_smd_cplx_thrd[3] << 16);
	int smd_cplx_thrd2 = s->qpg_smd_cplx_thrd[4] | (s->qpg_smd_cplx_thrd[5] << 16);
	int smd_cplx_thrd3 = s->qpg_smd_cplx_thrd[6] | (s->qpg_smd_cplx_thrd[7] << 16);

	int sobel_cplx_thrd0 = s->qpg_sobel_cplx_thrd[0] | (s->qpg_sobel_cplx_thrd[1] << 16);
	int sobel_cplx_thrd1 = s->qpg_sobel_cplx_thrd[2] | (s->qpg_sobel_cplx_thrd[3] << 16);
	int sobel_cplx_thrd2 = s->qpg_sobel_cplx_thrd[4] | (s->qpg_sobel_cplx_thrd[5] << 16);
	int sobel_cplx_thrd3 = s->qpg_sobel_cplx_thrd[6] | (s->qpg_sobel_cplx_thrd[7] << 16);

	int ifac_all_en = (//s->ifa_para.sobel_c_en |
					   //(s->ifa_para.ery_c_en << 1) |
			(s->smd_c_en << 2) |
			(s->motion_c_en << 3) |
			(s->rrs_c_en << 4) |
			(s->cps_c_en << 5) |
			(rrs1_c_en << 6 ));
	int ifac_all1_en = ((s->rrs_thrd_c  << 0) |
			(s->motion_of_thrd_c << 4) |
			(s->motion_thrd_c << 12));
	int skin_thrd0 =  (s->skin_thrd[0][0][1] |
			(s->skin_thrd[0][0][0] << 8) |
			(s->skin_thrd[0][1][1] << 16) |
			(s->skin_thrd[0][1][0] << 24) );
	int skin_thrd1 =  (s->skin_thrd[1][0][1] |
			(s->skin_thrd[1][0][0] << 8) |
			(s->skin_thrd[1][1][1] << 16) |
			(s->skin_thrd[1][1][0] << 24) );
	int skin_thrd2 =  (s->skin_thrd[2][0][1] |
			(s->skin_thrd[2][0][0] << 8) |
			(s->skin_thrd[2][1][1] << 16) |
			(s->skin_thrd[2][1][0] << 24) );
	int skin_lvl0  =  (s->skin_level[0] |
			(s->skin_level[1] << 16) );
	int skin_lvl1  =  (s->skin_level[2] |
			(s->skin_level[3] << 16) );
	int skin_factor = (  s->skin_mul_factor[0]         |
			(s->skin_mul_factor[1] << 8)   |
			(s->skin_mul_factor[2] << 16)  |
			(s->skin_mul_factor[3] << 24)  );
	int rrs_cfg = (s->efe_rrs_en |
			(s->rrs_mode << 1)   |
			(s->rrs_of_en << 2)  |
			(s->motion_en << 3)  |
			(s->rrs_thrd << 4)   |
			(s->motion_of_thrd << 8)|
			(s->motion_thrd << 16) );


	int qp_val      = ((s->cqp_offset & 0x1F) |
			(s->base_qp<<8) |
			(s->r_min_qp<<16) |
			(s->r_max_qp<<24));
	int pet_var_thr = ((s->petbc_var_thr[2] & 0x1ff) |
			((s->petbc_var_thr[1] & 0x1ff) << 9) |
			((s->petbc_var_thr[0] & 0x1ff) << 18));
	int pet_ssm_thr0 = ((s->petbc_ssm_thr[0][1] & 0xff) |
			((s->petbc_ssm_thr[0][0] & 0xff) << 8));
	int pet_ssm_thr1 = ((s->petbc_ssm_thr[1][3] & 0xff) |
			((s->petbc_ssm_thr[1][2] & 0xff) << 8)  |
			((s->petbc_ssm_thr[1][1] & 0xff) << 16) |
			((s->petbc_ssm_thr[1][0] & 0xff) << 24));
	int pet_ssm_thr2 = ((s->petbc_ssm_thr[2][3] & 0xff) |
			((s->petbc_ssm_thr[2][2] & 0xff) << 8)  |
			((s->petbc_ssm_thr[2][1] & 0xff) << 16) |
			((s->petbc_ssm_thr[2][0] & 0xff) << 24));
	int pet2_var_thr = ((s->petbc2_var_thr[2] & 0x1ff) |
			((s->petbc2_var_thr[1] & 0x1ff) << 9) |
			((s->petbc2_var_thr[0] & 0x1ff) << 18));
	int pet2_ssm_thr0 = ((s->petbc2_ssm_thr[0][1] & 0xff) |
			((s->petbc2_ssm_thr[0][0] & 0xff) << 8));
	int pet2_ssm_thr1 = ((s->petbc2_ssm_thr[1][3] & 0xff) |
			((s->petbc2_ssm_thr[1][2] & 0xff) << 8)  |
			((s->petbc2_ssm_thr[1][1] & 0xff) << 16) |
			((s->petbc2_ssm_thr[1][0] & 0xff) << 24));
	int pet2_ssm_thr2 = ((s->petbc2_ssm_thr[2][3] & 0xff) |
			((s->petbc2_ssm_thr[2][2] & 0xff) << 8)  |
			((s->petbc2_ssm_thr[2][1] & 0xff) << 16) |
			((s->petbc2_ssm_thr[2][0] & 0xff) << 24));

	int rrc_motion_thr0 = s->rrc_motion_thr[0] | ((s->rrc_motion_thr[1] & 0xffff) << 16);
	int rrc_motion_thr1 = s->rrc_motion_thr[2] | ((s->rrc_motion_thr[3] & 0xffff) << 16);
	int rrc_motion_thr2 = s->rrc_motion_thr[4] | ((s->rrc_motion_thr[5] & 0xffff) << 16);
	int rrc_motion_thr3 = s->rrc_motion_thr[6] ;

	int pet_qp_idx      =  ((s->qpg_pet_qp_idx[0] & 0x1f)        |
			((s->qpg_pet_qp_idx[1] & 0x1f) << 5)  |
			((s->qpg_pet_qp_idx[2] & 0x1f) << 10) |
			((s->qpg_pet_qp_idx[3] & 0x1f) << 15) |
			((s->qpg_pet_qp_idx[4] & 0x1f) << 20));
	int pet_cplx_thrd00 = ((s->qpg_pet_cplx_thrd[0][ 3] & 0xff)        |
			((s->qpg_pet_cplx_thrd[0][ 2] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[0][ 1] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[0][ 0] & 0xff) << 24));
	int pet_cplx_thrd01 = ((s->qpg_pet_cplx_thrd[0][ 7] & 0xff)        |
			((s->qpg_pet_cplx_thrd[0][ 6] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[0][ 5] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[0][ 4] & 0xff) << 24));
	int pet_cplx_thrd02 = ((s->qpg_pet_cplx_thrd[0][11] & 0xff)        |
			((s->qpg_pet_cplx_thrd[0][10] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[0][ 9] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[0][ 8] & 0xff) << 24));
	int pet_cplx_thrd10 = ((s->qpg_pet_cplx_thrd[1][ 3] & 0xff)        |
			((s->qpg_pet_cplx_thrd[1][ 2] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[1][ 1] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[1][ 0] & 0xff) << 24));
	int pet_cplx_thrd11 = ((s->qpg_pet_cplx_thrd[1][ 7] & 0xff)        |
			((s->qpg_pet_cplx_thrd[1][ 6] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[1][ 5] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[1][ 4] & 0xff) << 24));
	int pet_cplx_thrd12 = ((s->qpg_pet_cplx_thrd[1][11] & 0xff)        |
			((s->qpg_pet_cplx_thrd[1][10] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[1][ 9] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[1][ 8] & 0xff) << 24));
	int pet_cplx_thrd20 = ((s->qpg_pet_cplx_thrd[2][ 3] & 0xff)        |
			((s->qpg_pet_cplx_thrd[2][ 2] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[2][ 1] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[2][ 0] & 0xff) << 24));
	int pet_cplx_thrd21 = ((s->qpg_pet_cplx_thrd[2][ 7] & 0xff)        |
			((s->qpg_pet_cplx_thrd[2][ 6] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[2][ 5] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[2][ 4] & 0xff) << 24));
	int pet_cplx_thrd22 = ((s->qpg_pet_cplx_thrd[2][11] & 0xff)        |
			((s->qpg_pet_cplx_thrd[2][10] & 0xff) <<  8) |
			((s->qpg_pet_cplx_thrd[2][ 9] & 0xff) << 16) |
			((s->qpg_pet_cplx_thrd[2][ 8] & 0xff) << 24));

	int pet_qp_ofst0    =	((s->qpg_pet_qp_ofst[0][6] & 0xf )          |
			((s->qpg_pet_qp_ofst[0][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst[0][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst[0][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst[0][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst[0][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst[0][0] & 0xf ) << 24));
	int pet_qp_ofst1    =	((s->qpg_pet_qp_ofst[1][6] & 0xf )          |
			((s->qpg_pet_qp_ofst[1][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst[1][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst[1][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst[1][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst[1][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst[1][0] & 0xf ) << 24));
	int pet_qp_ofst2    =	((s->qpg_pet_qp_ofst[2][6] & 0xf )          |
			((s->qpg_pet_qp_ofst[2][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst[2][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst[2][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst[2][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst[2][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst[2][0] & 0xf ) << 24));
	int pet_qp_ofst3    =	((s->qpg_pet_qp_ofst[3][6] & 0xf )          |
			((s->qpg_pet_qp_ofst[3][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst[3][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst[3][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst[3][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst[3][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst[3][0] & 0xf ) << 24));
	int pet_qp_ofst4    =	((s->qpg_pet_qp_ofst[4][6] & 0xf )          |
			((s->qpg_pet_qp_ofst[4][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst[4][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst[4][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst[4][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst[4][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst[4][0] & 0xf ) << 24));

	int pet_qp_ofst_mt0 =	((s->qpg_pet_qp_ofst_mt[0][6] & 0xf )          |
			((s->qpg_pet_qp_ofst_mt[0][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst_mt[0][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst_mt[0][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst_mt[0][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst_mt[0][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst_mt[0][0] & 0xf ) << 24));
	int pet_qp_ofst_mt1 =	((s->qpg_pet_qp_ofst_mt[1][6] & 0xf )          |
			((s->qpg_pet_qp_ofst_mt[1][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst_mt[1][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst_mt[1][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst_mt[1][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst_mt[1][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst_mt[1][0] & 0xf ) << 24));
	int pet_qp_ofst_mt2 =	((s->qpg_pet_qp_ofst_mt[2][6] & 0xf )          |
			((s->qpg_pet_qp_ofst_mt[2][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst_mt[2][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst_mt[2][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst_mt[2][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst_mt[2][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst_mt[2][0] & 0xf ) << 24));
	int pet_qp_ofst_mt3 =	((s->qpg_pet_qp_ofst_mt[3][6] & 0xf )          |
			((s->qpg_pet_qp_ofst_mt[3][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst_mt[3][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst_mt[3][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst_mt[3][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst_mt[3][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst_mt[3][0] & 0xf ) << 24));
	int pet_qp_ofst_mt4 =	((s->qpg_pet_qp_ofst_mt[4][6] & 0xf )          |
			((s->qpg_pet_qp_ofst_mt[4][5] & 0xf ) <<  4)   |
			((s->qpg_pet_qp_ofst_mt[4][4] & 0xf ) <<  8)   |
			((s->qpg_pet_qp_ofst_mt[4][3] & 0xf ) << 12)   |
			((s->qpg_pet_qp_ofst_mt[4][2] & 0xf ) << 16)   |
			((s->qpg_pet_qp_ofst_mt[4][1] & 0xf ) << 20)   |
			((s->qpg_pet_qp_ofst_mt[4][0] & 0xf ) << 24));

	int qpg_dlt_thrd    = ((s->qpg_dlt_thr[0] & 0x3f)         |
			((s->qpg_dlt_thr[1] & 0x3f) << 6)  |
			((s->qpg_dlt_thr[2] & 0x3f) << 12) |
			((s->qpg_dlt_thr[3] & 0x3f) << 18));

	int cfm_cfg0 = ((s->force_i16 & 0x1)    << 0 |
			(s->i16_qp_sel & 0x1)   << 1 |
			(s->cplx_thd_sel & 0x1) << 2 |
			(s->refresh_en & 0x1)   << 4 |            // refresh_en
			(s->refresh_mode & 0x1) << 5 |
			(s->i16dc_qp_sel & 0x1) << 6 |
			(s->i16_qp_base & 0x3F) << 8 |
			(s->refresh_cplx_thd & 0xFF) << 16 |
			(s->i16dc_cplx_thd & 0xFF) << 24 );

	int cfm_cfg1 = ((s->i16dc_qp_base & 0x3F) << 0 );


	int cfm_cplx_idx_cfg0 = ((s->cplx_thd_idx[0] & 0x7) << 0  |
			(s->cplx_thd_idx[1] & 0x7) << 3  |
			(s->cplx_thd_idx[2] & 0x7) << 6  |
			(s->cplx_thd_idx[3] & 0x7) << 9  |
			(s->cplx_thd_idx[4] & 0x7) << 12 |
			(s->cplx_thd_idx[5] & 0x7) << 15 |
			(s->cplx_thd_idx[6] & 0x7) << 18 |
			(s->cplx_thd_idx[7] & 0x7) << 21 |
			(s->cplx_thd_idx[8] & 0x7) << 24 |
			(s->cplx_thd_idx[9] & 0x7) << 27 );

	int cfm_cplx_idx_cfg1 = ((s->cplx_thd_idx[10] & 0x7) << 0  |
			(s->cplx_thd_idx[11] & 0x7) << 3  |
			(s->cplx_thd_idx[12] & 0x7) << 6  |
			(s->cplx_thd_idx[13] & 0x7) << 9  |
			(s->cplx_thd_idx[14] & 0x7) << 12 |
			(s->cplx_thd_idx[15] & 0x7) << 15 |
			(s->cplx_thd_idx[16] & 0x7) << 18 |
			(s->cplx_thd_idx[17] & 0x7) << 21 |
			(s->cplx_thd_idx[18] & 0x7) << 24 |
			(s->cplx_thd_idx[19] & 0x7) << 27 );

	int cfm_cplx_idx_cfg2 = ((s->cplx_thd_idx[20] & 0x7) << 0 |
			(s->cplx_thd_idx[21] & 0x7) << 3 |
			(s->cplx_thd_idx[22] & 0x7) << 6 |
			(s->cplx_thd_idx[23] & 0x7) << 9 |
			(s->diff_cplx_thd & 0xFF) << 24 );

	int cfm_cplx_thrd_cfg0 = ((s->cplx_thd[0] & 0xFF) << 0  |
			(s->cplx_thd[1] & 0xFF) << 8  |
			(s->cplx_thd[2] & 0xFF) << 16 |
			(s->cplx_thd[3] & 0xFF) << 24 );

	int cfm_cplx_thrd_cfg1 = ((s->cplx_thd[4] & 0xFF) << 0  |
			(s->cplx_thd[5] & 0xFF) << 8  |
			(s->cplx_thd[6] & 0xFF) << 16 |
			(s->cplx_thd[7] & 0xFF) << 24 );

	int set_modectrl_bit0 = s->mb_mode_use & 0x1;
	int set_modectrl_bit2 = s->rrs_en || s->force_i16 || s->refresh_en;

	/**************************************************
	  JRFD configuration
	 *************************************************/
	unsigned int jrfd_y_addr = 0;
	unsigned int jrfd_c_addr = 0;
	int align_height = (s->frame_height + 15) & ~15;

	unsigned int esti_ctrl = 0;
	unsigned int md_cfg5 = ((1 & 0x1) << 0); //264 conf 1
											 //ned_disable cfg
	unsigned int md_ned_disable0    = {s->ned_sad_thr[0][0][0]  |
		s->ned_sad_thr[0][0][1] << 8 |
			s->ned_sad_thr[0][1][0] << 16 |
			s->ned_sad_thr[0][1][1] << 24 };
	unsigned int md_ned_disable1    = {s->ned_sad_thr[1][0][0]  |
		s->ned_sad_thr[1][0][1] << 5 |
			s->ned_sad_thr[1][1][0] << 10 |
			s->ned_sad_thr[1][1][1] << 16 |
			s->ned_motion_en        << 22 |
			s->ned_motion_shift     << 23 |
			s->c_sad_min_bias       << 26 |
			s->dct8x8_en            << 29 |
			s->recon_resi_en        << 30 |
			s->pskip_en             << 31
	};
	unsigned int md_ned_disable2    = {s->ned_sad_thr_cpx_step[0][0]       |
		s->ned_sad_thr_cpx_step[0][1] << 2  |
			s->ned_sad_thr_cpx_step[1][0] << 4  |
			s->ned_sad_thr_cpx_step[1][1] << 6
	};
	// rate control register and ram cfg
	unsigned int md_rcfg[7] = {0,0,0,0,0,0,0}; // rc cfg

	int emc_cfg_en = ( 1           // emc init
			| (0x1 <<1)   // bs wr en (always 1)
			| ((s->emc_cps_en    & 0x1)   << 2)
			| ((s->emc_sobel_en  & 0x1)   << 3)
			| ((s->emc_mix_en    & 0x1)   << 4)
			| ((s->emc_mixc_en   & 0x1)   << 5)
			| ((s->emc_flag_en   & 0x1)   << 7)
			| ((s->qpg_table_en  & 0x1)   << 9)
			| ((s->mb_mode_use   & 0x1)   << 11)
			| ((s->efe_rrs_en    & 0x1)   << 13)
			| ((s->ai_mark_en    & 0x1)   << 15) ) ;

	//when buf share is new addr(dynamic), else old addr(static)
	unsigned int odma_y_addr = 0;
	unsigned int odma_c_addr = 0;
	/////////////
	// jrfc
	/////////////
	unsigned int odma_jrfc_head_y_addr  = (unsigned int)s->jh[0][0];
	unsigned int odma_jrfc_head_c_addr  = (unsigned int)s->jh[0][1];
	unsigned int odma_jrfc_head_sp_y_addr = (unsigned int)s->spe_y_addr;
	unsigned int odma_jrfc_head_sp_c_addr = (unsigned int)s->spe_c_addr;
	/////////////
	// bufshare
	/////////////
	unsigned int odma_bfsh_base_y_addr  = (unsigned int)s->fb[0][0];
	unsigned int odma_bfsh_base_c_addr  = (unsigned int)s->fb[0][1];
	unsigned int odma_bfsh_bynd_y_addr  = 0;
	unsigned int odma_bfsh_bynd_c_addr  = 0;

	int idx = 0;

	s->mb_width  = (s->frame_width +15)/16;
	s->mb_height = (s->frame_height+15)/16;
	if(0){
		BUF_SHARE_CFG(s, buf_addr_group, &buf_ref_mby_size, &buf_odma_spe_flag, &buf_odma_alg_flag);

		buf_beyond_yaddr = buf_addr_group[0];
		buf_beyond_caddr = buf_addr_group[1];
		buf_start_yaddr  = buf_addr_group[2];
		buf_start_caddr  = buf_addr_group[3];
		refy_addr_ba0 = buf_addr_group[4];
		refy_addr_ba1 = buf_addr_group[5];
		refc_addr_ba0 = buf_addr_group[6];
		refc_addr_ba1 = buf_addr_group[7];
		ref_mby_size = buf_ref_mby_size;
		odma_spe_flag = buf_odma_spe_flag;
		odma_alg_flag = buf_odma_alg_flag;
	}

	buf_beyond_yaddr = s->buf_beyond_yaddr;
	buf_beyond_caddr = s->buf_beyond_caddr;
	buf_start_yaddr  = s->buf_start_yaddr;
	buf_start_caddr  = s->buf_start_caddr;
	refy_addr_ba0 = s->buf_ref_yaddr;
	refy_addr_ba1 = s->buf_base_yaddr;
	refc_addr_ba0 = s->buf_ref_caddr;
	refc_addr_ba1 = s->buf_base_caddr;
	ref_mby_size = s->buf_ref_rem_mby;
	odma_spe_flag = 0;
	odma_alg_flag = 0;

	for(i=0;i<4;i++)
		roi_ctrl_cfg[i] = (s->roi_info[i*4+0].roi_md << 0 |
				(s->roi_info[i*4+0].roi_qp & 0x3f) << 2 |
				s->roi_info[i*4+1].roi_md << 8 |
				(s->roi_info[i*4+1].roi_qp & 0x3f) << 10 |
				s->roi_info[i*4+2].roi_md << 16 |
				(s->roi_info[i*4+2].roi_qp & 0x3f) << 18 |
				s->roi_info[i*4+3].roi_md << 24 |
				(s->roi_info[i*4+3].roi_qp & 0x3f) << 26 );

	qpg_roi_area_en = (s->roi_info[ 0].roi_en <<  0|
			s->roi_info[ 1].roi_en <<  1|
			s->roi_info[ 2].roi_en <<  2|
			s->roi_info[ 3].roi_en <<  3|
			s->roi_info[ 4].roi_en <<  4|
			s->roi_info[ 5].roi_en <<  5|
			s->roi_info[ 6].roi_en <<  6|
			s->roi_info[ 7].roi_en <<  7|
			s->roi_info[ 8].roi_en <<  8|
			s->roi_info[ 9].roi_en <<  9|
			s->roi_info[10].roi_en << 10|
			s->roi_info[11].roi_en << 11|
			s->roi_info[12].roi_en << 12|
			s->roi_info[13].roi_en << 13|
			s->roi_info[14].roi_en << 14|
			s->roi_info[15].roi_en << 15
			);

	for(i=0;i<16;i++)
		roi_pos_cfg[i] = ((s->roi_info[i].roi_lmbx & 0xff) << 0  |
				(s->roi_info[i].roi_rmbx & 0xff) << 8  |
				(s->roi_info[i].roi_umby & 0xff) << 16 |
				(s->roi_info[i].roi_bmby & 0xff) << 24 );

	/*
	   int cplx_thrd0 = s->qpg_cplx_thrd[0] | (s->qpg_cplx_thrd[1] << 16);
	   int cplx_thrd1 = s->qpg_cplx_thrd[2] | (s->qpg_cplx_thrd[3] << 16);
	   int cplx_thrd2 = s->qpg_cplx_thrd[4] | (s->qpg_cplx_thrd[5] << 16);
	   int cplx_thrd3 = s->qpg_cplx_thrd[6] | (s->qpg_cplx_thrd[7] << 16);
	   */
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CTRL         , 0, efe_ctrl);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_FRM_SIZE     , 0,
			(((s->rotate_mode == 1 || s->rotate_mode == 3) ? s->hw_width : s->hw_height) << 16) |
			((s->rotate_mode == 1 || s->rotate_mode == 3) ? s->hw_height : s->hw_width) );
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RAWY_ADDR    , 0, (int)s->raw_y_pa);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RAWC_ADDR    , 0, (int)s->raw_c_pa);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RAW_STRD     , 0, EFE_RAW_STRDY(s->frame_y_stride) | EFE_RAW_STRDC(s->frame_y_stride));
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_FRM_QP       , 0, qp_val);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRS_CFG      , 0, rrs_cfg);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_EXTRA_CFG0   , 0, efe_extra_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFA_CFG      , 0, ifa_cfg);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFA_CFG1     , 0, ifa_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RFH_CFG1     , 0, rfh_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_QPG_CFG      , 0, qpg_cfg);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_QPG_CFG1     , 0, qpg_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFAC_ALL_EN  , 0, ifac_all_en);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFAC_ALL1_EN , 0, ifac_all1_en);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_FTR     , 0, skin_factor);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_LVL0    , 0, skin_lvl0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_LVL1    , 0, skin_lvl1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_THRD0   , 0, skin_thrd0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_THRD1   , 0, skin_thrd1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_THRD2   , 0, skin_thrd2);
	/*
	   GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_THRD0   , 0, cplx_thrd0);
	   GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_THRD1   , 0, cplx_thrd1);
	   GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_THRD2   , 0, cplx_thrd2);
	   GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_THRD3   , 0, cplx_thrd3);
	   */
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD0   , 0, smd_cplx_thrd0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD1   , 0, smd_cplx_thrd1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD2   , 0, smd_cplx_thrd2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD3   , 0, smd_cplx_thrd3);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD0   , 0, sobel_cplx_thrd0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD1   , 0, sobel_cplx_thrd1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD2   , 0, sobel_cplx_thrd2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD3   , 0, sobel_cplx_thrd3);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_EN       , 0, qpg_roi_area_en);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOA    , 0, roi_ctrl_cfg[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOB    , 0, roi_ctrl_cfg[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOC    , 0, roi_ctrl_cfg[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOD    , 0, roi_ctrl_cfg[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI0_POS     , 0, roi_pos_cfg[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI1_POS     , 0, roi_pos_cfg[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI2_POS     , 0, roi_pos_cfg[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI3_POS     , 0, roi_pos_cfg[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI4_POS     , 0, roi_pos_cfg[4]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI5_POS     , 0, roi_pos_cfg[5]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI6_POS     , 0, roi_pos_cfg[6]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI7_POS     , 0, roi_pos_cfg[7]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI8_POS     , 0, roi_pos_cfg[8]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI9_POS     , 0, roi_pos_cfg[9]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI10_POS    , 0, roi_pos_cfg[10]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI11_POS    , 0, roi_pos_cfg[11]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI12_POS    , 0, roi_pos_cfg[12]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI13_POS    , 0, roi_pos_cfg[13]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI14_POS    , 0, roi_pos_cfg[14]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI15_POS    , 0, roi_pos_cfg[15]);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_FILTER_VALID  , 0, petbc_filter_valid);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST0   , 0, ai_motion_smd_ofst0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST1   , 0, ai_motion_smd_ofst1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST2   , 0, ai_motion_smd_ofst2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST3   , 0, ai_motion_smd_ofst3);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST4   , 0, ai_motion_smd_ofst4);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST5   , 0, ai_motion_smd_ofst5);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST6   , 0, ai_motion_smd_ofst6);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST7   , 0, ai_motion_smd_ofst7);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST8   , 0, ai_motion_smd_ofst8);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST9   , 0, ai_motion_smd_ofst9);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_VAR_THR     , 0, pet_var_thr);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_SSM_THR0    , 0, pet_ssm_thr0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_SSM_THR1    , 0, pet_ssm_thr1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_SSM_THR2    , 0, pet_ssm_thr2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_VAR_THR    , 0, pet2_var_thr);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_SSM_THR0   , 0, pet2_ssm_thr0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_SSM_THR1   , 0, pet2_ssm_thr1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_SSM_THR2   , 0, pet2_ssm_thr2);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD00   , 0, pet_cplx_thrd00);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD01   , 0, pet_cplx_thrd01);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD02   , 0, pet_cplx_thrd02);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD10   , 0, pet_cplx_thrd10);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD11   , 0, pet_cplx_thrd11);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD12   , 0, pet_cplx_thrd12);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD20   , 0, pet_cplx_thrd20);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD21   , 0, pet_cplx_thrd21);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD22   , 0, pet_cplx_thrd22);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST0   , 0, pet_qp_ofst0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST1   , 0, pet_qp_ofst1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST2   , 0, pet_qp_ofst2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST3   , 0, pet_qp_ofst3);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST4   , 0, pet_qp_ofst4);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT0, 0, pet_qp_ofst_mt0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT1, 0, pet_qp_ofst_mt1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT2, 0, pet_qp_ofst_mt2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT3, 0, pet_qp_ofst_mt3);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT4, 0, pet_qp_ofst_mt4);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_QPG_DLT_THRD, 0, qpg_dlt_thrd);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_IDX, 0, pet_qp_idx);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_QP0, 0, cplx_qp0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_QP1, 0, cplx_qp1);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR0, 0, rrc_motion_thr0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR1, 0, rrc_motion_thr1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR2, 0, rrc_motion_thr2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR3, 0, rrc_motion_thr3);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CFG0, 0, cfm_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CFG1, 0, cfm_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_IDX_CFG0, 0, cfm_cplx_idx_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_IDX_CFG1, 0, cfm_cplx_idx_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_IDX_CFG2, 0, cfm_cplx_idx_cfg2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_THRD_CFG0, 0, cfm_cplx_thrd_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_THRD_CFG1, 0, cfm_cplx_thrd_cfg1);

	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_CTRL, 0, (((s->buf_share_en & 0x1 ) <<31) |
				(1<<30) | //is_helix
				(0<<28) | //clk_gate
				(s->jrfd_enable <<27) |
				(align_height <<14) |
				s->frame_width));

	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_HDYA, 0, s->jh[1][0]);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_HDCA, 0, s->jh[1][1]);

	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_HSTR, 0, (((s->cm_head_total & 0xffff) << 16) | (s->lm_head_total & 0xffff)) );

	jrfd_y_addr = s->buf_share_en ? (unsigned int)s->buf_ref_yaddr : (unsigned int)s->fb[1][0];
	jrfd_c_addr = s->buf_share_en ? (unsigned int)s->buf_ref_caddr : (unsigned int)s->fb[1][1];
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BDYA, 0, jrfd_y_addr);//tile Y address
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BDCA, 0, jrfd_c_addr); //tile C address

	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BSTR, 0,
			JRFD_BODY_STRDY(((s->frame_width+15)/16)*16) |
			JRFD_BODY_STRDC(((s->frame_width+15)/16)*8) );


	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BASEY_ADDR, 0, (int)s->buf_base_yaddr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BASEC_ADDR, 0, (int)s->buf_base_caddr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BEYDY_ADDR, 0, (int)s->buf_beyond_yaddr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BEYDC_ADDR, 0, (int)s->buf_beyond_caddr);


	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MHDY, 0, s->jh[2][0]);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MHDC, 0, s->jh[2][1]);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MBDY, 0, s->fb[2][0]);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MBDC, 0, s->fb[2][1]);

	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_TRIG, 0, ((1 << 5) |//init
				(0 << 4)  //ckg low means do clock-gating
				));//enable_tile

	/**************************************************
	  Motion configuration
	 *************************************************/
	if(s->frame_type) {
		esti_ctrl |= MCE_ESTI_CTRL_PUE_16X16;
		esti_ctrl |= (MCE_ESTI_CTRL_SCL(s->scl)/*trbl mvp*/ |
				MCE_ESTI_CTRL_FBG(0) |
				MCE_ESTI_CTRL_CLMV |
				MCE_ESTI_CTRL_MSS(s->max_sech_step_i) |
				MCE_ESTI_CTRL_QRL(s->qpel_en) |
				MCE_ESTI_CTRL_HRL(s->hpel_en) |
				MCE_ESTI_CTRL_RF8(0) |
				MCE_ESTI_CTRL_RF16(0) |
				MCE_ESTI_CTRL_RF32(0)
				);

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_FRM_SIZE , 0, (MCE_FRM_SIZE_FH(s->frame_height-1) |
					MCE_FRM_SIZE_FW(s->frame_width-1)));
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_FRM_STRD , 0, (MCE_FRM_STRD_STRDC(((s->frame_width+63)/64)*64) |
					MCE_FRM_STRD_STRDY(((s->frame_width+63)/64)*64)));

		if(s->buf_share_en){
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  SLUT_MCE_RLUT(0, 0) , 0, (int)s->buf_base_yaddr);//base
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  SLUT_MCE_RLUT(0, 0)+4 , 0, (int)s->buf_base_caddr);//base
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_BFSH_ADRY, 0, refy_addr_ba0);
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_BFSH_ADRC, 0, refc_addr_ba0);
			/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_REFY0_1, 0, refy_addr_ba1); */
			/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_REFC0_1, 0, refc_addr_ba1); */
		}
		else {
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_BFSH_ADRY, 0, s->fb[1][0]);
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_BFSH_ADRC, 0, s->fb[1][1]);
			/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_REFY0_1, 0, s->fb[3][0]); */
			/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_REFC0_1, 0, s->fb[3][1]); */
		}
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MREF_ADRY , 0, (int)s->fb[2][0]);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MREF_ADRC , 0, (int)s->fb[2][1]);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_BOTM_ID , 0, s->buf_ref_rem_mby);

		/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_BFSH_ADRY , 0, (int)s->fb[1][0]);//base */
		/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_BFSH_ADRC , 0, (int)s->fb[1][1]);//base */
		/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MREF_ADRY , 0, (int)s->fb[2][0]);//base */
		/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MREF_ADRC , 0, (int)s->fb[2][1]);//base */
		/* GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_BOTM_ID , 0, 0/\* s->buf_rem_mby *\/); */

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_PREF_EXPD , 0, (MCE_PREF_EXPD_L(64) |
					MCE_PREF_EXPD_R(64) |
					MCE_PREF_EXPD_D(32) |
					MCE_PREF_EXPD_T(32)));

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_COMP_CTRL , 0, (MCE_COMP_CTRL_CCE |
					MCE_COMP_CTRL_CTAP(MCE_TAP_TAP2 & 0x3) |//x265:tap4, svac:tap8
					MCE_COMP_CTRL_CSPT(MCE_SPT_BILI) |
					MCE_COMP_CTRL_CSPP(MCE_SPP_EPEL) |
					MCE_COMP_CTRL_YCE |
					MCE_COMP_CTRL_YTAP(MCE_TAP_TAP6) |
					MCE_COMP_CTRL_YSPT(MCE_SPT_AUTO) |
					MCE_COMP_CTRL_YSPP(MCE_SPP_QPEL)));
		for(i=0; i<16; i++){
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_Y+i*8 , 0,  MCE_ILUT_INFO(IntpFMT[H264_QPEL][i].intp[0],
						IntpFMT[H264_QPEL][i].intp_pkg[0],
						IntpFMT[H264_QPEL][i].hldgl,/*idgl*/
						0,/*edgl*/
						IntpFMT[H264_QPEL][i].intp_dir[0],
						IntpFMT[H264_QPEL][i].intp_rnd[0],
						IntpFMT[H264_QPEL][i].intp_sft[0],
						IntpFMT[H264_QPEL][i].intp_sintp[0],
						IntpFMT[H264_QPEL][i].intp_srnd[0],
						IntpFMT[H264_QPEL][i].intp_sbias[0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_Y+i*8+4 , 0,MCE_ILUT_INFO(IntpFMT[H264_QPEL][i].intp[1],
						IntpFMT[H264_QPEL][i].intp_pkg[1],
						IntpFMT[H264_QPEL][i].hldgl,/*idgl*/
						0,
						IntpFMT[H264_QPEL][i].intp_dir[1],
						IntpFMT[H264_QPEL][i].intp_rnd[1],
						IntpFMT[H264_QPEL][i].intp_sft[1],
						IntpFMT[H264_QPEL][i].intp_sintp[1],
						IntpFMT[H264_QPEL][i].intp_srnd[1],
						IntpFMT[H264_QPEL][i].intp_sbias[1]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16 , 0, MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[0][3],
						IntpFMT[H264_QPEL][i].intp_coef[0][2],
						IntpFMT[H264_QPEL][i].intp_coef[0][1],
						IntpFMT[H264_QPEL][i].intp_coef[0][0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16+4 , 0, MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[0][7],
						IntpFMT[H264_QPEL][i].intp_coef[0][6],
						IntpFMT[H264_QPEL][i].intp_coef[0][5],
						IntpFMT[H264_QPEL][i].intp_coef[0][4]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16+8 , 0, MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[1][3],
						IntpFMT[H264_QPEL][i].intp_coef[1][2],
						IntpFMT[H264_QPEL][i].intp_coef[1][1],
						IntpFMT[H264_QPEL][i].intp_coef[1][0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16+12 , 0,MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[1][7],
						IntpFMT[H264_QPEL][i].intp_coef[1][6],
						IntpFMT[H264_QPEL][i].intp_coef[1][5],
						IntpFMT[H264_QPEL][i].intp_coef[1][4]));
		}
		for(i=0; i<16; i++){

			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_C+i*8 , 0, MCE_ILUT_INFO(IntpFMT[H264_EPEL][i].intp[0], /*fir & 0x1*/
						IntpFMT[H264_EPEL][i].intp_pkg[0],/*clip & 0x1*/
						0,/*idgl*/
						0,/*edgl*/
						IntpFMT[H264_EPEL][i].intp_dir[0],
						IntpFMT[H264_EPEL][i].intp_rnd[0],
						IntpFMT[H264_EPEL][i].intp_sft[0],
						0,/*savg & 0x1*/
						0,/*srnd*/
						0));/*sbias*/
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_C+i*8+4 , 0, MCE_ILUT_INFO(IntpFMT[H264_EPEL][i].intp[1],
						IntpFMT[H264_EPEL][i].intp_pkg[1],
						0, 0,
						IntpFMT[H264_EPEL][i].intp_dir[1],
						IntpFMT[H264_EPEL][i].intp_rnd[1],
						IntpFMT[H264_EPEL][i].intp_sft[1],
						0, 0, 0));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_C+i*16 , 0, MCE_CLUT_INFO(IntpFMT[H264_EPEL][i].intp_coef[0][3],
						IntpFMT[H264_EPEL][i].intp_coef[0][2],
						IntpFMT[H264_EPEL][i].intp_coef[0][1],
						IntpFMT[H264_EPEL][i].intp_coef[0][0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_C+i*16+4 , 0, MCE_CLUT_INFO(IntpFMT[H264_EPEL][i].intp_coef[0][7],
						IntpFMT[H264_EPEL][i].intp_coef[0][6],
						IntpFMT[H264_EPEL][i].intp_coef[0][5],
						IntpFMT[H264_EPEL][i].intp_coef[0][4]));
		}

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_SLC_SPOS , 0, 0);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_SLC_MV , 0, (MCE_SLC_MVY(s->glb_mvy) | MCE_SLC_MVX(s->glb_mvx)));

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_ESTI_CTRL , 0, esti_ctrl);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MRGI , 0, 0);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA +  REG_MCE_MVR, 0,
				MCE_MVR_MVRY(s->max_mvry_i*4) |
				MCE_MVR_MVRX(s->max_mvrx_i*4) );
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_GLB_CTRL , 0, (MCE_GLB_CTRL_INIT |
					MCE_GLB_CTRL_RESI(0) |//(cu32, cu16, cu8)
					MCE_GLB_CTRL_VTYPE(0x2) |//encode formate 0:x265, 1:svac, 2:x264
					MCE_GLB_CTRL_FMT(0x2) |
					MCE_GLB_CTRL_ED(0) |//0:encode, 1:decode
					MCE_GLB_CTRL_TMVP(0) |
					MCE_GLB_CTRL_PS(s->pskip_en) |
					MCE_GLB_CTRL_MREF(s->mref_en) | //ref1-mv0 enable
					MCE_GLB_CTRL_DCT8(s->dct8x8_en) |
					(0<<4)));//crc_en
	}

	/**************************************************
	  VMAU configuration
	 *************************************************/
	// vmau global
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_GBL_CTR, 0, (VMAU_TFM8_SCA_CUSTOM(s->is_scaling_custom)
				| VMAU_NED_CTRL(s->efe_rrs_en)
				| VMAU_FMT_H264) ); // fixed: fmt
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_VIDEO_TYPE, 0, (VMAU_MODE_ENC
				| VMAU_YUV_GRAY
				| VMAU_IP_MODE_MASK
				| VMAU_IS_ISLICE(!s->frame_type)) ); // fixed: enc, gray, mode_msk
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_Y_GS, 0, (VMAU_FRM_WID(s->mb_width*16)
				| VMAU_FRM_HEI(s->mb_height*16)) );

	// tfm config
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_DEADZONE, 0, ( VMAU_DEADZONE0_IY(s->deadzone[0])
				| VMAU_DEADZONE1_PY(s->deadzone[1])
				| VMAU_DEADZONE2_IC(s->deadzone[2])
				| VMAU_DEADZONE3_PC(s->deadzone[3]) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_DEADZONE1, 0, ((s->deadzone[4] & 0x3f) << 0 |
				(s->deadzone[5] & 0x3f) << 6 |
				(s->deadzone[6] & 0x3f) << 12 |
				(s->deadzone[7] & 0x3f) << 18 |
				(s->deadzone[8] & 0x3f) << 24));
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_ACMASK, 0, (s->acmask_mode<<14) + 2 ); /* 2 for TEST: rd file */

	// md config
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG0, 0, ( VMAU_MD_SLICE_I(!s->frame_type)
				| VMAU_MD_SLICE_P(s->frame_type)
				| VMAU_MD_I4_DIS(0)
				| VMAU_MD_I16_DIS(0)
				| VMAU_MD_PSKIP_DIS(0)
				| VMAU_MD_P_L0_DIS(0)
				| VMAU_MD_I8_DIS(0)
				| VMAU_MD_PT8_DIS(0)
				| VMAU_MD_DREF_EN(s->mref_en)
				| VMAU_MD_DCT8_EN(s->dct8x8_en)
				| VMAU_MD_FRM_REDGE(s->mb_width - 1)
				| VMAU_MD_FRM_BEDGE(s->mb_height - 1) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG1, 0, ( VMAU_IPMY_BIAS_EN(s->intra_lambda_y_bias_en)
				| VMAU_IPMC_BIAS_EN(s->intra_lambda_c_bias_en)
				| VMAU_COST_BIAS_EN(s->cost_bias_en)
				| VMAU_CSSE_BIAS_EN(s->chroma_sse_bias_en)
				| VMAU_JMLAMBDA2_EN(s->jm_lambda2_en)
				| VMAU_INTER_NEI_EN(s->inter_nei_en)
				| VMAU_SKIP_BIAS_EN(s->skip_bias_en)
				| VMAU_LMD_BIAS_EN(s->sse_lambda_bias_en)
				| VMAU_INFO_EN(s->info_en)
				| VMAU_DCM_EN(s->dcm_en)
				| VMAU_MVDS_ALL(s->mvd_sum_all)
				| VMAU_MVDS_ABS(s->mvd_sum_abs)
				| VMAU_MVS_ALL(s->mv_sum_all)
				| VMAU_MVS_ABS(s->mv_sum_abs)
				| VMAU_P_L0_BIAS(s->cost_bias_p_l0)
				| VMAU_PSKIP_BIAS(s->cost_bias_p_skip)
				| VMAU_I4_BIAS(s->cost_bias_i_4x4)
				| VMAU_I16_BIAS(s->cost_bias_i_16x16) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG2, 0, ( VMAU_IPM_BIAS_0(s->intra_lambda_bias_0)
				| VMAU_IPM_BIAS_1(s->intra_lambda_bias_1)
				| VMAU_IPM_BIAS_2(s->intra_lambda_bias_2)
				| VMAU_IPM_BIAS_QP0(s->intra_lambda_bias_qp0)
				| VMAU_IPM_BIAS_QP1(s->intra_lambda_bias_qp1)
				| VMAU_MD_FBC_EP(s->fbc_ep) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG3, 0, ( VMAU_CSSE_BIAS_0(s->chroma_sse_bias_0)
				| VMAU_CSSE_BIAS_1(s->chroma_sse_bias_1)
				| VMAU_CSSE_BIAS_2(s->chroma_sse_bias_2)
				| VMAU_CSSE_BIAS_QP0(s->chroma_sse_bias_qp0)
				| VMAU_CSSE_BIAS_QP1(s->chroma_sse_bias_qp1)
				| VMAU_LMD_BIAS(s->sse_lambda_bias)
				| VMAU_PL0_FS_DIS(s->p_skip_pl0f_dis)
				| VMAU_PT8_FS_DIS(s->p_skip_pt8f_dis)
				| VMAU_PL0_COST_MAX(s->p_l0_dis)
				| VMAU_PT8_COST_MAX(s->p_t8_dis)
				) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG4, 0, ( VMAU_YSSE_THR(s->ysse_thr)
				| VMAU_I8_BIAS(s->cost_bias_i_8x8)
				| VMAU_PT8_BIAS(s->cost_bias_p_t8)
				) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG5, 0, ( VMAU_CSSE_THR(s->csse_thr)
				| VMAU_CQP_OFFSET(s->cqp_offset)
				| VMAU_I4_COST_MAX(s->i_4x4_dis)
				| VMAU_I8_COST_MAX(s->i_8x8_dis)
				| VMAU_I16_COST_MAX(s->i_16x16_dis)
				) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG6, 0, ( VMAU_DCM_PARAM(s->dcm_param)
				| VMAU_SDE_PRIOR(s->sde_prior)
				| VMAU_DB_PRIOR(s->db_prior)
				) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG7, 0, ( VMAU_CFG_SIZE_X(s->cfg_size_x)
				| VMAU_CFG_SIZE_Y(s->cfg_size_y)
				| VMAU_CFG_IW_THR(s->cfg_iw_thr)
				| VMAU_CFG_BASEQP(s->base_qp)
				| VMAU_CFG_ALPHA(s->alpha_c0_offset)
				| VMAU_PS_COST_MAX(s->p_skip_dis)
				) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG8, 0, ( VMAU_CFG_MVR_THR1(s->cfg_mvr_thr1)
				| VMAU_CFG_MVR_THR2(s->cfg_mvr_thr2) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG9, 0, ( VMAU_CFG_MVR_THR3(s->cfg_mvr_thr3)
				| VMAU_CFG_BETA(s->beta_offset)
				) );
	/* eigen leverages mode decision */
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_CFG11, 0, ( VMAU_CFG_MD_RBIAS(s->refresh_bias)
				| VMAU_CFG_MD_RBIAS_EN(1)
				| VMAU_CFG_MD_PCDC_N0(s->mode_ctrl >> 3 & 7)
				| VMAU_CFG_MD_IFA_VLD(set_modectrl_bit2)
				| VMAU_CFG_MD_SLV_VLD(0)
				| VMAU_CFG_MD_SET_VLD((s->mode_ctrl & 0x2) || set_modectrl_bit0)
				) );
	for (j = 0; j < 40; ++j)
		GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_MD_MODE, 0, s->mode_ctrl_param[j] );
	for (j = 0; j < 42; j++)
		GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_CTX, 0, s->state[ j<10 ? ( !s->frame_type ? j+3 : j+14 )
				: j<22 ? j-10+73
				: j<28 ? j-22+64 : j-28+40 ] );

	// md ned config
	RADIX_GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG5 , 0, md_cfg5);
	md_rcfg[0] = (((s->rc_en & 0x1) << 0) +
			(((s->bu_size - 1) & 0xFFFF) << 4) +
			((0 & 0x1) << 20) +
			((0 & 0x1) << 21) +
			((1 & 0x1) << 22) +
			((s->bu_len & 0xFF) << 23) +
			((s->rc_dly_en & 0x1) << 31));
	md_rcfg[1] = (((s->bu_max_dqp  & 0x1F) << 0)  +
			((s->bu_min_dqp  & 0x1F) << 5)  +
			((s->rc_avg_len  & 0x1F) << 10) +
			((s->rc_max_prop & 0x7)  << 15) +
			((s->rc_min_prop & 0x7)  << 18) +
			((s->rc_method   & 0x7)  << 21));

	md_rcfg[2] = (((s->rc_thd[0]  & 0xFFFF) << 0) +
			((s->rc_thd[1]  & 0xFFFF) << 16));
	md_rcfg[3] = (((s->rc_thd[2]  & 0xFFFF) << 0) +
			((s->rc_thd[3]  & 0xFFFF) << 16));
	md_rcfg[4] = (((s->rc_thd[4]  & 0xFFFF) << 0) +
			((s->rc_thd[5]  & 0xFFFF) << 16));
	md_rcfg[5] = (((s->rc_thd[6]  & 0xFFFF) << 0) +
			((s->rc_thd[7]  & 0xFFFF) << 16));
	md_rcfg[6] = (((s->rc_thd[8]  & 0xFFFF) << 0) +
			((s->rc_thd[9]  & 0xFFFF) << 16));
	md_rcfg[7] = (((s->rc_thd[10] & 0xFFFF) << 0) +
			((s->rc_thd[11] & 0xFFFF) << 16));

	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_0  , 0, md_ned_disable0);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_1  , 0, md_ned_disable1);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_2  , 0, md_ned_disable2);

	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG0 , 0, md_rcfg[0]);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG1 , 0, md_rcfg[1]);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG2 , 0, md_rcfg[2]);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG3 , 0, md_rcfg[3]);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG4 , 0, md_rcfg[4]);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG5 , 0, md_rcfg[5]);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG6 , 0, md_rcfg[6]);
	GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG7 , 0, md_rcfg[7]);
	for (i=0; i<s->bu_len; i++) {
		GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + (((1 << 9) + i) << 2), 0, s->rc_info[i]);
	}

	// ipred config
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG0, 0, (VMAU_IP_MD_VAL(s->force_i16dc) |
				VMAU_IP_REF_NEB_4(s->ref_neb_4) |
				VMAU_IP_REF_NEB_8(s->ref_neb_8) |
				VMAU_IP_REF_PRD_C(s->pri_uv) |
				VMAU_IP_REF_PRD_16(s->pri_16) |
				VMAU_IP_REF_PRD_8(s->pri_8) |
				VMAU_IP_REF_PRD_4(s->pri_4) |
				VMAU_IP_REF_C4_EN(s->c_4_en) |
				VMAU_IP_REF_C8_EN(s->c_8_en) |
				VMAU_IP_REF_C16_EN(s->c_16_en) |
				VMAU_IP_REF_CUV_EN(s->c_uv_en) |
				VMAU_IP_REF_LMD4_EN(s->lamb_4_en) |
				VMAU_IP_REF_LMD8_EN(s->lamb_8_en) |
				VMAU_IP_REF_LMD16_EN(s->lamb_16_en) |
				VMAU_IP_REF_LMDUV_EN(s->lamb_uv_en) |
				VMAU_IP_REF_BIT4_EN(s->bit_4_en) |
				VMAU_IP_REF_BIT8_EN(s->bit_8_en) |
				VMAU_IP_REF_BIT16_EN(s->bit_16_en) |
				VMAU_IP_REF_BITUV_EN(s->bit_uv_en) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG1, 0, (VMAU_IP_REF_4_BIT0(s->bit_4[0]) |
				VMAU_IP_REF_4_BIT1(s->bit_4[1]) |
				VMAU_IP_REF_4_BIT2(s->bit_4[2]) |
				VMAU_IP_REF_4_BIT3(s->bit_4[3]) |
				VMAU_IP_REF_8_BIT0(s->bit_8[0]) |
				VMAU_IP_REF_8_BIT1(s->bit_8[1]) |
				VMAU_IP_REF_8_BIT2(s->bit_8[2]) |
				VMAU_IP_REF_8_BIT3(s->bit_8[3]) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG2, 0, (VMAU_IP_REF_C_BIT0(s->bit_uv[0]) |
				VMAU_IP_REF_C_BIT1(s->bit_uv[1]) |
				VMAU_IP_REF_C_BIT2(s->bit_uv[2]) |
				VMAU_IP_REF_C_BIT3(s->bit_uv[3]) |
				VMAU_IP_REF_16_BIT0(s->bit_16[0]) |
				VMAU_IP_REF_16_BIT1(s->bit_16[1]) |
				VMAU_IP_REF_16_BIT2(s->bit_16[2]) |
				VMAU_IP_REF_16_BIT3(s->bit_16[3]) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG3, 0, (VMAU_IP_REF_LMDUV_IFO(s->lambda_infouv) |
				VMAU_IP_REF_LMD16_IFO(s->lambda_info16) |
				VMAU_IP_REF_LMD8_IFO(s->lambda_info8) |
				VMAU_IP_REF_LMD4_IFO(s->lambda_info4) |
				VMAU_IP_REF_NEB_4REF(s->ref_4) |
				VMAU_IP_REF_NEB_8REF(s->ref_8) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG4, 0, (VMAU_IP_REF_C4_IFO((s->const_4[0]<<0) |
					(s->const_4[1]<<4) |
					(s->const_4[2]<<8) |
					(s->const_4[3]<<12) ) |
				VMAU_IP_REF_C8_IFO((s->const_8[0]<<0) |
					(s->const_8[1]<<4) |
					(s->const_8[2]<<8) |
					(s->const_8[3]<<12) ) ) );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG5, 0, (VMAU_IP_REF_C16_IFO((s->const_16[0]<<0) |
					(s->const_16[1]<<4) |
					(s->const_16[2]<<8) |
					(s->const_16[3]<<12) ) |
				VMAU_IP_REF_CUV_IFO((s->const_uv[0]<<0) |
					(s->const_uv[1]<<4) |
					(s->const_uv[2]<<8) |
					(s->const_uv[3]<<12) ) ) );

	// vmau global
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_GBL_RUN, 0, VMAU_RUN );

	/**************************************************
	  DBLK configuration
	 *************************************************/
	GEN_VDMA_ACFG(chn, RADIX_DBLK_BASE_VDMA + REG_DBLK_CFG, 0, ((1 << 31) | // 0:265, 1:264
				DBLK_CFG_ALPHA(s->alpha_c0_offset + 12)
				| DBLK_CFG_BETA(s->beta_offset + 12)
				| DBLK_CFG_NO_LFT(!s->deblock) ) );
	GEN_VDMA_ACFG(chn, RADIX_DBLK_BASE_VDMA + REG_DBLK_TRIG, 0, (DBLK_SLICE_RUN));

	/**************************************************
	  SDE configuration
	 *************************************************/
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_STAT, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_GL_CTRL, 0, (SDE_MODE_STEP | 1) );
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(s->mb_height, s->mb_width,
				s->first_mby, 0) );
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CODEC_ID, 0, SDE_FMT_H264_ENC);
	//slice_info0 {desp_link_en[1], auto_syn_en[0]}
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG0, 0, 0x3);
	//slice_info1 {qp[8], slice_type[0]}
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG1, 0, ((s->qp << 8) |
				(s->mref_en << 6) | /*dual-ref enable*/
				(s->dct8x8_en << 5) | /*dct8x8 enable*/
				(s->skip_en << 4) |
				(s->frame_type + 1)) );
	//desp_addr
	//GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG2, 0, VRAM_SDE_CHN_BASE);
	//sync_addr
	//GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG3, 0, VRAM_SDE_SYNA);
	//bs_addr
	//GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG4, 0, s->bs);
	//max bs size
	//GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG5, 0, ((0<<31) | (1<<22)) ); // frame max bs size, [31]:enable, [23:0]:max_size
	//context table
	for(i=0; i<460; i++){
		idx = s->state[i];
		if(idx <= 63)
			idx = 63 - idx;
		else
			idx -= 64;
		GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CTX_TBL+i*4, 0,
				(lps_range[idx] | ((s->state[i]>>6) & 0x1)) );
	}
	//init sync
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_SL_CTRL, 0, SDE_SLICE_INIT);
	/*************************************************************************************
	  TMC Module
	 *************************************************************************************/
	RADIX_GEN_VDMA_ACFG(chn,RADIX_TMC_BASE_VDMA + HERA_REG_TMC_CTRL, 0, ( 0 | (1 << 1) ));

	/**************************************************
	  EMC init
	 *************************************************/
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_BSFULL_EN  , 0, s->bsfull_intr_en   );
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_BSFULL_SIZE, 0, s->bsfull_intr_size );

	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_BS0  , 0, VDMA_ACFG_DHA(s->emc_bs_pa));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_BS1  , 0, VDMA_ACFG_DHA(s->emc_bs_pa1));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_CPS  , 0, VDMA_ACFG_DHA(s->emc_cps_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_AI   , 0, VDMA_ACFG_DHA(s->emc_ai_mark_pa));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_SOBEL, 0, VDMA_ACFG_DHA(s->emc_sobel_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_MIX  , 0, VDMA_ACFG_DHA(s->emc_mix_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_FLAG , 0, VDMA_ACFG_DHA(s->emc_flag_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_QPT  , 0, VDMA_ACFG_DHA(s->emc_qpt_pa));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_IPC  , 0, VDMA_ACFG_DHA(s->emc_mod_pa));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_MIXC , 0, VDMA_ACFG_DHA(s->emc_mixc_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_RRS0 , 0, VDMA_ACFG_DHA(s->emc_cps_ref0));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_CFG       , 0, emc_cfg_en );

	/**************************************************
	  ODMA configuration
	 *************************************************/
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_CTRL, 0, (s->buf_share_en << 31 |
				(s->jrfc_enable << 30) |//buf share
				(odma_alg_flag << 29) |
				(s->frame_height << 14) |
				s->frame_width));

	odma_y_addr = s->buf_share_en ? buf_start_yaddr : s->fb[0][0];
	odma_c_addr = s->buf_share_en ? buf_start_caddr : s->fb[0][1];
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BDYA, 0, odma_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BDCA, 0, odma_c_addr);

	/*  GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BSTR, 0,
	//		ODMA_REC_STRDY(((s->frame_width+15)/16)*16) |
	//		ODMA_REC_STRDC(((s->frame_width+15)/16)*8) );
	ODMA_REC_STRDY(((s->frame_width+63)/64)*64) |
	ODMA_REC_STRDC(((s->frame_width+63)/64)*64));
	*/

	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_HDYA, 0, odma_jrfc_head_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_HDCA, 0, odma_jrfc_head_c_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_SPYA, 0, odma_jrfc_head_sp_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_SPCA, 0, odma_jrfc_head_sp_c_addr);

	odma_bfsh_bynd_y_addr  = (unsigned int)buf_beyond_yaddr;
	odma_bfsh_bynd_c_addr  = (unsigned int)buf_beyond_caddr;
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BASEY_ADDR, 0, odma_bfsh_base_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BASEC_ADDR, 0, odma_bfsh_base_c_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BEYDY_ADDR, 0, odma_bfsh_bynd_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BEYDC_ADDR, 0, odma_bfsh_bynd_c_addr);


	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_TRIG, 0,
			((1 << 31) |  //0:265, 1:264
			 (0 << 6) |  //0 is crc en
			 (1 << 5) | //init
			 (0 << 4)  //ckg low means do clock-gating
			));

	//efe start
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_STAT, 0, 1);

	GEN_VDMA_ACFG(chn, RADIX_VDMA_BASE_VDMA + RADIX_REG_CFGC_ACM_CTRL, VDMA_ACFG_TERM, 0);
#if 0
	int cn_len = chn - s->des_va;
	printk("cn_len=%d\n", cn_len);
	unsigned int* tmp1=(unsigned int*)s->des_va;
	for (i=0; i<cn_len; i++) {
		printk("%d, 0x%x, \n",i,tmp1[i]);
	}
#endif
}
