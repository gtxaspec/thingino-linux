#include "hera.h"//ttt
#include "h265_core.h"
#include <linux/kernel.h>

//context
typedef struct X_ContextModel {
	unsigned char m_state;  ///< internal state variable
	unsigned char bBinsCoded;
} X_ContextModel;

#define RADIX_X_NUM_SPLIT_FLAG_CTX            3       ///< number of context models for split flag
#define RADIX_X_NUM_SKIP_FLAG_CTX             3       ///< number of context models for skip flag

#define RADIX_X_NUM_MERGE_FLAG_EXT_CTX        1       ///< number of context models for merge flag of merge extended
#define RADIX_X_NUM_MERGE_IDX_EXT_CTX         1       ///< number of context models for merge index of merge extended

#define RADIX_X_NUM_PART_SIZE_CTX             4       ///< number of context models for partition size
#define RADIX_X_NUM_PRED_MODE_CTX             1       ///< number of context models for prediction mode

#define RADIX_X_NUM_ADI_CTX                   1       ///< number of context models for intra prediction

#define RADIX_X_NUM_CHROMA_PRED_CTX           2       ///< number of context models for intra prediction (chroma)
#define RADIX_X_NUM_INTER_DIR_CTX             5       ///< number of context models for inter prediction direction
#define RADIX_X_NUM_MV_RES_CTX                2       ///< number of context models for motion vector difference

#define RADIX_X_NUM_REF_NO_CTX                2       ///< number of context models for reference index
#define RADIX_X_NUM_TRANS_SUBDIV_FLAG_CTX     3       ///< number of context models for transform subdivision flags
#define RADIX_X_NUM_QT_CBF_CTX                6       ///< number of context models for QT CBF
#define RADIX_X_NUM_QT_ROOT_CBF_CTX           1       ///< number of context models for QT ROOT CBF
#define RADIX_X_NUM_DELTA_QP_CTX              3       ///< number of context models for dQP

#define RADIX_X_NUM_SIG_CG_FLAG_CTX           2       ///< number of context models for MULTI_LEVEL_SIGNIFICANCE

#define RADIX_X_NUM_SIG_FLAG_CTX              42      ///< number of context models for sig flag
#define RADIX_X_NUM_SIG_FLAG_CTX_LUMA         27      ///< number of context models for luma sig flag
#define RADIX_X_NUM_SIG_FLAG_CTX_CHROMA       15      ///< number of context models for chroma sig flag

#define RADIX_X_NUM_CTX_LAST_FLAG_XY          18      ///< number of context models for last coefficient position
#define RADIX_X_NUM_CTX_LAST_FLAG_XY_LUMA     15      ///< number of context models for last coefficient position of luma
#define RADIX_X_NUM_CTX_LAST_FLAG_XY_CHROMA    3      ///< number of context models for last coefficient position of chroma

#define RADIX_X_NUM_ONE_FLAG_CTX              24      ///< number of context models for greater than 1 flag
#define RADIX_X_NUM_ONE_FLAG_CTX_LUMA         16      ///< number of context models for greater than 1 flag of luma
#define RADIX_X_NUM_ONE_FLAG_CTX_CHROMA        8      ///< number of context models for greater than 1 flag of chroma
#define RADIX_X_NUM_ABS_FLAG_CTX               6      ///< number of context models for greater than 2 flag
#define RADIX_X_NUM_ABS_FLAG_CTX_LUMA          4      ///< number of context models for greater than 2 flag of luma
#define RADIX_X_NUM_ABS_FLAG_CTX_CHROMA        2      ///< number of context models for greater than 2 flag of chroma

#define RADIX_X_NUM_MVP_IDX_CTX               1       ///< number of context models for MVP index

#define RADIX_X_NUM_SAO_MERGE_FLAG_CTX        1       ///< number of context models for SAO merge flags
#define RADIX_X_NUM_SAO_TYPE_IDX_CTX          1       ///< number of context models for SAO type index

#define RADIX_X_NUM_TRANSFORMSKIP_FLAG_CTX    1       ///< number of context models for transform skipping
#define RADIX_X_NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX  1
#define RADIX_X_CNU                           154      ///< dummy initialization value for unused context models 'Context model Not Used'
#define RADIX_X_NUM_TOTAL                     156

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

unsigned char all_init_state[RADIX_X_NUM_TOTAL];
X_ContextModel X_contextModels[RADIX_X_NUM_TOTAL];

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


	int  slope      = (initValue >> 4) * 5 - 45;
	int  offset     = ((initValue & 15) << 3) - 16;
	int tmp_max = 0;
	int  initState  = 0;
	int mpState = 0;
	unsigned char N_state = 0;

	qp = qp < 0 ? 0 : qp > 51 ? 51 : qp;
	tmp_max = (((slope * qp) >> 4) + offset) > 1 ? (((slope * qp) >> 4) + offset) : 1;
	initState  = tmp_max < 126 ? tmp_max : 126;
	mpState = (initState >= 64);
	N_state = ((mpState ? (initState - 64) : (63 - initState)) << 1) + mpState;

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

