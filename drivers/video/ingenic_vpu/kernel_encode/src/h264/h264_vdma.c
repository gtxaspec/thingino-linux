#include "hera.h"
#include "h264_core.h"
#include <linux/kernel.h>

#define MB_WID ((s->frame_width + 15) / 16)
#define MB_HEI ((s->frame_height + 15) / 16)
#define VALID_VAL3(vbit, bit, var) (((var) & (vbit)) << (bit))
#define VALID_VAL2(bit, var)       ((var) << (bit))
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

#if 0
void H264e_sort(unsigned int *addr, unsigned int *val, int n, int module_idx)
{
	int i,j,m;
	unsigned int a,v;
	for(i=0;i<n-1;i++) {
		for(j=0;j<n-i-1;j++) {
			if((addr[j]&0x7FFF) > (addr[j+1]&0x7FFF)) {
				a        = addr[j];
				addr[j]  = addr[j+1];
				addr[j+1] = a;
				v        = val[j];
				val[j]   = val[j+1];
				val[j+1] = v;
			}
		}
	}
}
void H264e_SliceInit_Dump_Chn(_H264E_SliceInfo *s, unsigned int *chn, unsigned int chn_size)
{
	FILE *chn_fp = fopen("264_t33_chn_dump.log", "a+");
	int i,m,k;
	int chn_num=0;
	unsigned int RADIX_CFGC_BASE_VDMA  = (RADIX_HID_M0 << 15);
	unsigned int RADIX_VDMA_BASE_VDMA  = (RADIX_HID_M1 << 15);
	unsigned int RADIX_ODMA_BASE_VDMA  = (RADIX_HID_M2 << 15);
	unsigned int RADIX_TMC_BASE_VDMA   = (RADIX_HID_M3 << 15);
	unsigned int RADIX_EFE_BASE_VDMA   = (RADIX_HID_M4 << 15);
	unsigned int RADIX_JRFD_BASE_VDMA  = (RADIX_HID_M5 << 15);
	unsigned int RADIX_MCE_BASE_VDMA   = (RADIX_HID_M6 << 15);
	unsigned int RADIX_TFM_BASE_VDMA   = (RADIX_HID_M7 << 15);
	unsigned int RADIX_MD_BASE_VDMA    = (RADIX_HID_M8 << 15);
	unsigned int RADIX_DT_BASE_VDMA    = (RADIX_HID_M9 << 15);
	unsigned int RADIX_DBLK_BASE_VDMA  = (RADIX_HID_M10 << 15);
	unsigned int RADIX_SAO_BASE_VDMA   = (RADIX_HID_M11 << 15);
	unsigned int RADIX_BC_BASE_VDMA    = (RADIX_HID_M12 << 15);
	unsigned int RADIX_SDE_BASE_VDMA   = (RADIX_HID_M13 << 15);
	unsigned int RADIX_IPRED_BASE_VDMA = (RADIX_HID_M14 << 15);
	unsigned int RADIX_STC_BASE_VDMA   = (RADIX_HID_M15 << 15);
	unsigned int RADIX_VMAU_BASE_VDMA  = (RADIX_HID_M16 << 15);
	unsigned int RADIX_EMC_BASE_VDMA   = (RADIX_HID_M18 << 15);
	int cnt[17]={0};
	unsigned int rst[17][2][512];
	for(i=0;i<chn_size/4;i+=2) {
		unsigned int hid = (chn[i+1] >> 15) & 0x1F;
		switch(hid) {
			case RADIX_HID_M1 :  rst[0][0][cnt[0]]   = chn[i+1]; rst[0][1][cnt[0]]   = chn[i]; cnt[0]++; break;
			case RADIX_HID_M2 :  rst[1][0][cnt[1]]   = chn[i+1]; rst[1][1][cnt[1]]   = chn[i]; cnt[1]++; break;
			case RADIX_HID_M3 :  rst[2][0][cnt[2]]   = chn[i+1]; rst[2][1][cnt[2]]   = chn[i]; cnt[2]++; break;
			case RADIX_HID_M4 :  rst[3][0][cnt[3]]   = chn[i+1]; rst[3][1][cnt[3]]   = chn[i]; cnt[3]++; break;
			case RADIX_HID_M5 :  rst[4][0][cnt[4]]   = chn[i+1]; rst[4][1][cnt[4]]   = chn[i]; cnt[4]++; break;
			case RADIX_HID_M6 :  rst[5][0][cnt[5]]   = chn[i+1]; rst[5][1][cnt[5]]   = chn[i]; cnt[5]++; break;
			case RADIX_HID_M7 :  rst[6][0][cnt[6]]   = chn[i+1]; rst[6][1][cnt[6]]   = chn[i]; cnt[6]++; break;
			case RADIX_HID_M8 :  rst[7][0][cnt[7]]   = chn[i+1]; rst[7][1][cnt[7]]   = chn[i]; cnt[7]++; break;
			case RADIX_HID_M9 :  rst[8][0][cnt[8]]   = chn[i+1]; rst[8][1][cnt[8]]   = chn[i]; cnt[8]++; break;
			case RADIX_HID_M10:  rst[9][0][cnt[9]]   = chn[i+1]; rst[9][1][cnt[9]]   = chn[i]; cnt[9]++; break;
			case RADIX_HID_M11:  rst[10][0][cnt[10]] = chn[i+1]; rst[10][1][cnt[10]] = chn[i]; cnt[10]++;break;
			case RADIX_HID_M12:  rst[11][0][cnt[11]] = chn[i+1]; rst[11][1][cnt[11]] = chn[i]; cnt[11]++;break;
			case RADIX_HID_M13:  rst[12][0][cnt[12]] = chn[i+1]; rst[12][1][cnt[12]] = chn[i]; cnt[12]++;break;
			case RADIX_HID_M14:  rst[13][0][cnt[13]] = chn[i+1]; rst[13][1][cnt[13]] = chn[i]; cnt[13]++;break;
			case RADIX_HID_M15:  rst[14][0][cnt[14]] = chn[i+1]; rst[14][1][cnt[14]] = chn[i]; cnt[14]++;break;
			case RADIX_HID_M16:  rst[15][0][cnt[15]] = chn[i+1]; rst[15][1][cnt[15]] = chn[i]; cnt[15]++;break;
			case RADIX_HID_M18:  rst[16][0][cnt[16]] = chn[i+1]; rst[16][1][cnt[16]] = chn[i]; cnt[16]++;break;
			default:printf("chn[%d]: error(hid=%d)\n",i/2, hid);
		}
	}
	char *module_str[17] = {"VDMA","ODMA","TMC","EFE","JRFD","MCE","TFM","MD","DT","DBLK","SAO","BC","SDE","IPRED","STC","VMAU","EMC"};

	unsigned int efe_addr[20] = {0xc, //REG_EFE_RAWY_ADDR               0x000c
		0x10,//REG_EFE_RAWC_ADDR               0x0010
	};
	unsigned int mce_addr[20] = {0x70, //RADIX_REG_MCE_BFSH_ADRY       0x0070
		0x74, //RADIX_REG_MCE_BFSH_ADRC       0x0074
		0x78, //T33_RADIX_REG_MCE_BYND_YADDR      0x0078
		0x7C, //T33_RADIX_REG_MCE_BYND_CADDR      0x007C
		0x800,//SLUT_MCE_RLUT(l, i)     (0x0800 + (i)*8 + (l)*0x80)
		0x804,//SLUT_MCE_RLUT(l, i)     (0x0800 + (i)*8 + (l)*0x80)
		0x1B8,//RADIX_REG_MCE_MREF_ADRY       0x01B8
		0x1BC,//RADIX_REG_MCE_MREF_ADRC       0x01BC
	};
	unsigned int odma_addr[20] = {0x8, //REG_ODMA_BDYA                        0x0008
		0xc, //REG_ODMA_BDCA                        0x000C
		0x20,//REG_ODMA_HDYA                        0x0020
		0x24,//REG_ODMA_HDCA                        0x0024
		0x28,//T33_REG_JRFC_YSPACE                  0x0028
		0x2c,//T33_REG_JRFC_CSPACE                  0x002c
		0x30,//REG_ODMA_BUFS_BASEY_ADDR		  0x0030
		0x34,//REG_ODMA_BUFS_BASEC_ADDR		  0x0034
		0x38,//REG_ODMA_BUFS_BEYDY_ADDR		  0x0038
		0x3c,//REG_ODMA_BUFS_BEYDC_ADDR		  0x003c
	};
	unsigned int jrfd_addr[20] = {0x8,//REG_JRFD_HDYA                   0x0008
		0xc,//REG_JRFD_HDCA                   0x000C
		0x14,//REG_JRFD_BDYA                   0x0014
		0x18,//REG_JRFD_BDCA                   0x0018
		0x20,//REG_JRFD_BUFS_BASEY_ADDR        0x0020
		0x24,//REG_JRFD_BUFS_BASEC_ADDR        0x0024
		0x28,//REG_JRFD_BUFS_BEYDY_ADDR        0x0028
		0x2c,//REG_JRFD_BUFS_BEYDC_ADDR        0x002C
		0x30,//REG_JRFD_MHDY                   0x0030
		0x34,//REG_JRFD_MHDC                   0x0034
		0x38,//REG_JRFD_MBDY                   0x0038
		0x3c,//REG_JRFD_MBDC                   0x003C
	};
	unsigned int emc_addr[20] = {0x8,  //HERA_REG_EMC_ADDR_BS0           0x8
		0xc,  //HERA_REG_EMC_ADDR_CPS           0xc
		0x10, //HERA_REG_EMC_ADDR_SOBEL         0x10
		0x14, //HERA_REG_EMC_ADDR_MIX           0x14
		0x1c, //HERA_REG_EMC_ADDR_BS1           0x1c
		0x20, //HERA_REG_EMC_ADDR_FLAG          0x20
		0x28, //HERA_REG_EMC_ADDR_QPT           0x28
		0x2c, //HERA_REG_EMC_ADDR_MCEC          0x2c
		0x30, //HERA_REG_EMC_ADDR_IPC           0x30
		0x38, //HERA_REG_EMC_ADDR_RRS0          0x38
		0x50, //HERA_REG_EMC_ADDR_AI            0x50
		0x54, //HERA_REG_EMC_ADDR_MIXC          0x54
	};
	int odma_addr_cnt = 10;
	int mce_addr_cnt  = 8;
	int efe_addr_cnt  = 2;
	int jrfd_addr_cnt = 12;
	int emc_addr_cnt  = 12;

	static int cont_frm = 0;
	for(m=0;m<17;m++) {

		fprintf(chn_fp,"M[%d] = %d------------frm=%d.\n",m, cnt[m],cont_frm);
		//        printf("M[%d] = %d\n",m, cnt[m]);
		if(cnt[m] > 1)
			H264e_sort(&rst[m][0][0],  &rst[m][1][0],  cnt[m],   m);
		if(m==1) { //odma
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<odma_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == odma_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					fprintf(chn_fp,"[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					fprintf(chn_fp,"[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==3) {//efe
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<efe_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == efe_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					fprintf(chn_fp,"[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					fprintf(chn_fp,"[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==4){//jrfd
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<jrfd_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == jrfd_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					fprintf(chn_fp,"[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					fprintf(chn_fp,"[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==5){//mce
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<mce_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == mce_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					fprintf(chn_fp,"[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					fprintf(chn_fp,"[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==16){//emc
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<emc_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == emc_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					fprintf(chn_fp,"[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					fprintf(chn_fp,"[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else {
			for(i=0;i<cnt[m];i++)  {
				fprintf(chn_fp,"[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
	}
	/*
	   for(m=0;m<17;m++) {
	   printf("M[%d] = %d\n",m, cnt[m]);
	   if(cnt[m] > 1)
	   H264e_sort(&rst[m][0][0],  &rst[m][1][0],  cnt[m],   m);
	   if(cnt[m] > 0) {
	   for(i=0;i<cnt[m];i++)  {
	   fprintf(chn_fp,"[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
	   }
	   }
	   }
	   */
	cont_frm++;
	fclose(chn_fp);
}
#endif

void H264E_T32V_SliceInit(struct h264_slice_info *s)
{
	int checksum = 0;
	unsigned int i, j;
	volatile unsigned int *chn = (volatile unsigned int *)s->des_va;
	h264_ifa_para_t *ifa = &s->ifa_para;

	//unsigned int RADIX_CFGC_BASE_VDMA  = (RADIX_HID_M0 << 15);
	unsigned int RADIX_VDMA_BASE_VDMA  = (RADIX_HID_M1 << 15);
	unsigned int RADIX_ODMA_BASE_VDMA  = (RADIX_HID_M2 << 15);
	unsigned int RADIX_TMC_BASE_VDMA   = (RADIX_HID_M3 << 15);
	unsigned int RADIX_EFE_BASE_VDMA   = (RADIX_HID_M4 << 15);
	unsigned int RADIX_JRFD_BASE_VDMA  = (RADIX_HID_M5 << 15);
	unsigned int RADIX_MCE_BASE_VDMA   = (RADIX_HID_M6 << 15);
	unsigned int RADIX_TFM_BASE_VDMA   = (RADIX_HID_M7 << 15);
	unsigned int RADIX_MD_BASE_VDMA    = (RADIX_HID_M8 << 15);
	//unsigned int RADIX_DT_BASE_VDMA    = (RADIX_HID_M9 << 15);
	unsigned int RADIX_DBLK_BASE_VDMA  = (RADIX_HID_M10 << 15);
	//unsigned int RADIX_SAO_BASE_VDMA   = (RADIX_HID_M11 << 15);
	//unsigned int RADIX_BC_BASE_VDMA    = (RADIX_HID_M12 << 15);
	unsigned int RADIX_SDE_BASE_VDMA   = (RADIX_HID_M13 << 15);
	unsigned int RADIX_IPRED_BASE_VDMA = (RADIX_HID_M14 << 15);
	//unsigned int RADIX_STC_BASE_VDMA   = (RADIX_HID_M15 << 15);
	unsigned int RADIX_VMAU_BASE_VDMA  = (RADIX_HID_M16 << 15);
	unsigned int RADIX_EMC_BASE_VDMA   = (RADIX_HID_M18 << 15);

	/**************************************************
	  buf share cfg
	 *************************************************/
	//unsigned int buf_addr_group[8];
	//unsigned char buf_ref_mby_size;
	//unsigned char buf_odma_spe_flag;
	//unsigned char buf_odma_alg_flag;

	unsigned int buf_beyond_yaddr = s->buf_beyond_yaddr;
	unsigned int buf_beyond_caddr = s->buf_beyond_caddr;
	unsigned int buf_start_yaddr  = s->buf_start_yaddr;
	unsigned int buf_start_caddr  = s->buf_start_caddr;
	unsigned int refy_addr_ba0    = s->buf_ref_yaddr;
	//unsigned int refy_addr_ba1    = s->buf_base_yaddr;
	unsigned int refc_addr_ba0    = s->buf_ref_caddr;
	//unsigned int refc_addr_ba1    = s->buf_base_caddr;
	//unsigned char  ref_mby_size     = s->buf_ref_rem_mby;
	//unsigned char  odma_spe_flag    = 0;
	unsigned char  odma_alg_flag    = 0;

	/*************************************************************************************
	  EFE Module
	 *************************************************************************************/
	unsigned int is_264               = 1;
	unsigned int raw_avg_en           = 1;
	unsigned int emc_out_en[2]        = {1,1};
	unsigned int rrs1_en              = 0;
	unsigned int refresh_en           = 0;
	unsigned int rrs1_c_en            = 0;
	unsigned int sobel_16_or_14       = 1;
	unsigned int cplx_qp[2]           = {0};
	unsigned int smd_cplx_thrd[4]     = {0};
	unsigned int sobel_cplx_thrd[4]   = {0};
	unsigned int skin_thrd[3]         = {0};
	unsigned int skin_lvl[2]          = {0};
	unsigned int pet_cplx_thrd[3][3]  = {0};
	unsigned int pet_qp_ofst[5]       = {0};
	unsigned int pet_qp_ofst_mt[5]    = {0};
	unsigned int cfm_cplx_thrd_cfg[2] = {0};
	unsigned int roi_ctrl_cfg[4]      = {0};
	unsigned int roi_pos_cfg[16]      = {0};

#ifdef CONFIG_SOC_PRJ007
	unsigned int efe_width            = (s->rotate_mode == 1 || s->rotate_mode == 3) ? s->frame_width  : s->frame_height;
	unsigned int efe_height           = (s->rotate_mode == 1 || s->rotate_mode == 3) ? s->frame_height : s->frame_width;
#elif defined(CONFIG_SOC_PRJ008)
	unsigned int efe_width            = (s->rotate_mode == 1 || s->rotate_mode == 3) ? s->hw_width  : s->hw_height;
	unsigned int efe_height           = (s->rotate_mode == 1 || s->rotate_mode == 3) ? s->hw_height : s->hw_width;
#endif
	unsigned int efe_stride           = EFE_RAW_STRDY(s->frame_y_stride) | EFE_RAW_STRDC(s->frame_y_stride);
	unsigned int efe_frm_size    = (0
			+ VALID_VAL2 (16, efe_width)
			+ VALID_VAL2 ( 0, efe_height));

	int max_outstanding_num = 16;
	unsigned int efe_ctrl            = ( 0
			+ VALID_VAL3 (0x1,  0, 0)
			+ VALID_VAL3 (0x1,  1, 1)
			+ VALID_VAL3 (0x1,  3, s->frame_type)
			+ VALID_VAL3 (0x1,  4, 1)
			+ VALID_VAL3 (0x1, 11, 1)
			+ VALID_VAL3 (0x1, 12, s->md_cfg_en)
			+ VALID_VAL3 (0x1, 14, s->mce_cfg_en)
			+ VALID_VAL3 (0x3, 30, s->rotate_mode));
	unsigned int ifac_all_en         = ( 0
			+ VALID_VAL2 (2, ifa->smd_c_en)
			+ VALID_VAL2 (3, ifa->motion_c_en)
			+ VALID_VAL2 (4, ifa->rrs_c_en)
			+ VALID_VAL2 (5, ifa->cps_c_en)
			+ VALID_VAL2 (6, rrs1_c_en));
	unsigned int ifac_all1_en        = ( 0
			+ VALID_VAL2 ( 0, ifa->rrs_thrd_c)
			+ VALID_VAL2 ( 4, ifa->motion_of_thrd_c)
			+ VALID_VAL2 (12, ifa->motion_thrd_c));
	unsigned int petbc_filter_valid  = ( 0
			+ VALID_VAL3 (0x1, 3, ifa->pet_filter_valid[0])
			+ VALID_VAL3 (0x1, 2, ifa->pet_filter_valid[1])
			+ VALID_VAL3 (0x1, 1, ifa->pet_filter_valid[2])
			+ VALID_VAL3 (0x1, 0, ifa->pet_filter_valid[3]));
	unsigned int ai_motion_smd_ofst0 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[0][0][0])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[0][0][1])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[0][0][2])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[0][0][3])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[0][0][4]));
	unsigned int ai_motion_smd_ofst1 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[0][0][5])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[0][0][6])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[0][0][7])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[0][1][0])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[0][1][1]));
	unsigned int ai_motion_smd_ofst2 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[0][1][2])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[0][1][3])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[0][1][4])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[0][1][5])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[0][1][6]));
	unsigned int ai_motion_smd_ofst3 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[0][1][7])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[1][0][0])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[1][0][1])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[1][0][2])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[1][0][3]));
	unsigned int ai_motion_smd_ofst4 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[1][0][4])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[1][0][5])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[1][0][6])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[1][0][7])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[1][1][0]));
	unsigned int ai_motion_smd_ofst5 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[1][1][1])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[1][1][2])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[1][1][3])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[1][1][4])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[1][1][5]));
	unsigned int ai_motion_smd_ofst6 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[1][1][6])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[1][1][7])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[2][0][0])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[2][0][1])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[2][0][2]));
	unsigned int ai_motion_smd_ofst7 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[2][0][3])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[2][0][4])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[2][0][5])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[2][0][6])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[2][0][7]));
	unsigned int ai_motion_smd_ofst8 = ( 0
			+ VALID_VAL3 (0x3f, 24, ifa->qpg_ai_motion_smd_ofst[2][1][0])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_ai_motion_smd_ofst[2][1][1])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[2][1][2])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[2][1][3])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[2][1][4]));
	unsigned int ai_motion_smd_ofst9 = ( 0
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_ai_motion_smd_ofst[2][1][5])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_ai_motion_smd_ofst[2][1][6])
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_ai_motion_smd_ofst[2][1][7]));
	unsigned int ifa_cfg1            = ( 0
			+ VALID_VAL2 (2, rrs1_en)
			+ VALID_VAL2 (1, ifa->pet_mode));
	unsigned int rfh_cfg1            = refresh_en;
	unsigned int qpg_cfg1            = ( 0
			+ VALID_VAL3 (0x1,  22, s->rc_en)
			+ VALID_VAL3 (0x3f, 16, s->rc_min_qp)
			+ VALID_VAL3 (0x3f, 10, s->rc_max_qp)
			+ VALID_VAL3 (0x1,   9, s->rc_dly_en)
			+ VALID_VAL3 (0x1,   8, ifa->ai_mark_en)
			+ VALID_VAL3 (0xf,   4, ifa->ai_mark_type)
			+ VALID_VAL3 (0x1,   3, ifa->qpg_roi_en)
			+ VALID_VAL3 (0x1,   2, ifa->qpg_filte_en)
			+ VALID_VAL3 (0x1,   0, ifa->qpg_petbc_en));
	unsigned int skin_factor         = ( 0
			+ VALID_VAL2 ( 0, ifa->skin_mul_factor[0])
			+ VALID_VAL2 ( 8, ifa->skin_mul_factor[1])
			+ VALID_VAL2 (16, ifa->skin_mul_factor[2])
			+ VALID_VAL2 (24, ifa->skin_mul_factor[3]));
	unsigned int rrs_cfg             = ( 0
			+ VALID_VAL2 ( 0, ifa->efe_rrs_en)
			+ VALID_VAL2 ( 1, s->rm.rrs_mode)//T33=0
			+ VALID_VAL2 ( 2, s->rm.rrs_of_en)//T33=0
			+ VALID_VAL2 ( 3, ifa->motion_en)
			+ VALID_VAL2 ( 4, ifa->rrs_thrd)
			+ VALID_VAL2 ( 8, ifa->motion_of_thrd)
			+ VALID_VAL2 (16, ifa->motion_thrd));
	unsigned int pet_var_thr         = ( 0
			+ VALID_VAL3 (0x1ff,  0, ifa->petbc_var_thr[2])
			+ VALID_VAL3 (0x1ff,  9, ifa->petbc_var_thr[1])
			+ VALID_VAL3 (0x1ff, 18, ifa->petbc_var_thr[0]));
	unsigned int pet_ssm_thr0        = ( 0
			+ VALID_VAL3 (0xff, 0, ifa->petbc_ssm_thr[0][1])
			+ VALID_VAL3 (0xff, 8, ifa->petbc_ssm_thr[0][0]));
	unsigned int pet_ssm_thr1        = ( 0
			+ VALID_VAL3 (0xff,  0, ifa->petbc_ssm_thr[1][3])
			+ VALID_VAL3 (0xff,  8, ifa->petbc_ssm_thr[1][2])
			+ VALID_VAL3 (0xff, 16, ifa->petbc_ssm_thr[1][1])
			+ VALID_VAL3 (0xff, 24, ifa->petbc_ssm_thr[1][0]));
	unsigned int pet_ssm_thr2        = ( 0
			+ VALID_VAL3 (0xff,  0, ifa->petbc_ssm_thr[2][3])
			+ VALID_VAL3 (0xff,  8, ifa->petbc_ssm_thr[2][2])
			+ VALID_VAL3 (0xff, 16, ifa->petbc_ssm_thr[2][1])
			+ VALID_VAL3 (0xff, 24, ifa->petbc_ssm_thr[2][0]));
	unsigned int pet2_var_thr        = ( 0
			+ VALID_VAL3 (0x1ff,  0, ifa->petbc2_var_thr[2])
			+ VALID_VAL3 (0x1ff,  9, ifa->petbc2_var_thr[1])
			+ VALID_VAL3 (0x1ff, 18, ifa->petbc2_var_thr[0]));
	unsigned int pet2_ssm_thr0       = ( 0
			+ VALID_VAL3 (0xff, 0, ifa->petbc2_ssm_thr[0][1])
			+ VALID_VAL3 (0xff, 8, ifa->petbc2_ssm_thr[0][0]));
	unsigned int pet2_ssm_thr1       = ( 0
			+ VALID_VAL3 (0xff,  0, ifa->petbc2_ssm_thr[1][3])
			+ VALID_VAL3 (0xff,  8, ifa->petbc2_ssm_thr[1][2])
			+ VALID_VAL3 (0xff, 16, ifa->petbc2_ssm_thr[1][1])
			+ VALID_VAL3 (0xff, 24, ifa->petbc2_ssm_thr[1][0]));
	unsigned int pet2_ssm_thr2       = ( 0
			+ VALID_VAL3 (0xff,  0, ifa->petbc2_ssm_thr[2][3])
			+ VALID_VAL3 (0xff,  8, ifa->petbc2_ssm_thr[2][2])
			+ VALID_VAL3 (0xff, 16, ifa->petbc2_ssm_thr[2][1])
			+ VALID_VAL3 (0xff, 24, ifa->petbc2_ssm_thr[2][0]));
	unsigned int rrc_motion_thr0     = ( 0
			+ VALID_VAL3 (0xffff,  0, ifa->rrc_motion_thr[0])
			+ VALID_VAL3 (0xffff, 16, ifa->rrc_motion_thr[1]));
	unsigned int rrc_motion_thr1     = ( 0
			+ VALID_VAL3 (0xffff,  0, ifa->rrc_motion_thr[2])
			+ VALID_VAL3 (0xffff, 16, ifa->rrc_motion_thr[3]));
	unsigned int rrc_motion_thr2     = ( 0
			+ VALID_VAL3 (0xffff,  0, ifa->rrc_motion_thr[4])
			+ VALID_VAL3 (0xffff, 16, ifa->rrc_motion_thr[5]));
	unsigned int rrc_motion_thr3     = ( 0
			+ VALID_VAL3 (0xffff,  0, ifa->rrc_motion_thr[6]));
	unsigned int pet_qp_idx          = ( 0
			+ VALID_VAL3 (0x1f,  0, ifa->qpg_pet_qp_idx[0])
			+ VALID_VAL3 (0x1f,  5, ifa->qpg_pet_qp_idx[1])
			+ VALID_VAL3 (0x1f, 10, ifa->qpg_pet_qp_idx[2])
			+ VALID_VAL3 (0x1f, 15, ifa->qpg_pet_qp_idx[3])
			+ VALID_VAL3 (0x1f, 20, ifa->qpg_pet_qp_idx[4]));
	unsigned int qpg_dlt_thrd        = ( 0
			+ VALID_VAL3 (0x3f,  0, ifa->qpg_dlt_thr[0])
			+ VALID_VAL3 (0x3f,  6, ifa->qpg_dlt_thr[1])
			+ VALID_VAL3 (0x3f, 12, ifa->qpg_dlt_thr[2])
			+ VALID_VAL3 (0x3f, 18, ifa->qpg_dlt_thr[3]));
	unsigned int cfm_cfg0            = ( 0
			+ VALID_VAL3 (0x1,   0, s->force_i16)
			+ VALID_VAL3 (0x1,   1, s->i16_qp_sel)
			+ VALID_VAL3 (0x1,   2, s->cplx_thd_sel)
			+ VALID_VAL3 (0x1,   4, s->refresh_en)
			+ VALID_VAL3 (0x1,   5, s->refresh_mode)
			+ VALID_VAL3 (0x1,   6, s->i16dc_qp_sel)
			+ VALID_VAL3 (0x3F,  8, s->i16_qp_base)
			+ VALID_VAL3 (0xFF, 16, s->refresh_cplx_thd)
			+ VALID_VAL3 (0xFF, 24, s->i16dc_cplx_thd));
	unsigned int cfm_cfg1            = ( 0
			+ VALID_VAL3 (0x3F, 0, s->i16dc_qp_base));
	unsigned int cfm_cplx_idx_cfg0   = ( 0
			+ VALID_VAL3 (0x7,  0, s->cplx_thd_idx[0])
			+ VALID_VAL3 (0x7,  3, s->cplx_thd_idx[1])
			+ VALID_VAL3 (0x7,  6, s->cplx_thd_idx[2])
			+ VALID_VAL3 (0x7,  9, s->cplx_thd_idx[3])
			+ VALID_VAL3 (0x7, 12, s->cplx_thd_idx[4])
			+ VALID_VAL3 (0x7, 15, s->cplx_thd_idx[5])
			+ VALID_VAL3 (0x7, 18, s->cplx_thd_idx[6])
			+ VALID_VAL3 (0x7, 21, s->cplx_thd_idx[7])
			+ VALID_VAL3 (0x7, 24, s->cplx_thd_idx[8])
			+ VALID_VAL3 (0x7, 27, s->cplx_thd_idx[9]));
	unsigned int cfm_cplx_idx_cfg1   = ( 0
			+ VALID_VAL3 (0x7,  0, s->cplx_thd_idx[10])
			+ VALID_VAL3 (0x7,  3, s->cplx_thd_idx[11])
			+ VALID_VAL3 (0x7,  6, s->cplx_thd_idx[12])
			+ VALID_VAL3 (0x7,  9, s->cplx_thd_idx[13])
			+ VALID_VAL3 (0x7, 12, s->cplx_thd_idx[14])
			+ VALID_VAL3 (0x7, 15, s->cplx_thd_idx[15])
			+ VALID_VAL3 (0x7, 18, s->cplx_thd_idx[16])
			+ VALID_VAL3 (0x7, 21, s->cplx_thd_idx[17])
			+ VALID_VAL3 (0x7, 24, s->cplx_thd_idx[18])
			+ VALID_VAL3 (0x7, 27, s->cplx_thd_idx[19]));
