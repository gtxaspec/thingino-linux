/*
 * ingenic_bsp/chip-x2000/fpga/dpu/st7280.c
 *
 * Copyright (C) 2016 Ingenic Semiconductor Inc.
 *
 * Author:clwang<chunlei.wang@ingenic.com>
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

#include <mach/jzfb.h>
#include <soc/gpio.h>

static int st7280_power_on(struct lcdc_gpio_struct *gpio)
{
#ifdef LCD_DISPLAY_EN_GPIO
	gpio_request(LCD_DISPLAY_EN_GPIO, "lcd_display");
	gpio_direction_output(LCD_DISPLAY_EN_GPIO, 1);
#endif

//#ifdef LCD_BACKLIGHT_EN_GPIO
//	gpio_request(LCD_BACKLIGHT_EN_GPIO, "lcd_backlight");
//	gpio_direction_output(LCD_BACKLIGHT_EN_GPIO, 1);
//#endif


	printk("%s,%s,%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

static int st7280_power_off(struct lcdc_gpio_struct *gpio)
{
	return 0;
}

static struct lcd_callback_ops st7280_ops = {
	.lcd_power_on_begin  = (void*)st7280_power_on,
	.lcd_power_off_begin = (void*)st7280_power_off,
};

static struct fb_videomode jzfb_st7280_videomode = {
	.name = "480x272",
	.refresh = 79.77,
	.xres = 480,
	.yres = 272,
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

static struct jzfb_tft_config st7280_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_24B,
};

static struct lcdc_gpio_struct gpio_assign = {
	.func_pins = (0xff << 2 | 0xff << 12 | 0x3f << 22),
	.func_port = GPIO_PORT_D,
	.func_num = GPIO_FUNC_0,
};

struct jzfb_platform_data jzfb_data[] = {
	{
	.num_modes = 1,
	.modes = &jzfb_st7280_videomode,
	.lcd_type = LCD_TYPE_TFT,
	.bpp = 24,
	.width = 480,
	.height = 272,

	.tft_config = &st7280_cfg,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.gpio_assign = &gpio_assign,
	.lcd_callback_ops = &st7280_ops,
	},{
	.num_modes = 2,
	.modes = &jzfb_st7280_videomode,
	.lcd_type = LCD_TYPE_TFT,
	.bpp = 24,
	.width = 480,
	.height = 272,
	
	.tft_config = &st7280_cfg,
	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.gpio_assign = &gpio_assign,
	.lcd_callback_ops = &st7280_ops,

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
//	gpio_free(LCD_BACKLIGHT_EN_GPIO);
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
