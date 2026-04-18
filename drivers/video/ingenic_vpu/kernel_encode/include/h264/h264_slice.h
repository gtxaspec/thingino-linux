#ifndef _H264_SLICE_H_
#define _H264_SLICE_H_

#define I264E_REF_MAX 2
enum slice_type_e
{
	I264E_SLICE_TYPE_P  = 0,
	I264E_SLICE_TYPE_B  = 1,
	I264E_SLICE_TYPE_I  = 2,
};

enum i264e_profile_e {
	PROFILE_BASELINE = 66,
	PROFILE_MAIN     = 77,
	PROFILE_HIGH    = 100,
	PROFILE_HIGH10  = 110,
	PROFILE_HIGH422 = 122,
	PROFILE_HIGH444_PREDICTIVE = 244,
};

enum i264e_chroma_format_e {
	CHROMA_400 = 0,
	CHROMA_420 = 1,
	CHROMA_422 = 2,
	CHROMA_444 = 3,
};

enum i264e_cqm4_e {
	CQM_4IY = 0,
	CQM_4PY = 1,
	CQM_4IC = 2,
	CQM_4PC = 3
};

enum i264e_cqm8_e {
	CQM_8IY = 0,
	CQM_8PY = 1,
	CQM_8IC = 2,
	CQM_8PC = 3,
};

typedef struct {
	int level_idc;
	int mbps;        /* max macroblock processing rate (macroblocks/sec) */
	int frame_size;  /* max frame size (macroblocks) */
	int dpb;         /* max decoded picture buffer (mbs) */
	int bitrate;     /* max bitrate (kbit/sec) */
	int cpb;         /* max vbv buffer (kbit) */
	int mv_range;    /* max vertical mv component range (pixels) */
	int mvs_per_2mb; /* max mvs per 2 consecutive mbs. */
	int slice_rate;  /* ?? */
	int mincr;       /* min compression ratio */
	int bipred8x8;   /* limit bipred to >=8x8 */
	int direct8x8;   /* limit b_direct to >=8x8 */
	int frame_only;  /* forbid interlacing */
} i264e_level_t;

typedef struct {
	int i_id;

	int i_profile_idc;
	int i_level_idc;

	int b_constraint_set0;
	int b_constraint_set1;
	int b_constraint_set2;
	int b_constraint_set3;

	int i_log2_max_frame_num;

	int i_poc_type;
	/* poc 0 */
	int i_log2_max_poc_lsb;

	int i_num_ref_frames;
	int b_gaps_in_frame_num_value_allowed;
	int i_mb_width;
	int i_mb_height;
	int b_frame_mbs_only;
	int b_mb_adaptive_frame_field;
	int b_direct8x8_inference;

	int b_crop;
	struct
	{
		int i_left;
		int i_right;
		int i_top;
		int i_bottom;
	} crop;

	int b_vui;
	struct
	{
		int b_aspect_ratio_info_present;
		int i_sar_width;
		int i_sar_height;

		int b_overscan_info_present;
		int b_overscan_info;

		int b_signal_type_present;
		int i_vidformat;
		int b_fullrange;
		int b_color_description_present;
		int i_colorprim;
		int i_transfer;
		int i_colmatrix;

		int b_chroma_loc_info_present;
		int i_chroma_loc_top;
		int i_chroma_loc_bottom;

		int b_timing_info_present;
		unsigned int i_num_units_in_tick;
		unsigned int i_time_scale;
		int b_fixed_frame_rate;

		int b_nal_hrd_parameters_present;
		int b_vcl_hrd_parameters_present;

		struct
		{
			int i_cpb_cnt;
			int i_bit_rate_scale;
			int i_cpb_size_scale;
			int i_bit_rate_value;
			int i_cpb_size_value;
			int i_bit_rate_unscaled;
			int i_cpb_size_unscaled;
			int b_cbr_hrd;

			int i_initial_cpb_removal_delay_length;
			int i_cpb_removal_delay_length;
			int i_dpb_output_delay_length;
			int i_time_offset_length;
		} hrd;

		int b_pic_struct_present;
		int b_bitstream_restriction;
		int b_motion_vectors_over_pic_boundaries;
		int i_max_bytes_per_pic_denom;
		int i_max_bits_per_mb_denom;
		int i_log2_max_mv_length_horizontal;
		int i_log2_max_mv_length_vertical;
		int i_num_reorder_frames;
		int i_max_dec_frame_buffering;

		/* FIXME to complete */
	} vui;

	int b_qpprime_y_zero_transform_bypass;
	int i_chroma_format_idc;

} i264e_sps_t;

typedef struct {
	int i_id;
	int i_sps_id;
	int b_cabac;
	int b_pic_order;
	int i_num_slice_groups;
	int i_num_ref_idx_l0_default_active;
	int i_num_ref_idx_l1_default_active;
	int b_weighted_pred;
	int b_weighted_bipred;
	int i_pic_init_qp;
	int i_pic_init_qs;
	int i_chroma_qp_index_offset;
	int b_deblocking_filter_control;
	int b_constrained_intra_pred;
	int b_redundant_pic_cnt;
	int b_transform_8x8_mode;
	int i_cqm_preset;
	const unsigned char *scaling_list[8]; /* could be 12, but we don't allow separate Cb/Cr lists */
} i264e_pps_t;

#define DECLARE_ALIGNED(var, n) var __attribute__((aligned(n)))
#define ALIGNED_16(var) DECLARE_ALIGNED( var, 16 )
struct i264e_weight_t;
typedef unsigned char pixel;
typedef void (* weight_fn_t)( pixel *, int, pixel *, int, const struct i264e_weight_t *, int );
typedef struct i264e_weight_t
{
	/* aligning the first member is a gcc hack to force the struct to be
	 * 16 byte aligned, as well as force sizeof(struct) to be a multiple of 16 */
	ALIGNED_16( unsigned short cachea[8] );
	unsigned short cacheb[8];
	unsigned int i_denom;
	unsigned int i_scale;
	unsigned int i_offset;
	weight_fn_t *weightfn;
} ALIGNED_16( i264e_weight_t );

typedef struct
{
	i264e_sps_t *sps;
	i264e_pps_t *pps;

	int i_type;
	int i_first_mb;
	int i_last_mb;

	int i_pps_id;

	int i_frame;

	int b_mbaff;
	int b_field_pic;
	int b_bottom_field;

	int i_idr_pic_id;   /* -1 if nal_type != 5 */

	int i_poc;
	int i_delta_poc_bottom;

	int i_delta_poc[2];
	int i_redundant_pic_cnt;

	int b_direct_spatial_mv_pred;

	int b_num_ref_idx_override;
	int i_num_ref_idx_l0_active;
	int i_num_ref_idx_l1_active;

	int b_ref_pic_list_reordering[2];
	struct
	{
		int idc;
		int arg;
	} ref_pic_list_order[2][I264E_REF_MAX];

	/* P-frame weighting */
	int b_weighted_pred;
	i264e_weight_t weight[I264E_REF_MAX*2][3];

	int i_mmco_remove_from_end;
	int i_mmco_command_count;
	struct /* struct for future expansion */
	{
		int idc;
		int arg;
		int i_poc;
	} mmco[I264E_REF_MAX];

	int i_cabac_init_idc;

	int i_qp;
	int i_qp_delta;
	int b_sp_for_swidth;
	int i_qs_delta;

	/* deblocking filter */
	int i_disable_deblocking_filter_idc;
	int i_alpha_c0_offset;
	int i_beta_offset;
} i264e_slice_header_t;

#endif

