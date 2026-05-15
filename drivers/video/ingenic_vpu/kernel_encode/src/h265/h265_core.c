#include "h265_core.h"
#include "vpu_common.h"
#include "h265_vpi.h"
#include "h265_entropy.h"
#include "hera.h"
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
//#define DUMP_HERA_REG
//#define DUMP_SLICE_INFO
extern struct completion *hera_done_for_kernEnc;
extern int hera_irq_stat_for_kernEnc;
extern void H265E_T32V_SliceInit(struct h265_slice_info *s);
int h265_code_headers(struct h265_header_info *header, struct vpu_io_info *io);
int h265_code_headers_segment(struct h265_header_info *header, struct vpu_io_info *io, unsigned int bslen, int count, int is_final);

#ifdef DUMP_HERA_REG
static void dump_hera_regs(void)
{
	int i = 0;
	int offset = 0;

#define DUMP_HERA_BASE 0x13200000
#define DUMP_IVDC_BASE 0x13100000
	for (i = 0; i < 0xfc; i ++) {
		offset = RADIX_CFGC_BASE + i * 4;
		printk(KERN_DEBUG "kern-CFGC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x0c; i ++) {
		offset = RADIX_VDMA_BASE + i * 4;
		printk(KERN_DEBUG "kern-VDMA:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x67; i ++) {
		offset = RADIX_ODMA_BASE + i * 4;
		printk(KERN_DEBUG "kern-ODMA:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x3a0; i ++) {
		offset = RADIX_EFE_BASE + i * 4;
		printk(KERN_DEBUG "kern-EFE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x10c; i ++) {
		offset = RADIX_JRFD_BASE + i * 4;
		printk(KERN_DEBUG "kern-JRFD:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x7c; i ++) {
		offset = RADIX_MCE_BASE + i * 4;
		printk(KERN_DEBUG "kern-MCE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}
	for (i = 0x100; i < 0x1bc; i ++) {
		offset = RADIX_MCE_BASE + i * 4;
		printk(KERN_DEBUG "kern-MCE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x80; i ++) {
		offset = RADIX_TFM_BASE + i * 4;
		printk(KERN_DEBUG "kern-TFM:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x290; i ++) {
		offset = RADIX_MD_BASE + i * 4;
		printk(KERN_DEBUG "kern-MD:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x8; i ++) {
		offset = RADIX_DT_BASE + i * 4;
		printk(KERN_DEBUG "kern-DT:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x98; i ++) {
		offset = RADIX_DBLK_BASE + i * 4;
		printk(KERN_DEBUG "kern-DBLK:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}
	for (i = 0x240; i < 0x248; i ++) {
		offset = RADIX_DBLK_BASE + i * 4;
		printk(KERN_DEBUG "kern-DBLK:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0x20; i < 0x34; i ++) {
		offset = RADIX_SAO_BASE + i * 4;
		printk(KERN_DEBUG "kern-SAO:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x8; i ++) {
		offset = RADIX_BC_BASE + i * 4;
		printk(KERN_DEBUG "kern-BC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x68; i ++) {
		offset = RADIX_SDE_BASE + i * 4;
		printk(KERN_DEBUG "kern-SDE:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x13c; i ++) {
		offset = RADIX_IPRED_BASE + i * 4;
		printk(KERN_DEBUG "kern-IPRED:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x4; i ++) {
		offset = RADIX_STC_BASE + i * 4;
		printk(KERN_DEBUG "kern-STC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}
	for (i = 0x1000; i < 0x1004; i ++) {
		offset = RADIX_STC_BASE + i * 4;
		printk(KERN_DEBUG "kern-STC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x1d4; i ++) {
		offset = RADIX_VMAU_BASE + i * 4;
		printk(KERN_DEBUG "kern-VMAU:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x68; i ++) {
		offset = RADIX_EMC_BASE + i * 4;
		printk(KERN_DEBUG "kern-EMC:::0x%08x = 0x%08x\n", DUMP_HERA_BASE + offset, *(volatile int *)(0xa0000000 + offset));
	}

	for (i = 0; i < 0x200; i ++) {
		offset = i * 4;
		printk(KERN_DEBUG "IVDC:::0x%08x = 0x%08x\n", DUMP_IVDC_BASE + offset, *(volatile int *)(0xb3100000 + offset));
	}
}
#endif

#ifdef DUMP_SLICE_INFO
#define DUMP_ADDR(addr) printk("kern-  0x%x,",addr);
#define DUMP_VEC(vec)   printk("kern-  %d,",vec);

#define DUMP_ARRAY1(array, num)					\
    printk("kern-  {");							\
    for(i=0;i<num;i++)							\
    printk("kern- %d,",array[i]);				\
    printk("kern-},");

#define DUMP_ARRAY2(array, num1, num2)				\
    printk("kern-  {\n");								\
    for(i=0;i<num1;i++) {								\
        printk("kern-    {");							\
        for(j=0;j<num2;j++)							\
        printk("kern- %d,",array[i][j]);			\
        printk("kern-},\n");							\
    }													\
    printk("kern-  },");

#define DUMP_ARRAY3(array, num1, num2, num3)		\
    printk("kern-  {\n");								\
    for(i=0;i<num1;i++) {								\
        printk("kern-    {\n");						\
        for(j=0;j<num2;j++) {							\
            printk("kern-      {");					\
            for(k=0;k<num3;k++) 						\
            printk("kern- %d,",array[i][j][k]);	\
            printk("kern-},\n");						\
        }												\
        printk("kern-    },\n");						\
    }													\
    printk("kern-  },");

#define DUMP_ARRAY4(array, num1, num2, num3, num4)			\
    printk("kern-  {\n");									\
    for(i=0;i<num1;i++) {									\
        printk("kern-    {\n");								\
        for(j=0;j<num2;j++) {								\
            printk("kern-      {\n");						\
            for(k=0;k<num3;k++) {							\
                printk("kern-        {");					\
                for(m=0;m<num4;m++)							\
                printk("kern- %d,",array[i][j][k][m]);	\
                printk("kern-},\n");							\
            }												\
            printk("kern-      },\n");						\
        }													\
        printk("kern-    },\n");								\
    }														\
    printk("kern-  },");


static void dump_t32v_h265_sliceinfo(struct h265_slice_info *si)
{
	int i, j, k, m, g;
	printk("kern----------------------frm_num: %d\n", si->frm_num_gop);
	printk("kern-{\n");

	printk("kern-~~~~~~~~~~~~~dump\r\n");
	printk("kern-  //--------------------\r\n");
	printk("kern-  //  BASE param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->is_decode);       printk("kern-	//si->is_decode\n");
	DUMP_VEC(si->work_mode);       printk("kern-	//si->work_mode\n");
	DUMP_VEC(si->frame_type);      printk("kern-	//si->frame_type\n");
	DUMP_VEC(si->raw_format);      printk("kern-	//si->raw_format\n");
	DUMP_VEC(si->frame_width);     printk("kern-	//si->frame_width\n");
	DUMP_VEC(si->frame_height);    printk("kern-	//si->frame_height\n");
	DUMP_VEC(si->frame_qp);        printk("kern-	//si->frame_qp\n");
	DUMP_VEC(si->frame_cqp_offset);printk("kern-	//si->frame_cqp_offset\n");
	DUMP_VEC(si->tu_inter_depth);  printk("kern-	//si->tu_inter_depth\n");
	DUMP_VEC(si->use_dqp_flag);    printk("kern-	//si->use_dqp_flag\n");
	DUMP_VEC(si->dqp_max_depth);   printk("kern-	//si->dqp_max_depth\n");
	DUMP_VEC(si->sign_hide_flag);  printk("kern-	//si->sign_hide_flag\n");
	DUMP_VEC(si->ipred_usis);      printk("kern-	//si->ipred_usis\n");
	DUMP_VEC(si->frame_y_stride);  printk("kern-	//si->frame_y_stride\n");
	DUMP_VEC(si->frame_c_stride);  printk("kern-	//si->frame_c_stride\n");
	DUMP_VEC(si->dst_y_stride);    printk("kern-	//si->dst_y_stride\n");
	DUMP_VEC(si->dst_c_stride);    printk("kern-	//si->dst_c_stride\n");
	DUMP_VEC(si->ref_y_stride);    printk("kern-	//si->ref_y_stride\n");
	DUMP_VEC(si->ref_c_stride);    printk("kern-	//si->ref_c_stride\n");

	printk("kern-  //--------------------\r\n");
	printk("kern-  //  BUFFER addr\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_ADDR(si->des_va);          printk("kern-	//si->des_va\n");
	DUMP_ADDR(si->des_pa);          printk("kern-	//si->des_pa\n");
	DUMP_ADDR(si->raw_y_pa);        printk("kern-	//si->raw_y_pa\n");
	DUMP_ADDR(si->raw_c_pa);        printk("kern-	//si->raw_c_pa\n");
	DUMP_ADDR(si->dst_y_pa);        printk("kern-	//si->dst_y_pa\n");
	DUMP_ADDR(si->dst_c_pa);        printk("kern-	//si->dst_c_pa\n");
	DUMP_ADDR(si->ref_y_pa);        printk("kern-	//si->ref_y_pa\n");
	DUMP_ADDR(si->ref_c_pa);        printk("kern-	//si->ref_c_pa\n");
	DUMP_ADDR(si->mref_y_pa);       printk("kern-	//si->mref_y_pa\n");
	DUMP_ADDR(si->mref_c_pa);       printk("kern-	//si->mref_c_pa\n");
	DUMP_ADDR(si->bitstream_pa);    printk("kern-	//si->bitstream_pa\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  EFE Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->efe_hw_width);    printk("kern-	//si->efe_hw_width\n");
	DUMP_VEC(si->efe_hw_height);   printk("kern-	//si->efe_hw_height\n");
	DUMP_VEC(si->chng_en);         printk("kern-	//si->chng_en\n");
	DUMP_VEC(si->mce_cfg_en);      printk("kern-	//si->mce_cfg_en\n");
	DUMP_VEC(si->md_cfg_en);       printk("kern-	//si->md_cfg_en\n");
	DUMP_VEC(si->ip_cfg_en);       printk("kern-	//si->ip_cfg_en\n");
	DUMP_VEC(si->rotate_mode);     printk("kern-	//si->rotate_mode\n");

	printk("kern-{  // ROI\n");
	for(i=0;i<16;i++){
		printk("kern-{");
		printk("kern-%d,",si->roi_info[i].roi_en);
		printk("kern-%d,",si->roi_info[i].roi_md);
		printk("kern-%d,",si->roi_info[i].roi_qp);
		printk("kern-%d,",si->roi_info[i].roi_lmbx);
		printk("kern-%d,",si->roi_info[i].roi_rmbx);
		printk("kern-%d,",si->roi_info[i].roi_umby);
		printk("kern-%d,",si->roi_info[i].roi_bmby);
		printk("kern-},\n");
	}
	printk("kern-},// roi_en, roi_md, roi_qp, roi_lmbx, roi_rmbx, roi_umby, roi_bmby\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  IFA Param\r\n");
	printk("kern-  //--------------------\r\n");
	printk("kern-{\n");
	DUMP_VEC(si->ifa_para.ifa_en);    printk("kern-	//si->ifa_para.ifa_en\n");
	DUMP_ARRAY1(si->ifa_para.emc_out_en,  2);   printk("kern-	//si->ifa_para.emc_out_en[2]\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  cps\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.cps_en);            printk("kern-	//si->ifa_para.cps_en\n");
	DUMP_VEC(si->ifa_para.cps_c_en);          printk("kern-	//si->ifa_para.cps_c_en\n");
	DUMP_ADDR(si->ifa_para.cps_ref0);          printk("kern-	//si->ifa_para.cps_ref0\n");
	DUMP_ADDR(si->ifa_para.cps_ref1);          printk("kern-	//si->ifa_para.cps_ref1\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  rrs\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.rrs_en);            printk("kern-	//si->ifa_para.rrs_en\n");
	DUMP_VEC(si->ifa_para.rrs1_en);           printk("kern-	//si->ifa_para.rrs1_en\n");
	DUMP_VEC(si->ifa_para.rrs_c_en);          printk("kern-	//si->ifa_para.rrs_c_en\n");
	DUMP_VEC(si->ifa_para.rrs1_c_en);         printk("kern-	//si->ifa_para.rrs1_c_en\n");
	DUMP_VEC(si->ifa_para.rrs_thrd);          printk("kern-	//si->ifa_para.rrs_thrd\n");
	DUMP_VEC(si->ifa_para.rrs_thrd_c);        printk("kern-	//si->ifa_para.rrs_thrd_c\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  motion\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.motion_en);         printk("kern-	//si->ifa_para.motion_en\n");
	DUMP_VEC(si->ifa_para.motion_c_en);       printk("kern-	//si->ifa_para.motion_c_en\n");
	DUMP_VEC(si->ifa_para.motion_thrd);       printk("kern-	//si->ifa_para.motion_thrd\n");
	DUMP_VEC(si->ifa_para.motion_thrd_c);     printk("kern-	//si->ifa_para.motion_thrd_c\n");
	DUMP_VEC(si->ifa_para.motion_of_thrd);    printk("kern-	//si->ifa_para.motion_of_thrd\n");
	DUMP_VEC(si->ifa_para.motion_of_thrd_c);  printk("kern-	//si->ifa_para.motion_of_thrd_c\n");

	DUMP_ARRAY1(si->ifa_para.rrc_motion_thr,  7);  printk("kern-	//si->ifa_para.rrc_motion_thr[7]\n");

	DUMP_VEC(si->ifa_para.frm_mt_smd_thr);  printk("kern-	//si->ifa_para.frm_mt_smd_thr\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  still times\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.still_times_en);            printk("kern-	//si->ifa_para.still_times_en\n");
	DUMP_VEC(si->ifa_para.still_times_motion_shift);  printk("kern-	//si->ifa_para.still_times_motion_shift\n");
	DUMP_VEC(si->ifa_para.max_still_times);           printk("kern-	//si->ifa_para.max_still_times\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  sobel\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.sobel_en);         printk("kern-	//si->ifa_para.sobel_en\n");
	DUMP_VEC(si->ifa_para.sobel_edge_thrd);  printk("kern-	//si->ifa_para.sobel_edge_thrd\n");
	DUMP_VEC(si->ifa_para.sobel_16_or_14);   printk("kern-	//si->ifa_para.sobel_16_or_14\n");
	DUMP_VEC(si->ifa_para.ras_en);           printk("kern-	//si->ifa_para.ras_en\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  smd\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.smd_en);       printk("kern-	//si->ifa_para.smd_en\n");
	DUMP_VEC(si->ifa_para.smd_c_en);     printk("kern-	//si->ifa_para.smd_c_en\n");
	DUMP_VEC(si->ifa_para.row_smd_unit); printk("kern-	//si->ifa_para.row_smd_unit\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  enery\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.ery_en);     printk("kern-	//si->ifa_para.ery_en\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  skin\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.skin_en);             printk("kern-	//si->ifa_para.skin_en\n");
	DUMP_ARRAY1(si->ifa_para.skin_lvl_en,  3);  printk("kern-	//si->ifa_para.skin_lvl_en[3]\n");
	DUMP_VEC(si->ifa_para.skin_cnt_thrd);       printk("kern-	//si->ifa_para.skin_cnt_thrd\n");
	DUMP_ARRAY3(si->ifa_para.skin_thrd,3,2,2);  printk("kern-	//si->ifa_para.skin_thrd[3][2][2]\n");

	DUMP_ARRAY1(si->ifa_para.skin_level,  4);  printk("kern-	//si->ifa_para.skin_level[4]\n");

	DUMP_ARRAY1(si->ifa_para.skin_mul_factor,  4);  printk("kern-	//si->ifa_para.skin_mul_factor[4]\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  neighbour complexity\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.nei_cplx_en);     printk("kern-	//si->ifa_para.nei_cplx_en\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  small-block flat\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.var_flat_en);            printk("kern-	//si->ifa_para.var_flat_en\n");
	DUMP_ARRAY2(si->ifa_para.var_flat_thr, 2, 2);  printk("kern-	//si->ifa_para.var_flat_thr[2][2]\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  sub-block uniformation\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.cu_unifor_en);            printk("kern-	//si->ifa_para.cu_unifor_en\n");
	DUMP_VEC(si->ifa_para.var_split_thrd);          printk("kern-	//si->ifa_para.var_split_thrd\n");
	DUMP_ARRAY1(si->ifa_para.smd_blk16_bias,  2);   printk("kern-	//si->ifa_para.smd_blk16_bias[2]\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  complexity detection\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_ARRAY1(si->ifa_para.smd_blk8_thr,  6);   printk("kern-	//si->ifa_para.smd_blk8_thr[6]\n");

	DUMP_ARRAY2(si->ifa_para.smd_blk16_thr, 8, 2);  printk("kern-	//si->ifa_para.smd_blk16_thr[8][2]\n");
	DUMP_ARRAY2(si->ifa_para.smd_blk32_thr, 3, 2);  printk("kern-	//si->ifa_para.smd_blk32_thr[3][2]\n");
	DUMP_ARRAY1(si->ifa_para.sobel_blk16_thr,  3);   printk("kern-	//si->ifa_para.sobel_blk16_thr[3]\n");

	DUMP_ARRAY1(si->ifa_para.sobel_blk32_thr,  3);   printk("kern-	//si->ifa_para.sobel_blk32_thr[3]\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  AI mark\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.ai_mark_en);       printk("kern-	//si->ifa_para.ai_mark_en\n");
	DUMP_VEC(si->ifa_para.ai_mark_type);     printk("kern-	//si->ifa_para.ai_mark_type\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  refresh flag\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.refresh_en);       printk("kern-	//si->ifa_para.refresh_en\n");
	DUMP_VEC(si->ifa_para.refresh_mode);     printk("kern-	//si->ifa_para.refresh_mode\n");
	DUMP_VEC(si->ifa_para.rfh_raw_thrd);     printk("kern-	//si->ifa_para.rfh_raw_thrd\n");
	DUMP_VEC(si->ifa_para.rfh_edge_thrd);    printk("kern-	//si->ifa_para.rfh_edge_thrd\n");
	DUMP_VEC(si->ifa_para.rfh_flat_thrd);    printk("kern-	//si->ifa_para.rfh_flat_thrd\n");
	DUMP_VEC(si->ifa_para.rrs_filter_mode);  printk("kern-	//si->ifa_para.rrs_filter_mode\n");
	printk("kern-{");
	for(i=0;i<4;i++){
		printk("kern-%d,",si->ifa_para.filter_thrd[i]);
	}
	printk("kern-},  //si->ifa_para.filter_thrd[4]\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  pet mark\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.petbc_en);       printk("kern-	//si->ifa_para.petbc_en\n");
	DUMP_VEC(si->ifa_para.pet_mode);       printk("kern-	//si->ifa_para.pet_mode\n");
	DUMP_VEC(si->ifa_para.pet_fil_en);     printk("kern-	//si->ifa_para.pet_fil_en\n");

	DUMP_ARRAY1(si->ifa_para.petbc_var_thr,  3);   printk("kern-	//si->ifa_para.petbc_var_thr[3]\n");

	DUMP_ARRAY2(si->ifa_para.petbc_ssm_thr, 3, 4);  printk("kern-	//si->ifa_para.petbc_ssm_thr[3][4]\n");
	DUMP_ARRAY1(si->ifa_para.petbc2_var_thr,  3);   printk("kern-	//si->ifa_para.petbc2_var_thr[3]\n");
	DUMP_ARRAY2(si->ifa_para.petbc2_ssm_thr, 3, 4); printk("kern-	//si->ifa_para.petbc2_ssm_thr[3][4]\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  qpg\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ifa_para.qpg_en);       printk("kern-	//si->ifa_para.qpg_en\n");
	DUMP_VEC(si->ifa_para.base_qp);      printk("kern-	//si->ifa_para.base_qp\n");
	DUMP_VEC(si->ifa_para.max_qp);       printk("kern-	//si->ifa_para.max_qp\n");
	DUMP_VEC(si->ifa_para.min_qp);       printk("kern-	//si->ifa_para.min_qp\n");
	DUMP_VEC(si->ifa_para.qpg_max_qp);       printk("kern-	//si->ifa_para.qpg_max_qp\n");
	DUMP_VEC(si->ifa_para.qpg_min_qp);       printk("kern-	//si->ifa_para.qpg_min_qp\n");
	DUMP_VEC(si->ifa_para.qpg_cu32_qpm); printk("kern-	//si->ifa_para.qpg_cu32_qpm\n");
	DUMP_VEC(si->ifa_para.qpg_filte_en); printk("kern-	//si->ifa_para.qpg_filte_en\n");
	DUMP_VEC(si->ifa_para.qpg_table_en); printk("kern-	//si->ifa_para.qpg_table_en\n");
	DUMP_VEC(si->ifa_para.qpg_roi_en);   printk("kern-	//si->ifa_para.qpg_roi_en\n");
	DUMP_VEC(si->ifa_para.qpg_skin_en);  printk("kern-	//si->ifa_para.qpg_skin_en\n");
	DUMP_ARRAY1(si->ifa_para.qpg_skin_qp_ofst,  3);   printk("kern-	//si->ifa_para.qpg_skin_qp_ofst[3]\n");
	DUMP_VEC(si->ifa_para.qpg_ery_en);       printk("kern-	//si->ifa_para.qpg_ery_en\n");
	DUMP_VEC(si->ifa_para.qpg_sobel_en);     printk("kern-	//si->ifa_para.qpg_sobel_en\n");

	DUMP_ARRAY1(si->ifa_para.qpg_sobel_cplx_thrd,  8);   printk("kern-	//si->ifa_para.qpg_sobel_cplx_thrd[8]\n");
	DUMP_VEC(si->ifa_para.qpg_smd_en);       printk("kern-	//si->ifa_para.qpg_smd_en\n");
	DUMP_VEC(si->ifa_para.smd_filter_thrd);  printk("kern-	//si->ifa_para.smd_filter_thrd\n");

	DUMP_ARRAY1(si->ifa_para.qpg_smd_cplx_thrd,  8);   printk("kern-	//si->ifa_para.qpg_smd_cplx_thrd[8]\n");

	DUMP_ARRAY1(si->ifa_para.qpg_cplx_qp_ofst,  8);   printk("kern-	//si->ifa_para.qpg_cplx_qp_ofst[8]\n");

	DUMP_VEC(si->ifa_para.qpg_petbc_en);                 printk("kern-	//si->ifa_para.qpg_petbc_en\n");
	DUMP_ARRAY2(si->ifa_para.qpg_pet_cplx_thrd, 3, 12);  printk("kern-	//si->ifa_para.qpg_pet_cplx_thrd[3][12]\n");
	DUMP_ARRAY2(si->ifa_para.qpg_pet_qp_ofst, 5, 7);     printk("kern-	//si->ifa_para.qpg_pet_qp_ofst[5][7]\n");
	DUMP_ARRAY2(si->ifa_para.qpg_pet_qp_ofst_mt, 5, 7);  printk("kern-	//si->ifa_para.qpg_pet_qp_ofst_mt[5][7]\n");

	DUMP_ARRAY1(si->ifa_para.qpg_dlt_thr,  4);   printk("kern-	//si->ifa_para.qpg_dlt_thr[4]\n");

	DUMP_ARRAY1(si->ifa_para.pet_filter_valid,  4);   printk("kern-	//si->ifa_para.pet_filter_valid[4]\n");

	DUMP_ARRAY1(si->ifa_para.qpg_pet_qp_idx,  5);   printk("kern-	//si->ifa_para.qpg_pet_qp_idx[5]\n");

	DUMP_ARRAY3(si->ifa_para.qpg_ai_motion_smd_ofst, 3, 2, 8);   printk("kern-	//si->ifa_para.qpg_ai_motion_smd_ofst[3][2][8]\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  data stat\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_ARRAY1(si->ifa_para.frm_cplx_thrd,  2);           printk("kern-	//si->ifa_para.frm_cplx_thrd[2]\n");
	printk("kern-},\r\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  EMC Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->emc_mvi_en);       printk("kern-	//si->emc_mvi_en\n");
	DUMP_VEC(si->emc_ai_en);        printk("kern-	//si->emc_ai_en\n");
	DUMP_VEC(si->emc_qpt_en);       printk("kern-	//si->emc_qpt_en\n");
	DUMP_ADDR(si->emc_mvi_addr);     printk("kern-	//si->emc_mvi_addr\n");
	DUMP_ADDR(si->emc_ai_addr);      printk("kern-	//si->emc_ai_addr\n");
	DUMP_ADDR(si->emc_ipc_addr);     printk("kern-	//si->emc_ipc_addr\n");
	DUMP_ADDR(si->emc_mcec_addr);    printk("kern-	//si->emc_mcec_addr\n");
	DUMP_ADDR(si->emc_mdc_addr);     printk("kern-	//si->emc_mdc_addr\n");
	DUMP_ADDR(si->emc_qpt_addr);     printk("kern-	//si->emc_qpt_addr\n");
	DUMP_VEC(si->bsfull_intr_en);   printk("kern-	//si->bsfull_intr_en\n");
	DUMP_VEC(si->bsfull_intr_size); printk("kern-	//si->bsfull_intr_size\n");

	//    DUMP_VEC(si->mix_gate_mode); printk("kern-	//si->mix_gate_mode\n");

	DUMP_VEC(si->emc_bs_en);        printk("kern-	//si->emc_bs_en\n");
	DUMP_VEC(si->emc_cps_en);       printk("kern-	//si->emc_cps_en\n");
	DUMP_VEC(si->emc_sobel_en);     printk("kern-	//si->emc_sobel_en\n");
	DUMP_VEC(si->emc_mix_en);       printk("kern-	//si->emc_mix_en\n");
	DUMP_VEC(si->emc_mixc_en);      printk("kern-	//si->emc_mixc_en\n");
	DUMP_VEC(si->emc_flag_en);      printk("kern-	//si->emc_flag_en\n");
	DUMP_VEC(si->emc_mvo_en);       printk("kern-	//si->emc_mvo_en\n");
	DUMP_ADDR(si->emc_bs_addr0);     printk("kern-	//si->emc_bs_addr0\n");
	DUMP_ADDR(si->emc_bs_addr1);     printk("kern-	//si->emc_bs_addr1\n");
	DUMP_ADDR(si->emc_cps_addr);     printk("kern-	//si->emc_cps_addr\n");
	DUMP_ADDR(si->emc_sobel_addr);   printk("kern-	//si->emc_sobel_addr\n");
	DUMP_ADDR(si->emc_mix_addr);     printk("kern-	//si->emc_mix_addr\n");
	DUMP_ADDR(si->emc_mixc_addr);    printk("kern-	//si->emc_mixc_addr\n");
	DUMP_ADDR(si->emc_flag_addr);    printk("kern-	//si->emc_flag_addr\n");
	DUMP_ADDR(si->emc_mvo_addr);     printk("kern-	//si->emc_mvo_addr\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  IPRED Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->sobel_en);            printk("kern-	//si->sobel_en\n");
	DUMP_VEC(si->ipred_size);          printk("kern-	//si->ipred_size\n");
	DUMP_VEC(si->sobel_size);          printk("kern-	//si->sobel_size\n");
	DUMP_VEC(si->ipmd_mpm_use_maxnum); printk("kern-	//si->ipmd_mpm_use_maxnum\n");
	DUMP_VEC(si->md_bc_close);         printk("kern-	//si->md_bc_close\n");
	DUMP_VEC(si->intra_pu4_en);        printk("kern-	//si->intra_pu4_en\n");
	DUMP_VEC(si->ppred_slv_en);        printk("kern-	//si->ppred_slv_en\n");
	DUMP_VEC(si->ppred8_slv_en);       printk("kern-	//si->ppred8_slv_en\n");
	DUMP_VEC(si->ppred16_slv_en);      printk("kern-	//si->ppred16_slv_en\n");
	DUMP_VEC(si->ppred32_slv_en);      printk("kern-	//si->ppred32_slv_en\n");
	DUMP_VEC(si->ppred_mode_num);      printk("kern-	//si->ppred_mode_num\n");
	DUMP_VEC(si->ppred8_mode_num);     printk("kern-	//si->ppred8_mode_num\n");
	DUMP_VEC(si->ppred16_mode_num);    printk("kern-	//si->ppred16_mode_num\n");
	DUMP_VEC(si->ppred32_mode_num);    printk("kern-	//si->ppred32_mode_num\n");
	DUMP_VEC(si->ipred_slv_en);        printk("kern-	//si->ipred_slv_en\n");

	DUMP_ARRAY1(si->intra_mode,  5);             printk("kern-	//si->intra_mode[5]\n");
	DUMP_ARRAY1(si->intra8_mode,  5);            printk("kern-	//si->intra8_mode[5]\n");
	DUMP_ARRAY1(si->intra16_mode,  5);           printk("kern-	//si->intra16_mode[5]\n");
	DUMP_ARRAY1(si->intra32_mode,  5);           printk("kern-	//si->intra32_mode[5]\n");
	DUMP_ARRAY1(si->frmb_ip_bits_en,  5);        printk("kern-	//si->frmb_ip_bits_en[5]\n");

	DUMP_ARRAY2(si->intra_bits, 16, 5);      printk("kern-	//si->intra_bits[16][5]\n");
	DUMP_ARRAY1(si->frmb_idx,  5);           printk("kern-	//si->frmb_idx[5]\n");

	DUMP_VEC(si->i_pmd_en);             printk("kern-	//si->i_pmd_en\n");
	DUMP_VEC(si->frm_ipmd_angle_en);    printk("kern-	//si->frm_ipmd_angle_en\n");

	DUMP_ARRAY1(si->intra_angle_bias,  4);          printk("kern-	//si->intra_angle_bias[4]\n");
	DUMP_ARRAY2(si->frmb_ip_bias, 5, 2);            printk("kern-	//si->frmb_ip_bias[5][2]\n");
	DUMP_ARRAY1(si->frmb_ipc_bias,  5);             printk("kern-	//si->frmb_ipc_bias[5]\n");
	DUMP_ARRAY3(si->ipmd_mpm_cost_bias, 3, 2, 4);   printk("kern-	//si->ipmd_mpm_cost_bias[3][2][4]\n");
	DUMP_ARRAY2(si->ipmd_dm_cost_bias, 3, 5);       printk("kern-	//si->ipmd_dm_cost_bias[3][5]\n");
	DUMP_VEC(si->ipmd_ras_en);                      printk("kern-	//si->ipmd_ras_en\n");
	DUMP_ARRAY1(si->ipmd_sobel_thr,  3);            printk("kern-	//si->ipmd_sobel_thr[3]\n");
	DUMP_VEC(si->ipmd_sobel_edge_thrd);             printk("kern-	//si->ipmd_sobel_edge_thrd\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  MCE Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_ARRAY2(si->inter_mode, 4, 4);       printk("kern-	//si->inter_mode[4][4]\n");
	DUMP_VEC(si->half_pixel_sech_en);        printk("kern-	//si->half_pixel_sech_en\n");
	DUMP_VEC(si->quart_pixel_sech_en);       printk("kern-	//si->quart_pixel_sech_en\n");
	DUMP_VEC(si->subpixel_sech_cu8_en);      printk("kern-	//si->subpixel_sech_cu8_en\n");
	DUMP_VEC(si->subpixel_sech_cu16_en);     printk("kern-	//si->subpixel_sech_cu16_en\n");
	DUMP_VEC(si->subpixel_sech_cu32_en);     printk("kern-	//si->subpixel_sech_cu32_en\n");
	DUMP_VEC(si->mce_scl_mode);              printk("kern-	//si->mce_scl_mode\n");
	DUMP_VEC(si->mce_pref_mode);              printk("kern-	//si->mce_pref_mode\n");
	DUMP_VEC(si->merge_cand_num);            printk("kern-	//si->merge_cand_num\n");
	DUMP_VEC(si->max_sech_step);             printk("kern-	//si->max_sech_step\n");
	DUMP_VEC(si->mce_slc_mvx);               printk("kern-	//si->mce_slc_mvx\n");
	DUMP_VEC(si->mce_slc_mvy);               printk("kern-	//si->mce_slc_mvy\n");
	DUMP_VEC(si->max_mvrx);                  printk("kern-	//si->max_mvrx\n");
	DUMP_VEC(si->max_mvry);                  printk("kern-	//si->max_mvry\n");

	printk("kern-{  // mcec_range_grp \r\n");
	for (k = 0; k < 4; k++){
		printk("kern-{ \r\n");
		DUMP_VEC(si->mcec_range_grp[k].mcec_lx);    printk("kern-	//si->mcec_range_grp[i].mcec_lx\n");
		DUMP_VEC(si->mcec_range_grp[k].mcec_rx);    printk("kern-	//si->mcec_range_grp[i].mcec_rx\n");
		DUMP_VEC(si->mcec_range_grp[k].mcec_ty);    printk("kern-	//si->mcec_range_grp[i].mcec_ty\n");
		DUMP_VEC(si->mcec_range_grp[k].mcec_by);    printk("kern-	//si->mcec_range_grp[i].mcec_by\n");
		DUMP_VEC(si->mcec_range_grp[k].skip_pixel); printk("kern-	//si->mcec_range_grp[i].skip_pixel\n");
		printk("kern-},\r\n");
	}

	printk("kern-{  // mcec_grp \r\n");
	for (k = 0; k < 4; k++){
		printk("kern-{ \r\n");
		DUMP_VEC(si->mcec_grp[k].mcec_en);          printk("kern-	//si->mcec_grp[i].mcec_en\n");
		DUMP_VEC(si->mcec_grp[k].mcec_mode);        printk("kern-	//si->mcec_grp[i].mcec_mode\n");
		DUMP_VEC(si->mcec_grp[k].fs_cfg_idx);       printk("kern-	//si->mcec_grp[i].fs_cfg_idx\n");
		DUMP_VEC(si->mcec_grp[k].cros_cfg_idx);     printk("kern-	//si->mcec_grp[i].cros_cfg_idx\n");
		DUMP_VEC(si->mcec_grp[k].force_zero_mv_en); printk("kern-	//si->mcec_grp[i].force_zero_mv_en\n");
		DUMP_VEC(si->mcec_grp[k].mvc_bc_wei);       printk("kern-	//si->mcec_grp[i].mvc_bc_wei\n");
		DUMP_VEC(si->mcec_grp[k].sech_bc_wei);      printk("kern-	//si->mcec_grp[i].sech_bc_wei\n");
		printk("kern-},\r\n");
	}

	DUMP_VEC(si->mref_en);                   printk("kern-	//si->mref_en\n");
	DUMP_VEC(si->mref_wei);                  printk("kern-	//si->mref_wei\n");
	DUMP_VEC(si->mrg1_en);                   printk("kern-	//si->mrg1_en\n");
	DUMP_VEC(si->mrg_fpel_en);               printk("kern-	//si->mrg_fpel_en\n");
	DUMP_VEC(si->sech_force_mv0_en);         printk("kern-	//si->sech_force_mv0_en\n");
	DUMP_VEC(si->sech_force_mv0_mode);       printk("kern-	//si->sech_force_mv0_mode\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  MD Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->ccf_rm_en);                      printk("kern-	//si->ccf_rm_en\n");
	DUMP_VEC(si->rfh_sel_intra32_en);             printk("kern-	//si->rfh_sel_intra32_en\n");
	DUMP_VEC(si->rfh_sel_intra16_en);             printk("kern-	//si->rfh_sel_intra16_en\n");
	DUMP_VEC(si->rfh_msk_skip32_en);              printk("kern-	//si->rfh_msk_skip32_en\n");
	DUMP_VEC(si->rfh_msk_skip16_en);              printk("kern-	//si->rfh_msk_skip16_en\n");
	DUMP_VEC(si->uv_dist_scale_en);               printk("kern-	//si->uv_dist_scale_en\n");
	DUMP_ARRAY1(si->uv_dist_scale,  3);           printk("kern-	//si->uv_dist_scale[3]\n");

	DUMP_ARRAY2(si->md_cfg_grp, 16, 2);           printk("kern-	//si->md_cfg_grp[16][2]\n");
	DUMP_VEC(si->srd_en);                         printk("kern-	//si->srd_en\n");
	DUMP_VEC(si->mf_sel_sech_skip32_en);          printk("kern-	//si->mf_sel_sech_skip32_en\n");
	DUMP_VEC(si->mf_sel_sech_skip16_en);          printk("kern-	//si->mf_sel_sech_skip16_en\n");
	DUMP_VEC(si->srd_motion_shift);               printk("kern-	//si->srd_motion_shift\n");
	DUMP_VEC(si->i_csd_en);                       printk("kern-	//si->i_csd_en\n");
	DUMP_VEC(si->p_csd_en);                       printk("kern-	//si->p_csd_en\n");

	DUMP_ARRAY1(si->csd_qp_thrd,  5);             printk("kern-	//si->csd_qp_thrd[5]\n");
	DUMP_ARRAY2(si->var_flat_recon_thrd, 2, 5);  printk("kern-	//si->var_flat_recon_thrd[2][5]\n");
	DUMP_VEC(si->var_flat_sub_size);             printk("kern-	//si->var_flat_sub_size\n");
	DUMP_VEC(si->csd_color);                     printk("kern-	//si->csd_color\n");
	DUMP_VEC(si->ned_en);                        printk("kern-	//si->ned_en\n");
	DUMP_ARRAY2(si->recon_resi_en, 4, 2);        printk("kern-	//si->recon_resi_en[4][2]\n");
	DUMP_VEC(si->rcn_plain_en);                  printk("kern-	//si->rcn_plain_en\n");
	DUMP_ARRAY3(si->ned_score_table, 2, 2, 8);   printk("kern-	//si->ned_score_table[2][2][8]\n");
	DUMP_ARRAY1(si->ned_score_bias,  2);         printk("kern-	//si->ned_score_bias[2]\n");

	DUMP_VEC(si->ned_motion_en);                      printk("kern-	//si->ned_motion_en\n");
	DUMP_VEC(si->ned_motion_shift);                   printk("kern-	//si->ned_motion_shift\n");
	DUMP_VEC(si->ned_block_frmnum);                   printk("kern-	//si->ned_block_frmnum\n");
	DUMP_ARRAY3(si->ned_sad_thr, 2, 2, 4);            printk("kern-	//si->ned_sad_thr[2][2][4]\n");
	DUMP_ARRAY2(si->ned_sad_thr_cpx_step, 2, 3);      printk("kern-	//si->ned_sad_thr_cpx_step[2][3]\n");
	DUMP_VEC(si->c_sad_min_bias);                     printk("kern-	//si->c_sad_min_bias\n");
	DUMP_VEC(si->mosaic_en);                          printk("kern-	//si->mosaic_en\n");
	DUMP_VEC(si->mosaic_intra_texture_en);            printk("kern-	//si->mosaic_intra_texture_en\n");
	DUMP_VEC(si->mosaic_motion_shift);                printk("kern-	//si->mosaic_motion_shift\n");
	DUMP_ARRAY1(si->mosaic_recon_flat_thr,  4);       printk("kern-	//si->mosaic_recon_flat_thr[4]\n");
	DUMP_ARRAY1(si->mosaic_diff_thr,  4);             printk("kern-	//si->mosaic_diff_thr[4]\n");
	DUMP_VEC(si->inter_sa8d_en);                      printk("kern-	//si->inter_sa8d_en\n");
	DUMP_VEC(si->pmdctrl_en);                         printk("kern-	//si->pmdctrl_en\n");
	DUMP_VEC(si->pmdctrl_mode);                       printk("kern-	//si->pmdctrl_mode\n");
	DUMP_VEC(si->pmdsu_en);                           printk("kern-	//si->pmdsu_en\n");
	DUMP_ARRAY1(si->pmdsu_thrd,  3);                  printk("kern-	//si->pmdsu_thrd[3]\n");
	DUMP_VEC(si->dlambda_ppmd_en);                    printk("kern-	//si->dlambda_ppmd_en\n");
	DUMP_VEC(si->dlambda_icsd_en);                    printk("kern-	//si->dlambda_icsd_en\n");
	DUMP_VEC(si->dlambda_pcsd_en);                    printk("kern-	//si->dlambda_pcsd_en\n");
	DUMP_VEC(si->dlambda_motion_shift);               printk("kern-	//si->dlambda_motion_shift\n");
	DUMP_VEC(si->dlambda_max_depth);                  printk("kern-	//si->dlambda_max_depth\n");
	DUMP_VEC(si->dlambda_frmqp_thrd);                 printk("kern-	//si->dlambda_frmqp_thrd\n");
	DUMP_VEC(si->dlambda_simple_dqp);                 printk("kern-	//si->dlambda_simple_dqp\n");
	DUMP_VEC(si->dlambda_nei_cplx_dqp);               printk("kern-	//si->dlambda_nei_cplx_dqp\n");
	DUMP_VEC(si->dlambda_still_dqp);                  printk("kern-	//si->dlambda_still_dqp\n");
	DUMP_VEC(si->dlambda_edge_dqp);                   printk("kern-	//si->dlambda_edge_dqp\n");
	DUMP_VEC(si->dlambda_recovery_en);                printk("kern-	//si->dlambda_recovery_en\n");
	DUMP_VEC(si->dlambda_start_recovery);             printk("kern-	//si->dlambda_start_recovery\n");
	DUMP_VEC(si->dlambda_end_recovery);               printk("kern-	//si->dlambda_end_recovery\n");
	DUMP_VEC(si->dlambda_recovery_dqp);               printk("kern-	//si->dlambda_recovery_dqp\n");
	DUMP_VEC(si->mvp_force_zero_en);                  printk("kern-	//si->mvp_force_zero_en\n");
	DUMP_VEC(si->mvp_force_zero_motion_shift);        printk("kern-	//si->mvp_force_zero_motion_shift\n");
	DUMP_VEC(si->color_shadow_sse_en);                printk("kern-	//si->color_shadow_sse_en\n");
	DUMP_VEC(si->color_shadow_sse_priority_en);       printk("kern-	//si->color_shadow_sse_priority_en\n");
	DUMP_VEC(si->color_shadow_sse_value_thrd_en);     printk("kern-	//si->color_shadow_sse_value_thrd_en\n");
	DUMP_VEC(si->color_shadow_sse_qp);                printk("kern-	//si->color_shadow_sse_qp\n");
	DUMP_VEC(si->color_shadow_sse_motion_shift);      printk("kern-	//si->color_shadow_sse_motion_shift\n");
	DUMP_ARRAY1(si->color_shadow_sse_ratio_thrd,  2); printk("kern-	//si->color_shadow_sse_ratio_thrd[2]\n");

	DUMP_ARRAY1(si->color_shadow_sse_value_thrd,  3); printk("kern-	//si->color_shadow_sse_value_thrd[3]\n");

	DUMP_ARRAY1(si->lambda_cfg,  11);                 printk("kern-	//si->lambda_cfg[11]\n");
	DUMP_VEC(si->md_jrfc_exceed_en);                  printk("kern-	//si->md_jrfc_exceed_en\n");
	DUMP_VEC(si->md_max_ofst_x);                      printk("kern-	//si->md_max_ofst_x\n");
	DUMP_VEC(si->md_max_ofst_y0);                     printk("kern-	//si->md_max_ofst_y0\n");
	DUMP_VEC(si->md_max_ofst_y1);                     printk("kern-	//si->md_max_ofst_y1\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  SDE Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->nal_en);   printk("kern-	//si->nal_en\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  TFM Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->tfm_path_en);            printk("kern-	//si->tfm_path_en\n");
	DUMP_VEC(si->scaling_list_en);        printk("kern-	//si->scaling_list_en\n");
	DUMP_VEC(si->do_not_clear_deadzone);  printk("kern-	//si->do_not_clear_deadzone\n");
	DUMP_VEC(si->deadzone_en);            printk("kern-	//si->deadzone_en\n");
	DUMP_VEC(si->rdoq_en);                printk("kern-	//si->rdoq_en\n");
	DUMP_VEC(si->acmask_en);              printk("kern-	//si->acmask_en\n");
	DUMP_VEC(si->acmask_type);            printk("kern-	//si->acmask_type\n");

	printk("kern-{  // tfm_grp\n");
	for(g=0;g<16;g++){
		printk("kern-{ \n");
		DUMP_ARRAY1(si->tfm_grp[g].rdoq_level_y,  3);   printk("kern-	//si->tfm_grp[g].rdoq_level_y[3]\n");
		DUMP_ARRAY1(si->tfm_grp[g].rdoq_level_c,  3);   printk("kern-	//si->tfm_grp[g].rdoq_level_c[3]\n");
		DUMP_ARRAY1(si->tfm_grp[g].deadzone_y_en, 3);   printk("kern-	//si->tfm_grp[g].deadzone_y_en[3]\n");
		DUMP_ARRAY1(si->tfm_grp[g].deadzone_y,    3);   printk("kern-	//si->tfm_grp[g].deadzone_y[3]\n");
		DUMP_ARRAY1(si->tfm_grp[g].deadzone_c_en, 3);   printk("kern-	//si->tfm_grp[g].deadzone_c_en[3]\n");
		DUMP_ARRAY1(si->tfm_grp[g].deadzone_c,    3);   printk("kern-	//si->tfm_grp[g].deadzone_c[3]\n");
		DUMP_ARRAY1(si->tfm_grp[g].acmask_y,      3);   printk("kern-	//si->tfm_grp[g].acmask_y[3]\n");
		DUMP_ARRAY1(si->tfm_grp[g].acmask_c,      3);   printk("kern-	//si->tfm_grp[g].acmask_c[3]\n");
		printk("kern-},\n");
	}
	printk("kern-},\n");

	printk("kern-{  // tfm_rdoq_param \n");
	for(g=0;g<9;g++){
		printk("kern-{ \n");
		DUMP_VEC(si->tfm_rdoq_param[g].clear1);              printk("kern-	//si->tfm_rdoq_param[g].clear1\n");
		DUMP_ARRAY1(si->tfm_rdoq_param[g].val0,      4);     printk("kern-	//si->tfm_rdoq_param[g].val0[4]\n");
		DUMP_ARRAY1(si->tfm_rdoq_param[g].val1,      4);     printk("kern-	//si->tfm_rdoq_param[g].val1[4]\n");
		DUMP_ARRAY1(si->tfm_rdoq_param[g].score,     4);     printk("kern-	//si->tfm_rdoq_param[g].score[4]\n");
		DUMP_ARRAY1(si->tfm_rdoq_param[g].tu8_last,  3);     printk("kern-	//si->tfm_rdoq_param[g].tu8_last[3]\n");
		DUMP_ARRAY1(si->tfm_rdoq_param[g].tu16_last, 3);     printk("kern-	//si->tfm_rdoq_param[g].tu16_last[3]\n");
		DUMP_ARRAY1(si->tfm_rdoq_param[g].tu32_last, 3);     printk("kern-	//si->tfm_rdoq_param[g].tu32_last[3]\n");
		DUMP_ARRAY1(si->tfm_rdoq_param[g].delta,     4);     printk("kern-	//si->tfm_rdoq_param[g].delta[4]\n");
		printk("kern-},\n");
	}
	printk("kern-},\n");

	DUMP_VEC(si->tfm_recovery_en);            printk("kern-	//si->tfm_recovery_en\n");
	DUMP_VEC(si->tfm_start_recovery);         printk("kern-	//si->tfm_start_recovery\n");
	DUMP_VEC(si->tfm_end_recovery);           printk("kern-	//si->tfm_end_recovery\n");
	DUMP_ARRAY2(si->tfm_recovery_add, 3, 3);  printk("kern-	//si->tfm_recovery_add[3][3]\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  DBLK Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->dblk_en);                 printk("kern-	//si->dblk_en\n");
	DUMP_VEC(si->beta_offset_div2);        printk("kern-	//si->beta_offset_div2\n");
	DUMP_VEC(si->tc_offset_div2);          printk("kern-	//si->tc_offset_div2\n");
	DUMP_VEC(si->sao_en);                  printk("kern-	//si->sao_en\n");
	DUMP_VEC(si->sao_y_flag);              printk("kern-	//si->sao_y_flag\n");
	DUMP_VEC(si->sao_c_flag);              printk("kern-	//si->sao_c_flag\n");
	DUMP_VEC(si->stc_cfg_mode);            printk("kern-	//si->stc_cfg_mode\n");
	DUMP_VEC(si->stc_fixed_merge);         printk("kern-	//si->stc_fixed_merge\n");
	DUMP_VEC(si->stc_fixed_mode);          printk("kern-	//si->stc_fixed_mode\n");
	DUMP_VEC(si->stc_fixed_subtype);       printk("kern-	//si->stc_fixed_subtype\n");
	DUMP_VEC(si->stc_fixed_offset0);       printk("kern-	//si->stc_fixed_offset0\n");
	DUMP_VEC(si->stc_fixed_offset1);       printk("kern-	//si->stc_fixed_offset1\n");
	DUMP_VEC(si->stc_fixed_offset2);       printk("kern-	//si->stc_fixed_offset2\n");
	DUMP_VEC(si->stc_fixed_offset3);       printk("kern-	//si->stc_fixed_offset3\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  BURC Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->rc_en);       printk("kern-	//si->rc_en\n");
	DUMP_VEC(si->bu_size);     printk("kern-	//si->bu_size\n");
	DUMP_VEC(si->bu_len);      printk("kern-	//si->bu_len\n");
	DUMP_VEC(si->rc_method);   printk("kern-	//si->rc_method\n");
	DUMP_ARRAY1(si->rc_thd,  12);       printk("kern-	//si->rc_thd[12]\n");
	DUMP_ARRAY1(si->rc_info,  128);     printk("kern-	//si->rc_info[128]\n");
	DUMP_VEC(si->bu_max_dqp);           printk("kern-	//si->bu_max_dqp\n");
	DUMP_VEC(si->bu_min_dqp);           printk("kern-	//si->bu_min_dqp\n");
	DUMP_VEC(si->rc_avg_len);           printk("kern-	//si->rc_avg_len\n");
	DUMP_VEC(si->rc_min_prop);          printk("kern-	//si->rc_min_prop\n");
	DUMP_VEC(si->rc_max_prop);          printk("kern-	//si->rc_max_prop\n");
	DUMP_VEC(si->rc_dly_en);            printk("kern-	//si->rc_dly_en\n");
	DUMP_VEC(si->rc_max_qp);            printk("kern-	//si->rc_max_qp\n");
	DUMP_VEC(si->rc_min_qp);            printk("kern-	//si->rc_min_qp\n");
	DUMP_VEC(si->rc_dqp_offset);        printk("kern-	//si->rc_dqp_offset\n");
	DUMP_VEC(si->tgtbs_offset_unit);    printk("kern-	//si->tgtbs_offset_unit\n");
	DUMP_VEC(si->force_max_dqp_bits);   printk("kern-	//si->force_max_dqp_bits\n");
	DUMP_VEC(si->adjust_unit_size);     printk("kern-	//si->adjust_unit_size\n");
	DUMP_VEC(si->adjust_unit_len);      printk("kern-	//si->adjust_unit_len\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  JRFCD Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->jrfc_en);                  printk("kern-	//si->jrfc_en\n");
	DUMP_VEC(si->jrfcv2_en);                printk("kern-	//si->jrfcv2_en\n");
	DUMP_ADDR(si->head_dsty_addr);          printk("kern-	//si->head_dsty_addr\n");
	DUMP_ADDR(si->head_dstc_addr);          printk("kern-	//si->head_dstc_addr\n");
	DUMP_ADDR(si->head_sp_dsty_addr);       printk("kern-	//si->head_sp_dsty_addr\n");
	DUMP_ADDR(si->head_sp_dstc_addr);       printk("kern-	//si->head_sp_dstc_addr\n");
	DUMP_ADDR(si->head_refy_addr);          printk("kern-	//si->head_refy_addr\n");
	DUMP_ADDR(si->head_refc_addr);          printk("kern-	//si->head_refc_addr\n");
	DUMP_ADDR(si->head_mrefy_addr);         printk("kern-	//si->head_mrefy_addr\n");
	DUMP_ADDR(si->head_mrefc_addr);         printk("kern-	//si->head_mrefc_addr\n");
	DUMP_ADDR(si->lm_head_total);           printk("kern-	//si->lm_head_total\n");
	DUMP_ADDR(si->cm_head_total);           printk("kern-	//si->cm_head_total\n");
	DUMP_VEC(si->body_combine_en);          printk("kern-	//si->body_combine_en\n");

	DUMP_VEC(si->jrfc_ref_ck_en);                 printk("kern-	//si->jrfc_ref_ck_en\n");
	DUMP_VEC(si->jrfc_max_ck_en);                 printk("kern-	//si->jrfc_max_ck_en\n");
	DUMP_VEC(si->jrfc_ref_lcux_range);            printk("kern-	//si->jrfc_ref_lcux_range\n");
	DUMP_VEC(si->jrfc_ref_lcuy_range);            printk("kern-	//si->jrfc_ref_lcuy_range\n");
	DUMP_VEC(si->jrfc_max_size_ratio);            printk("kern-	//si->jrfc_max_size_ratio\n");
	DUMP_VEC(si->jrfc_exceed_max_en);             printk("kern-	//si->jrfc_exceed_max_en\n");
	DUMP_VEC(si->jrfc_exceed_max_plcu_x);         printk("kern-	//si->jrfc_exceed_max_plcu_x\n");
	DUMP_VEC(si->jrfc_exceed_max_plcu_y);         printk("kern-	//si->jrfc_exceed_max_plcu_y\n");
	DUMP_VEC(si->jrfc_nxt_frm_head);              printk("kern-	//si->jrfc_nxt_frm_head\n");


	printk("kern-  //--------------------\r\n");
	printk("kern-  //  BUFSHARE Module Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_ADDR(si->buf_share_en);         printk("kern-	//si->buf_share_en\n");
	DUMP_ADDR(si->buf_share_size);       printk("kern-	//si->buf_share_size\n");
	DUMP_ADDR(si->buf_base_yaddr);        printk("kern-	//si->buf_base_yaddr\n");
	DUMP_ADDR(si->buf_base_caddr);        printk("kern-	//si->buf_base_caddr\n");
	DUMP_ADDR(si->buf_beyond_yaddr);      printk("kern-	//si->buf_beyond_yaddr\n");
	DUMP_ADDR(si->buf_beyond_caddr);      printk("kern-	//si->buf_beyond_caddr\n");
	DUMP_ADDR(si->buf_start_yaddr);       printk("kern-	//si->buf_start_yaddr\n");
	DUMP_ADDR(si->buf_start_caddr);       printk("kern-	//si->buf_start_caddr\n");
	DUMP_ADDR(si->buf_ref_yaddr);         printk("kern-	//si->buf_ref_yaddr\n");
	DUMP_ADDR(si->buf_ref_caddr);         printk("kern-	//si->buf_ref_caddr\n");
	DUMP_ADDR(si->buf_rem_mby);          printk("kern-	//si->buf_rem_mby\n");



	printk("kern-  //--------------------\r\n");
	printk("kern-  //  SW Param\r\n");
	printk("kern-  //--------------------\r\n");
	DUMP_VEC(si->frm_num_gop);                 printk("kern-	//si->frm_num_gop\n");
	DUMP_VEC(si->maxnum_ref);                  printk("kern-	//si->maxnum_ref\n");
	DUMP_VEC(si->use_dummy_byd);               printk("kern-	//si->use_dummy_byd\n");
	DUMP_ADDR(si->dummy_buf_beyond_yaddr);      printk("kern-	//si->dummy_buf_beyond_yaddr\n");
	DUMP_ADDR(si->dummy_buf_beyond_caddr);      printk("kern-	//si->dummy_buf_beyond_caddr\n");
}
#endif

int h265_set_slice_info(struct h265_slice_info *si, struct vpu_io_info *io)
{
	int i, j;
	int intra_mode = 0;
	unsigned int md_ctrl = 0;
	unsigned int md_info = 0;
	int uqpoffset = 0;
	int uv_qp_ofst = 0;
	int uv_dist_bias = 0;
	char qp_idx = 0;
	int cu32x_n      = 0;
	int cu32y_n      = 0;
	int cu16_num     = 0;
	int cps_size     = 0;
	int mdc_size     = 0;
	int mcec_size    = 0;
	int qpt_size     = 0;
	int ai_size      = 0;
	int tup_resi_thr_score = 0;
	int thr_idx = 0;
	h265_ifa_para_t *ifa = NULL;
	vpi_method_s m;
	int frame_idx = 0;
	unsigned int ddr_y_base = 0;
	unsigned int ddr_c_base = 0;
	int frame_width   = 0;
	int frame_height  = 0;

	//share buf
	int beyond_size   = 0;
	int y_space       = 0;
	int c_space       = 0;
	int y_every_space = 0;
	int c_every_space = 0;
	int last_y_ofst_addr = 0;
	int last_c_ofst_addr = 0;
	int y_ofst_addr      = 0;
	int c_ofst_addr      = 0;
	int emc_ofst = 0;
	int emc_start_addr = 0;
	int mv_size = 0;
	int pet_dqp_idx = 0;
	unsigned char idx = 0;
	unsigned char *p = NULL;
	int n = 0;
	int md_csd_bias = 0;
	int md_pmd_bias = 0;
	int md_lambda_bias = 0;


	unsigned char qpg_pet_cplx_thrd[3][12] = {{0,0,0,0,0,0,0,0,0,0,0,0},
		{255,255,255,255,255,255,255,255,255,255,255,255},
		{255,255,255,255,255,255,255,255,255,255,255,255}};

	char inter_size[4][4] = {{0},{1,0,0,1},{1,0,0,1},{1,0,0,1}};

	memset(si, 0, sizeof(struct h265_slice_info));
	memset(&m, 0, sizeof(vpi_method_s));
	/* h265_api_enc_init */
	ifa = &si->ifa_para;
	si->frame_type          = io->frame_type;
	si->frm_num_gop         = io->frm_num_gop;
	ifa->cps_ref1           = NULL;
	ifa->rrs1_en            = 0;
	ifa->rrs1_c_en          = 0;
	ifa->sobel_en           = 0;
	ifa->sobel_16_or_14     = 0;
	ifa->nei_cplx_en        = 0;
#ifdef CONFIG_SOC_PRJ007
	ifa->skin_en            = 1;
#endif
	ifa->skin_lvl_en[0]     = ifa->skin_en;
	ifa->skin_lvl_en[1]     = ifa->skin_en;
	ifa->skin_lvl_en[2]     = ifa->skin_en;
	md_info = 0x010;
	si->rm.md_mvs_all          = (md_info >> 4)  & 0xF;
	si->rm.md_mvs_abs          = (md_info >> 8)  & 0xF;
	md_ctrl = 0x110F;
	si->rm.md_pre_pmd[0]       = (md_ctrl >> 0)  & 0x3;//not used
	si->rm.md_pre_pmd[1]       = (md_ctrl >> 2)  & 0x3;//not used
	si->rm.md_pre_csd[0]       = (md_ctrl >> 9)  & 0x3;//not used
	si->rm.md_pre_csd[1]       = (md_ctrl >> 13) & 0x3;//not used

	si->video_type          = 0;
	si->frame_width         = C_ALIGN(io->width, 8);
	si->frame_height        = C_ALIGN(io->height, 8);
	si->efe_hw_width        = C_ALIGN(io->width, 8);
	si->efe_hw_height       = C_ALIGN(io->height, 8);
	si->frame_y_stride      = C_ALIGN(io->width, 8);
	si->frame_c_stride      = C_ALIGN(io->height, 8);
	si->work_mode           = 2;
	//si->frame_qp            = io->qp;
	//si->frame_qp_svac       = io->qp*4;
	si->raw_format          = 0;
	si->is_decode           = 0;
	si->rotate_mode         = 0;
	si->sign_hide_flag      = 0; //fix value
	si->ipred_usis          = 0;//needtodo 1; //fix value
	si->chng_en             = 1; //fix value
								 //ipred
	intra_mode  = 0x268a040;
	for(i=0;i<5;i++) si->intra_mode[i]   = (intra_mode  >> (i*6)) & 0x3f;
	for(i=0;i<5;i++) si->intra8_mode[i]  = (intra_mode  >> (i*6)) & 0x3f;
	for(i=0;i<5;i++) si->intra16_mode[i] = (intra_mode  >> (i*6)) & 0x3f;
	for(i=0;i<5;i++) si->intra32_mode[i] = (intra_mode  >> (i*6)) & 0x3f;
	si->csd_color					   = md_csd_cfg[2][m.func.csd];
	ifa->var_split_thrd                = md_csd_cfg2[m.thrd.csd][0];//csd
	RC_MEMCPY(ifa->frm_cplx_thrd,    cnst_frm_cplx_thrd);
	si->maxnum_ref                     = 1;
	si->md_jrfc_exceed_en = 0;
	si->md_max_ofst_y1    = si->md_max_ofst_y0 + 16;//t33
#ifdef CONFIG_SOC_PRJ007
	ifa->refresh_en         = 1;
	ifa->refresh_mode       = 0;
	ifa->rfh_raw_thrd       = 16;
	ifa->rfh_edge_thrd      = 130;
	ifa->rfh_flat_thrd      = 800;
	ifa->rrs_filter_mode    = 1;
	ifa->cu_unifor_en       = 1;
	ifa->sobel_edge_thrd    = 130;
	ifa->ery_en             = 1;
	ifa->still_times_en     = 0;//t32 always close

	//speed up
	si->pmdsu_thrd[0]  = 10;
	si->pmdsu_thrd[1]  = 11;
	si->pmdsu_thrd[2]  = 12;

	//skin
	RC_MEMCPY( ifa->skin_level,       cnst_skin_level);
	RC_MEMCPY( ifa->skin_thrd,        cnst_skin_thrd);
	RC_MEMCPY( ifa->skin_mul_factor,  cnst_skin_mul_factor);
	RC_MEMCPY( ifa->qpg_skin_qp_ofst, cnst_qpg_skin_qp_ofst);

	//ifa_v31_flag
	RC_MEMCPY( ifa->sobel_blk16_thr,    cnst_sobel_blk16_thr);
	RC_MEMCPY( ifa->sobel_blk32_thr,    cnst_sobel_blk32_thr);
	RC_MEMCPY( ifa->smd_blk8_thr,       cnst_smd_blk8_thr);
	RC_MEMCPY( ifa->smd_blk16_thr,      cnst_smd_blk16_thr);
	RC_MEMCPY( ifa->smd_blk32_thr,      cnst_smd_blk32_thr);

	//frm_cplx_thrd
	RC_MEMCPY(ifa->frm_cplx_thrd,    cnst_frm_cplx_thrd);
	RC_MEMCPY(ifa->filter_thrd,      cnst_filter_thrd);

	//petbc
	ifa->pet_mode          = 1;
	RC_MEMCPY( ifa->petbc_var_thr,      cnst_petbc_var_thr);
	RC_MEMCPY( ifa->petbc_ssm_thr,      cnst_petbc_ssm_thr);
	RC_MEMCPY( ifa->petbc2_var_thr,     cnst_petbc2_var_thr);
	RC_MEMCPY( ifa->petbc2_ssm_thr,     cnst_petbc2_ssm_thr);

	//qpg
	ifa->qpg_cu32_qpm       = 0;
	ifa->crp_thrd           = 2;

	//cu_uniform
	ifa->smd_blk16_bias[0] = 1;
	ifa->smd_blk16_bias[1] = 2;
	RC_MEMCPY(si->rm.ppmd_cost_thr,             cnst_ppmd_cost_thr);
	RC_MEMCPY(si->rm.pmd_qp_thrd,               cnst_pmd_qp_thrd);
	RC_MEMCPY(si->rm.ppmd_thr_list,             cnst_ppmd_thr_list);
	RC_MEMCPY(si->rm.ppmd_merge_thr_idx,        cnst_ppmd_merge_thr_idx);
	RC_MEMCPY(si->rm.ppmd_search_thr_idx,       cnst_ppmd_search_thr_idx);
	RC_MEMCPY(si->rm.ppmd_merge_search_thr_idx, cnst_ppmd_merge_search_thr_idx);
	RC_MEMCPY(si->rm.ppmd_intra_thr_idx,        cnst_ppmd_intra_thr_idx);
	si->mrg1_en                       = 1;

	//frm md
	md_csd_bias = 0;
	md_pmd_bias = 0;
	md_lambda_bias = 0;
	for (i=0;i<4;i++) si->rm.md_csd_bias[i][0]    = (md_csd_bias >> i)          & 0x1; //cu8/16/32/64 en, not used
	for (i=0;i<4;i++) si->rm.md_csd_bias[i][1]    = (md_csd_bias >> 4*(i+1))    & 0xF; //cu8/16/32/64 val, not used
	for (i=0;i<3;i++) si->rm.md_pmd_bias[i][0]    = (md_pmd_bias >> i)          & 0x1; //cu8/16/32 ned bias en
	for (i=0;i<3;i++) si->rm.md_pmd_bias[i][1]    = (md_pmd_bias >> 4*(i+1))    & 0xF; //cu8/16/32 ned bias val
	for (i=0;i<3;i++) si->rm.md_lambda_bias[i][0] = (md_lambda_bias >> i)       & 0x1; //sa8d/sse/stc lambda en
	for (i=0;i<3;i++) si->rm.md_lambda_bias[i][1] = (md_lambda_bias >> 4*(i+1)) & 0xF; //sa8d/sse/stc lambda val

	si->rm.srd_cfg = 1; // srd
	si->rm.cps_mode = 1;
	si->rm.rrs_mode = 1;
	si->rm.raw_avg_en = 1;
	ifa->sobel_en = 1;

	/* motion */
	ifa->motion_thrd = si->rm.cps_mode ? 16 : 128;
	si->rm.frm_motion_thrd[0] = ((int)ifa->motion_thrd / 2 - 10) < 0 ? 0 : ifa->motion_thrd / 2 - 10;
	si->rm.frm_motion_thrd[1] = ((int)ifa->motion_thrd / 2 - 5) < 0 ? 0 : ifa->motion_thrd / 2 - 5;
	si->rm.frm_motion_thrd[2] = ifa->motion_thrd * 2 + 5;
	si->rm.frm_motion_thrd[3] = ifa->motion_thrd * 2 + 10;
	ifa->motion_of_thrd = si->rm.cps_mode ? 12 : 32;
	ifa->motion_of_thrd_c = si->rm.cps_mode ? 3 : 16;
	ifa->motion_thrd_c = si->rm.cps_mode ? 16 : 32;

	si->acmask_type = 1;

	si->dlambda_ppmd_en = md_dlbda_cfg[2][m.func.dlbda];
	si->dlambda_icsd_en = md_dlbda_cfg[3][m.func.dlbda];
	si->dlambda_pcsd_en = md_dlbda_cfg[4][m.func.dlbda];

	si->pmdctrl_en = md_su_cfg[2][m.func.su];
	si->rm.blk_pmdctrl_en = md_su_cfg[3][m.func.su];
	si->inter_sa8d_en = md_su_cfg[0][m.func.su] && si->frame_type == H265_SLICE_TYPE_P;
	if (si->inter_sa8d_en)
	{
		si->pmdctrl_en = 0xF;
		si->pmdctrl_mode = 0;
		si->rm.blk_pmdctrl_en = 0;
	}

	si->buf_share_en = 1;
	si->buf_share_size = 1;
	si->max_mvrx = 64;
	si->max_mvry = 32;

	si->var_flat_sub_size = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0 : 1; // csd
	si->rm.csd_p_cu64_en = md_csd_cfg[3][m.func.csd];

	si->uv_dist_scale_en = 1;

	RC_MEMCPY( si->ned_score_table,               cnst_ned_score_table[m.thrd.ned]); //ned cost
	RC_MEMCPY( si->ned_score_bias,                cnst_ned_score_bias[m.thrd.ned]);     //ned cost
	RC_MEMCPY( ifa->qpg_pet_qp_ofst,      cnst_qpg_pet_qp_ofst);
	RC_MEMCPY( ifa->qpg_pet_qp_ofst_mt,   cnst_qpg_pet_qp_ofst_mt);
	RC_MEMCPY( ifa->qpg_dlt_thr,          cnst_qpg_dlt_thr);
#elif defined(CONFIG_SOC_PRJ008)
	ifa->frm_mt_smd_thr               = 22;
	/* still times */
	ifa->still_times_en               = 1;//t33 always open
	ifa->still_times_motion_shift     = 7;

	ifa->row_smd_unit                 = si->frame_height > 2048 ? 1 : 0;//fix value
	ifa->smd_filter_thrd              = 2;
	// pmd
	si->ipmd_ras_en                   = 1;
	si->ipmd_sobel_edge_thrd          = 130;
	RC_MEMCPY( si->ipmd_sobel_thr,          cnst_intra_angle_thr);

	// mce
	si->subpixel_sech_cu8_en          = 1;
	si->subpixel_sech_cu16_en         = 1;
	si->subpixel_sech_cu32_en         = 1;
	si->mrg1_en                       = 1;
	//recovery
	si->dlambda_start_recovery        = 1;
	si->dlambda_end_recovery          = 3;
	si->dlambda_recovery_dqp          = -10;

	si->dlambda_ppmd_en               = 1;
	si->dlambda_icsd_en               = 1;
	si->dlambda_pcsd_en               = 1;
	si->pmdctrl_en                    = 0xF;
	si->inter_sa8d_en                 = 0;//needtodo t33

	// bjmb
	si->jrfcv2_en                     = 0;
	si->body_combine_en               = 0;
	si->jrfc_ref_lcux_range = si->jrfcv2_en? 3 : 0;
	si->jrfc_ref_lcuy_range = si->jrfcv2_en? 1 : 0;
	si->jrfc_max_size_ratio = si->jrfcv2_en? 100 : 0;
	/* motion */
	ifa->motion_thrd                  = 16;
	ifa->motion_of_thrd               = 12;
	ifa->motion_of_thrd_c             = 3;
	ifa->motion_thrd_c                = 16;
	si->acmask_type                   = 0;

	si->buf_share_en = 1;
	si->buf_share_size = 1;
	si->max_mvrx                      = 192;
	si->max_mvry                      = si->buf_share_en && (si->buf_share_size * 64 - 8 < 192) ? si->buf_share_size * 64 - 8 : 192;

	si->var_flat_sub_size             = 0; //fix value
	si->uv_dist_scale_en              = 1;

	/* nal_en:‌检查到连续的00 00就插入0x03字节，防止数据内部出现与NALU起始码（0x000001）冲突的序列；
	   为T33新增功能，t33默认开启nal_en，由软件做header部分的检查，硬件做裸码流的检查；
	   t32无此参数，由纯软件做header+码流的检查 */
	si->nal_en                        = 0;
#endif
	si->emc_bs_en                     = 1; //fix value
	si->ip_cfg_en                     = 0; //fix value
	si->emc_cps_en                    = 1; //fix value

#ifdef CONFIG_SOC_PRJ007
	si->emc_mvo_en = si->frm_num_gop > 0;
	si->emc_mvi_en = si->emc_mvo_en && si->frm_num_gop > 1;
#elif defined(CONFIG_SOC_PRJ008)
	si->emc_mvo_en = ifa->still_times_en ? 1 : si->frm_num_gop > 0;
	si->emc_mvi_en = ifa->still_times_en ? si->frm_num_gop > 0 : si->frm_num_gop > 1;
	// dlambda recovery
	si->dlambda_recovery_en = si->dlambda_ppmd_en && ifa->still_times_en;
#endif
	/* h265_api_emc_addr */
	cu32x_n      = (si->frame_width+31)/32;
	cu32y_n      = (si->frame_height+31)/32;
	cu16_num     = cu32x_n*cu32y_n*4;
	emc_ofst = 0;
	emc_start_addr = C_ALIGN((int)io->vpu_phyaddr, 256);
	ai_size    = C_ALIGN(cu16_num*4,  256); // 128bits    per cu32   cu16_num/4*4*4
	mdc_size   = C_ALIGN(cu16_num*4,  256); // 128bits    per cu32   cu16_num/4*4*4
	mcec_size  = C_ALIGN(cu16_num/2,  256); // 16 bits    per cu32   cu16_num/2
	qpt_size   = C_ALIGN(cu16_num,    256); // 128bits    per cu64   cu16_num/16*4*4
	mv_size    = C_ALIGN(cu16_num*4,  256); // 128bits    per cu32   cu16_num/4*4*4
	cps_size   = C_ALIGN(cu16_num*24, 256); // 128+64bits per cu16   cu16_num*4*4 + cu16_num/2*4*4  cps+cpsc

#ifdef CONFIG_SOC_PRJ007
	ai_size = 0;
	mcec_size  = 0;//C_ALIGN(cu16_num*4,  256);
#endif
	si->emc_cps_addr  = (unsigned char *)(emc_start_addr + emc_ofst); emc_ofst += cps_size;
	si->emc_mdc_addr  = (unsigned char *)(emc_start_addr + emc_ofst); emc_ofst += mdc_size;
	si->emc_qpt_addr  = (unsigned char *)(emc_start_addr + emc_ofst); emc_ofst += qpt_size;
	si->emc_mcec_addr = (unsigned char *)(emc_start_addr + emc_ofst); emc_ofst += mcec_size;
	si->emc_ai_addr   = (unsigned char *)(emc_start_addr + emc_ofst); emc_ofst += ai_size;
	si->emc_mvo_addr  = (unsigned char *)(emc_start_addr + emc_ofst); emc_ofst += mv_size;
	si->emc_mvi_addr   = si->emc_mvi_en  ? si->emc_mvo_addr : NULL;
	ifa->cps_ref0     = si->emc_cps_addr;

	si->bsfull_intr_en = io->bsfull_en;
	if(si->bsfull_intr_en == 1) {
		si->bsfull_intr_size   = io->bsfull_size;
		si->emc_bs_addr0       = (unsigned char *)io->bs_phyaddr;
		si->emc_bs_addr1       = (unsigned char *)(io->bs_phyaddr + io->bsfull_size * 1024);
	}
	else {
		si->bsfull_intr_size  = 0;
		si->emc_bs_addr0 = (unsigned char *)io->bs_phyaddr;
		si->emc_bs_addr1 = NULL;
	}

	//---------------------------------------
	//     ifa cfg(33)
	//---------------------------------------
	ifa->max_still_times              = 5;
	ifa->ras_en                       = 1;//265 fix 1; 264 fix 0
	ifa->ifa_en                       = 1; //fix value
	ifa->qpg_en                       = 1; //fix value
	ifa->emc_out_en[0]                = 1; //fix value
	ifa->emc_out_en[1]                = 1; //fix value
	ifa->cps_en                       = 1; //fix value
	ifa->cps_c_en                     = 1; //fix value
	ifa->smd_en                       = 1; //fix value
	ifa->smd_c_en                     = 1; //fix value
	ifa->petbc_en                     = 1; //fix value
	ifa->crp_thrd                     = 2;
	ifa->qpg_cu32_qpm                 = 0;

	/* rrs */
	ifa->rrs_thrd                     = 4;
	ifa->rrs_thrd_c                   = 2;
	/* motion */
	RC_MEMCPY(ifa->rrc_motion_thr,    cnst_rrc_motion_thr);
	/* petbc */
	ifa->pet_fil_en                   = 1;
	RC_MEMCPY(ifa->petbc_var_thr,     cnst_petbc_var_thr);
	RC_MEMCPY(ifa->petbc_ssm_thr,     cnst_petbc_ssm_thr);
	/* qpg */
	ifa->qpg_smd_en                   = 1;
	ifa->qpg_filte_en                 = 1;
	RC_MEMCPY(ifa->qpg_smd_cplx_thrd, cnst_qpg_smd_thrd[1]);

	qp_idx = (io->qp < 20 ? 0 :
			io->qp < 27 ? 1 :
			io->qp < 33 ? 2 :
			io->qp < 39 ? 3 :
			io->qp < 42 ? 4 :
			io->qp < 46 ? 5 : 6);
	RC_MEMSET(ifa->qpg_pet_qp_idx,       qp_idx);

	pet_dqp_idx = io->qp <= 22 ? 0 : (io->qp - 22)/3;
	for(n=0;n<3;n++) {
		idx = cnst_qpg_pet_dqp_idx[n][pet_dqp_idx];
		p = cnst_qpg_pet_dqp[idx];
		for(i=1;i<=3;i++)
			for(j=p[i-1];j<p[i];j++)
				qpg_pet_cplx_thrd[n][j] = cnst_qpg_pet_cplx_thrd[n][i-1];
	}
	RC_MEMCPY( ifa->qpg_pet_cplx_thrd,    qpg_pet_cplx_thrd);
	RC_MEMCPY( ifa->qpg_cplx_qp_ofst,     cnst_qpg_cplx_qp_ofst[qp_idx]);
	RC_MEMCPY( ifa->qpg_smd_cplx_thrd,    cnst_qpg_smd_thrd[1]);

	/* var flat */
	ifa->var_flat_en                  = 1;
	RC_MEMCPY(ifa->var_flat_thr,      cnst_var_flat_thr);

	//---------------------------------------
	//     ipred(12)
	//---------------------------------------
	si->sobel_en                      = 1; //fix value
	si->ipred_size                    = 7; //fix value
	si->sobel_size                    = 7; //fix value
	si->md_bc_close                   = 1; //fix value
	si->ppred_mode_num                = 3; //fix value
	si->ipmd_mpm_use_maxnum           = 1;
	si->ipred_slv_en                  = 0; //fix value
	si->ppred_slv_en                  = 0; //fix value
	si->frm_ipmd_angle_en	      	  = 0;

	RC_MEMCPY( si->intra_angle_bias,    cnst_ipred_angle_bias);
	RC_MEMCPY( si->frmb_ip_bits_en,     cnst_frmb_ip_bits_en);
	RC_MEMCPY( si->frmb_ip_bias,        cnst_frmb_ip_bias[0]);//t33use1
	RC_MEMCPY( si->frmb_ipc_bias,       cnst_frmb_ipc_bias);
	RC_MEMCPY( si->frmb_idx,            cnst_frmb_idx);
	RC_MEMCPY( si->ipmd_dm_cost_bias,   cnst_ipmd_dm_cost_bias);
	RC_MEMCPY( si->intra_bits,          cnst_intra_bits);

	si->half_pixel_sech_en            = 1;
	si->quart_pixel_sech_en           = 1;
	si->mrg_fpel_en                   = 0;//twx t32use0, 0x3;
	si->max_sech_step                 = 13;
	si->mce_scl_mode                  = 3;
	si->merge_cand_num                = 2;
	si->mce_slc_mvx                   = 0;
	si->mce_slc_mvy                   = 0;
	RC_MEMCPY(si->inter_mode, inter_size);

	//---------------------------------------
	//     md(45)
	//---------------------------------------
	si->ccf_rm_en                     = 0;//twx t32use0 , 1;
	si->dlambda_motion_shift          = 5;
	si->dlambda_max_depth             = 2;
	si->dlambda_nei_cplx_dqp          = 0;
	si->dlambda_still_dqp             = 3;
	si->dlambda_frmqp_thrd            = 44;

	//srd
	si->srd_motion_shift	      = 5;

	//mosaic
	RC_MEMCPY( si->mosaic_recon_flat_thr,       cnst_mosaic_recon_flat_thr);
	RC_MEMCPY( si->mosaic_diff_thr,             cnst_mosaic_diff_thr);
	si->mosaic_motion_shift           = si->frame_type == H265_SLICE_TYPE_P ? md_mos_cfg2[m.thrd.mos][0] : 0;//mosaic

	//ned
	si->ned_motion_en                 = 1;
	si->ned_motion_shift              = 7;
	si->c_sad_min_bias                = 5;
	si->ned_block_frmnum              = 5;
	RC_MEMCPY( si->ned_sad_thr_cpx_step,        cnst_ned_sad_thr_cpx_step);
	RC_MEMCPY( si->ned_sad_thr,                 cnst_ned_sad_thr);

	//color shadow sse
	si->mvp_force_zero_en              = 1;
	si->mvp_force_zero_motion_shift    = 7;
	si->color_shadow_sse_en            = 1;
	si->color_shadow_sse_priority_en   = 1;
	si->color_shadow_sse_value_thrd_en = 0;
	si->color_shadow_sse_qp            = 35;
	si->color_shadow_sse_motion_shift  = 5;
	RC_MEMCPY( si->color_shadow_sse_value_thrd, cnst_color_shadow_sse_value_thrd);
	RC_MEMCPY( si->color_shadow_sse_ratio_thrd, cnst_color_shadow_sse_ratio_thrd);

	//---------------------------------------
	//     others(5)
	//---------------------------------------
	RC_MEMCPY( ifa->pet_filter_valid,   cnst_pet_filter_valid);

	si->do_not_clear_deadzone         = 0;
	si->dblk_en                       = 1;
	si->beta_offset_div2              = 0;
	si->tc_offset_div2                = 0;

	/* get_base_cfg */
	qp_idx = (io->qp < 33 ? 1 :
			io->qp < 39 ? 2 :
			io->qp < 46 ? 3 : 4);

	//base
	si->frame_type      = io->frame_type == H265_SLICE_TYPE_IDR ? 2 : 1;
	si->frame_qp        = io->qp;
	ifa->base_qp        = io->qp;
	si->tfm_path_en     = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0x124 : 0x1FF;
	ifa->rrs_en         = ifa->cps_en && si->frame_type != H265_SLICE_TYPE_IDR;
	ifa->rrs_c_en       = ifa->rrs_en;
	ifa->motion_en      = ifa->rrs_en;
	ifa->motion_c_en    = ifa->rrs_c_en;

	si->mce_pref_mode   = si->frame_type == 2 ? 0 : 3;
	//    si->mref_en         = (f->file_param_en ? si->mref_en : 0) && si->frm_num_gop > 1;
	si->mref_wei        = si->mref_en ? 0 : 0;
	//    RC_MEMCPY(ifa->qpg_cplx_qp_ofst,  cnst_qpg_cplx_qp_ofst[qp_idx]);
	RC_MEMCPY(si->ipmd_mpm_cost_bias, cnst_ipmd_mpm_cost_bias[qp_idx]);
	uqpoffset = 0;
	uv_qp_ofst = si->frame_qp - (chromaScale_ofst_radix[si->frame_qp] + uqpoffset);
	uv_dist_bias = (uv_qp_ofst + 12) % 3;
	si->uv_dist_scale[0] = uv_dist_bias==0 ? 16 : uv_dist_bias==1 ? 20 : 25;
	si->uv_dist_scale[1] = (unsigned char)(((uv_qp_ofst < 0) ? (-uv_qp_ofst+2) : uv_qp_ofst) / 3);
	si->uv_dist_scale[2] = (unsigned char)(uv_qp_ofst < 0);
	//csd
	si->i_csd_en	= 1 && si->frame_type == H265_SLICE_TYPE_IDR;
	si->p_csd_en	= 1 && si->frame_type == H265_SLICE_TYPE_P;

	//ipmd
	si->i_pmd_en        = 1;
	//srd
	si->srd_en                      = 0 && si->frame_type == H265_SLICE_TYPE_P;
	si->sech_force_mv0_en       = si->srd_en ? 1 : 0;
	si->mf_sel_sech_skip32_en       = si->srd_en ? 1 : 0;
	si->mf_sel_sech_skip16_en       = si->srd_en ? 1 : 0;
	si->sech_force_mv0_mode     = si->srd_en ? 1 : 1;
	//ned
	if(si->frame_type == H265_SLICE_TYPE_IDR)
		RC_MEMSET( si->recon_resi_en,             0);
	else
		RC_MEMCPY( si->recon_resi_en,             cnst_recon_resi_en);
	//lambda
	RC_MEMCPY( si->lambda_cfg,  cnst_lambda_cfg[6]);

	// T32 unique
#ifdef CONFIG_SOC_PRJ007
	si->rm.frame_qp_svac           = io->qp*4;
#elif defined(CONFIG_SOC_PRJ008)
	si->rm.frame_qp_svac           = 0;
#endif
	si->rm.tfm_buf_en              = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0x17 : 0x1F;
	//si->tfm_path_en[0]          = (si->frame_type == H265_SLICE_TYPE_IDR) ? 0x124 : 0x3FF;
	//si->tfm_path_en[1]          = 0;

	tup_resi_thr_score = 16;
	for(i = 0; i < 8; i++){
		if(si->frame_qp < tup_resi_qp_tbl[0][i]){
			tup_resi_thr_score = tup_resi_qp_tbl[1][i];
			break;
		}
	}
	si->rm.mce_resi_thr[0][0][0]   = tup_resi_thr_score;
	si->rm.mce_resi_thr[0][0][1]   = 32;
	si->rm.mce_resi_thr[0][0][2]   = tup_resi_thr_score;
	si->rm.mce_resi_thr[0][0][3]   = 256;

	si->rm.mce_resi_thr[1][0][0]   = tup_resi_thr_score;
	si->rm.mce_resi_thr[1][0][1]   = 32;
	si->rm.mce_resi_thr[1][0][2]   = tup_resi_thr_score;
	si->rm.mce_resi_thr[1][0][3]   = 256;

	// si->rm.intra_resi_thr[0][0][0] = tup_resi_thr_score;
	// si->rm.intra_resi_thr[0][0][1] = 32;
	// si->rm.intra_resi_thr[0][0][2] = tup_resi_thr_score;
	// si->rm.intra_resi_thr[0][0][3] = 256;

	//si->rm.intra_resi_thr[1][0][0] = tup_resi_thr_score;
	//si->rm.intra_resi_thr[1][0][1] = 32;
	//si->rm.intra_resi_thr[1][0][2] = tup_resi_thr_score;
	//si->rm.intra_resi_thr[1][0][3] = 256;

	//si->rm.cps_size           = ifa->cps_mode ? 16 : 64;
	//si->rm.stride_refy        = si->frame_width / (ifa->cps_mode == 0 ? 2 : 4);
	si->rm.rrs_of_en          = ifa->rrs_en;

	/* get_rc_control_api_cfg */
	si->rcn_plain_en	            = si->i_csd_en || si->p_csd_en;

	si->sech_force_mv0_mode	    = 1;
	ifa->motion_thrd		        = 16;//same with init
	ifa->motion_of_thrd		        = 12;//same with init
	si->color_shadow_sse_en	        = 1;//0;
	si->color_shadow_sse_priority_en  = si->color_shadow_sse_en;
	si->dlambda_simple_dqp	        = -5;
	si->dlambda_edge_dqp	        = -2;
	si->dlambda_motion_shift	    = 5;//7;

	si->mosaic_en               = 1 && si->frame_type == H265_SLICE_TYPE_P;
	si->mosaic_intra_texture_en = 1 && si->mosaic_en;

	si->rc_en         = 0;
	si->bu_size = 0;
	si->bu_len = 0;
	si->rc_method = 0;
	si->bu_max_dqp = 0;
	si->bu_min_dqp = 0;
	si->rc_avg_len = 0;
	si->rc_min_prop = 0;
	si->rc_max_prop = 0;
	si->rc_dly_en = 0;
	memset(si->rc_thd, 0, sizeof(unsigned short) * 12);
	memset(si->rc_info, 0, sizeof(unsigned int) * 128);

	ifa->max_qp             = si->frame_qp+25;
	ifa->min_qp             = si->frame_qp;
	C_CLIP3(1,51,ifa->max_qp);
	C_CLIP3(1,51,ifa->min_qp);
	ifa->qpg_max_qp          = ifa->max_qp;
	ifa->qpg_min_qp          = ifa->min_qp;
	si->rc_min_qp = ifa->min_qp;
	si->rc_max_qp = ifa->max_qp;

	for (i = 0; i < 7; i++)
	{
		if (si->frame_qp < cnst_csd_qp_thrd_t32[m.thrd.csd][i])
		{
			thr_idx = i;
			break;
		}
	}
	if (thr_idx < 4)
	{
		for (i = 0; i < 5; i++)
		{
			si->csd_qp_thrd[i] = cnst_csd_qp_thrd_t32[m.thrd.csd][i];
			si->var_flat_recon_thrd[0][i] = cnst_var_flat_recon_thrd_t32[m.thrd.csd][0][i];
			si->var_flat_recon_thrd[1][i] = cnst_var_flat_recon_thrd_t32[m.thrd.csd][1][i];
		}
	}
	else if (thr_idx == 6)
	{
		for (i = 2; i < 7; i++)
		{
			si->csd_qp_thrd[i - 2] = cnst_csd_qp_thrd_t32[m.thrd.csd][i];
			si->var_flat_recon_thrd[0][i - 2] = cnst_var_flat_recon_thrd_t32[m.thrd.csd][0][i];
			si->var_flat_recon_thrd[1][i - 2] = cnst_var_flat_recon_thrd_t32[m.thrd.csd][1][i];
		}
	}
	else
	{
		for (i = thr_idx - 3; i < thr_idx + 2; i++)
		{
			si->csd_qp_thrd[i + 3 - thr_idx] = cnst_csd_qp_thrd_t32[m.thrd.csd][i];
			si->var_flat_recon_thrd[0][i + 3 - thr_idx] = cnst_var_flat_recon_thrd_t32[m.thrd.csd][0][i];
			si->var_flat_recon_thrd[1][i + 3 - thr_idx] = cnst_var_flat_recon_thrd_t32[m.thrd.csd][1][i];
		}
	}

	ifa->qpg_table_en   = 0;
	si->emc_qpt_en      = ifa->qpg_table_en;
	ifa->qpg_roi_en = 0;

	for(i=0;i<16;i++){
		si->roi_info[i].roi_en = 0;
		si->roi_info[i].roi_md = 1;
		si->roi_info[i].roi_qp = 21;
		si->roi_info[i].roi_lmbx = 0;
		si->roi_info[i].roi_rmbx = 0;
		si->roi_info[i].roi_umby = 0;
		si->roi_info[i].roi_bmby = 0;
	}

	si->mce_cfg_en      = 0;
	si->acmask_en	= 0;
	si->deadzone_en     = 0;
	//si->md_cfg_en       = 1 || si->acmask_en || si->deadzone_en;
	si->md_cfg_en       = 0;

	/* codec */
    si->use_dqp_flag     = 1;
    si->dqp_max_depth    = 2;
    si->tu_inter_depth   = 1;

	if (io->is_ivdc) {
		si->frame_y_stride   = 4096;
		si->frame_c_stride   = 4096;
	} else {
		si->frame_y_stride   = io->y_stride;
		si->frame_c_stride   = io->c_stride;
	}

	/* encode.c */
	if (io->is_ivdc) {
		si->raw_y_pa = 0x80000000;
		si->raw_c_pa = 0x81000000;
	} else {
		si->raw_y_pa = io->raw_y_phyaddr;
		si->raw_c_pa = io->raw_c_phyaddr;
	}
	si->des_va = io->des_viraddr;
	si->des_pa = io->des_phyaddr;
	si->bitstream_pa = io->bs_phyaddr;
	si->dst_y_stride  = (si->frame_width + 63) & ~63;
	si->dst_c_stride  = si->dst_y_stride/2;
	si->ref_y_stride  = (si->frame_width + 63) & ~63;
	si->ref_c_stride  = si->ref_y_stride;
#if 0
	si->dst_y_pa  = io->dec_y_phyaddr;
	si->dst_c_pa  = io->dec_c_phyaddr;
	si->ref_y_pa  = io->ref_y_phyaddr;
	si->ref_c_pa  = io->ref_c_phyaddr;
#else
	si->dst_y_pa  = 0;
	si->dst_c_pa  = 0;
	si->ref_y_pa  = 0;
	si->ref_c_pa  = 0;
#endif
	if (si->buf_share_en)
	{
		frame_idx = si->frm_num_gop;
		ddr_y_base = (unsigned int)(io->rds_y_phyaddr + 255) & ~255;
		ddr_c_base = (unsigned int)(io->rds_c_phyaddr + 255) & ~255;

		// align
		frame_width = (si->frame_width + 63) & ~63;
#ifdef CONFIG_SOC_PRJ007
		frame_height = (si->frame_height + 63) & ~63;
#elif defined(CONFIG_SOC_PRJ008)
		frame_height = (si->frame_height + 15) & ~15;
#endif
		// share buf
		beyond_size = (si->buf_share_size + 1) * 64;
		y_space = frame_width * (frame_height + beyond_size);
		c_space = frame_width * (frame_height / 2 + beyond_size / 2);
		y_every_space = frame_width * beyond_size;
		c_every_space = frame_width * (beyond_size / 2);

		si->buf_beyond_yaddr = ddr_y_base + y_space;
		si->buf_beyond_caddr = ddr_c_base + c_space;
		si->buf_base_yaddr = ddr_y_base;
		si->buf_base_caddr = ddr_c_base;
		si->buf_ref_yaddr = ddr_y_base;
		si->buf_ref_caddr = ddr_c_base;
		si->buf_start_yaddr = ddr_y_base;
		si->buf_start_caddr = ddr_c_base;

		if (si->frame_type != H265_SLICE_TYPE_IDR)
		{
			last_y_ofst_addr = io->s_last_y_ofst_addr;						  // cur ref y ofst addr
			last_c_ofst_addr = io->s_last_c_ofst_addr;						  // cur ref c ofst addr
			y_ofst_addr = (io->s_last_y_ofst_addr + y_every_space) % y_space; // cur rec y ofst addr
			c_ofst_addr = (io->s_last_c_ofst_addr + c_every_space) % c_space; // cur rec c ofst addr
			si->buf_start_yaddr = y_ofst_addr ? (si->buf_beyond_yaddr - y_ofst_addr) : ddr_y_base;
			si->buf_start_caddr = c_ofst_addr ? (si->buf_beyond_caddr - c_ofst_addr) : ddr_c_base;
			si->buf_ref_yaddr = last_y_ofst_addr ? (si->buf_beyond_yaddr - last_y_ofst_addr) : ddr_y_base;
			si->buf_ref_caddr = last_c_ofst_addr ? (si->buf_beyond_caddr - last_c_ofst_addr) : ddr_c_base;

			io->s_last_y_ofst_addr = y_ofst_addr;
			io->s_last_c_ofst_addr = c_ofst_addr;
		}
		else
		{
			io->s_last_y_ofst_addr = 0;
			io->s_last_c_ofst_addr = 0;
		}

		si->buf_rem_mby = ((si->buf_beyond_yaddr - si->buf_ref_yaddr) / (frame_width)) / 64 - 1;
	}
	si->mref_y_pa = 0;
	si->mref_c_pa = 0;

#ifdef CONFIG_SOC_PRJ008
	memset(&si->rm, 0, sizeof(h265_rm_t));
#endif

#ifdef DUMP_SLICE_INFO
	dump_t32v_h265_sliceinfo(si);
#endif
	H265E_T32V_SliceInit(si);
	return 0;
}

int h265_param_init(h265_t *inst, struct h265_param *param)
{
	struct h265_instance *h265 = (struct h265_instance *)inst;
	VPS *vps = NULL;
	SPS *sps = NULL;
	PPS *pps = NULL;

	memset(h265, 0, sizeof(struct h265_instance));

	memcpy(&h265->param, param, sizeof(struct h265_param));
	vps = &h265->header.nalu.m_vps;
	sps = &h265->header.nalu.m_sps;
	pps = &h265->header.nalu.m_pps;
	defaultVPS(vps, param);
	defaultSPS(sps, param);
	defaultPPS(pps, param);
	initBits(&h265->header.bs);

	return 0;
}

void h265_exit(h265_t *inst)
{
	struct h265_instance *h265 = (struct h265_instance *)inst;
	freeBits(&h265->header.bs);
	return;
}

int h265_wait_for_completion(struct h265_instance *h265, struct vpu_io_info *io)
{
	int v0_status = 0;
	int ret = 0;
wait_retry:
	ret = wait_for_completion_interruptible_timeout(hera_done_for_kernEnc, msecs_to_jiffies(5000));
	if (ret > 0) {
#if 0
		//debug hw time
		int t0 = *(volatile unsigned int*)(0xb3200000+0xa4);
		int hw_time = t0 * 1024 / (600 * 1000);
		printk("vpu hw_time=%d\n", hw_time);
#endif
		if (hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) {
			io->bslen = vpu_wrap_read_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_BSLEN);
			if (io->is_ivdc) {
				v0_status = *(volatile int *)(0xb310007c);
				if (v0_status & (0x3<<30)) {
					//printk("bad frame,overflow:0x%08x\n", v0_status);
					return -1;
				} else if ((((v0_status>>4) & 0x7) != io->sensor_id)) {
					//printk("encode sid%d, need sid%d\n", (v0_status>>4) & 0x7, io->sensor_id);
					return -1;
				}
			}
		} else {
			VPU_ERROR("vpu intr err!\n");
			return -1;
		}
	} else if (ret == -ERESTARTSYS) {
		goto wait_retry;
	} else {
		VPU_ERROR("vpu wait intr timeout!\n");
#ifdef DUMP_HERA_REG
		dump_hera_regs();
#endif
		return -1;
	}

	//code headers
	ret = h265_code_headers(&h265->header, io);
	if (ret < 0) {
		VPU_ERROR("h265 code header failed\n");
		return ret;
	}

	return 0;
}

int h265_wait_for_completion_bsfull(struct h265_instance *h265, struct vpu_io_info *io)
{
	int v0_status = 0;
	int i = 0;
	int bslen = 0;
	int bslen_delta = 0;
	int ret = 0;
	int ret_bs = 0;
	int bs_err_cnt = 0;

	while (1) {
		ret = wait_for_completion_interruptible_timeout(hera_done_for_kernEnc, msecs_to_jiffies(5000));
		if (ret > 0) {
#if 0
			//debug hw time
			int t0 = *(volatile unsigned int*)(0xb3200000+0xa4);
			int hw_time = t0 * 1024 / (600 * 1000);
			printk("vpu hw_time=%d\n", hw_time);
#endif
			if (hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) {
				if (io->is_ivdc) {
					v0_status = *(volatile int *)(0xb310007c);
					if (v0_status & (0x3<<30)) {
						//printk("bad frame,overflow:0x%08x\n", v0_status);
						return -1;
					} else if ((((v0_status>>4) & 0x7) != io->sensor_id)) {
						//printk("encode sid%d, need sid%d\n", (v0_status>>4) & 0x7, io->sensor_id);
						return -1;
					}
				}
			}
			bslen = vpu_wrap_read_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_BSLEN);
			bslen_delta = bslen - (i + 1) * io->bsfull_size * 1024;
			if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_BSFULL) && (bslen_delta >= 0)) {
				vpu_wrap_flush_cache(io->bs_viraddr, CACHE_ALL_SIZE, 2);
				if ((ret_bs = h265_code_headers_segment(&h265->header, io, io->bsfull_size * 1024, i, 0)) < 0) {
					VPU_ERROR("h265 encode segment error\n");
					bs_err_cnt ++;
				}
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
				i ++;
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_BSFULL) && (bslen_delta < 0)) {
				vpu_wrap_write_reg(RADIX_EMC_BASE + 0x60, 1);
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) && (i > 0)) {
				vpu_wrap_flush_cache(io->bs_viraddr, CACHE_ALL_SIZE, 2);
				if ((ret_bs = h265_code_headers_segment(&h265->header, io, bslen - i * io->bsfull_size * 1024, i, 1)) < 0) {
					VPU_ERROR("h265 encode segment error\n");
					bs_err_cnt ++;
				}
				io->vpu_curr_stream_len = h265->header.list.m_occupancy;
				return 0;
			} else if ((hera_irq_stat_for_kernEnc & RADIX_CFGC_INTE_ENDF) && (i == 0)) {
				io->bslen = bslen;
				break;
			} else {
				VPU_ERROR("vpu intr err!\n");
				return -1;
			}
		} else if (ret == -ERESTARTSYS) {
			continue;
		} else {
			VPU_ERROR("vpu wait intr timeout!\n");
			return -1;
		}
	}

	if (bs_err_cnt) {
		return ret_bs;
	}

	//code headers
	ret = h265_code_headers(&h265->header, io);
	if (ret < 0) {
		VPU_ERROR("h265 code header failed\n");
		return ret;
	}

	return 0;
}

int h265_encode_start(struct h265_instance *h265, struct vpu_io_info *io)
{
	int timeout = 0x7fff;
	unsigned int cfgc_glb_ctrl = 0;

	//flush cache
	vpu_wrap_flush_cache(io->des_viraddr, CACHE_ALL_SIZE, 0);

	//vpu reset
	vpu_wrap_write_reg(RADIX_CFGC_BASE+RADIX_REG_CFGC_GLB_CTRL, (0x1<<31) + 0xffff);//hera
	while (cfgc_glb_ctrl && --timeout) {
		cfgc_glb_ctrl = vpu_wrap_read_reg(RADIX_CFGC_BASE+RADIX_REG_CFGC_GLB_CTRL);
		vpu_wrap_usleep(20);
	}
	if (timeout == 0) {
		VPU_ERROR("vpu reset err!\n");
		return -1;
	}

	//ivdc
	if (io->is_ivdc) {
		*(volatile int *)(0xb3100000+0x78) = 0x1;;//write runfifo
		*(volatile int *)(0xb3100000+0x70) = io->sensor_id << 8 | 0x1;//write v0 start
	}

	//vpu start
	vpu_wrap_write_reg(RADIX_CFGC_BASE  + RADIX_REG_CFGC_GLB_CTRL, (0<<22) | (((1<<16) - 1) & 0xffff));
	vpu_wrap_write_reg(RADIX_CFGC_BASE  + RADIX_REG_CFGC_INTR_EN, RADIX_CFGC_INTE_IVDCRST | RADIX_CFGC_INTE_AMCERR | RADIX_CFGC_INTE_BSFULL | RADIX_CFGC_INTE_ENDF);
	vpu_wrap_write_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_VTYPE, 0x2 << 6);
#ifdef CONFIG_SOC_PRJ008
	vpu_wrap_write_reg(RADIX_CFGC_BASE + RADIX_REG_CFGC_EARB, (61 & 0xFF));
#endif
	vpu_wrap_write_reg(RADIX_EMC_BASE + RADIX_REG_EMC_ADDR_CHN, RADIX_VDMA_ACFG_DHA(io->des_phyaddr));
	vpu_wrap_write_reg(RADIX_CFGC_BASE+RADIX_REG_CFGC_ACM_CTRL, RADIX_VDMA_ACFG_RUN);

	//vpu intr
	if (io->bsfull_en) {
		return h265_wait_for_completion_bsfull(h265, io);
	} else {
		return h265_wait_for_completion(h265, io);
	}
}

int h265_code_headers_segment(struct h265_header_info *header, struct vpu_io_info *io, unsigned int bslen, int count, int is_final)
{
	Slice *slice = &header->nalu.m_slice;
	NALList *list = &header->list;
	int g_maxSlices = 1;
	int ret = 0;

	if (count == 0) {
		slice->m_pps = &header->nalu.m_pps;
		slice->m_sps = &header->nalu.m_sps;
		slice->m_poc = io->frm_num_gop;
		if (!io->frm_num_gop || io->frame_type == H265_SLICE_TYPE_IDR) {
			//frame I
			header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
			slice->m_sliceType = I_SLICE;
		} else {
			//frame P
			header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
			slice->m_sliceType = P_SLICE;
		}

		if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
			slice->m_lastIDR = slice->m_poc;
		}
		if (slice->m_sliceType == H265_SLICE_TYPE_B) {
			slice->m_colFromL0Flag = 0;
			slice->m_colRefIdx = 0;
		} else {
			slice->m_colFromL0Flag = 1;
			slice->m_colRefIdx = 0;
		}
		slice->m_maxNumMergeCand = 2;
#define SLFASE_CONSTANT  0x5f4e4a53
		g_maxSlices = 1;
		slice->m_sLFaseFlag = (g_maxSlices > 1) ? false : ((SLFASE_CONSTANT & (1 << (slice->m_poc % 31))) > 0);;
		slice->m_sliceQp = io->qp;
		slice->numRefIdxDefault[0] = slice->m_pps->numRefIdxDefault[0];
		slice->numRefIdxDefault[1] = slice->m_pps->numRefIdxDefault[1];
		slice->m_rpsIdx = -1;
		slice->m_bUseSao = 0;

		//we dont have long term ref, and only have I&P frame
		computeRPS(slice);
		// Ensuring L0 contains just the -ve POC
		slice->m_numRefIdx[0] = C_MIN(1, slice->m_rps.numberOfNegativePictures + slice->m_rps.numberOfLongtermPictures);
		// Ensuring L1 contains just the -ve POC
		slice->m_numRefIdx[1] = C_MIN(1, slice->m_rps.numberOfPositivePictures);

		initNal(list, (char *)io->vpu_stream_buffer + io->vpu_stream_buffer_occupancy, io->vpu_stream_buffer_len - io->vpu_stream_buffer_occupancy);
		if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
			getStreamHeaders(&header->nalu, list, &header->bs);
		}
	}
	ret = getStreamSegment(&header->nalu, list, &header->bs, (char *)io->bs_viraddr + (count % 2) * io->bsfull_size * 1024, bslen, count, is_final);
	if (ret < 0) {
		VPU_ERROR("h265 getStreamSegment failed\n");
		return ret;
	}
	return 0;
}

int h265_code_headers(struct h265_header_info *header, struct vpu_io_info *io)
{
	Slice *slice = &header->nalu.m_slice;
	NALList *list = &header->list;
	int g_maxSlices = 1;
	int ret = 0;

	slice->m_pps = &header->nalu.m_pps;
	slice->m_sps = &header->nalu.m_sps;
	slice->m_poc = io->frm_num_gop;
	if (!io->frm_num_gop || io->frame_type == H265_SLICE_TYPE_IDR) {
		//frame I
		header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
		slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
		slice->m_sliceType = I_SLICE;
	} else {
		//frame P
		header->nalu.m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
		slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_TRAIL_R;
		slice->m_sliceType = P_SLICE;
	}

	if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
		slice->m_lastIDR = slice->m_poc;
	}
	if (slice->m_sliceType == H265_SLICE_TYPE_B) {
		slice->m_colFromL0Flag = 0;
		slice->m_colRefIdx = 0;
	} else {
		slice->m_colFromL0Flag = 1;
		slice->m_colRefIdx = 0;
	}
	slice->m_maxNumMergeCand = 2;
#define SLFASE_CONSTANT  0x5f4e4a53
	g_maxSlices = 1;
	slice->m_sLFaseFlag = (g_maxSlices > 1) ? false : ((SLFASE_CONSTANT & (1 << (slice->m_poc % 31))) > 0);;
	slice->m_sliceQp = io->qp;
	slice->numRefIdxDefault[0] = slice->m_pps->numRefIdxDefault[0];
	slice->numRefIdxDefault[1] = slice->m_pps->numRefIdxDefault[1];
	slice->m_rpsIdx = -1;
	slice->m_bUseSao = 0;

	//we dont have long term ref, and only have I&P frame
	computeRPS(slice);
	// Ensuring L0 contains just the -ve POC
	slice->m_numRefIdx[0] = C_MIN(1, slice->m_rps.numberOfNegativePictures + slice->m_rps.numberOfLongtermPictures);
	// Ensuring L1 contains just the -ve POC
	slice->m_numRefIdx[1] = C_MIN(1, slice->m_rps.numberOfPositivePictures);

	initNal(list, (char *)io->vpu_stream_buffer + io->vpu_stream_buffer_occupancy, io->vpu_stream_buffer_len - io->vpu_stream_buffer_occupancy);
	if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL) {
		getStreamHeaders(&header->nalu, list, &header->bs);
	}
	ret = getStream(&header->nalu, list, &header->bs, io->bs_viraddr, io->bslen);
	if (ret < 0) {
		VPU_ERROR("h265 getStream failed\n");
		return ret;
	}

	io->vpu_curr_stream_len = header->list.m_occupancy;
	//printk("kernel encode %d curr len = %d\n", io->tot_frame_num, io->vpu_curr_stream_len);
	return 0;
}

int h265_encode(h265_t *inst, struct vpu_io_info *io)
{
	struct h265_instance *h265 = (struct h265_instance *)inst;
	int ret = 0;

	//init vdma
	h265_set_slice_info(&h265->vdma_info, io);

	//start hw
	//ktime_t start, end;
	//start = ktime_get();
	ret = h265_encode_start(h265, io);
	if (ret < 0) {
		VPU_ERROR("h265 encode failed\n");
		return ret;
	}
	//end = ktime_get();
	//printk("h265_hw_start cost %lld ms\n", ktime_to_ms(end) - ktime_to_ms(start));
	return 0;
}