#ifdef CONFIG_SOC_PRJ007
	unsigned int cfm_cplx_idx_cfg2= ( 0
			+ VALID_VAL3 (0x7, 0, s->cplx_thd_idx[20])
			+ VALID_VAL3 (0x7, 3, s->cplx_thd_idx[21])
			+ VALID_VAL3 (0x7, 6, s->cplx_thd_idx[22])
			+ VALID_VAL3 (0x7, 9, s->cplx_thd_idx[23])
			+ VALID_VAL3 (0xFF, 24, s->rm.diff_cplx_thd));
#elif defined(CONFIG_SOC_PRJ008)
	unsigned int cfm_cplx_idx_cfg2   = ( 0
			+ VALID_VAL3 (0x7, 0, s->cplx_thd_idx[20])
			+ VALID_VAL3 (0x7, 3, s->cplx_thd_idx[21])
			+ VALID_VAL3 (0x7, 6, s->cplx_thd_idx[22])
			+ VALID_VAL3 (0x7, 9, s->cplx_thd_idx[23]));
#endif
	unsigned int qpg_roi_area_en     = ( 0
			+ VALID_VAL2 (15, s->roi_info[15].roi_en)
			+ VALID_VAL2 (14, s->roi_info[14].roi_en)
			+ VALID_VAL2 (13, s->roi_info[13].roi_en)
			+ VALID_VAL2 (12, s->roi_info[12].roi_en)
			+ VALID_VAL2 (11, s->roi_info[11].roi_en)
			+ VALID_VAL2 (10, s->roi_info[10].roi_en)
			+ VALID_VAL2 ( 9, s->roi_info[ 9].roi_en)
			+ VALID_VAL2 ( 8, s->roi_info[ 8].roi_en)
			+ VALID_VAL2 ( 7, s->roi_info[ 7].roi_en)
			+ VALID_VAL2 ( 6, s->roi_info[ 6].roi_en)
			+ VALID_VAL2 ( 5, s->roi_info[ 5].roi_en)
			+ VALID_VAL2 ( 4, s->roi_info[ 4].roi_en)
			+ VALID_VAL2 ( 3, s->roi_info[ 3].roi_en)
			+ VALID_VAL2 ( 2, s->roi_info[ 2].roi_en)
			+ VALID_VAL2 ( 1, s->roi_info[ 1].roi_en)
			+ VALID_VAL2 ( 0, s->roi_info[ 0].roi_en));
