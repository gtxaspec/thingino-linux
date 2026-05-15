#ifndef _H265_CORE_H_
#define _H265_CORE_H_

#include <linux/types.h>
#include "h265_slice.h"
#include "h265_nal.h"
#include "h265_bitstream.h"
#include "vpu_core.h"

//slice type
#define H265_SLICE_TYPE_B   0
#define H265_SLICE_TYPE_P   1
#define H265_SLICE_TYPE_IDR 2
#define H265_SLICE_TYPE_VI  6

typedef struct h265_instance h265_t;

struct h265_header_info {
	NALU nalu;
	Bitstream bs;
	NALList list;
};

struct h265_param {
	unsigned short width;
	unsigned short height;
	unsigned short y_stride;
	unsigned short c_stride;
	unsigned short i_qp;
	unsigned short p_qp;
	unsigned short gop;
	unsigned short fps;
	unsigned short fps_den;
};

/* vpu h265 vdma struct start */
typedef struct {
	struct {
		int ipred;
		int mce_size;
		int mce_ctrl;
		int blk;
		int tfm;
		int csd;
		int mos;
		int dlbda;
		int su;
		int fmd;
		int qpg;
		int csh;
	}func;
	struct {
		int tus;
		int srd;
		int csd;
		int mos;
		int ned;
		int csh;
		int dlbda;
		int mvp_force_zero;
		int pmdctrl;
		int mce;
		int ipred;
	}thrd;
}vpi_method_s;

typedef struct{
  unsigned char       roi_en;
  unsigned char       roi_md;
  char        roi_qp;
  unsigned char       roi_lmbx;
  unsigned char       roi_rmbx;
  unsigned char       roi_umby;
  unsigned char       roi_bmby;
} roi_info_h265_s;

typedef struct h265_ifa_para_t{
  unsigned char        ifa_en;                             // enable of image feature analysis.
  unsigned char        emc_out_en[2];                      // enable of 0: cps only, 1: others
  //--------------------
  //  cps
  //--------------------
  unsigned char        cps_en;                             // enable of downscale raw
  unsigned char        cps_c_en;                           // enable of downscale raw C
  unsigned char       *cps_ref0;                           // ADDR of ref data or last cps_raw data.
  unsigned char       *cps_ref1;                           // ADDR of ref1 data or last cps_raw1 data.
  //--------------------
  //  rrs
  //--------------------
  unsigned char        rrs_en;                             // enable of frame diff. raw-ref0
  unsigned char        rrs1_en;                            // enable of frame diff. raw-ref1
  unsigned char        rrs_c_en;                           // enable of frame diff C. raw-ref0
  unsigned char        rrs1_c_en;                          // enable of frame diff C. raw-ref1
  unsigned char        rrs_thrd;                           // threshold to stat count of Y pixel rrs > thrd  in mb. 0~16
  unsigned char        rrs_thrd_c;                         // threshold to stat count of C pixel rrs > thrd  in mb. 0~4
  //--------------------
  //  motion
  //--------------------
  unsigned char        motion_en;                          // enable of motion flag
  unsigned char        motion_c_en;                        // enable of motion flag C
  unsigned short       motion_thrd;                        // threshold to compare with rrs
  unsigned short       motion_thrd_c;                      // threshold to compare with rrs_c
  unsigned char        motion_of_thrd;                     // threshold to compare with rrs stat cnt
  unsigned char        motion_of_thrd_c;                   // threshold to compare with rrs_c stat cnt
  unsigned short       rrc_motion_thr[7];                  // threshold of different rrs range
  unsigned short       frm_mt_smd_thr;                     // threshold of different rrs smd sum
  //--------------------
  //  still times
  //--------------------
  unsigned char        still_times_en;                     // enable of stat mb still times enable
  unsigned char        still_times_motion_shift;           // set motion flag valid bit.
  unsigned char        max_still_times;                    // set still times up-limit.
  //--------------------
  //  sobel
  //--------------------
  unsigned char        sobel_en;                           // enable of direction detection enable
  unsigned char        sobel_edge_thrd;                    // threshold of grand thrd for detect edge
  unsigned char        sobel_16_or_14;                     // mode of sobel. 0:16x16 pixel 1:14x14 pixel
  unsigned char        ras_en;                             // enable of radix sobel
  //--------------------
  //  smd
  //--------------------
  unsigned char        smd_en;                             // enable of image cplx detection
  unsigned char        smd_c_en;                           // enable of image cplx detection C
    int     crp_thrd;
  unsigned char        row_smd_unit;                       // number of one row_smd_unit 64 lines - 1
  //--------------------
  //  energy
  //--------------------
  unsigned char        ery_en;                             // enable of mb energy
  //--------------------
  //  skin
  //--------------------
  unsigned char        skin_en;                            // enable of skin detection
  unsigned char        skin_lvl_en[3];                     // enable of 3 level skin detection
  unsigned char        skin_cnt_thrd;                      // threshold of skin count
  unsigned char        skin_thrd[3][2][2];                 // [sobel][u/v][max/min]
  short        skin_level[4];                      // formula coefficient.
  unsigned char        skin_mul_factor[4];                 // formula coefficient.
  //--------------------
  //neighbour_cplx
  //--------------------
  unsigned char        nei_cplx_en;                        // enable of region cplx
  //--------------------
  //small-blk flat
  //--------------------
  unsigned char        var_flat_en;                        // enable of small-blk flat detection
  unsigned char        var_flat_thr[2][2];                 // threshold of variation.
  //--------------------
  //subblk uniformation
  //--------------------
  unsigned char        cu_unifor_en;                       // enable of inside cplx uniformation
  unsigned char        var_split_thrd;                     // threshold of allowned variation smd range. only for blk32
  unsigned char        smd_blk16_bias[2];                  // bias of allowned variation. only for blk16
  //--------------------
  // cplx detect
  //--------------------
  unsigned short       smd_blk8_thr[6];                    // threshold of cplx detection. [thr0~5]
  unsigned short       smd_blk16_thr[8][2];                // threshold of cplx detection. [thr0~7][y,c]
  unsigned short       smd_blk32_thr[3][2];                // threshold of cplx detection. [thr0~1][y,c]
  unsigned short       sobel_blk16_thr[3];                 // threshold of cplx detection. [sad,dif,cnt]
  unsigned short       sobel_blk32_thr[3];                 // threshold of cplx detection. [sad,dif,cnt]
  //--------------------
  //  AI mark
  //--------------------
  unsigned char        ai_mark_en;                         // enable of ai mark
  unsigned char        ai_mark_type;                       // num of ai mark type.
  //--------------------
  //  refresh
  //--------------------
  unsigned char        refresh_en;                         // enable of refresh.
  unsigned char        refresh_mode;                       // enable of final refresh flag
  unsigned char        rfh_raw_thrd;                       // threshold of luma avg feature
  unsigned char        rfh_edge_thrd;                      // threshold of edge feature
  int            rfh_flat_thrd;                      // threshold of flat feature
  unsigned char        rrs_filter_mode;                    // mode of motion feature filter. 0: normal. 1: not change motion to static
  unsigned char        filter_thrd[4];                     // threshold of filter.
  //--------------------
  //  pet mark
  //--------------------
  unsigned char        petbc_en;                           // enable of petbc flag
  unsigned char        pet_mode;                           // petbc flag mode. 0: only calculate petbc0, 1: calculate petbc0 and petbc1
  char         pet_fil_en;                         // enable of petbc flag filter
  unsigned short       petbc_var_thr[3];                   // threshold of petbc0
  unsigned short       petbc_ssm_thr[3][4];                // threshold of petbc0
  unsigned short       petbc2_var_thr[3];                  // threshold of petbc1
  unsigned short       petbc2_ssm_thr[3][4];               // threshold of petbc1
  //--------------------
  //  qpg
  //--------------------
  unsigned char        qpg_en;                             // enable of mb qp.
  unsigned char        base_qp;                            // base qp of mb level.
  unsigned char        max_qp;                             // max qp of mb level.(after roi or qpt)
  unsigned char        min_qp;                             // min qp of mb level.(after roi or qpt)
  unsigned char        qpg_max_qp;                         // max qp of qpg.(after cplx qpg)
  unsigned char        qpg_min_qp;                         // min qp of qpg.(after cplx qpg)
  unsigned char        qpg_cu32_qpm;                       // mode of cu32 qp. 0:avg, 1:first.
  unsigned char        qpg_filte_en;                       // enable of qpg filter
  //qpg  tab
  unsigned char        qpg_table_en;                       // enable of qpg table
  //qpg roi
  unsigned char        qpg_roi_en;                         // enable of qpg roi
  //qpg skin
  unsigned char        qpg_skin_en;                        // enable of qpg skin
  char         qpg_skin_qp_ofst[3];                // offset of qp with diffenent skin range
  //qpg enery
  unsigned char        qpg_ery_en;                         // enable of qpg energy
  //qpg sobel
  unsigned char        qpg_sobel_en;                       // enable of qpg sobel
  short        qpg_sobel_cplx_thrd[8];             // threshold of sobel gradient threshold
  //qpg smd
  unsigned char        qpg_smd_en;                         // enable of qpg smd
  int            smd_filter_thrd;                    // threshold of smd filter
  short        qpg_smd_cplx_thrd[8];               // threshold of smd range.
  char         qpg_cplx_qp_ofst[8];                // offset of qp with different smd range.
  //qpg petbc
  unsigned char        qpg_petbc_en;                       // enable of qpg petbc
  unsigned char        qpg_pet_cplx_thrd[3][12];           // threshold of smd range
  unsigned char        qpg_pet_qp_ofst[5][7];              // offset of qp with different smd range
  unsigned char        qpg_pet_qp_ofst_mt[5][7];           // offset of qp with different smd range in motion region
  unsigned char        qpg_dlt_thr[4];                     // threshold of
  unsigned char        pet_filter_valid[4];                // whether neighbour block petbc flag is valid. [0].left, [1]:right, [2]: top, [3]: bottom.
  unsigned char        qpg_pet_qp_idx[5];                  // idx of qp ofst with different qp.
  //qpg ai
  char         qpg_ai_motion_smd_ofst[3][2][8];    // offset of qpg ai with different motion and cplx region.
  //--------------------
  //  data stat
  //--------------------
  unsigned short       frm_cplx_thrd[2];                   // threshold of different cplx strong. [0] smd thrd, [1] sobel thrd.
}h265_ifa_para_t;

