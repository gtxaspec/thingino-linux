/*
 * Copyright (C) 2016 Novatek MicroElectronics Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/thermal.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <plat/adc_reg.h>
#include <plat/efuse_protected.h>
#include <plat/top.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#define DRV_VERSION "1.06.002"

#ifdef CONFIG_NVT_FPGA_EMULATION
#define CALI_VDDADC 3300
#else
#define CALI_VDDADC 2700
#endif
#define CALI_OFFSET 0

#define DEFAULT_THERMAL_LEVEL 259
#define DEFAULT_THERMAL_528_LEVEL 231

#define ADC_CHANNEL(num, id)                    \
	{                           \
		.type = IIO_VOLTAGE,                \
		.indexed = 1,                   \
		.channel = num,                 \
		.address = num,                 \
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_AVERAGE_RAW),   \
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),\
		.scan_index = num,              \
		.scan_type = {                  \
			.sign = 's',                \
			.realbits = 9,             \
			.storagebits = 9,          \
			.endianness = IIO_CPU,          \
		},                      \
		.datasheet_name = id,			\
	}

static uint nvt_adc_poll_mode = 1;
module_param(nvt_adc_poll_mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_adc_poll_mode, "1:poll mode, 0:irq mode");

static uint nvt_adc_pow2div = 7;
module_param(nvt_adc_pow2div, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_adc_pow2div, "adc sample clock pow2 div");

static uint nvt_adc_plusdiv = 0;
module_param(nvt_adc_plusdiv, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_adc_plusdiv, "adc sample clock fine tune plus div");

struct nvt_adc_state {
	u32 sample_rate;
	struct completion data_complete;
	u32 interrupt_status;
	u32 base;
	struct clk *clk;
	unsigned long trim_value;
	u32 cali_value;
	struct delayed_work     adc_avg_work;// cal adc mid work
	struct proc_dir_entry *pproc_adc;
	
};

// ADC Average func //
// data 0
static UINT32 *t_data = NULL;
static UINT32 t_data_now = 0; // store t_data amount
static UINT32 adc_mid_val = 0; // store average value
static UINT32 adc_mid_vol = 0; // store average voltage
static UINT32 adc_mid = 0; 
static UINT32 adc_mid_first = 0; // first read ready 

// data_1 
static UINT32 *t_data1 = NULL;
static UINT32 t_data_now1 = 0; // store t_data amount
static UINT32 adc_mid1_val = 0; // store average value
static UINT32 adc_mid1_vol = 0; // store average voltage
static UINT32 adc_mid1 = 0;
static UINT32 adc_mid1_first = 0; // mid2 first read ready 

static u32 adc_base = 0;
static UINT32 adc_trim = 0;

static UINT32 adc_avg_times = 50;
static UINT32 filter = 3 ;
static UINT32 avg_sel = 0;
static UINT32 avg_sel_sts = 0;


static u32 adc_getreg(struct nvt_adc_state *adc, u32 offset)
{
	return nvt_readl(adc->base + offset);
}

static void adc_setreg(struct nvt_adc_state *adc, u32 offset, u32 value)
{
	nvt_writel(value, adc->base + offset);
}

static int adc_chk_enable(struct nvt_adc_state *adc)
{
	union ADC_CTRL_REG ctrl_reg;

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);

	return ctrl_reg.bit.adc_en;
}

static void adc_set_enable(struct nvt_adc_state *adc, int input)
{
	union ADC_CTRL_REG ctrl_reg;

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);
	ctrl_reg.bit.adc_en = input;
	adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);
}

static void adc_set_default_hw(struct nvt_adc_state *adc)
{
	union ADC_CTRL_REG ctrl_reg;
	union ADC_INTCTRL_REG intctrl_reg;

	intctrl_reg.reg = 0xFF;
	adc_setreg(adc, ADC_INTCTRL_REG_OFS, intctrl_reg.reg);

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);
	ctrl_reg.bit.ain0_mode = 1;
	ctrl_reg.bit.ain1_mode = 1;
	ctrl_reg.bit.ain2_mode = 1;
	ctrl_reg.bit.ain3_mode = 1;
	ctrl_reg.bit.clkdiv = nvt_adc_pow2div;
	ctrl_reg.bit.sampavg = 0;
	ctrl_reg.bit.extsamp_cnt = nvt_adc_plusdiv;
	ctrl_reg.bit.adc_en = 1;
	adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);
}

#if 1
static int nvt_adc_read_avg_int(void)
{
	t_data = (int*)kmalloc(sizeof(int) * adc_avg_times , GFP_KERNEL);
	memset(t_data ,0, adc_avg_times*sizeof(int));

	t_data1 = (int*)kmalloc(sizeof(int) * adc_avg_times , GFP_KERNEL);
	memset(t_data1 ,0, adc_avg_times*sizeof(int));
	
	return 1;
}

static void nvt_adc_read_avg(struct work_struct *work) //(struct nvt_adc_state *adc, u8 channel)
{
	UINT32 adc_data = 0;
	UINT32 i = 0, temp = 0;
	UINT32 channel = 0;
	union ADC_CTRL_REG ctrl_reg;

	struct nvt_adc_state *adc = container_of(work, struct nvt_adc_state,
					    adc_avg_work.work);
	
	// Check ADC is Enable
	ctrl_reg.reg = nvt_readl(adc_base +  ADC_CTRL_REG_OFS);
	if(!ctrl_reg.bit.adc_en){
		ctrl_reg.reg = nvt_readl(adc_base +  ADC_CTRL_REG_OFS);
		ctrl_reg.bit.adc_en = 1;
		nvt_writel(ctrl_reg.reg, adc_base + ADC_CTRL_REG_OFS);
	}
	
	//adc_mid_val = adc_mid;
	//adc_mid_vol = (adc_mid +CALI_OFFSET)*adc_trim/511;
	//printk("\n  adc_mid = %d ,  adc_mid_val = %d ,  adc_mid_vol = %d \r\n\n", adc_mid, adc_mid_val, adc_mid_vol);

	// data 0
	if( avg_sel == 0 ){
		
		if (adc_mid_first == 0 ){ // firstly ready
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;
			}
			adc_mid  = nvt_readl(adc_base + ADC_AIN0_DATA_REG_OFS + channel*4);
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;

			}else{
				adc_mid_val = adc_mid;
				adc_mid_vol = (adc_mid + CALI_OFFSET)*adc_trim/511;
				adc_mid_first++ ;
			}
			
		}else { 
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;
			}
			temp  = nvt_readl(adc_base + ADC_AIN0_DATA_REG_OFS + channel*4);
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;

			}else {
			
				if( (temp <= (adc_mid + filter)) && (temp >= (adc_mid - filter) )  ){
					t_data[t_data_now] = temp;
					t_data_now++;
				}
				/*if (t_data_now != 0){
					printk("1.temp = %d,  t_data[t_data_now = %d] = %d \r\n",temp, t_data_now-1, t_data[t_data_now-1]);
				}*/
			
				// calculate average
				if (t_data_now >= adc_avg_times ){
					adc_data = 0;
					for( i = 0 ; i < adc_avg_times ; i++ ){
						adc_data += t_data[i];
					}

					adc_data = adc_data / adc_avg_times ;

					adc_mid_val = adc_data;
					adc_mid = adc_data;
					t_data_now = 0 ;
					
					adc_mid_val = adc_mid;
					adc_mid_vol = (adc_mid +CALI_OFFSET)*adc_trim/511;

					//printk("reset0 adc_mid_val = %d,  adc_mid_vol = %d \r\n",adc_mid_val, adc_mid_vol);
				}
			}
		}

		
	// data 1	
	} else if( avg_sel == 1 ) {
	
		if (adc_mid1_first == 0 ){ // firstly ready
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;
			}
			adc_mid1  = nvt_readl(adc_base + ADC_AIN0_DATA_REG_OFS + channel*4);
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;
			} else {
				adc_mid1_val = adc_mid1;
				adc_mid1_vol = (adc_mid1 + CALI_OFFSET)*adc_trim/511;
				adc_mid1_first++ ;
			}
			
			
		}else{
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;
			} 
			temp  = nvt_readl(adc_base + ADC_AIN0_DATA_REG_OFS + channel*4);
			
			if (avg_sel_sts == 1 ){
				mdelay(3);
				avg_sel_sts = 0;

			} else {
				if( (temp <= (adc_mid1 + filter)) && (temp >= (adc_mid1 - filter) )  ){
					t_data1[t_data_now1] = temp;
					t_data_now1++;
				}

				/*if (t_data_now1 != 0){
					printk("2.temp = %d,  t_data1[t_data_now1 = %d] = %d \r\n",temp ,t_data_now1-1, t_data1[t_data_now1-1]);
				}*/
			
				// calculate average
				if (t_data_now1 >= adc_avg_times ){
					adc_data = 0;
					for( i = 0 ; i < adc_avg_times ; i++ ){
						adc_data += t_data1[i];
					}

					adc_data = adc_data / adc_avg_times ;

					adc_mid1_val = adc_data;
					adc_mid1 = adc_data;
					t_data_now1 = 0 ;
					
					adc_mid1_val = adc_mid1;
					adc_mid1_vol = (adc_mid1 +CALI_OFFSET)*adc_trim/511;

					//printk("reset1 adc_mid1 = %d,  adc_mid1_vol = %d \r\n",adc_mid1, adc_mid1_val);
				}
			}
		}
	}
		
	schedule_delayed_work(&adc->adc_avg_work, msecs_to_jiffies(10));
}
#endif
#if 1

