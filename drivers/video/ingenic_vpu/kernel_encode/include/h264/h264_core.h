#ifndef _H264_CORE_H_
#define _H264_CORE_H_

#include "vpu_core.h"
#include "h264_bitstream.h"
#include "h264_slice.h"
#include "h264_nal.h"

//slice type
#define H264_SLICE_TYPE_P   0
#define H264_SLICE_TYPE_B   1
#define H264_SLICE_TYPE_IDR 2
#define H264_SLICE_TYPE_VI  6

typedef struct h264_instance h264_t;

struct h264_header_info {
	bs_t bs;
	i264e_nal_t nal;
	i264e_sps_t sps;
	i264e_pps_t pps;
	i264e_slice_header_t header;
};

struct h264_param {
	unsigned short width;
	unsigned short height;
	unsigned short y_stride;
	unsigned short c_stride;
	unsigned short i_qp;
	unsigned short p_qp;
	unsigned short gop;
	unsigned short fps;
	unsigned short fps_den;
	unsigned char  state[1024]; //cabac
	unsigned char  *bs;
	unsigned int   bs_len;
};

typedef struct roi_info_t{
	unsigned char roi_en;
	unsigned char roi_md;
	unsigned char roi_qp;
	unsigned char roi_lmbx;
	unsigned char roi_rmbx;
	unsigned char roi_umby;
	unsigned char roi_bmby;
} roi_info_t;

