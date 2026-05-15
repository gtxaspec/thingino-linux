#include "h265_core.h"
#include "vpu_common.h"
#include "h265_vpi.h"
#include "h265_entropy.h"
#include "hera.h"
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>

extern struct completion *hera_done_for_kernEnc;
extern int hera_irq_stat_for_kernEnc;
extern void H265E_T32V_SliceInit(struct h265_slice_info *s);
int h265_code_headers(struct h265_header_info *header, struct vpu_io_info *io);
int h265_code_headers_segment(struct h265_header_info *header, struct vpu_io_info *io, unsigned int bslen, int count, int is_final);

int h265_set_slice_info(struct h265_slice_info *si, struct vpu_io_info *io)
{
	int i, j;
	int intra_mode = 0;
	unsigned int md_ctrl = 0;
	unsigned int md_csd_bias = 0;
	unsigned int md_pmd_bias = 0;
	unsigned int md_lambda_bias = 0;
	unsigned int md_info = 0;
	unsigned char *tmp_addr = NULL;
	int uqpoffset = 0;
	int uv_qp_ofst = 0;
	int uv_dist_bias = 0;
	char qp_idx = 0;
	int cu32x_n      = 0;
	int cu32y_n      = 0;
	int cu16_num     = 0;
	int bs_len_size  = 0;
	int mvo_size     = 0;
	int mvi_size     = 0;
	int cps_size     = 0;
	int sobel_size   = 0;
	int mixc_size    = 0;
	int mix_size     = 0;
	int flag_size    = 0;
	int mdc_size     = 0;
	int mcec_size    = 0;
	int ipc_size     = 0;
	int qpt_size     = 0;
	int ai_size      = 0;
	int tup_resi_thr_score = 0;
	int thr_idx = 0;
	int cu8_search_en         = 0;
	int cu8_merge_en          = 0;
	int cu16_search_en        = 0;
	int cu16_merge_en         = 0;
	int cu32_search_en        = 0;
	int cu32_merge_en         = 0;
	int cu64_merge_en         = 0;
	ifa_s *ifa = NULL;
	vpi_method_s m;
	int frame_idx = 0;
	uint32_t ddr_y_base = 0;
	uint32_t ddr_c_base = 0;
	int frame_width   = 0;
	int frame_height  = 0;

	//share buf
	int beyond_size   = 0;
	int jump_size     = 0;
	int y_space       = 0;
	int c_space       = 0;
	int y_every_space = 0;
	int c_every_space = 0;
	int last_y_ofst_addr = 0;
	int last_c_ofst_addr = 0;
	int y_ofst_addr      = 0;
	int c_ofst_addr      = 0;

	memset(si, 0, sizeof(struct h265_slice_info));
	/* h265_api_default_set */
	ifa = &si->ifa_para;
	si->frame_type          = io->frame_type;
	si->frm_num_gop         = io->frm_num_gop;
	si->work_mode           = 2;
	si->frame_width         = C_ALIGN(io->width, 8);
	si->frame_height        = C_ALIGN(io->height, 8);
	si->efe_hw_width        = C_ALIGN(io->width, 8);
	si->efe_hw_height       = C_ALIGN(io->height, 8);
	si->frame_qp            = io->qp;
	si->frame_qp_svac       = io->qp*4;
	si->raw_format          = 0;
	si->video_type          = 0;
	si->is_decode           = 0;
	si->tu_split_en         = 0;
	si->time_out_threshold  = 0;
	si->qp_mode             = 0;
	si->rotate_mode         = 0;
	si->efe_mode            = 0;
	si->context_type        = si->frame_type;
	si->keyint              = io->gop;
	//pps and sps assign
	//si->use_dqp_flag     = pps.bUseDQP;
	//si->dqp_max_depth    = pps.maxCuDQPDepth;
	//si->frame_cqp_offset = param.cbQpOffset;
	//si->tu_inter_depth   = sps.quadtreeTUMaxDepthInter;
	si->motion_flag         = 0; //not used
	si->petbc               = 0; //not used
	si->depth               = 0; //not used
	si->sign_hide_flag      = 0; //not used
	si->filter_level        = 0; //only in svac2: 25;
	si->mb_lim              = 0; //only in svac2: 82;
	si->lim                 = 0; //only in svac2: 26;
	si->hev_thr             = 0; //only in svac2: 1;
	si->mb_lim_p            = 0; //only in svac2: 79;
	si->lim_p               = 0; //only in svac2: 25;
	si->hev_thr_p           = 0; //only in svac2: 1;
	si->use_hp              = 0; //only in svac2
	si->gray_en             = 0; //not used
	si->mce_slc_mvx         = 0; //not used
	si->mce_slc_mvy         = 0; //not used
	si->step_adapt_en       = 0; //not used
	si->step_thrd_0         = 0; //not used
	si->step_thrd_1         = 0; //not used
	si->interp_filter       = 0; //not used
	si->md_fskip_en         = 0; //not used
	si->md_fskip_cfg        = 0; //not used
	si->ifa_para.refy       = NULL;
	si->ifa_para.refc       = NULL;
	si->emc_cpsc_addr       = NULL;
	si->ifa_para.qp_table   = NULL;
	si->ifa_para.ref1       = NULL;
	si->ifa_para.ref1c      = NULL;
	si->emc_cpsc_en         = 1;
	si->nv12_flag           = 0;
	si->dst_y_pa_nv12       = 0;
	si->dst_c_pa_nv12       = 0;
	si->dst_y_stride_nv12   = 0;
	si->dst_c_stride_nv12   = 0;
	si->ipred_usis          = 0;
	si->ipred_slv_en        = 0;
	si->ppred_slv_en        = 0;
	si->ppred_slv_dif       = 0;
	si->ppred8_slv_en       = 0;
	si->ppred16_slv_en      = 0;
	si->ppred32_slv_en      = 0;
	si->md_bc_close         = 1;
	si->ppred8_mode_num     = 0;
	si->ppred16_mode_num    = 0;
	si->ppred32_mode_num    = 0;
	si->dblk_gray_en        = 0;
	si->stc_cfg_mode        = 0;
	si->stc_fixed_merge     = 0;
	si->stc_fixed_mode      = 0;
	si->stc_fixed_subtype   = 0;
	si->stc_fixed_offset0   = 0;
	si->stc_fixed_offset1   = 0;
	si->stc_fixed_offset2   = 0;
	si->stc_fixed_offset3   = 0;
	si->ned_block_frmnum    = 5;//not used
	si->i_intra_pu4_en      = 0;//hw not support
	si->p_intra_pu4_en      = 0;//hw not support
	si->sao_en              = 0;//hw not support
	si->sao_y_flag          = 0;//hw not support
	si->sao_c_flag          = 0;//hw not support
	si->pmdsu_en			= 0;//hw not support
	si->qp_table_cu32_mode  = 0;
	si->qp_table_len        = 0;
	si->qp_table            = NULL;
	si->chng_en             = 1;
	ifa->ifa_en             = 1;
	ifa->refresh_en         = 1;
	ifa->raw_avg_en         = 1;
	ifa->skin_en            = 1;
	ifa->ery_en             = 1;
	ifa->emc_out_en[0]      = 1;
	ifa->emc_out_en[1]      = 1;
	ifa->ras_en             = 1;
	ifa->still_times_en     = 0;
	ifa->qpg_skin_en        = 0;
	ifa->qpg_ery_en         = 0;
	ifa->qpg_en             = 1;//must be 1
								//still_times
	ifa->motion_level      = 7;
	ifa->max_still_times   = 5;
	//refresh
	ifa->refresh_mode       = 0;
	ifa->rfh_raw_thrd       = 16;
	ifa->rfh_edge_thrd      = 130;
	ifa->rfh_flat_thrd      = 800;
	ifa->rrs_filter_mode    = 1;
	memcpy(ifa->filter_thrd,      cnst_filter_thrd,      sizeof(unsigned char)*4);
	//qp
	ifa->base_qp            = si->frame_qp;
	ifa->max_qp             = si->frame_qp+12;
	ifa->min_qp             = si->frame_qp <= 12 ? 1 : si->frame_qp-12;
	C_CLIP3(1,51,ifa->max_qp);
	C_CLIP3(1,51,ifa->min_qp);
	si->qpg_max_qp          = si->ifa_para.max_qp;
	si->qpg_min_qp          = si->ifa_para.min_qp;
	si->rc_min_qp = ifa->min_qp;
	si->rc_max_qp = ifa->max_qp;
	//skin
	ifa->skin_lvl_en[0]     = ifa->skin_en;
	ifa->skin_lvl_en[1]     = ifa->skin_en;
	ifa->skin_lvl_en[2]     = ifa->skin_en;
	ifa->skin_cnt_thrd      = 30;
	memcpy( ifa->skin_level,       cnst_skin_level,       sizeof(short) * 4);
	memcpy( ifa->skin_thrd,        cnst_skin_thrd,        sizeof(unsigned char) * 12);
	memcpy( ifa->skin_mul_factor,  cnst_skin_mul_factor,  sizeof(unsigned char) * 4);
	memcpy( ifa->qpg_skin_qp_ofst,     cnst_qpg_skin_qp_ofst,            sizeof(char)  * 3);
	//ifa_v31_flag
	memcpy( ifa->sobel_blk16_thr,    cnst_sobel_blk16_thr,    sizeof(unsigned short)*3);
	memcpy( ifa->sobel_blk32_thr,    cnst_sobel_blk32_thr,    sizeof(unsigned short)*3);
	memcpy( ifa->smd_blk8_thr,       cnst_smd_blk8_thr,       sizeof(unsigned short)*6);
	memcpy( ifa->smd_blk16_thr,      cnst_smd_blk16_thr,      sizeof(unsigned short)*8*2);
	memcpy( ifa->smd_blk32_thr,      cnst_smd_blk32_thr,      sizeof(unsigned short)*3*2);
	//frm_cplx_thrd
	memcpy(ifa->frm_cplx_thrd,    cnst_frm_cplx_thrd,    sizeof(unsigned short)*2);
	//speed up
	si->pmdsu_thrd[0]  = 10;
	si->pmdsu_thrd[1]  = 11;
	si->pmdsu_thrd[2]  = 12;
	//ppmd
	memcpy(si->ppmd_cost_thr,             cnst_ppmd_cost_thr,             sizeof(unsigned char)*4);
	memcpy(si->pmd_qp_thrd,               cnst_pmd_qp_thrd,               sizeof(unsigned char)*7);
	memcpy(si->ppmd_thr_list,             cnst_ppmd_thr_list,             sizeof(unsigned char)*30*2);
	memcpy(si->ppmd_merge_thr,            cnst_ppmd_merge_thr,            sizeof(unsigned char)*2*4*5*2);
	memcpy(si->ppmd_search_thr,           cnst_ppmd_search_thr,           sizeof(unsigned char)*2*3*5*2);
	memcpy(si->ppmd_merge_search_thr,     cnst_ppmd_merge_search_thr,     sizeof(unsigned char)*2*3*5*2);
	memcpy(si->ppmd_intra_thr,            cnst_ppmd_intra_thr,            sizeof(unsigned char)*2*3*5*2);
	memcpy(si->ppmd_merge_thr_idx,        cnst_ppmd_merge_thr_idx,        sizeof(unsigned char)*40);
	memcpy(si->ppmd_search_thr_idx,       cnst_ppmd_search_thr_idx,       sizeof(unsigned char)*30);
	memcpy(si->ppmd_merge_search_thr_idx, cnst_ppmd_merge_search_thr_idx, sizeof(unsigned char)*30);
	memcpy(si->ppmd_intra_thr_idx,        cnst_ppmd_intra_thr_idx,        sizeof(unsigned char)*30);
	//ipred
	intra_mode  = 0x268a040;
	for(i=0;i<5;i++) si->intra_mode[i]   = (intra_mode  >> (i*6)) & 0x3f;
	for(i=0;i<5;i++) si->intra8_mode[i]  = (intra_mode  >> (i*6)) & 0x3f;
	for(i=0;i<5;i++) si->intra16_mode[i] = (intra_mode  >> (i*6)) & 0x3f;
	for(i=0;i<5;i++) si->intra32_mode[i] = (intra_mode  >> (i*6)) & 0x3f;

	//md base cfg
	md_ctrl = 0x110F;
	md_ctrl = 0x110F;
	si->md_pre_pmd[0] = (md_ctrl >> 0)  & 0x3;//not used
	si->md_pre_pmd[1] = (md_ctrl >> 2)  & 0x3;//not used
	si->md_pre_csd[0] = (md_ctrl >> 9)  & 0x3;//not used
	si->md_pre_csd[1] = (md_ctrl >> 13) & 0x3;//not used

	//frm md
	md_csd_bias = 0;
	md_pmd_bias = 0;
	md_lambda_bias = 0;
	for (i=0;i<4;i++) si->md_csd_bias[i][0]    = (md_csd_bias >> i)          & 0x1; //cu8/16/32/64 en, not used
	for (i=0;i<4;i++) si->md_csd_bias[i][1]    = (md_csd_bias >> 4*(i+1))    & 0xF; //cu8/16/32/64 val, not used
	for (i=0;i<3;i++) si->md_pmd_bias[i][0]    = (md_pmd_bias >> i)          & 0x1; //cu8/16/32 ned bias en
	for (i=0;i<3;i++) si->md_pmd_bias[i][1]    = (md_pmd_bias >> 4*(i+1))    & 0xF; //cu8/16/32 ned bias val
	for (i=0;i<3;i++) si->md_lambda_bias[i][0] = (md_lambda_bias >> i)       & 0x1; //sa8d/sse/stc lambda en
	for (i=0;i<3;i++) si->md_lambda_bias[i][1] = (md_lambda_bias >> 4*(i+1)) & 0xF; //sa8d/sse/stc lambda val

	memset(si->md_fskip_val, 0, sizeof(unsigned char)*128);//not used

	//do not care
	si->uv_dist_scale_en	          = 1;
	si->md_force_split                = 0;
	si->p_pmd_en			          = 0;//si->frame_type == H265_SLICE_TYPE_P && si->ifa_para.motion_en;
	md_info = 0x010;
	si->md_mvs_all                    = (md_info >> 4)  & 0xF;
	si->md_mvs_abs                    = (md_info >> 8)  & 0xF;
	si->tus_ifa_intra32_en            = 0;//md_tus_cfg[m.md_tus_lvl][0];
	si->tus_ifa_intra16_en            = 0;//md_tus_cfg[m.md_tus_lvl][1];
	si->tus_ifa_inter32_en	          = 0;//md_tus_cfg[m.md_tus_lvl][2];
	si->tus_ifa_inter16_en	          = 0;//md_tus_cfg[m.md_tus_lvl][3];
	si->tus_mce_inter32_en	          = 0;//md_tus_cfg[m.md_tus_lvl][4];
	si->tus_mce_inter16_en	          = 0;//md_tus_cfg[m.md_tus_lvl][5];
	si->tus_mce_inter8_en	          = 0;//md_tus_cfg[m.md_tus_lvl][6];
	si->rfh_sel_intra32_en	          = 0;//md_rfh_cfg[m.md_rfh_lvl][0];
	si->rfh_sel_intra16_en	          = 0;//md_rfh_cfg[m.md_rfh_lvl][1];
	si->rfh_msk_skip32_en	          = 0;//md_rfh_cfg[m.md_rfh_lvl][2];
	si->rfh_msk_skip16_en	          = 0;//md_rfh_cfg[m.md_rfh_lvl][3];

	//dist_scale
	uqpoffset = 0;
	uv_qp_ofst = si->frame_qp - (chromaScale_ofst_radix[si->frame_qp] + uqpoffset);
	uv_dist_bias = (uv_qp_ofst + 12) % 3;
	si->uv_dist_scale[0] = uv_dist_bias==0 ? 16 : uv_dist_bias==1 ? 20 : 25;
	si->uv_dist_scale[1] = (unsigned char)(((uv_qp_ofst < 0) ? (-uv_qp_ofst+2) : uv_qp_ofst) / 3);
	si->uv_dist_scale[2] = (unsigned char)(uv_qp_ofst < 0);

	/* h265_api_method */
	memset(&m, 0, sizeof(vpi_method_s));
	m.func.ipred			 = vpi_func_cfg[0][0];
	m.func.mce_size		 = vpi_func_cfg[1][0];
	m.func.mce_ctrl		 = vpi_func_cfg[2][0];
	m.func.blk				 = vpi_func_cfg[3][0];
	m.func.tfm				 = vpi_func_cfg[4][0];
	m.func.csd				 = vpi_func_cfg[5][0];
	m.func.mos				 = vpi_func_cfg[6][0];
	m.func.csh				 = vpi_func_cfg[7][0];
	m.func.dlbda			 = vpi_func_cfg[8][0];
	m.func.su				 = vpi_func_cfg[9][0];
	m.func.fmd 			 = vpi_func_cfg[10][0];
	m.func.qpg				 = vpi_func_cfg[11][0];
	m.thrd.tus 			 = md_thrd_cfg[0][0];
	m.thrd.srd	   		     = md_thrd_cfg[1][0];
	m.thrd.csd 			 = md_thrd_cfg[2][0];
	m.thrd.mos              = md_thrd_cfg[3][0];
	m.thrd.ned  			 = md_thrd_cfg[4][0];
	m.thrd.csh	             = md_thrd_cfg[5][0];
	m.thrd.dlbda	     	 = md_thrd_cfg[6][0];
	m.thrd.mvp_force_zero   = md_thrd_cfg[7][0];
	m.thrd.pmdctrl		     = md_thrd_cfg[8][0];
	m.thrd.mce			     = mce_thrd_cfg[0][0];
	m.thrd.ipred			 = ipred_thrd_cfg[0][0];

	/* get_func_enable_cfg */
	si->ifa_para.petbc_en = 1;//must 1
							  //blk
	si->ip_cfg_en                      = emc_blk_cfg[0][m.func.blk];
	si->mce_cfg_en                     = emc_blk_cfg[1][m.func.blk];
	si->md_cfg_en                      = emc_blk_cfg[2][m.func.blk];
	si->emc_ai_en                      = emc_blk_cfg[3][m.func.blk];
	si->bsfull_intr_en                 = emc_blk_cfg[4][m.func.blk];
	//qpg
	si->ifa_para.qpg_smd_en            = qpg_cfg[0][m.func.qpg];
	si->ifa_para.qpg_sobel_en          = qpg_cfg[1][m.func.qpg];
	si->ifa_para.qpg_petbc_en          = qpg_cfg[2][m.func.qpg];
	si->ifa_para.qpg_table_en          = qpg_cfg[3][m.func.qpg];
	si->ifa_para.qpg_filte_en          = qpg_cfg[4][m.func.qpg] && si->ifa_para.petbc_en && (si->ifa_para.qpg_ery_en || si->ifa_para.qpg_smd_en ||
			si->ifa_para.qpg_sobel_en || si->ifa_para.qpg_petbc_en);

	//ipred
	si->i_pmd_en                       = ipred_cfg[0][m.func.ipred];
	si->ipmd_mpm_use_maxnum            = ipred_cfg[1][m.func.ipred];
	si->frm_ipmd_angle_en              = ipred_cfg[2][m.func.ipred];
	si->sobel_en                       = ipred_cfg[3][m.func.ipred];
	si->ppred_mode_num                 = ipred_cfg[4][m.func.ipred];
	si->ipred_size                     = ipred_cfg[5][m.func.ipred];
	si->sobel_size                     = ipred_cfg[6][m.func.ipred];
	//mce
	si->half_pixel_sech_en             = mce_cfg0[0][m.func.mce_ctrl];
	si->quart_pixel_sech_en            = mce_cfg0[1][m.func.mce_ctrl];
	si->mrg_en                         = mce_cfg0[2][m.func.mce_ctrl];
	si->mrg_fpel_en                    = mce_cfg0[3][m.func.mce_ctrl];
	si->max_sech_step                  = mce_cfg0[5][m.func.mce_ctrl];
	//si->max_mvrx                       = mce_cfg0[6][m.func.mce_ctrl];
	//si->max_mvry                       = mce_cfg0[7][m.func.mce_ctrl];
	si->max_mvrx                       = 64;
	si->max_mvry                       = 32;
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			si->inter_mode[i][j] = (mce_ctrl[m.func.mce_size] >> (i*4+j)) & 1;
	//tfm
	si->deadzone_en                    = tfm_cfg[0][m.func.tfm];
	si->acmask_en                      = tfm_cfg[1][m.func.tfm];
	si->rdoq_en                        = tfm_cfg[2][m.func.tfm];
	si->acmask_type                    = tfm_cfg[3][m.func.tfm];
	//md
	si->i_csd_en					   = md_csd_cfg[0][m.func.csd] && si->frame_type == H265_SLICE_TYPE_IDR;
	si->p_csd_en					   = md_csd_cfg[1][m.func.csd] && si->frame_type == H265_SLICE_TYPE_P;
	si->csd_color					   = md_csd_cfg[2][m.func.csd];
	si->csd_p_cu64_en				   = md_csd_cfg[3][m.func.csd];
	si->ccf_rm_en					   = md_csd_cfg[4][m.func.csd];
	si->rcn_plain_en				   = md_csd_cfg[5][m.func.csd] || si->i_csd_en || si->p_csd_en;
	si->mosaic_en					   = md_mos_cfg[0][m.func.mos] && si->frame_type == H265_SLICE_TYPE_P;
	si->mosaic_intra_texture_en		   = md_mos_cfg[1][m.func.mos] && si->mosaic_en;
	si->ned_en						   = md_csh_cfg[0][m.func.csh];
	si->ned_motion_en				   = md_csh_cfg[1][m.func.csh];
	si->color_shadow_en				   = md_csh_cfg[2][m.func.csh];
	si->color_shadow_sse_en			   = md_csh_cfg[3][m.func.csh];
	si->color_shadow_sse_priority_en   = md_csh_cfg[4][m.func.csh];
	si->color_shadow_sse_value_thrd_en = md_csh_cfg[5][m.func.csh];
	si->dlambda_framelevel_en		   = md_dlbda_cfg[0][m.func.dlbda];
	si->dlambda_ipmd_en				   = md_dlbda_cfg[1][m.func.dlbda];
	si->dlambda_ppmd_en				   = md_dlbda_cfg[2][m.func.dlbda];
	si->dlambda_icsd_en				   = md_dlbda_cfg[3][m.func.dlbda];
	si->dlambda_pcsd_en				   = md_dlbda_cfg[4][m.func.dlbda];
	si->pmdctrl_en					   = md_su_cfg[2][m.func.su];
	si->blk_pmdctrl_en				   = md_su_cfg[3][m.func.su];
	si->inter_sa8d_en				   = md_su_cfg[0][m.func.su] && si->frame_type == H265_SLICE_TYPE_P;
	if(si->inter_sa8d_en){
		si->pmdctrl_en     = 0xF;
		si->pmdctrl_mode   = 0;
		si->blk_pmdctrl_en = 0;
	}
	si->mvp_force_zero_en			   = md_fmode_cfg[0][m.func.fmd];
	si->srd_en						   = md_fmode_cfg[1][m.func.fmd] && si->frame_type == H265_SLICE_TYPE_P;
	//dblk
	si->dblk_en                        = 1;
	//buf share, jrfc
	si->buf_share_flag                 = 1;
	si->compress_flag                  = 0;
	si->mref_en                        = 0;
	si->maxnum_ref                     = 1;

	/* get_ifa_cfg */
	//ifa enable
	ifa->smd_en             = 1;
	ifa->sobel_en           = 1;
	ifa->cps_en             = 1;
	ifa->rrs_en             = ifa->cps_en && si->frame_type != H265_SLICE_TYPE_IDR;
	ifa->cps_c_en           = ifa->cps_en;
	ifa->rrs_c_en           = ifa->rrs_en;
	ifa->rrs_of_en          = ifa->rrs_en;
	ifa->rrs1_en            = 0 && si->frame_type == H265_SLICE_TYPE_IDR && (si->frm_num_gop > 1);
	ifa->rrs1_c_en          = 0 && ifa->rrs1_en;
	ifa->smd_c_en           = ifa->smd_en;
	ifa->motion_en          = ifa->rrs_en;
	ifa->motion_c_en        = ifa->rrs_c_en;
	ifa->petbc_en           = 1;
	ifa->nei_cplx_en        = 1;
	ifa->var_flat_en        = 1;
	ifa->cu_unifor_en       = 1;
	ifa->pet_fil_en         = 1;
	ifa->ai_mark_en         = 0;
	ifa->cps1_copy_en       = si->frame_type == H265_SLICE_TYPE_IDR;
	ifa->ai_prev_mark_en    = 0;
	ifa->qpg_table_en       = 0;
	ifa->qpg_roi_en         = 0;

	//cps
	ifa->cps_mode           = 1;
	ifa->cps_size           = ifa->cps_mode ? 16 : 64;
	ifa->stride_refy        = si->frame_width / (ifa->cps_mode == 0 ? 2 : 4);
	//rrs
	ifa->rrs_mode           = 1;
	ifa->rrs_thrd           = 4;
	ifa->rrs_thrd_c         = 2;
	//sobel
	ifa->sobel_edge_thrd    = 130;
	ifa->sobel_16_or_14     = 0;
	//qpg
	ifa->qpg_cu32_qpm       = 0;
	ifa->crp_thrd           = 2;
	ifa->qpg_mt_idx         = 7;
	for(i = 0; i < 5; i++)
		ifa->qpg_pet_qp_idx[i] = 4;
	qp_idx = (si->frame_qp < 20 ? 0 :
			si->frame_qp < 27 ? 1 :
			si->frame_qp < 33 ? 2 :
			si->frame_qp < 39 ? 3 :
			si->frame_qp < 42 ? 4 :
			si->frame_qp < 46 ? 5 : 6);
	memcpy( ifa->qpg_pet_qp_idx,       cnst_qpg_pet_qp_idx,              sizeof(unsigned char) * 5);
	memcpy( ifa->qpg_pet_cplx_thrd,    cnst_qpg_pet_cplx_thrd,           sizeof(unsigned char) * 3 * 12);
	memcpy( ifa->qpg_pet_qp_ofst,      cnst_qpg_pet_qp_ofst,             sizeof(unsigned char) * 5 * 7);
	memcpy( ifa->qpg_pet_qp_ofst_mt,   cnst_qpg_pet_qp_ofst_mt,          sizeof(unsigned char) * 5 * 7);
	memcpy( ifa->qpg_dlt_thr,          cnst_qpg_dlt_thr,                 sizeof(unsigned char) * 4);
	memcpy( ifa->qpg_cplx_qp_ofst,    &cnst_qpg_cplx_qp_ofst[qp_idx][0], sizeof(char)  * 8);
	memcpy( ifa->qpg_smd_cplx_thrd,    cnst_qpg_smd_thrd,                sizeof(short) * 8);
	memcpy( ifa->qpg_sobel_cplx_thrd,  cnst_qpg_sobel_thrd,              sizeof(short) * 8);
	//petbc
	ifa->pet_mode          = 1;
	ifa->pet_cnt_alg       = 0;
	memcpy( ifa->petbc_var_thr,      cnst_petbc_var_thr,      sizeof(unsigned short)*3);
	memcpy( ifa->petbc_ssm_thr,      cnst_petbc_ssm_thr,      sizeof(unsigned short)*3*4);
	memcpy( ifa->petbc2_var_thr,     cnst_petbc2_var_thr,     sizeof(unsigned short)*3);
	memcpy( ifa->petbc2_ssm_thr,     cnst_petbc2_ssm_thr,     sizeof(unsigned short)*3*4);
	memcpy( ifa->pet_filter_valid,   cnst_pet_filter_valid,   sizeof(unsigned char) *4);
	//cu_uniform
	ifa->smd_blk16_bias[0] = 1;
	ifa->smd_blk16_bias[1] = 2;
	//var flat
	memcpy( ifa->var_flat_thr,       cnst_var_flat_thr,       sizeof(unsigned char) *2*2);
	//motion
	ifa->motion_thrd        = ifa->cps_mode ? 16 : 128;
	ifa->frm_motion_thrd[0] = ((int)ifa->motion_thrd/2 - 10) < 0 ? 0 : ifa->motion_thrd/2 - 10;
	ifa->frm_motion_thrd[1] = ((int)ifa->motion_thrd/2 - 5 ) < 0 ? 0 : ifa->motion_thrd/2 - 5;
	ifa->frm_motion_thrd[2] = ifa->motion_thrd*2 + 5;
	ifa->frm_motion_thrd[3] = ifa->motion_thrd*2 + 10;
	ifa->motion_of_thrd     = ifa->cps_mode ? 12 : 32;
	ifa->motion_of_thrd_c   = ifa->cps_mode ? 3 : 16;
	ifa->motion_thrd_c      = ifa->cps_mode ? 16 : 32;
	//frm_motion_cnt
	/*ifa->rrc_motion_thr[0] = ifa->frm_motion_thrd[0];
	  ifa->rrc_motion_thr[1] = ifa->frm_motion_thrd[1];
	  ifa->rrc_motion_thr[2] = ifa->motion_thrd/2;
	  ifa->rrc_motion_thr[3] = ifa->motion_thrd;
	  ifa->rrc_motion_thr[4] = ifa->motion_thrd*2;
	  ifa->rrc_motion_thr[5] = ifa->frm_motion_thrd[2];
	  ifa->rrc_motion_thr[6] = ifa->frm_motion_thrd[3];*/
	si->ifa_para.rrc_motion_thr[0] = 8;
	si->ifa_para.rrc_motion_thr[1] = 12;
	si->ifa_para.rrc_motion_thr[2] = 17;
	si->ifa_para.rrc_motion_thr[3] = 22;
	si->ifa_para.rrc_motion_thr[4] = 32;
	si->ifa_para.rrc_motion_thr[5] = 42;
	si->ifa_para.rrc_motion_thr[6] = 64;
	//AI
	ifa->ai_mark_input     = NULL;
	ifa->ai_scaling_factor = 12;
	ifa->ai_mark_type      = 4;
	ifa->frame_type        = si->frame_type;
	ifa->ai_collect_blk8   = 0;
	if(ifa->ai_mark_en)
		memcpy(ifa->qpg_ai_motion_smd_ofst,  cnst_ai_motion_smd_ofst,   sizeof(char)*3*2*8);
	else
		memcpy(ifa->qpg_ai_motion_smd_ofst,  cnst_motion_smd_ofst,   sizeof(char)*3*2*8);

	/* get_emc_cfg */
	cu32x_n      = (si->frame_width+31)/32;
	cu32y_n      = (si->frame_height+31)/32;
	cu16_num     = cu32x_n*cu32y_n*4;

	si->emc_mdc_en     = si->md_cfg_en;
	si->emc_ipc_en     = si->ip_cfg_en;
	si->emc_mcec_en    = si->mce_cfg_en;
	si->emc_bs_en      = 1;
	si->emc_bslen_en   = 0;
	si->emc_cps_en     = si->ifa_para.cps_en;
	si->emc_sobel_en   = si->ifa_para.sobel_en;
	si->emc_mix_en     = 1;
	si->emc_flag_en    = 1;
	si->emc_mixc_en    = 1;
	si->emc_mvo_en     = si->frm_num_gop > 0;
	si->emc_mvi_en     = si->emc_mvo_en && si->frm_num_gop > 1;
	si->emc_qpt_en     = si->ifa_para.qpg_table_en;

	bs_len_size  = si->emc_bs_en    ? 0               : 0;
	mvo_size     = si->emc_mvo_en   ? cu16_num*4      : 0;
	mvi_size     = si->emc_mvi_en   ? cu16_num*4      : 0;
	cps_size     = si->emc_cps_en   ? cu16_num*16*3/2 : 0;
	sobel_size   = si->emc_sobel_en ? cu16_num*8      : 0;
	mixc_size    = si->emc_mixc_en  ? cu16_num*16     : 0;
	mix_size     = si->emc_mix_en   ? cu16_num*16     : 0;
	flag_size    = si->emc_flag_en  ? cu16_num*4      : 0;
	mdc_size     = si->emc_mdc_en   ? cu16_num*4      : 0;
	mcec_size    = si->emc_mcec_en  ? cu16_num*4      : 0;
	ipc_size     = si->emc_ipc_en   ? cu16_num*4      : 0;
	qpt_size     = si->emc_qpt_en   ? cu16_num        : 0;
	ai_size      = si->emc_ai_en    ? cu16_num        : 0;

	si->emc_cps_addr       = (unsigned char *)(((int)io->vpu_phyaddr                          + 255) & ~255);
	si->emc_sobel_addr     = (unsigned char *)(((int)si->emc_cps_addr       + cps_size    + 255) & ~255);
	si->emc_mix_addr       = (unsigned char *)(((int)si->emc_sobel_addr     + sobel_size  + 255) & ~255);
	si->emc_bslen_addr     = (unsigned char *)(((int)si->emc_mix_addr       + mix_size    + 255) & ~255);
	si->emc_flag_addr      = (unsigned char *)(((int)si->emc_bslen_addr     + bs_len_size + 255) & ~255);
	si->emc_mdc_addr       = (unsigned char *)(((int)si->emc_flag_addr      + flag_size   + 255) & ~255);
	si->emc_ipc_addr       = (unsigned char *)(((int)si->emc_mdc_addr       + mdc_size    + 255) & ~255);
	si->emc_qpt_addr       = (unsigned char *)(((int)si->emc_ipc_addr       + ipc_size    + 255) & ~255);
	si->emc_mcec_addr      = (unsigned char *)(((int)si->emc_qpt_addr       + qpt_size    + 255) & ~255);
	si->emc_mixc_addr      = (unsigned char *)(((int)si->emc_mcec_addr      + mcec_size   + 255) & ~255);
	si->ifa_para.cps_ref0  = (unsigned char *)(((int)si->emc_mixc_addr      + mixc_size   + 255) & ~255);
	si->ifa_para.cps_ref1  = (unsigned char *)(((int)si->ifa_para.cps_ref0  + cps_size    + 255) & ~255);
	si->emc_ai_addr        = (unsigned char *)(((int)si->ifa_para.cps_ref1  + cps_size    + 255) & ~255);
	// must last
	si->emc_mvo_addr       = (unsigned char *)(((int)si->emc_ai_addr        + ai_size    + 255) & ~255);
	si->emc_mvi_addr       = (unsigned char *)(((int)si->emc_mvo_addr       + mvo_size    + 255) & ~255);
	si->bsfull_intr_en = io->bsfull_en;//for test
	if(si->bsfull_intr_en == 1) {
		si->bsfull_intr_size   = io->bsfull_size;
		si->emc_bs_addr0       = (unsigned char *)io->bs_phyaddr;
		si->emc_bs_addr1       = (unsigned char *)(io->bs_phyaddr + io->bsfull_size * 1024);
	}
	else {
		si->bsfull_intr_size  = 0;
		si->emc_bs_addr0 = (unsigned char *)io->bs_phyaddr;
		si->emc_bs_addr1 = NULL;
	}

	if(si->ifa_para.cps_en) {
		if(io->frame_type == H265_SLICE_TYPE_IDR) {
			io->ref1_addr = si->ifa_para.cps_ref1;
			io->ref_addr  = si->ifa_para.cps_ref0;
			io->cps_addr  = si->emc_cps_addr;
		}
		else {
			tmp_addr     = io->ref_addr;
			io->ref_addr          = io->cps_addr;
			io->cps_addr          = tmp_addr;
			si->emc_cps_addr      = io->cps_addr;
			si->ifa_para.cps_ref0 = io->ref_addr;
		}
	}
	if(si->emc_mvo_en && si->emc_mvi_en){
		if(si->frm_num_gop > 1){
			si->emc_mvi_addr = si->emc_mvo_addr;
		}
	}

	/* get_ipred_cfg */
	si->intra_angle_bias[0]   = 1;
	memcpy( si->intra_angle_bias,   cnst_ipred_angle_bias[m.thrd.ipred],   sizeof(unsigned char)*4);
	//ipmd !angle_bias
	memcpy( si->frmb_ip_bits_en,    cnst_frmb_ip_bits_en[m.thrd.ipred],    sizeof(unsigned char)*5);
	memcpy( si->frmb_ip_bias,       cnst_frmb_ip_bias[m.thrd.ipred],       sizeof(unsigned char)*5*2);
	memcpy( si->frmb_ipc_bias,      cnst_frmb_ipc_bias[m.thrd.ipred],      sizeof(unsigned char)*5);
	memcpy( si->frmb_idx,           cnst_frmb_idx[m.thrd.ipred],           sizeof(unsigned char)*5);
	memcpy( si->ipmd_mpm_cost_bias, cnst_ipmd_mpm_cost_bias[m.thrd.ipred], sizeof(unsigned char)*3*2*4);
	memcpy( si->ipmd_dm_cost_bias,  cnst_ipmd_dm_cost_bias[m.thrd.ipred],  sizeof(unsigned char)*3*5);
	memcpy( si->intra_bits,         cnst_intra_bits[m.thrd.ipred],         sizeof(unsigned char)*16*5);

	/* get_mce_cfg */
	si->mce_scl_mode        = mce_cfg2[m.thrd.mce][0];
	si->refine_cu8_mode     = mce_cfg2[m.thrd.mce][1];
	si->refine_cu16_mode    = mce_cfg2[m.thrd.mce][2];
	si->refine_cu32_mode    = mce_cfg2[m.thrd.mce][3];
	si->mref_wei            = mce_cfg2[m.thrd.mce][4];
	si->merge_cand_num      = mce_cfg2[m.thrd.mce][5];

	/* get_md_cfg */
	si->mf_sel_sech_skip32_en         = md_srd_cfg[m.thrd.srd][0];//force sech skip
	si->mf_sel_sech_skip16_en         = md_srd_cfg[m.thrd.srd][1];//force sech skip
	si->srd_cfg                       = md_srd_cfg[m.thrd.srd][2];//srd
	si->srd_motion_shift              = md_srd_cfg[m.thrd.srd][3];//srd
	si->srd_mode_ctrl                 = md_srd_cfg[m.thrd.srd][4];//srd
	si->mosaic_motion_shift           = si->frame_type == H265_SLICE_TYPE_P ? md_mos_cfg2[m.thrd.mos][0] : 0;//mosaic
	si->ned_motion_shift              = md_ned_cfg2[m.thrd.ned][0];//ned
	si->c_sad_min_bias                = md_ned_cfg2[m.thrd.ned][1];//ned
	si->color_shadow_motion_shift     = md_color_shadow_cfg2[m.thrd.csh][0];//color_shadow
	si->color_shadow_motion_shift_sse = md_color_shadow_cfg2[m.thrd.csh][2];//color_shadow_sse
	si->dlambda_motion_shift          = md_dlbda_cfg3[m.thrd.dlbda][0];//dlambda
	si->dlambda_max_depth             = md_dlbda_cfg3[m.thrd.dlbda][1];//dlambda
	si->mvp_force_zero_motion_shift   = md_mvp_force_zero_cfg2[m.thrd.mvp_force_zero][0];//mvp_force_zero
	si->pmdctrl_mode                  = md_pmdctrl_cfg2[m.thrd.pmdctrl][0];//pmdctrl
	si->var_split_thrd                = md_csd_cfg2[m.thrd.csd][0];//csd
	si->var_flat_sub_size             = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : 1;//csd
																						//file cfg
	si->color_shadow_qp               = md_color_shadow_cfg2[m.thrd.csh][1];//color_shadow_sse
	si->dlambda_IKeyFrame             = md_dlbda_cfg2[m.thrd.dlbda][0];//dlambda
	si->dlambda_PKeyFrame             = md_dlbda_cfg2[m.thrd.dlbda][1];//dlambda
	si->dlambda_PNorFrame             = md_dlbda_cfg2[m.thrd.dlbda][2];//dlambda
	si->dlambda_simple_dqp            = md_dlbda_cfg2[m.thrd.dlbda][3];//dlambda
	si->dlambda_nei_cplx_dqp          = md_dlbda_cfg2[m.thrd.dlbda][4];//dlambda
	si->dlambda_still_dqp             = md_dlbda_cfg2[m.thrd.dlbda][5];//dlambda
	si->dlambda_edge_dqp              = md_dlbda_cfg2[m.thrd.dlbda][6];//dlambda
	si->dlambda_frmqp_thrd            = md_dlbda_cfg3[m.thrd.dlbda][2];//dlambda
																	   //array
	memcpy( si->intra_resi_en,                 cnst_intra_resi_en[m.thrd.tus],                 sizeof(unsigned char)  *3);     //tus

	tup_resi_thr_score = 16;
	for(i = 0; i < 8; i++){
		if(si->frame_qp < tup_resi_qp_tbl[0][i]){
			tup_resi_thr_score = tup_resi_qp_tbl[1][i];
			break;
		}
	}
	si->mce_resi_thr[0][0][0] = tup_resi_thr_score;
	si->mce_resi_thr[0][0][1] = 32;
	si->mce_resi_thr[0][0][2] = tup_resi_thr_score;
	si->mce_resi_thr[0][0][3] = 256;

	si->mce_resi_thr[1][0][0] = tup_resi_thr_score;
	si->mce_resi_thr[1][0][1] = 32;
	si->mce_resi_thr[1][0][2] = tup_resi_thr_score;
	si->mce_resi_thr[1][0][3] = 256;

	si->intra_resi_thr[0][0][0] = tup_resi_thr_score;
	si->intra_resi_thr[0][0][1] = 32;
	si->intra_resi_thr[0][0][2] = tup_resi_thr_score;
	si->intra_resi_thr[0][0][3] = 256;

	si->intra_resi_thr[1][0][0] = tup_resi_thr_score;
	si->intra_resi_thr[1][0][1] = 32;
	si->intra_resi_thr[1][0][2] = tup_resi_thr_score;
	si->intra_resi_thr[1][0][3] = 256;

	memcpy( si->recon_resi_thr,                cnst_recon_resi_thr[m.thrd.tus],                sizeof(unsigned short) *2*4*2); //tus

	for(i = 0; i < 7; i++){
		if(si->frame_qp < cnst_csd_qp_thrd[m.thrd.csd][i]){
			thr_idx = i;
			break;
		}
	}
	if(thr_idx < 4){
		for(i = 0; i < 5; i++){
			si->csd_qp_thrd[i] = cnst_csd_qp_thrd[m.thrd.csd][i];
			si->var_flat_recon_thrd[0][i] = cnst_var_flat_recon_thrd[m.thrd.csd][0][i];
			si->var_flat_recon_thrd[1][i] = cnst_var_flat_recon_thrd[m.thrd.csd][1][i];
		}
	}
	else if(thr_idx == 6){
		for(i = 2; i < 7; i++){
			si->csd_qp_thrd[i-2] = cnst_csd_qp_thrd[m.thrd.csd][i];
			si->var_flat_recon_thrd[0][i-2] = cnst_var_flat_recon_thrd[m.thrd.csd][0][i];
			si->var_flat_recon_thrd[1][i-2] = cnst_var_flat_recon_thrd[m.thrd.csd][1][i];
		}
	}
	else{
		for(i = thr_idx - 3; i < thr_idx + 2; i++){
			si->csd_qp_thrd[i+3-thr_idx] = cnst_csd_qp_thrd[m.thrd.csd][i];
			si->var_flat_recon_thrd[0][i+3-thr_idx] = cnst_var_flat_recon_thrd[m.thrd.csd][0][i];
			si->var_flat_recon_thrd[1][i+3-thr_idx] = cnst_var_flat_recon_thrd[m.thrd.csd][1][i];
		}
	}

	memcpy( si->mosaic_recon_flat_thr,         cnst_mosaic_recon_flat_thr[m.thrd.mos],         sizeof(unsigned char)  *4);     //mosaic
	memcpy( si->mosaic_diff_thr,               cnst_mosaic_diff_thr[m.thrd.mos],               sizeof(unsigned char)  *4);     //mosaic
	memset( si->ned_qp_sad_thr_tbl,            0,                                               sizeof(unsigned short) *10);    //ned
	memcpy( si->ned_sad_thr_cpx_step,          cnst_ned_sad_thr_cpx_step[m.thrd.ned],          sizeof(unsigned short) *2*3);   //ned
	memcpy( si->ned_sad_thr,                   cnst_ned_sad_thr[m.thrd.ned],                   sizeof(unsigned char)  *2*2*4); //ned
	memcpy( si->ifa_raw_thr,                   cnst_ifa_raw_thr[m.thrd.ned],                   sizeof(unsigned short) *8*2);   //ned
	memcpy( si->ned_score_table,               cnst_ned_score_table[m.thrd.ned],               sizeof(unsigned char)  *2*2*8); //ned cost
	memcpy( si->ned_score_bias,                cnst_ned_score_bias[m.thrd.ned],                sizeof(unsigned char)  *2);     //ned cost
	memcpy( si->color_shadow_sad_thrd,         cnst_color_shadow_sad_thrd[m.thrd.csh],         sizeof(unsigned char)  *2*5);   //color_shadow
	memcpy( si->color_shadow_sse_ratio_thrd,   cnst_color_shadow_sse_ratio_thrd[m.thrd.csh],   sizeof(unsigned char)  *2);     //color_shadow_sse
	memcpy( si->color_shadow_sse_value_thrd,   cnst_color_shadow_sse_value_thrd[m.thrd.csh],   sizeof(unsigned int) *3);     //color_shadow_sse
	memcpy( si->lambda_cfg,                    cnst_lambda_cfg[m.thrd.dlbda],                  sizeof(unsigned int) *11);    //delta lambdaQP
	if(si->frame_type == H265_SLICE_TYPE_IDR)
		memset( si->recon_resi_en,             0,                                               sizeof(unsigned char)  *4*2);   //pmd_disable for ned
	else
		memcpy( si->recon_resi_en,             cnst_recon_resi_en[m.thrd.ned],                 sizeof(unsigned char)  *4*2);   //pmd_disable for ned

	/* get_tfm_cfg */
	si->scaling_list_en       = 0;//keep with header
	si->scaling_list_present  = 0;
	si->do_not_clear_deadzone = 0;
	si->sse_mask              = 0;
	si->tfm_buf_en            = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0x17 : 0x1F;
	si->tfm_path_en[0]        = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0x124 : 0x3FF;
	si->tfm_path_en[1]        = 0;
	cu8_search_en         = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : (si->inter_mode[3][0]);
	cu8_merge_en          = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : (si->inter_mode[3][3]);
	cu16_search_en        = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : (si->inter_mode[2][0]);
	cu16_merge_en         = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : (si->inter_mode[2][3]);
	cu32_search_en        = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : (si->inter_mode[1][0]);
	cu32_merge_en         = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : (si->inter_mode[1][3]);
	cu64_merge_en         = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : (si->inter_mode[0][3]);

	si->tfm_path_en[0] &= (
			((!!(cu64_merge_en )) << 9) +
			((!!(si->ipred_size & 0x4)) << 8) +
			((!!(cu32_merge_en )) << 7) +
			((!!(cu32_search_en)) << 6) +
			((!!(si->ipred_size & 0x2)) << 5) +
			((!!(cu16_merge_en )) << 4) +
			((!!(cu16_search_en)) << 3) +
			((!!(si->ipred_size & 0x1)) << 2) +
			((!!(cu8_merge_en  )) << 1) +
			((!!(cu8_search_en )) << 0) +
			0);

	/* get_filter_cfg */
	si->beta_offset_div2    = 0;
	si->tc_offset_div2      = 0;

	/* get_cu_ctrl_cfg */
	/* get_cu_qp_cfg */
	/* get_buf_share_cfg */
	si->buf_rem_mby = 0x3F;
	/* get_jrfcd_cfg */
	si->min_qp              = 28;

	/* codec */
	if (io->is_ivdc) {
		si->frame_y_stride   = 4096;
		si->frame_c_stride   = 4096;
	} else {
		si->frame_y_stride   = io->y_stride;
		si->frame_c_stride   = io->c_stride;
	}
	si->frame_cqp_offset = 0;
	si->use_dqp_flag     = 1;
	si->dqp_max_depth    = 2;
	if (si->dqp_max_depth == 0) {
		si->qp_mode = 0;
	}
	si->tu_inter_depth   = 1;

	/* encode.c */
	if (io->is_ivdc) {
		si->raw_y_pa = 0x80000000;
		si->raw_c_pa = 0x81000000;
	} else {
		si->raw_y_pa = io->raw_y_phyaddr;
		si->raw_c_pa = io->raw_c_phyaddr;
	}
	si->des_va = io->des_viraddr;
	si->des_pa = io->des_phyaddr;
	si->bitstream_pa = io->bs_phyaddr;
	si->dst_y_stride  = (si->frame_width + 63) & ~63;
	si->dst_c_stride  = si->dst_y_stride/2;
	si->ref_y_stride  = (si->frame_width + 63) & ~63;
	si->ref_c_stride  = si->ref_y_stride;
#if 0
	si->dst_y_pa  = io->dec_y_phyaddr;
	si->dst_c_pa  = io->dec_c_phyaddr;
	si->ref_y_pa  = io->ref_y_phyaddr;
	si->ref_c_pa  = io->ref_c_phyaddr;
#else
	si->dst_y_pa  = 0;
	si->dst_c_pa  = 0;
	si->ref_y_pa  = 0;
	si->ref_c_pa  = 0;
#endif
	if (si->buf_share_flag) {
		frame_idx = si->frm_num_gop;
		ddr_y_base = (uint32_t)(io->rds_y_phyaddr + 255) & ~255;
		ddr_c_base = (uint32_t)(io->rds_c_phyaddr + 255) & ~255;
		si->buf_share_size = 2;
		//align
		frame_width   = (si->frame_width  + 63) & ~63;
		frame_height  = (si->frame_height + 63) & ~63;

		//share buf
		beyond_size   = (si->buf_share_size + 1) * 64;
		jump_size     = si->compress_flag ? si->buf_share_size * 64 : beyond_size;//buf share with jrfc
		y_space       = frame_width * (frame_height + beyond_size);
		c_space       = frame_width * (frame_height/2 + beyond_size/2);
		y_every_space = frame_width * jump_size;
		c_every_space = frame_width * (jump_size / 2);

		si->buf_beyond_yaddr = ddr_y_base + y_space;
		si->buf_beyond_caddr = ddr_c_base + c_space;
		si->buf_base_yaddr   = ddr_y_base;
		si->buf_base_caddr   = ddr_c_base;
		si->buf_ref_yaddr    = ddr_y_base;
		si->buf_ref_caddr    = ddr_c_base;
		si->buf_start_yaddr  = ddr_y_base;
		si->buf_start_caddr  = ddr_c_base;

		if(frame_idx > 0) {
			last_y_ofst_addr = ((frame_idx-1) * y_every_space) % y_space;//cur ref y ofst addr
			last_c_ofst_addr = ((frame_idx-1) * c_every_space) % c_space;//cur ref c ofst addr
			y_ofst_addr      = (frame_idx * y_every_space ) % y_space;   //cur rec y ofst addr
			c_ofst_addr      = (frame_idx * c_every_space ) % c_space;   //cur rec c ofst addr
			si->buf_start_yaddr  = y_ofst_addr      ? (si->buf_beyond_yaddr - y_ofst_addr)      : ddr_y_base;
			si->buf_start_caddr  = c_ofst_addr      ? (si->buf_beyond_caddr - c_ofst_addr)      : ddr_c_base;
			si->buf_ref_yaddr    = last_y_ofst_addr ? (si->buf_beyond_yaddr - last_y_ofst_addr) : ddr_y_base;
			si->buf_ref_caddr    = last_c_ofst_addr ? (si->buf_beyond_caddr - last_c_ofst_addr) : ddr_c_base;
		}
		si->buf_rem_mby = ((si->buf_beyond_yaddr - si->buf_ref_yaddr) / (frame_width)) / 64 - 1;
	}
	si->mref_y_pa = 0;
	si->mref_c_pa = 0;
	H265E_T32V_SliceInit(si);
	return 0;
}

