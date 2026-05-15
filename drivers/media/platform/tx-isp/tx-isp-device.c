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
#include <linux/crc32.h>
#include <linux/ctype.h>
#include <linux/memblock.h>
#include <linux/page-isolation.h>
#include <linux/dma-mapping.h>
#include <asm/dma-contiguous.h>
#include <asm/page.h>
#include <linux/sizes.h>
#include "tx-isp-device.h"
#include <jz_proc.h>
extern int tx_isp_init(void);
extern void tx_isp_exit(void);

extern int use_num_sensor;

#ifdef CONFIG_BLK_DEV_INITRD
extern void zrootfs_start(void);
#endif/*CONFIG_BLK_DEV_INITRD*/
unsigned int ldc_mode = 0;

static unsigned int iq_addr_src = 0;
static unsigned int iq_addr_dec = 0;
static unsigned int iq_addr_size = 0;
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

unsigned long frame_channel_width[12] = {0};
EXPORT_SYMBOL(frame_channel_width);

unsigned long frame_channel_height[12] = {0};
EXPORT_SYMBOL(frame_channel_height);

unsigned long frame_channel_nrvbs[12] = {0};
EXPORT_SYMBOL(frame_channel_nrvbs);

unsigned long direct_mode_save_kernel_ch1 = 0;
EXPORT_SYMBOL(direct_mode_save_kernel_ch1);

unsigned long sensor_number = 1;
EXPORT_SYMBOL(sensor_number);

unsigned long i2c_num = 0;
EXPORT_SYMBOL(i2c_num);

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
unsigned long Joint_mode = 0;
int joint_sizeimage = 0;
int joint_vb_chn = 0;
int mdns_en = 0;
EXPORT_SYMBOL(Joint_mode);

unsigned int user_wdr_mode = 0;
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
static struct proc_dir_entry *proc_jz_zeratul = NULL;

/*
 * If RISCV_ROOTFS_MEM_RESERVE is enabled,
 * it will reserve the specified memory by default.
 * The reserved memory must be manually released.
 * */
