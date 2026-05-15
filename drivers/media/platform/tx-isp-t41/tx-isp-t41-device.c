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
#include "../../../arch/mips/xburst2/lib/isp-t41/include/fast_start_common.h"
extern int tx_isp_init(void);
extern void tx_isp_exit(void);

extern int use_num_sensor;
unsigned int ldc_mode = 0;
EXPORT_SYMBOL(ldc_mode);

int use_num_sensor;
EXPORT_SYMBOL(use_num_sensor);

int check_sensor_num;
EXPORT_SYMBOL(check_sensor_num);

unsigned long ir_switch_mode = 2;
EXPORT_SYMBOL(ir_switch_mode);

unsigned long photosensitive_value = 50;
EXPORT_SYMBOL(photosensitive_value);

unsigned long isp_debug_flag = 0;
EXPORT_SYMBOL(isp_debug_flag);

unsigned long isp_skip_count = 3;
EXPORT_SYMBOL(isp_skip_count);

unsigned long isp_discard_count = 6;
EXPORT_SYMBOL(isp_discard_count);

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

unsigned long frame_channel1_width = 0;
EXPORT_SYMBOL(frame_channel1_width);

unsigned long frame_channel1_height = 0;
EXPORT_SYMBOL(frame_channel1_height);

unsigned long frame_channel1_nrvbs = 0;
EXPORT_SYMBOL(frame_channel1_nrvbs);

unsigned long direct_mode_save_kernel_ch1 = 0;
EXPORT_SYMBOL(direct_mode_save_kernel_ch1);

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
unsigned long sensor0_calibration_mode = 0;
EXPORT_SYMBOL(sensor0_calibration_mode);
unsigned long sensor1_calibration_mode = 0;
EXPORT_SYMBOL(sensor1_calibration_mode);

unsigned long user_wdr_mode = 0;
EXPORT_SYMBOL(user_wdr_mode);

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

    p = strstr(str_t, "photosensitive_value=");
    if(p != NULL) {
        photosensitive_value = simple_strtoul(p+strlen("photosensitive_value="), NULL, 10);
    }

    p = strstr(str_t, "isp_debug_flag=");
    if(p != NULL) {
        isp_debug_flag = simple_strtoul(p+strlen("isp_debug_flag="), NULL, 10);
    }

    p = strstr(str_t, "isp_skip_count=");
    if(p != NULL) {
        isp_skip_count = simple_strtoul(p+strlen("isp_skip_count="), NULL, 10);
    }

    p = strstr(str_t, "isp_discard_count=");
    if(p != NULL) {
        isp_discard_count = simple_strtoul(p+strlen("isp_discard_count="), NULL, 10);
    }

    p = strstr(str_t, "use_num_sensor=");
    if(p != NULL) {
	use_num_sensor = simple_strtol(p + strlen("use_num_sensor="), NULL, 10);
//		printk("[%s][%d]------> use_num_sensor = %d \n", __func__, __LINE__, use_num_sensor);
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

    p = strstr(str_t, "init_vw_ch1=");
    if(p != NULL) {
        frame_channel1_width = simple_strtoul(p+strlen("init_vw_ch1="), NULL, 10);
    }

    p = strstr(str_t, "init_vh_ch1=");
    if(p != NULL) {
        frame_channel1_height = simple_strtoul(p+strlen("init_vh_ch1="), NULL, 10);
    }

    p = strstr(str_t, "nrvbs_ch1=");
    if(p != NULL) {
        frame_channel1_nrvbs = simple_strtoul(p+strlen("nrvbs_ch1="), NULL, 10);
    }

    p = strstr(str_t, "direct_mode_save_kernel_ch1=");
    if(p != NULL) {
        direct_mode_save_kernel_ch1 = simple_strtoul(p+strlen("direct_mode_save_kernel_ch1="), NULL, 10);
    }

    p = strstr(str_t, "sensor_num=");
    if(p != NULL) {
        sensor_number = simple_strtoul(p+strlen("sensor_num="), NULL, 10);
    }

    p = strstr(str_t, "hfnum=");
    if(p != NULL) {
        high_framerate_mode_num = simple_strtoul(p+strlen("hfnum="), NULL, 10);
    }

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
        sensor0_calibration_mode = simple_strtol(p+strlen("mode="), NULL, 10);
        sensor1_calibration_mode = simple_strtol(p+strlen("mode="), NULL, 10);
    }
    printk("Sensor Calibration Mode:%ld\n", sensor_calibration_mode);

	p = strstr(str_t, "ldc_mode=");
    if(p != NULL) {
		ldc_mode = simple_strtoul(p+strlen("ldc_mode="), NULL, 10);
	}

	p = strstr(str_t, "user_wdr_mode=");
	if(p != NULL) {
		user_wdr_mode = simple_strtoul(p+strlen("user_wdr_mode="), NULL, 10);
	}

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

}

