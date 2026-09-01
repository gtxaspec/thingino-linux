/**
    NVT logfile-dam-buf function
    This file will provide the logfile shared buffer object
    @file logfile-dma-buf.c
    @ingroup
    @note
    Copyright Novatek Microelectronics Corp. 2021. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#define LOGFILE_BUFFER_SIZE	60*1024
#define LOGFILE_BASE		'l'
#define LOGFILE_IOCTL_FD	_IOR(LOGFILE_BASE, 0, int)

struct dma_buf *logfile_dmabuf = NULL;
EXPORT_SYMBOL(logfile_dmabuf);

static int logfile_dmabuf_attach(struct dma_buf *dmabuf, struct dma_buf_attachment *attachment)
{
	return 0;
}

static void logfile_dmabuf_detach(struct dma_buf *dmabuf, struct dma_buf_attachment *attachment)
{
}

static struct sg_table *logfile_dmabuf_map_dma_buf(struct dma_buf_attachment *attachment,
					 							enum dma_data_direction dir)
{
	return NULL;
}

static void logfile_dmabuf_unmap_dma_buf(struct dma_buf_attachment *attachment,
										struct sg_table *table,
										enum dma_data_direction dir)
{
}

static void logfile_dmabuf_release(struct dma_buf *dmabuf)
{
	kfree(dmabuf->priv);
	logfile_dmabuf = NULL;
}

static int logfile_dmabuf_begin_cpu_access(struct dma_buf *dmabuf, enum dma_data_direction dir)
{
	return 0;
}

static int logfile_dmabuf_end_cpu_access(struct dma_buf *dmabuf, enum dma_data_direction dir)
{
	return 0;
}

static void *logfile_dmabuf_map(struct dma_buf *dmabuf, unsigned long num)
{
	return dmabuf->priv;
}

static void logfile_dmabuf_unmap(struct dma_buf *dmabuf, unsigned long num, void *p)
{
}

static int logfile_dmabuf_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	size_t dmabuf_size = LOGFILE_BUFFER_SIZE;
	void *vaddr = dmabuf->priv;
	return remap_pfn_range(vma, vma->vm_start, virt_to_pfn(vaddr), dmabuf_size, vma->vm_page_prot);
}

static void *logfile_dmabuf_vmap(struct dma_buf *dmabuf)
{
	return dmabuf->priv;
}

static void logfile_dmabuf_vunmap(struct dma_buf *dmabuf, void *vaddr)
{
}

static const struct dma_buf_ops logfile_dmabuf_ops = {
	.map_dma_buf = logfile_dmabuf_map_dma_buf,
	.unmap_dma_buf = logfile_dmabuf_unmap_dma_buf,
	.mmap = logfile_dmabuf_mmap,
	.release = logfile_dmabuf_release,
	.attach = logfile_dmabuf_attach,
	.detach = logfile_dmabuf_detach,
	.begin_cpu_access = logfile_dmabuf_begin_cpu_access,
	.end_cpu_access = logfile_dmabuf_end_cpu_access,
	.map = logfile_dmabuf_map,
	.unmap = logfile_dmabuf_unmap,
	.vmap = logfile_dmabuf_vmap,
	.vunmap = logfile_dmabuf_vunmap,
};

static struct dma_buf *logfile_dmabuf_alloc(size_t size)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct dma_buf *dmabuf;
	int flags;
	size_t dmabuf_size;
	void *vaddr;

	dmabuf_size = size;
	vaddr = kzalloc(dmabuf_size, GFP_KERNEL);
	if (!vaddr)
		return NULL;

	/*
	 * Open for read/write and enable the close-on-exec flag
	 * for the new file descriptor.
	 */
	flags = O_RDWR | O_CLOEXEC;

	exp_info.ops = &logfile_dmabuf_ops;
	exp_info.size = dmabuf_size;
	exp_info.flags = flags;
	exp_info.priv = vaddr;

	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		kfree(vaddr);
		return NULL;
	}

	return dmabuf;
}

static long logfile_mdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case LOGFILE_IOCTL_FD:
	{
		int fd;
		int flags;

		/*
		 * Open for read/write and enable the close-on-exec flag
		 * for the new file descriptor.
		 */
		flags = O_RDWR | O_CLOEXEC;
		fd = dma_buf_fd(logfile_dmabuf, flags);
		if (fd < 0)
			return fd;

		if (copy_to_user((void __user *)arg, &fd, sizeof(fd)))
			return -EFAULT;

		break;
	}
	default:
		return -EFAULT;
	}
	return 0;
}

static int logfile_mdev_mmap(struct file *file, struct vm_area_struct *vma)
{
	/* Setup up a userspace mmap with the given vma. */
	return dma_buf_mmap(logfile_dmabuf, vma, 0);
}

static const struct file_operations logfile_mdev_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl	= logfile_mdev_ioctl,
	.mmap = logfile_mdev_mmap,
};

static struct miscdevice logfile_mdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "logfile",
	.fops = &logfile_mdev_fops,
};

static int __init logfile_mdev_init(void)
{
	int ret;

	/*
	 * Create the logfile dma_buf object here,
	 * and return error if allocate failed to
	 * avoid userspace application using invalid mdev.
	 */
	logfile_dmabuf = logfile_dmabuf_alloc(LOGFILE_BUFFER_SIZE);
	if (!logfile_dmabuf)
		return -ENOMEM;

	ret = misc_register(&logfile_mdev);
	return ret;
}

static void __exit logfile_mdev_exit(void)
{
	misc_deregister(&logfile_mdev);
}

module_init(logfile_mdev_init);
module_exit(logfile_mdev_exit);

MODULE_AUTHOR("Novatek Microelectronics Corp.");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("logfile-dma-buf function for NVT");
MODULE_VERSION("1.00.000");