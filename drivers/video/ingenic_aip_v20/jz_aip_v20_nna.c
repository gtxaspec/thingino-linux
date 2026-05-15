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
#include <linux/genalloc.h>
#include <linux/smp.h>
#include <linux/slab.h>

#include "jz_aip_v20_drv.h"
#include "jz_aip_v20_trace.h"

int jz_nna_v20_core_init(struct platform_device *pdev, struct jz_aip_v20_dev *nna)
{
	int ret;
	int oram_cfg;

	volatile uint32_t *ccu_mscr =
		(volatile uint32_t *)(0xb2200060);

	oram_cfg = (*ccu_mscr & 0x1c00) >> 10;

	nna->total_l2c_size = 512 * 1024; //T41 512KB, T40 1MB , A1 1MB
	switch (oram_cfg) {
		case 0:
			nna->curr_l2c_size = 0 * 1024;
			break;
		case 1:
			nna->curr_l2c_size = 128 * 1024;
			break;
		case 2:
			nna->curr_l2c_size = 256 * 1024;
			break;
		case 3:
			nna->curr_l2c_size = 512 * 1024;
			break;
		default:
			JZ_AIP_V20_ERROR("Get NNA ORAM size error %d\n", oram_cfg);
			return -1;
	}
	nna->oram_size = nna->total_l2c_size - nna->curr_l2c_size;

	JZ_AIP_V20_INFO("L2 cache size:%dkB  NNA ORAM size %dkB\n",
			nna->curr_l2c_size / 1024,
			nna->oram_size / 1024);

	INIT_LIST_HEAD(&nna->nna_job_list);
	spin_lock_init(&nna->nna_job_lock);
	mutex_init(&nna->nna_oram_lock);
	idr_init(&nna->nna_oram_handles);
	mutex_init(&nna->nna_mutex);

	nna->oram_pool = NULL;
	if (!nna->oram_size)
		return 0;

	nna->dma_size = JZ_NNA_DMA_IOSIZE;
	nna->dma_pbase = JZ_NNA_DMA_IOBASE;
	nna->dma_desram_size = JZ_NNA_DMA_DESRAM_IOSIZE;
	nna->dma_desram_pbase = JZ_NNA_DMA_DESRAM_IOBASE;
	nna->oram_pbase = JZ_NNA_ORAM_IOBASE;

	nna->oram_pool = devm_gen_pool_create(&pdev->dev,
			ilog2(JZ_NNA_V20_ORAM_GRANULARITY),
			NUMA_NO_NODE, "jz_nna_oram");
	if (IS_ERR(nna->oram_pool))
		return PTR_ERR(nna->oram_pool);

	nna->virt_oram_base = devm_ioremap_wc(nna->mdev.this_device,
			nna->oram_pbase, nna->oram_size);
	if (IS_ERR(nna->virt_oram_base))
		return PTR_ERR(nna->virt_oram_base);

	ret = gen_pool_add_virt(nna->oram_pool,
			(unsigned long)nna->virt_oram_base,
			nna->oram_pbase + nna->curr_l2c_size,
			nna->oram_size, NUMA_NO_NODE);
	if (ret < 0) {
		JZ_AIP_V20_ERROR("Failed to register oram pool.\n");
		return ret;
	}

	gen_pool_set_algo(nna->oram_pool, gen_pool_best_fit, NULL);

	return 0;
}

static int jz_nna_lock(struct jz_aip_v20_dev *nna,
		unsigned long arg, ktime_t submit_time, pid_t pid)
{
	struct jz_nna_v20_exec_info *exec = NULL;

	mutex_lock(&nna->nna_mutex);

	exec = kcalloc(1, sizeof(struct jz_nna_v20_exec_info), GFP_KERNEL);
	if (!exec) {
		JZ_AIP_V20_ERROR("Can not create NNA exec info.\n");
		return -EFAULT;
	}
	/*
	 *#ifdef CONFIG_SCHED_INFO
	 *	exec->sched_pcount = current->sched_info.pcount;
	 *#endif
	 */
	exec->submit_time = submit_time;
	exec->pid = pid;

	if (!cpumask_equal(tsk_cpus_allowed(current), cpumask_of(0))) {
		set_cpus_allowed_ptr(current, cpumask_of(0));
	}
	JZ_AIP_V20_SET_JOB_NAME((aip_v20_ioctl_nna_mutex_lock_t *)arg, nna);

	spin_lock(&nna->nna_job_lock);
	nna->nna_seqno++;
	nna->nna_pid = exec->pid;
	list_add_tail(&exec->head, &nna->nna_job_list);
	exec->start_time = ktime_get_boottime();
	spin_unlock(&nna->nna_job_lock);
	return 0;
}

