#ifndef _H265_VPI_H_
#define _H265_VPI_H_

static unsigned char vpi_func_cfg[12][2] = {
	{0,0}, //0 ipred
	{0,0}, //1 mce_size
	{0,0}, //2 mce_ctrl
	{0,0}, //3 blk
	{0,0}, //4 tfm
	{0,0}, //5 csd
	{0,0}, //6 mos
	{0,0}, //7 csh
	{0,0}, //8 dlbda
	{0,0}, //9 su
	{0,0}, //10 fmd
	{0,0}, //11 qpg
};
static unsigned char ipred_thrd_cfg[1][2] = {
	{0,0}, //0:ipred
};
static unsigned char mce_thrd_cfg[1][2] = {
	{0,0}, //0:mce
};
static unsigned char md_thrd_cfg[9][2] = {
	{0,0}, //0:tus
	{0,0}, //1:srd
	{0,0}, //2:csd
	{0,0}, //3:mos
	{0,0}, //4:ned
	{0,0}, //5:csh
	{0,0}, //6:dlbda
	{0,0}, //7:mvp_force_zero
	{0,0}, //8:pmdctrl
};
//------------------------------------------
//    1.vpi func enable cfg
//------------------------------------------
static unsigned char emc_blk_cfg[5][2] = {
	{0, 0},//0 ip_cfg_en
	{0, 0},//1 mce_cfg_en
	{0, 0},//2 md_cfg_en
	{0, 0},//3 emc_ai_en
	{0, 0},//4 bufull_intr_en
};
static unsigned char ipred_cfg[7][2] = {
	{1,1}, //0 i_pmd_en
	{1,1}, //1 ipmd_mpm_use_maxnum
	{0,0}, //4 frm_ipmd_angle_en
	{1,1}, //5 sobel_en
	{3,3}, //6 ppred_mode_num
	{7,7}, //7 ipred_size
	{7,7}, //8 sobel_size
};
static unsigned int mce_ctrl[2]  ={
	0x9990, 0x9990, //inter_mode
};
static unsigned short mce_cfg0[8][2] = {
	{1, 1},     //0 half_pixel_sech_en
	{1, 1},     //1 quart_pixel_sech_en
	{1, 1},     //2 mrg_en
	{0, 0},     //3 mrg_fpel_en
	{1, 1},     //4 force_dia_en
	{13,13},    //5 max_sech_step
	{256, 256}, //6 max_mvrx
	{256, 256}, //7 max_mvry
};
static unsigned char tfm_cfg[4][2] = {
	{0,0}, //0 deadzone_en
	{0,0}, //1 acmask_en
	{0,0}, //2 rdoq_en
	{0,0}, //3 acmask_type
};
static unsigned char md_csd_cfg[6][2] = {
	{1, 1},//0 i_csd_en
	{1, 1},//1 p_csd_en
	{0, 0},//2 csd_color
	{0, 0},//3 csd_p_cu64_en
	{0, 0},//4 ccf_rm_en
	{1, 1},//5 rcn_plain_en
};
static unsigned char md_csh_cfg[6][2] = {
	{1, 1},//0 ned_en
	{1, 1},//1 ned_motion_en
	{0, 0},//2 color_shadow_en
	{1, 1},//3 color_shadow_sse_en
	{1, 1},//4 color_shadow_sse_priority_en
	{0, 0},//5 color_shadow_sse_value_thrd_en
};
static unsigned char md_su_cfg[5][2] = {
	{1, 1},//0 inter_sa8d_en
	{0, 0},//2 pmdsu_en
	{0, 0},//3 pmdctrl_en
	{0, 0},//4 blk_pmdctrl_en
};
static unsigned char md_fmode_cfg[5][2] = {
	{1, 1},//0 mvp_force_zero_en
	{0, 0},//srd_en
};
static unsigned char md_mos_cfg[2][2] = {
	{1, 1},//0 mosaic_en
	{1, 1},//1 mosaic_intra_texture_en
};
static unsigned char md_dlbda_cfg[5][2] = {
	{0, 0},//0 dlambda_framelevel_en
	{0, 0},//1 dlambda_ipmd_en
	{1, 1},//2 dlambda_ppmd_en
	{1, 1},//3 dlambda_icsd_en
	{1, 1},//4 dlambda_pcsd_en
};
static unsigned char qpg_cfg[5][2] = {
	{1, 1},//0 qpg smd
	{0, 0},//1 qpg sobel
	{0, 0},//2 qpg_petbc en
	{0, 0},//3 qpg_table_en
	{1, 1},//4 qpg_filte_en
};
//------------------------------------------
//    2.ipred cfg
//------------------------------------------
static unsigned char cnst_ipred_angle_bias[2][4] = {
	{1, 31, 30, 0},
	{1, 31, 30, 0},
};
static unsigned char cnst_frmb_idx[2][5] = {
	{0,0,0,0,0},
	{0,0,0,0,0},
};
static unsigned char cnst_frmb_ip_bits_en[2][5] = {
	{1,1,1,1,1},
	{1,1,1,1,1},
};
static unsigned char cnst_frmb_ip_bias[2][5][2] = {
	{
		{16,16},{16,16},{16,16},{16,16},{16,16},
	},
	{
		{16,16},{16,16},{16,16},{16,16},{16,16},
	},
};
static unsigned char cnst_frmb_ipc_bias[2][5] = {
	{16,16,16,16,16},
	{16,16,16,16,16},
};
static unsigned char cnst_intra_bits[2][16][5]   =   {
	{
		{ 1, 2, 5, 0, 4},
		{ 2, 4, 6, 1, 8},
		{ 0, 0, 0, 0, 0},
		{ 15, 15, 15, 15, 15},
		{ 0, 15, 15, 0, 15},
		{ 1, 2, 3, 1, 2},
		{ 2, 4, 6, 4, 2},
		{ 8, 6, 4, 3, 5},
		{ 6, 8, 10, 6, 7},
		{ 1, 2, 3, 4, 5},
		{ 2, 4, 6, 8, 10},
		{ 4, 8, 15, 15, 8},
		{ 3, 3, 3, 3, 3},
		{ 7, 7, 7, 7, 7},
		{ 10, 15, 5, 5, 10},
		{ 5, 10, 15, 10, 5},
	},
	{
		{ 1, 2, 5, 0, 4},
		{ 2, 4, 6, 1, 8},
		{ 0, 0, 0, 0, 0},
		{ 15, 15, 15, 15, 15},
		{ 0, 15, 15, 0, 15},
		{ 1, 2, 3, 1, 2},
		{ 2, 4, 6, 4, 2},
		{ 8, 6, 4, 3, 5},
		{ 6, 8, 10, 6, 7},
		{ 1, 2, 3, 4, 5},
		{ 2, 4, 6, 8, 10},
		{ 4, 8, 15, 15, 8},
		{ 3, 3, 3, 3, 3},
		{ 7, 7, 7, 7, 7},
		{ 10, 15, 5, 5, 10},
		{ 5, 10, 15, 10, 5},
	}
};
static unsigned char cnst_ipmd_mpm_cost_bias[2][3][2][4] = {
	{
		{{ 27, 27, 29, 32}, { 29, 29, 30, 32}},
		{{ 27, 27, 29, 32}, { 29, 29, 30, 32}},
		{{ 27, 27, 29, 32}, { 29, 29, 30, 32}},
	},
	{
		{{ 27, 27, 29, 32}, { 29, 29, 30, 32}},
		{{ 27, 27, 29, 32}, { 29, 29, 30, 32}},
		{{ 27, 27, 29, 32}, { 29, 29, 30, 32}},
	},
};
static unsigned char cnst_ipmd_dm_cost_bias[2][3][5] = {
	{
		{ 32, 32, 32, 32, 32},
		{ 32, 32, 32, 32, 32},
		{ 32, 32, 32, 32, 32},
	},
	{
		{ 32, 32, 32, 32, 32},
		{ 32, 32, 32, 32, 32},
		{ 32, 32, 32, 32, 32},
	},
};
//------------------------------------------
//    3. mce cfg
//------------------------------------------
static unsigned char mce_cfg2[2][6] = {
	//0:mce_scl_mode
	//1:refine_cu8_mode
	//2:refine_cu16_mode
	//3:refine_cu32_mode
	//4:mref_wei
	//5:merge_cand_num
	{3, 0, 0, 0, 0, 2},
	{3, 0, 0, 0, 0, 2},
};

