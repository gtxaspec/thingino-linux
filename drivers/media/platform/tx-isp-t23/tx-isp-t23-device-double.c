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
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include "../../../../arch/mips/xburst/lib/isp-t23/include/fast_start_common.h"

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

unsigned long frame_channel3_width = 0;
EXPORT_SYMBOL(frame_channel3_width);

unsigned long frame_channel3_height = 0;
EXPORT_SYMBOL(frame_channel3_height);

unsigned long frame_channel3_nrvbs = 0;
EXPORT_SYMBOL(frame_channel3_nrvbs);

unsigned long sensor_number = 0;
EXPORT_SYMBOL(sensor_number);

unsigned long high_framerate_mode_num = 5;
EXPORT_SYMBOL(high_framerate_mode_num);

unsigned long high_framerate_kernel_mode_en = 0;
EXPORT_SYMBOL(high_framerate_kernel_mode_en);

unsigned long high_framerate_risc_v_mode_en = 1;
EXPORT_SYMBOL(high_framerate_risc_v_mode_en);

unsigned long sensor_calibration_mode = 0;
EXPORT_SYMBOL(sensor_calibration_mode);


long g_day_ae_val = 0;
EXPORT_SYMBOL(g_day_ae_val);

long g_night_ae_val = 0;
EXPORT_SYMBOL(g_night_ae_val);

long g_ae_coeff = 0;
EXPORT_SYMBOL(g_ae_coeff);

long g_wb_r = 0;
EXPORT_SYMBOL(g_wb_r);

long g_wb_b = 0;
EXPORT_SYMBOL(g_wb_b);

int riscv_fw = 0;
int sensor_setting_fw = 0;

static int __init ir_switch_parse(char *str)
{
	char *p = NULL;
	char str_t[1024] = "";

	p = strstr(str, "eenv");
	if(p == NULL) {
		p = strstr((str + strlen(str) + 1), "eenv");
		if(p != NULL) {
			memcpy(str_t, str, (p-str) < (sizeof(str_t) - 1) ? (p-str) : (sizeof(str_t)-1));
			if(str_t[strlen(str)] == '\0') {
				str_t[strlen(str)] = ' ';
			}
		} else {
			memcpy(str_t, str, strlen(str) < (sizeof(str_t) - 1) ? strlen(str) : (sizeof(str_t)-1));
		}
	} else {
		memcpy(str_t, str, strlen(str) < (sizeof(str_t) - 1) ? strlen(str) : (sizeof(str_t)-1));
	}

	p = strstr(str_t, "ir_mode=");
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

	p = strstr(str_t, "min=");
	if(p != NULL) {
		ir_threshold_min = simple_strtoul(p, NULL, 10);
	}

	p = strstr(str_t, "max=");
	if(p != NULL) {
		ir_threshold_max = simple_strtoul(p, NULL, 10);
	}

	printk("%s mode: %lu threshold min:%lu max:%lu\n", __func__, ir_switch_mode, ir_threshold_min, ir_threshold_max);

	p = strstr(str_t, "init_vw=");
	if(p != NULL) {
		frame_channel_width = simple_strtoul(p+strlen("init_vw="), NULL, 10);
	}

	p = strstr(str_t, "init_vh=");
	if(p != NULL) {
		frame_channel_height = simple_strtoul(p+strlen("init_vh="), NULL, 10);
    }

    p = strstr(str_t, "nrvbs=");
    if(p != NULL) {
        frame_channel_nrvbs = simple_strtoul(p+strlen("nrvbs="), NULL, 10);
    }

    p = strstr(str_t, "sensor_num=");
    if(p != NULL) {
        sensor_number = simple_strtoul(p+strlen("sensor_num="), NULL, 10);                                
    }

    if (sensor_number == 2) {
        p = strstr(str_t, "sub_init_vw=");
        if(p != NULL) {
            frame_channel3_width = simple_strtoul(p+strlen("sub_init_vw="), NULL, 10);
        }

        p = strstr(str_t, "sub_init_vh=");
        if(p != NULL) {
            frame_channel3_height = simple_strtoul(p+strlen("sub_init_vh="), NULL, 10);
        }

        p = strstr(str_t, "sub_nrvbs=");
        if(p != NULL) {
            frame_channel3_nrvbs = simple_strtoul(p+strlen("sub_nrvbs="), NULL, 10);
        }
    }

    p = strstr(str_t, "hfnum=");
    if(p != NULL) {
        high_framerate_mode_num = simple_strtoul(p+strlen("hfnum="), NULL, 10);
    }
    
    if(sensor_number == 2)
    	printk("%s width:%lu height:%lu nrvbs:%lu sub_width:%lu sub_height:%lu sub_nrvbs:%lu \n", __func__, frame_channel_width, frame_channel_height, frame_channel_nrvbs, frame_channel3_width, frame_channel3_height, frame_channel3_nrvbs);
	else
	    printk("%s width:%lu height:%lu nrvbs:%lu\n", __func__, frame_channel_width, frame_channel_height, frame_channel_nrvbs);

    printk("%s hight framerate mode change num:%d\n", __func__, (int)high_framerate_mode_num);

	p = strstr(str_t, "dayEV=");
	if(p != NULL) {
		g_day_ae_val = simple_strtol(p+strlen("dayEV="), NULL, 10);
	}

	p = strstr(str_t, "nightEV=");
	if(p != NULL) {
		g_night_ae_val = simple_strtol(p+strlen("nightEV="), NULL, 10);
	}

	p = strstr(str_t, "coeff=");
	if(p != NULL) {
		g_ae_coeff = simple_strtol(p+strlen("coeff="), NULL, 10);
	}

	p = strstr(str_t, "wbr=");
	if(p != NULL) {
		g_wb_r = simple_strtol(p+strlen("wbr="), NULL, 10);
	}

	p = strstr(str_t, "wbb=");
	if(p != NULL) {
		g_wb_b = simple_strtol(p+strlen("wbb="), NULL, 10);
	}

	printk("%s dayEv:%ld nightEv:%ld coeff:%ld wbr:%ld wbb:%ld\n", __func__, g_day_ae_val, g_night_ae_val, g_ae_coeff, g_wb_r, g_wb_b);

	p = strstr(str_t, "mode=");
	if(p != NULL) {
		sensor_calibration_mode = simple_strtol(p+strlen("mode="), NULL, 10);
	}
	printk("Sensor Calibration Mode:%ld\n", sensor_calibration_mode);

	return 1;
}

