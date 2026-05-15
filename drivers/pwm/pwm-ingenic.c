/* PWM driver of Ingenic's SoC T41
 *
 * Copyright (C) 2022 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pwm.h>
#include <soc/gpio.h>

// #define DEBUG

#define PWMCCFG0  0x000
#define PWMCCFG1  0x004
#define PWMENS    0x010
#define PWMENC    0x014
#define PWMEN     0x018
#define PWMUPDATE 0x020
#define PWMBUSYR  0x024
#define PWMLVL    0x030
#define PWMWCFG   0x0B0
#define PWMDES    0x100
#define PWMDEC    0x104
#define PWMDE     0x108
#define PWMDCR0   0x110
#define PWMDCR1   0x114
#define PWMDTRIG  0x120
#define PWMDFER   0x124
#define PWMDFSM   0x128
#define PWMDSR    0x130
#define PWMDSCR   0x134
#define PWMDINTC  0x138
#define PWMnDAR   0x140
#define PWMnDTLR  0x190
#define PWMOEN    0x300

#define CLK_1 0
#define CLK_2 1
#define CLK_4 2
#define CLK_8 3
#define CLK_16 4
#define CLK_32 5
#define CLK_64 6
#define CLK_128 7

#define PWM_CHN_NUM 4
#define PWM_CLK_RATE 50000000

struct ingenic_pwm_gpio
{
	enum gpio_port group;
	unsigned int pins;
	enum gpio_function func;
};

struct ingenic_pwm_gpio pwm_pins[PWM_CHN_NUM] = {
#if defined(CONFIG_PWM_CHANNEL0_PA17)
	[0] = {.group = GPIO_PORT_A, .pins = 1 << 17, .func = GPIO_FUNC_3},
#elif defined(CONFIG_PWM_CHANNEL0_PB17)
	[0] = {.group = GPIO_PORT_B, .pins = 1 << 17, .func = GPIO_FUNC_0},
#elif defined(CONFIG_PWM_CHANNEL0_PB30)
#if defined(CONFIG_SOC_PRJ007)
	[0] = {.group = GPIO_PORT_B, .pins = 1 << 30, .func = GPIO_FUNC_2},
#elif defined(CONFIG_SOC_PRJ008)
	[0] = {.group = GPIO_PORT_B, .pins = 1 << 30, .func = GPIO_FUNC_0},
#endif
#elif defined(CONFIG_PWM_CHANNEL0_PC08)
	[0] = {.group = GPIO_PORT_C, .pins = 1 << 8, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL0_PC25)
	[0] = {.group = GPIO_PORT_C, .pins = 1 << 25, .func = GPIO_FUNC_0},
#endif
#if defined(CONFIG_PWM_CHANNEL1_PA19)
	[1] = {.group = GPIO_PORT_A, .pins = 1 << 19, .func = GPIO_FUNC_3},
#elif defined(CONFIG_PWM_CHANNEL1_PB18)
	[1] = {.group = GPIO_PORT_B, .pins = 1 << 18, .func = GPIO_FUNC_0},
#elif defined(CONFIG_PWM_CHANNEL1_PB26)
	[1] = {.group = GPIO_PORT_B, .pins = 1 << 26, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL1_PC29)
	[1] = {.group = GPIO_PORT_C, .pins = 1 << 29, .func = GPIO_FUNC_1},
#endif
#if defined(CONFIG_PWM_CHANNEL2_PA15)
	[2] = {.group = GPIO_PORT_A, .pins = 1 << 15, .func = GPIO_FUNC_3},
#elif defined(CONFIG_PWM_CHANNEL2_PB11)
	[2] = {.group = GPIO_PORT_B, .pins = 1 << 11, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL2_PB27)
	[2] = {.group = GPIO_PORT_B, .pins = 1 << 27, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL2_PC09)
	[2] = {.group = GPIO_PORT_C, .pins = 1 << 9, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL3_PA14)
	[3] = {.group = GPIO_PORT_A, .pins = 1 << 14, .func = GPIO_FUNC_3},
#elif defined(CONFIG_PWM_CHANNEL3_PB13)
	[3] = {.group = GPIO_PORT_B, .pins = 1 << 13, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL3_PB28)
	[3] = {.group = GPIO_PORT_B, .pins = 1 << 28, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL3_PB31)
	[3] = {.group = GPIO_PORT_B, .pins = 1 << 31, .func = GPIO_FUNC_0},
#endif
};

struct ingenic_pwm_chip
{
	void __iomem *io_base;
	struct pwm_chip chip;
	struct ingenic_pwm_gpio *pwm_pins;
	unsigned int clk_ch[PWM_CHN_NUM]; // Divided frequency
	unsigned char clk_div[PWM_CHN_NUM];
	unsigned int period_ns_max[PWM_CHN_NUM];
	unsigned int period_ns_min[PWM_CHN_NUM];
	unsigned int chn_en_mask;
	unsigned int pwm_en;
// char gpio_func[PWM_CHN_NUM];
	struct clk *clk_pwm;
	struct clk *clk_gate;
	spinlock_t lock;
};

static int ingenic_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int pwm_reg;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);

	if (!(ingenic_pwm->chn_en_mask & (1 << pwm->hwpwm)))
	{
		dev_warn(chip->dev, "requested PWM channel %d not support\n", pwm->hwpwm);
		return -EINVAL;
	}
	pwm_set_chip_data(pwm, &ingenic_pwm->pwm_pins[pwm->hwpwm]);

	spin_lock(&ingenic_pwm->lock);
	pwm_reg = readl(ingenic_pwm->io_base + PWMCCFG0);
	pwm_reg = pwm_reg & (~(0x0f << 4 * pwm->hwpwm));
	pwm_reg = pwm_reg | (ingenic_pwm->clk_div[pwm->hwpwm] << 4 * pwm->hwpwm);
	writel(pwm_reg, ingenic_pwm->io_base + PWMCCFG0);
	spin_unlock(&ingenic_pwm->lock);
	writel(0, ingenic_pwm->io_base + PWMDCR0);
	writel(0, ingenic_pwm->io_base + PWMDINTC);

	pwm->duty_cycle = 0;
	pwm->period = ingenic_pwm->period_ns_max[pwm->hwpwm];

	return 0;
}

static void ingenic_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	pwm_set_chip_data(pwm, NULL);
}

static int __pwm_config(struct ingenic_pwm_chip *ingenic_pwm, struct pwm_device *pwm,
				  uint duty_ns, uint period_ns)
{
	unsigned int high, low, period, pwm_reg;
	if (period_ns > ingenic_pwm->period_ns_max[pwm->hwpwm]
		|| period_ns < ingenic_pwm->period_ns_min[pwm->hwpwm]) {
		printk("%s:Invalid argument\n",__func__);
		return -EINVAL;
	}
	// period = (ingenic_pwm->clk_ch[n] * period_ns + 500000000) / 1000000000;
	period = div_u64((u64)ingenic_pwm->clk_ch[pwm->hwpwm] * period_ns + 500000000, 1000000000);
	if(pwm->polarity == PWM_POLARITY_NORMAL)
	{
		high = div_u64((u64)ingenic_pwm->clk_ch[pwm->hwpwm] * duty_ns + 500000000, 1000000000);
		low = period - high;
	}
	else
	{
		low = div_u64((u64)ingenic_pwm->clk_ch[pwm->hwpwm] * duty_ns + 500000000, 1000000000);
		high = period - low;
	}
	pr_debug("high=%u,low=%u\n", high, low);

	spin_lock(&ingenic_pwm->lock);
	if (high == 0)
	{
		pwm_reg = readl(ingenic_pwm->io_base + PWMLVL);
		pwm_reg &= ~(1 << pwm->hwpwm);
		writel(pwm_reg, ingenic_pwm->io_base + PWMLVL);
	}
	else if (low == 0)
	{
		pwm_reg = readl(ingenic_pwm->io_base + PWMLVL);
		pwm_reg |= (1 << pwm->hwpwm);
		writel(pwm_reg, ingenic_pwm->io_base + PWMLVL);
	}
	spin_unlock(&ingenic_pwm->lock);

#if 0
	printk("ingenic_pwm->pwm_en : 0x%x\n",ingenic_pwm->pwm_en);
	printk("#pwm->hwpwm : 0x%x\n",pwm->hwpwm);
	printk("PWMCCFG0      0x%x \n ", readl(ingenic_pwm->io_base + PWMCCFG0	));
        printk("PWMCCFG1      0x%x \n ", readl(ingenic_pwm->io_base + PWMCCFG1	));
        printk("PWMENS        0x%x \n ", readl(ingenic_pwm->io_base + PWMENS	));
        printk("PWMENC        0x%x \n ", readl(ingenic_pwm->io_base + PWMENC	));
        printk("PWMEN         0x%x \n ", readl(ingenic_pwm->io_base + PWMEN	));
        printk("PWMUPDATE     0x%x \n ", readl(ingenic_pwm->io_base + PWMUPDATE	));
        printk("PWMBUSYR      0x%x \n ", readl(ingenic_pwm->io_base + PWMBUSYR	));
        printk("PWMLVL        0x%x \n ", readl(ingenic_pwm->io_base + PWMLVL	));
        printk("PWMWCFG       0x%x \n ", readl(ingenic_pwm->io_base + PWMWCFG	));
        printk("PWMDES        0x%x \n ", readl(ingenic_pwm->io_base + PWMDES	));
        printk("PWMDEC        0x%x \n ", readl(ingenic_pwm->io_base + PWMDEC	));
        printk("PWMDE         0x%x \n ", readl(ingenic_pwm->io_base + PWMDE	));
        printk("PWMDCR0       0x%x \n ", readl(ingenic_pwm->io_base + PWMDCR0	));
        printk("PWMDCR1       0x%x \n ", readl(ingenic_pwm->io_base + PWMDCR1	));
        printk("PWMDTRIG      0x%x \n ", readl(ingenic_pwm->io_base + PWMDTRIG	));
        printk("PWMDFER       0x%x \n ", readl(ingenic_pwm->io_base + PWMDFER	));
        printk("PWMDFSM       0x%x \n ", readl(ingenic_pwm->io_base + PWMDFSM	));
        printk("PWMDSR        0x%x \n ", readl(ingenic_pwm->io_base + PWMDSR	));
        printk("PWMDSCR       0x%x \n ", readl(ingenic_pwm->io_base + PWMDSCR	));
        printk("PWMDINTC      0x%x \n ", readl(ingenic_pwm->io_base + PWMDINTC	));
        printk("PWMnDAR       0x%x \n ", readl(ingenic_pwm->io_base + PWMnDAR	));
        printk("PWMnDTLR      0x%x \n ", readl(ingenic_pwm->io_base + PWMnDTLR	));
        printk("PWMOEN        0x%x \n ", readl(ingenic_pwm->io_base + PWMOEN	));
#endif

	if (ingenic_pwm->pwm_en & (1 << pwm->hwpwm)) {
		while (readl(ingenic_pwm->io_base + PWMBUSYR) & (1 << pwm->hwpwm));
	}

	writel((high << 16) | low, ingenic_pwm->io_base + PWMWCFG + 4 * pwm->hwpwm);
	if(ingenic_pwm->pwm_en & (1 << pwm->hwpwm))
		writel(1 << pwm->hwpwm, ingenic_pwm->io_base + PWMUPDATE);
	return 0;
}

static int ingenic_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int pwm_reg;
	struct ingenic_pwm_gpio *pwm_pin;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);
	pr_debug("[%d]%s\n", __LINE__, __func__);
	pr_debug("period=%u,duty=%u,polarity=%d\n", pwm->period, pwm->duty_cycle, (int)pwm->polarity);

	__pwm_config(ingenic_pwm, pwm, pwm->duty_cycle, pwm->period);

	writel(1 << pwm->hwpwm, ingenic_pwm->io_base + PWMENS);
	spin_lock(&ingenic_pwm->lock);
	pwm_reg = readl(ingenic_pwm->io_base + PWMOEN);
	pwm_reg |= (1 << pwm->hwpwm);
	writel(pwm_reg, ingenic_pwm->io_base + PWMOEN);

	ingenic_pwm->pwm_en |= 1 << pwm->hwpwm;
	spin_unlock(&ingenic_pwm->lock);

	pwm_pin = pwm->chip_data;
	jzgpio_set_func(pwm_pin->group, pwm_pin->func, pwm_pin->pins);
	// ingenic_pwm->gpio_func[pwm->hwpwm] = 1;
	return 0;
}

static void ingenic_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int pwm_reg;
	struct ingenic_pwm_gpio *pwm_pin;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);

	pr_debug("[%d]%s\n", __LINE__, __func__);
	pr_debug("period=%u,duty=%u,polarity=%d\n", pwm->period, pwm->duty_cycle, (int)pwm->polarity);

	writel(1 << pwm->hwpwm, ingenic_pwm->io_base + PWMENC);
	spin_lock(&ingenic_pwm->lock);
	pwm_reg = readl(ingenic_pwm->io_base + PWMOEN);
	pwm_reg &= ~(1 << pwm->hwpwm);
	writel(pwm_reg, ingenic_pwm->io_base + PWMOEN);

	ingenic_pwm->pwm_en &= ~(1 << pwm->hwpwm);
	spin_unlock(&ingenic_pwm->lock);

	pwm_pin = pwm->chip_data;
	jzgpio_set_func(pwm_pin->group, pwm->polarity ? GPIO_OUTPUT1:GPIO_OUTPUT0, pwm_pin->pins);
	// ingenic_pwm->gpio_func[pwm->hwpwm] = 0;
}

static int ingenic_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,int duty_ns, int period_ns)
{
	int ret = 0;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);

	pr_debug("[%d]%s:duty=%d,period=%d\n", __LINE__, __func__, duty_ns, period_ns);
	pr_debug("period=%u,duty=%u,polarity=%d\n", pwm->period, pwm->duty_cycle, (int)pwm->polarity);

	ret = __pwm_config(ingenic_pwm, pwm, duty_ns, period_ns);
	return ret;
}

static int ingenic_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
	pr_debug("[%d]%s:polarity=%d\n", __LINE__, __func__, polarity);

	return 0;
}

static const struct pwm_ops ingenic_pwm_ops = {
	.request = ingenic_pwm_request,
	.free = ingenic_pwm_free,
	.enable = ingenic_pwm_enable,
	.disable = ingenic_pwm_disable,
	.config = ingenic_pwm_config,
	.set_polarity = ingenic_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static int ingenic_pwm_probe(struct platform_device *pdev)
{
	struct ingenic_pwm_chip *ingenic_pwm;
	struct resource *base;
	int ret = 0, n;
	u32 pwm_clk;
	ingenic_pwm = devm_kzalloc(&pdev->dev, sizeof(struct ingenic_pwm_chip), GFP_KERNEL);
	if (ingenic_pwm == NULL)
		return -ENOMEM;

	base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ingenic_pwm->io_base = devm_ioremap_resource(&pdev->dev, base);
	if (IS_ERR(ingenic_pwm->io_base))
		return PTR_ERR(ingenic_pwm->io_base);

	ingenic_pwm->chip.dev = &pdev->dev;
	ingenic_pwm->chip.ops = &ingenic_pwm_ops;
	ingenic_pwm->chip.npwm = PWM_CHN_NUM;
	ingenic_pwm->chip.can_sleep = true;
	ingenic_pwm->chip.base = -1;
	ingenic_pwm->pwm_pins = pwm_pins;

	ingenic_pwm->clk_div[0] = CLK_1;
	ingenic_pwm->clk_div[1] = CLK_1;
	ingenic_pwm->clk_div[2] = CLK_1;
	ingenic_pwm->clk_div[3] = CLK_1;
/*
#ifdef CONFIG_FPGA_TEST
	for (n = 0; n < PWM_CHN_NUM; n++) {
		ingenic_pwm->clk_ch[n] = 25000000 / (1 << ingenic_pwm->clk_div[n]);
	}
#else*/
	ingenic_pwm->clk_gate = devm_clk_get(&pdev->dev, "gate_pwm");
	if (IS_ERR(ingenic_pwm->clk_gate)){
		dev_err(&pdev->dev, "get pwm clk gate failed %ld\n", PTR_ERR(ingenic_pwm->clk_gate));
		return PTR_ERR(ingenic_pwm->clk_gate);
	}

	ingenic_pwm->clk_pwm = devm_clk_get(&pdev->dev, "div_pwm");
	if (IS_ERR(ingenic_pwm->clk_pwm)){
		dev_err(&pdev->dev, "get pwm clk failed %ld\n", PTR_ERR(ingenic_pwm->clk_pwm));
		return PTR_ERR(ingenic_pwm->clk_pwm);
	}

	if (ingenic_pwm->clk_gate)
	{
#if defined(CONFIG_KERNEL_4_4_94)
		ret = clk_prepare_enable(ingenic_pwm->clk_gate);
#else
		ret = clk_enable(ingenic_pwm->clk_gate);
#endif
		if (ret){
			dev_err(&pdev->dev, "enable pwm clock gate failed!\n");
		}
	}

	if (ingenic_pwm->clk_pwm){
		ret = clk_set_rate(ingenic_pwm->clk_pwm, PWM_CLK_RATE);
		if (ret){
			dev_err(&pdev->dev, "set pwm clock rate failed!\n");
		}
#if defined(CONFIG_KERNEL_4_4_94)
		ret = clk_prepare_enable(ingenic_pwm->clk_gate);
#else
		ret = clk_enable(ingenic_pwm->clk_gate);
#endif
		if (ret){
			dev_err(&pdev->dev, "enable pwm clock failed!\n");
		}
	}

	pwm_clk = clk_get_rate(ingenic_pwm->clk_pwm);
	dev_info(&pdev->dev, "PWM clk:%u", pwm_clk);
