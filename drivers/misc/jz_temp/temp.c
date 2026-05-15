/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 *
 * */
#include <asm-generic/errno-base.h>
#include <asm/errno.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/mfd/core.h>
#include <linux/mutex.h>
#include <linux/range.h>
#include <linux/stddef.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#ifdef CONFIG_KERNEL_3_10
#include <soc/base.h>
#include "mach/jzdma.h"
#include "soc/irq.h"
#endif
#include <linux/jz_adc.h>
#include <jz_proc.h>
#include "temp.h"

#ifdef CONFIG_MFD_JZ_SADC_AUX
#define ADCUSEMFD (1)
#else
#define ADCUSEMFD (0)
#endif

DEFINE_MUTEX(tmutex);
struct mutex adcmutex;/*ADC and Tsensor mutex*/
#ifdef CONFIG_TS_AUTO_REFI
static void ts_AutoRefi_IsOn(unsigned int is_on);
#endif

static inline unsigned int stemp_read(struct stemp_operation *stemp_ope, int offset) {
	return readl(stemp_ope->iomem + offset);
}

static inline void stemp_write(struct stemp_operation *stemp_ope, int offset, unsigned int val) {
	writel(val, stemp_ope->iomem + offset);
}

#ifdef TSENSOR_DBUG
static void stemp_dump(struct stemp_operation *stemp_ope) {
	printk("0x10070000 : %#x\n",stemp_read(stemp_ope,ADENA));
	printk("0x10070004 : %#x\n",stemp_read(stemp_ope,ADCFG));
	printk("0x10070008 : %#x\n",stemp_read(stemp_ope,ADCTRL));
	printk("0x1007000c : %#x\n",stemp_read(stemp_ope,ADSTATE));
	printk("0x10070010 : %#x\n",stemp_read(stemp_ope,ADATA0));
	printk("0x10070014 : %#x\n",stemp_read(stemp_ope,ADATA1));
	printk("0x10070020 : %#x\n",stemp_read(stemp_ope,ADCLK));
	printk("0x10070024 : %#x\n",stemp_read(stemp_ope,ADSTB));
	printk("0x10070028 : %#x\n",stemp_read(stemp_ope,ADRETM));
	return ;
}
#endif

static int get_temp_value(int index,int tsensorout,int valueh,int valuel,int div_num)
{
	int i = 0,temp = 0 ,ret = 0;
	int zs_temp,fl_temp;
	i = index;
	temp = temprange[i];

	/*将5℃分成（valueh-valuel）份， 该值通常很小,整除后误差较大，因此，需要将改成放大1000倍，得到一个整数
	*那么实际的温度值如下计算。
	*/
	ret = div_num * 1000 / (valueh - valuel);
	temp = temp*1000 + ret*(valueh - tsensorout);

	zs_temp = temp / 1000;
	fl_temp = temp % 1000;

	// printk("ret:%d,j=%d,temp:%d,valueh:%d,tsenout:%d,valuel:%d,T:%d.%d\n",ret,j,temp,valueh,tsensorout,valuel,zs_temp,fl_temp);
	return temp;
}
static int lookup_temp(int tsensorout)
{
	//lookup key table
	int temperature = 0;
	int i = 0, size = 0;
	size = sizeof(tsensor_value) / sizeof(tsensor_value[0]);

	for(i = 0; i < size - 1;i++){
		if(tsensorout > tsensor_value[0] || tsensorout < tsensor_value[size-1]){
			printk("err!!!! tsensor out of temprange\n");
			return -EINVALTEMP;
		}

		if( tsensorout <= tsensor_value[i] && tsensorout >= tsensor_value[i+1]) {
			temperature = get_temp_value(i,tsensorout,tsensor_value[i],tsensor_value[i+1],TEMP_DIV_NUM);
			return temperature;
		}
	}

	if(i == size){
		printk("not find,tsensorout:%d,i:%d,range[%d,%d]\n",tsensorout,i,tsensor_value[i],tsensor_value[i-1]);
	}

	return -EINVALTEMP;
}

