#include "hera.h"//ttt
#include "h265_core.h"
#include <linux/kernel.h>

//#define H265_VDMA_DEBUG
#define VALID_VAL3(vbit, bit, var) (((var) & (vbit)) << (bit))
#define VALID_VAL2(bit, var)       ((var) << (bit))

//context
typedef struct X_ContextModel {
	unsigned char m_state;  ///< internal state variable
	unsigned char bBinsCoded;
} X_ContextModel;

#define RADIX_X_NUM_SPLIT_FLAG_CTX                 3       ///< number of context models for split flag
#define RADIX_X_NUM_SKIP_FLAG_CTX                  3       ///< number of context models for skip flag
#define RADIX_X_NUM_MERGE_FLAG_EXT_CTX             1       ///< number of context models for merge flag of merge extended
#define RADIX_X_NUM_MERGE_IDX_EXT_CTX              1       ///< number of context models for merge index of merge extended
#define RADIX_X_NUM_PART_SIZE_CTX                  4       ///< number of context models for partition size
#define RADIX_X_NUM_PRED_MODE_CTX                  1       ///< number of context models for prediction mode
#define RADIX_X_NUM_ADI_CTX                        1       ///< number of context models for intra prediction
#define RADIX_X_NUM_CHROMA_PRED_CTX                2       ///< number of context models for intra prediction (chroma)
#define RADIX_X_NUM_INTER_DIR_CTX                  5       ///< number of context models for inter prediction direction
#define RADIX_X_NUM_MV_RES_CTX                     2       ///< number of context models for motion vector difference
#define RADIX_X_NUM_REF_NO_CTX                     2       ///< number of context models for reference index
#define RADIX_X_NUM_TRANS_SUBDIV_FLAG_CTX          3       ///< number of context models for transform subdivision flags
#define RADIX_X_NUM_QT_CBF_CTX                     6       ///< number of context models for QT CBF
#define RADIX_X_NUM_QT_ROOT_CBF_CTX                1       ///< number of context models for QT ROOT CBF
#define RADIX_X_NUM_DELTA_QP_CTX                   3       ///< number of context models for dQP
#define RADIX_X_NUM_SIG_CG_FLAG_CTX                2       ///< number of context models for MULTI_LEVEL_SIGNIFICANCE
#define RADIX_X_NUM_SIG_FLAG_CTX                   42      ///< number of context models for sig flag
#define RADIX_X_NUM_SIG_FLAG_CTX_LUMA              27      ///< number of context models for luma sig flag
#define RADIX_X_NUM_SIG_FLAG_CTX_CHROMA            15      ///< number of context models for chroma sig flag
#define RADIX_X_NUM_CTX_LAST_FLAG_XY               18      ///< number of context models for last coefficient position
#define RADIX_X_NUM_CTX_LAST_FLAG_XY_LUMA          15      ///< number of context models for last coefficient position of luma
#define RADIX_X_NUM_CTX_LAST_FLAG_XY_CHROMA        3       ///< number of context models for last coefficient position of chroma
#define RADIX_X_NUM_ONE_FLAG_CTX                   24      ///< number of context models for greater than 1 flag
#define RADIX_X_NUM_ONE_FLAG_CTX_LUMA              16      ///< number of context models for greater than 1 flag of luma
#define RADIX_X_NUM_ONE_FLAG_CTX_CHROMA            8       ///< number of context models for greater than 1 flag of chroma
#define RADIX_X_NUM_ABS_FLAG_CTX                   6       ///< number of context models for greater than 2 flag
#define RADIX_X_NUM_ABS_FLAG_CTX_LUMA              4       ///< number of context models for greater than 2 flag of luma
#define RADIX_X_NUM_ABS_FLAG_CTX_CHROMA            2       ///< number of context models for greater than 2 flag of chroma
#define RADIX_X_NUM_MVP_IDX_CTX                    1       ///< number of context models for MVP index
#define RADIX_X_NUM_SAO_MERGE_FLAG_CTX             1       ///< number of context models for SAO merge flags
#define RADIX_X_NUM_SAO_TYPE_IDX_CTX               1       ///< number of context models for SAO type index
#define RADIX_X_NUM_TRANSFORMSKIP_FLAG_CTX         1       ///< number of context models for transform skipping
#define RADIX_X_NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX  1
#define RADIX_X_CNU                                154     ///< dummy initialization value for unused context models 'Context model Not Used'
#define RADIX_X_NUM_TOTAL                          156

// Offset for context
#define RADIX_X_OFF_SPLIT_FLAG_CTX                  (0)
#define RADIX_X_OFF_SKIP_FLAG_CTX                   (RADIX_X_OFF_SPLIT_FLAG_CTX         +     RADIX_X_NUM_SPLIT_FLAG_CTX)
#define RADIX_X_OFF_MERGE_FLAG_EXT_CTX              (RADIX_X_OFF_SKIP_FLAG_CTX          +     RADIX_X_NUM_SKIP_FLAG_CTX)
#define RADIX_X_OFF_MERGE_IDX_EXT_CTX               (RADIX_X_OFF_MERGE_FLAG_EXT_CTX     +     RADIX_X_NUM_MERGE_FLAG_EXT_CTX)
#define RADIX_X_OFF_PART_SIZE_CTX                   (RADIX_X_OFF_MERGE_IDX_EXT_CTX      +     RADIX_X_NUM_MERGE_IDX_EXT_CTX)
#define RADIX_X_OFF_PRED_MODE_CTX                   (RADIX_X_OFF_PART_SIZE_CTX          +     RADIX_X_NUM_PART_SIZE_CTX)
#define RADIX_X_OFF_ADI_CTX                         (RADIX_X_OFF_PRED_MODE_CTX          +     RADIX_X_NUM_PRED_MODE_CTX)
#define RADIX_X_OFF_CHROMA_PRED_CTX                 (RADIX_X_OFF_ADI_CTX                +     RADIX_X_NUM_ADI_CTX)
#define RADIX_X_OFF_DELTA_QP_CTX                    (RADIX_X_OFF_CHROMA_PRED_CTX        +     RADIX_X_NUM_CHROMA_PRED_CTX)
#define RADIX_X_OFF_INTER_DIR_CTX                   (RADIX_X_OFF_DELTA_QP_CTX           +     RADIX_X_NUM_DELTA_QP_CTX)
#define RADIX_X_OFF_REF_NO_CTX                      (RADIX_X_OFF_INTER_DIR_CTX          +     RADIX_X_NUM_INTER_DIR_CTX)
#define RADIX_X_OFF_MV_RES_CTX                      (RADIX_X_OFF_REF_NO_CTX             +     RADIX_X_NUM_REF_NO_CTX)
#define RADIX_X_OFF_QT_CBF_CTX                      (RADIX_X_OFF_MV_RES_CTX             +     RADIX_X_NUM_MV_RES_CTX)
#define RADIX_X_OFF_TRANS_SUBDIV_FLAG_CTX           (RADIX_X_OFF_QT_CBF_CTX             +     RADIX_X_NUM_QT_CBF_CTX)
#define RADIX_X_OFF_QT_ROOT_CBF_CTX                 (RADIX_X_OFF_TRANS_SUBDIV_FLAG_CTX  +     RADIX_X_NUM_TRANS_SUBDIV_FLAG_CTX)
#define RADIX_X_OFF_SIG_CG_FLAG_CTX                 (RADIX_X_OFF_QT_ROOT_CBF_CTX        +     RADIX_X_NUM_QT_ROOT_CBF_CTX)
#define RADIX_X_OFF_SIG_FLAG_CTX                    (RADIX_X_OFF_SIG_CG_FLAG_CTX        + 2 * RADIX_X_NUM_SIG_CG_FLAG_CTX)
#define RADIX_X_OFF_CTX_LAST_FLAG_X                 (RADIX_X_OFF_SIG_FLAG_CTX           +     RADIX_X_NUM_SIG_FLAG_CTX)
#define RADIX_X_OFF_CTX_LAST_FLAG_Y                 (RADIX_X_OFF_CTX_LAST_FLAG_X        +     RADIX_X_NUM_CTX_LAST_FLAG_XY)
#define RADIX_X_OFF_ONE_FLAG_CTX                    (RADIX_X_OFF_CTX_LAST_FLAG_Y        +     RADIX_X_NUM_CTX_LAST_FLAG_XY)
#define RADIX_X_OFF_ABS_FLAG_CTX                    (RADIX_X_OFF_ONE_FLAG_CTX           +     RADIX_X_NUM_ONE_FLAG_CTX)
#define RADIX_X_OFF_MVP_IDX_CTX                     (RADIX_X_OFF_ABS_FLAG_CTX           +     RADIX_X_NUM_ABS_FLAG_CTX)
#define RADIX_X_OFF_SAO_MERGE_FLAG_CTX              (RADIX_X_OFF_MVP_IDX_CTX            +     RADIX_X_NUM_MVP_IDX_CTX)
#define RADIX_X_OFF_SAO_TYPE_IDX_CTX                (RADIX_X_OFF_SAO_MERGE_FLAG_CTX     +     RADIX_X_NUM_SAO_MERGE_FLAG_CTX)
#define RADIX_X_OFF_TRANSFORMSKIP_FLAG_CTX          (RADIX_X_OFF_SAO_TYPE_IDX_CTX       +     RADIX_X_NUM_SAO_TYPE_IDX_CTX)
#define RADIX_X_OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX   (RADIX_X_OFF_TRANSFORMSKIP_FLAG_CTX + 2 * RADIX_X_NUM_TRANSFORMSKIP_FLAG_CTX)
#define RADIX_N_MARADIX_X_OFF_CTX_MOD               (RADIX_X_OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX + RADIX_X_NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX)

static unsigned char
X_INIT_CU_TRANSQUANT_BYPASS_FLAG[3][RADIX_X_NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX] =
{
	{ 154 },
	{ 154 },
	{ 154 },
};

// initial probability for split flag
static unsigned char
X_INIT_SPLIT_FLAG[3][RADIX_X_NUM_SPLIT_FLAG_CTX] =
{
	{ 107,  139,  126, },
	{ 107,  139,  126, },
	{ 139,  141,  157, },
};

static unsigned char
X_INIT_SKIP_FLAG[3][RADIX_X_NUM_SKIP_FLAG_CTX] =
{
	{ 197,  185,  201, },
	{ 197,  185,  201, },
	{ RADIX_X_CNU,  RADIX_X_CNU,  RADIX_X_CNU, },
};

static unsigned char
X_INIT_MERGE_FLAG_EXT[3][RADIX_X_NUM_MERGE_FLAG_EXT_CTX] =
{
	{ 154, },
	{ 110, },
	{ RADIX_X_CNU, },
};

static unsigned char
X_INIT_MERGE_IDX_EXT[3][RADIX_X_NUM_MERGE_IDX_EXT_CTX] =
{
	{ 137, },
	{ 122, },
	{ RADIX_X_CNU, },
};

static unsigned char
X_INIT_PART_SIZE[3][RADIX_X_NUM_PART_SIZE_CTX] =
{
	{ 154,  139,  154, 154 },
	{ 154,  139,  154, 154 },
	{ 184,  RADIX_X_CNU,  RADIX_X_CNU, RADIX_X_CNU },
};

static unsigned char
X_INIT_PRED_MODE[3][RADIX_X_NUM_PRED_MODE_CTX] =
{
	{ 134, },
	{ 149, },
	{ RADIX_X_CNU, },
};

static unsigned char
X_INIT_INTRA_PRED_MODE[3][RADIX_X_NUM_ADI_CTX] =
{
	{ 183, },
	{ 154, },
	{ 184, },
};

static unsigned char
X_INIT_CHROMA_PRED_MODE[3][RADIX_X_NUM_CHROMA_PRED_CTX] =
{
	{ 152,  139, },
	{ 152,  139, },
	{  63,  139, },
};

static unsigned char
X_INIT_INTER_DIR[3][RADIX_X_NUM_INTER_DIR_CTX] =
{
	{  95,   79,   63,   31,  31, },
	{  95,   79,   63,   31,  31, },
	{ RADIX_X_CNU,  RADIX_X_CNU,  RADIX_X_CNU,  RADIX_X_CNU, RADIX_X_CNU, },
};

static unsigned char
X_INIT_MVD[3][RADIX_X_NUM_MV_RES_CTX] =
{
	{ 169,  198, },
	{ 140,  198, },
	{ RADIX_X_CNU,  RADIX_X_CNU, },
};

static unsigned char
X_INIT_REF_PIC[3][RADIX_X_NUM_REF_NO_CTX] =
{
	{ 153,  153 },
	{ 153,  153 },
	{ RADIX_X_CNU,  RADIX_X_CNU },
};

static unsigned char
X_INIT_DQP[3][RADIX_X_NUM_DELTA_QP_CTX] =
{
	{ 154,  154,  154, },
	{ 154,  154,  154, },
	{ 154,  154,  154, },
};

static unsigned char
X_INIT_QT_CBF[3][RADIX_X_NUM_QT_CBF_CTX] =
{
	{ 153,  111,  149,   92,  167,  154, },
	{ 153,  111,  149,  107,  167,  154, },
	{ 111,  141,   94,  138,  182,  154, },
};

static unsigned char
X_INIT_QT_ROOT_CBF[3][RADIX_X_NUM_QT_ROOT_CBF_CTX] =
{
	{  79, },
	{  79, },
	{ RADIX_X_CNU, },
};

static unsigned char
X_INIT_LAST[3][RADIX_X_NUM_CTX_LAST_FLAG_XY] =
{
	{ 125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
		108,  123,   93 },
	{ 125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
		108,  123,  108 },
	{ 110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79,
		108,  123,   63 },
};

static unsigned char
X_INIT_SIG_CG_FLAG[3][2 * RADIX_X_NUM_SIG_CG_FLAG_CTX] =
{
	{ 121,  140,
		61,  154, },
	{ 121,  140,
		61,  154, },
	{  91,  171,
		134,  141, },
};

static unsigned char
X_INIT_SIG_FLAG[3][RADIX_X_NUM_SIG_FLAG_CTX] =
{
	{ 170,  154,  139,  153,  139,  123,  123,   63,  124,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  138,  138,  122,  121,  122,  121,  167,  151,  183,  140,  151,  183,  140,  },
	{ 155,  154,  139,  153,  139,  123,  123,   63,  153,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  123,  123,  107,  121,  107,  121,  167,  151,  183,  140,  151,  183,  140,  },
	{ 111,  111,  125,  110,  110,   94,  124,  108,  124,  107,  125,  141,  179,  153,  125,  107,  125,  141,  179,  153,  125,  107,  125,  141,  179,  153,  125,  140,  139,  182,  182,  152,  136,  152,  136,  153,  136,  139,  111,  136,  139,  111,  },
};

static unsigned char
X_INIT_ONE_FLAG[3][RADIX_X_NUM_ONE_FLAG_CTX] =
{
	{ 154,  196,  167,  167,  154,  152,  167,  182,  182,  134,  149,  136,  153,  121,  136,  122,  169,  208,  166,  167,  154,  152,  167,  182, },
	{ 154,  196,  196,  167,  154,  152,  167,  182,  182,  134,  149,  136,  153,  121,  136,  137,  169,  194,  166,  167,  154,  167,  137,  182, },
	{ 140,   92,  137,  138,  140,  152,  138,  139,  153,   74,  149,   92,  139,  107,  122,  152,  140,  179,  166,  182,  140,  227,  122,  197, },
};

static unsigned char
X_INIT_ABS_FLAG[3][RADIX_X_NUM_ABS_FLAG_CTX] =
{
	{ 107,  167,   91,  107,  107,  167, },
	{ 107,  167,   91,  122,  107,  167, },
	{ 138,  153,  136,  167,  152,  152, },
};

static unsigned char
X_INIT_MVP_IDX[3][RADIX_X_NUM_MVP_IDX_CTX] =
{
	{ 168 },
	{ 168 },
	{ RADIX_X_CNU },
};

static unsigned char
X_INIT_SAO_MERGE_FLAG[3][RADIX_X_NUM_SAO_MERGE_FLAG_CTX] =
{
	{ 153,  },
	{ 153,  },
	{ 153,  },
};

static unsigned char
X_INIT_SAO_TYPE_IDX[3][RADIX_X_NUM_SAO_TYPE_IDX_CTX] =
{
	{ 160, },
	{ 185, },
	{ 200, },
};

static unsigned char
X_INIT_TRANS_SUBDIV_FLAG[3][RADIX_X_NUM_TRANS_SUBDIV_FLAG_CTX] =
{
	{ 224,  167,  122, },
	{ 124,  138,   94, },
	{ 153,  138,  138, },
};

static unsigned char
X_INIT_TRANSFORMSKIP_FLAG[3][2 * RADIX_X_NUM_TRANSFORMSKIP_FLAG_CTX] =
{
	{ 139,  139 },
	{ 139,  139 },
	{ 139,  139 },
};

unsigned char xvec_sbacInit(int qp, int initValue)
{

	int qp_clip = qp < 0 ? 0 : qp > 51 ? 51 : qp;

	int  slope      = (initValue >> 4) * 5 - 45;
	int  offset     = ((initValue & 15) << 3) - 16;
	int tmp_max = (((slope * qp_clip) >> 4) + offset) > 1 ? (((slope * qp_clip) >> 4) + offset) : 1;
	int  initState  = tmp_max < 126 ? tmp_max : 126;
	int mpState = (initState >= 64);
	unsigned char N_state = ((mpState ? (initState - 64) : (63 - initState)) << 1) + mpState;

	return N_state;
}

void xvec_initBuffer(X_ContextModel* X_contextModel, int sliceType, int qp, unsigned char* ctxModel, int size)
{
	int n;
	ctxModel += sliceType * size;
	for (n = 0; n < size; n++)
	{
		X_contextModel[n].m_state = xvec_sbacInit(qp, ctxModel[n]);
		X_contextModel[n].bBinsCoded = 0;
	}
}

#ifdef H265_VDMA_DEBUG
void H265E_sort(unsigned int *addr, unsigned int *val, int n, int module_idx)
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

