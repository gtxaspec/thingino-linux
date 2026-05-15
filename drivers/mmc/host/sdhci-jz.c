#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/pm.h>

#include <mach/sdhci-jz.h>
#include <mach/jzmmc.h>

#include "sdhci.h"
#include "sdhci-jz-regs.h"


#if 0
static void sdhci_jz_fpga_clk(unsigned int clock)
{
#define CPM_MSC_CLK_R CPM_MSC0_CLK_R
//#define CPM_MSC_CLK_R CPM_MSC1_CLK_R
	unsigned int val;

	if(500000 <= clock){
		val = readl((const volatile void*)CPM_MSC_CLK_R);
		val |= MSC_CLK_H_FREQ;
		writel(val, (void*)CPM_MSC_CLK_R);
	} else {
		val = readl((const volatile void*)CPM_MSC_CLK_R);
		val &= ~MSC_CLK_H_FREQ;
		writel(val, (void*)CPM_MSC_CLK_R);
	}
	//printk("\tclk=%d, CPM_MSC0_CLK_R: %08x\n\n", clock, readl((const volatile void*)CPM_MSC0_CLK_R));
}
#endif

static LIST_HEAD(manual_list);
/**
 *	jzmmc_manual_detect - insert or remove card manually
 *	@index: host->index, namely the index of the controller.
 *	@on: 1 means insert card, 0 means remove card.
 *
 *	This functions will be called by manually card-detect driver such as
 *	wifi. To enable this mode you can set value pdata.removal = MANUAL.
 */
int jzmmc_manual_detect(int index, int on)
{
	struct sdhci_jz *jz;
	struct list_head *pos;

	list_for_each(pos, &manual_list) {
		jz = list_entry(pos, struct sdhci_jz, list);
		if (jz->id == index)
			break;
		else
			jz = NULL;
	}

	if (!jz) {
		dev_err(&jz->pdev->dev, "no manual card detect\n");
		return -ENXIO;
	}

	dev_info(&jz->pdev->dev, "card insert manually\n");
	mmc_detect_change(jz->host->mmc, 0);

	return 0;
}
EXPORT_SYMBOL(jzmmc_manual_detect);

/**
 *	jzsdhci_manual_detect - insert or remove card manually
 *	@dev: pdev->dev, device information.
 *	@index: host->index, namely the index of the controller.
 *	@on: 1 means insert card, 0 means remove card.
 *
 *	This functions will be called by manually card-detect driver such as
 *	wifi. To enable this mode you can set value pdata.removal = MANUAL.
 */
int jzsdhci_manual_detect(struct device *dev,int index, int on)
{
	struct sdhci_jz *sdhci_ing;
	struct sdhci_host *host = dev_get_drvdata(dev);
	int old_flags;

    sdhci_ing = sdhci_priv(host);
    if(!sdhci_ing) {
		printk("no manual card detect\n");
		return -1;
	}
	old_flags = host->flags;

	if (on) {
		dev_err(&sdhci_ing->pdev->dev, "card insert manually\n");
		set_bit(INGENIC_MMC_CARD_PRESENT, &sdhci_ing->flags);
#ifdef CLK_CTRL
		ingenic_mmc_clk_onoff(sdhci_ing, 1);
#endif
		host->flags &= ~SDHCI_DEVICE_DEAD;
		host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
		mmc_detect_change(sdhci_ing->host->mmc, 0);
	} else {
		dev_err(&sdhci_ing->pdev->dev, "card remove manually\n");
		clear_bit(INGENIC_MMC_CARD_PRESENT, &sdhci_ing->flags);

		host->flags |= SDHCI_DEVICE_DEAD;
		host->quirks &= ~SDHCI_QUIRK_BROKEN_CARD_DETECTION;
		mmc_detect_change(sdhci_ing->host->mmc, 0);
#ifdef CLK_CTRL
		ingenic_mmc_clk_onoff(sdhci_ing, 0);
#endif
	}

	if(old_flags != host->flags){
		printk("%s %s tasklet finish\n", mmc_hostname(host->mmc), __func__);
		tasklet_schedule(&host->finish_tasklet);
	}

	return 0;
}

/**
 * sdhci_jz_get_max_clk - callback to get maximum clock frequency.
 * @host: The SDHCI host instance.
 *
 * Callback to return the maximum clock rate acheivable by the controller.
*/
static unsigned int sdhci_jz_get_max_clk(struct sdhci_host *host)
{
	struct sdhci_jz *jz = sdhci_priv(host);

	return jz->max_freq[jz->id];
}