int stemp_setup_config2(struct stemp_operation *stemp_ope)
{
	int ret = 0;
	static int times = 0;
	if(!stemp_ope){
		printk("stemp_ope is null!\n");
		return -ENOMEM;
	}
	//.configure ADCLK
	ret = stemp_read(stemp_ope,ADCLK);
	// stemp_write(stemp_ope, ADCLK, ret | (3 << 16) | 1<<1);
	stemp_write(stemp_ope, ADCLK, 0x4000c);
	// printk("clkret:%#x,aret:%#x\n",ret,stemp_read(stemp_ope,ADCLK));

	//config2 ADCCFG.ADC_RST 0 to 1
	ret = stemp_read(stemp_ope,ADCFG);
	stemp_write(stemp_ope, ADCFG, ret & ~(1<<0));
	msleep(10);
	ret = stemp_read(stemp_ope,ADCFG);
	stemp_write(stemp_ope, ADCFG, ret | 0x1);

	//1、configure ADENA.PD to 0 to power on SAR A/D
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, (ret & ~(1<<15))|(1<<10));

	//2.wait 1ms,then set ADCFG.temp_TRIM to 0xC0F
	msleep(1);
	ret = stemp_read(stemp_ope,ADCFG);
	// stemp_write(stemp_ope, ADCFG, ret | (0xC0F << 8));
	stemp_write(stemp_ope, ADCFG, ret | (0x2 << 8));
	// printk("ret:%#x,ret:%#x\n",stemp_read(stemp_ope,ADCFG),ret);
	//3.set ADENA.tempEN to 1
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret | 1<<8);
	// printk("ADENA:%#x,ret:%#x\n",stemp_read(stemp_ope,ADENA),ret);

	//4.Configure ADSTB register
	//TODO:要配置成多少呢？暂时不配置
	ret = stemp_read(stemp_ope,ADSTB);
	stemp_write(stemp_ope, ADSTB, ret | 4);//delay periods of sampling
	// printk("ret:%#x,aret:%#x\n",ret,stemp_read(stemp_ope,ADSTB));

STEP:
	//6.wait at least 1ms ,configure ADENA.TSENSOR_EN as 1 to enalbe the temperature channel
	msleep(1);
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret | 1<<3);

	//7.wait until ADSTATE.ARDY3 is 1 and write 1 to clear the flag
	while(!(ret = stemp_read(stemp_ope,ADSTATE) & (1<<3))){
		printk("wait ARDY3 to 1,ret:%#x\n",ret);
		msleep(100);
	}


	stemp_write(stemp_ope, ADSTATE, ret | 1<<3);

	//8.read temperature data from adata1
	ret = stemp_read(stemp_ope,ADATA1);

	msleep(1000);
	times++;
	printk("times:%d\n\n",times);
goto STEP;
	// //6.wait at least 9ms ,configure ADENA.TSENSOR_EN as 1 to enalbe the temperature channel
	// ret = stemp_read(stemp_ope,ADENA);
	// stemp_write(stemp_ope, ADENA, ret | 1<<3);

	/*****disable chn******/
	//9.configure ADENA.TSENSOR_EN as 0 to disalbe the temperature chan
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret & ~(1<<3));

	//10. set ADENA.tempEN to 0
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret & ~(1<<8));

	//11.set ADCFG.temp_TRIM to 0
	ret = stemp_read(stemp_ope,ADCFG);
	stemp_write(stemp_ope, ADCFG, ret & ~(0xffffffff << 8));


	return 0;
}

static void temp_compensation(int *ptempdata)
{
	int temp;
	int temp_compensation = 0;

	temp = *ptempdata;
	temp_compensation = readl((void *)EFUSETSENSOR);
#ifdef CONFIG_SOC_PRJ008
	//bit15 0 is `+` ;1 is `-`
	if( temp_compensation & (1<<15)) {
		temp -= ((temp_compensation >> 8) & (0x7f));//bit15~bit8 is compensation
	} else {
		temp += ((temp_compensation >> 8) & (0x7f));
	}
#else
	//bit11 0 is `+` ;1 is `-`
	if( temp_compensation & (1<<11)) {
		temp -= (temp_compensation & 0x07ff);//bit10~bit0 is compensation
	} else {
		temp += (temp_compensation & 0x07ff);
	}
#endif
	*ptempdata = temp;

	return ;
}