#define MAX_CMD_LENGTH 10
static ssize_t data_proc_cmd_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
{
	int val;
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *p_arg;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		printk("%s: Command length %d is too long\n", __func__, len);
		return -EINVAL;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		return -EINVAL;
	}

	if (len == 0)
		cmd_line[0] = '\0';
	else
		cmd_line[len - 1] = '\0';
	
	p_arg = strsep(&cmdstr, delimiters);
	sscanf(p_arg, "%x", &val);

	// set avg_sel = 0 or 1 for 1 CH connect two device 
	avg_sel = val;
	avg_sel_sts = 1;

	//printk("avg_sel = %d \n", avg_sel);

	return size;
}


static int adc_proc_show(struct seq_file *seq, void *v)
{
	UINT32 volt = 0 ;
	//printk("adc_avg_value  = %d, adc_avg_voltage = %d \n", adc_mid_val, adc_mid_vol);
	//printk("adc_avg2 value = %d, adc2_avg_voltage = %d \n", adc_mid1_val, adc_mid1_vol);
	if ( avg_sel == 0 ){
		//printk("%d\n", adc_mid_vol);
		volt = adc_mid_vol;
	}else{
		//printk("%d\n", adc_mid1_vol);
		volt = adc_mid1_vol;
	}
	seq_printf(seq,"%d\n",volt);
	
	return 0;
}