static void sdhci_jz_sel_rx_phase(struct sdhci_jz *jz, volatile void* cpm_msc)
{
	unsigned int val;
	val = readl(cpm_msc);
	if(((val >> 17) & 0x7) != (jz->pdata->rx_sel & 0x7))
	{
		//printk("rx_sel is %d\n", jz->pdata->rx_sel);
		val &= ~(0x7 << 17);
		val |= ((jz->pdata->rx_sel) & 0x7) << 17;
		writel(val, cpm_msc);
	}
}
static void sdhci_jz_sel_tx_phase(struct sdhci_jz *jz, volatile void* cpm_msc)
{
	unsigned int val;
	val = readl(cpm_msc);
	if(((val >> 15) & 0x3) != (jz->pdata->tx_sel & 0x3))
	{
		//printk("tx_sel is %d\n", jz->pdata->tx_sel);
		val &= ~(0x3 << 15);
		val |= ((jz->pdata->tx_sel) & 0x3) << 15;
		writel(val, cpm_msc);
	}
}

#define MSC_WAIT_BUSY_TIME 300
static inline void jz_mmc_clk_switch_onoff(struct sdhci_jz *jz, unsigned int on)
{
	volatile void *msc_base_addr;
	unsigned int val;
	unsigned int time = MSC_WAIT_BUSY_TIME;

	if (0 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC0_CLK_R;
	else if (1 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC1_CLK_R;

	val = readl(msc_base_addr);
	if (on)
	{
		val &= ~(MSC_CLK_STOP);
		val |= MSC_CLK_CE;
		writel(val, msc_base_addr);
		while(time--)
		{
			if(!(readl(msc_base_addr) & MSC_CLK_BUSY))
				break;
#ifndef CONFIG_FPGA_TEST
			else if(!time)
				printk("wait msc busy timeout!!! %s +%d %s()\n",
					__FILE__, __LINE__, __func__);
#endif
		}
		val = readl(msc_base_addr);
		val &= ~(MSC_CLK_CE);
		writel(val, msc_base_addr);
	}
	else
	{
		while(time--)
		{
			if(!(readl(msc_base_addr) & MSC_CLK_BUSY))
				break;
#ifndef CONFIG_FPGA_TEST
			else if(!time)
				printk("wait msc busy timeout!!! %s +%d %s()\n",
					__FILE__, __LINE__, __func__);
#endif
		}
		val |= MSC_CLK_STOP;
		val |= MSC_CLK_CE;
		writel(val, msc_base_addr);
	}
}

/**
 * sdhci_jz_set_clock - callback on clock change
 * @host: The SDHCI host being changed
 * @clock: The clock rate being requested.
 *
 * When the card's clock is going to be changed, look at the new frequency
 * and find the best clock source to go with it.
*/
static void sdhci_jz_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct clk *ext;
	unsigned int value = 0;
	unsigned long pll_rate;
#if 1
	struct sdhci_jz *jz = sdhci_priv(host);
	volatile void *msc_base_addr;
	const char *parent_name;
	unsigned int val = 0, cdr = 0, i = 0;
#ifdef CONFIG_SOC_PRJ008
	char *pll_name[4] = {"sclka", "mpll", "ext1", "ext1"};
#else
	char *pll_name[4] = {"sclka", "mpll", "vpll", "ext1"};
#endif

	jz_mmc_clk_switch_onoff(jz, 0);

	if (0 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC0_CLK_R;
	else if (1 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC1_CLK_R;

	val = readl(msc_base_addr);
	val &= ~(MSC_CLK_CS(3) | MSC_CDR);

	if (clock > 400000)
	{
		parent_name = clk_get_name(clk_get_parent(jz->clk));
		for (i = 0; i < 4; i++)
		{
			if (!strcmp(pll_name[i], parent_name))
			{
				pll_rate = clk_get_rate(jz->pll);
				cdr = (pll_rate / clock / 4 - 1) & 0xff;
				cdr = (pll_rate % (clock * 4)) ? (cdr + 1) : cdr;
				val |= MSC_CLK_CS(i) | cdr;
				break;
			}
		}
		writel(val, msc_base_addr);
		clk_set_rate(jz->clk, clock);
	}
	else
	{
		val |= MSC_CLK_CS(3);
		cdr = ((CONFIG_EXTAL_CLOCK * 1000000 / clock) / 4 - 1) & 0xff;
		val |= cdr;
		writel(val, msc_base_addr);
	}
	jz_mmc_clk_switch_onoff(jz, 1);

	/* set msc tx sel */
	sdhci_jz_sel_tx_phase(jz, msc_base_addr);
	/* set msc rx sel */
	sdhci_jz_sel_rx_phase(jz, msc_base_addr);
#else
	sdhci_jz_fpga_clk(clock);
#endif
}

/**
 * sdhci_jz_get_min_clock - callback to get minimal supported clock value
 * @host: The SDHCI host being queried
 *
 * To init mmc host properly a minimal clock value is needed. For high system
 * bus clock's values the standard formula gives values out of allowed range.
 * The clock still can be set to lower values, if clock source other then
 * system bus is selected.
*/
static unsigned int sdhci_jz_get_min_clock(struct sdhci_host *host)
{
	return 300000;
}


static int sdhci_jz_set_uhs_signaling(struct sdhci_host *host, unsigned int uhs)
{
	struct mmc_ios *ios = &host->mmc->ios;
	unsigned int ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;

	/* Select Bus Speed Mode for host */
	if (ios->timing == MMC_TIMING_MMC_HS200)
		ctrl_2 |= SDHCI_EMMC_CTRL_HS_SDR200;
	else if (ios->timing == MMC_TIMING_UHS_SDR12)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
	else if (ios->timing == MMC_TIMING_UHS_SDR25)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
	else if (ios->timing == MMC_TIMING_UHS_SDR50)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
	else if (ios->timing == MMC_TIMING_UHS_SDR104)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
	else if (ios->timing == MMC_TIMING_UHS_DDR50)
		ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
	sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);

	return 0;
}

static void sdhci_jz_set_pwr(struct sdhci_host *host, int enable) {
	struct sdhci_jz *jz = sdhci_priv(host);
	if(enable) {
		if(gpio_is_valid(jz->pdata->ext_pwr_gpio)) {
			if(jz->pdata->ext_pwr_gpio_invert == LOW_ENABLE)
				enable = !enable;
			gpio_direction_output(jz->pdata->ext_pwr_gpio, enable);
		}
		mdelay(50);
		if(jz->host->mmc->index == 0)
		{
#ifdef CONFIG_MMC_SDHCI_MMC0_PB_4BIT
			jzgpio_set_func(GPIO_PORT_B, GPIO_FUNC_0, 0x3f);
#endif
		}
		if(jz->host->mmc->index == 1)
		{
#ifdef CONFIG_MMC_SDHCI_MMC1_PA_4BIT
			jzgpio_set_func(GPIO_PORT_A, GPIO_FUNC_2, (0x7 << 20 | 0x1 << 17 | 0x3 << 14));
#elif defined(CONFIG_MMC_SDHCI_MMC1_PB_4BIT)
			jzgpio_set_func(GPIO_PORT_B, GPIO_FUNC_1, (0x6f << 8));
#elif defined(CONFIG_MMC_SDHCI_MMC1_PC_4BIT)
			jzgpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, (0x3f << 2));
#endif
		}
	} else {
		if(jz->host->mmc->index == 0)
		{
#ifdef CONFIG_MMC_SDHCI_MMC0_PB_4BIT
			jzgpio_set_func(GPIO_PORT_B, GPIO_OUTPUT0, 0x3f);
#endif
		}
		if(jz->host->mmc->index == 1)
		{
#ifdef CONFIG_MMC_SDHCI_MMC1_PA_4BIT
			jzgpio_set_func(GPIO_PORT_A, GPIO_OUTPUT0, (0x7 << 20 | 0x1 << 17 | 0x3 << 14));
#elif defined(CONFIG_MMC_SDHCI_MMC1_PB_4BIT)
			jzgpio_set_func(GPIO_PORT_B, GPIO_OUTPUT0, (0x6f << 8));
#elif defined(CONFIG_MMC_SDHCI_MMC1_PC_4BIT)
			jzgpio_set_func(GPIO_PORT_C, GPIO_OUTPUT0, (0x3f << 2));
#endif
		}
		if(gpio_is_valid(jz->pdata->ext_pwr_gpio)) {
			if(jz->pdata->ext_pwr_gpio_invert == LOW_ENABLE)
				enable = !enable;
			gpio_direction_output(jz->pdata->ext_pwr_gpio, enable);
		}
	}
	return;
}

void sdhci_jz_voltage_switch(struct sdhci_host *host)
{
	struct sdhci_jz *jz = sdhci_priv(host);
	struct device *dev = &jz->pdev->dev;
	if(gpio_is_valid(jz->pdata->voltage_switch_gpio))
	{
		if(devm_gpio_request(dev, jz->pdata->voltage_switch_gpio, "SDHCI VOLTAGE SWITCH")) {
			dev_err(dev, "cannot request gpio for voltage_switch");
			jz->pdata->voltage_switch_gpio = -EBUSY;
			return;
		}
		if (jz->pdata->voltage_switch_gpio_invert == LOW_ENABLE)
			gpio_direction_output(jz->pdata->voltage_switch_gpio, 0);
		else
			gpio_direction_output(jz->pdata->voltage_switch_gpio, 1);
		printk("request gpio for voltage_switch!\n");
	}
	return;
}

static struct sdhci_ops sdhci_jz_ops = {
	.get_max_clock		= sdhci_jz_get_max_clk,
	.set_clock			= sdhci_jz_set_clock,
	.get_min_clock		= sdhci_jz_get_min_clock,
	.set_uhs_signaling  = sdhci_jz_set_uhs_signaling,
	.set_pwr			= sdhci_jz_set_pwr,
	.voltage_switch     = sdhci_jz_voltage_switch,
};

static void sdhci_jz_notify_change(struct platform_device *dev, int state)
{
	struct sdhci_host *host = platform_get_drvdata(dev);
	unsigned long flags;

	if (host) {
		spin_lock_irqsave(&host->lock, flags);
		if (state) {
			dev_dbg(&dev->dev, "card inserted.\n");
			host->flags &= ~SDHCI_DEVICE_DEAD;
			host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
		} else {
			dev_dbg(&dev->dev, "card removed.\n");
			host->flags |= SDHCI_DEVICE_DEAD;
			host->quirks &= ~SDHCI_QUIRK_BROKEN_CARD_DETECTION;
		}
		tasklet_schedule(&host->card_tasklet);
		spin_unlock_irqrestore(&host->lock, flags);
	}
}

static irqreturn_t sdhci_jz_gpio_card_detect_thread(int irq, void *dev_id)
{
	struct sdhci_jz *jz = dev_id;
	int status = gpio_get_value(jz->ext_cd_gpio);
	if (jz->pdata->ext_cd_gpio_invert == LOW_ENABLE)
		status = !status;
	sdhci_jz_notify_change(jz->pdev, status);
	return IRQ_HANDLED;
}

static void sdhci_jz_request_card_pwr_gpio(struct sdhci_jz *jz)
{
	struct jz_sdhci_platdata *pdata = jz->pdata;
	struct device *dev = &jz->pdev->dev;
	if(devm_gpio_request(dev, pdata->ext_pwr_gpio, "SDHCI EXT PWR")) {
		dev_err(dev, "cannot request gpio for pwr control");
		pdata->ext_pwr_gpio = -EBUSY;
	}
};

static void sdhci_jz_setup_card_detect_gpio(struct sdhci_jz *jz)
{
	struct jz_sdhci_platdata *pdata = jz->pdata;
	struct device *dev = &jz->pdev->dev;

    if (devm_gpio_request(dev, pdata->ext_cd_gpio, "SDHCI EXT CD") == 0) {
		jz->ext_cd_gpio = pdata->ext_cd_gpio;
		jz->ext_cd_irq = gpio_to_irq(pdata->ext_cd_gpio);
		if (jz->ext_cd_irq &&
		    request_threaded_irq(jz->ext_cd_irq, NULL,
					 sdhci_jz_gpio_card_detect_thread,
					 IRQF_TRIGGER_RISING |
					 IRQF_TRIGGER_FALLING |
					 IRQF_ONESHOT,
					 dev_name(dev), jz) == 0) {
			int status = gpio_get_value(jz->ext_cd_gpio);
			if (pdata->ext_cd_gpio_invert == LOW_ENABLE)
				status = !status;
			sdhci_jz_notify_change(jz->pdev, status);
		} else {
			dev_warn(dev, "cannot request irq for card detect\n");
			jz->ext_cd_irq = 0;
		}
	} else {
		dev_err(dev, "cannot request gpio for card detect\n");
	}
}

static ssize_t sdhci_jz_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sdhci_host *host = dev_get_drvdata(dev);

	sdhci_dumpregs(host);

	return 0;
}

