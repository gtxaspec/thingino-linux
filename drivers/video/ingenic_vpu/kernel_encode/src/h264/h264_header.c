#include "vpu_common.h"
#include "h264_bitstream.h"
#include "h264_header.h"
#include "h264_slice.h"
#include "h264_macroblock.h"

#define bs_write_ue bs_write_ue_big
#define C_BIT_DEPTH 8
#define QP_BD_OFFSET 0
#define I264E_REF_MAX 2
#define I264E_CQM_FLAT 0

const i264e_level_t i264e_levels[] =
{
	{ 10,    1485,    99,    396,     64,    175,  64, 64,  0, 2, 0, 0, 1 },
	{  9,    1485,    99,    396,    128,    350,  64, 64,  0, 2, 0, 0, 1 }, /* "1b" */
	{ 11,    3000,   396,    900,    192,    500, 128, 64,  0, 2, 0, 0, 1 },
	{ 12,    6000,   396,   2376,    384,   1000, 128, 64,  0, 2, 0, 0, 1 },
	{ 13,   11880,   396,   2376,    768,   2000, 128, 64,  0, 2, 0, 0, 1 },
	{ 20,   11880,   396,   2376,   2000,   2000, 128, 64,  0, 2, 0, 0, 1 },
	{ 21,   19800,   792,   4752,   4000,   4000, 256, 64,  0, 2, 0, 0, 0 },
	{ 22,   20250,  1620,   8100,   4000,   4000, 256, 64,  0, 2, 0, 0, 0 },
	{ 30,   40500,  1620,   8100,  10000,  10000, 256, 32, 22, 2, 0, 1, 0 },
	{ 31,  108000,  3600,  18000,  14000,  14000, 512, 16, 60, 4, 1, 1, 0 },
	{ 32,  216000,  5120,  20480,  20000,  20000, 512, 16, 60, 4, 1, 1, 0 },
	{ 40,  245760,  8192,  32768,  20000,  25000, 512, 16, 60, 4, 1, 1, 0 },
	{ 41,  245760,  8192,  32768,  50000,  62500, 512, 16, 24, 2, 1, 1, 0 },
	{ 42,  522240,  8704,  34816,  50000,  62500, 512, 16, 24, 2, 1, 1, 1 },
	{ 50,  589824, 22080, 110400, 135000, 135000, 512, 16, 24, 2, 1, 1, 1 },
	{ 51,  983040, 36864, 184320, 240000, 240000, 512, 16, 24, 2, 1, 1, 1 },
	{ 52, 2073600, 36864, 184320, 240000, 240000, 512, 16, 24, 2, 1, 1, 1 },
	{ 0 }
};

static void scaling_list_write( bs_t *s, i264e_pps_t *pps, int idx )
{
	int j = 0;
	const int len = idx < 4 ? 16 : 64;
	const unsigned char *zigzag = idx<4 ? i264e_zigzag_scan4[0] : i264e_zigzag_scan8[0];
	const unsigned char *list = pps->scaling_list[idx];
	const unsigned char *def_list = (idx==CQM_4IC) ? pps->scaling_list[CQM_4IY]
		: (idx==CQM_4PC) ? pps->scaling_list[CQM_4PY]
		: (idx==CQM_8IC+4) ? pps->scaling_list[CQM_8IY+4]
		: (idx==CQM_8PC+4) ? pps->scaling_list[CQM_8PY+4]
		: i264e_cqm_jvt[idx];
	if( !memcmp( list, def_list, len ) )
		bs_write1( s, 0 );   // scaling_list_present_flag
	else if( !memcmp( list, i264e_cqm_jvt[idx], len ) )
	{
		bs_write1( s, 1 );   // scaling_list_present_flag
		bs_write_se( s, -8 ); // use jvt list
	}
	else
	{
		int run;
		bs_write1( s, 1 );   // scaling_list_present_flag

		// try run-length compression of trailing values
		for( run = len; run > 1; run-- )
			if( list[zigzag[run-1]] != list[zigzag[run-2]] )
				break;
		if( run < len && len - run < bs_size_se( (char)-list[zigzag[run]] ) )
			run = len;

		for( j = 0; j < run; j++ )
			bs_write_se( s, (char)(list[zigzag[j]] - (j>0 ? list[zigzag[j-1]] : 8)) ); // delta

		if( run < len )
			bs_write_se( s, (char)-list[zigzag[run]] );
	}
}

int i264e_validate_levels(struct h264_param *param, int i_level_idc)
{
	int ret = 0;
	int mbs = 0;
	int dpb = 0;
	int mb_width = 0;
	int mb_height = 0;
	const i264e_level_t *l = i264e_levels;

	mb_width = (C_ALIGN(param->width, 16) / 16);
	mb_height = (C_ALIGN(param->height, 16) / 16);
	mbs = mb_width * mb_height;
	dpb = mbs * 1;

	while((l->level_idc != 0) && (l->level_idc != i_level_idc)) {
		l++;
	}

	if(l->frame_size < mbs
			|| l->frame_size*8 < mb_width * mb_width
			|| l->frame_size*8 < mb_height * mb_height) {
		ret = 1;
	}
	if(dpb > l->dpb) {
		ret = 1;
	}

	//if (h->common.param.analyse.i_mv_range > l->mv_range) {
	//	ret = 1;
	//}
	if(param->fps_den > 0) {
		if (mbs * param->fps / param->fps_den > l->mbps) {
			ret = 1;
		}
	}

	/* TODO check the rest of the limits */
	return ret;
}