static int adc_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, adc_proc_show, NULL);
}

static struct file_operations proc_adc_fops = {
        .owner   = THIS_MODULE,
        .open    = adc_proc_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = single_release,
		.write	 = data_proc_cmd_write,
};




#endif

static u32 nvt_adc_get_value(struct nvt_adc_state *adc, u8 channel)
{
	u16 adc_data;

	adc_data = adc_getreg(adc, ADC_AIN0_DATA_REG_OFS + channel*4);
	return (adc_data+CALI_OFFSET)*adc->cali_value/511;
}

static int nvt_adc_read_raw(struct iio_dev *iio,
				struct iio_chan_spec const *channel, int *value,
				int *shift, long mask)
{
	struct nvt_adc_state *adc = iio_priv(iio);

	if (!adc_chk_enable(adc))
		adc_set_enable(adc, 1);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if(nvt_adc_poll_mode == 0)
			wait_for_completion(&adc->data_complete);

		*value = nvt_adc_get_value(adc, channel->address);
		return IIO_VAL_INT;

	// Read Average
	case IIO_CHAN_INFO_AVERAGE_RAW:
		if ( avg_sel == 0 ){
			*value = adc_mid_vol;
		}else{
			*value = adc_mid1_vol;
		}
		
		return IIO_VAL_INT;
		
	case IIO_CHAN_INFO_SCALE:
		printk("IIO_CHAN_INFO_SCALE\n");
		return IIO_VAL_FRACTIONAL_LOG2;

	default:
		break;
	}

	return -EINVAL;
}

