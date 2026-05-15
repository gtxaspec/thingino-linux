#include <linux/mmc/host.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <mach/jzmmc.h>

#include "board_base.h"

int bcm_wlan_init(void);

static struct card_gpio tf_gpio = {
	.wp	    	= {GPIO_MMC_WP_N,	GPIO_MMC_WP_N_LEVEL},
	.cd			= {GPIO_MMC_CD_N,	GPIO_MMC_CD_N_LEVEL},
	.pwr		= {GPIO_MMC_PWR,	GPIO_MMC_PWR_LEVEL},
	.rst		= {GPIO_MMC_RST_N,	GPIO_MMC_RST_N_LEVEL},
};

/* common pdata for both tf_card and sdio wifi on fpga board */
struct jzmmc_platform_data tf_pdata = {
	.removal  			= REMOVABLE_ONDEMAND,
	.sdio_clk			= 0,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA,
	.pm_flags			= 0,
	.recovery_info			= NULL,
	.gpio				= &tf_gpio,
	.max_freq                       = 24000000,
	.pio_mode                       = 0,
};

 struct jzmmc_platform_data sdio_pdata = {
	 .removal  			= MANUAL,
	 .sdio_clk			= 1,
	 .ocr_avail			= MMC_VDD_29_30 | MMC_VDD_30_31,
	 .capacity  			= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ | MMC_CAP_NONREMOVABLE,
	 .max_freq                      = 24000000,
	 .recovery_info			= NULL,
	 .gpio				= NULL,
	 .pio_mode			= 0,
	 .private_init			= bcm_wlan_init,
 };
