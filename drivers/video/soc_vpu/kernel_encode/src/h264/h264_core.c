#include "h264_core.h"
#include "vpu_common.h"
#include "h264_vpi.h"
#include "h264_cabac.h"
#include "h264_header.h"
#include "hera.h"
#include <linux/completion.h>
#include <linux/jiffies.h>

extern struct completion *hera_done_for_kernEnc;
extern int hera_irq_stat_for_kernEnc;
extern void H264E_T32V_SliceInit(struct h264_slice_info *s);
static int h264_code_headers(struct h264_instance *h264, struct vpu_io_info *io);
static int h264_code_headers_segment(struct h264_instance *h264, struct vpu_io_info *io, unsigned int bslen, int count, int is_final);

static int h264_set_slice_info(struct h264_slice_info *si, struct vpu_io_info *io)
{
	unsigned char dqp_y = 0;
	unsigned char dqp_c = 0;
	int i;
	int qpg_idx = 0;
	int mce_idx = 0;
	int dblk_idx = 0;
	int max_qp = 0;
	int min_qp = 0;
	int qp_idx = 0;
	int mb_num = 0;
	int qpt_size   = 0;
	int mod_size   = 0;
	int cps_size   = 0;
	int sobel_size = 0;
	int mix_size   = 0;
	int flag_size  = 0;
	int ai_size    = 0;
	unsigned int start_addr = 0;
	unsigned char *tmp_addr = NULL;
	int fs_idx = 0;//c->cfg->u16MbWidth > 50 && c->cfg->u16MbHeight > 38 ? 1 : 0;
				   //int mvx_cnt = frm->mvCnt & 0xffff;
				   //int mvy_cnt = (frm->mvCnt >> 16) & 0xffff;
				   //float a = 0;
	int a = 0;
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

	/* default set */
	si->frame_type          = io->frame_type;
	si->frm_num_gop         = io->frm_num_gop;
	si->mb_width            = (io->width + 15) / 16;
	si->mb_height           = (io->height + 15) / 16;
	si->frame_width         = C_ALIGN(io->width, 16);
	si->frame_height        = C_ALIGN(io->height, 16);
	si->first_mby           = 0;
	si->last_mby            = si->mb_height - 1; //for multi-slice
	si->qp                  = io->qp;
	si->base_qp             = io->qp;
	si->max_qp              = io->qp + 13;
	si->min_qp              = io->qp - 12;
	si->frame_idx           = io->frm_num_gop;
	si->hw_width            = C_ALIGN(io->width, 16);
	si->hw_height           = C_ALIGN(io->height, 16);
	si->raw_format          = 8;
	si->bs_size_en          = 1;
	si->bs_size             = 1024;
	si->rotate_mode         = 0;
	si->rotate              = 0;
	si->info_en             = 1;
	si->mvd_sum_all         = 0;
	si->mvd_sum_abs         = 0;
	si->mv_sum_all          = 0;        //1;
	si->mv_sum_abs          = 0;        //1;
	si->cfg_size_x          = 0x4;      //1;
	si->cfg_size_y          = 0x4;      //1;
	si->cfg_iw_thr          = 0;
	si->cfg_mvr_thr1        = 0;
	si->cfg_mvr_thr2        = 0;
	si->cfg_mvr_thr3        = 0;
	si->skin_dt_en          = 0;
	si->skin_lvl            = 0;
	si->skin_cnt_thd        = 20;
	si->ncu_mov_en          = 0;
	si->ncu_move_len        = 20;
	si->ncu_move_info       = NULL;
	si->qpg_sobel_en        = 0;
	si->qpg_ery_en          = 0;
	si->qpg_skin_en         = 0;
	si->me_step_en          = 0;
	si->me_step_0           = 0;
	si->me_step_1           = 0;
	si->ref_mode            = 0;        //fix 1
	si->frm_re[0]           = 0;
	si->frm_re[1]           = 0;
	si->frm_re[2]           = 0;
	si->frm_re[3]           = 0;
	si->intra_mode_msk      = 0;
	si->i_4x4_dis           = 0;
	si->i_8x8_dis           = 0;
	si->i_16x16_dis         = 0;
	si->p_l0_dis            = 0;
	si->p_t8_dis            = 0;
	si->p_skip_dis          = 0;
	si->p_skip_pl0f_dis     = 0;
	si->p_skip_pt8f_dis     = 0;
	si->use_intra_in_pframe = 1;
	si->acmask_mode         = 0;
	si->is_scaling_custom   = 1;
	si->use_fast_mvp        = 1;
	si->size_mode           = 0;
	si->step_mode           = 0;
	si->mode_ctrl           = 4;//0;
	si->daisy_chain_en      = 0;
	si->curr_thread_id      = 0;
	si->fbc_ep              = 204;
	si->ysse_thr            = 0;
	si->csse_thr            = 0;
	si->sde_prior           = 5;
	si->db_prior            = 5;
	si->cqp_offset          = 0;//pps
	si->jm_lambda2_en       = 0;
	si->skip_en             = 1;
	si->rrs_dump_en         = 0;
	si->rrs_en              = (io->frame_type != H264_SLICE_TYPE_IDR);
	si->rrs_uv_en           = (io->frame_type != H264_SLICE_TYPE_IDR);
	si->rrs_size_y          = 16;
	si->rrs_size_c          = 8;
	si->inter_en            = 0;
	si->still_times_en      = 1;
	si->max_still_times     = 5;
	dqp_y = (si->base_qp > si->diff_qp_base[0]) ? (si->base_qp - si->diff_qp_base[0]) : 0;
	dqp_c = (si->base_qp > si->diff_qp_base[1]) ? (si->base_qp - si->diff_qp_base[1]) : 0;

	si->rrs_thrd_y = si->diff_thd_base[0] + dqp_y * si->diff_thd_ofst[0];
	si->rrs_thrd_u = si->diff_thd_base[1] + dqp_c * si->diff_thd_ofst[1];
	si->rrs_thrd_v = si->rrs_thrd_u;

	C_CLIP3(0,51,si->max_qp);
	C_CLIP3(0,51,si->min_qp);
	//skin
	si->skin_en        = 0;
	si->skin_cnt_thrd  = 30;
	si->skin_lvl_en[0] = si->skin_en & 1;
	si->skin_lvl_en[1] = si->skin_en & 1;
	si->skin_lvl_en[2] = si->skin_en & 1;

	memcpy(si->skin_level,      const_skin_level,      sizeof(short)*4);
	memcpy(si->skin_thrd,       const_skin_thrd,       sizeof(unsigned char) * 3 * 2 * 2);
	memcpy(si->skin_mul_factor, const_skin_mul_factor, sizeof(unsigned char) * 4);
	memcpy(si->skin_pxlu_thd, rc_skin_pxlu_thd, sizeof(si->skin_pxlu_thd));
	memcpy(si->skin_pxlv_thd, rc_skin_pxlv_thd, sizeof(si->skin_pxlv_thd));
	memcpy(si->skin_qp_ofst,  rc_skin_qp_ofst,  sizeof(si->skin_qp_ofst));
	memcpy(si->mult_factor,   rc_mult_factor,   sizeof(si->mult_factor));
	memcpy(si->shift_factor,  rc_shift_factor,  sizeof(si->shift_factor));
	memcpy(si->skin_ofst,     rc_skin_ofst,     sizeof(si->skin_ofst));

	/* get_func_enable_cfg */
	//qpg
	si->qpg_en         = 1;//must 1
	si->qpg_pet_fil_en = 1;
	si->qpg_smd_en     = qpg_cfg[0][qpg_idx];
	si->qpg_petbc_en   = qpg_cfg[1][qpg_idx];
	si->qpg_roi_en     = qpg_cfg[2][qpg_idx];
	si->qpg_table_en   = qpg_cfg[3][qpg_idx];
	si->qpg_roi_en = 0;
	si->qpg_table_en = 0;
	si->qpg_filte_en   = si->qpg_smd_en || si->qpg_petbc_en;
	//mce
	si->hpel_en         = mce_cfg[0][mce_idx];
	si->qpel_en         = mce_cfg[0][mce_idx];
	si->max_sech_step_i = mce_cfg[1][mce_idx];
	si->max_mvrx_i      = mce_cfg[2][mce_idx];
	si->max_mvry_i      = mce_cfg[3][mce_idx];
	//deblock
	si->deblock         = dblk_cfg[dblk_idx];
	//bjm
	si->buf_share_en   = 1;
	si->jrfcd_flag     = 0;
	si->mref_en        = 0;
	//AI
	si->ai_mark_en   = 0;
	si->ai_mark_type = 4;

	/* get_ifa_cfg */
	max_qp             = si->qp + 5;
	min_qp             = si->qp - 5;
	si->r_max_qp           = max_qp > 51 ? 51 : max_qp;
	si->r_min_qp           = min_qp < 1 ? 1 : min_qp;
	si->rc_max_qp          = si->r_max_qp;
	si->rc_min_qp          = si->r_min_qp;
	si->ifa_en             = 1;
	//smd
	si->smd_en             = 1;
	si->smd_c_en           = 1;
	//sobel
	si->sobel_en           = 1;
	si->sobel_edge_thrd    = 130;     //edge pixel thrd
	si->ras_en             = 0;
	//energy
	si->ery_en             = 1;
	//petbc
	si->crp_thrd           = 2;
	si->petbc_en           = 1;
	si->pet_mode           = 0;
	//rrs_en
	si->efe_rrs_en         = (si->frame_type == H264_SLICE_TYPE_P);
	si->rrs_c_en           = si->efe_rrs_en;          //enable
	si->rrs_mode           = 1;                       //0: raw-ref, 1: raw-raw
	si->rrs_of_en          = si->efe_rrs_en;          //overflow enable
	si->rrs_thrd           = 4;                       //overflow threshold
	si->rrs_thrd_c         = 2;                       //overflow threshold
													  //cps
	si->cps_en             = si->rrs_mode ? 1 : 0;    //cps_en  & frame==P_SLICE
	si->cps_c_en           = si->cps_en ? 1 : 0;      //compress raw chroma enable
	si->cps_mode           = 1;                       //down scale factor. 0: 2x2, 1: 4x4
	si->cps_size           = si->cps_mode ? 16: 64;   //cps_mode=0: 64, cps_mode=1: 16.
													  //motion
	si->motion_en          = si->efe_rrs_en;          //motion enable
	si->motion_c_en        = si->efe_rrs_en;;         //motion enable of chroma
	si->motion_thrd        = si->cps_mode ? 16 : 128;
	si->motion_of_thrd     = si->cps_mode ? 12 : 32;  //count threshold
	si->motion_of_thrd_c   = si->cps_mode ? 3 : 16;   //count threshold
	si->motion_thrd_c      = si->cps_mode ? 16 : 32;
	si->frm_motion_thrd[0] = (int)si->motion_of_thrd / 2 - 10 < 0 ? 0 : si->motion_of_thrd / 2 - 10;
	si->frm_motion_thrd[1] = (int)si->motion_of_thrd / 2 - 5 < 0 ? 0 : si->motion_of_thrd / 2 - 5;
	si->frm_motion_thrd[2] = si->motion_of_thrd * 2 + 5;
	si->frm_motion_thrd[3] = si->motion_of_thrd * 2 + 10;
	si->rrc_motion_thr[0]  = 8;//si->frm_motion_thrd[0];
	si->rrc_motion_thr[1]  = 12;//si->frm_motion_thrd[1];
	si->rrc_motion_thr[2]  = 17;//si->motion_thrd/2;
	si->rrc_motion_thr[3]  = 22;//si->motion_thrd;
	si->rrc_motion_thr[4]  = 32;//si->motion_thrd*2;
	si->rrc_motion_thr[5]  = 42;//si->frm_motion_thrd[2];
	si->rrc_motion_thr[6]  = 64;//si->frm_motion_thrd[3];
								//add refresh radix-ned
	si->ned_motion_shift   = 7;
	si->ned_motion_en      = 1;

	qp_idx = si->base_qp < 30 ? 0 : si->base_qp < 42 ? 1 : 2;
	memcpy(si->qpg_dlt_thr,        const_qpg_dlt_thr[qp_idx],sizeof(unsigned char)*4);
	qp_idx = (si->base_qp < 20 ? 0 :
			si->base_qp < 27 ? 1 :
			si->base_qp < 33 ? 2 :
			si->base_qp < 39 ? 3 :
			si->base_qp < 42 ? 4 :
			si->base_qp < 46 ? 5 : 6);
	memcpy(si->qpg_cplx_qp_ofst,       const_cplx_qp_ofst[qp_idx],   sizeof(char)   * 8);
	memcpy(si->qpg_ai_motion_smd_ofst, const_qpg_ai_motion_smd_ofst, sizeof(char)   * 3*2*8);
	memcpy(si->pet_filter_valid,       const_pet_filter_valid,       sizeof(unsigned char)  * 4);
	memcpy(si->petbc_var_thr,          const_petbc_var_thr,          sizeof(unsigned short) * 3);
	memcpy(si->petbc_ssm_thr,          const_petbc_ssm_thr,          sizeof(unsigned short) * 3 * 4);
	memcpy(si->petbc2_var_thr,         const_petbc2_var_thr,         sizeof(unsigned short) * 3);
	memcpy(si->petbc2_ssm_thr,         const_petbc2_ssm_thr,         sizeof(unsigned short) * 3 * 4);
	if(si->qpg_smd_en)
		memcpy(si->qpg_smd_cplx_thrd,      const_smd_cu16_thrd,          sizeof(short)  * 8);
	if(si->qpg_sobel_en)
		memcpy(si->qpg_sobel_cplx_thrd,      const_sobel_cu16_thrd,        sizeof(short)  * 8);
	memcpy(si->qpg_pet_cplx_thrd,      const_qpg_pet_cplx_thrd,      sizeof(unsigned char)  * 3 * 12);
	memcpy(si->qpg_pet_qp_ofst,        const_qpg_pet_qp_ofst,        sizeof(unsigned char)  * 5 * 7);
	memcpy(si->qpg_pet_qp_ofst_mt,     const_qpg_pet_qp_ofst_mt,     sizeof(unsigned char)  * 5 * 7);
	memcpy(si->qpg_pet_qp_idx,         const_qpg_pet_qp_idx,         sizeof(unsigned char)  * 5);
	memcpy(si->qpg_skin_qp_ofst,       const_skin_qp_ofst,           sizeof(char)   * 3);

	/* get_emc_cfg */
	mb_num = si->mb_width * si->mb_height;
	si->emc_cps_en       = 1;
	si->emc_sobel_en     = 1;
	si->emc_mix_en       = 1;
	si->emc_cpsc_en      = 0;
	si->emc_mixc_en      = 1;
	si->emc_flag_en      = 1;
	si->bsfull_intr_en   = io->bsfull_en;
	si->bsfull_intr_size = si->bsfull_intr_en ? io->bsfull_size : 0;

	qpt_size   = si->qpg_table_en ? mb_num            : 0;
	mod_size   = si->mb_mode_use  ? mb_num*2          : 0;
	cps_size   = si->emc_cps_en   ? mb_num * (16 + 8) : 0;
	sobel_size = si->emc_sobel_en ? mb_num * 8        : 0;
	mix_size   = si->emc_mix_en   ? mb_num * 16       : 0;
	flag_size  = si->emc_flag_en  ? mb_num * 8        : 0;
	ai_size    = si->ai_mark_en   ? mb_num * 4        : 0;

	si->emc_table_addr = 0;

	start_addr = (unsigned int)(io->vpu_phyaddr);
	si->emc_qpt_pa      = (unsigned int)(start_addr);
	//si->bu_ref_info     = (int *)(si->emc_qpt_pa);
	//si->mb_ref_info     = si->bu_ref_info;
	si->emc_mod_pa      = (unsigned int)(((unsigned int)si->emc_qpt_pa         + qpt_size                    + 255) & ~255);
	si->mb_mode_info    = (unsigned int *)(((unsigned int)si->emc_mod_pa       + mod_size                    + 255) & ~255);
	si->emc_ai_mark_pa  = (unsigned int)(((unsigned int)si->mb_mode_info       + mb_num                      + 255) & ~255);
	si->emc_bs_pa       = 0;
	si->emc_bs_pa1      = 0;
	si->emc_mixc_addr   = (unsigned char  *)(((unsigned int)si->emc_ai_mark_pa + ai_size                     + 255) & ~255);
	si->emc_cps_ref0    = (unsigned char  *)(((unsigned int)si->emc_mixc_addr  + mix_size                    + 255) & ~255);
	si->emc_cps_ref1    = (unsigned char  *)(((unsigned int)si->emc_cps_ref0   + cps_size                    + 255) & ~255);
	si->emc_cps_addr    = (unsigned char  *)(((unsigned int)si->emc_cps_ref1   + cps_size                    + 255) & ~255);
	si->emc_sobel_addr  = (unsigned char  *)(((unsigned int)si->emc_cps_addr   + cps_size                    + 255) & ~255);
	si->emc_mix_addr    = (unsigned char  *)(((unsigned int)si->emc_sobel_addr + sobel_size                  + 255) & ~255);
	si->emc_flag_addr   = (unsigned char  *)(((unsigned int)si->emc_mix_addr   + mix_size                    + 255) & ~255);

	if(si->emc_cps_en) {
		if(io->frame_type == H264_SLICE_TYPE_IDR) {
			io->ref1_addr = si->emc_cps_ref1;
			io->ref_addr  = si->emc_cps_ref0;
			io->cps_addr  = si->emc_cps_addr;
		}
		else {
			tmp_addr = io->ref_addr;
			io->ref_addr          = io->cps_addr;
			io->cps_addr          = tmp_addr;
			si->emc_cps_addr      = io->cps_addr;
			si->emc_cps_ref0 = io->ref_addr;
		}
	}

	/* get_ipred_cfg */
	si->mb_mode_val   = 0;
	si->bit_16_en     = 0;
	si->bit_8_en      = 1;
	si->bit_4_en      = 1;
	si->bit_uv_en     = 0;
	for (i = 0; i < 4; ++i) si->bit_4[i]  = 4;
	for (i = 0; i < 4; ++i) si->bit_8[i]  = 4;
	for (i = 0; i < 4; ++i) si->bit_16[i] = 0;
	for (i = 0; i < 4; ++i) si->bit_uv[i] = 0;
	//change lambda weight
	si->lamb_16_en    = 0;
	si->lamb_8_en     = 0;
	si->lamb_4_en     = 0;
	si->lamb_uv_en    = 0;
	si->lambda_info16 = 0;
	si->lambda_info8  = 0;
	si->lambda_info4  = 0;
	si->lambda_infouv = 0;
	//constant cost weight
	si->c_16_en       = 0;
	si->c_8_en        = 0;
	si->c_4_en        = 0;
	si->c_uv_en       = 0;
	memset(si->const_16, 0,  sizeof(si->const_16));
	memset(si->const_uv, 0,  sizeof(si->const_uv));
	memset(si->const_4,  0,  sizeof(si->const_4));
	memset(si->const_8,  0,  sizeof(si->const_8));
	//when same cost, increase the priority of mode
	si->pri_16        = 0;
	si->pri_8         = 0;
	si->pri_4         = 0;
	si->pri_uv        = 0;
	//when cur mode is same with neighbour, decrease the bit of mode
	si->ref_neb_4     = 1;
	si->ref_neb_8     = 1;
	si->ref_4         = 4;
	si->ref_8         = 3;

	/* get_mce_cfg */
	si->pskip_en               = 1;
	si->scl                    = 3;
	si->lambda_scale_parameter = 8;
	si->fs_en                  = fs_cfg[0][fs_idx];
	si->fs_md                  = fs_cfg[1][fs_idx];
	si->fs_px                  = fs_cfg[2][fs_idx];
	si->fs_py                  = fs_cfg[3][fs_idx];
	si->fs_rx                  = fs_cfg[4][fs_idx];
	si->fs_ry                  = fs_cfg[5][fs_idx];
	si->frm_mv_en              = 0;
	si->frm_mv_size            = 0;
	si->glb_mv_en              = 0;
	//si->glb_mvx                = mvx_cnt == 0 && mvy_cnt == 0 ? frm->mvxSum / mvx_cnt : 0;
	//si->glb_mvy                = mvx_cnt == 0 && mvy_cnt == 0 ? frm->mvySum / mvy_cnt : 0;

	/* get_md_cfg */
	//cost bias
	si->cost_bias_en            = 0;
	si->cost_bias_i_4x4         = 0;
	si->cost_bias_i_8x8         = 0;
	si->cost_bias_i_16x16       = 0;
	si->cost_bias_p_l0          = 0;
	si->cost_bias_p_t8          = 0;
	si->cost_bias_p_skip        = 0;
	//intra lambda bias
	si->intra_lambda_y_bias_en  = 0;
	si->intra_lambda_c_bias_en  = 0;
	si->intra_lambda_bias_qp0   = 0;
	si->intra_lambda_bias_qp1   = 0;
	si->intra_lambda_bias_0     = 0;
	si->intra_lambda_bias_1     = 0;
	si->intra_lambda_bias_2     = 0;
	//chroma sse bias
	si->chroma_sse_bias_en      = 0;
	si->chroma_sse_bias_qp0     = 0;
	si->chroma_sse_bias_qp1     = 0;
	si->chroma_sse_bias_0       = 0;
	si->chroma_sse_bias_1       = 0;
	si->chroma_sse_bias_2       = 0;
	//sse lambda bias
	si->sse_lambda_bias_en      = 0;
	si->sse_lambda_bias         = 0;
	//uniform neighbour intra mode
	si->inter_nei_en            = 0;
	//toward to inter or intra4
	si->skip_bias_en            = 0;
	//decrimate stream
	si->dcm_en                  = 0;
	si->dcm_param               = 0x4304;
	//mosaic
	si->mosaic_en               = 0;
	si->mos_gthd[0]             = 1;
	si->mos_gthd[1]             = 4;
	si->mos_sthd[0]             = 8;
	si->mos_sthd[1]             = 6;
	//ned
	si->c_sad_min_bias = 0;//3;
	si->recon_resi_en = si->frame_type == H264_SLICE_TYPE_P ? 1 : 0;
	memcpy(si->ned_sad_thr, const_ned_sad_thr, sizeof(unsigned char) * 2 * 2 * 4);
	memcpy(si->ned_sad_thr_cpx_step, const_ned_sad_thr_cpx_step, sizeof(unsigned short) * 2 * 3);

#ifdef CFG_MODE_CTRL_PARAM
	helix_md_ctl_gen_set(&(md_ctl_set[0]), &(mode_ctrl_param[0]));
	for (i = 0; i < 40; ++i)
		si->mode_ctrl_param[i] = mode_ctrl_param[i];
#else
	memset(si->mode_ctrl_param,  0, sizeof(si->mode_ctrl_param));
#endif
	//refresh
	si->refresh_en       = 0;
	//a = si->refresh_en ? (io->frame_num % io->gop) / 10 + 3.5 : 2;
	a = 2;
	si->mb_mode_use      = 0;
	si->force_i16dc      = 0;
	si->force_i16        = 1;
	si->refresh_mode     = 0;
	si->refresh_bias     = 10;
	si->refresh_cplx_thd = 4;
	si->cplx_thd_sel     = 0;
	si->diff_cplx_sel    = 0;
	si->diff_thd_sel     = 0;
	si->i16dc_cplx_thd   = io->frame_type == H264_SLICE_TYPE_IDR ? 6 : 3;
	si->i16dc_qp_base    = 33;
	si->i16dc_qp_sel     = 0;
	si->i16_qp_base      = io->frame_type == H264_SLICE_TYPE_IDR ? 25 : 28;
	si->i16_qp_sel       = 0;
	si->diff_cplx_thd    = 10;
	si->diff_qp_base[0]  = 33;
	si->diff_qp_base[1]  = 33;
	si->diff_qp_sel[0]   = 0;
	si->diff_qp_sel[1]   = 0;
	si->sas_eigen_en     = 0;
	si->crp_eigen_en     = 0;
	si->sas_eigen_dump   = 0;
	si->crp_eigen_dump   = 0;
	si->diff_thd_base[0] = (1<<9) + (1<<8);
	si->diff_thd_base[1] = (1<<7) + (1<<6);
	si->diff_thd_base[2] = (1<<7) + (1<<6);
	si->diff_thd_ofst[0] = ((1<<7) + (1<<6)) / 2;
	si->diff_thd_ofst[1] = ((1<<5) + (1<<4)) / 2;
	si->diff_thd_ofst[2] = ((1<<5) + (1<<4)) / 2;
	si->diff_thd_base[0] = (si->diff_thd_base[0] + a/2) / a;
	si->diff_thd_ofst[0] = (si->diff_thd_ofst[0] + a/2) / a;
	si->diff_thd_base[1] = (si->diff_thd_base[1] + a/2) / a;
	si->diff_thd_ofst[1] = (si->diff_thd_ofst[1] + a/2) / a;
	si->diff_thd_base[2] = si->diff_thd_base[1];
	si->diff_thd_ofst[2] = si->diff_thd_ofst[1];

	dqp_y = (si->base_qp > si->diff_qp_base[0]) ? (si->base_qp - si->diff_qp_base[0]) : 0;
	dqp_c = (si->base_qp > si->diff_qp_base[1]) ? (si->base_qp - si->diff_qp_base[1]) : 0;
	si->rrs_thrd_y = si->diff_thd_base[0] + dqp_y * si->diff_thd_ofst[0];
	si->rrs_thrd_u = si->diff_thd_base[1] + dqp_c * si->diff_thd_ofst[1];
	si->rrs_thrd_v = si->rrs_thrd_u;
	memcpy(si->cplx_thd_idx, cnst_cplx_thd_idx, sizeof(unsigned char)*24);
	memcpy(si->cplx_thd,     cnst_cplx_thd,     sizeof(unsigned char)*8);

	/* get_tfm_cfg */
	si->dct8x8_en = 0;
	memcpy(si->scaling_list,     cnst_scaling_list, sizeof(unsigned char)*64);
	memcpy(si->scaling_list8[0], cqm_8iy_tab,       sizeof(unsigned char)*64);
	memcpy(si->scaling_list8[1], cqm_8py_tab,       sizeof(unsigned char)*64);
	memcpy(si->deadzone,         cnst_deadzone,     sizeof(int)*9);

	/* get_filter_cfg */
	si->alpha_c0_offset = 0;//pps param
	si->beta_offset     = 0;//pps param

	/* get_cu_ctrl_cfg */
	/* get_cu_qp_cfg */
	/* get_buf_share_cfg */
	/* get_jrfcd_cfg */
	si->min_qp              = 28;
	si->dump_api_en         = 0;

	/* encode.c */
	si->des_va = io->des_viraddr;
	si->des_pa = io->des_phyaddr;
	si->bs = io->bs_phyaddr;
	if (si->bsfull_intr_en) {
		si->emc_bs_pa = io->bs_phyaddr;
		si->emc_bs_pa1 = io->bs_phyaddr + io->bsfull_size * 1024;
	} else {
		si->emc_bs_pa = si->bs;
	}
	if (io->is_ivdc) {
		si->stride[0] = 4096;
		si->stride[1] = 4096;
	} else {
		si->stride[0] = io->y_stride;
		si->stride[1] = io->c_stride;
	}
	si->frame_y_stride = si->stride[0];
	si->frame_c_stride = si->stride[1];

	if (si->buf_share_en) {
		frame_idx = si->frm_num_gop;
		ddr_y_base = (uint32_t)(io->rds_y_phyaddr + 255) & ~255;
		ddr_c_base = (uint32_t)(io->rds_c_phyaddr + 255) & ~255;
		si->buf_share_size = 2;
		//align
		frame_width   = (si->frame_width  + 63) & ~63;
		frame_height  = (si->frame_height + 63) & ~63;

		//share buf
		beyond_size   = (si->buf_share_size + 1) * 64;
		jump_size     = si->jrfcd_flag ? si->buf_share_size * 64 : beyond_size;//buf share with jrfc
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
		si->buf_ref_rem_mby = ((si->buf_beyond_yaddr - si->buf_ref_yaddr) / (frame_width)) / 64 - 1;
	}
#if 0
	si->fb[0][0] = io->dec_y_phyaddr;
	si->fb[0][1] = io->dec_c_phyaddr;
	si->fb[1][0] = io->ref_y_phyaddr;
	si->fb[1][1] = io->ref_c_phyaddr;
#else
	si->fb[0][0] = si->buf_base_yaddr;
	si->fb[0][1] = si->buf_base_caddr;
	si->fb[1][0] = 0;
	si->fb[1][1] = 0;
#endif

	if (io->is_ivdc) {
		si->raw[0] = 0x80000000;
		si->raw[1] = 0x81000000;
	} else {
		si->raw[0] = io->raw_y_phyaddr;
		si->raw[1] = io->raw_c_phyaddr;
	}
	si->raw_y_pa = si->raw[0];
	si->raw_c_pa = si->raw[1];
	si->frame_type = (si->frame_type != H264_SLICE_TYPE_IDR);

	H264E_T32V_SliceInit(si);
	return 0;
}

