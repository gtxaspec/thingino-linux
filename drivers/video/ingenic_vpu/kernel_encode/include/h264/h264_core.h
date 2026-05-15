#ifndef _H264_CORE_H_
#define _H264_CORE_H_

#include "vpu_core.h"
#include "h264_bitstream.h"
#include "h264_slice.h"
#include "h264_nal.h"

//slice type
#define H264_SLICE_TYPE_P   0
#define H264_SLICE_TYPE_B   1
#define H264_SLICE_TYPE_IDR 2
#define H264_SLICE_TYPE_VI  6

typedef struct h264_instance h264_t;

struct h264_header_info {
	bs_t bs;
	i264e_nal_t nal;
	i264e_sps_t sps;
	i264e_pps_t pps;
	i264e_slice_header_t header;
};

struct h264_param {
	unsigned short width;
	unsigned short height;
	unsigned short y_stride;
	unsigned short c_stride;
	unsigned short qp;
	unsigned short gop;
	unsigned short fps;
	unsigned short fps_den;
	unsigned char  state[1024]; //cabac
	unsigned char  *bs;
	unsigned int   bs_len;
};

typedef struct roi_info_t{
	unsigned char roi_en;
	unsigned char roi_md;
	unsigned char roi_qp;
	unsigned char roi_lmbx;
	unsigned char roi_rmbx;
	unsigned char roi_umby;
	unsigned char roi_bmby;
} roi_info_t;

struct h264_slice_info {
	/*basic*/
	unsigned char frame_type;
	unsigned short mb_width;
	unsigned short mb_height;
	unsigned char first_mby;
	unsigned char last_mby;  //for multi-slice

	/* motion */
	int frame_width;
	int frame_height;
	int hw_width;
	int hw_height;
	int rotate_mode;
	unsigned short frame_y_stride;		// frame Y stride
	unsigned short frame_c_stride;		// frame UV stride
	unsigned int raw_y_pa;		// raw Y base physical address
	unsigned int raw_c_pa;		// raw C base physical address
	unsigned char frm_re[4];
	unsigned char pskip_en;
	unsigned char mref_en;
	unsigned char dct8x8_en;
	unsigned char scl;
	unsigned char hpel_en;
	unsigned char qpel_en;
	unsigned char ref_mode;
	unsigned int max_sech_step_i;
	unsigned int max_mvrx_i;
	unsigned int max_mvry_i;
	unsigned char lambda_scale_parameter;
	unsigned char fs_en; //fs function enable
	unsigned int fs_md; //fs step mode, 0: 1, 1: 3
	unsigned char fs_px; //fs period x
	unsigned char fs_py; //fs period y
	unsigned char fs_rx; //fs range x, must be multiples of 3
	unsigned char fs_ry; //fs range y, must be multiples of 3
	unsigned char frm_mv_en; //add a frame level mv
	unsigned char frm_mv_size; //mv enable after x mb, x=2^(size+8)
	unsigned char glb_mv_en;  //global mv enable
	int glb_mvx;  //global mvx value
	int glb_mvy;  //global mvy value
	unsigned char me_step_en; //auto-modify max step number
	unsigned char me_step_0; //step number threshold 0
	unsigned char me_step_1; //step number threshold 1
	/*vmau scaling list*/
	unsigned char scaling_list[4][16];
	unsigned char scaling_list8[2][64];
	int deadzone[9];
	unsigned int acmask_mode;

	unsigned int intra_mode_msk;

	unsigned char is_scaling_custom;

	unsigned char i_4x4_dis;
	unsigned char i_8x8_dis;
	unsigned char i_16x16_dis;
	unsigned char p_l0_dis;
	unsigned char p_t8_dis;
	unsigned char p_skip_dis;
	unsigned char p_skip_pl0f_dis;
	unsigned char p_skip_pt8f_dis;

	unsigned char cost_bias_en;
	unsigned char cost_bias_i_4x4;
	unsigned char cost_bias_i_8x8;
	unsigned char cost_bias_i_16x16;
	unsigned char cost_bias_p_l0;
	unsigned char cost_bias_p_t8;
	unsigned char cost_bias_p_skip;