int h265_param_init(h265_t *inst, struct h265_param *param)
{
	struct h265_instance *h265 = (struct h265_instance *)inst;
	VPS *vps = NULL;
	SPS *sps = NULL;
	PPS *pps = NULL;

	memset(h265, 0, sizeof(struct h265_instance));

	memcpy(&h265->param, param, sizeof(struct h265_param));
	vps = &h265->header.nalu.m_vps;
	sps = &h265->header.nalu.m_sps;
	pps = &h265->header.nalu.m_pps;
	defaultVPS(vps, param);
	defaultSPS(sps, param);
	defaultPPS(pps, param);
	initBits(&h265->header.bs);

	return 0;
}

void h265_exit(h265_t *inst)
{
	struct h265_instance *h265 = (struct h265_instance *)inst;
	freeBits(&h265->header.bs);
	return;
}

int h265_wait_for_completion(struct h265_instance *h265, struct vpu_io_info *io)
{
	int v0_status = 0;
	int ret = 0;
wait_retry:
	ret = wait_for_completion_interruptible_timeout(hera_done_for_kernEnc, msecs_to_jiffies(5000));
	if (ret > 0) {
#if 0
		//debug hw time
		int t0 = *(volatile unsigned int*)(0xb3200000+0xa4);
		int hw_time = t0 * 1024 / (600 * 1000);
		printk("vpu hw_time=%d\n", hw_time);
#endif
		if (hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) {
			io->bslen = vpu_wrap_read_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_BSLEN);
			if (io->is_ivdc) {
				v0_status = *(volatile int *)(0xb310007c);
				if (v0_status & (0x3<<30)) {
					//printk("bad frame,overflow:0x%08x\n", v0_status);
					return -1;
				} else if ((((v0_status>>4) & 0x7) != io->sensor_id)) {
					//printk("encode sid%d, need sid%d\n", (v0_status>>4) & 0x7, io->sensor_id);
					return -1;
				}
			}
		} else {
			VPU_ERROR("vpu intr err!\n");
			return -1;
		}
	} else if (ret == -ERESTARTSYS) {
		goto wait_retry;
	} else {
		VPU_ERROR("vpu wait intr timeout!\n");
		return -1;
	}

	//code headers
	ret = h265_code_headers(&h265->header, io);
	if (ret < 0) {
		VPU_ERROR("h265 code header failed\n");
		return ret;
	}

	return 0;
}

