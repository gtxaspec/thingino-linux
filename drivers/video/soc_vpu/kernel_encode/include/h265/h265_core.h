#ifndef _H265_CORE_H_
#define _H265_CORE_H_

#include <linux/types.h>
#include "h265_slice.h"
#include "h265_nal.h"
#include "h265_bitstream.h"
#include "vpu_core.h"

//slice type
#define H265_SLICE_TYPE_B   0
#define H265_SLICE_TYPE_P   1
#define H265_SLICE_TYPE_IDR 2
#define H265_SLICE_TYPE_VI  6

typedef struct h265_instance h265_t;

struct h265_header_info {
	NALU nalu;
	Bitstream bs;
	NALList list;
};

struct h265_param {
	unsigned short width;
	unsigned short height;
	unsigned short y_stride;
	unsigned short c_stride;
	unsigned short i_qp;
	unsigned short p_qp;
	unsigned short gop;
	unsigned short fps;
	unsigned short fps_den;
};

/* vpu h265 vdma struct start */
typedef struct {
	struct {
		int ipred;
		int mce_size;
		int mce_ctrl;
		int blk;
		int tfm;
		int csd;
		int mos;
		int dlbda;
		int su;
		int fmd;
		int qpg;
		int csh;
	}func;
	struct {
		int tus;
		int srd;
		int csd;
		int mos;
		int ned;
		int csh;
		int dlbda;
		int mvp_force_zero;
		int pmdctrl;
		int mce;
		int ipred;
	}thrd;
}vpi_method_s;

typedef struct{
	unsigned char roi_en;
	unsigned char roi_md;
	char  roi_qp;
	unsigned char roi_lmbx;
	unsigned char roi_rmbx;
	unsigned char roi_umby;
	unsigned char roi_bmby;
} roi_s;

typedef struct {
	unsigned char *refy; //ref or cps-raw base address;
	unsigned char *ref1;//ref1 or cps-raw1 base address;

	unsigned char *cps_ref0; //ref or cps-raw base address;
	unsigned char *cps_ref1; //ref1 or cps-raw1 base address;

	int stride_refy;//ref y stride;

	//3x3 filter
	unsigned char filter_thrd[4];
	unsigned char rrs_filter_mode; //0: normal. 1: not change motion to static

	//cps
	unsigned char cps_en;  //compress raw enable
	unsigned char cps_c_en;           //compress raw chroma enable
	unsigned char cps_mode; //0: 2x2, 1: 4x4
	int cps_size;
	unsigned char cps1_copy_en;

	//rrs
	unsigned char rrs_en;  //enable
	unsigned char rrs_c_en;  //enable
	unsigned char rrs_mode;  //0: raw-raw, 1: raw-ref
	unsigned char rrs_of_en;  //overflow enable
	unsigned char rrs_thrd; //overflow threshold
	unsigned char motion_en;          //motion enable
	unsigned char motion_c_en;        //motion enable of chroma
	unsigned char motion_of_thrd; //count threshold
	unsigned short motion_thrd;
	unsigned short frm_motion_thrd[4]; //llzhang
	unsigned short rrc_motion_thr[7];
	unsigned char rrs_thrd_c; //overflow threshold
	unsigned char motion_of_thrd_c; //count threshold
	unsigned short motion_thrd_c;
	unsigned char still_times_en;
	unsigned char motion_level;
	unsigned char max_still_times;

	//raw-ref1
	unsigned char rrs1_en;   //raw-ref1 y enable
	unsigned char rrs1_c_en; //raw-ref1 c enable

	//sobel
	unsigned char sobel_en;
	unsigned char sobel_edge_thrd; //edge pixel thrd
	unsigned char sobel_16_or_14;//0:16x16 pixel 1:14x14 pixel
	unsigned char ras_en;

	//smd
	unsigned char smd_en;
	unsigned char smd_c_en;
	int     crp_thrd;

	//energy
	unsigned char ery_en;

	//skin
	unsigned char skin_en;
	unsigned char skin_lvl_en[3];
	unsigned char skin_cnt_thrd;
	short skin_level[4];
	unsigned char skin_thrd[3][2][2]; //[sobel][u/v][max/min]
	unsigned char skin_mul_factor[4];
	unsigned char *refc; //ref chroma  base address;
	unsigned char *ref1c;
	//refresh
	unsigned char refresh_en;
	unsigned char refresh_mode;
	unsigned char rfh_raw_thrd;
	unsigned char rfh_edge_thrd;
	int     rfh_flat_thrd;

	//ifa
	unsigned char ifa_en;
	unsigned char emc_out_en[2]; //0: cps only, 1: others
	unsigned char raw_avg_en;

	//qpg
	unsigned char qpg_en;
	unsigned char qpg_filte_en;
	unsigned char qpg_table_en;
	unsigned char qpg_roi_en;
	unsigned char qpg_cu32_qpm;  //0:avg, 1:first
	unsigned char qpg_skin_en;
	unsigned char qpg_ery_en;
	unsigned char qpg_sobel_en;
	unsigned char qpg_smd_en;
	char qpg_cplx_qp_ofst[8];
	short qpg_smd_cplx_thrd[8];
	short qpg_sobel_cplx_thrd[8];
	char qpg_skin_qp_ofst[3];
	char qpg_ai_motion_smd_ofst[3][2][8];
	char  pet_fil_en;
	unsigned char base_qp;
	unsigned char max_qp;
	unsigned char min_qp;
	unsigned int *qp_table;

	//pet
	unsigned char  petbc_en;
	unsigned char  pet_mode;
	unsigned char  pet_cnt_alg;
	unsigned char  qpg_petbc_en;
	unsigned char  qpg_mt_idx;
	//neighbour_cplx
	unsigned char  nei_cplx_en;
	//subblk_flat
	unsigned char  var_flat_en;
	unsigned char  var_flat_thr[2][2];
	/* unsigned char   var_flat_sub_size; */
	//cu_unifor
	unsigned char  cu_unifor_en;
	/* unsigned char  var_split_thrd;   */
	unsigned char  smd_blk16_bias[2];

	//cplx thrd new, llzhang
	unsigned short frm_cplx_thrd[2];//[0] smd, [1] sobel

	// frame level threshold for radix3.1 algorithm
	unsigned short smd_blk8_thr[6];      //[thr0~5]
	unsigned short smd_blk16_thr[8][2];  //[thr0~7][y,c]
	unsigned short smd_blk32_thr[3][2];  //[thr0~1][y,c]
	unsigned short sobel_blk16_thr[3];   //[sad,dif,cnt]
	unsigned short sobel_blk32_thr[3];   //[sad,dif,cnt]
	unsigned char qpg_pet_cplx_thrd[3][12];
	unsigned char  qpg_pet_qp_ofst[5][7];
	unsigned char  qpg_pet_qp_ofst_mt[5][7];
	unsigned char  qpg_dlt_thr[4];
	unsigned short petbc_var_thr[3];
	unsigned short petbc_ssm_thr[3][4];
	unsigned short petbc2_var_thr[3];
	unsigned short petbc2_ssm_thr[3][4];
	unsigned char  pet_filter_valid[4];
	unsigned char  qpg_pet_qp_idx[5];

	//AI
	unsigned char ai_mark_en;
	const char* ai_mark_input;
	unsigned char ai_scaling_factor;
	unsigned char ai_mark_type;
	unsigned char frame_type;
	unsigned char ai_prev_mark_en;
	unsigned char ai_collect_blk8;
} ifa_s;