static ssize_t jz_sdhci_rx_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	volatile void *msc_base_addr;
	unsigned int val = 0;

	if (0 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC0_CLK_R;
	else if (1 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC1_CLK_R;

	val = readl(msc_base_addr);
	val = (val >> 17) & 0x7;
	printk("rx_sel is %d°\n", (val * 45));
	return 0;
}

static int jz_sdhci_rx_sel(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	volatile void *msc_base_addr;
	unsigned int rx_sel = 0;
	unsigned int val = 0;
	int ret = 0;
	if (0 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC0_CLK_R;
	else if (1 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC1_CLK_R;

	val = readl(msc_base_addr);
	val &= ~(0x7 << 17);
	ret = kstrtoint(buf, 10, &rx_sel);
	if (ret < 0) {
		pr_err("Conversion failed, Error code %d\n", ret);

		if (ret == -EINVAL) {
			pr_err("Invalid parameter.\n");
		} else if (ret == -ERANGE) {
			pr_err("Value out of range.\n");
		}
		printk("err! rx_sel only set 0, 45, 90, 135, 180, 225, 270, 315\n");
		return count;
	}
	switch (rx_sel)
	{
		case 0:
			val |= ((TUNING_RX_SEL_0) & 0x7) << 17;
			break;
		case 45:
			val |= ((TUNING_RX_SEL_45) & 0x7) << 17;
			break;
		case 90:
			val |= ((TUNING_RX_SEL_90) & 0x7) << 17;
			break;
		case 135:
			val |= ((TUNING_RX_SEL_135) & 0x7) << 17;
			break;
		case 180:
			val |= ((TUNING_RX_SEL_180) & 0x7) << 17;
			break;
		case 225:
			val |= ((TUNING_RX_SEL_225) & 0x7) << 17;
			break;
		case 270:
			val |= ((TUNING_RX_SEL_270) & 0x7) << 17;
			break;
		case 315:
			val |= ((TUNING_RX_SEL_315) & 0x7) << 17;
			break;
		default:
			printk("err! rx_sel only set 0, 45, 90, 135, 180, 225, 270, 315\n");
			return count;
	}
	writel(val, msc_base_addr);
	return count;
}

static ssize_t jz_sdhci_tx_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	volatile void *msc_base_addr;
	unsigned int val = 0;

	if (0 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC0_CLK_R;
	else if (1 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC1_CLK_R;

	val = readl(msc_base_addr);
	val = (val >> 15) & 0x3;
	printk("tx_sel is %d°\n", (val * 45 + 90));
	return 0;
}

static int jz_sdhci_tx_sel(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	volatile void *msc_base_addr;
	unsigned int tx_sel = 0;
	unsigned int val = 0;
	int ret = 0;
	if (0 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC0_CLK_R;
	else if (1 == jz->id)
		msc_base_addr = (volatile void *)CPM_MSC1_CLK_R;

	val = readl(msc_base_addr);
	val &= ~(0x3 << 15);
	ret = kstrtoint(buf, 10, &tx_sel);
	if (ret < 0) {
		pr_err("Conversion failed, Error code %d\n", ret);

		if (ret == -EINVAL) {
			pr_err("Invalid parameter.\n");
		} else if (ret == -ERANGE) {
			pr_err("Value out of range.\n");
		}
		printk("err! tx_sel only set 90, 135, 180, 270\n");
		return count;
	}
	switch (tx_sel)
	{
		case 90:
			val |= ((TUNING_TX_SEL_90) & 0x3) << 15;
			break;
		case 135:
			val |= ((TUNING_TX_SEL_135) & 0x3) << 15;
			break;
		case 180:
			val |= ((TUNING_TX_SEL_180) & 0x3) << 15;
			break;
		case 270:
			val |= ((TUNING_TX_SEL_270) & 0x3) << 15;
			break;
		default:
			printk("err! tx_sel only set 90, 135, 180, 270\n");
			return count;
	}
	writel(val, msc_base_addr);
	return count;
}

static struct device_attribute sdhci_jz_sysfs_attrs[] = {
	__ATTR(sdhci_regs, S_IRUGO, sdhci_jz_regs_show, NULL),
	__ATTR(sdhci_rx_sel, S_IRUGO | S_IWUSR, jz_sdhci_rx_sel_show, jz_sdhci_rx_sel),
	__ATTR(sdhci_tx_sel, S_IRUGO | S_IWUSR, jz_sdhci_tx_sel_show, jz_sdhci_tx_sel),
};

/*-------------------------Begin Sysfs interface-----------------------------*/
/****************Position: /sys/devices/platform/jz-sdhci.0/ ***********/
static ssize_t jzmmc_present_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	ssize_t count = 0;

	if (host->flags & 0x8)
		count = sprintf(buf, "N\n");
	else
		count = sprintf(buf, "Y\n");

	return count;
}

static ssize_t jzmmc_present_store(struct device *dev,
				   struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct sdhci_host *host = dev_get_drvdata(dev);

	if ((buf == NULL)) {
		dev_err(dev, "can't set present\n");
		return count;
	}

	if (strncmp(buf, "INSERT", 6) == 0) {
		dev_info(dev, "card insert via sysfs\n");
        jzsdhci_manual_detect(dev, 0, 1);

	} else if (strncmp(buf, "REMOVE", 6) == 0) {
		dev_info(dev, "card remove via sysfs\n");
        jzsdhci_manual_detect(dev, 0, 0);

	} else {
		dev_err(dev, "set present error, "
			"the argument can't be recognised\n");
	}

	return count;
}

static DEVICE_ATTR(present, S_IWUSR | S_IRUSR, jzmmc_present_show, jzmmc_present_store);

static struct attribute *jzmmc_attributes[] = {
	&dev_attr_present.attr,
	NULL
};

static const struct attribute_group jzmmc_attr_group = {
	.attrs = jzmmc_attributes,
};
/*-------------------------End Sysfs interface-----------------------------*/
static int sdhci_jz_probe(struct platform_device *pdev)
{
	struct jz_sdhci_platdata *pdata;
	struct device *dev = &pdev->dev;
	struct sdhci_host *host;
	struct sdhci_jz *jz;
	struct resource *res;
	char clk_gate_name[16];
	char clkname[16];
	int ret, irq, i;

	if (!pdev->dev.platform_data && !pdev->dev.of_node) {
		dev_err(dev, "no device data specified\n");
		return -ENOENT;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "no irq specified\n");
		return irq;
	}

	host = sdhci_alloc_host(dev, sizeof(struct sdhci_jz));
	if (IS_ERR(host)) {
		dev_err(dev, "sdhci_alloc_host() failed\n");
		return PTR_ERR(host);
	}
	jz = sdhci_priv(host);

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ret = -ENOMEM;
		goto err_pdata_io_clk;
	}

	memcpy(pdata, pdev->dev.platform_data, sizeof(*pdata));
	jz->ext_cd_gpio = -1; /* invalid gpio number */

	jz->dev = &pdev->dev;
	jz->id = pdev->id;
	jz->host = host;
	jz->pdev = pdev;
	jz->pdata = pdata;

	switch(jz->id) {
	case 0:
#ifdef CONFIG_MMC0_MAX_FREQ
		jz->max_freq[jz->id] =  CONFIG_MMC0_MAX_FREQ;
#endif
		break;
	case 1:
#ifdef CONFIG_MMC1_MAX_FREQ
		jz->max_freq[jz->id] =  CONFIG_MMC1_MAX_FREQ;
#endif
		break;
	}

	platform_set_drvdata(pdev, host);

	sprintf(clk_gate_name, "gate_msc%d", pdev->id);
	jz->clk_gate = clk_get(&pdev->dev, clk_gate_name);
	if (IS_ERR(jz->clk_gate)) {
		return PTR_ERR(jz->clk_gate);
	}

	sprintf(clkname, "div_msc%d", pdev->id);
	jz->clk = clk_get(&pdev->dev, clkname);
	if (IS_ERR(jz->clk)) {
		return PTR_ERR(jz->clk);
	}

	jz->pll = clk_get(&pdev->dev, clk_get_name(clk_get_parent(jz->clk)));
	if (IS_ERR(jz->pll))
		return PTR_ERR(jz->pll);

	if (!(jz->pll) ||
		(CONFIG_EXTAL_CLOCK * 1000000 == clk_get_rate(jz->pll)))
	{
		dev_info(&pdev->dev, "The MSC_CLK parent is not obtained, and MPLL is used.\n");
		clk_put(jz->pll);
		jz->pll = clk_get(&pdev->dev, "mpll");
		if (IS_ERR(jz->pll))
			return PTR_ERR(jz->pll);

		ret = clk_set_parent(jz->clk, jz->pll);
		if (ret < 0){
			dev_err(&pdev->dev, "%s set parent failed\n", clkname);
		}
	}

	/**
	 * When using EXCLK on MSC, EXCLK_EN needs to be set to 1.
	 * Only when EXCLK_EN is set to 1, the configuration of
	 * PLL to EXCLK can take effect.
	 * It is recommended to keep EXCLK_EN constantly set to 1.
	 * Keep EXCLK_EN ON
	 */
	ret = readl((volatile void *)CPM_MSC0_CLK_R);
	if (!(ret & (0x1 << 21)))
		writel(ret | (0x1 << 21), (volatile void *)CPM_MSC0_CLK_R);

	/* enable the local io clock and keep it running for the moment. */
	clk_prepare_enable(jz->clk_gate);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->ioaddr = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(host->ioaddr)) {
		ret = PTR_ERR(host->ioaddr);
		goto err_req_regs;
	}

	/* sdio for WIFI init*/
	if (pdata->private_init) {
		ret = pdata->private_init();
		if (ret < 0)
			goto err_req_regs;
	}

	/* Ensure we have minimal gpio selected CMD/CLK/Detect */
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev, pdata->max_width);

	host->hw_name = "ingenic-msc";
	host->ops = &sdhci_jz_ops;
	host->quirks = 0;
	host->irq = irq;

	/* Software redefinition caps */
	host->quirks |= SDHCI_QUIRK_MISSING_CAPS;
	host->caps  = CAPABILITIES1_SW;
	host->caps1 = CAPABILITIES2_SW;

	/* max clk */
	host->quirks |= SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN;

	/* not check wp */
	host->quirks |= SDHCI_QUIRK_INVERTED_WRITE_PROTECT;

	/* Setup quirks for the controller */
	host->quirks |= SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC;
	host->quirks |= SDHCI_QUIRK_NO_HISPD_BIT;

	/* This host supports the Auto CMD12 */
	if(jz->pdata->enable_autocmd12)
		host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;

	/* PIO transfer mode */
	if(jz->pdata->pio_mode){
		host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
		host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
	}
	/* TODO:SoCs need BROKEN_ADMA_ZEROLEN_DESC */
/*	host->quirks |= SDHCI_QUIRK_BROKEN_ADMA_ZEROLEN_DESC;*/

	if (pdata->cd_type == JZ_SDHCI_CD_NONE ||
	    pdata->cd_type == JZ_SDHCI_CD_PERMANENT)
		host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;

	if (pdata->cd_type == JZ_SDHCI_CD_NON_INITIAL) {
			host->flags |= SDHCI_DEVICE_DEAD;
			host->quirks &= ~SDHCI_QUIRK_BROKEN_CARD_DETECTION;
	}

	if (pdata->cd_type == JZ_SDHCI_CD_PERMANENT)
		host->mmc->caps = MMC_CAP_NONREMOVABLE;

	switch (pdata->max_width) {
	case 8:
		host->mmc->caps |= MMC_CAP_8_BIT_DATA;
	case 4:
		host->mmc->caps |= MMC_CAP_4_BIT_DATA;
		break;
	}

	/* force 1 bit debug test*/
#ifdef CONFIG_FORCE_1BIT_TEST
	host->mmc->caps &= ~MMC_CAP_4_BIT_DATA;
	host->quirks |= SDHCI_QUIRK_FORCE_1_BIT_DATA;
#endif
	if (pdata->pm_caps)
		host->mmc->pm_caps |= pdata->pm_caps;

	/*
	 * Filter out MMC_PM_WAKE_SDIO_IRQ to prevent MMC from being
	 * a wakeup source. Only preserve power management flags that
	 * don't enable wakeup functionality.
	 */
	if (pdata->pm_flags)
		host->mmc->pm_flags |= (pdata->pm_flags & ~MMC_PM_WAKE_SDIO_IRQ);

	/*
	 * Ensure this device is NOT initialized as a wakeup source.
	 * This prevents the MMC controller from waking the system from suspend.
	 */
	device_init_wakeup(&pdev->dev, 0);

	host->quirks |= (SDHCI_QUIRK_32BIT_DMA_ADDR |
			 SDHCI_QUIRK_32BIT_DMA_SIZE);

	/* It supports additional host capabilities if needed */
	if (pdata->host_caps)
		host->mmc->caps |= pdata->host_caps;

	if (pdata->host_caps2)
		host->mmc->caps2 |= pdata->host_caps2;

	if (!((host->mmc->caps & MMC_CAP_8_BIT_DATA) ||
		(host->mmc->caps & MMC_CAP_4_BIT_DATA)))
			host->quirks |= SDHCI_QUIRK_FORCE_1_BIT_DATA;

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_enable(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_suspend_ignore_children(&pdev->dev, 1);
#endif

	if(gpio_is_valid(pdata->ext_pwr_gpio))
		sdhci_jz_request_card_pwr_gpio(jz);

	list_add(&(jz->list), &manual_list);
	ret = sdhci_add_host(host);
	if (ret) {
		dev_err(dev, "sdhci_add_host() failed\n");
		pm_runtime_forbid(&pdev->dev);
		pm_runtime_get_noresume(&pdev->dev);
		goto err_req_regs;
	}

	if (pdata->cd_type == JZ_SDHCI_CD_GPIO &&
	    gpio_is_valid(pdata->ext_cd_gpio))
		sdhci_jz_setup_card_detect_gpio(jz);

#ifdef CONFIG_PM_RUNTIME
	if (pdata->cd_type != JZ_SDHCI_CD_INTERNAL)
		clk_disable_unprepare(jz->clk_gate);
#endif

	for (i = 0; i < ARRAY_SIZE(sdhci_jz_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &sdhci_jz_sysfs_attrs[i]);
		if (ret)
			dev_warn(&pdev->dev,"attribute %s create failed %x",
					attr_name(sdhci_jz_sysfs_attrs[i]), ret);
	}
	ret = sysfs_create_group(&pdev->dev.kobj, &jzmmc_attr_group);
	if (ret < 0)
		dev_err(dev, "sysfs_create_group failed\n");

	return 0;

 err_req_regs:
#ifndef CONFIG_PM_RUNTIME
	clk_disable_unprepare(jz->clk_gate);
#endif
	clk_disable_unprepare(jz->clk);
	clk_put(jz->clk_gate);
	clk_put(jz->clk);
	clk_put(jz->pll);

 err_pdata_io_clk:
	sdhci_free_host(host);

	return ret;
}

static int sdhci_jz_remove(struct platform_device *pdev)
{
	struct sdhci_host *host =  platform_get_drvdata(pdev);
	struct sdhci_jz *jz = sdhci_priv(host);
#ifdef CONFIG_PM_RUNTIME
	struct jz_sdhci_platdata *pdata = jz->pdata;
#endif
	int i;

	if (jz->ext_cd_irq)
		free_irq(jz->ext_cd_irq, jz);

#ifdef CONFIG_PM_RUNTIME
	if (pdata->cd_type != JZ_SDHCI_CD_INTERNAL)
		clk_prepare_enable(jz->clk_gate);
#endif
	sdhci_remove_host(host, 1);

	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	clk_disable_unprepare(jz->clk_gate);
	clk_disable_unprepare(jz->clk);
	clk_put(jz->clk_gate);
	clk_put(jz->clk);
	clk_put(jz->pll);

	sdhci_free_host(host);
	platform_set_drvdata(pdev, NULL);

	sysfs_remove_group(&pdev->dev.kobj, &jzmmc_attr_group);
	for (i = 0; i < ARRAY_SIZE(sdhci_jz_sysfs_attrs); i++) {
		device_remove_file(&pdev->dev, &sdhci_jz_sysfs_attrs[i]);
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
/**
 * sdhci_jz_suspend - Suspend callback for Ingenic MMC driver
 * @dev: device to suspend
 *
 * This function handles the suspend operation for the Ingenic MMC controller.
 * It ensures that the MMC is NOT set as a wakeup source by explicitly
 * disabling wakeup capability.
 *
 * Returns: 0 on success, negative error code on failure
 */
static int sdhci_jz_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	int ret = 0;

	if (!host)
		return 0;

	/*
	 * Explicitly disable wakeup to prevent MMC from acting as a wakeup source.
	 * This ensures the system can enter deep sleep without being woken by MMC events.
	 */
	device_set_wakeup_enable(dev, false);

	if (host->mmc->card && host->mmc->card->type != MMC_TYPE_SDIO) {
		ret = sdhci_suspend_host(host);
		if (ret)
			return ret;

		/* Disable clock during suspend to save power */
#ifdef CONFIG_PM_RUNTIME
		if (jz->pdata->cd_type != JZ_SDHCI_CD_INTERNAL)
			clk_disable_unprepare(jz->clk_gate);
#endif
	}

	return ret;
}

/**
 * sdhci_jz_resume - Resume callback for Ingenic MMC driver
 * @dev: device to resume
 *
 * This function handles the resume operation for the Ingenic MMC controller.
 * It re-enables the clock and restores the controller state.
 *
 * Returns: 0 on success, negative error code on failure
 */
static int sdhci_jz_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	int ret = 0;

	if (!host)
		return 0;

	if (host->mmc->card && host->mmc->card->type != MMC_TYPE_SDIO) {
		/* Re-enable clock after resume */
#ifdef CONFIG_PM_RUNTIME
		if (jz->pdata->cd_type != JZ_SDHCI_CD_INTERNAL)
			clk_prepare_enable(jz->clk_gate);
#endif

		ret = sdhci_resume_host(host);
	}

	return ret;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int sdhci_jz_runtime_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	if (jz->pdata->cd_type != JZ_SDHCI_CD_INTERNAL)
		clk_disable_unprepare(jz->clk_gate);
	return 0;
}


static int sdhci_jz_runtime_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_jz *jz = sdhci_priv(host);
	if (jz->pdata->cd_type != JZ_SDHCI_CD_INTERNAL)
		clk_prepare_enable(jz->clk_gate);
	return 0;
}
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops sdhci_jz_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(sdhci_jz_suspend, sdhci_jz_resume)
	SET_RUNTIME_PM_OPS(sdhci_jz_runtime_suspend, sdhci_jz_runtime_resume,
			   NULL)
};

