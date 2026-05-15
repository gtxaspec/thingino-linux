/*
 * rmem.c - Reserved memory driver for libimp.
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "linux/export.h"
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <linux/dma-buf.h>

#define RMEM_MAGIC			'r'
#define RMEM_CMD_FLUSH_CACHE		_IOWR(RMEM_MAGIC, 0, struct rmem_flush_cache_info)
#define RMEM_CMD_RMEM_INFO			_IOWR(RMEM_MAGIC, 1, struct rmem_info)
#define RMEM_CMD_GET_DMABUF			_IOWR(RMEM_MAGIC, 2, struct rmem_dma_buf_info)

struct  rmem_info {
    unsigned int vaddr;
    unsigned int paddr;
};
static struct rmem_info rmem_addr;

struct rmem_dma_buf_info {
	unsigned int	addr;
	unsigned int	len;
	unsigned int	fd;
};

static struct miscdevice	mdev;

struct rmem_flush_cache_info {
	unsigned int	addr;
	unsigned int	len;
	unsigned int	dir;
};

static int rmem_open(struct inode *inode, struct file *filp)
{
	pr_debug("[%d:%d] %s\n", current->tgid, current->pid, __func__);
	return 0;
}

static int rmem_release(struct inode *inode, struct file *filp)
{
	pr_debug("[%d:%d] %s\n", current->tgid, current->pid, __func__);
	return 0;
}

static long rmem_cmd_flush_cache(long arg)
{
	struct rmem_flush_cache_info info;
	long ret = 0;
	if (copy_from_user(&info, (void *)arg, sizeof(info))) {
		return -EFAULT;
	}

	dma_cache_sync(NULL, (void *)info.addr, info.len, info.dir);

	return ret;
}

#define CONFIG_DMA_SHARED_BUFFER

#ifdef CONFIG_DMA_SHARED_BUFFER
static int rmem_dma_buf_attach(struct dma_buf *dmabuf,
				  struct dma_buf_attachment *attachment)
{
	printk("rmem attach\n");
	return 0;
}
static void rmem_dma_buf_detatch(struct dma_buf *dmabuf,
				    struct dma_buf_attachment *attachment)
{
	printk(" rmem_dma_buf_detatch\n");
}

static struct sg_table *
rmem_map_dma_buf(struct dma_buf_attachment *attachment,
		    enum dma_data_direction dir)
{
	printk("rmem rmem_map_dma_buf\n");
	return 0;
}

static void rmem_unmap_dma_buf(struct dma_buf_attachment *attach,
				  struct sg_table *table,
				  enum dma_data_direction dir)
{
	printk("rmem rmem_unmap_dma_buf\n");
}

static int rmem_dma_mmap(struct dma_buf *dmabuf,
			struct vm_area_struct *vma)
{
	printk("rmem rmem_dma_mmap\n");
	return 0;
}

static int rmem_dma_vmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
{
	printk("rmem rmem_dma_vmap\n");
	return 0;
}

static void rmem_dma_release(struct dma_buf *dmabuf)
{
	printk("rmem fastrpc_release\n");
	return ;
}

static const struct dma_buf_ops rmem_dmabuf_ops = {
	.attach = rmem_dma_buf_attach,
	.detach = rmem_dma_buf_detatch,
	.map_dma_buf = rmem_map_dma_buf,
	.unmap_dma_buf = rmem_unmap_dma_buf,
	.mmap = rmem_dma_mmap,
	.vmap = rmem_dma_vmap,
	.release = rmem_dma_release,
};

// static const struct dma_buf_ops rmem_dmabuf_ops = {0};

static long rmem_cmd_get_dmabuf(long arg)
{
	struct rmem_dma_buf_info dmabuf_info = {0};
	struct dma_buf *dmabuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	int fd = 0;
	long ret = 0;

	if (copy_from_user(&dmabuf_info, (void *)arg, sizeof(dmabuf_info))) {
		return -EFAULT;
	}

	exp_info.ops = &rmem_dmabuf_ops;
	exp_info.size = dmabuf_info.len;
	exp_info.flags = O_CLOEXEC | O_RDWR;
	exp_info.priv = (void *)dmabuf_info.addr;

	dmabuf = dma_buf_export(&exp_info);
	if(!dmabuf){
		printk("err,dmabuf is null\n");
		return -EFAULT;
	}
	fd = dma_buf_fd(dmabuf, O_CLOEXEC|O_RDWR);
	dmabuf_info.fd = fd;
	// printk("dmabuf_addr:%#x,fd:%d\n",dmabuf_info.addr,fd);

	if (copy_to_user((void *)arg, &dmabuf_info, sizeof(dmabuf_info)))
		return -EFAULT;
	return ret;
}
#endif

static long rmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case RMEM_CMD_FLUSH_CACHE:
		return rmem_cmd_flush_cache(arg);
	case RMEM_CMD_RMEM_INFO:
	    if (copy_to_user((void *)arg, &rmem_addr, sizeof(struct rmem_info))) {
		    return -EFAULT;
	    }
		break;
#ifdef CONFIG_DMA_SHARED_BUFFER
	case RMEM_CMD_GET_DMABUF:
		return rmem_cmd_get_dmabuf(arg);
#endif
	default:
		pr_err("Unknown ioctl: 0x%.8X\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static int rmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_IO;

	pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
	pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NONCOHERENT;

	if (io_remap_pfn_range(vma,vma->vm_start,
			       vma->vm_pgoff,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot))
		return -EAGAIN;
    rmem_addr.vaddr = vma->vm_start;
    rmem_addr.paddr = vma->vm_pgoff * PAGE_SIZE;


	return 0;
}

static struct file_operations rmem_misc_fops = {
	.open		= rmem_open,
	.release	= rmem_release,
	.unlocked_ioctl	= rmem_ioctl,
	.mmap		= rmem_mmap,
};

static int rmem_probe(struct platform_device *pdev)
{
	int ret;

#ifdef CONFIG_ZERATUL
	mdev.minor = 53;
#else
	mdev.minor = MISC_DYNAMIC_MINOR;
#endif
	mdev.name =  "rmem";
	mdev.fops = &rmem_misc_fops;

	ret = misc_register(&mdev);
	if (ret < 0) {
		pr_err("rmem register failed\n");
		return -1;
	}

    rmem_addr.vaddr = 0;
    rmem_addr.paddr = 0;

	return 0;
}

static int rmem_remove(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver rmem_driver = {
	.probe		= rmem_probe,
	.remove		= rmem_remove,
	.driver		= {
		.name	= "rmem",
	},
};

struct platform_device rmem_device = {
	.name = "rmem",
	.id = -1,
	.resource = NULL,
	.num_resources = 0,
};

static int __init rmem_init(void)
{
	platform_device_register(&rmem_device);
	return platform_driver_register(&rmem_driver);
}

static void __exit rmem_exit(void)
{
	platform_device_unregister(&rmem_device);
	platform_driver_unregister(&rmem_driver);
}

module_init(rmem_init);
module_exit(rmem_exit);

MODULE_LICENSE("GPL v2");