typedef struct tfm_grp_t {
	unsigned char rdoq_level_y[3]; 	/* 4-bit uns, range 0 ~ 9. 0:off, suggest:5~7, [0] search, [1] merge, [2] intra */
	unsigned char rdoq_level_c[3]; 	/* 4-bit uns, range 0 ~ 9. 0:off, suggest:5~7, [0] search, [1] merge, [2] intra */
	unsigned char deadzone_y_en[3]; 	/* 1-bit uns, [0] search, [1] merge, [2] intra */
	short deadzone_y[3]; 	/* 13-bit tc, [0] search, [1] merge, [2] intra */
	unsigned char deadzone_c_en[3]; 	/* 1-bit uns, [0] search, [1] merge, [2] intra */
	short deadzone_c[3]; 	/* 13-bit tc, [0] search, [1] merge, [2] intra */
	unsigned short acmask_y[3]; 	/* 14-bit uns, [0] search, [1] merge, [2] intra */
	unsigned short acmask_c[3]; 	/* 14-bit uns, [0] search, [1] merge, [2] intra */
} tfm_grp_t;

typedef struct tfm_rdoq_param_t {
	unsigned char clear1; 		/* 1-bit uns, default 0 */
	unsigned char val0[4]; 		/* 2-bit uns */
	unsigned char val1[4]; 		/* 2-bit uns */
	unsigned char score[4]; 		/* 5-bit uns */
	unsigned char tu8_last[3]; 		/* 2-bit uns */
	unsigned char tu16_last[3]; 	/* 3-bit uns */
	unsigned char tu32_last[3]; 	/* 4-bit uns */
	unsigned char delta[4]; 		/* 2-bit uns */
} tfm_rdoq_param_t;

struct h265_vdma_tmp {
	unsigned int RADIX_CFGC_BASE_VDMA;
	unsigned int RADIX_VDMA_BASE_VDMA;
	unsigned int RADIX_ODMA_BASE_VDMA;
	unsigned int RADIX_TMC_BASE_VDMA;
	unsigned int RADIX_EFE_BASE_VDMA;
	unsigned int RADIX_JRFD_BASE_VDMA;
	unsigned int RADIX_MCE_BASE_VDMA;
	unsigned int RADIX_TFM_BASE_VDMA;
	unsigned int RADIX_MD_BASE_VDMA;
	unsigned int RADIX_DT_BASE_VDMA;
	unsigned int RADIX_DBLK_BASE_VDMA;
	unsigned int RADIX_SAO_BASE_VDMA;
	unsigned int RADIX_BC_BASE_VDMA;
	unsigned int RADIX_SDE_BASE_VDMA;
	unsigned int RADIX_IPRED_BASE_VDMA;
	unsigned int RADIX_STC_BASE_VDMA;
	unsigned int RADIX_EMC_BASE_VDMA;
	int max_lcux;
	int max_lcuy;
	int lcu_num_w;
	int lcu_num_h;
	int inter_cu8;
	int inter_cu16;
	int inter_cu32;
	int inter_cu64;
	int intra_cu8 ;
	int intra_cu16;
	int intra_cu32;
	int cu8_enable;
	int cu16_enable;
	int cu32_enable;
	int cu64_enable;
	int cu_enable;
	int inter_enable;
	int intra_enable;
	int qkcf_vector_last;
	int qkcf_vector_signcg;
	int qkcf_vector_sign;
	int qkcf_vector_m1;
	int max_bs_en;
	int max_bs_size;
	int ipmd_mpm_cost_biass32;
	int ipmd_mpm_cost_biasw32;
	int ipmd_mpm_cost_biass16;
	int ipmd_mpm_cost_biasw16;
	int ipmd_mpm_cost_biass8;
	int ipmd_mpm_cost_biasw8;
	int ipmd_angle_bias;
	int ipmd_dm_bias32;
	int ipmd_dm_bias16;
	int ipmd_dm_bias8;
	int intra_bits0;
	int intra_bits1;
	int intra_bits2;
	int intra_bits3;
	int intra_bits4;
	int intra_bits5;
	int intra_bits6;
	int intra_bits7;
	int intra_bits8;
	int intra_bits9;
	unsigned int tfm_cfg_grp[118];
	unsigned int lcu_width;
	unsigned int lcu_height;