static int tsensor_get_temp(struct stemp_operation *stemp_ope)
{
	int ret = 0,temperature = 0;

	if(!stemp_ope){
		printk("stemp_ope is null!\n");
		return -ENOMEM;
	}
	mutex_lock(&adcmutex);
	//0.configure ADCLK
	//config 24/12 = 2M
	ret = stemp_read(stemp_ope,ADCLK);
	stemp_write(stemp_ope, ADCLK, ret | 0xc);

	//1、configure ADENA.PD to 0 to power on SAR A/D
	// SELEN set 1
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, (ret & ~(1<<15)) | (1<<10));

	//2.wait 1ms,then set ADCFG.temp_TRIM to 0xC0F
	msleep(1);
	ret = stemp_read(stemp_ope,ADCFG);
	stemp_write(stemp_ope, ADCFG, ret | (0xC0F << 8));

	//3.set ADENA.tempEN to 1
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret | 1<<8);

	//4.Configure ADSTB register
	ret = stemp_read(stemp_ope,ADSTB);
	stemp_write(stemp_ope, ADSTB, ret | 4);//delay periods of sampling

	//5.config ADCLK just for step 0

	//6.wait at least 9ms ,configure ADENA.TSENSOR_EN as 1 to enalbe the temperature channel
	msleep(9);
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret | 1<<3);
	msleep(9);

	//7.wait until ADSTATE.ARDY3 is 1 and write 1 to clear the flag
	while(!(ret = stemp_read(stemp_ope,ADSTATE) & (1<<3))){
		printk("wait ARDY3 to 1,may need open clk,ret:%#x\n",ret);
		msleep(100);
	}

	stemp_write(stemp_ope, ADSTATE, ret | 1<<3);

	//8.read temperature data from adata1
	ret = stemp_read(stemp_ope,ADATA1);

	//lookup temperature
	ret = (ret & (0xfff << 16)) >> 16;

	temp_compensation(&ret);
	temperature = lookup_temp(ret);

#ifdef TSENSOR_DBUG
	stemp_dump(stemp_ope);
#endif
	/*****disable chn******/
	//9.configure ADENA.TSENSOR_EN as 0 to disalbe the temperature chan
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret & ~(1<<3));

	//10. set ADENA.tempEN to 0
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret & ~(1<<8));

	//11.set ADCFG.temp_TRIM to 0
	ret = stemp_read(stemp_ope,ADCFG);
	stemp_write(stemp_ope, ADCFG, ret & ~(0xffffffff << 8));

	//12.configure ADENA.PD to 1 power down SAR A/D
	ret = stemp_read(stemp_ope,ADENA);
	stemp_write(stemp_ope, ADENA, ret | 1<<15);
	mutex_unlock(&adcmutex);

	return temperature;
}

static ssize_t temp_read(struct file *file, char __user * buffer, size_t count, loff_t * ppos)
{
	return 0;
}

static ssize_t temp_write(struct file *file, const char __user * buffer, size_t count, loff_t * ppos)
{
	return 0;
}
static void tsensor_clk_on(struct stemp_operation *stemp_ope);
static int temp_open(struct inode *inode, struct file *file)
{
	struct miscdevice *dev = file->private_data;
	struct stemp_operation *stemp_ope = container_of(dev, struct stemp_operation, temp_dev);
	if(!stemp_ope){
		printk("param err,open failed\n");
		return -EINVAL;
	}

	tsensor_clk_on(stemp_ope);

	return 0;
}

static int temp_release(struct inode *inode, struct file *file)
{
	struct miscdevice *dev = file->private_data;
	struct stemp_operation *stemp_ope = container_of(dev, struct stemp_operation, temp_dev);
	if(!stemp_ope){
		printk("param err,open failed\n");
		return -EINVAL;
	}

	return 0;
}