//------------------------------------------
//    4. md cfg
//------------------------------------------
static unsigned char md_srd_cfg[2][5] = {
	{0, 0, 1, 5, 0x1d},
	{0, 0, 1, 5, 0x1d}
};
unsigned char cnst_ppmd_merge_thr[2][4][5][2] = {
	{
		{{ 55, 36},{ 55, 36},{ 55, 36},{ 52, 30},{ 50, 30}},
		{{ 57, 38},{ 57, 38},{ 57, 38},{ 54, 32},{ 53, 31}},
		{{ 59, 41},{ 59, 41},{ 59, 41},{ 57, 33},{ 56, 32}},
		{{ 60, 41},{ 60, 41},{ 62, 41},{ 59, 35},{ 59, 33}}
	},
	{
		{{ 57, 45},{ 57, 45},{ 57, 45},{ 57, 30},{ 56, 30}},
		{{ 59, 45},{ 59, 45},{ 59, 45},{ 58, 32},{ 57, 32}},
		{{ 60, 45},{ 60, 45},{ 60, 45},{ 59, 33},{ 59, 33}},
		{{ 62, 45},{ 62, 45},{ 62, 45},{ 60, 35},{ 60, 35}}
	}

};
unsigned char cnst_ppmd_search_thr[2][3][5][2]= {
	{
		{{ 57, 38},{ 57, 38},{ 57, 38},{ 54, 32},{ 53, 31}},
		{{ 59, 41},{ 59, 41},{ 59, 41},{ 57, 33},{ 56, 32}},
		{{ 60, 41},{ 60, 41},{ 62, 41},{ 59, 35},{ 59, 33}}
	},
	{
		{{ 59, 45},{ 59, 45},{ 59, 45},{ 58, 32},{ 57, 32}},
		{{ 60, 45},{ 60, 45},{ 60, 45},{ 59, 33},{ 59, 33}},
		{{ 62, 45},{ 62, 45},{ 62, 45},{ 60, 35},{ 60, 35}}
	}
};
unsigned char cnst_ppmd_merge_search_thr[2][3][5][2]= {
	{
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}}
	},
	{
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}}
	}
};
unsigned char cnst_ppmd_intra_thr[2][3][5][2]= {
	{
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}}
	},
	{
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}},
		{{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51},{ 76, 51}}
	}
};
unsigned char cnst_pmd_qp_thrd[7] = {52,52,52,52,52,52,52};
unsigned char cnst_ppmd_cost_thr[4] = {1,1,1,1};
unsigned char cnst_ppmd_thr_list[30][2] = {
	{55,36}, {52,30}, {50,30}, {57,38}, {54,32},
	{53,31}, {59,41}, {57,33}, {56,32}, {60,41},
	{62,41}, {59,35}, {59,33}, {57,45}, {57,30},
	{56,30}, {59,45}, {58,32}, {57,32}, {60,45},
	{62,45}, {60,35}, {76,51}, {0,0},   {0,0},
	{0,0},   {0,0},   {0,0},   {0,0},   {0,0}
};
unsigned char cnst_ppmd_merge_thr_idx[40] = {
	0,  0,  0,  1,  2,  3,  3,  3,  4,  5,
	6,  6,  6,  7,  8,  9,  9, 10, 11, 12,
	13, 13, 13, 14, 15, 16, 16, 16, 17, 18,
	19, 19, 19, 12, 12, 20, 20, 20, 21, 21,
};
unsigned char cnst_ppmd_search_thr_idx[30] = {
	3,  3,  3,  4,  5,  6,  6,  6,  7,  8,
	9,  9, 10, 11, 12, 16, 16, 16, 17, 18,
	19, 19, 19, 12, 12, 20, 20, 20, 21, 21,
};
unsigned char cnst_ppmd_merge_search_thr_idx[30] = {
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
};
unsigned char cnst_ppmd_intra_thr_idx[30] = {
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
};