static int __init cmdline_private_data(char *str)
{
	char *p = NULL;
	char str_t[1024] = "";

	check_sensor_num = simple_strtol(str , NULL, 10);
	printk("[%s][%d]======------> check_sensor_num = %d \n", __func__, __LINE__, check_sensor_num);

	return 0;
}

__setup("senv", ir_switch_parse);
__setup("sensor_setting_fw=", sensor_setting_fw_parse);
__setup("riscv_fw=", riscv_fw_parse);
__setup("check_sensor_num=", cmdline_private_data);


#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
#define MAX_TAG_ITER_SIZE		    (64 * 1024)
#define SENSOR_SETTING_MAX_SIZE	    (256 * 1024)
/*lzma src addr must 64 bytes align*/
#define SENSOR_LOADADDR             (0x3300000)
#define DECOMPRESS_SENSOR_LOADADDR  (0x3500000)
#define SENSOR_SETTING_ADDR		    (DECOMPRESS_SENSOR_LOADADDR)

#define AE_TABLE_OFFSET             (1*1024*4 * 4)
#define AE_TABLE_ADDR               (0x3100000 + AE_TABLE_OFFSET)


#define MAX_TAG_ITER_SIZE_2SENSOR	(80 * 1024)
#define SENSOR0_SETTING_MAX_SIZE	(204 * 1024)
#define SENSOR1_SETTING_MAX_SIZE	(204 * 1024)
#define SENSOR0_SETTING_OFFSET	    (MAX_TAG_ITER_SIZE_2SENSOR)
#define SENSOR0_SETTING_ADDR		(0x3800000 + SENSOR0_SETTING_OFFSET) /* Consistent with uboot */
#define SENSOR1_SETTING_OFFSET	    (SENSOR0_SETTING_OFFSET + SENSOR0_SETTING_MAX_SIZE)
#define SENSOR1_SETTING_ADDR		(0x3800000 + SENSOR1_SETTING_OFFSET) /* Consistent with uboot */

#define AE0_TABLE_OFFSET            (1*1024*4 * 7)
#define AE0_TABLE_ADDR              (0x3800000 + AE0_TABLE_OFFSET)
#define AE1_TABLE_OFFSET            (1*1024*4 * 8)
#define AE1_TABLE_ADDR              (0x3800000 + AE1_TABLE_OFFSET)



/*
 * crc：original uncompressed file crc
 * len: original uncompressed file lenght 
 * lzma_len: lzma compressed file lenght
 * */
struct sensor_setting_info {
    unsigned int head;
    int len;
    int lzma_len;
    unsigned int crc;
    unsigned int align[12];
    uint8_t data[0]; /* Just a way for get data */
};
#define SENSOR_SETTING_INFO_SIZE    sizeof(struct sensor_setting_info)

static void *g_setting_info = NULL;
static void *g_setting_info_for_etc = NULL;
static void *g_setting_data = NULL;
static void *g_sensor1_setting_info = NULL;
void *setting_mem = NULL;
void *setting_info = NULL;
static int io_map_flag = 0;