	unsigned char intra_lambda_y_bias_en;
	unsigned char intra_lambda_c_bias_en;
	unsigned char intra_lambda_bias_qp0;
	unsigned char intra_lambda_bias_qp1;
	unsigned char intra_lambda_bias_0;
	unsigned char intra_lambda_bias_1;
	unsigned char intra_lambda_bias_2;

	unsigned char chroma_sse_bias_en;
	unsigned char chroma_sse_bias_qp0;
	unsigned char chroma_sse_bias_qp1;
	unsigned char chroma_sse_bias_0;
	unsigned char chroma_sse_bias_1;
	unsigned char chroma_sse_bias_2;

	unsigned char sse_lambda_bias_en;
	unsigned char sse_lambda_bias;

	unsigned char fbc_ep;
	unsigned char jm_lambda2_en;
	unsigned char inter_nei_en;
	unsigned char skip_bias_en;

	unsigned char info_en;
	unsigned char mvd_sum_all;
	unsigned char mvd_sum_abs;
	unsigned char mv_sum_all;
	unsigned char mv_sum_abs;

	unsigned int ysse_thr;
	unsigned int csse_thr;

	unsigned char  cfg_size_x;
	unsigned char  cfg_size_y;
	unsigned short cfg_iw_thr;

	unsigned short cfg_mvr_thr1;
	unsigned int cfg_mvr_thr2;
	unsigned int cfg_mvr_thr3;

	unsigned char  dcm_en;
	unsigned int dcm_param;

	unsigned char  sde_prior;
	unsigned char  db_prior;
	/*ipred bit&lambda ctrl*/
	unsigned char  mb_mode_val;
	unsigned char  bit_16_en;
	unsigned char  bit_8_en;
	unsigned char  bit_4_en;
	unsigned char  bit_uv_en;
	unsigned char  lamb_16_en;
	unsigned char  lamb_8_en;
	unsigned char  lamb_4_en;
	unsigned char  lamb_uv_en;
	unsigned char  c_16_en;
	unsigned char  c_8_en;
	unsigned char  c_4_en;
	unsigned char  c_uv_en;
	unsigned char  pri_16;
	unsigned char  pri_8;
	unsigned char  pri_4;
	unsigned char  pri_uv;
	unsigned char  ref_neb_4;
	unsigned char  ref_neb_8;
	unsigned char  bit_16[4];
	unsigned char  bit_uv[4];
	unsigned char  bit_4[4];
	unsigned char  bit_8[4];
	unsigned char  lambda_info16;
	unsigned char  lambda_info8;
	unsigned char  lambda_info4;
	unsigned char  lambda_infouv;
	unsigned char  ref_4;
	unsigned char  ref_8;
	unsigned char  const_16[4];
	unsigned char  const_uv[4];
	unsigned char  const_4[4];
	unsigned char  const_8[4];

	/*loop filter*/
	unsigned char deblock;
	unsigned char rotate;
	char alpha_c0_offset;
	char beta_offset;

	/*cabac*/
	unsigned char *state;
	unsigned int bs;             /*BS output address*/
	unsigned char qp;
	unsigned char  bs_size_en;
	unsigned int bs_size;
	unsigned char               bsfull_intr_en;
	unsigned int              bsfull_intr_size;

	unsigned char skip_en;
	char cqp_offset;
	/*mode decision*/
	unsigned char use_intra_in_pframe;
	unsigned char use_fast_mvp;

	unsigned char mode_ctrl;
	unsigned int mode_ctrl_param[40];

	/*frame buffer address: all of the buffers should be 256byte aligned!*/
	unsigned int fb[3][2];       /*{curr, ref}{tile_y, tile_c}*/
	unsigned int raw[3];         /*{rawy, rawu, rawv} or {rawy, rawc, N/C}*/
	int stride[2];          /*{stride_y, stride_c}, only used in raster raw*/

	/* RAW plane format */
	unsigned char raw_format;