typedef struct h265_tfm_grp_t {
  unsigned char        rdoq_level_y[3];                    // 4-bit uns, range 0 ~ 9. 0:off, suggest:5~7, [0] search, [1] merge, [2] intra
  unsigned char        rdoq_level_c[3];                    // 4-bit uns, range 0 ~ 9. 0:off, suggest:5~7, [0] search, [1] merge, [2] intra
  unsigned char        deadzone_y_en[3];                   // 1-bit uns,  [0] search, [1] merge, [2] intra
  short        deadzone_y[3];                      // 13-bit tc,  [0] search, [1] merge, [2] intra
  unsigned char        deadzone_c_en[3];                   // 1-bit uns,  [0] search, [1] merge, [2] intra
  short        deadzone_c[3];                      // 13-bit tc,  [0] search, [1] merge, [2] intra
  unsigned short       acmask_y[3];                        // 14-bit uns, [0] search, [1] merge, [2] intra
  unsigned short       acmask_c[3];                        // 14-bit uns, [0] search, [1] merge, [2] intra
} h265_tfm_grp_t;

typedef struct tfm_rdoq_param_t {
  unsigned char        clear1;                             // 1-bit uns, default 0
  unsigned char        val0[4];                            // 2-bit uns
  unsigned char        val1[4];                            // 2-bit uns
  unsigned char        score[4];                           // 5-bit uns
  unsigned char        tu8_last[3];                        // 2-bit uns
  unsigned char        tu16_last[3];                       // 3-bit uns
  unsigned char        tu32_last[3];                       // 4-bit uns
  unsigned char        delta[4];                           // 2-bit uns
} tfm_rdoq_param_t;