//#endif
	spin_lock_init(&ingenic_pwm->lock);

	ingenic_pwm->chn_en_mask = 0;
#ifdef CONFIG_PWM_CHANNEL0_ENABLE
	ingenic_pwm->chn_en_mask |= 1;
#endif
#ifdef CONFIG_PWM_CHANNEL1_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 1;
#endif
#ifdef CONFIG_PWM_CHANNEL2_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 2;
#endif
#ifdef CONFIG_PWM_CHANNEL3_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 3;
#endif
	ingenic_pwm->pwm_en = 0;

	for (n = 0; n < PWM_CHN_NUM; n++)
	{
		ingenic_pwm->clk_ch[n] = pwm_clk / (1 << ingenic_pwm->clk_div[n]);
		ingenic_pwm->period_ns_max[n] = 1000000000 / ingenic_pwm->clk_ch[n] * 0xffff;
		ingenic_pwm->period_ns_min[n] = 1000000000 * 2 / ingenic_pwm->clk_ch[n] + 1;
		if (ingenic_pwm->chn_en_mask & (1 << n))
			printk("PWM%d period_ns:max=%u,min=%u\n", n, ingenic_pwm->period_ns_max[n], ingenic_pwm->period_ns_min[n]);
	}

	ret = pwmchip_add(&ingenic_pwm->chip);
	if (ret < 0)
	{
		dev_err(&pdev->dev, "failed to add PWM chip %d\n", ret);
		return ret;
	}
	platform_set_drvdata(pdev, ingenic_pwm);

	return 0;
}