void dump_sps(i264e_sps_t *sps)
{
	printk("%s=%d\n", "i_id", sps->i_id);
	printk("%s=%d\n", "i_profile_idc", sps->i_profile_idc);
	printk("%s=%d\n", "i_level_idc", sps->i_level_idc);
	printk("%s=%d\n", "b_constraint_set0", sps->b_constraint_set0);
	printk("%s=%d\n", "b_constraint_set1", sps->b_constraint_set1);
	printk("%s=%d\n", "b_constraint_set2", sps->b_constraint_set2);
	printk("%s=%d\n", "b_constraint_set3", sps->b_constraint_set3);
	printk("%s=%d\n", "i_log2_max_frame_num", sps->i_log2_max_frame_num);
	printk("%s=%d\n", "i_poc_type", sps->i_poc_type);
	printk("%s=%d\n", "i_log2_max_poc_lsb", sps->i_log2_max_poc_lsb);
	printk("%s=%d\n", "i_num_ref_frames", sps->i_num_ref_frames);
	printk("%s=%d\n", "b_gaps_in_frame_num_value_allowed", sps->b_gaps_in_frame_num_value_allowed);
	printk("%s=%d\n", "i_mb_width", sps->i_mb_width);
	printk("%s=%d\n", "i_mb_height", sps->i_mb_height);
	printk("%s=%d\n", "b_frame_mbs_only", sps->b_frame_mbs_only);
	printk("%s=%d\n", "b_mb_adaptive_frame_field", sps->b_mb_adaptive_frame_field);
	printk("%s=%d\n", "b_direct8x8_inference", sps->b_direct8x8_inference);
	printk("%s=%d\n", "b_crop", sps->b_crop);
	printk("%s=%d\n", "i_left", sps->crop.i_left);
	printk("%s=%d\n", "i_right", sps->crop.i_right);
	printk("%s=%d\n", "i_top", sps->crop.i_top);
	printk("%s=%d\n", "i_bottom", sps->crop.i_bottom);
	printk("%s=%d\n", "b_vui", sps->b_vui);
	printk("%s=%d\n", "b_aspect_ratio_info_present", sps->vui.b_aspect_ratio_info_present);
	printk("%s=%d\n", "i_sar_width", sps->vui.i_sar_width);
	printk("%s=%d\n", "i_sar_height", sps->vui.i_sar_height);
	printk("%s=%d\n", "b_overscan_info_present", sps->vui.b_overscan_info_present);
	printk("%s=%d\n", "b_overscan_info", sps->vui.b_overscan_info);
	printk("%s=%d\n", "b_signal_type_present", sps->vui.b_signal_type_present);
	printk("%s=%d\n", "i_vidformat", sps->vui.i_vidformat);
	printk("%s=%d\n", "b_fullrange", sps->vui.b_fullrange);
	printk("%s=%d\n", "b_color_description_present", sps->vui.b_color_description_present);
	printk("%s=%d\n", "i_colorprim", sps->vui.i_colorprim);
	printk("%s=%d\n", "i_transfer", sps->vui.i_transfer);
	printk("%s=%d\n", "i_colmatrix", sps->vui.i_colmatrix);
	printk("%s=%d\n", "b_chroma_loc_info_present", sps->vui.b_chroma_loc_info_present);
	printk("%s=%d\n", "i_chroma_loc_top", sps->vui.i_chroma_loc_top);
	printk("%s=%d\n", "i_chroma_loc_bottom", sps->vui.i_chroma_loc_bottom);
	printk("%s=%d\n", "b_timing_info_present", sps->vui.b_timing_info_present);
	printk("%s=%d\n", "i_num_units_in_tick", sps->vui.i_num_units_in_tick);
	printk("%s=%d\n", "i_time_scale", sps->vui.i_time_scale);
	printk("%s=%d\n", "b_fixed_frame_rate", sps->vui.b_fixed_frame_rate);
	printk("%s=%d\n", "b_nal_hrd_parameters_present", sps->vui.b_nal_hrd_parameters_present);
	printk("%s=%d\n", "b_vcl_hrd_parameters_present", sps->vui.b_vcl_hrd_parameters_present);
	printk("%s=%d\n", "i_cpb_cnt", sps->vui.hrd.i_cpb_cnt);
	printk("%s=%d\n", "i_bit_rate_scale", sps->vui.hrd.i_bit_rate_scale);
	printk("%s=%d\n", "i_cpb_size_scale", sps->vui.hrd.i_cpb_size_scale);
	printk("%s=%d\n", "i_bit_rate_value", sps->vui.hrd.i_bit_rate_value);
	printk("%s=%d\n", "i_cpb_size_value", sps->vui.hrd.i_cpb_size_value);
	printk("%s=%d\n", "i_bit_rate_unscaled", sps->vui.hrd.i_bit_rate_unscaled);
	printk("%s=%d\n", "i_cpb_size_unscaled", sps->vui.hrd.i_cpb_size_unscaled);
	printk("%s=%d\n", "b_cbr_hrd", sps->vui.hrd.b_cbr_hrd);
	printk("%s=%d\n", "i_initial_cpb_removal_delay_length", sps->vui.hrd.i_initial_cpb_removal_delay_length);
	printk("%s=%d\n", "i_cpb_removal_delay_length", sps->vui.hrd.i_cpb_removal_delay_length);
	printk("%s=%d\n", "i_dpb_output_delay_length", sps->vui.hrd.i_dpb_output_delay_length);
	printk("%s=%d\n", "i_time_offset_length", sps->vui.hrd.i_time_offset_length);
	printk("%s=%d\n", "b_pic_struct_present", sps->vui.b_pic_struct_present);
	printk("%s=%d\n", "b_bitstream_restriction", sps->vui.b_bitstream_restriction);
	printk("%s=%d\n", "b_motion_vectors_over_pic_boundaries", sps->vui.b_motion_vectors_over_pic_boundaries);
	printk("%s=%d\n", "i_max_bytes_per_pic_denom", sps->vui.i_max_bytes_per_pic_denom);
	printk("%s=%d\n", "i_max_bits_per_mb_denom", sps->vui.i_max_bits_per_mb_denom);
	printk("%s=%d\n", "i_log2_max_mv_length_horizontal", sps->vui.i_log2_max_mv_length_horizontal);
	printk("%s=%d\n", "i_log2_max_mv_length_vertical", sps->vui.i_log2_max_mv_length_vertical);
	printk("%s=%d\n", "i_num_reorder_frames", sps->vui.i_num_reorder_frames);
	printk("%s=%d\n", "i_max_dec_frame_buffering", sps->vui.i_max_dec_frame_buffering);
	printk("%s=%d\n", "b_qpprime_y_zero_transform_bypass", sps->b_qpprime_y_zero_transform_bypass);
	printk("%s=%d\n", "i_chroma_format_idc", sps->i_chroma_format_idc);
}