typedef struct h265_mcec_range_grp_t {
    unsigned short    mcec_lx;                                    // left search range
    unsigned short    mcec_rx;                                    // right search range
    unsigned short    mcec_ty;                                    // top search range
    unsigned short    mcec_by;                                    // bottom search range
    unsigned short    skip_pixel;                                 // search pixel step
} h265_mcec_range_grp_t;
typedef struct h265_mcec_grp_t {
    unsigned char     mcec_en;                                    // enable block level mce control
    unsigned char     mcec_mode;                                  // search mode, 0: only dia, 1: only fs, 2: fs->dia, 3:cros->dia, 4:cros->fs, 5:cros->fs->dia
    unsigned char     fs_cfg_idx;                                 // full search h265_mcec_range_grp_t group index
    unsigned char     cros_cfg_idx;                               // cross search h265_mcec_range_grp_t group index
    unsigned char     force_zero_mv_en;                           // force mv (0,0)
    unsigned char     mvc_bc_wei;                                 // mvc cost bits weight (only 265)
    unsigned char     sech_bc_wei;                                // me cost bits weight (only 265)
} h265_mcec_grp_t;
typedef struct h265_rm_t {
    /*ok*/unsigned char*       refy;                                //ref or cps-raw base address;
    /*ok*/unsigned char*       ref1;                                //ref1 or cps-raw1 base address;
    /*ok*/unsigned char        cps_mode;                            //0: 2x2, 1: 4x4
    /*ok*/unsigned char        rrs_mode;                            //0: raw-raw, 1: raw-ref
    /*ok*/unsigned char        rrs_of_en;                           //overflow enable
    /*ok*/unsigned short       frm_motion_thrd[4];
    /*ok*/unsigned char*       refc;                                //ref chroma  base address;
    /*ok*/unsigned char*       ref1c;
    /*ok*/unsigned char        raw_avg_en;
    /*ok*/unsigned char        frame_qp_svac;                       // frame start qp for svac;
    /*ok*/unsigned char        tu_split_en;                         // (inter & !2nx2n) tu split enable
    /*ok*/unsigned char        gray_en;
    /*ok*/unsigned char        ppmd_cost_thr[4];
    /*ok*/unsigned char        pmd_qp_thrd[7];
    /*ok*/unsigned char        ppmd_thr_list[30][2];
    /*ok*/unsigned char        ppmd_merge_thr_idx[40];
    /*ok*/unsigned char        ppmd_search_thr_idx[30];
    /*ok*/unsigned char        ppmd_merge_search_thr_idx[30];
    /*ok*/unsigned char        ppmd_intra_thr_idx[30];
    /*ok*/unsigned char        sse_mask;                            // DO NOT TOUCH: legacy of T30
    /*ok*/unsigned char        scaling_list_present;                // only 0 is supported
    /*used*/unsigned short       mce_resi_thr[2][3][6];               //[y,uv][cu16, cu32, cu64][sad, max, avg]
    /*ok*/unsigned char        ppred_slv_dif;		                    // 0:ppred use ipred_n for ctrl 1:ppred use separate ipx_n for ctrl
    /*ok*/unsigned char        md_mvs_all;                          // value 0xF means all cus' mv sum, value 0 means sum of mv after mode decision
    /*ok*/unsigned char        md_mvs_abs;                          // value 0xF means mv abs sum, value 0 means sum of mv algebraic sum
    /*ok*/unsigned char        md_pre_pmd[2];                       // [0]:low msk, [1]: high msk
    /*ok*/unsigned char        md_pre_csd[2];                       // [0]:cu8, [1]:cu16. value 0:3sse, 1:4sa8d, 2:auto
    /*ok*/unsigned char        md_force_split;                      // force cu split if qps inside are not the same
    /*ok*/unsigned char        md_pmd_bias[3][2];                   // [3]:cu8~32        [2]:enable,value
    /*ok*/unsigned char        md_csd_bias[4][2];                   // [4]:cu8~642       [2]:enable,value
    /*ok*/unsigned char        md_lambda_bias[3][2];                // [3]:sa8d,sse,stc. [2]:enable,value
    /*ok*/unsigned char        md_fskip_en;                         // force skip(merge) mode enable
    /*ok*/unsigned char        md_fskip_val[8][16];                 // [lcuy>>1][lcux>>1]. max 2kx1k
    /*ok*/unsigned char        blk_pmdctrl_en;                      // block level pred mode decision control enable
    /*ok*/unsigned char        tus_ifa_intra32_en;
    /*ok*/unsigned char        tus_ifa_intra16_en;
    /*ok*/unsigned char        tus_ifa_inter32_en;
    /*ok*/unsigned char        tus_ifa_inter16_en;
    /*ok*/unsigned char        tus_mce_inter32_en;
    /*ok*/unsigned char        tus_mce_inter16_en;
    /*ok*/unsigned char        roi_flag;
    /*ok*/unsigned char        tus_mce_inter8_en;
    /*ok*/unsigned char        p_pmd_en;
    /*ok*/unsigned char        csd_p_cu64_en;
    /*ok*/unsigned char        srd_cfg;
#if 0
    /*t33 instead*/unsigned char        mrg_en; //mrg1_en                             // range 0~1, cu8_mrg1_en
    /*t33 instead*/int        crp_thrd;
    /*t33 instead*/unsigned char        emc_mdc_en;
    /*t33 instead*/unsigned char        emc_ipc_en;
    /*t33 instead*/unsigned char        emc_mcec_en;
    /*t33 instead*/unsigned short       tfm_path_en[2];                      // [0]: cfg of HEVC, [1]: cfg of SVAC2
    /*t33 instead*/unsigned char        refine_cu8_mode;
    /*t33 instead*/unsigned char        refine_cu16_mode;
    /*t33 instead*/unsigned char        refine_cu32_mode;
    /*t33 instead*/unsigned char        color_shadow_qp;
    /*t33 instead*/unsigned char        color_shadow_motion_shift_sse;       //color_shadow detect use chrom sse

    /*t33 instead*/unsigned char        compress_flag;                       // compress rebuild frame flag
    /*t33 instead*/unsigned int       head_dsty;                           // rebuild frame Y head physical address
    /*t33 instead*/unsigned int       head_dstc;                           // rebuild frame C head physical address
    /*t33 instead*/unsigned int       head_refy;                           // ref frame Y head physical address
    /*t33 instead*/unsigned int       head_refc;                           // ref frame C head physical address
    /*t33 instead*/unsigned int       head_mrefy;                          // ref frame Y head physical address
    /*t33 instead*/unsigned int       head_mrefc;                          // ref frame C head physical address
    /*t33 instead*/unsigned char        buf_share_flag;
    //------------------------------
    //  t32 no used
    //------------------------------
    /*t32 no used*/unsigned char        md_fskip_cfg;                        // force skip(merge) mode configure. 0:reserved, 1:cu64, other:reserved
    /*t32 no used*/unsigned char        nv12_flag;
    /*t32 no used*/unsigned int       dst_y_pa_nv12;                       // rebuild frame Y physical address for dec nv12
    /*t32 no used*/unsigned int       dst_c_pa_nv12;                       // rebuild frame C physical address for dec nv12
    /*t32 no used*/unsigned short      dst_y_stride_nv12;                   // rebuild frame Y stride, default equal to frame_width
    /*t32 no used*/unsigned short       dst_c_stride_nv12;                   // rebuild frame C stride, default equal to frame_width
    /*t32 no used*/unsigned short       ned_qp_sad_thr_tbl[10];
    /*t32 no used*/unsigned char        srd_mode_ctrl;
    /*t32 no used*/unsigned char        keyint;
    /*t32 no used*/unsigned char        dlambda_framelevel_en;
    /*t32 no used*/char         dlambda_IKeyFrame;
    /*t32 no used*/char         dlambda_PKeyFrame;
    /*t32 no used*/char         dlambda_PNorFrame;
    /*t32 no used*/unsigned char        dlambda_ipmd_en;
    /*t32 no used*/unsigned char        color_shadow_en;
    /*t32 no used*/unsigned char        color_shadow_motion_shift;
    /*t32 no used*/unsigned char        color_shadow_sad_thrd[2][5];         //[nei_cplx:0/1][0:sad_u/sad_v 1:sad_u+sad_v 2:sad_y+sad_u+sad_v 3:sad_u/sad_v 4:sad_u+sad_v]
    /*t32 no used*/unsigned char        step_adapt_en;
    /*t32 no used*/unsigned char        step_thrd_0;
    /*t32 no used*/unsigned char        step_thrd_1;
    /*t32 no used*/unsigned short       intra_resi_thr[2][3][6];             //[y,uv][cu8,  cu16, cu32][sad, max, avg]
    /*t32 no used*/unsigned short       recon_resi_thr[2][4][2];             //[shadow,dot][cu8, cu16, cu32, cu64][y,uv]
    /*t32 no used*/unsigned short       ifa_raw_thr[8][2];                   //[tus intra, tus inter, resi recon][y,uv]
    /*t32 no used*/unsigned char        intra_resi_en[3];                    //[cu8, cu16, cu32]
    /*t32 no used*/unsigned char        i_intra_pu4_en;                      // range 0~1, pu4 enable
    /*t32 no used*/unsigned char        p_intra_pu4_en;
        /*t32 no used*/unsigned short       crp_cu16_threshold[7];
    /*t32 no used*/unsigned short       crp_cu32_threshold[7];
    /*t32 no used*/char         crp_cu16_offset[8];
    /*t32 no used*/char         crp_cu32_offset[8];
    /*t32 no used*/unsigned char        crp_filter_limit_threshold;
    /*t32 no used*/unsigned char        crp_filter_top_threshold;
    /*t32 no used*/unsigned char        crp_filter_left_threshold;
    /*t32 no used*/unsigned char        crp_filter_right_threshold;
    /*t32 no used*/unsigned short       sas_cu16_threshold[6];
    /*t32 no used*/unsigned short       sas_cu32_threshold[6];
    /*t32 no used*/char         sas_cu16_offset[7];
    /*t32 no used*/char         sas_cu32_offset[7];
    /*t32 no used*/unsigned char        ppmd_merge_thr[2][4][5][2];
    /*t32 no used*/unsigned char        ppmd_search_thr[2][3][5][2];
    /*t32 no used*/unsigned char        ppmd_merge_search_thr[2][3][5][2];
    /*t32 no used*/unsigned char        ppmd_intra_thr[2][3][5][2];
    /*t32 no used*/unsigned char        motion_flag;

    /*t32 no used*/unsigned char        petbc;
    /*t32 no used*/unsigned char        depth;
    /*t32 no used*/unsigned char        emc_cpsc_en;
    /*t32 no used*/unsigned char        emc_bslen_en;
    /*t32 no used*/unsigned char*       emc_cpsc_addr;
    /*t32 no used*/unsigned char*       emc_bslen_addr;
    /*t32 no used*/unsigned char        dblk_gray_en;                        // only Y, no UV
    /*t32 no used*/unsigned char        filter_level;                        //svac2. dblk open when != 0.
    /*t32 no used*/unsigned char        mb_lim;                              //svac2. thresh 0
    /*t32 no used*/unsigned char        lim;                                 //svac2. thresh 1
    /*t32 no used*/unsigned char        hev_thr;                             //svac2. thresh 2
    /*t32 no used*/unsigned char        mb_lim_p;                            //svac2. thresh 0 for inter
    /*t32 no used*/unsigned char        lim_p;                               //svac2. thresh 1 for inter
    /*t32 no used*/unsigned char        hev_thr_p;                           //svac2. thresh 2 for inter
    /*t32 no used*/int        stride_refy;                         //ref y stride;
    /*t32 no used*/int        cps_size;
    /*t32 no used*/unsigned char        cps1_copy_en;
    /*t32 no used*/unsigned char        motion_level;
    /*t32 no used*/unsigned char        pet_cnt_alg;
    /*t32 no used*/unsigned char        qpg_mt_idx;
    /*t32 no used*/unsigned char*       ai_mark_input;
    /*t32 no used*/unsigned char        ai_scaling_factor;
    /*t32 no used*/unsigned char        ai_prev_mark_en;
    /*t32 no used*/unsigned char        ai_collect_blk8;
    /*t32 no used*/unsigned char        time_out_threshold;                  // 0-3, time out threshold value
    /*t32 no used*/unsigned char        efe_mode;                            // default value is 0;
    /*t32 no used*/unsigned short       qp_table_len;                        // QP table len, not bigger then 1024
    /*t32 no used*/unsigned int*      qp_table;                            // QP table virtual address, only for vdma chain.
    /*t32 no used*/unsigned char        qp_mode;                             // 0 - all close, 1 - crp, 2 - sas, 3 - msas, 4 - tab+crp, 5 - tab+sas, 6 - tab+msas
    /*t32 no used*/unsigned char        qp_table_cu32_mode;                  // CU32 QP select, 0 - first CU16 QP, 1 - average CU16 QP.

#endif
    /*no used*/unsigned char        use_hp;                              //code mvd use hp, svac2
    /*no used*/unsigned char        interp_filter;                       //default0,svac2
    /*no used*/unsigned char        tfm_buf_en;                          // DO NOT TOUCH: legacy of T30 // bit0:Y8; bit1:Y16; bit2:Y32; bit3:Y64; bit4:C
} h265_rm_t;