static unsigned char md_csd_cfg2[2][2] = {
	{150, 1},
	{150, 1},
};

unsigned char cnst_csd_qp_thrd[2][7] = {
	{20,27,33,39,42,46,52},
	{20,27,33,39,42,46,52},
};
unsigned char cnst_var_flat_recon_thrd[2][2][7] = {
	{
		{ 30, 25, 20, 15, 10, 10, 5},
		{ 20, 20, 20, 10, 10,  5, 5}
	},
	{
		{ 30, 25, 20, 15, 10, 10, 5},
		{ 20, 20, 20, 10, 10,  5, 5}
	}
};

static unsigned char md_ned_cfg2[2][2] = {
	{7, 5},
	{7, 5},
};
unsigned char cnst_ned_sad_thr[2][2][2][4] = {
	{
		{
			{ 255, 255, 255, 0},
			{ 255, 255, 255, 0}
		},
		{
			{ 15, 15, 0, 0},
			{ 25, 25, 0, 0}
		}
	},
	{
		{
			{ 255, 255, 255, 0},
			{ 255, 255, 255, 0}
		},
		{
			{ 15, 15, 0, 0},
			{ 25, 25, 0, 0}
		}
	}
};
unsigned short cnst_ned_sad_thr_cpx_step[2][2][3] = {
	{
		{ 0, 0, 0},{ 0, 0, 0}
	},
	{
		{ 0, 0, 0},{ 0, 0, 0}
	}
};
unsigned char cnst_ned_score_table[2][2][2][8] = {
	{
		{
			{ 16, 24, 32, 48, 64, 128, 254},
			{ 0, 4, 6, 8, 10, 12, 16, 20}},
		{
			{ 16, 24, 32, 48, 64, 128, 254},
			{ 0, 4, 6, 8, 10, 12, 16, 20}
		},
	},
	{
		{
			{ 16, 24, 32, 48, 64, 128, 254},
			{ 0, 4, 6, 8, 10, 12, 16, 20}},
		{
			{ 16, 24, 32, 48, 64, 128, 254},
			{ 0, 4, 6, 8, 10, 12, 16, 20}
		},
	}

};
unsigned char cnst_ned_score_bias[2][2] = {
	{32,32},
	{32,32},
};
static unsigned char md_mos_cfg2[2][1] = {
	{5},
	{5},
};
static unsigned char cnst_mosaic_recon_flat_thr[2][4] = {
	{3,3,3,3},
	{3,3,3,3}
};
unsigned char cnst_mosaic_diff_thr[2][4] = {
	{3,3,3,3},
	{3,3,3,3}
};
static unsigned char chromaScale_ofst_radix[64] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   //  0 -  9
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 10 - 19
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 20 - 29
	1, 1, 1, 1, 1, // 30 - 34
	2, 2, 3, 3, 4, 4, 5, 5,  // 35 - 42
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,   // 43 - 52
};
unsigned short cnst_recon_resi_thr[2][2][4][2] = {
	{
		{{ 32, 16},{ 0, 0}, { 0, 0}, { 0, 0}},
		{{ 16, 8}, { 0, 0}, { 0, 0}, { 0, 0}}
	},
	{
		{{ 32, 16},{ 0, 0}, { 0, 0}, { 0, 0}},
		{{ 16, 8}, { 0, 0}, { 0, 0}, { 0, 0}}
	},

};
unsigned short cnst_ifa_raw_thr[2][8][2] = {
	{
		{   8,  4},
		{ 128, 64},
		{   8,  4},
		{ 128, 64},
		{  32, 16},
		{ 128, 64},
		{  48, 16},
		{   8,  4}
	},
	{
		{   8,  4},
		{ 128, 64},
		{   8,  4},
		{ 128, 64},
		{  32, 16},
		{ 128, 64},
		{  48, 16},
		{   8,  4}
	}
};
unsigned char cnst_recon_resi_en[2][4][2] = {
	{
		{ 1, 0},
		{ 1, 0},
		{ 1, 0},
		{ 1, 0}
	},
	{
		{ 1, 0},
		{ 1, 0},
		{ 1, 0},
		{ 1, 0}
	},
};

