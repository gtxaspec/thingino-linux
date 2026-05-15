/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <asm/irq.h>
#ifdef CONFIG_AIP_V20_REPAIR_A1
	#include <dt-bindings/interrupt-controller/a1-irq.h>
#else
	#include <dt-bindings/interrupt-controller/t41-irq.h>
#endif

#include "jz_aip_v20_drv.h"

static uint64_t jz_aip_v20_dma_mask = ~((uint64_t)0);

static struct resource jz_aip_v20_resources[] = {
	[0] = {
		.start = JZ_AIP_V20_IOBASE,
		.end = JZ_AIP_V20_IOBASE + JZ_AIP_V20_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MODULE_IRQ_NUM(IRQ_AIP_V20_T),
		.end = MODULE_IRQ_NUM(IRQ_AIP_V20_T),
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = MODULE_IRQ_NUM(IRQ_AIP_V20_F),
		.end = MODULE_IRQ_NUM(IRQ_AIP_V20_F),
		.flags = IORESOURCE_IRQ,
	},
	[3] = {
		.start = MODULE_IRQ_NUM(IRQ_AIP_V20_P),
		.end = MODULE_IRQ_NUM(IRQ_AIP_V20_P),
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device jz_aip_v20_device = {
	.name = "jz-aip-v20",
	.id = 0,
	.dev = {
		.dma_mask = &jz_aip_v20_dma_mask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = ARRAY_SIZE(jz_aip_v20_resources),
	.resource = jz_aip_v20_resources,
};

/* Helper function for mapping the regs on a platform device. */
static void __iomem *jz_aip_v20_ioremap_regs(struct platform_device *pdev, int index)
{
	struct resource *res;
	void __iomem *map;

	res = platform_get_resource(pdev, IORESOURCE_MEM, index);
	map = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(map)) {
	        JZ_AIP_V20_ERROR("Failed to map registers: %ld\n", PTR_ERR(map));
	        return map;
	}

	return map;
}

static void jz_aip_v20_clk(int set)
{
	volatile uint32_t *ccu_cscr =
		(volatile uint32_t *)(0xb2200000);
	volatile uint32_t *ccu_mscr =
		(volatile uint32_t *)(0xb2200060);

	if (set) {
/* clear zero of CCUU.CSCR, force cpu cannot enter sleep status.
 * fix when cpu sleep , close aip clk
 */
		*ccu_cscr = 0;
		*ccu_mscr = *ccu_mscr | (1 << 25);

/* fix when cpu l2 idle, it can close oram clk */
		*ccu_mscr = *ccu_mscr | (1 << 5);
	} else {
		*ccu_cscr = -1;
		*ccu_mscr = *ccu_mscr & ~(1 << 25);
		*ccu_mscr = *ccu_mscr & ~(1 << 5);
	}
}

static int jz_aip_v20_open (struct inode *indo, struct file *file)
{
	struct miscdevice *mdev = file->private_data;
	struct jz_aip_v20_dev *aip =
		list_entry(mdev, struct jz_aip_v20_dev, mdev);

	mutex_lock(&aip->struct_mutex);

	if (!aip->is_ok) {
		jz_aip_v20_clk_open;
		aip->is_ok = 1;
		kref_init(&aip->ref);
	} else
		kref_get(&aip->ref);

	mutex_unlock(&aip->struct_mutex);

	return 0;
}

static void jz_aip_v20_close(struct kref *ref)
{
	struct jz_aip_v20_dev *aip =
		container_of(ref, struct jz_aip_v20_dev, ref);

	if (!list_empty(&aip->f_job_list)   ||
	    !list_empty(&aip->t_job_list)   ||
	    !list_empty(&aip->p_job_list)   ||
	    !idr_is_empty(&aip->bo_handles) ||
	    !idr_is_empty(&aip->nna_oram_handles) ||
	    mutex_is_locked(&aip->nna_mutex)) {
		JZ_AIP_V20_ERROR("AIP has unfinish task, but need close it!\n");
		BUG();
	} else {
		jz_aip_v20_clk_close;
	}
}

static int jz_aip_v20_f_release(struct jz_aip_v20_dev *aip, pid_t pid)
{
	unsigned long irqflags;
	int re_submit = 0;
	struct jz_aip_v20_f_exec_info *info, *info_tmp;

	spin_lock_irqsave(&aip->f_job_lock, irqflags);
	if (!list_empty(&aip->f_job_list)) {
		list_for_each_entry_safe(info, info_tmp, &aip->f_job_list, head) {
			if (info->pid == pid) {
				if (info->state == AIP_JOB_RUNNING) {
					re_submit = 1;
					jz_aip_v20_f_reset(aip);
				}
				list_del(&info->head);
				kfree(info);
			}
		}
	}
	if (re_submit)
		jz_aip_v20_submit_next_f_job(aip);
	spin_unlock_irqrestore(&aip->f_job_lock, irqflags);

	return 0;
}

static int jz_aip_v20_p_release(struct jz_aip_v20_dev *aip, pid_t pid)
{
	unsigned long irqflags;
	int re_submit = 0;
	struct jz_aip_v20_p_exec_info *info, *info_tmp;

	spin_lock_irqsave(&aip->p_job_lock, irqflags);
	if (!list_empty(&aip->p_job_list)) {
		list_for_each_entry_safe(info, info_tmp, &aip->p_job_list, head) {
			if (info->pid == pid) {
				if (info->state == AIP_JOB_RUNNING) {
					re_submit = 1;
					jz_aip_v20_p_reset(aip);
				}
				list_del(&info->head);
				kfree(info);
			}
		}
	}
	if (re_submit)
		jz_aip_v20_submit_next_p_job(aip);
	spin_unlock_irqrestore(&aip->p_job_lock, irqflags);

	return 0;
}

static int jz_aip_v20_t_release(struct jz_aip_v20_dev *aip, pid_t pid)
{
	unsigned long irqflags;
	int re_submit = 0;
	struct jz_aip_v20_t_exec_info *info, *info_tmp;

	spin_lock_irqsave(&aip->t_job_lock, irqflags);
	if (!list_empty(&aip->t_job_list)) {
		list_for_each_entry_safe(info, info_tmp, &aip->t_job_list, head) {
			if (info->pid == pid) {
				if (info->state == AIP_JOB_RUNNING) {
					re_submit = 1;
					jz_aip_v20_t_reset(aip);
				}
				list_del(&info->head);
				kfree(info);
			}
		}
	}
	if (re_submit)
		jz_aip_v20_submit_next_t_job(aip);
	spin_unlock_irqrestore(&aip->t_job_lock, irqflags);

	return 0;
}

static int jz_aip_v20_release(struct inode *inode, struct file *file)
{
	struct miscdevice *mdev = file->private_data;
	struct jz_aip_v20_dev *aip =
		list_entry(mdev, struct jz_aip_v20_dev, mdev);
	pid_t pid = task_tgid_vnr(current);

	jz_aip_v20_f_release(aip, pid);
	jz_aip_v20_p_release(aip, pid);
	jz_aip_v20_t_release(aip, pid);

	jz_aip_v20_bo_release(aip, pid);

	jz_nna_v20_release(aip, pid);

	if (kref_put_mutex(&aip->ref, jz_aip_v20_close, &aip->struct_mutex)) {
		aip->is_ok = 0;
		mutex_unlock(&aip->struct_mutex);
	}

	return 0;
}

static ssize_t jz_aip_v20_read(struct file *file, char __user * buffer, size_t count, loff_t * ppos)
{
	return 0;
}

static ssize_t jz_aip_v20_write(struct file *file, const char __user * buffer, size_t count, loff_t * ppos)
{
	return 0;
}

static int jz_aip_v20_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;

	if (vma->vm_pgoff == 0) {
		pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
		pgprot_val(vma->vm_page_prot) |= _CACHE_UNCACHED;
		ret = remap_pfn_range(vma, vma->vm_start,
				0,
				PAGE_SIZE,
				vma->vm_page_prot);
	} else if (vma->vm_pgoff << PAGE_SHIFT == 0x12620000) {
		pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
		pgprot_val(vma->vm_page_prot) |= _CACHE_UNCACHED_ACCELERATED;
		ret = io_remap_pfn_range(vma, vma->vm_start,
				vma->vm_pgoff,
				vma->vm_end - vma->vm_start,
				vma->vm_page_prot);
	} else {
		JZ_AIP_V20_ERROR("AIPV20 mmap fail! vm_pgoff = %lu\n", vma->vm_pgoff);
		ret = -EFAULT;
	}

	return ret;
}

static long jz_aip_v20_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct miscdevice *mdev = file->private_data;
	struct jz_aip_v20_dev *aip = to_jz_aip_v20_dev(mdev);