struct h265_slice_info {
  //--------------------
  //  base param
  //--------------------
  unsigned char        is_decode;                          // 0: ENC, 1: DEC
  unsigned char        work_mode;                          // encoder and image feature analysis. must be 2,
  unsigned char        frame_type;                         // [header param] 0: B, 1: P, 2: I
  unsigned char        raw_format;                         // [header param] 0: NV12, 1: NV21
  unsigned char        video_type;                         // 0: HEVC, 1: SVAC2 //needtodo:t33 delete?
  unsigned short       frame_width;                        // [header param] pixel width of frame
  unsigned short       frame_height;                       // [header param] pixel height of frame
  unsigned char        frame_qp;                           // [header param] frame start qp;
  char         frame_cqp_offset;                   // [header param] chroma QP offset, -12 ~ 12
  unsigned char        tu_inter_depth;                     // [header param] for inter tu split, 1 or 2
  unsigned char        use_dqp_flag;                       // [header param] code dqp enable
  unsigned char        dqp_max_depth;                      // [header param] code dqp max depth
  unsigned char        sign_hide_flag;                     // [header param] coeff sign hidden enable
  unsigned char        ipred_usis;                         // [head param] 1:ipred cu32 use strong intra stooming
  unsigned short       frame_y_stride;                     // frame Y stride
  unsigned short       frame_c_stride;                     // frame UV stride
  unsigned short       dst_y_stride;                       // rebuild frame Y stride, default equal to frame_width
  unsigned short       dst_c_stride;                       // rebuild frame C stride, default equal to frame_width
  unsigned short       ref_y_stride;                       // reference frame Y stride, default equal to frame_width
  unsigned short       ref_c_stride;                       // reference frame C stride, default equal to frame_width
  unsigned char        dump_api_en;
  //--------------------
  //  buffer addr
  //--------------------
  unsigned int*      des_va;                             // address of vdma descript chain. virtual
  unsigned int       des_pa;                             // address of vdma descript chain
  unsigned int       raw_y_pa;                           // address of raw frame Y
  unsigned int       raw_c_pa;                           // address of raw frame C
  unsigned int       dst_y_pa;                           // address of rebuild frame Y
  unsigned int       dst_c_pa;                           // address of rebuild frame C
  unsigned int       ref_y_pa;                           // address of reference frame 0 Y
  unsigned int       ref_c_pa;                           // address of reference frame 0 C
  unsigned int       mref_y_pa;                          // address of reference frame 1 Y
  unsigned int       mref_c_pa;                          // address of reference frame 1 C
  unsigned int       bitstream_pa;                       // address of bitstream.
  //------------------------
  //    EFE Module Param
  //------------------------
  unsigned short       efe_hw_width;                       // pixel width of origin frame
  unsigned short       efe_hw_height;                      // pixel height of origin frame
  unsigned char        chng_en;
  unsigned char        mce_cfg_en;                         // enable of mce module block configure.
  unsigned char        md_cfg_en;                          // enable of md module block configure.
  unsigned char        ip_cfg_en;                          // enable of md module block configure.
  unsigned char        rotate_mode;                        // raw rotate mode. 0: do not rotate. 1:90. 2:180, 3:270
  roi_info_h265_s     roi_info[16];                  // roi configure.
  h265_ifa_para_t     ifa_para;                           // image feature analysis
  //------------------------
  //    EMC Module Param
  //------------------------
  //intput
  unsigned char         emc_mvi_en;                        // enable of emc input last frame mb-mv.
  unsigned char         emc_ai_en;                         // enable of emc input ai block configure.
  unsigned char         emc_qpt_en;                        // enable of emc input qp table configure.
  unsigned char*        emc_mvi_addr;                      // address of emc input
  unsigned char*        emc_ai_addr;                       // address of emc input
  unsigned char*        emc_ipc_addr;                      // address of emc input
  unsigned char*        emc_mcec_addr;                     // address of emc input
  unsigned char*        emc_mdc_addr;                      // address of emc input
  unsigned char*        emc_qpt_addr;                      // address of emc input
  //output
  unsigned char         bsfull_intr_en;                    // enable of intrupt encode
  unsigned int        bsfull_intr_size;                  // intrupt encode bits size.(KByte)
  unsigned char         emc_bs_en;                         // enable  of emc output bitstream.
  unsigned char         emc_cps_en;                        // enable  of emc output cps feature.
  unsigned char         emc_sobel_en;                      // enable  of emc output sobel feature.
  unsigned char         emc_mix_en;                        // enable  of emc output mix feature.
  unsigned char         emc_mixc_en;                       // enable  of emc output mix feature.
  unsigned char         emc_flag_en;                       // enable  of emc output flag feature.
  unsigned char         emc_mvo_en;                        // enable  of emc output current frame mb-mv
  unsigned char*        emc_bs_addr0;                      // address of emc input
  unsigned char*        emc_bs_addr1;                      // address of emc input
  unsigned char*        emc_cps_addr;                      // address of emc output
  unsigned char*        emc_sobel_addr;                    // address of emc output
  unsigned char*        emc_mix_addr;                      // address of emc output
  unsigned char*        emc_mixc_addr;                     // address of emc output
  unsigned char*        emc_flag_addr;                     // address of emc output
  unsigned char*        emc_mvo_addr;                      // address of emc output
  //------------------------
  //    IPRED Module Param
  //------------------------
  unsigned char         sobel_en;                          // 0:close sobel 1:open sobel
  unsigned char         ipred_size;                        // 7:all 3 channel open 3bit
  unsigned char         sobel_size;                        // 0:all 3 channel open 3bit
  unsigned char         ipmd_mpm_use_maxnum;               // ipred candList use mpm max num
  unsigned char         md_bc_close;                       // 1:close sobel 0:open sobel
  unsigned char         intra_pu4_en;                      // enable of pu4. range 0~1
  //ppred
  unsigned char         ppred_slv_en;                      // ppred use slave mode only need ppred_n==1  1bit
  unsigned char         ppred8_slv_en;                     // ppred 8x8 use slave mode only need ppred_n==1   1bit
  unsigned char         ppred16_slv_en;                    // ppred 16x16 use slave mode only need ppred_n==1  1bit
  unsigned char         ppred32_slv_en;                    // ppred 32x32 use slave mode only need ppred_n==1  1bit
  unsigned char         ppred_mode_num;                    // 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
  unsigned char         ppred8_mode_num;                   // 8x8 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
  unsigned char         ppred16_mode_num;                  // 16x16 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
  unsigned char         ppred32_mode_num;                  // 32x32 0:close ppred 1:4mode 2:6mode 3:10mode 3bit
  //ipred
  unsigned char         ipred_slv_en;                      // ipred use slave mode 1bit
  unsigned char         intra_mode[5];                     // mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
  unsigned char         intra8_mode[5];                    // mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
  unsigned char         intra16_mode[5];                   // mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
  unsigned char         intra32_mode[5];                   // mode4[range2~34] mode3[range0~34] mode2[range0~34] mode1[range0~34] mode0[range0~34]
  //ipred bc tab
  unsigned char         frmb_ip_bits_en[5];                // enable of bc cfg. range 0~1 , y_bits_en
  unsigned char         intra_bits[16][5];                 // bc tab. 16*5*4bits(0~15)
  unsigned char         frmb_idx[5];                       // bc tab idx. range 0~15, y_bits,y,y,uv_bits,uv; default: 1 2 5 0 4
  //ipmd bias
  unsigned char         i_pmd_en;                          // enable of ipmd
  unsigned char         frm_ipmd_angle_en;                 // [angle strong]: 0: angle strong bias for all mode, 1: angle strong bias for angle mode
  unsigned char         intra_angle_bias[4];               // [angle strong]: [enable, cu16, cu32]
  unsigned char         frmb_ip_bias[5][2];                // [cost bias]: range 0~30, sad-cost bias; [1]dc,[0]planar
  unsigned char         frmb_ipc_bias[5];                  // [cost bias]: range 0~30, sad-cost bias; dm
  unsigned char         ipmd_mpm_cost_bias[3][2][4];       // [plain mpm bias]
  unsigned char         ipmd_dm_cost_bias[3][5];           // [plain mpm bias]
  unsigned char         ipmd_ras_en;                       // enable of ras, ifa ras to ipred
  unsigned short        ipmd_sobel_thr[3];                 // thrd of ras, ifa ras to ipred
  unsigned char         ipmd_sobel_edge_thrd;              // thrd of sobel, ifa ras to ipred
  //------------------------
  //    MCE Module Param
  //------------------------
  unsigned char         inter_mode[4][4];                  // [depth]: {merge, 2NxN, Nx2N, 2Nx2N}
  unsigned char         half_pixel_sech_en;                // half-pixel search enable. 0: close, 1:open
  unsigned char         quart_pixel_sech_en;               // quart-pixel search enable. 0:close, 1:open
  unsigned char         subpixel_sech_cu8_en;              // cu8  do sub-pixel sech enable. 0: close, 1: open
  unsigned char         subpixel_sech_cu16_en;             // cu16 do sub-pixel sech enable. 0: close, 1: open
  unsigned char         subpixel_sech_cu32_en;             // cu32 do sub-pixel sech enable. 0: close, 1: open
  unsigned char         mce_scl_mode;                      // 0: mvp = mv0, 1: mvp = one of {mv0,top_mv,left_mv}, 2: mvp = one of {mv0, top_mv, left_mv, topright_mv, belowleft_mv}
  unsigned char         mce_pref_mode;                     // [0]: mv0 pref_en [1] mv0 pref expd [2] top mv pref_en [3] top mv pref expd [4] left mv pref_en [5] left mv pref expd
  unsigned char         merge_cand_num;                    // mvp cand number
  unsigned char         max_sech_step;                     // range 1~192
  short         mce_slc_mvx;                       // slice mv, for sech start point
  short         mce_slc_mvy;                       // slice mv, for sech start point
  short         max_mvrx;                          // max sech mvx range
  short         max_mvry;                          // max sech mvy range
  h265_mcec_range_grp_t mcec_range_grp[4];                // mce range config table to the encoder
  h265_mcec_grp_t      mcec_grp[4];                       // mce configuration table to the encoder
  //long term reference
  unsigned char         mref_en;                           // current frame refer to long term reference enable.
  unsigned char         mref_wei;                          // mref distortion >>= mref_wei.
  //merge speed up
  unsigned char         mrg1_en;                           // merge do merge cand[1] enable, only support cu8. 0:close, 1:open
  unsigned char         mrg_fpel_en;                       // limit merge only use full-pixel cand mv enable. 0: no limit. 1: limit cu8, 2: limit cu16, 3: limit cu8 and cu16.
  //sech force mv0
  unsigned char         sech_force_mv0_en;                 // sech force mv0 for srd
  unsigned char         sech_force_mv0_mode;               // 0: from ddr, 1: mb_motion, 2: ddr || mb_motion, 3: ddr && mb_motion
  //------------------------
  //    MD Module Param
  //------------------------
  unsigned char         ccf_rm_en;                         // mode of check coef is zero.0: ignore left_top 3 pixel, 1: ignore left_top 1 pixel.
  unsigned char         rfh_sel_intra32_en;                // enable of cu32 refresh region select intra.
  unsigned char         rfh_sel_intra16_en;                // enable of cu16 refresh region select intra.
  unsigned char         rfh_msk_skip32_en;                 // enable of cu32 refresh region mask skip.
  unsigned char         rfh_msk_skip16_en;                 // enable of cu16 refresh region mask skip.
  unsigned char         uv_dist_scale_en;                  // enable of distortion_c * weight
  unsigned char         uv_dist_scale[3];                  // param of distortion_c weight
  unsigned int        md_cfg_grp[16][2];                 // md block configure table
  //srd
  unsigned char         srd_en;                            // enable of srd force select search skip
  unsigned char         mf_sel_sech_skip32_en;             // enable of srd cu32
  unsigned char         mf_sel_sech_skip16_en;             // enable of srd cu16
  unsigned char         srd_motion_shift;                  // srd flag valid bit in mb_motion
  //csd
  unsigned char         i_csd_en;                          // I frame use csd for cu partition
  unsigned char         p_csd_en;                          // P frame use csd for cu partition
  unsigned char         csd_qp_thrd[5];                    // [Ringing Effect] diff qp with diff var_flat_recon_thrd
  unsigned char         var_flat_recon_thrd[2][5];         // [Ringing Effect] [2]:{luma, chroma}, [5]:diff qp thrd
  unsigned char         var_flat_sub_size;                 // [Ringing Effect] block size
  unsigned char         csd_color;                         // [Ringing Effect] 0: only luma; 1: luma+chroma
  //ned
  unsigned char         ned_en;                            // enable of ned
  unsigned char         recon_resi_en[4][2];               // [cu8, cu16, cu32, cu64][shadow,dot]
  unsigned char         rcn_plain_en;                      // check recon plain
  unsigned char         ned_score_table[2][2][8];          // for ned cost
  unsigned char         ned_score_bias[2];                 // for ned cost
  unsigned char         ned_motion_en;                     // motion flag come from. 0:still times, 1:mb_motion
  unsigned char         ned_motion_shift;                  // when ned_motion_en = 1, motion flag valid bit in mb_motion
  unsigned char         ned_block_frmnum;                  // when ned_motion_en = 0, still times thrd
  unsigned char         ned_sad_thr[2][2][4];              // ned thr
  unsigned short        ned_sad_thr_cpx_step[2][3];        // ned extra thr
  unsigned char         c_sad_min_bias;                    // ned coeff
  //mosaic
  unsigned char         mosaic_en;                         // enable of mosaic
  unsigned char         mosaic_intra_texture_en;           // do not touch texture mosaic
  unsigned char         mosaic_motion_shift;               // motion flag valid bit
  unsigned char         mosaic_recon_flat_thr[4];          // 8x8 16x16 32x32 64x64
  unsigned char         mosaic_diff_thr[4];                // 8x8 16x16 32x32 64x64
  //inter sa8d
  unsigned char         inter_sa8d_en;                     // inter selects 3 out of 4 modes through sa8d
  //pmdctrl
  unsigned char         pmdctrl_en;                        // frame level pred mode decision control enable. value 0: use default sse-cost. value 1:use sa8d/sad at some mode
  unsigned char         pmdctrl_mode;                      // pred mode decision control mode. 0: merge/search sa8d/sad, inter/intra sse. vaule 1: merge/search/intra sa8d/sad.
  //pmdsu
  unsigned char         pmdsu_en;                          // enable of pmd speed up for cu32/cu16/cu8
  unsigned char         pmdsu_thrd[3];                     // pmd speed up threshold
  //delta lambdaQP
  unsigned char         dlambda_ppmd_en;
  unsigned char         dlambda_icsd_en;
  unsigned char         dlambda_pcsd_en;
  unsigned char         dlambda_motion_shift;
  unsigned char         dlambda_max_depth;
  unsigned char         dlambda_frmqp_thrd;
  char          dlambda_simple_dqp;
  char          dlambda_nei_cplx_dqp;
  char          dlambda_still_dqp;
  char          dlambda_edge_dqp;
  unsigned char         dlambda_recovery_en;
  unsigned char         dlambda_start_recovery;
  unsigned char         dlambda_end_recovery;
  char          dlambda_recovery_dqp;

