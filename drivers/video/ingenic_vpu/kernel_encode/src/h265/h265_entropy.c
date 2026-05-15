#include "vpu_common.h"
#include "h265_entropy.h"
#include "h265_nal.h"
#include <linux/kernel.h>

#define START_VALUE    8 // start value for dpcm mode
#define CHROMA_H_SHIFT(x) (x == CSP_I420 || x == CSP_I422)
#define CHROMA_V_SHIFT(x) (x == CSP_I420)

LevelSpec levels[] =
{
	{ 36864,    552960,     128,      0xFFFFFFFFU, 350,    0xFFFFFFFFU, 2, H265_LEVEL1,   "1",   10 },
	{ 122880,   3686400,    1500,     0xFFFFFFFFU, 1500,   0xFFFFFFFFU, 2, H265_LEVEL2,   "2",   20 },
	{ 245760,   7372800,    3000,     0xFFFFFFFFU, 3000,   0xFFFFFFFFU, 2, H265_LEVEL2_1, "2.1", 21 },
	{ 552960,   16588800,   6000,     0xFFFFFFFFU, 6000,   0xFFFFFFFFU, 2, H265_LEVEL3,   "3",   30 },
	{ 983040,   33177600,   10000,    0xFFFFFFFFU, 10000,  0xFFFFFFFFU, 2, H265_LEVEL3_1, "3.1", 31 },
	{ 2228224,  66846720,   12000,    30000,    12000,  30000,    4, H265_LEVEL4,   "4",   40 },
	{ 2228224,  133693440,  20000,    50000,    20000,  50000,    4, H265_LEVEL4_1, "4.1", 41 },
	{ 8912896,  267386880,  25000,    100000,   25000,  100000,   6, H265_LEVEL5,   "5",   50 },
	{ 8912896,  534773760,  40000,    160000,   40000,  160000,   8, H265_LEVEL5_1, "5.1", 51 },
	{ 8912896,  1069547520, 60000,    240000,   60000,  240000,   8, H265_LEVEL5_2, "5.2", 52 },
	{ 35651584, 1069547520, 60000,    240000,   60000,  240000,   8, H265_LEVEL6,   "6",   60 },
	{ 35651584, 2139095040, 120000,   480000,   120000, 480000,   8, H265_LEVEL6_1, "6.1", 61 },
	{ 35651584, 4278190080U, 240000,  800000,   240000, 800000,   6, H265_LEVEL6_2, "6.2", 62 },
	{ 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 1, H265_LEVEL8_5, "8.5", 85 },
};

void codeProfileTier(Bitstream *bs, const ProfileTierLevel *ptl, int maxTempSubLayers)
{
	int i = 0,j = 0;
	WRITE_CODE(bs, 0, 2,                "XXX_profile_space[]");
	WRITE_FLAG(bs, ptl->tierFlag,        "XXX_tier_flag[]");
	WRITE_CODE(bs, ptl->profileIdc, 5,   "XXX_profile_idc[]");
	for (j = 0; j < 32; j++)
		WRITE_FLAG(bs, ptl->profileCompatibilityFlag[j], "XXX_profile_compatibility_flag[][j]");

	WRITE_FLAG(bs, ptl->progressiveSourceFlag,   "general_progressive_source_flag");
	WRITE_FLAG(bs, ptl->interlacedSourceFlag,    "general_interlaced_source_flag");
	WRITE_FLAG(bs, ptl->nonPackedConstraintFlag, "general_non_packed_constraint_flag");
	WRITE_FLAG(bs, ptl->frameOnlyConstraintFlag, "general_frame_only_constraint_flag");

	if (ptl->profileIdc == H265_PROFILE_MAINREXT || ptl->profileIdc == H265_PROFILE_HIGHTHROUGHPUTREXT)
	{
		unsigned int bitDepthConstraint = ptl->bitDepthConstraint;
		int csp = ptl->chromaFormatConstraint;
		WRITE_FLAG(bs, bitDepthConstraint<=12, "general_max_12bit_constraint_flag");
		WRITE_FLAG(bs, bitDepthConstraint<=10, "general_max_10bit_constraint_flag");
		WRITE_FLAG(bs, bitDepthConstraint<= 8 && csp != CSP_I422 , "general_max_8bit_constraint_flag");
		WRITE_FLAG(bs, csp == CSP_I422 || csp == CSP_I420 || csp == CSP_I400, "general_max_422chroma_constraint_flag");
		WRITE_FLAG(bs, csp == CSP_I420 || csp == CSP_I400,                         "general_max_420chroma_constraint_flag");
		WRITE_FLAG(bs, csp == CSP_I400,                                                 "general_max_monochrome_constraint_flag");
		WRITE_FLAG(bs, ptl->intraConstraintFlag,        "general_intra_constraint_flag");
		WRITE_FLAG(bs, ptl->onePictureOnlyConstraintFlag,"general_one_picture_only_constraint_flag");
		WRITE_FLAG(bs, ptl->lowerBitRateConstraintFlag, "general_lower_bit_rate_constraint_flag");
		WRITE_CODE(bs, 0 , 16, "XXX_reserved_zero_35bits[0..15]");
		WRITE_CODE(bs, 0 , 16, "XXX_reserved_zero_35bits[16..31]");
		WRITE_CODE(bs, 0 ,  3, "XXX_reserved_zero_35bits[32..34]");
	}
	else
	{
		WRITE_CODE(bs, 0, 16, "XXX_reserved_zero_44bits[0..15]");
		WRITE_CODE(bs, 0, 16, "XXX_reserved_zero_44bits[16..31]");
		WRITE_CODE(bs, 0, 12, "XXX_reserved_zero_44bits[32..43]");
	}

	WRITE_CODE(bs, ptl->levelIdc, 8, "general_level_idc");

	if (maxTempSubLayers > 1)
	{
		WRITE_FLAG(bs, 0, "sub_layer_profile_present_flag[i]");
		WRITE_FLAG(bs, 0, "sub_layer_level_present_flag[i]");
		for (i = maxTempSubLayers - 1; i < 8 ; i++)
			WRITE_CODE(bs, 0, 2, "reserved_zero_2bits");
	}
}