/*#define RISCV_ROOTFS_MEM_RESERVE*/

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
	p = strstr(str_t, "mdns_en=");
	if(p != NULL) {
		mdns_en = simple_strtol(p + strlen("mdns_en="), NULL, 10);
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
		frame_channel_width[0] = simple_strtoul(p+strlen("init_vw="), NULL, 10);
	}

	p = strstr(str_t, "init_vh=");
	if(p != NULL) {
		frame_channel_height[0] = simple_strtoul(p+strlen("init_vh="), NULL, 10);
	}

	p = strstr(str_t, "nrvbs=");
	if(p != NULL) {
		frame_channel_nrvbs[0] = simple_strtoul(p+strlen("nrvbs="), NULL, 10);
	}

	p = strstr(str_t, "init_vw_ch1=");
	if(p != NULL) {
		frame_channel_width[1] = simple_strtoul(p+strlen("init_vw_ch1="), NULL, 10);
	}

	p = strstr(str_t, "init_vh_ch1=");
	if(p != NULL) {
		frame_channel_height[1] = simple_strtoul(p+strlen("init_vh_ch1="), NULL, 10);
	}

	p = strstr(str_t, "nrvbs_ch1=");
	if(p != NULL) {
		frame_channel_nrvbs[1] = simple_strtoul(p+strlen("nrvbs_ch1="), NULL, 10);
	}

	p = strstr(str_t, "init_vw_ch2=");
	if(p != NULL) {
		frame_channel_width[2] = simple_strtoul(p+strlen("init_vw_ch2="), NULL, 10);
	}

	p = strstr(str_t, "init_vh_ch2=");
	if(p != NULL) {
		frame_channel_height[2] = simple_strtoul(p+strlen("init_vh_ch2="), NULL, 10);
	}

	p = strstr(str_t, "nrvbs_ch2=");
	if(p != NULL) {
		frame_channel_nrvbs[2] = simple_strtoul(p+strlen("nrvbs_ch2="), NULL, 10);
	}

    p = strstr(str_t, "init_vw_ch3=");
    if(p != NULL) {
        frame_channel_width[3] = simple_strtoul(p+strlen("init_vw_ch3="), NULL, 10);
    }

    p = strstr(str_t, "init_vh_ch3=");
    if(p != NULL) {
        frame_channel_height[3] = simple_strtoul(p+strlen("init_vh_ch3="), NULL, 10);
    }

    p = strstr(str_t, "nrvbs_ch3=");
    if(p != NULL) {
        frame_channel_nrvbs[3] = simple_strtoul(p+strlen("nrvbs_ch3="), NULL, 10);
    }

    p = strstr(str_t, "init_vw_ch4=");
    if(p != NULL) {
        frame_channel_width[4] = simple_strtoul(p+strlen("init_vw_ch4="), NULL, 10);
    }

    p = strstr(str_t, "init_vh_ch4=");
    if(p != NULL) {
        frame_channel_height[4] = simple_strtoul(p+strlen("init_vh_ch4="), NULL, 10);
    }

    p = strstr(str_t, "nrvbs_ch4=");
    if(p != NULL) {
        frame_channel_nrvbs[4] = simple_strtoul(p+strlen("nrvbs_ch4="), NULL, 10);
    }

    p = strstr(str_t, "init_vw_ch5=");
    if(p != NULL) {
        frame_channel_width[5] = simple_strtoul(p+strlen("init_vw_ch5="), NULL, 10);
    }

    p = strstr(str_t, "init_vh_ch5=");
    if(p != NULL) {
        frame_channel_height[5] = simple_strtoul(p+strlen("init_vh_ch5="), NULL, 10);
    }

    p = strstr(str_t, "nrvbs_ch5=");
    if(p != NULL) {
        frame_channel_nrvbs[5] = simple_strtoul(p+strlen("nrvbs_ch5="), NULL, 10);
    }

	p = strstr(str_t, "init_vw_ch6=");
    if(p != NULL) {
        frame_channel_width[6] = simple_strtoul(p+strlen("init_vw_ch6="), NULL, 10);
    }

	p = strstr(str_t, "init_vh_ch6=");
    if(p != NULL) {
        frame_channel_height[6] = simple_strtoul(p+strlen("init_vh_ch6="), NULL, 10);
    }

	p = strstr(str_t, "nrvbs_ch6=");
    if(p != NULL) {
        frame_channel_nrvbs[6] = simple_strtoul(p+strlen("nrvbs_ch6="), NULL, 10);
    }

	p = strstr(str_t, "init_vw_ch7=");
    if(p != NULL) {
        frame_channel_width[7] = simple_strtoul(p+strlen("init_vw_ch7="), NULL, 10);
    }

	p = strstr(str_t, "init_vh_ch7=");
    if(p != NULL) {
        frame_channel_height[7] = simple_strtoul(p+strlen("init_vh_ch7="), NULL, 10);
    }

	p = strstr(str_t, "nrvbs_ch7=");
    if(p != NULL) {
        frame_channel_nrvbs[7] = simple_strtoul(p+strlen("nrvbs_ch7="), NULL, 10);
    }

	p = strstr(str_t, "init_vw_ch8=");
    if(p != NULL) {
        frame_channel_width[8] = simple_strtoul(p+strlen("init_vw_ch8="), NULL, 10);
    }

	p = strstr(str_t, "init_vh_ch8=");
    if(p != NULL) {
        frame_channel_height[8] = simple_strtoul(p+strlen("init_vh_ch8="), NULL, 10);
    }

	p = strstr(str_t, "nrvbs_ch8=");
    if(p != NULL) {
        frame_channel_nrvbs[8] = simple_strtoul(p+strlen("nrvbs_ch8="), NULL, 10);
    }

	p = strstr(str_t, "init_vw_ch9=");
    if(p != NULL) {
        frame_channel_width[9] = simple_strtoul(p+strlen("init_vw_ch9="), NULL, 10);
    }

	p = strstr(str_t, "init_vh_ch9=");
    if(p != NULL) {
        frame_channel_height[9] = simple_strtoul(p+strlen("init_vh_ch9="), NULL, 10);
    }

	p = strstr(str_t, "nrvbs_ch9=");
    if(p != NULL) {
        frame_channel_nrvbs[9] = simple_strtoul(p+strlen("nrvbs_ch9="), NULL, 10);
    }

	p = strstr(str_t, "init_vw_ch10=");
    if(p != NULL) {
        frame_channel_width[10] = simple_strtoul(p+strlen("init_vw_ch10="), NULL, 10);
    }

	p = strstr(str_t, "init_vh_ch10=");
    if(p != NULL) {
        frame_channel_height[10] = simple_strtoul(p+strlen("init_vh_ch10="), NULL, 10);
    }

	p = strstr(str_t, "nrvbs_ch10=");
    if(p != NULL) {
        frame_channel_nrvbs[10] = simple_strtoul(p+strlen("nrvbs_ch10="), NULL, 10);
    }

	p = strstr(str_t, "init_vw_ch11=");
    if(p != NULL) {
        frame_channel_width[11] = simple_strtoul(p+strlen("init_vw_ch11="), NULL, 10);
    }

	p = strstr(str_t, "init_vh_ch11=");
    if(p != NULL) {
        frame_channel_height[11] = simple_strtoul(p+strlen("init_vh_ch11="), NULL, 10);
    }

	p = strstr(str_t, "nrvbs_ch11=");
    if(p != NULL) {
        frame_channel_nrvbs[11] = simple_strtoul(p+strlen("nrvbs_ch11="), NULL, 10);
    }

	p = strstr(str_t, "direct_mode_save_kernel_ch1=");
	if(p != NULL) {
		direct_mode_save_kernel_ch1 = simple_strtoul(p+strlen("direct_mode_save_kernel_ch1="), NULL, 10);
	}

	p = strstr(str_t, "sensor_num=");
	if(p != NULL) {
		sensor_number = simple_strtoul(p+strlen("sensor_num="), NULL, 10);
	}

	p = strstr(str_t, "i2c_num=");
	if(p != NULL) {
		i2c_num = simple_strtoul(p+strlen("i2c_num="), NULL, 10);
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
	return 0;
}
static int __init cmdline_private_data(char *str)
{
	if (!str)
		return 0;

	check_sensor_num = simple_strtol(str , NULL, 10);
	printk("[%s][%d]check_sensor_num=%d\n", __func__, __LINE__, check_sensor_num);
	return 0;
}