void i264e_sps_init(i264e_sps_t *sps, int i_id, struct h264_param *param)
{
	int i_level_idc  = 0;
	const i264e_level_t *l = i264e_levels;
	int max_frame_num = 0;
	sps->i_id = i_id;
	sps->i_mb_width = C_ALIGN(param->width, 16) / 16;
	sps->i_mb_height= C_ALIGN(param->height, 16) / 16;
	sps->i_chroma_format_idc = CHROMA_420;

#if 0
	sps->b_qpprime_y_zero_transform_bypass = param->rc.i_rc_method == I264E_RC_CQP && param->rc.i_qp_constant == 0;
	if( sps->b_qpprime_y_zero_transform_bypass || sps->i_chroma_format_idc == CHROMA_444 )
		sps->i_profile_idc  = PROFILE_HIGH444_PREDICTIVE;
	else if( sps->i_chroma_format_idc == CHROMA_422 )
		sps->i_profile_idc  = PROFILE_HIGH422;
	else if( C_BIT_DEPTH > 8 )
		sps->i_profile_idc  = PROFILE_HIGH10;
	else if( param->analyse.b_transform_8x8 )
		sps->i_profile_idc  = PROFILE_HIGH;
	else if( param->b_cabac || param->analyse.i_weighted_pred > 0 )
		sps->i_profile_idc  = PROFILE_MAIN;
	else
		sps->i_profile_idc  = PROFILE_BASELINE;

	if (param->i_profile >= 0) {
		sps->i_profile_idc  = param->i_profile;
	}
#else
	sps->i_profile_idc  = PROFILE_HIGH;
#endif

	sps->b_constraint_set0  = sps->i_profile_idc == PROFILE_BASELINE;
	/* i264e doesn't support the features that are in Baseline and not in Main,
	 * namely arbitrary_slice_order and slice_groups. */
	sps->b_constraint_set1  = sps->i_profile_idc <= PROFILE_MAIN;
	/* Never set constraint_set2, it is not necessary and not used in real world. */
	sps->b_constraint_set2  = 0;
	sps->b_constraint_set3  = 0;

	do {
		i_level_idc = l->level_idc;
	} while (l[1].level_idc && i264e_validate_levels(param, i_level_idc) && l++);
	sps->i_level_idc = i_level_idc;
	if( i_level_idc == 9 && ( sps->i_profile_idc == PROFILE_BASELINE || sps->i_profile_idc == PROFILE_MAIN ) )
	{
		sps->b_constraint_set3 = 1; /* level 1b with Baseline or Main profile is signalled via constraint_set3 */
		sps->i_level_idc      = 11;
	}
	/* Intra profiles */
	//if( param->rc.i_fps_max == 1 && sps->i_profile_idc > PROFILE_HIGH )
	//	sps->b_constraint_set3 = 1;

	sps->vui.i_num_reorder_frames = 0;
	/* extra slot with pyramid so that we don't have to override the
	 * order of forgetting old pictures */
	sps->vui.i_max_dec_frame_buffering =
		//sps->i_num_ref_frames = C_MIN(I264E_REF_MAX, C_MAX4(param->i_frame_reference, 1 + sps->vui.i_num_reorder_frames, 1, param->i_dpb_size));
		sps->i_num_ref_frames = C_MIN(I264E_REF_MAX, C_MAX4(1, 1 + sps->vui.i_num_reorder_frames, 1, 1));
	//if( param->rc.i_fps_max == 1 )
	//{
	//	sps->i_num_ref_frames = 0;
	//	sps->vui.i_max_dec_frame_buffering = 0;
	//}

	/* number of refs + current frame */
	max_frame_num = sps->vui.i_max_dec_frame_buffering + 1;

	sps->i_log2_max_frame_num = 10;
	while( (1 << sps->i_log2_max_frame_num) <= max_frame_num )
		sps->i_log2_max_frame_num++;

	sps->i_poc_type = 2;
	sps->b_vui = 1;

	sps->b_gaps_in_frame_num_value_allowed = 1;
	sps->b_frame_mbs_only = 1;
	sps->b_mb_adaptive_frame_field = 0;
	sps->b_direct8x8_inference = 1;

	//非16对齐开启裁剪
	if (param->width % 16 || param->height % 16) {
		sps->crop.i_left   = 0;
		sps->crop.i_top    = 0;
		sps->crop.i_right  = sps->i_mb_width*16 - param->width;
		sps->crop.i_bottom = (sps->i_mb_height*16 - param->height) >> !sps->b_frame_mbs_only;
		sps->b_crop = sps->crop.i_left  || sps->crop.i_top || sps->crop.i_right || sps->crop.i_bottom;
	}

	sps->vui.b_aspect_ratio_info_present = 0;
	//if( param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0 )
	//{
	//	sps->vui.b_aspect_ratio_info_present = 1;
	//	sps->vui.i_sar_width = param->vui.i_sar_width;
	//	sps->vui.i_sar_height= param->vui.i_sar_height;
	//}

	//sps->vui.b_overscan_info_present = param->vui.i_overscan > 0 && param->vui.i_overscan <= 2;
	sps->vui.b_overscan_info_present = 0;

	//if( sps->vui.b_overscan_info_present )
	//	sps->vui.b_overscan_info = ( param->vui.i_overscan == 2 ? 1 : 0 );

	sps->vui.b_signal_type_present = 0;
	//sps->vui.i_vidformat = ( param->vui.i_vidformat >= 0 && param->vui.i_vidformat <= 5 ? param->vui.i_vidformat : 5 );
	//sps->vui.b_fullrange = ( param->vui.b_fullrange >= 0 && param->vui.b_fullrange <= 1 ? param->vui.b_fullrange : 0 );
	sps->vui.i_vidformat = 5;
	sps->vui.b_fullrange = 0;
	sps->vui.b_color_description_present = 0;

	//sps->vui.i_colorprim = ( param->vui.i_colorprim >= 0 && param->vui.i_colorprim <=  9 ? param->vui.i_colorprim : 2 );
	//sps->vui.i_transfer  = ( param->vui.i_transfer  >= 0 && param->vui.i_transfer  <= 15 ? param->vui.i_transfer  : 2 );
	//sps->vui.i_colmatrix = ( param->vui.i_colmatrix >= 0 && param->vui.i_colmatrix <= 10 ? param->vui.i_colmatrix : 2 );
	sps->vui.i_colorprim = 2;
	sps->vui.i_transfer  = 2;
	sps->vui.i_colmatrix = 2;

	if( sps->vui.i_colorprim != 2 ||
			sps->vui.i_transfer  != 2 ||
			sps->vui.i_colmatrix != 2 )
	{
		sps->vui.b_color_description_present = 1;
	}

	if( sps->vui.i_vidformat != 5 ||
			sps->vui.b_fullrange ||
			sps->vui.b_color_description_present )
	{
		sps->vui.b_signal_type_present = 1;
	}

	/* FIXME: not sufficient for interlaced video */
	//sps->vui.b_chroma_loc_info_present = param->vui.i_chroma_loc > 0 && param->vui.i_chroma_loc <= 5 &&
	//	sps->i_chroma_format_idc == CHROMA_420;
	sps->vui.b_chroma_loc_info_present = 0;
	//if( sps->vui.b_chroma_loc_info_present )
	//{
	//	sps->vui.i_chroma_loc_top = param->vui.i_chroma_loc;
	//	sps->vui.i_chroma_loc_bottom = param->vui.i_chroma_loc;
	//}

	//sps->vui.b_timing_info_present = param->i_timebase_num > 0 && param->i_timebase_den > 0;
	sps->vui.b_timing_info_present = 1;

	if( sps->vui.b_timing_info_present )
	{
		sps->vui.i_num_units_in_tick = param->fps_den;
		sps->vui.i_time_scale = param->fps * 2;
		//sps->vui.b_fixed_frame_rate = !param->b_vfr_input;
		sps->vui.b_fixed_frame_rate = 1;
	}

	sps->vui.b_vcl_hrd_parameters_present = 0;
	sps->vui.b_nal_hrd_parameters_present = 0;
	sps->vui.b_pic_struct_present = 0;

	// NOTE: HRD related parts of the SPS are initialised in i264e_ratecontrol_init_reconfigurable

	//sps->vui.b_bitstream_restriction = param->rc.i_fps_max > 1;
	sps->vui.b_bitstream_restriction = 0;
	//if( sps->vui.b_bitstream_restriction )
	//{
	//	sps->vui.b_motion_vectors_over_pic_boundaries = 1;
	//	sps->vui.i_max_bytes_per_pic_denom = 0;
	//	sps->vui.i_max_bits_per_mb_denom = 0;
	//	sps->vui.i_log2_max_mv_length_horizontal =
	//		sps->vui.i_log2_max_mv_length_vertical = (int)log2f( C_MAX( 1, param->analyse.i_mv_range*4-1 ) ) + 1;
	//}
	//dump_sps(sps);
}