void codeHrdParameters(Bitstream *bs, const HRDInfo *hrd, int maxSubTLayers)
{
	int i = 0;
	WRITE_FLAG(bs, 1, "nal_hrd_parameters_present_flag");
	WRITE_FLAG(bs, 0, "vcl_hrd_parameters_present_flag");
	WRITE_FLAG(bs, 0, "sub_pic_hrd_params_present_flag");

	WRITE_CODE(bs, hrd->bitRateScale, 4, "bit_rate_scale");
	WRITE_CODE(bs, hrd->cpbSizeScale, 4, "cpb_size_scale");

	WRITE_CODE(bs, hrd->initialCpbRemovalDelayLength - 1, 5, "initial_cpb_removal_delay_length_minus1");
	WRITE_CODE(bs, hrd->cpbRemovalDelayLength - 1,        5, "au_cpb_removal_delay_length_minus1");
	WRITE_CODE(bs, hrd->dpbOutputDelayLength - 1,         5, "dpb_output_delay_length_minus1");

	for (i = 0; i < maxSubTLayers; i++)
	{
		WRITE_FLAG(bs, 1, "fixed_pic_rate_general_flag");
		WRITE_UVLC(bs, 0, "elemental_duration_in_tc_minus1");
		WRITE_UVLC(bs, 0, "cpb_cnt_minus1");

		WRITE_UVLC(bs, hrd->bitRateValue - 1, "bit_rate_value_minus1");
		WRITE_UVLC(bs, hrd->cpbSizeValue - 1, "cpb_size_value_minus1");
		WRITE_FLAG(bs, hrd->cbrFlag, "cbr_flag");
	}
}

void codeVUI(Bitstream *bs, const VUI *vui, int maxSubTLayers, bool bEmitVUITimingInfo, bool bEmitVUIHRDInfo)
{
	WRITE_FLAG(bs, vui->aspectRatioInfoPresentFlag, "aspect_ratio_info_present_flag");
	if (vui->aspectRatioInfoPresentFlag)
	{
		WRITE_CODE(bs, vui->aspectRatioIdc, 8, "aspect_ratio_idc");
		if (vui->aspectRatioIdc == 255)
		{
			WRITE_CODE(bs, vui->sarWidth, 16, "sar_width");
			WRITE_CODE(bs, vui->sarHeight, 16, "sar_height");
		}
	}

	WRITE_FLAG(bs, vui->overscanInfoPresentFlag, "overscan_info_present_flag");
	if (vui->overscanInfoPresentFlag)
		WRITE_FLAG(bs, vui->overscanAppropriateFlag, "overscan_appropriate_flag");

	WRITE_FLAG(bs, vui->videoSignalTypePresentFlag, "video_signal_type_present_flag");
	if (vui->videoSignalTypePresentFlag)
	{
		WRITE_CODE(bs, vui->videoFormat, 3, "video_format");
		WRITE_FLAG(bs, vui->videoFullRangeFlag, "video_full_range_flag");
		WRITE_FLAG(bs, vui->colourDescriptionPresentFlag, "colour_description_present_flag");
		if (vui->colourDescriptionPresentFlag)
		{
			WRITE_CODE(bs, vui->colourPrimaries, 8, "colour_primaries");
			WRITE_CODE(bs, vui->transferCharacteristics, 8, "transfer_characteristics");
			WRITE_CODE(bs, vui->matrixCoefficients, 8, "matrix_coefficients");
		}
	}

	WRITE_FLAG(bs, vui->chromaLocInfoPresentFlag, "chroma_loc_info_present_flag");
	if (vui->chromaLocInfoPresentFlag)
	{
		WRITE_UVLC(bs, vui->chromaSampleLocTypeTopField, "chroma_sample_loc_type_top_field");
		WRITE_UVLC(bs, vui->chromaSampleLocTypeBottomField, "chroma_sample_loc_type_bottom_field");
	}

	WRITE_FLAG(bs, 0, "neutral_chroma_indication_flag");
	WRITE_FLAG(bs, vui->fieldSeqFlag, "field_seq_flag");
	WRITE_FLAG(bs, vui->frameFieldInfoPresentFlag, "frame_field_info_present_flag");

	WRITE_FLAG(bs, vui->defaultDisplayWindow.bEnabled, "default_display_window_flag");
	if (vui->defaultDisplayWindow.bEnabled)
	{
		WRITE_UVLC(bs, vui->defaultDisplayWindow.leftOffset, "def_disp_win_left_offset");
		WRITE_UVLC(bs, vui->defaultDisplayWindow.rightOffset, "def_disp_win_right_offset");
		WRITE_UVLC(bs, vui->defaultDisplayWindow.topOffset, "def_disp_win_top_offset");
		WRITE_UVLC(bs, vui->defaultDisplayWindow.bottomOffset, "def_disp_win_bottom_offset");
	}

	if (!bEmitVUITimingInfo)
		WRITE_FLAG(bs, 0, "vui_timing_info_present_flag");
	else
	{
		WRITE_FLAG(bs, 1, "vui_timing_info_present_flag");
		WRITE_CODE(bs, vui->timingInfo.numUnitsInTick, 32, "vui_num_units_in_tick");
		WRITE_CODE(bs, vui->timingInfo.timeScale, 32, "vui_time_scale");
		WRITE_FLAG(bs, 0, "vui_poc_proportional_to_timing_flag");
	}

	if (!bEmitVUIHRDInfo)
		WRITE_FLAG(bs, 0, "vui_hrd_parameters_present_flag");
	else
	{
		WRITE_FLAG(bs, vui->hrdParametersPresentFlag, "vui_hrd_parameters_present_flag");
		if (vui->hrdParametersPresentFlag)
			codeHrdParameters(bs, &vui->hrdParameters, maxSubTLayers);
	}

	WRITE_FLAG(bs, 0, "bitstream_restriction_flag");
}

void _codeScalingList(Bitstream *bs, const ScalingList *scalingList, unsigned int sizeId, unsigned int listId)
{
	//TODO
#if 0
	int i = 0;
	int coefNum = VPU_MIN(64, (int)ScalingList::s_numCoefPerSize[sizeId]);
	const unsigned short* scan = (sizeId == 0 ? g_scan4x4[SCAN_DIAG] : g_scan8x8diag);
	int nextCoef = START_VALUE;
	int *src = scalingList->m_scalingListCoef[sizeId][listId];
	int data;

	if (sizeId > BLOCK_8x8)
	{
		WRITE_SVLC(bs, scalingList->m_scalingListDC[sizeId][listId] - 8, "scaling_list_dc_coef_minus8");
		nextCoef = scalingList->m_scalingListDC[sizeId][listId];
	}
	for (i = 0; i < coefNum; i++)
	{
		data = src[scan[i]] - nextCoef;
		if (data < -128)
			data += 256;
		if (data > 127)
			data -= 256;
		nextCoef = (nextCoef + data + 256) % 256;
		WRITE_SVLC(bs, data,  "scaling_list_delta_coef");
	}
#endif
}

void codeScalingList(Bitstream *bs, const ScalingList *scalingList)
{
	//TODO
#if 0
	for (int sizeId = 0; sizeId < 4; sizeId++)
	{
		for (int listId = 0; listId < 6; listId += (sizeId == 3) ? 3 : 1)
		{
			int predList = scalingList.checkPredMode(sizeId, listId);
			WRITE_FLAG(bs, predList < 0, "scaling_list_pred_mode_flag");
			if (predList >= 0)
				WRITE_UVLC(bs, listId - predList, "scaling_list_pred_matrix_id_delta");
			else // DPCM Mode
				_codeScalingList(bs, scalingList, sizeId, listId);
		}
	}
#endif
}