#ifdef CONFIG_SOC_PRJ007
	unsigned int T32_qp_val = 0;
#elif defined(CONFIG_SOC_PRJ008)
	unsigned int T33_qp_val = 0;
#endif

#ifdef CONFIG_SOC_PRJ007
	unsigned int T32_efe_extra_cfg0 = 0;
	unsigned int T32_ifa_cfg = 0;
#endif

#ifdef CONFIG_SOC_PRJ008
	unsigned int T33_efe_extra_cfg0 = 0;
	unsigned int T33_ifa_cfg = 0;
#endif

#ifdef CONFIG_SOC_PRJ007
	int T32_qpg_cfg = 0;
#endif

#ifdef CONFIG_SOC_PRJ008
	unsigned int T33_qpg_cfg = 0;
#endif

	unsigned int jrfd_y_addr       = 0;
	unsigned int bfsh_beyond_yaddr = 0;
	unsigned int align_height      = 0;
	unsigned int jrfd_ctrl         = 0;
	unsigned int jrfd_stride       = 0;
	unsigned int jrfd_trig         = 0;
	unsigned int jrfd_c_addr       = 0;
	unsigned int jrfd_mc_addr      = 0;
	unsigned int bfsh_base_caddr   = 0;
	unsigned int bfsh_beyond_caddr = 0;

#ifdef CONFIG_SOC_PRJ007
	unsigned int jrfd_hstr = 0;
#endif

#ifdef CONFIG_SOC_PRJ008
	unsigned int jrfd_space = 0;
#endif

	unsigned int esti_ctrl     = 0;
	unsigned int mce_frm_size  = 0;
	unsigned int mce_frm_strd  = 0;
	unsigned int mce_pref_expd = 0;
	unsigned int mce_comp_ctrl = 0;
	unsigned int mce_slc_mv    = 0;
	unsigned int mce_mvr       = 0;
	unsigned int mce_glb_ctrl  = 0;
	unsigned int mce_bus_ctrl  = 0;
	unsigned int mce_rcc_cfg0  = 0;
	unsigned int mce_rcc_cfg1  = 0;
	unsigned int mce_rcc_cfg2  = 0;
	unsigned int mce_rcc_cfg3  = 0;
	int mce_sech_grp0      = 0;
	int mce_sech_grp1      = 0;
	int mce_sech_cfg0_l    = 0;
	int mce_sech_cfg0_h    = 0;
	int mce_sech_cfg1_l    = 0;
	int mce_sech_cfg1_h    = 0;
	int mce_sech_cfg2_l    = 0;
	int mce_sech_cfg2_h    = 0;
	int mce_sech_cfg3_l    = 0;
	int mce_sech_cfg3_h    = 0;

#ifdef CONFIG_SOC_PRJ008
	unsigned int mce_comp_gap = 0;
#endif

	unsigned int vmau_ddz         = 0;
	unsigned int vmau_ddz1        = 0;
	unsigned int vmau_acmask      = 0;
	unsigned int vmau_gbl_ctr     = 0;
	unsigned int vmau_video_type  = 0;
	unsigned int vmau_y_gs        = 0;
	unsigned int vmau_md_cfg0     = 0;
	unsigned int vmau_md_cfg1     = 0;
	unsigned int vmau_md_cfg2     = 0;
	unsigned int vmau_md_cfg3     = 0;
	unsigned int vmau_md_cfg4     = 0;
	unsigned int vmau_md_cfg5     = 0;
	unsigned int vmau_md_cfg6     = 0;
	unsigned int vmau_md_cfg7     = 0;
	unsigned int vmau_md_cfg8     = 0;
	unsigned int vmau_md_cfg9     = 0;
	unsigned int vmau_md_cfg10    = 0;
	unsigned char set_modectrl_bit0 = 0;
	unsigned char set_modectrl_bit2 = 0;
	unsigned int vmau_md_cfg11    = 0;
	unsigned int md_cfg5          = 0;
	unsigned int md_ned_disable0  = 0;
	unsigned int md_ned_disable1  = 0;
	unsigned int md_ned_disable2  = 0;
	unsigned int md_rcfg[10]      = {0};

	unsigned int vmau_ctx = 0;

#ifdef CONFIG_SOC_PRJ008
	unsigned int tfm_ddz_cfg0  = 0;
	unsigned int tfm_ddz_cfg1  = 0;
	unsigned int tfm_frm_ctl   = 0;
	unsigned int tfm_reg_sinit = 0;
	unsigned int tfm_cfg_grp[25];
#endif

#ifdef CONFIG_SOC_PRJ007
	unsigned int ipred_cfg0 = 0;
	unsigned int ipred_cfg1 = 0;
	unsigned int ipred_cfg2 = 0;
	unsigned int ipred_cfg3 = 0;
	unsigned int ipred_cfg4 = 0;
	unsigned int ipred_cfg5 = 0;
#endif

#ifdef CONFIG_SOC_PRJ008
	unsigned int ipred_ctrl       = 0;
	unsigned int ipred_frm_size   = 0;
	unsigned int ipred_bc_st      = 0;
	unsigned int ipred_slv_mode   = 0;
	unsigned int ipred_mode_ctrl  = 0;
	unsigned int ipred_mode0      = 0;
	unsigned int ipred_c4_if0     = 0;
	unsigned int ipred_c8_if0     = 0;
	unsigned int ipred_mode1      = 0;
	unsigned int ipred_c16_if0    = 0;
	unsigned int ipred_cuv_if0    = 0;
	unsigned int ipred_mode2      = 0;
#endif

#ifdef CONFIG_SOC_PRJ007
	unsigned int t32_dblk_cfg  = 0;
#endif

#ifdef CONFIG_SOC_PRJ008
	unsigned int dblk_cfg  = 0;
	unsigned int dblk_trig = 0;
#endif
	unsigned int sde_sl_geom = 0;

#ifdef CONFIG_SOC_PRJ007
	unsigned int T32_sde_cfg1 = 0;
#endif

#ifdef CONFIG_SOC_PRJ008
	unsigned int T33_sde_cfg1 = 0;
#endif

	int idx = 0;

#ifdef CONFIG_SOC_PRJ007
	int T32_emc_cfg_en = 0;
#endif

#ifdef CONFIG_SOC_PRJ008
	int T33_emc_cfg_en = 0;
#endif

	unsigned int odma_jrfc_head_y_addr = 0;
	unsigned int odma_jrfc_head_c_addr = 0;
	unsigned int odma_bfsh_base_y_addr = 0;
	unsigned int odma_bfsh_bynd_y_addr = 0;
	unsigned int odma_y_addr       = 0;
	unsigned int odma_c_addr       = 0;
	unsigned int odma_bfsh_base_c_addr = 0;
	unsigned int odma_bfsh_bynd_c_addr = 0;
	unsigned int odma_ctrl         = 0;
	unsigned int odma_trig         = 0;

#ifdef CONFIG_SOC_PRJ007
	unsigned int odma_jrfc_head_sp_y_addr = 0;
	unsigned int odma_jrfc_head_sp_c_addr = 0;
#endif
#ifdef CONFIG_SOC_PRJ008
	unsigned int odma_jrfc_bodyy_bfsh_space = 0;
	unsigned int odma_jrfc_bodyc_bfsh_space = 0;
#endif

	for(i=0;i<4;i++) {
		roi_ctrl_cfg[i]          = ( 0
				+ VALID_VAL3(0x3f, 26, s->roi_info[i*4+3].roi_qp)
				+ VALID_VAL2(      24, s->roi_info[i*4+3].roi_md)
				+ VALID_VAL3(0x3f, 18, s->roi_info[i*4+2].roi_qp)
				+ VALID_VAL2(      16, s->roi_info[i*4+2].roi_md)
				+ VALID_VAL3(0x3f, 10, s->roi_info[i*4+1].roi_qp)
				+ VALID_VAL2(       8, s->roi_info[i*4+1].roi_md)
				+ VALID_VAL3(0x3f,  2, s->roi_info[i*4+0].roi_qp)
				+ VALID_VAL2(       0, s->roi_info[i*4+0].roi_md));
		smd_cplx_thrd[i]         = ( 0
				+ VALID_VAL2 ( 0, ifa->qpg_smd_cplx_thrd[i*2+0])
				+ VALID_VAL2 (16, ifa->qpg_smd_cplx_thrd[i*2+1]));
		sobel_cplx_thrd[i]       = ( 0
				+ VALID_VAL2 ( 0, ifa->qpg_sobel_cplx_thrd[i*2+0])
				+ VALID_VAL2 (16, ifa->qpg_sobel_cplx_thrd[i*2+1]));
	}
	for(i=0;i<16;i++)
		roi_pos_cfg[i]           = ( 0
				+ VALID_VAL3 (0xff, 24, s->roi_info[i].roi_bmby)
				+ VALID_VAL3 (0xff, 16, s->roi_info[i].roi_umby)
				+ VALID_VAL3 (0xff,  8, s->roi_info[i].roi_rmbx)
				+ VALID_VAL3 (0xff,  0, s->roi_info[i].roi_lmbx));
	for(i=0;i<2;i++) {
		cplx_qp[i]               = ( 0
				+ VALID_VAL3 (0xff, 24, ifa->qpg_cplx_qp_ofst[i*4 + 3])
				+ VALID_VAL3 (0xff, 16, ifa->qpg_cplx_qp_ofst[i*4 + 2])
				+ VALID_VAL3 (0xff,  8, ifa->qpg_cplx_qp_ofst[i*4 + 1])
				+ VALID_VAL3 (0xff,  0, ifa->qpg_cplx_qp_ofst[i*4 + 0]));
		skin_lvl[i]              = ( 0
				+ VALID_VAL2 ( 0, ifa->skin_level[i*2+0])
				+ VALID_VAL2 (16, ifa->skin_level[i*2+1]));
		cfm_cplx_thrd_cfg[i]     = ( 0
				+ VALID_VAL3 (0xFF,  0, s->cplx_thd[i*4+0])
				+ VALID_VAL3 (0xFF,  8, s->cplx_thd[i*4+1])
				+ VALID_VAL3 (0xFF, 16, s->cplx_thd[i*4+2])
				+ VALID_VAL3 (0xFF, 24, s->cplx_thd[i*4+3]));
	}
	for(i=0; i<3; i++) {
		skin_thrd[i]             = ( 0
				+ VALID_VAL2 ( 0, ifa->skin_thrd[i][0][1])
				+ VALID_VAL2 ( 8, ifa->skin_thrd[i][0][0])
				+ VALID_VAL2 (16, ifa->skin_thrd[i][1][1])
				+ VALID_VAL2 (24, ifa->skin_thrd[i][1][0]));
		pet_cplx_thrd[i][0]      = ( 0
				+ VALID_VAL3 (0xff,  0, ifa->qpg_pet_cplx_thrd[i][ 3])
				+ VALID_VAL3 (0xff,  8, ifa->qpg_pet_cplx_thrd[i][ 2])
				+ VALID_VAL3 (0xff, 16, ifa->qpg_pet_cplx_thrd[i][ 1])
				+ VALID_VAL3 (0xff, 24, ifa->qpg_pet_cplx_thrd[i][ 0]));
		pet_cplx_thrd[i][1]      = ( 0
				+ VALID_VAL3 (0xff,  0, ifa->qpg_pet_cplx_thrd[i][ 7])
				+ VALID_VAL3 (0xff,  8, ifa->qpg_pet_cplx_thrd[i][ 6])
				+ VALID_VAL3 (0xff, 16, ifa->qpg_pet_cplx_thrd[i][ 5])
				+ VALID_VAL3 (0xff, 24, ifa->qpg_pet_cplx_thrd[i][ 4]));
		pet_cplx_thrd[i][2]      = ( 0
				+ VALID_VAL3 (0xff,  0, ifa->qpg_pet_cplx_thrd[i][11])
				+ VALID_VAL3 (0xff,  8, ifa->qpg_pet_cplx_thrd[i][10])
				+ VALID_VAL3 (0xff, 16, ifa->qpg_pet_cplx_thrd[i][ 9])
				+ VALID_VAL3 (0xff, 24, ifa->qpg_pet_cplx_thrd[i][ 8]));
	}
	for(i=0;i<5;i++) {
		pet_qp_ofst[i]           = ( 0
				+ VALID_VAL3 (0xf,  0, ifa->qpg_pet_qp_ofst[i][6])
				+ VALID_VAL3 (0xf,  4, ifa->qpg_pet_qp_ofst[i][5])
				+ VALID_VAL3 (0xf,  8, ifa->qpg_pet_qp_ofst[i][4])
				+ VALID_VAL3 (0xf, 12, ifa->qpg_pet_qp_ofst[i][3])
				+ VALID_VAL3 (0xf, 16, ifa->qpg_pet_qp_ofst[i][2])
				+ VALID_VAL3 (0xf, 20, ifa->qpg_pet_qp_ofst[i][1])
				+ VALID_VAL3 (0xf, 24, ifa->qpg_pet_qp_ofst[i][0]));
		pet_qp_ofst_mt[i]        = ( 0
				+ VALID_VAL3 (0xf,  0, ifa->qpg_pet_qp_ofst_mt[i][6])
				+ VALID_VAL3 (0xf,  4, ifa->qpg_pet_qp_ofst_mt[i][5])
				+ VALID_VAL3 (0xf,  8, ifa->qpg_pet_qp_ofst_mt[i][4])
				+ VALID_VAL3 (0xf, 12, ifa->qpg_pet_qp_ofst_mt[i][3])
				+ VALID_VAL3 (0xf, 16, ifa->qpg_pet_qp_ofst_mt[i][2])
				+ VALID_VAL3 (0xf, 20, ifa->qpg_pet_qp_ofst_mt[i][1])
				+ VALID_VAL3 (0xf, 24, ifa->qpg_pet_qp_ofst_mt[i][0]));
	}

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CTRL,               0, efe_ctrl);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_FRM_SIZE,           0, efe_frm_size);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RAWY_ADDR,          0, (int)s->raw_y_pa);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RAWC_ADDR,          0, (int)s->raw_c_pa);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RAW_STRD,           0, efe_stride);
#ifdef CONFIG_SOC_PRJ007
	T32_qp_val          = (0
			+ VALID_VAL3 (0x1F, 0, s->cqp_offset)
			+ VALID_VAL2 (      8, ifa->base_qp)
			+ VALID_VAL2 (     16, s->rm.r_min_qp)
			+ VALID_VAL2 (     24, s->rm.r_max_qp));
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_FRM_QP,     0, T32_qp_val);
#elif defined(CONFIG_SOC_PRJ008)
	T33_qp_val = ( 0
			+ VALID_VAL3 (0x1F, 0, s->cqp_offset)
			+ VALID_VAL2 (      8, ifa->base_qp)
			+ VALID_VAL2 (     16, ifa->min_qp )
			+ VALID_VAL2 (     24, ifa->max_qp ));
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_FRM_QP,     0, T33_qp_val);
#endif
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRS_CFG,            0, rrs_cfg);
#ifdef CONFIG_SOC_PRJ007
	T32_efe_extra_cfg0  = (0
			+ VALID_VAL3 (0x1,  16, is_264) // 264 enable
			+ VALID_VAL3 (0x1,  12, 1)      // md/mce cfg parity check enable
			+ VALID_VAL3 (0x3,  5,  0)
			+ VALID_VAL3 (0x1,  4,  1));
	T32_ifa_cfg         = (0
			+ VALID_VAL2 (31, sobel_16_or_14)
			+ VALID_VAL2 (27, ifa->petbc_en)
			+ VALID_VAL2 (26, ifa->skin_lvl_en[2])
			+ VALID_VAL2 (25, ifa->skin_lvl_en[1])
			+ VALID_VAL2 (24, ifa->skin_lvl_en[0])
			+ VALID_VAL2 (16, ifa->sobel_edge_thrd)
			+ VALID_VAL2 (10, ifa->skin_cnt_thrd)
			+ VALID_VAL2 ( 9, emc_out_en[1])
			+ VALID_VAL2 ( 8, emc_out_en[0])
			+ VALID_VAL2 ( 7, ifa->sobel_en)
			+ VALID_VAL2 ( 6, ifa->smd_en)
			+ VALID_VAL2 ( 5, raw_avg_en)
			+ VALID_VAL2 ( 3, ifa->ery_en)
			+ VALID_VAL2 ( 2, s->rm.cps_mode)
			+ VALID_VAL2 ( 1, ifa->cps_en)
			+ VALID_VAL2 ( 0, ifa->ifa_en));
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_EXTRA_CFG0, 0, T32_efe_extra_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFA_CFG,    0, T32_ifa_cfg);
#endif