	int pre_pmd_en;
	int pre_pmd_msk0;
	int pre_pmd_msk1;
	int pre_csd_en_cu8;
	int pre_csd_en_cu16;
	int pre_csd_sel_cu8;
	int pre_csd_sel_cu16;
	int pmd_bias_en_all;
	int pmd_bias_cu8;
	int pmd_bias_cu16;
	int pmd_bias_cu32;
	unsigned int md_cfg2;
	int slice_qp;
	int dqp_enable;
	int dqp_force_split;
	int dqp_depth_cu16;
	int csd_bias_en_all;
	int csd_bias_cu8   ;
	int csd_bias_cu16  ;
	int csd_bias_cu32  ;
	int csd_bias_cu64  ;
	unsigned int md_cfg3;
	int lambda_bias_en_all;
	int lambda_bias_sa8d;
	int lambda_bias_sse ;
	int lambda_bias_stc ;
	int enc_info_en;
	int enc_mv_all_en;
	int enc_mv_abs_en;
	unsigned int md_cfg4;
	int md_fskip_en;
	int md_fskip_max_size;
	unsigned int md_cfg5 ;
	unsigned int md_cfg6 ;
	unsigned int md_cfg7 ;
	unsigned int md_scfg[4];
	unsigned int md_pmd_cfg_ofst;
	unsigned int md_lambda_cfg_ofst;
	unsigned int md_rcfg[8];
	unsigned int md_pmon_cfg_ofst;
	int pmon_mode; // 0:mpt_empty;  1:ipt_rdy&mpt_empty;  2:ipt_empty
	unsigned int md_pmon_cfg0; // cu8,cu16,cu32
	/* cfg: | 12bit | 10bit | 10bit | */
	unsigned int md_pmon_cfg1;
	unsigned int md_pmon_cfg2;
	unsigned int md_pmon_cfg3;
	unsigned int md_pmon_cfg4;
	unsigned int md_pmon_cfg5;
	unsigned int md_pmon_cfg6;
	unsigned int ppmd_cost_thr;
	unsigned int ppmd_qp_thrd_0;
	unsigned int ppmd_qp_thrd_1;
	//pmd_thr_array
	unsigned int pmd_thr0;
	unsigned int pmd_thr1;
	unsigned int pmd_thr2;
	unsigned int pmd_thr3;
	unsigned int pmd_thr4;
	unsigned int pmd_thr5;
	unsigned int pmd_thr6;
	unsigned int pmd_thr7;
	unsigned int pmd_thr8;
	unsigned int pmd_thr9;
	unsigned int pmd_thr10;
	unsigned int pmd_thr11;
	unsigned int pmd_thr12;
	unsigned int pmd_thr13;
	unsigned int pmd_thr14;
	unsigned int pmd_thr15;
	unsigned int pmd_thr16;
	unsigned int pmd_thr17;
	unsigned int pmd_thr18;
	unsigned int pmd_thr19;
	unsigned int pmd_thr20;
	unsigned int pmd_thr21;
	unsigned int pmd_thr22;
	unsigned int pmd_thr23;
	unsigned int pmd_thr24;
	unsigned int pmd_thr25;
	unsigned int pmd_thr26;
	unsigned int pmd_thr27;
	unsigned int pmd_thr28;
	unsigned int pmd_thr29;
	//search pmd_thr_idx
	unsigned int cu8_pmd_s_thr_idx0;
	unsigned int cu8_pmd_s_thr_idx1;
	unsigned int cu8_pmd_s_thr_idx2;
	unsigned int cu8_pmd_s_thr_idx3;
	unsigned int cu8_pmd_s_thr_idx4;
	unsigned int cu8_pmd_s_thr_idx5;
	unsigned int pmd_thr_idx0;
	unsigned int cu8_pmd_s_thr_idx6;
	unsigned int cu8_pmd_s_thr_idx7;
	unsigned int cu8_pmd_s_thr_idx8;
	unsigned int cu8_pmd_s_thr_idx9;
	unsigned int cu16_pmd_s_thr_idx10;
	unsigned int cu16_pmd_s_thr_idx11;
	unsigned int pmd_thr_idx1;
	unsigned int cu16_pmd_s_thr_idx12;
	unsigned int cu16_pmd_s_thr_idx13;
	unsigned int cu16_pmd_s_thr_idx14;
	unsigned int cu16_pmd_s_thr_idx15;
	unsigned int cu16_pmd_s_thr_idx16;
	unsigned int cu16_pmd_s_thr_idx17;
	unsigned int pmd_thr_idx2;
	unsigned int cu16_pmd_s_thr_idx18;
	unsigned int cu16_pmd_s_thr_idx19;
	unsigned int cu32_pmd_s_thr_idx20;
	unsigned int cu32_pmd_s_thr_idx21;
	unsigned int cu32_pmd_s_thr_idx22;
	unsigned int cu32_pmd_s_thr_idx23;
	unsigned int pmd_thr_idx3;
	unsigned int cu32_pmd_s_thr_idx24;
	unsigned int cu32_pmd_s_thr_idx25;
	unsigned int cu32_pmd_s_thr_idx26;
	unsigned int cu32_pmd_s_thr_idx27;
	unsigned int cu32_pmd_s_thr_idx28;
	unsigned int cu32_pmd_s_thr_idx29;
	unsigned int pmd_thr_idx4;
	//mrege pmd_thr_idx
	unsigned int cu8_pmd_m_thr_idx0;
	unsigned int cu8_pmd_m_thr_idx1;
	unsigned int cu8_pmd_m_thr_idx2;
	unsigned int cu8_pmd_m_thr_idx3;
	unsigned int cu8_pmd_m_thr_idx4;
	unsigned int cu8_pmd_m_thr_idx5;
	unsigned int pmd_thr_idx5;

	unsigned int cu8_pmd_m_thr_idx6;
	unsigned int cu8_pmd_m_thr_idx7;
	unsigned int cu8_pmd_m_thr_idx8;
	unsigned int cu8_pmd_m_thr_idx9;
	unsigned int cu16_pmd_m_thr_idx10;
	unsigned int cu16_pmd_m_thr_idx11;
	unsigned int pmd_thr_idx6;

	unsigned int cu16_pmd_m_thr_idx12;
	unsigned int cu16_pmd_m_thr_idx13;
	unsigned int cu16_pmd_m_thr_idx14;
	unsigned int cu16_pmd_m_thr_idx15;
	unsigned int cu16_pmd_m_thr_idx16;
	unsigned int cu16_pmd_m_thr_idx17;
	unsigned int pmd_thr_idx7;

	unsigned int cu16_pmd_m_thr_idx18;
	unsigned int cu16_pmd_m_thr_idx19;
	unsigned int cu32_pmd_m_thr_idx20;
	unsigned int cu32_pmd_m_thr_idx21;
	unsigned int cu32_pmd_m_thr_idx22;
	unsigned int cu32_pmd_m_thr_idx23;
	unsigned int pmd_thr_idx8;

	unsigned int cu32_pmd_m_thr_idx24;
	unsigned int cu32_pmd_m_thr_idx25;
	unsigned int cu32_pmd_m_thr_idx26;
	unsigned int cu32_pmd_m_thr_idx27;
	unsigned int cu32_pmd_m_thr_idx28;
	unsigned int cu32_pmd_m_thr_idx29;
	unsigned int pmd_thr_idx9;

	unsigned int cu64_pmd_m_thr_idx30;
	unsigned int cu64_pmd_m_thr_idx31;
	unsigned int cu64_pmd_m_thr_idx32;
	unsigned int cu64_pmd_m_thr_idx33;
	unsigned int cu64_pmd_m_thr_idx34;
	unsigned int cu64_pmd_m_thr_idx35;
	unsigned int pmd_thr_idx10;

	unsigned int cu64_pmd_m_thr_idx36;
	unsigned int cu64_pmd_m_thr_idx37;
	unsigned int cu64_pmd_m_thr_idx38;
	unsigned int cu64_pmd_m_thr_idx39;
	//merge search pmd_thr_idx
	unsigned int cu8_pmd_m_s_thr_idx0;
	unsigned int cu8_pmd_m_s_thr_idx1;
	unsigned int pmd_thr_idx11;

