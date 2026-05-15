/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>

#include "jz_aip_v20_drv.h"
#include "jz_aip_v20_regs.h"

int jz_aip_v20_core_init(struct jz_aip_v20_dev *aip)
{
	// AIP-f
	init_waitqueue_head(&aip->f_job_wait_queue);
	aip->f_submited_seqno = 0ll;
	aip->f_finished_seqno = 0ll;
	INIT_WORK(&aip->f_job_done_work, jz_aip_v20_f_job_done_work);
	INIT_LIST_HEAD(&aip->f_job_list);
	INIT_LIST_HEAD(&aip->f_job_done_list);
	spin_lock_init(&aip->f_job_lock);

	// AIP-p
	init_waitqueue_head(&aip->p_job_wait_queue);
	aip->p_submited_seqno = 0ll;
	aip->p_finished_seqno = 0ll;
	INIT_WORK(&aip->p_job_done_work, jz_aip_v20_p_job_done_work);
	INIT_LIST_HEAD(&aip->p_job_list);
	INIT_LIST_HEAD(&aip->p_job_done_list);
	spin_lock_init(&aip->p_job_lock);

	// AIP-t
	init_waitqueue_head(&aip->t_job_wait_queue);
	aip->t_submited_seqno = 0ll;
	aip->t_finished_seqno = 0ll;
	INIT_WORK(&aip->t_job_done_work, jz_aip_v20_t_job_done_work);
	INIT_LIST_HEAD(&aip->t_job_list);
	INIT_LIST_HEAD(&aip->t_job_done_list);
	spin_lock_init(&aip->t_job_lock);
	mutex_init(&aip->t_job_mutex);

	return 0;
}

int jz_aip_v20_f_reset(struct jz_aip_v20_dev *aip)
{
	uint32_t ctl;

	while(1) {
		ctl = AIP_V20_READL(AIP_V20_F_CTL);
		if (!(ctl & AIP_V20_F_CTL_BUSY_FIELD) &&
		    !(ctl & AIP_V20_F_CTL_CHAIN_BUSY_FIELD))
			break;
		udelay(50);
	}

	AIP_V20_WRITEL(AIP_V20_F_CTL,
			AIP_V20_F_CTL_SOFT_RESET_FIELD);

	while (AIP_V20_READL(AIP_V20_F_CTL) &
			AIP_V20_F_CTL_SOFT_RESET_FIELD);
	return 0;
}

int jz_aip_v20_p_reset(struct jz_aip_v20_dev *aip)
{
	uint32_t ctl;

	while(1) {
		ctl = AIP_V20_READL(AIP_V20_P_CTL);
		if (!(ctl & AIP_V20_P_CTL_BUSY_FIELD) &&
		    !(ctl & AIP_V20_P_CTL_CHAIN_BUSY_FIELD))
			break;
		udelay(50);
	}

	AIP_V20_WRITEL(AIP_V20_P_CTL,
			AIP_V20_P_CTL_SOFT_RESET_FIELD);

	while (AIP_V20_READL(AIP_V20_P_CTL) &
			AIP_V20_P_CTL_SOFT_RESET_FIELD);
	return 0;
}

int jz_aip_v20_t_reset(struct jz_aip_v20_dev *aip)
{
	uint32_t ctl;

	while(1) {
		ctl = AIP_V20_READL(AIP_V20_T_CTL);
		if (ctl & AIP_V20_T_CTL_AXI_EMPTY_FIELD)
			break;
		udelay(50);
	}

	AIP_V20_WRITEL(AIP_V20_T_CTL,
			AIP_V20_T_CTL_SOFT_RESET_FIELD);

	while (AIP_V20_READL(AIP_V20_T_CTL) &
			AIP_V20_T_CTL_SOFT_RESET_FIELD);
	return 0;
}

int jz_aip_v20_submit_next_f_job(struct jz_aip_v20_dev *aip)
{
	uint32_t f_cfg, f_ctrl;
	struct jz_aip_v20_f_exec_info *exec;

	exec = jz_aip_v20_first_f_job(aip);
	if (exec == NULL)
		return 0;

	jz_aip_v20_flush_caches(aip);

	f_cfg = exec->cfg_reg;
	f_cfg |= AIP_V20_F_CFG_TIMEOUT_ENABLE_FIELD;
	f_cfg |= AIP_V20_F_CFG_IRQ_MASK_FIELD;
	f_cfg |= AIP_V20_F_CFG_CKG_MASK_FIELD;
#ifdef CONFIG_AIP_V20_REPAIR_A1
	f_cfg |= AIP_V20_F_CFG_CHAIN_IRQ_MASK_FIELD;
#endif

	jz_aip_v20_f_reset(aip); // Really need?

	AIP_V20_WRITEL(AIP_V20_F_CFG, f_cfg);
	AIP_V20_WRITEL(AIP_V20_F_CHAIN_BASE, exec->node->dma + exec->mem_ofst);
	AIP_V20_WRITEL(AIP_V20_F_CHAIN_SIZE, exec->node_size);

	f_ctrl = AIP_V20_READL(AIP_V20_F_CTL);
	f_ctrl |= AIP_V20_F_CTL_CHAIN_BUSY_FIELD;
	AIP_V20_WRITEL(AIP_V20_F_CTL, f_ctrl);
	JZ_AIP_V20_DEBUG("AIP f-job[%lld] start, cfg=0x%x, ctrl=0x%x.\n", exec->seqno, f_cfg, f_ctrl);

	exec->start_time = ktime_get_boottime();
	exec->state = AIP_JOB_RUNNING;

	return 0;
}