void codeShortTermRefPicSet(Bitstream *bs, const RPS *rps, int idx)
{
	int j = 0;
	int prev = 0;
	if (idx > 0)
		WRITE_FLAG(bs, 0, "inter_ref_pic_set_prediction_flag");

	WRITE_UVLC(bs, rps->numberOfNegativePictures, "num_negative_pics");
	WRITE_UVLC(bs, rps->numberOfPositivePictures, "num_positive_pics");
	for (j = 0; j < rps->numberOfNegativePictures; j++)
	{
		WRITE_UVLC(bs, prev - rps->deltaPOC[j] - 1, "delta_poc_s0_minus1");
		prev = rps->deltaPOC[j];
		WRITE_FLAG(bs, rps->bUsed[j], "used_by_curr_pic_s0_flag");
	}

	prev = 0;
	for (j = rps->numberOfNegativePictures; j < rps->numberOfNegativePictures + rps->numberOfPositivePictures; j++)
	{
		WRITE_UVLC(bs, rps->deltaPOC[j] - prev - 1, "delta_poc_s1_minus1");
		prev = rps->deltaPOC[j];
		WRITE_FLAG(bs, rps->bUsed[j], "used_by_curr_pic_s1_flag");
	}
}

/* code explicit wp tables */
void codePredWeightTable(Bitstream *bs, const Slice *slice)
{
	int list = 0,ref = 0,plane = 0;
	const WeightParam *wp;
	bool            bChroma = slice->m_sps->chromaFormatIdc != CSP_I400;
	bool            bDenomCoded  = false;
	int             numRefDirs   = slice->m_sliceType == B_SLICE ? 2 : 1;
	unsigned int        totalSignalledWeightFlags = 0;

	if ((slice->m_sliceType == P_SLICE && slice->m_pps->bUseWeightPred) ||
			(slice->m_sliceType == B_SLICE && slice->m_pps->bUseWeightedBiPred))
	{
		for (list = 0; list < numRefDirs; list++)
		{
			for (ref = 0; ref < slice->m_numRefIdx[list]; ref++)
			{
				wp = slice->m_weightPredTable[list][ref];
				if (!bDenomCoded)
				{
					WRITE_UVLC(bs, wp[0].log2WeightDenom, "luma_log2_weight_denom");

					if (bChroma)
					{
						int deltaDenom = wp[1].log2WeightDenom - wp[0].log2WeightDenom;
						WRITE_SVLC(bs, deltaDenom, "delta_chroma_log2_weight_denom");
					}
					bDenomCoded = true;
				}
				WRITE_FLAG(bs, !!wp[0].wtPresent, "luma_weight_lX_flag");
				totalSignalledWeightFlags += wp[0].wtPresent;
			}

			if (bChroma)
			{
				for (ref = 0; ref < slice->m_numRefIdx[list]; ref++)
				{
					wp = slice->m_weightPredTable[list][ref];
					WRITE_FLAG(bs, !!wp[1].wtPresent, "chroma_weight_lX_flag");
					totalSignalledWeightFlags += 2 * wp[1].wtPresent;
				}
			}

			for (ref = 0; ref < slice->m_numRefIdx[list]; ref++)
			{
				wp = slice->m_weightPredTable[list][ref];
				if (wp[0].wtPresent)
				{
					int deltaWeight = (wp[0].inputWeight - (1 << wp[0].log2WeightDenom));
					WRITE_SVLC(bs, deltaWeight, "delta_luma_weight_lX");
					WRITE_SVLC(bs, wp[0].inputOffset, "luma_offset_lX");
				}

				if (bChroma)
				{
					if (wp[1].wtPresent)
					{
						for (plane = 1; plane < 3; plane++)
						{
							int deltaWeight = (wp[plane].inputWeight - (1 << wp[1].log2WeightDenom));
							WRITE_SVLC(bs, deltaWeight, "delta_chroma_weight_lX");

							int pred = (128 - ((128 * wp[plane].inputWeight) >> (wp[plane].log2WeightDenom)));
							int deltaChroma = (wp[plane].inputOffset - pred);
							WRITE_SVLC(bs, deltaChroma, "delta_chroma_offset_lX");
						}
					}
				}
			}
		}

		VPU_CHECK(totalSignalledWeightFlags <= 24, "total weights must be <= 24\n");
	}
}

void codeVPS(Bitstream *bs, const VPS *vps)
{
	int i = 0;
	WRITE_CODE(bs, 0,       4, "vps_video_parameter_set_id");
	WRITE_CODE(bs, 3,       2, "vps_reserved_three_2bits");
	WRITE_CODE(bs, 0,       6, "vps_reserved_zero_6bits");
	WRITE_CODE(bs, vps->maxTempSubLayers - 1, 3, "vps_max_sub_layers_minus1");
	WRITE_FLAG(bs, vps->maxTempSubLayers == 1,   "vps_temporal_id_nesting_flag");
	WRITE_CODE(bs, 0xffff, 16, "vps_reserved_ffff_16bits");

	codeProfileTier(bs, &vps->ptl, vps->maxTempSubLayers);

	WRITE_FLAG(bs, true, "vps_sub_layer_ordering_info_present_flag");

	for (i = 0; i < vps->maxTempSubLayers; i++)
	{
		WRITE_UVLC(bs, vps->maxDecPicBuffering - 1, "vps_max_dec_pic_buffering_minus1[i]");
		WRITE_UVLC(bs, vps->numReorderPics,         "vps_num_reorder_pics[i]");
		WRITE_UVLC(bs, vps->maxLatencyIncrease + 1, "vps_max_latency_increase_plus1[i]");
	}

	WRITE_CODE(bs, 0, 6, "vps_max_nuh_reserved_zero_layer_id");
	WRITE_UVLC(bs, 0,    "vps_max_op_sets_minus1");
	WRITE_FLAG(bs, 0,    "vps_timing_info_present_flag"); /* we signal timing info in SPS-VUI */
	WRITE_FLAG(bs, 0,    "vps_extension_flag");
}