static int jz_nna_unlock(struct jz_aip_v20_dev *nna,
		unsigned long arg, pid_t pid)
{
	aip_v20_ioctl_nna_mutex_unlock_t i_mutex_unlock;
	struct jz_nna_v20_exec_info *exec = NULL;
	ktime_t curr_time;
	int64_t sched_time, run_time, total_time;
	cpumask_t cpumask;

	if (copy_from_user(&i_mutex_unlock,
				(const aip_v20_ioctl_nna_mutex_unlock_t __user *)arg,
				sizeof(aip_v20_ioctl_nna_mutex_unlock_t)) != 0) {
		return -EFAULT;
	}

	if (!i_mutex_unlock.force_unlock && nna->nna_pid != pid) {
		JZ_AIP_V20_ERROR("NNA mutex unlock by other proc.\n");
		return -EFAULT;
	}

	spin_lock(&nna->nna_job_lock);
	if (mutex_is_locked(&nna->nna_mutex)) {
		list_for_each_entry(exec, &nna->nna_job_list, head) {
			if (!exec) {
				JZ_AIP_V20_ERROR("NNA mutex unlock err.\n");
				BUG();
			}
			if (exec->pid != nna->nna_pid)
				continue;
			list_del(&exec->head);
			curr_time = ktime_get_boottime();
			sched_time = ktime_us_delta(
					exec->start_time, exec->submit_time);
			run_time = ktime_us_delta(
					curr_time, exec->start_time);
			total_time = ktime_us_delta(
					curr_time, exec->submit_time);
			trace_nna_v20_job("nna", exec->name,
					nna->nna_seqno, sched_time,
					run_time, total_time);
			kfree(exec);

			if (!i_mutex_unlock.force_unlock &&
					i_mutex_unlock.cpumask) {
				cpumask_setall(&cpumask);
				set_cpus_allowed_ptr(current, &cpumask);
			}
			mutex_unlock(&nna->nna_mutex);
			break;
		}
	} else {
		if (!i_mutex_unlock.force_unlock) {
			JZ_AIP_V20_ERROR("NNA mutex is not locked.\n");
			return -EFAULT;
		}
	}
	spin_unlock(&nna->nna_job_lock);

	if (atomic_read(&nna->ref.refcount) > 1)
		schedule();
	return 0;
}

static int jz_get_nna_status(struct jz_aip_v20_dev *nna, unsigned long arg)
{
	aip_v20_ioctl_nna_status_t status;

	status.total_l2c_size = nna->total_l2c_size;
	status.curr_l2c_size = nna->curr_l2c_size;
	status.oram_size = nna->oram_size;
	status.oram_pbase = nna->oram_pbase;
	status.dma_size = nna->dma_size;
	status.dma_pbase = nna->dma_pbase;
	status.dma_desram_size = nna->dma_desram_size;
	status.dma_desram_pbase = nna->dma_desram_pbase;

	if (copy_to_user((aip_v20_ioctl_nna_status_t __user *)arg, &status,
				sizeof(aip_v20_ioctl_nna_status_t)) != 0) {
		JZ_AIP_V20_ERROR("ioctl-nna:Failed to copy status_t from user space");
		return -EFAULT;
	}

	return 0;
}

static int jz_oram_alloc(struct jz_aip_v20_dev *nna,
		unsigned long arg, pid_t curr_pid)
{
	aip_v20_ioctl_nna_oram_t oram;
	size_t oram_size;
	struct jz_nna_v20_oram *oram_info;
	void *oram_alloc_addr;

	if (copy_from_user(&oram, (const aip_v20_ioctl_nna_oram_t __user *)arg,
				sizeof(aip_v20_ioctl_nna_oram_t)) != 0) {
		JZ_AIP_V20_ERROR("ioctl-nna:Failed to copy oram_t from user space");
		return -EFAULT;
	}

	oram_size = round_up(oram.size, 64);
	if (nna->oram_pool &&
			gen_pool_avail(nna->oram_pool) >= oram_size) {
		oram_info = kcalloc(1, sizeof(struct jz_nna_v20_oram), GFP_KERNEL);
		if (!oram_info) {
			JZ_AIP_V20_ERROR("Can not create oram info.\n");
			return -EFAULT;
		}

		oram_alloc_addr = gen_pool_dma_alloc(nna->oram_pool, oram_size,
				(dma_addr_t *)(&oram.paddr));
		if (oram_alloc_addr == NULL) {
			kfree(oram_info);
			return -EFAULT;
		}
		oram_info->pid = curr_pid;
		oram_info->kvaddr = (unsigned long)oram_alloc_addr;
		oram_info->size = oram_size;
		mutex_lock(&nna->nna_oram_lock);
		oram.handle = idr_alloc(&nna->nna_oram_handles,
				oram_info, 1, 0, GFP_KERNEL);
		if (oram.handle < 0) {
			gen_pool_free(nna->oram_pool, oram_info->kvaddr, oram_info->size);
			kfree(oram_info);
			JZ_AIP_V20_ERROR("Can not create oram handle.\n");
		}
		mutex_unlock(&nna->nna_oram_lock);
	} else {
		return -EFAULT;
	}

	if (copy_to_user((aip_v20_ioctl_nna_oram_t __user *)arg, &oram,
				sizeof(aip_v20_ioctl_nna_oram_t)) != 0) {
		JZ_AIP_V20_ERROR("ioctl-nna:Failed to copy oram_t to user space");
		return -EFAULT;
	}

	return 0;
}