int h264_param_init(h264_t *inst, struct h264_param *param)
{
	struct h264_instance *h264 = (struct h264_instance *)inst;

	memset(h264, 0, sizeof(struct h264_instance));
	i264e_cabac_init();
	i264e_sps_init(&h264->header.sps, 0, param);
	i264e_pps_init(&h264->header.pps, 0, param, &h264->header.sps);
	param->bs_len = param->width * param->height;
	param->bs = (unsigned char*)vpu_wrap_malloc(param->bs_len);
	if (!param->bs) {
		VPU_ERROR("bs alloc failed!\n");
		goto err_bs_alloc;
	}
	memcpy(&h264->param, param, sizeof(struct h264_param));
	return 0;

err_bs_alloc:
	return -1;
}

void h264_exit(h264_t *inst)
{
	struct h264_instance *h264 = (struct h264_instance *)inst;
	vpu_wrap_free(h264->param.bs);
	return;
}

int h264_wait_for_completion(struct h264_instance *h264, struct vpu_io_info *io)
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
	ret = h264_code_headers(h264, io);
	if (ret < 0) {
		VPU_ERROR("h264 code header failed\n");
		return ret;
	}

	return 0;
}

int h264_wait_for_completion_bsfull(struct h264_instance *h264, struct vpu_io_info *io)
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
				if ((ret_bs = h264_code_headers_segment(h264, io, io->bsfull_size * 1024, i, 0)) < 0) {
					VPU_ERROR("h264 encode segment error\n");
					return ret_bs;
				}
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
				i ++;
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_BSFULL) && (bslen_delta < 0)) {
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) && (i > 0)) {
				vpu_wrap_flush_cache(io->bs_viraddr, CACHE_ALL_SIZE, 2);
				if ((ret_bs = h264_code_headers_segment(h264, io, bslen - i * io->bsfull_size * 1024, i, 1)) < 0) {
					VPU_ERROR("h264 encode segment error\n");
					return ret_bs;
				}
				io->vpu_curr_stream_len = h264->header.nal.i_occupancy;
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
	ret = h264_code_headers(h264, io);
	if (ret < 0) {
		VPU_ERROR("h264 code header failed\n");
		return ret;
	}

	return 0;
}

