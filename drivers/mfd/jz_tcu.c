/*
 * jz_tcu.c - JZ Soc TCU MFD driver.
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 * Written by Zoro <yakun.li@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/syscore_ops.h>
#include <irq.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>

#include <asm/div64.h>

#include <soc/base.h>
#include <soc/extal.h>
#include <soc/gpio.h>

#include <soc/irq.h>

#include <linux/mfd/core.h>
#include <linux/mfd/jz_tcu.h>

struct jz_tcu *jztcu = NULL;
static struct jz_tcu_chn g_tcu_chn[NR_TCU_CHNS] = {{0}};

int tcu_chn_request(unsigned char chn)
{
	int ret = chn;
	if (chn >= TCU_CHN_NUM){
		printk("TCU channel error :%d\n", chn);
		ret = -EINVAL;
	}
	else if (jztcu->chn_mask & (1 << chn)){
		printk("TCU channel%d is busy\n", chn);
		ret = -EBUSY;
	}
	else{
		spin_lock(&jztcu->lock);
		jztcu->chn_mask |= (1 << chn);
		spin_unlock(&jztcu->lock);
	}
	return ret;
}

int tcu_chn_free(unsigned char chn)
{
	int ret = chn;
	if (chn >= TCU_CHN_NUM){
		printk("TCU channel error :%d\n", chn);
		ret = -EINVAL;
	}
	else{
		spin_lock(&jztcu->lock);
		jztcu->chn_mask &= ~(1 << chn);
		spin_unlock(&jztcu->lock);
	}
	return ret;
}

void tcu_open_clock(unsigned char chn)
{
	if (chn < TCU_CHN_NUM || chn == 16 || chn == 15){
		writel(1 << chn, jztcu->iomem + TCU_TSCLR);
	}
	else{
		printk("TCU channel error :%d\n", chn);
	}
}

void tcu_close_clock(unsigned char chn)
{
	if (chn < TCU_CHN_NUM || chn == 16 || chn == 15){
		writel(1 << chn, jztcu->iomem + TCU_TSSTR);
	}
	else{
		printk("TCU channel error :%d\n", chn);
	}
}

int tcu_set_config(struct tcu_config *config)
{
	unsigned int reg;
	if (config->chn >= TCU_CHN_NUM){
		printk("TCU channel error :%d\n", config->chn);
		return -EINVAL;
	}

	reg = (config->division << 3) | config->clk_src;

	if (config->mode == TCU_BYPASS_MODE){
		reg |= 1 << 11; //BYPASS
		writel((1 << (config->chn+16)) | (1 << config->chn), jztcu->iomem + TCU_TMSTR); //mask irq
	}
	else if (config->mode == TCU_PWM_MODE){
		reg |= 1 << 7; // PWM_EN
		reg |= 1 << 8; // INITL/polarity
		writel((1 << (config->chn+16)) | (1 << config->chn), jztcu->iomem + TCU_TMSTR); //mask irq
	}
	else if (config->mode == TCU_TIMER_MODE){
		writel(1 << (config->chn+16), jztcu->iomem + TCU_TMSTR); //mask half irq
		writel(1 << config->chn, jztcu->iomem + TCU_TMCLR);		 //unmask full irq
	}
	writel(reg, jztcu->iomem + TCU_TCSR0 + 0x10 * config->chn);

	return 0;
}

void tcu_set_initl(unsigned char chn, unsigned char initl)
{
	unsigned int reg;
	if (chn >= TCU_CHN_NUM){
		printk("TCU channel error :%d\n", chn);
		return;
	}
	reg = readl(jztcu->iomem + TCU_TCSR0 + 0x10 * chn);
	if (initl){
		reg |= 1 << 8; // INITL/polarity
	}
	else{
		reg &= ~(1 << 8);
	}
	writel(reg, jztcu->iomem + TCU_TCSR0 + 0x10 * chn);
}

void tcu_start_count(unsigned char chn)
{
	if (chn < TCU_CHN_NUM || chn == 15){
		writel(1 << chn, jztcu->iomem + TCU_TCSTR);
	}
	else{
		printk("TCU channel error :%d\n", chn);
	}
}

void tcu_stop_count(unsigned char chn)
{
	if (chn < TCU_CHN_NUM || chn == 15){
		writel(1 << chn, jztcu->iomem + TCU_TCCLR);
	}
	else{
		printk("TCU channel error :%d\n", chn);
	}
}

void tcu_set_count(unsigned char chn, unsigned int value)
{
	if (chn >= TCU_CHN_NUM){
		printk("TCU channel error :%d\n", chn);
		return;
	}
	writel(value, jztcu->iomem + TCU_TCNT0 + chn * 0x10);
}
void tcu_set_tdfr(unsigned char chn, unsigned int value)
{
	if (chn >= TCU_CHN_NUM){
		printk("TCU channel error :%d\n", chn);
		return;
	}
	writel(value, jztcu->iomem + TCU_TDFR0 + chn * 0x10);
}
void tcu_set_tdhr(unsigned char chn, unsigned int value)
{
	if (chn >= TCU_CHN_NUM){
		printk("TCU channel error :%d\n", chn);
		return;
	}
	writel(value, jztcu->iomem + TCU_TDHR0 + chn * 0x10);
}

inline unsigned int tcu_get_count(unsigned char chn)
{
	return readl(jztcu->iomem + TCU_TCNT0 + chn * 0x10);
}

static inline void jz_tcu_mask_full_irq(struct jz_tcu_chn *tcu_chn)
{
	tcu_writel(tcu_chn->tcu, TMSR, 1 << tcu_chn->index);
}

static inline void jz_tcu_unmask_full_irq(struct jz_tcu_chn *tcu_chn)
{
	tcu_writel(tcu_chn->tcu, TMCR, 1 << tcu_chn->index);
}

static inline void jz_tcu_clear_full_irq(struct jz_tcu_chn *tcu_chn)
{
	tcu_writel(tcu_chn->tcu, TFCR, 1 << tcu_chn->index);
}

static inline void jz_tcu_mask_half_irq(struct jz_tcu_chn *tcu_chn)
{
	tcu_writel(tcu_chn->tcu, TMSR, 1 << (tcu_chn->index + 16));
}

static inline void jz_tcu_unmask_half_irq(struct jz_tcu_chn *tcu_chn)
{
	tcu_writel(tcu_chn->tcu, TMCR, 1 << (tcu_chn->index + 16));
}

static inline void jz_tcu_clear_half_irq(struct jz_tcu_chn *tcu_chn)
{
	tcu_writel(tcu_chn->tcu, TFCR, 1 << (tcu_chn->index + 16));
}

static inline void jz_tcu_set_prescale(struct jz_tcu_chn *tcu_chn, enum tcu_prescale prescale)
{
	u32 tcsr = tcu_chn_readl(tcu_chn, CHN_TCSR) & ~(0x7 << 3);
	tcu_chn_writel(tcu_chn, CHN_TCSR, tcsr | (prescale << 3));
}

static inline void jz_tcu_set_pwm_output_init_level(struct jz_tcu_chn *tcu_chn, int level)
{
	if (level) {
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) | (TCSR_PWM_HIGH));
	}
	else {
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) & ~(TCSR_PWM_HIGH));
	}
}

static inline void jz_tcu_set_clksrc(struct jz_tcu_chn *tcu_chn, enum tcu_clksrc src)
{
	u32 tcsr = tcu_chn_readl(tcu_chn, CHN_TCSR) & ~0x7;
	tcu_chn_writel(tcu_chn, CHN_TCSR, tcsr | src);
}

void jz_tcu_config_chn(struct jz_tcu_chn *tcu_chn)
{
	/*update irq chip data*/
	irq_set_chip_data(tcu_chn->tcu->irq_base + tcu_chn->index, tcu_chn);

	/* Clear IRQ flag */
	jz_tcu_clear_full_irq(tcu_chn);
	jz_tcu_clear_half_irq(tcu_chn);

	/* Config IRQ */
	switch (tcu_chn->irq_type) {
	case NULL_IRQ_MODE :
		jz_tcu_mask_full_irq(tcu_chn);
		jz_tcu_mask_half_irq(tcu_chn);
		break;
	case FULL_IRQ_MODE :
		jz_tcu_unmask_full_irq(tcu_chn);
		jz_tcu_mask_half_irq(tcu_chn);
		break;
	case HALF_IRQ_MODE :
		jz_tcu_mask_full_irq(tcu_chn);
		jz_tcu_unmask_half_irq(tcu_chn);
		break;
	case FULL_HALF_IRQ_MODE :
		jz_tcu_unmask_full_irq(tcu_chn);
		jz_tcu_unmask_half_irq(tcu_chn);
		break;
	default:
		break;
	}

	/* init level */
	if (tcu_chn->init_level)
		jz_tcu_set_pwm_output_init_level(tcu_chn, 1);
	else
		jz_tcu_set_pwm_output_init_level(tcu_chn, 0);
	/* TCU mode */
	if (tcu_chn->index == 1 || tcu_chn->index == 2)
	{
		tcu_chn->tcu_mode = TCU_MODE_2;
	}
	else
	{
		tcu_chn->tcu_mode = TCU_MODE_1;
		/* shutdown mode */
		if (tcu_chn->shutdown_mode)
		{
			tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) | (TCSR_PWM_SD));
		}
		else
		{
			tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) & ~(TCSR_PWM_SD));
		}
	}

	/* clk source */
	jz_tcu_set_clksrc(tcu_chn, tcu_chn->clk_src);

	/* prescale */
	jz_tcu_set_prescale(tcu_chn, tcu_chn->prescale);

	/* pwm_out */
	if (tcu_chn->is_pwm)
	{
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) | (TCSR_PWM_EN));
	}
	else
	{
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) & ~(TCSR_PWM_EN));
	}
	/* pwm_bapass_mode */
	if (tcu_chn->pwm_bapass_mode)
	{
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) | (TCSR_PWM_BYPASS));
	}
	else
	{
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) & ~(TCSR_PWM_BYPASS));
	}
	/* pwm_in */
	if (tcu_chn->pwm_in)
	{
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) | (TCSR_PWM_IN));
	}
	else
	{
		tcu_chn_writel(tcu_chn, CHN_TCSR, tcu_chn_readl(tcu_chn, CHN_TCSR) & ~(TCSR_PWM_IN));
	}
}
EXPORT_SYMBOL_GPL(jz_tcu_config_chn);