int h265_wait_for_completion_bsfull(struct h265_instance *h265, struct vpu_io_info *io)
{
	int v0_status = 0;
	int i = 0;
	int bslen = 0;
	int bslen_delta = 0;
	int ret = 0;
	int ret_bs = 0;

	while (1) {
		ret = wait_for_completion_interruptible_timeout(hera_done_for_kernEnc, msecs_to_jiffies(5000));
		if (ret > 0) {
#if 0
			//debug hw time
			int t0 = *(volatile unsigned int*)(0xb3200000+0xa4);
			int hw_time = t0 * 1024 / (600 * 1000);
			printk("vpu hw_time=%d\n", hw_time);
#endif
			if (hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) {
				if (io->is_ivdc) {
					v0_status = *(volatile int *)(0xb310007c);
					if (v0_status & (0x3<<30)) {
						//printk("bad frame,overflow:0x%08x\n", v0_status);
						return -1;
					} else if ((((v0_status>>4) & 0x7) != io->sensor_id)) {
						//printk("encode sid%d, need sid%d\n", (v0_status>>4) & 0x7, io->sensor_id);
						return -1;
					}
				}
			}
			bslen = vpu_wrap_read_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_BSLEN);
			bslen_delta = bslen - (i + 1) * io->bsfull_size * 1024;
			if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_BSFULL) && (bslen_delta >= 0)) {
				vpu_wrap_flush_cache(io->bs_viraddr, CACHE_ALL_SIZE, 2);
				if ((ret_bs = h265_code_headers_segment(&h265->header, io, io->bsfull_size * 1024, i, 0)) < 0) {
					VPU_ERROR("h265 encode segment error\n");
					return ret_bs;
				}
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
				i ++;
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_BSFULL) && (bslen_delta < 0)) {
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) && (i > 0)) {
				vpu_wrap_flush_cache(io->bs_viraddr, CACHE_ALL_SIZE, 2);
				if ((ret_bs = h265_code_headers_segment(&h265->header, io, bslen - i * io->bsfull_size * 1024, i, 1)) < 0) {
					VPU_ERROR("h265 encode segment error\n");
					return ret_bs;
				}
				io->vpu_curr_stream_len = h265->header.list.m_occupancy;
				return 0;
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) && (i == 0)) {
				io->bslen = bslen;
				break;
			} else {
				VPU_ERROR("vpu intr err!\n");
				return -1;
			}
		} else if (ret == -ERESTARTSYS) {
			continue;
		} else {
			VPU_ERROR("vpu wait intr timeout!\n");
			return -1;
		}
	}

	//code headers
	ret = h265_code_headers(&h265->header, io);
	if (ret < 0) {
		VPU_ERROR("h265 code header failed\n");
		return ret;
	}

	return 0;
}