static int __init sensor_setting_fw_parse(char *str)
{
	if (!str)
		return 0;

	if(!strcmp("-1",str)){
		sensor_setting_fw = -1;
	}else {
		sensor_setting_fw = 0;
	}

	return 0;

}

static int __init riscv_fw_parse(char *str)
{
	if (!str)
		return 0;

	if(!strcmp("-1",str)){
		riscv_fw = -1;
	}else {
		riscv_fw = 0;
	}

	return 0;
}

__setup("senv", ir_switch_parse);
__setup("sensor_setting_fw=", sensor_setting_fw_parse);
__setup("riscv_fw=", riscv_fw_parse);

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
#define MAX_TAG_ITER_SIZE		(64 * 1024)
#define MAX_TAG_RISCV_FW_SIZE	(128 * 1024)
#define SENSOR_SETTING_MAX_SIZE	(160 * 1024)
#define SENSOR_SETTING_OFFSET	(MAX_TAG_ITER_SIZE + MAX_TAG_RISCV_FW_SIZE)
#define SENSOR_SETTING_ADDR		(0x3800000 + SENSOR_SETTING_OFFSET) /* Consistent with uboot */

#define AE_TABLE_OFFSET         (1*1024*4 * 6)
#define AE_TABLE_ADDR           (0x3800000 + AE_TABLE_OFFSET)


#define MAX_TAG_ITER_SIZE_2SENSOR		(80 * 1024)
#define SENSOR0_SETTING_MAX_SIZE	(160 * 1024)
#define SENSOR1_SETTING_MAX_SIZE	(160 * 1024)
#define SENSOR0_SETTING_OFFSET	(MAX_TAG_ITER_SIZE_2SENSOR)
#define SENSOR0_SETTING_ADDR		(0x3800000 + SENSOR0_SETTING_OFFSET) /* Consistent with uboot */
#define SENSOR1_SETTING_OFFSET	(SENSOR0_SETTING_OFFSET + SENSOR0_SETTING_MAX_SIZE)
#define SENSOR1_SETTING_ADDR		(0x3800000 + SENSOR1_SETTING_OFFSET) /* Consistent with uboot */

#define AE0_TABLE_OFFSET         (1*1024*4 * 7)
#define AE0_TABLE_ADDR           (0x3800000 + AE0_TABLE_OFFSET)
#define AE1_TABLE_OFFSET         (1*1024*4 * 8)
#define AE1_TABLE_ADDR           (0x3800000 + AE1_TABLE_OFFSET)


struct sensor_setting_info {
	unsigned int head;
	int len;
	unsigned int crc;
	uint8_t data[0]; /* Just a way for get data */
};

static void *g_setting_info = NULL;
static void *g_sensor1_setting_info = NULL;
static void *g_setting_info_for_etc = NULL;