#ifdef CONFIG_SOC_PRJ008
	T33_efe_extra_cfg0   = (0
			+ VALID_VAL3 (0x1,  26, ifa->row_smd_unit)
			+ VALID_VAL3 (0x3f, 20, max_outstanding_num)
			+ VALID_VAL3 (0x1,  16, is_264) // 264 enable
			+ VALID_VAL3 (0x1,  12, 1)   // md/mce cfg parity check enable
			+ VALID_VAL3 (0x3,  5,   0)
			+ VALID_VAL3 (0x1,  4,   1));
	T33_ifa_cfg  = ( 0
			+ VALID_VAL2 (        31, sobel_16_or_14)
			+ VALID_VAL2 (        27, ifa->petbc_en)
			+ VALID_VAL3 (0xFFFF, 10, ifa->frm_mt_smd_thr)
			+ VALID_VAL2 (         9, emc_out_en[1])
			+ VALID_VAL2 (         8, emc_out_en[0])
			+ VALID_VAL2 (         7, ifa->sobel_en)
			+ VALID_VAL2 (         6, ifa->smd_en)
			+ VALID_VAL2 (         5, raw_avg_en)
			+ VALID_VAL2 (         3, ifa->ery_en)
			+ VALID_VAL2 (         2, 0)
			+ VALID_VAL2 (         1, ifa->cps_en)
			+ VALID_VAL2 (         0, ifa->ifa_en));
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_EXTRA_CFG0, 0, T33_efe_extra_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFA_CFG,    0, T33_ifa_cfg);
#endif

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFA_CFG1,           0, ifa_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RFH_CFG1,           0, rfh_cfg1);
#ifdef CONFIG_SOC_PRJ007
	T32_qpg_cfg  = (0
			+ VALID_VAL2 (0, ifa->qpg_en)
			+ VALID_VAL2 (1, ifa->qpg_table_en)
			+ VALID_VAL2 (2, ifa->qpg_skin_en)
			+ VALID_VAL2 (3, ifa->qpg_ery_en)
			+ VALID_VAL2 (4, ifa->qpg_sobel_en)
			+ VALID_VAL2 (5, ifa->qpg_smd_en)
			+ VALID_VAL2 (6 , ifa->crp_thrd)
			+ VALID_VAL3 (0xff, 8,  ifa->qpg_skin_qp_ofst[0])
			+ VALID_VAL3 (0xff, 16, ifa->qpg_skin_qp_ofst[1])
			+ VALID_VAL3 (0xff, 24, ifa->qpg_skin_qp_ofst[2]));
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_QPG_CFG,    0, T32_qpg_cfg);
#endif

#ifdef CONFIG_SOC_PRJ008
	T33_qpg_cfg  = ( 0
			+ VALID_VAL2 (       0, ifa->qpg_en)
			+ VALID_VAL2 (       1, ifa->qpg_table_en)
			+ VALID_VAL2 (       2, ifa->qpg_skin_en)
			+ VALID_VAL2 (       3, ifa->qpg_ery_en)
			+ VALID_VAL2 (       4, ifa->qpg_sobel_en)
			+ VALID_VAL2 (       5, ifa->qpg_smd_en)
			+ VALID_VAL2 (       6, ifa->crp_thrd)
			+ VALID_VAL3 (0x3f,  8, ifa->qpg_max_qp)
			+ VALID_VAL3 (0x3f, 14, ifa->qpg_min_qp));
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_QPG_CFG,    0, T33_qpg_cfg);
#endif

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_QPG_CFG1,           0, qpg_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFAC_ALL_EN,        0, ifac_all_en);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_IFAC_ALL1_EN,       0, ifac_all1_en);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_FTR,           0, skin_factor);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_LVL0,          0, skin_lvl[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_LVL1,          0, skin_lvl[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_THRD0,         0, skin_thrd[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_THRD1,         0, skin_thrd[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SKIN_THRD2,         0, skin_thrd[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD0,     0, smd_cplx_thrd[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD1,     0, smd_cplx_thrd[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD2,     0, smd_cplx_thrd[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SMD_CPLX_THRD3,     0, smd_cplx_thrd[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD0,   0, sobel_cplx_thrd[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD1,   0, sobel_cplx_thrd[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD2,   0, sobel_cplx_thrd[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_SOBEL_CPLX_THRD3,   0, sobel_cplx_thrd[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_EN,             0, qpg_roi_area_en);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOA,          0, roi_ctrl_cfg[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOB,          0, roi_ctrl_cfg[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOC,          0, roi_ctrl_cfg[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI_INFOD,          0, roi_ctrl_cfg[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI0_POS,           0, roi_pos_cfg[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI1_POS,           0, roi_pos_cfg[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI2_POS,           0, roi_pos_cfg[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI3_POS,           0, roi_pos_cfg[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI4_POS,           0, roi_pos_cfg[4]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI5_POS,           0, roi_pos_cfg[5]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI6_POS,           0, roi_pos_cfg[6]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI7_POS,           0, roi_pos_cfg[7]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI8_POS,           0, roi_pos_cfg[8]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI9_POS,           0, roi_pos_cfg[9]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI10_POS,          0, roi_pos_cfg[10]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI11_POS,          0, roi_pos_cfg[11]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI12_POS,          0, roi_pos_cfg[12]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI13_POS,          0, roi_pos_cfg[13]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI14_POS,          0, roi_pos_cfg[14]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_ROI15_POS,          0, roi_pos_cfg[15]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_FILTER_VALID,   0, petbc_filter_valid);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST0,    0, ai_motion_smd_ofst0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST1,    0, ai_motion_smd_ofst1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST2,    0, ai_motion_smd_ofst2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST3,    0, ai_motion_smd_ofst3);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST4,    0, ai_motion_smd_ofst4);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST5,    0, ai_motion_smd_ofst5);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST6,    0, ai_motion_smd_ofst6);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST7,    0, ai_motion_smd_ofst7);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST8,    0, ai_motion_smd_ofst8);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_AI_MT_SMD_OFST9,    0, ai_motion_smd_ofst9);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_VAR_THR,      0, pet_var_thr);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_SSM_THR0,     0, pet_ssm_thr0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_SSM_THR1,     0, pet_ssm_thr1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC_SSM_THR2,     0, pet_ssm_thr2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_VAR_THR,     0, pet2_var_thr);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_SSM_THR0,    0, pet2_ssm_thr0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_SSM_THR1,    0, pet2_ssm_thr1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PETBC2_SSM_THR2,    0, pet2_ssm_thr2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD00,    0, pet_cplx_thrd[0][0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD01,    0, pet_cplx_thrd[0][1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD02,    0, pet_cplx_thrd[0][2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD10,    0, pet_cplx_thrd[1][0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD11,    0, pet_cplx_thrd[1][1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD12,    0, pet_cplx_thrd[1][2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD20,    0, pet_cplx_thrd[2][0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD21,    0, pet_cplx_thrd[2][1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_CPLX_THRD22,    0, pet_cplx_thrd[2][2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST0,       0, pet_qp_ofst[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST1,       0, pet_qp_ofst[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST2,       0, pet_qp_ofst[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST3,       0, pet_qp_ofst[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST4,       0, pet_qp_ofst[4]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT0,    0, pet_qp_ofst_mt[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT1,    0, pet_qp_ofst_mt[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT2,    0, pet_qp_ofst_mt[2]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT3,    0, pet_qp_ofst_mt[3]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_OFST_MT4,    0, pet_qp_ofst_mt[4]);

	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_QPG_DLT_THRD,       0, qpg_dlt_thrd);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_PET_QP_IDX,         0, pet_qp_idx);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_QP0,           0, cplx_qp[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CPLX_QP1,           0, cplx_qp[1]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR0,    0, rrc_motion_thr0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR1,    0, rrc_motion_thr1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR2,    0, rrc_motion_thr2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_RRC_MOTION_THR3,    0, rrc_motion_thr3);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CFG0,           0, cfm_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CFG1,           0, cfm_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_IDX_CFG0,  0, cfm_cplx_idx_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_IDX_CFG1,  0, cfm_cplx_idx_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_IDX_CFG2,  0, cfm_cplx_idx_cfg2);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_THRD_CFG0, 0, cfm_cplx_thrd_cfg[0]);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + REG_EFE_CFM_CPLX_THRD_CFG1, 0, cfm_cplx_thrd_cfg[1]);

#ifdef CONFIG_SOC_PRJ008
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG0, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG1, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG2, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG3, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG4, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG5, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG6, 0, 0);
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + T33_REG_EFE_DFT_CFG7, 0, 0);
#endif

	/*     //efe wr row_smd_ram  */
	/*     for (i=0; i<64; i++) */
	/*     GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SMD_WR_BASE_ADDR+i*4, 0, i); */

	/*************************************************************************************
	  JRFD Module
	 *************************************************************************************/
	jrfd_y_addr       = s->buf_share_en ? (unsigned int)s->buf_ref_yaddr : (unsigned int)s->ref_y_pa;
	bfsh_beyond_yaddr = ( s->frm_num_gop == 1 && s->use_dummy_byd ) ? s->dummy_buf_beyond_yaddr :  buf_beyond_yaddr;
	align_height      = (s->frame_height + 15) & ~15;
	jrfd_ctrl         = (0
			+ VALID_VAL3 (0x1, 31, s->buf_share_en)
			+ VALID_VAL2 (     30, 1)               //is_helix
			+ VALID_VAL2 (     28, 0)               //clk_gate
			+ VALID_VAL2 (     27, s->jrfcv2_en)
			+ VALID_VAL2 (     14, align_height)
			+ VALID_VAL2 (      0, s->frame_width));
	jrfd_stride       = JRFD_BODY_STRDY(((s->frame_width+15)/16)*16) | JRFD_BODY_STRDC(((s->frame_width+15)/16)*8);
	jrfd_trig         = (0
			+ VALID_VAL2 (5, 1)   //init
			+ VALID_VAL2 (4, 0)); //ckg low means do clock-gating
	jrfd_c_addr       = s->body_combine_en ? jrfd_y_addr : s->buf_share_en ? (unsigned int)s->buf_ref_caddr : (unsigned int)s->ref_c_pa;
	jrfd_mc_addr      = s->body_combine_en ? (unsigned int)s->mref_y_pa : (unsigned int)s->mref_c_pa;
	bfsh_base_caddr   = s->body_combine_en ? s->buf_base_yaddr : s->buf_base_caddr;
	bfsh_beyond_caddr = s->body_combine_en ? bfsh_beyond_yaddr : ( s->frm_num_gop == 1  && s->use_dummy_byd  ) ? s->dummy_buf_beyond_caddr : buf_beyond_caddr;
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_CTRL,            0, jrfd_ctrl);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_HDYA,            0, s->head_refy_addr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_HDCA,            0, s->head_refc_addr);
#ifdef CONFIG_SOC_PRJ007
	jrfd_hstr  = (0
			+ VALID_VAL3(0xffff, 16, s->cm_head_total)
			+ VALID_VAL3(0xffff, 0,  s->lm_head_total));
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + T32_REG_JRFD_HSTR, 0,  jrfd_hstr);
#endif

#ifdef CONFIG_SOC_PRJ008
	jrfd_space = (0
			+ VALID_VAL3 (0x7f,  13, 64 )
			+ VALID_VAL3 (0xff,   5, 128)
			+ VALID_VAL3 (0x1f,   0, 31 ));
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + T33_REG_JRFD_SPACE, 0, jrfd_space );
#endif
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BDYA,            0, jrfd_y_addr);            //tile Y address
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BDCA,            0, jrfd_c_addr);            //tile C address
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BSTR,            0, jrfd_stride);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BASEY_ADDR, 0, (int)s->buf_base_yaddr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BASEC_ADDR, 0, (int)bfsh_base_caddr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BEYDY_ADDR, 0, (int)bfsh_beyond_yaddr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_BUFS_BEYDC_ADDR, 0, (int)bfsh_beyond_caddr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MHDY,            0, s->head_mrefy_addr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MHDC,            0, s->head_mrefc_addr);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MBDY,            0, (unsigned int)s->mref_y_pa);
	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_MBDC,            0, jrfd_mc_addr);

	GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + REG_JRFD_TRIG,            0, jrfd_trig);              //enable_tile
	/*************************************************************************************
	  MCE Module
	 *************************************************************************************/
	if(s->frame_type) {
		esti_ctrl     = (0
				+ MCE_ESTI_CTRL_PUE_16X16
				+ MCE_ESTI_CTRL_SCL(s->scl)/*trbl mvp*/
				+ MCE_ESTI_CTRL_FBG(0)
				+ MCE_ESTI_CTRL_CLMV
				+ MCE_ESTI_CTRL_MSS(s->max_sech_step_i)
				+ MCE_ESTI_CTRL_QRL(s->qpel_en)
				+ MCE_ESTI_CTRL_HRL(s->hpel_en)
				+ MCE_ESTI_CTRL_RF8(0)
				+ MCE_ESTI_CTRL_RF16(0)
				+ MCE_ESTI_CTRL_RF32(0));
		mce_frm_size  = ( 0
				+ MCE_FRM_SIZE_FH(s->frame_height-1)
				+ MCE_FRM_SIZE_FW(s->frame_width-1));
		mce_frm_strd  = ( 0
				+ MCE_FRM_STRD_STRDC(((s->frame_width+63)/64)*64)
				+ MCE_FRM_STRD_STRDY(((s->frame_width+63)/64)*64));
		mce_pref_expd = ( 0
				+ MCE_PREF_EXPD_L(4)
				+ MCE_PREF_EXPD_R(4)
				+ MCE_PREF_EXPD_D(4)
				+ MCE_PREF_EXPD_T(4));
		mce_comp_ctrl = ( 0
				+ MCE_COMP_CTRL_CCE
				+ MCE_COMP_CTRL_CTAP(MCE_TAP_TAP2 & 0x3)//x265:tap4, svac:tap8
				+ MCE_COMP_CTRL_CSPT(MCE_SPT_BILI)
				+ MCE_COMP_CTRL_CSPP(MCE_SPP_EPEL)
				+ MCE_COMP_CTRL_YCE
				+ MCE_COMP_CTRL_YTAP(MCE_TAP_TAP6)
				+ MCE_COMP_CTRL_YSPT(MCE_SPT_AUTO)
				+ MCE_COMP_CTRL_YSPP(MCE_SPP_QPEL));
		mce_slc_mv    = ( 0
				+ MCE_SLC_MVY(s->glb_mvy)
				+ MCE_SLC_MVX(s->glb_mvx));
		mce_mvr       = ( 0
				+ MCE_MVR_MVRY(s->max_mvry_i*4)
				+ MCE_MVR_MVRX(s->max_mvrx_i*4));
		mce_glb_ctrl  = ( 0
				+ MCE_GLB_CTRL_INIT
				+ MCE_GLB_CTRL_RESI(0)            //(cu32, cu16, cu8)
				+ MCE_GLB_CTRL_VTYPE(0x2)         //encode formate 0:x265, 1:svac, 2:x264
				+ MCE_GLB_CTRL_FMT(0x2)
				+ MCE_GLB_CTRL_ED(0)              //0:encode, 1:decode
				+ MCE_GLB_CTRL_TMVP(0)
				+ MCE_GLB_CTRL_PREF(0x1)      //pref neighbour mv en//配置0x1即是预取关闭
				+ MCE_GLB_CTRL_PS(s->pskip_en)
				+ MCE_GLB_CTRL_MREF(s->mref_en)   //ref1-mv0 enable
				+ MCE_GLB_CTRL_DCT8(s->dct8x8_en)
				+ (0<<4));
		mce_bus_ctrl  = ( 0
				+ RADIX_MCE_BUS_CTRL_OTPY(16)  //prefy outstanding
				+ RADIX_MCE_BUS_CTRL_OTPC(8)   //prefc outstanding
				+ RADIX_MCE_BUS_CTRL_OTMY(8)   //missy outstanding
				+ RADIX_MCE_BUS_CTRL_OTMC(4)   //missc outstanding
				+ RADIX_MCE_BUS_CTRL_PROR(0)   //proirity
				+ RADIX_MCE_BUS_CTRL_ATHR(6)   //almost full threshold
				+ RADIX_MCE_BUS_CTRL_SENA(1)   //state en
				+ RADIX_MCE_BUS_CTRL_BFSH(s->buf_share_en));

		mce_rcc_cfg0  = ( 0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_rcc_cfg1  = ( 0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_rcc_cfg2  = (0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_rcc_cfg3  = (0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_sech_grp0      = (0
				+ VALID_VAL3(0x3, 29, s->mcec_grp[1].sech_bc_wei)
				+ VALID_VAL3(0x3, 27, s->mcec_grp[1].mvc_bc_wei)
				+ VALID_VAL3(0x1, 26, s->mcec_grp[1].force_zero_mv_en)
				+ VALID_VAL3(0x7, 23, s->mcec_grp[1].cros_cfg_idx)
				+ VALID_VAL3(0x7, 20, s->mcec_grp[1].fs_cfg_idx)
				+ VALID_VAL3(0x7, 17, s->mcec_grp[1].mcec_mode)
				+ VALID_VAL3(0x1, 16, s->mcec_grp[1].mcec_en && s->mce_cfg_en)
				+ VALID_VAL3(0x3, 13, s->mcec_grp[0].sech_bc_wei)
				+ VALID_VAL3(0x3, 11, s->mcec_grp[0].mvc_bc_wei)
				+ VALID_VAL3(0x1, 10, s->mcec_grp[0].force_zero_mv_en)
				+ VALID_VAL3(0x7, 7, s->mcec_grp[0].cros_cfg_idx)
				+ VALID_VAL3(0x7, 4, s->mcec_grp[0].fs_cfg_idx)
				+ VALID_VAL3(0x7, 1, s->mcec_grp[0].mcec_mode)
				+ VALID_VAL3(0x1, 0, s->mcec_grp[0].mcec_en && s->mce_cfg_en));
		mce_sech_grp1      = (0
				+ VALID_VAL3(0x3, 29, s->mcec_grp[3].sech_bc_wei)
				+ VALID_VAL3(0x3, 27, s->mcec_grp[3].mvc_bc_wei)
				+ VALID_VAL3(0x1, 26, s->mcec_grp[3].force_zero_mv_en)
				+ VALID_VAL3(0x7, 23, s->mcec_grp[3].cros_cfg_idx)
				+ VALID_VAL3(0x7, 20, s->mcec_grp[3].fs_cfg_idx)
				+ VALID_VAL3(0x7, 17, s->mcec_grp[3].mcec_mode)
				+ VALID_VAL3(0x1, 16, s->mcec_grp[3].mcec_en && s->mce_cfg_en)
				+ VALID_VAL3(0x3, 13, s->mcec_grp[2].sech_bc_wei)
				+ VALID_VAL3(0x3, 11, s->mcec_grp[2].mvc_bc_wei)
				+ VALID_VAL3(0x1, 10, s->mcec_grp[2].force_zero_mv_en)
				+ VALID_VAL3(0x7,  7, s->mcec_grp[2].cros_cfg_idx)
				+ VALID_VAL3(0x7,  4, s->mcec_grp[2].fs_cfg_idx)
				+ VALID_VAL3(0x7,  1, s->mcec_grp[2].mcec_mode)
				+ VALID_VAL3(0x1,  0, s->mcec_grp[2].mcec_en && s->mce_cfg_en));
		mce_sech_cfg0_l    = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[0].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[0].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[0].mcec_lx));
		mce_sech_cfg0_h    = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[0].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[0].mcec_by));
		mce_sech_cfg1_l    = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[1].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[1].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[1].mcec_lx));
		mce_sech_cfg1_h    = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[1].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[1].mcec_by));
		mce_sech_cfg2_l    = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[2].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[2].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[2].mcec_lx));
		mce_sech_cfg2_h    = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[2].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[2].mcec_by));
		mce_sech_cfg3_l    = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[3].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[3].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[3].mcec_lx));
		mce_sech_cfg3_h    = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[3].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[3].mcec_by));