static unsigned int rst[16][2][512];
void H265E_SliceInit_Dump_Chn(struct h265_slice_info *s, unsigned int *chn, unsigned int chn_size)
{
	int i,m,k;
	int chn_num=0;
	unsigned int RADIX_VDMA_BASE_VDMA = (RADIX_HID_M1 << 15);
	unsigned int RADIX_ODMA_BASE_VDMA = (RADIX_HID_M2 << 15);
	unsigned int RADIX_TMC_BASE_VDMA  = (RADIX_HID_M3 << 15);
	unsigned int RADIX_EFE_BASE_VDMA  = (RADIX_HID_M4 << 15);
	unsigned int RADIX_JRFD_BASE_VDMA = (RADIX_HID_M5 << 15);
	unsigned int RADIX_MCE_BASE_VDMA  = (RADIX_HID_M6 << 15);
	unsigned int RADIX_TFM_BASE_VDMA  = (RADIX_HID_M7 << 15);
	unsigned int RADIX_MD_BASE_VDMA   = (RADIX_HID_M8 << 15);
	unsigned int RADIX_DT_BASE_VDMA   = (RADIX_HID_M9 << 15);
	unsigned int RADIX_DBLK_BASE_VDMA = (RADIX_HID_M10 << 15);
	unsigned int RADIX_SAO_BASE_VDMA  = (RADIX_HID_M11 << 15);
	unsigned int RADIX_BC_BASE_VDMA   = (RADIX_HID_M12 << 15);
	unsigned int RADIX_SDE_BASE_VDMA  = (RADIX_HID_M13 << 15);
	unsigned int RADIX_IPRED_BASE_VDMA= (RADIX_HID_M14 << 15);
	unsigned int RADIX_STC_BASE_VDMA  = (RADIX_HID_M15 << 15);
	unsigned int RADIX_EMC_BASE_VDMA  = (RADIX_HID_M18 << 15);
	int cnt[16]={0};
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
			case RADIX_HID_M18:  rst[15][0][cnt[15]] = chn[i+1]; rst[15][1][cnt[15]] = chn[i]; cnt[15]++;break;
			default:printk(KERN_DEBUG"chn[%d]: error(hid=%d)\n",i/2, hid);
		}
	}
	char *module_str[16] = {"VDMA","ODMA","TMC","EFE","JRFD","MCE","TFM","MD","DT","DBLK","SAO","BC","SDE","IPRED","STC","EMC"};
	unsigned int efe_addr[20] = {0xc, //RADIX_REG_EFE_RAWY_ADDR		0xc
		0x10,//RADIX_REG_EFE_RAWC_ADDR		0x10
		0x18,//RADIX_REG_EFE_REF_ADDR		0x18
	};
	unsigned int mce_addr[20] = {0x70, //RADIX_REG_MCE_BFSH_ADRY       0x0070
		0x74, //RADIX_REG_MCE_BFSH_ADRC       0x0074
		0x78, //T33_RADIX_REG_MCE_BYND_YADDR      0x0078
		0x7C, //T33_RADIX_REG_MCE_BYND_CADDR      0x007C
		0x800,//RADIX_SLUT_MCE_RLUT(l, i)     (0x0800 + (i)*8 + (l)*0x80)
		0x804,//RADIX_SLUT_MCE_RLUT(l, i)     (0x0800 + (i)*8 + (l)*0x80)
	};
	unsigned int odma_addr[20] = {0x8, //RADIX_REG_ODMA_TLY_BA                0x8
		0xc, //RADIX_REG_ODMA_TLC_BA                0xC
		0x20,//RADIX_REG_ODMA_JRFC_HEAD_Y_BA        0x20
		0x24,//RADIX_REG_ODMA_JRFC_HEAD_C_BA        0x24
		0x28,//T33_RADIX_REG_ODMA_BUSH_BODY_Y_SPACE     0x28
		0x2c,//T33_RADIX_REG_ODMA_BUSH_BODY_C_SPACE     0x2C
		0x30,//RADIX_REG_ODMA_BFSH_BASE_Y_BA        0x30
		0x34,//RADIX_REG_ODMA_BFSH_BASE_C_BA        0x34
		0x38,//RADIX_REG_ODMA_BFSH_BYND_Y_BA        0x38
		0x3c,//RADIX_REG_ODMA_BFSH_BYND_C_BA        0x3C
		0x168,//T33_BFSH_LAST_YBODY_ADDR                 0x168
		0x16c,//T33_BFSH_LAST_CBODY_ADDR                 0x16C
		0x174,//T33_RADIX_REG_ODMA_REF_HEAD_BASE         0x174
		0x178,//T33_RADIX_REG_ODMA_REF_BODY_BASE         0x178
		0x17c,//T33_RADIX_REG_ODMA_REF_BODY_LAST         0x17C
		0x180,//T33_RADIX_REG_ODMA_REF_BODY_START        0x180
	};
	unsigned int jrfd_addr[20] = {0x14,//RADIX_REG_JRFD_BDYA             0x14
		0x18,//RADIX_REG_JRFD_BDCA             0x18
		0x20,//RADIX_REG_JRFD_BUFS_BASEY_ADDR  0x20
		0x24,//RADIX_REG_JRFD_BUFS_BASEC_ADDR  0x24
		0x28,//RADIX_REG_JRFD_BUFS_BEYDY_ADDR  0x28
		0x2c,//RADIX_REG_JRFD_BUFS_BEYDC_ADDR  0x2C
	};
	unsigned int tfm_addr[20] = {0x120};//RADIX_TFM_RAM_ADDR		(4*24)
	unsigned int sde_addr[20] = {0x8};//RADIX_REG_SDE_BS_ADDR       0x8
	unsigned int emc_addr[20] = {0x8,  //REG_EMC_DBLK_ADDR               0x0008
		0xc,  //HERA_REG_EMC_ADDR_CPS           0xc
		0x10, //HERA_REG_EMC_ADDR_SOBEL         0x10
		0x14, //HERA_REG_EMC_ADDR_MIX           0x14
		0x18, //HERA_REG_EMC_ADDR_MVO           0x18
		0x1c, //HERA_REG_EMC_ADDR_BS1           0x1c
		0x20, //HERA_REG_EMC_ADDR_FLAG          0x20
		0x24, //HERA_REG_EMC_ADDR_MOD           0x24
		0x28, //HERA_REG_EMC_ADDR_QPT           0x28
		0x2c, //HERA_REG_EMC_ADDR_MCEC          0x2c
		0x30, //HERA_REG_EMC_ADDR_IPC           0x30
		0x34, //HERA_REG_EMC_ADDR_MVI           0x34
		0x38, //HERA_REG_EMC_ADDR_RRS0          0x38
		0x3c, //HERA_REG_EMC_ADDR_RRS1          0x3c
		0x50, //HERA_REG_EMC_ADDR_AI            0x50
		0x54, //HERA_REG_EMC_ADDR_MIXC          0x54
		0x68  //HERA_REG_EMC_ADDR_CHN           0x68
	};
	int odma_addr_cnt = 16;
	int mce_addr_cnt  = 6;
	int efe_addr_cnt  = 3;
	int jrfd_addr_cnt = 6;
	int tfm_addr_cnt  = 1;
	int sde_addr_cnt  = 1;
	int emc_addr_cnt  = 17;
	static int cont_frm = 0;
	for(m=0;m<16;m++) {
		printk(KERN_DEBUG"kern-M[%d] = %d------------frm=%d.\n",m, cnt[m],cont_frm);
		if(cnt[m] > 1)
			H265E_sort(&rst[m][0][0],  &rst[m][1][0],  cnt[m],   m);
		if(m==1) { //odma
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<odma_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == odma_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					printk(KERN_DEBUG"kern-[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==3) {//efe
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<efe_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == efe_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					printk(KERN_DEBUG"kern-[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==4){//jrfd
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<jrfd_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == jrfd_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					printk(KERN_DEBUG"kern-[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==5){//mce
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<mce_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == mce_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					printk(KERN_DEBUG"kern-[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==6){//tfm
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<tfm_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == tfm_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					printk(KERN_DEBUG"kern-[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==12){//sde
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<sde_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == sde_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					printk(KERN_DEBUG"kern-[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else if(m==15){//emc
			for(i=0;i<cnt[m];i++)  {
				int break_flag = 0;
				for(k=0;k<emc_addr_cnt;k++) {
					if((rst[m][0][i]&0x7FFF) == emc_addr[k]) {break_flag = 1;break;}
				}
				if(break_flag)
					printk(KERN_DEBUG"kern-[%3d]: [%s]  addr\n ",i,  module_str[m]);
				else
					printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
		else {
			for(i=0;i<cnt[m];i++)  {
				printk(KERN_DEBUG"kern-[%3d]: [%s]  0x%8x 0x%8x: 0x%x\n ",i,  module_str[m], rst[m][0][i]&0xFFFFF, rst[m][0][i]&0x7FFF, rst[m][1][i]);
			}
		}
	}

	cont_frm++;
}
#endif

void H265E_T32V_SliceInit(struct h265_slice_info *s)//ttt
{
	unsigned int i, j;
	int checksum = 0;
	volatile unsigned int *chn = (volatile unsigned int *)s->des_va;
	//this is state cfg and need open for bc ,sde ,ipred.
	unsigned char all_init_state[RADIX_X_NUM_TOTAL];
	X_ContextModel X_contextModels[RADIX_X_NUM_TOTAL];
	int qp = s->frame_qp;
	int sliceType = s->frame_type;

	int lcu_num_w    = (s->frame_width%64) == 0 ? (s->frame_width >> 6) : (s->frame_width >> 6) + 1;
	int lcu_num_h    = (s->frame_height%64) == 0 ? (s->frame_height >> 6) : (s->frame_height >> 6) + 1;
	int inter_cu8    = (s->frame_type == 2) ? 0 : (s->inter_mode[3][0] | s->inter_mode[3][3]);
	int inter_cu16   = (s->frame_type == 2) ? 0 : (s->inter_mode[2][0] | s->inter_mode[2][1] | s->inter_mode[2][2] | s->inter_mode[2][3]);
	int inter_cu32   = (s->frame_type == 2) ? 0 : (s->inter_mode[1][0] | s->inter_mode[1][1] | s->inter_mode[1][2] | s->inter_mode[1][3]);
	int inter_cu64   = (s->frame_type == 2) ? 0 : s->inter_mode[0][3];
	int intra_cu8    = s->ipred_size&0x1;
	int intra_cu16   = (s->ipred_size>>1)&0x1;
	int intra_cu32   = (s->ipred_size>>2)&0x1;
	int cu8_enable   = inter_cu8  | intra_cu8;
	int cu16_enable  = inter_cu16 | intra_cu16;
	int cu32_enable  = inter_cu32 | intra_cu32;
	int cu64_enable  = inter_cu64;
	int cu_enable    = ((s->frame_type == 2) ? (cu8_enable | (cu16_enable << 1) | (cu32_enable << 2)) :
			(cu8_enable | (cu16_enable << 1) | (cu32_enable << 2) | (cu64_enable << 3)) );
	int inter_enable = (s->frame_type == 2) ? 0 : (inter_cu8 | (inter_cu16 << 1) | (inter_cu32 << 2) | (inter_cu64 << 3));
	int intra_enable = intra_cu8 | (intra_cu16 << 1) | (intra_cu32 << 2);

	/*************************************************************************************
	  VDMA Module
	 *************************************************************************************/
	/* common slave */
	//unsigned int RADIX_CFGC_BASE_VDMA = (RADIX_HID_M0 << 15);
	unsigned int RADIX_VDMA_BASE_VDMA = (RADIX_HID_M1 << 15);
	unsigned int RADIX_ODMA_BASE_VDMA = (RADIX_HID_M2 << 15);
	unsigned int RADIX_TMC_BASE_VDMA  = (RADIX_HID_M3 << 15);
	unsigned int RADIX_EFE_BASE_VDMA  = (RADIX_HID_M4 << 15);
	unsigned int RADIX_JRFD_BASE_VDMA = (RADIX_HID_M5 << 15);
	unsigned int RADIX_MCE_BASE_VDMA  = (RADIX_HID_M6 << 15);
	unsigned int RADIX_TFM_BASE_VDMA  = (RADIX_HID_M7 << 15);
	unsigned int RADIX_MD_BASE_VDMA   = (RADIX_HID_M8 << 15);
	unsigned int RADIX_DT_BASE_VDMA   = (RADIX_HID_M9 << 15);
	unsigned int RADIX_DBLK_BASE_VDMA = (RADIX_HID_M10 << 15);
	unsigned int RADIX_SAO_BASE_VDMA  = (RADIX_HID_M11 << 15);
	unsigned int RADIX_BC_BASE_VDMA   = (RADIX_HID_M12 << 15);
	unsigned int RADIX_SDE_BASE_VDMA  = (RADIX_HID_M13 << 15);
	unsigned int RADIX_IPRED_BASE_VDMA= (RADIX_HID_M14 << 15);
	unsigned int RADIX_STC_BASE_VDMA  = (RADIX_HID_M15 << 15);
	unsigned int RADIX_EMC_BASE_VDMA  = (RADIX_HID_M18 << 15);

	/*************************************************************************************
	  BC Module
	 *************************************************************************************/
	int qkcf_vector_last   = 0;
	int qkcf_vector_signcg = 0;
	int qkcf_vector_sign   = 0;
	int qkcf_vector_m1     = 0;
	int bc_cfg0     = 0;
	int bc_cfg1     = 0;
	int T33_bc_cfg2 = 0;
	int T32_bc_cfg2 = 0;

	/* state slave, y */
	int ii = 0;
	int max_bs_en     = 0;
	int max_bs_size   = 0;

	unsigned int T32_sde_cfg0 = 0;
	unsigned int T33_sde_cfg0 = 0;
	unsigned int sde_cfg1 = 0;
	unsigned int sde_cfg2 = 0;

	int ipmd_mpm_cost_biass32 = 0;
	int ipmd_mpm_cost_biasw32 = 0;
	int ipmd_mpm_cost_biass16 = 0;
	int ipmd_mpm_cost_biasw16 = 0;
	int ipmd_mpm_cost_biass8  = 0;
	int ipmd_mpm_cost_biasw8  = 0;
	int T33_ipmd_angle_bias   = 0;
	int T32_ipmd_angle_bias   = 0;
	int ipmd_dm_bias32        = 0;
	int ipmd_dm_bias16        = 0;
	int ipmd_dm_bias8         = 0;
	int intra_bits0           = 0; //8*4bits0
	int intra_bits1           = 0; //8*4bits
	int intra_bits2           = 0; //8*4bits
	int intra_bits3           = 0;//8*4bits
	int intra_bits4           = 0; //8*4bits
	int intra_bits5           = 0; //8*4bits
	int intra_bits6           = 0; //8*4bits
	int intra_bits7           = 0; //8*4bits
	int intra_bits8           = 0; //8*4bits
	int intra_bits9           = 0; //8*4bits
	unsigned int ipred_frm_size   = 0;
	unsigned int ipred_slv_mode   = 0;
	unsigned int ipred_mode_ctrl  = 0;
	unsigned int ipred_mode0      = 0;
	unsigned int ipred_mode1      = 0;
	unsigned int ipred_mode2      = 0;
	unsigned int ipred_ctrl       = 0;

	unsigned int tfm_cfg_grp[118];
	unsigned int tfm_ctrl = 0;
	unsigned int tfm_frm_size = 0;
	unsigned int x264_en=0;
	unsigned int tfm_init = 0;

	unsigned int lcu_width  = 0;
	unsigned int lcu_height = 0;
	int pre_pmd_en      = 0;
	int pre_csd_en_cu8  = 0;
	int pre_csd_en_cu16 = 0;
	int csd_bias_en_all = 0;
	int lambda_bias_en_all = 0;
	int slice_qp        = 0;
	int dqp_enable      = 0;
	int dqp_depth_cu16  = 0;
	int pmd_bias_en_all = 0;
	unsigned int md_cfg0 = 0;
	unsigned int md_cfg1 = 0;
	unsigned int md_cfg2 = 0;
	unsigned int md_cfg3 = 0;
	unsigned int md_cfg4 = 0;
	/* in T40, force skip is used to limite P frame max size
	 * if sde_bslen exceed max_bs_size in Bytes, only select cu64 skip
	 */
	int md_fskip_max_size  = 0; // 1M Byte
	int md_fskip_en  = 0;
	unsigned int md_cfg5 = 0;
	unsigned int md_cfg6 = 0;
	unsigned int md_cfg7 = 0;
	unsigned int md_jrfc_exceed_en = 0;
	unsigned int md_cfg8 = 0;
	unsigned int md_scfg[4] = {0,0,0,0};
	unsigned int md_pmd_cfg_ofst = 0;
	unsigned int md_lambda_cfg_ofst = 0; // lambda configure
	unsigned int md_rcfg[10] = {0,0,0,0,0,0,0,0,0,0}; // // rate control register and ram cfg. rc cfg

	unsigned int md_pmon_cfg_ofst = 0;
	int pmon_mode = 0; // 0:mpt_empty;  1:ipt_rdy&mpt_empty;  2:ipt_empty
	unsigned int md_pmon_cfg0  = 0;
	/* cfg: | 12bit | 10bit | 10bit | */
	unsigned int T33_md_pmon_cfg1  = 0;
	unsigned int T32_md_pmon_cfg1  = 0;
	unsigned int T33_md_pmon_cfg2  = 0;
	unsigned int T32_md_pmon_cfg2  = 0;
	unsigned int T33_md_pmon_cfg3  = 0;
	unsigned int T32_md_pmon_cfg3  = 0;
	unsigned int T33_md_pmon_cfg4  = 0;
	unsigned int T32_md_pmon_cfg4  = 0;
	unsigned int md_pmon_cfg5  = 0;
	unsigned int T33_md_pmon_cfg6  = 0;
	unsigned int T32_md_pmon_cfg6  = 0;
	unsigned int ppmd_cost_thr = 0;
	unsigned int ppmd_qp_thrd_0 = 0;
	unsigned int ppmd_qp_thrd_1 = 0;
	//pmd_thr_array
	unsigned int pmd_thr0  = 0; //{55, 36}
	unsigned int pmd_thr1  = 0; //{52, 30}
	unsigned int pmd_thr2  = 0; //{50, 30}
	unsigned int pmd_thr3  = 0; //{57, 38}
	unsigned int pmd_thr4  = 0; //{54, 32}
	unsigned int pmd_thr5  = 0; //{53, 31}
	unsigned int pmd_thr6  = 0; //{59, 41}
	unsigned int pmd_thr7  = 0; //{57, 33}
	unsigned int pmd_thr8  = 0; //{56, 32}
	unsigned int pmd_thr9  = 0; //{60, 41}
	unsigned int pmd_thr10 = 0; //{62, 41}
	unsigned int pmd_thr11 = 0; //{59, 35}
	unsigned int pmd_thr12 = 0; //{59, 33}
	unsigned int pmd_thr13 = 0; //{57, 45}
	unsigned int pmd_thr14 = 0; //{57, 30}
	unsigned int pmd_thr15 = 0; //{56, 30}
	unsigned int pmd_thr16 = 0; //{59, 45}
	unsigned int pmd_thr17 = 0; //{58, 32}
	unsigned int pmd_thr18 = 0; //{57, 32}
	unsigned int pmd_thr19 = 0; //{60, 45}
	unsigned int pmd_thr20 = 0; //{62, 45}
	unsigned int pmd_thr21 = 0; //{60, 35}
	unsigned int pmd_thr22 = 0; //{76, 51}
	unsigned int pmd_thr23 = 0;
	unsigned int pmd_thr24 = 0;
	unsigned int pmd_thr25 = 0;
	unsigned int pmd_thr26 = 0;
	unsigned int pmd_thr27 = 0;
	unsigned int pmd_thr28 = 0;
	unsigned int pmd_thr29 = 0;
	unsigned int pmd_thr_idx0 = 0;
	unsigned int pmd_thr_idx1 = 0;
	unsigned int pmd_thr_idx2 = 0;
	unsigned int pmd_thr_idx3 = 0;
	unsigned int pmd_thr_idx4 = 0;
	unsigned int pmd_thr_idx5 = 0;
	unsigned int pmd_thr_idx6 = 0;
	unsigned int pmd_thr_idx7 = 0;
	unsigned int pmd_thr_idx8 = 0;
	unsigned int pmd_thr_idx9 = 0;
	unsigned int pmd_thr_idx10 = 0;
	unsigned int pmd_thr_idx11 = 0;
	unsigned int pmd_thr_idx12 = 0;
	unsigned int pmd_thr_idx13 = 0;
	unsigned int pmd_thr_idx14 = 0;
	unsigned int pmd_thr_idx15 = 0;
	unsigned int pmd_thr_idx16 = 0;
	unsigned int pmd_thr_idx17 = 0;
	unsigned int pmd_thr_idx18 = 0;
	unsigned int pmd_thr_idx19 = 0;
	unsigned int pmd_thr_idx20 = 0;
	unsigned int pmd_thr_idx21 = 0;
	unsigned int md_ned_score_tbl0 = 0;
	unsigned int md_ned_score_tbl1 = 0;
	unsigned int md_ned_score_tbl2 = 0;
	unsigned int md_ned_score_tbl3 = 0;
	unsigned int md_ned_score_tbl4 = 0;
	unsigned int md_ned_disable0   = 0;
	unsigned int md_ned_disable1   = 0;
	unsigned int T33_md_ned_disable2   = 0;
	unsigned int T32_md_ned_disable2   = 0;
	unsigned int mosaic_cfg_0      = 0;
	unsigned int md_rcn_plain_cfg0 = 0;
	unsigned int md_rcn_plain_cfg1 = 0;
	unsigned int csd_qp_thrd_0      = 0;
	//- dlambda add by lqzhang
	int abs_dlambda_simple_dqp  = 0;
	int abs_dlambda_edge_dqp    = 0;

	//- dlambda add by hjiang
	int abs_dlambda_recovery_dqp  = 0;
	unsigned int T32_md_dlambda_cfg0 = 0;
	unsigned int T33_md_dlambda_cfg0 = 0;
	unsigned int md_dlambda_cfg1 =  0;
	//- color shadow sse add by lqzhang
	unsigned int md_css_cfg0 =  0;
	unsigned int md_css_cfg1 = 0;
	unsigned int md_css_cfg2 = 0;
	unsigned int md_css_cfg3 = 0;
	unsigned int md_cse_cfg0 = 0;
	unsigned int md_cse_cfg1 = 0;
	unsigned int md_cse_cfg2 = 0;
	// - FPGA md_pmd csd dbg
	unsigned int md_pmd_csd_lcu_x = 0;                 //dbg lcu x
	unsigned int md_pmd_csd_lcu_y = 0;                 //dbg lcu y
	unsigned int md_pmd_csd_lcu_cusize = 0;            //dbg cu size
	unsigned int md_pmd_csd_lcu_cuidx  = 0;

	#ifdef CONFIG_SOC_PRJ007
	unsigned int dt_cfg0 = 0;
	unsigned int dt_cfg1 = 0;
	#elif defined(CONFIG_SOC_PRJ008)
	unsigned int dt_cfg = 0;
	#endif

	unsigned int dblk_trig = 0;
	unsigned int dblk_ctrl = 0;

	int align_height = 0;
	int fm_h_is_align8 = 0;
	int nv12_flag = 0;
	//when buf share is new addr(dynamic), else old addr(static)
	unsigned int odma_y_addr = 0;
	unsigned int odma_c_addr = 0;
	// jrfc
	unsigned int odma_jrfc_head_y_addr  = 0;
	unsigned int odma_jrfc_head_c_addr  = 0;
	// bufshare
	unsigned int odma_bfsh_base_y_addr  = 0;
	unsigned int odma_bfsh_base_c_addr;
	unsigned int odma_bfsh_bynd_y_addr  = 0;
	unsigned int odma_bfsh_bynd_c_addr;
	unsigned int odma_jrfc_bodyy_bfsh_space = 0;

	unsigned int odma_jrfc_bodyc_bfsh_space = 0;
	unsigned int bfsh_base_yaddr = 0;
	unsigned int bfsh_beyond_yaddr = 0;
	unsigned int jrfd_y_addr  = 0;
	unsigned int odma_jrfc_ref_head_en    = 0;
	unsigned int odma_jrfc_max_body_en    = 0;
	unsigned int odma_jrfc_ref_head_cfg   = 0;
	unsigned int odma_jrfc_ref_head_base  = 0;
	unsigned int odma_jrfc_ref_body_base  = 0;
	unsigned int odma_jrfc_ref_body_last  = 0;
	unsigned int odma_jrfc_ref_body_start = 0;
	unsigned int odma_jrfc_max_body_size  = 0;
	unsigned int odma_jrfc_rec_ref_space;
	unsigned int odma_ctrl = 0;
	unsigned int odma_trig = 0;
	unsigned int odma_jrfc_head_sp_y_addr = 0;
	unsigned int odma_jrfc_head_sp_c_addr = 0;

	unsigned int jrfd_c_addr;
	unsigned int jrfd_my_addr = 0;
	unsigned int jrfd_mc_addr;

	unsigned int bfsh_base_caddr   = 0;
	unsigned int bfsh_beyond_caddr = 0;
	unsigned int jrfd_ctrl = 0;
	unsigned int jrfd_trig = 0;
	unsigned int jrfd_space = 0;
	int jfrd_max_ofst_x = 0;
	int jfrd_max_ofst_y = 0;
	unsigned int jrfd_jrfc_exceed  = 0;

	#ifdef CONFIG_SOC_PRJ007
	unsigned int buf_beyond_yaddr = 0;
	unsigned int buf_beyond_caddr = 0;
	#endif

	int mce_comp_ctap = 0;
	unsigned int merg_info = 0;
	unsigned int esti_ctrl =0;

	unsigned int mce_frm_size = 0;
	unsigned int mce_frm_strd = 0;
	unsigned int mce_comp_ctrl = 0;
	unsigned int mce_mvr       = 0;

	#ifdef CONFIG_SOC_PRJ007
	unsigned int mce_pref_expd = 0;
	unsigned int mce_glb_ctrl = 0;
	unsigned int mce_resi_tst = 0;
	unsigned int mce_resi_tds = 0;
	unsigned int mce_resi_tmax = 0;
	#endif

	#ifdef CONFIG_SOC_PRJ008
	unsigned int mce_pref_expd = 0;
	unsigned int mce_glb_ctrl = 0;
	unsigned int mce_bus_ctrl = 0;
	unsigned int mce_rcc_cfg0 = 0;
	unsigned int mce_rcc_cfg1 = 0;
	unsigned int mce_rcc_cfg2 = 0;
	unsigned int mce_rcc_cfg3 = 0;
	int mce_sech_grp0 = 0;
	int mce_sech_grp1 = 0;
	int mce_sech_cfg0_l = 0;
	int mce_sech_cfg0_h = 0;
	int mce_sech_cfg1_l = 0;
	int mce_sech_cfg1_h = 0;
	int mce_sech_cfg2_l = 0;
	int mce_sech_cfg2_h = 0;
	int mce_sech_cfg3_l = 0;
	int mce_sech_cfg3_h   = 0;
	unsigned int mce_comp_gap = 0;
	#endif

	unsigned int common_slv_cfg3 = 0;
	unsigned int spe_slv_cfg3    = 0;
	unsigned int stc_slv_cfg3    = 0;
	unsigned int stc_cfg0        = 0;
	unsigned int stc_cfg1 = 0;

	unsigned int sao_info = 0;
	unsigned int sao_size = 0;
	unsigned int sao_slice= 0;

	int T33_emc_cfg_en = 0;
	int T32_emc_cfg_en = 0;

	int dbg_emc_flag_ram_addr = 0;  //ram addr
	int dbg_emc_flag_data_high = 0;    //ram data , 1 is [127:64], 0 is [63:0];
	int dbg_cfg_info = 0;

	unsigned char intra_en  = 0;
	unsigned char pp_en     = 0;
	unsigned char inter_en  = 0;
	int max_outstanding_num = 0;
	unsigned int T33_efe_extra_cfg0 = 0;
	unsigned int T32_efe_extra_cfg0 = 0;
	unsigned int efe_extra_cfg1     = 0;
	unsigned int efe_ctrl           = 0;
	unsigned int  rrs_cfg           = 0;
	unsigned int  rfh_cfg0          = 0;
	unsigned int  rfh_cfg1          = 0;
	unsigned int  T33_qpg_cfg       = 0;
	unsigned int  T32_qpg_cfg       = 0;
	unsigned int  qpg_cfg1          = 0;
	unsigned int  cplx_qp0          = 0;
	unsigned int  cplx_qp1          = 0;
	unsigned int  pet_cplx_thrd00   = 0;
	unsigned int  pet_cplx_thrd01   = 0;
	unsigned int  pet_cplx_thrd02   = 0;
	unsigned int  pet_cplx_thrd10   = 0;
	unsigned int  pet_cplx_thrd11   = 0;
	unsigned int  pet_cplx_thrd12   = 0;
	unsigned int  pet_cplx_thrd20   = 0;
	unsigned int  pet_cplx_thrd21   = 0;
	unsigned int pet_cplx_thrd22    = 0;
	unsigned int pet_qp_ofst0       = 0;
	unsigned int pet_qp_ofst1       = 0;
	unsigned int pet_qp_ofst2       = 0;
	unsigned int pet_qp_ofst3       = 0;
	unsigned int pet_qp_ofst4       = 0;
	unsigned int pet_qp_ofst_mt0    = 0;
	unsigned int pet_qp_ofst_mt1    = 0;
	unsigned int pet_qp_ofst_mt2    = 0;
	unsigned int pet_qp_ofst_mt3    = 0;
	unsigned int pet_qp_ofst_mt4    = 0;
	unsigned int qpg_dlt_thrd       = 0;
	unsigned int petbc_var_thr      = 0;
	unsigned int petbc_ssm_thr0     = 0;
	unsigned int petbc_ssm_thr1     = 0;
	unsigned int petbc_ssm_thr2     = 0;
	unsigned int petbc2_var_thr     = 0;
	unsigned int petbc2_ssm_thr0    = 0;
	unsigned int petbc2_ssm_thr1    = 0;
	unsigned int petbc2_ssm_thr2    = 0;
	unsigned int petbc_filter_valid = 0;
	unsigned int ai_motion_smd_ofst0= 0;
	unsigned int ai_motion_smd_ofst1= 0;
	unsigned int ai_motion_smd_ofst2 = 0;
	unsigned int ai_motion_smd_ofst3   = 0;
	unsigned int ai_motion_smd_ofst4   = 0;
	unsigned int ai_motion_smd_ofst5   = 0;
	unsigned int ai_motion_smd_ofst6   = 0;
	unsigned int ai_motion_smd_ofst7   = 0;
	unsigned int ai_motion_smd_ofst8   = 0;
	unsigned int ai_motion_smd_ofst9   = 0;
	unsigned int smd_cplx_thrd0       = 0;
	unsigned int smd_cplx_thrd1       = 0;
	unsigned int smd_cplx_thrd2       = 0;
	unsigned int smd_cplx_thrd3       = 0;
	unsigned int sobel_cplx_thrd0      = 0;
	unsigned int sobel_cplx_thrd1      = 0;
	unsigned int sobel_cplx_thrd2      = 0;
	unsigned int sobel_cplx_thrd3      = 0;
	unsigned int T33_ifa_cfg          = 0;
	unsigned int T32_ifa_cfg           = 0;
	unsigned int T33_ifa_cfg1         = 0;
	unsigned int T32_ifa_cfg1         = 0;
	unsigned int ifac_all_en          = 0;
	unsigned int ifac_all1_en         = 0;
	unsigned int skin_thrd0       = 0;
	unsigned int skin_thrd1       = 0;
	unsigned int skin_thrd2       = 0;
	unsigned int skin_lvl0        = 0;
	unsigned int skin_lvl1        = 0;
	unsigned int skin_factor          = 0;
	unsigned int roi_ctrl_cfg[4];
	unsigned int roi_pos_cfg[16];
	unsigned int qpg_roi_area_en;

	unsigned int frm_cplx_thrd         = 0;
	unsigned int rrc_motion_thr0       = 0;
	unsigned int rrc_motion_thr1       = 0;
	unsigned int rrc_motion_thr2       = 0;
	unsigned int rrc_motion_thr3       = 0;
	unsigned int var_flat_thr          = 0;
	unsigned int frm_flat_unifor_thrd  = 0;
	unsigned int pet_qp_idx            = 0;
	unsigned int frmb_ip_param0        = 0;
	unsigned int frmb_ip_param1        = 0;
	unsigned int frmb_ip_param2        = 0;
	unsigned int frmb_ip_param3        = 0;
	unsigned int efe_frm_qp            = 0;
	unsigned int frm_motion_thrd0      = 0;
	unsigned int frm_motion_thrd1      = 0;
	unsigned int efe_frm_size          = 0;

	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SPLIT_FLAG_CTX],                sliceType, qp, (unsigned char*)X_INIT_SPLIT_FLAG, RADIX_X_NUM_SPLIT_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SKIP_FLAG_CTX],                 sliceType, qp, (unsigned char*)X_INIT_SKIP_FLAG, RADIX_X_NUM_SKIP_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MERGE_FLAG_EXT_CTX],            sliceType, qp, (unsigned char*)X_INIT_MERGE_FLAG_EXT, RADIX_X_NUM_MERGE_FLAG_EXT_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MERGE_IDX_EXT_CTX],             sliceType, qp, (unsigned char*)X_INIT_MERGE_IDX_EXT, RADIX_X_NUM_MERGE_IDX_EXT_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_PART_SIZE_CTX],                 sliceType, qp, (unsigned char*)X_INIT_PART_SIZE, RADIX_X_NUM_PART_SIZE_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_PRED_MODE_CTX],                 sliceType, qp, (unsigned char*)X_INIT_PRED_MODE, RADIX_X_NUM_PRED_MODE_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_ADI_CTX],                       sliceType, qp, (unsigned char*)X_INIT_INTRA_PRED_MODE, RADIX_X_NUM_ADI_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CHROMA_PRED_CTX],               sliceType, qp, (unsigned char*)X_INIT_CHROMA_PRED_MODE, RADIX_X_NUM_CHROMA_PRED_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_DELTA_QP_CTX],                  sliceType, qp, (unsigned char*)X_INIT_DQP, RADIX_X_NUM_DELTA_QP_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_INTER_DIR_CTX],                 sliceType, qp, (unsigned char*)X_INIT_INTER_DIR, RADIX_X_NUM_INTER_DIR_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_REF_NO_CTX],                    sliceType, qp, (unsigned char*)X_INIT_REF_PIC, RADIX_X_NUM_REF_NO_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MV_RES_CTX],                    sliceType, qp, (unsigned char*)X_INIT_MVD, RADIX_X_NUM_MV_RES_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_QT_CBF_CTX],                    sliceType, qp, (unsigned char*)X_INIT_QT_CBF, RADIX_X_NUM_QT_CBF_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_TRANS_SUBDIV_FLAG_CTX],         sliceType, qp, (unsigned char*)X_INIT_TRANS_SUBDIV_FLAG, RADIX_X_NUM_TRANS_SUBDIV_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_QT_ROOT_CBF_CTX],               sliceType, qp, (unsigned char*)X_INIT_QT_ROOT_CBF, RADIX_X_NUM_QT_ROOT_CBF_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SIG_CG_FLAG_CTX],               sliceType, qp, (unsigned char*)X_INIT_SIG_CG_FLAG, 2 * RADIX_X_NUM_SIG_CG_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SIG_FLAG_CTX],                  sliceType, qp, (unsigned char*)X_INIT_SIG_FLAG, RADIX_X_NUM_SIG_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CTX_LAST_FLAG_X],               sliceType, qp, (unsigned char*)X_INIT_LAST, RADIX_X_NUM_CTX_LAST_FLAG_XY);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CTX_LAST_FLAG_Y],               sliceType, qp, (unsigned char*)X_INIT_LAST, RADIX_X_NUM_CTX_LAST_FLAG_XY);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_ONE_FLAG_CTX],                  sliceType, qp, (unsigned char*)X_INIT_ONE_FLAG, RADIX_X_NUM_ONE_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_ABS_FLAG_CTX],                  sliceType, qp, (unsigned char*)X_INIT_ABS_FLAG, RADIX_X_NUM_ABS_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MVP_IDX_CTX],                   sliceType, qp, (unsigned char*)X_INIT_MVP_IDX, RADIX_X_NUM_MVP_IDX_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SAO_MERGE_FLAG_CTX],            sliceType, qp, (unsigned char*)X_INIT_SAO_MERGE_FLAG, RADIX_X_NUM_SAO_MERGE_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SAO_TYPE_IDX_CTX],              sliceType, qp, (unsigned char*)X_INIT_SAO_TYPE_IDX, RADIX_X_NUM_SAO_TYPE_IDX_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_TRANSFORMSKIP_FLAG_CTX],        sliceType, qp, (unsigned char*)X_INIT_TRANSFORMSKIP_FLAG, 2 * RADIX_X_NUM_TRANSFORMSKIP_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_CU_TRANSQUANT_BYPASS_FLAG, RADIX_X_NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX);
	for(i = 0 ; i< RADIX_X_NUM_TOTAL; i++){
		all_init_state[i] = X_contextModels[i].m_state;
	}

	qkcf_vector_last   = 51;
	qkcf_vector_signcg = 48;
	qkcf_vector_sign   = 54;
	qkcf_vector_m1     = 56;
	bc_cfg0     = (0
			+ VALID_VAL3 (0x1,  31, s->tu_inter_depth - 1)              // tu inter depth
			+ VALID_VAL3 (0x1,  30, 0x1)                                // gate
			+ VALID_VAL3 (0x1,  29, s->mref_en)                         // mref_en
			+ VALID_VAL3 (0x1,  21, 0x1)                                // grey
			+ VALID_VAL3 (0x1,  20, s->use_dqp_flag)                    // dqp_flag
			+ VALID_VAL3 (0x3,  18, s->frame_type)                      // frm_tupe
			+ VALID_VAL3 (0x1ff, 9, (((s->frame_height) >> 3) - 1))     // frame_height
			+ VALID_VAL3 (0x1ff, 0, (((s->frame_width ) >> 3) - 1)));   // frame_width
	bc_cfg1     = (0
			+ VALID_VAL3 (0x1,  10, s->sign_hide_flag)                  // hid_flag
			+ VALID_VAL3 (0x7,  7,  s->merge_cand_num)                  // merge_cand_num
			+ VALID_VAL3 (0x3f, 1,  s->frame_qp));                      // frame_qp
	T33_bc_cfg2 = (0
			+ VALID_VAL3 (0x1 , 31, 0x1)                                // slice_init
			+ VALID_VAL3 (0x1 , 30, 0x1)                                // ftbc_en
			+ VALID_VAL3 (0x3f, 24, qkcf_vector_m1)                     // m1
			+ VALID_VAL3 (0x3f, 16, qkcf_vector_sign)                   // sign
			+ VALID_VAL3 (0x3f, 8,  qkcf_vector_signcg)                 // signcg
			+ VALID_VAL3 (0x3f, 0,  qkcf_vector_last));                 // last
	T32_bc_cfg2 = (0
			+ VALID_VAL3 (0x1 , 31, 0x1)                                // slice_init
			+ VALID_VAL3 (0x3f, 24, qkcf_vector_m1)                     // m1
			+ VALID_VAL3 (0x3f, 16, qkcf_vector_sign)                   // sign
			+ VALID_VAL3 (0x3f, 8,  qkcf_vector_signcg)                 // signcg
			+ VALID_VAL3 (0x3f, 0,  qkcf_vector_last));                 // last