void codeSPS(Bitstream *bs, const SPS *sps, const ScalingList *scalingList, const ProfileTierLevel *ptl)
{
	int i = 0;
	const Window *conf = &sps->conformanceWindow;
	WRITE_CODE(bs, 0, 4, "sps_video_parameter_set_id");
	WRITE_CODE(bs, sps->maxTempSubLayers - 1, 3, "sps_max_sub_layers_minus1");
	WRITE_FLAG(bs, sps->maxTempSubLayers == 1,   "sps_temporal_id_nesting_flag");

	codeProfileTier(bs, ptl, sps->maxTempSubLayers);

	WRITE_UVLC(bs, 0, "sps_seq_parameter_set_id");
	WRITE_UVLC(bs, sps->chromaFormatIdc, "chroma_format_idc");

	if (sps->chromaFormatIdc == CSP_I444)
		WRITE_FLAG(bs, 0,                       "separate_colour_plane_flag");

	WRITE_UVLC(bs, sps->picWidthInLumaSamples,   "pic_width_in_luma_samples");
	WRITE_UVLC(bs, sps->picHeightInLumaSamples,  "pic_height_in_luma_samples");

	WRITE_FLAG(bs, conf->bEnabled, "conformance_window_flag");
	if (conf->bEnabled)
	{
		int hShift = CHROMA_H_SHIFT(sps->chromaFormatIdc), vShift = CHROMA_V_SHIFT(sps->chromaFormatIdc);
		WRITE_UVLC(bs, conf->leftOffset   >> hShift, "conf_win_left_offset");
		WRITE_UVLC(bs, conf->rightOffset  >> hShift, "conf_win_right_offset");
		WRITE_UVLC(bs, conf->topOffset    >> vShift, "conf_win_top_offset");
		WRITE_UVLC(bs, conf->bottomOffset >> vShift, "conf_win_bottom_offset");
	}

	WRITE_UVLC(bs, BIT_DEPTH - 8,   "bit_depth_luma_minus8");
	WRITE_UVLC(bs, BIT_DEPTH - 8,   "bit_depth_chroma_minus8");
	WRITE_UVLC(bs, sps->log2MaxPocLsb - 4, "log2_max_pic_order_cnt_lsb_minus4");
	WRITE_FLAG(bs, true,             "sps_sub_layer_ordering_info_present_flag");

	for (i = 0; i < sps->maxTempSubLayers; i++)
	{
		WRITE_UVLC(bs, sps->maxDecPicBuffering - 1, "sps_max_dec_pic_buffering_minus1[i]");
		WRITE_UVLC(bs, sps->numReorderPics,         "sps_num_reorder_pics[i]");
		WRITE_UVLC(bs, sps->maxLatencyIncrease + 1, "sps_max_latency_increase_plus1[i]");
	}

	WRITE_UVLC(bs, sps->log2MinCodingBlockSize - 3,    "log2_min_coding_block_size_minus3");
	WRITE_UVLC(bs, sps->log2DiffMaxMinCodingBlockSize, "log2_diff_max_min_coding_block_size");
	WRITE_UVLC(bs, sps->quadtreeTULog2MinSize - 2,     "log2_min_transform_block_size_minus2");
	WRITE_UVLC(bs, sps->quadtreeTULog2MaxSize - sps->quadtreeTULog2MinSize, "log2_diff_max_min_transform_block_size");
	WRITE_UVLC(bs, sps->quadtreeTUMaxDepthInter - 1,   "max_transform_hierarchy_depth_inter");
	WRITE_UVLC(bs, sps->quadtreeTUMaxDepthIntra - 1,   "max_transform_hierarchy_depth_intra");
	WRITE_FLAG(bs, scalingList->m_bEnabled,            "scaling_list_enabled_flag");
	if (scalingList->m_bEnabled)
	{
		//default disable
#if 0
		WRITE_FLAG(bs, scalingList->m_bDataPresent,    "sps_scaling_list_data_present_flag");
		if (scalingList->m_bDataPresent)
			codeScalingList(bs, scalingList);
#endif
	}
	WRITE_FLAG(bs, sps->bUseAMP, "amp_enabled_flag");
	WRITE_FLAG(bs, sps->bUseSAO, "sample_adaptive_offset_enabled_flag");

	WRITE_FLAG(bs, 0, "pcm_enabled_flag");
	WRITE_UVLC(bs, sps->spsrpsNum, "num_short_term_ref_pic_sets");
	for (i = 0; i < sps->spsrpsNum; i++)
		codeShortTermRefPicSet(bs, sps->spsrps + i, i);
	WRITE_FLAG(bs, 0, "long_term_ref_pics_present_flag");

	WRITE_FLAG(bs, sps->bTemporalMVPEnabled, "sps_temporal_mvp_enable_flag");
	WRITE_FLAG(bs, sps->bUseStrongIntraSmoothing, "sps_strong_intra_smoothing_enable_flag");

	WRITE_FLAG(bs, 1, "vui_parameters_present_flag");
	codeVUI(bs, &sps->vuiParameters, sps->maxTempSubLayers, sps->bEmitVUITimingInfo, sps->bEmitVUIHRDInfo);

	WRITE_FLAG(bs, 0, "sps_extension_flag");
}

void codePPS(Bitstream *bs, const PPS *pps, bool filerAcross, int iPPSInitQpMinus26 )
{
	WRITE_UVLC(bs, 0,                          "pps_pic_parameter_set_id");
	WRITE_UVLC(bs, 0,                          "pps_seq_parameter_set_id");
	WRITE_FLAG(bs, 0,                          "dependent_slice_segments_enabled_flag");
	WRITE_FLAG(bs, 0,                          "output_flag_present_flag");
	WRITE_CODE(bs, 0, 3,                       "num_extra_slice_header_bits");
	WRITE_FLAG(bs, pps->bSignHideEnabled,       "sign_data_hiding_flag");
	WRITE_FLAG(bs, 0,                          "cabac_init_present_flag");
	WRITE_UVLC(bs, pps->numRefIdxDefault[0] - 1, "num_ref_idx_l0_default_active_minus1");
	WRITE_UVLC(bs, pps->numRefIdxDefault[1] - 1, "num_ref_idx_l1_default_active_minus1");

	WRITE_SVLC(bs, iPPSInitQpMinus26,         "init_qp_minus26");
	WRITE_FLAG(bs, pps->bConstrainedIntraPred, "constrained_intra_pred_flag");
	WRITE_FLAG(bs, pps->bTransformSkipEnabled, "transform_skip_enabled_flag");

	WRITE_FLAG(bs, pps->bUseDQP,                "cu_qp_delta_enabled_flag");
	if (pps->bUseDQP)
		WRITE_UVLC(bs, pps->maxCuDQPDepth,      "diff_cu_qp_delta_depth");

	WRITE_SVLC(bs, pps->chromaQpOffset[0],      "pps_cb_qp_offset");
	WRITE_SVLC(bs, pps->chromaQpOffset[1],      "pps_cr_qp_offset");
	WRITE_FLAG(bs, pps->pps_slice_chroma_qp_offsets_present_flag, "pps_slice_chroma_qp_offsets_present_flag");

	WRITE_FLAG(bs, pps->bUseWeightPred,            "weighted_pred_flag");
	WRITE_FLAG(bs, pps->bUseWeightedBiPred,        "weighted_bipred_flag");
	WRITE_FLAG(bs, pps->bTransquantBypassEnabled,  "transquant_bypass_enable_flag");
	WRITE_FLAG(bs, 0,                             "tiles_enabled_flag");
	WRITE_FLAG(bs, pps->bEntropyCodingSyncEnabled, "entropy_coding_sync_enabled_flag");
	WRITE_FLAG(bs, filerAcross,                   "loop_filter_across_slices_enabled_flag");

	WRITE_FLAG(bs, pps->bDeblockingFilterControlPresent, "deblocking_filter_control_present_flag");
	if (pps->bDeblockingFilterControlPresent)
	{
		WRITE_FLAG(bs, 0,                               "deblocking_filter_override_enabled_flag");
		WRITE_FLAG(bs, pps->bPicDisableDeblockingFilter, "pps_disable_deblocking_filter_flag");
		if (!pps->bPicDisableDeblockingFilter)
		{
			WRITE_SVLC(bs, pps->deblockingFilterBetaOffsetDiv2, "pps_beta_offset_div2");
			WRITE_SVLC(bs, pps->deblockingFilterTcOffsetDiv2,   "pps_tc_offset_div2");
		}
	}

	WRITE_FLAG(bs, 0, "pps_scaling_list_data_present_flag");
	WRITE_FLAG(bs, 0, "lists_modification_present_flag");
	WRITE_UVLC(bs, 0, "log2_parallel_merge_level_minus2");
	WRITE_FLAG(bs, 0, "slice_segment_header_extension_present_flag");
	WRITE_FLAG(bs, 0, "pps_extension_flag");
}

