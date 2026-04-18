#ifndef _H265_SLICE_H_
#define _H265_SLICE_H_
#include <stdbool.h>

#define MAX_NUM_SHORT_TERM_RPS      64 // max. number of short term reference picture set in SPS
#define MAX_NUM_REF                 16 // max. number of entries in picture reference list
#define MAX_NUM_REF_PICS            16 // max. number of pictures used for reference

#define CSP_I400           0  /* yuv 4:0:0 planar */
#define CSP_I420           1  /* yuv 4:2:0 planar */
#define CSP_I422           2  /* yuv 4:2:2 planar */
#define CSP_I444           3  /* yuv 4:4:4 planar */
#define CSP_COUNT          4  /* Number of supported internal color spaces */

#define BIT_DEPTH          8

typedef enum
{
	B_SLICE,
	P_SLICE,
	I_SLICE
} SliceType;

typedef enum
{
	NAL_UNIT_CODED_SLICE_TRAIL_N = 0,
	NAL_UNIT_CODED_SLICE_TRAIL_R,
	NAL_UNIT_CODED_SLICE_TSA_N,
	NAL_UNIT_CODED_SLICE_TLA_R,
	NAL_UNIT_CODED_SLICE_STSA_N,
	NAL_UNIT_CODED_SLICE_STSA_R,
	NAL_UNIT_CODED_SLICE_RADL_N,
	NAL_UNIT_CODED_SLICE_RADL_R,
	NAL_UNIT_CODED_SLICE_RASL_N,
	NAL_UNIT_CODED_SLICE_RASL_R,
	NAL_UNIT_CODED_SLICE_BLA_W_LP = 16,
	NAL_UNIT_CODED_SLICE_BLA_W_RADL,
	NAL_UNIT_CODED_SLICE_BLA_N_LP,
	NAL_UNIT_CODED_SLICE_IDR_W_RADL,
	NAL_UNIT_CODED_SLICE_IDR_N_LP,
	NAL_UNIT_CODED_SLICE_CRA,
	NAL_UNIT_VPS = 32,
	NAL_UNIT_SPS,
	NAL_UNIT_PPS,
	NAL_UNIT_ACCESS_UNIT_DELIMITER,
	NAL_UNIT_EOS,
	NAL_UNIT_EOB,
	NAL_UNIT_FILLER_DATA,
	NAL_UNIT_PREFIX_SEI,
	NAL_UNIT_SUFFIX_SEI,
	NAL_UNIT_UNSPECIFIED = 62,
	NAL_UNIT_INVALID = 64,
} NalUnitType;

typedef enum
{
	H265_PROFILE_NONE = 0,
	H265_PROFILE_MAIN = 1,
	H265_PROFILE_MAIN10 = 2,
	H265_PROFILE_MAINSTILLPICTURE = 3,
	H265_PROFILE_MAINREXT = 4,
	H265_PROFILE_HIGHTHROUGHPUTREXT = 5
} PROFILE;

typedef enum
{
	H265_TIER_MAIN = 0,
	H265_TIER_HIGH = 1,
} Tier;

typedef enum
{
	H265_LEVEL_NONE = 0,
	H265_LEVEL1 = 30,
	H265_LEVEL2 = 60,
	H265_LEVEL2_1 = 63,
	H265_LEVEL3 = 90,
	H265_LEVEL3_1 = 93,
	H265_LEVEL4 = 120,
	H265_LEVEL4_1 = 123,
	H265_LEVEL5 = 150,
	H265_LEVEL5_1 = 153,
	H265_LEVEL5_2 = 156,
	H265_LEVEL6 = 180,
	H265_LEVEL6_1 = 183,
	H265_LEVEL6_2 = 186,
	H265_LEVEL8_5 = 255,
} Level;

typedef struct
{
	unsigned int maxLumaSamples;
	unsigned int maxLumaSamplesPerSecond;
	unsigned int maxBitrateMain;
	unsigned int maxBitrateHigh;
	unsigned int maxCpbSizeMain;
	unsigned int maxCpbSizeHigh;
	unsigned int minCompressionRatio;
	Level levelEnum;
	char name[16];
	int levelIdc;
} LevelSpec;

typedef struct WeightParam
{
	// Explicit weighted prediction parameters parsed in slice header,
	unsigned int log2WeightDenom;
	int      inputWeight;
	int      inputOffset;
	int      wtPresent;
} WeightParam;