int sensor_setting_init(void)
{

    if(io_map_flag == 1)
        return 0;

    io_map_flag = 1;

    if(sensor_calibration_mode > 1) { /* By default, only one IQ effect file is supported at most  */
        printk("sensor_calibration_mode(%ld) is invalid\n", sensor_calibration_mode);
        return -1;
    }

    setting_mem = (void *)ioremap(SENSOR_SETTING_ADDR, SENSOR_SETTING_MAX_SIZE);
    if(setting_mem == NULL) {
        printk("sensor setting mmap failed\n");
        return -2;
    }
    setting_info = (void *)ioremap(SENSOR_LOADADDR, SENSOR_SETTING_INFO_SIZE);
    if(setting_info == NULL) {
        printk("sensor setting  info mmap failed\n");
        return -2;
    }

    printk("Calibration ADDR = %p,info = %p\n", setting_mem,setting_info);

    g_setting_data = vmalloc(SENSOR_SETTING_MAX_SIZE);
    if(g_setting_data == NULL) {
        printk("sensor setting malloc failed\n");
        return -3;
    }
    g_setting_info = vmalloc(sizeof(struct sensor_setting_info));
    if(g_setting_info == NULL) {
        printk("sensor setting malloc failed\n");
        return -3;
    }

    memcpy(g_setting_data, setting_mem, SENSOR_SETTING_MAX_SIZE);
    memcpy(g_setting_info, setting_info, SENSOR_SETTING_INFO_SIZE);

    return 0;
}

 int read_iq(void)
 {
        char file_name[64];
        struct file *fd = NULL;
        mm_segment_t old_fs;
        loff_t *pos;
#ifndef CONFIG_INGENIC_NUM_SENSOR
        snprintf(file_name, sizeof(file_name), "/etc/sensor/%s-t41.bin",get_sensor_name());
#else
        if(use_num_sensor == 0)
            snprintf(file_name, sizeof(file_name), "/etc/sensor/%s-t41.bin",get_sensor_name_one());
        if(use_num_sensor == 1)
            snprintf(file_name, sizeof(file_name), "/etc/sensor/%s-t41.bin",get_sensor_name_two());
#endif
        fd = filp_open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 00766);
        if (fd < 0) {
                printk("Failed to open %s\n",file_name);
                return -1;;
        }
     g_setting_info_for_etc = get_sensor_setting();
        old_fs = get_fs();
        set_fs(KERNEL_DS);
        pos = &(fd->f_pos);
        vfs_write(fd,g_setting_info_for_etc , get_sensor_setting_len(), pos);
        filp_close(fd, NULL);
        set_fs(old_fs);
 }

 static ssize_t iq_addr_fops_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
 {
        read_iq();

        return 0;
 }

 static const struct file_operations iq_data_proc_fops ={
     .read = iq_addr_fops_read,
 };

uint8_t *get_sensor_setting(void)
{

    sensor_setting_init();

    if(g_setting_data == NULL) {
        printk("sensor calibration setting addr is invalid\n");
        return NULL;
    }
    return g_setting_data;
}

int get_sensor_setting_len(void)
{
    struct sensor_setting_info *info = NULL;

    sensor_setting_init();

    if(g_setting_info == NULL) {
        printk("sensor calibration setting addr is invalid\n");
        return 0;
    }

    info = (struct sensor_setting_info *)(g_setting_info);
    printk("Calibration len = %d\n", info->len);
    if(info->len > SENSOR_SETTING_MAX_SIZE)
        return SENSOR_SETTING_MAX_SIZE;
    return info->len;
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

    info = (struct sensor_setting_info *)(g_setting_info);
    sprintf(buf, "calibration crc %u", info->crc);
    return buf;
}

char *get_sensor_setting_date(void)
{
    static char buf[64] = "";
    sensor_setting_init();
    sprintf(buf, "calibration mode %ld", sensor_calibration_mode);
    return buf;
}

#if 0
/*sensor1 setting*/
int sensor1_setting_init(void)
{
    static int io_map_flag = 0;
    void *setting_mem = NULL;

    if(io_map_flag == 1)
        return 0;

    io_map_flag = 1;

    if(sensor1_calibration_mode > 1) { /* By default, only one IQ effect file is supported at most */
        printk("sensor_calibration_mode(%ld) is invalid\n", sensor_calibration_mode);
        return -1;
    }

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
void *ae_table = NULL;
int sensor_start_ae_table_init(void)
{
    static int io_map_flag = 0;
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

int get_sensor_start_ae_table(void)
{
    return g_start_ae_table;
}
#if 0
int get_sec_sensor_start_ae_table(void)
{
    return g_sec_start_ae_table;
}
#endif
static int __init tx_isp_t41_init(void)
{
    proc_create("iq_data", S_IRUGO, NULL, &iq_data_proc_fops);
    if (sensor_setting_fw == -1 || riscv_fw == -1) {
        printk("tx-isp-t41 not init(%d,%d)\n",sensor_setting_fw,riscv_fw);
        return 0;
    }else{
        return tx_isp_init();
    }
}

static void __exit tx_isp_t41_exit(void)
{
    tx_isp_exit();
    if(io_map_flag){
        iounmap(setting_mem);
        iounmap(setting_info);
        iounmap(ae_table);
        vfree(g_setting_data);
        vfree(g_setting_info);
        vfree(g_start_ae_table);
    }
}

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
subsys_initcall(tx_isp_t41_init);
#else
module_init(tx_isp_t41_init);
#endif
module_exit(tx_isp_t41_exit);

MODULE_AUTHOR("Ingenic xhshen");
MODULE_DESCRIPTION("tx isp driver");
MODULE_LICENSE("GPL");

