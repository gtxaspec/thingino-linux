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