void H265E_T32V_SliceInit(struct h265_slice_info *s)//ttt
{
	unsigned int i, j,ii;
	int checksum = 0;
	volatile unsigned int *chn = (volatile unsigned int *)s->des_va;
	//this is state cfg and need open for bc ,sde ,ipred.
	unsigned char all_init_state[RADIX_X_NUM_TOTAL];
	X_ContextModel X_contextModels[RADIX_X_NUM_TOTAL];
	int qp = s->frame_qp;
	int sliceType = s->context_type;
	struct h265_vdma_tmp *tmp = &s->tmp_val;

	tmp->RADIX_CFGC_BASE_VDMA =	 (RADIX_HID_M0 << 15);
	tmp->RADIX_VDMA_BASE_VDMA =	 (RADIX_HID_M1 << 15);
	tmp->RADIX_ODMA_BASE_VDMA =	 (RADIX_HID_M2 << 15);
	tmp->RADIX_TMC_BASE_VDMA  =	 (RADIX_HID_M3 << 15);
	tmp->RADIX_EFE_BASE_VDMA  =	 (RADIX_HID_M4 << 15);
	tmp->RADIX_JRFD_BASE_VDMA =	 (RADIX_HID_M5 << 15);
	tmp->RADIX_MCE_BASE_VDMA  =	 (RADIX_HID_M6 << 15);
	tmp->RADIX_TFM_BASE_VDMA  =    (RADIX_HID_M7 << 15);
	tmp->RADIX_MD_BASE_VDMA   =	 (RADIX_HID_M8 << 15);
	tmp->RADIX_DT_BASE_VDMA   =    (RADIX_HID_M9 << 15);
	tmp->RADIX_DBLK_BASE_VDMA =    (RADIX_HID_M10 << 15);
	tmp->RADIX_SAO_BASE_VDMA  =    (RADIX_HID_M11 << 15);
	tmp->RADIX_BC_BASE_VDMA   =    (RADIX_HID_M12 << 15);
	tmp->RADIX_SDE_BASE_VDMA  =    (RADIX_HID_M13 << 15);
	tmp->RADIX_IPRED_BASE_VDMA=	 (RADIX_HID_M14 << 15);
	tmp->RADIX_STC_BASE_VDMA  =    (RADIX_HID_M15 << 15);
	tmp->RADIX_EMC_BASE_VDMA  =    (RADIX_HID_M18 << 15);

	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SPLIT_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_SPLIT_FLAG, RADIX_X_NUM_SPLIT_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SKIP_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_SKIP_FLAG, RADIX_X_NUM_SKIP_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MERGE_FLAG_EXT_CTX], sliceType, qp, (unsigned char*)X_INIT_MERGE_FLAG_EXT, RADIX_X_NUM_MERGE_FLAG_EXT_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MERGE_IDX_EXT_CTX], sliceType, qp, (unsigned char*)X_INIT_MERGE_IDX_EXT, RADIX_X_NUM_MERGE_IDX_EXT_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_PART_SIZE_CTX], sliceType, qp, (unsigned char*)X_INIT_PART_SIZE, RADIX_X_NUM_PART_SIZE_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_PRED_MODE_CTX], sliceType, qp, (unsigned char*)X_INIT_PRED_MODE, RADIX_X_NUM_PRED_MODE_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_ADI_CTX], sliceType, qp, (unsigned char*)X_INIT_INTRA_PRED_MODE, RADIX_X_NUM_ADI_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CHROMA_PRED_CTX], sliceType, qp, (unsigned char*)X_INIT_CHROMA_PRED_MODE, RADIX_X_NUM_CHROMA_PRED_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_DELTA_QP_CTX], sliceType, qp, (unsigned char*)X_INIT_DQP, RADIX_X_NUM_DELTA_QP_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_INTER_DIR_CTX], sliceType, qp, (unsigned char*)X_INIT_INTER_DIR, RADIX_X_NUM_INTER_DIR_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_REF_NO_CTX], sliceType, qp, (unsigned char*)X_INIT_REF_PIC, RADIX_X_NUM_REF_NO_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MV_RES_CTX], sliceType, qp, (unsigned char*)X_INIT_MVD, RADIX_X_NUM_MV_RES_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_QT_CBF_CTX], sliceType, qp, (unsigned char*)X_INIT_QT_CBF, RADIX_X_NUM_QT_CBF_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_TRANS_SUBDIV_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_TRANS_SUBDIV_FLAG, RADIX_X_NUM_TRANS_SUBDIV_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_QT_ROOT_CBF_CTX], sliceType, qp, (unsigned char*)X_INIT_QT_ROOT_CBF, RADIX_X_NUM_QT_ROOT_CBF_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SIG_CG_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_SIG_CG_FLAG, 2 * RADIX_X_NUM_SIG_CG_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SIG_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_SIG_FLAG, RADIX_X_NUM_SIG_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CTX_LAST_FLAG_X], sliceType, qp, (unsigned char*)X_INIT_LAST, RADIX_X_NUM_CTX_LAST_FLAG_XY);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CTX_LAST_FLAG_Y], sliceType, qp, (unsigned char*)X_INIT_LAST, RADIX_X_NUM_CTX_LAST_FLAG_XY);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_ONE_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_ONE_FLAG, RADIX_X_NUM_ONE_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_ABS_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_ABS_FLAG, RADIX_X_NUM_ABS_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_MVP_IDX_CTX], sliceType, qp, (unsigned char*)X_INIT_MVP_IDX, RADIX_X_NUM_MVP_IDX_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SAO_MERGE_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_SAO_MERGE_FLAG, RADIX_X_NUM_SAO_MERGE_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_SAO_TYPE_IDX_CTX], sliceType, qp, (unsigned char*)X_INIT_SAO_TYPE_IDX, RADIX_X_NUM_SAO_TYPE_IDX_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_TRANSFORMSKIP_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_TRANSFORMSKIP_FLAG, 2 * RADIX_X_NUM_TRANSFORMSKIP_FLAG_CTX);
	xvec_initBuffer(&X_contextModels[RADIX_X_OFF_CU_TRANSQUANT_BYPASS_FLAG_CTX], sliceType, qp, (unsigned char*)X_INIT_CU_TRANSQUANT_BYPASS_FLAG, RADIX_X_NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX);
	for(i = 0 ; i< RADIX_X_NUM_TOTAL; i++){
		all_init_state[i] = X_contextModels[i].m_state;
	}

	tmp->max_lcux = s->frame_width%64 ? s->frame_width/64 : s->frame_width/64 - 1;
	tmp->max_lcuy = s->frame_height%64 ? s->frame_height/64 : s->frame_height/64 - 1;
	tmp->lcu_num_w = (s->frame_width%64) == 0 ? (s->frame_width >> 6) : (s->frame_width >> 6) + 1;
	tmp->lcu_num_h = (s->frame_height%64) == 0 ? (s->frame_height >> 6) : (s->frame_height >> 6) + 1;
	tmp->inter_cu8  = (s->frame_type == 2) ? 0 : (s->inter_mode[3][0] | s->inter_mode[3][3]);
	tmp->inter_cu16 = (s->frame_type == 2) ? 0 : (s->inter_mode[2][0] | s->inter_mode[2][1] | s->inter_mode[2][2] | s->inter_mode[2][3]);
	tmp->inter_cu32 = (s->frame_type == 2) ? 0 : (s->inter_mode[1][0] | s->inter_mode[1][1] | s->inter_mode[1][2] | s->inter_mode[1][3]);
	tmp->inter_cu64 = (s->frame_type == 2) ? 0 : s->inter_mode[0][3];
	tmp->intra_cu8  = s->ipred_size&0x1;
	tmp->intra_cu16 = (s->ipred_size>>1)&0x1;
	tmp->intra_cu32 = (s->ipred_size>>2)&0x1;
	tmp->cu8_enable  = tmp->inter_cu8  | tmp->intra_cu8;
	tmp->cu16_enable = tmp->inter_cu16 | tmp->intra_cu16;
	tmp->cu32_enable = tmp->inter_cu32 | tmp->intra_cu32;
	tmp->cu64_enable = tmp->inter_cu64;
	tmp->cu_enable    = ((s->frame_type == 2) ? (tmp->cu8_enable | (tmp->cu16_enable << 1) | (tmp->cu32_enable << 2)) :
			(tmp->cu8_enable | (tmp->cu16_enable << 1) | (tmp->cu32_enable << 2) | (tmp->cu64_enable << 3)) );
	tmp->inter_enable = (s->frame_type == 2) ? 0 : (tmp->inter_cu8 | (tmp->inter_cu16 << 1) | (tmp->inter_cu32 << 2) | (tmp->inter_cu64 << 3));
	tmp->intra_enable = tmp->intra_cu8 | (tmp->intra_cu16 << 1) | (tmp->intra_cu32 << 2);

	/*************************************************************************************
	  BC Module
	 *************************************************************************************/
	if(s->video_type == 1){//svac
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_CFG0 , 0, (((s->frame_type & 0x3) << 2)));//frame_width
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_CFG1 , 0, ((((((s->frame_height+15) >> 4) - 1) & 0xff) << 24) +//frame_height
					(((((s->frame_width+15) >> 4) - 1) & 0xff) << 16)  +
					(0x1 & 0x1)));/*slice_init*/

	}else{

		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_CFG0 , 0, ((((s->tu_inter_depth - 1) & 0x1) << 31) + //tu inter depth
					((0x1 & 0x1)                     << 30) + //gate
					((s->mref_en & 0x1)              << 29) + // mref_en
					((0x1 & 0x1)                     << 21) + //grey
					((s->use_dqp_flag & 0x1) << 20) + //dqp_flag
					((s->frame_type & 0x3) << 18) +//frm_tupe
					(((((s->frame_height) >> 3) - 1) & 0x1ff) << 9) +//frame_height
					((((s->frame_width) >> 3) - 1) & 0x1ff)));//frame_width
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_CFG1 , 0, (((s->sign_hide_flag & 0x1) << 10) +//hid_flag
					((s->merge_cand_num & 0x7) << 7) +//merge_cand_num
					((s->frame_qp & 0x3f) << 1) + //frame_qp
					0));


		tmp->qkcf_vector_last = 51;
		tmp->qkcf_vector_signcg = 48;
		tmp->qkcf_vector_sign = 54;
		tmp->qkcf_vector_m1 = 56;
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_CFG2 , 0, (((0x1 & 0x1) << 31) + /*slice_init*/
					((tmp->qkcf_vector_m1     & 0x3f) << 24) +//m1
					((tmp->qkcf_vector_sign   & 0x3f) << 16) +//sign
					((tmp->qkcf_vector_signcg & 0x3f) << 8) + //signcg
					(tmp->qkcf_vector_last    & 0x3f)));//last

		//0~9
		for(ii=0; ii<10; ii++){
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii]);
		}
		//10~12
		for(ii=10; ii<13; ii++){
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+2]);
		}

		//13~17
		for(ii=13; ii<18; ii++){
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+6]);
		}

		//18~21
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 18*4 , 0, all_init_state[26]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 19*4 , 0, all_init_state[27]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 20*4 , 0, all_init_state[150]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 21*4 , 0, all_init_state[24]);

		//tulv
		//22
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 22*4 , 0, all_init_state[37]);

		//23~25
		for(ii=23; ii<26; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+11]);

		//26~31
		for(ii=26; ii<32; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+2]);

		//32
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 32*4 , 0, all_init_state[16]);

		//sign cg 2
		//33~34
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 33*4 , 0, all_init_state[38]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 34*4 , 0, all_init_state[39]);
		//last x 15
		//35~49
		for(ii=35; ii<50; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+49]);
		//last y 15
		//idx:50~64, state:102~116
		for(ii=50; ii<65; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+52]);

		//sign 27
		//idx:65~91, state:42~68
		for(ii=65; ii<92; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii-23]);
		//m1 16
		//idx:92~107, state:120~135
		for(ii=92; ii<108; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+28]);

		//m2 4
		//idx:108~111, state:144~147
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 108*4 , 0, all_init_state[144]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 109*4 , 0, all_init_state[145]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 110*4 , 0, all_init_state[146]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 111*4 , 0, all_init_state[147]);
		////////
		// uv
		////////
		//signcg 2
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 112*4 , 0, all_init_state[40]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 113*4 , 0, all_init_state[41]);
		//last x 3
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 114*4 , 0, all_init_state[99]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 115*4 , 0, all_init_state[100]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 116*4 , 0, all_init_state[101]);
		//last y 3
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 117*4 , 0, all_init_state[117]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 118*4 , 0, all_init_state[118]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 119*4 , 0, all_init_state[119]);
		//sign 15
		//idx:120~134, state:69~83
		for(ii=120; ii<135; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii-51]);
		//m1 8
		//idx:135~142, state:136~143
		for(ii=135; ii<143; ii++)
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + ii*4 , 0, all_init_state[ii+1]);
		//m2 2
		//idx:143~144, state:148~149
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 143*4 , 0, all_init_state[148]);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_BC_BASE_VDMA + RADIX_REG_BC_STATE_BASE + 144*4 , 0, all_init_state[149]);
	}
	/*************************************************************************************
	  SDE Module
	 *************************************************************************************/


	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_CFG0 , 0, ((((s->tu_inter_depth - 1) & 0x1) << 31) + //tu inter depth
				((s->mref_en & 0x1)<<29) +
				((s->merge_cand_num & 0x7)<<22) +
				((s->use_hp & 0x1)<<21) +
				((s->interp_filter & 0x1)<<20) +
				((s->dqp_max_depth & 0x3)<<18) +
				((s->sao_c_flag & 0x1)<<17) +
				((s->sao_y_flag & 0x1)<<16) +
				((s->sao_en & 0x1)<<15) +
				((s->sign_hide_flag & 0x1)<<14) +
				((s->roi_flag & 0x1)<<13) +
				((s->frame_qp_svac & 0xff) <<5) +
				((s->use_dqp_flag & 0x1) <<4) +
				((s->frame_type & 0x3) <<2) +
				1 /*isHEVC*/ ));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_CFG1 , 0, (((s->frame_width & 0xffff) << 16) +
				((s->frame_height & 0xffff) << 0)) ) ;
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_BS_ADDR , 0, s->bitstream_pa);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_BS_LENG , 0, 0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_MAX_BS_SIZE , 0, (((tmp->max_bs_size & 0xFFFFFF) +
					((tmp->max_bs_en  & 0x1)<<31)))) ;
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_INIT , 0, 1);
	for(i=0;i<24;i++)
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_STATE_BASE + i*4 , 0, all_init_state[i]);
	for(i=24;i<151;i++)
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_STATE_BASE + i*4 , 0, all_init_state[i+2]);
	//mref
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SDE_BASE_VDMA + RADIX_REG_SDE_STATE_BASE + 151*4 , 0, all_init_state[24]);

	/*************************************************************************************
	  IPRED Module
	 *************************************************************************************/

	tmp->ipmd_mpm_cost_biass32 =
		s->ipmd_mpm_cost_bias[0][0][3] << 18 |
		s->ipmd_mpm_cost_bias[0][0][2] << 12 |
		s->ipmd_mpm_cost_bias[0][0][1] << 6 |
		s->ipmd_mpm_cost_bias[0][0][0];
	tmp->ipmd_mpm_cost_biasw32 =
		s->ipmd_mpm_cost_bias[0][1][3] << 18 |
		s->ipmd_mpm_cost_bias[0][1][2] << 12 |
		s->ipmd_mpm_cost_bias[0][1][1] << 6 |
		s->ipmd_mpm_cost_bias[0][1][0];
	tmp->ipmd_mpm_cost_biass16 =
		s->ipmd_mpm_cost_bias[1][0][3] << 18 |
		s->ipmd_mpm_cost_bias[1][0][2] << 12 |
		s->ipmd_mpm_cost_bias[1][0][1] << 6 |
		s->ipmd_mpm_cost_bias[1][0][0];
	tmp->ipmd_mpm_cost_biasw16 =
		s->ipmd_mpm_cost_bias[1][1][3] << 18 |
		s->ipmd_mpm_cost_bias[1][1][2] << 12 |
		s->ipmd_mpm_cost_bias[1][1][1] << 6 |
		s->ipmd_mpm_cost_bias[1][1][0];
	tmp->ipmd_mpm_cost_biass8 =
		s->ipmd_mpm_cost_bias[2][0][3] << 18 |
		s->ipmd_mpm_cost_bias[2][0][2] << 12 |
		s->ipmd_mpm_cost_bias[2][0][1] << 6 |
		s->ipmd_mpm_cost_bias[2][0][0];
	tmp->ipmd_mpm_cost_biasw8 =
		s->ipmd_mpm_cost_bias[2][1][3] << 18 |
		s->ipmd_mpm_cost_bias[2][1][2] << 12 |
		s->ipmd_mpm_cost_bias[2][1][1] << 6 |
		s->ipmd_mpm_cost_bias[2][1][0];

	tmp->ipmd_angle_bias =
		s->ipmd_mpm_use_maxnum << 15 |
		s->i_pmd_en << 14 |
		s->frm_ipmd_angle_en << 13 |
		s->intra_angle_bias[2] << 7 |
		s->intra_angle_bias[1] << 1 |
		s->intra_angle_bias[0];

	tmp->ipmd_dm_bias32 =
		s->ipmd_dm_cost_bias[0][2] << 12 |
		s->ipmd_dm_cost_bias[0][1] << 6 |
		s->ipmd_dm_cost_bias[0][0];
	tmp->ipmd_dm_bias16 =
		s->ipmd_dm_cost_bias[1][4] << 24 |
		s->ipmd_dm_cost_bias[1][3] << 18 |
		s->ipmd_dm_cost_bias[1][2] << 12 |
		s->ipmd_dm_cost_bias[1][1] << 6 |
		s->ipmd_dm_cost_bias[1][0];
	tmp->ipmd_dm_bias8 =
		s->ipmd_dm_cost_bias[2][4] << 24 |
		s->ipmd_dm_cost_bias[2][3] << 18 |
		s->ipmd_dm_cost_bias[2][2] << 12 |
		s->ipmd_dm_cost_bias[2][1] << 6 |
		s->ipmd_dm_cost_bias[2][0];

	tmp->intra_bits0 = ( //8*4bits0
			(s->intra_bits[1][2] & 0xf) << 28 |
			(s->intra_bits[1][1] & 0xf) << 24 |
			(s->intra_bits[1][0] & 0xf) << 20 |
			(s->intra_bits[0][4] & 0xf) << 16 |
			(s->intra_bits[0][3] & 0xf) << 12 |
			(s->intra_bits[0][2] & 0xf) << 8 |
			(s->intra_bits[0][1] & 0xf) << 4 |
			(s->intra_bits[0][0] & 0xf) );
	tmp->intra_bits1 = ( //8*4bits
			(s->intra_bits[3][0] & 0xf) << 28 |
			(s->intra_bits[2][4] & 0xf) << 24 |
			(s->intra_bits[2][3] & 0xf) << 20 |
			(s->intra_bits[2][2] & 0xf) << 16 |
			(s->intra_bits[2][1] & 0xf) << 12 |
			(s->intra_bits[2][0] & 0xf) << 8 |
			(s->intra_bits[1][4] & 0xf) << 4 |
			(s->intra_bits[1][3] & 0xf) );
	tmp->intra_bits2 = ( //8*4bits
			(s->intra_bits[4][3] & 0xf) << 28 |
			(s->intra_bits[4][2] & 0xf) << 24 |
			(s->intra_bits[4][1] & 0xf) << 20 |
			(s->intra_bits[4][0] & 0xf) << 16 |
			(s->intra_bits[3][4] & 0xf) << 12 |
			(s->intra_bits[3][3] & 0xf) << 8 |
			(s->intra_bits[3][2] & 0xf) << 4 |
			(s->intra_bits[3][1] & 0xf) );
	tmp->intra_bits3 = ( //8*4bits
			(s->intra_bits[6][1] & 0xf) << 28 |
			(s->intra_bits[6][0] & 0xf) << 24 |
			(s->intra_bits[5][4] & 0xf) << 20 |
			(s->intra_bits[5][3] & 0xf) << 16 |
			(s->intra_bits[5][2] & 0xf) << 12 |
			(s->intra_bits[5][1] & 0xf) << 8 |
			(s->intra_bits[5][0] & 0xf) << 4 |
			(s->intra_bits[4][4] & 0xf) );
	tmp->intra_bits4 = ( //8*4bits
			(s->intra_bits[7][4] & 0xf) << 28 |
			(s->intra_bits[7][3] & 0xf) << 24 |
			(s->intra_bits[7][2] & 0xf) << 20 |
			(s->intra_bits[7][1] & 0xf) << 16 |
			(s->intra_bits[7][0] & 0xf) << 12 |
			(s->intra_bits[6][4] & 0xf) << 8 |
			(s->intra_bits[6][3] & 0xf) << 4 |
			(s->intra_bits[6][2] & 0xf) );
	tmp->intra_bits5 = ( //8*4bits
			(s->intra_bits[9][2] & 0xf) << 28 |
			(s->intra_bits[9][1] & 0xf) << 24 |
			(s->intra_bits[9][0] & 0xf) << 20 |
			(s->intra_bits[8][4] & 0xf) << 16 |
			(s->intra_bits[8][3] & 0xf) << 12 |
			(s->intra_bits[8][2] & 0xf) << 8 |
			(s->intra_bits[8][1] & 0xf) << 4 |
			(s->intra_bits[8][0] & 0xf) );
	tmp->intra_bits6 = ( //8*4bits
			(s->intra_bits[11][0] & 0xf) << 28 |
			(s->intra_bits[10][4] & 0xf) << 24 |
			(s->intra_bits[10][3] & 0xf) << 20 |
			(s->intra_bits[10][2] & 0xf) << 16 |
			(s->intra_bits[10][1] & 0xf) << 12 |
			(s->intra_bits[10][0] & 0xf) << 8 |
			(s->intra_bits[9][4] & 0xf) << 4 |
			(s->intra_bits[9][3] & 0xf) );
	tmp->intra_bits7 = ( //8*4bits
			(s->intra_bits[12][3] & 0xf) << 28 |
			(s->intra_bits[12][2] & 0xf) << 24 |
			(s->intra_bits[12][1] & 0xf) << 20 |
			(s->intra_bits[12][0] & 0xf) << 16 |
			(s->intra_bits[11][4] & 0xf) << 12 |
			(s->intra_bits[11][3] & 0xf) << 8 |
			(s->intra_bits[11][2] & 0xf) << 4 |
			(s->intra_bits[11][1] & 0xf) );
	tmp->intra_bits8 = ( //8*4bits
			(s->intra_bits[14][1] & 0xf) << 28 |
			(s->intra_bits[14][0] & 0xf) << 24 |
			(s->intra_bits[13][4] & 0xf) << 20 |
			(s->intra_bits[13][3] & 0xf) << 16 |
			(s->intra_bits[13][2] & 0xf) << 12 |
			(s->intra_bits[13][1] & 0xf) << 8 |
			(s->intra_bits[13][0] & 0xf) << 4 |
			(s->intra_bits[12][4] & 0xf) );
	tmp->intra_bits9 = ( //8*4bits
			(s->intra_bits[15][4] & 0xf) << 28 |
			(s->intra_bits[15][3] & 0xf) << 24 |
			(s->intra_bits[15][2] & 0xf) << 20 |
			(s->intra_bits[15][1] & 0xf) << 16 |
			(s->intra_bits[15][0] & 0xf) << 12 |
			(s->intra_bits[14][4] & 0xf) << 8 |
			(s->intra_bits[14][3] & 0xf) << 4 |
			(s->intra_bits[14][2] & 0xf) );


	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_MPM_BIASS32 , 0, tmp->ipmd_mpm_cost_biass32);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_MPM_BIASW32 , 0, tmp->ipmd_mpm_cost_biasw32);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_MPM_BIASS16 , 0, tmp->ipmd_mpm_cost_biass16);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_MPM_BIASW16 , 0, tmp->ipmd_mpm_cost_biasw16);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_MPM_BIASS8 , 0, tmp->ipmd_mpm_cost_biass8);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_MPM_BIASW8 , 0, tmp->ipmd_mpm_cost_biasw8);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_ANGLE_BIAS , 0, tmp->ipmd_angle_bias);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_DM_BIAS32 , 0, tmp->ipmd_dm_bias32);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_DM_BIAS16 , 0, tmp->ipmd_dm_bias16);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_IPMD_DM_BIAS8 , 0, tmp->ipmd_dm_bias8);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS0 , 0, tmp->intra_bits0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS1 , 0, tmp->intra_bits1);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS2 , 0, tmp->intra_bits2);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS3 , 0, tmp->intra_bits3);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS4 , 0, tmp->intra_bits4);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS5 , 0, tmp->intra_bits5);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS6 , 0, tmp->intra_bits6);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS7 , 0, tmp->intra_bits7);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS8 , 0, tmp->intra_bits8);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BITS9 , 0, tmp->intra_bits9);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BC_ST , 0, all_init_state[13]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_BC_INIT , 0, 1);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_FRM_SIZE , 0, ((s->frame_width-1) |
				((s->frame_height-1)<<16)));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_SLV_MODE , 0, ( s->ppred_slv_en << 31 |
				s->ipred_slv_en << 30 |
				s->intra_mode[4] << 24 |
				s->intra_mode[3] << 18 |
				s->intra_mode[2] << 12 |
				s->intra_mode[1] << 6 |
				s->intra_mode[0]  ));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MOD_CTRL , 0,(1 << 15 |//ip32_n
				1 << 12 |//ip16_n
				1 << 9 |//ip8_n
				s->ppred32_mode_num << 6 |//pp32_n
				s->ppred16_mode_num << 3 |//pp16_n
				s->ppred8_mode_num << 0 ));//pp8_n
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MODE0 , 0,( s->ppred8_slv_en << 31 |
				0 << 30 |//ip8_slv_en
				s->intra8_mode[4] << 24 |
				s->intra8_mode[3] << 18 |
				s->intra8_mode[2] << 12 |
				s->intra8_mode[1] << 6 |
				s->intra8_mode[0]  ));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MODE1 , 0,( s->ppred16_slv_en << 31 |
				0 << 30 |//ip16_slv_en
				s->intra16_mode[4] << 24 |
				s->intra16_mode[3] << 18 |
				s->intra16_mode[2] << 12 |
				s->intra16_mode[1] << 6 |
				s->intra16_mode[0]  ));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_MODE2 , 0,( s->ppred32_slv_en << 31 |
				0 << 30 |//ip32_slv_en
				s->intra32_mode[4] << 24 |
				s->intra32_mode[3] << 18 |
				s->intra32_mode[2] << 12 |
				s->intra32_mode[1] << 6 |
				s->intra32_mode[0]  ));

	s->md_bc_close = 1;  // always close md-rd-ip-bc in T32V
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_IPRED_BASE_VDMA + RADIX_REG_IPRED_CTRL , 0, (
				s->video_type << 30 |// 0:hevc, 1:svac2
				0 << 29 |//ip_dif
				s->ppred_slv_dif << 28 |//pp_dif
				((!tmp->inter_cu8) | s->md_bc_close) << 27 |//md_bc_close//
				(0x1f) <<22 |//ipred_crc
				(s->frame_type==2) << 21 |//slice_i
				(s->ipred_size & 0x7) <<18 |
				(s->sobel_size & 0x7) <<15 |
				0 <<14 |//plane_type
				s->ipred_usis <<13 |//usis
				3 <<11 |//lcu_size
				(s->ppred_mode_num & 0x7) <<8 |
				(1 & 0x7) <<5 |//ipred_n
				s->sobel_en <<4 |
				0 <<3|//decode
				0 <<2 |//ste
				0 <<1 |//cge
				1 ));//slice_init
	/*************************************************************************************
	  TFM Module
	 *************************************************************************************/
	tmp->tfm_cfg_grp[0]
		= 0
		| (0 & 0x1) << 7 		/* TODO: i4_enable */
		| (s->acmask_type & 0x1) << 6
		| (s->acmask_en & 0x1) << 5
		| (s->rdoq_en & 0x1) << 4
		| (s->deadzone_en & 0x1) << 3
		| (s->do_not_clear_deadzone & 0x1) << 2
		| (s->scaling_list_en & 0x1) << 1
		| (s->scaling_list_present & 0x1) << 0;

	for (i = 0; i < 4; ++i)
		for (j = 0; j < 3; ++j) {
			tmp->tfm_cfg_grp[i * 6 + j + 1]
				= (s->tfm_grp[i].rdoq_level_y[j] & 0xF) << 0
				| (s->tfm_grp[i].deadzone_y_en[j] & 0x1) << 4
				| (s->tfm_grp[i].deadzone_y[j] & 0x1FFF) << 5
				| (s->tfm_grp[i].acmask_y[j] & 0x3FFF) << 18;

			tmp->tfm_cfg_grp[i * 6 + j + 3 + 1]
				= (s->tfm_grp[i].rdoq_level_c[j] & 0xF) << 0
				| (s->tfm_grp[i].deadzone_c_en[j] & 0x1) << 4
				| (s->tfm_grp[i].deadzone_c[j] & 0x1FFF) << 5
				| (s->tfm_grp[i].acmask_c[j] & 0x3FFF) << 18;
		}

	for (i = 0; i < 8; ++i) {
		tmp->tfm_cfg_grp[25 + i * 2]
			= (s->tfm_rdoq_param[i].clear1 & 0x1) << 0
			| (s->tfm_rdoq_param[i].val0[0] & 0x3) << 1
			| (s->tfm_rdoq_param[i].val1[0] & 0x3) << 3
			| (s->tfm_rdoq_param[i].score[0] & 0x1F) << 5
			| (s->tfm_rdoq_param[i].val0[1] & 0x3) << 10
			| (s->tfm_rdoq_param[i].val1[1] & 0x3) << 12
			| (s->tfm_rdoq_param[i].score[1] & 0x1F) << 14
			| (s->tfm_rdoq_param[i].val0[2] & 0x3) << 19
			| (s->tfm_rdoq_param[i].val1[2] & 0x3) << 21
			| (s->tfm_rdoq_param[i].score[2] & 0x1F) << 23
			| (s->tfm_rdoq_param[i].val0[3] & 0x3) << 28
			| (s->tfm_rdoq_param[i].val1[3] & 0x3) << 30;

		tmp->tfm_cfg_grp[25 + i * 2 + 1]
			= (s->tfm_rdoq_param[i].score[3] & 0x1F) << 0
			| (s->tfm_rdoq_param[i].tu8_last[0] & 0x3) << 5
			| (s->tfm_rdoq_param[i].tu16_last[0] & 0x7) << 7
			| (s->tfm_rdoq_param[i].tu32_last[0] & 0xF) << 10
			| (s->tfm_rdoq_param[i].tu8_last[1] & 0x3) << 14
			| (s->tfm_rdoq_param[i].tu16_last[1] & 0x7) << 16
			| (s->tfm_rdoq_param[i].tu32_last[1] & 0xF) << 19
			| (s->tfm_rdoq_param[i].tu8_last[2] & 0x3) << 23
			| (s->tfm_rdoq_param[i].tu16_last[2] & 0x7) << 25
			| (s->tfm_rdoq_param[i].tu32_last[2] & 0xF) << 28;
	}

	tmp->tfm_cfg_grp[41]
		= (s->tfm_rdoq_param[1 - 1].delta[0] & 0x3) << 0
		| (s->tfm_rdoq_param[1 - 1].delta[1] & 0x3) << 2
		| (s->tfm_rdoq_param[1 - 1].delta[2] & 0x3) << 4
		| (s->tfm_rdoq_param[1 - 1].delta[3] & 0x3) << 6
		| (s->tfm_rdoq_param[2 - 1].delta[0] & 0x3) << 8
		| (s->tfm_rdoq_param[2 - 1].delta[1] & 0x3) << 10
		| (s->tfm_rdoq_param[2 - 1].delta[2] & 0x3) << 12
		| (s->tfm_rdoq_param[2 - 1].delta[3] & 0x3) << 14
		| (s->tfm_rdoq_param[3 - 1].delta[0] & 0x3) << 16
		| (s->tfm_rdoq_param[3 - 1].delta[1] & 0x3) << 18
		| (s->tfm_rdoq_param[3 - 1].delta[2] & 0x3) << 20
		| (s->tfm_rdoq_param[3 - 1].delta[3] & 0x3) << 22
		| (s->tfm_rdoq_param[4 - 1].delta[0] & 0x3) << 24
		| (s->tfm_rdoq_param[4 - 1].delta[1] & 0x3) << 26
		| (s->tfm_rdoq_param[4 - 1].delta[2] & 0x3) << 28
		| (s->tfm_rdoq_param[4 - 1].delta[3] & 0x3) << 30;

	tmp->tfm_cfg_grp[42]
		= (s->tfm_rdoq_param[5 - 1].delta[0] & 0x3) << 0
		| (s->tfm_rdoq_param[5 - 1].delta[1] & 0x3) << 2
		| (s->tfm_rdoq_param[5 - 1].delta[2] & 0x3) << 4
		| (s->tfm_rdoq_param[5 - 1].delta[3] & 0x3) << 6
		| (s->tfm_rdoq_param[6 - 1].delta[0] & 0x3) << 8
		| (s->tfm_rdoq_param[6 - 1].delta[1] & 0x3) << 10
		| (s->tfm_rdoq_param[6 - 1].delta[2] & 0x3) << 12
		| (s->tfm_rdoq_param[6 - 1].delta[3] & 0x3) << 14
		| (s->tfm_rdoq_param[7 - 1].delta[0] & 0x3) << 16
		| (s->tfm_rdoq_param[7 - 1].delta[1] & 0x3) << 18
		| (s->tfm_rdoq_param[7 - 1].delta[2] & 0x3) << 20
		| (s->tfm_rdoq_param[7 - 1].delta[3] & 0x3) << 22;
	/*     | (s->tfm_rdoq_param[8 - 1].delta[0] & 0x3) << 24 */
	/*     | (s->tfm_rdoq_param[8 - 1].delta[1] & 0x3) << 26 */
	/*     | (s->tfm_rdoq_param[8 - 1].delta[2] & 0x3) << 28 */
	/*     | (s->tfm_rdoq_param[8 - 1].delta[3] & 0x3) << 30; */

	/*   tmp->tfm_cfg_grp[117] */
	/*     = (s->tfm_rdoq_param[9 - 1].delta[0] & 0x3) << 0 */
	/*     | (s->tfm_rdoq_param[9 - 1].delta[1] & 0x3) << 2 */
	/*     | (s->tfm_rdoq_param[9 - 1].delta[2] & 0x3) << 4 */
	/*     | (s->tfm_rdoq_param[9 - 1].delta[3] & 0x3) << 6; */

	for (i = 0; i < 43; ++i)
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_TFM_BASE_VDMA + RADIX_TFM_FRM_CTL + i * 4, 0, tmp->tfm_cfg_grp[i]);

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_TFM_BASE_VDMA + RADIX_TFM_REG_SCTRL , 0, (s->sse_mask <<31 |
				0x0 <<28 |//opn_msk
				0x1 <<27 |//zeroblk_en
				0x1 <<26 |//tfm_en
				(s->tfm_buf_en & 0x1f) <<21 |//buf_en
				(0 & 0x3) <<19 |//pro
				(0 & 0x7) <<16 |//pri
				(s->frame_type == 2) <<15 |//i_slice
				(0 & 0x7) <<12 |//bitd_c8
				(0 & 0x7) <<9 |//bitd_y8
				(3 & 0x3) <<7 |//lcu_size
				(0 & 0x3) <<5 |//plantyp
				(0 & 0x1) <<4 |//uselist
				(0 & 0x1) <<3 |//ste
				(0 & 0x1) <<2 |//cge
				(1 & 0x1) <<1 ));//is_enc
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_TFM_BASE_VDMA + RADIX_TFM_FRM_SIZE , 0, (s->frame_width |
				(s->frame_height << 16)));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_TFM_BASE_VDMA + RADIX_TFM_REG_SINIT , 0, ((s->frame_cqp_offset & 0x7F) <<24 |
				(s->frame_cqp_offset & 0x7F) <<17 |
				(s->tfm_path_en[1] & 0xF) <<13 |
				(s->tfm_path_en[0] & 0x3FF) <<3 |
				((1 == s->video_type) & 0x1) <<2 |
				((0 == s->video_type) & 0x1) <<1 |
				(1 & 0x1) <<0 ));
	/*************************************************************************************
	  MD Module
	 *************************************************************************************/
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG0 , 0, (((s->frame_type & 0x3) << 0) +
				((tmp->inter_enable & 0xF) << 4) +
				((tmp->intra_enable & 0xF) << 8) +
				((1 & 0x1) << 12) +//uv_enable
				((s->md_cfg_en & 0x1) << 13) +// efe cu cfg enable
				((s->md_cfg_en & 0x1) << 14) +// efe cu cfg bias enable
				((tmp->cu_enable & 0xF) << 16) +
				((s->tu_split_en & 0x1) << 20) +
				((0 & 0x1) << 21) +//bc_disable
				(((!!(s->frame_width%16)) & 0x1) << 22) +
				(((!!(s->frame_height%16)) & 0x1) << 23) +
				(((s->frame_width <= 64*6) & 0x1) << 24) +
				((0 & 0x1) << 26) +//crc_sel
				((1 & 0x1) << 27) +//crc_cken
				((0 & 0x1) << 28) +//ckg_enable
				(((s->frame_width <= 64*6) & 0x1) << 29) +//efe_step
				(((!!s->sao_en) & 0x1) << 30) +
				((s->video_type) << 31) + // 0:hevc, 1:svac2
				0));
	tmp->lcu_width = (s->frame_width + 63) >> 6;
	tmp->lcu_height = (s->frame_height + 63) >> 6;
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG1 , 0, (((0 & 0xFF) << 0) +                                // frm left
				(((tmp->lcu_width - 1) & 0xFF) << 8) +       // frm right
				((0 & 0xFF) << 16) +                               // frm top
				(((tmp->lcu_height - 1) & 0xFF) << 24) +     // frm bottom
				0));
	tmp->pre_pmd_en   = tmp->inter_cu8 & tmp->intra_cu8 & (!(s->frame_type == 2)) & !s->md_bc_close;
	tmp->pre_pmd_msk0 = s->md_pre_pmd[0];
	tmp->pre_pmd_msk1 = s->md_pre_pmd[1];
	tmp->pre_csd_en_cu8   = tmp->cu8_enable  & (!(s->frame_type == 2));
	tmp->pre_csd_en_cu16  = tmp->cu16_enable & (!(s->frame_type == 2));
	tmp->pre_csd_sel_cu8  = s->md_pre_csd[0]; // 0:3sse, 1:4sa8d, 2:auto
	tmp->pre_csd_sel_cu16 = s->md_pre_csd[1]; // 0:3sse, 1:4sa8d, 2:auto
	tmp->pmd_bias_en_all = ((s->md_pmd_bias[2][0] & 0x1) << 2) +
		((s->md_pmd_bias[1][0] & 0x1) << 1) + ((s->md_pmd_bias[0][0] & 0x1) << 0);
	tmp->pmd_bias_cu8    = s->md_pmd_bias[0][1];
	tmp->pmd_bias_cu16   = s->md_pmd_bias[1][1];
	tmp->pmd_bias_cu32   = s->md_pmd_bias[2][1];
	tmp->md_cfg2 = (((tmp->pre_pmd_en & 0x1) << 0) +        //
			((tmp->pre_pmd_msk0 & 0x3) << 1) +      //
			((tmp->pre_pmd_msk1 & 0x3) << 3) +      //
			((tmp->pre_csd_en_cu8 & 0x1) << 8) +
			((tmp->pre_csd_sel_cu8 & 0x3) << 9) +
			((tmp->pmd_bias_en_all & 0x7) << 12) +
			((tmp->pmd_bias_cu8 & 0xF) << 16) +
			((tmp->pmd_bias_cu16 & 0xF) << 20) +
			((tmp->pmd_bias_cu32 & 0xF) << 24) +
			((tmp->pre_csd_en_cu16 & 0x1) << 28) +
			((tmp->pre_csd_sel_cu16 & 0x3) << 29) +
			0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG2 , 0, tmp->md_cfg2);
	tmp->slice_qp = s->frame_qp;
	tmp->dqp_enable = 1;
	tmp->dqp_force_split = s->md_force_split;
	tmp->dqp_depth_cu16 = s->dqp_max_depth == 2;
	tmp->csd_bias_en_all = ((s->md_csd_bias[3][0] & 0x1) << 3) +((s->md_csd_bias[2][0] & 0x1) << 2) +
		((s->md_csd_bias[1][0] & 0x1) << 1) + ((s->md_csd_bias[0][0] & 0x1) << 0);
	tmp->csd_bias_cu8    = s->md_csd_bias[0][1];
	tmp->csd_bias_cu16   = s->md_csd_bias[1][1];
	tmp->csd_bias_cu32   = s->md_csd_bias[2][1];
	tmp->csd_bias_cu64   = s->md_csd_bias[3][1];
	tmp->md_cfg3 = (((tmp->slice_qp & 0x3F) << 0) +           // slice qp
			((tmp->dqp_enable & 0x1)  << 6) +         // qp diff enable
			((tmp->dqp_force_split & 0x1)  << 7) +    // value 1: force split if qp not the same
			((tmp->dqp_depth_cu16 & 0x1)  << 8) +     // 0:dqp cu32,  1:dqp cu16
			((tmp->csd_bias_en_all & 0xF) << 12) +
			((tmp->csd_bias_cu8 & 0xF) << 16) +
			((tmp->csd_bias_cu16 & 0xF) << 20) +
			((tmp->csd_bias_cu32 & 0xF) << 24) +
			((tmp->csd_bias_cu64 & 0xF) << 28) +
			0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG3 , 0, tmp->md_cfg3);
	tmp->lambda_bias_en_all = ((s->md_lambda_bias[2][0] & 0x1) << 2) +
		((s->md_lambda_bias[1][0] & 0x1) << 1) + ((s->md_lambda_bias[0][0] & 0x1) << 0);
	tmp->lambda_bias_sa8d   = s->md_lambda_bias[0][1];
	tmp->lambda_bias_sse    = s->md_lambda_bias[1][1];
	tmp->lambda_bias_stc    = s->md_lambda_bias[2][1];
	tmp->enc_info_en   = 0xF;  // [0]:cu8, [1]:cu16, [2]:cu32, [3]:cu64
	tmp->enc_mv_all_en = s->md_mvs_all;
	tmp->enc_mv_abs_en = s->md_mvs_abs;
	tmp->md_cfg4 = (((tmp->lambda_bias_en_all & 0x7) << 0) +
			((tmp->lambda_bias_sa8d & 0xF) << 4) +
			((tmp->lambda_bias_sse & 0xF) << 8) +
			((tmp->lambda_bias_stc & 0xF) << 12) +
			((tmp->enc_info_en & 0xF) << 16) +
			((tmp->enc_mv_all_en & 0xF) << 20) +
			((tmp->enc_mv_abs_en & 0xF) << 24) +
			((!!s->emc_mvo_en) << 28) + // write out curr-mv
			0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG4 , 0, tmp->md_cfg4);
	/* in T32V, force skip is used to limite P frame max size
	 * if sde_bslen exceed max_bs_size in Bytes, only select cu64 skip
	 */
	tmp->md_fskip_en  = s->md_fskip_en & (!(s->frame_type == 2));
	tmp->md_fskip_max_size  = (1 << 20); // 1M Byte
	tmp->md_cfg5 = ((((s->video_type) & 0x1) << 0) +  // use_sse_mode
			((tmp->md_fskip_en & 0x1) << 4) +
			((tmp->md_fskip_max_size  & ((1<<24)-1)) << 8) +
			0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG5 , 0, tmp->md_cfg5);
	tmp->md_cfg6 = (
			((s->inter_sa8d_en & 0x1) << 27) + //27: inter_sa8d_en
			((0x10 & 0x1F) << 22) +   // 26:22  mode selable when no-mode-selable
			((s->tus_ifa_intra32_en & 0x1) << 21) +   // 21  tu_split, intra cu32 ifa en
			((s->tus_ifa_intra16_en & 0x1) << 20) +   // 20  tu_split, intra cu16 ifa en
			((s->tus_mce_inter32_en & 0x1) << 19) +   // 19  tu_split, inter cu32 mce en
			((s->tus_mce_inter16_en & 0x1) << 18) +   // 18  tu_split, inter cu16 mce en
			((s->tus_ifa_inter32_en & 0x1) << 17) +   // 17  tu_split, inter cu32 ifa en
			((s->tus_ifa_inter16_en & 0x1) << 16) +   // 16  tu_split, inter cu16 ifa en
			((s->uv_dist_scale[1] & 0x7) << 12) +   // 14:12  shift
			((s->uv_dist_scale[0] & 0x3) << 10) +   // 11:10  bias
			((s->uv_dist_scale[2] & 0x1) << 9) +    // 9      direction
			((s->uv_dist_scale_en & 0x1) << 8) +    // 8      enable
			((s->frame_cqp_offset & 0x1F) << 0) +   // chroma qp offset, -16 ~ 15
			0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG6 , 0, tmp->md_cfg6);
	tmp->md_cfg7 = (
			(((s->pmdsu_thrd[2]) & 0xF) << 28) +  // 31:28 pmd speed up threshold cu32
			(((s->pmdsu_thrd[1]) & 0xF) << 24) +  // 27:24 pmd speed up threshold cu16
			(((s->pmdsu_thrd[0]) & 0xF) << 20) +  // 23:20 pmd speed up threshold cu8.  (sa8d < 2^N)
			(((s->pmdsu_en) & 0x7) << 16) +       // 18:16 pmd speed up enable for cu32/cu16/cu8
			(((s->pmdctrl_en >> 1) & 0x7) << 12) +  // 14:12 pmd ctrl enable for cu32/cu16/cu8
			((s->blk_pmdctrl_en & 0x1) << 10) +  // 10 block level pred mode decision control enable. value 0: use default sse-cost. value 1:use sa8d/sad at some mode
			((s->pmdctrl_mode & 0x1) << 9) +     // 9  pred mode decision control mode. 0: merge/search sa8d/sad, inter/intra sse. vaule 1: merge/search/intra sa8d/sad.
			((s->pmdctrl_en & 0x1) << 8) +       // 8  frame level pred mode decision control enable. value 0: use default sse-cost. value 1:use sa8d/sad at some mode
			((s->rfh_sel_intra32_en & 0x1) << 7) +   // 7  refresh flag select intra, cu32
			((s->rfh_sel_intra16_en & 0x1) << 6) +   // 6  refresh flag select intra, cu16
			((s->rfh_msk_skip32_en & 0x1) << 5) +   // 5  refresh flag mask skip, cu32
			((s->rfh_msk_skip16_en & 0x1) << 4) +   // 4  refresh flag mask skip, cu16
			((s->mf_sel_sech_skip32_en & 0x1) << 1) +   // 1  move flag select search skip, cu32
			((s->mf_sel_sech_skip16_en & 0x1) << 0) +   // 0  move flag select search skip, cu16
			0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_CFG7 , 0, tmp->md_cfg7);
	//
	for(j=0; j<8; j++) {
		for(i=0; i<16; i++) tmp->md_scfg[j>>1] += s->md_fskip_val[j][i] << ((j & 1) ? (i + 16) : i);
	}
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_SCFG0 , 0, tmp->md_scfg[0]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_SCFG1 , 0, tmp->md_scfg[1]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_SCFG2 , 0, tmp->md_scfg[2]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_SCFG3 , 0, tmp->md_scfg[3]);
	/*
	 * pmd-cfg-grp haddr[10:8] == 'b100
	 * lambda-cfg-grp haddr[10:8] == 'b101
	 */
	tmp->md_pmd_cfg_ofst = tmp->RADIX_MD_BASE_VDMA + (1<<10);
	for (i=0; i<16; i++) {
		RADIX_GEN_VDMA_ACFG(chn,(tmp->md_pmd_cfg_ofst + ((i*2+0)<<2)), 0, s->md_cfg_grp[i][0]);
		RADIX_GEN_VDMA_ACFG(chn,(tmp->md_pmd_cfg_ofst + ((i*2+1)<<2)), 0, s->md_cfg_grp[i][1]);
	}
	// lambda configure
	tmp->md_lambda_cfg_ofst = tmp->RADIX_MD_BASE_VDMA + (1<<10) + (0x2<<7);
	for (i=0; i<11; i++) {
		RADIX_GEN_VDMA_ACFG(chn,(tmp->md_lambda_cfg_ofst + (i<<2)), 0, s->lambda_cfg[i]);
	}
	// rate control register and ram cfg
	tmp->md_rcfg[0] = (((s->rc_en     & 0x1)  << 0) +
			(((s->bu_size - 1) & 0xFFFF) << 4) +
			((0            & 0x1)  << 20) +
			((0            & 0x1)  << 21) +
			((1            & 0x1)  << 22) +
			((s->bu_len    & 0xFF) << 23) +
			((s->rc_dly_en & 0x1)  << 31));

	tmp->md_rcfg[1] = (((s->bu_max_dqp  & 0x1F) << 0)  +
			((s->bu_min_dqp  & 0x1F) << 5)  +
			((s->rc_avg_len  & 0x1F) << 10) +
			((s->rc_max_prop & 0x7)  << 15) +
			((s->rc_min_prop & 0x7)  << 18) +
			((s->rc_method   & 0x7)  << 21));

	tmp->md_rcfg[2] = (((s->rc_thd[0]  & 0xFFFF) << 0) +
			((s->rc_thd[1]  & 0xFFFF) << 16));
	tmp->md_rcfg[3] = (((s->rc_thd[2]  & 0xFFFF) << 0) +
			((s->rc_thd[3]  & 0xFFFF) << 16));
	tmp->md_rcfg[4] = (((s->rc_thd[4]  & 0xFFFF) << 0) +
			((s->rc_thd[5]  & 0xFFFF) << 16));
	tmp->md_rcfg[5] = (((s->rc_thd[6]  & 0xFFFF) << 0) +
			((s->rc_thd[7]  & 0xFFFF) << 16));
	tmp->md_rcfg[6] = (((s->rc_thd[8]  & 0xFFFF) << 0) +
			((s->rc_thd[9]  & 0xFFFF) << 16));
	tmp->md_rcfg[7] = (((s->rc_thd[10] & 0xFFFF) << 0) +
			((s->rc_thd[11] & 0xFFFF) << 16));

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG0 , 0, tmp->md_rcfg[0]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG1 , 0, tmp->md_rcfg[1]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG2 , 0, tmp->md_rcfg[2]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG3 , 0, tmp->md_rcfg[3]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG4 , 0, tmp->md_rcfg[4]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG5 , 0, tmp->md_rcfg[5]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG6 , 0, tmp->md_rcfg[6]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_RCFG7 , 0, tmp->md_rcfg[7]);

	for (i=0; i<s->bu_len; i++) {
		//for (i=0; i<63; i++) {
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + (((1 << 9) + i) << 2), 0, s->rc_info[i]);
	}// 0x800

	tmp->md_pmon_cfg_ofst = 0xc0 << 2;
	tmp->pmon_mode = 0; // 0:mpt_empty;  1:ipt_rdy&mpt_empty;  2:ipt_empty
	tmp->md_pmon_cfg0 = (tmp->pmon_mode<<0) + (tmp->pmon_mode<<2) + (tmp->pmon_mode<<4); // cu8,cu16,cu32
	/* cfg: | 12bit | 10bit | 10bit | */
	tmp->md_pmon_cfg1 = (40<<0) + (100<<10) + (200<<20); // cu8 mce pmon thrd
	tmp->md_pmon_cfg2 = (130<<0) + (300<<10) + (700<<20); // cu16 mce pmon thrd
	tmp->md_pmon_cfg3 = (400<<0) + (1000<<10) + (2500<<20); // cu32 mce pmon thrd
	tmp->md_pmon_cfg4 = (50<<0) + (60<<10) + (70<<20); // cu8 tfm pmon thrd
	tmp->md_pmon_cfg5 = (320<<0) + (360<<10) + (400<<20); // cu16 tfm pmon thrd
	tmp->md_pmon_cfg6 = (((1300>>1)&0x3FF)<<0) + (((1500>>1)&0x3FF)<<10) + (1700<<20); // cu32 tfm pmon thrd
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + tmp->md_pmon_cfg_ofst + 0*4, 0, tmp->md_pmon_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + tmp->md_pmon_cfg_ofst + 1*4, 0, tmp->md_pmon_cfg1);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + tmp->md_pmon_cfg_ofst + 2*4, 0, tmp->md_pmon_cfg2);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + tmp->md_pmon_cfg_ofst + 3*4, 0, tmp->md_pmon_cfg3);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + tmp->md_pmon_cfg_ofst + 4*4, 0, tmp->md_pmon_cfg4);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + tmp->md_pmon_cfg_ofst + 5*4, 0, tmp->md_pmon_cfg5);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + tmp->md_pmon_cfg_ofst + 6*4, 0, tmp->md_pmon_cfg6);

	tmp->ppmd_cost_thr = (s->p_pmd_en<<21 |
			s->i_csd_en<<20 |
			s->p_csd_en<<19 |
			s->csd_p_cu64_en<<18 |
			s->csd_color<<17 |
			s->var_flat_sub_size<<16) |
		(s->ppmd_cost_thr[3] << 12) |
		(s->ppmd_cost_thr[2] << 8) |
		(s->ppmd_cost_thr[0] << 4 ) |
		(s->ppmd_cost_thr[1]);
	tmp->ppmd_qp_thrd_0 = (s->pmd_qp_thrd[4] << 24) |
		(s->pmd_qp_thrd[3] << 18) |
		(s->pmd_qp_thrd[2] << 12) |
		(s->pmd_qp_thrd[1] << 6) |
		(s->pmd_qp_thrd[0]);
	tmp->ppmd_qp_thrd_1 = (s->pmd_qp_thrd[6] << 6) |
		(s->pmd_qp_thrd[5]);
	//pmd_cost_thr
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_THR_CLR_OFST,  0, tmp->ppmd_cost_thr);
	//pmd_qp_thrd
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_QP_THRD_0, 0, tmp->ppmd_qp_thrd_0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_QP_THRD_1, 0, tmp->ppmd_qp_thrd_1);

	//pmd_thr_array
	tmp->pmd_thr0  = (s->ppmd_thr_list[0][0]<<7 | s->ppmd_thr_list[0][1]) & 0x3fff; //{55, 36}
	tmp->pmd_thr1	 = (s->ppmd_thr_list[1][0]<<7 | s->ppmd_thr_list[1][1]) & 0x3fff; //{52, 30}
	tmp->pmd_thr2	 = (s->ppmd_thr_list[2][0]<<7 | s->ppmd_thr_list[2][1]) & 0x3fff; //{50, 30}
	tmp->pmd_thr3	 = (s->ppmd_thr_list[3][0]<<7 | s->ppmd_thr_list[3][1]) & 0x3fff; //{57, 38}
	tmp->pmd_thr4	 = (s->ppmd_thr_list[4][0]<<7 | s->ppmd_thr_list[4][1]) & 0x3fff; //{54, 32}
	tmp->pmd_thr5	 = (s->ppmd_thr_list[5][0]<<7 | s->ppmd_thr_list[5][1]) & 0x3fff; //{53, 31}
	tmp->pmd_thr6	 = (s->ppmd_thr_list[6][0]<<7 | s->ppmd_thr_list[6][1]) & 0x3fff; //{59, 41}
	tmp->pmd_thr7	 = (s->ppmd_thr_list[7][0]<<7 | s->ppmd_thr_list[7][1]) & 0x3fff; //{57, 33}
	tmp->pmd_thr8	 = (s->ppmd_thr_list[8][0]<<7 | s->ppmd_thr_list[8][1]) & 0x3fff; //{56, 32}
	tmp->pmd_thr9	 = (s->ppmd_thr_list[9][0]<<7 | s->ppmd_thr_list[9][1]) & 0x3fff; //{60, 41}
	tmp->pmd_thr10 = (s->ppmd_thr_list[10][0]<<7 | s->ppmd_thr_list[10][1]) & 0x3fff; //{62, 41}
	tmp->pmd_thr11 = (s->ppmd_thr_list[11][0]<<7 | s->ppmd_thr_list[11][1]) & 0x3fff; //{59, 35}
	tmp->pmd_thr12 = (s->ppmd_thr_list[12][0]<<7 | s->ppmd_thr_list[12][1]) & 0x3fff; //{59, 33}
	tmp->pmd_thr13 = (s->ppmd_thr_list[13][0]<<7 | s->ppmd_thr_list[13][1]) & 0x3fff; //{57, 45}
	tmp->pmd_thr14 = (s->ppmd_thr_list[14][0]<<7 | s->ppmd_thr_list[14][1]) & 0x3fff; //{57, 30}
	tmp->pmd_thr15 = (s->ppmd_thr_list[15][0]<<7 | s->ppmd_thr_list[15][1]) & 0x3fff; //{56, 30}
	tmp->pmd_thr16 = (s->ppmd_thr_list[16][0]<<7 | s->ppmd_thr_list[16][1]) & 0x3fff; //{59, 45}
	tmp->pmd_thr17 = (s->ppmd_thr_list[17][0]<<7 | s->ppmd_thr_list[17][1]) & 0x3fff; //{58, 32}
	tmp->pmd_thr18 = (s->ppmd_thr_list[18][0]<<7 | s->ppmd_thr_list[18][1]) & 0x3fff; //{57, 32}
	tmp->pmd_thr19 = (s->ppmd_thr_list[19][0]<<7 | s->ppmd_thr_list[19][1]) & 0x3fff; //{60, 45}
	tmp->pmd_thr20 = (s->ppmd_thr_list[20][0]<<7 | s->ppmd_thr_list[20][1]) & 0x3fff; //{62, 45}
	tmp->pmd_thr21 = (s->ppmd_thr_list[21][0]<<7 | s->ppmd_thr_list[21][1]) & 0x3fff; //{60, 35}
	tmp->pmd_thr22 = (s->ppmd_thr_list[22][0]<<7 | s->ppmd_thr_list[22][1]) & 0x3fff; //{76, 51}
	tmp->pmd_thr23 = (s->ppmd_thr_list[23][0]<<7 | s->ppmd_thr_list[23][1]) & 0x3fff;
	tmp->pmd_thr24 = (s->ppmd_thr_list[24][0]<<7 | s->ppmd_thr_list[24][1]) & 0x3fff;
	tmp->pmd_thr25 = (s->ppmd_thr_list[25][0]<<7 | s->ppmd_thr_list[25][1]) & 0x3fff;
	tmp->pmd_thr26 = (s->ppmd_thr_list[26][0]<<7 | s->ppmd_thr_list[26][1]) & 0x3fff;
	tmp->pmd_thr27 = (s->ppmd_thr_list[27][0]<<7 | s->ppmd_thr_list[27][1]) & 0x3fff;
	tmp->pmd_thr28 = (s->ppmd_thr_list[28][0]<<7 | s->ppmd_thr_list[28][1]) & 0x3fff;
	tmp->pmd_thr29 = (s->ppmd_thr_list[29][0]<<7 | s->ppmd_thr_list[29][1]) & 0x3fff;
	//search pmd_thr_idx
	tmp->cu8_pmd_s_thr_idx0 = s->ppmd_search_thr_idx[10];//9;
	tmp->cu8_pmd_s_thr_idx1 = s->ppmd_search_thr_idx[11];//9;
	tmp->cu8_pmd_s_thr_idx2 = s->ppmd_search_thr_idx[12];//10;
	tmp->cu8_pmd_s_thr_idx3 = s->ppmd_search_thr_idx[13];//11;
	tmp->cu8_pmd_s_thr_idx4 = s->ppmd_search_thr_idx[14];//12;
	tmp->cu8_pmd_s_thr_idx5 = s->ppmd_search_thr_idx[25];//20;
	tmp->pmd_thr_idx0 = (tmp->cu8_pmd_s_thr_idx5<<25 | tmp->cu8_pmd_s_thr_idx4<<20 |
			tmp->cu8_pmd_s_thr_idx3<<15 | tmp->cu8_pmd_s_thr_idx2<<10 |
			tmp->cu8_pmd_s_thr_idx1<<5 | tmp->cu8_pmd_s_thr_idx0);
	tmp->cu8_pmd_s_thr_idx6 = s->ppmd_search_thr_idx[26];//20;
	tmp->cu8_pmd_s_thr_idx7 = s->ppmd_search_thr_idx[27];//20;
	tmp->cu8_pmd_s_thr_idx8 = s->ppmd_search_thr_idx[28];//21;
	tmp->cu8_pmd_s_thr_idx9 = s->ppmd_search_thr_idx[29];//21;
	tmp->cu16_pmd_s_thr_idx10 = s->ppmd_search_thr_idx[5];//6;
	tmp->cu16_pmd_s_thr_idx11 = s->ppmd_search_thr_idx[6];//6;
	tmp->pmd_thr_idx1 = (tmp->cu16_pmd_s_thr_idx11<<25 | tmp->cu16_pmd_s_thr_idx10<<20 |
			tmp->cu8_pmd_s_thr_idx9<<15 | tmp->cu8_pmd_s_thr_idx8<<10 |
			tmp->cu8_pmd_s_thr_idx7<<5 | tmp->cu8_pmd_s_thr_idx6);
	tmp->cu16_pmd_s_thr_idx12 = s->ppmd_search_thr_idx[7];//6;
	tmp->cu16_pmd_s_thr_idx13 = s->ppmd_search_thr_idx[8];//7;
	tmp->cu16_pmd_s_thr_idx14 = s->ppmd_search_thr_idx[9];//8;
	tmp->cu16_pmd_s_thr_idx15 = s->ppmd_search_thr_idx[20];//19;
	tmp->cu16_pmd_s_thr_idx16 = s->ppmd_search_thr_idx[21];//19;
	tmp->cu16_pmd_s_thr_idx17 = s->ppmd_search_thr_idx[22];//19;
	tmp->pmd_thr_idx2 = (tmp->cu16_pmd_s_thr_idx17<<25 | tmp->cu16_pmd_s_thr_idx16<<20 |
			tmp->cu16_pmd_s_thr_idx15<<15 | tmp->cu16_pmd_s_thr_idx14<<10 |
			tmp->cu16_pmd_s_thr_idx13<<5 | tmp->cu16_pmd_s_thr_idx12);
	tmp->cu16_pmd_s_thr_idx18 = s->ppmd_search_thr_idx[23];//12;
	tmp->cu16_pmd_s_thr_idx19 = s->ppmd_search_thr_idx[24];//12;
	tmp->cu32_pmd_s_thr_idx20 = s->ppmd_search_thr_idx[0];//3;
	tmp->cu32_pmd_s_thr_idx21 = s->ppmd_search_thr_idx[1];//3;
	tmp->cu32_pmd_s_thr_idx22 = s->ppmd_search_thr_idx[2];//3;
	tmp->cu32_pmd_s_thr_idx23 = s->ppmd_search_thr_idx[3];//4;
	tmp->pmd_thr_idx3 = (tmp->cu32_pmd_s_thr_idx23<<25 | tmp->cu32_pmd_s_thr_idx22<<20 |
			tmp->cu32_pmd_s_thr_idx21<<15 | tmp->cu32_pmd_s_thr_idx20<<10 |
			tmp->cu16_pmd_s_thr_idx19<<5 | tmp->cu16_pmd_s_thr_idx18);
	tmp->cu32_pmd_s_thr_idx24 = s->ppmd_search_thr_idx[4];//5;
	tmp->cu32_pmd_s_thr_idx25 = s->ppmd_search_thr_idx[15];//16;
	tmp->cu32_pmd_s_thr_idx26 = s->ppmd_search_thr_idx[16];//16;
	tmp->cu32_pmd_s_thr_idx27 = s->ppmd_search_thr_idx[17];//16;
	tmp->cu32_pmd_s_thr_idx28 = s->ppmd_search_thr_idx[18];//17;
	tmp->cu32_pmd_s_thr_idx29 = s->ppmd_search_thr_idx[19];//18;
	tmp->pmd_thr_idx4 = (tmp->cu32_pmd_s_thr_idx29<<25 | tmp->cu32_pmd_s_thr_idx28<<20 |
			tmp->cu32_pmd_s_thr_idx27<<15 | tmp->cu32_pmd_s_thr_idx26<<10 |
			tmp->cu32_pmd_s_thr_idx25<<5 | tmp->cu32_pmd_s_thr_idx24);
	//mrege pmd_thr_idx
	tmp->cu8_pmd_m_thr_idx0 = s->ppmd_merge_thr_idx[15];//9;
	tmp->cu8_pmd_m_thr_idx1 = s->ppmd_merge_thr_idx[16];//9;
	tmp->cu8_pmd_m_thr_idx2 = s->ppmd_merge_thr_idx[17];//10;
	tmp->cu8_pmd_m_thr_idx3 = s->ppmd_merge_thr_idx[18];//11;
	tmp->cu8_pmd_m_thr_idx4 = s->ppmd_merge_thr_idx[19];//12;
	tmp->cu8_pmd_m_thr_idx5 = s->ppmd_merge_thr_idx[35];//20;
	tmp->pmd_thr_idx5 = (tmp->cu8_pmd_m_thr_idx5<<25 | tmp->cu8_pmd_m_thr_idx4<<20 |
			tmp->cu8_pmd_m_thr_idx3<<15 | tmp->cu8_pmd_m_thr_idx2<<10 |
			tmp->cu8_pmd_m_thr_idx1<<5 | tmp->cu8_pmd_m_thr_idx0);

	tmp->cu8_pmd_m_thr_idx6   = s->ppmd_merge_thr_idx[36];//20;
	tmp->cu8_pmd_m_thr_idx7   = s->ppmd_merge_thr_idx[37];//20;
	tmp->cu8_pmd_m_thr_idx8   = s->ppmd_merge_thr_idx[38];//21;
	tmp->cu8_pmd_m_thr_idx9   = s->ppmd_merge_thr_idx[39];//21;
	tmp->cu16_pmd_m_thr_idx10 = s->ppmd_merge_thr_idx[10];//6;
	tmp->cu16_pmd_m_thr_idx11 = s->ppmd_merge_thr_idx[11];//6;
	tmp->pmd_thr_idx6 = (tmp->cu16_pmd_m_thr_idx11<<25 | tmp->cu16_pmd_m_thr_idx10<<20 |
			tmp->cu8_pmd_m_thr_idx9<<15 | tmp->cu8_pmd_m_thr_idx8<<10 |
			tmp->cu8_pmd_m_thr_idx7<<5 | tmp->cu8_pmd_m_thr_idx6);

	tmp->cu16_pmd_m_thr_idx12 = s->ppmd_merge_thr_idx[12];//6;
	tmp->cu16_pmd_m_thr_idx13 = s->ppmd_merge_thr_idx[13];//7;
	tmp->cu16_pmd_m_thr_idx14 = s->ppmd_merge_thr_idx[14];//8;
	tmp->cu16_pmd_m_thr_idx15 = s->ppmd_merge_thr_idx[30];//19;
	tmp->cu16_pmd_m_thr_idx16 = s->ppmd_merge_thr_idx[31];//19;
	tmp->cu16_pmd_m_thr_idx17 = s->ppmd_merge_thr_idx[32];//19;
	tmp->pmd_thr_idx7 = (tmp->cu16_pmd_m_thr_idx17<<25 | tmp->cu16_pmd_m_thr_idx16<<20 |
			tmp->cu16_pmd_m_thr_idx15<<15 | tmp->cu16_pmd_m_thr_idx14<<10 |
			tmp->cu16_pmd_m_thr_idx13<<5 | tmp->cu16_pmd_m_thr_idx12);

	tmp->cu16_pmd_m_thr_idx18 = s->ppmd_merge_thr_idx[33];//12;
	tmp->cu16_pmd_m_thr_idx19 = s->ppmd_merge_thr_idx[34];//12;
	tmp->cu32_pmd_m_thr_idx20 = s->ppmd_merge_thr_idx[5];//3;
	tmp->cu32_pmd_m_thr_idx21 = s->ppmd_merge_thr_idx[6];//3;
	tmp->cu32_pmd_m_thr_idx22 = s->ppmd_merge_thr_idx[7];//3;
	tmp->cu32_pmd_m_thr_idx23 = s->ppmd_merge_thr_idx[8];//4;
	tmp->pmd_thr_idx8 = (tmp->cu32_pmd_m_thr_idx23<<25 | tmp->cu32_pmd_m_thr_idx22<<20 |
			tmp->cu32_pmd_m_thr_idx21<<15 | tmp->cu32_pmd_m_thr_idx20<<10 |
			tmp->cu16_pmd_m_thr_idx19<<5 | tmp->cu16_pmd_m_thr_idx18);

	tmp->cu32_pmd_m_thr_idx24 = s->ppmd_merge_thr_idx[9];//5;
	tmp->cu32_pmd_m_thr_idx25 = s->ppmd_merge_thr_idx[25];//16;
	tmp->cu32_pmd_m_thr_idx26 = s->ppmd_merge_thr_idx[26];//16;
	tmp->cu32_pmd_m_thr_idx27 = s->ppmd_merge_thr_idx[27];//16;
	tmp->cu32_pmd_m_thr_idx28 = s->ppmd_merge_thr_idx[28];//17;
	tmp->cu32_pmd_m_thr_idx29 = s->ppmd_merge_thr_idx[29];//18;
	tmp->pmd_thr_idx9 = (tmp->cu32_pmd_m_thr_idx29<<25 | tmp->cu32_pmd_m_thr_idx28<<20 |
			tmp->cu32_pmd_m_thr_idx27<<15 | tmp->cu32_pmd_m_thr_idx26<<10 |
			tmp->cu32_pmd_m_thr_idx25<<5 | tmp->cu32_pmd_m_thr_idx24);

	tmp->cu64_pmd_m_thr_idx30 = s->ppmd_merge_thr_idx[0];//0;
	tmp->cu64_pmd_m_thr_idx31 = s->ppmd_merge_thr_idx[1];//0;
	tmp->cu64_pmd_m_thr_idx32 = s->ppmd_merge_thr_idx[2];//0;
	tmp->cu64_pmd_m_thr_idx33 = s->ppmd_merge_thr_idx[3];//1;
	tmp->cu64_pmd_m_thr_idx34 = s->ppmd_merge_thr_idx[4];//2;
	tmp->cu64_pmd_m_thr_idx35 = s->ppmd_merge_thr_idx[20];//13;
	tmp->pmd_thr_idx10 = (tmp->cu64_pmd_m_thr_idx35<<25 | tmp->cu64_pmd_m_thr_idx34<<20 |
			tmp->cu64_pmd_m_thr_idx33<<15 | tmp->cu64_pmd_m_thr_idx32<<10 |
			tmp->cu64_pmd_m_thr_idx31<<5 | tmp->cu64_pmd_m_thr_idx30);

	tmp->cu64_pmd_m_thr_idx36 = s->ppmd_merge_thr_idx[21];//13;
	tmp->cu64_pmd_m_thr_idx37 = s->ppmd_merge_thr_idx[22];//13;
	tmp->cu64_pmd_m_thr_idx38 = s->ppmd_merge_thr_idx[23];//14;
	tmp->cu64_pmd_m_thr_idx39 = s->ppmd_merge_thr_idx[24];//15;
														  //merge search pmd_thr_idx
	tmp->cu8_pmd_m_s_thr_idx0 = s->ppmd_merge_search_thr_idx[10];//22;
	tmp->cu8_pmd_m_s_thr_idx1 = s->ppmd_merge_search_thr_idx[11];//22;
	tmp->pmd_thr_idx11 = (tmp->cu8_pmd_m_s_thr_idx1<<25 | tmp->cu8_pmd_m_s_thr_idx0<<20 |
			tmp->cu64_pmd_m_thr_idx39<<15 | tmp->cu64_pmd_m_thr_idx38<<10 |
			tmp->cu64_pmd_m_thr_idx37<<5 | tmp->cu64_pmd_m_thr_idx36);

	tmp->cu8_pmd_m_s_thr_idx2 = s->ppmd_merge_search_thr_idx[12];//22;
	tmp->cu8_pmd_m_s_thr_idx3 = s->ppmd_merge_search_thr_idx[13];//22;
	tmp->cu8_pmd_m_s_thr_idx4 = s->ppmd_merge_search_thr_idx[14];//22;
	tmp->cu8_pmd_m_s_thr_idx5 = s->ppmd_merge_search_thr_idx[25];//22;
	tmp->cu8_pmd_m_s_thr_idx6 = s->ppmd_merge_search_thr_idx[26];//22;
	tmp->cu8_pmd_m_s_thr_idx7 = s->ppmd_merge_search_thr_idx[27];//22;
	tmp->pmd_thr_idx12 = (tmp->cu8_pmd_m_s_thr_idx7<<25 | tmp->cu8_pmd_m_s_thr_idx6<<20 |
			tmp->cu8_pmd_m_s_thr_idx5<<15 | tmp->cu8_pmd_m_s_thr_idx4<<10 |
			tmp->cu8_pmd_m_s_thr_idx3<<5 | tmp->cu8_pmd_m_s_thr_idx2);

	tmp->cu8_pmd_m_s_thr_idx8 = s->ppmd_merge_search_thr_idx[28];//22;
	tmp->cu8_pmd_m_s_thr_idx9 = s->ppmd_merge_search_thr_idx[29];//22;
	tmp->cu16_pmd_m_s_thr_idx10 = s->ppmd_merge_search_thr_idx[5];//22;
	tmp->cu16_pmd_m_s_thr_idx11 = s->ppmd_merge_search_thr_idx[6];//22;
	tmp->cu16_pmd_m_s_thr_idx12 = s->ppmd_merge_search_thr_idx[7];//22;
	tmp->cu16_pmd_m_s_thr_idx13 = s->ppmd_merge_search_thr_idx[8];//22;
	tmp->pmd_thr_idx13 = (tmp->cu16_pmd_m_s_thr_idx13<<25 | tmp->cu16_pmd_m_s_thr_idx12<<20 |
			tmp->cu16_pmd_m_s_thr_idx11<<15 | tmp->cu16_pmd_m_s_thr_idx10<<10 |
			tmp->cu8_pmd_m_s_thr_idx9<<5 | tmp->cu8_pmd_m_s_thr_idx8);

	tmp->cu16_pmd_m_s_thr_idx14 = s->ppmd_merge_search_thr_idx[9];//22;
	tmp->cu16_pmd_m_s_thr_idx15 = s->ppmd_merge_search_thr_idx[20];//22;
	tmp->cu16_pmd_m_s_thr_idx16 = s->ppmd_merge_search_thr_idx[21];//22;
	tmp->cu16_pmd_m_s_thr_idx17 = s->ppmd_merge_search_thr_idx[22];//22;
	tmp->cu16_pmd_m_s_thr_idx18 = s->ppmd_merge_search_thr_idx[23];//22;
	tmp->cu16_pmd_m_s_thr_idx19 = s->ppmd_merge_search_thr_idx[24];//22;
	tmp->pmd_thr_idx14 = (tmp->cu16_pmd_m_s_thr_idx19<<25 | tmp->cu16_pmd_m_s_thr_idx18<<20 |
			tmp->cu16_pmd_m_s_thr_idx17<<15 | tmp->cu16_pmd_m_s_thr_idx16<<10 |
			tmp->cu16_pmd_m_s_thr_idx15<<5 | tmp->cu16_pmd_m_s_thr_idx14);

	tmp->cu32_pmd_m_s_thr_idx20 = s->ppmd_merge_search_thr_idx[0];//22;
	tmp->cu32_pmd_m_s_thr_idx21 = s->ppmd_merge_search_thr_idx[1];//22;
	tmp->cu32_pmd_m_s_thr_idx22 = s->ppmd_merge_search_thr_idx[2];//22;
	tmp->cu32_pmd_m_s_thr_idx23 = s->ppmd_merge_search_thr_idx[3];//22;
	tmp->cu32_pmd_m_s_thr_idx24 = s->ppmd_merge_search_thr_idx[4];//22;
	tmp->cu32_pmd_m_s_thr_idx25 = s->ppmd_merge_search_thr_idx[15];//22;
	tmp->pmd_thr_idx15 = (tmp->cu32_pmd_m_s_thr_idx25<<25 | tmp->cu32_pmd_m_s_thr_idx24<<20 |
			tmp->cu32_pmd_m_s_thr_idx23<<15 | tmp->cu32_pmd_m_s_thr_idx22<<10 |
			tmp->cu32_pmd_m_s_thr_idx21<<5 | tmp->cu32_pmd_m_s_thr_idx20);

	tmp->cu32_pmd_m_s_thr_idx26 = s->ppmd_merge_search_thr_idx[16];//22;
	tmp->cu32_pmd_m_s_thr_idx27 = s->ppmd_merge_search_thr_idx[17];//22;
	tmp->cu32_pmd_m_s_thr_idx28 = s->ppmd_merge_search_thr_idx[18];//22;
	tmp->cu32_pmd_m_s_thr_idx29 = s->ppmd_merge_search_thr_idx[19];//22;
																   //intra pmd_thr_idx
	tmp->cu8_pmd_intra_thr_idx0 = s->ppmd_intra_thr_idx[10];//22;
	tmp->cu8_pmd_intra_thr_idx1 = s->ppmd_intra_thr_idx[11];//22;
	tmp->pmd_thr_idx16 = (tmp->cu8_pmd_intra_thr_idx1<<25 | tmp->cu8_pmd_intra_thr_idx0<<20 |
			tmp->cu32_pmd_m_s_thr_idx29<<15 | tmp->cu32_pmd_m_s_thr_idx28<<10 |
			tmp->cu32_pmd_m_s_thr_idx27<<5 | tmp->cu32_pmd_m_s_thr_idx26);

	tmp->cu8_pmd_intra_thr_idx2 = s->ppmd_intra_thr_idx[12];//22;
	tmp->cu8_pmd_intra_thr_idx3 = s->ppmd_intra_thr_idx[13];//22;
	tmp->cu8_pmd_intra_thr_idx4 = s->ppmd_intra_thr_idx[14];//22;
	tmp->cu8_pmd_intra_thr_idx5 = s->ppmd_intra_thr_idx[25];//22;
	tmp->cu8_pmd_intra_thr_idx6 = s->ppmd_intra_thr_idx[26];//22;
	tmp->cu8_pmd_intra_thr_idx7 = s->ppmd_intra_thr_idx[27];//22;
	tmp->pmd_thr_idx17 = (tmp->cu8_pmd_intra_thr_idx7<<25 | tmp->cu8_pmd_intra_thr_idx6<<20 |
			tmp->cu8_pmd_intra_thr_idx5<<15 | tmp->cu8_pmd_intra_thr_idx4<<10 |
			tmp->cu8_pmd_intra_thr_idx3<<5 | tmp->cu8_pmd_intra_thr_idx2);

	tmp->cu8_pmd_intra_thr_idx8 = s->ppmd_intra_thr_idx[28];//22;
	tmp->cu8_pmd_intra_thr_idx9 = s->ppmd_intra_thr_idx[29];//22;
	tmp->cu16_pmd_intra_thr_idx10 = s->ppmd_intra_thr_idx[5];//22;
	tmp->cu16_pmd_intra_thr_idx11 = s->ppmd_intra_thr_idx[6];//22;
	tmp->cu16_pmd_intra_thr_idx12 = s->ppmd_intra_thr_idx[7];//22;
	tmp->cu16_pmd_intra_thr_idx13 = s->ppmd_intra_thr_idx[8];//22;
	tmp->pmd_thr_idx18 = (tmp->cu16_pmd_intra_thr_idx13<<25 | tmp->cu16_pmd_intra_thr_idx12<<20 |
			tmp->cu16_pmd_intra_thr_idx11<<15 | tmp->cu16_pmd_intra_thr_idx10<<10 |
			tmp->cu8_pmd_intra_thr_idx9<<5 | tmp->cu8_pmd_intra_thr_idx8);

	tmp->cu16_pmd_intra_thr_idx14 = s->ppmd_intra_thr_idx[9];//22;
	tmp->cu16_pmd_intra_thr_idx15 = s->ppmd_intra_thr_idx[20];//22;
	tmp->cu16_pmd_intra_thr_idx16 = s->ppmd_intra_thr_idx[21];//22;
	tmp->cu16_pmd_intra_thr_idx17 = s->ppmd_intra_thr_idx[22];//22;
	tmp->cu16_pmd_intra_thr_idx18 = s->ppmd_intra_thr_idx[23];//22;
	tmp->cu16_pmd_intra_thr_idx19 = s->ppmd_intra_thr_idx[24];//22;
	tmp->pmd_thr_idx19 = (tmp->cu16_pmd_intra_thr_idx19<<25 | tmp->cu16_pmd_intra_thr_idx18<<20 |
			tmp->cu16_pmd_intra_thr_idx17<<15 | tmp->cu16_pmd_intra_thr_idx16<<10 |
			tmp->cu16_pmd_intra_thr_idx15<<5 | tmp->cu16_pmd_intra_thr_idx14);

	tmp->cu32_pmd_intra_thr_idx20 = s->ppmd_intra_thr_idx[0];//22;
	tmp->cu32_pmd_intra_thr_idx21 = s->ppmd_intra_thr_idx[1];//22;
	tmp->cu32_pmd_intra_thr_idx22 = s->ppmd_intra_thr_idx[2];//22;
	tmp->cu32_pmd_intra_thr_idx23 = s->ppmd_intra_thr_idx[3];//22;
	tmp->cu32_pmd_intra_thr_idx24 = s->ppmd_intra_thr_idx[4];//22;
	tmp->cu32_pmd_intra_thr_idx25 = s->ppmd_intra_thr_idx[15];//22;
	tmp->pmd_thr_idx20 = (tmp->cu32_pmd_intra_thr_idx25<<25 | tmp->cu32_pmd_intra_thr_idx24<<20 |
			tmp->cu32_pmd_intra_thr_idx23<<15 | tmp->cu32_pmd_intra_thr_idx22<<10 |
			tmp->cu32_pmd_intra_thr_idx21<<5 | tmp->cu32_pmd_intra_thr_idx20);

	tmp->cu32_pmd_intra_thr_idx26 = s->ppmd_intra_thr_idx[16];//22;
	tmp->cu32_pmd_intra_thr_idx27 = s->ppmd_intra_thr_idx[17];//22;
	tmp->cu32_pmd_intra_thr_idx28 = s->ppmd_intra_thr_idx[18];//22;
	tmp->cu32_pmd_intra_thr_idx29 = s->ppmd_intra_thr_idx[19];//22;
	tmp->pmd_thr_idx21 = (tmp->cu32_pmd_intra_thr_idx29<<15 | tmp->cu32_pmd_intra_thr_idx28<<10 |
			tmp->cu32_pmd_intra_thr_idx27<<5 | tmp->cu32_pmd_intra_thr_idx26);

	//setting pmd_thrd
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_1_0, 0, tmp->pmd_thr1<<14 | tmp->pmd_thr0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_3_2, 0, tmp->pmd_thr3<<14 | tmp->pmd_thr2);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_5_4, 0, tmp->pmd_thr5<<14 | tmp->pmd_thr4);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_7_6, 0, tmp->pmd_thr7<<14 | tmp->pmd_thr6);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_9_8, 0, tmp->pmd_thr9<<14 | tmp->pmd_thr8);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_11_10, 0,tmp->pmd_thr11<<14 | tmp->pmd_thr10);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_13_12, 0,tmp->pmd_thr13<<14 | tmp->pmd_thr12);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_15_14, 0,tmp->pmd_thr15<<14 | tmp->pmd_thr14);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_17_16, 0,tmp->pmd_thr17<<14 | tmp->pmd_thr16);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_19_18, 0,tmp->pmd_thr19<<14 | tmp->pmd_thr18);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_21_20, 0,tmp->pmd_thr21<<14 | tmp->pmd_thr20);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_23_22, 0,tmp->pmd_thr23<<14 | tmp->pmd_thr22);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_25_24, 0,tmp->pmd_thr25<<14 | tmp->pmd_thr24);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_27_26, 0,tmp->pmd_thr27<<14 | tmp->pmd_thr26);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_29_28, 0,tmp->pmd_thr29<<14 | tmp->pmd_thr28);
	//setting pmd_thrd_idx
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX0, 0, tmp->pmd_thr_idx0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX1, 0, tmp->pmd_thr_idx1);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX2, 0, tmp->pmd_thr_idx2);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX3, 0, tmp->pmd_thr_idx3);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX4, 0, tmp->pmd_thr_idx4);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX5, 0, tmp->pmd_thr_idx5);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX6, 0, tmp->pmd_thr_idx6);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX7, 0, tmp->pmd_thr_idx7);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX8, 0, tmp->pmd_thr_idx8);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX9, 0, tmp->pmd_thr_idx9);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX10, 0, tmp->pmd_thr_idx10);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX11, 0, tmp->pmd_thr_idx11);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX12, 0, tmp->pmd_thr_idx12);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX13, 0, tmp->pmd_thr_idx13);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX14, 0, tmp->pmd_thr_idx14);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX15, 0, tmp->pmd_thr_idx15);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX16, 0, tmp->pmd_thr_idx16);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX17, 0, tmp->pmd_thr_idx17);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX18, 0, tmp->pmd_thr_idx18);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX19, 0, tmp->pmd_thr_idx19);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX20, 0, tmp->pmd_thr_idx20);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_PMD_THR_IDX21, 0, tmp->pmd_thr_idx21);

	//ned cfg add by xdzhang
	tmp->md_ned_score_tbl0 = (s->ned_score_table[0][0][0]>>1)  |
		(s->ned_score_table[0][0][1]>>1)<< 4  |
		(s->ned_score_table[0][0][2]>>1)<< 9  |
		(s->ned_score_table[0][0][3]>>1)<< 14 |
		(s->ned_score_table[0][0][4]>>1)<< 20 |
		(s->ned_score_table[0][0][5]>>1)<< 26;
	tmp->md_ned_score_tbl1 = (s->ned_score_table[0][0][5]>>1)>> 6  |
		(s->ned_score_table[0][0][6]>>1)<< 1  |
		s->ned_score_table[0][1][0]<< 8  |
		s->ned_score_table[0][1][1]<< 10 |
		s->ned_score_table[0][1][2]<< 13 |
		s->ned_score_table[0][1][3]<< 16 |
		s->ned_score_table[0][1][4]<< 20 |
		s->ned_score_table[0][1][5]<< 24 |
		s->ned_score_table[0][1][6]<< 28;
	tmp->md_ned_score_tbl2 = s->ned_score_table[0][1][6]>> 4  |
		s->ned_score_table[0][1][7]<< 1  |
		(s->ned_score_table[1][0][0]>>1)<< 6  |
		(s->ned_score_table[1][0][1]>>1)<< 10 |
		(s->ned_score_table[1][0][2]>>1)<< 15 |
		(s->ned_score_table[1][0][3]>>1)<< 20 |
		(s->ned_score_table[1][0][4]>>1)<< 26;
	tmp->md_ned_score_tbl3 = (s->ned_score_table[1][0][5]>>1)      |
		(s->ned_score_table[1][0][6]>>1)<< 7  |
		s->ned_score_table[1][1][0]<< 14  |
		s->ned_score_table[1][1][1]<< 16 |
		s->ned_score_table[1][1][2]<< 19 |
		s->ned_score_table[1][1][3]<< 22 |
		s->ned_score_table[1][1][4]<< 26 |
		s->ned_score_table[1][1][5]<< 30;
	tmp->md_ned_score_tbl4 = s->ned_score_table[1][1][5]>> 2  |
		s->ned_score_table[1][1][6]<< 2  |
		s->ned_score_table[1][1][7]<< 7  |
		s->ned_score_bias[0] << 12       |
		s->ned_score_bias[1] << 18       |
		s->ned_en << 24;
	tmp->md_ned_disable0    = s->ned_sad_thr[0][0][0]  |
		s->ned_sad_thr[0][0][1] << 8 |
		s->ned_sad_thr[0][1][0] << 16 |
		s->ned_sad_thr[0][1][1] << 24 ;
	tmp->md_ned_disable1    = s->ned_sad_thr[1][0][0]  |
		s->ned_sad_thr[1][0][1] << 5 |
		s->ned_sad_thr[1][1][0] << 10 |
		s->ned_sad_thr[1][1][1] << 16 |
		s->ned_motion_en        << 22 |
		s->ned_motion_shift     << 23 |
		s->c_sad_min_bias       << 26 |
		s->recon_resi_en[0][0]  << 29 |
		s->recon_resi_en[1][0]  << 30 |
		s->recon_resi_en[2][0]  << 31;
	tmp->md_ned_disable2    = s->ned_sad_thr_cpx_step[0][0]       |
		s->ned_sad_thr_cpx_step[0][1] << 2  |
		s->ned_sad_thr_cpx_step[1][0] << 4  |
		s->ned_sad_thr_cpx_step[1][1] << 6  |
		s->ccf_rm_en                  << 8  |
		s->mosaic_motion_shift        << 9  ;
	//csd_qp_thrd
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_SC_TBL_0  , 0, tmp->md_ned_score_tbl0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_SC_TBL_1  , 0, tmp->md_ned_score_tbl1);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_SC_TBL_2  , 0, tmp->md_ned_score_tbl2);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_SC_TBL_3  , 0, tmp->md_ned_score_tbl3);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_SC_TBL_4  , 0, tmp->md_ned_score_tbl4);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_0  , 0, tmp->md_ned_disable0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_1  , 0, tmp->md_ned_disable1);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_DISA_2  , 0, tmp->md_ned_disable2);
	//mosaic cfg add by xdzhang
	tmp->mosaic_cfg_0       = (s->mosaic_diff_thr[3]       << 28) |
		(s->mosaic_diff_thr[2]       << 24) |
		(s->mosaic_diff_thr[1]       << 20) |
		(s->mosaic_diff_thr[0]       << 16) |
		(s->mosaic_recon_flat_thr[3] << 12) |
		(s->mosaic_recon_flat_thr[2] << 8)  |
		(s->mosaic_recon_flat_thr[1] << 4)  |
		(s->mosaic_recon_flat_thr[0]);
	tmp->md_rcn_plain_cfg0  = s->var_flat_recon_thrd[0][0]	|
		s->var_flat_recon_thrd[0][1] << 6	|
		s->var_flat_recon_thrd[0][2] << 12 |
		s->var_flat_recon_thrd[0][3] << 18 |
		s->var_flat_recon_thrd[0][4] << 24 |
		s->rcn_plain_en  << 30  |
		s->mosaic_en << 31;
	tmp->md_rcn_plain_cfg1  = s->var_flat_recon_thrd[1][0]	      |
		s->var_flat_recon_thrd[1][1] << 6	|
		s->var_flat_recon_thrd[1][2] << 12 |
		s->var_flat_recon_thrd[1][3] << 18 |
		s->var_flat_recon_thrd[1][4] << 24 |
		s->var_flat_sub_size         << 30 |
		s->mosaic_intra_texture_en   << 31;
	tmp->csd_qp_thrd_0      =  (s->csd_qp_thrd[4] << 24) |
		(s->csd_qp_thrd[3] << 18) |
		(s->csd_qp_thrd[2] << 12) |
		(s->csd_qp_thrd[1] << 6)  |
		(s->csd_qp_thrd[0]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_MOSAIC_CFG_0  , 0, tmp->mosaic_cfg_0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_CSD_QP_THRD_0, 0, tmp->csd_qp_thrd_0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_RCN_PLAIN_CFG_0  , 0, tmp->md_rcn_plain_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_NED_RCN_PLAIN_CFG_1  , 0, tmp->md_rcn_plain_cfg1);


	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + RADIX_REG_MD_INIT , 0, 1);
	//- dlambda add by lqzhang
	tmp->abs_dlambda_simple_dqp  = (abs(s->dlambda_simple_dqp) == 0 ) ? 0 : (( abs(s->dlambda_simple_dqp) & 0xF ) | 0x10 );
	tmp->abs_dlambda_edge_dqp    = (abs(s->dlambda_edge_dqp) == 0 ) ? 0 : (( abs(s->dlambda_edge_dqp) & 0xF ) | 0x10 );

	tmp->md_dlambda_cfg0 = ((s->dlambda_max_depth & 0x03)   << 6)  |
		((s->dlambda_motion_shift & 0x07)<< 3)  |
		((s->dlambda_icsd_en & 0x01)     << 2)  |
		((s->dlambda_pcsd_en & 0x01)     << 1)  |
		(s->dlambda_ppmd_en & 0x01);


	tmp->md_dlambda_cfg1 =  ((s->dlambda_frmqp_thrd & 0x3F)   << 26) |
		((s->frame_qp & 0x3F)             << 20) |
		((s->dlambda_still_dqp & 0x1F)    << 15) |
		((tmp->abs_dlambda_edge_dqp & 0x1F)    << 10) |
		((s->dlambda_nei_cplx_dqp & 0x1F) << 5 ) |
		((tmp->abs_dlambda_simple_dqp & 0x1F))        ;

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_DLAMBDA_CFG0 , 0, tmp->md_dlambda_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_DLAMBDA_CFG1 , 0, tmp->md_dlambda_cfg1);

	//- color shadow sse add by lqzhang

	tmp->md_css_cfg0 =  (((s->color_shadow_sse_priority_en & 0x1  ) << 18) |
			((s->color_shadow_qp & 0x3F  )             << 12) |
			((s->color_shadow_sse_value_thrd_en & 0x1) << 11) |
			((s->color_shadow_sse_ratio_thrd[1] & 0x7) << 8) |
			((s->color_shadow_sse_ratio_thrd[0] & 0x7) << 5) |
			((s->color_shadow_motion_shift_sse & 0xF) << 1) |
			(s->color_shadow_sse_en & 0x1) ) ;

	tmp->md_css_cfg1 = s->color_shadow_sse_value_thrd[0] ;
	tmp->md_css_cfg2 = s->color_shadow_sse_value_thrd[1] ;
	tmp->md_css_cfg3 = s->color_shadow_sse_value_thrd[2] ;

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_CSS_CFG0 , 0, tmp->md_css_cfg0);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_CSS_CFG1 , 0, tmp->md_css_cfg1);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_CSS_CFG2 , 0, tmp->md_css_cfg2);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MD_BASE_VDMA + MD_SLV_CSS_CFG3 , 0, tmp->md_css_cfg3);

	/*************************************************************************************
	  DT Module
	 *************************************************************************************/
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DT_BASE_VDMA + RADIX_REG_DT_CFG0 , 0, ((((1<<6) & 0xFF) << 0) +//codec_id
				((s->frame_type & 0xF) << 8) +//slice_type
				((6 & 0x7) << 12) +//lcu_size
				((0 & 0x1) << 15) +//id_decode
				((1) << 16) +//tu_enable
				((s->frame_cqp_offset & 0xFF) << 20) +
				0));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DT_BASE_VDMA + RADIX_REG_DT_CFG1 , 0, (((s->dblk_en) << 0) +//dblk_chng_en
				((6 & 0x7) << 3) +//lcu_size
				((1) << 6) +// cross slice
				((0) << 7) +// cross tile
				((0) << 8) +// is_dec
				0));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DT_BASE_VDMA + RADIX_REG_DT_INIT , 0, 1);

	/*************************************************************************************
	  DBLK Module
	 *************************************************************************************/