static int ingenic_pwm_remove(struct platform_device *pdev)
{
	struct ingenic_pwm_chip *ingenic_pwm = platform_get_drvdata(pdev);
	return pwmchip_remove(&ingenic_pwm->chip);
}

#ifdef CONFIG_PM_SLEEP
static int ingenic_pwm_suspend(struct device *dev)
{
	int i;
	struct pwm_device *pwm;
	struct ingenic_pwm_chip *ingenic_pwm = dev_get_drvdata(dev);
	for (i = 0; i < PWM_CHN_NUM; ++i)
	{
		pwm = &ingenic_pwm->chip.pwms[i];
		if (test_bit(PWMF_REQUESTED, &pwm->flags))
		{
			if (test_bit(PWMF_ENABLED, &pwm->flags))
			{
				ingenic_pwm->pwm_en &= ~(1 << pwm->hwpwm);
			}
		}
	}

	clk_disable_unprepare(ingenic_pwm->clk_gate);
	clk_disable_unprepare(ingenic_pwm->clk_pwm);

	return 0;
}

static int ingenic_pwm_resume(struct device *dev)
{
	int i,ret;
	unsigned int pwm_reg;
	struct pwm_device *pwm;
	struct ingenic_pwm_chip *ingenic_pwm = dev_get_drvdata(dev);

#if defined(CONFIG_KERNEL_4_4_94)
		ret = clk_prepare_enable(ingenic_pwm->clk_gate);
#else
		ret = clk_enable(ingenic_pwm->clk_gate);
#endif
	if (ret){
		dev_err(dev, "enable pwm clock gate failed!\n");
	}
	ret = clk_set_rate(ingenic_pwm->clk_pwm, PWM_CLK_RATE);
	if (ret){
		dev_err(dev, "set pwm clock rate failed!\n");
	}
#if defined(CONFIG_KERNEL_4_4_94)
		ret = clk_prepare_enable(ingenic_pwm->clk_gate);
#else
		ret = clk_enable(ingenic_pwm->clk_gate);
#endif
	if (ret){
		dev_err(dev, "enable pwm clock failed!\n");
	}
	pr_debug("PWM clk:%lu\n", clk_get_rate(ingenic_pwm->clk_pwm));

	for (i = 0; i < PWM_CHN_NUM; ++i)
	{
		pwm = &ingenic_pwm->chip.pwms[i];
		if (test_bit(PWMF_REQUESTED, &pwm->flags))
		{
			pwm_reg = readl(ingenic_pwm->io_base + PWMCCFG0);
			pwm_reg = pwm_reg & (~(0x0f << 4 * pwm->hwpwm));
			pwm_reg = pwm_reg | (ingenic_pwm->clk_div[pwm->hwpwm] << 4 * pwm->hwpwm);
			writel(pwm_reg, ingenic_pwm->io_base + PWMCCFG0);
			writel(0, ingenic_pwm->io_base + PWMDCR0);
			writel(0, ingenic_pwm->io_base + PWMDINTC);
			if (test_bit(PWMF_ENABLED, &pwm->flags))
			{
				ingenic_pwm_enable(&ingenic_pwm->chip, pwm);
			}
		}
	}
	return 0;
}
static SIMPLE_DEV_PM_OPS(ingenic_pwm_pm_ops, ingenic_pwm_suspend, ingenic_pwm_resume);
#endif