static unsigned char cnst_intra_resi_en[2][3] = {
	{0, 0, 0},
	{0, 0, 0},
};

unsigned char tup_resi_qp_tbl[2][8] = {
	{30, 35, 40, 45, 50, 64},
	{16, 18, 20, 22, 24, 26},
};

//------------------------------------------
//    5.ifa cfg
//------------------------------------------
//petbc
unsigned short cnst_petbc_var_thr[3] =  {0,0,256};
unsigned short cnst_petbc2_var_thr[3] = {0,0,250};
unsigned short cnst_petbc_ssm_thr[3][4]  = {
	{ 0, 150, 0, 0},
	{ 5, 50,  5, 5},
	{ 5, 50,  5, 5},
};
unsigned short cnst_petbc2_ssm_thr[3][4] = {
	{ 0, 145, 0, 0},
	{ 6, 53, 6, 6},
	{ 6, 53, 6, 6}
};
unsigned char cnst_pet_filter_valid[4] = {1,0,1,0};
//qpg petbc
unsigned char cnst_qpg_pet_qp_idx[5] = {0,0,0,0,0};
unsigned char cnst_qpg_pet_cplx_thrd[3][12] = {
	{ 10, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0},
	{ 20, 30, 40, 50, 60, 70, 80, 100, 120, 140, 170, 170},
	{ 6, 10, 16, 22, 30, 40, 50, 60, 70, 80, 90, 90}
};
unsigned char cnst_qpg_pet_qp_ofst[5][7] = {
	{ 6, 5, 4, 3, 2, 2, 1},
	{ 6, 5, 4, 3, 3, 2, 2},
	{ 2, 2, 2, 3, 4, 5, 6},
	{ 2, 2, 3, 4, 5, 6, 6},
	{ 2, 2, 3, 4, 5, 5, 5},
};
unsigned char cnst_qpg_pet_qp_ofst_mt[5][7] =   {
	{ 6, 5, 4, 3, 2, 2, 1},
	{ 6, 5, 4, 3, 3, 2, 2},
	{ 2, 2, 2, 3, 4, 5, 6},
	{ 2, 2, 3, 4, 5, 6, 6},
	{ 2, 2, 3, 4, 5, 5, 5},
};
unsigned char cnst_qpg_dlt_thr[4] ={5,8,8,8};
//ifa_v31
unsigned short cnst_smd_blk8_thr[6] = {8,128,8,128,32,128};
unsigned short cnst_sobel_blk16_thr[3] = {11155, 5577, 43};
unsigned short cnst_sobel_blk32_thr[3] = {11155, 5577, 43};
unsigned short cnst_smd_blk16_thr[8][2] = {
	{ 8,   4}, { 128, 64}, {8, 4}, {128, 64},
	{ 32, 16}, { 128, 64}, {8, 4}, {  0,  0}
};
unsigned short cnst_smd_blk32_thr[3][2] = {{ 32, 16}, { 128, 64}, { 8, 4}};
//qpg sobel and smd
static short  cnst_qpg_smd_thrd[8] = {500,1000,2500,5000,8000,14000,20000,25000};
static short  cnst_qpg_sobel_thrd[8] = {160,350,450,1260,2700,5000,6000,8000};
static char cnst_qpg_cplx_qp_ofst[7][8] = {
	{-1, 0, 1, 1, 2, 3, 4, 4},//if frm qp: [0,19],use this
	{-1, 0, 1, 1, 2, 3, 4, 4},//[20.26]
	{-1, 0, 1, 2, 3, 3, 4, 4},//[27.33]
	{-1, 0, 1, 2, 3, 3, 4, 4},//[33,38]
	{-4,-3,-1, 0, 1, 2, 3, 4},//[39,41]
	{-5,-4,-2,-1, 0, 2, 3, 4},//[42,45]
	{-7,-5,-4,-2, 0, 2, 3, 4} //[56,51]
};
//ai
static char cnst_ai_motion_smd_ofst[3][2][8] = {
	{{0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}},
	{{1,1,2,2,2,3,3,3}, {1,1,2,2,2,3,3,3}},
	{{1,1,2,2,2,3,3,3}, {1,1,2,2,2,3,3,3}}
};
static char cnst_motion_smd_ofst[3][2][8] = {
	{{0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}},
	{{0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}},
	{{0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}}
};
//color shadow
static unsigned char md_color_shadow_cfg2[2][3] = {
	{7, 35, 5},
	{7, 35, 5},
};
static unsigned char cnst_color_shadow_sad_thrd[2][2][5] = {
	{
		{10,15,70,7,12},
		{20,30,85,15,25}
	},
	{
		{10,15,70,7,12},
		{20,30,85,15,25}
	},
};
//color shadow sse
static unsigned char cnst_color_shadow_sse_ratio_thrd[2][2] = {
	{4,4},
	{4,4},
};
static unsigned int cnst_color_shadow_sse_value_thrd[2][3] = {
	{1<<8,1<<10,1<<12},
	{1<<8,1<<10,1<<12}
};
//qpg skin
static char   cnst_qpg_skin_qp_ofst[3] = {-3,-2,-1};
//frame cplx
static unsigned short cnst_frm_cplx_thrd[2] = {500, 680};
//lambda
static char md_dlbda_cfg2[2][7] = {
	{-10, -3, 3, -5, 0, 3, -2},
	{-10, -3, 3, -5, 0, 3, -2},
};
static unsigned char md_dlbda_cfg3[2][3] = {
	{5, 2, 44},
	{5, 2, 44},
};