int h265_encode_start(struct h265_instance *h265, struct vpu_io_info *io)
{
	int timeout = 0x7fff;
	unsigned int cfgc_glb_ctrl = 0;

	//flush cache
	vpu_wrap_flush_cache(io->des_viraddr, CACHE_ALL_SIZE, 0);

	//vpu reset
	vpu_wrap_write_reg(RADIX_CFGC_BASE+RADIX_REG_CFGC_GLB_CTRL, (0x1<<31) + 0xffff);//hera
	while (cfgc_glb_ctrl && --timeout) {
		cfgc_glb_ctrl = vpu_wrap_read_reg(RADIX_CFGC_BASE+RADIX_REG_CFGC_GLB_CTRL);
		vpu_wrap_usleep(20);
	}
	if (timeout == 0) {
		VPU_ERROR("vpu reset err!\n");
		return -1;
	}

	//ivdc
	if (io->is_ivdc) {
		*(volatile int *)(0xb3100000+0x78) = 0x1;;//write runfifo
		*(volatile int *)(0xb3100000+0x70) = io->sensor_id << 8 | 0x1;//write v0 start
	}

	//vpu start
	vpu_wrap_write_reg(RADIX_CFGC_BASE  + RADIX_REG_CFGC_GLB_CTRL, (0<<22) | (((1<<16) - 1) & 0xffff));
	vpu_wrap_write_reg(RADIX_CFGC_BASE  + RADIX_REG_CFGC_INTR_EN, RADIX_CFGC_INTE_IVDCRST | RADIX_CFGC_INTE_AMCERR | RADIX_CFGC_INTE_BSFULL | RADIX_CFGC_INTE_ENDF);
	vpu_wrap_write_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_VTYPE, 0x2 << 6);
	vpu_wrap_write_reg(RADIX_EMC_BASE + RADIX_REG_EMC_ADDR_CHN, RADIX_VDMA_ACFG_DHA(io->des_phyaddr));
	vpu_wrap_write_reg(RADIX_CFGC_BASE+RADIX_REG_CFGC_ACM_CTRL, RADIX_VDMA_ACFG_RUN);

	//vpu intr
	if (io->bsfull_en) {
		return h265_wait_for_completion_bsfull(h265, io);
	} else {
		return h265_wait_for_completion(h265, io);
	}
}