#define RADIX_WRITE_CHN_BC(addr_ofst, val, var)   RADIX_GEN_VDMA_ACFG(chn, RADIX_BC_BASE_VDMA + addr_ofst, val, var)
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_CFG0 , 0, bc_cfg0);
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_CFG1 , 0, bc_cfg1);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_CFG2 , 0, T32_bc_cfg2); //last
#elif defined(CONFIG_SOC_PRJ008)
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_CFG2 , 0, T33_bc_cfg2); //last
#endif

	for(ii=0; ii<10; ii++)  RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii]); //0~9
	for(ii=10; ii<13; ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+2]); //10~12
	for(ii=13; ii<18; ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+6]); //13~17
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 18*4 , 0, all_init_state[26]); //18
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 19*4 , 0, all_init_state[27]); //19
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 20*4 , 0, all_init_state[150]);//20
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 21*4 , 0, all_init_state[24]); //21
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 22*4 , 0, all_init_state[37]); //22 rtcbf
	for(ii=23; ii<26; ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+11]); //23~25 subdiv

#ifdef CONFIG_SOC_PRJ007
	for(ii=26; ii<32; ii++) RADIX_GEN_VDMA_ACFG(chn,RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+2]);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 32*4 , 0, all_init_state[16]);
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 33*4 , 0, all_init_state[38]); //33 sign cg 2
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 34*4 , 0, all_init_state[39]); //34 sign cg 2
	for(ii=35; ii<50;  ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+49]);  //35~49 last x 15
	for(ii=50; ii<65;  ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+52]);  //idx:50~64, state:102~116 last y 15
	for(ii=65; ii<92;  ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii-23]);  //idx:65~91, state:42~68 sign 27
	for(ii=92; ii<108; ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+28]);  //idx:92~107, state:120~135 m1 16
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 108*4 , 0, all_init_state[144]);  //idx:108, state:144~147, m2 4
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 109*4 , 0, all_init_state[145]);  //idx:109, state:144~147, m2 4
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 110*4 , 0, all_init_state[146]);  //idx:110, state:144~147, m2 4
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 111*4 , 0, all_init_state[147]);  //idx:111, state:144~147, m2 4
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 112*4 , 0, all_init_state[40]);   //uv signcg 2
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 113*4 , 0, all_init_state[41]);   //uv signcg 2
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 114*4 , 0, all_init_state[99]);   //last x 3
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 115*4 , 0, all_init_state[100]);  //last x 3
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 116*4 , 0, all_init_state[101]);  //last x 3
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 117*4 , 0, all_init_state[117]);  //last y 3
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 118*4 , 0, all_init_state[118]);  //last y 3
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 119*4 , 0, all_init_state[119]);  //last y 3
	for(ii=120; ii<135; ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii-51]); //idx:120~134, state:69~83, sign 15
	for(ii=135; ii<143; ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+1]); //idx:135~142, state:136~143, m1 8
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 143*4 , 0, all_init_state[148]);  //idx:143~144, state:148~149, m2 2
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 144*4 , 0, all_init_state[149]);  //idx:143~144, state:148~149, m2 2
#elif defined(CONFIG_SOC_PRJ008)
	RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + 26*4 , 0, all_init_state[16]); //26 dqp : old is 32
	for(ii=27; ii<33; ii++) RADIX_WRITE_CHN_BC (RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+1]); //27~32 cbf : old is 26~31
#endif

	/*************************************************************************************
	  SDE Module
	 *************************************************************************************/
	T32_sde_cfg0 = (0
			+ VALID_VAL3 (0x1, 31, s->tu_inter_depth - 1) //tu inter depth
			+ VALID_VAL3 (0x1, 29, s->mref_en)
			+ VALID_VAL3 (0x7, 22, s->merge_cand_num)
			+ VALID_VAL3 (0x1, 21, s->rm.use_hp)          // T33=0
			+ VALID_VAL3 (0x1, 20, s->rm.interp_filter)   // T33=0
			+ VALID_VAL3 (0x3, 18, s->dqp_max_depth)
			+ VALID_VAL3 (0x1, 17, s->sao_c_flag)
			+ VALID_VAL3 (0x1, 16, s->sao_y_flag)
			+ VALID_VAL3 (0x1, 15, s->sao_en)
			+ VALID_VAL3 (0x1, 14, s->sign_hide_flag)
			+ VALID_VAL3 (0x1, 13, s->rm.roi_flag)        // T33=0
			+ VALID_VAL3 (0xff,5,  s->rm.frame_qp_svac)   // T33=0
			+ VALID_VAL3 (0x1, 4,  s->use_dqp_flag)
			+ VALID_VAL3 (0x3, 2,  s->frame_type)
			+ VALID_VAL3 (0x1, 0,  1));                   // isHEVC
	T33_sde_cfg0 = (0
			+ VALID_VAL3 (0x1, 31, s->tu_inter_depth - 1) //tu inter depth
			+ VALID_VAL3 (0x1, 29, s->mref_en)
			+ VALID_VAL3 (0x1, 28, s->nal_en)
			+ VALID_VAL3 (0x7, 22, s->merge_cand_num)
			+ VALID_VAL3 (0x1, 21, s->rm.use_hp)          // T33=0
			+ VALID_VAL3 (0x1, 20, s->rm.interp_filter)   // T33=0
			+ VALID_VAL3 (0x3, 18, s->dqp_max_depth)
			+ VALID_VAL3 (0x1, 17, s->sao_c_flag)
			+ VALID_VAL3 (0x1, 16, s->sao_y_flag)
			+ VALID_VAL3 (0x1, 15, s->sao_en)
			+ VALID_VAL3 (0x1, 14, s->sign_hide_flag)
			+ VALID_VAL3 (0x1, 13, s->rm.roi_flag)        // T33=0
			+ VALID_VAL3 (0xff,5,  s->rm.frame_qp_svac)   // T33=0
			+ VALID_VAL3 (0x1, 4,  s->use_dqp_flag)
			+ VALID_VAL3 (0x3, 2,  s->frame_type)
			+ VALID_VAL3 (0x1, 0,  1));                   // isHEVC
	sde_cfg1 = (0
			+ VALID_VAL3 (0xffff, 16, s->frame_width)
			+ VALID_VAL3 (0xffff, 0,  s->frame_height));
	sde_cfg2 = (0
			+ VALID_VAL3 (0xFFFFFF, 0,  max_bs_size)
			+ VALID_VAL3 (0x1,      31, max_bs_en));

	//硬件配置寄存器有顺序要求，比如cfg0需要在state_base之前配置；如修改本文件，务必知会硬件相关人员！
#ifdef CONFIG_SOC_PRJ007
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_CFG0,                 0, T32_sde_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_CFG1,                 0, sde_cfg1);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_BS_ADDR,              0, s->bitstream_pa);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_BS_LENG,              0, 0);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_MAX_BS_SIZE,          0, sde_cfg2);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_INIT,                 0, 1);
#endif

#ifdef CONFIG_SOC_PRJ008
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_BS_LENG,              0, 0);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_INIT,                 0, 1);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_CFG0,                 0, T33_sde_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_CFG1,                 0, sde_cfg1);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_MAX_BS_SIZE,          0, sde_cfg2);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_BS_ADDR,              0, s->bitstream_pa);
#endif

	for(i=0;i<24;i++){
		RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_STATE_BASE + i*4, 0, all_init_state[i]);
	}
for(i=24;i<151;i++){
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_STATE_BASE + i*4, 0, all_init_state[i+2]);
}
RADIX_GEN_VDMA_ACFG(chn,RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_STATE_BASE + 151*4,   0, all_init_state[24]); //mref

/*************************************************************************************
  IPRED Module
 *************************************************************************************/
s->md_bc_close = 1;  // always close md-rd-ip-bc in T40
	ipmd_mpm_cost_biass32 = (0
			+ VALID_VAL2 (18, s->ipmd_mpm_cost_bias[0][0][3])
			+ VALID_VAL2 (12, s->ipmd_mpm_cost_bias[0][0][2])
			+ VALID_VAL2 (6,  s->ipmd_mpm_cost_bias[0][0][1])
			+ VALID_VAL2 (0,  s->ipmd_mpm_cost_bias[0][0][0]));
	ipmd_mpm_cost_biasw32 = (0
			+ VALID_VAL2 (18, s->ipmd_mpm_cost_bias[0][1][3])
			+ VALID_VAL2 (12, s->ipmd_mpm_cost_bias[0][1][2])
			+ VALID_VAL2 (6,  s->ipmd_mpm_cost_bias[0][1][1])
			+ VALID_VAL2 (0,  s->ipmd_mpm_cost_bias[0][1][0]));
	ipmd_mpm_cost_biass16 = (0
			+ VALID_VAL2 (18, s->ipmd_mpm_cost_bias[1][0][3])
			+ VALID_VAL2 (12, s->ipmd_mpm_cost_bias[1][0][2])
			+ VALID_VAL2 (6,  s->ipmd_mpm_cost_bias[1][0][1])
			+ VALID_VAL2 (0,  s->ipmd_mpm_cost_bias[1][0][0]));
	ipmd_mpm_cost_biasw16 = (0
			+ VALID_VAL2 (18, s->ipmd_mpm_cost_bias[1][1][3])
			+ VALID_VAL2 (12, s->ipmd_mpm_cost_bias[1][1][2])
			+ VALID_VAL2 (6,  s->ipmd_mpm_cost_bias[1][1][1])
			+ VALID_VAL2 (0,  s->ipmd_mpm_cost_bias[1][1][0]));
	ipmd_mpm_cost_biass8  = (0
			+ VALID_VAL2 (18, s->ipmd_mpm_cost_bias[2][0][3])
			+ VALID_VAL2 (12, s->ipmd_mpm_cost_bias[2][0][2])
			+ VALID_VAL2 (6,  s->ipmd_mpm_cost_bias[2][0][1])
			+ VALID_VAL2 (0,  s->ipmd_mpm_cost_bias[2][0][0]));
	ipmd_mpm_cost_biasw8  = (0
			+ VALID_VAL2 (18, s->ipmd_mpm_cost_bias[2][1][3])
			+ VALID_VAL2 (12, s->ipmd_mpm_cost_bias[2][1][2])
			+ VALID_VAL2 (6,  s->ipmd_mpm_cost_bias[2][1][1])
			+ VALID_VAL2 (0,  s->ipmd_mpm_cost_bias[2][1][0]));
	T33_ipmd_angle_bias   = (0
			+ VALID_VAL2 (19, s->ipmd_sobel_edge_thrd)
			+ VALID_VAL2 (18, s->ipmd_ras_en)
			+ VALID_VAL2 (17, s->intra_pu4_en)
			+ VALID_VAL2 (15, s->ipmd_mpm_use_maxnum)
			+ VALID_VAL2 (14, s->i_pmd_en)
			+ VALID_VAL2 (13, s->frm_ipmd_angle_en)
			+ VALID_VAL2 (7,  s->intra_angle_bias[2])
			+ VALID_VAL2 (1,  s->intra_angle_bias[1])
			+ VALID_VAL2 (0,  s->intra_angle_bias[0]));
	T32_ipmd_angle_bias   = (0
			+ VALID_VAL2 (15, s->ipmd_mpm_use_maxnum)
			+ VALID_VAL2 (14, s->i_pmd_en)
			+ VALID_VAL2 (13, s->frm_ipmd_angle_en)
			+ VALID_VAL2 (7,  s->intra_angle_bias[2])
			+ VALID_VAL2 (1,  s->intra_angle_bias[1])
			+ VALID_VAL2 (0,  s->intra_angle_bias[0]));
	ipmd_dm_bias32        = (0
			+ VALID_VAL2 (12, s->ipmd_dm_cost_bias[0][2])
			+ VALID_VAL2 (6,  s->ipmd_dm_cost_bias[0][1])
			+ VALID_VAL2 (0,  s->ipmd_dm_cost_bias[0][0]));
	ipmd_dm_bias16        = (0
			+ VALID_VAL2 (24, s->ipmd_dm_cost_bias[1][4])
			+ VALID_VAL2 (18, s->ipmd_dm_cost_bias[1][3])
			+ VALID_VAL2 (12, s->ipmd_dm_cost_bias[1][2])
			+ VALID_VAL2 (6,  s->ipmd_dm_cost_bias[1][1])
			+ VALID_VAL2 (0,  s->ipmd_dm_cost_bias[1][0]));
	ipmd_dm_bias8         = (0
			+ VALID_VAL2 (24, s->ipmd_dm_cost_bias[2][4])
			+ VALID_VAL2 (18, s->ipmd_dm_cost_bias[2][3])
			+ VALID_VAL2 (12, s->ipmd_dm_cost_bias[2][2])
			+ VALID_VAL2 (6,  s->ipmd_dm_cost_bias[2][1])
			+ VALID_VAL2 (0,  s->ipmd_dm_cost_bias[2][0]));
	intra_bits0           = (0 //8*4bits0
			+ VALID_VAL3 (0xf, 28, s->intra_bits[1][2])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[1][1])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[1][0])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[0][4])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[0][3])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[0][2])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[0][1])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[0][0]));
	intra_bits1           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[3][0])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[2][4])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[2][3])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[2][2])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[2][1])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[2][0])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[1][4])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[1][3]));
	intra_bits2           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[4][3])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[4][2])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[4][1])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[4][0])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[3][4])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[3][3])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[3][2])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[3][1]));
	intra_bits3           = (0//8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[6][1])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[6][0])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[5][4])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[5][3])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[5][2])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[5][1])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[5][0])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[4][4]));
	intra_bits4           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[7][4])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[7][3])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[7][2])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[7][1])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[7][0])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[6][4])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[6][3])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[6][2]));
	intra_bits5           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[9][2])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[9][1])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[9][0])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[8][4])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[8][3])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[8][2])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[8][1])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[8][0]));
	intra_bits6           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[11][0])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[10][4])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[10][3])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[10][2])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[10][1])
			+ VALID_VAL3 (0xf, 8, s->intra_bits[10][0])
			+ VALID_VAL3 (0xf, 4, s->intra_bits[9][4])
			+ VALID_VAL3 (0xf, 0, s->intra_bits[9][3]));
	intra_bits7           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[12][3])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[12][2])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[12][1])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[12][0])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[11][4])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[11][3])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[11][2])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[11][1]));
	intra_bits8           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[14][1])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[14][0])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[13][4])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[13][3])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[13][2])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[13][1])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[13][0])
			+ VALID_VAL3 (0xf, 0,  s->intra_bits[12][4]));
	intra_bits9           = (0 //8*4bits
			+ VALID_VAL3 (0xf, 28, s->intra_bits[15][4])
			+ VALID_VAL3 (0xf, 24, s->intra_bits[15][3])
			+ VALID_VAL3 (0xf, 20, s->intra_bits[15][2])
			+ VALID_VAL3 (0xf, 16, s->intra_bits[15][1])
			+ VALID_VAL3 (0xf, 12, s->intra_bits[15][0])
			+ VALID_VAL3 (0xf, 8,  s->intra_bits[14][4])
			+ VALID_VAL3 (0xf, 4,  s->intra_bits[14][3])
			+ VALID_VAL3 (0xf, 0, s->intra_bits[14][2]));
	ipred_frm_size   = (0
			+ VALID_VAL2 (0,  s->frame_width-1)
			+ VALID_VAL2 (16, s->frame_height-1));
	ipred_slv_mode   = (0
			+ VALID_VAL2 (31, s->ppred_slv_en)
			+ VALID_VAL2 (30, s->ipred_slv_en)
			+ VALID_VAL2 (24, s->intra_mode[4])
			+ VALID_VAL2 (18, s->intra_mode[3])
			+ VALID_VAL2 (12, s->intra_mode[2])
			+ VALID_VAL2 (6,  s->intra_mode[1])
			+ VALID_VAL2 (0,  s->intra_mode[0]));
	ipred_mode_ctrl  = (0
			+ VALID_VAL2 (15, 1)                      //ip32_n
			+ VALID_VAL2 (12, 1)                      //ip16_n
			+ VALID_VAL2 (9,  1)                      //ip8_n
			+ VALID_VAL2 (6,  s->ppred32_mode_num)    //pp32_n
			+ VALID_VAL2 (3,  s->ppred16_mode_num)    //pp16_n
			+ VALID_VAL2 (0,  s->ppred8_mode_num));   //pp8_n
	ipred_mode0      = (0
			+ VALID_VAL2 (31, s->ppred8_slv_en)
			+ VALID_VAL2 (30, 0)                      //ip8_slv_en
			+ VALID_VAL2 (24, s->intra8_mode[4])
			+ VALID_VAL2 (18, s->intra8_mode[3])
			+ VALID_VAL2 (12, s->intra8_mode[2])
			+ VALID_VAL2 (6,  s->intra8_mode[1])
			+ VALID_VAL2 (0,  s->intra8_mode[0]));
	ipred_mode1      = (0
			+ VALID_VAL2 (31, s->ppred16_slv_en)
			+ VALID_VAL2 (30, 0)                      //ip16_slv_en
			+ VALID_VAL2 (24, s->intra16_mode[4])
			+ VALID_VAL2 (18, s->intra16_mode[3])
			+ VALID_VAL2 (12, s->intra16_mode[2])
			+ VALID_VAL2 (6,  s->intra16_mode[1])
			+ VALID_VAL2 (0,  s->intra16_mode[0]));
	ipred_mode2      = (0
			+ VALID_VAL2 (31, s->ppred32_slv_en)
			+ VALID_VAL2 (30, 0)                      //ip32_slv_en
			+ VALID_VAL2 (24, s->intra32_mode[4])
			+ VALID_VAL2 (18, s->intra32_mode[3])
			+ VALID_VAL2 (12, s->intra32_mode[2])
			+ VALID_VAL2 (6,  s->intra32_mode[1])
			+ VALID_VAL2 (0,  s->intra32_mode[0]));
	ipred_ctrl       = (0
			+ VALID_VAL2 (30, 0)                              // 0:hevc, 1:svac2
			+ VALID_VAL2 (29, 0)                              //ip_dif
			+ VALID_VAL2 (28, s->rm.ppred_slv_dif)            //T33=0;pp_dif
			+ VALID_VAL2 (27, (!inter_cu8 | s->md_bc_close))  //md_bc_close
			+ VALID_VAL2 (22, 0x1f)                           //ipred_crc
			+ VALID_VAL2 (21, (s->frame_type==2))             //slice_i
			+ VALID_VAL2 (18, (s->ipred_size & 0x7))
			+ VALID_VAL2 (15, (s->sobel_size & 0x7))
			+ VALID_VAL2 (14, 0)                              //plane_type
			+ VALID_VAL2 (13, s->ipred_usis)                  //usis
			+ VALID_VAL2 (11, 3)                              //lcu_size
			+ VALID_VAL2 (8,  (s->ppred_mode_num & 0x7))
			+ VALID_VAL2 (5,  (1 & 0x7))                      //ipred_n
			+ VALID_VAL2 (4,  s->sobel_en)
			+ VALID_VAL2 (3,  0)                              //decode
			+ VALID_VAL2 (2,  0)                              //ste
			+ VALID_VAL2 (1,  0)                              //cge
			+ VALID_VAL2 (0,  1));                            //slice_init

#define RADIX_WRITE_CHN_IPRED(addr_ofst, val, var)   RADIX_GEN_VDMA_ACFG(chn, RADIX_IPRED_BASE_VDMA + addr_ofst, val, var)
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_IPRED (T33_RADIX_REG_IPRED_RAS_EDGE_CNT_THR, 0, s->ipmd_sobel_thr[2]);
	RADIX_WRITE_CHN_IPRED (T33_RADIX_REG_IPRED_RAS_SAD_DIF_THR,  0, s->ipmd_sobel_thr[1] << 16 | s->ipmd_sobel_thr[0]);
#endif
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_MPM_BIASS32, 0, ipmd_mpm_cost_biass32);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_MPM_BIASW32, 0, ipmd_mpm_cost_biasw32);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_MPM_BIASS16, 0, ipmd_mpm_cost_biass16);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_MPM_BIASW16, 0, ipmd_mpm_cost_biasw16);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_MPM_BIASS8,  0, ipmd_mpm_cost_biass8);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_MPM_BIASW8,  0, ipmd_mpm_cost_biasw8);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_ANGLE_BIAS,  0, T32_ipmd_angle_bias);
#elif defined(CONFIG_SOC_PRJ008)
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_ANGLE_BIAS,  0, T33_ipmd_angle_bias);
#endif
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_DM_BIAS32,   0, ipmd_dm_bias32);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_DM_BIAS16,   0, ipmd_dm_bias16);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_IPMD_DM_BIAS8,    0, ipmd_dm_bias8);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS0,            0, intra_bits0);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS1,            0, intra_bits1);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS2,            0, intra_bits2);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS3,            0, intra_bits3);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS4,            0, intra_bits4);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS5,            0, intra_bits5);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS6,            0, intra_bits6);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS7,            0, intra_bits7);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS8,            0, intra_bits8);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BITS9,            0, intra_bits9);

	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BC_ST,            0, all_init_state[13]);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_BC_INIT,          0, 1);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_FRM_SIZE,         0, ipred_frm_size);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_SLV_MODE,         0, ipred_slv_mode);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_MOD_CTRL,         0, ipred_mode_ctrl);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_MODE0,            0, ipred_mode0);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_MODE1,            0, ipred_mode1);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_MODE2,            0, ipred_mode2);
	RADIX_WRITE_CHN_IPRED (RADIX_REG_IPRED_CTRL,             0, ipred_ctrl);

	/*************************************************************************************
	  TFM Module
	 *************************************************************************************/
	tfm_cfg_grp[0] = (0
			+ VALID_VAL3 (0x1, 7, s->intra_pu4_en) /*pu4_enable */
			+ VALID_VAL3 (0x1, 6, s->acmask_type)
			+ VALID_VAL3 (0x1, 5, s->acmask_en)
			+ VALID_VAL3 (0x1, 4, s->rdoq_en)
			+ VALID_VAL3 (0x1, 3, s->deadzone_en)
			+ VALID_VAL3 (0x1, 2, s->do_not_clear_deadzone)
			+ VALID_VAL3 (0x1, 1, s->scaling_list_en)
			+ VALID_VAL3 (0x1, 0, s->rm.scaling_list_present));// T33=0

	for (i = 0; i < 4; ++i)
	for (j = 0; j < 3; ++j) {
		tfm_cfg_grp[i * 6 + j + 1]     = (0
				+ VALID_VAL3 (0xF,    0,  s->tfm_grp[i].rdoq_level_y[j])
				+ VALID_VAL3 (0x1,    4,  s->tfm_grp[i].deadzone_y_en[j])
				+ VALID_VAL3 (0x1FFF, 5,  s->tfm_grp[i].deadzone_y[j])
				+ VALID_VAL3 (0x3FFF, 18, s->tfm_grp[i].acmask_y[j]));
		tfm_cfg_grp[i * 6 + j + 3 + 1] = (0
				+ VALID_VAL3 (0xF,    0,  s->tfm_grp[i].rdoq_level_c[j])
				+ VALID_VAL3 (0x1,    4,  s->tfm_grp[i].deadzone_c_en[j])
				+ VALID_VAL3 (0x1FFF, 5,  s->tfm_grp[i].deadzone_c[j])
				+ VALID_VAL3 (0x3FFF, 18, s->tfm_grp[i].acmask_c[j]));
	}

for (i = 0; i < 7; ++i) {
	tfm_cfg_grp[25 + i * 2] = (0
			+ VALID_VAL3 (0x1,  0,  s->tfm_rdoq_param[i].clear1)
			+ VALID_VAL3 (0x3,  1,  s->tfm_rdoq_param[i].val0[0])
			+ VALID_VAL3 (0x3,  3,  s->tfm_rdoq_param[i].val1[0])
			+ VALID_VAL3 (0x1F, 5,  s->tfm_rdoq_param[i].score[0])
			+ VALID_VAL3 (0x3,  10, s->tfm_rdoq_param[i].val0[1])
			+ VALID_VAL3 (0x3,  12, s->tfm_rdoq_param[i].val1[1])
			+ VALID_VAL3 (0x1F, 14, s->tfm_rdoq_param[i].score[1])
			+ VALID_VAL3 (0x3,  19, s->tfm_rdoq_param[i].val0[2])
			+ VALID_VAL3 (0x3,  21, s->tfm_rdoq_param[i].val1[2])
			+ VALID_VAL3 (0x1F, 23, s->tfm_rdoq_param[i].score[2])
			+ VALID_VAL3 (0x3,  28, s->tfm_rdoq_param[i].val0[3])
			+ VALID_VAL3 (0x3,  30, s->tfm_rdoq_param[i].val1[3]));
	tfm_cfg_grp[25 + i * 2 + 1] = (0
			+ VALID_VAL3 (0,  0x1F, s->tfm_rdoq_param[i].score[3])
			+ VALID_VAL3 (5,  0x3,  s->tfm_rdoq_param[i].tu8_last[0])
			+ VALID_VAL3 (7,  0x7,  s->tfm_rdoq_param[i].tu16_last[0])
			+ VALID_VAL3 (10, 0xF,  s->tfm_rdoq_param[i].tu32_last[0])
			+ VALID_VAL3 (14, 0x3,  s->tfm_rdoq_param[i].tu8_last[1])
			+ VALID_VAL3 (16, 0x7,  s->tfm_rdoq_param[i].tu16_last[1])
			+ VALID_VAL3 (19, 0xF,  s->tfm_rdoq_param[i].tu32_last[1])
			+ VALID_VAL3 (23, 0x3,  s->tfm_rdoq_param[i].tu8_last[2])
			+ VALID_VAL3 (25, 0x7,  s->tfm_rdoq_param[i].tu16_last[2])
			+ VALID_VAL3 (28, 0xF,  s->tfm_rdoq_param[i].tu32_last[2]));
}
tfm_cfg_grp[39]=0x0;
	tfm_cfg_grp[41] = (0
			+ VALID_VAL3 (0x3, 0,  s->tfm_rdoq_param[1 - 1].delta[0])
			+ VALID_VAL3 (0x3, 2,  s->tfm_rdoq_param[1 - 1].delta[1])
			+ VALID_VAL3 (0x3, 4,  s->tfm_rdoq_param[1 - 1].delta[2])
			+ VALID_VAL3 (0x3, 6,  s->tfm_rdoq_param[1 - 1].delta[3])
			+ VALID_VAL3 (0x3, 8,  s->tfm_rdoq_param[2 - 1].delta[0])
			+ VALID_VAL3 (0x3, 10, s->tfm_rdoq_param[2 - 1].delta[1])
			+ VALID_VAL3 (0x3, 12, s->tfm_rdoq_param[2 - 1].delta[2])
			+ VALID_VAL3 (0x3, 14, s->tfm_rdoq_param[2 - 1].delta[3])
			+ VALID_VAL3 (0x3, 16, s->tfm_rdoq_param[3 - 1].delta[0])
			+ VALID_VAL3 (0x3, 18, s->tfm_rdoq_param[3 - 1].delta[1])
			+ VALID_VAL3 (0x3, 20, s->tfm_rdoq_param[3 - 1].delta[2])
			+ VALID_VAL3 (0x3, 22, s->tfm_rdoq_param[3 - 1].delta[3])
			+ VALID_VAL3 (0x3, 24, s->tfm_rdoq_param[4 - 1].delta[0])
			+ VALID_VAL3 (0x3, 26, s->tfm_rdoq_param[4 - 1].delta[1])
			+ VALID_VAL3 (0x3, 28, s->tfm_rdoq_param[4 - 1].delta[2])
			+ VALID_VAL3 (0x3, 30, s->tfm_rdoq_param[4 - 1].delta[3]));
	tfm_cfg_grp[42] = (0
			+ VALID_VAL3 (0,  0x3, s->tfm_rdoq_param[5 - 1].delta[0])
			+ VALID_VAL3 (2,  0x3, s->tfm_rdoq_param[5 - 1].delta[1])
			+ VALID_VAL3 (4,  0x3, s->tfm_rdoq_param[5 - 1].delta[2])
			+ VALID_VAL3 (6,  0x3, s->tfm_rdoq_param[5 - 1].delta[3])
			+ VALID_VAL3 (8,  0x3, s->tfm_rdoq_param[6 - 1].delta[0])
			+ VALID_VAL3 (10, 0x3, s->tfm_rdoq_param[6 - 1].delta[1])
			+ VALID_VAL3 (12, 0x3, s->tfm_rdoq_param[6 - 1].delta[2])
			+ VALID_VAL3 (14, 0x3, s->tfm_rdoq_param[6 - 1].delta[3])
			+ VALID_VAL3 (16, 0x3, s->tfm_rdoq_param[7 - 1].delta[0])
			+ VALID_VAL3 (18, 0x3, s->tfm_rdoq_param[7 - 1].delta[1])
			+ VALID_VAL3 (20, 0x3, s->tfm_rdoq_param[7 - 1].delta[2])
			+ VALID_VAL3 (22, 0x3, s->tfm_rdoq_param[7 - 1].delta[3]));
	tfm_ctrl = (0
			+ VALID_VAL2 (31, s->rm.sse_mask)            // T33=0
			+ VALID_VAL2 (28, 0x0)                    //opn_msk
			+ VALID_VAL2 (27, 0x1)                    //zeroblk_en
			+ VALID_VAL2 (26, 0x1)                    //tfm_en
			+ VALID_VAL2 (21, (s->rm.tfm_buf_en & 0x1f)) // T33=0
			+ VALID_VAL2 (19, (0 & 0x3))              //pro
			+ VALID_VAL2 (16, (0 & 0x7))              //pri
			+ VALID_VAL2 (15, (s->frame_type == 2))   //i_slice
			+ VALID_VAL2 (12, (0 & 0x7))              //bitd_c8
			+ VALID_VAL2 (9,  (0 & 0x7))              //bitd_y8
			+ VALID_VAL2 (7,  (3 & 0x3))              //lcu_size
			+ VALID_VAL2 (5,  (0 & 0x3))              //plantyp
			+ VALID_VAL2 (4,  (0 & 0x1))              //uselist
			+ VALID_VAL2 (3,  (0 & 0x1))              //ste
			+ VALID_VAL2 (2,  (0 & 0x1))              //cge
			+ VALID_VAL2 (1,  (1 & 0x1)));
	tfm_frm_size = (0
			+ VALID_VAL2 (0,  s->frame_width)
			+ VALID_VAL2 (16, s->frame_height));