void i264e_sps_write(bs_t *s, i264e_sps_t *sps)
{
	bs_realign( s );
	bs_write( s, 8, sps->i_profile_idc );
	bs_write1( s, sps->b_constraint_set0 );
	bs_write1( s, sps->b_constraint_set1 );
	bs_write1( s, sps->b_constraint_set2 );
	bs_write1( s, sps->b_constraint_set3 );

	bs_write( s, 4, 0 );    /* reserved */

	bs_write( s, 8, sps->i_level_idc );

	bs_write_ue( s, sps->i_id );

	if( sps->i_profile_idc >= PROFILE_HIGH )
	{
		bs_write_ue( s, sps->i_chroma_format_idc );
		if( sps->i_chroma_format_idc == CHROMA_444 )
			bs_write1( s, 0 ); // separate_colour_plane_flag
		bs_write_ue( s, C_BIT_DEPTH-8 ); // bit_depth_luma_minus8
		bs_write_ue( s, C_BIT_DEPTH-8 ); // bit_depth_chroma_minus8
		bs_write1( s, sps->b_qpprime_y_zero_transform_bypass );
		bs_write1( s, 0 ); // seq_scaling_matrix_present_flag
	}

	bs_write_ue( s, sps->i_log2_max_frame_num - 4 );
	bs_write_ue( s, sps->i_poc_type );
	if( sps->i_poc_type == 0 )
		bs_write_ue( s, sps->i_log2_max_poc_lsb - 4 );
	bs_write_ue( s, sps->i_num_ref_frames );
	bs_write1( s, sps->b_gaps_in_frame_num_value_allowed );
	bs_write_ue( s, sps->i_mb_width - 1 );
	bs_write_ue( s, (sps->i_mb_height >> !sps->b_frame_mbs_only) - 1);
	bs_write1( s, sps->b_frame_mbs_only );
	if( !sps->b_frame_mbs_only )
		bs_write1( s, sps->b_mb_adaptive_frame_field );
	bs_write1( s, sps->b_direct8x8_inference );

	bs_write1( s, sps->b_crop );
	if( sps->b_crop )
	{
		int h_shift = sps->i_chroma_format_idc == CHROMA_420 || sps->i_chroma_format_idc == CHROMA_422;
		int v_shift = sps->i_chroma_format_idc == CHROMA_420;
		bs_write_ue( s, sps->crop.i_left   >> h_shift );
		bs_write_ue( s, sps->crop.i_right  >> h_shift );
		bs_write_ue( s, sps->crop.i_top    >> v_shift );
		bs_write_ue( s, sps->crop.i_bottom >> v_shift );
	}

	bs_write1( s, sps->b_vui );
	if( sps->b_vui )
	{
		bs_write1( s, sps->vui.b_aspect_ratio_info_present );
		if( sps->vui.b_aspect_ratio_info_present )
		{
			int i;
			static const struct { unsigned char w, h, sar; } sar[] =
			{
				// aspect_ratio_idc = 0 -> unspecified
				{  1,  1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
				{ 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
				{ 80, 33, 9 }, { 18, 11, 10}, { 15, 11, 11}, { 64, 33, 12},
				{160, 99, 13}, {  4,  3, 14}, {  3,  2, 15}, {  2,  1, 16},
				// aspect_ratio_idc = [17..254] -> reserved
				{ 0, 0, 255 }
			};
			for( i = 0; sar[i].sar != 255; i++ )
			{
				if( sar[i].w == sps->vui.i_sar_width &&
						sar[i].h == sps->vui.i_sar_height )
					break;
			}
			bs_write( s, 8, sar[i].sar );
			if( sar[i].sar == 255 ) /* aspect_ratio_idc (extended) */
			{
				bs_write( s, 16, sps->vui.i_sar_width );
				bs_write( s, 16, sps->vui.i_sar_height );
			}
		}

		bs_write1( s, sps->vui.b_overscan_info_present );
		if( sps->vui.b_overscan_info_present )
			bs_write1( s, sps->vui.b_overscan_info );

		bs_write1( s, sps->vui.b_signal_type_present );
		if( sps->vui.b_signal_type_present )
		{
			bs_write( s, 3, sps->vui.i_vidformat );
			bs_write1( s, sps->vui.b_fullrange );
			bs_write1( s, sps->vui.b_color_description_present );
			if( sps->vui.b_color_description_present )
			{
				bs_write( s, 8, sps->vui.i_colorprim );
				bs_write( s, 8, sps->vui.i_transfer );
				bs_write( s, 8, sps->vui.i_colmatrix );
			}
		}

		bs_write1( s, sps->vui.b_chroma_loc_info_present );
		if( sps->vui.b_chroma_loc_info_present )
		{
			bs_write_ue( s, sps->vui.i_chroma_loc_top );
			bs_write_ue( s, sps->vui.i_chroma_loc_bottom );
		}

		bs_write1( s, sps->vui.b_timing_info_present );
		if( sps->vui.b_timing_info_present )
		{
			bs_write32( s, sps->vui.i_num_units_in_tick );
			bs_write32( s, sps->vui.i_time_scale );
			bs_write1( s, sps->vui.b_fixed_frame_rate );
		}

		bs_write1( s, sps->vui.b_nal_hrd_parameters_present );
		if( sps->vui.b_nal_hrd_parameters_present )
		{
			bs_write_ue( s, sps->vui.hrd.i_cpb_cnt - 1 );
			bs_write( s, 4, sps->vui.hrd.i_bit_rate_scale );
			bs_write( s, 4, sps->vui.hrd.i_cpb_size_scale );

			bs_write_ue( s, sps->vui.hrd.i_bit_rate_value - 1 );
			bs_write_ue( s, sps->vui.hrd.i_cpb_size_value - 1 );

			bs_write1( s, sps->vui.hrd.b_cbr_hrd );

			bs_write( s, 5, sps->vui.hrd.i_initial_cpb_removal_delay_length - 1 );
			bs_write( s, 5, sps->vui.hrd.i_cpb_removal_delay_length - 1 );
			bs_write( s, 5, sps->vui.hrd.i_dpb_output_delay_length - 1 );
			bs_write( s, 5, sps->vui.hrd.i_time_offset_length );
		}

		bs_write1( s, sps->vui.b_vcl_hrd_parameters_present );

		if( sps->vui.b_nal_hrd_parameters_present || sps->vui.b_vcl_hrd_parameters_present )
			bs_write1( s, 0 );   /* low_delay_hrd_flag */

		bs_write1( s, sps->vui.b_pic_struct_present );
		bs_write1( s, sps->vui.b_bitstream_restriction );
		if( sps->vui.b_bitstream_restriction )
		{
			bs_write1( s, sps->vui.b_motion_vectors_over_pic_boundaries );
			bs_write_ue( s, sps->vui.i_max_bytes_per_pic_denom );
			bs_write_ue( s, sps->vui.i_max_bits_per_mb_denom );
			bs_write_ue( s, sps->vui.i_log2_max_mv_length_horizontal );
			bs_write_ue( s, sps->vui.i_log2_max_mv_length_vertical );
			bs_write_ue( s, sps->vui.i_num_reorder_frames );
			bs_write_ue( s, sps->vui.i_max_dec_frame_buffering );
		}
	}

	bs_rbsp_trailing( s );
	bs_flush( s );
}

void i264e_pps_init(i264e_pps_t *pps, int i_id, struct h264_param *param, i264e_sps_t *sps)
{
	int i = 0;

	pps->i_id = i_id;
	pps->i_sps_id = sps->i_id;
	//pps->b_cabac = param->b_cabac;
	pps->b_cabac = 1;

	pps->b_pic_order = 0;
	pps->i_num_slice_groups = 1;

	//if (param->b_mref && param->i_frame_reference > 1) {
	//	pps->i_num_ref_idx_l0_default_active = param->i_frame_reference;
	//} else {
	pps->i_num_ref_idx_l0_default_active = 1;
	//}

	pps->i_num_ref_idx_l1_default_active = 1;

	//pps->b_weighted_pred = param->analyse.i_weighted_pred > 0;
	pps->b_weighted_pred = 0;
	pps->b_weighted_bipred = 0;

	//pps->i_pic_init_qp = (param->rc.i_rc_method != I264E_RC_CQP)
	//	? (26 + QP_BD_OFFSET) : SPEC_QP( param->rc.i_qp_constant );
	//pps->i_pic_init_qp = param->i_qp;
	pps->i_pic_init_qp = 26 + QP_BD_OFFSET;
	pps->i_pic_init_qs = 26 + QP_BD_OFFSET;

	//pps->i_chroma_qp_index_offset = param->analyse.i_chroma_qp_offset;
	pps->i_chroma_qp_index_offset = 0;
	pps->b_deblocking_filter_control = 1;
	pps->b_constrained_intra_pred = 0;
	pps->b_redundant_pic_cnt = 0;

	//pps->b_transform_8x8_mode = param->analyse.b_transform_8x8 ? 1 : 0;
	pps->b_transform_8x8_mode = 1;

	pps->i_cqm_preset = 2;

	for( i = 0; i < 8; i++ )
		pps->scaling_list[i] = i264e_cqm_flat16;
#if 1
	{ //add by cbzhang in 20180806
		pps->scaling_list[4] = i264e_cqm_jvt8i;
		pps->scaling_list[5] = i264e_cqm_jvt8p;
	}
#endif
}

void i264e_pps_write(bs_t *s, i264e_sps_t *sps, i264e_pps_t *pps)
{
	bs_realign( s );
	bs_write_ue( s, pps->i_id );
	bs_write_ue( s, pps->i_sps_id );

	bs_write1( s, pps->b_cabac );
	bs_write1( s, pps->b_pic_order );
	bs_write_ue( s, pps->i_num_slice_groups - 1 );

	bs_write_ue( s, pps->i_num_ref_idx_l0_default_active - 1 );
	bs_write_ue( s, pps->i_num_ref_idx_l1_default_active - 1 );
	bs_write1( s, pps->b_weighted_pred );
	bs_write( s, 2, pps->b_weighted_bipred );

	bs_write_se( s, pps->i_pic_init_qp - 26 - QP_BD_OFFSET );
	bs_write_se( s, pps->i_pic_init_qs - 26 - QP_BD_OFFSET );
	bs_write_se( s, pps->i_chroma_qp_index_offset );

	bs_write1( s, pps->b_deblocking_filter_control );
	bs_write1( s, pps->b_constrained_intra_pred );
	bs_write1( s, pps->b_redundant_pic_cnt );

	if( pps->b_transform_8x8_mode || pps->i_cqm_preset != I264E_CQM_FLAT )
	{
		bs_write1( s, pps->b_transform_8x8_mode );
		bs_write1( s, (pps->i_cqm_preset != I264E_CQM_FLAT) );
		if( pps->i_cqm_preset != I264E_CQM_FLAT )
		{
			scaling_list_write( s, pps, CQM_4IY );
			scaling_list_write( s, pps, CQM_4IC );
			bs_write1( s, 0 ); // Cr = Cb
			scaling_list_write( s, pps, CQM_4PY );
			scaling_list_write( s, pps, CQM_4PC );
			bs_write1( s, 0 ); // Cr = Cb
			if( pps->b_transform_8x8_mode )
			{
				if( sps->i_chroma_format_idc == CHROMA_444 )
				{
					scaling_list_write( s, pps, CQM_8IY+4 );
					scaling_list_write( s, pps, CQM_8IC+4 );
					bs_write1( s, 0 ); // Cr = Cb
					scaling_list_write( s, pps, CQM_8PY+4 );
					scaling_list_write( s, pps, CQM_8PC+4 );
					bs_write1( s, 0 ); // Cr = Cb
				}
				else
				{
					scaling_list_write( s, pps, CQM_8IY+4 );
					scaling_list_write( s, pps, CQM_8PY+4 );
				}
			}
		}
		bs_write_se( s, pps->i_chroma_qp_index_offset );
	}

	bs_rbsp_trailing( s );
	bs_flush( s );
}

#if 0
void i264e_sei_write(bs_t *s, unsigned char *payload, int payload_size, int payload_type)
{
	int i;

	bs_realign( s );

	for( i = 0; i <= payload_type-255; i += 255 )
		bs_write( s, 8, 255 );
	bs_write( s, 8, payload_type-i );

	for( i = 0; i <= payload_size-255; i += 255 )
		bs_write( s, 8, 255 );
	bs_write( s, 8, payload_size-i );

	for( i = 0; i < payload_size; i++ )
		bs_write( s, 8, payload[i] );

	bs_rbsp_trailing( s );
	bs_flush( s );
}

int i264e_sei_version_write(i264e_thread_t *t, bs_t *s)
{
	// random ID number generated according to ISO-11578
	static const unsigned char uuid[16] =
	{
		0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
		0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef
	};

	char *opts = i264e_param2string(&t->h->common.param, 0);
	char *payload;
	int length;

	if(!opts) {
		i264e_log(NULL, C_LOG_ERROR, "param2string failed\n");
		goto err_param2string;
	}

	if ((payload = malloc(200 + strlen(opts))) == NULL) {
		i264e_log(NULL, C_LOG_ERROR, "malloc payload size:%d failed\n", 200 + strlen(opts));
		goto err_malloc_payload;
	}

	memcpy(payload, uuid, 16 );

	sprintf(payload+16, "i264e - version: %s build:%s - H.264/MPEG-4 AVC codec - options:\n%s",
			I264E_VERSION, I264E_BUILD, opts);

	length = strlen(payload) + 1;

	i264e_sei_write(s, (unsigned char *)payload, length, I264E_SEI_USER_DATA_UNREGISTERED );

	free(opts);
	free(payload);

	return 0;

err_malloc_payload:
	free( opts );
err_param2string:
	return -1;
}

void i264e_sei_pic_timing_write(i264e_thread_t *t, bs_t *s)
{
	bs_t q;
	unsigned char tmp_sei_buf[100];

	memset(tmp_sei_buf, 0, sizeof(tmp_sei_buf));
	bs_init( &q, tmp_sei_buf, 100 );
	bs_realign( &q );
	bs_align_10( &q );
	bs_flush( &q );
	i264e_sei_write( s, tmp_sei_buf, bs_pos( &q ) / 8, I264E_SEI_PIC_TIMING );
}
#endif

void i264e_slice_header_init(i264e_slice_header_t *sh, struct h264_param *param,
		i264e_sps_t *sps, i264e_pps_t *pps,
		int i_idr_pic_id, int i_frame, int i_qp)
{
	int deblock_thresh = 0;
	/* First we fill all fields */
	sh->sps = sps;
	sh->pps = pps;

	sh->i_first_mb  = 0;
	sh->i_last_mb   = (C_ALIGN(param->width, 16) / 16) * (C_ALIGN(param->height, 16) / 16) - 1;
	sh->i_pps_id    = pps->i_id;

	sh->i_frame = i_frame;
	sh->i_type = i_frame == 0 ? I264E_SLICE_TYPE_I : I264E_SLICE_TYPE_P;

	sh->b_mbaff = 0;
	sh->b_field_pic = 0;    /* no field support for now */
	sh->b_bottom_field = 0; /* not yet used */

	sh->i_idr_pic_id = i_idr_pic_id;

	/* poc stuff, fixed later */
	sh->i_poc = 0;
	sh->i_delta_poc_bottom = 0;
	sh->i_delta_poc[0] = 0;
	sh->i_delta_poc[1] = 0;

	sh->i_redundant_pic_cnt = 0;


	//sh->i_cabac_init_idc = param->i_cabac_init_idc;
	sh->i_cabac_init_idc = 0;

	sh->i_qp = c_clip3(1, 51, i_qp);
	sh->i_qp_delta = sh->i_qp - pps->i_pic_init_qp;
	sh->b_sp_for_swidth = 0;
	sh->i_qs_delta = 0;

	//int deblock_thresh = i_qp + 2 * C_MIN(param->i_deblocking_filter_alphac0, param->i_deblocking_filter_beta);
	deblock_thresh = i_qp;
	/* If effective qp <= 15, deblocking would have no effect anyway */
	//if( param->b_deblocking_filter && (15 < deblock_thresh))
	if( 1 && (15 < deblock_thresh) )
		sh->i_disable_deblocking_filter_idc = 0;
	else
		sh->i_disable_deblocking_filter_idc = 1;
	//sh->i_alpha_c0_offset = param->i_deblocking_filter_alphac0 << 1;
	//sh->i_beta_offset = param->i_deblocking_filter_beta << 1;
	sh->i_alpha_c0_offset = 0;
	sh->i_beta_offset = 0;
}

void i264e_slice_header_write(bs_t *s, i264e_slice_header_t *sh, int i_nal_ref_idc )
{
	int i = 0, j = 0;

	bs_realign( s );
	if( sh->b_mbaff )
	{
		int first_x = sh->i_first_mb % sh->sps->i_mb_width;
		int first_y = sh->i_first_mb / sh->sps->i_mb_width;
		//assert( (first_y&1) == 0 );
		bs_write_ue( s, (2*first_x + sh->sps->i_mb_width*(first_y&~1) + (first_y&1)) >> 1 );
	}
	else
		bs_write_ue( s, sh->i_first_mb );

	bs_write_ue( s, sh->i_type);   /* same type things */
	bs_write_ue( s, sh->i_pps_id );
	bs_write( s, sh->sps->i_log2_max_frame_num, sh->i_frame & ((1<<sh->sps->i_log2_max_frame_num)-1) );

	if( !sh->sps->b_frame_mbs_only )
	{
		bs_write1( s, sh->b_field_pic );
		if( sh->b_field_pic )
			bs_write1( s, sh->b_bottom_field );
	}

	if( sh->i_idr_pic_id >= 0 ) /* NAL IDR */
		bs_write_ue( s, sh->i_idr_pic_id );

	if( sh->sps->i_poc_type == 0 )
	{
		bs_write( s, sh->sps->i_log2_max_poc_lsb, sh->i_poc & ((1<<sh->sps->i_log2_max_poc_lsb)-1) );
		if( sh->pps->b_pic_order && !sh->b_field_pic )
			bs_write_se( s, sh->i_delta_poc_bottom );
	}

	if( sh->pps->b_redundant_pic_cnt )
		bs_write_ue( s, sh->i_redundant_pic_cnt );

	if( sh->i_type == I264E_SLICE_TYPE_B )
		bs_write1( s, sh->b_direct_spatial_mv_pred );

	sh->b_ref_pic_list_reordering[0] = 0;
	sh->b_ref_pic_list_reordering[1] = 0;
	sh->i_mmco_command_count = 0;
	sh->b_num_ref_idx_override = 0;
	sh->i_num_ref_idx_l0_active = sh->pps->i_num_ref_idx_l0_default_active;
	sh->i_num_ref_idx_l1_active = 1;

	if( sh->i_type == I264E_SLICE_TYPE_P || sh->i_type == I264E_SLICE_TYPE_B )
	{
		bs_write1( s, sh->b_num_ref_idx_override );
		if( sh->b_num_ref_idx_override )
		{
			bs_write_ue( s, sh->i_num_ref_idx_l0_active - 1 );
			if( sh->i_type == I264E_SLICE_TYPE_B )
				bs_write_ue( s, sh->i_num_ref_idx_l1_active - 1 );
		}
	}

	/* ref pic list reordering */
	if( sh->i_type != I264E_SLICE_TYPE_I )
	{
		bs_write1( s, sh->b_ref_pic_list_reordering[0] );
		if( sh->b_ref_pic_list_reordering[0] )
		{
			for( i = 0; i < sh->i_num_ref_idx_l0_active; i++ )
			{
				bs_write_ue( s, sh->ref_pic_list_order[0][i].idc );
				bs_write_ue( s, sh->ref_pic_list_order[0][i].arg );
			}
			bs_write_ue( s, 3 );
		}
	}
	if( sh->i_type == I264E_SLICE_TYPE_B )
	{
		bs_write1( s, sh->b_ref_pic_list_reordering[1] );
		if( sh->b_ref_pic_list_reordering[1] )
		{
			for( i = 0; i < sh->i_num_ref_idx_l1_active; i++ )
			{
				bs_write_ue( s, sh->ref_pic_list_order[1][i].idc );
				bs_write_ue( s, sh->ref_pic_list_order[1][i].arg );
			}
			bs_write_ue( s, 3 );
		}
	}

	sh->b_weighted_pred = 0;
	if( sh->pps->b_weighted_pred && sh->i_type == I264E_SLICE_TYPE_P )
	{
		sh->b_weighted_pred = sh->weight[0][0].weightfn || sh->weight[0][1].weightfn || sh->weight[0][2].weightfn;
		/* pred_weight_table() */
		bs_write_ue( s, sh->weight[0][0].i_denom );
		bs_write_ue( s, sh->weight[0][1].i_denom );
		for( i = 0; i < sh->i_num_ref_idx_l0_active; i++ )
		{
			int luma_weight_l0_flag = !!sh->weight[i][0].weightfn;
			int chroma_weight_l0_flag = !!sh->weight[i][1].weightfn || !!sh->weight[i][2].weightfn;
			bs_write1( s, luma_weight_l0_flag );
			if( luma_weight_l0_flag )
			{
				bs_write_se( s, sh->weight[i][0].i_scale );
				bs_write_se( s, sh->weight[i][0].i_offset );
			}
			bs_write1( s, chroma_weight_l0_flag );
			if( chroma_weight_l0_flag )
			{
				for( j = 1; j < 3; j++ )
				{
					bs_write_se( s, sh->weight[i][j].i_scale );
					bs_write_se( s, sh->weight[i][j].i_offset );
				}
			}
		}
	}
	else if( sh->pps->b_weighted_bipred == 1 && sh->i_type == I264E_SLICE_TYPE_B )
	{
		/* TODO */
	}

	if( i_nal_ref_idc != 0 )
	{
		if( sh->i_idr_pic_id >= 0 )
		{
			bs_write1( s, 0 );  /* no output of prior pics flag */
			bs_write1( s, sh->i_mmco_command_count > 0);  /* long term reference flag */
		}
		else
		{
			bs_write1( s, sh->i_mmco_command_count > 0 ); /* adaptive_ref_pic_marking_mode_flag */
			if( sh->i_mmco_command_count > 0 )
			{
				for( i = 0; i < sh->i_mmco_command_count; i++ )
				{
					bs_write_ue( s, sh->mmco[i].idc ); /* mark short term ref as unused */
					bs_write_ue( s, sh->mmco[i].arg );
				}
				bs_write_ue( s, 0 ); /* end command list */
			}
		}
	}

	if( sh->pps->b_cabac && sh->i_type != I264E_SLICE_TYPE_I )
		bs_write_ue( s, sh->i_cabac_init_idc );

	bs_write_se( s, sh->i_qp_delta );      /* slice qp delta */

	if( sh->pps->b_deblocking_filter_control )
	{
		bs_write_ue( s, sh->i_disable_deblocking_filter_idc );
		if( sh->i_disable_deblocking_filter_idc != 1 )
		{
			bs_write_se( s, sh->i_alpha_c0_offset >> 1 );
			bs_write_se( s, sh->i_beta_offset >> 1 );
		}
	}

	//if cabac
	bs_align_1( s );
}