static irqreturn_t nvt_adc_irq(int irq, void *devid)
{
	struct nvt_adc_state *adc = devid;

	adc->interrupt_status = adc_getreg(adc, ADC_STATUS_REG_OFS);

	if (adc->interrupt_status) {
		adc_setreg(adc, ADC_STATUS_REG_OFS, adc->interrupt_status);
		complete(&adc->data_complete);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

static ssize_t nvt_adc_enable(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t len)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);
	unsigned long input;
	int ret;

	ret = kstrtoul(buf, 10, &input);
	if (ret < 0)
		return ret;

	adc_set_enable(adc, input);

	return len;
}

static ssize_t nvt_adc_is_enable(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);

	return sprintf(buf, "%d\n", adc_chk_enable(adc));
}

static ssize_t nvt_adc_get_poll_mode(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	return sprintf(buf, "%d\n", nvt_adc_poll_mode);
}

static u32 nvt_adc_set_frequency(struct nvt_adc_state *adc, u32 frequency)
{
	int ret = 0;
	union ADC_CTRL_REG ctrl_reg;

	if (frequency > 38461) {
		pr_err("\nmax frequency is 38461 Hz, set value is %d\n", frequency);
		ret = -1;
		return ret;
	}

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);

	if (ctrl_reg.bit.adc_en) {
		pr_err("\nplease disable adc first\n");
		ret = -1;
		return ret;
	}

	nvt_adc_pow2div = ( clk_get_rate(adc->clk) / (frequency * 328));

	nvt_adc_plusdiv = ( (clk_get_rate(adc->clk) / (2 * (nvt_adc_pow2div + 1))) / (frequency * 4) - 26);

	adc->sample_rate = (clk_get_rate(adc->clk) / (2 * (nvt_adc_pow2div + 1))) / (4 * (26 + nvt_adc_plusdiv));

	pr_info("\nset frequency is %d\n", adc->sample_rate);

	ctrl_reg.bit.clkdiv = nvt_adc_pow2div;
	ctrl_reg.bit.extsamp_cnt = nvt_adc_plusdiv;
	adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);

	return ret;
}

static ssize_t nvt_adc_set_in_clk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t len)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);
	unsigned long input;
	int ret;

	ret = kstrtoul(buf, 10, &input);
	if (ret < 0)
		return ret;

	nvt_adc_set_frequency(adc, input);

	return len;
}

static ssize_t nvt_adc_get_in_clk(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);

	return sprintf(buf, "%d\n", adc->sample_rate);
}

static const struct iio_chan_spec nvt_adc_channels[] = {
	IIO_CHAN_SOFT_TIMESTAMP(6),
	ADC_CHANNEL(0, "adc0"),
	ADC_CHANNEL(1, "adc1"),
	ADC_CHANNEL(2, "adc2"),
};

/* for consumer drivers */
static struct iio_map nvt_adc_default_maps[] = {
	IIO_MAP("adc0", "nvt-adc0", "adc0"),
	IIO_MAP("adc1", "nvt-adc1", "adc1"),
	IIO_MAP("adc2", "nvt-adc2", "adc2"),
	{},
};

