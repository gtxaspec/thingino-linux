/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/cma.h>

#include "jz_aip_v20_drv.h"
#include "jz_aip_v20_regs.h"

static int jz_aip_v20_set_status(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	aip_v20_ioctl_action_status_t status;

	status.drv_version = AIP_V20_DRV_VERSION;
#ifdef CONFIG_AIP_V20_REPAIR_A1
	status.chip_version = 0xa1;
#else
	status.chip_version = 0x41;
#endif

	if (copy_to_user((aip_v20_ioctl_action_status_t __user *)arg, &status,
			sizeof(aip_v20_ioctl_action_status_t)) != 0) {
		JZ_AIP_V20_ERROR("ioctl-action:Failed to copy status_t to user space\n");
		return -EFAULT;
	}

	return 0;
}

static int jz_aip_v20_set_nice(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	int ret;
	aip_v20_ioctl_action_set_proc_nice_t set_proc_nice;

	ret = copy_from_user(&set_proc_nice, (aip_v20_ioctl_action_set_proc_nice_t __user *)arg,
			sizeof(aip_v20_ioctl_action_set_proc_nice_t));
	if (ret) {
		JZ_AIP_V20_ERROR("ioctl-action:Failed to copy proc_nic from user space, ret = %d\n", ret);
		return ret;
	}

	set_user_nice(current, set_proc_nice.value);

	return 0;
}

static int jz_aip_v20_cache_flush(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	int ret;
	aip_v20_ioctl_action_cacheflush_t cacheflush;

	ret = copy_from_user(&cacheflush, (aip_v20_ioctl_action_cacheflush_t __user *)arg,
			sizeof(aip_v20_ioctl_action_cacheflush_t));
	if (ret) {
		JZ_AIP_V20_ERROR("ioctl-action:Failed to copy cacheflush_t from user space, ret = %d\n", ret);
		return ret;
	}
	dma_cache_sync(NULL, cacheflush.vaddr, cacheflush.size,
			(enum dma_data_direction)(cacheflush.dir));

	return 0;
}

static int jz_aip_v20_get_meminfo(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	aip_v20_ioctl_action_meminfo_t meminfo;
	struct sysinfo si;
	si_meminfo(&si);

	meminfo.mem_total = si.totalram << (PAGE_SHIFT - 10);
	meminfo.mem_free = si.freeram << (PAGE_SHIFT - 10);
	meminfo.cma_total = totalcma_pages << (PAGE_SHIFT - 10);
	meminfo.cma_free = (global_page_state(NR_FREE_CMA_PAGES)) << (PAGE_SHIFT - 10);

	if (copy_to_user((aip_v20_ioctl_action_meminfo_t *)arg, &meminfo,
				sizeof(aip_v20_ioctl_action_meminfo_t))) {
		JZ_AIP_V20_ERROR("ioctl-action:Failed to copy meminfo_t to user space\n");
		return -EFAULT;
	}
	return 0;
}

int jz_aip_v20_action(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	int ret = 0;
	uint32_t op;

	ret = copy_from_user(&op, (uint32_t __user *)arg, sizeof(uint32_t));
	if (ret != 0) {
		JZ_AIP_V20_ERROR("ioctl-action:Failed to copy op from user space, ret = %d\n", ret);
		return -EFAULT;
	}

	switch (op) {
	case AIP_ACTION_STATUS:
		ret = jz_aip_v20_set_status(aip, arg);
		break;
	case AIP_ACTION_SET_PROC_NICE:
		ret = jz_aip_v20_set_nice(aip, arg);
		break;
	case AIP_ACTION_CACHE_FLUSH:
		ret = jz_aip_v20_cache_flush(aip, arg);
		break;
	case AIP_ACTION_GET_MEMINFO:
		ret = jz_aip_v20_get_meminfo(aip, arg);
		break;
	case AIP_ACTION_CTX_LOCK:
		mutex_lock(&aip->nna_mutex);
		break;
	case AIP_ACTION_CTX_UNLOCK:
		mutex_unlock(&aip->nna_mutex);;
		break;
	default:
		JZ_AIP_V20_ERROR("ioctl-action:%d can not support op!\n", op);
		return -1;
	}

	return ret;
}