#ifdef SET_NEW_UDBLK
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_GLB_TRIG , 0, ((s->frame_height & 0xffff) |
				((s->frame_width & 0xffff) << 16 )));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_GLB_CTRL , 0, (((0) <<31) |     //0:265, 1:264
				((s->frame_cqp_offset & 0xf) <<24) | //cb,cr qp offset value
				(s->dblk_en <<16) |                //udblk_en
				((s->beta_offset_div2 & 0xff)<<8) | //beta_o:wffset_div
				((s->tc_offset_div2 & 0xff)<<0) )); //tc_offset_div
#else
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_GLB_TRIG , 0, ((1 << 16) |//crc_en
				(1 << 8) |//glb_en
				(1 << 4)));//sw_rst
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_GLB_CTRL , 0, ( (s->dblk_en<<0) |
				(s->dblk_en<<1) |
				(s->sao_en<<2) |
				(0 << 3) |/*video_type*/
				(s->dblk_gray_en << 6) |
				(1 << 9)|/*flush_slice*/
				(1 << 10) |/*cross_tile*/
				(1 << 11) |/*cross_slice*/
				(2 << 12) |/*lcu_level*/
				(((s->frame_height & 15) ? 1 : 0) << 16) |
				(((s->frame_width & 15) ? 1 : 0) << 17)));

	tmp->hevc_mb_size = (((s->frame_width + 15)/16 - 1) |
			(((s->frame_height + 15)/16 - 1) << 8) |
			(0 << 16) | /*first_mbx*/
			(0 << 24));

	tmp->svac_mb_size = (((s->frame_width & 0xffff) << 16) |
			((s->frame_height & 0xffff) << 0));

	tmp->mb_size      = s->video_type == 1 ? tmp->svac_mb_size : tmp->hevc_mb_size;

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_MB_SIZE , 0, (tmp->mb_size));

	tmp->hevc_filter_param = ((s->tc_offset_div2 & 0xff) |
			((s->beta_offset_div2 & 0xff) << 8) |
			((s->frame_cqp_offset & 0xff) << 16) |
			((s->frame_cqp_offset & 0xff) << 24));

	tmp->svac_filter_param = (1 << 19 |
			((s->filter_level != 0) & 0x1) << 18 |
			(s->mb_lim & 0xff) << 10 |
			(s->lim & 0x3f) << 4 |
			s->hev_thr & 0xf);

	tmp->svac_filter_param_p = ((s->mb_lim_p & 0xff) << 10 |
			(s->lim_p & 0x3f) << 4 |
			s->hev_thr_p & 0xf);

	int common_filter_param = s->video_type == 1 ? tmp->svac_filter_param_p : tmp->hevc_filter_param;

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_CHN_BA , 0, tmp->svac_filter_param); //useless in h265

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_FLT_PARA , 0, common_filter_param);

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_FRM_SIZE , 0, (((s->frame_height - 1) << 16) |
				(s->frame_width - 1)));
	/*   RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_DBLK_BASE_VDMA + RADIX_REG_DBLK_GLB_TRIG , 0, ((1 << 16) */
	/* 									       (1 << 8) | /\*glb_en*\/ */
	/* 									       (1 << 12))); */
