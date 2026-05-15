/*
 * Video Class definitions of Tomahawk series SoC.
 *
 * Copyright 2017, <xianghui.shen@ingenic.com>
 *
 * This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/bug.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>

extern int tx_isp_init(void);
extern void tx_isp_exit(void);

unsigned long ir_switch_mode = 2;
EXPORT_SYMBOL(ir_switch_mode);

unsigned long ir_threshold_min = 2000;
EXPORT_SYMBOL(ir_threshold_min);

unsigned long ir_threshold_max = 2500;
EXPORT_SYMBOL(ir_threshold_max);

unsigned long frame_channel_width = 0;
EXPORT_SYMBOL(frame_channel_width);

unsigned long frame_channel_height = 0;
EXPORT_SYMBOL(frame_channel_height);

unsigned long frame_channel_nrvbs = 0;
EXPORT_SYMBOL(frame_channel_nrvbs);

long g_day_ae_val = 0;
EXPORT_SYMBOL(g_day_ae_val);

long g_night_ae_val = 0;
EXPORT_SYMBOL(g_night_ae_val);

//long g_ae_coeff = 0;
//EXPORT_SYMBOL(g_ae_coeff);

long g_wb_r = 0;
EXPORT_SYMBOL(g_wb_r);

long g_wb_b = 0;
EXPORT_SYMBOL(g_wb_b);

static int __init ir_switch_parse(char *str)
{
	char *p = NULL;

	p = strstr(str, "ir_mode=");
	if(p != NULL) {
		if(strncmp((p+strlen("ir_mode=")), "off", 3) == 0) {
			ir_switch_mode = 0;
		} else if(strncmp((p+strlen("ir_mode=")), "on", 2) == 0) {
			ir_switch_mode = 1;
		} else if(strncmp((p+strlen("ir_mode=")), "auto", 4) == 0) {
			ir_switch_mode = 2;
		} else {
			printk("Invalid ir_mode info\n");
		}
	}

	p = strstr(str, "min=");
	if(p != NULL) {
		ir_threshold_min = simple_strtoul(p, NULL, 10);
	}

	p = strstr(str, "max=");
	if(p != NULL) {
		ir_threshold_max = simple_strtoul(p, NULL, 10);
	}

	printk("%s mode: %lu threshold min:%lu max:%lu\n", __func__, ir_switch_mode, ir_threshold_min, ir_threshold_max);

	p = strstr(str, "init_vw=");
	if(p != NULL) {
		frame_channel_width = simple_strtoul(p+strlen("init_vw="), NULL, 10);
	}

	p = strstr(str, "init_vh=");
	if(p != NULL) {
		frame_channel_height = simple_strtoul(p+strlen("init_vh="), NULL, 10);
	}

	p = strstr(str, "nrvbs=");
	if(p != NULL) {
		frame_channel_nrvbs = simple_strtoul(p+strlen("nrvbs="), NULL, 10);
	}

	printk("%s width:%lu height:%lu nrvbs:%lu\n", __func__, frame_channel_width, frame_channel_height, frame_channel_nrvbs);

	p = strstr(str, "dayEV=");
	if(p != NULL) {
		g_day_ae_val = simple_strtol(p+strlen("dayEV="), NULL, 10);
	}

	p = strstr(str, "nightEV=");
	if(p != NULL) {
		g_night_ae_val = simple_strtol(p+strlen("nightEV="), NULL, 10);
	}

//	p = strstr(str, "coeff=");
//	if(p != NULL) {
//		g_ae_coeff = simple_strtol(p+strlen("coeff="), NULL, 10);
//	}

	p = strstr(str, "wbr=");
	if(p != NULL) {
		g_wb_r = simple_strtol(p+strlen("wbr="), NULL, 10);
	}

	p = strstr(str, "wbb=");
	if(p != NULL) {
		g_wb_b = simple_strtol(p+strlen("wbb="), NULL, 10);
	}

	printk("%s dayEv:%ld nightEv:%ld wbr:%ld wbb:%ld\n", __func__, g_day_ae_val, g_night_ae_val, g_wb_r, g_wb_b);

	return 1;
}
__setup("senv", ir_switch_parse);

static int __init tx_isp_t21_init(void)
{
	return tx_isp_init();
}

static void __exit tx_isp_t21_exit(void)
{
	tx_isp_exit();
}

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
subsys_initcall(tx_isp_t21_init);
#else
module_init(tx_isp_t21_init);
#endif
module_exit(tx_isp_t21_exit);

MODULE_AUTHOR("Ingenic xhshen");
MODULE_DESCRIPTION("tx isp driver");
MODULE_LICENSE("GPL");