typedef struct RPS
{
	int  numberOfPictures;
	int  numberOfNegativePictures;
	int  numberOfPositivePictures;
	int  numberOfLongtermPictures;

	int  poc[MAX_NUM_REF_PICS];
	int  deltaPOC[MAX_NUM_REF_PICS];
	bool bUsed[MAX_NUM_REF_PICS];
	int  pocLSBLT[MAX_NUM_REF_PICS];
	int  pocMSBLT[MAX_NUM_REF_PICS];
	int  deltaPOCMSBCycleLT[MAX_NUM_REF_PICS];
	bool deltaPocMSBPresentFlag[MAX_NUM_REF_PICS];
} RPS;

typedef struct ScalingList
{
#if 0
	int     s_numCoefPerSize[4];
	int s_invQuantScales[6];
	int s_quantScales[6];
	char MatrixType[4][6][20];
	char MatrixType_DC[4][12][22];

	int  m_scalingListDC[4][6];   // the DC value of the matrix coefficient for 16x16
	int* m_scalingListCoef[4][6]; // quantization matrix

	int* m_quantCoef[4][6][6];   // array of quantization matrix coefficient 4x4
	int* m_dequantCoef[4][6][6]; // array of dequantization matrix coefficient 4x4

#endif
	bool     m_bEnabled;
	bool     m_bDataPresent; // non-default scaling lists must be signaled

	/* used during SPS coding */
	//int      checkPredMode(int sizeId, int listId) const;
} ScalingList;

typedef struct FrameData
{
	bool           bSaoFlag[2];
	int            internalCsp;
	int            selectiveSAO;
	unsigned int   maxSlices;
} FrameData;

#if 0
enum Name
{
	NONE = 0,
	MAIN = 1,
	MAIN10 = 2,
	MAINSTILLPICTURE = 3,
	MAINREXT = 4,
	HIGHTHROUGHPUTREXT = 5
};

enum Tier
{
	MAIN = 0,
	HIGH = 1,
};

enum Name
{
	NONE = 0,
	LEVEL1 = 30,
	LEVEL2 = 60,
	LEVEL2_1 = 63,
	LEVEL3 = 90,
	LEVEL3_1 = 93,
	LEVEL4 = 120,
	LEVEL4_1 = 123,
	LEVEL5 = 150,
	LEVEL5_1 = 153,
	LEVEL5_2 = 156,
	LEVEL6 = 180,
	LEVEL6_1 = 183,
	LEVEL6_2 = 186,
	LEVEL8_5 = 255,
};
#endif

typedef struct ProfileTierLevel
{
	int      profileIdc;
	int      levelIdc;
	unsigned int minCrForLevel;
	unsigned int maxLumaSrForLevel;
	unsigned int bitDepthConstraint;
	int      chromaFormatConstraint;
	bool     tierFlag;
	bool     progressiveSourceFlag;
	bool     interlacedSourceFlag;
	bool     nonPackedConstraintFlag;
	bool     frameOnlyConstraintFlag;
	bool     profileCompatibilityFlag[32];
	bool     intraConstraintFlag;
	bool     onePictureOnlyConstraintFlag;
	bool     lowerBitRateConstraintFlag;
} ProfileTierLevel;

typedef struct HRDInfo
{
	unsigned int bitRateScale;
	unsigned int cpbSizeScale;
	unsigned int initialCpbRemovalDelayLength;
	unsigned int cpbRemovalDelayLength;
	unsigned int dpbOutputDelayLength;
	unsigned int bitRateValue;
	unsigned int cpbSizeValue;
	bool     cbrFlag;
} HRDInfo;

typedef struct TimingInfo
{
	unsigned int numUnitsInTick;
	unsigned int timeScale;
} TimingInfo;

typedef struct Window
{
	int  leftOffset;
	int  rightOffset;
	int  topOffset;
	int  bottomOffset;
	bool bEnabled;
} Window;

typedef struct VUI
{
	int        aspectRatioIdc;
	int        sarWidth;
	int        sarHeight;
	int        videoFormat;
	int        colourPrimaries;
	int        transferCharacteristics;
	int        matrixCoefficients;
	int        chromaSampleLocTypeTopField;
	int        chromaSampleLocTypeBottomField;

	bool       aspectRatioInfoPresentFlag;
	bool       overscanInfoPresentFlag;
	bool       overscanAppropriateFlag;
	bool       videoSignalTypePresentFlag;
	bool       videoFullRangeFlag;
	bool       colourDescriptionPresentFlag;
	bool       chromaLocInfoPresentFlag;
	bool       frameFieldInfoPresentFlag;
	bool       fieldSeqFlag;
	bool       hrdParametersPresentFlag;

	HRDInfo    hrdParameters;
	Window     defaultDisplayWindow;
	TimingInfo timingInfo;
} VUI;

