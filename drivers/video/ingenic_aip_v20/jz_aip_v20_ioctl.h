/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#ifndef _JZ_AIP_V20_IOCTL_H_
#define _JZ_AIP_V20_IOCTL_H_

// R&D Version 2.0.0    LiuTianyang
// R&D Version 2.1.0    LiuTianyang  2023-06-13
// Beta Version 2.0.1   ZhangJiawei  2023-07-27
#define AIP_V20_DRV_VERSION ((0x2 << 16) | (0x0 << 8) | (0x3))

// Reference Documentation/ioctl/ioctl-number.txt
#define AIP_V20_SUBMIT          0x20
#define AIP_V20_WAIT            0x21
#define AIP_V20_BO              0x22
#define NNA_V20_OP              0x23
#define AIP_V20_ACTION          0x24

#define IOCTL_AIP_V20_SUBMIT    _IOWR(0xF4, AIP_V20_SUBMIT, int)
#define IOCTL_AIP_V20_WAIT      _IOWR(0xF4, AIP_V20_WAIT, int)
#define IOCTL_AIP_V20_BO        _IOWR(0xF4, AIP_V20_BO, int)
#define IOCTL_NNA_V20_OP        _IOWR(0xF4, NNA_V20_OP, int)
#define IOCTL_AIP_V20_ACTION    _IOWR(0xF4, AIP_V20_ACTION, int)
// NNA
typedef enum {
	NNA_OP_STATUS,
	NNA_OP_MUTEX_LOCK,
	NNA_OP_MUTEX_TRYLOCK,
	NNA_OP_MUTEX_UNLOCK,
	NNA_OP_ORAM_ALLOC,
	NNA_OP_ORAM_FREE,
} aip_v20_ioctl_nna_op_t;

typedef struct {
	aip_v20_ioctl_nna_op_t          op;
	size_t                          total_l2c_size;
	size_t                          curr_l2c_size;
	size_t                          oram_size;
	uint32_t                        oram_pbase;
	size_t                          dma_size;
	uint32_t                        dma_pbase;
	size_t                          dma_desram_size;
	uint32_t                        dma_desram_pbase;
} aip_v20_ioctl_nna_status_t;

typedef struct {
	aip_v20_ioctl_nna_op_t          op;
	size_t                          size;
	int                             handle;
	uint32_t                        paddr;
} aip_v20_ioctl_nna_oram_t;

typedef struct {
	aip_v20_ioctl_nna_op_t          op;
	char                           *name;
} aip_v20_ioctl_nna_mutex_lock_t;

typedef struct {
	aip_v20_ioctl_nna_op_t          op;
	int                             force_unlock;
	int                             cpumask;
} aip_v20_ioctl_nna_mutex_unlock_t;

// AIP
typedef enum {
	AIP_CH_OP_P = 0,
	AIP_CH_OP_T,
	AIP_CH_OP_F,
	AIP_CH_OP_T_TASK,
} aip_v20_ioctl_ch_op_t;

typedef enum {
	AIP_DAT_POS_DDR = 0,
	AIP_DAT_POS_ORAM = 1,
} aip_v20_ioctl_dat_pos_t;

typedef enum {
	AIP_FORMAT_Y = 0,
	AIP_FORMAT_C = 1,
	AIP_FORMAT_BGRA = 2,
	AIP_FORMAT_FEATURE2 = 3,
	AIP_FORMAT_FEATURE4 = 4,
	AIP_FORMAT_FEATURE8 = 5,
	AIP_FORMAT_FEATURE8_H = 6,
	AIP_FORMAT_NV12 = 7,
} aip_v20_ioctl_f_format_t;

typedef enum {
	AIP_MODE_Y_TO_Y = 0,
	AIP_MODE_C_TO_C = 1,
	AIP_MODE_NV12_TO_BGRA = 2,
	AIP_MODE_BGRA_TO_BGRA = 3,
} aip_v20_ioctl_p_mode_t;

typedef enum{
	AIP_ORDER_BGRA = 0,
	AIP_ORDER_GBRA = 1,
	AIP_ORDER_RBGA = 2,
	AIP_ORDER_BRGA = 3,
	AIP_ORDER_GRBA = 4,
	AIP_ORDER_RGBA = 5,
	AIP_ORDER_ABGR = 8,
	AIP_ORDER_AGBR = 9,
	AIP_ORDER_ARBG = 10,
	AIP_ORDER_ABRG = 11,
	AIP_ORDER_AGRB = 12,
	AIP_ORDER_ARGB = 13,
} aip_v20_ioctl_order_t;

// AIP submit
typedef struct {
	uint32_t                      timeout[2];
	uint32_t                      scale_x[2];
	uint32_t                      scale_y[2];
	int32_t                       trans_x[2];
	int32_t                       trans_y[2];
	uint32_t                      src_base_y[2];
	uint32_t                      src_base_uv[2];
	uint32_t                      dst_base_y[2];
	uint32_t                      dst_base_uv[2];
	uint32_t                      src_size[2];
	uint32_t                      dst_size[2];
	uint32_t                      stride[2];
	uint32_t                      f_ctrl[2];
} aip_v20_f_node_t;