#ifdef CONFIG_SOC_PRJ008
		mce_comp_gap = (0
				+ VALID_VAL2 (22, 0x0) //prefc enable
				+ VALID_VAL2 (21, s->jrfcv2_en)
				+ VALID_VAL2 (19, s->buf_share_size) // 0~3,when size >= 3, trans 3
				+ VALID_VAL2 (18, s->buf_share_en)
				+ VALID_VAL2 (12, s->mce_pref_mode));//(zero, top, left) prefc enable

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_GRP0,   0, mce_sech_grp0  );
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_GRP1,   0, mce_sech_grp1  );
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG0_L, 0, mce_sech_cfg0_l);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG0_H, 0, mce_sech_cfg0_h);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG1_L, 0, mce_sech_cfg1_l);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG1_H, 0, mce_sech_cfg1_h);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG2_L, 0, mce_sech_cfg2_l);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG2_H, 0, mce_sech_cfg2_h);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG3_L, 0, mce_sech_cfg3_l);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_SECH_CFG3_H, 0, mce_sech_cfg3_h);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_FRM_SIZE,        0, mce_frm_size);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_FRM_STRD,        0, mce_frm_strd);

		if(s->buf_share_en){
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_RLUT(0, 0),     0, (int)s->buf_base_yaddr);//base
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_RLUT(0, 0)+4,   0, (int)s->buf_base_caddr);//base
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRY, 0, refy_addr_ba0);
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRC, 0, refc_addr_ba0);
		}
		else {
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRY, 0, s->ref_y_pa);
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRC, 0, s->ref_c_pa);
		}

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MREF_ADRY,       0, (int)s->mref_y_pa);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MREF_ADRC,       0, (int)s->mref_c_pa);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_BYND_YADDR,  0, s->buf_beyond_yaddr);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_BYND_CADDR,  0, s->buf_beyond_caddr);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_PREF_EXPD,       0, mce_pref_expd);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_COMP_CTRL,       0, mce_comp_ctrl);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_SLC_SPOS,        0, 0);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_SLC_MV,          0, mce_slc_mv);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_ESTI_CTRL,       0, esti_ctrl);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MRGI,            0, 0);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MVR,             0, mce_mvr);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_COMP_GAP,        0, mce_comp_gap);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_BUS_CTRL,    0, mce_bus_ctrl);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_RCC_CTRL,    0, (0x1));
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_RCC_CFG0,    0, mce_rcc_cfg0);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_RCC_CFG1,    0, mce_rcc_cfg1);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_RCC_CFG2,    0, mce_rcc_cfg2);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + T33_RADIX_REG_MCE_RCC_CFG3,    0, mce_rcc_cfg3);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_GLB_CTRL,              0, mce_glb_ctrl);//crc_en
#endif

#ifdef CONFIG_SOC_PRJ007
		mce_frm_size  = (0
				+ MCE_FRM_SIZE_FH(s->frame_height-1)
				+ MCE_FRM_SIZE_FW(s->frame_width-1));
		mce_frm_strd  = (0
				+ MCE_FRM_STRD_STRDC(((s->frame_width+63)/64)*64)
				+ MCE_FRM_STRD_STRDY(((s->frame_width+63)/64)*64));
		mce_pref_expd = (0
				+ MCE_PREF_EXPD_L(4)
				+ MCE_PREF_EXPD_R(4)
				+ MCE_PREF_EXPD_D(4)
				+ MCE_PREF_EXPD_T(4));
		mce_comp_ctrl = (0
				+ MCE_COMP_CTRL_CCE
				+ MCE_COMP_CTRL_CTAP(MCE_TAP_TAP2 & 0x3)//x265:tap4, svac:tap8
				+ MCE_COMP_CTRL_CSPT(MCE_SPT_BILI)
				+ MCE_COMP_CTRL_CSPP(MCE_SPP_EPEL)
				+ MCE_COMP_CTRL_YCE
				+ MCE_COMP_CTRL_YTAP(MCE_TAP_TAP6)
				+ MCE_COMP_CTRL_YSPT(MCE_SPT_AUTO)
				+ MCE_COMP_CTRL_YSPP(MCE_SPP_QPEL));
		mce_glb_ctrl  = (0
				+ MCE_GLB_CTRL_INIT
				+ MCE_GLB_CTRL_RESI(0)            //(cu32, cu16, cu8)
				+ MCE_GLB_CTRL_VTYPE(0x2)         //encode formate 0:x265, 1:svac, 2:x264
				+ MCE_GLB_CTRL_FMT(0x2)
				+ MCE_GLB_CTRL_ED(0)              //0:encode, 1:decode
				+ MCE_GLB_CTRL_TMVP(0)
				+ MCE_GLB_CTRL_PS(s->pskip_en)
				+ MCE_GLB_CTRL_MREF(s->mref_en)   //ref1-mv0 enable
				+ MCE_GLB_CTRL_DCT8(s->dct8x8_en)
				+ MCE_GLB_CTRL_PREF(0x1)
				+ (0<<4));                       //crc_en
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_FRM_SIZE,  0, mce_frm_size);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_FRM_STRD,  0, mce_frm_strd);

		if(s->buf_share_en){
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_RLUT(0, 0),     0, (int)s->buf_base_yaddr);//base
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_RLUT(0, 0)+4,   0, (int)s->buf_base_caddr);//base
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRY, 0, refy_addr_ba0);
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRC, 0, refc_addr_ba0);
		}
		else {
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRY, 0, s->ref_y_pa);
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRC, 0, s->ref_c_pa);
		}

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MREF_ADRY, 0, (unsigned int)s->mref_y_pa);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MREF_ADRC, 0, (unsigned int)s->mref_y_pa);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_BOTM_ID,   0, s->buf_ref_rem_mby);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_PREF_EXPD, 0, mce_pref_expd);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_COMP_CTRL, 0, mce_comp_ctrl);
		for(i=0; i<16; i++){
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_Y+i*8,    0, MCE_ILUT_INFO(IntpFMT[H264_QPEL][i].intp[0],
						IntpFMT[H264_QPEL][i].intp_pkg[0],
						IntpFMT[H264_QPEL][i].hldgl,/*idgl*/
						0,/*edgl*/
						IntpFMT[H264_QPEL][i].intp_dir[0],
						IntpFMT[H264_QPEL][i].intp_rnd[0],
						IntpFMT[H264_QPEL][i].intp_sft[0],
						IntpFMT[H264_QPEL][i].intp_sintp[0],
						IntpFMT[H264_QPEL][i].intp_srnd[0],
						IntpFMT[H264_QPEL][i].intp_sbias[0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_Y+i*8+4,  0, MCE_ILUT_INFO(IntpFMT[H264_QPEL][i].intp[1],
						IntpFMT[H264_QPEL][i].intp_pkg[1],
						IntpFMT[H264_QPEL][i].hldgl,/*idgl*/
						0,
						IntpFMT[H264_QPEL][i].intp_dir[1],
						IntpFMT[H264_QPEL][i].intp_rnd[1],
						IntpFMT[H264_QPEL][i].intp_sft[1],
						IntpFMT[H264_QPEL][i].intp_sintp[1],
						IntpFMT[H264_QPEL][i].intp_srnd[1],
						IntpFMT[H264_QPEL][i].intp_sbias[1]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16,   0, MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[0][3],
						IntpFMT[H264_QPEL][i].intp_coef[0][2],
						IntpFMT[H264_QPEL][i].intp_coef[0][1],
						IntpFMT[H264_QPEL][i].intp_coef[0][0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16+4, 0, MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[0][7],
						IntpFMT[H264_QPEL][i].intp_coef[0][6],
						IntpFMT[H264_QPEL][i].intp_coef[0][5],
						IntpFMT[H264_QPEL][i].intp_coef[0][4]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16+8, 0, MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[1][3],
						IntpFMT[H264_QPEL][i].intp_coef[1][2],
						IntpFMT[H264_QPEL][i].intp_coef[1][1],
						IntpFMT[H264_QPEL][i].intp_coef[1][0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_Y+i*16+12, 0,MCE_CLUT_INFO(IntpFMT[H264_QPEL][i].intp_coef[1][7],
						IntpFMT[H264_QPEL][i].intp_coef[1][6],
						IntpFMT[H264_QPEL][i].intp_coef[1][5],
						IntpFMT[H264_QPEL][i].intp_coef[1][4]));
		}
		for(i=0; i<16; i++){
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_C+i*8,     0, MCE_ILUT_INFO(IntpFMT[H264_EPEL][i].intp[0],     /*fir & 0x1*/
						IntpFMT[H264_EPEL][i].intp_pkg[0], /*clip & 0x1*/
						0,                                 /*idgl*/
						0,                                 /*edgl*/
						IntpFMT[H264_EPEL][i].intp_dir[0],
						IntpFMT[H264_EPEL][i].intp_rnd[0],
						IntpFMT[H264_EPEL][i].intp_sft[0],
						0,                                /*savg & 0x1*/
						0,                                /*srnd*/
						0));                              /*sbias*/
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_ILUT_C+i*8+4,   0, MCE_ILUT_INFO(IntpFMT[H264_EPEL][i].intp[1],
						IntpFMT[H264_EPEL][i].intp_pkg[1],
						0, 0,
						IntpFMT[H264_EPEL][i].intp_dir[1],
						IntpFMT[H264_EPEL][i].intp_rnd[1],
						IntpFMT[H264_EPEL][i].intp_sft[1],
						0, 0, 0));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_C+i*16,    0, MCE_CLUT_INFO(IntpFMT[H264_EPEL][i].intp_coef[0][3],
						IntpFMT[H264_EPEL][i].intp_coef[0][2],
						IntpFMT[H264_EPEL][i].intp_coef[0][1],
						IntpFMT[H264_EPEL][i].intp_coef[0][0]));
			GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + SLUT_MCE_CLUT_C+i*16+4,  0, MCE_CLUT_INFO(IntpFMT[H264_EPEL][i].intp_coef[0][7],
						IntpFMT[H264_EPEL][i].intp_coef[0][6],
						IntpFMT[H264_EPEL][i].intp_coef[0][5],
						IntpFMT[H264_EPEL][i].intp_coef[0][4]));
		}

		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_SLC_SPOS,  0, 0);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_SLC_MV,    0, (MCE_SLC_MVY(s->glb_mvy) | MCE_SLC_MVX(s->glb_mvx)));
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_ESTI_CTRL, 0, esti_ctrl);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MRGI,      0, 0);
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_MVR,       0, MCE_MVR_MVRY(s->max_mvry_i*4) | MCE_MVR_MVRX(s->max_mvrx_i*4) );
		GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + REG_MCE_GLB_CTRL,  0, mce_glb_ctrl);