	unsigned int cu8_pmd_m_s_thr_idx2;
	unsigned int cu8_pmd_m_s_thr_idx3;
	unsigned int cu8_pmd_m_s_thr_idx4;
	unsigned int cu8_pmd_m_s_thr_idx5;
	unsigned int cu8_pmd_m_s_thr_idx6;
	unsigned int cu8_pmd_m_s_thr_idx7;
	unsigned int pmd_thr_idx12;

	unsigned int cu8_pmd_m_s_thr_idx8;
	unsigned int cu8_pmd_m_s_thr_idx9;
	unsigned int cu16_pmd_m_s_thr_idx10;
	unsigned int cu16_pmd_m_s_thr_idx11;
	unsigned int cu16_pmd_m_s_thr_idx12;
	unsigned int cu16_pmd_m_s_thr_idx13;
	unsigned int pmd_thr_idx13;

	unsigned int cu16_pmd_m_s_thr_idx14;
	unsigned int cu16_pmd_m_s_thr_idx15;
	unsigned int cu16_pmd_m_s_thr_idx16;
	unsigned int cu16_pmd_m_s_thr_idx17;
	unsigned int cu16_pmd_m_s_thr_idx18;
	unsigned int cu16_pmd_m_s_thr_idx19;
	unsigned int pmd_thr_idx14;

	unsigned int cu32_pmd_m_s_thr_idx20;
	unsigned int cu32_pmd_m_s_thr_idx21;
	unsigned int cu32_pmd_m_s_thr_idx22;
	unsigned int cu32_pmd_m_s_thr_idx23;
	unsigned int cu32_pmd_m_s_thr_idx24;
	unsigned int cu32_pmd_m_s_thr_idx25;
	unsigned int pmd_thr_idx15;

	unsigned int cu32_pmd_m_s_thr_idx26;
	unsigned int cu32_pmd_m_s_thr_idx27;
	unsigned int cu32_pmd_m_s_thr_idx28;
	unsigned int cu32_pmd_m_s_thr_idx29;
	//intra pmd_thr_idx
	unsigned int cu8_pmd_intra_thr_idx0;
	unsigned int cu8_pmd_intra_thr_idx1;
	unsigned int pmd_thr_idx16;

	unsigned int cu8_pmd_intra_thr_idx2;
	unsigned int cu8_pmd_intra_thr_idx3;
	unsigned int cu8_pmd_intra_thr_idx4;
	unsigned int cu8_pmd_intra_thr_idx5;
	unsigned int cu8_pmd_intra_thr_idx6;
	unsigned int cu8_pmd_intra_thr_idx7;
	unsigned int pmd_thr_idx17;

	unsigned int cu8_pmd_intra_thr_idx8;
	unsigned int cu8_pmd_intra_thr_idx9;
	unsigned int cu16_pmd_intra_thr_idx10;
	unsigned int cu16_pmd_intra_thr_idx11;
	unsigned int cu16_pmd_intra_thr_idx12;
	unsigned int cu16_pmd_intra_thr_idx13;
	unsigned int pmd_thr_idx18;

	unsigned int cu16_pmd_intra_thr_idx14;
	unsigned int cu16_pmd_intra_thr_idx15;
	unsigned int cu16_pmd_intra_thr_idx16;
	unsigned int cu16_pmd_intra_thr_idx17;
	unsigned int cu16_pmd_intra_thr_idx18;
	unsigned int cu16_pmd_intra_thr_idx19;
	unsigned int pmd_thr_idx19;

	unsigned int cu32_pmd_intra_thr_idx20;
	unsigned int cu32_pmd_intra_thr_idx21;
	unsigned int cu32_pmd_intra_thr_idx22;
	unsigned int cu32_pmd_intra_thr_idx23;
	unsigned int cu32_pmd_intra_thr_idx24;
	unsigned int cu32_pmd_intra_thr_idx25;
	unsigned int pmd_thr_idx20;

	unsigned int cu32_pmd_intra_thr_idx26;
	unsigned int cu32_pmd_intra_thr_idx27;
	unsigned int cu32_pmd_intra_thr_idx28;
	unsigned int cu32_pmd_intra_thr_idx29;
	unsigned int pmd_thr_idx21;
	unsigned int md_ned_score_tbl0;
	unsigned int md_ned_score_tbl1;
	unsigned int md_ned_score_tbl2;
	unsigned int md_ned_score_tbl3;
	unsigned int md_ned_score_tbl4;
	unsigned int md_ned_disable0;
	unsigned int md_ned_disable1;
	unsigned int md_ned_disable2;
	unsigned int mosaic_cfg_0;
	unsigned int md_rcn_plain_cfg0;
	unsigned int md_rcn_plain_cfg1;
	unsigned int csd_qp_thrd_0;
	int abs_dlambda_simple_dqp;
	int abs_dlambda_edge_dqp;
	unsigned int md_dlambda_cfg0;
	unsigned int md_dlambda_cfg1;
	unsigned int md_css_cfg0;
	unsigned int md_css_cfg1;
	unsigned int md_css_cfg2;
	unsigned int md_css_cfg3;
	int hevc_mb_size;
	int svac_mb_size;
	int mb_size;
	int hevc_filter_param;
	int svac_filter_param;
	int svac_filter_param_p;
	int align_height;
	int fm_h_is_align8;
	int nv12_flag;
	unsigned int odma_y_addr;
	unsigned int odma_c_addr;
	unsigned int odma_jrfc_head_y_addr;
	unsigned int odma_jrfc_head_c_addr;
	unsigned int odma_jrfc_head_sp_y_addr;
	unsigned int odma_jrfc_head_sp_c_addr;
	unsigned int odma_bfsh_base_y_addr;
	unsigned int odma_bfsh_base_c_addr;
	unsigned int odma_bfsh_bynd_y_addr;
	unsigned int odma_bfsh_bynd_c_addr;
	unsigned int jrfd_y_addr;
	unsigned int jrfd_c_addr;
	unsigned int jrfd_my_addr;
	unsigned int jrfd_mc_addr;
	unsigned int buf_beyond_yaddr;
	unsigned int buf_beyond_caddr;
	unsigned int esti_ctrl;
	unsigned int merg_info;
	int mce_comp_ctap;
	int common_slv_cfg3;
	int spe_slv_cfg3;
	int stc_slv_cfg3;
	int emc_cfg_en;
	int dbg_emc_flag_ram_addr;
	int dbg_emc_flag_data_high;
	int dbg_cfg_info;
	unsigned char cu64_en;
	unsigned char intra_en;
	unsigned char pp_en;
	unsigned char efe_mode;
	unsigned char inter_en;
	unsigned int efe_extra_cfg0;
	int efe_ctrl;
	int rrs_cfg;
	int rfh_cfg0;
	int rfh_cfg1;
	int qpg_cfg;
	int qpg_cfg1;
	int cplx_qp0;
	int cplx_qp1;
	int pet_cplx_thrd00;
	int pet_cplx_thrd01;
	int pet_cplx_thrd02;
	int pet_cplx_thrd10;
	int pet_cplx_thrd11;
	int pet_cplx_thrd12;
	int pet_cplx_thrd20;
	int pet_cplx_thrd21;
	int pet_cplx_thrd22;
	int pet_qp_ofst0;
	int pet_qp_ofst1;
	int pet_qp_ofst2;
	int pet_qp_ofst3;
	int pet_qp_ofst4;
	int pet_qp_ofst_mt0;
	int pet_qp_ofst_mt1;
	int pet_qp_ofst_mt2;
	int pet_qp_ofst_mt3;
	int pet_qp_ofst_mt4;
	int qpg_dlt_thrd;
	int petbc_var_thr;
	int petbc_ssm_thr0;
	int petbc_ssm_thr1;
	int petbc_ssm_thr2;
	int petbc2_var_thr;
	int petbc2_ssm_thr0;
	int petbc2_ssm_thr1;
	int petbc2_ssm_thr2;
	int petbc_filter_valid;
	int ai_motion_smd_ofst0;
	int ai_motion_smd_ofst1;
	int ai_motion_smd_ofst2;
	int ai_motion_smd_ofst3;
	int ai_motion_smd_ofst4;
	int ai_motion_smd_ofst5;
	int ai_motion_smd_ofst6;
	int ai_motion_smd_ofst7;
	int ai_motion_smd_ofst8;
	int ai_motion_smd_ofst9;
	int smd_cplx_thrd0;
	int smd_cplx_thrd1;
	int smd_cplx_thrd2;
	int smd_cplx_thrd3;
	int sobel_cplx_thrd0;
	int sobel_cplx_thrd1;
	int sobel_cplx_thrd2;
	int sobel_cplx_thrd3;
	int ifa_cfg;
	int ifa_cfg1;
	int ifac_all_en;
	int ifac_all1_en;
	int skin_thrd0;
	int skin_thrd1;
	int skin_thrd2;
	int skin_lvl0;
	int skin_lvl1;
	int skin_factor;
	int roi_ctrl_cfg[4];
	int roi_pos_cfg[16];
	int qpg_roi_area_en;
	int frm_cplx_thrd;
	int frm_motion_thrd0;
	int frm_motion_thrd1;
	int rrc_motion_thr0;
	int rrc_motion_thr1;
	int rrc_motion_thr2;
	int rrc_motion_thr3;
	int frm_flat_unifor_thrd;
	int pet_qp_idx;
	int frmb_ip_param0;
	int frmb_ip_param1;
	int frmb_ip_param2;
	int frmb_ip_param3;
};