int sensor_setting_init(void)
{
    static int io_map_flag = 0;
    void *setting_mem = NULL;

    if(io_map_flag == 1)
        return 0;

    io_map_flag = 1;
    if (sensor_number == 2) {
        setting_mem = (void *)ioremap(SENSOR0_SETTING_ADDR, SENSOR_SETTING_MAX_SIZE);
        if(setting_mem == NULL) {
            printk("sensor setting mmap failed\n");
            return -2;
        }

    }else {
        setting_mem = (void *)ioremap(SENSOR_SETTING_ADDR, SENSOR_SETTING_MAX_SIZE);
        if(setting_mem == NULL) {
            printk("sensor setting mmap failed\n");
            return -2;
        }
    }

    printk("Calibration ADDR = %p\n", setting_mem);
	printk("Data = %x \n", *(unsigned int *)setting_mem);

	g_setting_info = vmalloc(SENSOR_SETTING_MAX_SIZE);
	if(g_setting_info == NULL) {
		printk("sensor setting malloc failed\n");
		return -3;
	}

	memcpy(g_setting_info, setting_mem, SENSOR_SETTING_MAX_SIZE);
	return 0;
}

uint8_t *get_sensor_setting(void)
{
	struct sensor_setting_info *info = NULL;

	sensor_setting_init();

	if(g_setting_info == NULL) {
		printk("sensor calibration setting addr is invalid\n");
		return NULL;
	}

	info = (struct sensor_setting_info *)(g_setting_info + (sensor_calibration_mode * SENSOR_SETTING_MAX_SIZE));
	if(info->data)
		return info->data;
	return NULL;
}

int get_sensor_setting_len(void)
{
	struct sensor_setting_info *info = NULL;

	sensor_setting_init();

	if(g_setting_info == NULL) {
		printk("sensor calibration setting addr is invalid\n");
		return 0;
	}

	info = (struct sensor_setting_info *)(g_setting_info + (sensor_calibration_mode * SENSOR_SETTING_MAX_SIZE));
	printk("Calibration len = %d\n", info->len);
	if(info->len > SENSOR_SETTING_MAX_SIZE)
		return SENSOR_SETTING_MAX_SIZE;
	return info->len;
}

char *get_sensor_setting_date(void)
{
	static char buf[64] = "";
	sensor_setting_init();
	sprintf(buf, "calibration mode %ld", sensor_calibration_mode);
	return buf;
}

char *get_sensor_setting_md5(void)
{
	struct sensor_setting_info *info = NULL;
	static char buf[64] = "";

	sensor_setting_init();

	if(g_setting_info == NULL) {
		printk("sensor calibration setting addr is invalid\n");
		return NULL;
	}

	info = (struct sensor_setting_info *)(g_setting_info + (sensor_calibration_mode * SENSOR_SETTING_MAX_SIZE));
	sprintf(buf, "calibration crc %u", info->crc);
	return buf;
}

#if 1
/*sensor1 setting*/
int sensor1_setting_init(void)
{
	static int io_map_flag = 0;
	void *setting_mem = NULL;

	if(io_map_flag == 1)
		return 0;

	io_map_flag = 1;
	setting_mem = (void *)ioremap(SENSOR1_SETTING_ADDR, SENSOR_SETTING_MAX_SIZE);
	if(setting_mem == NULL) {
		printk("sensor setting mmap failed\n");
		return -2;
	}
	printk("Calibration ADDR = %p\n", setting_mem);

	g_sensor1_setting_info = vmalloc(SENSOR_SETTING_MAX_SIZE);
	if(g_sensor1_setting_info == NULL) {
		printk("sensor setting malloc failed\n");
		return -3;
	}

	memcpy(g_sensor1_setting_info, setting_mem, SENSOR_SETTING_MAX_SIZE);
	return 0;
}

uint8_t *get_sensor1_setting(void)
{
	struct sensor_setting_info *info = NULL;

	sensor1_setting_init();

	if(g_sensor1_setting_info == NULL) {
		printk("sensor calibration setting addr is invalid\n");
		return NULL;
	}

	info = (struct sensor_setting_info *)(g_sensor1_setting_info + (sensor_calibration_mode * SENSOR_SETTING_MAX_SIZE));
	if(info->data)
		return info->data;
	return NULL;
}

int get_sensor1_setting_len(void)
{
	struct sensor_setting_info *info = NULL;

	sensor1_setting_init();

	if(g_sensor1_setting_info == NULL) {
		printk("sensor calibration setting addr is invalid\n");
		return 0;
	}

	info = (struct sensor_setting_info *)(g_sensor1_setting_info + (sensor_calibration_mode * SENSOR_SETTING_MAX_SIZE));
	printk("Calibration len = %d\n", info->len);
	if(info->len > SENSOR_SETTING_MAX_SIZE)
		return SENSOR_SETTING_MAX_SIZE;
	return info->len;
}