__setup("senv", ir_switch_parse);
__setup("sensor_setting_fw=", sensor_setting_fw_parse);
__setup("riscv_fw=", riscv_fw_parse);
__setup("check_sensor_num=", cmdline_private_data);

#ifdef CONFIG_ZERATUL
static struct proc_dir_entry *zrt_proc_entry;
static struct proc_dir_entry *zrt_variable_proc_entry;
extern int zeratul_open;
#define PROC_NAME "zeratul"
enum{
	PRO_ZRT_IOCTL_TYPE = 1,
	PRO_ZRT_IOCTL_GET_JOINTSIZE,
	PRO_ZRT_IOCTL_GET_JOINT_VB_CHN,
	PRO_ZRT_IOCTL_INVALUE,
};
#define ZRT_IOCTL_TYPE 'Z'
#define ZRT_IOCTL_READ				_IOR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_TYPE, int)
#define ZRT_IOCTL_GET_JOINTSIZE	_IOR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_GET_JOINTSIZE, int)
#define ZRT_IOCTL_GET_JOINT_VB_CHN	_IOR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_GET_JOINT_VB_CHN, int)
static long zeratul_proc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case ZRT_IOCTL_READ:
		{
			if (copy_to_user((int __user *)arg, &zeratul_open, sizeof(zeratul_open)))
				return -EFAULT;
		}
		break;
	default:
		printk("Proc zeratul dir cmd invalue\n");
		return -ENOTTY;
	}

	return 0;
}