int jz_aip_v20_submit_next_p_job(struct jz_aip_v20_dev *aip)
{
	uint32_t p_cfg, p_ctrl;
	struct jz_aip_v20_p_exec_info *exec;

	exec = jz_aip_v20_first_p_job(aip);
	if (exec == NULL)
		return 0;

	jz_aip_v20_flush_caches(aip);

	p_cfg = exec->cfg_reg;
	p_cfg |= AIP_V20_P_CFG_TIMEOUT_ENABLE_FIELD;
	p_cfg |= AIP_V20_P_CFG_IRQ_MASK_FIELD;
	p_cfg |= AIP_V20_P_CFG_CKG_MASK_FIELD;
#ifdef CONFIG_AIP_V20_REPAIR_A1
	p_cfg |= AIP_V20_P_CFG_CHAIN_IRQ_MASK_FIELD;
#endif

	jz_aip_v20_p_reset(aip); // Really need?

	AIP_V20_WRITEL(AIP_V20_P_PARAM0, exec->param[0]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM1, exec->param[1]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM2, exec->param[2]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM3, exec->param[3]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM4, -exec->param[4]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM5, -exec->param[5]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM6, exec->param[6]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM7, exec->param[7]);
	AIP_V20_WRITEL(AIP_V20_P_PARAM8, exec->param[8]);

	AIP_V20_WRITEL(AIP_V20_P_CFG, p_cfg);
	AIP_V20_WRITEL(AIP_V20_P_CHAIN_BASE, exec->node->dma + exec->mem_ofst);
	AIP_V20_WRITEL(AIP_V20_P_CHAIN_SIZE, exec->node_size);

	p_ctrl = AIP_V20_READL(AIP_V20_P_CTL);
	p_ctrl |= AIP_V20_P_CTL_CHAIN_BUSY_FIELD;
	AIP_V20_WRITEL(AIP_V20_P_CTL, p_ctrl);
	JZ_AIP_V20_DEBUG("AIP p-job[%lld] start, cfg=0x%x, ctrl=0x%x.\n", exec->seqno, p_cfg, p_ctrl);

	exec->start_time = ktime_get_boottime();
	exec->state = AIP_JOB_RUNNING;

	return 0;
}

int jz_aip_v20_submit_next_t_job(struct jz_aip_v20_dev *aip)
{
	uint32_t t_cfg, t_ctrl;
	struct jz_aip_v20_t_exec_info *exec;

	exec = jz_aip_v20_first_t_job(aip);
	if (exec == NULL)
		return 0;

	jz_aip_v20_flush_caches(aip);

	t_cfg = exec->cfg_reg;
	t_cfg |= AIP_V20_T_CFG_IRQ_SELECT_FIELD;
	t_cfg |= AIP_V20_T_CFG_TIMEOUT_ENABLE_FIELD;
	t_cfg |= AIP_V20_T_CFG_CHAIN_IRQ_MASK_FIELD;
	t_cfg |= AIP_V20_T_CFG_IRQ_MASK_FIELD;
	t_cfg |= AIP_V20_T_CFG_CKG_MASK_FIELD;

	/* T41 AIP BUG: T_T ?????
	 * When the AIP t-channel works in
	 * the tasklen state multiple,
	 * after the task is completed,
	 * the timeout counter will work
	 * until an interrupt is generated and
	 * this interrupt cannot be cleared,
	 * in the end a state with status=0,
	 * but the interrupt still exists.
	 * So mask it temporarity.
	 */
	t_cfg |= AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_FIELD;

	jz_aip_v20_t_reset(aip); // Really need?
	AIP_V20_WRITEL(AIP_V20_T_PARAM0, exec->param[0]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM1, exec->param[1]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM2, exec->param[2]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM3, exec->param[3]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM4, -exec->param[4]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM5, -exec->param[5]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM6, exec->param[6]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM7, exec->param[7]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM8, exec->param[8]);

	AIP_V20_WRITEL(AIP_V20_T_TIMEOUT, 0xffffffff);
	AIP_V20_WRITEL(AIP_V20_T_YBASE_SRC, exec->node.src_ybase);
	AIP_V20_WRITEL(AIP_V20_T_CBASE_SRC, exec->node.src_cbase);
	AIP_V20_WRITEL(AIP_V20_T_BASE_DST, exec->node.dst_base);
	AIP_V20_WRITEL(AIP_V20_T_TASK_LEN, exec->node.task_len);
	AIP_V20_WRITEL(AIP_V20_T_OFST, exec->offset);
	AIP_V20_WRITEL(AIP_V20_T_WH,
			exec->node.src_h << 16 | exec->node.src_w);
	AIP_V20_WRITEL(AIP_V20_T_STRIDE,
			exec->node.dst_stride << 16 | exec->node.src_stride);
	/* AIP_V20_WRITEL(AIP_V20_T_TASK_SKIP_BASE,
	 * 			exec->node.task_skip_base);
	 *
	 * The AIP hardware was unable to run the jump
	 * function properly, so the write operation to
	 * this register was cancelled.
	 */
	AIP_V20_WRITEL(AIP_V20_T_CFG, t_cfg);

	//t_ctrl = AIP_V20_READL(AIP_V20_T_CTL);
	//t_ctrl |= AIP_V20_T_CTL_BUSY_FIELD;
	t_ctrl = AIP_V20_T_CTL_BUSY_FIELD;
	AIP_V20_WRITEL(AIP_V20_T_CTL, t_ctrl);
	t_ctrl = AIP_V20_T_CTL_TASK_FIELD;
	AIP_V20_WRITEL(AIP_V20_T_CTL, t_ctrl);
	exec->start_time = ktime_get_boottime();
	exec->state = AIP_JOB_RUNNING;
	JZ_AIP_V20_DEBUG("AIP t-job[%lld] start, cfg=0x%x, ctrl=0x%x.\n", exec->seqno, t_cfg, t_ctrl);


	return 0;
}