static long temp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int temperature = 0;
	int *usr_temp = (int *)arg;
	struct miscdevice *dev = file->private_data;
	struct stemp_operation *stemp_ope = container_of(dev, struct stemp_operation, temp_dev);
	if(!stemp_ope){
		return -EINVAL;
	}

	switch(cmd){
		case TSENSOR_GET_TEMP:
			mutex_lock(&tmutex);
			temperature = tsensor_get_temp(stemp_ope);
			mutex_unlock(&tmutex);
			if(copy_to_user(usr_temp, &temperature, sizeof(int))){
				return -EINVAL;
			}
			break;
		default:
			printk("[%s][%d] inval cmd\n",__func__,__LINE__);
			break;

	}
	return 0;
}

const struct file_operations temp_fops = {
	.owner = THIS_MODULE,
	.read = temp_read,
	.write = temp_write,
	.open = temp_open,
	.unlocked_ioctl = temp_ioctl,
	.release = temp_release,
};

static void tsensor_clk_on(struct stemp_operation *stemp_ope)
{
	if (stemp_ope->clk == NULL) {
		stemp_ope->clk = clk_get(stemp_ope->dev, "gate_sadc");
		if (IS_ERR(stemp_ope->clk)) {
			dev_err(stemp_ope->dev, "stemp clk get failed!\n");
			return ;
		}
	}
	if (!clk_is_enabled(stemp_ope->clk)) {
		clk_enable(stemp_ope->clk);
	}
}

static void tsensor_clk_off(struct stemp_operation *stemp_ope)
{
}

static ssize_t tsensor_temp_enable_store(struct device *dev,
				   struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int temperature = 0;
	struct stemp_operation *stemp_ope = dev_get_drvdata(dev);

	if (buf == NULL)
		return 0;

	if (strncmp(buf, "1", 1) == 0) {
		tsensor_clk_on(stemp_ope);
		mutex_lock(&tmutex);
		temperature = tsensor_get_temp(stemp_ope);
		mutex_unlock(&tmutex);
		tsensor_clk_off(stemp_ope);
		printk("Temprature:%d.%ld\n",temperature/1000,abs(temperature%1000));
	} else {
		dev_err(stemp_ope->dev, "Invalid option\n");
		return -1;
	}

	return count;
}

#ifdef CONFIG_TS_AUTO_REFI
static ssize_t show_auto_refi_status(struct device *dev,
				   struct device_attribute *attr,
					char *buf)
{
	ssize_t count = 1;

	if (tsensor_timer.data)
	{
		count = sprintf(buf, "enable refi(%d) reg:0x%08x[%x %x %x]\n", trefi, tsreadl(RTR),
			Trefi7800, Trefi3900, Trefi2920);
	}
	else
	{
		count = sprintf(buf, "disable\n");
	}

	return count;
}

static ssize_t set_auto_refi_status(struct device *dev,
				   struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned int on = 1;
	if (strncmp(buf, "disable", 7) == 0)
		on = 0;
	else if (strncmp(buf, "enable", 6) == 0)
		on = 1;
	else
		return count;

	ts_AutoRefi_IsOn(on);
	return count;
}
static DEVICE_ATTR(auto_refi_enable,  S_IRUGO | S_IWUSR, show_auto_refi_status, set_auto_refi_status);
#endif

static DEVICE_ATTR(enable, S_IWUSR, NULL, tsensor_temp_enable_store);

static struct attribute *tsensor_temp_attr[] = {
	&dev_attr_enable.attr,
#ifdef CONFIG_TS_AUTO_REFI
	&dev_attr_auto_refi_enable.attr,
#endif
	NULL
};

static const struct attribute_group tsensor_temp_attr_group = {
	.attrs = tsensor_temp_attr,
};

static int temp_info_show(struct seq_file *m, void *v)
{
	int len = 0;
	int temperature = 0;
	struct stemp_operation *stemp_ope = (struct stemp_operation *)m->private;
	tsensor_clk_on(stemp_ope);
	mutex_lock(&tmutex);
	temperature = tsensor_get_temp(stemp_ope);
	mutex_unlock(&tmutex);
	tsensor_clk_off(stemp_ope);
	len += seq_printf(m ,"Temprature:%d.%ld\n",temperature/1000,abs(temperature%1000));

	return len;
}