static long zeratul_variable_proc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case ZRT_IOCTL_GET_JOINTSIZE:
		{
			if (copy_to_user((int __user *)arg, &joint_sizeimage, sizeof(joint_sizeimage)))
				return -EFAULT;
		}
		break;
	case ZRT_IOCTL_GET_JOINT_VB_CHN:
		{
			if (copy_to_user((int __user *)arg, &joint_vb_chn, sizeof(joint_vb_chn)))
				return -EFAULT;
		}
		break;
	default:
		printk("Proc zeratul dir cmd invalue\n");
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations zrt_proc_fops = {
	.owner = NULL,
	.unlocked_ioctl = zeratul_proc_ioctl,
};
static const struct file_operations zrt_variable_proc_fops = {
	.owner = NULL,
	.unlocked_ioctl = zeratul_variable_proc_ioctl,
};

struct proc_dir_entry *create_proc_jz_zeratul(char *name)
{
	if(proc_jz_zeratul){
		return proc_jz_zeratul;
	}else{
		proc_jz_zeratul = jz_proc_mkdir(name);
		if (!proc_jz_zeratul) {
			pr_err("[%s %d]create_proc_entry for common reset failed.\n",__func__,__LINE__);
			return NULL;
		}
		return proc_jz_zeratul;
	}
}

static int zeratul_proc_create(void)
{
	struct proc_dir_entry *p = NULL;
	p = create_proc_jz_zeratul(PROC_NAME);
	if (!p) {
		pr_warning("[%s %d]create_proc_entry for common reset failed.\n",__func__,__LINE__);
		return -ENODEV;
	}
	zrt_proc_entry = proc_create_data("type", 0444, p, &zrt_proc_fops, NULL);
	if (!zrt_proc_entry)
		return -ENOMEM;
	zrt_variable_proc_entry = proc_create_data("variable", 0444, p, &zrt_variable_proc_fops, NULL);
	if (!zrt_variable_proc_entry)
		return -ENOMEM;

	return 0;
}
static void zeratul_proc_destory(void)
{
	proc_remove(zrt_proc_entry);
}
#endif/*CONFIG_ZERATUL*/

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
static struct tx_isp_iq_attr tx_isp_bin[MAX_SENSOR_NUMS];
static int io_map_flag = 0;
static struct sensor_calibration_head *head = NULL;

int fast_dma_declare_contiguous(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t alignment;
	phys_addr_t base_bk = base;
	/* Sanitise input arguments */
	alignment = PAGE_SIZE;
	base = ALIGN(base, alignment);
	if(base != base_bk){
		printk("Failed alloc iq reserve,base not %x align\n", alignment);
		return -EINVAL;
	}
	size = ALIGN(size, alignment);
	if (!size)
		return -EINVAL;

	/* Reserve memory */
	if (base) {
		if (memblock_is_region_reserved(base, size) ||
		    memblock_reserve(base, size) < 0) {
			base = -EBUSY;
			goto err;
		}
		return 0;
	}else{
		base = -EINVAL;
	}
err:
	return base;
}

int fast_dma_declare_free(phys_addr_t base, phys_addr_t size)
{
	int ret = 0;
	ret = memblock_free(base,size);
	if(ret){
		printk("Failed to free IQ mem reserve\n");
	}
	/*memblock_dump_all();*/
	return ret;
}

