#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <plat/hardware.h>
#include <linux/time.h>
#include <linux/io.h>
#include <mach/nvt_type.h>
//#include "nvt_pd.h"
#include <linux/delay.h>

#define PD_DRV_VERSION "1.00.000"
#define INREG32(x)                      (*((volatile UINT32*)(x)))
#define OUTREG32(x, y)                  (*((volatile UINT32*)(x)) = (y))
#define USB_PD_DONE_BIT 7

static UINT32 USB_PHY_BASE_ADDR = 0;

static void usb_pd(void)
{
	USB_PHY_BASE_ADDR = (UINT32)ioremap_nocache(NVT_USB1_BASE_PHYS, 0x4000);
	if (INREG32(USB_PHY_BASE_ADDR + 0x1000) & (0x1 << USB_PD_DONE_BIT)) { //phy reg 0x0[7] will be used to determined if USB power save mode already done or not.
		//already enter usb_pd
	} else {
		printk("do usb_pd\n");
		OUTREG32(USB_PHY_BASE_ADDR + 0x10D0, 0x000000FF);
		mdelay(10);
		OUTREG32(USB_PHY_BASE_ADDR + 0x10D4, 0x00000034);
		mdelay(10);
		OUTREG32(USB_PHY_BASE_ADDR + 0x1008, 0x00000014);
		mdelay(10);
		OUTREG32(USB_PHY_BASE_ADDR + 0x1000, 0x00000088);
		mdelay(10);
		OUTREG32(USB_PHY_BASE_ADDR + 0x1030, 0x00000035);
		mdelay(10);
		OUTREG32(USB_PHY_BASE_ADDR + 0x102C, 0x000000FF);
		mdelay(10);
		OUTREG32(USB_PHY_BASE_ADDR + 0x1028, 0x00000020);
		mdelay(10);
		OUTREG32(USB_PHY_BASE_ADDR + 0x1024, 0x00000093);
	}
}

int __init nvt_pd_init(void)
{
	int ret = 0;
	printk("NVT_PD version:%s\n", PD_DRV_VERSION);
	usb_pd();

	return ret;
}

void __exit nvt_pd_exit(void)
{
}

module_init(nvt_pd_init);
module_exit(nvt_pd_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt pd");
MODULE_LICENSE("Proprietary");