#ifdef CONFIG_OF
static const struct of_device_id ingenic_pwm_matches[] = {
	{.compatible = "ingenic,PRJ007-pwm", .data = NULL},
	{.compatible = "ingenic,PRJ008-pwm", .data = NULL},
	{.compatible = "ingenic,t32-pwm", .data = NULL},
	{.compatible = "x2000-pwm", .data = NULL},
	{},
};
MODULE_DEVICE_TABLE(of, ingenic_pwm_matches);
#else
static struct platform_device_id pla_pwm_ids[] = {
#if defined(CONFIG_SOC_PRJ008)  
	{.name = "ingenic,PRJ008-pwm", .driver_data = 0},
#elif defined(CONFIG_SOC_PRJ007)
	{.name = "ingenic,PRJ007-pwm", .driver_data = 0},
#endif
	{},
};
MODULE_DEVICE_TABLE(platform, pla_pwm_ids);
#endif

static struct platform_driver ingenic_pwm_driver = {
	.driver = {
#if defined(CONFIG_SOC_PRJ008)  
		.name = "ingenic,PRJ008-pwm",
#elif defined(CONFIG_SOC_PRJ007)
		.name = "ingenic,PRJ007-pwm",
#endif
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ingenic_pwm_matches),
#endif
#ifdef CONFIG_PM_SLEEP
		.pm = &ingenic_pwm_pm_ops,
#endif
	},
#ifndef CONFIG_OF
	.id_table = pla_pwm_ids,
#endif
	.probe = ingenic_pwm_probe,
	.remove = ingenic_pwm_remove,
};

static int __init ingenic_pwm_init(void)
{
	return platform_driver_register(&ingenic_pwm_driver);
}

static void __exit ingenic_pwm_exit(void)
{
	platform_driver_unregister(&ingenic_pwm_driver);
}

module_init(ingenic_pwm_init);
module_exit(ingenic_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ingenic SoC PWM driver");
MODULE_ALIAS("platform:ingenic-pwm");
