#include <linux/init.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <soc/cache.h>
#include <soc/base.h>
#include <asm/io.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#define ZBOOST_INFO_LEVEL          0x0
#define ZBOOST_WARNING_LEVEL       0x1
#define ZBOOST_ERROR_LEVEL         0x2
static int print_level = ZBOOST_INFO_LEVEL;

int __attribute__((weak)) ingenic_zboost_init(void)
{
	return 0;
}

int __attribute__((weak)) ingenic_zboost_exit(void)
{
	return 0;
}

int zb_platform_driver_register(struct platform_driver *drv)
{
	return platform_driver_register(drv);
}

void zb_platform_driver_unregister(struct platform_driver *drv)
{
	platform_driver_unregister(drv);
}

int zb_platform_device_register(struct platform_device *pdev)
{
	return platform_device_register(pdev);
}

void zb_platform_device_unregister(struct platform_device *pdev)
{
	platform_device_unregister(pdev);
}

int zboost_printf(unsigned int level, unsigned char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int r = 0;

	if(level >= print_level){
		va_start(args, fmt);
		vaf.fmt = fmt;
		vaf.va = &args;
		r = printk("%pV",&vaf);
		va_end(args);
		if(level >= ZBOOST_ERROR_LEVEL)
			dump_stack();
	}
	return r;
}