static int temp_info_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, temp_info_show, PDE_DATA(inode),1024);
}

static const struct file_operations temp_info_fops ={
	.read = seq_read,//fs/seq_file.c
	.open = temp_info_open,
	.llseek = seq_lseek,//fs/seq_file.c
	.release = single_release,//fs/seq_file.c
};

#ifdef CONFIG_TS_AUTO_REFI
static void ts_CalRefiInit(struct stemp_operation *stemp_ope)
{
	unsigned int rate, p2t;

	tsensor_stemp_ope = stemp_ope;

	rate = clk_get_rate(clk_get(NULL, "mpll")) / 1000000;
	rate = rate * 2 / ((*(volatile unsigned int*)(0xb000002c) & 0xff) + 1);
	p2t = 1000000 / rate;

	trefi = (tsreadl(RTR) >> 16) & 0xff;

	ts_debug("rate:%dMHz;  tck_ps:%dps; trefi:0x%08x\n", rate, p2t, trefi);

	Trefi7800 = 7800000 / p2t / 64;
	Trefi3900 = 3900000 / p2t / 64;
	Trefi2920 = 2920000 / p2t / 64;
}

static void ts_FillRefi(unsigned int t_rfc_nom_x1_x32)
{
	unsigned int ts_rfshtmg;

	ts_rfshtmg = tsreadl(RTR);
	ts_rfshtmg &= ~(0xfff << 16);
	ts_rfshtmg |= t_rfc_nom_x1_x32 << 16;

	tswritel(SRWEN, 0x1);
	tswritel(SRPCEN, 0x0);
	tswritel(RTR, ts_rfshtmg);
	tswritel(SRWEN, 0x0);
	tswritel(SRPCEN, 0x1);

	ts_debug("tsreadl(RTR):0x%08x\n", tsreadl(RTR));
}

static void ts_SetRefi(int temperatureInt)
{
	if((temperatureInt >= -20) && (temperatureInt < 80)) {
		if (trefi != 7800)
		{
			ts_FillRefi(Trefi7800);
			trefi = 7800;
		}
	} else if(((temperatureInt >= 80) && (temperatureInt < 90)) || (temperatureInt < -20)) {
		if (trefi != 3900)
		{
			ts_FillRefi(Trefi3900);
			trefi = 3900;
		}
	} else if(temperatureInt >= 90) {
		if (trefi != 2920)
		{
			ts_FillRefi(Trefi2920);
			trefi = 2920;
		}
	} else {
		if (trefi != 3900)
		{
			ts_FillRefi(Trefi3900);
			trefi = 3900;
		}
	}
	ts_debug("temperature:%d refi:%dns reg:0x%08x[%x %x %x]\n", temperatureInt, trefi, tsreadl(RTR),
	Trefi7800, Trefi3900, Trefi2920);
}

static void work_handler(struct work_struct *work)
{
	int temperature = 0;

	tsensor_clk_on(tsensor_stemp_ope);
	mutex_lock(&tmutex);
	temperature = tsensor_get_temp(tsensor_stemp_ope) / 1000;
	mutex_unlock(&tmutex);
	tsensor_clk_off(tsensor_stemp_ope);
	ts_SetRefi(temperature);
}

static void tsensor_timer_callback(unsigned long is_on)
{
	if (tsensor_timer.data)
	{
		queue_work(wq, &wrk);
		mod_timer(&tsensor_timer, jiffies + msecs_to_jiffies(CONFIG_AUTOREFI_DELAY));
	}
	else
	{
		printk("TSensor auto tsRefi_func is disabled!\n");
	}
}

static void ts_AutoRefi_IsOn(unsigned int is_on) // AutotsRefi_func enable
{
	if (is_on && !tsensor_timer.data)
	{
		printk("TSensor enable auto tsRefi_func\n");
		tsensor_timer.data = 1;
		tsensor_timer_callback(1);
	}
	else if(!is_on && tsensor_timer.data)
	{
		printk("Wait for TSensor auto tsRefi_func to shut down...\n");
		tsensor_timer.data = 0;
	}

}
#endif