struct h265_slice_info {
	unsigned int*		des_va;			// vdma descript chain virtual address
	unsigned int		des_pa;			// vdma descript chain physical address
	unsigned int 		raw_y_pa;		// raw Y base physical address
	unsigned int 		raw_c_pa;		// raw C base physical address
	unsigned int 		dst_y_pa;		// rebuild frame Y physical address
	unsigned int 		dst_c_pa;		// rebuild frame C physical address
	unsigned int 		ref_y_pa;		// reference frame Y physical address
	unsigned int 		ref_c_pa;		// reference frame C physical address
	unsigned int 		mref_y_pa;		// reference frame 1 Y physical address
	unsigned int 		mref_c_pa;		// reference frame 1 C physical address
	unsigned int 		bitstream_pa;		// bitstream physical address
	unsigned char 		frame_type;		// 0: B, 1: P, 2: I
	unsigned int              frm_num_gop;
	unsigned char 		raw_format;		// 0: NV12, 1: NV21
	unsigned char               video_type;             // 0: HEVC, 1: SVAC2
	unsigned char               is_decode;              // 0: ENC, 1: DEC
	unsigned char               work_mode;              // 0: normal encode/decode, 1: pre-analyse,
	unsigned short 		frame_width;		// pixel width of frame
	unsigned short 		frame_height;		// pixel height of frame
	unsigned short 		efe_hw_width;		// pixel width of frame
	unsigned short 		efe_hw_height;		// pixel height of frame
	unsigned short		frame_y_stride;		// frame Y stride
	unsigned short 		frame_c_stride;		// frame UV stride
	unsigned short 		dst_y_stride;		// rebuild frame Y stride, default equal to frame_width
	unsigned short 		dst_c_stride;		// rebuild frame C stride, default equal to frame_width
	unsigned short 		ref_y_stride;		// reference frame Y stride, default equal to frame_width
	unsigned short 		ref_c_stride;		// reference frame C stride, default equal to frame_width
	unsigned char 		frame_qp;		// frame start qp;
	unsigned char               frame_qp_svac;          // frame start qp for svac;
	char 		frame_cqp_offset;	// chroma QP offset, -12 ~ 12
	unsigned char 		tu_split_en;		// (inter & !2nx2n) tu split enable
	unsigned char               tu_inter_depth;         // for inter tu split, 1 or 2
	unsigned char 		time_out_threshold;	// 0-3, time out threshold value

	/* efe parameters */
	unsigned char               chng_en;
	unsigned char               mce_cfg_en;
	unsigned char               md_cfg_en;
	unsigned char               ip_cfg_en;
	unsigned char               gray_en;
	unsigned char               rotate_mode;
	unsigned char 		efe_mode;		// default value is 0;
	roi_s 		roi_info[16];
	unsigned short 		qp_table_len;		// QP table len, not bigger then 1024
	unsigned int*		qp_table;		// QP table virtual address, only for vdma chain.
	ifa_s                 ifa_para;