static void jz_tcu_irq_mask(struct irq_data *data)
{
	unsigned long flags;
	struct jz_tcu_chn *tcu_chn = irq_data_get_irq_chip_data(data);

	spin_lock_irqsave(&tcu_chn->tcu->lock, flags);
	switch (tcu_chn->irq_type) {
	case FULL_IRQ_MODE :
		jz_tcu_mask_full_irq(tcu_chn);
		break;
	case HALF_IRQ_MODE :
		jz_tcu_mask_half_irq(tcu_chn);
		break;
	case FULL_HALF_IRQ_MODE :
		jz_tcu_mask_full_irq(tcu_chn);
		jz_tcu_mask_half_irq(tcu_chn);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&tcu_chn->tcu->lock, flags);
}

static void jz_tcu_irq_unmask(struct irq_data *data)
{
	unsigned long flags;
	struct jz_tcu_chn *tcu_chn = irq_data_get_irq_chip_data(data);

	spin_lock_irqsave(&tcu_chn->tcu->lock, flags);
	switch (tcu_chn->irq_type) {
	case FULL_IRQ_MODE :
		jz_tcu_unmask_full_irq(tcu_chn);
		break;
	case HALF_IRQ_MODE :
		jz_tcu_unmask_half_irq(tcu_chn);
		break;
	case FULL_HALF_IRQ_MODE :
		jz_tcu_unmask_full_irq(tcu_chn);
		jz_tcu_unmask_half_irq(tcu_chn);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&tcu_chn->tcu->lock, flags);
}