static int h264_encode_start(struct h264_instance *h264, struct vpu_io_info *io)
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
	vpu_wrap_write_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_VTYPE, (0x2 << 6) + (1 << 4) + 0x1);
	vpu_wrap_write_reg(RADIX_EMC_BASE + RADIX_REG_EMC_ADDR_CHN, RADIX_VDMA_ACFG_DHA(io->des_phyaddr));
	vpu_wrap_write_reg(RADIX_CFGC_BASE+RADIX_REG_CFGC_ACM_CTRL, RADIX_VDMA_ACFG_RUN);

	//vpu intr
	if (io->bsfull_en) {
		return h264_wait_for_completion_bsfull(h264, io);
	} else {
		return h264_wait_for_completion(h264, io);
	}
}

static int h264_code_headers(struct h264_instance *h264, struct vpu_io_info *io)
{
	struct h264_header_info *header = &h264->header;
	struct h264_param *param = &h264->param;
	int i_nal_ref_idc = 0;
	int i_idr_pic_id = 0;
	int i_type = 0;
	int length = io->bslen;
	unsigned char *bit_ptr = (unsigned char *)io->bs_viraddr;
	int ret = 0;

	i264e_nal_init(&header->nal,
			(char *)io->vpu_stream_buffer + io->vpu_stream_buffer_occupancy,
			io->vpu_stream_buffer_len - io->vpu_stream_buffer_occupancy);
	if (!io->frm_num_gop || io->frame_type == H264_SLICE_TYPE_IDR) {
		//frame I
		bs_init(&header->bs, param->bs, param->bs_len);
		i264e_sps_write(&header->bs, &header->sps);
		ret = i264e_nal_start(&header->nal, &header->bs, I264E_NAL_SPS, I264E_NAL_PRIORITY_HIGHEST);
		if (ret < 0) {
			VPU_ERROR("i264e_nal_start failed\n");
			return VPU_BUFFULL;
		}

		bs_init(&header->bs, param->bs, param->bs_len);
		i264e_pps_write(&header->bs, &header->sps, &header->pps);
		ret = i264e_nal_start(&header->nal, &header->bs, I264E_NAL_PPS, I264E_NAL_PRIORITY_HIGHEST);
		if (ret < 0) {
			VPU_ERROR("i264e_nal_start failed\n");
			return VPU_BUFFULL;
		}
		i_nal_ref_idc = I264E_NAL_PRIORITY_HIGHEST;
		i_idr_pic_id = 0;
		i_type = I264E_NAL_SLICE_IDR;
	} else {
		//frame P
		i_nal_ref_idc = I264E_NAL_PRIORITY_HIGH;
		i_idr_pic_id = -1;
		i_type = I264E_NAL_SLICE;
	}
	bs_init(&header->bs, param->bs, param->bs_len);
	i264e_slice_header_init(&header->header, param, &header->sps, &header->pps, i_idr_pic_id, io->frm_num_gop, io->qp);
	i264e_slice_header_write(&header->bs, &header->header, i_nal_ref_idc);
	if (header->bs.i_left == 32) {
		i264e_nal_start_substream(&header->nal, bit_ptr, length);
		ret = i264e_nal_start(&header->nal, &header->bs, i_type, i_nal_ref_idc);
		if (ret < 0) {
			VPU_ERROR("i264e_nal_start failed\n");
			return VPU_BUFFULL;
		}
	} else {
		while(length--)
			bs_write(&header->bs, 8, *bit_ptr++);
		bs_rbsp_trailing(&header->bs);
		ret = i264e_nal_start(&header->nal, &header->bs, i_type, i_nal_ref_idc);
		if (ret < 0) {
			VPU_ERROR("i264e_nal_start failed\n");
			return VPU_BUFFULL;
		}
	}

	io->vpu_curr_stream_len = header->nal.i_occupancy;
	//printk("kernel encode %d curr len = %d\n", io->tot_frame_num, io->vpu_curr_stream_len);
	return 0;
}