	unsigned char size_mode;
	unsigned char step_mode;
	/*descriptor address*/
	unsigned int *des_va, des_pa;
	unsigned int emc_bs_pa,emc_dblk_pa;
	unsigned int emc_bs_pa1;
	unsigned int emc_recon_pa,emc_mv_pa,emc_se_pa;
	unsigned int *emc_qpt_va, emc_qpt_pa;
	unsigned int *emc_rc_va, emc_rc_pa;
	unsigned int *emc_cpx_va, emc_cpx_pa;
	unsigned int *emc_mod_va, emc_mod_pa;
	unsigned int *emc_ncu_va, emc_ncu_pa;
	unsigned int *emc_sad_va, emc_sad_pa;
	unsigned int *emc_ai_mark_va, emc_ai_mark_pa;
	unsigned char* emc_cps_ref0;
	unsigned char* emc_cps_ref1;
	unsigned char* emc_cps_addr;
	unsigned char* emc_cpsc_addr;
	unsigned char* emc_sobel_addr;
	unsigned char* emc_mix_addr;
	unsigned char* emc_mixc_addr;
	unsigned char* emc_flag_addr;

	/* ROI info */
	unsigned char base_qp;
	unsigned char max_qp;
	unsigned char min_qp;

	unsigned char qp_tab_mode;
	unsigned char qp_tab_en;
	unsigned int qp_tab_len;
	unsigned int *qp_tab;
	unsigned char sas_en;
	unsigned char crp_en;
	unsigned char sas_mthd;
	unsigned short qpg_mb_thd[7];
	char qpg_mbqp_ofst[8];
	unsigned char qpg_flt_thd[5];
	unsigned char mbrc_qpg_sel; //whether use crp/sas qp offset or not, when MB rate control enable.

	/* rate ctrl */
	unsigned char rc_mb_en;
	unsigned char rc_bu_wait_en;// efe chn wait rc qp offset en
	unsigned char rc_mb_wait_en;// efe chn wait rc qp offset en
	unsigned char rc_bu_num;// total basic unit number
	unsigned short rc_bu_size;// mb number in a basic unit
	unsigned char rc_bu_level;// 1:1line 2:2line 3:4line 4:8line 5:16line 6:32line 7:64line
	unsigned char rc_mb_level;// 0:skip 1mb, 1:skip 2mb, 2:skip 4mb, 3:skip 8mb
	int *mb_ref_info;
	int *bu_ref_info;
	unsigned int rc_frm_tbs;
	unsigned int avg_bu_bs;
	unsigned short tar_bs_thd[6];
	char bu_alg0_qpo[6];
	char bu_alg1_qpo[6];
	char mb_cs_qpo[7];
	char mb_top_bs_qpo[2];
	char mb_rinfo_qpo[2];
	unsigned short mb_target_avg_bs[2];
	unsigned short mb_gp_num;
	unsigned short last_bu_size;
	unsigned char rc_bcfg_mode;

	/* rate ctrl hera */
	unsigned char 		rc_en;                  // hw ratecontrol enable
	int    		bu_size;                // lcu numbers in a bu
	int    		bu_len;                 // bu numbers in a frame
	unsigned char 		rc_method;              // rc alg sel
	unsigned short 		rc_thd[12];             // rc alg threshold
	unsigned int 		rc_info[128];           // bu's target bits
	char                bu_max_dqp;
	char                bu_min_dqp;
	unsigned char               rc_avg_len;
	unsigned char               rc_max_prop;
	unsigned char               rc_min_prop;

	unsigned char               rc_dly_en;
	unsigned char               rc_max_qp;
	unsigned char               rc_min_qp;

	//mosaic
	unsigned char mosaic_en;
	unsigned char mos_gthd[2];
	unsigned char mos_sthd[2];
	/* VPU Daisy Chain setting */
	unsigned char daisy_chain_en;
	unsigned char curr_thread_id;

	//odma,jrfc,jrfd
	unsigned char jrfcd_flag;
	unsigned char jrfc_enable;
	unsigned char jrfd_enable;
	unsigned int lm_head_total;
	unsigned int cm_head_total;
	unsigned int jh[3][2];       /*head addr {curr, ref0, ref1}{y, c}*/
	unsigned int spe_y_addr;
	unsigned int spe_c_addr;