int h265_code_headers_segment(struct h265_header_info *header, struct vpu_io_info *io, unsigned int bslen, int count, int is_final)
{
	Slice *slice = &header->nalu.m_slice;
	NALList *list = &header->list;
	int g_maxSlices = 1;
	int ret = 0;

	if (count == 0) {
		slice->m_pps = &header->nalu.m_pps;
		slice->m_sps = &header->nalu.m_sps;
		slice->m_poc = io->frm_num_gop;
		if (!io->frm_num_gop || io->frame_type == H265_SLICE_TYPE_IDR) {
			//frame I
			header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
			slice->m_sliceType = I_SLICE;
		} else {
			//frame P
			header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
			slice->m_sliceType = P_SLICE;
		}

		if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
			slice->m_lastIDR = slice->m_poc;
		}
		if (slice->m_sliceType == H265_SLICE_TYPE_B) {
			slice->m_colFromL0Flag = 0;
			slice->m_colRefIdx = 0;
		} else {
			slice->m_colFromL0Flag = 1;
			slice->m_colRefIdx = 0;
		}
		slice->m_maxNumMergeCand = 2;
#define SLFASE_CONSTANT  0x5f4e4a53
		g_maxSlices = 1;
		slice->m_sLFaseFlag = (g_maxSlices > 1) ? false : ((SLFASE_CONSTANT & (1 << (slice->m_poc % 31))) > 0);;
		slice->m_sliceQp = io->qp;
		slice->numRefIdxDefault[0] = slice->m_pps->numRefIdxDefault[0];
		slice->numRefIdxDefault[1] = slice->m_pps->numRefIdxDefault[1];
		slice->m_rpsIdx = -1;
		slice->m_bUseSao = 0;

		//we dont have long term ref, and only have I&P frame
		computeRPS(slice);
		// Ensuring L0 contains just the -ve POC
		slice->m_numRefIdx[0] = C_MIN(1, slice->m_rps.numberOfNegativePictures + slice->m_rps.numberOfLongtermPictures);
		// Ensuring L1 contains just the -ve POC
		slice->m_numRefIdx[1] = C_MIN(1, slice->m_rps.numberOfPositivePictures);

		initNal(list, (char *)io->vpu_stream_buffer + io->vpu_stream_buffer_occupancy, io->vpu_stream_buffer_len - io->vpu_stream_buffer_occupancy);
		if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
			getStreamHeaders(&header->nalu, list, &header->bs);
		}
	}
	ret = getStreamSegment(&header->nalu, list, &header->bs, (char *)io->bs_viraddr + (count % 2) * io->bsfull_size * 1024, bslen, count, is_final);
	if (ret < 0) {
		VPU_ERROR("h265 getStreamSegment failed\n");
		return ret;
	}
}