static void jz_tcu_irq_ack(struct irq_data *data)
{
	unsigned long flags;
	struct jz_tcu_chn *tcu_chn = irq_data_get_irq_chip_data(data);

	spin_lock_irqsave(&tcu_chn->tcu->lock, flags);
	switch (tcu_chn->irq_type) {
	case FULL_IRQ_MODE :
		jz_tcu_clear_full_irq(tcu_chn);
		break;
	case HALF_IRQ_MODE :
		jz_tcu_clear_half_irq(tcu_chn);
		break;
	case FULL_HALF_IRQ_MODE :
		jz_tcu_clear_full_irq(tcu_chn);
		jz_tcu_clear_half_irq(tcu_chn);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&tcu_chn->tcu->lock, flags);
}

static void jz_tcu_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	struct jz_tcu *tcu = irq_desc_get_handler_data(desc);
	uint8_t status;
	unsigned int i;

	status = tcu_readl(tcu, TFR);
	for (i = 0; i < TCU_NR_IRQS; i++) {
		if (status & (1 << i))
			generic_handle_irq(tcu->irq_base + i);
	}
}

static struct irq_chip jz_tcu_irq_chip = {
	.name = "jz-tcu",
	.irq_mask = jz_tcu_irq_mask,
	.irq_disable = jz_tcu_irq_mask,
	.irq_unmask = jz_tcu_irq_unmask,
	.irq_ack = jz_tcu_irq_ack,
};

#define TCU_CELL_RES(NO)                                 \
	static struct resource tcucell##NO##_resources[] = { \
		{                                                \
			.start = NO,                                 \
			.flags = IORESOURCE_IRQ,                     \
		},                                               \
	}
TCU_CELL_RES(0);
TCU_CELL_RES(1);
TCU_CELL_RES(2);
TCU_CELL_RES(3);
#undef TCU_CELL_RES