static IIO_DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, nvt_adc_is_enable, nvt_adc_enable, 0);
static IIO_DEVICE_ATTR(poll_mode, S_IRUGO, nvt_adc_get_poll_mode, 0, 0);
static IIO_DEVICE_ATTR(frequency, S_IRUGO | S_IWUSR, nvt_adc_get_in_clk, nvt_adc_set_in_clk, 0);

static struct attribute *nvt_adc_attributes[] = {
	&iio_dev_attr_enable.dev_attr.attr,
	&iio_dev_attr_poll_mode.dev_attr.attr,
	&iio_dev_attr_frequency.dev_attr.attr,
	NULL,
};

static const struct attribute_group nvt_attribute_group = {
	.attrs = nvt_adc_attributes,
};

static const struct iio_info nvt_adc_info = {
	.read_raw = &nvt_adc_read_raw,
	.attrs = &nvt_attribute_group,
};

static void thermal_get_trimdata(struct nvt_adc_state *adc)
{
	u16 data = 0x0;

	if (efuse_readParamOps(EFUSE_THERMAL_TRIM_DATA, &data)) {
		if (nvt_get_chip_id() == CHIP_NA51055)
			adc->trim_value = DEFAULT_THERMAL_LEVEL;
		else
			adc->trim_value = DEFAULT_THERMAL_528_LEVEL;
	} else
		adc->trim_value = data;
}

static int thermal_get_temp(struct thermal_zone_device *thermal, int *temp)
{
	struct nvt_adc_state *adc = thermal->devdata;
	union THERMAL_SENSOR_CONFIGURE_REG thermal_reg;
	union ADC_CTRL_REG ctrl_reg;
	signed long temp_value;

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);
	if (!ctrl_reg.bit.adc_en) {
		ctrl_reg.bit.adc_en = 1;
		adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);
	}

	thermal_reg.reg = adc_getreg(adc, THERMAL_SENSOR_CONFIGURE_REG_OFS);
	temp_value = thermal_reg.bit.ain_avg_out;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		*temp = (temp_value - adc->trim_value)*13;
		*temp = *temp/10 + 30;
	} else {
		*temp = (temp_value - adc->trim_value)*1319;
		*temp = *temp/1000 + 30;
	}

	return 0;
}

static struct thermal_zone_device_ops ops = {
	.get_temp = thermal_get_temp,
};

#ifdef CONFIG_OF
static const struct of_device_id nvt_adc_of_dt_ids[] = {
	{ .compatible = "nvt,nvt_adc", },
	{},
};

MODULE_DEVICE_TABLE(of, nvt_adc_of_dt_ids);
#endif

#if (defined(CONFIG_NVT_IVOT_PLAT_NA51055) || defined(CONFIG_NVT_IVOT_PLAT_NA51089))
static void nvt_adc_get_trimdata(struct nvt_adc_state *adc)
{
	u16 data0, data1 = 0x0;
	u32 slope, cali_value = 0x0;
	u32 upper_bound = CALI_VDDADC + 200;
	u32 lower_bound = CALI_VDDADC - 200;

#if (defined(CONFIG_NVT_IVOT_PLAT_NA51055))
    if (efuse_readParamOps(EFUSE_ADC_TRIM_A_DATA, &data0)) {
		adc->cali_value = CALI_VDDADC;
	} else {
	    data0 = ((data0 & 0x3F80)>>7); //Trim A @ bit[13..7]

        if (efuse_readParamOps(EFUSE_ADC_TRIM_B_DATA, &data1) ) {
    	    adc->cali_value = CALI_VDDADC;
        } else {
    	    data1 = (data1 & 0x1FF); //Trim B @ bit[8..0]
            slope = data1 - data0;
            cali_value = (511 - data0) * 1000;
            cali_value = 2*(cali_value/slope) + 500;

            if ((cali_value > upper_bound) || (cali_value < lower_bound))
            	adc->cali_value = CALI_VDDADC;
            else
            	adc->cali_value = cali_value;
        }
	}
#else  //#if (defined(CONFIG_NVT_IVOT_PLAT_NA51089)
	if (efuse_readParamOps(EFUSE_ADC_TRAM_A_DATA, &data0)) {
		adc->cali_value = CALI_VDDADC;
	} else {
		if (efuse_readParamOps(EFUSE_ADC_TRAM_B_DATA, &data1)) {
			adc->cali_value = CALI_VDDADC;
		} else {
			slope = data1 - data0;
			cali_value = (511 - data0) * 1000;
			cali_value = 2*(cali_value/slope) + 500;

			if ((cali_value > upper_bound) || (cali_value < lower_bound))
				adc->cali_value = CALI_VDDADC;
			else
				adc->cali_value = cali_value;
		}
	}
#endif
	adc_trim = adc->cali_value;
	//printk("adc_trim = %d  \r\n",adc_trim);
}
#endif