int h264_code_headers_segment(struct h264_instance *h264, struct vpu_io_info *io, unsigned int bslen, int count, int is_final)
{
	struct h264_header_info *header = &h264->header;
	struct h264_param *param = &h264->param;
	int i_nal_ref_idc = 0;
	int i_idr_pic_id = 0;
	int i_type = 0;
	unsigned char *bit_ptr = NULL;
	int ret = 0;

	if (count == 0) {
		i264e_nal_init(&header->nal,
				(char *)io->vpu_stream_buffer + io->vpu_stream_buffer_occupancy,
				io->vpu_stream_buffer_len - io->vpu_stream_buffer_occupancy);
		if (!io->frm_num_gop || io->frame_type == H264_SLICE_TYPE_IDR) {
			//frame I
			bs_init(&header->bs, param->bs, param->bs_len);
			i264e_sps_write(&header->bs, &header->sps);
			ret = i264e_nal_start(&header->nal, &header->bs, I264E_NAL_SPS, I264E_NAL_PRIORITY_HIGHEST);
			if (ret < 0) {
				VPU_ERROR("i264e_nal_start failed\n");
				return VPU_BUFFULL;
			}

			bs_init(&header->bs, param->bs, param->bs_len);
			i264e_pps_write(&header->bs, &header->sps, &header->pps);
			ret = i264e_nal_start(&header->nal, &header->bs, I264E_NAL_PPS, I264E_NAL_PRIORITY_HIGHEST);
			if (ret < 0) {
				VPU_ERROR("i264e_nal_start failed\n");
				return VPU_BUFFULL;
			}
			i_nal_ref_idc = I264E_NAL_PRIORITY_HIGHEST;
			i_idr_pic_id = 0;
			i_type = I264E_NAL_SLICE_IDR;
		} else {
			//frame P
			i_nal_ref_idc = I264E_NAL_PRIORITY_HIGH;
			i_idr_pic_id = -1;
			i_type = I264E_NAL_SLICE;
		}
		bs_init(&header->bs, param->bs, param->bs_len);
		i264e_slice_header_init(&header->header, param, &header->sps, &header->pps, i_idr_pic_id, io->frm_num_gop, io->qp);
		i264e_slice_header_write(&header->bs, &header->header, i_nal_ref_idc);
	}

	if (header->bs.i_left == 32) {
		if (count == 0) {
			ret = i264e_nal_start(&header->nal, &header->bs, i_type, i_nal_ref_idc);
			if (ret < 0) {
				VPU_ERROR("i264e_nal_start failed\n");
				return VPU_BUFFULL;
			}
		}
		ret = i264e_nal_start_segment(&header->nal, &header->bs, (char *)io->bs_viraddr + (count % 2) * io->bsfull_size * 1024, bslen, is_final);
		if (ret < 0) {
			VPU_ERROR("i264e_nal_start_segment failed\n");
			return VPU_BUFFULL;
		}
	} else {
		if (!is_final) {
			bit_ptr = (char *)io->bs_viraddr + (count % 2) * io->bsfull_size * 1024;
			while(bslen--)
				bs_write(&header->bs, 8, *bit_ptr++);
		} else {
			bs_rbsp_trailing(&header->bs);
			ret = i264e_nal_start(&header->nal, &header->bs, i_type, i_nal_ref_idc);
			if (ret < 0) {
				VPU_ERROR("i264e_nal_start failed\n");
				return VPU_BUFFULL;
			}
		}
	}
}

int h264_encode(h264_t *inst, struct vpu_io_info *io)
{
	struct h264_instance *h264 = (struct h264_instance *)inst;
	int ret = 0;

	//init vdma
	i264e_cabac_context_init(h264->param.state, io->frm_num_gop ? I264E_SLICE_TYPE_P : I264E_SLICE_TYPE_I, io->qp, 0);
	memset(&h264->vdma_info, 0x0, sizeof(struct h264_slice_info));
	h264->vdma_info.state = h264->param.state;
	h264_set_slice_info(&h264->vdma_info, io);

	//start hw
	ret = h264_encode_start(h264, io);
	if (ret < 0) {
		VPU_ERROR("h264 hw encode failed\n");
		return ret;
	}

	return 0;
}