static
void jz_aip_v20_f_complete_exec(struct jz_aip_v20_dev *aip, struct jz_aip_v20_f_exec_info *exec)
{
	kfree(exec);
}

static
void jz_aip_v20_p_complete_exec(struct jz_aip_v20_dev *aip, struct jz_aip_v20_p_exec_info *exec)
{
	kfree(exec);
}

static
void jz_aip_v20_t_complete_exec(struct jz_aip_v20_dev *aip, struct jz_aip_v20_t_exec_info *exec)
{
	kfree(exec);
}

static
int jz_aip_v20_f_op(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	aip_v20_ioctl_submit_f_t dat_f;
	int i, fnode_num;
	unsigned long irqflags;
	struct jz_aip_v20_f_exec_info *exec;
	aip_v20_f_node_t *f_chain;

	if (copy_from_user(&dat_f, (const aip_v20_ioctl_submit_f_t __user *)arg,
				sizeof(aip_v20_ioctl_submit_f_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_f from user space.\n");
		return -EFAULT;
	}

	exec = kcalloc(1, sizeof(*exec), GFP_KERNEL);
	if (!exec) {
		JZ_AIP_V20_ERROR("Can not create AIP-f exec info.\n");
		return -EFAULT;
	}

	exec->pid = task_tgid_vnr(current);
	exec->state = AIP_JOB_INITIAL;

	JZ_AIP_V20_SET_JOB_NAME(&dat_f, aip-f);
	exec->submit_time = ktime_get_boottime();
	exec->cfg_reg  = (dat_f.format  & AIP_V20_F_CFG_BW_MASK  )
		<< AIP_V20_F_CFG_BW_SHIFT;
	exec->cfg_reg |= (dat_f.pos_in  & AIP_V20_F_CFG_IBUS_MASK)
		<< AIP_V20_F_CFG_IBUS_SHIFT;
	exec->cfg_reg |= (dat_f.pos_out & AIP_V20_F_CFG_OBUS_MASK)
		<< AIP_V20_F_CFG_OBUS_SHIFT;

	exec->node_num = dat_f.node_num;
	exec->node_size = exec->node_num * sizeof(aip_v20_f_node_t);
	exec->node = idr_find(&aip->bo_handles, dat_f.node_handle);
	if (exec->node == NULL) {
		JZ_AIP_V20_ERROR("Can not find node bo.\n");
		goto fail;
	}
	exec->mem_ofst = dat_f.paddr - exec->node->dma;

#ifdef CONFIG_AIP_V20_REPAIR_A1
	fnode_num = dat_f.node_num - 1;
#else
	fnode_num = dat_f.node_num;
#endif

	f_chain = (aip_v20_f_node_t *)(exec->node->vaddr + exec->mem_ofst);
	for (i = 0; i < fnode_num; i++) {
   		f_chain[i].timeout[1]      = JZ_AIP_V20_IOBASE + AIP_V20_F_TIME_CNT;
		f_chain[i].scale_x[1]      = JZ_AIP_V20_IOBASE + AIP_V20_F_SCALEX;
		f_chain[i].scale_y[1]      = JZ_AIP_V20_IOBASE + AIP_V20_F_SCALEY;
		f_chain[i].trans_x[1]      = JZ_AIP_V20_IOBASE + AIP_V20_F_TRANSX;
		f_chain[i].trans_y[1]      = JZ_AIP_V20_IOBASE + AIP_V20_F_TRANSY;
		f_chain[i].src_base_y[1]   = JZ_AIP_V20_IOBASE + AIP_V20_F_SRC_BASE;
		f_chain[i].src_base_uv[1]  = JZ_AIP_V20_IOBASE + AIP_V20_F_SRC_BASE_C;
		f_chain[i].dst_base_y[1]   = JZ_AIP_V20_IOBASE + AIP_V20_F_DST_BASE;
		f_chain[i].dst_base_uv[1]  = JZ_AIP_V20_IOBASE + AIP_V20_F_DST_BASE_C;
		f_chain[i].src_size[1]     = JZ_AIP_V20_IOBASE + AIP_V20_F_SRC_SIZE;
		f_chain[i].dst_size[1]     = JZ_AIP_V20_IOBASE + AIP_V20_F_DST_SIZE;
		f_chain[i].stride[1]       = JZ_AIP_V20_IOBASE + AIP_V20_F_STRD;
		f_chain[i].f_ctrl[0]       = 0x1;
		f_chain[i].f_ctrl[1]       = (JZ_AIP_V20_IOBASE + AIP_V20_F_CTL) | (1 << 31);
	}
#ifdef CONFIG_AIP_V20_REPAIR_A1
	f_chain[i].timeout[0]      = 0x2;
	f_chain[i].timeout[1]      = JZ_AIP_V20_IOBASE + AIP_V20_F_TIME_CNT;
	f_chain[i].f_ctrl[0]       = 0x1;
	f_chain[i].f_ctrl[1]       = (JZ_AIP_V20_IOBASE + AIP_V20_F_CTL) | (1 << 31);
#endif

	spin_lock_irqsave(&aip->f_job_lock, irqflags);

	aip->f_submited_seqno++;
	exec->seqno = aip->f_submited_seqno;

	list_add_tail(&exec->head, &aip->f_job_list);
	if (jz_aip_v20_first_f_job(aip) == exec) {
		jz_aip_v20_submit_next_f_job(aip);
	} else {
		JZ_AIP_V20_DEBUG("Having job running, seq[%lld] is delayed.\n", exec->seqno);
	}

	spin_unlock_irqrestore(&aip->f_job_lock, irqflags);

	dat_f.seqno = aip->f_submited_seqno;
	if (copy_to_user((aip_v20_ioctl_submit_f_t __user *)arg, &dat_f,
				sizeof(aip_v20_ioctl_submit_f_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_f to user space.\n");
		return -EFAULT;
	}

	return 0;

fail:
	jz_aip_v20_f_complete_exec(aip, exec);
	return -EFAULT;
}

static
int jz_aip_v20_p_op(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	aip_v20_ioctl_submit_p_t dat_p;
	int i, pnode_num;
	unsigned long irqflags;
	struct jz_aip_v20_p_exec_info *exec;
	aip_v20_p_node_t *p_chain;
	uint32_t p_offset = 0;

	if (copy_from_user(&dat_p, (const aip_v20_ioctl_submit_p_t __user *)arg,
				sizeof(aip_v20_ioctl_submit_p_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_p from user space.\n");
		return -EFAULT;
	}

	exec = kcalloc(1, sizeof(*exec), GFP_KERNEL);
	if (!exec) {
		JZ_AIP_V20_ERROR("Can not create AIP-p exec info.\n");
		return -EFAULT;
	}

	exec->pid = task_tgid_vnr(current);
	exec->state = AIP_JOB_INITIAL;

	JZ_AIP_V20_SET_JOB_NAME(&dat_p, aip-p);
	exec->submit_time = ktime_get_boottime();
	exec->cfg_reg |= (dat_p.pos_in  & AIP_V20_P_CFG_IBUS_MASK)
		<< AIP_V20_P_CFG_IBUS_SHIFT;
	exec->cfg_reg |= (dat_p.pos_out & AIP_V20_P_CFG_OBUS_MASK)
		<< AIP_V20_P_CFG_OBUS_SHIFT;

	if (copy_from_user(exec->param, dat_p.param, 9 * sizeof(uint32_t)) != 0) {
		JZ_AIP_V20_ERROR("Can not get p_param.\n");
		return -EFAULT;
	}

	exec->node_num = dat_p.node_num;
	exec->node_size = exec->node_num * sizeof(aip_v20_p_node_t);
	exec->node = idr_find(&aip->bo_handles, dat_p.node_handle);
	if (exec->node < 0) {
		JZ_AIP_V20_ERROR("Can not find node bo.\n");
		goto fail;
	}
	exec->mem_ofst = dat_p.paddr - exec->node->dma;

	p_offset = (dat_p.offset.order & AIP_V20_P_OFST_NV2BGR_ORDER_MASK)
		<< AIP_V20_P_OFST_NV2BGR_ORDER_SHIFT;
	p_offset |= (dat_p.offset.alpha & AIP_V20_P_OFST_NV2BGR_ALPHA_MASK)
		<< AIP_V20_P_OFST_NV2BGR_ALPHA_SHIFT;
	p_offset |= (dat_p.offset.offset[1] & AIP_V20_P_OFST_NV2BGR_OFSET1_MASK)
		<< AIP_V20_P_OFST_NV2BGR_OFSET1_SHIFT;
	p_offset |= (dat_p.offset.offset[0] & AIP_V20_P_OFST_NV2BGR_OFSET0_MASK)
		<< AIP_V20_P_OFST_NV2BGR_OFSET0_SHIFT;


#ifdef CONFIG_AIP_V20_REPAIR_A1
	pnode_num = dat_p.node_num - 1;
#else
	pnode_num = dat_p.node_num;
#endif


	p_chain = (aip_v20_p_node_t *)(exec->node->vaddr + exec->mem_ofst);
	for (i = 0; i < pnode_num; i++) {
		p_chain[i].timeout[1]      = JZ_AIP_V20_IOBASE + AIP_V20_P_TIME_CNT;
		p_chain[i].mode[0]         = dat_p.mode;
		p_chain[i].mode[1]         = JZ_AIP_V20_IOBASE + AIP_V20_P_MODE;
		p_chain[i].src_base_y[1]   = JZ_AIP_V20_IOBASE + AIP_V20_P_SRC_YBASE;
		p_chain[i].src_base_uv[1]  = JZ_AIP_V20_IOBASE + AIP_V20_P_SRC_CBASE;
		p_chain[i].src_stride[1]   = JZ_AIP_V20_IOBASE + AIP_V20_P_SRC_STRIDE;
		p_chain[i].dst_base[1]     = JZ_AIP_V20_IOBASE + AIP_V20_P_DST_BASE;
		p_chain[i].dst_stride[1]   = JZ_AIP_V20_IOBASE + AIP_V20_P_DST_STRIDE;
		p_chain[i].dst_size[1]     = JZ_AIP_V20_IOBASE + AIP_V20_P_DWH;
		p_chain[i].src_size[1]     = JZ_AIP_V20_IOBASE + AIP_V20_P_SWH;
		p_chain[i].dummy_val[1]    = JZ_AIP_V20_IOBASE + AIP_V20_P_DUMMT_VAL;
		p_chain[i].coef0[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF0;
		p_chain[i].coef1[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF1;
		p_chain[i].coef2[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF2;
		p_chain[i].coef3[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF3;
		p_chain[i].coef4[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF4;
		p_chain[i].coef5[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF5;
		p_chain[i].coef6[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF6;
		p_chain[i].coef7[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF7;
		p_chain[i].coef8[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF8;
		p_chain[i].coef9[1]        = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF9;
		p_chain[i].coef10[1]       = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF10;
		p_chain[i].coef11[1]       = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF11;
		p_chain[i].coef12[1]       = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF12;
		p_chain[i].coef13[1]       = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF13;
		p_chain[i].coef14[1]       = JZ_AIP_V20_IOBASE + AIP_V20_P_COEF14;
		p_chain[i].offset[0]       = p_offset;
		p_chain[i].offset[1]       = JZ_AIP_V20_IOBASE + AIP_V20_P_OFST;
		p_chain[i].p_ctrl[0]       = 0x1;
		p_chain[i].p_ctrl[1]       = (JZ_AIP_V20_IOBASE + AIP_V20_P_CTL) | (1 << 31);
	}
#ifdef CONFIG_AIP_V20_REPAIR_A1
	p_chain[i].timeout[0]      = 0x2;
	p_chain[i].timeout[1]      = JZ_AIP_V20_IOBASE + AIP_V20_P_TIME_CNT;
	p_chain[i].p_ctrl[0]       = 0x1;
	p_chain[i].p_ctrl[1]       = (JZ_AIP_V20_IOBASE + AIP_V20_P_CTL) | (1 << 31);
#endif

	spin_lock_irqsave(&aip->p_job_lock, irqflags);

	aip->p_submited_seqno++;
	exec->seqno = aip->p_submited_seqno;

	list_add_tail(&exec->head, &aip->p_job_list);
	if (jz_aip_v20_first_p_job(aip) == exec) {
		jz_aip_v20_submit_next_p_job(aip);
	} else {
		JZ_AIP_V20_DEBUG("Having job running.\n");
	}

	spin_unlock_irqrestore(&aip->p_job_lock, irqflags);

	dat_p.seqno = aip->p_submited_seqno;
	if (copy_to_user((aip_v20_ioctl_submit_p_t __user *)arg, &dat_p,
				sizeof(aip_v20_ioctl_submit_p_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_p to user space.\n");
		return -EFAULT;
	}

	return 0;

fail:
	jz_aip_v20_p_complete_exec(aip, exec);
	return -EFAULT;
}


static
int jz_aip_v20_t_op(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	aip_v20_ioctl_submit_t_t dat_t;
	unsigned long irqflags;
	struct jz_aip_v20_t_exec_info *exec;
	uint32_t t_offset = 0;

	if (copy_from_user(&dat_t, (const aip_v20_ioctl_submit_t_t __user *)arg,
				sizeof(aip_v20_ioctl_submit_t_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_t from user space.\n");
		return -EFAULT;
	}

	exec = kcalloc(1, sizeof(*exec), GFP_KERNEL);
	if (!exec) {
		JZ_AIP_V20_ERROR("Can not create exec info.\n");
		return -EFAULT;
	}

	exec->pid = task_tgid_vnr(current);
	exec->state = AIP_JOB_INITIAL;

	JZ_AIP_V20_SET_JOB_NAME(&dat_t, aip-t);
	exec->submit_time = ktime_get_boottime();
	exec->cfg_reg |= (dat_t.pos_in & AIP_V20_T_CFG_IBUS_MASK)
		<< AIP_V20_T_CFG_IBUS_SHIFT;
	exec->cfg_reg |= (dat_t.pos_out & AIP_V20_T_CFG_OBUS_MASK)
		<< AIP_V20_T_CFG_OBUS_SHIFT;

	if (copy_from_user(exec->param, dat_t.param, 9 * sizeof(uint32_t)) != 0) {
		JZ_AIP_V20_ERROR("Can not get t_param.\n");
		goto fail;
	}

	exec->node_size = sizeof(aip_v20_t_node_t);
	if (copy_from_user(&exec->node, dat_t.node, exec->node_size) != 0) {
		JZ_AIP_V20_ERROR("Can not get t_node.\n");
		goto fail;
	}

	t_offset = (dat_t.offset.order & AIP_V20_T_OFST_NV2BGR_ORDER_MASK)
		<< AIP_V20_T_OFST_NV2BGR_ORDER_SHIFT;
	t_offset |= (dat_t.offset.alpha & AIP_V20_T_OFST_NV2BGR_ALPHA_MASK)
		<< AIP_V20_T_OFST_NV2BGR_ALPHA_SHIFT;
	t_offset |= (dat_t.offset.offset[1] & AIP_V20_T_OFST_NV2BGR_OFSET1_MASK)
		<< AIP_V20_T_OFST_NV2BGR_OFSET1_SHIFT;
	t_offset |= (dat_t.offset.offset[0] & AIP_V20_T_OFST_NV2BGR_OFSET0_MASK)
		<< AIP_V20_T_OFST_NV2BGR_OFSET0_SHIFT;
	exec->offset = t_offset;

	spin_lock_irqsave(&aip->t_job_lock, irqflags);
	aip->t_submited_seqno++;
	exec->seqno = aip->t_submited_seqno;

	list_add_tail(&exec->head, &aip->t_job_list);
	if (jz_aip_v20_first_t_job(aip) == exec) {
		jz_aip_v20_submit_next_t_job(aip);
	} else {
		JZ_AIP_V20_DEBUG("Having job running.\n");
	}

	spin_unlock_irqrestore(&aip->t_job_lock, irqflags);

	dat_t.seqno = aip->t_submited_seqno;
	if (copy_to_user((void __user *)arg, &dat_t,
				sizeof(aip_v20_ioctl_submit_t_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_t to user space.\n");
		return -EFAULT;
	}

	return 0;

fail:
	jz_aip_v20_t_complete_exec(aip, exec);
	return -EFAULT;
}

static
int jz_aip_v20_t_task_op(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	aip_v20_ioctl_submit_t_t dat_t;
	struct jz_aip_v20_t_exec_info *exec;
	uint32_t t_offset = 0;
	uint32_t t_cfg, t_ctrl;

	if (copy_from_user(&dat_t, (const aip_v20_ioctl_submit_t_t __user *)arg,
				sizeof(aip_v20_ioctl_submit_t_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_t from user space.\n");
		return -EFAULT;
	}

	exec = kcalloc(1, sizeof(*exec), GFP_KERNEL);
	if (!exec) {
		JZ_AIP_V20_ERROR("Can not create exec info.\n");
		return -EFAULT;
	}

	exec->submit_time = ktime_get_boottime();
	exec->pid = task_tgid_vnr(current);
	exec->state = AIP_JOB_INITIAL;

	JZ_AIP_V20_SET_JOB_NAME(&dat_t, aip-t);
	exec->cfg_reg |= (dat_t.pos_in  & AIP_V20_T_CFG_IBUS_MASK)
		<< AIP_V20_T_CFG_IBUS_SHIFT;
	exec->cfg_reg |= (dat_t.pos_out & AIP_V20_T_CFG_OBUS_MASK)
		<< AIP_V20_T_CFG_OBUS_SHIFT;

	if (copy_from_user(exec->param, dat_t.param, 9 * sizeof(uint32_t)) != 0) {
		JZ_AIP_V20_ERROR("Can not get t_param.\n");
		goto fail;
	}

	exec->node_size = sizeof(aip_v20_t_node_t);
	if (copy_from_user(&exec->node, dat_t.node, exec->node_size) != 0) {
		JZ_AIP_V20_ERROR("Can not get t_node.\n");
		goto fail;
	}

	t_offset = (dat_t.offset.order & AIP_V20_T_OFST_NV2BGR_ORDER_MASK)
		<< AIP_V20_T_OFST_NV2BGR_ORDER_SHIFT;
	t_offset |= (dat_t.offset.alpha & AIP_V20_T_OFST_NV2BGR_ALPHA_MASK)
		<< AIP_V20_T_OFST_NV2BGR_ALPHA_SHIFT;
	t_offset |= (dat_t.offset.offset[1] & AIP_V20_T_OFST_NV2BGR_OFSET1_MASK)
		<< AIP_V20_T_OFST_NV2BGR_OFSET1_SHIFT;
	t_offset |= (dat_t.offset.offset[0] & AIP_V20_T_OFST_NV2BGR_OFSET0_MASK)
		<< AIP_V20_T_OFST_NV2BGR_OFSET0_SHIFT;
	exec->offset = t_offset;

	mutex_lock(&aip->t_job_mutex);
	aip->t_submited_seqno++;
	exec->seqno = aip->t_submited_seqno;

	while(!list_empty(&aip->t_job_list)) {
		udelay(10);
	}
	list_add_tail(&exec->head, &aip->t_job_list);

	jz_aip_v20_flush_caches(aip);

	jz_aip_v20_t_reset(aip);

	t_cfg = exec->cfg_reg;
	t_cfg |= AIP_V20_T_CFG_IRQ_SELECT_FIELD;
	t_cfg |= AIP_V20_T_CFG_TIMEOUT_ENABLE_FIELD;
	t_cfg |= AIP_V20_T_CFG_CHAIN_IRQ_MASK_FIELD;
	t_cfg |= AIP_V20_T_CFG_IRQ_MASK_FIELD;
	t_cfg |= AIP_V20_T_CFG_CKG_MASK_FIELD;
	t_cfg |= AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_FIELD;
	t_cfg |= AIP_V20_T_CFG_TASK_IRQ_FIELD;
	/* Blocking the T module of AIP caused the interruption,
	 * in order to make AIP run faster at the user layer.
	 */

	AIP_V20_WRITEL(AIP_V20_T_PARAM0, exec->param[0]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM1, exec->param[1]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM2, exec->param[2]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM3, exec->param[3]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM4, -exec->param[4]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM5, -exec->param[5]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM6, exec->param[6]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM7, exec->param[7]);
	AIP_V20_WRITEL(AIP_V20_T_PARAM8, exec->param[8]);

	AIP_V20_WRITEL(AIP_V20_T_TIMEOUT, 0xffffffff);
	AIP_V20_WRITEL(AIP_V20_T_YBASE_SRC, exec->node.src_ybase);
	AIP_V20_WRITEL(AIP_V20_T_CBASE_SRC, exec->node.src_cbase);
	AIP_V20_WRITEL(AIP_V20_T_OFST, exec->offset);
	AIP_V20_WRITEL(AIP_V20_T_WH,
			exec->node.src_h << 16 | exec->node.src_w);
	AIP_V20_WRITEL(AIP_V20_T_STRIDE,
			exec->node.dst_stride << 16 | exec->node.src_stride);

	AIP_V20_WRITEL(AIP_V20_T_CFG, t_cfg);
	t_ctrl = AIP_V20_T_CTL_BUSY_FIELD;
	AIP_V20_WRITEL(AIP_V20_T_CTL, t_ctrl);
	exec->start_time = ktime_get_boottime();
	exec->state = AIP_JOB_RUNNING;

	dat_t.seqno = aip->t_submited_seqno;
	if (copy_to_user((aip_v20_ioctl_submit_t_t __user *)arg, &dat_t,
				sizeof(aip_v20_ioctl_submit_t_t))) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy submit_t to user space.\n");
		return -EFAULT;
	}

	return 0;

fail:
	jz_aip_v20_t_complete_exec(aip, exec);
	return -EFAULT;
}


static void jz_aip_v20_f_job_handle_completed(struct jz_aip_v20_dev *aip)
{
	unsigned long irqflags;
	int64_t total_time, run_time;

	spin_lock_irqsave(&aip->f_job_lock, irqflags);

	while (!list_empty(&aip->f_job_done_list)) {
		struct jz_aip_v20_f_exec_info *exec =
			list_first_entry(&aip->f_job_done_list,
					struct jz_aip_v20_f_exec_info, head);
		list_del(&exec->head);

		spin_unlock_irqrestore(&aip->f_job_lock, irqflags);
		total_time = ktime_us_delta(exec->end_time, exec->submit_time);
		run_time = ktime_us_delta(exec->end_time, exec->start_time);
		trace_aip_v20_job(
				"aip-f",
				exec->name,
				exec->seqno,
				total_time,
				run_time,
				exec->hw_time);
		jz_aip_v20_f_complete_exec(aip, exec);
		spin_lock_irqsave(&aip->f_job_lock, irqflags);
	}

	spin_unlock_irqrestore(&aip->f_job_lock, irqflags);
}

static void jz_aip_v20_p_job_handle_completed(struct jz_aip_v20_dev *aip)
{
	unsigned long irqflags;
	int64_t total_time, run_time;

	spin_lock_irqsave(&aip->p_job_lock, irqflags);

	while (!list_empty(&aip->p_job_done_list)) {
		struct jz_aip_v20_p_exec_info *exec =
			list_first_entry(&aip->p_job_done_list,
					struct jz_aip_v20_p_exec_info, head);
		list_del(&exec->head);

		spin_unlock_irqrestore(&aip->p_job_lock, irqflags);
		total_time = ktime_us_delta(exec->end_time, exec->submit_time);
		run_time = ktime_us_delta(exec->end_time, exec->start_time);
		trace_aip_v20_job(
				"aip-p",
				exec->name,
				exec->seqno,
				total_time,
				run_time,
				exec->hw_time);
		jz_aip_v20_p_complete_exec(aip, exec);
		spin_lock_irqsave(&aip->p_job_lock, irqflags);
	}

	spin_unlock_irqrestore(&aip->p_job_lock, irqflags);
}

static void jz_aip_v20_t_job_handle_completed(struct jz_aip_v20_dev *aip)
{
	unsigned long irqflags;
	int64_t total_time, run_time;

	spin_lock_irqsave(&aip->t_job_lock, irqflags);
	while (!list_empty(&aip->t_job_done_list)) {
		struct jz_aip_v20_t_exec_info *exec =
			list_first_entry(&aip->t_job_done_list,
			struct jz_aip_v20_t_exec_info, head);
		list_del(&exec->head);

		spin_unlock_irqrestore(&aip->t_job_lock, irqflags);
		total_time = ktime_us_delta(exec->end_time, exec->submit_time);
		run_time = ktime_us_delta(exec->end_time, exec->start_time);
		trace_aip_v20_job(
				"aip-t",
				exec->name,
				exec->seqno,
				total_time,
				run_time,
				exec->hw_time);
		jz_aip_v20_t_complete_exec(aip, exec);
		spin_lock_irqsave(&aip->t_job_lock, irqflags);
	}
	spin_unlock_irqrestore(&aip->t_job_lock, irqflags);
}


void jz_aip_v20_f_job_done_work(struct work_struct *work)
{
	struct jz_aip_v20_dev *aip =
		container_of(work, struct jz_aip_v20_dev, f_job_done_work);

	jz_aip_v20_f_job_handle_completed(aip);
}

void jz_aip_v20_p_job_done_work(struct work_struct *work)
{
	struct jz_aip_v20_dev *aip =
		container_of(work, struct jz_aip_v20_dev, p_job_done_work);

	jz_aip_v20_p_job_handle_completed(aip);
}

void jz_aip_v20_t_job_done_work(struct work_struct *work)
{
	struct jz_aip_v20_dev *aip =
		container_of(work, struct jz_aip_v20_dev, t_job_done_work);

	jz_aip_v20_t_job_handle_completed(aip);
}


int jz_aip_v20_submit(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	int ret = 0;
	uint32_t op;

	ret = copy_from_user(&op, (__user uint32_t *)arg, sizeof(uint32_t));
	if (ret != 0) {
		JZ_AIP_V20_ERROR("ioctl-submit:Failed to copy op from user space, ret = %d\n", ret);
		return -EFAULT;
	}

	switch (op) {
		case AIP_CH_OP_F:
			ret = jz_aip_v20_f_op(aip, arg);
			break;
		case AIP_CH_OP_P:
			ret = jz_aip_v20_p_op(aip, arg);
			break;
		case AIP_CH_OP_T:
			ret = jz_aip_v20_t_op(aip, arg);
			break;
		case AIP_CH_OP_T_TASK:
			ret = jz_aip_v20_t_task_op(aip, arg);
			break;
		default:
			JZ_AIP_V20_ERROR("ioctl-submit:%d can not support op!\n", op);
	}

	return ret;
}

static int jz_aip_v20_wait_for_seqno(struct jz_aip_v20_dev *aip,
		aip_v20_ioctl_ch_op_t op,
		uint64_t seqno,
		uint64_t timeout_ns,
		bool interruptible) {
	int ret = 0;
	uint64_t finished_seqno;
	uint64_t submited_seqno;
	wait_queue_head_t *job_wait_queue;
	unsigned long timeout_expire;
	unsigned long timeout_jiffies;
	DEFINE_WAIT(wait);

	if (op == AIP_CH_OP_F) {
		submited_seqno = aip->f_submited_seqno;
		job_wait_queue = &aip->f_job_wait_queue;
	} else if (op == AIP_CH_OP_P) {
		submited_seqno = aip->p_submited_seqno;
		job_wait_queue = &aip->p_job_wait_queue;
	} else if (op == AIP_CH_OP_T) {
		submited_seqno = aip->t_submited_seqno;
		job_wait_queue = &aip->t_job_wait_queue;
	} else {
		JZ_AIP_V20_ERROR("ioctl-wait_for_seqno:%d can not support op!\n", op);
		return -1;
	}

	if (seqno > submited_seqno) {
		JZ_AIP_V20_ERROR("ioctl-wait_for_seqno:seqno:%lld overflow!\n", seqno);
		return -1;
	}

	finished_seqno = jz_aip_v20_get_finished_seqno(aip, op);
	if (seqno <= finished_seqno) {
		JZ_AIP_V20_DEBUG("Finished_seqno >= seqno %lld %lld\n",
				finished_seqno, seqno);
		return 0;
	}

//	if (timeout_ns == 0)
//		return -ETIME;

	timeout_jiffies = nsecs_to_jiffies(timeout_ns);
	if (timeout_jiffies < 2)
		timeout_jiffies = 2;
	timeout_expire = jiffies + timeout_jiffies;

	for (;;) {
		prepare_to_wait(job_wait_queue, &wait,
				interruptible ? TASK_INTERRUPTIBLE :
				TASK_UNINTERRUPTIBLE);
		if (interruptible && signal_pending(current)) {
			JZ_AIP_V20_DEBUG("ERESTARTSY:Waiting for forced termination.\n");
			ret = -ERESTARTSYS;
			break;
		}

		finished_seqno = jz_aip_v20_get_finished_seqno(aip, op);
		if (finished_seqno >= seqno)
	                break;

//		if (timeout_ns != ~0ull) {
//			if (time_after_eq(jiffies, timeout_expire)) {
//				ret = -ETIME;
//				break;
//			}
//			schedule_timeout(timeout_jiffies);
//		} else {
//			schedule();
//		}

		if (timeout_ns == ULLONG_MAX) {
			JZ_AIP_V20_DEBUG("schedule...\n");
			schedule();
		} else {
			JZ_AIP_V20_DEBUG("schedule_timeout...\n");
			if (time_after_eq(jiffies, timeout_expire)) {
				JZ_AIP_V20_ERROR("Waiting timeout.\n");
				ret = -ETIME;
				break;
			}
			schedule_timeout(timeout_jiffies);
		}
	}

	JZ_AIP_V20_DEBUG("finish job_wait_queue, timeout_jiffies=%ld ret=%d.\n",
			timeout_jiffies, ret);
	finish_wait(job_wait_queue, &wait);

	return ret;
}

int jz_aip_v20_wait(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	aip_v20_ioctl_wait_t dat;
	int ret;
	struct jz_aip_v20_t_exec_info *exec;
	int64_t total_time, run_time;
	ktime_t start_wait = ktime_get_boottime();
	unsigned long start = jiffies;
	u64 delta;

	if (copy_from_user(&dat, (aip_v20_ioctl_wait_t *)arg,
				sizeof(aip_v20_ioctl_wait_t)) != 0) {
		JZ_AIP_V20_ERROR("ioctl-wait:Failed to copy wait_t from user space\n");
		return -EFAULT;
	}

	if (dat.op == AIP_CH_OP_T_TASK) {
		exec = jz_aip_v20_first_t_job(aip);
		AIP_V20_WRITEL(AIP_V20_T_CTL,
				AIP_V20_T_CTL_SOFT_RESET_FIELD);
		total_time = ktime_us_delta(exec->end_time, exec->submit_time);
		run_time = ktime_us_delta(exec->end_time, exec->start_time);

		trace_aip_v20_job("aip-t", exec->name,
				exec->seqno, total_time,
				run_time, exec->hw_time);
		list_del(&exec->head);
		kfree(exec);
		mutex_unlock(&aip->t_job_mutex);
		JZ_AIP_V20_DEBUG("AIP t-reg successfully closed\n");
		ret = 0;
	} else {
		ret = jz_aip_v20_wait_for_seqno(aip,
				dat.op,
				dat.seqno,
				dat.timeout_ns, true);
//		if ((ret == -EINTR || ret == -ERESTARTSYS) && dat.timeout_ns != ~0ull) {
		if ((ret == -EINTR || ret == -ERESTARTSYS) && dat.timeout_ns != ULLONG_MAX) {
			delta = jiffies_to_nsecs(jiffies - start);
			if (dat.timeout_ns >= delta)
				dat.timeout_ns -= delta;
		}

		dat.wait_time = ktime_us_delta(ktime_get_boottime(), start_wait);

		if (copy_to_user((aip_v20_ioctl_wait_t __user *)arg, &dat,
					sizeof(aip_v20_ioctl_wait_t)) != 0) {
			JZ_AIP_V20_ERROR("ioctl-wait:Failed to copy wait_t to user space\n");
			return -EFAULT;
		}
		JZ_AIP_V20_DEBUG("seq[%lld] wait time is %lldus\n", dat.seqno, dat.wait_time);
	}
	return ret;
}