static struct mfd_cell jz_tcu_cells[] = {
#define DEF_TCU_CELL_NAME(NO, NAME)                           \
	{                                                         \
		.id = NO,                                             \
		.name = NAME,                                         \
		.num_resources = ARRAY_SIZE(tcucell##NO##_resources), \
		.resources = tcucell##NO##_resources,                 \
		.platform_data = &g_tcu_chn[NO],                      \
		.pdata_size = sizeof(struct jz_tcu_chn)               \
	}

	DEF_TCU_CELL_NAME(0, "tcu_chn0"),
	DEF_TCU_CELL_NAME(1, "tcu_chn1"),
	DEF_TCU_CELL_NAME(2, "tcu_chn2"),
	DEF_TCU_CELL_NAME(3, "tcu_chn3"),
};
#undef DEF_TCU_CELL_NAME

static int jztcu_probe(struct platform_device *pdev)
{
	struct jz_tcu *tcu;
	struct resource *mem_base;
	int irq, irq_base, i, ret = 0;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
	{
		dev_err(&pdev->dev, "Failed to get platform irq\n");
		return irq;
	}

	irq_base = platform_get_irq(pdev, 1);
	if (irq_base < 0)
	{
		dev_err(&pdev->dev, "Failed to get irq base\n");
		return irq_base;
	}

	mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_base)
	{
		dev_err(&pdev->dev, "No iomem resource\n");
		return -ENXIO;
	}

	tcu = kmalloc(sizeof(struct jz_tcu), GFP_KERNEL);
	if (!tcu)
	{
		dev_err(&pdev->dev, "Failed to allocate driver struct\n");
		return -ENOMEM;
	}

	tcu->irq = irq;
	tcu->irq_base = irq_base;

	tcu->iomem = ioremap_nocache(mem_base->start, resource_size(mem_base));
	if (!tcu->iomem)
		goto err_ioremap;

	spin_lock_init(&tcu->lock);

	platform_set_drvdata(pdev, tcu);

	for (i = 0; i < NR_TCU_CHNS; i++) {
		g_tcu_chn[i].index = i;
		g_tcu_chn[i].reg_base = 0x40 + i * 0x10;
		g_tcu_chn[i].irq_type = NULL_IRQ_MODE;
		g_tcu_chn[i].pwm_bapass_mode = 0;
		g_tcu_chn[i].pwm_in = 0;
		g_tcu_chn[i].is_pwm = 0;
		g_tcu_chn[i].clk_src = TCU_CLKSRC_EXT;
		g_tcu_chn[i].tcu_mode = TCU_MODE_1;
		g_tcu_chn[i].prescale = TCU_PRESCALE_1;
		g_tcu_chn[i].init_level = 0;
		g_tcu_chn[i].shutdown_mode = 0;
		g_tcu_chn[i].tcu = tcu;
	}

	for (i = tcu->irq_base; i < tcu->irq_base + TCU_NR_IRQS; i++) {
		irq_set_chip_data(i, &g_tcu_chn);
		irq_set_chip_and_handler(i, &jz_tcu_irq_chip,
								 handle_level_irq);
	}

	irq_set_handler_data(tcu->irq, tcu);
	irq_set_chained_handler(tcu->irq, jz_tcu_irq_demux);

	ret = mfd_add_devices(&pdev->dev, 0, jz_tcu_cells,
			ARRAY_SIZE(jz_tcu_cells), mem_base, tcu->irq_base,NULL);
	if (ret < 0) {
		goto err_mfd_add;
	}
	tcu->chn_mask = 0;
	jztcu = tcu;
	printk("jz TCU driver register completed\n");

	return 0;
err_mfd_add:
	iounmap(tcu->iomem);
err_ioremap:
	kfree(tcu);

	return ret;
}

static int jztcu_remove(struct platform_device *pdev)
{
	struct jz_tcu *tcu = platform_get_drvdata(pdev);

	mfd_remove_devices(&pdev->dev);

	irq_set_handler_data(tcu->irq, NULL);
	irq_set_chained_handler(tcu->irq, NULL);

	iounmap(tcu->iomem);

	platform_set_drvdata(pdev, NULL);

	kfree(tcu);

	return 0;
}

struct platform_driver jztcu_driver = {
	.probe = jztcu_probe,
	.remove = jztcu_remove,
	.driver = {
		.name = "jz-tcu",
		.owner = THIS_MODULE,
	},
};

static int __init jztcu_init(void)
{
	return platform_driver_register(&jztcu_driver);
}
module_init(jztcu_init);

static void __exit jztcu_exit(void)
{
	platform_driver_unregister(&jztcu_driver);
}
module_exit(jztcu_exit);

MODULE_LICENSE("GPL v2");