int h265_code_headers(struct h265_header_info *header, struct vpu_io_info *io)
{
	Slice *slice = &header->nalu.m_slice;
	NALList *list = &header->list;
	int g_maxSlices = 1;
	int ret = 0;

	slice->m_pps = &header->nalu.m_pps;
	slice->m_sps = &header->nalu.m_sps;
	slice->m_poc = io->frm_num_gop;
	if (!io->frm_num_gop || io->frame_type == H265_SLICE_TYPE_IDR) {
		//frame I
		header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
		slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
		slice->m_sliceType = I_SLICE;
	} else {
		//frame P
		header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
		slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
		slice->m_sliceType = P_SLICE;
	}

	if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
		slice->m_lastIDR = slice->m_poc;
	}
	if (slice->m_sliceType == H265_SLICE_TYPE_B) {
		slice->m_colFromL0Flag = 0;
		slice->m_colRefIdx = 0;
	} else {
		slice->m_colFromL0Flag = 1;
		slice->m_colRefIdx = 0;
	}
	slice->m_maxNumMergeCand = 2;
#define SLFASE_CONSTANT  0x5f4e4a53
	g_maxSlices = 1;
	slice->m_sLFaseFlag = (g_maxSlices > 1) ? false : ((SLFASE_CONSTANT & (1 << (slice->m_poc % 31))) > 0);;
	slice->m_sliceQp = io->qp;
	slice->numRefIdxDefault[0] = slice->m_pps->numRefIdxDefault[0];
	slice->numRefIdxDefault[1] = slice->m_pps->numRefIdxDefault[1];
	slice->m_rpsIdx = -1;
	slice->m_bUseSao = 0;

	//we dont have long term ref, and only have I&P frame
	computeRPS(slice);
	// Ensuring L0 contains just the -ve POC
	slice->m_numRefIdx[0] = C_MIN(1, slice->m_rps.numberOfNegativePictures + slice->m_rps.numberOfLongtermPictures);
	// Ensuring L1 contains just the -ve POC
	slice->m_numRefIdx[1] = C_MIN(1, slice->m_rps.numberOfPositivePictures);

	initNal(list, (char *)io->vpu_stream_buffer + io->vpu_stream_buffer_occupancy, io->vpu_stream_buffer_len - io->vpu_stream_buffer_occupancy);
	if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
		getStreamHeaders(&header->nalu, list, &header->bs);
	}
	ret = getStream(&header->nalu, list, &header->bs, io->bs_viraddr, io->bslen);
	if (ret < 0) {
		VPU_ERROR("h265 getStream failed\n");
		return ret;
	}

	io->vpu_curr_stream_len = header->list.m_occupancy;
	//printk("kernel encode %d curr len = %d\n", io->tot_frame_num, io->vpu_curr_stream_len);
	return 0;
}

int h265_encode(h265_t *inst, struct vpu_io_info *io)
{
	struct h265_instance *h265 = (struct h265_instance *)inst;
	int ret = 0;

	//init vdma
	h265_set_slice_info(&h265->vdma_info, io);

	//start hw
	//ktime_t start, end;
	//start = ktime_get();
	ret = h265_encode_start(h265, io);
	if (ret < 0) {
		VPU_ERROR("h265 encode failed\n");
		return ret;
	}
	//end = ktime_get();
	//printk("h265_hw_start cost %lld ms\n", ktime_to_ms(end) - ktime_to_ms(start));
	return 0;
}