	switch (cmd) {
		case IOCTL_AIP_V20_SUBMIT:
			ret = jz_aip_v20_submit(aip, arg);
			break;
		case IOCTL_AIP_V20_WAIT:
			ret = jz_aip_v20_wait(aip, arg);
			break;
		case IOCTL_AIP_V20_BO:
			ret = jz_aip_v20_bo(aip, arg);
			break;
		case IOCTL_NNA_V20_OP:
			ret = jz_nna_v20_op(aip, arg);
			break;
		case IOCTL_AIP_V20_ACTION:
			ret = jz_aip_v20_action(aip, arg);
			break;
		default:
			ret = -1;
			JZ_AIP_V20_ERROR("ioctl:%d can not support cmd!\n", cmd);
			break;
	}

	return ret;
}

const struct file_operations jz_aip_v20_fops = {
	.owner          = THIS_MODULE,
	.open           = jz_aip_v20_open,
	.release        = jz_aip_v20_release,
	.read           = jz_aip_v20_read,
	.write          = jz_aip_v20_write,
	.mmap           = jz_aip_v20_mmap,
	.unlocked_ioctl = jz_aip_v20_ioctl,
};

static int jz_aip_v20_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct jz_aip_v20_dev *aip;
	unsigned int __attribute__((unused)) value;


	aip = devm_kzalloc(dev, sizeof(*aip), GFP_KERNEL);
        if (!aip)
                return -ENOMEM;

	platform_set_drvdata(pdev, aip);

