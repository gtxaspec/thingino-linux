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
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/ktime.h>

#include "jz_aip_v20_drv.h"
#include "jz_aip_v20_regs.h"

static int jz_aip_v20_f_irq_finish_job(struct jz_aip_v20_dev *aip, ktime_t now)
{
	struct jz_aip_v20_f_exec_info *exec = jz_aip_v20_first_f_job(aip);
	uint32_t hw_timecnt = AIP_V20_READL(AIP_V20_F_TIME_CNT);

	// current job has been cancel
	if (!exec) {
		return 0;
	}
	if (exec->state != AIP_JOB_RUNNING) {
		jz_aip_v20_submit_next_f_job(aip);
		return 0;
	}

	exec->end_time = now;
	exec->hw_time = 0xffffffff - hw_timecnt;
	aip->f_finished_seqno = exec->seqno;
	JZ_AIP_V20_DEBUG("AIP f-job[%lld] finished.\n", aip->f_finished_seqno);

	list_move_tail(&exec->head, &aip->f_job_done_list);
	jz_aip_v20_submit_next_f_job(aip);

	wake_up_all(&aip->f_job_wait_queue);
	schedule_work(&aip->f_job_done_work);

	return 0;
}

static int jz_aip_v20_p_irq_finish_job(struct jz_aip_v20_dev *aip, ktime_t now)
{
	struct jz_aip_v20_p_exec_info *exec = jz_aip_v20_first_p_job(aip);
	uint32_t hw_timecnt = AIP_V20_READL(AIP_V20_P_TIME_CNT);

	// current job has been cancel
	if (!exec) {
		return 0;
	}
	if (exec->state != AIP_JOB_RUNNING) {
		jz_aip_v20_submit_next_p_job(aip);
		return 0;
	}

	exec->end_time = now;
	exec->hw_time = 0xffffffff - hw_timecnt;
	aip->p_finished_seqno = exec->seqno;
	JZ_AIP_V20_DEBUG("AIP p-job[%lld] finished.\n", aip->p_finished_seqno);

	list_move_tail(&exec->head, &aip->p_job_done_list);
	jz_aip_v20_submit_next_p_job(aip);

	wake_up_all(&aip->p_job_wait_queue);
	schedule_work(&aip->p_job_done_work);

	return 0;
}

static int jz_aip_v20_t_irq_finish_job(struct jz_aip_v20_dev *aip, ktime_t now)
{
	struct jz_aip_v20_t_exec_info *exec = jz_aip_v20_first_t_job(aip);
	uint32_t hw_timecnt = AIP_V20_READL(AIP_V20_T_TIMEOUT);

	// current job has been cancel
	if (!exec) {
		return 0;
	}

	exec->end_time = now;
	exec->hw_time = 0xffffffff - hw_timecnt;
	aip->t_finished_seqno = exec->seqno;
	JZ_AIP_V20_DEBUG("AIP t-job[%lld] finished.\n", aip->t_finished_seqno);

	list_move_tail(&exec->head, &aip->t_job_done_list);
	jz_aip_v20_submit_next_t_job(aip);

	schedule_work(&aip->t_job_done_work);
	wake_up_all(&aip->t_job_wait_queue);

	return 0;
}


static irqreturn_t
jz_aip_v20_f_irq_handler(int irq, void *data)
{
	ktime_t now = ktime_get_boottime();
	struct jz_aip_v20_dev *aip = (struct jz_aip_v20_dev *)data;
	irqreturn_t status = IRQ_NONE;
	uint32_t irq_status = AIP_V20_READL(AIP_V20_F_IRQ);
	AIP_V20_WRITEL(AIP_V20_F_IRQ, irq_status);

	JZ_AIP_V20_DEBUG("AIP f-job done, irq=%d status=0x%02x.\n", irq, irq_status);

	if (irq_status & AIP_V20_F_IRQ_IRQ_FIELD) {
		JZ_AIP_V20_DEBUG("AIP f-job irq.\n");
	}
	if (irq_status & AIP_V20_F_IRQ_CHAIN_IRQ_FIELD) {
		JZ_AIP_V20_DEBUG("AIP f-job chain irq.\n");
		spin_lock(&aip->f_job_lock);
		jz_aip_v20_f_irq_finish_job(aip, now);
		spin_unlock(&aip->f_job_lock);
	}
	if (irq_status & AIP_V20_F_IRQ_TIMEOUT_IRQ_FIELD) {
#ifdef CONFIG_AIP_V20_REPAIR_A1
		JZ_AIP_V20_DEBUG("Response to AIP f-job irq on A1.\n");
		spin_lock(&aip->f_job_lock);
		jz_aip_v20_f_irq_finish_job(aip, now);
		spin_unlock(&aip->f_job_lock);
#else
		JZ_AIP_V20_DEBUG("AIP f-job timeout irq.\n");
#endif
	}

	status = IRQ_HANDLED;
	return status;
}