int jz_stemp_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct proc_dir_entry *proc;
	struct stemp_operation *stemp_ope = NULL;
#if ADCUSEMFD
	struct jz_adc_tsensor *ptsensor = NULL;
#endif

	stemp_ope = (struct stemp_operation *)kzalloc(sizeof(struct stemp_operation), GFP_KERNEL);
	if (!stemp_ope) {
		dev_err(&pdev->dev, "alloc stemp mem_region failed!\n");
		return -ENOMEM;
	}
	sprintf(stemp_ope->name, "jz-temp");

#if ADCUSEMFD
	stemp_ope->cell = mfd_get_cell(pdev);
	if (!stemp_ope->cell) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get mfd cell for jz_adc_tsnsor!\n");
		goto failed_get_mem;
	}

	ptsensor = (struct jz_adc_tsensor *)stemp_ope->cell->platform_data;
	if(!ptsensor){
		dev_err(&pdev->dev, "should open adc first\n");
		ret = -EINVAL;
		goto failed_get_mem;
	}

	stemp_ope->iomem = ptsensor->iomem;
	if (!stemp_ope->iomem) {
		dev_err(&pdev->dev, "failed to remap regs memory region\n");
		ret = -EINVAL;
		goto failed_get_mem;
	}

	printk("tsensor iomem:%p\n",stemp_ope->iomem);
#else
	stemp_ope->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!stemp_ope->res) {
		dev_err(&pdev->dev, "failed to get dev resources\n");
		ret = -EINVAL;
		goto failed_get_mem;
	}

	stemp_ope->res = devm_request_mem_region(&pdev->dev,stemp_ope->res->start,\
			stemp_ope->res->end - stemp_ope->res->start + 1,
			pdev->name);
	if (!stemp_ope->res) {
		dev_err(&pdev->dev, "failed to request regs memory region");
		ret = -EINVAL;
		goto failed_req_region;
	}
	stemp_ope->iomem = ioremap(stemp_ope->res->start, resource_size(stemp_ope->res));
	if (!stemp_ope->iomem) {
		dev_err(&pdev->dev, "failed to remap regs memory region\n");
		ret = -EINVAL;
		goto failed_iomap;
	}
#endif

	stemp_ope->temp_dev.minor = MISC_DYNAMIC_MINOR;
	stemp_ope->temp_dev.fops = &temp_fops;
	stemp_ope->temp_dev.name = "temp";

	ret = misc_register(&stemp_ope->temp_dev);
	if(ret) {
		dev_err(&pdev->dev,"request misc device failed!\n");
		ret = -EINVAL;
		goto failed_misc;
	}

	// 1. use sysfs to get temperature
	ret = sysfs_create_group(&pdev->dev.kobj, &tsensor_temp_attr_group);
	if (ret < 0){
		ret = -EINVAL;
		goto failed_misc;
	}

	//2. use proc to get temperature
	proc = jz_proc_mkdir("temp");
	if (!proc) {
		proc = NULL;
		printk("create jz_proc_mkdir failed!\n");
	} else {
		stemp_ope->proc = proc;
	}
	proc_create_data("temperature", S_IRUGO, proc, &temp_info_fops, (void *)stemp_ope);

	platform_set_drvdata(pdev,stemp_ope);
	mutex_init(&adcmutex);
#ifdef CONFIG_TS_AUTO_REFI
	ts_CalRefiInit(stemp_ope);
#endif
	return 0;

failed_misc:
	misc_deregister(&stemp_ope->temp_dev);
	iounmap(stemp_ope->iomem);
#if !ADCUSEMFD
failed_iomap:
	devm_release_mem_region(&pdev->dev,stemp_ope->res->start, stemp_ope->res->end - stemp_ope->res->start + 1);
failed_req_region:
#endif
failed_get_mem:
	kfree(stemp_ope);
	return ret;
}