	//eigen cfg
	unsigned char     mb_mode_use;
	unsigned int    *mb_mode_info;
	unsigned char     force_i16dc;//ipred
	unsigned char     force_i16;
	unsigned char     refresh_en;
	unsigned char     refresh_mode;
	unsigned char     refresh_bias;
	unsigned char     refresh_cplx_thd;
	unsigned char     cplx_thd_sel;
	unsigned char     diff_cplx_sel;
	unsigned char     diff_thd_sel;
	unsigned char     i16dc_cplx_thd;
	unsigned char     i16dc_qp_base;
	unsigned char     i16dc_qp_sel;
	unsigned char     i16_qp_base;
	unsigned char     i16_qp_sel;
	unsigned char     diff_cplx_thd;
	unsigned char     diff_qp_base[2];
	unsigned char     diff_qp_sel[2];
	unsigned char     cplx_thd_idx[24];
	unsigned char     cplx_thd[8];
	unsigned short    diff_thd_base[3];
	unsigned short    diff_thd_ofst[3];
	unsigned char     sas_eigen_en;
	unsigned char     crp_eigen_en;
	unsigned char     sas_eigen_dump;
	unsigned char     crp_eigen_dump;
	//ifa
	unsigned char     rrs_en;
	unsigned char     rrs_dump_en;
	unsigned char     rrs_uv_en;
	unsigned char     rrs_size_y; //0: 4, 1: 8, 2: 12, 3: 16
	unsigned char     rrs_size_c; //0: 4, 1: 8
	unsigned short    rrs_thrd_y; //threshold
	unsigned short    rrs_thrd_u;
	unsigned short    rrs_thrd_v;

	//265 efe_ifa transplant -> 264
	//add 265-ifa_qpg
	unsigned char  r_max_qp;
	unsigned char  r_min_qp;

	unsigned char  ifa_en;
	//smd
	unsigned char  smd_en;
	unsigned char  smd_c_en;
	//petbc
	int      crp_thrd;  //2 def
	unsigned char  petbc_en;
	unsigned char  pet_mode;
	unsigned char  pet_filter_valid[4];
	unsigned short petbc_var_thr[3];
	unsigned short petbc_ssm_thr[3][4];
	unsigned short petbc2_var_thr[3];
	unsigned short petbc2_ssm_thr[3][4];
	//rrs_en
	//unsigned char  rrs_en;
	unsigned char  efe_rrs_en;
	unsigned char  rrs_c_en;           //enable
	unsigned char  rrs_mode;           //0: raw-ref, 1: raw-raw
	unsigned char  rrs_of_en;          //overflow enable
	unsigned char  rrs_thrd;           //overflow threshold
	unsigned char  rrs_thrd_c;         //overflow threshold

	//cps
	unsigned char  cps_en;             //cps_en  & frame==P_SLICE
	unsigned char  cps_c_en;           //compress raw chroma enable
	unsigned char  cps_mode;           //down scale factor. 0: 2x2, 1: 4x4
	int      cps_size;           //cps_mode=0: 64, cps_mode=1: 16.