#endif
	}

	/**************************************************
	  VMAU configuration
	 *************************************************/
	vmau_ddz         = ( 0
			+ VMAU_DEADZONE0_IY(s->deadzone[0])
			+ VMAU_DEADZONE1_PY(s->deadzone[1])
			+ VMAU_DEADZONE2_IC(s->deadzone[2])
			+ VMAU_DEADZONE3_PC(s->deadzone[3]) );
	vmau_ddz1        = ( 0
			+ VALID_VAL3 (0x3f,  0, s->deadzone[4])
			+ VALID_VAL3 (0x3f,  6, s->deadzone[5])
			+ VALID_VAL3 (0x3f, 12, s->deadzone[6])
			+ VALID_VAL3 (0x3f, 18, s->deadzone[7])
			+ VALID_VAL3 (0x3f, 24, s->deadzone[8]));
	vmau_acmask      = ( 0
			+ (s->rm.acmask_mode<<14) + 2);/* T33=0; 2 for TEST: rd file */
	vmau_gbl_ctr     = ( 0
			+ VMAU_TFM8_SCA_CUSTOM(s->is_scaling_custom)
			+ VMAU_NED_CTRL(ifa->efe_rrs_en)
			+ VMAU_FMT_H264);// fixed: fmt
	vmau_video_type  = ( 0
			+ VMAU_MODE_ENC
			+ VMAU_YUV_GRAY
			+ VMAU_IP_MODE_MASK
			+ VMAU_IS_ISLICE(!s->frame_type));// fixed: enc, gray, mode_msk
	vmau_y_gs        = ( 0
			+ VMAU_FRM_WID(s->mb_width*16)
			+ VMAU_FRM_HEI(s->mb_height*16));

	vmau_md_cfg0     = ( 0
			+  VMAU_MD_SLICE_I(!s->frame_type)
			+ VMAU_MD_SLICE_P(s->frame_type)
			+ VMAU_MD_I4_DIS(0)
			+ VMAU_MD_I16_DIS(0)
			+ VMAU_MD_PSKIP_DIS(0)
			+ VMAU_MD_P_L0_DIS(0)
			+ VMAU_MD_I8_DIS(0)
			+ VMAU_MD_PT8_DIS(0)
			+ VMAU_MD_DREF_EN(s->mref_en)
			+ VMAU_MD_DCT8_EN(s->dct8x8_en)
			+ VMAU_MD_FRM_REDGE(s->mb_width - 1)
			+ VMAU_MD_FRM_BEDGE(s->mb_height - 1) );
	vmau_md_cfg1     = ( 0
			+ VMAU_IPMY_BIAS_EN( s->rm.intra_lambda_y_bias_en ) //T33=0
			+ VMAU_IPMC_BIAS_EN( s->rm.intra_lambda_c_bias_en ) //T33=0
			+ VMAU_COST_BIAS_EN( s->rm.cost_bias_en )           //T33=0
			+ VMAU_CSSE_BIAS_EN( s->rm.chroma_sse_bias_en )     //T33=0
			+ VMAU_JMLAMBDA2_EN(s->jm_lambda2_en)
			+ VMAU_INTER_NEI_EN(s->inter_nei_en)
			+ VMAU_SKIP_BIAS_EN(s->skip_bias_en)
			+ VMAU_LMD_BIAS_EN( s->rm.sse_lambda_bias_en )      //T33=0
			+ VMAU_INFO_EN(s->info_en)
			+ VMAU_DCM_EN( s->rm.dcm_en )
			+ VMAU_MVDS_ALL(s->mvd_sum_all)
			+ VMAU_MVDS_ABS(s->mvd_sum_abs)
			+ VMAU_MVS_ALL(s->mv_sum_all)
			+ VMAU_MVS_ABS(s->mv_sum_abs)
			+ VMAU_P_L0_BIAS( s->rm.cost_bias_p_l0 )          //T33=0
			+ VMAU_PSKIP_BIAS( s->rm.cost_bias_p_skip )       //T33=0
			+ VMAU_I4_BIAS( s->rm.cost_bias_i_4x4 )           //T33=0
			+ VMAU_I16_BIAS(s->rm.cost_bias_i_16x16 ) );      //T33=0
	vmau_md_cfg2     = ( 0
			+ VMAU_IPM_BIAS_0( s->rm.intra_lambda_bias_0 )    //T33=0
			+ VMAU_IPM_BIAS_1( s->rm.intra_lambda_bias_1 )    //T33=0
			+ VMAU_IPM_BIAS_2( s->rm.intra_lambda_bias_2 )    //T33=0
			+ VMAU_IPM_BIAS_QP0( s->rm.intra_lambda_bias_qp0 )//T33=0
			+ VMAU_IPM_BIAS_QP1( s->rm.intra_lambda_bias_qp1 )//T33=0
			+ VMAU_MD_FBC_EP(s->fbc_ep) );
	vmau_md_cfg3     = ( 0
			+ VMAU_CSSE_BIAS_0(s->rm.chroma_sse_bias_0 )    //T33=0
			+ VMAU_CSSE_BIAS_1(s->rm.chroma_sse_bias_1 )    //T33=0
			+ VMAU_CSSE_BIAS_2(s->rm.chroma_sse_bias_2 )    //T33=0
			+ VMAU_CSSE_BIAS_QP0(s->rm.chroma_sse_bias_qp0 )//T33=0
			+ VMAU_CSSE_BIAS_QP1(s->rm.chroma_sse_bias_qp1 )//T33=0
			+ VMAU_LMD_BIAS(s->rm.sse_lambda_bias )         //T33=0
			+ VMAU_PL0_FS_DIS(s->rm.p_skip_pl0f_dis )       //T33=0
			+ VMAU_PT8_FS_DIS(s->rm.p_skip_pt8f_dis )       //T33=0
			+ VMAU_PL0_COST_MAX(s->rm.p_l0_dis )            //T33=0
			+ VMAU_PT8_COST_MAX(s->rm.p_t8_dis ));          //T33=0
	vmau_md_cfg4     = ( 0
			+ VMAU_YSSE_THR(s->ysse_thr)
			+ VMAU_I8_BIAS(s->rm.cost_bias_i_8x8)           //T33=0
			+ VMAU_PT8_BIAS(s->rm.cost_bias_p_t8));         //T33=0
	vmau_md_cfg5     = ( 0
			+ VMAU_CSSE_THR(s->csse_thr)
			+ VMAU_CQP_OFFSET(s->cqp_offset)
			+ VMAU_I4_COST_MAX( s->rm.i_4x4_dis )           //T33=0
			+ VMAU_I8_COST_MAX( s->rm.i_8x8_dis )           //T33=0
			+ VMAU_I16_COST_MAX( s->rm.i_16x16_dis ));      //T33=0
	vmau_md_cfg6     = ( 0
			+  VMAU_DCM_PARAM( s->rm.dcm_param)             //T33=0
			+ VMAU_SDE_PRIOR(s->sde_prior)
			+ VMAU_DB_PRIOR(s->db_prior));
	vmau_md_cfg7     = ( 0
			+ VMAU_CFG_SIZE_X(s->cfg_size_x)
			+ VMAU_CFG_SIZE_Y(s->cfg_size_y)
			+ VMAU_CFG_IW_THR(s->cfg_iw_thr)
			+ VMAU_CFG_BASEQP(ifa->base_qp)
			+ VMAU_CFG_ALPHA(s->alpha_c0_offset)
			+ VMAU_PS_COST_MAX( s->rm.p_skip_dis ));        //T33=0
	vmau_md_cfg8      = ( 0
			+ VMAU_CFG_MVR_THR1(s->cfg_mvr_thr1)
			+ VMAU_CFG_MVR_THR2(s->cfg_mvr_thr2));
	vmau_md_cfg9     = ( 0
			+ VMAU_CFG_MVR_THR3(s->cfg_mvr_thr3)
			+ VMAU_CFG_BETA(s->beta_offset));
	vmau_md_cfg10     = ( 0
			+ VALID_VAL2(0, s->color_shadow_sse_en)
			+ VALID_VAL2(1, s->color_shadow_sse_motion_shift)
			+ VALID_VAL2(4, s->colow_shadow_sse_ratio_thrd[0])
			+ VALID_VAL2(7, s->colow_shadow_sse_ratio_thrd[1]));
	set_modectrl_bit0 = s->md_cfg_en & 0x1;
	set_modectrl_bit2 = s->rrs_en || s->force_i16 || s->refresh_en;
	vmau_md_cfg11    = ( 0
			+ VMAU_CFG_MD_RBIAS(s->refresh_bias)
			+ VMAU_CFG_MD_RBIAS_EN(1)
			+ VMAU_CFG_MD_PCDC_N0(s->mode_ctrl >> 3 & 7)
			+ VMAU_CFG_MD_IFA_VLD(set_modectrl_bit2)
			+ VMAU_CFG_MD_SLV_VLD(0)
			+ VMAU_CFG_MD_SET_VLD((s->mode_ctrl & 0x2) + set_modectrl_bit0));
	md_cfg5          = ((1 & 0x1) << 0); //264 conf 1

	md_ned_disable0  = ( 0
			+ VALID_VAL2 ( 0, s->ned_sad_thr[0][0][0])
			+ VALID_VAL2 ( 8, s->ned_sad_thr[0][0][1])
			+ VALID_VAL2 (16, s->ned_sad_thr[0][1][0])
			+ VALID_VAL2 (24, s->ned_sad_thr[0][1][1]));
	md_ned_disable1  = ( 0
			+ VALID_VAL2 ( 0, s->ned_sad_thr[1][0][0])
			+ VALID_VAL2 ( 5, s->ned_sad_thr[1][0][1])
			+ VALID_VAL2 (10, s->ned_sad_thr[1][1][0])
			+ VALID_VAL2 (16, s->ned_sad_thr[1][1][1])
			+ VALID_VAL2 (22, s->ned_motion_en)
			+ VALID_VAL2 (23, s->ned_motion_shift)
			+ VALID_VAL2 (26, s->c_sad_min_bias)
			+ VALID_VAL2 (29, s->dct8x8_en)
			+ VALID_VAL2 (30, s->recon_resi_en)
			+ VALID_VAL2 (31, s->pskip_en));
	md_ned_disable2  = ( 0
			+ VALID_VAL2 (0, s->ned_sad_thr_cpx_step[0][0])
			+ VALID_VAL2 (2, s->ned_sad_thr_cpx_step[0][1])
			+ VALID_VAL2 (4, s->ned_sad_thr_cpx_step[1][0])
			+ VALID_VAL2 (6, s->ned_sad_thr_cpx_step[1][1]));
#ifdef CONFIG_SOC_PRJ007
	md_rcfg[0] = (((s->rc_en & 0x1) << 0) +
			(((s->bu_size - 1) & 0xFFFF) << 4) +
			((1 & 0x1) << 20) +
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
	md_rcfg[2] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd_t32[0])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[1]));
	md_rcfg[3] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd_t32[2])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[3]));
	md_rcfg[4] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd_t32[4])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[5]));
	md_rcfg[5] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd_t32[6])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[7]));
	md_rcfg[6] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd_t32[8])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[9]));
	md_rcfg[7] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd_t32[10])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[11]));
#endif