#ifdef CONFIG_SOC_PRJ007
	x264_en = 1;
#elif defined(CONFIG_SOC_PRJ008)
	x264_en = 0;
#endif
	tfm_init = (0
			+ VALID_VAL2 (24, (s->frame_cqp_offset & 0x7F))
			+ VALID_VAL2 (17, (s->frame_cqp_offset & 0x7F))
			+ VALID_VAL2 (13, (0x0 & 0xF))//s->tfm_path_en[1] in T32
			+ VALID_VAL2 (3,  (s->tfm_path_en & 0x3FF))//s->tfm_path_en[0] in T32
			+ VALID_VAL2 (2,  (0 & 0x1)) //t32:(1 == s->video_type)   t33:0
			+ VALID_VAL2 (1,  (x264_en & 0x1)) //t32:(0 == s->video_type) & 0x1)  t33:0
	+ VALID_VAL2 (0,  (1 & 0x1)));


	for (i = 0; i < 43; ++i)
	RADIX_GEN_VDMA_ACFG(chn,RADIX_TFM_BASE_VDMA + RADIX_TFM_FRM_CTL + i * 4, 0, tfm_cfg_grp[i]);

	RADIX_GEN_VDMA_ACFG(chn,RADIX_TFM_BASE_VDMA + RADIX_TFM_REG_SCTRL, 0, tfm_ctrl);//is_enc
	RADIX_GEN_VDMA_ACFG(chn,RADIX_TFM_BASE_VDMA + RADIX_TFM_FRM_SIZE,  0, tfm_frm_size);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_TFM_BASE_VDMA + RADIX_TFM_REG_SINIT, 0, tfm_init);

	/*************************************************************************************
	  MD Module
	 *************************************************************************************/
	lcu_width  = (s->frame_width + 63) >> 6;
	lcu_height = (s->frame_height + 63) >> 6;
	pre_pmd_en      = inter_cu8 & intra_cu8 & (!(s->frame_type == 2)) & !s->md_bc_close;
	pre_csd_en_cu8  = cu8_enable  & (!(s->frame_type == 2));
	pre_csd_en_cu16 = cu16_enable & (!(s->frame_type == 2));
	csd_bias_en_all = ( ((s->rm.md_csd_bias[3][0] & 0x1) << 3)
			+ ((s->rm.md_csd_bias[2][0] & 0x1) << 2)
			+ ((s->rm.md_csd_bias[1][0] & 0x1) << 1)
			+ ((s->rm.md_csd_bias[0][0] & 0x1) << 0));
	lambda_bias_en_all = ( ((s->rm.md_lambda_bias[2][0] & 0x1) << 2)
			+ ((s->rm.md_lambda_bias[1][0] & 0x1) << 1)
			+ ((s->rm.md_lambda_bias[0][0] & 0x1) << 0));
	slice_qp        = s->frame_qp;
	dqp_enable      = 1;
	dqp_depth_cu16  = s->dqp_max_depth == 2;
	pmd_bias_en_all = (((s->rm.md_pmd_bias[2][0] & 0x1) << 2)
			+ ((s->rm.md_pmd_bias[1][0] & 0x1) << 1)
			+ ((s->rm.md_pmd_bias[0][0] & 0x1) << 0));
	md_cfg0 = (0
			+ VALID_VAL3 (0x3, 0,  s->frame_type)
			+ VALID_VAL3 (0xF, 4,  inter_enable)
			+ VALID_VAL3 (0xF, 8,  intra_enable)
			+ VALID_VAL3 (0x1, 12, 1)                        //uv_enable
			+ VALID_VAL3 (0x1, 13, s->md_cfg_en)             // efe cu cfg enable
			+ VALID_VAL3 (0x1, 14, s->md_cfg_en)             // efe cu cfg bias enable
			+ VALID_VAL3 (0xF, 16, cu_enable)
			+ VALID_VAL3 (0x1, 20, s->rm.tu_split_en)           // T33=0
			+ VALID_VAL3 (0x1, 21, 0)                        //bc_disable
			+ VALID_VAL3 (0x1, 22, (!!(s->frame_width%16)))
			+ VALID_VAL3 (0x1, 23, (!!(s->frame_height%16)))
			+ VALID_VAL3 (0x1, 24, (s->frame_width <= 64*6))
			+ VALID_VAL3 (0x1, 26, 0)                        //crc_sel
			+ VALID_VAL3 (0x1, 27, 1)                        //crc_cken
			+ VALID_VAL3 (0x1, 28, 0)                        //ckg_enable
			+ VALID_VAL3 (0x1, 29, (s->frame_width <= 64*6)) //efe_step
			+ VALID_VAL3 (0x1, 30, (!!s->sao_en))
			+ VALID_VAL2 (     31, 0));                      // 0:hevc, 1:svac2
	md_cfg1 = (0
			+ VALID_VAL3 (0xFF, 0,  0)                       // frm left
			+ VALID_VAL3 (0xFF, 8,  (lcu_width - 1))         // frm right
			+ VALID_VAL3 (0xFF, 16, 0)                       // frm top
			+ VALID_VAL3 (0xFF, 24, (lcu_height - 1)));      // frm bottom
	md_cfg2 = (0
			+ VALID_VAL3 (0x1, 0,  pre_pmd_en)
			+ VALID_VAL3 (0x3, 1,  s->rm.md_pre_pmd[0])         // T33=0
			+ VALID_VAL3 (0x3, 3,  s->rm.md_pre_pmd[1])         // T33=0
			+ VALID_VAL3 (0x1, 8,  pre_csd_en_cu8)
			+ VALID_VAL3 (0x3, 9,  s->rm.md_pre_csd[0])         // T33=0
			+ VALID_VAL3 (0x7, 12, pmd_bias_en_all)          // T33=0
			+ VALID_VAL3 (0xF, 16, s->rm.md_pmd_bias[0][1])     // T33=0
			+ VALID_VAL3 (0xF, 20, s->rm.md_pmd_bias[1][1])     // T33=0
			+ VALID_VAL3 (0xF, 24, s->rm.md_pmd_bias[2][1])     // T33=0
			+ VALID_VAL3 (0x1, 28, pre_csd_en_cu16)
			+ VALID_VAL3 (0x3, 29, s->rm.md_pre_csd[1]));       // T33=0
	md_cfg3 = (0
			+ VALID_VAL3 (0x3F, 0,  slice_qp)                  // slice qp
			+ VALID_VAL3 (0x1,  6,  dqp_enable)                // qp diff enable
			+ VALID_VAL3 (0x1,  7,  s->rm.md_force_split)      // T33=0; value 1: force split if qp not the same
			+ VALID_VAL3 (0x1,  8,  dqp_depth_cu16)            // 0:dqp cu32,  1:dqp cu16
			+ VALID_VAL3 (0xF,  12, csd_bias_en_all)           // T33=0
			+ VALID_VAL3 (0xF,  16, s->rm.md_csd_bias[0][1])   // T33=0
			+ VALID_VAL3 (0xF,  20, s->rm.md_csd_bias[1][1])   // T33=0
			+ VALID_VAL3 (0xF,  24, s->rm.md_csd_bias[2][1])   // T33=0
			+ VALID_VAL3 (0xF,  28, s->rm.md_csd_bias[3][1])); // T33=0
	md_cfg4 = (0
			+ VALID_VAL3 (0x7, 0,  lambda_bias_en_all)
			+ VALID_VAL3 (0xF, 4,  s->rm.md_lambda_bias[0][1]) // T33=0
			+ VALID_VAL3 (0xF, 8,  s->rm.md_lambda_bias[1][1]) // T33=0
			+ VALID_VAL3 (0xF, 12, s->rm.md_lambda_bias[2][1]) // T33=0
			+ VALID_VAL3 (0xF, 16, 0xF)                        // [0]:cu8, [1]:cu16, [2]:cu32, [3]:cu64
			+ VALID_VAL3 (0xF, 20, s->rm.md_mvs_all)           // T33=0
			+ VALID_VAL3 (0xF, 24, s->rm.md_mvs_abs)           // T33=0
			+ VALID_VAL3 (0x1, 28, s->emc_mvo_en));            // write out curr-mv
	/* in T40, force skip is used to limite P frame max size
	 * if sde_bslen exceed max_bs_size in Bytes, only select cu64 skip
	 */
	md_fskip_max_size  = (1 << 20); // 1M Byte
	md_fskip_en  = s->rm.md_fskip_en & (!(s->frame_type == 2));
	md_cfg5 = (0
			+ VALID_VAL3 (0x1,      0, 0)                        // use_sse_mode
			+ VALID_VAL3 (0x1,      4, md_fskip_en)              // T33=0
			+ VALID_VAL3 (0xffffff, 8, md_fskip_max_size));
	md_cfg6 = (0
			+ VALID_VAL3 (0x1 , 27, s->inter_sa8d_en)            // 27: inter_sa8d_en
			+ VALID_VAL3 (0x1F, 22, 0x10)                        // 26:22  mode selable when no-mode-selable
			+ VALID_VAL3 (0x1,  21, s->rm.tus_ifa_intra32_en)    // T33=0; 21  tu_split, intra cu32 ifa en
			+ VALID_VAL3 (0x1,  20, s->rm.tus_ifa_intra16_en)    // T33=0; 20  tu_split, intra cu16 ifa en
			+ VALID_VAL3 (0x1,  19, s->rm.tus_mce_inter32_en)    // T33=0; 19  tu_split, inter cu32 mce en
			+ VALID_VAL3 (0x1,  18, s->rm.tus_mce_inter16_en)    // T33=0; 18  tu_split, inter cu16 mce en
			+ VALID_VAL3 (0x1,  17, s->rm.tus_ifa_inter32_en)    // T33=0; 17  tu_split, inter cu32 ifa en
			+ VALID_VAL3 (0x1,  16, s->rm.tus_ifa_inter16_en)    // T33=0; 16  tu_split, inter cu16 ifa en
			+ VALID_VAL3 (0x7,  12, s->uv_dist_scale[1])         // 14:12  shift
			+ VALID_VAL3 (0x3,  10, s->uv_dist_scale[0])         // 11:10  bias
			+ VALID_VAL3 (0x1,  9,  s->uv_dist_scale[2])         // 9      direction
			+ VALID_VAL3 (0x1,  8,  s->uv_dist_scale_en)         // 8      enable
			+ VALID_VAL3 (0x1F, 0,  s->frame_cqp_offset));       // chroma qp offset, -16 ~ 15
	md_cfg7 = (0
			+ VALID_VAL3 (0xF, 28, s->pmdsu_thrd[2])             // 31:28 pmd speed up threshold cu32
			+ VALID_VAL3 (0xF, 24, s->pmdsu_thrd[1])             // 27:24 pmd speed up threshold cu16
			+ VALID_VAL3 (0xF, 20, s->pmdsu_thrd[0])             // 23:20 pmd speed up threshold cu8.  (sa8d < 2^N)
			+ VALID_VAL3 (0x7, 16, s->pmdsu_en)                  // 18:16 pmd speed up enable for cu32/cu16/cu8
			+ VALID_VAL3 (0x7, 12, s->pmdctrl_en >> 1)           // 14:12 pmd ctrl enable for cu32/cu16/cu8
			+ VALID_VAL3 (0x1, 10, s->rm.blk_pmdctrl_en)         // T33=0; 10 block level pred mode decision control enable. value 0: use default sse-cost. value 1:use sa8d/sad at some mode
			+ VALID_VAL3 (0x1 , 9, s->pmdctrl_mode)              // 9   pred mode decision control mode. 0: merge/search sa8d/sad, inter/intra sse. vaule 1: merge/search/intra sa8d/sad.
			+ VALID_VAL3 (0x1 , 8, s->pmdctrl_en)                // 8  frame level pred mode decision control enable. value 0: use default sse-cost. value 1:use sa8d/sad at some mode
			+ VALID_VAL3 (0x1 , 7, s->rfh_sel_intra32_en)        // 7  refresh flag select intra, cu32
			+ VALID_VAL3 (0x1 , 6, s->rfh_sel_intra16_en)        // 6  refresh flag select intra, cu16
			+ VALID_VAL3 (0x1 , 5, s->rfh_msk_skip32_en)         // 5  refresh flag mask skip, cu32
			+ VALID_VAL3 (0x1 , 4, s->rfh_msk_skip16_en)         // 4  refresh flag mask skip, cu16
			+ VALID_VAL3 (0x1 , 1, s->mf_sel_sech_skip32_en)     // 1  move flag select search skip, cu32
			+ VALID_VAL3 (0x1 , 0, s->mf_sel_sech_skip16_en));   // 0  move flag select search skip, cu16
	md_jrfc_exceed_en = s->md_jrfc_exceed_en;
	md_cfg8 = (0
			+ VALID_VAL3 (0x3ff, 22, s->md_max_ofst_y1 )    // y1, uint 4 pixel
			+ VALID_VAL3 (0x3ff, 12, s->md_max_ofst_y0 )    // y0, uint 4 pixel
			+ VALID_VAL3 (0xff,   4, s->md_max_ofst_x  )    // x, uint 64 pixel
			+ VALID_VAL3 (0x1,    2, md_jrfc_exceed_en )    // y1 enable
			+ VALID_VAL3 (0x1,    1, md_jrfc_exceed_en )    // y0 enable
			+ VALID_VAL3 (0x1,    0, md_jrfc_exceed_en ));  // x enable

	for(j=0; j<8; j++) {
		for(i=0; i<16; i++)
			md_scfg[j>>1] += s->rm.md_fskip_val[j][i] << ((j & 1) ? (i + 16) : i);
	}
/*
 * pmd-cfg-grp haddr[10:8] == 'b100
 * lambda-cfg-grp haddr[10:8] == 'b101
 */
md_pmd_cfg_ofst = RADIX_MD_BASE_VDMA + (1<<10);
md_lambda_cfg_ofst = RADIX_MD_BASE_VDMA + (1<<10) + (0x2<<7); // lambda configure

#ifdef CONFIG_SOC_PRJ007
	md_rcfg[0] = (0
			+ VALID_VAL3 (0x1,    0, s->rc_en)
			+ VALID_VAL3 (0xFFFF, 4, s->bu_size - 1)
			+ VALID_VAL3 (0x1,   20, 1)
			+ VALID_VAL3 (0x1,   21, 0)
			+ VALID_VAL3 (0x1,   22, 1)
			+ VALID_VAL3 (0xFF,  23, s->bu_len)
			+ VALID_VAL3 (0x1,   31, s->rc_dly_en));
	md_rcfg[1] = (0
			+ VALID_VAL3 (0x7F, 0,  s->bu_max_dqp)
			+ VALID_VAL3 (0x1F, 5,  s->bu_min_dqp)
			+ VALID_VAL3 (0x1F, 10, s->rc_avg_len)
			+ VALID_VAL3 (0x7,  15, s->rc_max_prop)
			+ VALID_VAL3 (0x7,  18, s->rc_min_prop)
			+ VALID_VAL3 (0x7,  21, s->rc_method));
	md_rcfg[2] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd_t32[0])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[1]));
	md_rcfg[3] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd_t32[2])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[3]));
	md_rcfg[4] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd_t32[4])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[5]));
	md_rcfg[5] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd_t32[6])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[7]));
	md_rcfg[6] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd_t32[8])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[9]));
	md_rcfg[7] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd_t32[10])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd_t32[11]));
#elif defined(CONFIG_SOC_PRJ008)
	md_rcfg[0] = (0
			+ VALID_VAL3 (0x1,    0, s->rc_en)
			+ VALID_VAL3 (0xFFFF, 4, s->bu_size - 1)
			+ VALID_VAL3 (0x1,   20, 0)
			+ VALID_VAL3 (0x1,   21, 0)
			+ VALID_VAL3 (0x1,   22, 1)
			+ VALID_VAL3 (0xFF,  23, s->bu_len)
			+ VALID_VAL3 (0x1,   31, s->rc_dly_en));
	md_rcfg[1] = (0
			+ VALID_VAL3 (0x7F, 0,  s->bu_max_dqp)
			+ VALID_VAL3 (0x7F, 7,  s->bu_min_dqp)
			+ VALID_VAL3 (0x7,  14, s->rc_max_prop)
			+ VALID_VAL3 (0x7,  17, s->rc_min_prop)
			+ VALID_VAL3 (0x7,  20, s->rc_method)
			+ VALID_VAL3 (0x7F, 23, s->rc_dqp_offset));
	md_rcfg[2] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd[0])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[1]));
	md_rcfg[3] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd[2])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[3]));
	md_rcfg[4] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd[4])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[5]));
	md_rcfg[5] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd[6])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[7]));
	md_rcfg[6] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd[8])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[9]));
	md_rcfg[7] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->rc_thd[10])
			+ VALID_VAL3 (0xFFFF, 16, s->rc_thd[11]));
	md_rcfg[8] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->tgtbs_offset_unit)
			+ VALID_VAL3 (0xFFFF, 16, s->force_max_dqp_bits));
	md_rcfg[9] = (0
			+ VALID_VAL3 (0xFFFF, 0,  s->adjust_unit_len-1)
			+ VALID_VAL3 (0xFF, 16,  s->adjust_unit_size-1));