void codeSliceHeader(Bitstream *bs, const Slice *slice, unsigned int maxSlices, unsigned int slice_addr, unsigned int slice_addr_bits, int sliceQp)
{
	int numBits = 0;
	int code = 0;
	WRITE_FLAG(bs, (slice_addr == 0 ? 1 : 0), "first_slice_segment_in_pic_flag");
	if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
			slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP ||
			slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA)
		WRITE_FLAG(bs, 0, "no_output_of_prior_pics_flag");

	WRITE_UVLC(bs, 0, "slice_pic_parameter_set_id");

	/* x265 does not use dependent slices, so always write all this data */
	if (slice_addr)
	{
		// if( dependent_slice_segments_enabled_flag )
		//     dependent_slice_segment_flag             u(1)
		WRITE_CODE(bs, slice_addr, slice_addr_bits, "slice_segment_address");
	}

	WRITE_UVLC(bs, slice->m_sliceType, "slice_type");

	if (!(slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
				slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP))
	{
		int picOrderCntLSB = (slice->m_poc - slice->m_lastIDR + (1 << slice->m_sps->log2MaxPocLsb)) % (1 << slice->m_sps->log2MaxPocLsb);
		WRITE_CODE(bs, picOrderCntLSB, slice->m_sps->log2MaxPocLsb, "pic_order_cnt_lsb");

		if (slice->m_rpsIdx < 0)
		{
			WRITE_FLAG(bs, 0, "short_term_ref_pic_set_sps_flag");
			codeShortTermRefPicSet(bs, &slice->m_rps, slice->m_sps->spsrpsNum);
		}
		else
		{
			WRITE_FLAG(bs, 1, "short_term_ref_pic_set_sps_flag");
			numBits = 0;
			while ((1 << numBits) < slice->m_iNumRPSInSPS)
				numBits++;

			if (numBits > 0)
				WRITE_CODE(bs, slice->m_rpsIdx, numBits, "short_term_ref_pic_set_idx");
		}

		if (slice->m_sps->bTemporalMVPEnabled)
			WRITE_FLAG(bs, 1, "slice_temporal_mvp_enable_flag");
	}
	if (slice->m_bUseSao)
	{
		//WRITE_FLAG(bs, encData->bSaoFlag[0], "slice_sao_luma_flag");
		WRITE_FLAG(bs, true, "slice_sao_luma_flag");
		//if (encData->internalCsp != CSP_I400)
		//WRITE_FLAG(bs, encData->bSaoFlag[1], "slice_sao_chroma_flag");
		WRITE_FLAG(bs, false, "slice_sao_chroma_flag");
	}
#if 0
	else if(encData->selectiveSAO)
	{
		WRITE_FLAG(bs, 0, "slice_sao_luma_flag");
		if (encData->internalCsp != CSP_I400)
			WRITE_FLAG(bs, 0, "slice_sao_chroma_flag");
	}
#endif

	// check if numRefIdx match the defaults (1, hard-coded in PPS). If not, override
	// TODO: this might be a place to optimize a few bits per slice, by using param->refs for L0 default

	if (!(slice->m_sliceType == I_SLICE))
	{
		bool overrideFlag = (slice->m_numRefIdx[0] != slice->numRefIdxDefault[0] || (slice->m_sliceType == B_SLICE && slice->m_numRefIdx[1] != slice->numRefIdxDefault[1]));
		WRITE_FLAG(bs, overrideFlag, "num_ref_idx_active_override_flag");
		if (overrideFlag)
		{
			WRITE_UVLC(bs, slice->m_numRefIdx[0] - 1, "num_ref_idx_l0_active_minus1");
			if (slice->m_sliceType == B_SLICE)
				WRITE_UVLC(bs, slice->m_numRefIdx[1] - 1, "num_ref_idx_l1_active_minus1");
			else
			{
				VPU_CHECK(slice->m_numRefIdx[1] == 0, "expected no L1 references for P slice\n");
			}
		}
	}
	else
	{
		VPU_CHECK(!slice->m_numRefIdx[0] && !slice->m_numRefIdx[1], "expected no references for I slice\n");
	}

	if (slice->m_sliceType == B_SLICE)
		WRITE_FLAG(bs, 0, "mvd_l1_zero_flag");

	if (slice->m_sps->bTemporalMVPEnabled)
	{
		if (slice->m_sliceType == B_SLICE)
			WRITE_FLAG(bs, slice->m_colFromL0Flag, "collocated_from_l0_flag");

		if (slice->m_sliceType != I_SLICE &&
				((slice->m_colFromL0Flag && slice->m_numRefIdx[0] > 1) ||
				 (!slice->m_colFromL0Flag && slice->m_numRefIdx[1] > 1)))
		{
			WRITE_UVLC(bs, slice->m_colRefIdx, "collocated_ref_idx");
		}
	}
	if ((slice->m_pps->bUseWeightPred && slice->m_sliceType == P_SLICE) || (slice->m_pps->bUseWeightedBiPred && slice->m_sliceType == B_SLICE))
		codePredWeightTable(bs, slice);

