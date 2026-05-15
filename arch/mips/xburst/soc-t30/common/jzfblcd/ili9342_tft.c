/*
 * Copyright (C) 2016 Ingenic Semiconductor Inc.
 *
 * This program is free software, you can redistribute it and/or modify it
 *
 * under the terms of the GNU General Public License version 2 as published by
 *
 * the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/digital_pulse_backlight.h>
#include <linux/delay.h>

#include <mach/jzfb.h>
#include <soc/gpio.h>
#include "../../chip-t30/isvp/Monkey/board.h"

#define ILI9342_LCD_CS GPIO_PD(9)
#define ILI9342_LCD_DAT GPIO_PC(27)
#define ILI9342_LCD_CLK GPIO_PC(28)

#define ILI9342_LCD_RESET GPIO_PD(27)

static int ili9342_lcdcs_request(void)
{
	static int gpio_is_inited = 0;
	int ret = 0;

	if(!gpio_is_inited) {
		ret = gpio_request_one(ILI9342_LCD_CS, GPIOF_OUT_INIT_HIGH, "ili9342_cs");
		if(ret < 0) {
			printk("gpio_request_one gpio_cs failed\n");
			return -1;
		}
		gpio_is_inited = 1;
	}

	return 0;
}

static int ili9342_lcdclk_request(void)
{
	static int gpio_is_inited = 0;
	int ret = 0;

	if(!gpio_is_inited) {
		ret = gpio_request_one(ILI9342_LCD_CLK, GPIOF_OUT_INIT_HIGH, "ili9342_clk");
		if(ret < 0) {
			printk("gpio_request_one gpio_clk failed\n");
			return -1;
		}
		gpio_is_inited = 1;
	}

	return 0;
}

static int ili9342_lcdreset_request(void)
{
	static int gpio_is_inited = 0;
	int ret = 0;

	if(!gpio_is_inited) {
		ret = gpio_request_one(ILI9342_LCD_RESET, GPIOF_OUT_INIT_LOW, "ili9342_reset");
		if(ret < 0) {
			printk("gpio_request_one gpio_RESET failed\n");
			return -1;
		}
		gpio_is_inited = 1;
	}

	return 0;
}

static int ili9342_lcddat_request(void)
{
	static int gpio_is_inited = 0;
	int ret = 0;

	if(!gpio_is_inited) {
		ret = gpio_request_one(ILI9342_LCD_DAT, GPIOF_OUT_INIT_HIGH, "ili9342_dat");
		if(ret < 0) {
			printk("gpio_request_one gpio_dat failed\n");
			return -1;
		}
		gpio_is_inited = 1;
	}

	return 0;
}

static int ili9342_lcdclk_output(int is_high)
{
	if(is_high) {
		gpio_direction_output(ILI9342_LCD_CLK, 1);
	} else {
		gpio_direction_output(ILI9342_LCD_CLK, 0);
	}

	return 0;
}

static int ili9342_lcdcs_output(int is_high)
{
	if(is_high) {
		gpio_direction_output(ILI9342_LCD_CS, 1);
	} else {
		gpio_direction_output(ILI9342_LCD_CS, 0);
	}

	return 0;
}

static int ili9342_lcddat_output(int is_high)
{
	if(is_high) {
		gpio_direction_output(ILI9342_LCD_DAT, 1);
	} else {
		gpio_direction_output(ILI9342_LCD_DAT, 0);
	}

	return 0;
}

static int ili9342_lcdreset_output(int is_high)
{
	if(is_high) {
		gpio_direction_output(ILI9342_LCD_RESET, 1);
	} else {
		gpio_direction_output(ILI9342_LCD_RESET, 0);
	}

	return 0;
}

static struct smart_lcd_data_table ili9342_data_table[] = {
	{SMART_CONFIG_CMD, 0xC8},
	{SMART_CONFIG_PRM, 0xFF},
	{SMART_CONFIG_PRM, 0x93},
	{SMART_CONFIG_PRM, 0x42},

	{SMART_CONFIG_CMD, 0x36},
	{SMART_CONFIG_PRM, 0xC8},

	{SMART_CONFIG_CMD, 0x3A},
	{SMART_CONFIG_PRM, 0x66},

	{SMART_CONFIG_CMD, 0xC0},
	{SMART_CONFIG_PRM, 0x0F},
	{SMART_CONFIG_PRM, 0x0F},

	{SMART_CONFIG_CMD, 0xC1},
	{SMART_CONFIG_PRM, 0x01},

	{SMART_CONFIG_CMD, 0xC5},
	{SMART_CONFIG_PRM, 0xDB},

	{SMART_CONFIG_CMD, 0xB4},
	{SMART_CONFIG_PRM, 0x02},

	{SMART_CONFIG_CMD, 0xB7},
	{SMART_CONFIG_PRM, 0x07},

	{SMART_CONFIG_CMD, 0xE0},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x05},
	{SMART_CONFIG_PRM, 0x08},
	{SMART_CONFIG_PRM, 0x02},
	{SMART_CONFIG_PRM, 0x1A},
	{SMART_CONFIG_PRM, 0x0C},
	{SMART_CONFIG_PRM, 0x42},
	{SMART_CONFIG_PRM, 0x7A},
	{SMART_CONFIG_PRM, 0x54},
	{SMART_CONFIG_PRM, 0x08},
	{SMART_CONFIG_PRM, 0x0D},
	{SMART_CONFIG_PRM, 0x0C},
	{SMART_CONFIG_PRM, 0x23},
	{SMART_CONFIG_PRM, 0x25},
	{SMART_CONFIG_PRM, 0x0F},

	{SMART_CONFIG_CMD, 0xE1},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x29},
	{SMART_CONFIG_PRM, 0x2F},
	{SMART_CONFIG_PRM, 0x03},
	{SMART_CONFIG_PRM, 0x0F},
	{SMART_CONFIG_PRM, 0x05},
	{SMART_CONFIG_PRM, 0x42},
	{SMART_CONFIG_PRM, 0x55},
	{SMART_CONFIG_PRM, 0x53},
	{SMART_CONFIG_PRM, 0x06},
	{SMART_CONFIG_PRM, 0x0F},
	{SMART_CONFIG_PRM, 0x0C},
	{SMART_CONFIG_PRM, 0x38},
	{SMART_CONFIG_PRM, 0x3A},
	{SMART_CONFIG_PRM, 0x0F},

	{SMART_CONFIG_CMD, 0x26},
	{SMART_CONFIG_PRM, 0x01},

	{SMART_CONFIG_CMD, 0xB0},
	{SMART_CONFIG_PRM, 0xE0},

	{SMART_CONFIG_CMD, 0xF6},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x07},

	{SMART_CONFIG_CMD, 0x11},
	{SMART_CONFIG_UDELAY, 120000},

	{SMART_CONFIG_CMD, 0x29},
	{SMART_CONFIG_CMD, 0x2C},
};

static void ili9342_send_mcu_command(unsigned long cmd)
{
	unsigned int  cmd_value,j;
	cmd_value = (unsigned int)cmd;

	//printk("%s %lu\n", __func__, cmd);

	ili9342_lcdclk_output(0);
	ndelay(5);//500
	ili9342_lcdcs_output(0);

	for(j = 9; j > 0; j--) {
		if(((cmd_value >> (j - 1)) & 0x01)) {
			ili9342_lcddat_output(1);
		} else {
			ili9342_lcddat_output(0);
		}

		ndelay(1);//100
		ili9342_lcdclk_output(1);
		ndelay(2);//200
		ili9342_lcdclk_output(0);
		ndelay(1);//100
	}
	//udelay(500);
	ili9342_lcdcs_output(1);
	ili9342_lcddat_output(1);
	ili9342_lcdclk_output(1);
}

static void ili9342_send_mcu_prm(unsigned long data)
{
	unsigned int  data_value,j;
	data_value = (unsigned int)data;
	data_value |= 0x100;

	//printk("%s %lu\n", __func__, data);

	ili9342_lcdclk_output(0);
	ndelay(5);//500
	ili9342_lcdcs_output(0);

	for(j = 9; j > 0; j--) {
		if(((data_value >> (j - 1)) & 0x01)) {
			ili9342_lcddat_output(1);
		} else {
			ili9342_lcddat_output(0);
		}

		ndelay(1);//100
		ili9342_lcdclk_output(1);
		ndelay(2);//200
		ili9342_lcdclk_output(0);
		ndelay(1);//100
	}
	//udelay(500);
	ili9342_lcdcs_output(1);
	ili9342_lcddat_output(1);
	ili9342_lcdclk_output(1);
}

static int ili9342_mcu_init(void)
{
	struct smart_lcd_data_table *data_table = ili9342_data_table;
	uint32_t length_data_table = ARRAY_SIZE(ili9342_data_table);
	uint32_t i;

	//printk("%s mcu send config\n", __func__);

	ili9342_lcdcs_request();
	ili9342_lcdclk_request();
	ili9342_lcddat_request();
	ili9342_lcdreset_request();

	udelay(1);//100
	ili9342_lcdreset_output(0);
	udelay(1);//100
	ili9342_lcdreset_output(1);

	if(length_data_table && data_table) {
		for(i = 0; i < length_data_table; i++) {
			switch (data_table[i].type) {
			case SMART_CONFIG_PRM:
				ili9342_send_mcu_prm(data_table[i].value);
				break;
			case SMART_CONFIG_CMD:
				ili9342_send_mcu_command(data_table[i].value);
				break;
			case SMART_CONFIG_UDELAY:
				udelay(data_table[i].value);
				break;
			default:
				printk("Unknow Config data type\n");
				break;
			}
		}
	}

	return 0;
}

static int ili9342_power_on(struct lcdc_gpio_struct *gpio)
{
#ifdef LCD_DISPLAY_EN_GPIO
	printk("%s display on GPIO:%d\n", __func__, LCD_DISPLAY_EN_GPIO);
	gpio_request(LCD_DISPLAY_EN_GPIO, "lcd_display");
	gpio_direction_output(LCD_DISPLAY_EN_GPIO, 1);
#endif

	ili9342_mcu_init();
	return 0;
}

static int ili9342_power_off(struct lcdc_gpio_struct *gpio)
{
	return 0;
}

static struct lcd_callback_ops ili9342_ops = {
	.lcd_power_on_begin  = (void *)ili9342_power_on,
	.lcd_power_off_begin = (void *)ili9342_power_off,
};

static struct fb_videomode jzfb_ili9342_videomode = {
	.name = "320*240",
	.refresh = 60,
	.xres = 320,
	.yres = 240,
	.pixclock = KHZ2PICOS(12000),
	.left_margin = 2,
	.right_margin = 43,
	.upper_margin = 1,
	.lower_margin = 12,
	.hsync_len = 1,
	.vsync_len = 1,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct jzfb_tft_config ili9342_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_SERIAL_RGB,
};

static struct lcdc_gpio_struct gpio_assign = {
	.func_pins = (0x3f << 2 | 0x3 << 8 | 0x3 << 18),
	.func_port = GPIO_PORT_D,
	.func_num = GPIO_FUNC_0,
};

struct jzfb_platform_data jzfb_data[] = {
	{
	.num_modes = 1,
	.modes = &jzfb_ili9342_videomode,
	.lcd_type = LCD_TYPE_TFT,
	.bpp = 24,
	.width = 320,
	.height = 240,

	.tft_config = &ili9342_cfg,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.gpio_assign = &gpio_assign,
	.lcd_callback_ops = &ili9342_ops,
	},{
	.num_modes = 2,
	.modes = &jzfb_ili9342_videomode,
	.lcd_type = LCD_TYPE_TFT,
	.bpp = 24,
	.width = 320,
	.height = 240,
	
	.tft_config = &ili9342_cfg,
	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.gpio_assign = &gpio_assign,
	.lcd_callback_ops = &ili9342_ops,

	},
};

/**************************************************************************************************/

#ifdef CONFIG_BACKLIGHT_PWM
static int backlight_init(struct device *dev)
{
	return 0;
}

static void backlight_exit(struct device *dev)
{
}

static struct platform_pwm_backlight_data jz_backlight_data = {
	.pwm_id		= 5,
	.max_brightness	= 255,
	.dft_brightness	= 120,
	.pwm_period_ns	= 30000,
	.init		= backlight_init,
	.exit		= backlight_exit,
};

struct platform_device jz_backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.platform_data	= &jz_backlight_data,
	},
};

#endif

/***********************************************************************************************/