  //mvp_force_zero
  unsigned char         mvp_force_zero_en;                 // when block is still, merge force select mv0 for mvp
  unsigned char         mvp_force_zero_motion_shift;       // motion flag valid bit
  //color_shadow_sse
  unsigned char         color_shadow_sse_en;               // enable of color_shadow detect use chrom sse
  unsigned char         color_shadow_sse_priority_en;      // enable of
  unsigned char         color_shadow_sse_value_thrd_en;    // enable of
  unsigned char         color_shadow_sse_qp;               //
  unsigned char         color_shadow_sse_motion_shift;     // color_shadow detect use chrom sse
  unsigned char         color_shadow_sse_ratio_thrd[2];    //
  unsigned int        color_shadow_sse_value_thrd[3];    //
  //lambda table
  unsigned int        lambda_cfg[11];
  // mv exceed check to force sel intra
  unsigned char         md_jrfc_exceed_en;                 //
  unsigned char         md_max_ofst_x;                     // max x pixel ofst used as ref frame, unit 64 pixel. used with y0
  unsigned int        md_max_ofst_y0;                    // max y0 pixel ofst used as ref frame, unit 4 pixel. sel intra when mv y0 exceed & mv x exceed
  unsigned int        md_max_ofst_y1;                    // max y1 pixel ofst used as ref frame, unit 4 pixel. sel intra when mv y1 exceed
  //------------------------
  //  SDE Modulue Param
  //------------------------
  unsigned char         nal_en;                            // hw encode nal enable

