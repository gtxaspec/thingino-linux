/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#ifndef _JZ_AIP_V20_DRV_H_
#define _JZ_AIP_V20_DRV_H_

#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <asm/cacheflush.h>

#include "jz_aip_v20_dbg.h"
#include "jz_aip_v20_ioctl.h"
#include "jz_aip_v20_trace.h"

#define JZ_AIP_V20_DRV_NAME "aip2.0"

#ifdef CONFIG_AIP_V20_REPAIR_A1
	#define JZ_AIP_V20_IOBASE	0x13090000
#else
	#define JZ_AIP_V20_IOBASE	0x12b00000
#endif

#define JZ_AIP_V20_IOSIZE		0x400

#define IRQ_AIP_V20_T			IRQ_AIP0
#define IRQ_AIP_V20_F			IRQ_AIP1
#define IRQ_AIP_V20_P			IRQ_AIP2

#define JZ_NNA_V20_ORAM_GRANULARITY     256

// for T41
#define JZ_NNA_DMA_DESRAM_IOBASE        0x12500000
#define JZ_NNA_DMA_DESRAM_IOSIZE        (16 * 1024)
#define JZ_NNA_DMA_IOBASE               0x12508000
#define JZ_NNA_DMA_IOSIZE               0x20
#define JZ_NNA_ORAM_IOBASE              0x12600000

#define jz_aip_v20_clk_open jz_aip_v20_clk(1)
#define jz_aip_v20_clk_close jz_aip_v20_clk(0)