#endif
	md_pmon_cfg_ofst = 0xc0 << 2;
	pmon_mode = 0; // 0:mpt_empty;  1:ipt_rdy&mpt_empty;  2:ipt_empty
	md_pmon_cfg0  = (0
			+ VALID_VAL2 (0, pmon_mode)
			+ VALID_VAL2 (2, pmon_mode)
			+ VALID_VAL2 (4, pmon_mode));// cu8,cu16,cu32
	/* cfg: | 12bit | 10bit | 10bit | */
	T33_md_pmon_cfg1  = (0
			+ VALID_VAL2 (0 , 64)
			+ VALID_VAL2 (10, 74)
			+ VALID_VAL2 (20, 100));// cu8 mce pmon thrd
	T32_md_pmon_cfg1  = (0
			+ VALID_VAL2 (0 , 40)
			+ VALID_VAL2 (10, 100)
			+ VALID_VAL2 (20, 200));// cu8 mce pmon thrd
	T33_md_pmon_cfg2  = (0
			+ VALID_VAL2 (0 , 157)
			+ VALID_VAL2 (10, 259)
			+ VALID_VAL2 (20, 360));// cu16 mce pmon thrd
	T32_md_pmon_cfg2  = (0
			+ VALID_VAL2 (0 , 130)
			+ VALID_VAL2 (10, 300)
			+ VALID_VAL2 (20, 700));// cu16 mce pmon thrd
	T33_md_pmon_cfg3  = (0
			+ VALID_VAL2 ( 0, 900)
			+ VALID_VAL2 (10, 1023)
			+ VALID_VAL2 (20, 1300));// cu32 mce pmon thrd
	T32_md_pmon_cfg3  = (0
			+ VALID_VAL2 ( 0, 400)
			+ VALID_VAL2 (10, 1000)
			+ VALID_VAL2 (20, 2500));// cu32 mce pmon thrd
	T33_md_pmon_cfg4  = (0
			+ VALID_VAL2 (0 , 55)
			+ VALID_VAL2 (10, 60)
			+ VALID_VAL2 (20, 65));// cu8 tfm pmon thrd
	T32_md_pmon_cfg4  = (0
			+ VALID_VAL2 (0 , 50)
			+ VALID_VAL2 (10, 60)
			+ VALID_VAL2 (20, 70));// cu8 tfm pmon thrd
	md_pmon_cfg5  = (0
			+ VALID_VAL2 (0 , 320)
			+ VALID_VAL2 (10, 360)
			+ VALID_VAL2 (20, 400));// cu16 tfm pmon thrd
	T33_md_pmon_cfg6  = (0
			+ VALID_VAL2 (0 , ((1000>>1)&0x3FF))
			+ VALID_VAL2 (10, ((1400>>1)&0x3FF))
			+ VALID_VAL2 (20, 1600));// cu32 tfm pmon thrd
	T32_md_pmon_cfg6  = (0
			+ VALID_VAL2 (0 , ((1300>>1)&0x3FF))
			+ VALID_VAL2 (10, ((1500>>1)&0x3FF))
			+ VALID_VAL2 (20, 1700));// cu32 tfm pmon thrd

	ppmd_cost_thr = (0
			+ VALID_VAL2 (21, s->rm.p_pmd_en)           //T33=0
			+ VALID_VAL2 (20, s->i_csd_en)
			+ VALID_VAL2 (19, s->p_csd_en)
			+ VALID_VAL2 (18, s->rm.csd_p_cu64_en)      //T33=0
			+ VALID_VAL2 (17, s->csd_color)
			+ VALID_VAL2 (16, s->var_flat_sub_size)
			+ VALID_VAL2 (12, s->rm.ppmd_cost_thr[3])   //T33=0
			+ VALID_VAL2 (8,  s->rm.ppmd_cost_thr[2])   //T33=0
			+ VALID_VAL2 (4,  s->rm.ppmd_cost_thr[0])   //T33=0
			+ VALID_VAL2 (0,  s->rm.ppmd_cost_thr[1])); //T33=0

	ppmd_qp_thrd_0 = ((s->rm.pmd_qp_thrd[4] << 24) |
			(s->rm.pmd_qp_thrd[3] << 18) |
			(s->rm.pmd_qp_thrd[2] << 12) |
			(s->rm.pmd_qp_thrd[1] << 6) |
			(s->rm.pmd_qp_thrd[0]));
	ppmd_qp_thrd_1 = (s->rm.pmd_qp_thrd[6] << 6) | (s->rm.pmd_qp_thrd[5]);
	//pmd_thr_array
	pmd_thr0  = (s->rm.ppmd_thr_list[0][0]<<7 | s->rm.ppmd_thr_list[0][1]) & 0x3fff; //{55, 36}
	pmd_thr1  = (s->rm.ppmd_thr_list[1][0]<<7 | s->rm.ppmd_thr_list[1][1]) & 0x3fff; //{52, 30}
	pmd_thr2  = (s->rm.ppmd_thr_list[2][0]<<7 | s->rm.ppmd_thr_list[2][1]) & 0x3fff; //{50, 30}
	pmd_thr3  = (s->rm.ppmd_thr_list[3][0]<<7 | s->rm.ppmd_thr_list[3][1]) & 0x3fff; //{57, 38}
	pmd_thr4  = (s->rm.ppmd_thr_list[4][0]<<7 | s->rm.ppmd_thr_list[4][1]) & 0x3fff; //{54, 32}
	pmd_thr5  = (s->rm.ppmd_thr_list[5][0]<<7 | s->rm.ppmd_thr_list[5][1]) & 0x3fff; //{53, 31}
	pmd_thr6  = (s->rm.ppmd_thr_list[6][0]<<7 | s->rm.ppmd_thr_list[6][1]) & 0x3fff; //{59, 41}
	pmd_thr7  = (s->rm.ppmd_thr_list[7][0]<<7 | s->rm.ppmd_thr_list[7][1]) & 0x3fff; //{57, 33}
	pmd_thr8  = (s->rm.ppmd_thr_list[8][0]<<7 | s->rm.ppmd_thr_list[8][1]) & 0x3fff; //{56, 32}
	pmd_thr9  = (s->rm.ppmd_thr_list[9][0]<<7 | s->rm.ppmd_thr_list[9][1]) & 0x3fff; //{60, 41}
	pmd_thr10 = (s->rm.ppmd_thr_list[10][0]<<7 | s->rm.ppmd_thr_list[10][1]) & 0x3fff; //{62, 41}
	pmd_thr11 = (s->rm.ppmd_thr_list[11][0]<<7 | s->rm.ppmd_thr_list[11][1]) & 0x3fff; //{59, 35}
	pmd_thr12 = (s->rm.ppmd_thr_list[12][0]<<7 | s->rm.ppmd_thr_list[12][1]) & 0x3fff; //{59, 33}
	pmd_thr13 = (s->rm.ppmd_thr_list[13][0]<<7 | s->rm.ppmd_thr_list[13][1]) & 0x3fff; //{57, 45}
	pmd_thr14 = (s->rm.ppmd_thr_list[14][0]<<7 | s->rm.ppmd_thr_list[14][1]) & 0x3fff; //{57, 30}
	pmd_thr15 = (s->rm.ppmd_thr_list[15][0]<<7 | s->rm.ppmd_thr_list[15][1]) & 0x3fff; //{56, 30}
	pmd_thr16 = (s->rm.ppmd_thr_list[16][0]<<7 | s->rm.ppmd_thr_list[16][1]) & 0x3fff; //{59, 45}
	pmd_thr17 = (s->rm.ppmd_thr_list[17][0]<<7 | s->rm.ppmd_thr_list[17][1]) & 0x3fff; //{58, 32}
	pmd_thr18 = (s->rm.ppmd_thr_list[18][0]<<7 | s->rm.ppmd_thr_list[18][1]) & 0x3fff; //{57, 32}
	pmd_thr19 = (s->rm.ppmd_thr_list[19][0]<<7 | s->rm.ppmd_thr_list[19][1]) & 0x3fff; //{60, 45}
	pmd_thr20 = (s->rm.ppmd_thr_list[20][0]<<7 | s->rm.ppmd_thr_list[20][1]) & 0x3fff; //{62, 45}
	pmd_thr21 = (s->rm.ppmd_thr_list[21][0]<<7 | s->rm.ppmd_thr_list[21][1]) & 0x3fff; //{60, 35}
	pmd_thr22 = (s->rm.ppmd_thr_list[22][0]<<7 | s->rm.ppmd_thr_list[22][1]) & 0x3fff; //{76, 51}
	pmd_thr23 = (s->rm.ppmd_thr_list[23][0]<<7 | s->rm.ppmd_thr_list[23][1]) & 0x3fff;
	pmd_thr24 = (s->rm.ppmd_thr_list[24][0]<<7 | s->rm.ppmd_thr_list[24][1]) & 0x3fff;
	pmd_thr25 = (s->rm.ppmd_thr_list[25][0]<<7 | s->rm.ppmd_thr_list[25][1]) & 0x3fff;
	pmd_thr26 = (s->rm.ppmd_thr_list[26][0]<<7 | s->rm.ppmd_thr_list[26][1]) & 0x3fff;
	pmd_thr27 = (s->rm.ppmd_thr_list[27][0]<<7 | s->rm.ppmd_thr_list[27][1]) & 0x3fff;
	pmd_thr28 = (s->rm.ppmd_thr_list[28][0]<<7 | s->rm.ppmd_thr_list[28][1]) & 0x3fff;
	pmd_thr29 = (s->rm.ppmd_thr_list[29][0]<<7 | s->rm.ppmd_thr_list[29][1]) & 0x3fff;
	pmd_thr_idx0 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_search_thr_idx[25])
			+ VALID_VAL2 (20, s->rm.ppmd_search_thr_idx[14])
			+ VALID_VAL2 (15, s->rm.ppmd_search_thr_idx[13])
			+ VALID_VAL2 (10, s->rm.ppmd_search_thr_idx[12])
			+ VALID_VAL2 (5,  s->rm.ppmd_search_thr_idx[11])
			+ VALID_VAL2 (0,  s->rm.ppmd_search_thr_idx[10]));

	pmd_thr_idx1 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_search_thr_idx[6])
			+ VALID_VAL2 (20, s->rm.ppmd_search_thr_idx[5])
			+ VALID_VAL2 (15, s->rm.ppmd_search_thr_idx[29])
			+ VALID_VAL2 (10, s->rm.ppmd_search_thr_idx[28])
			+ VALID_VAL2 (5,  s->rm.ppmd_search_thr_idx[27])
			+ VALID_VAL2 (0,  s->rm.ppmd_search_thr_idx[26]));

	pmd_thr_idx2 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_search_thr_idx[22])
			+ VALID_VAL2 (20, s->rm.ppmd_search_thr_idx[21])
			+ VALID_VAL2 (15, s->rm.ppmd_search_thr_idx[20])
			+ VALID_VAL2 (10, s->rm.ppmd_search_thr_idx[9])
			+ VALID_VAL2 (5,  s->rm.ppmd_search_thr_idx[8])
			+ VALID_VAL2 (0,  s->rm.ppmd_search_thr_idx[7]));

	pmd_thr_idx3 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_search_thr_idx[3])
			+ VALID_VAL2 (20, s->rm.ppmd_search_thr_idx[2])
			+ VALID_VAL2 (15, s->rm.ppmd_search_thr_idx[1])
			+ VALID_VAL2 (10, s->rm.ppmd_search_thr_idx[0])
			+ VALID_VAL2 (5,  s->rm.ppmd_search_thr_idx[24])
			+ VALID_VAL2 (0,  s->rm.ppmd_search_thr_idx[23]));

	pmd_thr_idx4 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_search_thr_idx[19])
			+ VALID_VAL2 (20, s->rm.ppmd_search_thr_idx[18])
			+ VALID_VAL2 (15, s->rm.ppmd_search_thr_idx[17])
			+ VALID_VAL2 (10, s->rm.ppmd_search_thr_idx[16])
			+ VALID_VAL2 (5,  s->rm.ppmd_search_thr_idx[15])
			+ VALID_VAL2 (0,  s->rm.ppmd_search_thr_idx[4]));
	pmd_thr_idx5 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_thr_idx[35])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_thr_idx[19])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_thr_idx[18])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_thr_idx[17])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_thr_idx[16])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_thr_idx[15]));

	pmd_thr_idx6 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_thr_idx[11])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_thr_idx[10])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_thr_idx[39])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_thr_idx[38])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_thr_idx[37])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_thr_idx[36]));

	pmd_thr_idx7 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_thr_idx[32])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_thr_idx[31])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_thr_idx[30])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_thr_idx[14])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_thr_idx[13])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_thr_idx[12]));

	pmd_thr_idx8 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_thr_idx[8])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_thr_idx[7])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_thr_idx[6])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_thr_idx[5])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_thr_idx[34])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_thr_idx[33]));

	pmd_thr_idx9 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_thr_idx[29])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_thr_idx[28])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_thr_idx[27])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_thr_idx[26])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_thr_idx[25])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_thr_idx[9]));

	pmd_thr_idx10 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_thr_idx[20])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_thr_idx[4])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_thr_idx[3])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_thr_idx[2])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_thr_idx[1])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_thr_idx[0]));

	pmd_thr_idx11 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_search_thr_idx[11])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_search_thr_idx[10])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_thr_idx[24])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_thr_idx[23])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_thr_idx[22])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_thr_idx[21]));

	pmd_thr_idx12 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_search_thr_idx[27])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_search_thr_idx[26])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_search_thr_idx[25])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_search_thr_idx[14])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_search_thr_idx[13])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_search_thr_idx[12]));

	pmd_thr_idx13 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_search_thr_idx[8])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_search_thr_idx[7])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_search_thr_idx[6])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_search_thr_idx[5])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_search_thr_idx[29])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_search_thr_idx[28]));

	pmd_thr_idx14 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_search_thr_idx[24])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_search_thr_idx[23])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_search_thr_idx[22])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_search_thr_idx[21])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_search_thr_idx[20])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_search_thr_idx[9]));

	pmd_thr_idx15 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_merge_search_thr_idx[15])
			+ VALID_VAL2 (20, s->rm.ppmd_merge_search_thr_idx[4])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_search_thr_idx[3])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_search_thr_idx[2])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_search_thr_idx[1])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_search_thr_idx[0]));

	pmd_thr_idx16 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_intra_thr_idx[11])
			+ VALID_VAL2 (20, s->rm.ppmd_intra_thr_idx[10])
			+ VALID_VAL2 (15, s->rm.ppmd_merge_search_thr_idx[19])
			+ VALID_VAL2 (10, s->rm.ppmd_merge_search_thr_idx[18])
			+ VALID_VAL2 (5,  s->rm.ppmd_merge_search_thr_idx[17])
			+ VALID_VAL2 (0,  s->rm.ppmd_merge_search_thr_idx[16]));

	pmd_thr_idx17 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_intra_thr_idx[27])
			+ VALID_VAL2 (20, s->rm.ppmd_intra_thr_idx[26])
			+ VALID_VAL2 (15, s->rm.ppmd_intra_thr_idx[25])
			+ VALID_VAL2 (10, s->rm.ppmd_intra_thr_idx[14])
			+ VALID_VAL2 (5,  s->rm.ppmd_intra_thr_idx[13])
			+ VALID_VAL2 (0,  s->rm.ppmd_intra_thr_idx[12]));

	pmd_thr_idx18 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_intra_thr_idx[8])
			+ VALID_VAL2 (20, s->rm.ppmd_intra_thr_idx[7])
			+ VALID_VAL2 (15, s->rm.ppmd_intra_thr_idx[6])
			+ VALID_VAL2 (10, s->rm.ppmd_intra_thr_idx[5])
			+ VALID_VAL2 (5,  s->rm.ppmd_intra_thr_idx[29])
			+ VALID_VAL2 (0,  s->rm.ppmd_intra_thr_idx[28]));

	pmd_thr_idx19 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_intra_thr_idx[24])
			+ VALID_VAL2 (20, s->rm.ppmd_intra_thr_idx[23])
			+ VALID_VAL2 (15, s->rm.ppmd_intra_thr_idx[22])
			+ VALID_VAL2 (10, s->rm.ppmd_intra_thr_idx[21])
			+ VALID_VAL2 (5,  s->rm.ppmd_intra_thr_idx[20])
			+ VALID_VAL2 (0,  s->rm.ppmd_intra_thr_idx[9]));

	pmd_thr_idx20 = (0
			+ VALID_VAL2 (25, s->rm.ppmd_intra_thr_idx[15])
			+ VALID_VAL2 (20, s->rm.ppmd_intra_thr_idx[4])
			+ VALID_VAL2 (15, s->rm.ppmd_intra_thr_idx[3])
			+ VALID_VAL2 (10, s->rm.ppmd_intra_thr_idx[2])
			+ VALID_VAL2 (5,  s->rm.ppmd_intra_thr_idx[1])
			+ VALID_VAL2 (0,  s->rm.ppmd_intra_thr_idx[0]));

	pmd_thr_idx21 = (0
			+ VALID_VAL2 (15, s->rm.ppmd_intra_thr_idx[19])
			+ VALID_VAL2 (10, s->rm.ppmd_intra_thr_idx[18])
			+ VALID_VAL2 (5,  s->rm.ppmd_intra_thr_idx[17])
			+ VALID_VAL2 (0,  s->rm.ppmd_intra_thr_idx[16]));

	md_ned_score_tbl0 = (0
			+ VALID_VAL2 (0,  s->ned_score_table[0][0][0]>>1)
			+ VALID_VAL2 (4,  s->ned_score_table[0][0][1]>>1)
			+ VALID_VAL2 (9,  s->ned_score_table[0][0][2]>>1)
			+ VALID_VAL2 (14, s->ned_score_table[0][0][3]>>1)
			+ VALID_VAL2 (20, s->ned_score_table[0][0][4]>>1)
			+ VALID_VAL2 (26, s->ned_score_table[0][0][5]>>1));
	md_ned_score_tbl1 = (0
			+ VALID_VAL2 (0,  (s->ned_score_table[0][0][5]>>1)>> 6)
			+ VALID_VAL2 (1,  (s->ned_score_table[0][0][6]>>1))
			+ VALID_VAL2 (8,  s->ned_score_table[0][1][0])
			+ VALID_VAL2 (10, s->ned_score_table[0][1][1])
			+ VALID_VAL2 (13, s->ned_score_table[0][1][2])
			+ VALID_VAL2 (16, s->ned_score_table[0][1][3])
			+ VALID_VAL2 (20, s->ned_score_table[0][1][4])
			+ VALID_VAL2 (24, s->ned_score_table[0][1][5])
			+ VALID_VAL2 (28, s->ned_score_table[0][1][6]));
	md_ned_score_tbl2 = (0
			+ VALID_VAL2 (0,  s->ned_score_table[0][1][6]>> 4)
			+ VALID_VAL2 (1,  s->ned_score_table[0][1][7])
			+ VALID_VAL2 (6,  (s->ned_score_table[1][0][0]>>1))
			+ VALID_VAL2 (10, (s->ned_score_table[1][0][1]>>1))
			+ VALID_VAL2 (15, (s->ned_score_table[1][0][2]>>1))
			+ VALID_VAL2 (20, (s->ned_score_table[1][0][3]>>1))
			+ VALID_VAL2 (26, (s->ned_score_table[1][0][4]>>1)));
	md_ned_score_tbl3 = (0
			+ VALID_VAL2 (0,  (s->ned_score_table[1][0][5]>>1))
			+ VALID_VAL2 (7,  (s->ned_score_table[1][0][6]>>1))
			+ VALID_VAL2 (14, s->ned_score_table[1][1][0])
			+ VALID_VAL2 (16, s->ned_score_table[1][1][1])
			+ VALID_VAL2 (19, s->ned_score_table[1][1][2])
			+ VALID_VAL2 (22, s->ned_score_table[1][1][3])
			+ VALID_VAL2 (26, s->ned_score_table[1][1][4])
			+ VALID_VAL2 (30, s->ned_score_table[1][1][5]));
	md_ned_score_tbl4 = (0
			+ VALID_VAL2 (0,  s->ned_score_table[1][1][5]>> 2)
			+ VALID_VAL2 (2,  s->ned_score_table[1][1][6])
			+ VALID_VAL2 (7,  s->ned_score_table[1][1][7])
			+ VALID_VAL2 (12, s->ned_score_bias[0])
			+ VALID_VAL2 (18, s->ned_score_bias[1])
			+ VALID_VAL2 (24, s->ned_en));
	md_ned_disable0   = (0
			+ VALID_VAL2 (0,  s->ned_sad_thr[0][0][0])
			+ VALID_VAL2 (8,  s->ned_sad_thr[0][0][1])
			+ VALID_VAL2 (16, s->ned_sad_thr[0][1][0])
			+ VALID_VAL2 (24, s->ned_sad_thr[0][1][1]));
	md_ned_disable1   = (0
			+ VALID_VAL2 (0,  s->ned_sad_thr[1][0][0])
			+ VALID_VAL2 (5,  s->ned_sad_thr[1][0][1])
			+ VALID_VAL2 (10, s->ned_sad_thr[1][1][0])
			+ VALID_VAL2 (16, s->ned_sad_thr[1][1][1])
			+ VALID_VAL2 (22, s->ned_motion_en)
			+ VALID_VAL2 (23, s->ned_motion_shift)
			+ VALID_VAL2 (26, s->c_sad_min_bias)
			+ VALID_VAL2 (29, s->recon_resi_en[0][0])
			+ VALID_VAL2 (30, s->recon_resi_en[1][0])
			+ VALID_VAL2 (31, s->recon_resi_en[2][0]));
	T33_md_ned_disable2   = (0
			+ VALID_VAL2 (0,  s->ned_sad_thr_cpx_step[0][0])
			+ VALID_VAL2 (2,  s->ned_sad_thr_cpx_step[0][1])
			+ VALID_VAL2 (4,  s->ned_sad_thr_cpx_step[1][0])
			+ VALID_VAL2 (6,  s->ned_sad_thr_cpx_step[1][1])
			+ VALID_VAL2 (8,  s->ccf_rm_en)
			+ VALID_VAL2 (9,  s->mosaic_motion_shift)
			+ VALID_VAL2 (12, (s->intra_pu4_en & 0x1)));
	T32_md_ned_disable2   = (0
			+ VALID_VAL2 (0,  s->ned_sad_thr_cpx_step[0][0])
			+ VALID_VAL2 (2,  s->ned_sad_thr_cpx_step[0][1])
			+ VALID_VAL2 (4,  s->ned_sad_thr_cpx_step[1][0])
			+ VALID_VAL2 (6,  s->ned_sad_thr_cpx_step[1][1])
			+ VALID_VAL2 (8,  s->ccf_rm_en)
			+ VALID_VAL2 (9,  s->mosaic_motion_shift));
	mosaic_cfg_0      = (0
			+ VALID_VAL2 (28, s->mosaic_diff_thr[3])
			+ VALID_VAL2 (24, s->mosaic_diff_thr[2])
			+ VALID_VAL2 (20, s->mosaic_diff_thr[1])
			+ VALID_VAL2 (16, s->mosaic_diff_thr[0])
			+ VALID_VAL2 (12, s->mosaic_recon_flat_thr[3])
			+ VALID_VAL2 (8,  s->mosaic_recon_flat_thr[2])
			+ VALID_VAL2 (4,  s->mosaic_recon_flat_thr[1])
			+ VALID_VAL2 (0,  s->mosaic_recon_flat_thr[0]));
	md_rcn_plain_cfg0 = (0
			+ VALID_VAL2 (0,  s->var_flat_recon_thrd[0][0])
			+ VALID_VAL2 (6,  s->var_flat_recon_thrd[0][1])
			+ VALID_VAL2 (12, s->var_flat_recon_thrd[0][2])
			+ VALID_VAL2 (18, s->var_flat_recon_thrd[0][3])
			+ VALID_VAL2 (24, s->var_flat_recon_thrd[0][4])
			+ VALID_VAL2 (30, s->rcn_plain_en)
			+ VALID_VAL2 (31, s->mosaic_en));
	md_rcn_plain_cfg1 = (0
			+ VALID_VAL2 (0,  s->var_flat_recon_thrd[1][0])
			+ VALID_VAL2 (6,  s->var_flat_recon_thrd[1][1])
			+ VALID_VAL2 (12, s->var_flat_recon_thrd[1][2])
			+ VALID_VAL2 (18, s->var_flat_recon_thrd[1][3])
			+ VALID_VAL2 (24, s->var_flat_recon_thrd[1][4])
			+ VALID_VAL2 (30, s->var_flat_sub_size)
			+ VALID_VAL2 (31, s->mosaic_intra_texture_en));
	csd_qp_thrd_0      = (0
			+ VALID_VAL2 (24, s->csd_qp_thrd[4])
			+ VALID_VAL2 (18, s->csd_qp_thrd[3])
			+ VALID_VAL2 (12, s->csd_qp_thrd[2])
			+ VALID_VAL2 (6,  s->csd_qp_thrd[1])
			+ VALID_VAL2 (0,  s->csd_qp_thrd[0]));
	//- dlambda add by lqzhang
	abs_dlambda_simple_dqp  = (abs(s->dlambda_simple_dqp) == 0) ? 0 : (( abs(s->dlambda_simple_dqp) & 0xF ) | 0x10 );
	abs_dlambda_edge_dqp    = (abs(s->dlambda_edge_dqp) == 0) ? 0 : (( abs(s->dlambda_edge_dqp) & 0xF )  | 0x10 );

	//- dlambda add by hjiang
	abs_dlambda_recovery_dqp  = (s->dlambda_recovery_dqp == 0 ? 0 :
			s->dlambda_recovery_dqp  > 0 ? s->dlambda_recovery_dqp : ((abs(s->dlambda_recovery_dqp) & 0xF) | 0x10));

	T32_md_dlambda_cfg0 = (0
			+ VALID_VAL3 (0x03, 6, s->dlambda_max_depth)
			+ VALID_VAL3 (0x07, 3, s->dlambda_motion_shift)
			+ VALID_VAL3 (0x01, 2, s->dlambda_icsd_en)
			+ VALID_VAL3 (0x01, 1, s->dlambda_pcsd_en)
			+ VALID_VAL3 (0x01, 0, s->dlambda_ppmd_en));
	T33_md_dlambda_cfg0 = (0
			+ VALID_VAL3 (0x1f, 15, abs_dlambda_recovery_dqp)
			+ VALID_VAL3 (0x07, 12, s->dlambda_end_recovery)
			+ VALID_VAL3 (0x07, 9, s->dlambda_start_recovery)
			+ VALID_VAL3 (0x01, 8, s->dlambda_recovery_en)
			+ VALID_VAL3 (0x03, 6, s->dlambda_max_depth)
			+ VALID_VAL3 (0x07, 3, s->dlambda_motion_shift)
			+ VALID_VAL3 (0x01, 2, s->dlambda_icsd_en)
			+ VALID_VAL3 (0x01, 1, s->dlambda_pcsd_en)
			+ VALID_VAL3 (0x01, 0, s->dlambda_ppmd_en));
	md_dlambda_cfg1 =  (0
			+ VALID_VAL3 (0x3F, 26, s->dlambda_frmqp_thrd)
			+ VALID_VAL3 (0x3F, 20, s->frame_qp)
			+ VALID_VAL3 (0x1F, 15, s->dlambda_still_dqp)
			+ VALID_VAL3 (0x1F, 10, abs_dlambda_edge_dqp)
			+ VALID_VAL3 (0x1F, 5,  s->dlambda_nei_cplx_dqp)
			+ VALID_VAL3 (0x1F, 0,  abs_dlambda_simple_dqp));
	//- color shadow sse add by lqzhang
	md_css_cfg0 =  (0
			+ VALID_VAL3 (0x1,  18, s->color_shadow_sse_priority_en)
			+ VALID_VAL3 (0x3F, 12, s->color_shadow_sse_qp)
			+ VALID_VAL3 (0x1,  11, s->color_shadow_sse_value_thrd_en)
			+ VALID_VAL3 (0x7,   8, s->color_shadow_sse_ratio_thrd[1])
			+ VALID_VAL3 (0x7,   5, s->color_shadow_sse_ratio_thrd[0])
			+ VALID_VAL3 (0xF,   1, s->color_shadow_sse_motion_shift)
			+ VALID_VAL3 (0x1,   0, s->color_shadow_sse_en));
	md_css_cfg1 = s->color_shadow_sse_value_thrd[0];
	md_css_cfg2 = s->color_shadow_sse_value_thrd[1];
	md_css_cfg3 = s->color_shadow_sse_value_thrd[2];
	md_cse_cfg0 = (0
			+ VALID_VAL3 (0x1,  0,  0)
			+ VALID_VAL3 (0x7,  1,  0)
			+ VALID_VAL3 (0xf,  4,  0)
			+ VALID_VAL3 (0xff, 8,  0)
			+ VALID_VAL3 (0xff, 16, 0)
			+ VALID_VAL3 (0xff, 24, 0));
	md_cse_cfg1 = (0
			+ VALID_VAL3 (0xff, 0,  0)
			+ VALID_VAL3 (0xff, 8,  0)
			+ VALID_VAL3 (0xff, 16, 0)
			+ VALID_VAL3 (0xff, 24, 0));
	md_cse_cfg2 = (0
			+ VALID_VAL3 (0xff, 0,  0)
			+ VALID_VAL3 (0xff, 8,  0)
			+ VALID_VAL3 (0xff, 16, 0));

	// - FPGA md_pmd csd dbg
	md_pmd_csd_lcu_x = 1;                 //dbg lcu x
	md_pmd_csd_lcu_y = 0;                 //dbg lcu y
	md_pmd_csd_lcu_cusize = 1;            //dbg cu size
	md_pmd_csd_lcu_cuidx  = (( 0 << 4) |  //cu32 cnt in cu64
			( 0 << 2) |  //cu16 cnt in cu32
			( 0 << 0));  //cu8  cnt in cu16



#define RADIX_WRITE_CHN_MD(addr_ofst, val, var)   RADIX_GEN_VDMA_ACFG(chn, RADIX_MD_BASE_VDMA + addr_ofst, val, var)
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG0,          0, md_cfg0);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG1,          0, md_cfg1);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG2,          0, md_cfg2);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG3,          0, md_cfg3);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG4,          0, md_cfg4);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG5,          0, md_cfg5);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG6,          0, md_cfg6);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG7,          0, md_cfg7);
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_CFG8,          0, md_cfg8);
#endif
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_SCFG0,         0, md_scfg[0]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_SCFG1,         0, md_scfg[1]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_SCFG2,         0, md_scfg[2]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_SCFG3,         0, md_scfg[3]);

	for (i=0; i<16; i++) {
		RADIX_GEN_VDMA_ACFG(chn,(md_pmd_cfg_ofst + ((i*2+0)<<2)), 0, s->md_cfg_grp[i][0]);
		RADIX_GEN_VDMA_ACFG(chn,(md_pmd_cfg_ofst + ((i*2+1)<<2)), 0, s->md_cfg_grp[i][1]);
	}
	for (i=0; i<11; i++) {
		RADIX_GEN_VDMA_ACFG(chn,(md_lambda_cfg_ofst + (i<<2)), 0, s->lambda_cfg[i]);
	}

	#ifdef CONFIG_SOC_PRJ008
	for (i=0; i<s->bu_len; i++) {
		RADIX_WRITE_CHN_MD ((((1 << 9) + i) << 2), 0, s->rc_info[i]);
	}// 0x800
	#endif
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG0,         0, md_rcfg[0]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG1,         0, md_rcfg[1]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG2,         0, md_rcfg[2]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG3,         0, md_rcfg[3]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG4,         0, md_rcfg[4]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG5,         0, md_rcfg[5]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG6,         0, md_rcfg[6]);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_RCFG7,         0, md_rcfg[7]);
	#ifdef CONFIG_SOC_PRJ007
	for (i=0; i<s->bu_len; i++) {
		RADIX_GEN_VDMA_ACFG(chn,RADIX_MD_BASE_VDMA + (((1 << 9) + (1 << 6) + i) << 2), 0, s->rc_info[i]);
	}
	#endif
	#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_MD (T33_RADIX_REG_MD_RCFG8,         0, md_rcfg[8]);
	RADIX_WRITE_CHN_MD (T33_RADIX_REG_MD_RCFG9,         0, md_rcfg[9]);
	#endif
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 0*4,     0, md_pmon_cfg0);
	#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 1*4,     0, T32_md_pmon_cfg1);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 2*4,     0, T32_md_pmon_cfg2);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 3*4,     0, T32_md_pmon_cfg3);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 4*4,     0, T32_md_pmon_cfg4);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 5*4,     0, md_pmon_cfg5);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 6*4,     0, T32_md_pmon_cfg6);
	#endif
	#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 1*4,     0, T33_md_pmon_cfg1);//C0<<2=300
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 2*4,     0, T33_md_pmon_cfg2);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 3*4,     0, T33_md_pmon_cfg3);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 4*4,     0, T33_md_pmon_cfg4);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 5*4,     0, md_pmon_cfg5);
	RADIX_WRITE_CHN_MD (md_pmon_cfg_ofst + 6*4,     0, T33_md_pmon_cfg6);
	#endif
	//pmd_cost_thr
	RADIX_WRITE_CHN_MD (MD_SLV_THR_CLR_OFST,        0, ppmd_cost_thr);
	//pmd_qp_thrd
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_QP_THRD_0,       0, ppmd_qp_thrd_0);// T33=0
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_QP_THRD_1,       0, ppmd_qp_thrd_1);// T33=0
																	   //setting pmd_thrd
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_1_0,         0, pmd_thr1<<14 | pmd_thr0);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_3_2,         0, pmd_thr3<<14 | pmd_thr2);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_5_4,         0, pmd_thr5<<14 | pmd_thr4);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_7_6,         0, pmd_thr7<<14 | pmd_thr6);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_9_8,         0, pmd_thr9<<14 | pmd_thr8);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_11_10,       0 ,pmd_thr11<<14 | pmd_thr10);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_13_12,       0, pmd_thr13<<14 | pmd_thr12);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_15_14,       0, pmd_thr15<<14 | pmd_thr14);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_17_16,       0, pmd_thr17<<14 | pmd_thr16);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_19_18,       0, pmd_thr19<<14 | pmd_thr18);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_21_20,       0, pmd_thr21<<14 | pmd_thr20);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_23_22,       0, pmd_thr23<<14 | pmd_thr22);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_25_24,       0, pmd_thr25<<14 | pmd_thr24);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_27_26,       0, pmd_thr27<<14 | pmd_thr26);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_29_28,       0, pmd_thr29<<14 | pmd_thr28);
	//setting pmd_thrd_idx
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX0,        0, pmd_thr_idx0);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX1,        0, pmd_thr_idx1);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX2,        0, pmd_thr_idx2);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX3,        0, pmd_thr_idx3);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX4,        0, pmd_thr_idx4);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX5,        0, pmd_thr_idx5);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX6,        0, pmd_thr_idx6);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX7,        0, pmd_thr_idx7);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX8,        0, pmd_thr_idx8);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX9,        0, pmd_thr_idx9);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX10,       0, pmd_thr_idx10);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX11,       0, pmd_thr_idx11);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX12,       0, pmd_thr_idx12);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX13,       0, pmd_thr_idx13);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX14,       0, pmd_thr_idx14);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX15,       0, pmd_thr_idx15);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX16,       0, pmd_thr_idx16);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX17,       0, pmd_thr_idx17);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX18,       0, pmd_thr_idx18);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX19,       0, pmd_thr_idx19);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX20,       0, pmd_thr_idx20);
	RADIX_WRITE_CHN_MD (MD_SLV_PMD_THR_IDX21,       0, pmd_thr_idx21);
	//csd_qp_thrd
	RADIX_WRITE_CHN_MD (MD_SLV_NED_SC_TBL_0,        0, md_ned_score_tbl0);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_SC_TBL_1,        0, md_ned_score_tbl1);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_SC_TBL_2,        0, md_ned_score_tbl2);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_SC_TBL_3,        0, md_ned_score_tbl3);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_SC_TBL_4,        0, md_ned_score_tbl4);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_DISA_0,          0, md_ned_disable0);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_DISA_1,          0, md_ned_disable1);
	#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_MD (MD_SLV_NED_DISA_2,          0, T32_md_ned_disable2);
	#endif
	#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_MD (MD_SLV_NED_DISA_2,          0, T33_md_ned_disable2);
	#endif
	RADIX_WRITE_CHN_MD (MD_SLV_MOSAIC_CFG_0,        0, mosaic_cfg_0);
	RADIX_WRITE_CHN_MD (MD_SLV_CSD_QP_THRD_0,       0, csd_qp_thrd_0);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_RCN_PLAIN_CFG_0, 0, md_rcn_plain_cfg0);
	RADIX_WRITE_CHN_MD (MD_SLV_NED_RCN_PLAIN_CFG_1, 0, md_rcn_plain_cfg1);
	RADIX_WRITE_CHN_MD (RADIX_REG_MD_INIT,          0, 1);
	#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_MD (MD_SLV_DLAMBDA_CFG0,        0, T32_md_dlambda_cfg0);
	#endif
	#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_MD (MD_SLV_DLAMBDA_CFG0,        0, T33_md_dlambda_cfg0);
	#endif
	RADIX_WRITE_CHN_MD (MD_SLV_DLAMBDA_CFG1,        0, md_dlambda_cfg1);
	RADIX_WRITE_CHN_MD (MD_SLV_CSS_CFG0,            0, md_css_cfg0);
	RADIX_WRITE_CHN_MD (MD_SLV_CSS_CFG1,            0, md_css_cfg1);
	RADIX_WRITE_CHN_MD (MD_SLV_CSS_CFG2,            0, md_css_cfg2);
	RADIX_WRITE_CHN_MD (MD_SLV_CSS_CFG3,            0, md_css_cfg3);

	/*************************************************************************************
	  DT Module
	 *************************************************************************************/
	#ifdef CONFIG_SOC_PRJ007
	dt_cfg0 = ((((1<<6) & 0xFF) << 0) +//codec_id
		((s->frame_type & 0xF) << 8) +//slice_type
		((6 & 0x7) << 12) +//lcu_size
		((0 & 0x1) << 15) +//id_decode
		((1) << 16) +//tu_enable
		((s->frame_cqp_offset & 0xFF) << 20) +
		0);        //codec_id
	dt_cfg1 = (0
			+ VALID_VAL3 (0x1, 8, 0) //is_dec
			+ VALID_VAL3 (0x1, 7, 0) //cross tile
			+ VALID_VAL3 (0x1, 6, 1) //cross slice
			+ VALID_VAL3 (0x7, 3, 6) //lcu_size
			+ VALID_VAL3 (0x1, 0, s->dblk_en));//dblk_chng_en

	RADIX_GEN_VDMA_ACFG(chn,RADIX_DT_BASE_VDMA + T32_RADIX_REG_DT_CFG0, 0, dt_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_DT_BASE_VDMA + T32_RADIX_REG_DT_CFG1, 0, dt_cfg1);
