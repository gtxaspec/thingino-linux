/*
 * Platform device support for Jz4780 SoC.
 *
 * Copyright 2007, <zpzhong@ingenic.cn>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/i2c-gpio.h>

#include <soc/gpio.h>
#include <soc/base.h>
#include <dt-bindings/interrupt-controller/t23-irq.h>

#include <soc/platform.h>
#include <soc/txx-funcs.h>
extern unsigned long ispmem_base;
extern unsigned long ispmem_size;



int jz_device_register(struct platform_device *pdev, void *pdata)
{
	pdev->dev.platform_data = pdata;

	return platform_device_register(pdev);
}

static void get_isp_priv_mem(unsigned int *phyaddr, unsigned int *size)
{
	*phyaddr = ispmem_base;
	*size = ispmem_size;
}

#ifndef CONFIG_PROC_FS
#error NOT config procfs
#endif

static struct task_struct *mythread_run(int (*threadfn)(void *data),
					   void *data, const char namefmt[])
{
	return kthread_run(threadfn, data, namefmt);
}

static void *mykmalloc(size_t s, gfp_t gfp)
{
	return kmalloc(s, gfp);
}

static void mykfree(void *p)
{
	return kfree(p);
}

static int myseq_printf(struct seq_file *m, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int r = 0;
	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	r = seq_printf(m, "%pV", &vaf);
	va_end(args);
	return r;
}

static long my_copy_from_user(void *to, const void __user *from, long size)
{
	return copy_from_user(to, from, size);
}

static long my_copy_to_user(void __user *to, const void *from, long size)
{
	return copy_to_user(to, from, size);
}


static int my_wait_event_interruptible(wait_queue_head_t *q, int (*state)(void *data), void *data)
{
	return wait_event_interruptible((*q), state(data));
}

static void my_wake_up_all(wait_queue_head_t *q)
{
	wake_up_all(q);
}

static void my_wake_up(wait_queue_head_t *q)
{
	wake_up(q);
}

static void my_init_waitqueue_head(wait_queue_head_t *q)
{
	init_waitqueue_head(q);
}

static struct resource * private_request_mem_region(resource_size_t start, resource_size_t n,
				   const char *name)
{
	return request_mem_region(start, n, name);
}

static void private_release_mem_region(resource_size_t start, resource_size_t n)
{
	release_mem_region(start, n);
}

static inline void __iomem * private_ioremap(phys_t offset, unsigned long size)
{
	return ioremap(offset, size);
}

static void private_spin_lock_irqsave(spinlock_t *lock, unsigned long *flags)
{
	_raw_spin_lock_irqsave(spinlock_check(lock), *flags);
}

static void private_spin_lock_init(spinlock_t *lock)
{
	spin_lock_init(lock);
}

static void private_raw_mutex_init(struct mutex *lock, const char *name, struct lock_class_key *key)
{
	__mutex_init(lock, name, key);
}

static int private_request_module(bool wait, const char *fmt, ...)
{
	int ret = 0;
	struct va_format vaf;
	va_list args;
	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	ret =  __request_module(true,"%pV", &vaf);
	va_end(args);
	return ret;
}

static struct sk_buff *private_nlmsg_new(size_t payload, gfp_t flags)
{
	return nlmsg_new(payload, flags);
}

static struct nlmsghdr *private_nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq,
					 int type, int payload, int flags)
{
	return nlmsg_put(skb, portid, seq, type, payload, flags);
}


static struct sock *private_netlink_kernel_create(struct net *net, int unit, struct netlink_kernel_cfg *cfg)
{
	return netlink_kernel_create(net, unit, cfg);
}


static mm_segment_t private_get_fs(void)
{
	return get_fs();
}

static void private_set_fs(mm_segment_t val)
{
	set_fs(val);
}

extern struct net init_net;
static struct net *private_get_init_net(void)
{
	return &init_net;
}

static struct jz_driver_common_interfaces private_funcs = {
	.flags_0 = (unsigned int)&(printk),
	/* platform interface */
	//.priv_platform_driver_register = __platform_driver_register,
	.priv_platform_driver_unregister = platform_driver_unregister,
	.priv_platform_set_drvdata = platform_set_drvdata,
	.priv_platform_get_drvdata = platform_get_drvdata,
	.priv_platform_device_register = platform_device_register,
	.priv_platform_device_unregister = platform_device_unregister,
	.priv_platform_get_resource = platform_get_resource,
	.priv_dev_set_drvdata = dev_set_drvdata,
	.priv_dev_get_drvdata = dev_get_drvdata,
	.priv_platform_get_irq = platform_get_irq,
	.priv_request_mem_region = private_request_mem_region,
	.priv_release_mem_region = private_release_mem_region,
	.priv_ioremap = private_ioremap,
	.priv_iounmap = iounmap,

	/* interrupt interface */
	.priv_request_threaded_irq = request_threaded_irq,
	.priv_enable_irq = enable_irq,
	.priv_disable_irq = disable_irq,
	.priv_free_irq = free_irq,

	/* lock and mutex interface */
	.priv_spin_unlock_irqrestore = spin_unlock_irqrestore,
	.priv_mutex_lock = mutex_lock,
	.priv_mutex_unlock = mutex_unlock,
	.priv_spin_lock_irqsave = private_spin_lock_irqsave,
	.priv_spin_lock_init = private_spin_lock_init,
	.priv_raw_mutex_init = private_raw_mutex_init,

	/* clock interfaces */
	.priv_clk_get = clk_get,
	.priv_clk_enable = clk_prepare_enable,
	//.priv_clk_is_enabled = clk_is_enabled,
	.priv_clk_disable = clk_disable_unprepare,
	.priv_clk_get_rate = clk_get_rate,
	.priv_clk_put = clk_put,
	.priv_clk_set_rate = clk_set_rate,

	/* i2c interfaces */
	.priv_i2c_get_adapter = i2c_get_adapter,
	.priv_i2c_put_adapter = i2c_put_adapter,
	.priv_i2c_transfer = i2c_transfer,
	.priv_i2c_register_driver = i2c_register_driver,
	.priv_i2c_del_driver = i2c_del_driver,

	.priv_i2c_new_device = i2c_new_device,
	.priv_i2c_get_clientdata = i2c_get_clientdata,
	.priv_i2c_set_clientdata = i2c_set_clientdata,
	.priv_i2c_unregister_device = i2c_unregister_device,

	/* gpio interfaces */
	.priv_gpio_request = gpio_request,
	.priv_gpio_free = gpio_free,
	.priv_gpio_direction_output = gpio_direction_output,
	.priv_gpio_direction_input = gpio_direction_input,
	.priv_gpio_set_debounce = gpio_set_debounce,
	.priv_jzgpio_set_func = jzgpio_set_func,
	//.priv_jzgpio_ctrl_pull = jzgpio_ctrl_pull,

	/* system interface */
	.priv_msleep = msleep,
	.priv_capable = capable,
	.priv_sched_clock = sched_clock,
	.priv_try_module_get = try_module_get,
	.priv_request_module = private_request_module,
	.priv_module_put = module_put,

	/* wait */
	.priv_init_completion = init_completion,
	.priv_complete = complete,
	.priv_wait_for_completion_interruptible = wait_for_completion_interruptible,
	.priv_wait_event_interruptible = my_wait_event_interruptible,
	.priv_wake_up_all = my_wake_up_all,
	.priv_wake_up = my_wake_up,
	.priv_init_waitqueue_head = my_init_waitqueue_head,
	.priv_wait_for_completion_timeout = wait_for_completion_timeout,

	/* misc */
	.priv_misc_register = misc_register,
	.priv_misc_deregister = misc_deregister,
	.priv_proc_create_data = proc_create_data,
	/* proc */
	.priv_seq_read = seq_read,
	.priv_seq_lseek = seq_lseek,
	.priv_single_release = single_release,
	.priv_single_open_size = single_open_size,
	.priv_jz_proc_mkdir = jz_proc_mkdir,
	.priv_proc_remove = proc_remove,
	.priv_seq_printf = myseq_printf,
	.priv_simple_strtoull = simple_strtoull,

	/* kthread */
	.priv_kthread_should_stop = kthread_should_stop,
	.priv_kthread_run = mythread_run,
	.priv_kthread_stop = kthread_stop,

	.priv_kmalloc = mykmalloc,
	.priv_kfree = mykfree,
	.priv_copy_from_user = my_copy_from_user,
	.priv_copy_to_user = my_copy_to_user,

	/* netlink */
	.priv_nlmsg_new = private_nlmsg_new,
	.priv_nlmsg_put = private_nlmsg_put,
	.priv_netlink_unicast = netlink_unicast,
	.priv_netlink_kernel_create = private_netlink_kernel_create,
	.priv_sock_release = sock_release,

	/* filp */
	.priv_filp_open = filp_open,
	.priv_filp_close = filp_close,
	.priv_get_fs = private_get_fs,
	.priv_set_fs = private_set_fs,
	.priv_vfs_read = vfs_read,
	.priv_vfs_write = vfs_write,
	.priv_vfs_llseek = vfs_llseek,
	.priv_dma_cache_sync = dma_cache_sync,

	.priv_getrawmonotonic = getrawmonotonic,
	.priv_get_init_net = private_get_init_net,
	/* isp driver interface */
	.get_isp_priv_mem = get_isp_priv_mem,
	.flags_1 = (unsigned int)&(printk),
};

void *get_driver_common_interfaces(void)
{
	return (void *)&private_funcs;
}

EXPORT_SYMBOL(get_driver_common_interfaces);