	unsigned char 		qpg_max_qp;		// suggest set frame_qp + 13, but not bigger then 51
	unsigned char 		qpg_min_qp;		// suggest set frame_qp - 12, but not less then 0
	unsigned char 		qp_mode;		// 0 - all close, 1 - crp, 2 - sas, 3 - msas, 4 - tab+crp, 5 - tab+sas, 6 - tab+msas
	unsigned char 		qp_table_cu32_mode;	// CU32 QP select, 0 - first CU16 QP, 1 - average CU16 QP.
	unsigned short 		crp_cu16_threshold[7];
	unsigned short 		crp_cu32_threshold[7];
	char 		crp_cu16_offset[8];
	char 		crp_cu32_offset[8];
	unsigned char 		crp_filter_limit_threshold;
	unsigned char 		crp_filter_top_threshold;
	unsigned char 		crp_filter_left_threshold;
	unsigned char 		crp_filter_right_threshold;
	unsigned short 		sas_cu16_threshold[6];
	unsigned short 		sas_cu32_threshold[6];
	char 		sas_cu16_offset[7];
	char 		sas_cu32_offset[7];
	// * ipmd *******
	unsigned char       i_pmd_en; //mfchen added
	unsigned char       ipmd_mpm_use_maxnum;
	unsigned char       ipmd_mpm_cost_bias[3][2][4];
	unsigned char       ipmd_dm_cost_bias[3][5];
	unsigned char       ppmd_cost_thr[4];
	unsigned char       ppmd_merge_thr[2][4][5][2];
	unsigned char       ppmd_search_thr[2][3][5][2];
	unsigned char       ppmd_merge_search_thr[2][3][5][2];
	unsigned char       ppmd_intra_thr[2][3][5][2];
	unsigned char       pmd_qp_thrd[7];
	unsigned char       ppmd_thr_list[30][2];
	unsigned char       ppmd_merge_thr_idx[40];
	unsigned char       ppmd_search_thr_idx[30];
	unsigned char       ppmd_merge_search_thr_idx[30];
	unsigned char       ppmd_intra_thr_idx[30];
	unsigned char     motion_flag;
	unsigned char     petbc;
	unsigned char     depth;
	/* emc */
	unsigned char               bsfull_intr_en;
	unsigned int              bsfull_intr_size;
	unsigned char               emc_bs_en;
	unsigned char               emc_cps_en;
	unsigned char               emc_cpsc_en;
	unsigned char               emc_sobel_en;
	unsigned char               emc_mix_en;
	unsigned char               emc_mixc_en;
	unsigned char               emc_mvo_en; //md write out curr-mv
	unsigned char               emc_mvi_en; //mce read ref-mv
	unsigned char               emc_bslen_en;
	unsigned char               emc_flag_en;
	unsigned char               emc_mdc_en;
	unsigned char               emc_ipc_en;
	unsigned char               emc_qpt_en;
	unsigned char               emc_mcec_en;
	unsigned char               emc_ai_en;
	unsigned char*              emc_bs_addr0;
	unsigned char*              emc_bs_addr1;
	unsigned char*              emc_cps_addr;
	unsigned char*              emc_cpsc_addr;
	unsigned char*              emc_sobel_addr;
	unsigned char*              emc_mix_addr;
	unsigned char*              emc_mixc_addr;
	unsigned char*              emc_mvo_addr; // md write cur mv
	unsigned char*              emc_mvi_addr; // mce read ref mv
	unsigned char*              emc_bslen_addr;
	unsigned char*              emc_flag_addr;
	unsigned char*              emc_mdc_addr;
	unsigned char*              emc_ipc_addr;
	unsigned char*              emc_qpt_addr;
	unsigned char*              emc_mcec_addr;
	unsigned char*              emc_ai_addr;