#define SDHCI_JZ_PMOPS (&sdhci_jz_pmops)

#else
#define SDHCI_JZ_PMOPS NULL
#endif


static struct platform_device_id sdhci_jz_driver_ids[] = {
	{
		.name		= "jz-sdhci",
		.driver_data	= (kernel_ulong_t)NULL,
	},
	{ }
};
MODULE_DEVICE_TABLE(platform, sdhci_jz_driver_ids);

#ifdef CONFIG_OF
static const struct of_device_id sdhci_jz_dt_match[] = {
	{},
};
MODULE_DEVICE_TABLE(of, sdhci_jz_dt_match);
#endif

static struct platform_driver sdhci_jz_driver = {
	.probe		= sdhci_jz_probe,
	.remove		= sdhci_jz_remove,
	.id_table	= sdhci_jz_driver_ids,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "jz-sdhci",
		.pm	= SDHCI_JZ_PMOPS,
		.suppress_bind_attrs = false,  /* Allow bind/unbind via sysfs */
	},
};

static int __init sdhci_jz_init(void)
{
	return platform_driver_register(&sdhci_jz_driver);
}

static void __exit sdhci_jz_exit(void)
{
	platform_driver_unregister(&sdhci_jz_driver);
}

module_init(sdhci_jz_init);
module_exit(sdhci_jz_exit);

MODULE_DESCRIPTION("Ingenic SDHCI (MSC) driver");
MODULE_AUTHOR("Large Dipper <wangquan.shao@ingenic.cn>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20160808");