static int __init early_iq_reserve(char *str)
{
	int ret = 0;
	char *retptr1 = NULL;
	char *retptr2 = NULL;
	if (!str)
		return 0;
	iq_addr_src = memparse(str, &retptr1);
	if(*retptr1 == '@'){
		iq_addr_dec = memparse(retptr1+1, &retptr2);
		if(*retptr2 == '@'){
			iq_addr_size = memparse(retptr2+1, NULL);
		}
	}
	iq_addr_src &= 0x0FFFFFFF;
	iq_addr_dec &= 0x0FFFFFFF;
	ret = fast_dma_declare_contiguous(iq_addr_dec, iq_addr_size);
	if(ret){
		printk("Failed to IQ reserve memlock\n");
	}else{
		printk(" IQ mem reserve %#x @ %#x\n",iq_addr_dec, iq_addr_size);
	}

	return 0;
}
early_param("iq_addr_start", early_iq_reserve);


#ifdef RISCV_ROOTFS_MEM_RESERVE
static uint32_t riscv_addr_start = 0x3900000;
static uint32_t riscv_fw_size = 0x700000;
static uint32_t rootfs_addr_start = 0x2a00000;
static uint32_t rootfs_size = 0x500000;
static bool mem_mask_set = false;

int riscv_request_dma_mem_contiguous(phys_addr_t base, phys_addr_t size)
{
	mem_mask_set = true;
	return fast_dma_declare_contiguous(base, size);
}
int rootfs_request_dma_mem_contiguous(phys_addr_t base, phys_addr_t size)
{
	mem_mask_set = true;
	return fast_dma_declare_contiguous(base, size);
}

int riscv_release_dma_mem_contiguous(void)
{
	if(!mem_mask_set){
		return 0;
	}
	return fast_dma_declare_free(riscv_addr_start, riscv_fw_size);
}

int rootfs_release_dma_mem_contiguous(void)
{
	if(!mem_mask_set){
		return 0;
	}
	return fast_dma_declare_free(rootfs_addr_start, rootfs_size);
}

static int __init early_mem_riscv_reserve(char *str)
{
	char *retptr1 = NULL;
	unsigned int mem_size = 0;
	mem_size = memparse(str, &retptr1);

	if(riscv_addr_start > mem_size){
		return 0;
	}else if(mem_size > riscv_addr_start && (riscv_addr_start + riscv_fw_size > mem_size)){
		riscv_fw_size = mem_size - riscv_addr_start;
	}
	return riscv_request_dma_mem_contiguous(riscv_addr_start, riscv_fw_size);
}
static int __init early_mem_rootfs_reserve(char *str)
{
	char *retptr1 = NULL;
	unsigned int mem_size = 0,ret = 0;
	mem_size = memparse(str, &retptr1);
	if(rootfs_addr_start > mem_size){
		return 0;
	}else if(mem_size > rootfs_addr_start && (rootfs_addr_start + rootfs_size > mem_size)){
		rootfs_size = mem_size - rootfs_addr_start;
	}
	ret = rootfs_request_dma_mem_contiguous(rootfs_addr_start, rootfs_size);
	if(ret){
		printk("Failed to rootfs reserve memlock\n");
	}else{
		printk(" rootfs mem reserve %#x @ %#x\n",rootfs_addr_start, rootfs_size);
	}
	return ret;
}
early_param("mem", early_mem_riscv_reserve);
early_param("mem", early_mem_rootfs_reserve);
#endif /*RISCV_ROOTFS_MEM_RESERVE*/