	//motion
	unsigned char  motion_en;          //motion enable
	unsigned char  motion_c_en;        //motion enable of chroma
	unsigned short motion_thrd;
	unsigned char  motion_of_thrd;     //count threshold
	unsigned char  motion_of_thrd_c;   //count threshold
	unsigned short motion_thrd_c;
	unsigned char  still_times_en;
	unsigned char  max_still_times;
	unsigned short frm_motion_thrd[4];
	unsigned short rrc_motion_thr[7];
	unsigned char  inter_en;
	//add refresh radix-ned
	unsigned char  ned_motion_shift;
	unsigned char  ned_motion_en;
	//sobel
	unsigned char  sobel_en;
	unsigned char  sobel_edge_thrd;     //edge pixel thrd
	unsigned char  ras_en;
	//energy
	unsigned char  ery_en;
	//skin
	unsigned char  skin_en;
	unsigned char  skin_lvl_en[3];
	unsigned char  skin_cnt_thrd;
	short  skin_level[4];
	unsigned char  skin_thrd[3][2][2]; //[sobel][u/v][max/min]
	unsigned char  skin_mul_factor[4];
	//qpg
	unsigned char  qpg_en;
	//  short  qpg_cplx_thrd[8];
	short  qpg_smd_cplx_thrd[8];
	short  qpg_sobel_cplx_thrd[8];
	unsigned char  qpg_filte_en; //use in qp_ofset filter
								 //x265 :ifa-qpg-en
	unsigned char  qpg_smd_en;
	unsigned char  qpg_ery_en;
	unsigned char  qpg_skin_en;
	unsigned char  qpg_sobel_en;
	unsigned char  qpg_petbc_en;
	unsigned char  qpg_pet_fil_en;//petbc filter enable
	unsigned char  qpg_pet_cplx_thrd[3][12];
	unsigned char  qpg_pet_qp_ofst[5][7];
	unsigned char  qpg_pet_qp_ofst_mt[5][7];
	unsigned char  qpg_dlt_thr[4];
	unsigned char  qpg_pet_qp_idx[5];
	char   qpg_cplx_qp_ofst[8];
	char   qpg_skin_qp_ofst[3];
	char   qpg_ai_motion_smd_ofst[3][2][8];

	unsigned char  qpg_table_en;
	unsigned char  qpg_roi_en;
	roi_info_t roi_info[16];
	int      emc_table_addr;

	//ifa wr-> ddr en
	unsigned char emc_cps_en;
	unsigned char emc_sobel_en;
	unsigned char emc_mix_en;
	unsigned char emc_cpsc_en;
	unsigned char emc_mixc_en;
	unsigned char emc_flag_en;

	//AI
	unsigned char ai_mark_en;
	unsigned char ai_mark_type;

	//skin
	unsigned char     skin_dt_en;
	unsigned char     skin_lvl;
	unsigned char     skin_cnt_thd;
	unsigned char     skin_pxlu_thd[3][2];
	unsigned char     skin_pxlv_thd[3][2];
	char      skin_qp_ofst[4];
	unsigned char     mult_factor[3];
	unsigned char     shift_factor[3][3];//[0][]:0.4, [1][]:0.6, [2][]: 5.1
	unsigned short    skin_ofst[4];//[0]: 1.5, [1]:0.4, [2]:0.6, [3]: 5.1
	unsigned char     ncu_mov_en;
	unsigned int    ncu_move_len;
	unsigned int    *ncu_move_info;
	//buf-share
	unsigned char     buf_share_en;
	unsigned char     buf_share_size;
	unsigned int    buf_start_yaddr;
	unsigned int    buf_beyond_yaddr;
	unsigned int    buf_start_caddr;
	unsigned int    buf_beyond_caddr;
	unsigned int    tmp_beyond_caddr;
	unsigned int    buf_ref_yaddr;
	unsigned int    buf_ref_caddr;
	unsigned int    buf_base_yaddr;
	unsigned int    buf_base_caddr;
	int         buf_ref_rem_mby;
	int         buf_spe_flag;
	int         buf_alg_flag;
	unsigned int    frame_idx;
	//ned
	unsigned char     ned_sad_thr[2][2][4];
	unsigned short    ned_sad_thr_cpx_step[2][3];
	unsigned char     c_sad_min_bias;
	unsigned char     recon_resi_en;

	unsigned char               use_dummy_byd;
	unsigned int              dummy_buf_beyond_yaddr;
	unsigned int              dummy_buf_beyond_caddr;

	unsigned char dump_api_en;
	unsigned int frm_num_gop;
};
/* vpu h264 vdma struct end */

struct h264_instance {
	struct h264_param param;
	struct h264_header_info header;
	struct h264_slice_info vdma_info;
};

int h264_param_init(h264_t *inst, struct h264_param *param);
void h264_exit(h264_t *inst);
int h264_encode(h264_t *inst, struct vpu_io_info *input);

#endif //_VPU_CORE_H_