#endif
	/*************************************************************************************
	  ODMA Module
	 *************************************************************************************/

	tmp->align_height = (s->frame_height + 15) & ~15;
	tmp->fm_h_is_align8 = (s->frame_height & 0xf) != 0;
	tmp->nv12_flag = 0;
	//when buf share is new addr(dynamic), else old addr(static)
	tmp->odma_y_addr = s->buf_share_flag ? s->buf_start_yaddr : (unsigned int)s->dst_y_pa;
	tmp->odma_c_addr = s->buf_share_flag ? s->buf_start_caddr : (unsigned int)s->dst_c_pa;
	/////////////
	// jrfc
	/////////////
	tmp->odma_jrfc_head_y_addr  = (unsigned int)s->head_dsty;
	tmp->odma_jrfc_head_c_addr  = (unsigned int)s->head_dstc;
	tmp->odma_jrfc_head_sp_y_addr = (unsigned int)s->head_sp_dsty;
	tmp->odma_jrfc_head_sp_c_addr = (unsigned int)s->head_sp_dstc;
	/////////////
	// bufshare
	/////////////
	tmp->odma_bfsh_base_y_addr  = (unsigned int)s->buf_base_yaddr;
	tmp->odma_bfsh_base_c_addr  = (unsigned int)s->buf_base_caddr;
	tmp->odma_bfsh_bynd_y_addr  = (unsigned int)s->buf_beyond_yaddr;
	tmp->odma_bfsh_bynd_c_addr  = (unsigned int)s->buf_beyond_caddr;

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_GLB_CTRL , 0, ((s->buf_share_flag <<31) |
				(s->compress_flag <<30) |
				(tmp->nv12_flag <<29) |
				(s->frame_height << 14) |
				s->frame_width));

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_TLY_BA , 0, tmp->odma_y_addr);//tile Y address / body Y address
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_TLC_BA , 0, tmp->odma_c_addr); //tile C address / body C address
																									  //  RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_TL_STR , 0, ((s->dst_y_stride << 16) |  s->dst_c_stride));

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_JRFC_HEAD_Y_BA,    0, tmp->odma_jrfc_head_y_addr   );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_JRFC_HEAD_C_BA,    0, tmp->odma_jrfc_head_c_addr   );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_JRFC_SP_HEAD_Y_BA, 0, tmp->odma_jrfc_head_sp_y_addr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_JRFC_SP_HEAD_C_BA, 0, tmp->odma_jrfc_head_sp_c_addr);

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BASE_Y_BA, 0, tmp->odma_bfsh_base_y_addr   );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BASE_C_BA, 0, tmp->odma_bfsh_base_c_addr   );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BYND_Y_BA, 0, tmp->odma_bfsh_bynd_y_addr   );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_BFSH_BYND_C_BA, 0, tmp->odma_bfsh_bynd_c_addr   );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_ODMA_BASE_VDMA + RADIX_REG_ODMA_GLB_TRIG , 0, ((0 << 31) | //0: 265, 1: 264
				(1 << 5) | //init
				(0 << 6) | //0 is crc_en
				(0 << 4)   //ckg low means do clock-gating
				));

	/*************************************************************************************
	  JRFD Module
	 *************************************************************************************/

	tmp->jrfd_y_addr = s->buf_share_flag ? (unsigned int)s->buf_ref_yaddr : (unsigned int)s->ref_y_pa;
	tmp->jrfd_c_addr = s->buf_share_flag ? (unsigned int)s->buf_ref_caddr : (unsigned int)s->ref_c_pa;
	tmp->jrfd_my_addr = /*s->buf_share_flag ? (unsigned int)s->buf_ref_yaddr :*/ (unsigned int)s->mref_y_pa;
	tmp->jrfd_mc_addr = /*s->buf_share_flag ? (unsigned int)s->buf_ref_caddr :*/ (unsigned int)s->mref_c_pa;
	tmp->buf_beyond_yaddr = (s->frm_num_gop == 1 && s->use_dummy_byd) ? s->dummy_buf_beyond_yaddr : s->buf_beyond_yaddr;
	tmp->buf_beyond_caddr = (s->frm_num_gop == 1 && s->use_dummy_byd) ? s->dummy_buf_beyond_caddr : s->buf_beyond_caddr;

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_CTRL , 0, (((s->buf_share_flag & 0x1 ) <<31) |
				(0<<30) | //is_helix
				(0<<28) | //clk_gate
				(s->compress_flag <<27) |
				((tmp->align_height & 0x1fff) <<14) | //[13] is idle
				(s->frame_width & 0x1fff)));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_HDYA , 0, (int)s->head_refy);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_HDCA , 0, (int)s->head_refc);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_HSTR , 0, (((s->cm_head_total & 0xffff) << 16) | (s->lm_head_total & 0xffff)));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BDYA , 0, tmp->jrfd_y_addr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BDCA , 0, tmp->jrfd_c_addr);

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BASEY_ADDR , 0, (int)s->buf_base_yaddr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BASEC_ADDR , 0, (int)s->buf_base_caddr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDY_ADDR , 0, (int)s->buf_beyond_yaddr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDC_ADDR , 0, (int)s->buf_beyond_caddr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDY_ADDR , 0, (int)tmp->buf_beyond_yaddr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_BUFS_BEYDC_ADDR , 0, (int)tmp->buf_beyond_caddr);

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_MHDY , 0, (int)s->head_mrefy);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_MHDC , 0, (int)s->head_mrefc);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_MBDY , 0, tmp->jrfd_my_addr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_MBDC , 0, tmp->jrfd_mc_addr);


	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_JRFD_BASE_VDMA + RADIX_REG_JRFD_TRIG , 0, ((1 << 5) |//init
				(0 << 4)  //ckg low means do clock-gating
				));


	/*************************************************************************************
	  MCE Module
	 *************************************************************************************/
	if(s->frame_type != 2){
		tmp->esti_ctrl |= (RADIX_MCE_ESTI_CTRL_SCL(s->mce_scl_mode)/*trbl mvp*/ |
				RADIX_MCE_ESTI_CTRL_FBG(0) |
				RADIX_MCE_ESTI_CTRL_CLMV |
				RADIX_MCE_ESTI_CTRL_MSS(s->max_sech_step) |
				RADIX_MCE_ESTI_CTRL_QRL(s->quart_pixel_sech_en) |
				RADIX_MCE_ESTI_CTRL_HRL(s->half_pixel_sech_en) |
				RADIX_MCE_ESTI_CTRL_RF8(s->refine_cu8_mode) |
				RADIX_MCE_ESTI_CTRL_RF16(s->refine_cu16_mode) |
				RADIX_MCE_ESTI_CTRL_RF32(s->refine_cu32_mode)
				);
		if(s->inter_mode[3][0])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_8X8;
		if(s->inter_mode[2][1])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_16X8;
		if(s->inter_mode[2][2])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_8X16;
		if(s->inter_mode[2][0])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_16X16;
		if(s->inter_mode[1][1])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_32X16;
		if(s->inter_mode[1][2])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_16X32;
		if(s->inter_mode[1][0])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_32X32;
		if(s->inter_mode[0][3])
			tmp->esti_ctrl |= RADIX_MCE_ESTI_CTRL_PUE_64X64;
		tmp->merg_info = 0;
		if(s->inter_mode[3][3])
			tmp->merg_info |= RADIX_MCE_MRGI_MRGE_8X8;
		if(s->inter_mode[2][3])
			tmp->merg_info |= RADIX_MCE_MRGI_MRGE_16X16;
		if(s->inter_mode[1][3])
			tmp->merg_info |= RADIX_MCE_MRGI_MRGE_32X32;
		if(s->inter_mode[0][3])
			tmp->merg_info |= RADIX_MCE_MRGI_MRGE_64X64;

		if(s->mrg_en)
			tmp->merg_info |= RADIX_MCE_MRGI_MRGE1_CU8;

		if(s->mrg_fpel_en & 0x1)
			tmp->merg_info |= RADIX_MCE_MRGI_FPEL_CU8;
		if(s->mrg_fpel_en & 0x2)
			tmp->merg_info |= RADIX_MCE_MRGI_FPEL_CU16;

		s->max_mvrx = 64;
		s->max_mvry = 32;
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_FRM_SIZE , 0, (RADIX_MCE_FRM_SIZE_FH(s->frame_height-1) |
					RADIX_MCE_FRM_SIZE_FW(s->frame_width-1)));
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_FRM_STRD , 0, (RADIX_MCE_FRM_STRD_STRDC(s->ref_c_stride) |
					RADIX_MCE_FRM_STRD_STRDY(s->ref_y_stride)));
		tmp->mce_comp_ctap = (s->video_type == 1) ? RADIX_MCE_TAP_TAP8 : RADIX_MCE_TAP_TAP4;
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_COMP_CTRL , 0, (RADIX_MCE_COMP_CTRL_CCE |
					RADIX_MCE_COMP_CTRL_CTAP(tmp->mce_comp_ctap & 0x3) |//x265:tap4, svac:tap8
					RADIX_MCE_COMP_CTRL_CSPT(RADIX_MCE_SPT_SYMM) |
					RADIX_MCE_COMP_CTRL_CSPP(RADIX_MCE_SPP_EPEL) |
					RADIX_MCE_COMP_CTRL_YCE |
					RADIX_MCE_COMP_CTRL_YTAP(RADIX_MCE_TAP_TAP8) |
					RADIX_MCE_COMP_CTRL_YSPT(RADIX_MCE_SPT_AUTO) |
					RADIX_MCE_COMP_CTRL_YSPP(RADIX_MCE_SPP_QPEL)));
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_ESTI_CTRL , 0, tmp->esti_ctrl);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MRGI , 0, tmp->merg_info);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MVR , 0, (RADIX_MCE_MVR_MVRY(s->max_mvry * 4) | /*RADIX_MAX_MVRY*/
					RADIX_MCE_MVR_MVRX(s->max_mvrx * 4)));/*RADIX_MAX_MVRX*/
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_RESI_TST , 0, (RADIX_MCE_RTST7(12) |
					RADIX_MCE_RTST6(10) |
					RADIX_MCE_RTST5(8) |
					RADIX_MCE_RTST4(6) |
					RADIX_MCE_RTST3(4) |
					RADIX_MCE_RTST2(2) |
					RADIX_MCE_RTST1(1) |
					RADIX_MCE_RTST0(0)));
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_RESI_TDS , 0, (RADIX_MCE_RTDF1(s->mce_resi_thr[0][0][2]) |//diff
					RADIX_MCE_RTSC0(s->mce_resi_thr[0][0][0])));//score
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_RESI_TMAX , 0, (RADIX_MCE_RTMAX1(s->mce_resi_thr[0][0][3]) |//max1
					RADIX_MCE_RTMAX0(s->mce_resi_thr[0][0][1])));//max
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_PREF_EXPD , 0, (RADIX_MCE_PREF_EXPD_L(4) |
					RADIX_MCE_PREF_EXPD_R(4) |
					RADIX_MCE_PREF_EXPD_D(4) |
					RADIX_MCE_PREF_EXPD_T(4)));
		if(s->buf_share_flag){
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_RLUT(0, 0) , 0, (int)s->buf_base_yaddr);//base
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_RLUT(0, 0)+4 , 0, (int)s->buf_base_caddr);//base
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRY , 0, (int)s->buf_ref_yaddr);
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRC , 0, (int)s->buf_ref_caddr);
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MREF_ADRY , 0, (int)s->mref_y_pa);//ref 1
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MREF_ADRC , 0, (int)s->mref_c_pa);//ref 1
		}else{
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_RLUT(0, 0) , 0, (int)s->dst_y_pa);
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_RLUT(0, 0)+4 , 0, (int)s->dst_c_pa);
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRY , 0, (int)s->ref_y_pa);//base
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BFSH_ADRC , 0, (int)s->ref_c_pa);//base
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MREF_ADRY , 0, (int)s->mref_y_pa);//ref 1
			RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_MREF_ADRC , 0, (int)s->mref_c_pa);//ref 1
		}
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_BOTM_ID , 0, s->buf_rem_mby);

		if(s->video_type == 1){//svac
			for(i=0; i<16; i++){
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_Y+i*8 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_SVAC_QPEL][i].intp[0],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_pkg[0],
							0, 0,
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_dir[0],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_rnd[0],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_sft[0],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_sintp[0],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_srnd[0],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_sbias[0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_Y+i*8+4 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_SVAC_QPEL][i].intp[1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_pkg[1],
							0, 0,
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_dir[1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_rnd[1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_sft[1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_sintp[1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_srnd[1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_sbias[1]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][3],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][2],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16+4 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][7],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][6],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][5],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[0][4]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16+8 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][3],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][2],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][1],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16+12 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][7],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][6],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][5],
							radix_IntpFMT[RADIX_SVAC_QPEL][i].intp_coef[1][4]));
			}
			for(i=0; i<8; i++){
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_C+i*8 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_SVAC_EPEL][i].intp[0],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_pkg[0],
							0, 0,
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_dir[0],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_rnd[0],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_sft[0],
							0, 0, 0));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_C+i*8+4 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_SVAC_EPEL][i].intp[1],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_pkg[1],
							0, 0,
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_dir[1],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_rnd[1],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_sft[1],
							0, 0, 0));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_C+i*16 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][3],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][2],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][1],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_C+i*16+4 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][7],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][6],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][5],
							radix_IntpFMT[RADIX_SVAC_EPEL][i].intp_coef[0][4]));
			}
		}else{
			for(i=0; i<16; i++){
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_Y+i*8 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp[0],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_pkg[0],
							0, 0,
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_dir[0],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_rnd[0],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sft[0],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sintp[0],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_srnd[0],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sbias[0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_Y+i*8+4 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp[1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_pkg[1],
							0, 0,
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_dir[1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_rnd[1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sft[1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sintp[1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_srnd[1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_sbias[1]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][3],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][2],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16+4 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][7],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][6],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][5],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[0][4]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16+8 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][3],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][2],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][1],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_Y+i*16+12 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][7],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][6],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][5],
							radix_IntpFMT[RADIX_HEVC_QPEL][i].intp_coef[1][4]));
			}
			for(i=0; i<8; i++){
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_C+i*8 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp[0],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_pkg[0],
							0, 0,
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_dir[0],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_rnd[0],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_sft[0],
							0, 0, 0));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_ILUT_C+i*8+4 , 0, RADIX_MCE_ILUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp[1],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_pkg[1],
							0, 0,
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_dir[1],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_rnd[1],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_sft[1],
							0, 0, 0));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_C+i*16 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][3],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][2],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][1],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][0]));
				RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_SLUT_MCE_CLUT_C+i*16+4 , 0, RADIX_MCE_CLUT_INFO(radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][7],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][6],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][5],
							radix_IntpFMT[RADIX_HEVC_EPEL][i].intp_coef[0][4]));
			}
		}
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_SLC_SPOS , 0, 0);
		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_SLC_MV , 0, (RADIX_MCE_SLC_MVY(s->mce_slc_mvy) | RADIX_MCE_SLC_MVX(s->mce_slc_mvx)));

		RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_MCE_BASE_VDMA + RADIX_REG_MCE_GLB_CTRL , 0, (RADIX_MCE_GLB_CTRL_INIT |
					RADIX_MCE_GLB_CTRL_RESI(((s->tus_mce_inter32_en & 0x1) << 2) +
						((s->tus_mce_inter16_en & 0x1) << 1) +
						((s->tus_mce_inter8_en & 0x1) << 0)) |
					RADIX_MCE_GLB_CTRL_FMT(s->video_type)        |  //encode formate 0:x265, 1:svac
					RADIX_MCE_GLB_CTRL_ED(0)                     |  //0:encode, 1:decode
					RADIX_MCE_GLB_CTRL_TMVP(s->emc_mvi_en)       |  //tmvp en
					RADIX_MCE_GLB_CTRL_RMV(s->mref_en)           |  //mref en
					RADIX_MCE_GLB_CTRL_MWT(s->mref_wei)          |  //mref cost weight
					RADIX_MCE_GLB_CTRL_VTYPE(0x0)                |  //video type 0:x265
					RADIX_MCE_GLB_CTRL_SRD(s->srd_en)            |  //srd en
					RADIX_MCE_GLB_CTRL_PREF(0x1)                 |  //pref neighbour mv en
					RADIX_MCE_GLB_CTRL_FMZ(s->mvp_force_zero_en) |  //force merge zero
					RADIX_MCE_GLB_CTRL_MFS(s->mvp_force_zero_motion_shift) |  //force merge zero
					(0<<4)                                                 |  //crc_en
					RADIX_MCE_GLB_CTRL_FSAD(1)                                // FSAD_CORE1_EN
					));
	}
	/*************************************************************************************
	  STC Module
	 *************************************************************************************/

	tmp->common_slv_cfg3 = ((0x1 << 31) + //slice_init
			all_init_state[152]);
	tmp->spe_slv_cfg3    = ((0x1 << 31) +
			((s->stc_fixed_offset3 & 0xf) << 23) +
			((s->stc_fixed_offset2 & 0xf) << 19) +
			((s->stc_fixed_offset1 & 0xf) << 15) +
			((s->stc_fixed_offset0 & 0xf) << 11) +
			((s->stc_fixed_subtype & 0x1f) << 6) +
			((s->stc_fixed_mode    & 0xf)  << 2) +
			((s->stc_fixed_merge   & 0x3)  << 0) );

	tmp->stc_slv_cfg3    = s->stc_cfg_mode == 0 ? tmp->common_slv_cfg3 : tmp->spe_slv_cfg3;


	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_STC_BASE_VDMA + RADIX_REG_STC_CFG0 , 0, (((s->stc_cfg_mode & 0x3) << 30) +
				((s->frame_height & 0x1fff)<<13) +
				(s->frame_width & 0x1fff)));


	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_STC_BASE_VDMA + RADIX_REG_STC_CFG1 , 0, (((0 & 0x1) << 31) +//merge_ctrl
				((5 & 0xf) << 20) +//merge_num_h
				((5 & 0xf) << 16) +//merge_num_w
				((tmp->lcu_num_h & 0xff) << 8)) +
			(tmp->lcu_num_w & 0xff));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_STC_BASE_VDMA + RADIX_REG_STC_SAO_MG , 0, all_init_state[151]);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_STC_BASE_VDMA + RADIX_REG_STC_SAO_TP , 0, tmp->stc_slv_cfg3);
	/*************************************************************************************
	  SAO Module
	 *************************************************************************************/
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_GLB_INFO , 0, ((0 & 0x3) << 30 |/*lcu_sz*/
				((0 & 0x3) << 28) |/*lcu_sz*/
				((0 & 0x1) << 11) |/*FLUSH_SLICE*/
				(1 << 10) |/*MODE_SEL*/
				((1 & 0x1) << 9) |/*cr_tile*/
				((1 & 0x1) << 8) |/*cr_slice*/
				(0 << 7) | (0 << 6) | (0 << 5) |
				((1 & 0x1) << 4) |/*sao_y_flag*/
				((0 & 0x1) << 3) |/*sao_c_flag*/
				((1 & 0x1) << 2) |/*chroma*/
				((s->sao_en & 0x1 )<< 1) |
				(0 & 0x1)));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_PIC_SIZE , 0, ((s->frame_width & 0xffff) << 16 |
				(s->frame_height & 0xffff)));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_SLICE_XY , 0, ((0 & 0xffff) << 16 |/*slice_x*/
				(0 & 0xffff)));/*slice_y*/
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_SAO_BASE_VDMA + RADIX_REG_SAO_SLICE_INIT , 0, 1);//slice_init
	/*************************************************************************************
	  TMC Module
	 *************************************************************************************/
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_TMC_BASE_VDMA + HERA_REG_TMC_CTRL, 0, ( 0 | (0 << 1) ));

	/*************************************************************************************
	  EMC Module
	 *************************************************************************************/
	tmp->emc_cfg_en = ( 1  /* initial */
			| (s->emc_bs_en        << 1)
			| (s->emc_cps_en       << 2)
			| (s->emc_sobel_en     << 3)
			| (s->emc_mix_en       << 4)
			| (s->emc_mixc_en      << 5)
			| (s->emc_mvo_en       << 6)
			| (s->emc_flag_en      << 7)
			| (s->emc_mdc_en       << 8)
			| (s->emc_qpt_en       << 9)
			| (s->emc_mcec_en      << 10)
			| (s->emc_ipc_en       << 11)
			| (s->emc_mvi_en       << 12)
			| (s->ifa_para.rrs_en  << 13)
			| (s->ifa_para.rrs1_en << 14)
			| (s->emc_ai_en        << 15) );

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_BSFULL_EN  , 0, s->bsfull_intr_en   );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_BSFULL_SIZE, 0, s->bsfull_intr_size );

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_BS0  , 0, (int)s->emc_bs_addr0  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_BS1  , 0, (int)s->emc_bs_addr1  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_CPS  , 0, (int)s->emc_cps_addr  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_SOBEL, 0, (int)s->emc_sobel_addr);
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_MIX  , 0, (int)s->emc_mix_addr  );
	//RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_CPSC , 0, (int)s->emc_cpsc_addr );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_MIXC , 0, (int)s->emc_mixc_addr );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_MVO  , 0, (int)s->emc_mvo_addr  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_FLAG , 0, (int)s->emc_flag_addr );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_MDC  , 0, (int)s->emc_mdc_addr  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_QPT  , 0, (int)s->emc_qpt_addr  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_MCEC , 0, (int)s->emc_mcec_addr );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_IPC  , 0, (int)s->emc_ipc_addr  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_MVI  , 0, (int)s->emc_mvi_addr  );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_RRS0 , 0, (int)s->ifa_para.cps_ref0 );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_RRS1 , 0, (int)s->ifa_para.cps_ref1 );
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_AI   , 0, (int)s->emc_ai_addr );

	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_CFG       , 0, tmp->emc_cfg_en            );

	/* dbg */
	tmp->dbg_emc_flag_ram_addr = 0xC5;  //ram addr
	tmp->dbg_emc_flag_data_high = 1;    //ram data , 1 is [127:64], 0 is [63:0];
	tmp->dbg_cfg_info = ((tmp->dbg_emc_flag_data_high & 0x1) << 16 |
			(tmp->dbg_emc_flag_ram_addr & 0xffff));
	RADIX_GEN_VDMA_ACFG(chn,tmp->RADIX_EMC_BASE_VDMA + RADIX_REG_EMC_ADDR_DBG_CFG,  0, tmp->dbg_cfg_info);

	/*************************************************************************************
	  EFE Module
	 *************************************************************************************/
	tmp->cu64_en = (s->frame_type == 2) ? 0 : s->inter_mode[0][3];
	tmp->intra_en = (s->frame_type == 2) ? 1 : (s->ipred_size ? 1 : 0);
	tmp->pp_en = (s->sobel_en || s->ppred_slv_en) & tmp->intra_en;
	tmp->efe_mode = (s->frame_width <= 64*6) ? 3 : s->efe_mode;
	tmp->inter_en  = s->frame_type != 2;

	tmp->efe_extra_cfg0 = (((((~(s->frame_type == 2)) & 0x1) << 17) |
				(((0 & 0x1) << 16) |    // 0:265, 1:264
				 ((1 & 0x1) << 12))) +  // md/mce cfg parity check enable
			((s->srd_cfg & 0x3) << 5) +
			((1 & 0x1) << 4) +
			(s->ifa_para.qpg_cu32_qpm & 0x1) +
			0);

	tmp->efe_ctrl = (RADIX_EFE_CTRL_YUV(s->raw_format) |
			RADIX_EFE_CTRL_IP(tmp->intra_en) |
			RADIX_EFE_CTRL_PP(tmp->pp_en) |
			RADIX_EFE_CTRL_MCE(tmp->inter_en) |
			RADIX_EFE_CTRL_CKG(1) |
			RADIX_EFE_CTRL_SAO(s->sao_en) |
			RADIX_EFE_CTRL_WM(0) |
			RADIX_EFE_CTRL_GRAY(s->gray_en) |
			RADIX_EFE_CTRL_CRC(0) |
			RADIX_EFE_CTRL_MVC(s->emc_mvi_en) |
			RADIX_EFE_CTRL_CHNG(s->chng_en) |
			RADIX_EFE_CTRL_IPC(s->ip_cfg_en) |
			RADIX_EFE_CTRL_MDC(s->md_cfg_en) |
			RADIX_EFE_CTRL_MCEC(s->mce_cfg_en) |
			RADIX_EFE_CTRL_FMT(s->video_type) |
			RADIX_EFE_CTRL_ROTATE(s->rotate_mode) );

	tmp->rrs_cfg = (s->ifa_para.rrs_en |
			(s->ifa_para.rrs_mode << 1)  |
			(s->ifa_para.rrs_of_en << 2) |
			(s->ifa_para.motion_en << 3) |
			(s->ifa_para.rrs_thrd << 4)  |
			(s->ifa_para.motion_of_thrd << 8)  |
			(s->ifa_para.motion_thrd << 16) );
	tmp->rfh_cfg0 = (s->ifa_para.rfh_raw_thrd |
			(s->ifa_para.rfh_raw_thrd << 8) |
			(s->ifa_para.rfh_flat_thrd << 16));
	tmp->rfh_cfg1 = (s->ifa_para.refresh_en |
			(s->ifa_para.refresh_mode << 1) |
			(s->ifa_para.rfh_edge_thrd << 8) |
			(s->ifa_para.filter_thrd[0] << 16) |
			(s->ifa_para.filter_thrd[1] << 19) |
			(s->ifa_para.filter_thrd[2] << 22) |
			(s->ifa_para.filter_thrd[3] << 25) |
			(s->ifa_para.rrs_filter_mode << 28) );
	tmp->qpg_cfg  = (s->ifa_para.qpg_en |
			(s->ifa_para.qpg_table_en << 1) |
			(s->ifa_para.qpg_skin_en << 2)  |
			(s->ifa_para.qpg_ery_en << 3)   |
			(s->ifa_para.qpg_sobel_en << 4) |
			(s->ifa_para.qpg_smd_en << 5)   |
			(s->ifa_para.crp_thrd << 6)   |
			(s->ifa_para.qpg_skin_qp_ofst[0] & 0xff) << 8   |
			(s->ifa_para.qpg_skin_qp_ofst[1] & 0xff) << 16  |
			(s->ifa_para.qpg_skin_qp_ofst[2] & 0xff) << 24  );

	tmp->qpg_cfg1 = ((s->ifa_para.qpg_petbc_en    & 0x1)        |
			((s->ifa_para.pet_fil_en     & 0x1) << 1)  |
			((s->ifa_para.qpg_filte_en   & 0x1) << 2)  |
			((s->ifa_para.qpg_roi_en     & 0x1) << 3)  |
			((s->ifa_para.ai_mark_type   & 0xf) << 4)  |
			((s->ifa_para.ai_mark_en     & 0x1) << 8)  |
			((s->rc_dly_en               & 0x1) << 9)  |
			((s->rc_max_qp               & 0x3f) << 10)|
			((s->rc_min_qp               & 0x3f) << 16)|
			((s->rc_en                   & 0x1)  << 22));

	tmp->cplx_qp0  = ((s->ifa_para.qpg_cplx_qp_ofst[0] & 0xff) |
			((s->ifa_para.qpg_cplx_qp_ofst[1] & 0xff) << 8)  |
			((s->ifa_para.qpg_cplx_qp_ofst[2] & 0xff) << 16) |
			((s->ifa_para.qpg_cplx_qp_ofst[3] & 0xff) << 24) );
	tmp->cplx_qp1  = ((s->ifa_para.qpg_cplx_qp_ofst[4] & 0xff) |
			((s->ifa_para.qpg_cplx_qp_ofst[5] & 0xff) << 8)  |
			((s->ifa_para.qpg_cplx_qp_ofst[6] & 0xff) << 16) |
			((s->ifa_para.qpg_cplx_qp_ofst[7] & 0xff) << 24) );

	//petbc  zhliang
	tmp->pet_cplx_thrd00 = ((s->ifa_para.qpg_pet_cplx_thrd[0][ 3] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 2] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 1] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 0] & 0xff) << 24));
	tmp->pet_cplx_thrd01 = ((s->ifa_para.qpg_pet_cplx_thrd[0][ 7] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 6] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 5] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 4] & 0xff) << 24));
	tmp->pet_cplx_thrd02 = ((s->ifa_para.qpg_pet_cplx_thrd[0][11] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[0][10] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 9] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[0][ 8] & 0xff) << 24));

	tmp->pet_cplx_thrd10 = ((s->ifa_para.qpg_pet_cplx_thrd[1][ 3] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 2] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 1] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 0] & 0xff) << 24));
	tmp->pet_cplx_thrd11 = ((s->ifa_para.qpg_pet_cplx_thrd[1][ 7] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 6] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 5] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 4] & 0xff) << 24));
	tmp->pet_cplx_thrd12 = ((s->ifa_para.qpg_pet_cplx_thrd[1][11] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[1][10] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 9] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[1][ 8] & 0xff) << 24));

	tmp->pet_cplx_thrd20 = ((s->ifa_para.qpg_pet_cplx_thrd[2][ 3] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 2] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 1] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 0] & 0xff) << 24));
	tmp->pet_cplx_thrd21 = ((s->ifa_para.qpg_pet_cplx_thrd[2][ 7] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 6] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 5] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 4] & 0xff) << 24));
	tmp->pet_cplx_thrd22 = ((s->ifa_para.qpg_pet_cplx_thrd[2][11] & 0xff)        |
			((s->ifa_para.qpg_pet_cplx_thrd[2][10] & 0xff) <<  8) |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 9] & 0xff) << 16) |
			((s->ifa_para.qpg_pet_cplx_thrd[2][ 8] & 0xff) << 24));

	tmp->pet_qp_ofst0    =	((s->ifa_para.qpg_pet_qp_ofst[0][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst[0][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst[0][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst[0][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst[0][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst[0][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst[0][0] & 0xf ) << 24));
	tmp->pet_qp_ofst1    =	((s->ifa_para.qpg_pet_qp_ofst[1][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst[1][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst[1][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst[1][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst[1][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst[1][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst[1][0] & 0xf ) << 24));
	tmp->pet_qp_ofst2    =	((s->ifa_para.qpg_pet_qp_ofst[2][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst[2][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst[2][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst[2][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst[2][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst[2][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst[2][0] & 0xf ) << 24));
	tmp->pet_qp_ofst3    =	((s->ifa_para.qpg_pet_qp_ofst[3][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst[3][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst[3][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst[3][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst[3][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst[3][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst[3][0] & 0xf ) << 24));
	tmp->pet_qp_ofst4    =	((s->ifa_para.qpg_pet_qp_ofst[4][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst[4][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst[4][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst[4][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst[4][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst[4][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst[4][0] & 0xf ) << 24));

	tmp->pet_qp_ofst_mt0 =	((s->ifa_para.qpg_pet_qp_ofst_mt[0][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst_mt[0][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[0][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[0][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[0][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[0][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[0][0] & 0xf ) << 24));
	tmp->pet_qp_ofst_mt1 =	((s->ifa_para.qpg_pet_qp_ofst_mt[1][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst_mt[1][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[1][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[1][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[1][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[1][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[1][0] & 0xf ) << 24));
	tmp->pet_qp_ofst_mt2 =	((s->ifa_para.qpg_pet_qp_ofst_mt[2][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst_mt[2][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[2][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[2][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[2][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[2][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[2][0] & 0xf ) << 24));
	tmp->pet_qp_ofst_mt3 =	((s->ifa_para.qpg_pet_qp_ofst_mt[3][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst_mt[3][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[3][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[3][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[3][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[3][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[3][0] & 0xf ) << 24));
	tmp->pet_qp_ofst_mt4 =	((s->ifa_para.qpg_pet_qp_ofst_mt[4][6] & 0xf )          |
			((s->ifa_para.qpg_pet_qp_ofst_mt[4][5] & 0xf ) <<  4)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[4][4] & 0xf ) <<  8)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[4][3] & 0xf ) << 12)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[4][2] & 0xf ) << 16)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[4][1] & 0xf ) << 20)   |
			((s->ifa_para.qpg_pet_qp_ofst_mt[4][0] & 0xf ) << 24));

	tmp->qpg_dlt_thrd    = ((s->ifa_para.qpg_dlt_thr[0] & 0x3f)         |
			((s->ifa_para.qpg_dlt_thr[1] & 0x3f) << 6)  |
			((s->ifa_para.qpg_dlt_thr[2] & 0x3f) << 12) |
			((s->ifa_para.qpg_dlt_thr[3] & 0x3f) << 18));


	tmp->petbc_var_thr = ((s->ifa_para.petbc_var_thr[2] & 0x1ff) |
			((s->ifa_para.petbc_var_thr[1] & 0x1ff) << 9) |
			((s->ifa_para.petbc_var_thr[0] & 0x1ff) << 18));
	tmp->petbc_ssm_thr0 = ((s->ifa_para.petbc_ssm_thr[0][1] & 0xff) |
			((s->ifa_para.petbc_ssm_thr[0][0] & 0xff) << 8));
	tmp->petbc_ssm_thr1 = ((s->ifa_para.petbc_ssm_thr[1][3] & 0xff) |
			((s->ifa_para.petbc_ssm_thr[1][2] & 0xff) << 8)  |
			((s->ifa_para.petbc_ssm_thr[1][1] & 0xff) << 16) |
			((s->ifa_para.petbc_ssm_thr[1][0] & 0xff) << 24));
	tmp->petbc_ssm_thr2 = ((s->ifa_para.petbc_ssm_thr[2][3] & 0xff) |
			((s->ifa_para.petbc_ssm_thr[2][2] & 0xff) << 8)  |
			((s->ifa_para.petbc_ssm_thr[2][1] & 0xff) << 16) |
			((s->ifa_para.petbc_ssm_thr[2][0] & 0xff) << 24));

	tmp->petbc2_var_thr = ((s->ifa_para.petbc2_var_thr[2] & 0x1ff) |
			((s->ifa_para.petbc2_var_thr[1] & 0x1ff) << 9) |
			((s->ifa_para.petbc2_var_thr[0] & 0x1ff) << 18));
	tmp->petbc2_ssm_thr0 = ((s->ifa_para.petbc2_ssm_thr[0][1] & 0xff) |
			((s->ifa_para.petbc2_ssm_thr[0][0] & 0xff) << 8));
	tmp->petbc2_ssm_thr1 = ((s->ifa_para.petbc2_ssm_thr[1][3] & 0xff) |
			((s->ifa_para.petbc2_ssm_thr[1][2] & 0xff) << 8)  |
			((s->ifa_para.petbc2_ssm_thr[1][1] & 0xff) << 16) |
			((s->ifa_para.petbc2_ssm_thr[1][0] & 0xff) << 24));
	tmp->petbc2_ssm_thr2 = ((s->ifa_para.petbc2_ssm_thr[2][3] & 0xff) |
			((s->ifa_para.petbc2_ssm_thr[2][2] & 0xff) << 8)  |
			((s->ifa_para.petbc2_ssm_thr[2][1] & 0xff) << 16) |
			((s->ifa_para.petbc2_ssm_thr[2][0] & 0xff) << 24));

	tmp->petbc_filter_valid = ((s->ifa_para.pet_filter_valid[3] & 0x1) |
			((s->ifa_para.pet_filter_valid[2] & 0x1)<< 1) |
			(s->ifa_para.pet_filter_valid[1] & 0x1) << 2 |
			(s->ifa_para.pet_filter_valid[0] & 0x1) << 3 );
	tmp->ai_motion_smd_ofst0= ((s->ifa_para.qpg_ai_motion_smd_ofst[0][0][0] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][0][1] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][0][2] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][0][3] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][0][4] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst1= ((s->ifa_para.qpg_ai_motion_smd_ofst[0][0][5] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][0][6] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][0][7] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][1][0] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][1][1] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst2= ((s->ifa_para.qpg_ai_motion_smd_ofst[0][1][2] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][1][3] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][1][4] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][1][5] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[0][1][6] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst3= ((s->ifa_para.qpg_ai_motion_smd_ofst[0][1][7] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][0][0] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][0][1] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][0][2] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][0][3] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst4= ((s->ifa_para.qpg_ai_motion_smd_ofst[1][0][4] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][0][5] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][0][6] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][0][7] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][1][0] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst5= ((s->ifa_para.qpg_ai_motion_smd_ofst[1][1][1] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][1][2] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][1][3] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][1][4] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][1][5] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst6= ((s->ifa_para.qpg_ai_motion_smd_ofst[1][1][6] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[1][1][7] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][0][0] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][0][1] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][0][2] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst7= ((s->ifa_para.qpg_ai_motion_smd_ofst[2][0][3] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][0][4] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][0][5] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][0][6] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][0][7] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst8= ((s->ifa_para.qpg_ai_motion_smd_ofst[2][1][0] & 0x3f) << 24 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][1][1] & 0x3f) << 18 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][1][2] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][1][3] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][1][4] & 0x3f) <<  0 );
	tmp->ai_motion_smd_ofst9= ((s->ifa_para.qpg_ai_motion_smd_ofst[2][1][5] & 0x3f) << 12 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][1][6] & 0x3f) <<  6 |
			(s->ifa_para.qpg_ai_motion_smd_ofst[2][1][7] & 0x3f) <<  0 );

	tmp->smd_cplx_thrd0   = s->ifa_para.qpg_smd_cplx_thrd[0] | (s->ifa_para.qpg_smd_cplx_thrd[1] << 16);
	tmp->smd_cplx_thrd1   = s->ifa_para.qpg_smd_cplx_thrd[2] | (s->ifa_para.qpg_smd_cplx_thrd[3] << 16);
	tmp->smd_cplx_thrd2   = s->ifa_para.qpg_smd_cplx_thrd[4] | (s->ifa_para.qpg_smd_cplx_thrd[5] << 16);
	tmp->smd_cplx_thrd3   = s->ifa_para.qpg_smd_cplx_thrd[6] | (s->ifa_para.qpg_smd_cplx_thrd[7] << 16);

	tmp->sobel_cplx_thrd0 = s->ifa_para.qpg_sobel_cplx_thrd[0] | (s->ifa_para.qpg_sobel_cplx_thrd[1] << 16);
	tmp->sobel_cplx_thrd1 = s->ifa_para.qpg_sobel_cplx_thrd[2] | (s->ifa_para.qpg_sobel_cplx_thrd[3] << 16);
	tmp->sobel_cplx_thrd2 = s->ifa_para.qpg_sobel_cplx_thrd[4] | (s->ifa_para.qpg_sobel_cplx_thrd[5] << 16);
	tmp->sobel_cplx_thrd3 = s->ifa_para.qpg_sobel_cplx_thrd[6] | (s->ifa_para.qpg_sobel_cplx_thrd[7] << 16);

	/*
	   int cplx_thrd0 = s->ifa_para.qpg_cplx_thrd[0] | (s->ifa_para.qpg_cplx_thrd[1] << 16);
	   int cplx_thrd1 = s->ifa_para.qpg_cplx_thrd[2] | (s->ifa_para.qpg_cplx_thrd[3] << 16);
	   int cplx_thrd2 = s->ifa_para.qpg_cplx_thrd[4] | (s->ifa_para.qpg_cplx_thrd[5] << 16);
	   int cplx_thrd3 = s->ifa_para.qpg_cplx_thrd[6] | (s->ifa_para.qpg_cplx_thrd[7] << 16);
	   */
	tmp->ifa_cfg = (s->ifa_para.ifa_en |
			(s->ifa_para.cps_en << 1)   |
			(s->ifa_para.cps_mode << 2) |
			(s->ifa_para.ery_en << 3) |
			(s->ifa_para.raw_avg_en << 5) |
			(s->ifa_para.smd_en << 6) |
			(s->ifa_para.sobel_en << 7) |
			(s->ifa_para.emc_out_en[0] << 8) |
			(s->ifa_para.emc_out_en[1] << 9) |
			(s->ifa_para.skin_cnt_thrd << 10) |
			(s->ifa_para.sobel_edge_thrd << 16) |
			(s->ifa_para.skin_lvl_en[0] << 24) |
			(s->ifa_para.skin_lvl_en[1] << 25) |
			(s->ifa_para.skin_lvl_en[2] << 26) |
			(s->ifa_para.petbc_en << 27)|
			(s->ifa_para.cu_unifor_en << 28)  |
			(s->ifa_para.var_flat_en << 29)  |
			(s->var_flat_sub_size << 30) |
			(s->ifa_para.sobel_16_or_14 << 31));

	tmp->ifa_cfg1 = (s->ifa_para.nei_cplx_en       |
			(s->ifa_para.pet_mode   << 1) |
			(s->ifa_para.rrs1_en    << 2) |
			(s->ifa_para.ras_en     << 3) |
			((s->srd_motion_shift & 0x7) << 4) |
			(s->srd_en & 0x1) << 7);

	tmp->ifac_all_en = (//s->ifa_para.sobel_c_en |
						//(s->ifa_para.ery_c_en << 1) |
			(s->ifa_para.smd_c_en << 2) |
			(s->ifa_para.motion_c_en << 3) |
			(s->ifa_para.rrs_c_en << 4) |
			(s->ifa_para.cps_c_en << 5) |
			(s->ifa_para.rrs1_c_en << 6 ));

	tmp->ifac_all1_en = ((s->ifa_para.rrs_thrd_c  << 0) |
			(s->ifa_para.motion_of_thrd_c << 4) |
			(s->ifa_para.motion_thrd_c << 12));

	tmp->skin_thrd0 = (s->ifa_para.skin_thrd[0][0][1] |
			(s->ifa_para.skin_thrd[0][0][0] << 8) |
			(s->ifa_para.skin_thrd[0][1][1] << 16) |
			(s->ifa_para.skin_thrd[0][1][0] << 24) );
	tmp->skin_thrd1 = (s->ifa_para.skin_thrd[1][0][1] |
			(s->ifa_para.skin_thrd[1][0][0] << 8) |
			(s->ifa_para.skin_thrd[1][1][1] << 16) |
			(s->ifa_para.skin_thrd[1][1][0] << 24) );
	tmp->skin_thrd2 = (s->ifa_para.skin_thrd[2][0][1] |
			(s->ifa_para.skin_thrd[2][0][0] << 8) |
			(s->ifa_para.skin_thrd[2][1][1] << 16) |
			(s->ifa_para.skin_thrd[2][1][0] << 24) );
	tmp->skin_lvl0  = (s->ifa_para.skin_level[0] |
			(s->ifa_para.skin_level[1] << 16) );
	tmp->skin_lvl1  = (s->ifa_para.skin_level[2] |
			(s->ifa_para.skin_level[3] << 16) );
	tmp->skin_factor = ( s->ifa_para.skin_mul_factor[0]         |
			(s->ifa_para.skin_mul_factor[1] << 8)   |
			(s->ifa_para.skin_mul_factor[2] << 16)  |
			(s->ifa_para.skin_mul_factor[3] << 24)  );
#if 0
	for(i=0;i<16;i++){
		s->roi_info[i].roi_en = 0;
		s->roi_info[i].roi_md = 0;
		s->roi_info[i].roi_qp = 0;
		s->roi_info[i].roi_lmbx = 0;
		s->roi_info[i].roi_rmbx = 0;
		s->roi_info[i].roi_umby = 0;
		s->roi_info[i].roi_bmby = 0;
	}
#endif

	for(i=0;i<4;i++)
		tmp->roi_ctrl_cfg[i] = (s->roi_info[i*4+0].roi_md << 0 |
				(s->roi_info[i*4+0].roi_qp & 0x3f) << 2 |
				s->roi_info[i*4+1].roi_md << 8 |
				(s->roi_info[i*4+1].roi_qp & 0x3f) << 10 |
				s->roi_info[i*4+2].roi_md << 16 |
				(s->roi_info[i*4+2].roi_qp & 0x3f) << 18 |
				s->roi_info[i*4+3].roi_md << 24 |
				(s->roi_info[i*4+3].roi_qp & 0x3f) << 26 );

	tmp->qpg_roi_area_en      = (s->roi_info[ 0].roi_en <<  0|
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
		tmp->roi_pos_cfg[i] = ((s->roi_info[i].roi_lmbx & 0xff) << 0  |
				(s->roi_info[i].roi_rmbx & 0xff) << 8  |
				(s->roi_info[i].roi_umby & 0xff) << 16 |
				(s->roi_info[i].roi_bmby & 0xff) << 24 );

	tmp->frm_cplx_thrd = s->ifa_para.frm_cplx_thrd[0] | ((s->ifa_para.frm_cplx_thrd[1] & 0xffff) << 16);
	tmp->frm_motion_thrd0 = s->ifa_para.frm_motion_thrd[0] | ((s->ifa_para.frm_motion_thrd[1] & 0xffff) << 16);
	tmp->frm_motion_thrd1 = s->ifa_para.frm_motion_thrd[2] | ((s->ifa_para.frm_motion_thrd[3] & 0xffff) << 16);

	tmp->rrc_motion_thr0 = s->ifa_para.rrc_motion_thr[0] | ((s->ifa_para.rrc_motion_thr[1] & 0xffff) << 16);
	tmp->rrc_motion_thr1 = s->ifa_para.rrc_motion_thr[2] | ((s->ifa_para.rrc_motion_thr[3] & 0xffff) << 16);
	tmp->rrc_motion_thr2 = s->ifa_para.rrc_motion_thr[4] | ((s->ifa_para.rrc_motion_thr[5] & 0xffff) << 16);
	tmp->rrc_motion_thr3 = s->ifa_para.rrc_motion_thr[6] ;
	tmp->frm_flat_unifor_thrd =  ( s->var_split_thrd             |
			(s->ifa_para.var_flat_thr[0][1] << 9)   |
			((s->var_flat_sub_size ? s->ifa_para.var_flat_thr[1][0] :  s->ifa_para.var_flat_thr[0][0])  << 18)  |
			(s->ifa_para.smd_blk16_bias[0] << 27)  |
			(s->ifa_para.smd_blk16_bias[1] << 29) );
	tmp->pet_qp_idx      = ((s->ifa_para.qpg_pet_qp_idx[0] & 0x1f)        |
			((s->ifa_para.qpg_pet_qp_idx[1] & 0x1f) << 5)  |
			((s->ifa_para.qpg_pet_qp_idx[2] & 0x1f) << 10) |
			((s->ifa_para.qpg_pet_qp_idx[3] & 0x1f) << 15) |
			((s->ifa_para.qpg_pet_qp_idx[3] & 0x1f) << 20));


	tmp->frmb_ip_param0 =
		((s->frmb_ip_bias[0][0] & 0x1f) << 25) | //[29:25] 32 pla
		((s->frmb_ip_bits_en[4] & 0x1) << 24)  | //[24]
		((s->frmb_ip_bits_en[3] & 0x1) << 23)  | //[23]
		((s->frmb_ip_bits_en[2] & 0x1) << 22)  | //[22]
		((s->frmb_ip_bits_en[1] & 0x1) << 21)  | //[21]
		((s->frmb_ip_bits_en[0] & 0x1) << 20)  | //[20]
		((s->frmb_idx[4] & 0xf) << 16)	     | //[19:16]
		((s->frmb_idx[3] & 0xf) << 12)	     | //[15:12]
		((s->frmb_idx[2] & 0xf) << 8)	     | //[11:8]
		((s->frmb_idx[1] & 0xf) << 4)	     | //[7:4]
		(s->frmb_idx[0] & 0xf);		       //[3:0]

	tmp->frmb_ip_param1 =
		((s->frmb_ip_bias[3][0] & 0x1f) << 25) | //[29:25] 162 pla
		((s->frmb_ip_bias[2][1] & 0x1f) << 20) | //[24:20] 161 dc
		((s->frmb_ip_bias[2][0] & 0x1f) << 15) | //[19:15] 161 pla
		((s->frmb_ip_bias[1][1] & 0x1f) << 10) | //[14:10] 160 dc
		((s->frmb_ip_bias[1][0] & 0x1f) << 5)  | //[9:5] 160 pla
		(s->frmb_ip_bias[0][1] & 0x1f);          //[4:0] 32 dc

	tmp->frmb_ip_param2 =
		((s->frmb_ipc_bias[2] & 0x1f) << 25) | //[29:25] 16 dm1
		((s->frmb_ipc_bias[1] & 0x1f) << 20) | //[24:20] 16 dm0
		((s->frmb_ipc_bias[0] & 0x1f) << 15) | //[19:15] 32 dm
		((s->frmb_ip_bias[4][1] & 0x1f) << 10) | //[14:10] 163 dc
		((s->frmb_ip_bias[4][0] & 0x1f) << 5)  | //[9:5] 163 pla
		(s->frmb_ip_bias[3][1] & 0x1f);          //[4:0] 162 dc

	tmp->frmb_ip_param3 =
		((s->frmb_ipc_bias[4] & 0x1f) << 5)  | //[9:5] 16 dm3
		(s->frmb_ipc_bias[3] & 0x1f);          //[4:0] 16 dm2
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CTRL, 0, tmp->efe_ctrl);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_FRM_SIZE, 0,
			(((s->rotate_mode == 1 || s->rotate_mode == 3) ? s->efe_hw_width: s->efe_hw_height) << 16) |
			((s->rotate_mode == 1 || s->rotate_mode == 3) ? s->efe_hw_height : s->efe_hw_width) );

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RAWY_ADDR, 0, (int)s->raw_y_pa);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RAWC_ADDR, 0, (int)s->raw_c_pa);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RAW_STRD, 0, s->frame_y_stride | (s->frame_c_stride<<16) );
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_REF_ADDR, 0, (int)s->ifa_para.refy);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_IFAC_REF_ADDR, 0, (int)s->ifa_para.refc);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_REF1_ADDR, 0, (int)s->ifa_para.ref1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_REF1C_ADDR, 0, (int)s->ifa_para.ref1c);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_FRM_QP, 0,
			(s->frame_qp_svac | (s->ifa_para.base_qp<<8) |
			 (s->ifa_para.min_qp<<16) | (s->ifa_para.max_qp<<24)) );
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RRS_CFG, 0, tmp->rrs_cfg);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RFH_CFG0, 0, tmp->rfh_cfg0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RFH_CFG1, 0, tmp->rfh_cfg1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_QPG_CFG, 0, tmp->qpg_cfg);

	//zhliang
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_QPG_CFG1, 0, tmp->qpg_cfg1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_QPG_DLT_THRD, 0, tmp->qpg_dlt_thrd);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_IDX, 0, tmp->pet_qp_idx);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD00, 0, tmp->pet_cplx_thrd00);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD01, 0, tmp->pet_cplx_thrd01);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD02, 0, tmp->pet_cplx_thrd02);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD10, 0, tmp->pet_cplx_thrd10);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD11, 0, tmp->pet_cplx_thrd11);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD12, 0, tmp->pet_cplx_thrd12);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD20, 0, tmp->pet_cplx_thrd20);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD21, 0, tmp->pet_cplx_thrd21);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_CPLX_THRD22, 0, tmp->pet_cplx_thrd22);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST0, 0, tmp->pet_qp_ofst0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST1, 0, tmp->pet_qp_ofst1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST2, 0, tmp->pet_qp_ofst2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST3, 0, tmp->pet_qp_ofst3);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST4, 0, tmp->pet_qp_ofst4);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST_MT0, 0, tmp->pet_qp_ofst_mt0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST_MT1, 0, tmp->pet_qp_ofst_mt1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST_MT2, 0, tmp->pet_qp_ofst_mt2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST_MT3, 0, tmp->pet_qp_ofst_mt3);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_QP_OFST_MT4, 0, tmp->pet_qp_ofst_mt4);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC_VAR_THR, 0, tmp->petbc_var_thr);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC_SSM_THR0, 0, tmp->petbc_ssm_thr0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC_SSM_THR1, 0, tmp->petbc_ssm_thr1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC_SSM_THR2, 0, tmp->petbc_ssm_thr2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC2_VAR_THR, 0, tmp->petbc2_var_thr);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC2_SSM_THR0, 0, tmp->petbc2_ssm_thr0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC2_SSM_THR1, 0, tmp->petbc2_ssm_thr1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PETBC2_SSM_THR2, 0, tmp->petbc2_ssm_thr2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PET_FILTER_VALID, 0, tmp->petbc_filter_valid);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST0, 0, tmp->ai_motion_smd_ofst0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST1, 0, tmp->ai_motion_smd_ofst1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST2, 0, tmp->ai_motion_smd_ofst2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST3, 0, tmp->ai_motion_smd_ofst3);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST4, 0, tmp->ai_motion_smd_ofst4);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST5, 0, tmp->ai_motion_smd_ofst5);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST6, 0, tmp->ai_motion_smd_ofst6);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST7, 0, tmp->ai_motion_smd_ofst7);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST8, 0, tmp->ai_motion_smd_ofst8);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_AI_MT_SMD_OFST9, 0, tmp->ai_motion_smd_ofst9);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CPLX_QP0, 0, tmp->cplx_qp0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CPLX_QP1, 0, tmp->cplx_qp1);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SMD_CPLX_THRD0, 0, tmp->smd_cplx_thrd0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SMD_CPLX_THRD1, 0, tmp->smd_cplx_thrd1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SMD_CPLX_THRD2, 0, tmp->smd_cplx_thrd2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SMD_CPLX_THRD3, 0, tmp->smd_cplx_thrd3);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SOBEL_CPLX_THRD0, 0, tmp->sobel_cplx_thrd0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SOBEL_CPLX_THRD1, 0, tmp->sobel_cplx_thrd1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SOBEL_CPLX_THRD2, 0, tmp->sobel_cplx_thrd2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SOBEL_CPLX_THRD3, 0, tmp->sobel_cplx_thrd3);

	/*
	   RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CPLX_THRD0, 0, cplx_thrd0);
	   RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CPLX_THRD1, 0, cplx_thrd1);
	   RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CPLX_THRD2, 0, cplx_thrd2);
	   RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CPLX_THRD3, 0, cplx_thrd3);
	   */
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI_EN, 0, tmp->qpg_roi_area_en);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI_INFOA, 0, tmp->roi_ctrl_cfg[0]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI_INFOB, 0, tmp->roi_ctrl_cfg[1]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI_INFOC, 0, tmp->roi_ctrl_cfg[2]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI_INFOD, 0, tmp->roi_ctrl_cfg[3]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI0_POS, 0, tmp->roi_pos_cfg[0]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI1_POS, 0, tmp->roi_pos_cfg[1]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI2_POS, 0, tmp->roi_pos_cfg[2]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI3_POS, 0, tmp->roi_pos_cfg[3]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI4_POS, 0, tmp->roi_pos_cfg[4]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI5_POS, 0, tmp->roi_pos_cfg[5]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI6_POS, 0, tmp->roi_pos_cfg[6]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI7_POS, 0, tmp->roi_pos_cfg[7]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI8_POS, 0, tmp->roi_pos_cfg[8]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI9_POS, 0, tmp->roi_pos_cfg[9]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI10_POS, 0, tmp->roi_pos_cfg[10]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI11_POS, 0, tmp->roi_pos_cfg[11]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI12_POS, 0, tmp->roi_pos_cfg[12]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI13_POS, 0, tmp->roi_pos_cfg[13]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI14_POS, 0, tmp->roi_pos_cfg[14]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_ROI15_POS, 0, tmp->roi_pos_cfg[15]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_IFA_CFG, 0, tmp->ifa_cfg);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_IFA_CFG1, 0, tmp->ifa_cfg1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SKIN_FTR, 0, tmp->skin_factor);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SKIN_LVL0, 0, tmp->skin_lvl0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SKIN_LVL1, 0, tmp->skin_lvl1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SKIN_THRD0, 0, tmp->skin_thrd0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SKIN_THRD1, 0, tmp->skin_thrd1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_SKIN_THRD2, 0, tmp->skin_thrd2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_DFT_CFG0, 0, 0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_DFT_CFG1, 0, 0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_DFT_CFG2, 0, 0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_DFT_CFG3, 0, 0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_DFT_CFG4, 0, 0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_DFT_CFG4, 0, 0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_EXTRA_CFG0, 0, tmp->efe_extra_cfg0);

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_IFAC_ALL_EN, 0, tmp->ifac_all_en);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_IFAC_ALL1_EN, 0, tmp->ifac_all1_en);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_MT_THRD0, 0, tmp->frm_motion_thrd0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_MT_THRD1, 0, tmp->frm_motion_thrd1);
	//zhliang
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RRC_MOTION_THR0, 0, tmp->rrc_motion_thr0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RRC_MOTION_THR1, 0, tmp->rrc_motion_thr1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RRC_MOTION_THR2, 0, tmp->rrc_motion_thr2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RRC_MOTION_THR3, 0, tmp->rrc_motion_thr3);
	//xdzhang
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_PLAIN_UNIFOR_THRD, 0, tmp->frm_flat_unifor_thrd);
	//mfchen
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RAS_SAD_DIF_THR, 0, s->ifa_para.sobel_blk16_thr[1] << 16| s->ifa_para.sobel_blk16_thr[0]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_RAS_EDGE_CNT_THR, 0, s->ifa_para.sobel_blk16_thr[2]);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_FRMB_IP_PARAM0, 0, tmp->frmb_ip_param0);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_FRMB_IP_PARAM1, 0, tmp->frmb_ip_param1);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_FRMB_IP_PARAM2, 0, tmp->frmb_ip_param2);
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_FRMB_IP_PARAM3, 0, tmp->frmb_ip_param3);


	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_CPLX_THRD, 0, tmp->frm_cplx_thrd);
	//RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_STAT, 1, 1);
	//RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_STAT, 0, 1);//normal start vpu
	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_EFE_BASE_VDMA + RADIX_REG_EFE_STAT, 0, 1);//only read chain

	RADIX_GEN_VDMA_ACFG(chn, tmp->RADIX_VDMA_BASE_VDMA | RADIX_REG_CFGC_ACM_CTRL, RADIX_VDMA_ACFG_TERM, 0);//VDMA start

#if 0
	int cn_len = chn - s->des_va;
	printk("cn_len=%d\n", cn_len);
	unsigned int* tmp1=(unsigned int*)s->des_va;
	for (i=0; i<cn_len; i++) {
		printk("%d, 0x%x, \n",i,tmp1[i]);
	}
#endif
	}