#define MRG_MAX_NUM_CANDS   5
	VPU_CHECK(slice->m_maxNumMergeCand <= MRG_MAX_NUM_CANDS, "too many merge candidates\n");
	if (!(slice->m_sliceType == I_SLICE))
		WRITE_UVLC(bs, MRG_MAX_NUM_CANDS - slice->m_maxNumMergeCand, "five_minus_max_num_merge_cand");

	code = sliceQp - (slice->m_iPPSQpMinus26 + 26);
	WRITE_SVLC(bs, code, "slice_qp_delta");

	if (slice->m_pps->pps_slice_chroma_qp_offsets_present_flag)
	{
		WRITE_SVLC(bs, slice->m_chromaQpOffset[0], "slice_cb_qp_offset");
		WRITE_SVLC(bs, slice->m_chromaQpOffset[1], "slice_cr_qp_offset");
	}
	// TODO: Enable when pps_loop_filter_across_slices_enabled_flag==1
	//       We didn't support filter across slice board, so disable it now

	if (maxSlices <= 1)
	{
		//bool isSAOEnabled = slice->m_sps->bUseSAO && slice->m_bUseSao ? encData->bSaoFlag[0] || encData->bSaoFlag[1] : false;
		//bool isDBFEnabled = !slice->m_pps->bPicDisableDeblockingFilter;
		bool isSAOEnabled = slice->m_sps->bUseSAO;
		bool isDBFEnabled = !slice->m_pps->bPicDisableDeblockingFilter;

		if (isSAOEnabled || isDBFEnabled)
			WRITE_FLAG(bs, slice->m_sLFaseFlag, "slice_loop_filter_across_slices_enabled_flag");
	}
}

void sortDeltaPOC(RPS *rps)
{
	int j = 0,k = 0;
	int dPOC = 0,used = 0;
	int temp = 0;
	int numNegPics = 0;
	// sort in increasing order (smallest first)
	for (j = 1; j < rps->numberOfNegativePictures + rps->numberOfPositivePictures; j++)
	{
		dPOC = rps->deltaPOC[j];
		used = rps->bUsed[j];
		for (k = j - 1; k >= 0; k--)
		{
			temp = rps->deltaPOC[k];
			if (dPOC < temp)
			{
				rps->deltaPOC[k + 1] = temp;
				rps->bUsed[k + 1] = rps->bUsed[k];
				rps->deltaPOC[k] = dPOC;
				rps->bUsed[k] = used;
			}
		}
	}

	// flip the negative values to largest first
	numNegPics = rps->numberOfNegativePictures;
	for (j = 0, k = numNegPics - 1; j < numNegPics >> 1; j++, k--)
	{
		dPOC = rps->deltaPOC[j];
		used = rps->bUsed[j];
		rps->deltaPOC[j] = rps->deltaPOC[k];
		rps->bUsed[j] = rps->bUsed[k];
		rps->deltaPOC[k] = dPOC;
		rps->bUsed[k] = used;
	}
}

void sortPocLSBLT(RPS *rps)
{
	int j = 0,k = 0;
	// sort in decreasing order of pocLSBLT and increasing order of deltaPOCMSBCycleLT
	int offset = rps->numberOfNegativePictures + rps->numberOfPositivePictures;

	for (j = offset + 1; j < rps->numberOfPictures; j++)
	{
		int  jpoc = rps->poc[j];
		int  jdeltaPOC = rps->deltaPOC[j];
		bool jbUsed = rps->bUsed[j];
		int  jpocLSBLT = rps->pocLSBLT[j];
		int  jpocMSBLT = rps->pocMSBLT[j];
		int  jdeltaPOCMSBCycleLT = rps->deltaPOCMSBCycleLT[j];
		bool jdeltaPocMSBPresentFlag = rps->deltaPocMSBPresentFlag[j];

		for (k = j - 1; k >= offset; k--)
		{
			if ((rps->pocLSBLT[k] < jpocLSBLT) || ((rps->pocLSBLT[k] == jpocLSBLT) && (rps->deltaPOCMSBCycleLT[k] > jdeltaPOCMSBCycleLT)))
			{
				rps->poc[k + 1] = rps->poc[k];
				rps->deltaPOC[k + 1] = rps->deltaPOC[k];
				rps->bUsed[k + 1] = rps->bUsed[k];
				rps->pocLSBLT[k + 1] = rps->pocLSBLT[k];
				rps->pocMSBLT[k + 1] = rps->pocMSBLT[k];
				rps->deltaPOCMSBCycleLT[k + 1] = rps->deltaPOCMSBCycleLT[k];
				rps->deltaPocMSBPresentFlag[k + 1] = rps->deltaPocMSBPresentFlag[k];

				rps->poc[k] = jpoc;
				rps->deltaPOC[k] = jdeltaPOC;
				rps->bUsed[k] = jbUsed;
				rps->pocLSBLT[k] = jpocLSBLT;
				rps->pocMSBLT[k] = jpocMSBLT;
				rps->deltaPOCMSBCycleLT[k] = jdeltaPOCMSBCycleLT;
				rps->deltaPocMSBPresentFlag[k] = jdeltaPocMSBPresentFlag;
			}
		}
	}
}

void computeRPS(Slice *slice)
{
	unsigned int refNum = 0;
	//we dont have long ref
	int m_blongRef = 0;
	unsigned int i = 0;
	int cur = 0;
	//int maxPicOrderCntLSB = 1 << slice->m_sps->log2MaxPocLsb;
	int m_poc = slice->m_poc;
	//int curLSB = m_poc % maxPicOrderCntLSB;
	//int curMSB = m_poc - curLSB;
	RPS *m_rps = &slice->m_rps;

	//we only have one slice per frame
	if (slice->m_sliceType == I_SLICE || !m_poc) {
		refNum = 0;
	} else {
		refNum = 1;
	}
	memset(m_rps, 0, sizeof(RPS));
	/* short term picture values are sorted with deltaPoc(ve) value, and -ve values before the +ve values,
	 * -ve values are in decreasing order, +ve values are in increasing order */
	for (i = 0; i < refNum; i++) {
		if (!m_blongRef || m_poc == 1) {
			m_rps->poc[cur] = m_poc - 1;
			m_rps->bUsed[cur] = !(slice->m_nalUnitType >= 16 && slice->m_nalUnitType <= 23);
			m_rps->deltaPOC[cur] = m_rps->poc[cur] - m_poc;
			(m_rps->deltaPOC[cur] < 0) ? m_rps->numberOfNegativePictures++ : m_rps->numberOfPositivePictures++;
			m_rps->numberOfPictures++;
			cur++;
		}
	}

#if 0
	/* long term picture in the decreasing order of pocLSBLT and among the pictures with the same lsb,
	 * it arranges them in increasing deltaPOCMSBCycleLT */
	for (i = 0; i < refNum; i++) {
		if (m_blongRef && m_poc > 1) {
			m_rps.poc[cur] = fref[i]->m_poc;
			m_rps.bUsed[cur] = !isIRAP();
			m_rps.deltaPOC[cur] = m_rps.poc[cur] - m_poc;
			m_rps.pocLSBLT[cur] = getLSB(m_rps.poc[cur], maxPicOrderCntLSB);
			m_rps.pocMSBLT[cur] = m_rps.poc[cur] - m_rps.pocLSBLT[cur];
			m_rps.deltaPOCMSBCycleLT[cur] = (curMSB - m_rps.pocMSBLT[cur]) / maxPicOrderCntLSB;
			m_rps.deltaPocMSBPresentFlag[cur] = false;
			m_rps.numberOfLongtermPictures++;
			m_rps.numberOfPictures++;
			cur++;
		}
	}
#endif
	sortDeltaPOC(m_rps);
	sortPocLSBLT(m_rps);

	return;
}