#elif defined(CONFIG_SOC_PRJ008)
	dt_cfg = (0
			+ VALID_VAL2 (      4, 1)                     //tu_enable
			+ VALID_VAL3 (0x7,  1, s->frame_type)         //slice_type
			+ VALID_VAL2 (      0, s->dblk_en));    //dblk_chng_en
	RADIX_GEN_VDMA_ACFG(chn,RADIX_DT_BASE_VDMA + T33_RADIX_REG_DT_CFG,  0, dt_cfg);
#endif
	RADIX_GEN_VDMA_ACFG(chn,RADIX_DT_BASE_VDMA + RADIX_REG_DT_INIT, 0, 1);
	/*************************************************************************************
	  DBLK Module
	 *************************************************************************************/
	dblk_trig = (0
			+ VALID_VAL3 (0xffff, 0,  s->frame_height)
			+ VALID_VAL3 (0xffff, 16, s->frame_width));
	dblk_ctrl = (0
			+ VALID_VAL3 (0x1, 31, 0)                    //0:265, 1:264
			+ VALID_VAL3 (0xf, 24, s->frame_cqp_offset)  //cb,cr qp offset value
			+ VALID_VAL2 (     16, s->dblk_en)           //udblk_en
			+ VALID_VAL3 (0xff, 8, s->beta_offset_div2)  //beta_o:wffset_div
			+ VALID_VAL3 (0xff, 0, s->tc_offset_div2));  //tc_offset_div

	RADIX_GEN_VDMA_ACFG(chn,RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_GLB_TRIG, 0, dblk_trig);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_GLB_CTRL, 0, dblk_ctrl);
	/*************************************************************************************
	  ODMA Module
	 *************************************************************************************/
	align_height = (s->frame_height + 15) & ~15;
	fm_h_is_align8 = (s->frame_height & 0xf) != 0;
	nv12_flag = 0;
	//when buf share is new addr(dynamic), else old addr(static)
	odma_y_addr = s->buf_share_en ? s->buf_start_yaddr : (unsigned int)s->dst_y_pa;
	// jrfc
	odma_jrfc_head_y_addr  = (unsigned int)s->head_dsty_addr;
	odma_jrfc_head_c_addr  = (unsigned int)s->head_dstc_addr;
	// bufshare
	odma_bfsh_base_y_addr  = (unsigned int)s->buf_base_yaddr;
	odma_bfsh_bynd_y_addr  = (unsigned int)s->buf_beyond_yaddr;

	odma_jrfc_bodyy_bfsh_space = odma_bfsh_bynd_y_addr - odma_y_addr;
#ifdef CONFIG_SOC_PRJ007
	odma_c_addr = s->buf_share_en ? s->buf_start_caddr : (unsigned int)s->dst_c_pa;
	odma_bfsh_base_c_addr  = (unsigned int)s->buf_base_caddr;
	odma_bfsh_bynd_c_addr  = (unsigned int)s->buf_beyond_caddr;
#elif defined(CONFIG_SOC_PRJ008)
	odma_c_addr = s->body_combine_en ? odma_y_addr : s->buf_share_en ? s->buf_start_caddr : (unsigned int)s->dst_c_pa;
	odma_bfsh_base_c_addr  = s->body_combine_en ? odma_bfsh_base_y_addr : (unsigned int)s->buf_base_caddr;
	odma_bfsh_bynd_c_addr  = s->body_combine_en ? odma_bfsh_bynd_y_addr : (unsigned int)s->buf_beyond_caddr;
#endif
	odma_jrfc_bodyc_bfsh_space = odma_bfsh_bynd_c_addr - odma_c_addr;
	bfsh_base_yaddr = s->buf_base_yaddr;
	bfsh_beyond_yaddr = (s->frm_num_gop == 1 && s->use_dummy_byd) ? s->dummy_buf_beyond_yaddr : s->buf_beyond_yaddr;
	jrfd_y_addr  = s->buf_share_en ? (unsigned int)s->buf_ref_yaddr : (unsigned int)s->ref_y_pa;
	odma_jrfc_ref_head_en    = s->jrfc_ref_ck_en;
	odma_jrfc_max_body_en    = s->jrfc_max_ck_en;
	odma_jrfc_ref_head_cfg   = (0
			+ VALID_VAL3 (0xff, 16,  s->jrfc_ref_lcuy_range) // lcu y range
			+ VALID_VAL3 (0xff,  8,  s->jrfc_ref_lcux_range) // lcu x range
			+ VALID_VAL3 (0x1,   1,  odma_jrfc_max_body_en)
			+ VALID_VAL3 (0x1,   0,  odma_jrfc_ref_head_en));
	odma_jrfc_ref_head_base  = (unsigned int)s->head_refy_addr;
	odma_jrfc_ref_body_base  = (unsigned int)bfsh_base_yaddr;
	odma_jrfc_ref_body_last  = (unsigned int)bfsh_beyond_yaddr;
	odma_jrfc_ref_body_start = (unsigned int)jrfd_y_addr;
	odma_jrfc_max_body_size  = ((s->frame_width + 63) & ~63) * ((s->frame_height + 15) & ~15)*3 / 2;
	odma_jrfc_max_body_size = odma_jrfc_max_body_size*s->jrfc_max_size_ratio / 256;

if( s->frm_num_gop == 0 || (s->frm_num_gop == 1 && s->mref_en) )
	odma_jrfc_rec_ref_space   = (((s->frame_width + 63) & ~63) * ((s->frame_height + 15) & ~15) * 3) / 2 ;
else if( odma_jrfc_ref_body_base > odma_y_addr )
	odma_jrfc_rec_ref_space   = odma_jrfc_ref_body_base - odma_y_addr  ;
else if( odma_jrfc_ref_body_base < odma_y_addr )
	odma_jrfc_rec_ref_space   = odma_jrfc_bodyy_bfsh_space + ( odma_jrfc_ref_body_base - odma_bfsh_base_y_addr ) ;

	odma_ctrl = (0
			+ VALID_VAL2 (31, s->buf_share_en)
			+ VALID_VAL2 (30, s->jrfcv2_en)
			+ VALID_VAL2 (29, nv12_flag)
			+ VALID_VAL2 (14, s->frame_height)
			+ VALID_VAL2 (0, s->frame_width));

	odma_trig = 0;

#ifdef CONFIG_SOC_PRJ007
	odma_trig = (0
			+ VALID_VAL2 (31, 0)    //0: 265, 1: 264
			+ VALID_VAL2 (5,  1)    //init
			+ VALID_VAL2 (6,  0)    //0 is crc_en
			+ VALID_VAL2 (4,  0)    //ckg low means do clock-gating
			);
#elif defined(CONFIG_SOC_PRJ008)
	odma_trig = (0
			+ VALID_VAL2 (31, 0)    //0: 265, 1: 264
			+ VALID_VAL2 (5,  1)    //init
			+ VALID_VAL2 (6,  0)    //0 is crc_en
			+ VALID_VAL2 (4,  0)    //ckg low means do clock-gating
			+ VALID_VAL2 (1,  s->body_combine_en)); //combine body into one space
#endif
	odma_jrfc_head_sp_y_addr = (unsigned int)s->head_sp_dsty;
	odma_jrfc_head_sp_c_addr = (unsigned int)s->head_sp_dstc;

	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_GLB_CTRL,          0, odma_ctrl);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_TLY_BA,            0, odma_y_addr); //tile Y address / body Y address
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_TLC_BA,            0, odma_c_addr); //tile C address / body C address

	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_JRFC_HEAD_Y_BA,    0, odma_jrfc_head_y_addr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_JRFC_HEAD_C_BA,    0, odma_jrfc_head_c_addr);
#ifdef CONFIG_SOC_PRJ008
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_BUSH_BODY_Y_SPACE, 0, odma_jrfc_bodyy_bfsh_space);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_BUSH_BODY_C_SPACE, 0, odma_jrfc_bodyc_bfsh_space);
#endif

#ifdef CONFIG_SOC_PRJ007
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T32_RADIX_REG_ODMA_JRFC_SP_HEAD_Y_BA, 0, odma_jrfc_head_sp_y_addr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T32_RADIX_REG_ODMA_JRFC_SP_HEAD_C_BA, 0, odma_jrfc_head_sp_c_addr);
#endif

	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BASE_Y_BA,    0, odma_bfsh_base_y_addr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BASE_C_BA,    0, odma_bfsh_base_c_addr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BYND_Y_BA,    0, odma_bfsh_bynd_y_addr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BYND_C_BA,    0, odma_bfsh_bynd_c_addr);

#ifdef CONFIG_SOC_PRJ008
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_REF_HEAD_CFG,      0, odma_jrfc_ref_head_cfg);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_REF_HEAD_BASE,     0, odma_jrfc_ref_head_base);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_REF_BODY_BASE,     0, odma_jrfc_ref_body_base);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_REF_BODY_LAST,     0, odma_jrfc_ref_body_last);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_REF_BODY_START,    0, odma_jrfc_ref_body_start);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_MAX_BODY_SIZE,     0, odma_jrfc_max_body_size);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + T33_RADIX_REG_ODMA_REC_REF_SPACE,     0, odma_jrfc_rec_ref_space);//twx:这个不需要了吗？
#endif
	RADIX_GEN_VDMA_ACFG(chn,RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_GLB_TRIG,          0, odma_trig);

	/*************************************************************************************
	  JRFD Module
	 *************************************************************************************/
	jrfd_my_addr = (unsigned int)s->mref_y_pa;
#ifdef CONFIG_SOC_PRJ007
	jrfd_c_addr = s->buf_share_en ? (unsigned int)s->buf_ref_caddr : (unsigned int)s->ref_c_pa;
	jrfd_mc_addr = (unsigned int)s->mref_c_pa;
#elif defined(CONFIG_SOC_PRJ008)
	jrfd_c_addr = s->body_combine_en ? jrfd_y_addr : s->buf_share_en ? (unsigned int)s->buf_ref_caddr : (unsigned int)s->ref_c_pa;
	jrfd_mc_addr = s->body_combine_en ? jrfd_my_addr : (unsigned int)s->mref_c_pa;
#endif

	bfsh_base_yaddr   = s->buf_base_yaddr;
	bfsh_base_caddr   = s->body_combine_en ? bfsh_base_yaddr : s->buf_base_caddr;
	bfsh_beyond_yaddr = (s->frm_num_gop == 1 && s->use_dummy_byd) ? s->dummy_buf_beyond_yaddr : s->buf_beyond_yaddr;
	bfsh_beyond_caddr = s->body_combine_en ? bfsh_beyond_yaddr : (s->frm_num_gop == 1 && s->use_dummy_byd) ? s->dummy_buf_beyond_caddr : s->buf_beyond_caddr;
	jrfd_ctrl = (0
			+ VALID_VAL3 (0x1,    31, s->buf_share_en)
			+ VALID_VAL2 (        30, 0)                 //is_helix
			+ VALID_VAL2 (        28, 0)                 //clk_gate
			+ VALID_VAL2 (        27, s->jrfcv2_en )
			+ VALID_VAL3 (0x1fff, 14, align_height)      //[13] is idle
			+ VALID_VAL3 (0x1fff, 0,  s->frame_width));
	jrfd_trig = (0
			+ VALID_VAL2 (5,1)   //init
			+ VALID_VAL2 (4,0)); //ckg low means do clock-gating

	jrfd_space = (0
			+ VALID_VAL3 (0x7f,  13, 64 )
			+ VALID_VAL3 (0xff,   5, 128)
			+ VALID_VAL3 (0x1f,   0, 31 ));

	jfrd_max_ofst_x = s->jrfc_exceed_max_plcu_x - 1;
	jfrd_max_ofst_x     = (jfrd_max_ofst_x < 0) ? 0 : jfrd_max_ofst_x;
	jfrd_max_ofst_y = (s->jrfc_exceed_max_plcu_y / 16 - 1) * 16 + 15;
	jfrd_max_ofst_y     = (jfrd_max_ofst_y < 0) ? 0 : jfrd_max_ofst_y;
	jrfd_jrfc_exceed  = (0
			+ VALID_VAL3 (0x3ff, 22, jfrd_max_ofst_y + 16 )    // y1, uint 4 pixel
			+ VALID_VAL3 (0x3ff, 12, jfrd_max_ofst_y      )    // y0, uint 4 pixel
			+ VALID_VAL3 (0xff,   4, jfrd_max_ofst_x      )    // x, uint 64 pixel
			+ VALID_VAL3 (0x1,    0, md_jrfc_exceed_en   ));   // jrfc_exceed_enable


#define RADIX_WRITE_CHN_JRFD(addr_ofst, val, var)   RADIX_GEN_VDMA_ACFG(chn, RADIX_JRFD_BASE_VDMA + addr_ofst, val, var)
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_CTRL,            0, jrfd_ctrl);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_HDYA,            0, (int)s->head_refy_addr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_HDCA,            0, (int)s->head_refc_addr);
#ifdef CONFIG_SOC_PRJ007
	RADIX_GEN_VDMA_ACFG(chn,RADIX_JRFD_BASE_VDMA + T32_RADIX_REG_JRFD_HSTR , 0, (((s->cm_head_total & 0xffff) << 16) | (s->lm_head_total & 0xffff)));
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_JRFD (T33_RADIX_REG_JRFD_BSPACE,          0, jrfd_space );
#endif
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_BDYA,            0, jrfd_y_addr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_BDCA,            0, jrfd_c_addr);
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_BUFS_BASEY_ADDR, 0, (int)bfsh_base_yaddr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_BUFS_BASEC_ADDR, 0, (int)bfsh_base_caddr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_BUFS_BEYDY_ADDR, 0, (int)bfsh_beyond_yaddr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_BUFS_BEYDC_ADDR, 0, (int)bfsh_beyond_caddr);
#endif
#ifdef CONFIG_SOC_PRJ007
	buf_beyond_yaddr = (s->frm_num_gop == 1 && s->use_dummy_byd) ? s->dummy_buf_beyond_yaddr : s->buf_beyond_yaddr;
	buf_beyond_caddr = (s->frm_num_gop == 1 && s->use_dummy_byd) ? s->dummy_buf_beyond_caddr : s->buf_beyond_caddr;
	RADIX_GEN_VDMA_ACFG(chn,RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BASEY_ADDR, 0, (int)s->buf_base_yaddr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BASEC_ADDR, 0, (int)s->buf_base_caddr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDY_ADDR , 0, (int)s->buf_beyond_yaddr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDC_ADDR , 0, (int)s->buf_beyond_caddr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDY_ADDR, 0, (int)buf_beyond_yaddr);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDC_ADDR, 0, (int)buf_beyond_caddr);
#endif
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_MHDY,            0, (int)s->head_mrefy_addr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_MHDC,            0, (int)s->head_mrefc_addr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_MBDY,            0, jrfd_my_addr);
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_MBDC,            0, jrfd_mc_addr);

#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_JRFD (T33_RADIX_REG_JRFD_HEAD_INFO,       0, s->jrfc_nxt_frm_head );
	RADIX_WRITE_CHN_JRFD (T33_RADIX_REG_JRFD_EXCEED,          0, jrfd_jrfc_exceed     );
#endif
	RADIX_WRITE_CHN_JRFD (RADIX_REG_JRFD_TRIG,            0, jrfd_trig);

	/*************************************************************************************
	  MCE Module
	 *************************************************************************************/
	if(s->frame_type != 2){
		mce_comp_ctap = (0) ? RADIX_MCE_TAP_TAP8 : RADIX_MCE_TAP_TAP4;
		merg_info = 0;
		esti_ctrl =0;

		esti_ctrl |=  (0
				+ RADIX_MCE_ESTI_CTRL_SCL(s->mce_scl_mode)     /*trbl mvp*/
				+ RADIX_MCE_ESTI_CTRL_FBG(0)
				+ RADIX_MCE_ESTI_CTRL_CLMV
				+ RADIX_MCE_ESTI_CTRL_MSS(s->max_sech_step)
				+ RADIX_MCE_ESTI_CTRL_QRL(s->quart_pixel_sech_en)
				+ RADIX_MCE_ESTI_CTRL_HRL(s->half_pixel_sech_en)
				+ RADIX_MCE_ESTI_CTRL_RF8(!s->subpixel_sech_cu8_en)
				+ RADIX_MCE_ESTI_CTRL_RF16(!s->subpixel_sech_cu16_en)
				+ RADIX_MCE_ESTI_CTRL_RF32(!s->subpixel_sech_cu32_en));

		mce_frm_size = (0
				+ RADIX_MCE_FRM_SIZE_FH(s->frame_height-1)
				+ RADIX_MCE_FRM_SIZE_FW(s->frame_width-1));
		mce_frm_strd = (0
				+ RADIX_MCE_FRM_STRD_STRDC(s->ref_c_stride)
				+ RADIX_MCE_FRM_STRD_STRDY(s->ref_y_stride));
		mce_comp_ctrl = (0
				+ RADIX_MCE_COMP_CTRL_CCE
				+ RADIX_MCE_COMP_CTRL_CTAP(mce_comp_ctap & 0x3) //x265:tap4, svac:tap8
				+ RADIX_MCE_COMP_CTRL_CSPT(RADIX_MCE_SPT_SYMM)
				+ RADIX_MCE_COMP_CTRL_CSPP(RADIX_MCE_SPP_EPEL)
				+ RADIX_MCE_COMP_CTRL_YCE
				+ RADIX_MCE_COMP_CTRL_YTAP(RADIX_MCE_TAP_TAP8)
				+ RADIX_MCE_COMP_CTRL_YSPT(RADIX_MCE_SPT_AUTO)
				+ RADIX_MCE_COMP_CTRL_YSPP(RADIX_MCE_SPP_QPEL));
		mce_mvr       = (0
				+ RADIX_MCE_MVR_MVRY(s->max_mvry * 4)      //RADIX_MAX_MVRY
				+ RADIX_MCE_MVR_MVRX(s->max_mvrx * 4));    //RADIX_MAX_MVRX
		if(s->inter_mode[3][0])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_8X8;
		if(s->inter_mode[2][1])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_16X8;
		if(s->inter_mode[2][2])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_8X16;
		if(s->inter_mode[2][0])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_16X16;
		if(s->inter_mode[1][1])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_32X16;
		if(s->inter_mode[1][2])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_16X32;
		if(s->inter_mode[1][0])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_32X32;
		if(s->inter_mode[0][3])  esti_ctrl += RADIX_MCE_ESTI_CTRL_PUE_64X64;
		if(s->inter_mode[3][3])  merg_info += RADIX_MCE_MRGI_MRGE_8X8;
		if(s->inter_mode[2][3])  merg_info += RADIX_MCE_MRGI_MRGE_16X16;
		if(s->inter_mode[1][3])  merg_info += RADIX_MCE_MRGI_MRGE_32X32;
		if(s->inter_mode[0][3])  merg_info += RADIX_MCE_MRGI_MRGE_64X64;
		if(s->mrg1_en)           merg_info += RADIX_MCE_MRGI_MRGE1_CU8;
		if(s->mrg_fpel_en & 0x1) merg_info += RADIX_MCE_MRGI_FPEL_CU8;
		if(s->mrg_fpel_en & 0x2) merg_info += RADIX_MCE_MRGI_FPEL_CU16;

#define RADIX_WRITE_CHN_MCE(addr_ofst, val, var)   RADIX_GEN_VDMA_ACFG(chn, RADIX_MCE_BASE_VDMA + addr_ofst, val, var)
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_FRM_SIZE,            0, mce_frm_size );
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_FRM_STRD,            0, mce_frm_strd);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_COMP_CTRL,           0, mce_comp_ctrl);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_ESTI_CTRL,           0, esti_ctrl);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_MRGI,                0, merg_info);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_MVR,                 0, mce_mvr); /*RADIX_MAX_MVRX*/
		if(s->buf_share_en){
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_RLUT(0, 0),     0, (int)s->buf_base_yaddr); //base
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_RLUT(0, 0)+4,   0, (int)s->buf_base_caddr); //base
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_BFSH_ADRY,       0, (int)s->buf_ref_yaddr);
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_BFSH_ADRC,       0, (int)s->buf_ref_caddr);
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_MREF_ADRY,       0, (int)s->mref_y_pa);      //ref 1
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_MREF_ADRC,       0, (int)s->mref_c_pa);      //ref 1
		}
		else{
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_RLUT(0, 0),     0, (int)s->dst_y_pa);
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_RLUT(0, 0)+4,   0, (int)s->dst_c_pa);
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_BFSH_ADRY,       0, (int)s->ref_y_pa);      //base
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_BFSH_ADRC,       0, (int)s->ref_c_pa);      //base
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_MREF_ADRY,       0, (int)s->mref_y_pa);     //ref 1
			RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_MREF_ADRC,       0, (int)s->mref_c_pa);     //ref 1
		}
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_SLC_SPOS,            0, 0);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_SLC_MV,              0, (RADIX_MCE_SLC_MVY(s->mce_slc_mvy) + RADIX_MCE_SLC_MVX(s->mce_slc_mvx)));

#ifdef CONFIG_SOC_PRJ007
		mce_pref_expd = (0
				+ RADIX_MCE_PREF_EXPD_L(4)
				+ RADIX_MCE_PREF_EXPD_R(4)
				+ RADIX_MCE_PREF_EXPD_D(4)
				+ RADIX_MCE_PREF_EXPD_T(4));
		mce_glb_ctrl = (0
				+ RADIX_MCE_GLB_CTRL_INIT
				+ RADIX_MCE_GLB_CTRL_RESI(((s->rm.tus_mce_inter32_en & 0x1) << 2) +
					((s->rm.tus_mce_inter16_en & 0x1) << 1) +
					((s->rm.tus_mce_inter8_en & 0x1) << 0))
				+ RADIX_MCE_GLB_CTRL_FMT(0)                                     //encode formate 0:x265, 1:svac
				+ RADIX_MCE_GLB_CTRL_ED(0)                                      //0:encode, 1:decode
				+ RADIX_MCE_GLB_CTRL_TMVP(s->emc_mvi_en && (s->frm_num_gop > 1)) //tmvp en
				+ RADIX_MCE_GLB_CTRL_RMV(s->mref_en)                            //mref en
				+ RADIX_MCE_GLB_CTRL_MWT(s->mref_wei)                           //mref cost weight
				+ RADIX_MCE_GLB_CTRL_VTYPE(0x0)                                 //video type 0:x265
				+ RADIX_MCE_GLB_CTRL_SRD(s->sech_force_mv0_en)                  //srd en
				+ RADIX_MCE_GLB_CTRL_PREF(0x1)                                  //pref neighbour mv en
				+ RADIX_MCE_GLB_CTRL_FMZ(s->mvp_force_zero_en)                  //force merge zero
				+ RADIX_MCE_GLB_CTRL_MFS(s->mvp_force_zero_motion_shift)        //force merge zero
				+ (0<<4)                                                        //crc_en
				+ RADIX_MCE_GLB_CTRL_FSAD(1));                                  // FSAD_CORE1_EN

		mce_resi_tst = (0 + (RADIX_MCE_RTST7(12) |
					RADIX_MCE_RTST6(10) |
					RADIX_MCE_RTST5(8) |
					RADIX_MCE_RTST4(6) |
					RADIX_MCE_RTST3(4) |
					RADIX_MCE_RTST2(2) |
					RADIX_MCE_RTST1(1) |
					RADIX_MCE_RTST0(0)));

		mce_resi_tds = (0
				+ RADIX_MCE_RTDF1(s->rm.mce_resi_thr[0][0][2]) //diff
				+ RADIX_MCE_RTSC0(s->rm.mce_resi_thr[0][0][0]));
		mce_resi_tmax = (0
				+ RADIX_MCE_RTMAX1(s->rm.mce_resi_thr[0][0][3]) //max1
				+ RADIX_MCE_RTMAX0(s->rm.mce_resi_thr[0][0][1]));

		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_PREF_EXPD, 0, mce_pref_expd);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_GLB_CTRL,  0, mce_glb_ctrl);
		RADIX_WRITE_CHN_MCE (T32_RADIX_REG_MCE_BOTM_ID,   0, s->buf_rem_mby);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_RESI_TST,  0, mce_resi_tst);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_RESI_TDS,  0, mce_resi_tds);//score
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_RESI_TMAX, 0, mce_resi_tmax);//max
		for(i=0; i<16; i++){
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_ILUT_Y+i*8,     0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp[0],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_pkg[0],
						0, 0,
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_dir[0],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_rnd[0],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sft[0],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sintp[0],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_srnd[0],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sbias[0]));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_ILUT_Y+i*8+4,   0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp[1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_pkg[1],
						0, 0,
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_dir[1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_rnd[1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sft[1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sintp[1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_srnd[1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sbias[1]));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_CLUT_Y+i*16,    0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][3],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][2],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][0]));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_CLUT_Y+i*16+4,  0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][7],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][6],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][5],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][4]));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_CLUT_Y+i*16+8,  0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][3],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][2],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][1],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][0]));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_CLUT_Y+i*16+12, 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][7],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][6],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][5],
						radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][4]));
		}
		for(i=0; i<8; i++){
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_ILUT_C+i*8,    0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp[0],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_pkg[0],
						0, 0,
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_dir[0],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_rnd[0],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_sft[0],
						0, 0, 0));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_ILUT_C+i*8+4,  0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp[1],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_pkg[1],
						0, 0,
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_dir[1],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_rnd[1],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_sft[1],
						0, 0, 0));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_CLUT_C+i*16,   0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][3],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][2],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][1],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][0]));
			RADIX_WRITE_CHN_MCE (RADIX_SLUT_MCE_CLUT_C+i*16+4, 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][7],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][6],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][5],
						radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][4]));
		}
