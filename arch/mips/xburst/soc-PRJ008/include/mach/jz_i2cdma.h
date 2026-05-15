/*
 *  Copyright (C) 2025 Ingenic Semiconductor Inc.
 *
 *  Author: <jim.jwang@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * This file is a part of i2c, it's
 * used for i2cdma deal.
 */

#ifndef __MACH_JZI2CDMA_H__
#define __MACH_JZI2CDMA_H__

#include <linux/i2c.h>


typedef enum {
	I2CDMA_MSG_STOP,
	I2CDMA_MSG_REPEAT,
}i2cmode_e;

typedef struct{
	unsigned long slaver_addr;
	void *buf;
	int unit_size;//单元大小
	int unit_num;//单元个数
	void (*i2c_dma_callback)(void *arg,int ret);//该接口不可睡眠，中断中调用
	void *param;	//回调函数参数
	i2cmode_e i2cmode;	//0:is stop mode 1:restart mode
}i2c_dma_param_t;

int i2c_dma_write_transfer(struct i2c_adapter *adapter,i2c_dma_param_t *param);

#endif