void defaultVPS(VPS *vps, struct h265_param *param)
{
	unsigned int lumaSamples = 0;
	unsigned int samplesPerSec = 0;
	unsigned int bitrate = 0;
	unsigned int NumLevels = 0;
	unsigned int MaxDpbPicBuf = 6;
	unsigned int maxDpbSize = 0;
	int i = 0;

	memset(vps, 0, sizeof(VPS));
	vps->hrdParameters.bitRateScale = 0;
	vps->hrdParameters.cpbSizeScale = 0;
	vps->hrdParameters.initialCpbRemovalDelayLength = 1;
	vps->hrdParameters.cpbRemovalDelayLength = 1;
	vps->hrdParameters.dpbOutputDelayLength = 1;
	vps->hrdParameters.bitRateValue = 0;
	vps->hrdParameters.cpbSizeValue = 0;
	vps->hrdParameters.cbrFlag = 0;

	//default main profile
	vps->maxDecPicBuffering = 2;
	vps->ptl.profileIdc = H265_PROFILE_MAIN;
	lumaSamples = param->width * param->height;
	samplesPerSec = (unsigned int)(lumaSamples * (param->fps / param->fps_den));
	bitrate = 0;//fixqp
	NumLevels = sizeof(levels) / sizeof(levels[0]);
	for (i = 0; i < NumLevels; i ++) {
		if (lumaSamples > levels[i].maxLumaSamples)
			continue;
		else if (samplesPerSec > levels[i].maxLumaSamplesPerSecond)
			continue;
		else if (bitrate > levels[i].maxBitrateMain && levels[i].maxBitrateHigh == 0xFFFFFFFFU)
			continue;
		else if (bitrate > levels[i].maxBitrateHigh)
			continue;
		else if (param->width > int_sqrt(levels[i].maxLumaSamples * 8))
			continue;
		else if (param->height > int_sqrt(levels[i].maxLumaSamples * 8))
			continue;
		maxDpbSize = MaxDpbPicBuf;

		if (lumaSamples <= (levels[i].maxLumaSamples >> 2))
			maxDpbSize = C_MIN(4 * MaxDpbPicBuf, 16);
		else if (lumaSamples <= (levels[i].maxLumaSamples >> 1))
			maxDpbSize = C_MIN(2 * MaxDpbPicBuf, 16);
		else if (lumaSamples <= ((3 * levels[i].maxLumaSamples) >> 2))
			maxDpbSize = C_MIN((4 * MaxDpbPicBuf) / 3, 16);

		/* The value of sps_max_dec_pic_buffering_minus1[ HighestTid ] + 1 shall be less than
		 * or equal to MaxDpbSize */
		if (vps->maxDecPicBuffering > maxDpbSize)
			continue;

#if 0
		/* For level 5 and higher levels, the value of CtbSizeY shall be equal to 32 or 64 */
		if (levels[i].levelEnum >= Level::LEVEL5 && param.maxCUSize < 32)
		{
			x265_log(&param, X265_LOG_WARNING, "level %s detected, but CTU size 16 is non-compliant\n", levels[i].name);
			vps.ptl.profileIdc = Profile::NONE;
			vps.ptl.levelIdc = Level::NONE;
			vps.ptl.tierFlag = Level::MAIN;
			x265_log(&param, X265_LOG_INFO, "NONE profile, Level-NONE (Main tier)\n");
			return;
		}

		/* The value of NumPocTotalCurr shall be less than or equal to 8 */
		int numPocTotalCurr = param.maxNumReferences + vps.numReorderPics;
		if (numPocTotalCurr > 8)
		{
			x265_log(&param, X265_LOG_WARNING, "level %s detected, but NumPocTotalCurr (total references) is non-compliant\n", levels[i].name);
			vps.ptl.profileIdc = Profile::NONE;
			vps.ptl.levelIdc = Level::NONE;
			vps.ptl.tierFlag = Level::MAIN;
			x265_log(&param, X265_LOG_INFO, "NONE profile, Level-NONE (Main tier)\n");
			return;
		}
#endif
		vps->ptl.levelIdc = levels[i].levelEnum;
		vps->ptl.minCrForLevel = levels[i].minCompressionRatio;
		vps->ptl.maxLumaSrForLevel = levels[i].maxLumaSamplesPerSecond;
		break;
	}
	//vps->ptl.levelIdc = 0;
	//vps->ptl.minCrForLevel = 4;
	//vps->ptl.maxLumaSrForLevel = 66846720;

	//vps->ptl.levelIdc = 120;
	vps->ptl.minCrForLevel = 4;
	vps->ptl.maxLumaSrForLevel = 66846720;
	vps->ptl.bitDepthConstraint = 8;
	vps->ptl.chromaFormatConstraint = 1;
	vps->ptl.tierFlag = H265_TIER_MAIN;
	//disable interlace mode
	vps->ptl.progressiveSourceFlag = 1;
	vps->ptl.interlacedSourceFlag = 0;
	vps->ptl.nonPackedConstraintFlag = 0;
	vps->ptl.frameOnlyConstraintFlag = 1;
	vps->ptl.profileCompatibilityFlag[1] = 1;
	vps->ptl.profileCompatibilityFlag[2] = 1;
	//default multi frames
	vps->ptl.intraConstraintFlag = 0;
	vps->ptl.onePictureOnlyConstraintFlag = 0;
	vps->ptl.lowerBitRateConstraintFlag = 1;

	vps->maxTempSubLayers = 1;
	vps->numReorderPics = 0;
	vps->maxLatencyIncrease = 0;
	return;
}