#define JZ_AIP_V20_SET_JOB_NAME(dat, n) \
	if (JZ_AIP_JOB_NAME_LEN) {\
		memset(exec->name, 0, JZ_AIP_JOB_NAME_LEN);\
		if ((dat)->name != NULL) {\
			if (strncpy_from_user(exec->name, (dat)->name,\
					JZ_AIP_JOB_NAME_LEN - 1) < 0)\
				JZ_AIP_V20_ERROR("Can not get "#n" name.\n");\
		} else {\
			strcpy(exec->name, "nul");\
		}\
	}

enum jz_aip_v20_job_state {
	AIP_JOB_INITIAL,
	AIP_JOB_RUNNING,
	AIP_JOB_FINISH,
};

struct jz_aip_v20_bo_info {
	pid_t                            pid;
	void                            *vaddr;
	dma_addr_t                       dma;
	size_t                           size;
	struct idr                       handles;
};

struct jz_nna_v20_oram {
	pid_t                            pid;
	unsigned long                    kvaddr;
	size_t                           size;
	struct list_head                 head;
};

struct jz_aip_v20_f_exec_info {
	struct list_head                 head;
	pid_t                            pid;
	enum jz_aip_v20_job_state        state;

	ktime_t                          submit_time;
	ktime_t                          start_time;
	ktime_t                          end_time;
	uint32_t                         hw_time;
	char                             name[JZ_AIP_JOB_NAME_LEN];

	uint64_t                         seqno;
	struct jz_aip_v20_bo_info       *node;
	size_t                           node_num;
	size_t                           node_size;
	size_t                           mem_ofst;
	uint32_t                         cfg_reg;
};

struct jz_aip_v20_p_exec_info {
	struct list_head                 head;
	pid_t                            pid;
	enum jz_aip_v20_job_state        state;

	ktime_t                          submit_time;
	ktime_t                          start_time;
	ktime_t                          end_time;
	uint32_t                         hw_time;
	char                             name[JZ_AIP_JOB_NAME_LEN];

	uint64_t                         seqno;
	struct jz_aip_v20_bo_info       *node;
	size_t                           node_num;
	size_t                           node_size;
	size_t                           mem_ofst;
	uint32_t                         cfg_reg;
	uint32_t                         param[9];
};

struct jz_aip_v20_t_exec_info {
	struct list_head                 head;
	pid_t                            pid;
	enum jz_aip_v20_job_state        state;

	ktime_t                          submit_time;
	ktime_t                          start_time;
	ktime_t                          end_time;
	uint32_t                         hw_time;
	char                             name[JZ_AIP_JOB_NAME_LEN];

	uint64_t                         seqno;
	size_t                           node_size;
	uint32_t                         cfg_reg;
	uint32_t                         param[9];
	uint32_t                         offset;
	aip_v20_t_node_t                 node;
};

struct jz_nna_v20_exec_info {
	struct list_head                 head;
	pid_t                            pid;

	ktime_t                          submit_time;
	ktime_t                          start_time;
	char                             name[JZ_AIP_JOB_NAME_LEN];
#ifdef CONFIG_SCHED_INFO
	unsigned long                    sched_pcount;
#endif
};

struct jz_aip_v20_dev {
	void __iomem                    *io_regs;
	int                              f_irq;
	int                              p_irq;
	int                              t_irq;

	struct miscdevice                mdev;

	struct mutex                     struct_mutex;
	struct kref                      ref;
	int                              is_ok;
	struct clk                      *clk_gate;

	// NNA
	uint64_t                         nna_seqno;
	struct list_head                 nna_job_list;
	struct idr                       nna_oram_handles;
	struct mutex                     nna_mutex;
	spinlock_t                       nna_job_lock;
	struct mutex                     nna_oram_lock;
	pid_t                            nna_pid;
	void __iomem                    *virt_oram_base;
	struct gen_pool                 *oram_pool;
	size_t                           total_l2c_size;
	size_t                           curr_l2c_size;
	size_t                           oram_size;
	uint32_t                         oram_pbase;
	size_t                           dma_size;
	uint32_t                         dma_pbase;
	size_t                           dma_desram_size;
	uint32_t                         dma_desram_pbase;

	// BO
	struct mutex                     bo_mutex;
	struct idr                       bo_handles;

	// AIP
	wait_queue_head_t                f_job_wait_queue;
	uint64_t                         f_finished_seqno;
	uint64_t                         f_submited_seqno;
	struct work_struct               f_job_done_work;

	spinlock_t                       f_job_lock;
	struct list_head                 f_job_list;
	struct list_head                 f_job_done_list;

	wait_queue_head_t                p_job_wait_queue;
	uint64_t                         p_finished_seqno;
	uint64_t                         p_submited_seqno;
	struct work_struct               p_job_done_work;

	spinlock_t                       p_job_lock;
	struct list_head                 p_job_list;
	struct list_head                 p_job_done_list;

	wait_queue_head_t                t_job_wait_queue;
	uint64_t                         t_finished_seqno;
	uint64_t                         t_submited_seqno;
	struct work_struct               t_job_fast_work;
	struct work_struct               t_job_done_work;

	spinlock_t                       t_job_lock;
	struct mutex                     t_job_mutex;
	struct list_head                 t_job_list;
	struct list_head                 t_job_done_list;
};

static inline struct jz_aip_v20_dev *
to_jz_aip_v20_dev(struct miscdevice *dev)
{
	return container_of(dev, struct jz_aip_v20_dev, mdev);
}

static inline struct jz_aip_v20_f_exec_info *
jz_aip_v20_first_f_job(struct jz_aip_v20_dev *aip)
{
	return list_first_entry_or_null(&aip->f_job_list,
			struct jz_aip_v20_f_exec_info, head);
}

static inline struct jz_aip_v20_p_exec_info *
jz_aip_v20_first_p_job(struct jz_aip_v20_dev *aip)
{
	return list_first_entry_or_null(&aip->p_job_list,
			struct jz_aip_v20_p_exec_info, head);
}

static inline struct jz_aip_v20_t_exec_info *
jz_aip_v20_first_t_job(struct jz_aip_v20_dev *aip)
{
	return list_first_entry_or_null(&aip->t_job_list,
			struct jz_aip_v20_t_exec_info, head);
}

static inline struct jz_nna_v20_exec_info *
jz_nna_v20_first_job(struct jz_aip_v20_dev *nna)
{
	return list_first_entry_or_null(&nna->nna_job_list,
			struct jz_nna_v20_exec_info, head);
}

static inline void
jz_aip_v20_flush_caches(struct jz_aip_v20_dev *aip)
{
// TODO
#if 0
	flush_cache_mm(current->mm);
#else
	__flush_cache_all();
#endif
}

static inline uint64_t
jz_aip_v20_get_finished_seqno(struct jz_aip_v20_dev *aip, aip_v20_ioctl_ch_op_t op)
{
	if (op == AIP_CH_OP_F) {
		return aip->f_finished_seqno;
	} else if (op == AIP_CH_OP_P) {
		return aip->p_finished_seqno;
	} else if (op == AIP_CH_OP_T) {
		return aip->t_finished_seqno;
	} else {
		JZ_AIP_V20_ERROR("ioctl-wait:%d can not support op!\n", op);
	}
	return -1ll;
}

/* jz_aip_v20_bo.c */
int jz_aip_v20_bo_init(struct jz_aip_v20_dev *aip);
struct jz_aip_v20_bo_info *jz_aip_v20_bo_create(struct jz_aip_v20_dev *aip, size_t size);
int jz_aip_v20_bo(struct jz_aip_v20_dev *aip, unsigned long arg);
int jz_aip_v20_bo_release(struct jz_aip_v20_dev *aip, pid_t pid);

/* jz_aip_v20_core.c */
int jz_aip_v20_f_reset(struct jz_aip_v20_dev *aip);
int jz_aip_v20_p_reset(struct jz_aip_v20_dev *aip);
int jz_aip_v20_t_reset(struct jz_aip_v20_dev *aip);
int jz_aip_v20_core_init(struct jz_aip_v20_dev *aip);
int jz_aip_v20_submit(struct jz_aip_v20_dev *aip, unsigned long arg);
int jz_aip_v20_wait(struct jz_aip_v20_dev *aip, unsigned long arg);
int jz_aip_v20_submit_next_f_job(struct jz_aip_v20_dev *aip);
void jz_aip_v20_f_job_done_work(struct work_struct *work);
int jz_aip_v20_submit_next_p_job(struct jz_aip_v20_dev *aip);
void jz_aip_v20_p_job_done_work(struct work_struct *work);
int jz_aip_v20_submit_next_t_job(struct jz_aip_v20_dev *aip);
void jz_aip_v20_t_job_done_work(struct work_struct *work);

/* jz_aip_v20_irq.c */
int jz_aip_v20_irq_init(struct platform_device *pdev, struct jz_aip_v20_dev *aip);

/* jz_nna_v20_action.c */
int jz_aip_v20_action(struct jz_aip_v20_dev *aip, unsigned long arg);

/* jz_nna_v20_core.c */
int jz_nna_v20_core_init(struct platform_device *pdev, struct jz_aip_v20_dev *nna);
int jz_nna_v20_op(struct jz_aip_v20_dev *aip, unsigned long arg);
int jz_nna_v20_release(struct jz_aip_v20_dev *aip, pid_t pid);

#endif