typedef struct VPS
{
	HRDInfo          hrdParameters;
	ProfileTierLevel ptl;
	unsigned int         maxTempSubLayers;
	unsigned int         numReorderPics;
	unsigned int         maxDecPicBuffering;
	unsigned int         maxLatencyIncrease;
} VPS;

typedef struct SPS
{
	/* cached PicYuv offset arrays, shared by all instances of
	 * PicYuv created by this encoder */
	unsigned int* cuOffsetY;
	unsigned int* cuOffsetC;
	unsigned int* buOffsetY;
	unsigned int* buOffsetC;

	int      chromaFormatIdc;        // use param
	unsigned int picWidthInLumaSamples;  // use param
	unsigned int picHeightInLumaSamples; // use param

	unsigned int numCuInWidth;
	unsigned int numCuInHeight;
	unsigned int numCUsInFrame;
	unsigned int numPartitions;
	unsigned int numPartInCUSize;

	int      log2MinCodingBlockSize;
	int      log2DiffMaxMinCodingBlockSize;
	int      log2MaxPocLsb;

	unsigned int quadtreeTULog2MaxSize;
	unsigned int quadtreeTULog2MinSize;

	unsigned int quadtreeTUMaxDepthInter; // use param
	unsigned int quadtreeTUMaxDepthIntra; // use param

	unsigned int maxAMPDepth;

	unsigned int maxTempSubLayers;   // max number of Temporal Sub layers
	unsigned int maxDecPicBuffering; // these are dups of VPS values
	unsigned int maxLatencyIncrease;
	int      numReorderPics;

	RPS      spsrps[MAX_NUM_SHORT_TERM_RPS];
	int      spsrpsNum;
	int      numGOPBegin;

	bool     bUseSAO; // use param
	bool     bUseAMP; // use param
	bool     bUseStrongIntraSmoothing; // use param
	bool     bTemporalMVPEnabled;
	bool     bEmitVUITimingInfo;
	bool     bEmitVUIHRDInfo;

	Window   conformanceWindow;
	VUI      vuiParameters;
} SPS;

typedef struct PPS
{
	unsigned int maxCuDQPDepth;

	int      chromaQpOffset[2];      // use param
	int      deblockingFilterBetaOffsetDiv2;
	int      deblockingFilterTcOffsetDiv2;

	bool     bUseWeightPred;         // use param
	bool     bUseWeightedBiPred;     // use param
	bool     bUseDQP;
	bool     bConstrainedIntraPred;  // use param

	bool     bTransquantBypassEnabled;  // Indicates presence of cu_transquant_bypass_flag in CUs.
	bool     bTransformSkipEnabled;     // use param
	bool     bEntropyCodingSyncEnabled; // use param
	bool     bSignHideEnabled;          // use param

	bool     bDeblockingFilterControlPresent;
	bool     bPicDisableDeblockingFilter;

	int      numRefIdxDefault[2];
	bool     pps_slice_chroma_qp_offsets_present_flag;
} PPS;

typedef struct Slice
{
	SPS*  m_sps;
	PPS*  m_pps;

	WeightParam m_weightPredTable[2][MAX_NUM_REF][3]; // [list][refIdx][0:Y, 1:U, 2:V]
	RPS         m_rps;

	NalUnitType m_nalUnitType;
	SliceType   m_sliceType;
	int         m_chromaQpOffset[2];
	int         m_poc;
	int         m_lastIDR;
	int         m_rpsIdx;

	unsigned int    m_colRefIdx;       // never modified

	int         m_numRefIdx[2];

	unsigned int    m_maxNumMergeCand; // use param

	bool        m_sLFaseFlag;      // loop filter boundary flag
	bool        m_colFromL0Flag;   // collocated picture from List0 or List1 flag
	int         m_bUseSao;

	int         m_iPPSQpMinus26;
	int         numRefIdxDefault[2];
	int         m_iNumRPSInSPS;
	int         m_sliceQp;
} Slice;

typedef struct NALU
{
	NalUnitType m_nalUnitType;
	VPS m_vps;
	SPS m_sps;
	PPS m_pps;
	Slice m_slice;
} NALU;

#endif // _H265_SLICE_H_