static int nvt_adc_probe(struct platform_device *pdev)
{
	struct iio_dev *iio;
	struct nvt_adc_state *adc;
	int err = 0, irq , i=0;
	struct resource *res;
	struct thermal_zone_device *thermal_zone = NULL;
	int ret_avg = 0;
	struct proc_dir_entry *pentry = NULL;
	
	union ADC_AIN0_DATA_REG data_reg;
	union ADC_STATUS_REG sts_reg;
	
#ifdef CONFIG_OF
	const struct of_device_id *of_id;

	of_id = of_match_device(nvt_adc_of_dt_ids, &pdev->dev);
	if (!of_id) {
		dev_err(&pdev->dev, "[NVT ADC] OF not found\n");
		return -EINVAL;
	}
	//pr_info("%s %d: of_id->compatible = %s\n", __func__, __LINE__,  of_id->compatible);
#endif

	iio = iio_device_alloc(sizeof(struct nvt_adc_state) );
	if (!iio){
		return -ENOMEM;
	}

	adc = iio_priv(iio);
	if (!adc ){
		err = -ENOMEM;
		goto iio_free;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("%s: get IO resource fail\n", __func__);
		goto iio_free;
	}

	adc->base = (u32)devm_ioremap_resource(&pdev->dev, res);
	if (!adc->base)
		goto iio_free;

	adc_base = (u32)adc->base;

	adc->clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (!IS_ERR(adc->clk)) {
		clk_prepare(adc->clk);
		clk_enable(adc->clk);
	} else {
		pr_err("%s: %s clk not found\n", __func__, dev_name(&pdev->dev));
		goto iio_free;
	}

	if(nvt_adc_poll_mode == 0) {
		irq = platform_get_irq(pdev, 0);
		if (irq < 0) {
			pr_err("%s: get irq fail %d\n", __func__, irq);
			goto iio_free;
		}

		init_completion(&adc->data_complete);

		err = request_irq(irq, nvt_adc_irq, 0, pdev->name, adc);
		if (err < 0) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
			goto iio_free;
		}
	}

#if (defined(CONFIG_NVT_IVOT_PLAT_NA51055) || defined(CONFIG_NVT_IVOT_PLAT_NA51089))
     nvt_adc_get_trimdata(adc);
#else
	if (of_property_read_u32(pdev->dev.of_node, "cali_value", &adc->cali_value))
		adc->cali_value = CALI_VDDADC;