int sensor_iq_bin_init(int s, int it, int st)
{

	struct sensor_calibration_info *sc_info = NULL;

	uint32_t iq_paddr = 0;
	uint32_t iq_size = 0;
	void *iq_remap = NULL;
	struct tx_isp_iq_groups *iq_tx_isp = NULL;
	IQ *iq_ft = NULL;
	iq_tx_isp = &(tx_isp_bin[s].iq_groups[it]);
	if(iq_tx_isp->iq.section[st].ready_init){
		return 0;
	}
	//head ioremap
	sc_info = &head->info;
	if(sc_info->sensor_cnt > MAX_SECTIONS_NUMS){
		printk("IQ files exceed the maximum number\n");
		goto failed_head;
	}else if(sc_info->sensor[s].iq_cnt == 0){
		printk("IQ file is invalid\n");
		goto failed_head;
	}else if(st > sc_info->sensor[s].iq_groups[it].sections_cnt){
		printk("IQ file nums is invalid\n");
		goto failed_head;
	}

	iq_ft = &(sc_info->sensor[s].iq_groups[it]);
	if(iq_ft->sections_cnt > 0){
		iq_paddr = iq_ft->vmap.section[st].offset + iq_addr_dec;
		iq_size = iq_ft->vmap.section[st].size;
		if(iq_addr_src && iq_addr_dec && iq_addr_size){
			iq_tx_isp->iq.section[st].vaddr = phys_to_virt(iq_paddr);
		}else{
			iq_remap = ioremap(iq_paddr, iq_size);
			iq_tx_isp->iq.section[st].vaddr = kmalloc(iq_size,GFP_KERNEL);
			memcpy(iq_tx_isp->iq.section[st].vaddr, iq_remap, iq_size);
			iounmap(iq_remap);
		}
		iq_tx_isp->iq.section[st].size = iq_size;
		iq_tx_isp->iq.section[st].ready_init = 1;
		iq_tx_isp->iq.section[st].crc = iq_ft->vmap.section[st].crc;
	}else{
		goto failed_section;
	}

	return 0;
failed_section:
failed_head:
	return -1;
}

int sensor_iq_bin_deinit(int s, int it, int st)
{
	return 0;
}

int sensor_setting_init(void)
{
	void *head_remap = NULL;
	if(io_map_flag == 1)
		return 0;
	if(!iq_addr_src){
		return -1;
	}
	io_map_flag = 1;
	memset(tx_isp_bin, 0, sizeof(tx_isp_bin));

	head_remap = ioremap(iq_addr_src, SENSOR_CALIBRATION_HEAD_SIZE);
	//TD not free
	head = vmalloc(SENSOR_CALIBRATION_HEAD_SIZE * sizeof(char));
	memcpy(head, head_remap, SENSOR_CALIBRATION_HEAD_SIZE);

	iounmap(head_remap);

	return 0;
}

void *get_sensor_iq_data(int sensor_t, int iq_t, int section_t)
{
	int ret = 0;
	if(section_t > MAX_SENSOR_NUMS - 1){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return NULL;
	}else if(iq_t > MAX_SECTIONS_NUMS - 1){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return NULL;
	}else if(sensor_t > MAX_SENSOR_NUMS - 1){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return NULL;
	}

	ret = sensor_setting_init();
	if(ret){
		printk("[%s %d] iq init failed\n",__func__,__LINE__);
		return NULL;
	}
	ret = sensor_iq_bin_init(sensor_t, iq_t, section_t);
	if(ret != 0){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return NULL;
	}else{
		return tx_isp_bin[sensor_t].iq_groups[iq_t].iq.section[section_t].vaddr;
	}

}


uint32_t get_sensor_iq_size(int sensor_t, int iq_t, int section_t)
{
	int ret = 0;
	if(section_t > MAX_SENSOR_NUMS - 1){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return 0;
	}else if(iq_t > MAX_SECTIONS_NUMS - 1){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return 0;
	}else if(sensor_t > MAX_SENSOR_NUMS - 1){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return 0;
	}

	ret = sensor_setting_init();
	if(ret){
		printk("[%s %d] iq init failed\n",__func__,__LINE__);
		return 0;
	}

	ret = sensor_iq_bin_init(sensor_t, iq_t, section_t);
	if(ret != 0){
		printk("[%s %d]ERROR Init IQ \n",__func__,__LINE__);
		return 0;
	}else{
		return tx_isp_bin[sensor_t].iq_groups[iq_t].iq.section[section_t].size;
	}
	return 0;
}