	/* dblk parameters */
	unsigned char 		dblk_en;		// filter is enable
	unsigned char 		sao_en;			// sao is enable
	unsigned char 		sao_y_flag;		// sao y is enable
	unsigned char 		sao_c_flag;		// sao c is enable
	unsigned char               stc_cfg_mode;           //0 is common, 1 is cfg a mode for a lcu0, others merge. 2 is merge mode, lcu 0 calc real param, other lcu merge , not insist now; if open sao, defalt 1
	unsigned char               stc_fixed_merge;        //2bits. {left, up}
unsigned char               stc_fixed_mode;         //4bits. 0~3 is E0, 4 is BO, 8 is NO-SAO
unsigned char               stc_fixed_subtype;      //5bits. 0~28 useful when BO.
char                stc_fixed_offset0;      //4bits. if bo,-7~7; [3] is sign bit. if eo, 0~6...(ori svac is -1~6)
char                stc_fixed_offset1;      //4bits. if bo,-7~7; [3] is sign bit. if eo, 0~1...
char                stc_fixed_offset2;      //4bits. if bo -7~7; [3] is sign bit. if eo, -1~0...
char                stc_fixed_offset3;      //4bits. if bo -7~7; [3] is sign bit. if eo, -6~0...(ori svac is -6~1)
unsigned char 		dblk_gray_en;		// only Y, no UV
char 		beta_offset_div2;	// -6 ~ +6
char 		tc_offset_div2;		// -6 ~ +6
unsigned char               filter_level;      //svac2. dblk open when != 0.
unsigned char               mb_lim;            //svac2. thresh 0
unsigned char               lim;               //svac2. thresh 1
unsigned char               hev_thr;           //svac2. thresh 2
unsigned char               mb_lim_p;          //svac2. thresh 0 for inter
unsigned char               lim_p;             //svac2. thresh 1 for inter
unsigned char               hev_thr_p;         //svac2. thresh 2 for inter
/* bc & sde */
unsigned char 		use_dqp_flag;		// code dqp enable
unsigned char 		dqp_max_depth;		// code dqp max depth
unsigned char 		sign_hide_flag;		// coeff sign hidden enable
unsigned char 		merge_cand_num;		// mvp cand number
unsigned char 		context_type; 		// one kind of frame type, used for context model init
unsigned char 		use_hp;                 //code mvd use hp, svac2
unsigned char 		interp_filter;          //default0,svac2
/* tfm parameter*/
unsigned char 		tfm_buf_en;		// DO NOT TOUCH: legacy of T30 // bit0:Y8; bit1:Y16; bit2:Y32; bit3:Y64; bit4:C
unsigned char 		sse_mask;               // DO NOT TOUCH: legacy of T30
unsigned short              tfm_path_en[2];         // [0]: cfg of HEVC, [1]: cfg of SVAC2
													// note of tfm_path_en[2]:
													// cfg of HEVC: [0] means the lsb:
													// [9] CU64  merge, [8] CU32  intra,
													// [7] CU32  merge, [6] CU32 search, [5] CU16  intra, [4] CU16  merge,
													// [3] CU16 search, [2]  CU8  intra, [1]  CU8  merge, [0]  CU8 search
													// cfg of SVAC2: [0] means the lsb: [3] CU64, [2] CU32, [1] CU16, [0] CU8
unsigned char               scaling_list_en;        // 0: do not use scalinglist, 1: use scalinglist
unsigned char               scaling_list_present;   // only 0 is supported
unsigned char               do_not_clear_deadzone;  // 0: default, 1: do not clear deadzone when the result is opposite
unsigned char               deadzone_en;
unsigned char               rdoq_en;
unsigned char               acmask_en;
unsigned char               acmask_type;            // 0: default, 1: diagonal type
struct tfm_grp_t      tfm_grp[16];
struct tfm_rdoq_param_t tfm_rdoq_param[9];    // 9 rdoq param sets according to rdoq level 1~9. rdoq level 0 means turning off and does not need rdoq param.
/* mce ctrl */
unsigned char 		inter_mode[4][4]; 	// [depth]: {merge, 2NxN, Nx2N, 2Nx2N}
unsigned char 		half_pixel_sech_en;
unsigned char 		quart_pixel_sech_en;
unsigned char       max_sech_step;     // range 0~63, max_sech_step
unsigned char 		refine_cu8_mode;
unsigned char 		refine_cu16_mode;
unsigned char 		refine_cu32_mode;
unsigned char 		mce_scl_mode;
unsigned char 		mref_en;
unsigned char 		mref_wei;
unsigned char               mrg_en;            // range 0~1, cu8_mrg1_en
unsigned char               mrg_fpel_en;       // range 0~1, cu8_mrg1_en
short               mce_slc_mvx;
short               mce_slc_mvy;
short               max_mvrx;
short               max_mvry;
unsigned char               step_adapt_en;
unsigned char               step_thrd_0;
unsigned char               step_thrd_1;
unsigned short              mce_resi_thr[2][3][6];   //[y,uv][cu16, cu32, cu64][sad, max, avg]
unsigned short              intra_resi_thr[2][3][6]; //[y,uv][cu8,  cu16, cu32][sad, max, avg]
unsigned short              recon_resi_thr[2][4][2]; //[shadow,dot][cu8, cu16, cu32, cu64][y,uv]
unsigned short              ifa_raw_thr[8][2];       //[tus intra, tus inter, resi recon][y,uv]
unsigned char               intra_resi_en[3];        //[cu8, cu16, cu32]
unsigned char               recon_resi_en[4][2];     //[cu8, cu16, cu32, cu64][shadow,dot]
unsigned char               maxnum_ref;
/* intra mode */
unsigned char               ipred_usis;             // 1:ipred cu32 use strong intra stooming
unsigned char 		ipred_size;		// 7:all 3 channel open 3bit
unsigned char 		sobel_size;		// 0:all 3 channel open 3bit
unsigned char 		sobel_en;		// 0:close sobel 1:open sobel
unsigned char 		md_bc_close;		// 1:close sobel 0:open sobel
unsigned char 		ppred_slv_dif;		// 0:ppred use ipred_n for ctrl 1:ppred use separate ipx_n for ctrl
unsigned char 		ppred_mode_num;		// 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
unsigned char 		ipred_slv_en;		// ipred use slave mode 1bit
unsigned char 		ppred_slv_en;		// ppred use slave mode only need ppred_n==1  1bit
unsigned char 		intra_mode[5]; 		// mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
unsigned char 		ppred8_mode_num;	// 8x8 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
unsigned char 		ppred8_slv_en;		// ppred 8x8 use slave mode only need ppred_n==1  1bit
unsigned char 		intra8_mode[5]; 	// mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
unsigned char 		ppred16_mode_num;	// 16x16 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
unsigned char 		ppred16_slv_en;		// ppred 16x16 use slave mode only need ppred_n==1  1bit
unsigned char 		intra16_mode[5]; 	// mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
unsigned char 		ppred32_mode_num;	// 32x32 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
unsigned char 		ppred32_slv_en;		// ppred 32x32 use slave mode only need ppred_n==1  1bit
unsigned char 		intra32_mode[5]; 	// mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
unsigned char               intra_angle_bias[4];    //[enable, cu16, cu32]
unsigned char               frm_ipmd_angle_en;      //0: angle strong bias for all mode, 1: angle strong bias for angle mode
unsigned char               intra_bits[16][5];      //16*5*4bits(0~15)
unsigned char               frmb_idx[5];            // range 0~15, y_bits,y,y,uv_bits,uv; default: 1 2 5 0 4
unsigned char		frmb_ip_bits_en[5];	// range 0~1 , y_bits_en
unsigned char		frmb_ip_bias[5][2];	// range 0~30, sad-cost bias; [1]dc,[0]planar
unsigned char		frmb_ipc_bias[5];       // range 0~30, sad-cost bias; dm
unsigned char       i_intra_pu4_en;           // range 0~1, pu4 enable
unsigned char       p_intra_pu4_en;
/* md ctrl */
unsigned char 		md_mvs_all;		// value 0xF means all cus' mv sum, value 0 means sum of mv after mode decision
unsigned char 		md_mvs_abs;		// value 0xF means mv abs sum, value 0 means sum of mv algebraic sum
unsigned char 		md_pre_pmd[2];		// [0]:low msk, [1]: high msk
unsigned char 		md_pre_csd[2];		// [0]:cu8, [1]:cu16. value 0:3sse, 1:4sa8d, 2:auto
unsigned char 		md_force_split;		// force cu split if qps inside are not the same
unsigned char 		md_pmd_bias[3][2];	// [3]:cu8~32        [2]:enable,value
unsigned char 		md_csd_bias[4][2];	// [4]:cu8~642       [2]:enable,value
unsigned char 		md_lambda_bias[3][2];	// [3]:sa8d,sse,stc. [2]:enable,value
unsigned char 		md_fskip_en; 		// force skip(merge) mode enable
unsigned char 		md_fskip_cfg;		// force skip(merge) mode configure. 0:reserved, 1:cu64, other:reserved
unsigned char 		md_fskip_val[8][16];	// [lcuy>>1][lcux>>1]. max 2kx1k
unsigned char               uv_dist_scale_en;       //
unsigned char               uv_dist_scale[3];       // [0]:bias [1]:shift [2]direction
unsigned char               pmdctrl_en;             // frame level pred mode decision control enable. value 0: use default sse-cost. value 1:use sa8d/sad at some mode
unsigned char               pmdctrl_mode;           // pred mode decision control mode. 0: merge/search sa8d/sad, inter/intra sse. vaule 1: merge/search/intra sa8d/sad.
unsigned char               blk_pmdctrl_en;         // block level pred mode decision control enable
unsigned char               pmdsu_en;               // pmd speed up enable for cu32/cu16/cu8
unsigned char               pmdsu_thrd[3];          // pmd speed up threshold
unsigned char               tus_ifa_intra32_en;
unsigned char               tus_ifa_intra16_en;
unsigned char               tus_ifa_inter32_en;
unsigned char               tus_ifa_inter16_en;
unsigned char               tus_mce_inter32_en;
unsigned char               tus_mce_inter16_en;
unsigned char               tus_mce_inter8_en;
unsigned char               rfh_sel_intra32_en;
unsigned char               rfh_sel_intra16_en;
unsigned char               rfh_msk_skip32_en;
unsigned char               rfh_msk_skip16_en;
unsigned char               mf_sel_sech_skip32_en;
unsigned char               mf_sel_sech_skip16_en;
unsigned char               srd_en;
unsigned char               srd_cfg;
unsigned char               srd_motion_shift;
unsigned char               srd_mode_ctrl;
unsigned char               csd_qp_thrd[5];
unsigned char               var_split_thrd;
unsigned char               var_flat_recon_thrd[2][5];
/* unsigned char               var_flat_subblk_size_intra; */
/* unsigned char               var_flat_subblk_size_inter; */
unsigned char               var_flat_sub_size;
unsigned char               csd_color;//hjiang added
unsigned char               p_csd_en;//hjiang added
unsigned char               i_csd_en;//hjiang added
unsigned char               p_pmd_en;//hjiang added
unsigned char               csd_p_cu64_en;//hjiang added
unsigned char               ned_block_frmnum;
unsigned char               ned_motion_en;
unsigned char               ned_motion_shift;
unsigned char               ned_sad_thr[2][2][4];
unsigned short              ned_qp_sad_thr_tbl[10];
unsigned short              ned_sad_thr_cpx_step[2][3];
//unsigned char                ned_ai_background_sad;
//ned
unsigned char               ccf_rm_en;
unsigned char               ned_en;
unsigned char               rcn_plain_en;
unsigned char               ned_score_table[2][2][8];
unsigned char               ned_score_bias[2];
unsigned char               min_qp;
unsigned char               c_sad_min_bias;
//mosaic
unsigned char               mosaic_en;
unsigned char               mosaic_recon_flat_thr[4]; //8x8 16x16 32x32 64x64
unsigned char               mosaic_diff_thr[4];       //8x8 16x16 32x32 64x64
unsigned char               mosaic_intra_texture_en;
unsigned char               mosaic_motion_shift;
// inter sa8d
unsigned char               inter_sa8d_en; // inter selects 3 out of 4 modes through sa8d
										   //keyint
unsigned char               keyint;
//delta lambdaQP
unsigned char               dlambda_framelevel_en;
char                dlambda_IKeyFrame;
char                dlambda_PKeyFrame;
char                dlambda_PNorFrame;
unsigned char               dlambda_ipmd_en;
unsigned char               dlambda_ppmd_en;
unsigned char               dlambda_icsd_en;
unsigned char               dlambda_pcsd_en;
unsigned char               dlambda_motion_shift;
unsigned char               dlambda_max_depth;
unsigned char               dlambda_frmqp_thrd;
char                dlambda_simple_dqp;
char                dlambda_nei_cplx_dqp;
char                dlambda_still_dqp;
char                dlambda_edge_dqp;
//char                dlambda_ai_background_dqp;
//char                dlambda_ai_foreground_dqp;
//mvp_force_zero
unsigned char               mvp_force_zero_en;
unsigned char               mvp_force_zero_motion_shift;
//color_shadow
unsigned char               color_shadow_en;
unsigned char               color_shadow_motion_shift;
unsigned char               color_shadow_sad_thrd[2][5]; //[nei_cplx:0/1][0:sad_u/sad_v 1:sad_u+sad_v 2:sad_y+sad_u+sad_v 3:sad_u/sad_v 4:sad_u+sad_v]
														 //color_shadow_sse
unsigned char               color_shadow_sse_en; //color_shadow detect use chrom sse
unsigned char               color_shadow_qp;
unsigned char               color_shadow_motion_shift_sse; //color_shadow detect use chrom sse
unsigned char               color_shadow_sse_ratio_thrd[2];
unsigned char               color_shadow_sse_value_thrd_en;
unsigned int              color_shadow_sse_value_thrd[3];
unsigned char               color_shadow_sse_priority_en;
// 16 group of configure
unsigned int 		md_cfg_grp[16][2];	//
unsigned int 		lambda_cfg[11];	//
/* rate ctrl */
unsigned char 		rc_en;                  // hw ratecontrol enable
unsigned char 		bu_size;                // lcu numbers in a bu
unsigned char 		bu_len;                 // bu numbers in a frame
unsigned char 		rc_method;              // rc alg sel
unsigned short 		rc_thd[12];             // rc alg threshold
unsigned int 		rc_info[128];           // bu's target bits
char                bu_max_dqp;
char                bu_min_dqp;
unsigned char               rc_avg_len;
unsigned char               rc_min_prop;
unsigned char               rc_max_prop;
unsigned char               rc_dly_en;
unsigned char               rc_max_qp;
unsigned char               rc_min_qp;
/* jrfcd */
unsigned char               compress_flag;          // compress rebuild frame flag
unsigned int 		head_dsty;		// rebuild frame Y head physical address
unsigned int 		head_dstc;		// rebuild frame C head physical address
unsigned int 		head_sp_dsty;		// rebuild frame sp Y head physical address
unsigned int 		head_sp_dstc;		// rebuild frame sp C head physical address
unsigned int 		head_refy;		// ref frame Y head physical address
unsigned int 		head_refc;		// ref frame C head physical address
unsigned int 		head_mrefy;		// ref frame Y head physical address
unsigned int 		head_mrefc;		// ref frame C head physical address
unsigned int              lm_head_total;          // luma head total
unsigned int              cm_head_total;          // chroma head total
/* nv12 */
unsigned char               nv12_flag;
unsigned int 		dst_y_pa_nv12;		// rebuild frame Y physical address for dec nv12
unsigned int 		dst_c_pa_nv12;		// rebuild frame C physical address for dec nv12
unsigned short 		dst_y_stride_nv12;	// rebuild frame Y stride, default equal to frame_width
unsigned short 		dst_c_stride_nv12;	// rebuild frame C stride, default equal to frame_width
/*buf-share*/
unsigned char               buf_share_flag;
unsigned char               buf_share_size; //0 is 64 rows of pixel, 1 is 128, 2 is 192...
unsigned int              buf_start_yaddr;
unsigned int              buf_start_caddr;
unsigned int              buf_beyond_yaddr;
unsigned int              buf_beyond_caddr;
unsigned int              buf_ref_yaddr; //for mce. start y addr
unsigned int              buf_ref_caddr; //for mce. start c addr
unsigned int              buf_base_yaddr; //for mce. fix y addr
unsigned int              buf_base_caddr; //for mce. fix c addr
unsigned int              buf_rem_mby;   //for mce. rem = beyond - start
unsigned char               use_dummy_byd; // for vector mult test. if bjm all open, JRFD need dummy byd addr
unsigned int              dummy_buf_beyond_yaddr;
unsigned int              dummy_buf_beyond_caddr;
unsigned char               roi_flag;
unsigned char               dump_api_en;
//In case stack overflow
struct h265_vdma_tmp tmp_val;
};
/* vpu h265 vdma struct end */

struct h265_instance {
	struct h265_param param;
	struct h265_header_info header;
	struct h265_slice_info vdma_info;
};

int h265_param_init(h265_t *inst, struct h265_param *param);
void h265_exit(h265_t *inst);
int h265_encode(h265_t *inst, struct vpu_io_info *input);

#endif //_VPU_CORE_H_