char *get_sensor1_setting_date(void)
{
	static char buf[64] = "";
	sensor1_setting_init();
	sprintf(buf, "calibration mode %ld", sensor_calibration_mode);
	return buf;
}

char *get_sensor1_setting_md5(void)
{
	struct sensor_setting_info *info = NULL;
	static char buf[64] = "";

	sensor1_setting_init();

	if(g_sensor1_setting_info == NULL) {
		printk("sensor calibration setting addr is invalid\n");
		return NULL;
	}

	info = (struct sensor_setting_info *)(g_sensor1_setting_info + (sensor_calibration_mode * SENSOR_SETTING_MAX_SIZE));
	sprintf(buf, "calibration crc %u", info->crc);
	return buf;
}
#endif/*end sensor1 setting*/

#endif

static void *g_start_ae_table = NULL;
static void *g_sec_start_ae_table = NULL;
int sensor_start_ae_table_init(void)
{
	static int io_map_flag = 0;
	void *ae_table = NULL;
	void *sec_ae_table = NULL;

	if(io_map_flag == 1)
		return 0;

	io_map_flag = 1;

	
	if(sensor_number == 2) {
		ae_table = (void *)ioremap(AE0_TABLE_ADDR, 4096);
		if(ae_table == NULL) {
			printk("sensor start ae table mmap failed\n");
			return -2;
		}
		sec_ae_table = (void *)ioremap(AE1_TABLE_ADDR, 4096);
		if(ae_table == NULL) {
			printk("sensor start ae table mmap failed\n");
			return -2;
		}
	}else{
		ae_table = (void *)ioremap(AE_TABLE_ADDR, 4096);
		if(ae_table == NULL) {
			printk("sensor start ae table mmap failed\n");
			return -2;
		}
	}

	printk("Start AE Table ADDR = %p\n", ae_table);
	if(sensor_number == 2)
		printk("Start AE1 Table ADDR = %p\n", sec_ae_table);

	g_start_ae_table = vmalloc(4096);
	if(g_start_ae_table == NULL) {
		printk("sensor setting malloc failed\n");
		return -3;
	}

	memcpy(g_start_ae_table, ae_table, 4096);

	if(sensor_number == 2) {
		g_sec_start_ae_table = vmalloc(4096);
		if(g_sec_start_ae_table == NULL) {
			printk("second sensor setting malloc failed\n");
			return -3;
		}
		memcpy(g_sec_start_ae_table, sec_ae_table, 4096);
	}

	return 0;
}

void* get_sensor_start_ae_table(void)
{
	return g_start_ae_table;
}

int get_sec_sensor_start_ae_table(void)
{
	return g_sec_start_ae_table;
}

#if 0
int read_iq(void)
{
	char file_name[64];
	struct file *fd = NULL;
	mm_segment_t old_fs;
	loff_t *pos;

	snprintf(file_name, sizeof(file_name), "/etc/sensor/%s-t23.bin",get_sensor_name());
	fd = filp_open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 00766);
	if (fd < 0) {
		printk("Failed to open %s\n",file_name);
		return -1;;
	}
    g_setting_info_for_etc = get_sensor_setting();
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = &(fd->f_pos);
//	vfs_write(fd, g_setting_data, 254032, pos);
	vfs_write(fd,g_setting_info_for_etc , get_sensor_setting_len(), pos);
	filp_close(fd, NULL);
	set_fs(old_fs);

}
#endif
static ssize_t iq_addr_fops_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
//	read_iq();

	return 0;
}

static const struct file_operations iq_data_proc_fops ={
    .read = iq_addr_fops_read,
};

static int __init tx_isp_t23_init(void)
{
	proc_create("iq_data", S_IRUGO, NULL, &iq_data_proc_fops);
#if 0
	if (sensor_setting_fw == -1 || riscv_fw == -1) {
		return 0;
	}else{
		return tx_isp_init();
	}
#endif
	tx_isp_init();
    /* return 0; */
}

static void __exit tx_isp_t23_exit(void)
{
	tx_isp_exit();
}

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
subsys_initcall(tx_isp_t23_init);
#else
module_init(tx_isp_t23_init);
#endif
module_exit(tx_isp_t23_exit);

MODULE_AUTHOR("Ingenic xhshen");
MODULE_DESCRIPTION("tx isp driver");
MODULE_LICENSE("GPL");
