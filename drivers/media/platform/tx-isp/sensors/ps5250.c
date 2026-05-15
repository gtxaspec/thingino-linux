/*
 * ps5250.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <sensor-common.h>
#include <apical-isp/apical_math.h>
#include <linux/proc_fs.h>
#include <soc/gpio.h>

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
#include "ps5250_setting.c"
#endif

#define PS5250_CHIP_ID_H	(0x52)
#define PS5250_CHIP_ID_L	(0x50)
#define PS5250_REG_END		0xff
#define PS5250_REG_DELAY	0xfe
#define PS5250_BANK_REG		0xef

#define PS5250_SUPPORT_PCLK (38000000)
#define SENSOR_OUTPUT_MAX_FPS 15
#define SENSOR_OUTPUT_MIN_FPS 5

#define SENSOR_WITHOUT_INIT

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_LOW_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	unsigned int value;
	unsigned int gain;
};

struct again_lut ps5250_again_lut[] = {
	{0, 0},
	{1, 5731},
	{2, 11136},
	{3, 16247},
	/* start frome 1.25x */
	{4, 21097},
	{5, 25710},
	{6, 30108},
	{7, 34311},
	{8, 38335},
	{9, 42195},
	{10, 45903},
	{11, 49471},
	{12, 52910},
	{13, 56227},
	{14, 59433},
	{15, 62533},
	{16, 65535},
	{17, 71266},
	{18, 76671},
	{19, 81782},
	{20, 86632},
	{21, 91245},
	{22, 95643},
	{23, 99846},
	{24, 103870},
	{25, 107730},
	{26, 111438},
	{27, 115006},
	{28, 118445},
	{29, 121762},
	{30, 124968},
	{31, 128068},
	{32, 131070},
	{33, 136801},
	{34, 142206},
	{35, 147317},
	{36, 152167},
	{37, 156780},
	{38, 161178},
	{39, 165381},
	{40, 169405},
	{41, 173265},
	{42, 176973},
	{43, 180541},
	{44, 183980},
	{45, 187297},
	{46, 190503},
	{47, 193603},
	{48, 196605},
	{49, 202336},
	{50, 207741},
	{51, 212852},
	{52, 217702},
	{53, 222315},
	{54, 226713},
	{55, 230916},
	{56, 234940},
	{57, 238800},
	{58, 242508},
	{59, 246076},
	{60, 249515},
	{61, 252832},
	{62, 256038},
	{63, 259138},
	{64, 262140},
	{65, 267871},
	{66, 273276},
	{67, 278387},
	{68, 283237},
	{69, 287850},
	{70, 292248},
	{71, 296451},
	{72, 300475},
	{73, 304335},
	{74, 308043},
	{75, 311611},
	{76, 315050},
	{77, 318367},
	{78, 321573},
	{79, 324673},
	{80, 327675},
};

struct tx_isp_sensor_attribute ps5250_attr;

