#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>
#include <linux/delay.h>

#include "board_base.h"

struct wifi_data {
	struct wake_lock                wifi_wake_lock;
	int                             wifi_reset;
};

struct resource wlan_resources[] = {
	[0] = {
		.start = WL_WAKE_HOST,
		.end = WL_WAKE_HOST,
		.name = "bcmdhd_wlan_irq",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};
struct platform_device wlan_device = {
	.name   = "bcmdhd_wlan",
	.id     = 1,
	.dev    = {
		.platform_data = NULL,
	},
	.resource       = wlan_resources,
	.num_resources  = ARRAY_SIZE(wlan_resources),
};

int bcm_wlan_init(void)
{
#if defined (WL_REG_EN) && (WL_REG_EN > 0)
	gpio_request_one(WL_REG_EN, GPIOF_OUT_INIT_LOW, "wl_reg_on");
#endif
	gpio_request(WL_WAKE_HOST, "oob irq");

	return 0;
}
EXPORT_SYMBOL(bcm_wlan_init);

int bcm_customer_wlan_get_oob_irq(void)
{
  int host_oob_irq = 0;
  //printk("GPIO(WL_HOST_WAKE) = EXYNOS4_GPX0(7) = %d\n", GPIO_PA(9));
  host_oob_irq = gpio_to_irq(WL_WAKE_HOST);
  // gpio_direction_input(GPIO_PA(9));
  printk("host_oob_irq: %d \r\n", host_oob_irq);

  return host_oob_irq;
}
EXPORT_SYMBOL(bcm_customer_wlan_get_oob_irq);

int bcm_manual_detect(int on)
{
	return 0;
}
EXPORT_SYMBOL(bcm_manual_detect);

int bcm_wlan_power_on(int flag)
{
#if defined (WL_REG_EN) && (WL_REG_EN > 0)
	gpio_direction_output(WL_REG_EN, 1);
	usleep_range(1, 1000);
	gpio_direction_output(WL_REG_EN, 0);
	msleep(5);
#endif
	return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_on);

int bcm_wlan_power_off(int flag)
{
	return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_off);
