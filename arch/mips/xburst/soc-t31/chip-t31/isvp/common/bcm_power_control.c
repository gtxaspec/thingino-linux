#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>
#include <linux/delay.h>

#include "board_base.h"

int bcm_wlan_init(void)
{
#if defined (WL_REG_EN) && (WL_REG_EN > 0)
	gpio_request_one(WL_REG_EN, GPIOF_OUT_INIT_LOW, "wl_reg_on");
#endif

#if defined WL_SDIO_SW_EN
	gpio_request_one(WL_SDIO_SW_EN, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT, "sdio_sw_en");
#endif

#if defined WL_WAKE_HOST
	gpio_request(WL_WAKE_HOST, "oob irq");
#endif

	return 0;
}
EXPORT_SYMBOL(bcm_wlan_init);

int bcm_customer_wlan_get_oob_irq(void)
{
  int host_oob_irq = 0;

#if defined WL_WAKE_HOST
  host_oob_irq = gpio_to_irq(WL_WAKE_HOST);
#endif

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
#if defined(CONFIG_BOARD_INDUS_T31_QFN) || defined(CONFIG_BOARD_INDUS_T31_QFN_V2)
	// Set SDIO GPIO Pull-up
	*(volatile unsigned int *)0xb0011114 = 0x6f00;
#endif

#ifdef CONFIG_BOARD_INDUS_T31_BGA
	*(volatile unsigned int *)0xb0012134 = 0x20;
#endif

#if defined (WL_REG_EN) && (WL_REG_EN > 0)
	gpio_direction_output(WL_REG_EN, 1);
	usleep_range(1, 1000);
	gpio_direction_output(WL_REG_EN, 0);
	msleep(5);
#endif
#if defined WL_SDIO_SW_EN
	gpio_direction_output(WL_SDIO_SW_EN, 1);
#endif
	return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_on);

int bcm_wlan_power_off(int flag)
{
#if defined WL_SDIO_SW_EN
	gpio_direction_output(WL_SDIO_SW_EN, 0);
#endif
	return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_off);