  //------------------------
  //    TFm Module Param
  //------------------------
  unsigned short        tfm_path_en;                       // [9]:CU64 merge, [8]:CU32 intra, [7]:CU32 merge, [6]:CU32 search, [5]:CU16 intra, [4]:CU16 merge, [3]:CU16 search, [2]:CU8 intra, [1]:CU8 merge, [0]:CU8 search
  unsigned char         scaling_list_en;                   // 0: do not use scalinglist, 1: use scalinglist
  unsigned char         do_not_clear_deadzone;             // 0: default, 1: do not clear deadzone when the result is opposite
  unsigned char         deadzone_en;                       // enable of block level deadzone cfg
  unsigned char         rdoq_en;                           // enable of block level rdoq cfg
  unsigned char         acmask_en;                         // enable of block level acmask cfg
  unsigned char         acmask_type;                       // 0: default, 1: diagonal type
  h265_tfm_grp_t        tfm_grp[16];                      // tfm block configure table
  tfm_rdoq_param_t tfm_rdoq_param[9];                // 9 rdoq param sets according to rdoq level 1~9. rdoq level 0 means turning off and does not need rdoq param.
  //recovery_tfm
  unsigned char          tfm_recovery_en;
  unsigned char          tfm_start_recovery;
  unsigned char          tfm_end_recovery;
  int              tfm_recovery_add[3][3];
  //------------------------
  //    DBLK Module Param
  //------------------------
  //dblk
  unsigned char        dblk_en;                            // enable of deblock
  char         beta_offset_div2;                   // dblk param -6 ~ +6
  char         tc_offset_div2;                     // dblk param -6 ~ +6
  //sao
  unsigned char        sao_en;                             // enable of sao
  unsigned char        sao_y_flag;                         // enable of sao y
  unsigned char        sao_c_flag;                         // enable of sao c
  unsigned char        stc_cfg_mode;                       // 0 is common, 1 is cfg a mode for a lcu0, others merge. 2 is merge mode, lcu 0 calc real param, other lcu merge , not insist now; if open sao, defalt 1
  unsigned char        stc_fixed_merge;                    // 2bits. {left, up}
  unsigned char        stc_fixed_mode;                     // 4bits. 0~3 is E0, 4 is BO, 8 is NO-SAO
  unsigned char        stc_fixed_subtype;                  // 5bits. 0~28 useful when BO.
  char         stc_fixed_offset0;                  // 4bits. if bo,-7~7; [3] is sign bit. if eo, 0~6...(ori svac is -1~6)
  char         stc_fixed_offset1;                  // 4bits. if bo,-7~7; [3] is sign bit. if eo, 0~1...
  char         stc_fixed_offset2;                  // 4bits. if bo -7~7; [3] is sign bit. if eo, -1~0...
  char         stc_fixed_offset3;                  // 4bits. if bo -7~7; [3] is sign bit. if eo, -6~0...(ori svac is -6~1)
  //------------------------
  //    BURC Module Param
  //------------------------
  unsigned char         rc_en;                             // enable of row rc
  unsigned int        bu_size;                           // lcu numbers in a bu
  unsigned int        bu_len;                            // bu numbers in a frame
  unsigned char         rc_method;                         // rc alg sel
  unsigned short        rc_thd_t32[12];                        // rc alg threshold
  short         rc_thd[12];                        // rc alg threshold
  unsigned int        rc_info[128];                      // bu's target bits
  char          bu_max_dqp;                        // bu level max delta qp
  char          bu_min_dqp;                        // bu level min delta qp
  unsigned char         rc_avg_len;                        // dqp = sum >> rc_avg_len.
  unsigned char         rc_min_prop;                       // min proportion of bu target bits compensate
  unsigned char         rc_max_prop;                       // max proportion of bu target bits compensate
  unsigned char         rc_dly_en;                         // enable of bu dqp delay
  unsigned char         rc_max_qp;                         // max qp to limit the result of qpg stacked with burc
  unsigned char         rc_min_qp;                         // min qp to limit the result of qpg stacked with burc
  char          rc_dqp_offset;                     // rc_thd's dqp offset
  unsigned int        tgtbs_offset_unit;                 // force adjust dqp bits threshold
  unsigned int        force_max_dqp_bits;                // force use of max dqp bits threshold
  unsigned char         adjust_unit_size;                  // force adjust dqp unit size
  unsigned short        adjust_unit_len;                   // force adjust dqp unit number in a bu