#elif defined(CONFIG_SOC_PRJ008)
		mce_pref_expd = (0
				+ RADIX_MCE_PREF_EXPD_L(60)
				+ RADIX_MCE_PREF_EXPD_R(60)
				+ RADIX_MCE_PREF_EXPD_D(30)
				+ RADIX_MCE_PREF_EXPD_T(30));
		mce_glb_ctrl = (0
				+ RADIX_MCE_GLB_CTRL_INIT
				+ RADIX_MCE_GLB_CTRL_FMT(0)                                      //encode formate 0:x265, 1:svac
				+ RADIX_MCE_GLB_CTRL_ED(0)                                       //0:encode, 1:decode
				+ RADIX_MCE_GLB_CTRL_TMVP(s->emc_mvi_en && (s->frm_num_gop > 1)) //tmvp en
				+ RADIX_MCE_GLB_CTRL_RMV(s->mref_en)                             //mref en
				+ RADIX_MCE_GLB_CTRL_MWT(s->mref_wei)                            //mref cost weight
				+ T33_RADIX_MCE_GLB_CTRL_SRM(s->sech_force_mv0_mode)
				+ RADIX_MCE_GLB_CTRL_VTYPE(0x0)                                  //video type 0:x265
				+ RADIX_MCE_GLB_CTRL_SRD(s->sech_force_mv0_en)                   //srd en
				+ RADIX_MCE_GLB_CTRL_PREF(0x0)                                   //pref neighbour mv en
				+ RADIX_MCE_GLB_CTRL_FMZ(s->mvp_force_zero_en)                   //force merge zero
				+ RADIX_MCE_GLB_CTRL_MFS(s->mvp_force_zero_motion_shift)         //force merge zero
				+ (0<<4)                                                         //crc_en
				+ RADIX_MCE_GLB_CTRL_FSAD(1));                                   // FSAD_CORE1_EN
		mce_bus_ctrl = (0
				+ RADIX_MCE_BUS_CTRL_OTPY(16) //prefy outstanding
				+ RADIX_MCE_BUS_CTRL_OTPC(8)  //prefc outstanding
				+ RADIX_MCE_BUS_CTRL_OTMY(8)  //missy outstanding
				+ RADIX_MCE_BUS_CTRL_OTMC(4)  //missc outstanding
				+ RADIX_MCE_BUS_CTRL_PROR(0)  //proirity
				+ RADIX_MCE_BUS_CTRL_ATHR(6)  //almost full threshold
				+ RADIX_MCE_BUS_CTRL_SENA(1)
				+ RADIX_MCE_BUS_CTRL_BFSH(s->buf_share_en));//buf_share_en
		mce_rcc_cfg0 = (0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_rcc_cfg1 = (0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_rcc_cfg2 = (0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_rcc_cfg3 = (0
				+ RADIX_MCE_RCC_TICK3(2)
				+ RADIX_MCE_RCC_TICK2(2)
				+ RADIX_MCE_RCC_TICK1(2)
				+ RADIX_MCE_RCC_TICK0(2));
		mce_sech_grp0 = (0
				+ VALID_VAL3(0x3, 29, s->mcec_grp[1].sech_bc_wei )
				+ VALID_VAL3(0x3, 27, s->mcec_grp[1].mvc_bc_wei  )
				+ VALID_VAL3(0x1, 26, s->mcec_grp[1].force_zero_mv_en)
				+ VALID_VAL3(0x7, 23, s->mcec_grp[1].cros_cfg_idx)
				+ VALID_VAL3(0x7, 20, s->mcec_grp[1].fs_cfg_idx  )
				+ VALID_VAL3(0x7, 17, s->mcec_grp[1].mcec_mode   )
				+ VALID_VAL3(0x1, 16, s->mcec_grp[1].mcec_en && s->mce_cfg_en)
				+ VALID_VAL3(0x3, 13, s->mcec_grp[0].sech_bc_wei )
				+ VALID_VAL3(0x3, 11, s->mcec_grp[0].mvc_bc_wei  )
				+ VALID_VAL3(0x1, 10, s->mcec_grp[0].force_zero_mv_en)
				+ VALID_VAL3(0x7, 7,  s->mcec_grp[0].cros_cfg_idx )
				+ VALID_VAL3(0x7, 4,  s->mcec_grp[0].fs_cfg_idx   )
				+ VALID_VAL3(0x7, 1,  s->mcec_grp[0].mcec_mode    )
				+ VALID_VAL3(0x1, 0,  s->mcec_grp[0].mcec_en && s->mce_cfg_en));

		mce_sech_grp1 = (0
				+ VALID_VAL3(0x3, 29, s->mcec_grp[3].sech_bc_wei )
				+ VALID_VAL3(0x3, 27, s->mcec_grp[3].mvc_bc_wei  )
				+ VALID_VAL3(0x1, 26, s->mcec_grp[3].force_zero_mv_en)
				+ VALID_VAL3(0x7, 23, s->mcec_grp[3].cros_cfg_idx)
				+ VALID_VAL3(0x7, 20, s->mcec_grp[3].fs_cfg_idx  )
				+ VALID_VAL3(0x7, 17, s->mcec_grp[3].mcec_mode   )
				+ VALID_VAL3(0x1, 16, s->mcec_grp[3].mcec_en && s->mce_cfg_en)
				+ VALID_VAL3(0x3, 13, s->mcec_grp[2].sech_bc_wei )
				+ VALID_VAL3(0x3, 11, s->mcec_grp[2].mvc_bc_wei  )
				+ VALID_VAL3(0x1, 10, s->mcec_grp[2].force_zero_mv_en)
				+ VALID_VAL3(0x7,  7, s->mcec_grp[2].cros_cfg_idx)
				+ VALID_VAL3(0x7,  4, s->mcec_grp[2].fs_cfg_idx  )
				+ VALID_VAL3(0x7,  1, s->mcec_grp[2].mcec_mode   )
				+ VALID_VAL3(0x1,  0, s->mcec_grp[2].mcec_en && s->mce_cfg_en));

		mce_sech_cfg0_l = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[0].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[0].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[0].mcec_lx));

		mce_sech_cfg0_h = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[0].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[0].mcec_by));

		mce_sech_cfg1_l = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[1].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[1].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[1].mcec_lx));

		mce_sech_cfg1_h = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[1].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[1].mcec_by));

		mce_sech_cfg2_l = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[2].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[2].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[2].mcec_lx));

		mce_sech_cfg2_h = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[2].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[2].mcec_by));

		mce_sech_cfg3_l = (0
				+ VALID_VAL3(0x1ff, 18, s->mcec_range_grp[3].mcec_ty)
				+ VALID_VAL3(0x1ff, 9,  s->mcec_range_grp[3].mcec_rx)
				+ VALID_VAL3(0x1ff, 0,  s->mcec_range_grp[3].mcec_lx));

		mce_sech_cfg3_h   = (0
				+ VALID_VAL3(0x3ff, 9, s->mcec_range_grp[3].skip_pixel)
				+ VALID_VAL3(0x1ff, 0, s->mcec_range_grp[3].mcec_by));
		mce_comp_gap = (0
				+ VALID_VAL2(22, 0x0) //prefc enable
				+ VALID_VAL2(21, s->jrfcv2_en)
				+ VALID_VAL2(19, s->buf_share_size)// 0~3,when size >= 3, trans 3
				+ VALID_VAL2(18, s->buf_share_en) //
				+ VALID_VAL2(12, s->mce_pref_mode) //(zero, top, left) pref enable
				+ T33_RADIX_MCE_COMP_GAP_CU8(4)
				+ T33_RADIX_MCE_COMP_GAP_CU16(1)
				+ T33_RADIX_MCE_COMP_GAP_CU32(1));

		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_PREF_EXPD,           0, mce_pref_expd);
		RADIX_WRITE_CHN_MCE (RADIX_REG_MCE_GLB_CTRL,            0, mce_glb_ctrl);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_BYND_YADDR,          0, (int)bfsh_beyond_yaddr);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_BYND_CADDR,          0, (int)bfsh_beyond_caddr);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_BUS_CTRL,            0, mce_bus_ctrl);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_RCC_CTRL,            0, (0x1));
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_RCC_CFG0,            0, mce_rcc_cfg0);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_RCC_CFG1,            0, mce_rcc_cfg1);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_RCC_CFG2,            0, mce_rcc_cfg2);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_RCC_CFG3,            0, mce_rcc_cfg3);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_GRP0,           0, mce_sech_grp0);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_GRP1,           0, mce_sech_grp1);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG0_L,         0, mce_sech_cfg0_l);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG0_H,         0, mce_sech_cfg0_h);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG1_L,         0, mce_sech_cfg1_l);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG1_H,         0, mce_sech_cfg1_h);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG2_L,         0, mce_sech_cfg2_l);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG2_H,         0, mce_sech_cfg2_h);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG3_L,         0, mce_sech_cfg3_l);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_MCE_SECH_CFG3_H,         0, mce_sech_cfg3_h);
		RADIX_WRITE_CHN_MCE (T33_RADIX_REG_COMP_GAP,                0, mce_comp_gap);
#endif
	}

/*************************************************************************************
  STC Module
 *************************************************************************************/
	common_slv_cfg3 = (0
			+ VALID_VAL2 (31, 0x1)//slice_init
			+ all_init_state[152]);
	spe_slv_cfg3    = (0
			+ VALID_VAL3 (0x1,  31, 0x1)
			+ VALID_VAL3 (0xf,  23, s->stc_fixed_offset3)
			+ VALID_VAL3 (0xf,  19, s->stc_fixed_offset2)
			+ VALID_VAL3 (0xf,  15, s->stc_fixed_offset1)
			+ VALID_VAL3 (0xf,  11, s->stc_fixed_offset0)
			+ VALID_VAL3 (0x1f, 6,  s->stc_fixed_subtype)
			+ VALID_VAL3 (0xf,  2,  s->stc_fixed_mode)
			+ VALID_VAL3 (0x3,  0,  s->stc_fixed_merge));
	stc_slv_cfg3    = s->stc_cfg_mode == 0 ? common_slv_cfg3 : spe_slv_cfg3;
	stc_cfg0        = (0
			+ VALID_VAL3 (0x3,    30, s->stc_cfg_mode)
			+ VALID_VAL3 (0x1fff, 13, s->frame_height)
			+ VALID_VAL3 (0x1fff, 0,  s->frame_width));
#ifdef CONFIG_SOC_PRJ007
	stc_cfg1        =  0+(((0 & 0x1) << 31) +//merge_ctrl
			((5 & 0xf) << 20) +//merge_num_h
			((5 & 0xf) << 16) +//merge_num_w
			(lcu_num_h & 0xff) << 8) +
	(lcu_num_w & 0xff);
#elif defined(CONFIG_SOC_PRJ008)
	stc_cfg1        = (0
			+ VALID_VAL3 (0x1, 31, 0)         //merge_ctrl
			+ VALID_VAL3 (0xf, 20, 5)         //merge_num_h
			+ VALID_VAL3 (0xf, 16, 5)         //merge_num_w
			+ VALID_VAL3 (0xff, 8, lcu_num_h)
			+ VALID_VAL3 (0xff, 0, lcu_num_w));
#endif
	RADIX_GEN_VDMA_ACFG(chn,RADIX_STC_BASE_VDMA + RADIX_REG_STC_CFG0,   0, stc_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_STC_BASE_VDMA + RADIX_REG_STC_CFG1,   0, stc_cfg1);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_STC_BASE_VDMA + RADIX_REG_STC_SAO_MG, 0, all_init_state[151]);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_STC_BASE_VDMA + RADIX_REG_STC_SAO_TP, 0, stc_slv_cfg3);
	/*************************************************************************************
	  SAO Module
	 *************************************************************************************/
	sao_info = (0
			+ VALID_VAL3 (0x3, 30, 0) // lcu_sz
			+ VALID_VAL3 (0x3, 28, 0) // lcu_sz
			+ VALID_VAL3 (0x1, 11, 0) // FLUSH_SLICE
			+ VALID_VAL2 (     10, 1) // MODE_SEL
			+ VALID_VAL3 (0x1, 9,  1) // cr_tile
			+ VALID_VAL3 (0x1, 8,  1) // cr_slice
			+ VALID_VAL2 (     7,  0)
			+ VALID_VAL2 (     6,  0)
			+ VALID_VAL2 (     5,  0)
			+ VALID_VAL3 (0x1, 4,  1) // sao_y_flag
			+ VALID_VAL3 (0x1, 3,  0) // sao_c_flag
			+ VALID_VAL3 (0x1, 2,  1) // chroma
			+ VALID_VAL3 (0x1, 1,  s->sao_en)
			+ VALID_VAL3 (0x1, 0,  0));
	sao_size = (0
			+ VALID_VAL3 (0xffff, 16, s->frame_width )
			+ VALID_VAL3 (0xffff, 0,  s->frame_height));
	sao_slice= (0
			+ VALID_VAL3 (0xffff, 16, 0)   // slice_x
			+ VALID_VAL3 (0xffff, 0,  0)); // slice_y

	RADIX_GEN_VDMA_ACFG(chn,RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_GLB_INFO,   0, sao_info);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_PIC_SIZE,   0, sao_size);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_SLICE_XY,   0, sao_slice);
	RADIX_GEN_VDMA_ACFG(chn,RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_SLICE_INIT, 0, 1);        //slice_init
	/*************************************************************************************
	  TMC Module
	 *************************************************************************************/
	RADIX_GEN_VDMA_ACFG(chn,RADIX_TMC_BASE_VDMA + HERA_REG_TMC_CTRL, 0, ( 0 | (0 << 1) ));

	/*************************************************************************************
	  EMC Module
	 *************************************************************************************/
	T33_emc_cfg_en = ( 1  + /* initial */
			+ VALID_VAL2 (1,  s->emc_bs_en)
			+ VALID_VAL2 (2,  s->emc_cps_en)
			+ VALID_VAL2 (3,  s->emc_mix_en)
			+ VALID_VAL2 (4,  s->emc_mvo_en)
			+ VALID_VAL2 (5,  s->md_cfg_en)
			+ VALID_VAL2 (6,  s->emc_qpt_en)
			+ VALID_VAL2 (7,  s->mce_cfg_en)
			+ VALID_VAL2 (8,  s->ip_cfg_en)
			+ VALID_VAL2 (9,  s->emc_mvi_en)
			+ VALID_VAL2 (10, s->ifa_para.rrs_en)
			+ VALID_VAL2 (11, s->ifa_para.rrs1_en)
			+ VALID_VAL2 (12, s->emc_ai_en));

	T32_emc_cfg_en = ( 1  + /* initial */
			+ VALID_VAL2 (1,  s->emc_bs_en)
			+ VALID_VAL2 (2,  s->emc_cps_en)
			+ VALID_VAL2 (3,  s->emc_sobel_en)
			+ VALID_VAL2 (4,  s->emc_mix_en)
			+ VALID_VAL2 (5,  s->emc_mixc_en)
			+ VALID_VAL2 (6,  s->emc_mvo_en)
			+ VALID_VAL2 (7,  s->emc_flag_en)
			+ VALID_VAL2 (8,  s->md_cfg_en)
			+ VALID_VAL2 (9,  s->emc_qpt_en)
			+ VALID_VAL2 (10, s->mce_cfg_en)
			+ VALID_VAL2 (11, s->ip_cfg_en)
			+ VALID_VAL2 (12, s->emc_mvi_en)
			+ VALID_VAL2 (13, s->ifa_para.rrs_en)
			+ VALID_VAL2 (14, s->ifa_para.rrs1_en)
			+ VALID_VAL2 (15, s->emc_ai_en));