static int jz_oram_free(struct jz_aip_v20_dev *nna, unsigned long arg)
{
	aip_v20_ioctl_nna_oram_t oram;
	struct jz_nna_v20_oram *oram_info;

	if (copy_from_user(&oram, (const aip_v20_ioctl_nna_oram_t __user *)arg,
				sizeof(aip_v20_ioctl_nna_oram_t)) != 0) {
		JZ_AIP_V20_ERROR("ioctl-nna:Failed to copy oram_t from user space");
		return -EFAULT;
	}

	if (nna->oram_pool &&
	    gen_pool_avail(nna->oram_pool) < gen_pool_size(nna->oram_pool)) {
		mutex_lock(&nna->nna_oram_lock);
		oram_info = idr_find(&nna->nna_oram_handles, oram.handle);
		if (oram_info) {
			gen_pool_free(nna->oram_pool, oram_info->kvaddr, oram_info->size);
			idr_remove(&nna->nna_oram_handles, oram.handle);
		}
		mutex_unlock(&nna->nna_oram_lock);	
	}

	return 0;
}

int jz_nna_v20_op(struct jz_aip_v20_dev *nna, unsigned long arg)
{
	int ret = 0;
	ktime_t submit_time = ktime_get_boottime();
	pid_t curr_pid = task_tgid_vnr(current);
	aip_v20_ioctl_nna_op_t op;

	if (copy_from_user(&op, (const aip_v20_ioctl_nna_op_t __user *)arg,
				sizeof(aip_v20_ioctl_nna_op_t)) != 0) {
		JZ_AIP_V20_ERROR("ioctl-nna:Failed to copy op from user space");
		return -EFAULT;
	}

	switch (op) {
		case NNA_OP_MUTEX_LOCK:
			ret = jz_nna_lock(nna, arg, submit_time, curr_pid);
			break;
		case NNA_OP_MUTEX_TRYLOCK:
			if (mutex_trylock(&nna->nna_mutex))
				ret = jz_nna_lock(nna, arg, submit_time, curr_pid);
			else
				ret = -EFAULT;
			break;
		case NNA_OP_MUTEX_UNLOCK:
			ret = jz_nna_unlock(nna, arg, curr_pid);
			break;
		case NNA_OP_STATUS:
			ret = jz_get_nna_status(nna, arg);
			break;
		case NNA_OP_ORAM_ALLOC:
			ret = jz_oram_alloc(nna, arg, curr_pid);
			break;
		case NNA_OP_ORAM_FREE:
			ret = jz_oram_free(nna, arg);
			break;
		default:
			JZ_AIP_V20_ERROR("ioctl-nna:Can not support op %d!\n", op);
			ret = -EFAULT;
			break;
	}

	return ret;
}

int jz_nna_v20_release(struct jz_aip_v20_dev *nna, pid_t pid)
{
	struct jz_nna_v20_exec_info *exec = NULL;
	int id;
	struct jz_nna_v20_oram *oram_info;

	spin_lock(&nna->nna_job_lock);
	list_for_each_entry(exec, &nna->nna_job_list, head) {
		if (!exec) {
			JZ_AIP_V20_ERROR("NNA mutex unlock err.\n");
			BUG();
		}
		if (exec->pid != pid) {
			continue;
		}
		list_del(&exec->head);
		mutex_unlock(&nna->nna_mutex);
		break;
	}
	spin_unlock(&nna->nna_job_lock);

	mutex_lock(&nna->nna_oram_lock);
	if (!idr_is_empty(&nna->nna_oram_handles)) {
		idr_for_each_entry(&nna->nna_oram_handles, oram_info, id) {
			if (oram_info->pid == pid) {
				gen_pool_free(nna->oram_pool, oram_info->kvaddr, oram_info->size);
				idr_remove(&nna->nna_oram_handles, id);
			}
		}
	}
	mutex_unlock(&nna->nna_oram_lock);

	return 0;
}