  //------------------------
  //    JRFCD Module Param ok
  //------------------------
  unsigned char         jrfc_en;                            // enable  of jzm reference frame compress
  unsigned char         jrfcv2_en;                          // enable  of jzm reference frame compress v2
  unsigned int        head_dsty_addr;                     // address of rebuild frame Y head
  unsigned int        head_dstc_addr;                     // address of rebuild frame C head
  unsigned int        head_sp_dsty_addr;                  // address of rebuild frame sp Y head
  unsigned int        head_sp_dstc_addr;                  // address of rebuild frame sp C head
  unsigned int        head_refy_addr;                     // address of ref frame Y head
  unsigned int        head_refc_addr;                     // address of ref frame C head
  unsigned int        head_mrefy_addr;                    // address of mref frame Y head
  unsigned int        head_mrefc_addr;                    // address of mref frame C head
  unsigned int        lm_head_total;                      // luma head total
  unsigned int        cm_head_total;                      // chroma head total
  unsigned char         body_combine_en;                    // jrfcv2 enable of body order
  unsigned char         jrfc_ref_ck_en;                     // jrfc ref addr exceed check enable
  unsigned char         jrfc_max_ck_en;                     // jrfc max size exceed check enable
  unsigned char         jrfc_ref_lcux_range;                // jrfc ref frm mv x range, unit lcu
  unsigned char         jrfc_ref_lcuy_range;                // jrfc ref frm mv y range, unit lcu
  unsigned char         jrfc_max_size_ratio;                // jrfc max size ratio, range 1~255