typedef struct h264_tfm_grp_t {
	unsigned short acmask_y[3]; 	                                // 14-bit uns, [0] search, [1] merge, [2] intra
	unsigned short acmask_c[3]; 	                                // 14-bit uns, [0] search, [1] merge, [2] intra
} h264_tfm_grp_t;
typedef struct h264_mcec_range_grp_t {
	unsigned short    mcec_lx;                                    // left search range
	unsigned short    mcec_rx;                                    // right search range
	unsigned short    mcec_ty;                                    // top search range
	unsigned short    mcec_by;                                    // bottom search range
	unsigned short    skip_pixel;                                 // search pixel step
} h264_mcec_range_grp_t;
typedef struct h264_mcec_grp_t {
	unsigned char     mcec_en;                                    // enable block level mce control
	unsigned char     mcec_mode;                                  // search mode, 0: only dia, 1: only fs, 2: fs->dia, 3:cros->dia, 4:cros->fs, 5:cros->fs->dia
	unsigned char     fs_cfg_idx;                                 // full search mcec_range_grp_t group index
	unsigned char     cros_cfg_idx;                               // cross search mcec_range_grp_t group index
	unsigned char     force_zero_mv_en;                           // force mv (0,0)
	unsigned char     mvc_bc_wei;                                 // mvc cost bits weight (only 265)
	unsigned char     sech_bc_wei;                                // me cost bits weight (only 265)
} h264_mcec_grp_t;
typedef struct h264_ifa_para_t{
	unsigned char          ifa_en;                                // enable of image feature analysis.
															//--------------------
															//  cps
															//--------------------
	unsigned char          cps_en;                                // enable of downscale raw
	unsigned char          cps_c_en;                              // enable of downscale raw C
	unsigned char         *emc_cps_ref0;                          // ADDR of ref data or last cps_raw data.
															//--------------------
															//  rrs
															//--------------------
	unsigned char          efe_rrs_en;                            // enable of frame diff. raw-ref0
	unsigned char          rrs_c_en;                              // enable of frame diff C. raw-ref0
	unsigned char          rrs_thrd;                              // threshold to stat count of Y pixel rrs > thrd  in mb. 0~16
	unsigned char          rrs_thrd_c;                            // threshold to stat count of C pixel rrs > thrd  in mb. 0~4
															//--------------------
															//  motion
															//--------------------
	unsigned char          motion_en;                             // enable of motion flag
	unsigned char          motion_c_en;                           // enable of motion flag C
	unsigned short         motion_thrd;                           // threshold to compare with rrs
	unsigned short         motion_thrd_c;                         // threshold to compare with rrs_c
	unsigned char          motion_of_thrd;                        // threshold to compare with rrs stat cnt
	unsigned char          motion_of_thrd_c;                      // threshold to compare with rrs_c stat cnt
	unsigned short         rrc_motion_thr[7];                     // threshold of different rrs range
	unsigned short         frm_mt_smd_thr;                        // threshold of different rrs smd sum
	int              crp_thrd;                              // mode of motion feature filter. 0: normal. 1: not change motion to static
															//--------------------
															//  still times
															//--------------------
	unsigned char          still_times_en;                        // enable of stat mb still times enable
	unsigned char          motion_level;                          // set motion flag valid bit.
	unsigned char          max_still_times;                       // set still times up-limit.
															//--------------------
															//  sobel
															//--------------------
	unsigned char          sobel_en;                              // enable of direction detection enable
	unsigned char          sobel_edge_thrd;                       // threshold of grand thrd for detect edge
															//--------------------
															//  smd
															//--------------------
	unsigned char          smd_en;                                // enable of image cplx detection
	unsigned char          smd_c_en;                              // enable of image cplx detection C
	unsigned char          row_smd_unit;                          // number of one row_smd_unit 64 lines - 1
															//--------------------
															//  energy
															//--------------------
	unsigned char          ery_en;                                // enable of mb energy
															//--------------------
															//  skin
															//--------------------
	unsigned char          skin_en;                               // enable of skin detection
	unsigned char          skin_lvl_en[3];                        // enable of 3 level skin detection
	unsigned char          skin_cnt_thrd;                         // threshold of skin count
	unsigned char          skin_thrd[3][2][2];                    //[sobel][u/v][max/min]
	short          skin_level[4];                         // formula coefficient.
	unsigned char          skin_mul_factor[4];                    // formula coefficient.
															//--------------------
															//  AI mark
															//--------------------
	unsigned char          ai_mark_en;                            // enable of ai mark
	unsigned char          ai_mark_type;                          // num of ai mark type.
															//--------------------
															//  pet mark
															//--------------------
	unsigned char          petbc_en;                              // enable of petbc flag
	unsigned char          pet_mode;                              // petbc flag mode. 0: only calculate petbc0, 1: calculate petbc0 and petbc1
	unsigned short         petbc_var_thr[3];                      // threshold of petbc0
	unsigned short         petbc_ssm_thr[3][4];                   // threshold of petbc0
	unsigned short         petbc2_var_thr[3];                     // threshold of petbc1
	unsigned short         petbc2_ssm_thr[3][4];                  // threshold of petbc1
															//--------------------
															//  qpg
															//--------------------
	unsigned char          qpg_en;                                // enable of mb qp.
	unsigned char          base_qp;                               // base qp of mb level.
	unsigned char          max_qp;                                // max qp of mb level.(after roi or qpt)
	unsigned char          min_qp;                                // min qp of mb level.(after roi or qpt)
	unsigned char          qpg_max_qp;                            // max qp of qpg.(after cplx qpg)
	unsigned char          qpg_min_qp;                            // min qp of qpg.(after cplx qpg)
	unsigned char          qpg_filte_en;                          // enable of qpg filter.
															//qpg  tab
	unsigned char          qpg_table_en;                          // enable of qpg table
															//qpg roi
	unsigned char          qpg_roi_en;                            // enable of qpg roi
															//qpg skin
	unsigned char          qpg_skin_en;                           // enable of qpg skin
	char           qpg_skin_qp_ofst[3];                   // offset of qp with diffenent skin range
															//qpg enery
	unsigned char          qpg_ery_en;                            // enable of qpg energy
															//qpg sobel
	unsigned char          qpg_sobel_en;                          // enable of qpg sobel
	short          qpg_sobel_cplx_thrd[8];                // threshold of sobel gradient threshold
															//qpg smd
	unsigned char          qpg_smd_en;                            // enable of qpg smd
	short          qpg_smd_cplx_thrd[8];                  // threshold of smd range.
	char           qpg_cplx_qp_ofst[8];                   // offset of qp with different smd range.
															//qpg petbc
	unsigned char          qpg_petbc_en;                          // enable of qpg petbc
	unsigned char          qpg_pet_cplx_thrd[3][12];              // threshold of smd range
	unsigned char          qpg_pet_qp_ofst[5][7];                 // offset of qp with different smd range
	unsigned char          qpg_pet_qp_ofst_mt[5][7];              // offset of qp with different smd range in motion region
	unsigned char          qpg_dlt_thr[4];                        // threshold of
	unsigned char          qpg_pet_qp_idx[5];                     // idx of qp ofst with different qp.
	unsigned char          pet_filter_valid[4];                   // whether neighbour block petbc flag is valid. [0].left, [1]:right, [2]: top, [3]: bottom.
	unsigned char          qpg_pet_fil_en;                        // do not used;petbc filter enable
															//qpg ai
	char           qpg_ai_motion_smd_ofst[3][2][8];       // offset of qpg ai with different motion and cplx region.
	unsigned char          dump_api_en;
}h264_ifa_para_t;
typedef struct h264_rm_t{
	// T32 unique
	/*used*/unsigned char         rrs_mode;                               //0: raw-ref, 1: raw-raw
	/*used*/unsigned char         rrs_of_en;                              //overflow enable
	/*used*/unsigned char         cps_mode;                               //down scale factor. 0: 2x2, 1: 4x4
	/*used*/unsigned int        tmp_beyond_caddr;
	/*used*/unsigned int        frame_idx;
	/*used*/unsigned int        acmask_mode;
	/*used*/unsigned char         i_4x4_dis;
	/*used*/unsigned char         i_8x8_dis;
	/*used*/unsigned char         i_16x16_dis;
	/*used*/unsigned char         p_l0_dis;
	/*used*/unsigned char         p_t8_dis;
	/*used*/unsigned char         p_skip_dis;
	/*used*/unsigned char         p_skip_pl0f_dis;
	/*used*/unsigned char         p_skip_pt8f_dis;
	/*used*/unsigned char         cost_bias_en;
	/*used*/unsigned char         cost_bias_i_4x4;
	/*used*/unsigned char         cost_bias_i_8x8;
	/*used*/unsigned char         cost_bias_i_16x16;
	/*used*/unsigned char         cost_bias_p_l0;
	/*used*/unsigned char         cost_bias_p_t8;
	/*used*/unsigned char         cost_bias_p_skip;
	/*used*/unsigned char         intra_lambda_y_bias_en;
	/*used*/unsigned char         intra_lambda_c_bias_en;
	/*used*/unsigned char         intra_lambda_bias_qp0;
	/*used*/unsigned char         intra_lambda_bias_qp1;
	/*used*/unsigned char         intra_lambda_bias_0;
	/*used*/unsigned char         intra_lambda_bias_1;
	/*used*/unsigned char         intra_lambda_bias_2;
	/*used*/unsigned char         chroma_sse_bias_en;
	/*used*/unsigned char         chroma_sse_bias_qp0;
	/*used*/unsigned char         chroma_sse_bias_qp1;
	/*used*/unsigned char         chroma_sse_bias_0;
	/*used*/unsigned char         chroma_sse_bias_1;
	/*used*/unsigned char         chroma_sse_bias_2;
	/*used*/unsigned char         sse_lambda_bias_en;
	/*used*/unsigned char         sse_lambda_bias;
	/*used*/unsigned char         dcm_en;
	/*used*/unsigned int        dcm_param;
	/*used*/unsigned char         diff_cplx_thd;
	/*used*/unsigned char         qp;
	/*used*/unsigned char         jrfc_enable;
	/*used*/unsigned char         jrfd_enable;
	/*used*/unsigned int        spe_y_addr;
	/*used*/unsigned int        spe_c_addr;
	/*used*/unsigned char         skip_en;

	/*t33 instead*/unsigned char         r_max_qp;
	/*t33 instead*/unsigned char         r_min_qp;
	/*t33 instead*/unsigned int        bs;                                     //BS output address
	/*t33 instead*/unsigned int        fb[3][2];                               // {curr, ref}{tile_y, tile_c}
	/*t33 instead*/unsigned char         jrfcd_flag;
	/*t33 instead*/unsigned int        jh[3][2];                               // head addr {curr, ref0, ref1}{y, c}
	/*t33 instead*/unsigned char         mb_mode_use;
	/*t32 instead*/unsigned int        emc_bs_pa;
	/*t32 instead*/unsigned int        emc_qpt_pa;
	/*t32 instead*/unsigned int        emc_mod_pa;
	/*t32 instead*/unsigned int        emc_ai_mark_pa;
#if 0
	/*t32 no used*/unsigned char  mb_mode_val;
	/*t32 no used*/unsigned char rotate;
	/*t32 no used*/unsigned char         bs_size_en;
	/*t32 no used*/unsigned int        bs_size;
	/*t32 no used*/unsigned int        raw[3];                                 // {rawy, rawu, rawv} or {rawy, rawc, N/C}
	/*t32 no used*/int         stride[2];                              // {stride_y, stride_c}, only used in raster raw
	/*t32 no used*/unsigned char         size_mode;
	/*t32 no used*/unsigned char         step_mode;
	/*t32 no used*/unsigned int       *mb_mode_info;
	/*t32 no used*/unsigned char         diff_cplx_sel;
	/*t32 no used*/unsigned char         diff_thd_sel;
	/*t32 no used*/unsigned char         diff_qp_base[2];
	/*t32 no used*/unsigned char         diff_qp_sel[2];
	/*t32 no used*/unsigned short        diff_thd_base[3];
	/*t32 no used*/unsigned short        diff_thd_ofst[3];
	/*t32 no used*/unsigned char         sas_eigen_en;
	/*t32 no used*/unsigned char         crp_eigen_en;
	/*t32 no used*/unsigned char         sas_eigen_dump;
	/*t32 no used*/unsigned char         crp_eigen_dump;
	/*t32 no used*/unsigned char         rc_mb_en;
	/*t32 no used*/unsigned char         rc_bu_wait_en;                          // efe chn wait rc qp offset en
	/*t32 no used*/unsigned char         rc_mb_wait_en;                          // efe chn wait rc qp offset en
	/*t32 no used*/unsigned char         rc_bu_num;                              // total basic unit number
	/*t32 no used*/unsigned short        rc_bu_size;                             // mb number in a basic unit
	/*t32 no used*/unsigned char         rc_bu_level;                            // 1:1line 2:2line 3:4line 4:8line 5:16line 6:32line 7:64line
	/*t32 no used*/unsigned char         rc_mb_level;                            // 0:skip 1mb, 1:skip 2mb, 2:skip 4mb, 3:skip 8mb
	/*t32 no used*/int        *mb_ref_info;
	/*t32 no used*/int        *bu_ref_info;
	/*t32 no used*/unsigned int        rc_fh264_rm_tbs;
	/*t32 no used*/unsigned int        avg_bu_bs;
	/*t32 no used*/unsigned short        tar_bs_thd[6];
	/*t32 no used*/char          bu_alg0_qpo[6];
	/*t32 no used*/char          bu_alg1_qpo[6];
	/*t32 no used*/char          mb_cs_qpo[7];
	/*t32 no used*/char          mb_top_bs_qpo[2];
	/*t32 no used*/char          mb_rinfo_qpo[2];
	/*t32 no used*/unsigned short        mb_target_avg_bs[2];
	/*t32 no used*/unsigned short        mb_gp_num;
	/*t32 no used*/unsigned short        last_bu_size;
	/*t32 no used*/unsigned char         rc_bcfg_mode;
	/*t32 no used*/unsigned char         mosaic_en;
	/*t32 no used*/unsigned char         mos_gthd[2];
	/*t32 no used*/unsigned char         mos_sthd[2];
	/*t32 no used*/unsigned char         daisy_chain_en;
	/*t32 no used*/unsigned char         curr_thread_id;
	/*t32 no used*/unsigned char         qp_tab_mode;
	/*t32 no used*/unsigned char         qp_tab_en;
	/*t32 no used*/unsigned int        qp_tab_len;
	/*t32 no used*/unsigned int       *qp_tab;
	/*t32 no used*/unsigned char         sas_en;
	/*t32 no used*/unsigned char         crp_en;
	/*t32 no used*/unsigned char         sas_mthd;
	/*t32 no used*/unsigned short        qpg_mb_thd[7];
	/*t32 no used*/char          qpg_mbqp_ofst[8];
	/*t32 no used*/unsigned char         qpg_flt_thd[5];
	/*t32 no used*/unsigned char         mbrc_qpg_sel;                           //whether use crp/sas qp offset or not, when MB rate control enable.
	/*t32 no used*/unsigned int        emc_dblk_pa;
	/*t32 no used*/unsigned int        emc_recon_pa;
	/*t32 no used*/unsigned int        emc_mv_pa;
	/*t32 no used*/unsigned int        emc_se_pa;
	/*t32 no used*/unsigned int       *emc_qpt_va;
	/*t32 no used*/unsigned int       *emc_rc_va;
	/*t32 no used*/unsigned int        emc_rc_pa;
	/*t32 no used*/unsigned int       *emc_cpx_va;
	/*t32 no used*/unsigned int        emc_cpx_pa;
	/*t32 no used*/unsigned int       *emc_mod_va;
	/*t32 no used*/unsigned int       *emc_ncu_va;
	/*t32 no used*/unsigned int        emc_ncu_pa;
	/*t32 no used*/unsigned int       *emc_sad_va;
	/*t32 no used*/unsigned int        emc_sad_pa;
	/*t32 no used*/unsigned int       *emc_ai_mark_va;
	/*t32 no used*/unsigned char        *emc_cps_ref1;
	/*t32 no used*/unsigned char        *emc_cpsc_addr;
	/*t32 no used*/unsigned char         rrs_dump_en;
	/*t32 no used*/unsigned char         rrs_uv_en;
	/*t32 no used*/unsigned char         rrs_size_y;                             //0: 4, 1: 8, 2: 12, 3: 16
	/*t32 no used*/unsigned char         rrs_size_c;                             //0: 4, 1: 8
	/*t32 no used*/unsigned short        rrs_thrd_y;                             //threshold
	/*t32 no used*/unsigned short        rrs_thrd_u;
	/*t32 no used*/unsigned short        rrs_thrd_v;
	/*t32 no used*/int         cps_size;                               //cps_mode=0: 64, cps_mode=1: 16.
	/*t32 no used*/unsigned short        frm_motion_thrd[4];
	/*t32 no used*/unsigned char         inter_en;
	/*t32 no used*/unsigned char         ras_en;
	/*t32 no used*/int         emc_table_addr;
	/*t32 no used*/unsigned char         emc_cpsc_en;
	/*t32 no used*/unsigned char         skin_dt_en;
	/*t32 no used*/unsigned char         skin_lvl;
	/*t32 no used*/unsigned char         skin_cnt_thd;
	/*t32 no used*/unsigned char         skin_pxlu_thd[3][2];
	/*t32 no used*/unsigned char         skin_pxlv_thd[3][2];
	/*t32 no used*/char          skin_qp_ofst[4];
	/*t32 no used*/unsigned char         mult_factor[3];
	/*t32 no used*/unsigned char         shift_factor[3][3];                     //[0][]:0.4, [1][]:0.6, [2][]: 5.1
	/*t32 no used*/unsigned short        skin_ofst[4];                           //[0]: 1.5, [1]:0.4, [2]:0.6, [3]: 5.1
	/*t32 no used*/unsigned char         ncu_mov_en;
	/*t32 no used*/unsigned int        ncu_move_len;
	/*t32 no used*/unsigned int       *ncu_move_info;
	/*t32 no used*/int         buf_spe_flag;
	/*t32 no used*/int         buf_alg_flag;
	/*t32 no used*/unsigned char         dump_api_en;
	/*t32 no used*/unsigned char         frm_re[4];
	/*t32 no used*/unsigned char         ref_mode;
	/*t32 no used*/unsigned char         lambda_scale_parameter;
	/*t32 no used*/unsigned char         fs_en;                                  // fs function enable
	/*t32 no used*/unsigned int        fs_md;                                  // fs step mode, 0: 1, 1: 3
	/*t32 no used*/unsigned char         fs_px;                                  // fs period x
	/*t32 no used*/unsigned char         fs_py;                                  // fs period y
	/*t32 no used*/unsigned char         fs_rx;                                  // fs range x, must be multiples of 3
	/*t32 no used*/unsigned char         fs_ry;                                  // fs range y, must be multiples of 3
	/*t32 no used*/unsigned char         frm_mv_en;                              // add a frame level mv
	/*t32 no used*/unsigned char         frm_mv_size;                            // mv enable after x mb, x=2^(size+8)
	/*t32 no used*/unsigned char         me_step_en;                             //auto-modify max step number
	/*t32 no used*/unsigned char         me_step_0;                              //step number threshold 0
	/*t32 no used*/unsigned char         me_step_1;                              //step number threshold 1
	/*t32 no used*/unsigned char         use_fast_mvp;
	/*t32 no used*/unsigned char         scaling_list[4][16];
	/*t32 no used*/unsigned char         scaling_list8[2][64];
	/*t32 no used*/unsigned int        intra_mode_msk;
#endif
}h264_rm_t;