void defaultSPS(SPS *sps, struct h265_param *param)
{
	//conformanceWindow
	memset(sps, 0, sizeof(SPS));
	sps->chromaFormatIdc = CSP_I420;
	sps->picWidthInLumaSamples = C_ALIGN(param->width, 8);
	sps->picHeightInLumaSamples = C_ALIGN(param->height, 8);

	sps->numCuInWidth = (param->width + 64 - 1) / 64;
	sps->numCuInHeight = (param->height + 64 - 1) / 64;
	sps->numCUsInFrame = sps->numCuInWidth * sps->numCuInHeight;
	sps->numPartitions = 256;
	sps->numPartInCUSize = 16;

	sps->log2MinCodingBlockSize = 3;
	sps->log2DiffMaxMinCodingBlockSize = 3;
	sps->log2MaxPocLsb = 8;

	sps->quadtreeTULog2MaxSize = 5;
	sps->quadtreeTULog2MinSize = 2;

	sps->quadtreeTUMaxDepthInter = 1; // use param
	sps->quadtreeTUMaxDepthIntra = 1; // use param

	sps->maxAMPDepth = 0;

	sps->maxTempSubLayers = 1;   // max number of Temporal Sub layers
	sps->maxDecPicBuffering = 2; // these are dups of VPS values
	sps->maxLatencyIncrease = 0;
	sps->numReorderPics = 0;

	//RPS
	sps->spsrpsNum = 0;
	sps->numGOPBegin = 0;

	sps->bUseSAO = 0; // use param
	sps->bUseAMP = 0; // use param
	sps->bUseStrongIntraSmoothing = 0; // use param
	sps->bTemporalMVPEnabled = 0;
	sps->bEmitVUITimingInfo = 1;
	sps->bEmitVUIHRDInfo = 1;

	if (param->width % 8 || param->height % 8) {
		sps->conformanceWindow.leftOffset = 0;
		sps->conformanceWindow.rightOffset = C_ALIGN(param->width, 8) - param->width;
		sps->conformanceWindow.topOffset = 0;
		sps->conformanceWindow.bottomOffset = C_ALIGN(param->height, 8) - param->height;
		sps->conformanceWindow.bEnabled = 1;
	} else {
		sps->conformanceWindow.leftOffset = 0;
		sps->conformanceWindow.rightOffset = 0;
		sps->conformanceWindow.topOffset = 0;
		sps->conformanceWindow.bottomOffset = 0;
		sps->conformanceWindow.bEnabled = 0;
	}

	sps->vuiParameters.aspectRatioIdc = 0;
	sps->vuiParameters.sarWidth = 0;
	sps->vuiParameters.sarHeight = 0;
	sps->vuiParameters.videoFormat = 5;
	sps->vuiParameters.colourPrimaries = 2;;
	sps->vuiParameters.transferCharacteristics = 2;
	sps->vuiParameters.matrixCoefficients = 2;
	sps->vuiParameters.chromaSampleLocTypeTopField = 0;
	sps->vuiParameters.chromaSampleLocTypeBottomField = 0;
	sps->vuiParameters.aspectRatioInfoPresentFlag = 0;
	sps->vuiParameters.overscanInfoPresentFlag = 0;
	sps->vuiParameters.overscanAppropriateFlag = 0;
	sps->vuiParameters.videoSignalTypePresentFlag = 0;
	sps->vuiParameters.videoFullRangeFlag = 0;
	sps->vuiParameters.colourDescriptionPresentFlag = 0;
	sps->vuiParameters.chromaLocInfoPresentFlag = 0;
	sps->vuiParameters.frameFieldInfoPresentFlag = 0;
	sps->vuiParameters.fieldSeqFlag = 0;
	sps->vuiParameters.hrdParametersPresentFlag = 0;
	sps->vuiParameters.defaultDisplayWindow.leftOffset = 0;
	sps->vuiParameters.defaultDisplayWindow.rightOffset = 0;
	sps->vuiParameters.defaultDisplayWindow.topOffset = 0;
	sps->vuiParameters.defaultDisplayWindow.bottomOffset = 0;
	sps->vuiParameters.defaultDisplayWindow.bEnabled = 0;

	sps->vuiParameters.timingInfo.numUnitsInTick = param->fps_den;
	sps->vuiParameters.timingInfo.timeScale = param->fps;
	return;
}

void defaultPPS(PPS *pps, struct h265_param *param)
{
	memset(pps, 0, sizeof(PPS));
	pps->maxCuDQPDepth = 2;
	pps->chromaQpOffset[0] = 0;      // use param
	pps->chromaQpOffset[1] = 0;      // use param
	pps->deblockingFilterBetaOffsetDiv2 = 0;
	pps->deblockingFilterTcOffsetDiv2 = 0;

	pps->bUseWeightPred = 0;         // use param
	pps->bUseWeightedBiPred = 0;     // use param
	pps->bUseDQP = 1;
	pps->bConstrainedIntraPred = 0;  // use param

	pps->bTransquantBypassEnabled = 0;  // Indicates presence of cu_transquant_bypass_flag in CUs.
	pps->bTransformSkipEnabled = 0;     // use param
	pps->bEntropyCodingSyncEnabled = 0; // use param
	pps->bSignHideEnabled = 0;          // use param

	pps->bDeblockingFilterControlPresent = 0;
	pps->bPicDisableDeblockingFilter = 0;

	pps->numRefIdxDefault[0] = 1;
	pps->numRefIdxDefault[1] = 1;
	pps->pps_slice_chroma_qp_offsets_present_flag = 0;
	return;
}

#if 0
void defaultSlice(Slice *slice, struct h265_param *param, SPS *sps, PPS *pps)
{
	slice->m_sps = sps;
	slice->m_pps = pps;

	slice->m_iPPSQpMinus26 = 0;
	slice->m_iNumRPSInSPS = 0;
	return;
}
#endif

void dumpNal(NALList *list)
{
	int i = 0,j = 0;

	printk("m_numNal=%d\n", list->m_numNal);
	for (i = 0; i < list->m_numNal; i ++) {
		printk("nal type = %d\n", list->m_nal[i].type);
		for (j = 0; j < list->m_nal[i].sizeBytes; j ++) {
			printk("%02x ", list->m_nal[i].payload[j]);
		}
		printk("\n");
	}
}

void getStreamHeaders(NALU *nalu, NALList *list, Bitstream *bs)
{
	VPS *m_vps = &nalu->m_vps;
	SPS *m_sps = &nalu->m_sps;
	PPS *m_pps = &nalu->m_pps;
	ScalingList m_scalingList;

	resetBits(bs);
	codeVPS(bs, m_vps);
	writeByteAlignment(bs);
	serialize(list, NAL_UNIT_VPS, bs);

	m_scalingList.m_bEnabled = 0;
	resetBits(bs);
	codeSPS(bs, m_sps, &m_scalingList, &m_vps->ptl);
	writeByteAlignment(bs);
	serialize(list, NAL_UNIT_SPS, bs);

	resetBits(bs);
	//codePPS(bs, m_pps, (m_param->maxSlices <= 1), m_iPPSQpMinus26);
	codePPS(bs, m_pps, 1, 0);
	writeByteAlignment(bs);
	serialize(list, NAL_UNIT_PPS, bs);

	//dumpNal(list);
}

int getStream(NALU *nalu, NALList *list, Bitstream *bs, void *addr, int len)
{
	Slice *slice = &nalu->m_slice;
	resetBits(bs);
	codeSliceHeader(bs, slice, 1, 0, 0, slice->m_sliceQp);
	serializeSubstreams(list, addr, len);
	writeByteAlignment(bs);
	return serialize(list, slice->m_nalUnitType, bs);
}

int getStreamSegment(NALU *nalu, NALList *list, Bitstream *bs, void *addr, int len, int count, int is_final)
{
	Slice *slice = &nalu->m_slice;
	int ret = 0;
	if (count == 0) {
		resetBits(bs);
		codeSliceHeader(bs, slice, 1, 0, 0, slice->m_sliceQp);
		writeByteAlignment(bs);
		ret = serialize(list, slice->m_nalUnitType, bs);
		if (ret < 0) {
			return ret;
		}
	}
	return serializeSegment(list, addr, len, is_final);
}