typedef struct {
	uint32_t                      timeout[2];
	uint32_t                      mode[2];
	uint32_t                      src_base_y[2];
	uint32_t                      src_base_uv[2];
	uint32_t                      src_stride[2];
	uint32_t                      dst_base[2];
	uint32_t                      dst_stride[2];
	uint32_t                      dst_size[2];
	uint32_t                      src_size[2];
	uint32_t                      dummy_val[2];
	uint32_t                      coef0[2];
	uint32_t                      coef1[2];
	uint32_t                      coef2[2];
	uint32_t                      coef3[2];
	uint32_t                      coef4[2];
	uint32_t                      coef5[2];
	uint32_t                      coef6[2];
	uint32_t                      coef7[2];
	uint32_t                      coef8[2];
	uint32_t                      coef9[2];
	uint32_t                      coef10[2];
	uint32_t                      coef11[2];
	uint32_t                      coef12[2];
	uint32_t                      coef13[2];
	uint32_t                      coef14[2];
	uint32_t                      offset[2];
	uint32_t                      p_ctrl[2];
} aip_v20_p_node_t;

typedef struct {
	uint32_t                      src_ybase;
	uint32_t                      src_cbase;
	uint32_t                      dst_base;
	uint32_t                      src_h;
	uint32_t                      src_w;
	uint32_t                      src_stride;
	uint32_t                      dst_stride;
	uint32_t                      task_len;
} aip_v20_t_node_t;

typedef struct {
	aip_v20_ioctl_order_t         order;
	uint32_t                      alpha;
	uint32_t                      offset[2];
} aip_v20_ioctl_offset_t;

typedef struct {
	aip_v20_ioctl_ch_op_t         op;
	aip_v20_ioctl_f_format_t      format;
	aip_v20_ioctl_dat_pos_t       pos_in;
	aip_v20_ioctl_dat_pos_t       pos_out;

	int                           node_handle;
	size_t                        node_num;
	uint32_t                      paddr;
	char                         *name;
	uint64_t                      seqno;
} aip_v20_ioctl_submit_f_t;

typedef struct {
	aip_v20_ioctl_ch_op_t         op;
	aip_v20_ioctl_p_mode_t        mode;
	aip_v20_ioctl_offset_t        offset;
	aip_v20_ioctl_dat_pos_t       pos_in;
	aip_v20_ioctl_dat_pos_t       pos_out;

	int                           node_handle;
	size_t                        node_num;
	uint32_t                      paddr;
	uint64_t                      seqno;
	void                         *param;
	char                         *name;
} aip_v20_ioctl_submit_p_t;

typedef struct {
	aip_v20_ioctl_ch_op_t         op;
	aip_v20_ioctl_offset_t        offset;
	aip_v20_ioctl_dat_pos_t       pos_in;
	aip_v20_ioctl_dat_pos_t       pos_out;
	void                         *param;
	void                         *node;

	char                         *name;
	uint64_t                      seqno;
} aip_v20_ioctl_submit_t_t;

// AIP wait
typedef struct {
	aip_v20_ioctl_ch_op_t         op;
	uint64_t                      seqno;
	uint64_t                      timeout_ns;
	uint32_t                      t_dst_base;
	uint32_t                      t_task_len;
	int64_t                       wait_time;
} aip_v20_ioctl_wait_t;


// AIP bo
typedef enum {
	AIP_BO_OP_APPEND = 0,
	AIP_BO_OP_DESTROY,
} aip_v20_ioctl_bo_op_t;

typedef struct {
	aip_v20_ioctl_bo_op_t         op;
	size_t                        size;
	int                           handle;
	uint32_t                      paddr;
} aip_v20_ioctl_bo_t;

struct jz_aip_v20_shared_data {
	uint64_t seqno;
};

// ACTION
typedef enum {
	AIP_ACTION_STATUS,
	AIP_ACTION_CACHE_FLUSH,
	AIP_ACTION_SET_PROC_NICE,
	AIP_ACTION_GET_AIP_RUNTIME,
	AIP_ACTION_GET_MEMINFO,
	AIP_ACTION_CTX_LOCK,
	AIP_ACTION_CTX_UNLOCK,
} aip_v20_ioctl_action_t;

typedef struct {
	aip_v20_ioctl_action_t op;
	uint32_t drv_version;
	uint32_t chip_version;
} aip_v20_ioctl_action_status_t;

typedef struct {
	aip_v20_ioctl_action_t op;
	uint32_t mem_total;
	uint32_t mem_free;
	uint32_t cma_total;
	uint32_t cma_free;
} aip_v20_ioctl_action_meminfo_t;

typedef struct {
	aip_v20_ioctl_action_t op;
	void *vaddr;
	size_t size;
	int dir; // enum dma_data_direction
} aip_v20_ioctl_action_cacheflush_t;

typedef struct {
	aip_v20_ioctl_action_t op;
	int32_t value;
} aip_v20_ioctl_action_set_proc_nice_t;

#endif