unsigned int ps5250_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = ps5250_again_lut;
	while(lut->gain <= ps5250_attr.max_again) {
		if(isp_gain <= ps5250_again_lut[0].gain) {
			*sensor_again = lut[0].value;
			return lut[0].gain;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == ps5250_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int ps5250_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute ps5250_attr={
	.name = "ps5250",
	.chip_id = 0x5250,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x48,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 327675,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 1123,
	.integration_time_limit = 1123,
	.total_width = 2252,
	.total_height = 1125,
	.max_integration_time = 1123,
	.one_line_expr_in_us = 30,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = ps5250_alloc_again,
	.sensor_ctrl.alloc_dgain = ps5250_alloc_dgain,
};


static struct regval_list ps5250_init_regs_1920_1080_15fps[] = {

	{0xEF, 0x00},
	{0x11, 0x00},
	{0x81, 0x80},//62, 0811 setting, Cmd_tpxoicut=128
	{0x9E, 0x08},//0A, 0602 setting
	{0xA2, 0x30},
	{0xA3, 0x03},
	{0xBE, 0x15},/*For ISP Hsync*/
	{0xD9, 0x64},
	{0xDA, 0x12},
	{0xDB, 0x84},
	{0xDC, 0x31},//1B, 0602 setting
	{0xED, 0x01},
	{0xEF, 0x01},
	{0x02, 0xFF},
	{0x03, 0x03},
	{0x04, 0x10},
	{0x05, 0x01},
	{0x06, 0xFF},
	{0x07, 0x04},
	{0x08, 0x00},
	{0x09, 0x00},
	{0x0A, 0x04},
	{0x0B, 0x64},
	{0x0C, 0x00},
	{0x0D, 0x02},
	{0x0E, 0x01},
	{0x0F, 0x2C},
	{0x20, 0x04},
	{0x28, 0xCC},
	{0x29, 0x02},//0811 setting, Cmd_Treset=2
	{0x2E, 0x78},//5A, 0811 setting, Cmd_Tltgon=120
	{0x41, 0x0F},
	{0x42, 0xC8},
	{0x4A, 0x21},
	{0x4C, 0xBE},
	{0x52, 0xE8},
	{0x56, 0x0A},
	{0x60, 0x78},//5A, 0811 setting, Cmd_Tlnepls=120
	{0x7C, 0x38},//0E, 0602 setting
	{0x8B, 0x44},
	{0xA8, 0x06},
	{0xD1, 0x04}, //44, 0602 setting
	{0xD2, 0x54},
	{0xD3, 0x17}, //14, 0602 setting
	{0xD4, 0x00},
	{0xD5, 0x01},
	{0xD6, 0x00},
	{0xD7, 0x06},
	{0xD8, 0x5E},
	{0xD9, 0x66},
	{0xDA, 0x70},
	{0xDB, 0x70},
	{0xDC, 0x10},
	{0xDD, 0x72},//62, 0811 setting, T_vdda_lvl=7
	{0xDE, 0x43},
	{0xDF, 0x40},
	{0xE0, 0x42},
	{0xE1, 0x11},
	{0xE2, 0x6D},//1D, 0811 setting, T_pos_pump2_lvl=6
	{0xE3, 0x21},
	{0xE4, 0x60},
	{0xE6, 0x00},
	{0xE7, 0x00},
	{0xEA, 0x7A},
	{0xF0, 0x03},
	{0xF1, 0x16},
	{0xF2, 0x24},
	{0xF4, 0x06},
	{0xF5, 0x11},
	{0xF6, 0xC8},
	{0xF7, 0x02},
	{0xF8, 0x00},//40, 0811 setting, T_pos_pump2_lvl=6
	{0xF9, 0x15},
	{0xFA, 0x3D},
	{0xFB, 0x02},
	{0xFC, 0x28},
	{0xFD, 0x32},
	{0x09, 0x01},
	{0xEF, 0x02},
	{0x33, 0x85},
	{0xED, 0x01},
	{0xEF, 0x05},
	{0x0F, 0x00},
	{0x42, 0x00},
	{0xED, 0x01},
	{0xEF, 0x06},
	{0x30, 0xB6},
	{0x31, 0x04},
	{0x32, 0x16},
	{0x33, 0x43},
	{0x34, 0x80},
	{0x35, 0x80},
	{0x36, 0x80},
	{0x37, 0x80},
	{0x38, 0xF4},
	{0x39, 0xF4},
	{0x3A, 0x88},
	{0x3B, 0x52},
	{0x3C, 0x52},
	{0x3D, 0x52},
	{0x3E, 0x0A},
	{0x3F, 0x0A},
	{0xED, 0x01},

	{PS5250_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the ps5250_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ps5250_win_sizes[] = {
	/* 1920*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 15 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ps5250_init_regs_1920_1080_15fps,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ps5250_stream_on[] = {
	{0xEF, 0x01},
	{0x05, 0x01},	/*sw pwdn off*/
	{0x02, 0xfb},	/*sw reset*/
	{0x09, 0x01},
	{0xEF, 0x01},	/* delay > 1ms */
	{PS5250_REG_DELAY, 0x02},
	{0xEF, 0x00},
	{0x11, 0x00},	/*clk not gated*/
	{PS5250_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ps5250_stream_off[] = {
	{0xEF, 0x00},
	{0x11, 0x80},	/*clk gated*/
	{0xEF, 0x01},
	{0x05, 0x05},	/*sw pwdn*/
	{0x09, 0x01},
	{PS5250_REG_END, 0x00},	/* END MARKER */
};

int ps5250_read(struct v4l2_subdev *sd, unsigned char reg, unsigned char *value)
{
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= &reg,
		},
		[1] = {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= value,
		}
	};

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;
	return ret;
}

int ps5250_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value)
{
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 2,
		.buf	= buf,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int ps5250_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != PS5250_REG_END) {
		if (vals->reg_num == PS5250_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ps5250_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			if (vals->reg_num == PS5250_BANK_REG){
				val &= 0xe0;
				val |= (vals->value & 0x1f);
				ret = ps5250_write(sd, vals->reg_num, val);
				ret = ps5250_read(sd, vals->reg_num, &val);
			}
		pr_debug("ps5250_read_array ->> vals->reg_num:0x%02x, vals->reg_value:0x%02x\n",vals->reg_num, val);
		}
		vals++;
	}
	return 0;
}

static int ps5250_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != PS5250_REG_END) {
		if (vals->reg_num == PS5250_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ps5250_write(sd, vals->reg_num, vals->value);
			if (ret < 0){
				printk("ps5250_write error  %d\n" ,__LINE__);
				return ret;
			}
		}
		vals++;
	}
	return 0;
}