static unsigned int cnst_lambda_cfg[2][11] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
};//hera_test

static unsigned char md_mvp_force_zero_cfg2[2][1] = {
	{7},
	{7},
};

static unsigned char md_pmdctrl_cfg2[2][1] = {
	{0},
	{0},
};

//refresh
static unsigned char  cnst_filter_thrd[4] = {6,3,4,2};
//skin
static short  cnst_skin_level[4] = {97,183,271,884};
static unsigned char  cnst_skin_mul_factor[4] = {12, 13, 19, 41};
static unsigned char  cnst_skin_thrd[3][2][2] = {
	{{100,130},{140,175}},
	{{40,110}, {120,155}},
	{{120,160},{165,225}}
};
//var flat
unsigned char cnst_var_flat_thr[2][2] = {{ 10, 10},{ 20, 20}};


unsigned short cnst_rc_thd_0[12][3] = {
	{1, 3, 5},//rc_thd[0]
	{2, 4, 6},
	{3, 5, 7},
	{4, 6, 8},
	{5, 7, 9},
	{6, 8, 10},
	{7, 9, 11},
	{8, 10,12},
	{9, 11,13},
	{10,12,14},
	{11,13,15},
	{12,14,16},
};
unsigned short cnst_rc_thd_1[12][3] = {
	{1, 3, 5},//rc_thd[0]
	{2, 4, 6},
	{3, 5, 7},
	{4, 6, 8},
	{5, 7, 9},
	{6, 8, 10},
	{7, 9, 11},
	{8, 10,12},
	{9, 11,13},
	{10,12,14},
	{11,13,15},
	{12,14,16},
};

unsigned char cnst_bu_size_idx[9] = {1,2,4,6,8,10,20,40,64};
char cnst_bu_dqp_limit[2][12] = {
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
	{-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12},
};
#endif //_H265_VPI_H_
