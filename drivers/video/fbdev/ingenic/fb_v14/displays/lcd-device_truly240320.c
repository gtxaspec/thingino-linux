/*
 * Copyright (c) 2016 Engenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * JZ-T20 orion board lcd setup routines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/lcd.h>
#include "../include/jzfb.h"

/******GPIO PIN************/
#undef GPIO_LCD_CS
#undef GPIO_LCD_RD
#undef GPIO_BL_PWR_EN
#if defined(CONFIG_SOC_PRJ007)
#define GPIO_LCD_CS			58
#define GPIO_BL_PWR_EN			60
#define GPIO_LCD_RST			59
#elif defined(CONFIG_SOC_PRJ008)
#define GPIO_LCD_CS			58
#define GPIO_BL_PWR_EN			62
#define GPIO_LCD_RST			63
#endif
/* not use */
/* #define GPIO_LCD_RD            108 */


/*ifdef is 18bit,6-6-6 ,ifndef default 5-6-5*/
/* #define CONFIG_SLCD_ILI9488_18BIT */

#ifdef	CONFIG_SLCD_TRULY_18BIT
static int slcd_inited = 1;
#else
static int slcd_inited = 0;
#endif

struct truly_tft240320_power{
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct truly_tft240320_power lcd_power = {
	NULL,
	NULL,
	0
};

int truly_tft240320_power_init(struct lcd_device *ld)
{
	int ret ;
	/* printk("======truly_tft240320_power_init==============\n"); */

	ret = gpio_request(GPIO_LCD_RST, "lcd rst");
	if (ret) {
		printk(KERN_ERR "can's request lcd rst\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_CS, "lcd cs");
	if (ret) {
		printk(KERN_ERR "can's request lcd cs\n");
		return ret;
	}
/*
	ret = gpio_request(GPIO_LCD_CS2, "lcd cs2");
	if (ret) {
		printk(KERN_ERR "can's request lcd cs2\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_RD, "lcd rd");
	if (ret) {
		printk(KERN_ERR "can's request lcd rd\n");
		return ret;
	}
*/

	lcd_power.inited = 1;
	return 0;
}

int truly_tft240320_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;

	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(100);
	gpio_direction_output(GPIO_LCD_RST, 0);
	mdelay(200);
	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(100);
	return 0;
}

int truly_tft240320_power_on(struct lcd_device *ld, int enable)
{
	if (!lcd_power.inited && truly_tft240320_power_init(ld))
		return -EFAULT;

	if (enable) {
		gpio_direction_output(GPIO_LCD_CS, 1);
		/* gpio_direction_output(GPIO_LCD_CS2, 1); */
		/* gpio_direction_output(GPIO_LCD_RD, 1); */
		truly_tft240320_power_reset(ld);
		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		/* gpio_direction_output(GPIO_LCD_CS2, 0); */

	} else {
		gpio_direction_output(GPIO_BL_PWR_EN, 0);
		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		/* gpio_direction_output(GPIO_LCD_CS2, 0); */
		/* gpio_direction_output(GPIO_LCD_RD, 0); */
		gpio_direction_output(GPIO_LCD_RST, 0);
		slcd_inited = 0;
	}
	return 0;
}

struct lcd_platform_data truly_tft240320_pdata = {
	.reset    = truly_tft240320_power_reset,
	.power_on = truly_tft240320_power_on,
};

/* LCD Panel Device */
struct platform_device truly_tft240320_device = {
	.name		= "truly_tft240320_slcd",
	.dev		= {
		.platform_data	= &truly_tft240320_pdata,
	},
};

static struct smart_lcd_data_table truly_tft240320_data_table[] = {
	{SMART_CONFIG_CMD, 0xfe},

	{SMART_CONFIG_CMD, 0xef},

	{SMART_CONFIG_CMD, 0x36},	//rgb format
	{SMART_CONFIG_DATA, 0x40},   //5-6-5

	{SMART_CONFIG_CMD, 0x3a},	//row
	{SMART_CONFIG_DATA, 0x05},

	{SMART_CONFIG_CMD, 0x21},

	{SMART_CONFIG_CMD, 0x86},
	{SMART_CONFIG_DATA, 0x98},

	{SMART_CONFIG_CMD, 0x89},
	{SMART_CONFIG_DATA, 0x03},

	{SMART_CONFIG_CMD, 0x8b},
	{SMART_CONFIG_DATA, 0x84},

	{SMART_CONFIG_CMD, 0x8d},
	{SMART_CONFIG_DATA, 0x33},

	{SMART_CONFIG_CMD, 0x8e},
	{SMART_CONFIG_DATA, 0x0f},

	{SMART_CONFIG_CMD, 0xe8},
	{SMART_CONFIG_DATA, 0x12},
	{SMART_CONFIG_DATA, 0x00},

	{SMART_CONFIG_CMD, 0xff},
	{SMART_CONFIG_DATA, 0x62},

	{SMART_CONFIG_CMD, 0x99},
	{SMART_CONFIG_DATA, 0x3e},

	{SMART_CONFIG_CMD, 0x9d},
	{SMART_CONFIG_DATA, 0x4b},

	{SMART_CONFIG_CMD, 0x98},
	{SMART_CONFIG_DATA, 0x3e},

	{SMART_CONFIG_CMD, 0x9c},
	{SMART_CONFIG_DATA, 0x4b},

	{SMART_CONFIG_CMD, 0xc3},
	{SMART_CONFIG_DATA, 0x30},

	{SMART_CONFIG_CMD, 0xc4},
	{SMART_CONFIG_DATA, 0x0b},

	{SMART_CONFIG_CMD, 0xc9},
	{SMART_CONFIG_DATA, 0x00},

	{SMART_CONFIG_CMD, 0xf0},
	{SMART_CONFIG_DATA, 0x88},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x1f},
	{SMART_CONFIG_DATA, 0x16},
	{SMART_CONFIG_DATA, 0x0b},
	{SMART_CONFIG_DATA, 0x3b},

	{SMART_CONFIG_CMD, 0xf1},
	{SMART_CONFIG_DATA, 0x4f},
	{SMART_CONFIG_DATA, 0xb7},
	{SMART_CONFIG_DATA, 0x97},
	{SMART_CONFIG_DATA, 0x25},
	{SMART_CONFIG_DATA, 0x29},
	{SMART_CONFIG_DATA, 0xbf},

	{SMART_CONFIG_CMD, 0xf2},
	{SMART_CONFIG_DATA, 0x4b},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x19},
	{SMART_CONFIG_DATA, 0x0d},
	{SMART_CONFIG_DATA, 0x39},