struct h264_slice_info {
	//--------------------
	//  base param
	//--------------------
	unsigned char          frame_type;                            // frame type.0: I_SLICE, 1: P_SLICE
	int              frame_width;                           // frame pixel num in width
	int              frame_height;                          // frame pixel num in height
	unsigned short         mb_width;                              // mb num in width
	unsigned short         mb_height;                             // mb num in height
	unsigned char          first_mby;                             // first mb idx
	unsigned char          last_mby;                              // last mb idx. for multi-slice
	unsigned char          raw_format;                            // RAW plane format
	unsigned short         frame_y_stride;                        // frame Y stride
	unsigned short         frame_c_stride;                        // frame UV stride
	char           cqp_offset;                            // [header param] chroma QP offset, -12 ~ 12
	unsigned char          use_intra_in_pframe;                   // enable of use intra prediction in p frame.
															//--------------------
															//  buffer addr
															//--------------------
	/*frame buffer address: all of the buffers should be 256byte aligned!*/
	unsigned int        *des_va;                               // descriptor address
	unsigned int         des_pa;                               // descriptor address
	unsigned int         raw_y_pa;                             // raw Y base physical address
	unsigned int         raw_c_pa;                             // raw C base physical address
	unsigned int         dst_y_pa;                             // address of rebuild frame Y
	unsigned int         dst_c_pa;                             // address of rebuild frame C
	unsigned int         ref_y_pa;                             // address of reference frame 0 Y
	unsigned int         ref_c_pa;                             // address of reference frame 0 C
	unsigned int         mref_y_pa;                            // address of reference frame 1 Y
	unsigned int         mref_c_pa;                            // address of reference frame 1 C
	unsigned int         bitstream_pa;                         // address of bitstream.
														   //------------------------
														   //    EFE Module Param
														   //------------------------
	int              hw_width;                             // frame pixel num in original width
	int              hw_height;                            // frame pixel num in original height
	int              rotate_mode;                          // frame rotate mode. 0: 0, 1: 90, 2: 180, 3: 270
	roi_info_t       roi_info[16];                         // roi configure.
	h264_ifa_para_t       ifa_para;                             // image feature analysis
														   //------------------------
														   //    EMC Module Param
														   //------------------------
														   //input
	unsigned char          bsfull_intr_en;                       // enable of intrupt encode
	unsigned int         bsfull_intr_size;                     // intrupt encode bits size.(KByte)
	unsigned char          emc_tfmc_en;                          // enable  of emc input tfm control
	unsigned char         *emc_bs_addr0;                         // address of emc input
	unsigned char         *emc_bs_addr1;                         // address of emc input
	unsigned int        *emc_qpt_addr;                         // address of emc input
	unsigned int        *emc_mdc_addr;                         // enable of emc input md module block configure.
	unsigned int        *emc_ai_mark_addr;                     // address of emc ai mark
	unsigned char         *emc_tfmc_addr;                        // address of emc input tfm control
	unsigned char         *emc_mcec_addr;                        // address of emc input mce control
														   //output
	unsigned char          emc_cps_en;                           // enable  of emc output cps feature.
	unsigned char          emc_sobel_en;                         // enable  of emc output sobel feature.
	unsigned char          emc_mix_en;                           // enable  of emc output mix feature.
	unsigned char          emc_mixc_en;                          // enable  of emc output mix feature.
	unsigned char          emc_flag_en;                          // enable  of emc output flag feature.
	unsigned char         *emc_cps_addr;                         // address of emc output
	unsigned char         *emc_sobel_addr;                       // address of emc output
	unsigned char         *emc_mix_addr;                         // address of emc output
	unsigned char         *emc_mixc_addr;                        // address of emc output
	unsigned char         *emc_flag_addr;                        // address of emc output
														   //------------------------
														   //    IPRED Module Param
														   //------------------------
														   //ipred bit
	unsigned char          bit_16_en;                            // enalbe of configure i16 bits.[0]: V. [1]: H, [2]:DC, [3]: Planar.
	unsigned char          bit_8_en;                             // enalbe of configure i8  bits.[0]: V. [1]: H, [2]:DC, [3]: Planar.
	unsigned char          bit_4_en;                             // enalbe of configure i4  bits.[0]: V. [1]: H, [2]:DC, [3]: Planar.
	unsigned char          bit_uv_en;                            // enalbe of configure iuv bits.[0]: V. [1]: H, [2]:DC, [3]: Planar.
	unsigned char          bit_16[4];                            // bit value.[0]: V. [1]: H, [2]:DC, [3]: Planar.
	unsigned char          bit_8[4];                             // bit value.[0]: V. [1]: H, [2]:DC, [3]: Planar.
	unsigned char          bit_4[4];                             // bit value.[0]: V. [1]: H, [2]:DC, [3]: Planar.
	unsigned char          bit_uv[4];                            // bit value.[0]: V. [1]: H, [2]:DC, [3]: Planar.
														   //lambda bias
	unsigned char          lamb_16_en;                           // enable of configure i16 lambda bias.
	unsigned char          lamb_8_en;                            // enable of configure i8 lambda bias.
	unsigned char          lamb_4_en;                            // enable of configure i4 lambda bias.
	unsigned char          lamb_uv_en;                           // enable of configure ic lambda bias.
	unsigned char          lambda_info16;                        // value of i16 lambda bias
	unsigned char          lambda_info8;                         // value of i8 lambda bias
	unsigned char          lambda_info4;                         // value of i4 lambda bias
	unsigned char          lambda_infouv;                        // value of ic lambda bias
														   //mode weight
	unsigned char          c_16_en;                              // enable of configure i16 const_value weight.
	unsigned char          c_8_en;                               // enable of configure i8 const_value weight.
	unsigned char          c_4_en;                               // enable of configure i4 const_value weight.
	unsigned char          c_uv_en;                              // enable of configure ic const_value weight.
	unsigned char          const_16[4];                          // weight
	unsigned char          const_8[4];                           // weight
	unsigned char          const_4[4];                           // weight
	unsigned char          const_uv[4];                          // weight
														   //mode priority
	unsigned char          pri_16;                               // enable of i16 mode priority.[0]: DC prior, [1]:V prior, [2]: H prior, [3]: P prior.
	unsigned char          pri_8;                                // enable of i8  mode priority.[0]: DC prior, [1]:V prior, [2]: H prior, [3]: P prior.
	unsigned char          pri_4;                                // enable of i4  mode priority.[0]: DC prior, [1]:V prior, [2]: H prior, [3]: P prior.
	unsigned char          pri_uv;                               // enable of ic  mode priority.[0]: DC prior, [1]:V prior, [2]: H prior, [3]: P prior.
														   //decrease bit
	unsigned char          ref_neb_8;                            // enable of 8x8 reference neighbour mode.
	unsigned char          ref_neb_4;                            // enable of 4x4 reference neighbour mode.
	unsigned char          ref_8;                                // value of decreased 8x8 bits when last_mode = cur_mode.
	unsigned char          ref_4;                                // value of decreased 4x4 bits when last_mode = cur_mode.
														   //------------------------
														   //    MCE Module Param
														   //------------------------
	unsigned char          mref_en;                              // current frame refer to long term reference enable.
	unsigned char          hpel_en;                              // half-pixel search enable. 0: close, 1:open
	unsigned char          qpel_en;                              // quart-pixel search enable. 0:close, 1:open
	unsigned char          scl;                                  // 0: mvp = mv0, 1: mvp = one of {mv0,top_mv,left_mv}, 2: mvp = one of {mv0, top_mv, left_mv, topright_mv, belowleft_mv}
	unsigned char          mce_pref_mode;                        // [0]: mv0 pref_en [1] mv0 pref expd [2] top mv pref_en [3] top mv pref expd
	unsigned int         max_sech_step_i;                      // range 1~192
	unsigned int         max_mvrx_i;                           // max sech mvx range
	unsigned int         max_mvry_i;                           // max sech mvy range
	unsigned char          glb_mv_en;                            // global mv enable
	int              glb_mvx;                              // global mvx value
	int              glb_mvy;                              // global mvy value
	unsigned char          mce_cfg_en;                           // enable of mce block level control
	h264_mcec_range_grp_t mcec_range_grp[4];                    // mce range config table to the encoder
	h264_mcec_grp_t       mcec_grp[4];                          // mce configuration table to the encoder
														   //------------------------
														   //    MD Module Param
														   //------------------------
														   // force i16dc
	unsigned char          force_i16dc;                          // force select i16 dc mode
	unsigned char          i16dc_qp_sel;                         // get qp mode. 0: frame qp, 1: mb qp
	unsigned char          i16dc_qp_base;                        // frame qp threshold
	unsigned char          i16dc_cplx_thd;                       // threshold of mad.
														   // force i16
	unsigned char          force_i16;                            // force select i16 size
	unsigned char          i16_qp_sel;                           // get qp mode. 0: frame qp, 1: mb qp
	unsigned char          i16_qp_base;                          // frame qp threshold
	unsigned char          cplx_thd_sel;                         // get cplx mode. 0:cplx_thd[0], 1: cplx_thd[i]
	unsigned char          cplx_thd[8];                          // threshold of mad with different qp.
	unsigned char          cplx_thd_idx[24];                     // index of cplx_thd[8] with different qp.
														   // refresh
	unsigned char          refresh_en;                           // enable of refresh intra
	unsigned char          refresh_mode;                         // refresh mode
	unsigned char          refresh_cplx_thd;                     // complexity threshold of refresh mb
	unsigned char          refresh_bias;                         // hw do not used
														   // region mode uniform
	unsigned char          inter_nei_en;                         // when neighboor mb mode are all inter and current mb is intra, bits += 128;
														   //pskip
	unsigned char          pskip_en;                             // enable of pskip.
	unsigned char          skip_bias_en;                         // enable of skip type bits add bias. 0: close, 1:pskip bits bias(hw do not support), 2: i4 bits bias
														   //block ctrl
	unsigned char          md_cfg_en;                            // enable of md module block configure.
	unsigned char          mode_ctrl;                            // do not select p skip mode. mode_ctrl & 0x1: nnz || mvd==0. (mode_ctrl >> 2) & 0x1: nnz.
	unsigned int         mode_ctrl_param[40];                  // mode cfg table
														   //ned
	unsigned char          recon_resi_en;                        // enable of ned
	unsigned char          ned_motion_en;                        // motion flag come from. 0:still times, 1:mb_motion
	unsigned char          ned_motion_shift;                     // when ned_motion_en = 1, motion flag valid bit in mb_motion
	unsigned char          ned_sad_thr[2][2][4];                 // ned thr
	unsigned short         ned_sad_thr_cpx_step[2][3];           // ned extra thr
	unsigned char          c_sad_min_bias;                       // ned coeff
	unsigned char          rrs_en;                               // hw do not used, but can't delete.
														   //frm stat
	unsigned char          info_en;                              // enable of stat mb info.
	unsigned char          mvd_sum_all;                          // enable of stat all mb mvd sum
	unsigned char          mvd_sum_abs;                          // enable of stat mvd abs sum
	unsigned char          mv_sum_all;                           // enable of stat all mb mv sum
	unsigned char          mv_sum_abs;                           // enable of stat mvd abs sum
	unsigned int         ysse_thr;                             // threshold of stat mb y sse beyond count. output by reg.
	unsigned int         csse_thr;                             // threshold of stat mb c sse beyond count. output by reg.
	unsigned char          cfg_size_x;                           // block size of stat mv sum. blk16<<2 | blk8<<1 | blk4<<0
	unsigned char          cfg_size_y;                           // block size of stat mv sum. blk16<<2 | blk8<<1 | blk4<<0
	unsigned short         cfg_iw_thr;                           // threshold of stat mb-intra weight
	unsigned short         cfg_mvr_thr1;                         // threshold of stat mb mv range count. output by reg.
	unsigned int         cfg_mvr_thr2;                         // threshold of stat mb mv range count. output by reg.
	unsigned int         cfg_mvr_thr3;                         // threshold of stat mb mv range count. output by reg.
	unsigned char          color_shadow_sse_en;                  // enable of color_shadow_sse
	unsigned char          color_shadow_sse_motion_shift;        // motion level of colow_shadow
	unsigned char          colow_shadow_sse_ratio_thrd[2];       // threshold of color_shadow_sse ratio
														   //------------------------
														   //    TFM Module Param
														   //------------------------
	unsigned char          dct8x8_en;                             // enable of dct 8x8
	int              deadzone[9];                           // deadzone value.[0]: 4IY, [1]: 4PY, [2]: 4IC, [3]: 4PC. [4]: 8IY. [5]: 8PY. [6]: IYDC. [7]: ICDC, [8]: PCDC
	unsigned char          is_scaling_custom;                     // enable of custom scaling list.0: flat scaling list. 1: custom scaling list.
	unsigned char          acmask_en;                             // block level acmask cfg total enable
	unsigned char          acmask_type;                           // acmask type, 0: default, 1: diagonal type
	unsigned char          tu4_mask;                              // (invalid) enable of skip tu4
	unsigned char          jm_lambda2_en;                         // enable of define lambda2 table. 0: default lambda table. 1: adjust smaller lambda table.
	h264_tfm_grp_t        tfm_grp[16];                           // tfm configuration table to the encoder
															//------------------------
															//    DBLK Module Param
															//------------------------
	unsigned char          deblock;                               // deblock enable
	char           alpha_c0_offset;                       // dblk param -6 ~ +6
	char           beta_offset;                           // dblk param -6 ~ +6
	unsigned char          db_prior;                              // hw inside debug.
															//------------------------
															//    SDE Module Param
															//------------------------
	unsigned char          fbc_ep   ;                             // fast bit count
	unsigned char         *state    ;                             // sde state table
	unsigned char          sde_prior;                             // hw inside debug.
	unsigned char          nal_en   ;                             // hw encode nal enable
															//------------------------
															//    BURC Module Param
															//------------------------
	unsigned char          rc_en;                                 // row rc enable
	int              bu_size;                               // lcu numbers in a bu
	int              bu_len;                                // bu numbers in a frame
	unsigned char          rc_method;                             // rc alg sel
	unsigned short         rc_thd_t32[12];                        // rc alg threshold
	short          rc_thd[12];                            // rc alg threshold
	unsigned int         rc_info[128];                          // bu's target bits
	char           bu_max_dqp;                            // bu level max delta qp
	char           bu_min_dqp;                            // bu level min delta qp
	unsigned char          rc_avg_len;                            // dqp = sum >> rc_avg_len.
	unsigned char          rc_max_prop;                           // min proportion of bu target bits compensate
	unsigned char          rc_min_prop;                           // max proportion of bu target bits compensate
	unsigned char          rc_dly_en;                             // calc bu dqp delay use enable
	unsigned char          rc_max_qp;                             // max qp to limit the result of qpg stacked with burc
	unsigned char          rc_min_qp;                             // min qp to limit the result of qpg stacked with burc
	char           rc_dqp_offset;                         // rc_thd's dqp offset
	unsigned int         tgtbs_offset_unit;                     // force adjust dqp bits threshold
	unsigned int         force_max_dqp_bits;                    // force use of max dqp bits threshold
	unsigned char          adjust_unit_size;                      // force adjust dqp unit size
	unsigned short         adjust_unit_len;                       // force adjust dqp unit number in a bu
															//------------------------
															//    JRFCD Module Param
															//------------------------
	unsigned char          jrfc_en;                               // enable  of jzm reference frame compress
	unsigned char          jrfcv2_en;                             // enable  of jzm reference frame compress v2
	unsigned int         head_dsty_addr;                        // address of rebuild frame Y head
	unsigned int         head_dstc_addr;                        // address of rebuild frame C head
	unsigned int         head_refy_addr;                        // address of ref frame Y head
	unsigned int         head_refc_addr;                        // address of ref frame C head
	unsigned int         head_mrefy_addr;                       // address of mref frame Y head
	unsigned int         head_mrefc_addr;                       // address of mref frame C head
	unsigned int         head_sp_dsty_addr;                     // address of rebuild frame sp Y head
	unsigned int         head_sp_dstc_addr;                     // address of rebuild frame sp C head
	unsigned int         lm_head_total;                         // luma head total
	unsigned int         cm_head_total;                         // chroma head total
	unsigned char          body_combine_en;                       // jrfcv2 enable of body order
															//------------------------
															//    BUFSHARE Module Param
															//------------------------
	unsigned char          buf_share_en;                          // enable of recon and ref data share buffer
	unsigned char          buf_share_size;                        // share buffer size. 1 is 128 rows of pixel, 2 is 192, 3 is 256.... 1 at least
	unsigned int         buf_base_yaddr;                        // address of bufshare Y start. fix y addr
	unsigned int         buf_base_caddr;                        // address of bufshare C start. fix c addr
	unsigned int         buf_beyond_yaddr;                      // address of bufshare Y +1.
	unsigned int         buf_beyond_caddr;                      // address of bufshare C +1.
	unsigned int         buf_start_yaddr;                       // address of bufshare recon Y
	unsigned int         buf_start_caddr;                       // address of bufshare recon C
	unsigned int         buf_ref_yaddr;                         // address of bufshare ref Y.
	unsigned int         buf_ref_caddr;                         // address of bufshare ref C.
	int              buf_ref_rem_mby;                       // rem = beyond - start
															//------------------------
															//   SW Param
															//------------------------
	unsigned int         frm_num_gop;                           // frm count in a gop
	unsigned char          maxnum_ref;                            // hw max used reference frame
	unsigned char          use_dummy_byd;
	unsigned int         dummy_buf_beyond_yaddr;
	unsigned int         dummy_buf_beyond_caddr;
	//------------------------
	//   Rm Param
	//------------------------
	h264_rm_t rm;
	unsigned char soc_type;
	/* VPU Daisy Chain setting */
	unsigned char daisy_chain_en;
	unsigned char curr_thread_id;
};
/* vpu h264 vdma struct end */

typedef struct _md_ctl {
	unsigned char cost_max[6];
	unsigned char cost_bias_en[6];
	unsigned char cost_bias_param[6];
	unsigned char pt4fs_dis;
	unsigned char pt8fs_dis;
	unsigned char csse_bias_en[3];
	unsigned char csse_bias_param[9];
	unsigned char ssel_bias_en[6];
	unsigned char ssel_bias_param[6];
	unsigned char ysse_clr[6];
	unsigned char cb_bias_en[5];
	unsigned char cb_bias_param[5];
	unsigned char ccfb_bias_en[2];
	unsigned char ccfb_bias_param[2];
	unsigned char ycfb_clr[5];
}md_ctl;

struct h264_instance {
	struct h264_param param;
	struct h264_header_info header;
	struct h264_slice_info vdma_info;
};

int h264_param_init(h264_t *inst, struct h264_param *param);
void h264_exit(h264_t *inst);
int h264_encode(h264_t *inst, struct vpu_io_info *input);

#endif //_VPU_CORE_H_
