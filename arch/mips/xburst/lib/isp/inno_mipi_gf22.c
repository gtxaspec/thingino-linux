/*
 * inno_mipi.c
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
#include <linux/proc_fs.h>
#include <soc/gpio.h>

#include "include/tx-isp-common.h"
#include "include/sensor-common.h"
#include "include/txx-funcs.h"
/* #include <linux/ioctl.h> */

#define  i2c_addr 0x48

struct regtest{
    int reg;
    int val;
};

struct i2c_client *client_ioc;

static int inno_mipi_write(struct i2c_client *client, unsigned char reg, unsigned char value)
{
	int ret;
#ifdef ADDRBIT16
	uint8_t buf[3] = {(reg >> 8) & 0xff, reg & 0xff, value};
#else
	unsigned char buf[2] = {reg, value};
#endif
	struct i2c_msg msg = {
		.addr   =  i2c_addr,
		.flags  = 0,
#ifdef ADDRBIT16
		.len    = 3,
#else
		.len    = 2,
#endif
		.buf    = buf,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int inno_mipi_read(struct i2c_client *client, unsigned char reg, unsigned char *value)
{
	int ret;
#ifdef ADDRBIT16
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
#endif
	struct i2c_msg msg[2] = {
		[0] = {
			.addr   = i2c_addr,
			.flags  = 0,
#ifdef ADDRBIT16
			.len    = 2,
			.buf    = buf,
#else
			.len    = 1,
			.buf    = &reg,
#endif
		},
		[1] = {
			.addr   = i2c_addr,
			.flags  = I2C_M_RD,
			.len    = 1,
			.buf    = value,
		}
	};

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}

static struct i2c_board_info inno_mipi_board_info = {
        .type = "inno_mipi",
        .addr = i2c_addr,
};

/* Load an i2c sub-device. */
struct tx_isp_subdev *isp_i2c_new_subdev_board1(struct i2c_adapter *adapter, struct i2c_board_info *info,
                                               const unsigned short *probe_addrs)
{
        struct tx_isp_subdev *sd = NULL;
        struct i2c_client *client;
        struct i2c_driver *driver;

        request_module(I2C_MODULE_PREFIX "%s", info->type);

        /* Create the i2c client */
        if (info->addr == 0){
                ISP_WARNING("[ %s:%d ]\n", __func__, __LINE__);
                return NULL;
        } else {
                client = i2c_new_device(adapter, info); // sensor probe
                client_ioc = client;
        }

        driver = to_i2c_driver(client->dev.driver);
        if (client == NULL || driver== NULL)
                goto error;

        /* Lock the module so we can safely get the tx_isp_subdev pointer */
        if (!try_module_get(driver->driver.owner))
                goto error;
        sd = i2c_get_clientdata(client);

        /* Decrease the module use count to match the first try_module_get. */
        module_put(driver->driver.owner);

error:
        /* If we have a client but no subdev, then something went wrong and
           we must unregister the client. */
        if (client && sd == NULL) {
                i2c_unregister_device(client);
        }

        return sd;
}

static int inno_mipi_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	unsigned char v;

	printk(">>>> probe: init inno mipi csi phy <<<<\n");

	/* *(volatile unsigned *)(0xB005400c) = 0; */
	/* mdelay(500); */
	/* *(volatile unsigned *)(0xB005400c) = 1; */

#if 1
	ret = inno_mipi_write(client, 0x00, 0x7d);
	if(ret) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
#endif
	ret = inno_mipi_read(client, 0x00, &v);
	if(ret < 0) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
	printk(">>>> func :0x00 data is 0x%08x, line : %d <<<<\n", v, __LINE__);

#if 1
	ret = inno_mipi_write(client, 0x4a, 0x3f);
	if(ret) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}	
#endif
	ret = inno_mipi_read(client, 0x4a, &v);
	if(ret < 0) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
	printk(">>>> func :0x4a data is 0x%08x, line : %d <<<<\n", v, __LINE__);

#if 1
	ret = inno_mipi_write(client, 0x23, 0x80);
	if(ret) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
#endif
	ret = inno_mipi_read(client, 0x23, &v);
	if(ret < 0) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
	printk(">>>> func :0x0b data is 0x%08x, line : %d <<<<\n", v, __LINE__);


#if 0
	ret = inno_mipi_write(client, 0x40, 0x87);
	if(ret) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}

	ret = inno_mipi_read(client, 0x40, &v);
	if(ret < 0) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
	printk(">>>> func : 0x%08x, line : %d <<<<\n", v, __LINE__);

	//ret = inno_mipi_write(client, 0x60, 0x80);
	ret = inno_mipi_write(client, 0x60, 0x87);
	if(ret) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}

	ret = inno_mipi_read(client, 0x60, &v);
	if(ret < 0) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
	printk(">>>> func : 0x%08x, line : %d <<<<\n", v, __LINE__);

	ret = inno_mipi_write(client, 0x80 , 0x87);
	if(ret) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}

	ret = inno_mipi_read(client, 0x80, &v);
	if(ret < 0) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
	printk(">>>> func : 0x%08x, line : %d <<<<\n", v, __LINE__);

	ret = inno_mipi_write(client, 0x4a , 0x3f);
	if(ret) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}

	ret = inno_mipi_read(client, 0x4a, &v);
	if(ret < 0) {
		printk("inno_mipi_setting() error line : %d!\n",__LINE__);
		goto error;
	}
	printk(">>>> func : 0x%08x, line : %d <<<<\n", v, __LINE__);
	mdelay(500);