#ifdef CONFIG_SOC_PRJ007
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_GBL_CTR,  0, vmau_gbl_ctr);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_VIDEO_TYPE, 0, vmau_video_type);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_Y_GS,     0, vmau_y_gs);

	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_DEADZONE,  0, vmau_ddz);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_DEADZONE1, 0, vmau_ddz1);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_ACMASK,    0, vmau_acmask);

	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG0,  0, vmau_md_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG1,  0, vmau_md_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG2,  0, vmau_md_cfg2);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG3,  0, vmau_md_cfg3);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG4,  0, vmau_md_cfg4);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG5,  0, vmau_md_cfg5);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG6,  0, vmau_md_cfg6);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG7,  0, vmau_md_cfg7);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG8,  0, vmau_md_cfg8);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG9,  0, vmau_md_cfg9);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_CFG11,     0, vmau_md_cfg11);
	for (j = 0; j < 40; ++j)
		GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_MD_MODE, 0, s->mode_ctrl_param[j] );
	for (j = 0; j < 42; j++) {
		vmau_ctx = s->state[ j<10 ? ( !s->frame_type ? j+3 : j+14 )
			: j<22 ? j-10+73
			: j<28 ? j-22+64 : j-28+40 ];
		GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_CTX, 0,  vmau_ctx);
	}

	// md ned config
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG5,  0, md_cfg5);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_0,  0, md_ned_disable0);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_1,  0, md_ned_disable1);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_2,  0, md_ned_disable2);

	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG0, 0, md_rcfg[0]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG1, 0, md_rcfg[1]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG2, 0, md_rcfg[2]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG3, 0, md_rcfg[3]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG4, 0, md_rcfg[4]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG5, 0, md_rcfg[5]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG6, 0, md_rcfg[6]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG7, 0, md_rcfg[7]);
	for (i=0; i<s->bu_len; i++) {
		GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + (((1 << 9) + (1 << 6) + i) << 2), 0, s->rc_info[i]);
	}
#endif
	/*************************************************************************************
	  TFM Module
	 *************************************************************************************/
#ifdef CONFIG_SOC_PRJ008
	tfm_ddz_cfg0  = ( 0
			+ TFM_DEADZONE_4IC(s->deadzone[2])
			+ TFM_DEADZONE_4IY(s->deadzone[0])
			+ TFM_DEADZONE_4PC(s->deadzone[3])
			+ TFM_DEADZONE_4PY(s->deadzone[1]));
	tfm_ddz_cfg1  = ( 0
			+ TFM_DEADZONE_PCDC(s->deadzone[8])
			+ TFM_DEADZONE_ICDC(s->deadzone[7])
			+ TFM_DEADZONE_IYDC(s->deadzone[6])
			+ TFM_DEADZONE_8IY( s->deadzone[4])
			+ TFM_DEADZONE_8PY( s->deadzone[5]));
	tfm_frm_ctl   = ( 0
			+ TFM_TU4_MASK(s->tu4_mask)
			+ TFM_TU8_MASK(0)
			+ ACMASK_TYPE(s->acmask_type)
			+ TFM_IS_ACMASK(s->acmask_en)
			+ TFM_TU8_SCA_LIST_EN(!s->is_scaling_custom));
	tfm_reg_sinit = ( 0
			+ TFM8_INTRA_EN(s->dct8x8_en)
			+ TFM8_INTER_EN(s->dct8x8_en)
			+ T33_TFM_FMT_H264
			+ T33_TFM_SLICE_INIT);

	tfm_cfg_grp[0]   = tfm_frm_ctl;

	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 3; ++j) {
			tfm_cfg_grp[i * 6 + j + 1]     = (0 + VALID_VAL3 (0x3FFF, 18, s->tfm_grp[i].acmask_y[j]));
			tfm_cfg_grp[i * 6 + j + 3 + 1] = (0 + VALID_VAL3 (0x3FFF, 18, s->tfm_grp[i].acmask_c[j]));
		}
	}
	for (i = 0; i < 25; ++i)
		GEN_VDMA_ACFG(chn, RADIX_TFM_BASE_VDMA + RADIX_TFM_FRM_CTL + i * 4, 0, tfm_cfg_grp[i]);

	GEN_VDMA_ACFG(chn, RADIX_TFM_BASE_VDMA + RADIX_TFM_REG_SCTRL,         0, TFM_IS_ISLICE(!s->frame_type));
	GEN_VDMA_ACFG(chn, RADIX_TFM_BASE_VDMA + T33_RADIX_TFM_DEADZONE_CFG0, 0, tfm_ddz_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_TFM_BASE_VDMA + T33_RADIX_TFM_DEADZONE_CFG1, 0, tfm_ddz_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_TFM_BASE_VDMA + RADIX_TFM_REG_SINIT,         0, tfm_reg_sinit);
#endif
	/*************************************************************************************
	  IPRED Module
	 *************************************************************************************/
#ifdef CONFIG_SOC_PRJ007
	ipred_cfg0 = (0
			+ VMAU_IP_MD_VAL(s->force_i16dc)
			+ VMAU_IP_REF_NEB_4(s->ref_neb_4)
			+ VMAU_IP_REF_NEB_8(s->ref_neb_8)
			+ VMAU_IP_REF_PRD_C(s->pri_uv)
			+ VMAU_IP_REF_PRD_16(s->pri_16)
			+ VMAU_IP_REF_PRD_8(s->pri_8)
			+ VMAU_IP_REF_PRD_4(s->pri_4)
			+ VMAU_IP_REF_C4_EN(s->c_4_en)
			+ VMAU_IP_REF_C8_EN(s->c_8_en)
			+ VMAU_IP_REF_C16_EN(s->c_16_en)
			+ VMAU_IP_REF_CUV_EN(s->c_uv_en)
			+ VMAU_IP_REF_LMD4_EN(s->lamb_4_en)
			+ VMAU_IP_REF_LMD8_EN(s->lamb_8_en)
			+ VMAU_IP_REF_LMD16_EN(s->lamb_16_en)
			+ VMAU_IP_REF_LMDUV_EN(s->lamb_uv_en)
			+ VMAU_IP_REF_BIT4_EN(s->bit_4_en)
			+ VMAU_IP_REF_BIT8_EN(s->bit_8_en)
			+ VMAU_IP_REF_BIT16_EN(s->bit_16_en)
			+ VMAU_IP_REF_BITUV_EN(s->bit_uv_en));
	ipred_cfg1 = (0
			+ VMAU_IP_REF_4_BIT0(s->bit_4[0])
			+ VMAU_IP_REF_4_BIT1(s->bit_4[1])
			+ VMAU_IP_REF_4_BIT2(s->bit_4[2])
			+ VMAU_IP_REF_4_BIT3(s->bit_4[3])
			+ VMAU_IP_REF_8_BIT0(s->bit_8[0])
			+ VMAU_IP_REF_8_BIT1(s->bit_8[1])
			+ VMAU_IP_REF_8_BIT2(s->bit_8[2])
			+ VMAU_IP_REF_8_BIT3(s->bit_8[3]));
	ipred_cfg2 = (0
			+ VMAU_IP_REF_C_BIT0(s->bit_uv[0])
			+ VMAU_IP_REF_C_BIT1(s->bit_uv[1])
			+ VMAU_IP_REF_C_BIT2(s->bit_uv[2])
			+ VMAU_IP_REF_C_BIT3(s->bit_uv[3])
			+ VMAU_IP_REF_16_BIT0(s->bit_16[0])
			+ VMAU_IP_REF_16_BIT1(s->bit_16[1])
			+ VMAU_IP_REF_16_BIT2(s->bit_16[2])
			+ VMAU_IP_REF_16_BIT3(s->bit_16[3]));
	ipred_cfg3 = (0
			+ VMAU_IP_REF_LMDUV_IFO(s->lambda_infouv)
			+ VMAU_IP_REF_LMD16_IFO(s->lambda_info16)
			+ VMAU_IP_REF_LMD8_IFO(s->lambda_info8)
			+ VMAU_IP_REF_LMD4_IFO(s->lambda_info4)
			+ VMAU_IP_REF_NEB_4REF(s->ref_4)
			+ VMAU_IP_REF_NEB_8REF(s->ref_8));
	ipred_cfg4 = (0
			+ VMAU_IP_REF_C4_IFO(0
				+ (s->const_4[0]<<0)
				+ (s->const_4[1]<<4)
				+ (s->const_4[2]<<8)
				+ (s->const_4[3]<<12))
			+ VMAU_IP_REF_C8_IFO(0
				+ (s->const_8[0]<<0)
				+ (s->const_8[1]<<4)
				+ (s->const_8[2]<<8)
				+ (s->const_8[3]<<12)));
	ipred_cfg5 = (0
			+ VMAU_IP_REF_C16_IFO(0
				+ (s->const_16[0]<<0)
				+ (s->const_16[1]<<4)
				+ (s->const_16[2]<<8)
				+ (s->const_16[3]<<12))
			+ VMAU_IP_REF_CUV_IFO(0
				+ (s->const_uv[0]<<0)
				+ (s->const_uv[1]<<4)
				+ (s->const_uv[2]<<8)
				+ (s->const_uv[3]<<12)));

	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG0, 0, ipred_cfg0 );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG1, 0, ipred_cfg1 );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG2, 0, ipred_cfg2 );
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG3, 0, ipred_cfg3);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG4, 0, ipred_cfg4);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_IPRED_CFG5, 0, ipred_cfg5 );
#endif
	// vmau global
#ifdef CONFIG_SOC_PRJ007
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + T32_REG_VMAU_GBL_RUN, 0, VMAU_RUN );
#endif

	//t33:ipred
#ifdef CONFIG_SOC_PRJ008
	ipred_ctrl       = ( 0
			+ VALID_VAL2 ( 2, 1)                // is_264
			+ VALID_VAL2 (20, s->dct8x8_en)     // 8x8dct
			+ VALID_VAL2 (21, !s->frame_type)); // slice_type
	ipred_frm_size   = ( 0
			+ VMAU_FRM_WID(s->mb_width*16)
			+ VMAU_FRM_HEI(s->mb_height*16));
	ipred_bc_st      = ( 0
			+ VMAU_IP_MD_VAL(s->force_i16dc)
			+ VMAU_IP_REF_NEB_4(s->ref_neb_4)
			+ VMAU_IP_REF_NEB_8(s->ref_neb_8)
			+ VMAU_IP_REF_PRD_C(s->pri_uv)
			+ VMAU_IP_REF_PRD_16(s->pri_16)
			+ VMAU_IP_REF_PRD_8(s->pri_8)
			+ VMAU_IP_REF_PRD_4(s->pri_4)
			+ VMAU_IP_REF_C4_EN(s->c_4_en)
			+ VMAU_IP_REF_C8_EN(s->c_8_en)
			+ VMAU_IP_REF_C16_EN(s->c_16_en)
			+ VMAU_IP_REF_CUV_EN(s->c_uv_en)
			+ VMAU_IP_REF_LMD4_EN(s->lamb_4_en)
			+ VMAU_IP_REF_LMD8_EN(s->lamb_8_en)
			+ VMAU_IP_REF_LMD16_EN(s->lamb_16_en)
			+ VMAU_IP_REF_LMDUV_EN(s->lamb_uv_en)
			+ VMAU_IP_REF_BIT4_EN(s->bit_4_en)
			+ VMAU_IP_REF_BIT8_EN(s->bit_8_en)
			+ VMAU_IP_REF_BIT16_EN(s->bit_16_en)
			+ VMAU_IP_REF_BITUV_EN(s->bit_uv_en));
	ipred_slv_mode   = ( 0
			+ VMAU_IP_REF_4_BIT0(s->bit_4[0])
			+ VMAU_IP_REF_4_BIT1(s->bit_4[1])
			+ VMAU_IP_REF_4_BIT2(s->bit_4[2])
			+ VMAU_IP_REF_4_BIT3(s->bit_4[3])
			+ VMAU_IP_REF_8_BIT0(s->bit_8[0])
			+ VMAU_IP_REF_8_BIT1(s->bit_8[1])
			+ VMAU_IP_REF_8_BIT2(s->bit_8[2])
			+ VMAU_IP_REF_8_BIT3(s->bit_8[3]));
	ipred_mode_ctrl  = ( 0
			+ VMAU_IP_REF_C_BIT0(s->bit_uv[0])
			+ VMAU_IP_REF_C_BIT1(s->bit_uv[1])
			+ VMAU_IP_REF_C_BIT2(s->bit_uv[2])
			+ VMAU_IP_REF_C_BIT3(s->bit_uv[3])
			+ VMAU_IP_REF_16_BIT0(s->bit_16[0])
			+ VMAU_IP_REF_16_BIT1(s->bit_16[1])
			+ VMAU_IP_REF_16_BIT2(s->bit_16[2])
			+ VMAU_IP_REF_16_BIT3(s->bit_16[3]));
	ipred_mode0      = ( 0
			+ VMAU_IP_REF_LMDUV_IFO(s->lambda_infouv)
			+ VMAU_IP_REF_LMD16_IFO(s->lambda_info16)
			+ VMAU_IP_REF_LMD8_IFO(s->lambda_info8)
			+ VMAU_IP_REF_LMD4_IFO(s->lambda_info4)
			+ VMAU_IP_REF_NEB_4REF(s->ref_4)
			+ VMAU_IP_REF_NEB_8REF(s->ref_8));
	ipred_c4_if0     = ( 0
			+ VALID_VAL2 ( 0, s->const_4[0])
			+ VALID_VAL2 ( 4, s->const_4[1])
			+ VALID_VAL2 ( 8, s->const_4[2])
			+ VALID_VAL2 (12, s->const_4[3]));
	ipred_c8_if0     = ( 0
			+ VALID_VAL2 ( 0, s->const_8[0])
			+ VALID_VAL2 ( 4, s->const_8[1])
			+ VALID_VAL2 ( 8, s->const_8[2])
			+ VALID_VAL2 (12, s->const_8[3]));
	ipred_mode1      = ( 0
			+ VMAU_IP_REF_C4_IFO(ipred_c4_if0)
			+ VMAU_IP_REF_C8_IFO(ipred_c8_if0));
	ipred_c16_if0    = ( 0
			+ VALID_VAL2 ( 0, s->const_16[0])
			+ VALID_VAL2 ( 4, s->const_16[1])
			+ VALID_VAL2 ( 8, s->const_16[2])
			+ VALID_VAL2 (12, s->const_16[3]));
	ipred_cuv_if0    = ( 0
			+ VALID_VAL2 ( 0, s->const_uv[0])
			+ VALID_VAL2 ( 4, s->const_uv[1])
			+ VALID_VAL2 ( 8, s->const_uv[2])
			+ VALID_VAL2 (12, s->const_uv[3]));
	ipred_mode2      = ( 0
			+ VMAU_IP_REF_C16_IFO(ipred_c16_if0)
			+ VMAU_IP_REF_CUV_IFO(ipred_cuv_if0));

	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_CTRL,     0, ipred_ctrl);
	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_FRM_SIZE, 0, ipred_frm_size);
	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BC_ST,    0, ipred_bc_st);
	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_SLV_MODE, 0, ipred_slv_mode);
	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MOD_CTRL, 0, ipred_mode_ctrl);
	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MODE0,    0, ipred_mode0);
	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MODE1,    0, ipred_mode1);
	GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MODE2,    0, ipred_mode2);
#endif

	/*************************************************************************************
	  DBLK Module
	 *************************************************************************************/
#ifdef CONFIG_SOC_PRJ007
	t32_dblk_cfg  = ( 0
			+ (1 << 31) // 0:265, 1:264
			+ DBLK_CFG_ALPHA(s->alpha_c0_offset + 12)
			+ DBLK_CFG_BETA(s->beta_offset + 12)
			+ DBLK_CFG_NO_LFT(!s->deblock));
	GEN_VDMA_ACFG(chn, RADIX_DBLK_BASE_VDMA + REG_DBLK_CFG,  0, t32_dblk_cfg);
	GEN_VDMA_ACFG(chn, RADIX_DBLK_BASE_VDMA + REG_DBLK_TRIG, 0, (DBLK_SLICE_RUN));
#endif

	/*************************************************************************************
	  MD Module
	 *************************************************************************************/
