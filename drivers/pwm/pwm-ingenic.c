/*
 * Copyright (c) 2007 Ben Dooks
 * Copyright (c) 2008 Simtec Electronics
 *     Ben Dooks <ben@simtec.co.uk>, <ben-linux@fluff.org>
 * Copyright (c) 2013 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * PWM driver for Samsung SoCs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <soc/gpio.h>
#include <linux/slab.h>
#ifdef CONFIG_SOC_T40
#include <linux/mfd/ingenic-tcu.h>
#else
#include <linux/mfd/jz_tcu.h>
#endif

#define PWM_CHN_NUM	NR_TCU_CHNS

#define PWM_CLK_RATE 24000000UL
#define TCU_CLK_DIV TCU_DIV_1

#if defined(CONFIG_SOC_T21)
struct jz_pwm_gpio pwm_pins[PWM_CHN_NUM] = {};
#elif defined(CONFIG_SOC_T23)
struct jz_pwm_gpio pwm_pins[PWM_CHN_NUM] = {};
#elif defined(CONFIG_SOC_T31)
struct jz_pwm_gpio pwm_pins[PWM_CHN_NUM] = {};
#elif defined(CONFIG_SOC_T40)
struct jz_pwm_gpio {
	enum gpio_port port;
	unsigned int pins;
	enum gpio_function func;
};
struct jz_pwm_gpio pwm_pins[PWM_CHN_NUM] = {
#if defined(CONFIG_PWM_CHANNEL0_PC02)
	[0] = {.port = GPIO_PORT_C, .pins = 1 << 2, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL0_PC27)
	[0] = {.port = GPIO_PORT_C, .pins = 1 << 27, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL0_PD26)
	[0] = {.port = GPIO_PORT_D, .pins = 1 << 26, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL1_PC03)
	[1] = {.port = GPIO_PORT_C, .pins = 1 << 3, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL1_PC28)
	[1] = {.port = GPIO_PORT_C, .pins = 1 << 28, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL1_PD27)
	[1] = {.port = GPIO_PORT_D, .pins = 1 << 27, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL2_PB30)
	[2] = {.port = GPIO_PORT_B, .pins = 1 << 30, .func = GPIO_FUNC_0},
#elif defined(CONFIG_PWM_CHANNEL2_PC04)
	[2] = {.port = GPIO_PORT_C, .pins = 1 << 4, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL3_PB31)
	[3] = {.port = GPIO_PORT_B, .pins = 1 << 31, .func = GPIO_FUNC_0},
#elif defined(CONFIG_PWM_CHANNEL3_PC05)
	[3] = {.port = GPIO_PORT_C, .pins = 1 << 5, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL4_PB25)
	[4] = {.port = GPIO_PORT_B, .pins = 1 << 25, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL4_PC06)
	[4] = {.port = GPIO_PORT_C, .pins = 1 << 6, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL5_PB26)
	[5] = {.port = GPIO_PORT_B, .pins = 1 << 26, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL5_PC07)
	[5] = {.port = GPIO_PORT_C, .pins = 1 << 7, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL6_PB06)
	[6] = {.port = GPIO_PORT_B, .pins = 1 << 6, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL6_PD22)
	[6] = {.port = GPIO_PORT_D, .pins = 1 << 22, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL7_PB07)
	[7] = {.port = GPIO_PORT_B, .pins = 1 << 7, .func = GPIO_FUNC_1},
#elif defined(CONFIG_PWM_CHANNEL7_PD23)
	[7] = {.port = GPIO_PORT_D, .pins = 1 << 23, .func = GPIO_FUNC_2},
#endif
};
#endif

struct ingenic_pwm_chip
{
	void __iomem *io_base;
	struct pwm_chip chip;
	struct jz_pwm_gpio *pwm_pins;
	unsigned int clk100k;
	unsigned int period_ns_max;
	unsigned int period_ns_min;
	unsigned int chn_en_mask;
	unsigned int pwm_en;
	char gpio_func[PWM_CHN_NUM];
	spinlock_t lock;
};

static int jz_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct tcu_config config;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);

	if (!(ingenic_pwm->chn_en_mask & (1 << pwm->hwpwm))){
		dev_warn(chip->dev, "Request PWM channel%d not support\n", pwm->hwpwm);
		return -EINVAL;
	}
	if (tcu_chn_request(pwm->hwpwm) < 0){
		dev_warn(chip->dev, "PWM channel%d is busy\n", pwm->hwpwm);
		return -EINVAL;
	}

	tcu_open_clock(pwm->hwpwm);
	config.chn = pwm->hwpwm;
	config.division = TCU_CLK_DIV;
	config.mode = TCU_PWM_MODE;
	config.clk_src=TCU_CLKSRC_EXT;
	tcu_set_config(&config);

	pwm_set_chip_data(pwm, &ingenic_pwm->pwm_pins[pwm->hwpwm]);
	pwm->duty_cycle = 0;
	pwm->period = ingenic_pwm->period_ns_max;
	return 0;
}

static void jz_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	tcu_stop_count(pwm->hwpwm);
	tcu_close_clock(pwm->hwpwm);
	tcu_chn_free(pwm->hwpwm);
	pwm_set_chip_data(pwm, NULL);
}

static int jz_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
						 int duty_ns, int period_ns)
{
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);
	unsigned int tdhr, tdfr;
	struct jz_pwm_gpio *pwm_pin;
	if (period_ns > ingenic_pwm->period_ns_max || period_ns < ingenic_pwm->period_ns_min){
		printk("%s:Invalid argument\n", __func__);
		return -EINVAL;
	}
	// printk("duty=%d,period=%d\n", duty_ns, period_ns);

	tdfr = (ingenic_pwm->clk100k * period_ns + 5000) / 10000;
	tdfr < 2 ? tdfr = 1 : --tdfr;
	tdhr = (ingenic_pwm->clk100k * duty_ns + 5000) / 10000;
	tdhr ? --tdhr : tdhr;
	// printk("tdfr=%u,tdhr=%u\n", tdfr, tdhr);
	tcu_set_tdfr(pwm->hwpwm, tdfr);
	tcu_set_tdhr(pwm->hwpwm, tdhr);

	pwm_pin = pwm->chip_data;
	if (ingenic_pwm->pwm_en & (1 << pwm->hwpwm)){
		if ((duty_ns == 0) && ingenic_pwm->gpio_func[pwm->hwpwm]){ // 0%
		// if (duty_ns < ingenic_pwm->period_ns_min){ //0%
			jzgpio_set_func(pwm_pin->port, pwm->polarity ? GPIO_OUTPUT1 : GPIO_OUTPUT0, pwm_pin->pins);
			ingenic_pwm->gpio_func[pwm->hwpwm] = 0;
		}
#if 0
		else if ((duty_ns == period_ns) && ingenic_pwm->gpio_func[pwm->hwpwm]){ //100%
			jzgpio_set_func(pwm_pin->port, pwm->polarity ? GPIO_OUTPUT0 : GPIO_OUTPUT1, pwm_pin->pins);
			ingenic_pwm->gpio_func[pwm->hwpwm] = 0;
		}
#endif
		else if (ingenic_pwm->gpio_func[pwm->hwpwm] == 0){
			jzgpio_set_func(pwm_pin->port, pwm_pin->func, pwm_pin->pins);
			ingenic_pwm->gpio_func[pwm->hwpwm] = 1;
		}
	}
	return 0;
}

static int jz_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_gpio *pwm_pin;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);

	tcu_start_count(pwm->hwpwm);
	spin_lock(&ingenic_pwm->lock);
	ingenic_pwm->pwm_en |= 1 << pwm->hwpwm;
	spin_unlock(&ingenic_pwm->lock);

	pwm_pin = pwm->chip_data;
	jzgpio_set_func(pwm_pin->port, pwm_pin->func, pwm_pin->pins);
	ingenic_pwm->gpio_func[pwm->hwpwm] = 1;
	jz_pwm_config(chip, pwm, pwm->duty_cycle, pwm->period);

	return 0;
}

static void jz_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_gpio *pwm_pin;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);

	tcu_stop_count(pwm->hwpwm);
	spin_lock(&ingenic_pwm->lock);
	ingenic_pwm->pwm_en &= ~(1 << pwm->hwpwm);
	spin_unlock(&ingenic_pwm->lock);

	pwm_pin = pwm->chip_data;
	jzgpio_set_func(pwm_pin->port, pwm->polarity ? GPIO_OUTPUT1 : GPIO_OUTPUT0, pwm_pin->pins);
	ingenic_pwm->gpio_func[pwm->hwpwm] = 0;
}

int jz_pwm_set_polarity(struct pwm_chip *chip,struct pwm_device *pwm, enum pwm_polarity polarity)
{
	tcu_set_initl(pwm->hwpwm, !polarity);
	return 0;
}

static const struct pwm_ops jz_pwm_ops = {
	.request = jz_pwm_request,
	.free = jz_pwm_free,
	.config = jz_pwm_config,
	.set_polarity = jz_pwm_set_polarity,
	.enable = jz_pwm_enable,
	.disable = jz_pwm_disable,
	.owner = THIS_MODULE,
};

static int jz_pwm_probe(struct platform_device *pdev)
{
	struct ingenic_pwm_chip *ingenic_pwm;
	int ret = 0;
	ingenic_pwm = devm_kzalloc(&pdev->dev, sizeof(struct ingenic_pwm_chip), GFP_KERNEL);
	if (ingenic_pwm == NULL)
		return -ENOMEM;

	ingenic_pwm->io_base = (void __iomem *)0xb0002000;
	//ingenic_pwm->io_base =devm_ioremap_resource(&pdev->dev, base);
	//if (IS_ERR(ingenic_pwm->io_base))
	//	return PTR_ERR(ingenic_pwm->io_base);

	ingenic_pwm->chip.dev = &pdev->dev;
	ingenic_pwm->chip.ops = &jz_pwm_ops;
	ingenic_pwm->chip.npwm = PWM_CHN_NUM;
	ingenic_pwm->chip.can_sleep = true;
	ingenic_pwm->chip.base = -1;

	ingenic_pwm->clk100k = PWM_CLK_RATE / (1 << (2*TCU_CLK_DIV)) / 100000; // unit:100KHz

	spin_lock_init(&ingenic_pwm->lock);
	ingenic_pwm->pwm_pins = pwm_pins;
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
#ifdef CONFIG_PWM_CHANNEL4_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 4;
#endif
#ifdef CONFIG_PWM_CHANNEL5_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 5;
#endif
#ifdef CONFIG_PWM_CHANNEL6_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 6;
#endif
#ifdef CONFIG_PWM_CHANNEL7_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 7;
#endif
	ingenic_pwm->pwm_en = 0;

	ingenic_pwm->period_ns_max = 10000 * (0xffff+1) / ingenic_pwm->clk100k;
	ingenic_pwm->period_ns_min = 10000 * (0+1) / ingenic_pwm->clk100k + 1;
	dev_info(&pdev->dev, "period_ns:max=%u,min=%u\n", ingenic_pwm->period_ns_max, ingenic_pwm->period_ns_min);

	ret = pwmchip_add(&ingenic_pwm->chip);
	if (ret < 0){
		dev_err(&pdev->dev, "failed to add PWM chip %d\n", ret);
		return ret;
	}
	platform_set_drvdata(pdev, ingenic_pwm);

	return 0;
}

static int jz_pwm_remove(struct platform_device *pdev)
{
	struct ingenic_pwm_chip *ingenic_pwm = platform_get_drvdata(pdev);
	return pwmchip_remove(&ingenic_pwm->chip);
}

#ifdef CONFIG_OF
static const struct of_device_id jz_pwm_matches[] = {
	{.compatible = "ingenic,pwm", .data = NULL},
	{},
};
MODULE_DEVICE_TABLE(of, jz_pwm_matches);
#else
static struct platform_device_id pla_pwm_ids[] = {
	{.name = "jz-pwm", .driver_data = 0},
	{},
};
MODULE_DEVICE_TABLE(platform, pla_pwm_ids);
#endif

static struct platform_driver jz_pwm_driver = {
	.driver = {
		.name = "jz-pwm",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(jz_pwm_matches),
#endif
	},
#ifndef CONFIG_OF
	.id_table = pla_pwm_ids,
#endif
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};

static int __init pwm_init(void)
{
	platform_driver_register(&jz_pwm_driver);
	return 0;
}

static void __exit pwm_exit(void)
{
	platform_driver_unregister(&jz_pwm_driver);
}

module_init(pwm_init);
module_exit(pwm_exit);

MODULE_ALIAS("platform:jz-pwm");
MODULE_LICENSE("GPL");