int read_iq(void)
{
#if 0
	char file_name[64];
	struct file *fd = NULL;
	mm_segment_t old_fs;
	loff_t *pos;
#ifndef CONFIG_INGENIC_MULTI_SENSOR
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
#endif
	return 0;
}

static ssize_t iq_addr_fops_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return read_iq();
}

static const struct file_operations iq_data_proc_fops ={
	.read = iq_addr_fops_read,
};

uint8_t *get_sensor_setting(int sensor_index, int iq_index, int iq_section)
{
	uint8_t *data = get_sensor_iq_data(sensor_index,iq_index,iq_section);
	if(data == NULL) {
		printk("[%s %d][ERROR]sensor calibration setting addr is invalid\n", __func__,__LINE__);
		return NULL;
	}
	return data;
}


int32_t get_sensor_section_num(int sensor_index, int iq_index)
{
	int ret = 0;
	int num = 0;
	struct sensor_calibration_info *sc_info = NULL;
	ret = sensor_setting_init();
	if(ret){
		printk("[%s %d] iq init failed\n",__func__,__LINE__);
		return 0;
	}

	sc_info = &head->info;
	if(sc_info->sensor_cnt > MAX_SECTIONS_NUMS){
		goto failed_head;
	}else if(sc_info->sensor[sensor_index].iq_cnt == 0){
		goto failed_head;
	}
	num = sc_info->sensor[sensor_index].iq_groups[iq_index].sections_cnt;
failed_head:
	return num;
}
/*
 *
 * Select the default startup IQ file index based on sensor_index
 * Default is 0, reserved for extended interfaces
 *
 * */
int8_t get_sensor_start_iq_index(int sensor_index)
{
	return 0;
}

uint32_t get_sensor_setting_len(int sensor_index, int iq_index, int iq_section)
{
	return get_sensor_iq_size(sensor_index,iq_index,iq_section);
}

void get_sensor_setting_md5(int sensor_index, int iq_index, int iq_section)
{
	if(tx_isp_bin[sensor_index].iq_groups[iq_index].iq.section[iq_section].ready_init)
		printk("Sensor%d-IQ%d-%s calibration crc %u\n",sensor_index,iq_index,section_name[iq_section],
		       tx_isp_bin[sensor_index].iq_groups[iq_index].iq.section[iq_section].crc);
	else{
		printk("ERROR:The IQ file buffer has not been initialized.");
	}
}

uint8_t *get_sensor_setting_date(int sensor_index, int iq_index, int iq_section)
{
	return NULL;
}
#endif


static int __init tx_isp_PRJ008_init(void)
{
	int retval = 0;
	if (sensor_setting_fw == -1 || riscv_fw == -1) {
		printk("tx-isp-PRJ008 not init(%d,%d)\n",sensor_setting_fw, riscv_fw);
	}else{
		retval = tx_isp_init();
	}
#ifdef CONFIG_BLK_DEV_INITRD
	zrootfs_start();
#endif
#ifdef CONFIG_ZERATUL
	zeratul_proc_create();
#endif
	proc_create("iq_data", S_IRUGO, NULL, &iq_data_proc_fops);
	return retval;
}

static void __exit tx_isp_PRJ008_exit(void)
{
	tx_isp_exit();
#ifdef CONFIG_ZERATUL
	zeratul_proc_destory();
#endif
#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
	fast_dma_declare_free(iq_addr_dec, iq_addr_size);
#endif
}
#ifdef CONFIG_ZERATUL
#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
subsys_initcall(tx_isp_PRJ008_init);
#else
module_init(tx_isp_PRJ008_init);
#endif/*CONFIG_BUILT_IN_SENSOR_SETTING*/

module_exit(tx_isp_PRJ008_exit);
#endif/*CONFIG_ZERATUL*/
MODULE_AUTHOR("Ingenic xhshen");
MODULE_DESCRIPTION("tx isp driver");
MODULE_LICENSE("GPL");