#ifdef CONFIG_SOC_PRJ008
	md_rcfg[0] = ( 0
			+ VALID_VAL3 (0x1,     0, s->rc_en)
			+ VALID_VAL3 (0xFFFF,  4, s->bu_size - 1)
			+ VALID_VAL3 (0x1,    20, 0)
			+ VALID_VAL3 (0x1,    21, 0)
			+ VALID_VAL3 (0x1,    22, 1)
			+ VALID_VAL3 (0xFF,   23, s->bu_len)
			+ VALID_VAL3 (0x1,    31, s->rc_dly_en));
	md_rcfg[1] = ( 0
			+ VALID_VAL3 (0x7F, 0,  s->bu_max_dqp)
			+ VALID_VAL3 (0x7F, 7,  s->bu_min_dqp)
			+ VALID_VAL3 (0x7,  14, s->rc_max_prop)
			+ VALID_VAL3 (0x7,  17, s->rc_min_prop)
			+ VALID_VAL3 (0x7,  20, s->rc_method)
			+ VALID_VAL3 (0x7F, 23, s->rc_dqp_offset));
	md_rcfg[2] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd[0])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[1]));
	md_rcfg[3] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd[2])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[3]));
	md_rcfg[4] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd[4])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[5]));
	md_rcfg[5] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd[6])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[7]));
	md_rcfg[6] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd[8])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[9]));
	md_rcfg[7] = ( 0
			+ VALID_VAL3 (0xFFFF,  0, s->rc_thd[10])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[11]));
#endif

	md_rcfg[8]                = ( 0
			+ VALID_VAL3 (0xFFFF, 0,  s->tgtbs_offset_unit)
			+ VALID_VAL3 (0xFFFF, 16, s->force_max_dqp_bits));
	md_rcfg[9]                = ( 0
			+ VALID_VAL3 (0xFFFF, 0,  s->adjust_unit_len-1)
			+ VALID_VAL3 (0xFF, 16,  s->adjust_unit_size-1));

#ifdef CONFIG_SOC_PRJ008
	// vmau global
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG5,  0, md_cfg5);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_GBL_CTR,    0, vmau_gbl_ctr);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_VIDEO_TYPE, 0, vmau_video_type);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_Y_GS,   0, vmau_y_gs);
	// tfm config
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_DEADZONE,  0, vmau_ddz);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_DEADZONE1, 0, vmau_ddz1);
	GEN_VDMA_ACFG(chn, RADIX_VMAU_BASE_VDMA + REG_VMAU_ACMASK,    0, vmau_acmask);
	// md config
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG0,    0, vmau_md_cfg0);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG1,    0, vmau_md_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG2,    0, vmau_md_cfg2);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG3,    0, vmau_md_cfg3);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG4,    0, vmau_md_cfg4);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG5,    0, vmau_md_cfg5);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG6,    0, vmau_md_cfg6);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG7,    0, vmau_md_cfg7);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG8,    0, vmau_md_cfg8);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG9,    0, vmau_md_cfg9);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG10,  0, vmau_md_cfg10);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_CFG11,   0, vmau_md_cfg11);

	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_0,  0, md_ned_disable0);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_1,  0, md_ned_disable1);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_2,  0, md_ned_disable2);

	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG0, 0, md_rcfg[0]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG1, 0, md_rcfg[1]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG2, 0, md_rcfg[2]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG3, 0, md_rcfg[3]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG4, 0, md_rcfg[4]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG5, 0, md_rcfg[5]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG6, 0, md_rcfg[6]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG7, 0, md_rcfg[7]);
#endif

#ifdef CONFIG_SOC_PRJ008
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA    + T33_RADIX_REG_MD_RCFG8, 0, md_rcfg[8]);
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA    + T33_RADIX_REG_MD_RCFG9, 0, md_rcfg[9]);
	for (j = 0; j < 40; ++j)
		GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_MD_MODE, 0, s->mode_ctrl_param[j] );
	for (j = 0; j < 42; j++) {
		vmau_ctx = s->state[ j<10 ? ( !s->frame_type ? j+3 : j+14 )
			: j<22 ? j-10+73
			: j<28 ? j-22+64 : j-28+40 ];
		GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_CTX, 0,  vmau_ctx);
	}
	for (i=0; i<s->bu_len; i++) {
		GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + (((1 << 9) + i) << 2), 0, s->rc_info[i]);
	}
	GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + T33_REG_VMAU_GBL_RUN, 0, VMAU_RUN );
#endif

#ifdef CONFIG_SOC_PRJ008
	dblk_cfg  = ( 0
			+ (1 << 31) // 0:265, 1:264
			+ (DBLK_SLICE_RUN << 24)
			+ DBLK_CFG_ALPHA(s->alpha_c0_offset + 12)
			+ DBLK_CFG_BETA(s->beta_offset + 12)
			+ DBLK_CFG_NO_LFT(!s->deblock));
	GEN_VDMA_ACFG(chn, RADIX_DBLK_BASE_VDMA + REG_DBLK_CFG,  0, dblk_cfg);
	dblk_trig = ( 0
			+ VALID_VAL3 (0xffff,  0, s->frame_height)
			+ VALID_VAL3 (0xffff, 16, s->frame_width));
	GEN_VDMA_ACFG(chn, RADIX_DBLK_BASE_VDMA + REG_DBLK_TRIG, 0, dblk_trig);
#endif

	/*************************************************************************************
	  SDE Module
	 *************************************************************************************/
	sde_sl_geom = SDE_SL_GEOM(s->mb_height, s->mb_width, s->first_mby, 0);
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_STAT,     0, 0);
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_GL_CTRL,  0, (SDE_MODE_STEP | 1) );
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_SL_GEOM,  0, sde_sl_geom );
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CODEC_ID, 0, SDE_FMT_H264_ENC);
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG0,     0, 0x3);

#ifdef CONFIG_SOC_PRJ007
	T32_sde_cfg1 = ( 0
			+ VALID_VAL2 (8, ifa->base_qp)
			+ VALID_VAL2 (6, s->mref_en)          // dual-ref enable
			+ VALID_VAL2 (5, s->dct8x8_en)        // dct8x8 enable
			+ VALID_VAL2 (4, s->rm.skip_en)       // reserved
			+ VALID_VAL2 (0, s->frame_type + 1));
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG1, 0, T32_sde_cfg1);
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG5, 0, ((0<<31) | (1<<22)) ); // frame max bs size, [31]:enable, [23:0]:max_size
#endif

#ifdef CONFIG_SOC_PRJ008
	T33_sde_cfg1 = ( 0
			+ VALID_VAL2 (8, ifa->base_qp)
			+ VALID_VAL2 (7, s->nal_en)
			+ VALID_VAL2 (6, s->mref_en)          // dual-ref enable
			+ VALID_VAL2 (5, s->dct8x8_en)        // dct8x8 enable
			+ VALID_VAL2 (4, s->rm.skip_en)       // T33=0;reserved
			+ VALID_VAL2 (0, s->frame_type + 1));
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CFG1, 0, T33_sde_cfg1);
#endif

	for(i=0; i<460; i++){
		idx = s->state[i];
		if(idx <= 63)
			idx = 63 - idx;
		else
			idx -= 64;
		GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_CTX_TBL+i*4, 0,
				(lps_range[idx] | ((s->state[i]>>6) & 0x1)) );
	}
	GEN_VDMA_ACFG(chn, RADIX_SDE_BASE_VDMA + REG_SDE_SL_CTRL,  0, SDE_SLICE_INIT);//init sync

	/*************************************************************************************
	  TMC Module
	 *************************************************************************************/
	GEN_VDMA_ACFG(chn,RADIX_TMC_BASE_VDMA + HERA_REG_TMC_CTRL, 0, ( 0 | (1 << 1) ));

	/*************************************************************************************
	  EMC Module
	 *************************************************************************************/
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_BSFULL_EN,   0, s->bsfull_intr_en);
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_BSFULL_SIZE, 0, s->bsfull_intr_size);
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_BS0,    0, VDMA_ACFG_DHA((unsigned int)s->emc_bs_addr0));    //T33 instead
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_BS1,    0, VDMA_ACFG_DHA((unsigned int)s->emc_bs_addr1));    //T33 instead
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_CPS,    0, VDMA_ACFG_DHA((unsigned int)s->emc_cps_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_AI,     0, VDMA_ACFG_DHA((unsigned int)s->emc_ai_mark_addr));//T33 instead
#ifdef CONFIG_SOC_PRJ007
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_SOBEL, 0, VDMA_ACFG_DHA(s->emc_sobel_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_MIX,    0, VDMA_ACFG_DHA((unsigned int)s->emc_mix_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_FLAG , 0, VDMA_ACFG_DHA(s->emc_flag_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_QPT,    0, VDMA_ACFG_DHA((unsigned int)s->emc_qpt_addr));    //T33 instead
#endif
#ifdef CONFIG_SOC_PRJ008
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_MIX,    0, VDMA_ACFG_DHA((unsigned int)s->emc_mix_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_QPT,    0, VDMA_ACFG_DHA((unsigned int)s->emc_qpt_addr));    //T33 instead
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_MCEC,  0, VDMA_ACFG_DHA((unsigned int)s->emc_mcec_addr));
#endif
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_IPC,    0, VDMA_ACFG_DHA((unsigned int)s->emc_mdc_addr));    //T33 instead
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_MIXC,   0, VDMA_ACFG_DHA((unsigned int)s->emc_mixc_addr));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_ADDR_RRS0,   0, VDMA_ACFG_DHA((unsigned int)ifa->emc_cps_ref0));
#ifdef CONFIG_SOC_PRJ007
	T32_emc_cfg_en = ( 1           // emc init
			| (0x1 <<1)   // bs wr en (always 1)
			| ((s->emc_cps_en    & 0x1)   << 2)
			| ((s->emc_sobel_en  & 0x1)   << 3)
			| ((s->emc_mix_en    & 0x1)   << 4)
			| ((s->emc_mixc_en   & 0x1)   << 5)
			| ((s->emc_flag_en   & 0x1)   << 7)
			| ((ifa->qpg_table_en  & 0x1)   << 9)
			| ((s->rm.mb_mode_use   & 0x1)   << 11)
			| ((ifa->efe_rrs_en    & 0x1)   << 13)
			| ((ifa->ai_mark_en    & 0x1)   << 15) ) ;
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_CFG,        0, T32_emc_cfg_en );
#endif
#ifdef CONFIG_SOC_PRJ008
	T33_emc_cfg_en = ( 0
			+ VALID_VAL3(0x1,  0, 1)                 // emc init
			+ VALID_VAL3(0x1,  1, 1)                 // bs wr en (always 1)
			+ VALID_VAL3(0x1,  2, s->emc_cps_en)
			+ VALID_VAL3(0x1,  3, s->emc_mix_en)
			+ VALID_VAL3(0x1,  6, ifa->qpg_table_en)
			+ VALID_VAL3(0x1,  7, s->mce_cfg_en)
			+ VALID_VAL3(0x1,  8, s->md_cfg_en)
			+ VALID_VAL3(0x1, 10, ifa->efe_rrs_en)
			+ VALID_VAL3(0x1, 12, ifa->ai_mark_en));
	GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + HERA_REG_EMC_CFG,    0, T33_emc_cfg_en );
#endif
	/*************************************************************************************
	  ODMA Module
	 *************************************************************************************/
	//when buf share is new addr(dynamic), else old addr(static)
	odma_jrfc_head_y_addr = (unsigned int)s->head_dsty_addr;//T33 instead
	odma_jrfc_head_c_addr = (unsigned int)s->head_dstc_addr;//T33 instead
	odma_bfsh_base_y_addr = (unsigned int)s->buf_base_yaddr;
	odma_bfsh_bynd_y_addr = (unsigned int)buf_beyond_yaddr;
	odma_y_addr       = s->buf_share_en ? buf_start_yaddr : s->dst_y_pa;// T33 instead
	odma_c_addr       = s->body_combine_en ? odma_y_addr : s->buf_share_en ? buf_start_caddr : s->dst_c_pa;
	odma_bfsh_base_c_addr = s->body_combine_en ? odma_bfsh_base_y_addr : (unsigned int)s->buf_base_caddr;
	odma_bfsh_bynd_c_addr = s->body_combine_en ? odma_bfsh_bynd_y_addr : (unsigned int)buf_beyond_caddr;
	odma_ctrl         = ( 0
			+ VALID_VAL2 (31, s->buf_share_en)
			+ VALID_VAL2 (30, s->jrfcv2_en)  //T33 instead
			+ VALID_VAL2 (29, odma_alg_flag)
			+ VALID_VAL2 (14, s->frame_height)
			+ VALID_VAL2 ( 0, s->frame_width));
	odma_trig         = ( 0
			+ VALID_VAL2 (31, 1)   //0:265, 1:264
			+ VALID_VAL2 ( 6, 0)   //0 is crc en
			+ VALID_VAL2 ( 5, 1)   //init
			+ VALID_VAL2 ( 4, 0)   //ckg low means do clock-gating
			+ VALID_VAL2 ( 1, s->body_combine_en));  //T32=0; combine body into one space

	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_CTRL,            0, odma_ctrl);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BDYA,            0, odma_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BDCA,            0, odma_c_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_HDYA,            0, odma_jrfc_head_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_HDCA,            0, odma_jrfc_head_c_addr);
#ifdef CONFIG_SOC_PRJ007
	odma_jrfc_head_sp_y_addr = (unsigned int)s->rm.spe_y_addr;
	odma_jrfc_head_sp_c_addr = (unsigned int)s->rm.spe_c_addr;
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + T32_REG_ODMA_SPYA, 0, odma_jrfc_head_sp_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + T32_REG_ODMA_SPCA, 0, odma_jrfc_head_sp_c_addr);
#endif
#ifdef CONFIG_SOC_PRJ008
	odma_jrfc_bodyy_bfsh_space = odma_bfsh_bynd_y_addr - odma_y_addr;
	odma_jrfc_bodyc_bfsh_space = odma_bfsh_bynd_c_addr - odma_c_addr;
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + T33_REG_JRFC_YSPACE, 0, odma_jrfc_bodyy_bfsh_space);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + T33_REG_JRFC_CSPACE, 0, odma_jrfc_bodyc_bfsh_space);
#endif

	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BASEY_ADDR, 0, odma_bfsh_base_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BASEC_ADDR, 0, odma_bfsh_base_c_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BEYDY_ADDR, 0, odma_bfsh_bynd_y_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_BUFS_BEYDC_ADDR, 0, odma_bfsh_bynd_c_addr);
	GEN_VDMA_ACFG(chn, RADIX_ODMA_BASE_VDMA + REG_ODMA_TRIG,            0, odma_trig);

	//efe start
	GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA  + REG_EFE_STAT, 0, 1);
	GEN_VDMA_ACFG(chn, RADIX_VDMA_BASE_VDMA + RADIX_REG_CFGC_ACM_CTRL, VDMA_ACFG_TERM, 0);
	//    unsigned int chn_start = (unsigned int)s->des_va;
	//    unsigned int chn_end   = (unsigned int)chn;
	//    unsigned int chn_size  = chn_end - chn_start;
	//    printf("chn_used_size = 0x%x, chn_start = 0x%x, chn_end = 0x%x\n",chn_size, chn_start, chn_end);
	//    H264e_SliceInit_Dump_Chn(s, (unsigned int *)s->des_va, chn_size);
}