	{SMART_CONFIG_CMD, 0xf3},
	{SMART_CONFIG_DATA, 0x4a},
	{SMART_CONFIG_DATA, 0xf5},
	{SMART_CONFIG_DATA, 0xb4},
	{SMART_CONFIG_DATA, 0x1d},
	{SMART_CONFIG_DATA, 0x21},
	{SMART_CONFIG_DATA, 0xcf},

	{SMART_CONFIG_CMD, 0x35},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x44},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x0a},

	{SMART_CONFIG_CMD, 0x11},
	{SMART_CONFIG_UDELAY, 120000},

	{SMART_CONFIG_CMD, 0x29},
	{SMART_CONFIG_CMD, 0x2c},
};

unsigned long truly_cmd_buf[]= {
	0x2C2C2C2C,
};

struct fb_videomode jzfb0_videomode = {
	.name = "240x320",
	.refresh = 60,
	.xres = 240,
	.yres = 320,
	.pixclock = KHZ2PICOS(30000),
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzfb_platform_data jzfb_pdata = {
	.num_modes = 1,
	.modes = &jzfb0_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp    = 16,
//	.bpp    = 18,
	.width = 240,
	.height = 320,
	.pinmd  = 0,
	/* rsply_cmd_high setting of LCDC should correspond with LCD.  */
	.smart_config.rsply_cmd_high       = 0,
	.smart_config.csply_active_high    = 0,
	.smart_config.cfg_fmt_conv =  1,
	/* write graphic ram command, in word, for example 8-bit bus, write_gram_cmd=C3C2C1C0. */
	.smart_config.write_gram_cmd = truly_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(truly_cmd_buf),
	.smart_config.bus_width = 8,
	.smart_config.length_data_table =  ARRAY_SIZE(truly_tft240320_data_table),
	.smart_config.data_table = truly_tft240320_data_table,
	.dither_enable = 0,
};
/**************************************************************************************************/
#ifdef CONFIG_BACKLIGHT_PWM
static int backlight_init(struct device *dev)
{
	int ret;
	ret = gpio_request(GPIO_LCD_PWM, "Backlight");
	if (ret) {
		printk(KERN_ERR "failed to request GPF for PWM-OUT1\n");
		return ret;
	}

	ret = gpio_request(GPIO_BL_PWR_EN, "BL PWR");
	if (ret) {
		printk(KERN_ERR "failed to reqeust BL PWR\n");
		return ret;
	}
	gpio_direction_output(GPIO_BL_PWR_EN, 1);
	return 0;
}

static int backlight_notify(struct device *dev, int brightness)
{
	if (brightness)
		gpio_direction_output(GPIO_BL_PWR_EN, 1);
	else
		gpio_direction_output(GPIO_BL_PWR_EN, 0);

	return brightness;
}

static void backlight_exit(struct device *dev)
{
	gpio_free(GPIO_LCD_PWM);
}

static struct platform_pwm_backlight_data backlight_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 120,
	.pwm_period_ns	= 30000,
	.init		= backlight_init,
	.exit		= backlight_exit,
	.notify		= backlight_notify,
};

struct platform_device backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.platform_data	= &backlight_data,
	},
};
#endif
