#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <mach/jzssi.h>
#include "board_base.h"

#if defined(CONFIG_SPI_GPIO)
static struct spi_gpio_platform_data jz4780_spi_gpio_data = {
	.sck	= GPIO_SPI_SCK,
	.mosi	= GPIO_SPI_MOSI,
	.miso	= GPIO_SPI_MISO,
	.num_chipselect	= 2,
};

static struct platform_device jz4780_spi_gpio_device = {
	.name	= "spi_gpio",
	.dev	= {
		.platform_data = &jz4780_spi_gpio_data,
	},
};
#endif

#ifdef CONFIG_JZ_SPI0
struct spi_board_info jz_spi0_board_info[] = {
	[0] = {
		.modalias       = "spidev",
		.bus_num	    = 0,
		.chip_select    = 0,
		.max_speed_hz   = 50000000,
		.mode           = SPI_MODE_0,
	},
};
int jz_spi0_board_info_size = ARRAY_SIZE(jz_spi0_board_info);
#endif

#ifdef CONFIG_JZ_SPI1
struct spi_board_info jz_spi1_board_info[] = {
	[0] = {
		.modalias       = "spidev",
		.bus_num        = 1,
		.chip_select    = 0,
		.max_speed_hz   = 50000000,
		.mode           = SPI_MODE_0,
	},
};
int jz_spi1_board_info_size = ARRAY_SIZE(jz_spi1_board_info);
#endif
