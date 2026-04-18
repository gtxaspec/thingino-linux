#include "h264_core.h"
#include "vpu_common.h"
#include "h264_vpi.h"
#include "h264_cabac.h"
#include "h264_header.h"
#include "hera.h"
#include <linux/completion.h>
#include <linux/jiffies.h>

//#define DUMP_HERA_REG
extern struct completion *hera_done_for_kernEnc;
extern int hera_irq_stat_for_kernEnc;
extern void H264E_T32V_SliceInit(struct h264_slice_info *s);
static int h264_code_headers(struct h264_instance *h264, struct vpu_io_info *io);
static int h264_code_headers_segment(struct h264_instance *h264, struct vpu_io_info *io, unsigned int bslen, int count, int is_final);

#ifdef DUMP_HERA_REG
static void dump_hera_regs()
{
	int i = 0;
	int offset = 0;

#define DUMP_HERA_BASE 0x13200000
#define DUMP_IVDC_BASE 0x13100000
	for (i = 0; i < 0xfc; i ++) {
		offset = RADIX_CFGC_BASE + i * 4;
		printk(KERN_ERR "CFGC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x0c; i ++) {
		offset = RADIX_VDMA_BASE + i * 4;
		printk(KERN_ERR "VDMA:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x67; i ++) {
		offset = RADIX_ODMA_BASE + i * 4;
		printk(KERN_ERR "ODMA:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x3a0; i ++) {
		offset = RADIX_EFE_BASE + i * 4;
		printk(KERN_ERR "EFE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x10c; i ++) {
		offset = RADIX_JRFD_BASE + i * 4;
		printk(KERN_ERR "JRFD:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x7c; i ++) {
		offset = RADIX_MCE_BASE + i * 4;
		printk(KERN_ERR "MCE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}
	for (i = 0x100; i < 0x1bc; i ++) {
		offset = RADIX_MCE_BASE + i * 4;
		printk(KERN_ERR "MCE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x80; i ++) {
		offset = RADIX_TFM_BASE + i * 4;
		printk(KERN_ERR "TFM:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x290; i ++) {
		offset = RADIX_MD_BASE + i * 4;
		printk(KERN_ERR "MD:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x8; i ++) {
		offset = RADIX_DT_BASE + i * 4;
		printk(KERN_ERR "DT:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x98; i ++) {
		offset = RADIX_DBLK_BASE + i * 4;
		printk(KERN_ERR "DBLK:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}
	for (i = 0x240; i < 0x248; i ++) {
		offset = RADIX_DBLK_BASE + i * 4;
		printk(KERN_ERR "DBLK:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0x20; i < 0x34; i ++) {
		offset = RADIX_SAO_BASE + i * 4;
		printk(KERN_ERR "SAO:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x8; i ++) {
		offset = RADIX_BC_BASE + i * 4;
		printk(KERN_ERR "BC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x68; i ++) {
		offset = RADIX_SDE_BASE + i * 4;
		printk(KERN_ERR "SDE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x13c; i ++) {
		offset = RADIX_IPRED_BASE + i * 4;
		printk(KERN_ERR "IPRED:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x4; i ++) {
		offset = RADIX_STC_BASE + i * 4;
		printk(KERN_ERR "STC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}
	for (i = 0x1000; i < 0x1004; i ++) {
		offset = RADIX_STC_BASE + i * 4;
		printk(KERN_ERR "STC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x1d4; i ++) {
		offset = RADIX_VMAU_BASE + i * 4;
		printk(KERN_ERR "VMAU:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x68; i ++) {
		offset = RADIX_EMC_BASE + i * 4;
		printk(KERN_ERR "EMC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x200; i ++) {
		offset = i * 4;
		printk(KERN_ERR "IVDC:::0x%08x = 0x%08x\n", DUMP_IVDC_BASE + offset, *(volatile int *)(0xb3100000 + offset));
	}
}
#endif

static int h264_set_slice_info(struct h264_slice_info *si, struct vpu_io_info *io)
{
	int i = 0;
	int qp_idx = 0;
	int qpt_size   = 0;
	int mod_size   = 0;
	int cps_size   = 0;
	int ai_size    = 0;
	int mcec_size  = 0;
	int frame_idx = 0;
	unsigned int ddr_y_base = 0;
	unsigned int ddr_c_base = 0;
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
	h264_ifa_para_t *ifa = &si->ifa_para;

	int emc_mb_width   = 0;
	int emc_mb_height  = 0;
	int emc_mb_num     = 0;

	unsigned int emc_ofst = 0;
	unsigned int emc_mem_tmp = 0;

	/* h264_api_enc_init */
	si->frame_type          = io->frame_type == H264_SLICE_TYPE_IDR ? H264_SLICE_TYPE_IDR : H264_SLICE_TYPE_P;
	si->frm_num_gop         = io->frm_num_gop;
	//---------------------------------------
	//     bjmb
	//---------------------------------------
	si->buf_share_en     = 1;
	si->buf_share_size = 1;
	si->bsfull_intr_en                = io->bsfull_en;
	si->bsfull_intr_size              = io->bsfull_size;
	si->maxnum_ref       = 1;
	C_CLIP3(1, 2, si->maxnum_ref);

	/* h264_api_enc_not_supported */
#ifdef CONFIG_SOC_PRJ007
	ifa->still_times_en   = 1;
	ifa->max_still_times  = 5;
	si->rm.skip_en                 = 1;
	si->rm.dcm_param               = 0x4304;
	si->rm.diff_cplx_thd    = 10;
	RC_MEMCPY(si->ned_sad_thr,                 const_ned_sad_t32v_thr);

	// ifa
	si->rm.r_max_qp = ifa->max_qp;
	si->rm.r_min_qp = ifa->min_qp;
	ifa->sobel_en = 1;
	ifa->sobel_edge_thrd = 130;
	ifa->ery_en = 1;
	ifa->qpg_pet_fil_en = 1;

	si->rm.cps_mode = 1; // down scale factor. 0: 2x2, 1: 4x4
	si->rm.rrs_mode = 1;
	si->rm.rrs_of_en = 0;

	ifa->skin_en = 1;
	ifa->skin_cnt_thrd = 30;
	ifa->skin_lvl_en[0] = ifa->skin_en & 1;
	ifa->skin_lvl_en[1] = ifa->skin_en & 1;
	ifa->skin_lvl_en[2] = ifa->skin_en & 1;
	memcpy(ifa->qpg_skin_qp_ofst, const_skin_qp_ofst, sizeof(char) * 3);
	memcpy(ifa->skin_level, const_skin_level, sizeof(short) * 4);
	memcpy(ifa->skin_thrd, const_skin_thrd, sizeof(unsigned char) * 3 * 2 * 2);
	memcpy(ifa->skin_mul_factor, const_skin_mul_factor, sizeof(unsigned char) * 4);

	memcpy(ifa->petbc2_var_thr, const_petbc2_var_thr, sizeof(unsigned short) * 3);
	memcpy(ifa->petbc2_ssm_thr, const_petbc2_ssm_thr, sizeof(unsigned short) * 3 * 4);

	memcpy(ifa->qpg_sobel_cplx_thrd, const_sobel_cu16_thrd, sizeof(short) * 8);
	memcpy(ifa->qpg_pet_cplx_thrd, const_qpg_pet_cplx_thrd, sizeof(unsigned char) * 3 * 12);
	memcpy(ifa->qpg_pet_qp_ofst, const_qpg_pet_qp_ofst, sizeof(unsigned char) * 5 * 7);
	memcpy(ifa->qpg_pet_qp_ofst_mt, const_qpg_pet_qp_ofst_mt, sizeof(unsigned char) * 5 * 7);
	memcpy(ifa->qpg_pet_qp_idx, const_qpg_pet_qp_idx, sizeof(unsigned char) * 5);
#elif defined(CONFIG_SOC_PRJ008)
	si->color_shadow_sse_motion_shift = 5;
	RC_MEMCPY(si->ned_sad_thr, const_ned_sad_thr);
	// ifa
	ifa->frm_mt_smd_thr = 22;
	ifa->row_smd_unit = si->frame_height > 2048 ? 1 : 0;
	// bjmb
	si->jrfcv2_en = 0;
	si->body_combine_en = 0;
	// other
	si->nal_en = 0;
#endif

	//---------------------------------------
	//     from header
	//---------------------------------------
	si->cqp_offset          = 0;
	si->use_intra_in_pframe = 1;
	si->is_scaling_custom   = 1;
	si->dct8x8_en           = 1;
	si->deblock             = 1;
	si->alpha_c0_offset     = 0;
	si->beta_offset         = 0;
	//---------------------------------------
	//     base cfg
	//---------------------------------------
	si->mb_width            = (io->width + 15) / 16;
	si->mb_height           = (io->height + 15) / 16;
	si->frame_width         = C_ALIGN(io->width, 16);
	si->frame_height        = C_ALIGN(io->height, 16);
	si->frame_y_stride      = si->frame_width;
	si->frame_c_stride      = si->frame_height;
	si->hw_width            = C_ALIGN(io->width, 16);
	si->hw_height           = C_ALIGN(io->height, 16);
	si->first_mby           = 0;
	si->last_mby            = si->mb_height - 1; //for multi-slice
	si->raw_format          = 8; //fix value
	si->rotate_mode         = 0; //fix value
								 //---------------------------------------
								 //     emc addr(3)
								 //---------------------------------------
	si->emc_cps_en          = 1; //fix value
	si->emc_mix_en          = 0;
	si->emc_mix_addr        = NULL;

	/* h264_api_emc_addr */
	emc_mb_width   = si->mb_width;
	emc_mb_height  = si->mb_height;
	emc_mb_num     = emc_mb_width * emc_mb_height;

	cps_size   = emc_mb_num*24;     cps_size   = C_ALIGN(cps_size,   256);
	qpt_size   = emc_mb_num;        qpt_size   = C_ALIGN(qpt_size,   256);
	mod_size   = emc_mb_num*4;      mod_size   = C_ALIGN(mod_size,   256);
	ai_size    = emc_mb_num*4;      ai_size    = C_ALIGN(ai_size,    256);
	mcec_size  = (emc_mb_num+1)/2;  mcec_size  = C_ALIGN(mcec_size,  256);

	emc_ofst = 0;
	emc_mem_tmp = (int)io->vpu_phyaddr + 0x2000;

#ifdef CONFIG_SOC_PRJ007
	ai_size = 0;
	mcec_size  = 0;//C_ALIGN(cu16_num*4,  256);
#endif
	si->emc_cps_addr     = (unsigned char  *)(emc_mem_tmp + emc_ofst); emc_ofst  += cps_size;
	si->emc_qpt_addr     = (unsigned int *)(emc_mem_tmp + emc_ofst); emc_ofst  += qpt_size;
	si->emc_mdc_addr     = (unsigned int *)(emc_mem_tmp + emc_ofst); emc_ofst  += mod_size;
	si->emc_ai_mark_addr = (unsigned int *)(emc_mem_tmp + emc_ofst); emc_ofst  += ai_size;
	si->emc_mcec_addr    = (unsigned char  *)(emc_mem_tmp + emc_ofst); emc_ofst  += mcec_size;
	ifa->emc_cps_ref0    = si->emc_cps_addr;

	//---------------------------------------
	//     ifa cfg
	//--------------------------------------
	ifa->ifa_en             = 1; //fix value
	ifa->qpg_en             = 1; //fix value
	ifa->qpg_smd_en         = 1; //fix value
	ifa->cps_en             = 1; //fix value
	ifa->cps_c_en           = 1; //fix value
	ifa->smd_en             = 1; //fix value
	ifa->smd_c_en           = 1; //fix value
	ifa->petbc_en           = 1; //fix value
	ifa->qpg_filte_en       = ifa->petbc_en && ifa->qpg_smd_en;
	ifa->crp_thrd           = 2;
	ifa->rrs_thrd           = 4;
	ifa->rrs_thrd_c         = 2;
	ifa->motion_thrd        = 16;
	ifa->motion_of_thrd     = 12;
	ifa->motion_of_thrd_c   = 3;
	ifa->motion_thrd_c      = 16;
	ifa->rrc_motion_thr[0]  = 8;
	ifa->rrc_motion_thr[1]  = 12;
	ifa->rrc_motion_thr[2]  = 17;
	ifa->rrc_motion_thr[3]  = 22;
	ifa->rrc_motion_thr[4]  = 32;
	ifa->rrc_motion_thr[5]  = 42;
	ifa->rrc_motion_thr[6]  = 64;
	RC_MEMCPY(ifa->qpg_smd_cplx_thrd, const_smd_cu16_thrd);
	RC_MEMCPY(ifa->petbc_var_thr,     const_petbc_var_thr);
	RC_MEMCPY(ifa->petbc_ssm_thr,     const_petbc_ssm_thr);
	RC_MEMCPY(ifa->pet_filter_valid,  const_pet_filter_valid);

	//---------------------------------------
	//     ipred
	//---------------------------------------
	si->bit_16_en       = 0;
	si->bit_8_en        = 1;
	si->bit_4_en        = 1;
	si->bit_uv_en       = 0;
	for (i = 0; i < 4; ++i) {
		si->bit_4[i]  = 4;
		si->bit_8[i]  = 4;
		si->bit_16[i] = 0;
		si->bit_uv[i] = 0;
	}
	//change lambda weight
	si->lamb_16_en      = 0;
	si->lamb_8_en       = 0;
	si->lamb_4_en       = 0;
	si->lamb_uv_en      = 0;
	si->lambda_info16   = 0;
	si->lambda_info8    = 0;
	si->lambda_info4    = 0;
	si->lambda_infouv   = 0;
	//constant cost weight
	si->c_16_en         = 0;
	si->c_8_en          = 0;
	si->c_4_en          = 0;
	si->c_uv_en         = 0;
	RC_MEMSET(si->const_16, 0);
	RC_MEMSET(si->const_uv, 0);
	RC_MEMSET(si->const_4,  0);
	RC_MEMSET(si->const_8,  0);
	//when same cost, increase the priority of mode
	si->pri_16          = 0;
	si->pri_8           = 0;
	si->pri_4           = 0;
	si->pri_uv          = 0;
	//when cur mode is same with neighbour, decrease the bit of mode
	si->ref_neb_4       = 1;
	si->ref_neb_8       = 1;
	si->ref_4           = 4;
	si->ref_8           = 3;
	//---------------------------------------
	//     mce
	//---------------------------------------
	si->hpel_en         = 1;
	si->qpel_en         = 1;
	si->max_sech_step_i = 63;
	si->max_mvrx_i      = 64;
	si->max_mvry_i      = 32;
	si->scl             = 3; //fix value
	si->glb_mv_en       = 0;
	si->glb_mvx         = 0;
	si->glb_mvy         = 0;
	if(si->buf_share_en ) {
		if(si->jrfcv2_en ) {
			if(si->max_mvry_i > (si->buf_share_size*64 - 8))
				si->max_mvry_i = (si->buf_share_size*64 - 8);
		}
		else {
			if(si->max_mvry_i > (si->buf_share_size*64 + 8))
				si->max_mvry_i = (si->buf_share_size*64 + 8);
		}
	}
	//---------------------------------------
	//     md
	//---------------------------------------
	si->force_i16dc      = 0;
	si->i16dc_cplx_thd   = io->frame_type == H264_SLICE_TYPE_IDR ? 6 : 3;
	si->i16dc_qp_base    = 33;
	si->i16dc_qp_sel     = 0;
	si->force_i16        = 1;
	si->i16_qp_base      = io->frame_type == H264_SLICE_TYPE_IDR ? 25 : 28;
	si->i16_qp_sel       = 0;
	si->cplx_thd_sel     = 0;
	si->refresh_en       = 0;//si->frame_type == H264_SLICE_TYPE_P ? 0 : 0;
	si->refresh_mode     = 0;
	si->refresh_bias     = 10;//fix value
	si->refresh_cplx_thd = 4;
	si->inter_nei_en     = 0;
	si->ned_motion_en    = 1;
	si->ned_motion_shift = 7;
	si->c_sad_min_bias   = 0;
	si->pskip_en         = 1; //fix value
	si->skip_bias_en     = 0;
	si->info_en          = 1;
	si->mvd_sum_all      = 0;
	si->mvd_sum_abs      = 0;
	si->mv_sum_all       = 0;
	si->mv_sum_abs       = 0;
	si->cfg_size_x       = 0x4;
	si->cfg_size_y       = 0x4;
	si->cfg_iw_thr       = 0;
	si->cfg_mvr_thr1     = 0;
	si->cfg_mvr_thr2     = 0;
	si->cfg_mvr_thr3     = 0;
	si->ysse_thr         = 0;
	si->csse_thr         = 0;
	si->jm_lambda2_en    = 0;
	RC_MEMCPY(si->ned_sad_thr_cpx_step,        const_ned_sad_thr_cpx_step);
	RC_MEMCPY(si->cplx_thd_idx,                cnst_cplx_thd_idx);
	RC_MEMCPY(si->cplx_thd,                    cnst_cplx_thd);
	RC_MEMCPY(si->colow_shadow_sse_ratio_thrd, const_colow_shadow_sse_ratio_thrd);
	//---------------------------------------
	//     others
	//---------------------------------------
	si->fbc_ep            = 204;
	si->sde_prior         = 5;
	si->db_prior          = 5;

	/* get_base_cfg */
	qp_idx = (io->qp < 20 ? 0 :
			io->qp < 27 ? 1 :
			io->qp < 33 ? 2 :
			io->qp < 39 ? 3 :
			io->qp < 42 ? 4 :
			io->qp < 46 ? 5 : 6);
	ifa->base_qp            = io->qp;
	ifa->efe_rrs_en         = (si->frame_type == H264_SLICE_TYPE_P);
	ifa->rrs_c_en           = ifa->efe_rrs_en;
	ifa->motion_en          = ifa->efe_rrs_en;
	ifa->motion_c_en        = ifa->efe_rrs_en;
	si->mref_en             = si->maxnum_ref == 2 && si->frm_num_gop > 1;
	si->mce_pref_mode       = si->frame_type == 0 ? 0 : 3;
	si->mode_ctrl           = 0;
	si->color_shadow_sse_en = 0;

	RC_MEMCPY( ifa->qpg_cplx_qp_ofst,       const_cplx_qp_ofst[qp_idx]);
	RC_MEMCPY( ifa->qpg_ai_motion_smd_ofst, const_qpg_ai_motion_smd_ofst);

#ifdef CONFIG_SOC_PRJ007
	si->rrs_en                 = (si->frame_type != H264_SLICE_TYPE_IDR);
	si->rm.rrs_of_en           = ifa->efe_rrs_en;
	si->rm.frame_idx           = si->frm_num_gop;
	si->rm.acmask_mode         = 0;
	si->rm.qp                  = ifa->base_qp;
	qp_idx = ifa->base_qp < 30 ? 0 : ifa->base_qp < 42 ? 1 : 2;
	memcpy(ifa->qpg_dlt_thr,        const_qpg_dlt_thr[qp_idx],sizeof(unsigned char)*4);

	si->recon_resi_en       = 0;
#elif defined(CONFIG_SOC_PRJ008)
	si->recon_resi_en       = 1;
	si->acmask_en           = 0;
	si->acmask_type         = 1;
#endif

	/* get_enc_func_api_cfg */
	ifa->qpg_table_en = 0;
	ifa->qpg_roi_en = 0;
	ifa->ai_mark_en = 0;
	ifa->ai_mark_type = 0;
	si->mce_cfg_en = 0;
	si->md_cfg_en = 0;
	si->rm.mb_mode_use = si->md_cfg_en;

	/* get_rc_control_api_cfg */
	si->rc_en = 0;
	ifa->max_qp             = io->qp+25;
	ifa->min_qp             = io->qp;
	C_CLIP3(1,51,ifa->max_qp);
	C_CLIP3(1,51,ifa->min_qp);
	ifa->qpg_max_qp          = ifa->max_qp;
	ifa->qpg_min_qp          = ifa->min_qp;
	si->rc_min_qp = ifa->min_qp;
	si->rc_max_qp = ifa->max_qp;

	/* encode.c */
	si->des_va = io->des_viraddr;
	si->des_pa = io->des_phyaddr;
	if (si->bsfull_intr_en) {
		si->emc_bs_addr0 = (unsigned char *)io->bs_phyaddr;
		si->emc_bs_addr1 = (unsigned char *)io->bs_phyaddr + io->bsfull_size * 1024;
	} else {
		si->emc_bs_addr0 = (unsigned char *)io->bs_phyaddr;
	}

	if (io->is_ivdc) {
		si->frame_y_stride = 4096;
		si->frame_c_stride = 4096;
	} else {
		si->frame_y_stride = io->y_stride;
		si->frame_c_stride = io->c_stride;
	}

/* get_buf_share_cfg */
	if(si->buf_share_en) {
		frame_idx = si->frm_num_gop;
		ddr_y_base = (unsigned int)(io->rds_y_phyaddr + 255) & ~255;
		ddr_c_base = (unsigned int)(io->rds_c_phyaddr + 255) & ~255;

		//align
		frame_width   = (si->frame_width  + 63) & ~63;
#ifdef CONFIG_SOC_PRJ007
		frame_height = (si->frame_height + 63) & ~63;
#elif defined(CONFIG_SOC_PRJ008)
		frame_height = (si->frame_height + 15) & ~15;
#endif

		//share buf
		beyond_size   = (si->buf_share_size + 1) * 64;
		jump_size     = (si->rm.jrfcd_flag || si->jrfc_en || si->jrfcv2_en) ? si->buf_share_size * 64 : beyond_size;//buf share with jrfc
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

		if(si->frame_type != H264_SLICE_TYPE_IDR) {
			last_y_ofst_addr = io->s_last_y_ofst_addr;                              //cur ref y ofst addr
			last_c_ofst_addr = io->s_last_c_ofst_addr;                              //cur ref c ofst addr
			y_ofst_addr      = (io->s_last_y_ofst_addr + y_every_space ) % y_space; //cur rec y ofst addr
			c_ofst_addr      = si->body_combine_en ? 0 : (io->s_last_c_ofst_addr + c_every_space ) % c_space; //cur rec c ofst addr
			si->buf_start_yaddr  = y_ofst_addr      ? (si->buf_beyond_yaddr - y_ofst_addr)      : ddr_y_base;
			si->buf_start_caddr  = c_ofst_addr      ? (si->buf_beyond_caddr - c_ofst_addr)      : ddr_c_base;
			si->buf_ref_yaddr    = last_y_ofst_addr ? (si->buf_beyond_yaddr - last_y_ofst_addr) : ddr_y_base;
			si->buf_ref_caddr    = last_c_ofst_addr ? (si->buf_beyond_caddr - last_c_ofst_addr) : ddr_c_base;

			io->s_last_y_ofst_addr  = y_ofst_addr;
			io->s_last_c_ofst_addr  = c_ofst_addr;
		} else {
			io->s_last_y_ofst_addr  = 0;
			io->s_last_c_ofst_addr  = 0;
		}

		si->buf_ref_rem_mby = ((si->buf_beyond_yaddr - si->buf_ref_yaddr) / (frame_width)) / 64 - 1;
	} else {
		si->buf_ref_rem_mby = 0x3F;//mce计算bufshare尾地址使用，t32v需要使用整行数；t33不再使用
	}
#if 0
	si->dst_y_pa = io->dec_y_phyaddr;
	si->dst_c_pa = io->dec_c_phyaddr;
	si->ref_y_pa = io->ref_y_phyaddr;
	si->ref_c_pa = io->ref_c_phyaddr;
#endif

	if (io->is_ivdc) {
		si->raw_y_pa = 0x80000000;
		si->raw_c_pa = 0x81000000;
	} else {
		si->raw_y_pa = io->raw_y_phyaddr;
		si->raw_c_pa = io->raw_c_phyaddr;
	}
	si->frame_type = (si->frame_type != H264_SLICE_TYPE_IDR);

#ifdef CONFIG_SOC_PRJ008
	memset(&si->rm, 0, sizeof(h264_rm_t));
#endif
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
	int bs_err_cnt = 0;

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
					bs_err_cnt ++;
				}
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
				i ++;
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_BSFULL) && (bslen_delta < 0)) {
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) && (i > 0)) {
				vpu_wrap_flush_cache(io->bs_viraddr, CACHE_ALL_SIZE, 2);
				if ((ret_bs = h264_code_headers_segment(h264, io, bslen - i * io->bsfull_size * 1024, i, 1)) < 0) {
					VPU_ERROR("h264 encode segment error\n");
					bs_err_cnt ++;
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

	if (bs_err_cnt) {
		return ret_bs;
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
#ifdef CONFIG_SOC_PRJ008
	vpu_wrap_write_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_EARB, (61 & 0xFF));
#endif
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
	return 0;
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
#if 0
	static int iii = 0;
	if(iii++ < 1)
		dump_t32v_h264_sliceinfo_test(&h264->vdma_info, NULL);
#endif

	//start hw
	ret = h264_encode_start(h264, io);
	if (ret < 0) {
		VPU_ERROR("h264 hw encode failed\n");
		return ret;
	}

	return 0;
}