static irqreturn_t
jz_aip_v20_p_irq_handler(int irq, void *data)
{
	ktime_t now = ktime_get_boottime();
	struct jz_aip_v20_dev *aip = (struct jz_aip_v20_dev *)data;
	irqreturn_t status = IRQ_NONE;
	uint32_t irq_status = AIP_V20_READL(AIP_V20_P_IRQ);
	AIP_V20_WRITEL(AIP_V20_P_IRQ, irq_status);

	JZ_AIP_V20_DEBUG("AIP p-job done, irq=%d status=0x%02x.\n", irq, irq_status);

	if (irq_status & AIP_V20_P_IRQ_IRQ_FIELD) {
		JZ_AIP_V20_DEBUG("AIP p-job irq.\n");
	}
	if (irq_status & AIP_V20_P_IRQ_CHAIN_IRQ_FIELD) {
		JZ_AIP_V20_DEBUG("AIP p-job chain irq.\n");
		spin_lock(&aip->p_job_lock);
		jz_aip_v20_p_irq_finish_job(aip, now);
		spin_unlock(&aip->p_job_lock);
	}
	if (irq_status & AIP_V20_P_IRQ_TIMEOUT_IRQ_FIELD) {
#ifdef CONFIG_AIP_V20_REPAIR_A1
		JZ_AIP_V20_DEBUG("Response to AIP p-job irq on A1.\n");
		spin_lock(&aip->p_job_lock);
		jz_aip_v20_p_irq_finish_job(aip, now);
		spin_unlock(&aip->p_job_lock);
#else
		JZ_AIP_V20_DEBUG("AIP p-job timeout irq.\n");
#endif
	}

	status = IRQ_HANDLED;
	return status;
}

static irqreturn_t
jz_aip_v20_t_irq_handler(int irq, void *data)
{
	ktime_t now = ktime_get_boottime();
	struct jz_aip_v20_dev *aip = (struct jz_aip_v20_dev *)data;
	irqreturn_t status = IRQ_NONE;
	uint32_t irq_status = AIP_V20_READL(AIP_V20_T_IRQ);
	AIP_V20_WRITEL(AIP_V20_T_IRQ, irq_status);

	JZ_AIP_V20_DEBUG("AIP t-job done, irq=%d status=0x%02x.\n", irq, irq_status);
	if (irq_status & AIP_V20_T_IRQ_IRQ_FIELD) {
		JZ_AIP_V20_DEBUG("AIP t-job irq.\n");
	}
	if (irq_status & AIP_V20_T_IRQ_TASK_FIELD) {
		spin_lock(&aip->t_job_lock);
		jz_aip_v20_t_irq_finish_job(aip, now);
		spin_unlock(&aip->t_job_lock);
	}
	if (irq_status & AIP_V20_T_IRQ_CHAIN_IRQ_FIELD) {
		JZ_AIP_V20_DEBUG("AIP t-job chain irq.\n");
	}
	if (irq_status & AIP_V20_T_IRQ_TIMEOUT_IRQ_FIELD) {
		JZ_AIP_V20_DEBUG("AIP t-job timeout irq.\n");
	}

	status = IRQ_HANDLED;
	return status;
}

int jz_aip_v20_irq_init(struct platform_device *pdev, struct jz_aip_v20_dev *aip)
{
	int t_ret, f_ret, p_ret;

	// Register AIP t-irq
	aip->t_irq = platform_get_irq(pdev, 0);
	//disable_irq_nosync(aip->t_irq);
	enable_irq(aip->t_irq);
	t_ret = devm_request_irq(&pdev->dev,
				 aip->t_irq,
				 jz_aip_v20_t_irq_handler, IRQF_SHARED,
				 "aip-t-irq", aip);
	if (t_ret)
		JZ_AIP_V20_ERROR("AIP t-irq setup failed!\n");

	// Register AIP f-irq
	aip->f_irq = platform_get_irq(pdev, 1);
	enable_irq(aip->f_irq);
	f_ret = devm_request_irq(&pdev->dev,
				 aip->f_irq,
				 jz_aip_v20_f_irq_handler, IRQF_SHARED,
				 "aip-f-irq", aip);
	if (f_ret)
		JZ_AIP_V20_ERROR("AIP f-irq setup failed!\n");

	// Register AIP p-irq
	aip->p_irq = platform_get_irq(pdev, 2);
	enable_irq(aip->p_irq);
	p_ret = devm_request_irq(&pdev->dev,
				 aip->p_irq,
				 jz_aip_v20_p_irq_handler, IRQF_SHARED,
				 "aip-p-irq", aip);
	if (p_ret)
		JZ_AIP_V20_ERROR("AIP p-irq setup failed!\n");

	return t_ret | f_ret | p_ret;
}