int jz_stemp_remove(struct platform_device *pdev)
{
	struct stemp_operation *stemp_ope = NULL;
	stemp_ope = platform_get_drvdata(pdev);
	if(!stemp_ope){
		printk("param is null ,remove\n");
		return -ENOMEM;
	}
	if (clk_is_enabled(stemp_ope->clk)) {
		clk_disable(stemp_ope->clk);
		clk_put(stemp_ope->clk);
		stemp_ope->clk = NULL;
	}
	sysfs_remove_group(&pdev->dev.kobj, &tsensor_temp_attr_group);
	misc_deregister(&stemp_ope->temp_dev);
#if !ADCUSEMFD
	iounmap(stemp_ope->iomem);
	devm_release_mem_region(&pdev->dev,stemp_ope->res->start, stemp_ope->res->end - stemp_ope->res->start + 1);
#endif
	kfree(stemp_ope);

	printk("remove succeed\n");

	return 0;
}

#if !ADCUSEMFD
/* temp controller*/
static struct resource jz_temp_resources[] = {
	{
		.start  = SADC_IOBASE,
		.end    = SADC_IOBASE + 0x32,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = IRQ_SADC,
		.end    = IRQ_SADC,
		.flags  = IORESOURCE_IRQ,
	},
	{
		.start  = IRQ_SADC_BASE,
		.end    = IRQ_SADC_BASE,
		.flags  = IORESOURCE_IRQ
	},
};

static void platform_device_release(struct device *dev)
{
	return ;
}

struct platform_device jz_temp_device = {
	.name   = "jz-temp",
	.id = -1,
	.num_resources  = ARRAY_SIZE(jz_temp_resources),
	.resource   = jz_temp_resources,
	.dev = {
		.release = platform_device_release,
	}
};
#endif

static struct platform_driver jz_stemp_driver = {
	.probe	= jz_stemp_probe,
	.remove	= jz_stemp_remove,
	.driver	= {
		.name	= "jz-temp",
		.owner	= THIS_MODULE,
	},
};


#ifdef CONFIG_TS_AUTO_REFI
static int __init tsensor_workenue_init(void)
{
	ts_debug("TSensor Work queue module init: %s %d\n", __FUNCTION__, __LINE__);
	wq = create_singlethread_workqueue("tsensor_single_thread");

	INIT_WORK(&wrk, work_handler);

	return 0;
}

static void __exit tsensor_workenue_exit(void)
{
	flush_workqueue(wq);
	destroy_workqueue(wq);
	printk("TSensor Work queue module exit: %s %d\n", __FUNCTION__, __LINE__);
}

static int __init tsensor_timer_init(void)
{
	ts_debug("TSensor Timer module loaded\n");
	setup_timer(&tsensor_timer, tsensor_timer_callback, 1);
	ts_debug("TSensor Setup timer to fire in 30s (%ld)\n", jiffies);
	add_timer(&tsensor_timer);

	printk("TSensor enable auto tsRefi_func\n");
	mod_timer(&tsensor_timer, jiffies + msecs_to_jiffies(CONFIG_AUTOREFI_DELAY));

	return 0;
}

static void tsensor_timer_exit(void)
{
	int retval;
	retval = del_timer(&tsensor_timer);
	if (retval)
		printk("TSensor The timer is still in use...\n");

	printk("TSensor Timer module unloaded\n");
	return;
}
#endif

int __init ingenic_stemp_init(void)
{
	int ret = 0;
#if !ADCUSEMFD
	platform_device_register(&jz_temp_device);
#endif
	ret = platform_driver_register(&jz_stemp_driver);
	if(ret < 0){
		printk("register err\n");
	}
#ifdef CONFIG_TS_AUTO_REFI
	tsensor_timer_init();
	tsensor_workenue_init();
#endif
	return ret;
}
void __exit ingenic_stemp_exit(void)
{
	platform_driver_unregister(&jz_stemp_driver);
#if !ADCUSEMFD
	platform_device_unregister(&jz_temp_device);
#endif
#ifdef CONFIG_TS_AUTO_REFI
	tsensor_workenue_exit();
	tsensor_timer_exit();
#endif
	return ;
}

module_init(ingenic_stemp_init);
module_exit(ingenic_stemp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jwang@ingenic.com");