static int ps5250_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ps5250_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;
	ret = ps5250_read(sd, 0x00, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0){
		printk("err: ps5250 write error, ret= %d \n",ret);
		return ret;
	}
	if (v != PS5250_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = ps5250_read(sd, 0x01, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != PS5250_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int ps5250_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	unsigned int Cmd_OffNy = 0;

	Cmd_OffNy = ps5250_attr.total_height - value;
	ret = ps5250_write(sd, 0xef, 0x01);
	/*Exp Line Control not set*/
	/*ret += ps5250_write(sd, 0x0e, 0x00);*/
	/*ret += ps5250_write(sd, 0x0f, 0x00);*/
	ret += ps5250_write(sd, 0x0d, (unsigned char)(Cmd_OffNy & 0xff));
	ret += ps5250_write(sd, 0x0c, (unsigned char)((Cmd_OffNy & 0xff00) >> 8));
	ret += ps5250_write(sd, 0x09, 0x01);
	if (ret < 0)
		return ret;
	return 0;
}

static int ps5250_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	unsigned int GDAC = value;

	ret += ps5250_write(sd, 0xef, 0x01);
	ret += ps5250_write(sd, 0x83, (unsigned char)(GDAC & 0x7f));
	ret += ps5250_write(sd, 0x09, 0x01);
	if (ret < 0)
		return ret;
	return 0;
}

static int ps5250_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ps5250_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ps5250_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ps5250_win_sizes[0];
	int ret = 0;

	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
#ifndef SENSOR_WITHOUT_INIT
	ret = ps5250_write_array(sd, wsize->regs);
	if (ret)
		return ret;
#endif
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ps5250_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = ps5250_write_array(sd, ps5250_stream_on);
		pr_debug("ps5250 stream on\n");
	}
	else {
		ret = ps5250_write_array(sd, ps5250_stream_off);
		pr_debug("ps5250 stream off\n");
	}
	return ret;
}

static int ps5250_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ps5250_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ps5250_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	unsigned int pclk = PS5250_SUPPORT_PCLK;
	unsigned int hts = 0;
	unsigned int vts = 0;
	unsigned int Cmd_Lpf = 0;
	unsigned int Cur_OffNy = 0;
	unsigned int Cur_ExpLine = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;

	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	ret = ps5250_write(sd, 0xef, 0x01);
	if(ret < 0)
		return -1;
	ret = ps5250_read(sd, 0x27, &tmp);
	hts = tmp;
	ret += ps5250_read(sd, 0x28, &tmp);
	if(ret < 0)
		return -1;
	hts = (((hts & 0x1f) << 8) | tmp);

	vts = (pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16));
	Cmd_Lpf = vts -1;
	ret = ps5250_write(sd, 0xef, 0x01);
	ret += ps5250_write(sd, 0x0b, (unsigned char)(Cmd_Lpf & 0xff));
	ret += ps5250_write(sd, 0x0a, (unsigned char)(Cmd_Lpf >> 8));
	ret += ps5250_write(sd, 0x09, 0x01);
	if(ret < 0){
		printk("err: ps5250_write err\n");
		return ret;
	}
	ret = ps5250_read(sd, 0x0c, &tmp);
	Cur_OffNy = tmp;
	ret += ps5250_read(sd, 0x0d, &tmp);
	if(ret < 0)
		return -1;
	Cur_OffNy = (((Cur_OffNy & 0xff) << 8) | tmp);
	Cur_ExpLine = ps5250_attr.total_height - Cur_OffNy;

	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 2;
	sensor->video.attr->integration_time_limit = vts - 2;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 2;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

	ret = ps5250_set_integration_time(sd, Cur_ExpLine);
	if(ret < 0)
		return -1;
	return ret;
}

