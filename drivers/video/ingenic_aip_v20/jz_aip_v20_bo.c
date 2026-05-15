/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

// bo:buffer object

#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>

#include "jz_aip_v20_drv.h"

int jz_aip_v20_bo_init(struct jz_aip_v20_dev *aip)
{
	mutex_init(&aip->bo_mutex);
	idr_init(&aip->bo_handles);

	return 0;
}

struct jz_aip_v20_bo_info *jz_aip_v20_bo_create(
		struct jz_aip_v20_dev *aip, size_t size)
{
	struct jz_aip_v20_bo_info *bo;

	size = round_up(size, PAGE_SIZE);

	bo = kzalloc(sizeof(*bo), GFP_KERNEL);
	if (!bo) {
		JZ_AIP_V20_ERROR("Can not create bo.\n");
		return ERR_PTR(-ENOMEM);
	}

	bo->vaddr = dma_alloc_writecombine(aip->mdev.this_device, size,
			&bo->dma, GFP_KERNEL | __GFP_NOWARN);
	bo->size = size;
	bo->pid = task_tgid_nr(current);

	if (!bo->vaddr) {
		JZ_AIP_V20_ERROR("Can not mmap vaddr for bo.\n");
		return ERR_PTR(-ENOMEM);
	}


	return bo;
}

static
int jz_aip_v20_bo_remove(struct jz_aip_v20_dev *aip, struct jz_aip_v20_bo_info *bo)
{
	dma_free_writecombine(aip->mdev.this_device, bo->size,
			bo->vaddr, bo->dma);
	kfree(bo);

	return 0;
}

static
int jz_aip_v20_bo_append(struct jz_aip_v20_dev *aip, aip_v20_ioctl_bo_t *dat)
{
	int r;
	struct jz_aip_v20_bo_info *bo;

	mutex_lock(&aip->bo_mutex);
	bo = jz_aip_v20_bo_create(aip, dat->size);
	if (IS_ERR(bo)) {
		mutex_unlock(&aip->bo_mutex);
		return -EINVAL;
	}
	r = idr_alloc(&aip->bo_handles, bo, 1, 0, GFP_KERNEL);
	if (r < 0) {
		mutex_unlock(&aip->bo_mutex);
		jz_aip_v20_bo_remove(aip, bo);
		return -EINVAL;
	}
	dat->size = bo->size;
	dat->paddr = bo->dma;
	dat->handle = r;
	mutex_unlock(&aip->bo_mutex);

	JZ_AIP_V20_DEBUG("BO[%d] is created, pid[%d], size[0x%x].\n",
			dat->handle, bo->pid, bo->size);

	return 0;
}

static
int jz_aip_v20_bo_destroy(struct jz_aip_v20_dev *aip, aip_v20_ioctl_bo_t *dat)
{
	int id;
	struct jz_aip_v20_bo_info *bo;

	mutex_lock(&aip->bo_mutex);
	id = dat->handle;
	bo = idr_find(&aip->bo_handles, id);
	if (bo) {
		jz_aip_v20_bo_remove(aip, bo);
		idr_remove(&aip->bo_handles, id);
	}
	mutex_unlock(&aip->bo_mutex);

	JZ_AIP_V20_DEBUG("BO[%d] is freed, pid[%d], size[0x%x]\n",
			dat->handle, bo->pid, bo->size);

	return 0;
}

int jz_aip_v20_bo(struct jz_aip_v20_dev *aip, unsigned long arg)
{
	int ret = 0;
	aip_v20_ioctl_bo_t dat;

	ret = copy_from_user(&dat, (const aip_v20_ioctl_bo_t __user *)arg, sizeof(aip_v20_ioctl_bo_t));
	if (ret != 0) {
		JZ_AIP_V20_ERROR("ioctl-bo:Failed to copy data from user space, ret = %d\n", ret);
		return -EFAULT;
	}

	switch (dat.op) {
		case AIP_BO_OP_APPEND:
			ret = jz_aip_v20_bo_append(aip, &dat);
			if (ret == 0) {
				if(copy_to_user((aip_v20_ioctl_bo_t __user *)arg,
						&dat, sizeof(aip_v20_ioctl_bo_t))) {
					JZ_AIP_V20_ERROR("ioctl-bo:Failed to copy data to user space, ret = %d\n", ret);
				}
			}
			break;
		case AIP_BO_OP_DESTROY:
			ret = jz_aip_v20_bo_destroy(aip, &dat);
			break;
		default:
			ret = -EINVAL;
			JZ_AIP_V20_ERROR("ioctl-bo:%d can not support op!\n", dat.op);
			break;
	}

	return ret;
}

int jz_aip_v20_bo_release(struct jz_aip_v20_dev *aip, pid_t pid)
{
	int id;
	struct jz_aip_v20_bo_info *bo;

	mutex_lock(&aip->bo_mutex);
	if (!idr_is_empty(&aip->bo_handles)) {
		idr_for_each_entry(&aip->bo_handles, bo, id) {
			if (bo->pid == pid) {
				jz_aip_v20_bo_remove(aip, bo);
				idr_remove(&aip->bo_handles, id);
			}
		}
	}
	mutex_unlock(&aip->bo_mutex);

	return 0;
}