#endif

	printk("*****************************************\n");
	printk("***                                   ***\n");
	printk("*** inno mipi rx phy init >>> PASS !  ***\n");
	printk("***                                   ***\n");
	printk("*****************************************\n");

/////*/////////////////////////////////////////////////////////////////////*/
	/*printk("\n\n");*/
	/*ret = inno_mipi_read(client, 0x20, &v);*/
	/*if(ret < 0) {*/
		/*printk("inno_mipi_setting() error line : %d!\n",__LINE__);*/
		/*return -1;*/
	/*}*/
	/*printk("*************** func : 0x%08x, line : %d ****************\n", v, __LINE__);*/

	/*ret = inno_mipi_write(client, 0xb3 , 0x2);*/
	/*if(ret) {*/
		/*printk("inno_mipi_setting() error line : %d!\n",__LINE__);*/
		/*return -1;*/
	/*}*/

	/*ret = inno_mipi_read(client, 0xb3, &v);*/
	/*if(ret < 0) {*/
		/*printk("inno_mipi_setting() error line : %d!\n",__LINE__);*/
		/*return -1;*/
	/*}*/
	/*printk("*************** func : 0x%08x, line : %d ****************\n", v, __LINE__);*/

	/*ret = inno_mipi_write(client, 0xc0 , 0x1f);*/
	/*if(ret) {*/
		/*printk("inno_mipi_setting() error line : %d!\n",__LINE__);*/
		/*return -1;*/
	/*}*/
	/*ret = inno_mipi_read(client, 0xc0, &v);*/
	/*if(ret < 0) {*/
		/*printk("inno_mipi_setting() error line : %d!\n",__LINE__);*/
		/*return -1;*/
	/*}*/
	/*printk("*************** func : 0x%08x, line : %d ****************\n", v, __LINE__);*/
/////////////////////////////////////////////////////////////////////////
	return 0;

error:
	printk("******************************************\n");
	printk("***                                    ***\n");
	printk("*** inno mipi rx phy init >>> FAILED ! ***\n");
	printk("***                                    ***\n");
	printk("******************************************\n");
	return -1;

}

static int inno_mipi_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id inno_mipi_id[] = {
	{ "inno_mipi", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, inno_mipi_id);

static struct i2c_driver inno_mipi_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "inno_mipi_gf22",
	},
	.probe		= inno_mipi_probe,
	.remove		= inno_mipi_remove,
	.id_table	= inno_mipi_id,
};

static uint32_t pb_1;
static uint32_t pb_2;
int init_inno_mipi(void)
{
        int ret = 0;
	printk("@@@@@@@@@@@@@@@@@@@ inno init !!!!\n");
        struct i2c_adapter *adapter= i2c_get_adapter(2);

        /* pb_1 = *(volatile u32 *)0xb0011020; */
        /* pb_2 = *(volatile u32 *)0xb0011030; */

        /* *(volatile u32 *)0xb0011020 = pb_1 & ~0x6000000; */
        /* *(volatile u32 *)0xb0011030 = pb_2 & ~0x6000000; */

        ret = i2c_add_driver(&inno_mipi_driver);

        if (!adapter) {
                printk("[ %s:%d ] Failed to get I2C adapter %s, deferring probe!!!\n",
                          __func__, __LINE__, inno_mipi_board_info.type);
                return -1;
        }

        isp_i2c_new_subdev_board1(adapter, &inno_mipi_board_info, NULL);

        printk("[%s %d] inno mipi phy is OK!!!\n", __func__, __LINE__);

	return ret;
}

static __exit void exit_inno_mipi(void)
{
        /* if (client_ioc) */
        /*         i2c_unregister_device(client_ioc); */
	private_i2c_del_driver(&inno_mipi_driver);
        /* private_jzgpio_set_func(GPIO_PORT_A, GPIO_OUTPUT0, 0xc000); */
        /* private_jzgpio_set_func(GPIO_PORT_B, GPIO_FUNC_0, 0x6000000); */

        /* *(volatile u32 *)0xb0011020 = pb_1; */
        /* *(volatile u32 *)0xb0011030 = pb_2; */

        printk("[%s %d] inno mipi phy del is OK!!!\n", __func__, __LINE__);
}

/*module_init(init_inno_mipi);*/
module_exit(exit_inno_mipi);

MODULE_DESCRIPTION("A low-level driver for OmniVision inno_mipi sensors");
MODULE_LICENSE("GPL");