#endif

	adc->sample_rate = (clk_get_rate(adc->clk) / (2 * (nvt_adc_pow2div + 1))) / (4 * (26 + nvt_adc_plusdiv));

	adc_set_default_hw(adc);
	
	iio->name = "nvt_adc";
	iio->modes = INDIO_DIRECT_MODE;
	iio->info = &nvt_adc_info;
	iio->dev.parent = &pdev->dev;
	iio->dev.of_node = pdev->dev.of_node;
	iio->channels = (void *)&nvt_adc_channels;
	iio->num_channels = sizeof(nvt_adc_channels)/sizeof(nvt_adc_channels[0]);
	err = iio_map_array_register(iio, nvt_adc_default_maps);
	if (err < 0)
		goto iio_free;

	err = iio_device_register(iio);
	if(err)
		goto iio_free;

	pr_info("%s: %s done voltage %d ver %s\n", __func__, dev_name(&pdev->dev), adc->cali_value, DRV_VERSION);

	udelay(30);

	// if data ready
	for( i = 0 ; i< 5 ; i++ ){
		sts_reg.reg = adc_getreg(adc, ADC_STATUS_REG_OFS); 
		if(sts_reg.bit.ain0_datardy ==1 ){
			break;
		}
	udelay(50);	
	}
	
	if (i==5){
		printk("\n adc ready timout   !!\r\n");
	}
	data_reg.reg = adc_getreg(adc, ADC_AIN0_DATA_REG_OFS);

	if ( avg_sel == 0 ){
		adc_mid = data_reg.reg;
		adc_mid_first = 1 ; // already get firstly value 
		adc_mid_val = adc_mid;
		adc_mid_vol = (adc_mid +CALI_OFFSET)*adc_trim/511;
		
		
	}else if(avg_sel == 1 ) {
		adc_mid1 = data_reg.reg;
		adc_mid1_first = 1 ; // already get firstly value 
		adc_mid1_val = adc_mid1;
		adc_mid1_vol = (adc_mid1 +CALI_OFFSET)*adc_trim/511;
		
	}


#if 1
    // ADC Average data
	/* tempeture compensation */
	ret_avg = nvt_adc_read_avg_int();

	if(ret_avg){
       	INIT_DELAYED_WORK(&adc->adc_avg_work, nvt_adc_read_avg);
       	schedule_delayed_work(&adc->adc_avg_work, msecs_to_jiffies(1000));
	}

	pentry = proc_create("adc", 0, NULL, &proc_adc_fops);
    if (pentry == NULL) {
            dev_err(&pdev->dev, "failed to create adc proc!\n");
            err = -EINVAL;
            goto t_data_free;
    }
	adc->pproc_adc = pentry;
#endif

	thermal_zone = thermal_zone_device_register("nvt_thermal", 0, 0,
				   adc, &ops, NULL, 0, 0);
	if (IS_ERR(thermal_zone)) {
		dev_err(&pdev->dev, "thermal zone device is NULL\n");
		err = PTR_ERR(thermal_zone);
		goto t_data_free;
	}

	thermal_get_trimdata(adc);

	dev_info(&thermal_zone->device, "Thermal Sensor probe\n");

	platform_set_drvdata(pdev, thermal_zone);

	return 0;
	
t_data_free:
	cancel_delayed_work(&adc->adc_avg_work);
	kfree(t_data);
iio_free:
	iio_device_free(iio);

	return err;
}

static int nvt_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *iio = platform_get_drvdata(pdev);
	struct nvt_adc_state *adc ;

	adc = iio_priv(iio);
	
	cancel_delayed_work(&adc->adc_avg_work);
	iio_device_unregister(iio);
	iio_device_free(iio);
	remove_proc_entry("adc", NULL);
	kfree(t_data);

	return 0;
}


static struct platform_driver nvt_adc_driver = {
	.driver = {
		.name = "nvt_adc",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nvt_adc_of_dt_ids,
#endif
	},
	.probe = nvt_adc_probe,
	.remove = nvt_adc_remove,
};

static int __init nvt_adc_init_driver(void)
{
	return platform_driver_register(&nvt_adc_driver);
}
module_init(nvt_adc_init_driver);

static void __exit nvt_adc_exit_driver(void)
{
	platform_driver_unregister(&nvt_adc_driver);
}
module_exit(nvt_adc_exit_driver);

MODULE_AUTHOR("Novatek");
MODULE_VERSION(DRV_VERSION);
MODULE_DESCRIPTION("NVT ADC driver");
MODULE_LICENSE("GPL v2");