  unsigned int        jrfc_exceed_max_en    ;
  unsigned int        jrfc_exceed_max_plcu_x;
  unsigned int        jrfc_exceed_max_plcu_y;
  unsigned int        jrfc_nxt_frm_head   ;

  //------------------------
  //    BUFSHARE Module Param ok
  //------------------------
  unsigned char         buf_share_en;                       // enable of recon and ref data share buffer
  unsigned char         buf_share_size;                     // share buffer size. 1 is 128 rows of pixel, 2 is 192, 3 is 256.... 1 at least
  unsigned int        buf_base_yaddr;                     // address of bufshare Y start. fix y addr
  unsigned int        buf_base_caddr;                     // address of bufshare C start. fix c addr
  unsigned int        buf_beyond_yaddr;                   // address of bufshare Y +1.
  unsigned int        buf_beyond_caddr;                   // address of bufshare C +1.
  unsigned int        buf_start_yaddr;                    // address of bufshare recon Y
  unsigned int        buf_start_caddr;                    // address of bufshare recon C
  unsigned int        buf_ref_yaddr;                      // address of bufshare ref Y.
  unsigned int        buf_ref_caddr;                      // address of bufshare ref C.
  unsigned int        buf_rem_mby;                        // rem = beyond - start
  //------------------------
  //   SW Param
  //------------------------
  unsigned int        frm_num_gop;                        // frm count in a gop
  unsigned char         maxnum_ref;                         // max reference frame num
  unsigned char         use_dummy_byd;                      // for vector mult test. if bjm all open, JRFD need dummy byd addr
  unsigned int        dummy_buf_beyond_yaddr;
  unsigned int        dummy_buf_beyond_caddr;
  //------------------------
  //   RM Param
  //------------------------
    h265_rm_t rm;
    unsigned char soc_type; //
    //------------------------
  //   T32 unique
  //------------------------
    /*ok*/unsigned int       head_sp_dsty;                        // rebuild frame sp Y head physical address
    /*ok*/unsigned int       head_sp_dstc;                        // rebuild frame sp C head physical address
//In case stack overflow
};
/* vpu h265 vdma struct end */

struct h265_instance {
	struct h265_param param;
	struct h265_header_info header;
	struct h265_slice_info vdma_info;
};

int h265_param_init(h265_t *inst, struct h265_param *param);
void h265_exit(h265_t *inst);
int h265_encode(h265_t *inst, struct vpu_io_info *input);

#endif //_VPU_CORE_H_