static int ps5250_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ps5250_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ps5250_win_sizes[0];
	}

	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}

static int ps5250_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
#ifndef SENSOR_WITHOUT_INIT
	/*if (pwdn_gpio != -1) {
		ret = gpio_request(pwdn_gpio, "ps5250_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			msleep(50);
			gpio_direction_output(pwdn_gpio, 0);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}*/
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ps5250_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(5);
			gpio_direction_output(reset_gpio, 0);
			msleep(10);
			gpio_direction_output(reset_gpio, 1);
			msleep(20);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}

	ret = ps5250_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ps5250 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
#else
	ident = 0x5250;
#endif
	v4l_info(client, "ps5250 chip found @ 0x%02x (%s)\n",
		client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ps5250_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static long ps5250_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = ps5250_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = ps5250_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = ps5250_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = ps5250_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = ps5250_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			ret = ps5250_write_array(sd, ps5250_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			ret = ps5250_write_array(sd, ps5250_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = ps5250_set_fps(sensor, ctrl->value);
			break;
		default:
			break;
	}
	return 0;
}

static long ps5250_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = ps5250_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ps5250_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ps5250_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ps5250_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ps5250_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ps5250_core_ops = {
	.g_chip_ident = ps5250_g_chip_ident,
	.reset = ps5250_reset,
	.init = ps5250_init,
	.s_power = ps5250_s_power,
	.ioctl = ps5250_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ps5250_g_register,
	.s_register = ps5250_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ps5250_video_ops = {
	.s_stream = ps5250_s_stream,
	.s_parm = ps5250_s_parm,
	.g_parm = ps5250_g_parm,
};

static const struct v4l2_subdev_ops ps5250_ops = {
	.core = &ps5250_core_ops,
	.video = &ps5250_video_ops,
};

static int ps5250_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ps5250_win_sizes[0];
	int ret;

	sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
	if(!sensor){
		printk("Failed to allocate sensor subdev.\n");
		return -ENOMEM;
	}
	memset(sensor, 0 ,sizeof(*sensor));
	/* request mclk of sensor */
	sensor->mclk = clk_get(NULL, "cgu_cim");
	if (IS_ERR(sensor->mclk)) {
		printk("Cannot get sensor input clock cgu_cim\n");
		goto err_get_mclk;
	}
#ifndef SENSOR_WITHOUT_INIT
	clk_set_rate(sensor->mclk, 12000000);
	clk_enable(sensor->mclk);
#endif
	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;
#if 0
	ps5250_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = ps5250_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = ps5250_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(ps5250_win_sizes); i++)
		ps5250_win_sizes[i].mbus_code = mbus;

#endif
	 /*
		convert sensor-gain into isp-gain,
	 */
	ps5250_attr.max_again = 327675;
	ps5250_attr.max_dgain = 0; //ps5250_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ps5250_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ps5250_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("@@@@@@@probe ok ------->ps5250\n");
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int ps5250_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct tx_isp_sensor *sensor = v4l2_get_subdev_hostdata(sd);

	if(reset_gpio != -1)
		gpio_free(reset_gpio);
	if(pwdn_gpio != -1)
		gpio_free(pwdn_gpio);

	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);

	v4l2_device_unregister_subdev(sd);
	kfree(sensor);
	return 0;
}

static const struct i2c_device_id ps5250_id[] = {
	{ "ps5250", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ps5250_id);

static struct i2c_driver ps5250_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ps5250",
	},
	.probe		= ps5250_probe,
	.remove		= ps5250_remove,
	.id_table	= ps5250_id,
};

char * get_sensor_name(void)
{
	return "ps5250";
}

int get_sensor_i2c_addr(void)
{
	return 0x48;
}

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
uint8_t *get_sensor_setting(void)
{
	return sensor_setting;
}

int get_sensor_setting_len(void)
{
	return sizeof(sensor_setting);
}

char *get_sensor_setting_date(void)
{
	return sensor_setting_build_date;
}

char *get_sensor_setting_md5(void)
{
	return sensor_setting_md5;
}
#endif

static __init int init_ps5250(void)
{
	return i2c_add_driver(&ps5250_driver);
}

static __exit void exit_ps5250(void)
{
	i2c_del_driver(&ps5250_driver);
}

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
subsys_initcall(init_ps5250);
#else
module_init(init_ps5250);
#endif
module_exit(exit_ps5250);

MODULE_DESCRIPTION("A low-level driver for Primesensor ps5250 sensors");
MODULE_LICENSE("GPL");