#define RADIX_WRITE_CHN_EMC(addr_ofst, val, var)   RADIX_GEN_VDMA_ACFG(chn, RADIX_EMC_BASE_VDMA + addr_ofst, val, var)

	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_BSFULL_EN,   0, s->bsfull_intr_en);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_BSFULL_SIZE, 0, s->bsfull_intr_size);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_BS0,    0, (int)s->emc_bs_addr0);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_BS1,    0, (int)s->emc_bs_addr1);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_CPS,    0, (int)s->emc_cps_addr);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_SOBEL, 0, (int)s->emc_sobel_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MIX,   0, (int)s->emc_mix_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MIXC , 0, (int)s->emc_mixc_addr );
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MVO,   0, (int)s->emc_mvo_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_FLAG , 0, (int)s->emc_flag_addr );
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MIX,    0, (int)s->emc_mix_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MVO,    0, (int)s->emc_mvo_addr);
#endif
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MDC,    0, (int)s->emc_mdc_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_QPT,    0, (int)s->emc_qpt_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MCEC,   0, (int)s->emc_mcec_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_IPC,    0, (int)s->emc_ipc_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_MVI,    0, (int)s->emc_mvi_addr);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_RRS0,   0, (int)s->ifa_para.cps_ref0);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_RRS1,   0, (int)s->ifa_para.cps_ref1);
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_AI,     0, (int)s->emc_ai_addr);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_CFG,        0, T32_emc_cfg_en);
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_CFG,        0, T33_emc_cfg_en);
#endif

	dbg_emc_flag_ram_addr = 0xC5;  //ram addr
	dbg_emc_flag_data_high = 1;    //ram data , 1 is [127:64], 0 is [63:0];
	dbg_cfg_info = (0
			+ VALID_VAL3 (0x1,   16, dbg_emc_flag_data_high)
			+ VALID_VAL3 (0xffff, 0, dbg_emc_flag_ram_addr));
	RADIX_WRITE_CHN_EMC (RADIX_REG_EMC_ADDR_DBG_CFG,  0, dbg_cfg_info);

	/*************************************************************************************
	  EFE Module
	 *************************************************************************************/
	intra_en  = (s->frame_type == 2) ? 1 : (s->ipred_size ? 1 : 0);
	pp_en     = (s->sobel_en || s->ppred_slv_en) & intra_en;
	inter_en  = s->frame_type != 2;
	max_outstanding_num = 16;
	T33_efe_extra_cfg0 = (0
			+ VALID_VAL3 (0x1 , 26,  s->ifa_para.row_smd_unit)
			+ VALID_VAL3 (0x3f, 20,  max_outstanding_num)
			+ VALID_VAL3 (0x1, 17, ((~(s->frame_type == 2))))
			+ VALID_VAL3 (0x1, 16, 0)                   // 0:265, 1:264
			+ VALID_VAL3 (0x1, 12, 1)                   // md/mce cfg parity check enable
			+ VALID_VAL3 (0x3, 5,  1)
			+ VALID_VAL3 (0x1, 4,  1)
			+ VALID_VAL3 (0x1, 0,  0));
	T32_efe_extra_cfg0 = (0
			+ VALID_VAL3 (0x1, 17, ((~(s->frame_type == 2))))
			+ VALID_VAL3 (0x1, 16, 0)                   // 0:265, 1:264
			+ VALID_VAL3 (0x1, 12, 1)                   // md/mce cfg parity check enable
			+ VALID_VAL3 (0x3, 5,  s->rm.srd_cfg)
			+ VALID_VAL3 (0x1, 4,  1)
			+ VALID_VAL3 (0x1, 0,  s->ifa_para.qpg_cu32_qpm));
	efe_extra_cfg1     = (0
			+ VALID_VAL3 (0x1  , 8,  1)    // raw fetch ereq min interval enable
			+ VALID_VAL3 (0xff , 0,  40)); // raw fetch ereq min interval value, 0~255
	efe_ctrl           = (0
			+ RADIX_EFE_CTRL_YUV(s->raw_format)
			+ RADIX_EFE_CTRL_IP(intra_en)
			+ RADIX_EFE_CTRL_PP(pp_en)
			+ RADIX_EFE_CTRL_MCE(inter_en)
			+ RADIX_EFE_CTRL_CKG(1)
			+ RADIX_EFE_CTRL_SAO(s->sao_en)
			+ RADIX_EFE_CTRL_WM(0)
			+ RADIX_EFE_CTRL_GRAY(s->rm.gray_en) //T33=0
			+ RADIX_EFE_CTRL_CRC(0)
			+ RADIX_EFE_CTRL_MVC(s->emc_mvi_en)
			+ RADIX_EFE_CTRL_CHNG(s->chng_en)
			+ RADIX_EFE_CTRL_IPC(s->ip_cfg_en)
			+ RADIX_EFE_CTRL_MDC(s->md_cfg_en)
			+ RADIX_EFE_CTRL_MCEC(s->mce_cfg_en)
			+ RADIX_EFE_CTRL_FMT(0) //s->video_type in T32
			+ RADIX_EFE_CTRL_ROTATE(s->rotate_mode));
	rrs_cfg            = (0
			+ VALID_VAL2 (16, s->ifa_para.motion_thrd)
			+ VALID_VAL2 (8,  s->ifa_para.motion_of_thrd)
			+ VALID_VAL2 (4,  s->ifa_para.rrs_thrd)
			+ VALID_VAL2 (3,  s->ifa_para.motion_en)
			+ VALID_VAL2 (2,  s->rm.rrs_of_en)  //T33=0
			+ VALID_VAL2 (1,  s->rm.rrs_mode)   //T33=0
			+ VALID_VAL2 (0,  s->ifa_para.rrs_en));
	rfh_cfg0           = (0
			+ VALID_VAL2 (16, s->ifa_para.rfh_flat_thrd)
			+ VALID_VAL2 (8,  s->ifa_para.rfh_raw_thrd)
			+ VALID_VAL2 (0,  s->ifa_para.rfh_raw_thrd));
	rfh_cfg1           = (0
			+ VALID_VAL2 (28, s->ifa_para.rrs_filter_mode)
			+ VALID_VAL2 (25, s->ifa_para.filter_thrd[3])
			+ VALID_VAL2 (22, s->ifa_para.filter_thrd[2])
			+ VALID_VAL2 (19, s->ifa_para.filter_thrd[1])
			+ VALID_VAL2 (16, s->ifa_para.filter_thrd[0])
			+ VALID_VAL2 (8,  s->ifa_para.rfh_edge_thrd)
			+ VALID_VAL2 (1,  s->ifa_para.refresh_mode)
			+ VALID_VAL2 (0,  s->ifa_para.refresh_en));
	T33_qpg_cfg        = (0
			+ VALID_VAL3 (0x3 , 20, s->ifa_para.qpg_cu32_qpm)
			+ VALID_VAL3 (0x3f, 14, s->ifa_para.qpg_min_qp)
			+ VALID_VAL3 (0x3f, 8,  s->ifa_para.qpg_max_qp)
			+ VALID_VAL2 (      6,  s->ifa_para.smd_filter_thrd)
			+ VALID_VAL2 (      5,  s->ifa_para.qpg_smd_en)
			+ VALID_VAL2 (      4,  s->ifa_para.qpg_sobel_en)
			+ VALID_VAL2 (      3,  s->ifa_para.qpg_ery_en)
			+ VALID_VAL2 (      2,  s->ifa_para.qpg_skin_en)
			+ VALID_VAL2 (      1,  s->ifa_para.qpg_table_en)
			+ VALID_VAL2 (      0,  s->ifa_para.qpg_en));
	T32_qpg_cfg        = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_skin_qp_ofst[2])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_skin_qp_ofst[1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_skin_qp_ofst[0])
			+ VALID_VAL2 (      6,  s->ifa_para.crp_thrd)
			+ VALID_VAL2 (      5,  s->ifa_para.qpg_smd_en)
			+ VALID_VAL2 (      4,  s->ifa_para.qpg_sobel_en)
			+ VALID_VAL2 (      3,  s->ifa_para.qpg_ery_en)
			+ VALID_VAL2 (      2,  s->ifa_para.qpg_skin_en)
			+ VALID_VAL2 (      1,  s->ifa_para.qpg_table_en)
			+ VALID_VAL2 (      0,  s->ifa_para.qpg_en));
	qpg_cfg1          = (0
			+ VALID_VAL3 (0x1,  22, s->rc_en)
			+ VALID_VAL3 (0x3f, 16, s->rc_min_qp)
			+ VALID_VAL3 (0x3f, 10, s->rc_max_qp)
			+ VALID_VAL3 (0x1,  9,  s->rc_dly_en)
			+ VALID_VAL3 (0x1,  8,  s->ifa_para.ai_mark_en)
			+ VALID_VAL3 (0xf,  4,  s->ifa_para.ai_mark_type)
			+ VALID_VAL3 (0x1,  3,  s->ifa_para.qpg_roi_en)
			+ VALID_VAL3 (0x1,  2,  s->ifa_para.qpg_filte_en)
			+ VALID_VAL3 (0x1,  1,  s->ifa_para.pet_fil_en)
			+ VALID_VAL3 (0x1,  0,  s->ifa_para.qpg_petbc_en));
	cplx_qp0          = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_cplx_qp_ofst[3])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_cplx_qp_ofst[2])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_cplx_qp_ofst[1])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_cplx_qp_ofst[0]));
	cplx_qp1          = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_cplx_qp_ofst[7])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_cplx_qp_ofst[6])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_cplx_qp_ofst[5])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_cplx_qp_ofst[4]));
	pet_cplx_thrd00   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[0][ 0])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[0][ 1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[0][ 2])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[0][ 3]));
	pet_cplx_thrd01   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[0][ 4])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[0][ 5])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[0][ 6])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[0][ 7]));
	pet_cplx_thrd02   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[0][ 8])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[0][ 9])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[0][10])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[0][11]));
	pet_cplx_thrd10   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[1][ 0])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[1][ 1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[1][ 2])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[1][ 3]));
	pet_cplx_thrd11   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[1][ 4])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[1][ 5])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[1][ 6])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[1][ 7]));
	pet_cplx_thrd12   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[1][ 8])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[1][ 9])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[1][10])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[1][11]));
	pet_cplx_thrd20   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[2][ 0])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[2][ 1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[2][ 2])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[2][ 3]));
	pet_cplx_thrd21   = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[2][ 4])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[2][ 5])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[2][ 6])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[2][ 7]));
	pet_cplx_thrd22    = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.qpg_pet_cplx_thrd[2][ 8])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.qpg_pet_cplx_thrd[2][ 9])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.qpg_pet_cplx_thrd[2][10])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.qpg_pet_cplx_thrd[2][11]));
	pet_qp_ofst0       = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst[0][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst[0][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst[0][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst[0][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst[0][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst[0][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst[0][6]));
	pet_qp_ofst1       = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst[1][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst[1][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst[1][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst[1][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst[1][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst[1][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst[1][6]));
	pet_qp_ofst2       = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst[2][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst[2][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst[2][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst[2][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst[2][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst[2][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst[2][6]));
	pet_qp_ofst3       = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst[3][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst[3][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst[3][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst[3][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst[3][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst[3][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst[3][6]));
	pet_qp_ofst4       = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst[4][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst[4][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst[4][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst[4][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst[4][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst[4][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst[4][6]));
	pet_qp_ofst_mt0    = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst_mt[0][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst_mt[0][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst_mt[0][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst_mt[0][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst_mt[0][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst_mt[0][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst_mt[0][6]));
	pet_qp_ofst_mt1    = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst_mt[1][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst_mt[1][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst_mt[1][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst_mt[1][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst_mt[1][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst_mt[1][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst_mt[1][6]));
	pet_qp_ofst_mt2    = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst_mt[2][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst_mt[2][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst_mt[2][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst_mt[2][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst_mt[2][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst_mt[2][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst_mt[2][6]));
	pet_qp_ofst_mt3    = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst_mt[3][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst_mt[3][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst_mt[3][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst_mt[3][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst_mt[3][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst_mt[3][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst_mt[3][6]));
	pet_qp_ofst_mt4    = (0
			+ VALID_VAL3 (0xf, 24, s->ifa_para.qpg_pet_qp_ofst_mt[4][0])
			+ VALID_VAL3 (0xf, 20, s->ifa_para.qpg_pet_qp_ofst_mt[4][1])
			+ VALID_VAL3 (0xf, 16, s->ifa_para.qpg_pet_qp_ofst_mt[4][2])
			+ VALID_VAL3 (0xf, 12, s->ifa_para.qpg_pet_qp_ofst_mt[4][3])
			+ VALID_VAL3 (0xf, 8,  s->ifa_para.qpg_pet_qp_ofst_mt[4][4])
			+ VALID_VAL3 (0xf, 4,  s->ifa_para.qpg_pet_qp_ofst_mt[4][5])
			+ VALID_VAL3 (0xf, 0,  s->ifa_para.qpg_pet_qp_ofst_mt[4][6]));
	qpg_dlt_thrd       = (0
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_dlt_thr[3])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_dlt_thr[2])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_dlt_thr[1])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_dlt_thr[0]));
	petbc_var_thr      = (0
			+ VALID_VAL3 (0x1ff, 18, s->ifa_para.petbc_var_thr[0])
			+ VALID_VAL3 (0x1ff, 9,  s->ifa_para.petbc_var_thr[1])
			+ VALID_VAL3 (0x1ff, 0,  s->ifa_para.petbc_var_thr[2]));
	petbc_ssm_thr0     = (0
			+ VALID_VAL3 (0xff, 8, s->ifa_para.petbc_ssm_thr[0][0])
			+ VALID_VAL3 (0xff, 0, s->ifa_para.petbc_ssm_thr[0][1]));
	petbc_ssm_thr1     = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.petbc_ssm_thr[1][0])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.petbc_ssm_thr[1][1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.petbc_ssm_thr[1][2])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.petbc_ssm_thr[1][3]));
	petbc_ssm_thr2     = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.petbc_ssm_thr[2][0])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.petbc_ssm_thr[2][1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.petbc_ssm_thr[2][2])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.petbc_ssm_thr[2][3]));
	petbc2_var_thr     = (0
			+ VALID_VAL3 (0x1ff, 18, s->ifa_para.petbc2_var_thr[0])
			+ VALID_VAL3 (0x1ff, 9,  s->ifa_para.petbc2_var_thr[1])
			+ VALID_VAL3 (0x1ff, 0,  s->ifa_para.petbc2_var_thr[2]));
	petbc2_ssm_thr0    = (0
			+ VALID_VAL3 (0xff, 8, s->ifa_para.petbc2_ssm_thr[0][0])
			+ VALID_VAL3 (0xff, 0, s->ifa_para.petbc2_ssm_thr[0][1]));
	petbc2_ssm_thr1    = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.petbc2_ssm_thr[1][0])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.petbc2_ssm_thr[1][1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.petbc2_ssm_thr[1][2])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.petbc2_ssm_thr[1][3]));
	petbc2_ssm_thr2    = (0
			+ VALID_VAL3 (0xff, 24, s->ifa_para.petbc2_ssm_thr[2][0])
			+ VALID_VAL3 (0xff, 16, s->ifa_para.petbc2_ssm_thr[2][1])
			+ VALID_VAL3 (0xff, 8,  s->ifa_para.petbc2_ssm_thr[2][2])
			+ VALID_VAL3 (0xff, 0,  s->ifa_para.petbc2_ssm_thr[2][3]));
	petbc_filter_valid = (0
			+ VALID_VAL3 (0x1, 3, s->ifa_para.pet_filter_valid[0])
			+ VALID_VAL3 (0x1, 2, s->ifa_para.pet_filter_valid[1])
			+ VALID_VAL3 (0x1, 1, s->ifa_para.pet_filter_valid[2])
			+ VALID_VAL3 (0x1, 0, s->ifa_para.pet_filter_valid[3]));
	ai_motion_smd_ofst0= (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[0][0][0])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[0][0][1])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[0][0][2])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[0][0][3])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[0][0][4]));
	ai_motion_smd_ofst1= (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[0][0][5])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[0][0][6])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[0][0][7])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[0][1][0])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[0][1][1]));
	ai_motion_smd_ofst2 = (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[0][1][2])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[0][1][3])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[0][1][4])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[0][1][5])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[0][1][6]));
	ai_motion_smd_ofst3   = (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[0][1][7])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[1][0][0])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[1][0][1])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[1][0][2])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[1][0][3]));
	ai_motion_smd_ofst4   = (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[1][0][4])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[1][0][5])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[1][0][6])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[1][0][7])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[1][1][0]));
	ai_motion_smd_ofst5   = (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[1][1][1])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[1][1][2])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[1][1][3])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[1][1][4])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[1][1][5]));
	ai_motion_smd_ofst6   = (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[1][1][6])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[1][1][7])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[2][0][0])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[2][0][1])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[2][0][2]));
	ai_motion_smd_ofst7   = (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[2][0][3])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[2][0][4])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[2][0][5])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[2][0][6])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[2][0][7]));
	ai_motion_smd_ofst8   = (0
			+ VALID_VAL3 (0x3f, 24, s->ifa_para.qpg_ai_motion_smd_ofst[2][1][0])
			+ VALID_VAL3 (0x3f, 18, s->ifa_para.qpg_ai_motion_smd_ofst[2][1][1])
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[2][1][2])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[2][1][3])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[2][1][4]));
	ai_motion_smd_ofst9   = (0
			+ VALID_VAL3 (0x3f, 12, s->ifa_para.qpg_ai_motion_smd_ofst[2][1][5])
			+ VALID_VAL3 (0x3f, 6,  s->ifa_para.qpg_ai_motion_smd_ofst[2][1][6])
			+ VALID_VAL3 (0x3f, 0,  s->ifa_para.qpg_ai_motion_smd_ofst[2][1][7]));
	smd_cplx_thrd0       = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_smd_cplx_thrd[1])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_smd_cplx_thrd[0]));
	smd_cplx_thrd1       = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_smd_cplx_thrd[3])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_smd_cplx_thrd[2]));
	smd_cplx_thrd2       = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_smd_cplx_thrd[5])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_smd_cplx_thrd[4]));
	smd_cplx_thrd3       = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_smd_cplx_thrd[7])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_smd_cplx_thrd[6]));
	sobel_cplx_thrd0      = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_sobel_cplx_thrd[1])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_sobel_cplx_thrd[0]));
	sobel_cplx_thrd1      = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_sobel_cplx_thrd[3])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_sobel_cplx_thrd[2]));
	sobel_cplx_thrd2      = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_sobel_cplx_thrd[5])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_sobel_cplx_thrd[4]));
	sobel_cplx_thrd3      = (0
			+ VALID_VAL2 (16, s->ifa_para.qpg_sobel_cplx_thrd[7])
			+ VALID_VAL2 (0,  s->ifa_para.qpg_sobel_cplx_thrd[6]));
	T33_ifa_cfg          = (0
			+ VALID_VAL2 (31, s->ifa_para.sobel_16_or_14)
			+ VALID_VAL2 (30, s->var_flat_sub_size)
			+ VALID_VAL2 (29, s->ifa_para.var_flat_en)
			+ VALID_VAL2 (28, s->ifa_para.cu_unifor_en)
			+ VALID_VAL2 (27, s->ifa_para.petbc_en)
			+ VALID_VAL3 (0xFFFF, 10, s->ifa_para.frm_mt_smd_thr)
			+ VALID_VAL2 (9,  s->ifa_para.emc_out_en[1])
			+ VALID_VAL2 (8,  s->ifa_para.emc_out_en[0])
			+ VALID_VAL2 (7,  s->ifa_para.sobel_en)
			+ VALID_VAL2 (6,  s->ifa_para.smd_en)
			+ VALID_VAL2 (5,  1)
			+ VALID_VAL2 (3,  s->ifa_para.ery_en)
			+ VALID_VAL2 (2,  0)
			+ VALID_VAL2 (1,  s->ifa_para.cps_en)
			+ VALID_VAL2 (0,  s->ifa_para.ifa_en));
	T32_ifa_cfg           = (0
			+ VALID_VAL2 (31, s->ifa_para.sobel_16_or_14)
			+ VALID_VAL2 (30, s->var_flat_sub_size)
			+ VALID_VAL2 (29, s->ifa_para.var_flat_en)
			+ VALID_VAL2 (28, s->ifa_para.cu_unifor_en)
			+ VALID_VAL2 (27, s->ifa_para.petbc_en)
			+ VALID_VAL2 (26, s->ifa_para.skin_lvl_en[2])
			+ VALID_VAL2 (25, s->ifa_para.skin_lvl_en[1])
			+ VALID_VAL2 (24, s->ifa_para.skin_lvl_en[0])
			+ VALID_VAL2 (16, s->ifa_para.sobel_edge_thrd)
			+ VALID_VAL2 (10, s->ifa_para.skin_cnt_thrd)
			+ VALID_VAL2 (9,  s->ifa_para.emc_out_en[1])
			+ VALID_VAL2 (8,  s->ifa_para.emc_out_en[0])
			+ VALID_VAL2 (7,  s->ifa_para.sobel_en)
			+ VALID_VAL2 (6,  s->ifa_para.smd_en)
			+ VALID_VAL2 (5,  s->rm.raw_avg_en)         //T33=0
			+ VALID_VAL2 (3,  s->ifa_para.ery_en)
			+ VALID_VAL2 (2,  s->rm.cps_mode)         //T33=0
			+ VALID_VAL2 (1,  s->ifa_para.cps_en)
			+ VALID_VAL2 (0,  s->ifa_para.ifa_en));
	T33_ifa_cfg1         = (0
			+ VALID_VAL3 (0x1, 15, s->frame_type == 2)
			+ VALID_VAL3 (0x7, 12, s->ifa_para.max_still_times)
			+ VALID_VAL3 (0x7, 9, s->ifa_para.still_times_motion_shift)
			+ VALID_VAL3 (0x1, 8, s->ifa_para.still_times_en )
			+ VALID_VAL3 (0x1, 7, s->srd_en)
			+ VALID_VAL3 (0x7, 4, s->srd_motion_shift)
			+ VALID_VAL3 (0x1, 3, s->ifa_para.ras_en)
			+ VALID_VAL3 (0x1, 2, s->ifa_para.rrs1_en)
			+ VALID_VAL3 (0x1, 1, s->ifa_para.pet_mode)
			+ VALID_VAL3 (0x1, 0, s->ifa_para.nei_cplx_en));

	T32_ifa_cfg1         = (0
			+ VALID_VAL3 (0x1, 7, s->srd_en)
			+ VALID_VAL3 (0x7, 4, s->srd_motion_shift)
			+ VALID_VAL3 (0x1, 3, s->ifa_para.ras_en)
			+ VALID_VAL3 (0x1, 2, s->ifa_para.rrs1_en)
			+ VALID_VAL3 (0x1, 1, s->ifa_para.pet_mode)
			+ VALID_VAL3 (0x1, 0, s->ifa_para.nei_cplx_en));

	ifac_all_en          = (0
			+ VALID_VAL2 (6, s->ifa_para.rrs1_c_en)
			+ VALID_VAL2 (5, s->ifa_para.cps_c_en)
			+ VALID_VAL2 (4, s->ifa_para.rrs_c_en)
			+ VALID_VAL2 (3, s->ifa_para.motion_c_en)
			+ VALID_VAL2 (2, s->ifa_para.smd_c_en));
	ifac_all1_en         = (0
			+ VALID_VAL2 (12, s->ifa_para.motion_thrd_c)
			+ VALID_VAL2 (4,  s->ifa_para.motion_of_thrd_c)
			+ VALID_VAL2 (0,  s->ifa_para.rrs_thrd_c));
	skin_thrd0       = (0
			+ VALID_VAL2 (24, s->ifa_para.skin_thrd[0][1][0])
			+ VALID_VAL2 (16, s->ifa_para.skin_thrd[0][1][1])
			+ VALID_VAL2 (8,  s->ifa_para.skin_thrd[0][0][0])
			+ VALID_VAL2 (0,  s->ifa_para.skin_thrd[0][0][1]));
	skin_thrd1       = (0
			+ VALID_VAL2 (24, s->ifa_para.skin_thrd[1][1][0])
			+ VALID_VAL2 (16, s->ifa_para.skin_thrd[1][1][1])
			+ VALID_VAL2 (8,  s->ifa_para.skin_thrd[1][0][0])
			+ VALID_VAL2 (0,  s->ifa_para.skin_thrd[1][0][1]));
	skin_thrd2       = (0
			+ VALID_VAL2 (24, s->ifa_para.skin_thrd[2][1][0])
			+ VALID_VAL2 (16, s->ifa_para.skin_thrd[2][1][1])
			+ VALID_VAL2 (8,  s->ifa_para.skin_thrd[2][0][0])
			+ VALID_VAL2 (0,  s->ifa_para.skin_thrd[2][0][1]));
	skin_lvl0        = (0
			+ VALID_VAL2 (16, s->ifa_para.skin_level[1])
			+ VALID_VAL2 (0,  s->ifa_para.skin_level[0]));
	skin_lvl1        = (0
			+ VALID_VAL2 (16, s->ifa_para.skin_level[3])
			+ VALID_VAL2 (0,  s->ifa_para.skin_level[2]));
	skin_factor          = (0
			+ VALID_VAL2 (24, s->ifa_para.skin_mul_factor[3])
			+ VALID_VAL2 (16, s->ifa_para.skin_mul_factor[2])
			+ VALID_VAL2 (8,  s->ifa_para.skin_mul_factor[1])
			+ VALID_VAL2 (0,  s->ifa_para.skin_mul_factor[0]));
	for(i=0;i<4;i++)
	roi_ctrl_cfg[i]       = (0
			+ VALID_VAL3 (0x3f, 26, s->roi_info[i*4+3].roi_qp)
			+ VALID_VAL3 (0x3,  24, s->roi_info[i*4+3].roi_md)
			+ VALID_VAL3 (0x3f, 18, s->roi_info[i*4+2].roi_qp)
			+ VALID_VAL3 (0x3,  16, s->roi_info[i*4+2].roi_md)
			+ VALID_VAL3 (0x3f, 10, s->roi_info[i*4+1].roi_qp)
			+ VALID_VAL3 (0x3,  8,  s->roi_info[i*4+1].roi_md)
			+ VALID_VAL3 (0x3f, 2,  s->roi_info[i*4+0].roi_qp)
			+ VALID_VAL3 (0x3,  0,  s->roi_info[i*4+0].roi_md));
	qpg_roi_area_en           = (0
			+ VALID_VAL3 (0x1, 15, s->roi_info[15].roi_en)
			+ VALID_VAL3 (0x1, 14, s->roi_info[14].roi_en)
			+ VALID_VAL3 (0x1, 13, s->roi_info[13].roi_en)
			+ VALID_VAL3 (0x1, 12, s->roi_info[12].roi_en)
			+ VALID_VAL3 (0x1, 11, s->roi_info[11].roi_en)
			+ VALID_VAL3 (0x1, 10, s->roi_info[10].roi_en)
			+ VALID_VAL3 (0x1, 9,  s->roi_info[ 9].roi_en)
			+ VALID_VAL3 (0x1, 8,  s->roi_info[ 8].roi_en)
			+ VALID_VAL3 (0x1, 7,  s->roi_info[ 7].roi_en)
			+ VALID_VAL3 (0x1, 6,  s->roi_info[ 6].roi_en)
			+ VALID_VAL3 (0x1, 5,  s->roi_info[ 5].roi_en)
			+ VALID_VAL3 (0x1, 4,  s->roi_info[ 4].roi_en)
			+ VALID_VAL3 (0x1, 3,  s->roi_info[ 3].roi_en)
			+ VALID_VAL3 (0x1, 2,  s->roi_info[ 2].roi_en)
			+ VALID_VAL3 (0x1, 1,  s->roi_info[ 1].roi_en)
			+ VALID_VAL3 (0x1, 0,  s->roi_info[ 0].roi_en));
	for(i=0;i<16;i++)
	roi_pos_cfg[i]        = (0
			+ VALID_VAL3 (0xff, 24, s->roi_info[i].roi_bmby)
			+ VALID_VAL3 (0xff, 16, s->roi_info[i].roi_umby)
			+ VALID_VAL3 (0xff, 8,  s->roi_info[i].roi_rmbx)
			+ VALID_VAL3 (0xff, 0,  s->roi_info[i].roi_lmbx));
	frm_cplx_thrd         = (0
			+ VALID_VAL3 (0xffff, 16, s->ifa_para.frm_cplx_thrd[1])   //[31:16]
			+ VALID_VAL3 (0xffff, 0,  s->ifa_para.frm_cplx_thrd[0])); //[15:0]
	rrc_motion_thr0       = (0
			+ VALID_VAL3 (0xffff, 16, s->ifa_para.rrc_motion_thr[1])   //[31:16]
			+ VALID_VAL3 (0xffff, 0,  s->ifa_para.rrc_motion_thr[0])); //[15:0]
	rrc_motion_thr1       = (0
			+ VALID_VAL3 (0xffff, 16, s->ifa_para.rrc_motion_thr[3])   //[31:16]
			+ VALID_VAL3 (0xffff, 0,  s->ifa_para.rrc_motion_thr[2])); //[15:0]
	rrc_motion_thr2       = (0
			+ VALID_VAL3 (0xffff, 16, s->ifa_para.rrc_motion_thr[5])   //[31:16]
			+ VALID_VAL3 (0xffff, 0,  s->ifa_para.rrc_motion_thr[4])); //[15:0]
	rrc_motion_thr3       = s->ifa_para.rrc_motion_thr[6] ;
	var_flat_thr          = s->var_flat_sub_size ? s->ifa_para.var_flat_thr[1][0] :  s->ifa_para.var_flat_thr[0][0];
	frm_flat_unifor_thrd  = (s->ifa_para.var_split_thrd             |
			(s->ifa_para.var_flat_thr[0][1] << 9)   |
			((s->var_flat_sub_size ? s->ifa_para.var_flat_thr[1][0] :  s->ifa_para.var_flat_thr[0][0])  << 18)  |
			(s->ifa_para.smd_blk16_bias[0] << 27)  |
			(s->ifa_para.smd_blk16_bias[1] << 29) );
	//        (0
	//                                          + VALID_VAL2 (29, s->ifa_para.smd_blk16_bias[1])
	//                                          + VALID_VAL2 (27, s->ifa_para.smd_blk16_bias[0])
	//                                          + VALID_VAL2 (18, var_flat_thr)
	//                                          + VALID_VAL2 (9,  s->ifa_para.var_flat_thr[0][1])
	//                                          + VALID_VAL2 (0,  s->ifa_para.var_split_thrd));
	pet_qp_idx            = (0
			+ VALID_VAL3 (0x1f, 20, s->ifa_para.qpg_pet_qp_idx[3])
			+ VALID_VAL3 (0x1f, 15, s->ifa_para.qpg_pet_qp_idx[3])
			+ VALID_VAL3 (0x1f, 10, s->ifa_para.qpg_pet_qp_idx[2])
			+ VALID_VAL3 (0x1f, 5,  s->ifa_para.qpg_pet_qp_idx[1])
			+ VALID_VAL3 (0x1f, 0,  s->ifa_para.qpg_pet_qp_idx[0]));
	frmb_ip_param0        = (0
			+ VALID_VAL3 (0x1f, 25, s->frmb_ip_bias[0][0])     //[29:25] 32 pla
			+ VALID_VAL3 (0x1,  24, s->frmb_ip_bits_en[4])     //[24]
			+ VALID_VAL3 (0x1,  23, s->frmb_ip_bits_en[3])     //[23]
			+ VALID_VAL3 (0x1,  22, s->frmb_ip_bits_en[2])     //[22]
			+ VALID_VAL3 (0x1,  21, s->frmb_ip_bits_en[1])     //[21]
			+ VALID_VAL3 (0x1,  20, s->frmb_ip_bits_en[0])     //[20]
			+ VALID_VAL3 (0xf,  16, s->frmb_idx[4])            //[19:16]
			+ VALID_VAL3 (0xf,  12, s->frmb_idx[3])            //[15:12]
			+ VALID_VAL3 (0xf,  8,  s->frmb_idx[2])            //[11:8]
			+ VALID_VAL3 (0xf,  4,  s->frmb_idx[1])            //[7:4]
			+ VALID_VAL3 (0xf,  0,  s->frmb_idx[0]));          //[3:0]
	frmb_ip_param1        = (0
			+ VALID_VAL3 (0x1f, 25, s->frmb_ip_bias[3][0])     //[29:25] 162 pla
			+ VALID_VAL3 (0x1f, 20, s->frmb_ip_bias[2][1])     //[24:20] 161 dc
			+ VALID_VAL3 (0x1f, 15, s->frmb_ip_bias[2][0])     //[19:15] 161 pla
			+ VALID_VAL3 (0x1f, 10, s->frmb_ip_bias[1][1])     //[14:10] 160 dc
			+ VALID_VAL3 (0x1f, 5,  s->frmb_ip_bias[1][0])     //[9:5] 160 pla
			+ VALID_VAL3 (0x1f, 0,  s->frmb_ip_bias[0][1]));   //[4:0] 32 dc
	frmb_ip_param2        = (0
			+ VALID_VAL3 (0x1f, 25, s->frmb_ipc_bias[2])       //[29:25] 16 dm1
			+ VALID_VAL3 (0x1f, 20, s->frmb_ipc_bias[1])       //[24:20] 16 dm0
			+ VALID_VAL3 (0x1f, 15, s->frmb_ipc_bias[0])       //[19:15] 32 dm
			+ VALID_VAL3 (0x1f, 10, s->frmb_ip_bias[4][1])     //[14:10] 163 dc
			+ VALID_VAL3 (0x1f, 5,  s->frmb_ip_bias[4][0])     //[9:5] 163 pla
			+ VALID_VAL3 (0x1f, 0,  s->frmb_ip_bias[3][1]));   //[4:0] 162 dc
	frmb_ip_param3        = (0
			+ VALID_VAL3 (0x1f, 5, s->frmb_ipc_bias[4])        //[9:5] 16 dm3
			+ VALID_VAL3 (0x1f, 0, s->frmb_ipc_bias[3]));      //[4:0] 16 dm2
	efe_frm_qp            = (0
			+ VALID_VAL2 (24, s->ifa_para.max_qp)
			+ VALID_VAL2 (16, s->ifa_para.min_qp)
			+ VALID_VAL2 (8,  s->ifa_para.base_qp)
			+ VALID_VAL2 (0,  s->rm.frame_qp_svac));              // T33=0
	frm_motion_thrd0      = s->rm.frm_motion_thrd[0] | ((s->rm.frm_motion_thrd[1] & 0xffff) << 16);
	frm_motion_thrd1      = s->rm.frm_motion_thrd[2] | ((s->rm.frm_motion_thrd[3] & 0xffff) << 16);
	efe_frm_size          = ((((s->rotate_mode == 1 || s->rotate_mode == 3) ? s->efe_hw_width: s->efe_hw_height) << 16) +
			(((s->rotate_mode == 1 || s->rotate_mode == 3) ? s->efe_hw_height : s->efe_hw_width) << 0) + 0);

#define RADIX_WRITE_CHN_EFE(addr_ofst, val, var)   RADIX_GEN_VDMA_ACFG(chn, RADIX_EFE_BASE_VDMA + addr_ofst, val, var)

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_CTRL,              0, efe_ctrl);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_FRM_SIZE,          0, efe_frm_size);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RAWY_ADDR,         0, (int)s->raw_y_pa);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RAWC_ADDR,         0, (int)s->raw_c_pa);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RAW_STRD,          0, s->frame_y_stride | (s->frame_c_stride<<16) );
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_REF_ADDR,      0, (int)s->rm.refy);//T33=0
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFAC_REF_ADDR, 0, (int)s->rm.refc);//T33=0
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_REF1_ADDR,     0, (int)s->rm.ref1);//T33=0
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_REF1C_ADDR,    0, (int)s->rm.ref1c);//T33=0
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_REF_ADDR,      0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFAC_REF_ADDR, 0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_REF1_ADDR,     0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_REF1C_ADDR,    0, 0);
#endif
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_FRM_QP,            0, efe_frm_qp);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RRS_CFG,           0, rrs_cfg);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RFH_CFG0,          0, rfh_cfg0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RFH_CFG1,          0, rfh_cfg1);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_QPG_CFG,       0, T32_qpg_cfg);
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_QPG_CFG,       0, T33_qpg_cfg);
#endif
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_QPG_CFG1,          0, qpg_cfg1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_QPG_DLT_THRD,      0, qpg_dlt_thrd);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_IDX,        0, pet_qp_idx);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD00,   0, pet_cplx_thrd00);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD01,   0, pet_cplx_thrd01);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD02,   0, pet_cplx_thrd02);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD10,   0, pet_cplx_thrd10);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD11,   0, pet_cplx_thrd11);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD12,   0, pet_cplx_thrd12);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD20,   0, pet_cplx_thrd20);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD21,   0, pet_cplx_thrd21);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_CPLX_THRD22,   0, pet_cplx_thrd22);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST0,      0, pet_qp_ofst0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST1,      0, pet_qp_ofst1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST2,      0, pet_qp_ofst2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST3,      0, pet_qp_ofst3);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST4,      0, pet_qp_ofst4);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST_MT0,   0, pet_qp_ofst_mt0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST_MT1,   0, pet_qp_ofst_mt1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST_MT2,   0, pet_qp_ofst_mt2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST_MT3,   0, pet_qp_ofst_mt3);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_QP_OFST_MT4,   0, pet_qp_ofst_mt4);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC_VAR_THR,     0, petbc_var_thr);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC_SSM_THR0,    0, petbc_ssm_thr0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC_SSM_THR1,    0, petbc_ssm_thr1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC_SSM_THR2,    0, petbc_ssm_thr2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC2_VAR_THR,    0, petbc2_var_thr);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC2_SSM_THR0,   0, petbc2_ssm_thr0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC2_SSM_THR1,   0, petbc2_ssm_thr1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PETBC2_SSM_THR2,   0, petbc2_ssm_thr2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PET_FILTER_VALID,  0, petbc_filter_valid);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST0,   0, ai_motion_smd_ofst0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST1,   0, ai_motion_smd_ofst1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST2,   0, ai_motion_smd_ofst2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST3,   0, ai_motion_smd_ofst3);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST4,   0, ai_motion_smd_ofst4);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST5,   0, ai_motion_smd_ofst5);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST6,   0, ai_motion_smd_ofst6);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST7,   0, ai_motion_smd_ofst7);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST8,   0, ai_motion_smd_ofst8);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_AI_MT_SMD_OFST9,   0, ai_motion_smd_ofst9);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_CPLX_QP0,          0, cplx_qp0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_CPLX_QP1,          0, cplx_qp1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SMD_CPLX_THRD0,    0, smd_cplx_thrd0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SMD_CPLX_THRD1,    0, smd_cplx_thrd1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SMD_CPLX_THRD2,    0, smd_cplx_thrd2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SMD_CPLX_THRD3,    0, smd_cplx_thrd3);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SOBEL_CPLX_THRD0,  0, sobel_cplx_thrd0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SOBEL_CPLX_THRD1,  0, sobel_cplx_thrd1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SOBEL_CPLX_THRD2,  0, sobel_cplx_thrd2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SOBEL_CPLX_THRD3,  0, sobel_cplx_thrd3);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI_EN,            0, qpg_roi_area_en);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI_INFOA,         0, roi_ctrl_cfg[0]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI_INFOB,         0, roi_ctrl_cfg[1]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI_INFOC,         0, roi_ctrl_cfg[2]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI_INFOD,         0, roi_ctrl_cfg[3]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI0_POS,          0, roi_pos_cfg[0]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI1_POS,          0, roi_pos_cfg[1]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI2_POS,          0, roi_pos_cfg[2]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI3_POS,          0, roi_pos_cfg[3]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI4_POS,          0, roi_pos_cfg[4]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI5_POS,          0, roi_pos_cfg[5]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI6_POS,          0, roi_pos_cfg[6]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI7_POS,          0, roi_pos_cfg[7]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI8_POS,          0, roi_pos_cfg[8]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI9_POS,          0, roi_pos_cfg[9]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI10_POS,         0, roi_pos_cfg[10]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI11_POS,         0, roi_pos_cfg[11]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI12_POS,         0, roi_pos_cfg[12]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI13_POS,         0, roi_pos_cfg[13]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI14_POS,         0, roi_pos_cfg[14]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_ROI15_POS,         0, roi_pos_cfg[15]);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFA_CFG,       0, T32_ifa_cfg);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFA_CFG1,      0, T32_ifa_cfg1);
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFA_CFG,       0, T33_ifa_cfg);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFA_CFG1,      0, T33_ifa_cfg1);
#endif
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SKIN_FTR,          0, skin_factor);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SKIN_LVL0,         0, skin_lvl0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SKIN_LVL1,         0, skin_lvl1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SKIN_THRD0,        0, skin_thrd0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SKIN_THRD1,        0, skin_thrd1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SKIN_THRD2,        0, skin_thrd2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG0,          0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG1,          0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG2,          0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG3,          0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG4,          0, 0);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG4,          0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_EXTRA_CFG0,    0, T32_efe_extra_cfg0);
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG5,      0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG6,      0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_DFT_CFG7,      0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_EXTRA_CFG0,    0, T33_efe_extra_cfg0);
	RADIX_WRITE_CHN_EFE( T33_RADIX_REG_EFE_EXTRA_CFG1,    0, efe_extra_cfg1);
#endif
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFAC_ALL_EN,       0, ifac_all_en);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_IFAC_ALL1_EN,      0, ifac_all1_en);
#ifdef CONFIG_SOC_PRJ007
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_MT_THRD0,      0, frm_motion_thrd0);//T33=0
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_MT_THRD1,      0, frm_motion_thrd1);//T33=0
#endif
#ifdef CONFIG_SOC_PRJ008
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_MT_THRD0,      0, 0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_MT_THRD1,      0, 0);
#endif
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RRC_MOTION_THR0,   0, rrc_motion_thr0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RRC_MOTION_THR1,   0, rrc_motion_thr1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RRC_MOTION_THR2,   0, rrc_motion_thr2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RRC_MOTION_THR3,   0, rrc_motion_thr3);

	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_PLAIN_UNIFOR_THRD, 0, frm_flat_unifor_thrd);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RAS_SAD_DIF_THR,   0, s->ifa_para.sobel_blk16_thr[1] << 16| s->ifa_para.sobel_blk16_thr[0]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_RAS_EDGE_CNT_THR,  0, s->ifa_para.sobel_blk16_thr[2]);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_FRMB_IP_PARAM0,    0, frmb_ip_param0);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_FRMB_IP_PARAM1,    0, frmb_ip_param1);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_FRMB_IP_PARAM2,    0, frmb_ip_param2);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_FRMB_IP_PARAM3,    0, frmb_ip_param3);
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_CPLX_THRD,         0, frm_cplx_thrd);

	/*     //efe wr row_smd_ram  */
	/*     for (i=0; i<64; i++) */
	/*       RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_SMD_WR_BASE_ADDR+i*4 ,       0, i); */
	RADIX_WRITE_CHN_EFE( RADIX_REG_EFE_STAT,              0, 1);
	RADIX_GEN_VDMA_ACFG(chn, RADIX_VDMA_BASE_VDMA | RADIX_REG_CFGC_ACM_CTRL, RADIX_VDMA_ACFG_TERM, 0);//VDMA start

#ifdef H265_VDMA_DEBUG
	unsigned int chn_start = (unsigned int)s->des_va;
	unsigned int chn_end   = (unsigned int)chn;
	unsigned int chn_size  = chn_end - chn_start;
	printk("kern-chn_used_size = 0x%x, chn_start = 0x%x, chn_end = 0x%x\n",chn_size, chn_start, chn_end);
	H265E_SliceInit_Dump_Chn(s, (unsigned int *)s->des_va, chn_size);
#endif
	}