#ifdef CONFIG_AIP_V20_REPAIR_A1
	aip->clk_gate = clk_get(&pdev->dev, "gate_aip");
	if (IS_ERR(aip->clk_gate)) {
		return PTR_ERR(aip->io_regs);
	}
	clk_prepare_enable(aip->clk_gate);

	// A1 reset aip through cpm reg
	value = *(volatile unsigned int *)(0xb00000f0);
	value |= (1<<28);
	*(volatile unsigned int *)(0xb00000f0) = value;

	value = *(volatile unsigned int *)(0xb00000f0);
	value &= ~(1<<28);
	*(volatile unsigned int *)(0xb00000f0) = value;
#endif

	aip->io_regs = jz_aip_v20_ioremap_regs(pdev, 0);
	if (IS_ERR(aip->io_regs))
		return PTR_ERR(aip->io_regs);

	ret = jz_aip_v20_irq_init(pdev, aip);
	if (ret)
		return ret;

	aip->mdev.minor = MISC_DYNAMIC_MINOR;
	aip->mdev.fops = &jz_aip_v20_fops;
	aip->mdev.name = JZ_AIP_V20_DRV_NAME;

	ret = misc_register(&aip->mdev);
	if (ret < 0) {
		JZ_AIP_V20_ERROR("Misc device register fail!\n");
                return -ENOMEM;
	}

	mutex_init(&aip->struct_mutex);

	jz_aip_v20_bo_init(aip);
	jz_aip_v20_core_init(aip);

	jz_nna_v20_core_init(pdev, aip);

	JZ_AIP_V20_INFO("Initialized %s on minor %d", aip->mdev.name, aip->mdev.minor);

	return 0;
}

static int jz_aip_v20_remove(struct platform_device *pdev)
{
	struct jz_aip_v20_dev *aip = platform_get_drvdata(pdev);

	idr_destroy(&aip->bo_handles);
	misc_deregister(&aip->mdev);
	kfree(aip);

	return 0;
}

#ifdef CONFIG_PM
static int jz_aip_v20_suspend(struct device *dev)
{
	jz_aip_v20_clk_close;
	return 0;
}

static int jz_aip_v20_resume(struct device *dev)
{
	jz_aip_v20_clk_open;
	return 0;
}

static const struct dev_pm_ops jz_aip_v20_pm_ops = {
	.suspend = jz_aip_v20_suspend,
	.resume  = jz_aip_v20_resume,
};
#endif

static const struct of_device_id jz_aip_v20_dt_match[] = {
	{ .compatible = "ingenic,aip-v20", .data = NULL },
	{},
};

static struct platform_driver jz_aip_v20_driver = {
	.probe  = jz_aip_v20_probe,
	.remove = jz_aip_v20_remove,
	.driver = {
		.name   = "jz-aip-v20",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(jz_aip_v20_dt_match),
#ifdef CONFIG_PM
		.pm = &jz_aip_v20_pm_ops,
#endif
        },
};

static int __init jz_aip_v20_init(void)
{
	int ret;

	ret = platform_device_register(&jz_aip_v20_device);
	if (ret) {
		JZ_AIP_V20_ERROR("Device register fail!\n");
		return ret;
	}

	ret = platform_driver_register(&jz_aip_v20_driver);
	if (ret) {
		JZ_AIP_V20_ERROR("Driver register fail!\n");
		platform_device_unregister(&jz_aip_v20_device);
		return ret;
	}

	return 0;
}

static void __exit jz_aip_v20_exit(void)
{
	platform_driver_unregister(&jz_aip_v20_driver);
	platform_device_unregister(&jz_aip_v20_device);
	JZ_AIP_V20_DEBUG("b-scaler driver unloaded\n");
}

module_init(jz_aip_v20_init);
module_exit(jz_aip_v20_exit);

MODULE_DESCRIPTION("JZ AIP Driver Ver2.0");
MODULE_AUTHOR("LiuTianyang <rick.tyliu@ingenic.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20230403");
