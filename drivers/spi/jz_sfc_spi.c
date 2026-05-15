/* linux/drivers/spi/jz_spi.c
 *
 * SSI controller for SPI protocol,use FIFO and DMA;
 * base-to: linux/drivers/spi/spi_bitbang.c
 *
 * Copyright (c) 2010 Ingenic
 * Author:Shumb <sbhuang@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/jzssi.h>

#include "jz_sfc_spi.h"
#include "linux/completion.h"
#include "linux/irqreturn.h"

#define USE_MODE1 1 //该模式是使用cmd 配置addr，用于sfc1 支持ssi功能的
#define USE_IC_CONFIG 0
// #define SSI_DEGUG
#ifdef SSI_DEGUG
#define  print_dbg(format,arg...)			\
	printk(format,## arg)
#else
#define  print_dbg(format,arg...)
#endif

#define JZ_SPI_RX_BUF(type) 				\
u32 jz_spi_rx_buf_##type(struct jz_spi *hw) 		\
{								\
	u32 data  = spi_readl(hw, SFC_DR);			\
	type * rx = (type *)hw->rx;				\
	*rx++ = (type)(data);			  		\
	hw->rx = (u8 *)rx;					\
	return (u32)data;					\
}

#define JZ_SPI_TX_BUF(type)					\
u32 jz_spi_tx_buf_##type(struct jz_spi *hw)		\
{								\
	u32 data;						\
	const type * tx = (type *)hw->tx;			\
	data = *tx++;						\
	hw->tx = (u8 *)tx;					\
	sfc_transmit_data(hw, data);				\
	return (u32)data;					\
}

JZ_SPI_RX_BUF(u8)
JZ_SPI_TX_BUF(u8)

JZ_SPI_RX_BUF(u16)
JZ_SPI_TX_BUF(u16)

JZ_SPI_RX_BUF(u32)
JZ_SPI_TX_BUF(u32)

/* the spi->mode bits understood by this driver: */
#define MODEBITS (SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST | SPI_LOOP)
#define SPI_BITS_SUPPORT  (SPI_BITS_8 | SPI_BITS_16 | SPI_BITS_32)

static void jz_spi_cs(struct jz_spi_info *spi, u8 cs, unsigned int pol)
{
#ifdef CONFIG_JZ_SPI_PIO_CE
	u32 pin_value = *(spi->chipselect + cs);
	printk("pin_value:%d,pol:%d\n",pin_value,pol);
	gpio_direction_output(pin_value, !pol ? 0 : 1);
#endif
}

static void jz_spi_chipsel(struct spi_device *spi, int value)
{
	struct jz_spi *hw = spi_master_get_devdata(spi->master);
	unsigned int cspol = spi->mode & SPI_CS_HIGH ? 1 : 0;

	switch (value) {
	case BITBANG_CS_INACTIVE:
		/* chip disable selected */
		if (hw->set_cs && hw->pdata)
			hw->set_cs(hw->pdata, spi->chip_select, cspol^1);
		break;
	case BITBANG_CS_ACTIVE:
		if (spi->mode & SPI_CPHA)
			sfc_set_spi_clock_phase(hw, 1);
		else
			sfc_set_spi_clock_phase(hw, 0);

		if (spi->mode & SPI_CPOL)
			sfc_set_spi_clock_polarity(hw, 1);
		else
			sfc_set_spi_clock_polarity(hw, 0);

		if (!(spi->mode & SPI_LSB_FIRST)) {
			sfc_set_tx_msb(hw);
			sfc_set_rx_msb(hw);
		} else {
			sfc_set_tx_lsb(hw);
			sfc_set_rx_lsb(hw);
		}
		sfc_set_tx_lsb(hw);//12.26
#if 0 //sfc中没有loop，先屏蔽
		if (spi->mode & SPI_LOOP)
			enable_loopback(hw);
		else
			disable_loopback(hw);
#endif
		/* chip enable selected */
		if (hw->set_cs && hw->pdata)
			hw->set_cs(hw->pdata, spi->chip_select, cspol);
		break;
	default:
		break;
	}
}

void sfc_dump_reg(struct jz_spi *hw,const char *str)
{
	// struct jz_spi *hw = spi_master_get_devdata(spi->master);
	int i = 0;
	printk("%s\n",str);
	printk("SFC_GLB0                :0x%08x\n", spi_readl(hw, SFC_GLB0));
	printk("SFC_GLB1                :0x%08x\n", spi_readl(hw, SFC_GLB1));
	printk("SFC_DEV_CONF            :0x%08x\n", spi_readl(hw, SFC_DEV_CONF));
	printk("SFC_DEV_STA_EXP         :0x%08x\n", spi_readl(hw, SFC_DEV_STA_EXP));
	printk("SFC_DEV_STA_RT          :0x%08x\n", spi_readl(hw, SFC_DEV_STA_RT));
	printk("SFC_DEV1_STA_RT         :0x%08x\n", spi_readl(hw, SFC_DEV1_STA_RT));
	printk("SFC_DEV_STA_MSK         :0x%08x\n", spi_readl(hw, SFC_DEV_STA_MSK));
	printk("SFC_TRAN_LEN            :0x%08x\n", spi_readl(hw, SFC_TRAN_LEN));
	for (i = 0; i < 6; i++)
		printk("SFC_TRAN_CONF(%d)        :0x%08x\n", i, spi_readl(hw, SFC_TRAN_CONF(i)));
	for (i = 0; i < 6; i++)
		printk("SFC_TRAN_CONF1(%d)       :0x%08x\n", i, spi_readl(hw, SFC_TRAN_CONF1(i)));
	for (i = 0; i < 6; i++)
		printk("SFC_DEV_ADDR(%d)         :0x%08x\n", i, spi_readl(hw, SFC_DEV_ADDR(i)));
	printk("SFC_MEM_ADDR            :0x%08x\n", spi_readl(hw, SFC_MEM_ADDR));
	printk("SFC_TRIG                :0x%08x\n", spi_readl(hw, SFC_TRIG));
	printk("SFC_SR                  :0x%08x\n", spi_readl(hw, SFC_SR));
	printk("SFC_SCR                 :0x%08x\n", spi_readl(hw, SFC_SCR));
	printk("SFC_INTC                :0x%08x\n", spi_readl(hw, SFC_INTC));
	printk("SFC_FSM                 :0x%08x\n", spi_readl(hw, SFC_FSM));
	printk("SFC_CGE                 :0x%08x\n", spi_readl(hw, SFC_CGE));
	// printk("SFC_DR                  :0x%08x\n", spi_readl(hw, SFC_DR));
}

/*************************************************************
 * jz_spi_set_clk: set the SPI_CLK.
 * The min clock is 23438Hz, and the max clock is defined
 * by max_clk or max_speed_hz(it is 54MHz for JZ4780, and
 * the test max clock is 30MHz).
 ************************************************************* */
static int jz_spi_set_clk(struct spi_device *spi, u32 hz)
{
	u16 cgv;
	unsigned long cpm_rate_a, cpm_rate_b;
	struct jz_spi *hw = spi_master_get_devdata(spi->master);

	if (hz > hw->pdata->max_clk) {
		pr_err("The max_speed_hz can not more than max_clk\n");
		return -EINVAL;
	}

	if (hw->set_clk_flag == 1)
		return 0;

	if (!hw->clk) {
		// hw->clk = clk_get(hw->dev, "div_ssi");
		hw->clk = clk_get(hw->dev, "div_sfc1");
		if (!hw->clk)
			return -EINVAL;
	}

	cpm_rate_a = clk_get_rate(hw->clk);

	if (hz >= 10000000)	{
		clk_set_rate(hw->clk, 2 * hz);
	} else {
		clk_set_rate(hw->clk, 5000000);
	}

	cpm_rate_b =  clk_get_rate(hw->clk);

	cgv = cpm_rate_b / (2 * hz);
	if (cgv > 0)
		cgv -= 1;

	// spi_writel(hw, SSI_GR, cgv);


	/* if the external clock is selected, cpm_rate_a equals to cpm_rate_b */
	if (cpm_rate_a != cpm_rate_b) {
		if (!clk_is_enabled(hw->clk))
			clk_enable(hw->clk);
	}
	hw->set_clk_flag = 1;

	return 0;
}

static u32 jz_spi_get_clk(struct spi_device *spi)
{
	u16 cgv;
	struct jz_spi *hw = spi_master_get_devdata(spi->master);

	cgv = spi_readl(hw, SSI_GR);

	return clk_get_rate(hw->clk) / (2 * (cgv + 1));
}

static void dma_tx_callback(void *data)
{
	struct jz_spi *hw = data;

	dma_unmap_sg(hw->txchan->device->dev, hw->sg_tx, 1, DMA_TO_DEVICE);
	complete(&hw->done_tx_dma);
}

static void dma_rx_callback(void *data)
{
	struct jz_spi *hw = data;

	dma_unmap_sg(hw->txchan->device->dev, hw->sg_tx, 1, DMA_TO_DEVICE);
	dma_unmap_sg(hw->rxchan->device->dev, hw->sg_rx, 1, DMA_FROM_DEVICE);
	complete(&hw->done_rx_dma);
}

static int jz_spi_dma_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	int ret;
	struct jz_spi *hw = spi_master_get_devdata(spi->master);
	struct dma_slave_config rx_config, tx_config;
	struct dma_async_tx_descriptor *rxdesc;
	struct dma_async_tx_descriptor *txdesc;
	struct dma_chan *rxchan = hw->rxchan;
	struct dma_chan *txchan = hw->txchan;
	struct jz_intr_cnt *g_jz_intr;
	int dma_ds[] = {64, 32, 16, 4, 2, 1};
	int i;

	/* Check that the channels are available */
	if (!txchan || !rxchan) {
		dev_err(&spi->dev, "no dma channel\n");
		return -ENODEV;
	}

	if (t->len % hw->transfer_unit_size) {
		pr_err("The length of tranfer data is error\n");
		return -EFAULT;
	}

	hw->rw_mode = 0;
	if(t->tx_buf)
		hw->rw_mode |= W_MODE;
	if(t->rx_buf)
		hw->rw_mode |= R_MODE;

	/* all transfer starts with tx, ends with rx. */
	if (hw->rw_mode & W_MODE)
		hw->tx = t->tx_buf;
	else
		hw->tx = hw->buffer;

	if (hw->rw_mode & R_MODE)
		hw->rx = t->rx_buf;
	else
		hw->rx = hw->buffer;

	memset(hw->buffer, 0, BUFFER_SIZE);

	switch (hw->transfer_unit_size) {
	case SPI_8BITS:
		tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		tx_config.dst_maxburst = 64;
		tx_config.src_maxburst = 1;
		rx_config.src_maxburst = 64;
		rx_config.dst_maxburst = 1;
		break;
	case SPI_16BITS:
		tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		tx_config.dst_maxburst = 2;
		tx_config.src_maxburst = 2;
		rx_config.src_maxburst = 2;
		rx_config.dst_maxburst = 2;
		break;
	case SPI_32BITS:
		tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		tx_config.dst_maxburst = 4;
		tx_config.src_maxburst = 4;
		rx_config.src_maxburst = 4;
		rx_config.dst_maxburst = 4;
		break;
	}

	tx_config.dst_addr = (dma_addr_t)(hw->phys + SSI_DR);
	rx_config.src_addr = (dma_addr_t)(hw->phys + SSI_DR);
	dmaengine_slave_config(txchan, &tx_config);
	dmaengine_slave_config(rxchan, &rx_config);

	/* set tx dma trigger */
	for (i = 0; i < ARRAY_SIZE(dma_ds); i++) {
		if (t->len / dma_ds[i])
			break;
	}

	if (i < ARRAY_SIZE(dma_ds)) {
		hw->dma_tx_unit = dma_ds[i];
	} else {
		print_dbg("DMA tx block_size force to defaut set!!!");
		hw->dma_tx_unit = JZ_SSI_DMA_BURST_LENGTH;
	}

	hw->tx_trigger = hw->dma_tx_unit / (hw->txfifo_width >> 3);
	//set_tx_trigger(hw, hw->tx_trigger);
	set_tx_trigger(hw, 8); //The transfer is steady if the trigger number is used
	print_dbg("t->len: %d, tx fifo width: %d, set tx trigger value to %d\n", t->len, hw->txfifo_width, hw->tx_trigger);

	sg_init_one(hw->sg_tx, hw->tx, t->len);
	if (dma_map_sg(hw->txchan->device->dev,
		       hw->sg_tx, 1, DMA_TO_DEVICE) != 1) {
		dev_err(&spi->dev, "dma_map_sg tx error\n");
		printk("%s LINE %d: %s\n", __func__, __LINE__, __FILE__);
		goto err_tx_sgmap;
	}

	txdesc = txchan->device->device_prep_slave_sg(txchan,
				      hw->sg_tx,
				      1,
				      DMA_TO_DEVICE,
				      DMA_PREP_INTERRUPT | DMA_CTRL_ACK,NULL);
	if (!txdesc) {
		dev_err(&spi->dev, "device_prep_slave_sg error\n");
		printk("%s LINE %d: %s\n", __func__, __LINE__, __FILE__);
		goto err_txdesc;
	}

	// config controller
	disable_tx_intr(hw);
	disable_rx_intr(hw);

	//revisit
	disable_tx_error_intr(hw);
	disable_rx_error_intr(hw);

	start_transmit(hw);
	//finish_transmit(hw);

	flush_fifo(hw);

	enable_receive(hw);
	clear_errors(hw);

	g_jz_intr = hw->g_jz_intr;
	memset(g_jz_intr, 0, sizeof *g_jz_intr);

	if (!(hw->rw_mode & R_MODE)) {
		txdesc->callback = dma_tx_callback;
		txdesc->callback_param = hw;
		enable_tx_error_intr(hw);

		dmaengine_submit(txdesc);
		dma_async_issue_pending(txchan);

		ret = wait_for_completion_interruptible_timeout(&hw->done_tx_dma, 60 * HZ);
		if (ret <= 0) {
			printk("The tx_dma umap wait timeout\n");
			goto err_txdesc;
		}
		ret = wait_for_completion_interruptible_timeout(&hw->done, 60 * HZ);
		if (ret <= 0) {
			printk("The spi transfer wait timeout\n");
			goto err_txdesc;
		}
		if(t->cs_change){
		finish_transmit(hw);
		}
		flush_rxfifo(hw);
		clear_errors(hw);

		return t->len;
	}
	/*  prepare spi dma rx */
	for (i = 0; i < ARRAY_SIZE(dma_ds); i++) {
		if (!(t->len % dma_ds[i]))
			break;
	}

	if (i < ARRAY_SIZE(dma_ds)) {
		hw->dma_rx_unit = dma_ds[i];
	} else {
		print_dbg("DMA rx block_size force to defaut set!!!");
		hw->dma_rx_unit = JZ_SSI_DMA_BURST_LENGTH;
	}

	hw->rx_trigger = hw->dma_rx_unit/(hw->rxfifo_width >> 3);
	//set_rx_trigger(hw, hw->rx_trigger);
	set_rx_trigger(hw, 1); //the rx trigger is steady for tranfer
	print_dbg("t->len: %d, rx fifo width: %d, set rx trigger value to %d\n", t->len, hw->rxfifo_width, hw->rx_trigger);

	sg_init_one(hw->sg_rx, hw->rx, t->len);

	if (dma_map_sg(hw->rxchan->device->dev,
		       hw->sg_rx, 1, DMA_FROM_DEVICE) != 1) {
		dev_err(&spi->dev, "dma_map_sg rx error\n");
		goto err_rx_sgmap;
	}

	rxdesc = rxchan->device->device_prep_slave_sg(rxchan,
				      hw->sg_rx,
				      1,
				      DMA_FROM_DEVICE,
				      DMA_PREP_INTERRUPT | DMA_CTRL_ACK,NULL);
	if (!rxdesc) {
		dev_err(&spi->dev, "device_prep_slave_sg error\n");
		goto err_rxdesc;
	}

	txdesc->callback = NULL;
	txdesc->callback_param = NULL;

	rxdesc->callback = dma_rx_callback;
	rxdesc->callback_param = hw;
	enable_rx_error_intr(hw);
	enable_tx_error_intr(hw);

	dmaengine_submit(txdesc);
	dmaengine_submit(rxdesc);
	dma_async_issue_pending(rxchan);
	dma_async_issue_pending(txchan);

	ret = wait_for_completion_interruptible_timeout(&hw->done_rx, 60 * HZ);
	if (ret <= 0) {
		dump_spi_reg(hw);
		printk("The spi receiver wait timeout\n");
		goto err_rxdesc;
	}

	ret = wait_for_completion_interruptible_timeout(&hw->done_rx_dma, 60 * HZ);
	if (ret <= 0) {
		dump_spi_reg(hw);
		printk("The spi dam_callback wait timeout\n");
		goto err_rxdesc;
	}

	finish_transmit(hw);
	//flush_rxfifo(hw);
	clear_errors(hw);

	return t->len;

err_rxdesc:
	dma_unmap_sg(rxchan->device->dev, hw->sg_rx, 1, DMA_FROM_DEVICE);
err_rx_sgmap:
err_txdesc:
	dma_unmap_sg(txchan->device->dev, hw->sg_tx, 1, DMA_TO_DEVICE);
err_tx_sgmap:
	printk("<< dma_txrx error. out of memory >>\n");
	return -ENOMEM;
}

static irqreturn_t jz_spi_dma_irq_callback(struct jz_spi *hw)
{
	struct jz_intr_cnt *g_jz_intr = hw->g_jz_intr;
	print_dbg("%s: status register: %08x\n", __func__, spi_readl(hw, SSI_SR));

	if (ssi_underrun(hw) && tx_error_intr(hw)) {
		print_dbg("UNDR:\n");

		g_jz_intr->ssi_eti++;
		disable_tx_error_intr(hw);

		clear_errors(hw);
		complete(&hw->done);
		complete(&hw->done_rx);

		goto irq_done;
	}

	if (ssi_overrun(hw) && rx_error_intr(hw)) {
			print_dbg(" overrun:\n");
			g_jz_intr->ssi_eri++;

			clear_errors(hw);
			complete(&hw->done);
			complete(&hw->done_rx);
	}

irq_done:
	return IRQ_HANDLED;
}

static inline u32 cpu_read_rxfifo(struct jz_spi *hw)
{
	u8 unit_size = hw->transfer_unit_size;
	u32 cnt, dat;
	u32 i;
	int dummy_read = 0;

	// print_dbg("The count of RxFIFO is %d \n", get_rxfifo_count(hw));
	// printk("The count of RxFIFO is %d ,SFC_DEV_STA_RT:%#x\n", sfc_get_txrxfifo_count(hw),spi_readl(hw,SFC_DEV_STA_RT));
#if USE_MODE1
	#ifdef SSI_DEGUG
		sfc_dump_reg(hw,"before clear tx rx sr status\n");
	#endif
	while(!sfc_get_txrxfifo_count(hw)){

	}
	// printk("wait rreq\n");
	while(!sfc_rxfifo_half_full(hw)){

	}

	sfc_clear_rreq(hw);
	print_dbg("after wait rxfifo write down:%d,req:%d\n",sfc_get_txrxfifo_count(hw),sfc_rxfifo_half_full(hw));
#endif
	// sfc_set_transferdir(hw,R_MODE);//共用fifo时，需要设置传输方向

	// if (sfc_get_txrxfifo_count(hw) < 1)
	// 	return 0;

	cnt = hw->rlen;
	if ((hw->rw_mode & RW_MODE) == W_MODE) {
		print_dbg("this is read mode ,but mode has W_MODE\n");
		dummy_read = 1;

		//这里使用rlen表示可以读的数据长度
		hw->rlen = hw->len;//sfc_get_txrxfifo_count(hw);
		return hw->rlen;
	}

	spin_lock(&hw->lock);

	// while (!rxfifo_empty(hw))
	while (sfc_get_txrxfifo_count(hw)) //若fifo不为空，则把fifo中的数据都给读出来
	{
		hw->rlen += unit_size;
		// sfc_set_transferdir(hw,W_MODE);
		// spi_writel(hw, SFC_DR, 0xff);
		// spi_writel(hw, SFC_DR, 0xff);
		sfc_set_transferdir(hw,R_MODE);
		// spi_writel(hw, SFC_DR, 0xff);
		// spi_writel(hw, SFC_DR, 0xff);
		// spi_writel(hw, SFC_DR, 0xff);

		// printk("wr dr:0xff\n");
		// if (dummy_read)
			// dat = spi_readl(hw, SFC_DR);//dat = spi_readl(hw, SSI_DR);
		// else
		// for(i = 0; i < 100;i++)
			dat = hw->get_rx(hw);

		printk("data:%#x,sta_rt:%#x\n",dat,spi_readl(hw,SFC_DEV_STA_RT));
	}

	spin_unlock(&hw->lock);

	return (hw->rlen - cnt);
}

static inline u32 cpu_write_txfifo(struct jz_spi *hw, u32 entries)
{
	u8 unit_size = hw->transfer_unit_size;
	u32 i, cnt, count;
	u32 dat;
	u32 j;

	if ((!entries ) || (!(hw->rw_mode & RW_MODE)))
		return 0;

	cnt = entries;
	count = cnt * unit_size;

	spin_lock(&hw->lock);
	//1、先写设备地址，通过dr写数据到fifo，
	//2、若第二个message是读数据，则从DR中读数据，dr会自动从fifo中获取从设备给的数据
	if (hw->rw_mode & W_MODE) {
		// sfc_set_transferdir(hw,W_MODE);
		print_dbg("start to transmit\n");
		// sfc_start_transmit(hw);//1230 在写完数据之后再start
		for (i = 0; i < cnt; i++) {
			hw->count += unit_size;
			dat = (u32)(hw->get_tx(hw));

			//下面的是造overflow
			// dat = (u32)(hw->get_tx(hw));
			// for(j = 0;j < 100;j++)
			// 	sfc_transmit_data(hw, 0xa5);
		}

	} else {		 /* read, fill txfifo with 0 */
#if 0 //这里是从DR中读数据，不应该再写0到fifo中，毕竟sfc只有一个fifo
		for (i = 0; i < cnt; i++) {
			hw->count += unit_size;
			// transmit_data(hw, 0);
			sfc_transmit_data(hw, 0);
		}
#endif
		cpu_read_rxfifo(hw);//直接从DR中读
	}
	spin_unlock(&hw->lock);

	print_dbg("hw->count:%d. %s LINE %d: %s\n", hw->count, __func__, __LINE__, __FILE__);
	return count;
}

static unsigned long sfc_get_tx_data_andnext(struct jz_spi *hw)
{
	u32 data = 0;
	u32 unit_size;
	unit_size = hw->transfer_unit_size;

	if (unit_size == SPI_8BITS){
		const u8 *tx = (u8 *)hw->tx;
		data = *tx++;
		hw->tx = (u8 *)tx;
	} else if (unit_size == SPI_16BITS )  {
		const u16 *tx = (u16 *)hw->tx;
		data = *tx++;
		hw->tx = (u8 *)tx;
	} else if (unit_size == SPI_32BITS ){
		const u32 *tx = (u32 *)hw->tx;
		data = *tx++;
		hw->tx = (u8 *)tx;
	}

	return data;
}

static int jz_spi_cpu_transfer(struct jz_spi *hw, long length)
{
	unsigned char int_flag = 0, last_flag = 0;
	u32 entries = 0, send_entries = 0;
	u32 unit_size, trigger;
	long leave_len_bytes;
	u32 retlen;
	u32 data;

	//print_dbg("%s LINE %d: %s\n", __func__, __LINE__, __FILE__);

	/* calculate the left entries */
	leave_len_bytes = hw->len - hw->count;
	// print_dbg("%s LINE %d: %s, translen:%d\n", __func__, __LINE__, __FILE__,leave_len_bytes);

	if (hw->len < hw->count) {
		dev_err(hw->dev,
			"Fill data len error!!!(len : count > %d : %d)\n",
			hw->len, hw->count);
		return -1;
	}

	if (leave_len_bytes == 0) {
		print_dbg("leave_len_bytes = 0\n");
		// printk("leave_len_bytes = 0\n");
		return 0;
	}

	if (hw->len % hw->transfer_unit_size) {
		pr_err("The length of tranfer data is error\n");
		return -EFAULT;
	}

	sfc_set_transfer_len(hw, leave_len_bytes);//硬件是按字节设置的吗，前面的fifo相关操作都是按word进行的,经过确认，这里是按字节设置的，要注意了
	// sfc_set_transfer_len(hw, 4);

	unit_size = hw->transfer_unit_size;
	if (unit_size == SPI_8BITS)
		entries = leave_len_bytes;
	else if (unit_size == SPI_16BITS )
		entries = leave_len_bytes >> 1;
	else if (unit_size == SPI_32BITS )
		entries = leave_len_bytes >> 2;
	else {
		dev_err(hw->dev,"transfer_unit_size error!\n");
		return -1;
	}
	// print_dbg("%s unit_size:%d, entries:%d\n", __func__, unit_size, entries);
	// printk("%s unit_size:%d, entries:%d,isfirst:%d,mode:%d\n", __func__, unit_size, entries,hw->is_first,hw->rw_mode);

	/* calculate the entries which will be sent currently
	 * distinguish between the first and interrupt */
	if (hw->is_first) {
		/* CPU Mode should reset SSI triggers at first */
		if(hw->rw_mode & W_MODE){
			hw->tx_trigger = SSI_TX_FIFO_THRESHOLD * 8;
			// hw->tx_trigger = SSI_TX_FIFO_THRESHOLD * 8*2;//12.26 test
			sfc_set_tx_trigger(hw, hw->tx_trigger);
		} else {
			// hw->rx_trigger = (SSI_RX_FIFO_THRESHOLD - SSI_SAFE_THRESHOLD) * 8;//为什么之前的阈值是0xe,但是读数据时，好像也可以一个字节就能读到。
			hw->tx_trigger = SSI_TX_FIFO_THRESHOLD * 8;
			// hw->tx_trigger = SSI_TX_FIFO_THRESHOLD * 8*2;//12.26 test
			sfc_set_tx_trigger(hw, hw->tx_trigger);
		}

		if(entries <= JZ_SSI_MAX_FIFO_ENTRIES)	{
			send_entries = entries;
			// last_flag = 1;
		} else {
			/* need enable half_intr, left entries will be sent
			   in SSI interrupt and receive the datas */
			send_entries = JZ_SSI_MAX_FIFO_ENTRIES;
			int_flag = 1;
		}
		// sfc_start_transmit(hw);//方案2屏蔽

		hw->is_first = 0;
	} else { /* happen in interrupts */
		trigger = JZ_SSI_MAX_FIFO_ENTRIES - hw->tx_trigger;
		if (entries <= trigger) {
			send_entries = entries;
			/* the last part of data shouldn't disable RXI_intr
			   at once !!! */
			last_flag = 1;
		} else {
			/* need enable half_intr, left entries will be sent
			   in SSI interrupt and receive the datas */
			send_entries = CPU_ONCE_BLOCK_ENTRIES;
			int_flag = 1;
		}
	}

	if (length > 0) {
		length = length/hw->transfer_unit_size;
		if (length < send_entries)
			send_entries = length;
	}

#if 1
	// sfc_enable_fifo_half_empty_intr(hw);
	// sfc_enable_fifo_half_full_intr(hw);
	//等待tx
	if (hw->rw_mode & W_MODE) {
#if !USE_MODE1
		sfc_set_transferdir(hw,W_MODE);
#else
		sfc_set_transferdir(hw,R_MODE);//方案二，这里全部配置成读模式
#endif
	}else {
		sfc_set_transferdir(hw,R_MODE);
	}
#if !USE_MODE1
	sfc_set_tran_conf1(hw);
	sfc_start_transmit(hw);//12.30
#endif

#if !USE_MODE1
	//1.9 wj:若一个transfer是读数据呢，则怎么也不会产生这个状态的
	// while(!sfc_txfifo_half_empty(hw)){

	// 	// printk("wait to tran_req to 1,now:%d\n",sfc_txfifo_half_empty(hw));
	// }
#endif
	// printk("wait to write......:tran_req%d,rreq:%d\n",sfc_txfifo_half_empty(hw),sfc_rxfifo_half_full(hw));
	sfc_clear_treq(hw);
	// sfc_clear_rreq(hw);//为了造出这两个状态的中断，故这里先屏蔽,第二个transfer读这里时，不能屏蔽，应该让其保持状态，读
	// printk("clr......:tran_req%d,rreq:%d\n",sfc_txfifo_half_empty(hw),sfc_rxfifo_half_full(hw));

#endif
	/* fill the txfifo with CPU Mode */
	//这个返回值是实际tx的值word
#if USE_MODE1
	//设置设备地址
	sfc_set_wp_en(hw);
	// sfc_disable_all_intr(hw);//屏蔽所有中断，传输完成后应该要开启吧
	sfc_enable_all_intr(hw); //表明后面满足条件即可触发中断
	sfc_set_glb1(hw);
	// sfc_set_tran_conf1(hw);
	// sfc_set_smp_delay(hw,DEV_CONF_SMP_DELAY_45);
	// sfc_set_smp_delay(hw,DEV_CONF_SMP_DELAY_135);
	// sfc_set_sta_exp(hw,0x81);
	// sfc_set_sta_mask(hw, 0x1);
	// if (hw->rw_mode & W_MODE)

	//若是读，则第一次先写设备地址到cmd中
	if(hw->tx && (1 == hw->len)){
		print_dbg("set rd cmd addr:%#lx\n",(unsigned long)(*hw->tx));
		sfc_set_cmd(hw,(unsigned long)(*hw->tx));
	} else if (hw->tx && (hw->len > 1)){
		sfc_set_transferdir(hw,W_MODE);
		data = sfc_get_tx_data_andnext(hw);
		send_entries -= 1;
		print_dbg("set wr cmd addr:%#x\n",data);
		sfc_set_cmd(hw,data);
	}

	// printk("(0)bf start:\n");
	// sfc_dump_reg(hw,"bf start");

	sfc_start_transmit(hw);
	// printk("(0)af start:\n");

	hw->count += hw->transfer_unit_size*send_entries;//以字节为单位，wordsize*wordnum

	if((hw->tx != NULL ) && (hw->len > 1)){

		retlen = cpu_write_txfifo(hw, send_entries);
	}

	if(hw->rx){
		// printk("wait rx irq to deal\n");
	}

#else
	// sfc_disable_end_intr(hw);
	retlen = cpu_write_txfifo(hw, send_entries);
	if (!retlen) {
		dev_info(hw->dev,"cpu_write_txfifo error!\n");
		return -1;
	}
#endif

	sfc_enable_tx_error_intr(hw);
	sfc_enable_rx_error_intr(hw);
	// sfc_disable_all_intr(hw);
	/* every time should control the SSI half_intrs */
	if (int_flag) {

		sfc_enable_fifo_half_full_intr(hw);
		sfc_enable_fifo_half_empty_intr(hw);
	} else {
		// sfc_disable_fifo_half_full_intr(hw);//req这里关闭了，怎么还会产生中断呢？然后产生的并不是tx、rx中断，而是end或者under run中断，表示传输完成，可以从dr中读数据了
		// sfc_disable_fifo_half_empty_intr(hw);//teq
		// sfc_enable_fifo_half_full_intr(hw);
		// sfc_enable_fifo_half_empty_intr(hw);
	}

	/* 针对sfc1 这里后面都可以删除掉 */
	if (last_flag) {
		last_flag = 0;
		sfc_enable_fifo_half_empty_intr(hw);
		sfc_enable_fifo_half_full_intr(hw);
	}

#ifdef SSI_DEGUG
	sfc_dump_reg(hw,"after cpu_transfer");
#endif

	return 0;
}

static int jz_spi_pio_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct jz_spi *hw = spi_master_get_devdata(spi->master);
	struct jz_intr_cnt *g_jz_intr = hw->g_jz_intr;
	u32 entries;
	int status;
	unsigned long flags;

	hw->tx = t->tx_buf;
	hw->rx = t->rx_buf;
	hw->len = t->len;
	hw->count = 0;
	hw->rlen = 0;
	hw->dma_flag &= ~SPI_DMA_ACK;

	hw->rw_mode = 0;
	if(hw->tx)
		hw->rw_mode |= W_MODE;
	if(hw->rx)
		hw->rw_mode |= R_MODE;

#if USE_IC_CONFIG
	//250103 这里是用IC给的寄存器配置来操作
// 	1、 配置 sfc_glb1 [1:0] chip_sel=00;
	u32 tmp;
	tmp = spi_readl(hw, SFC_INTC);
	tmp &= ~0x3;
	spi_writel(hw, SFC_GLB1, tmp);
// 2、 配置 sfc_glb0 [31:14]=0 关闭poll,des,cdt模式，[13]=0接受模式,  [12:7]threshold=1,[6]=0 cpu模式，[5:3]=1 Phase=1 ;
	tmp = spi_readl(hw, SFC_GLB0);
	tmp &= ~(0xffffffff);
	tmp |= 0x1<<7 | 1 << 3 | 0x7;
	spi_writel(hw, SFC_GLB0, tmp);
// 3、 配置 sfc_dev_conf  [31]cmd MSB/LSB与spi_slave保持一致 ，[4:3] CPHA和CPOL与spi_slave保持一致 0/0，[2:0] 默认值111;
	tmp = spi_readl(hw, SFC_DEV_CONF);
	tmp |= 0x3;
	spi_writel(hw, SFC_DEV_CONF, tmp);
// 4、 配置 sfc_tran_cfg0(0)  [31:29]=0 SDR模式, [28:17]=0, [24]=1 cmd_en  [16]=1  data_en, [15:0] cmd=0x00或0x01;
	tmp = spi_readl(hw, SFC_TRAN_CONF(0));
	tmp |= 1<<24 | 1<<16;
	tmp |= 1;//(unsigned long)(*hw->tx) & 0xff;//cmd addr
	spi_writel(hw, SFC_TRAN_CONF(0), tmp);
// 5、 配置 sfc_tran_cfg1(0)  [18]data MSB/LSB与spi_slave保持一致,其他全0；
	tmp = spi_readl(hw, SFC_TRAN_CONF1(0));
	tmp &= 0;
	spi_writel(hw, SFC_TRAN_CONF1(0), tmp);
// 6、 配置 sfc_tran_len  单位：bytes ,配置1；
	tmp = spi_readl(hw, SFC_TRAN_LEN);
	tmp |= 0x1;
	spi_writel(hw, SFC_TRAN_LEN, tmp);
// mask all int
	sfc_disable_all_intr(hw);
// 7、 配置 sfc_trigger [0]=1  start;
 	sfc_dump_reg(hw,"ic_config");
	tmp = spi_readl(hw, SFC_TRIG);
	tmp |= 1<<0 |1 <<2 ;
	spi_writel(hw, SFC_TRIG, tmp);
// 8、 等待sfc_status [2]=1 rece_req,  配置sfc_status_clear[2]  clear rece_req,  从sfc_dr 读数据，
//循环步骤8直至所有数据被读出，等待sfc_status[4]=1 tran_end , 然后sfc_status_clear[4]=1 clear end;
	printk("start wait rreq \n");
	while(!sfc_rxfifo_half_full(hw)){

	}
	printk("wait rreq 1 start to clear\n");
	sfc_clear_rreq(hw);

	while(!sfc_get_txrxfifo_count(hw)){

	}

	printk("fifo count:%d\n",sfc_get_txrxfifo_count(hw));
	printk("dr:%#x\n",spi_readl(hw,SFC_DR));
	printk("dr:%#x\n",spi_readl(hw,SFC_DR));

	return 1;
#endif

	sfc_disable_tx_intr(hw);
	sfc_disable_rx_intr(hw);

#if USE_MODE1
	//打开cmd，但是实测好像也是无效的,打开这个可以有时钟，否则没有时钟产生
	sfc_set_cmd_en(hw);
	// sfc_set_poll_en(hw);
	// sfc_start_transmit(hw);//方案一需要在这里打开，方案二，需要屏蔽，在传输时打开,放到最后
#else
	// sfc_start_transmit(hw);//方案一需要在这里打开，方案二，需要屏蔽，在传输时打开，注意：暂时调整到设置阈值后，否则无法产生sr寄存器的treq

	// sfc_enable_receive(hw);//有数据传输data_en
#endif

	if(hw->tx)
		sfc_flush_fifo(hw);

	sfc_clear_errors(hw);
	sfc_enable_receive(hw);//有数据传输data_en

	memset(g_jz_intr, 0, sizeof(struct jz_intr_cnt));
	/* Calculate Max IRQ numbers for SSI error out */
	entries = hw->len * 8 / hw->bits_per_word;
	g_jz_intr->max_ssi_intr = (entries + JZ_SSI_MAX_FIFO_ENTRIES - 1) /
				  JZ_SSI_MAX_FIFO_ENTRIES * 2 + 2;

#ifdef SSI_DEGUG
	// dump_spi_reg(hw);
#endif

	/* This start SSI transfer, write data or 0 to txFIFO.
	 * irq is locked to protect SSI config registers */
	spin_lock_irqsave(&hw->txrx_lock, flags);
	hw->is_first = 1;
	status = jz_spi_cpu_transfer(hw, 0);
	if (status < 0) {
		dev_err(hw->dev,"ERROR:spi_transfer error(%d)!\n", status);
		sfc_disable_tx_intr(hw);
		sfc_disable_rx_intr(hw);
		spin_unlock_irqrestore(&hw->txrx_lock, flags);

		return status;
	}

#ifdef SSI_DEGUG
	sfc_dump_reg(hw,"bf wait irq");
#endif
	spin_unlock_irqrestore(&hw->txrx_lock, flags);

	/* wait the interrupt finish the transfer( one spi_transfer be sent ) */
	wait_for_completion_interruptible(&hw->done);

	if (t->cs_change) {
		print_dbg("start to stop all transfer,cs change\n");
		sfc_finish_transmit(hw);	//sfc中结束传输，是不是需要把片选拉高，这里应该是要做拉高的操作
	}

	sfc_clear_errors(hw);

	if (hw->rlen != t->len) {
		dev_info(hw->dev, "Length error:hw->rlen=%d  t->len=%d\n", hw->rlen,t->len);

		if(hw->rlen > hw->len)
			hw->rlen = hw->len;
	}

	return hw->rlen;
}

static irqreturn_t jz_spi_pio_irq_callback(struct jz_spi *hw)
{
	struct jz_intr_cnt *g_jz_intr = hw->g_jz_intr;
	long left_count = hw->len - hw->count;
	u8 flag = 0;
	u32 cnt;
	int status;

	print_dbg("get a irq,sfc_sr:%#x\n",spi_readl(hw, SFC_SR));

	// g_jz_intr->ssi_intr_cnt++;//暂时屏蔽，不要考虑那么多可能得情况，sfc和ssi不大一样

	if(sfc_ssi_end(hw) && sfc_get_txrxfifo_count(hw)){
		print_dbg("end irq,decision rlen\n");
		cnt = cpu_read_rxfifo(hw);
		print_dbg("-:(%d)\n",cnt);
		// sfc_clear_errors(hw);//放到transfer中做
		complete(&hw->done);
	}

	/* to avoid die in interrupt if some error occur */
	if (g_jz_intr->ssi_intr_cnt > g_jz_intr->max_ssi_intr) {
		sfc_disable_tx_intr(hw);
		sfc_disable_rx_intr(hw);
		dev_err(hw->dev,"ssi interrupts too many count(%d)!,maxcnt:%d\n",
			g_jz_intr->ssi_intr_cnt,g_jz_intr->max_ssi_intr);

		printk("SFC_SR                  :0x%08x\n", spi_readl(hw, SFC_SR));
		printk("SFC_SCR                 :0x%08x\n", spi_readl(hw, SFC_SCR));
		printk("SFC_INTC                :0x%08x\n", spi_readl(hw, SFC_INTC));
		complete(&hw->done);
		goto irq_done;
	}

	// if ( ssi_underrun(hw) && tx_error_intr(hw) )
	if ( sfc_ssi_underrun(hw) ) //读溢出错误
	{
		printk("UNDR:%#x\n",spi_readl(hw, SFC_SR));
		g_jz_intr->ssi_eti++;
		sfc_disable_tx_error_intr(hw);

		if(left_count == 0){
			cnt = cpu_read_rxfifo(hw);
			printk("-:(%d)\n",cnt);

			sfc_disable_tx_intr(hw);
			sfc_disable_rx_intr(hw);

			complete(&hw->done);
		} else {
			sfc_clear_errors(hw);
			sfc_enable_tx_error_intr(hw);
		}

		flag++;
	}

	// if ( ssi_overrun(hw) && rx_error_intr(hw) )
	if ( sfc_ssi_overrun(hw) ) //写溢出错误
	{
		print_dbg(" overrun:");
		// printk("overrun:\n");
		g_jz_intr->ssi_eri++;

		cnt = cpu_read_rxfifo(hw);
		print_dbg("-:(%d)\n",cnt);

		flag++;
	}

	// if ( rxfifo_half_full(hw) && rxfifo_half_full_intr(hw))
	if ( sfc_rxfifo_half_full(hw) ) //正常读请求
	{

		print_dbg("RXI:");
		if(hw->tx){
			print_dbg("this is tx cmd,return\n");
			hw->rlen = hw->len;
			sfc_disable_tx_intr(hw);
			sfc_disable_rx_intr(hw);
			complete(&hw->done);
			goto irq_done;
		}
		// printk("RXI:\n");
		g_jz_intr->ssi_rxi++;

		cnt = cpu_read_rxfifo(hw);
		print_dbg("after read-:(%d)\n",cnt);
		complete(&hw->done);
		sfc_disable_rx_intr(hw);
		goto irq_done;

		flag++;
	}

	// if ( txfifo_half_empty_intr(hw) && txfifo_half_empty(hw))
	if ( sfc_txfifo_half_empty(hw) )	//正常写请求
	{

		print_dbg("TXI:");
		// printk("TXI:");
		g_jz_intr->ssi_txi++;

		flag++;
		//对于sfc，如果不手动关闭中断标记的话，会一直产生中断的。所以需要手动关闭，为什么呢？问问IC
		// cnt = cpu_read_rxfifo(hw);//为什么还要读？因为外层接口有判断，需要hw->rlen==hw->len，那直接设置即可
		hw->rlen = hw->len;
		sfc_disable_tx_intr(hw);
		sfc_disable_rx_intr(hw);
		complete(&hw->done);
	}

	if (!flag) {
		// dev_info(hw->dev, "\nERROR:SSI interrupt Type error\n");
		// complete(&hw->done);
		dev_info(hw->dev, "\nnow irq over\n");
	}

irq_done:
	sfc_clear_errors(hw);
	return IRQ_HANDLED;
}

/* every spi_transfer could call this routine to setup itself */
static int jz_spi_setupxfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct jz_spi *hw = spi_master_get_devdata(spi->master);
	u8  bpw, fifo_width;
	u32 hz;

	bpw = spi->bits_per_word;
	hz  = spi->max_speed_hz;

	if (t) {
		if(t->bits_per_word)
			bpw = t->bits_per_word;
		if(t->speed_hz)
			hz = t->speed_hz;
	}

	if (bpw < 2 || bpw > 32) {
		dev_err(&spi->dev, "invalid bits-per-word (%d)\n", bpw);
		return -EINVAL;
	}

	hw->use_dma = 0;//wj 这里暂时先不用dma模式，没写好

	if (hw->use_dma) {
		printk("dma mode not support yet,return ....\n");
		return -1;

		hw->txrx_bufs = &jz_spi_dma_txrx;
		hw->irq_callback = &jz_spi_dma_irq_callback;
	} else {
		hw->txrx_bufs = &jz_spi_pio_txrx;
		hw->irq_callback = &jz_spi_pio_irq_callback;
	}
#if !USE_IC_CONFIG
	hw->bits_per_word = bpw;
	if (bpw <= 8) {
		hw->transfer_unit_size = SPI_8BITS;
		hw->get_rx = jz_spi_rx_buf_u8;
		hw->get_tx = jz_spi_tx_buf_u8;
		fifo_width = FIFO_W8;
	} else if (bpw <= 16) {
		hw->transfer_unit_size = SPI_16BITS;
		hw->get_rx = jz_spi_rx_buf_u16;
		hw->get_tx = jz_spi_tx_buf_u16;
		fifo_width = FIFO_W16;
	} else {
		hw->transfer_unit_size = SPI_32BITS;
		hw->get_rx = jz_spi_rx_buf_u32;
		hw->get_tx = jz_spi_tx_buf_u32;
		fifo_width = FIFO_W32;
	}

	hw->txfifo_width = fifo_width;
	hw->rxfifo_width = fifo_width;
	sfc_set_frame_length(hw, fifo_width);
	sfc_select_ce_high(hw);//这里临时做个测试，把ce设置成1,即高电平无效

	if (spi->mode & SPI_LSB_FIRST) {
		// set_tx_lsb(hw);
		// set_rx_lsb(hw);
		sfc_set_tx_lsb(hw);
		sfc_set_rx_lsb(hw);
	} else {
		// set_tx_msb(hw);
		// set_rx_msb(hw);
		sfc_set_tx_msb(hw);
		sfc_set_rx_msb(hw);
	}

	sfc_set_tx_msb(hw);
	sfc_set_rx_msb(hw);
#endif
	// sfc_set_tx_lsb(hw);//12.26test
	// sfc_set_rx_lsb(hw);
	// spi_writel(hw, SFC_TRAN_CONF1(0), ~(1<<18));

	// printk("[%s]\n",__FUNCTION__);
	jz_spi_set_clk(spi, hz);
	// dev_dbg(&spi->dev, "The real SPI CLK is %d Hz\n", jz_spi_get_clk(spi));//先屏蔽，因为改sfc时钟，和ssi这块无法一一对应

	spin_lock(&hw->bitbang.lock);
	if (!hw->bitbang.busy) {
		hw->bitbang.chipselect(spi, BITBANG_CS_INACTIVE);
		/* need to ndelay for 0.5 clocktick ? */
	}
	spin_unlock(&hw->bitbang.lock);

	return 0;
}

static int jz_spi_setup(struct spi_device *spi)
{
	struct jz_spi *hw = spi_master_get_devdata(spi->master);
	unsigned long flags;
	unsigned int frmhl = 0;

	spin_lock_irqsave(&hw->lock, flags);
	if (hw->state & SUSPND) {
		spin_unlock_irqrestore(&hw->lock, flags);
		dev_err(&spi->dev,
			"setup: SPI-%d not active!\n", spi->master->bus_num);
		return -ESHUTDOWN;
	}
	spin_unlock_irqrestore(&hw->lock, flags);

	if (spi->chip_select >= spi->master->num_chipselect) {
		dev_err(&spi->dev, "cs%d >= max %d\n",
			spi->chip_select,
			spi->master->num_chipselect);
		return -EINVAL;
	}

	//sfc种的要如何设置片选呢，ssi这里片选为0，和1，为什么有两个状态位表示的两种状态
	if (spi->chip_select == 0) {
		// select_ce(hw);
		// frmhl = spi_readl(hw, SSI_CR1);
		// frmhl &= ~(1<<30);
		// frmhl |= (spi->mode & SPI_CS_HIGH ? 1 : 0) << 30;
		// spi_writel(hw, SSI_CR1, frmhl);
		printk("cs is low\n");
		// sfc_select_ce_low(hw);	//默认CHIP_SEL为00，即接一个外设
	} else if (spi->chip_select == 1) {
		// select_ce2(hw);
		// frmhl = spi_readl(hw, SSI_CR1);
		// frmhl &= ~(1<<31);
		// frmhl |= (spi->mode & SPI_CS_HIGH ? 1 : 0) << 31;
		// spi_writel(hw, SSI_CR1, frmhl);
		printk("cs is low\n");
		sfc_select_ce_high(hw);
	} else
		return -EINVAL;

	printk("[%s]chipselect:%d\n",__FUNCTION__,spi->chip_select);

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	if (spi->mode & ~MODEBITS) {
		dev_info(&spi->dev, "Warning: unsupported mode bits %x\n",
			spi->mode & ~MODEBITS);
		return -EINVAL;
	}
	hw->spi_mode = spi->mode;

	if (spi->mode & SPI_LSB_FIRST) {
		sfc_set_tx_lsb(hw);
		sfc_set_rx_lsb(hw);
	} else {
		sfc_set_tx_msb(hw);
		sfc_set_rx_msb(hw);
	}

	if (spi->bits_per_word & ~SPI_BITS_SUPPORT) {
		dev_info(&spi->dev, "Warning: unsupported bits_per_word: %d\n",
			spi->bits_per_word);
		return -EINVAL;
	}

	if (!spi->max_speed_hz) {
		return -EINVAL;
	}

	if (hw->src_clk < spi->max_speed_hz) {
		dev_info(&spi->dev, "Warning:invalid clock(%d Hz) be set to source clk(%d Hz)!\n",
		spi->max_speed_hz,(uint)hw->src_clk);
		spi->max_speed_hz = hw->src_clk;
	}

	spin_lock(&hw->bitbang.lock);
	if (!hw->bitbang.busy) {
		hw->bitbang.chipselect(spi, BITBANG_CS_INACTIVE);
		/* need to ndelay for 0.5 clocktick ? */
	}
	spin_unlock(&hw->bitbang.lock);

	return 0;
}

/**
 * jz_spi_txrx - functions which will handle transfer data
 * @spi: spi device on which data transfer to be done
 * @t: spi transfer in which transfer info is filled
 *
 * This function will put data to be transferred into data register
 * of SPI controller and then wait until the completion will be marked
 * by the IRQ Handler.
 */
static int jz_spi_txrx(struct spi_device * spi, struct spi_transfer *t)
{
	struct jz_spi * hw = spi_master_get_devdata(spi->master);
	unsigned int	ret;
	unsigned long	flags;

	spin_lock_irqsave(&hw->lock, flags);
	if (hw->state & SUSPND) {
		hw->state &= ~SPIBUSY;
		spin_unlock_irqrestore(&hw->lock, flags);
		printk("Now enter suspend, so cann't tranfer data\n");
		return -ESHUTDOWN;
	}
	hw->state |= SPIBUSY;
	spin_unlock_irqrestore(&hw->lock, flags);

	ret = hw->txrx_bufs(spi, t);

	spin_lock_irqsave(&hw->lock, flags);

	hw->state &= ~SPIBUSY;
	spin_unlock_irqrestore(&hw->lock, flags);
	return ret;
}

static irqreturn_t jz_spi_irq(int irq, void *dev)
{
	struct jz_spi *hw = dev;

	return hw->irq_callback(hw);
}

static int jz_spi_sfc_init_setup(struct jz_spi *hw)
{
	unsigned int tmp, i;
	// tmp = jz_sfc_readl(SFC_GLB);
	tmp = spi_readl(hw, SFC_GLB0);
	// tmp &= ~(GLB_TRAN_DIR | GLB_OP_MODE); // TRAN_DIR 传输方向   OP_MODE DMA/PIO mode 表示从spiflash中读，slave模式
	tmp &= ~GLB_OP_MODE; // TRAN_DIR 传输方向   OP_MODE DMA/PIO mode 表示写数据到从机，slave模式

	tmp &= ~(1<<14 | 1<< 15);
	tmp &= ~GLB_WP_EN; // T40 modify
	// tmp |= GLB_WP_EN;
	// jz_sfc_writel(tmp, SFC_GLB);
	spi_writel(hw, SFC_GLB0, tmp);

	// tmp = jz_sfc_readl(SFC_DEV_CONF);
	tmp = spi_readl(hw, SFC_DEV_CONF);

	tmp &= ~(DEV_CONF_CMD_TYPE | DEV_CONF_CPHA | DEV_CONF_CPOL | DEV_CONF_SMP_DELAY_MSK |
				DEV_CONF_THOLD_MSK | DEV_CONF_TSETUP_MSK | DEV_CONF_TSH_MSK);
	/*T40 ADD STA_ENDIAN_MSB*/
	// tmp |= (DEV_CONF_CEDL | DEV_CONF_HOLDDL | DEV_CONF_WPDL | 1 << DEV_CONF_SMP_DELAY_OFFSET | STA_ENDIAN_MSB);
	tmp |= (DEV_CONF_CEDL | DEV_CONF_HOLDDL | DEV_CONF_WPDL /* | DEV_CONF_CPOL */);
	tmp |= (0 << 31) | (1 << 3) | (1 << 4); //bit31: 0:LSB 1:MSB
	#ifdef CONFIG_SPI_QUAD
	tmp |= DEV_CONF_SMP_DELAY_135 << 16; // T41 SFC reg function
	tmp |= 0x2 << 5;                     // T41 SFC reg function
	#endif
	// jz_sfc_writel(tmp, SFC_DEV_CONF);
	// tmp |= 1 << 15;
	spi_writel(hw, SFC_DEV_CONF, tmp);

	for (i = 0; i < 6; i++)
	{
		if(0 == i){
			// jz_sfc_writel(jz_sfc_readl(SFC_TRAN_CONF(i)) & (~(TRAN_CONF0_PHASE_FORMAT)), SFC_TRAN_CONF(i));
			// jz_sfc_writel(jz_sfc_readl(SFC_TRAN_CONF1(i)) & (~(TRAN_CONF1_TRAN_MODE_MSK)), SFC_TRAN_CONF1(i)); // tran mode
			// jz_sfc_writel(((1<<18) |(2 << 16) | (5<<4)), SFC_TRAN_CONF1(i));//MSB
			// jz_sfc_writel(((0<<18) |(2 << 16) | (5<<4)), SFC_TRAN_CONF1(i));    //LSB

			// 新的接口替换
			spi_writel(hw,SFC_TRAN_CONF(i),spi_readl(hw,SFC_TRAN_CONF(i)) & (~(TRAN_CONF0_PHASE_FORMAT)));
			// spi_writel(hw,SFC_TRAN_CONF1(i),spi_readl(hw,SFC_TRAN_CONF1(i)) & (~(TRAN_CONF1_TRAN_MODE_MSK))); // tran mode
			// spi_writel(hw,SFC_TRAN_CONF1(i),((1<<18) |(2 << 16) | (0<<4)));//MSB

		} else {
			// jz_sfc_writel(0, SFC_TRAN_CONF(i));
			// jz_sfc_writel(0, SFC_TRAN_CONF1(i)); // tran mode

			spi_writel(hw,SFC_TRAN_CONF(i),0);
			spi_writel(hw,SFC_TRAN_CONF1(i),0);

		}

	}

	// jz_sfc_writel(0, SFC_INTC);
	spi_writel(hw,SFC_INTC,0);

	//jz_sfc_writel(0, SFC_CGE);
	//jz_sfc_writel((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5), SFC_CGE);
	// jz_sfc_writel(0x3f, SFC_CGE);
	spi_writel(hw,SFC_CGE,0x3f);

	// tmp = jz_sfc_readl(SFC_GLB);
	tmp = spi_readl(hw,SFC_GLB0);
	tmp &= ~(GLB_THRESHOLD_MSK);

	// tmp |= 0x4 << 7;//4.22 默认设置4
	tmp &= ~(GLB_BURST_MD_MSK); // T40 ADD
	tmp |= (THRESHOLD << GLB_THRESHOLD_OFFSET);
	// jz_sfc_writel(tmp, SFC_GLB);
	spi_writel(hw,SFC_GLB0,tmp);

	// tmp = jz_sfc_readl(SFC_GLB1);
	tmp = spi_readl(hw,SFC_GLB1);
	tmp |= 1 << 3;
	// jz_sfc_writel(tmp, SFC_GLB1);
	spi_writel(hw,SFC_GLB1,tmp);

	return 0;
}

static int jz_spi_init_setup(struct jz_spi *hw)
{
	if (!clk_is_enabled(hw->clk_gate))
		clk_enable(hw->clk_gate);

	/* disable the SSI controller */
	ssi_disable(hw);

	/* set default half_intr trigger */
	hw->tx_trigger = SSI_TX_FIFO_THRESHOLD * 8;
	hw->rx_trigger = SSI_RX_FIFO_THRESHOLD * 8;
	set_tx_trigger(hw, hw->tx_trigger);
	set_rx_trigger(hw, hw->rx_trigger);

	/* First,mask the interrupt, while verify the status ? */
	disable_tx_intr(hw);
	disable_rx_intr(hw);

	disable_receive(hw);

	set_spi_clock_phase(hw, 0);
	set_spi_clock_polarity(hw, 0);
	set_tx_msb(hw);
	set_rx_msb(hw);

	set_spi_format(hw);
	set_frame_length(hw, 8);
	disable_loopback(hw);
	flush_fifo(hw);

	underrun_auto_clear(hw);
	clear_errors(hw);
	ssi_enable(hw);

	return 0;
}

static bool spi_dma_chan_filter(struct dma_chan *chan, void *param)
{
	struct jz_spi *hw = param;

	return hw->dma_type == (int)chan->private;
}

static int __init jz_spi_probe(struct platform_device *pdev)
{
	struct jz_spi *hw;
	struct spi_master *master;
	struct resource *res;
	dma_cap_mask_t mask;
	int err = 0;
	int num_cs_got = 0;
	char clkname[16];
	int i;

#ifdef CONFIG_JZ_SPI_BOARD_INFO_REGISTER
	struct spi_board_info *bi;
#endif

	master = spi_alloc_master(&pdev->dev, sizeof(struct jz_spi));
	if (master == NULL) {
		dev_err(&pdev->dev, "No memory for spi_master\n");
		err = -ENOMEM;
		goto err_nomem;
	}

	/* the spi->mode bits understood by this drivers: */
	master->mode_bits = MODEBITS;

	hw = spi_master_get_devdata(master);
	memset(hw, 0, sizeof(struct jz_spi));

	hw->g_jz_intr = kzalloc(sizeof(struct jz_intr_cnt),GFP_KERNEL);
	if(hw->g_jz_intr == NULL)
	{
		dev_err(&pdev->dev, "No memory for jz_intr_cnt\n");
		err = -ENOMEM;
		goto err_nomem;
	}

	hw->master = spi_master_get(master);
	hw->dev = &pdev->dev;

	hw->pdata = pdev->dev.platform_data;
	if (hw->pdata == NULL) {
		dev_err(&pdev->dev, "No platform data supplied\n");
		err = -ENOENT;
		goto err_no_pdata;
	}

	hw->chnl= hw->pdata->chnl;

	master->bus_num = (s16)hw->pdata->bus_num;
	if(master->bus_num != 0 && master->bus_num != 1){
		dev_err(&pdev->dev, "No this channel, bus_num= %d.\n", master->bus_num);
		err = -ENOENT;
		goto err_no_pdata;
	}

#ifdef CONFIG_JZ_SPI_PIO_CE
	for (i = 0; i < hw->pdata->num_chipselect; i++, num_cs_got = i) {
		err = gpio_request(hw->pdata->chipselect[i], "JZ_SPI_CS");
		if(err && (!hw->pdata->allow_cs_same)) {
			dev_err(&pdev->dev, "Request cs_gpio: %d is occupied\n",
							hw->pdata->chipselect[i]);
			goto err_cs_gpio;
		}
	}
#endif

	/* find and map our resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		err = -ENOENT;
		goto err_no_iores;
	}

	hw->ioarea = request_mem_region(res->start, resource_size(res),
					pdev->name);
	if (hw->ioarea == NULL) {
		dev_err(&pdev->dev, "Cannot reserve iomem region\n");
		err = -ENXIO;
		goto err_no_iores;
	}

	hw->phys = res->start;

	hw->iomem = ioremap(res->start, (res->end - res->start)+1);
	if (hw->iomem == NULL) {
		dev_err(&pdev->dev, "Cannot map IO\n");
		err = -ENXIO;
		goto err_no_iomap;
	}

	hw->irq = platform_get_irq(pdev, 0);
	if (hw->irq <= 0) {
		dev_err(&pdev->dev, "No IRQ specified\n");
		err = -ENOENT;
		goto err_no_irq;
	}

	printk("phy:%#lx,vir:%#lx,irq:%d\n",hw->phys,(unsigned long)hw->iomem,hw->irq);

	//又到了修改时钟的时间，对与这块我还没有搞透，现在应该要换成sfc的时钟
	hw->clk = clk_get(&pdev->dev, "div_sfc1");
	if (IS_ERR(hw->clk)) {
		dev_err(&pdev->dev, "Cannot get ssi clock\n");
		return PTR_ERR(hw->clk);
	}

	sprintf(clkname, "gate_sfc1");
	hw->clk_gate = clk_get(&pdev->dev, clkname);
	if (IS_ERR(hw->clk_gate)) {
		dev_err(&pdev->dev, "Cannot get ssi%d clock\n", pdev->id);
		return PTR_ERR(hw->clk);
	}

	hw->src_clk = hw->pdata->max_clk;

#if 0 //先屏蔽，sfc中暂不支持dma
	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (res) {
		hw->dma_type = res->start;
		print_dbg("dma_type: %d. %s LINE %d: %s\n", hw->dma_type, __func__, __LINE__, __FILE__);
		if (pdev->dev.dma_mask)
			hw->use_dma = 1;
	}
#endif

	platform_set_drvdata(pdev, hw);
	init_completion(&hw->done);
	init_completion(&hw->done_rx);
	init_completion(&hw->done_tx_dma);
	init_completion(&hw->done_rx_dma);
	spin_lock_init(&hw->lock);
	spin_lock_init(&hw->txrx_lock);

	/* setup the state for the bitbang driver */
	hw->bitbang.master         = hw->master;
	hw->bitbang.setup_transfer = jz_spi_setupxfer;
	hw->bitbang.chipselect     = jz_spi_chipsel;
	hw->bitbang.txrx_bufs      = jz_spi_txrx;
	hw->bitbang.master->setup  = jz_spi_setup;

	// printk("----------------default value start--------------\n");
	// sfc_dump_reg(hw);
	// printk("----------------default value end--------------\n");

	dev_dbg(hw->dev, "bitbang at %p\n", &hw->bitbang);

	if (hw->use_dma) {
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);

		hw->txchan = dma_request_channel(mask, spi_dma_chan_filter, hw);
		if(!hw->txchan) {
			dev_err(&pdev->dev, "SPI request dma tx channel failed");
			goto err_no_irq;
		}

		hw->rxchan = dma_request_channel(mask, spi_dma_chan_filter, hw);
		if(!hw->rxchan) {
			dev_err(&pdev->dev, "SPI request dma rx channel failed");
			goto free_txchan;
		}

		//alloc temp buffer for dma
		hw->buffer = dma_alloc_coherent(&pdev->dev, BUFFER_SIZE,
						&hw->buffer_dma, GFP_KERNEL);
		if (!hw->buffer) {
			dev_err(&pdev->dev, "SPI request temp dma buffer failed");
			goto free_rxchan;
		}

#if 0
		hw->buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!hw->buffer) {
			dev_err(&pdev->dev, "SPI request temp dma buffer failed");
			goto free_rxchan;
		}
		print_dbg("<< hw->buffer addr:%p >>\n", hw->buffer);
#endif

		hw->sg_tx = kmalloc(sizeof(struct scatterlist), GFP_KERNEL);
		if (!hw->sg_tx) {
			dev_err(&pdev->dev, "Failed to alloc tx scatterlist\n");
			goto err_tx_sgmap;
		}

		hw->sg_rx = kmalloc(sizeof(struct scatterlist), GFP_KERNEL);
		if(!hw->sg_rx) {
			dev_err(&pdev->dev, "Failed to alloc rx scatterlist\n");
			goto err_rx_sgmap;
		}
	}

	/* request SSI irq */
	err = request_irq(hw->irq, jz_spi_irq, 0, pdev->name, hw);
	if (err) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		goto err_no_irq;
	}

	/* get controller associated params */
	master->bus_num = hw->pdata->bus_num;
	master->num_chipselect = hw->pdata->num_chipselect;

	hw->fifodepth = JZ_SSI_MAX_FIFO_ENTRIES;

	/* setup chipselect */
	hw->set_cs = &jz_spi_cs;
	/* SSI controller initializations for SPI */
	// jz_spi_init_setup(hw);
#if !USE_IC_CONFIG
	jz_spi_sfc_init_setup(hw);
#endif
	/* register our spi controller */
	err = spi_bitbang_start(&hw->bitbang);
	if (err) {
		dev_err(&pdev->dev, "Failed to register SPI master ERR_NO:%d\n",err);
		goto err_register;
	}
#ifdef CONFIG_JZ_SPI_BOARD_INFO_REGISTER
	/* register all the devices associated */
	bi = &hw->pdata->board_info[0];
	if(bi){
		for (i = 0; i < hw->pdata->board_size; i++, bi++) {
			dev_info(hw->dev, "registering %s\n", bi->modalias);
			bi->controller_data = hw;
			spi_new_device(master, bi);
		}
	}
#endif

	printk(KERN_INFO
	       "JZ SSI Controller for SPI channel %d driver ,numchipselect:%d\n",hw->chnl,master->num_chipselect);

	return 0;

err_register:
	free_irq(hw->irq, hw);
free_rxchan:
	dma_release_channel(hw->rxchan);
free_txchan:
	dma_release_channel(hw->txchan);
err_no_irq:
	if (hw->sg_rx)
		kfree(hw->sg_rx);
err_rx_sgmap:
	if (hw->sg_tx)
		kfree(hw->sg_tx);
err_tx_sgmap:
	if (hw->buffer)
		kfree(hw->buffer);
	iounmap(hw->iomem);
	clk_put(hw->clk_gate);
	clk_put(hw->clk);

err_no_iomap:
	release_resource(hw->ioarea);
	kfree(hw->ioarea);

err_no_iores:

#ifdef CONFIG_JZ_SPI_PIO_CE
err_cs_gpio:
	for (i = 0; i < num_cs_got; i++)
		gpio_free(hw->pdata->chipselect[i]);
#endif
err_no_pdata:
	spi_master_put(hw->master);;

err_nomem:
	return err;
}

static int __exit jz_spi_remove(struct platform_device *dev)
{
	struct jz_spi *hw = platform_get_drvdata(dev);
	int i;

	spi_master_put(hw->master);
	spi_bitbang_stop(&hw->bitbang);

	platform_set_drvdata(dev, NULL);

	free_irq(hw->irq, hw);
	iounmap(hw->iomem);

	if(clk_is_enabled(hw->clk_gate))
		clk_disable(hw->clk_gate);

	if(clk_is_enabled(hw->clk))
		clk_disable(hw->clk);

	clk_put(hw->clk_gate);
	clk_put(hw->clk);

	release_resource(hw->ioarea);
	kfree(hw->ioarea);

	/* release DMA channel */
	if (hw->rxchan) {
		dma_release_channel(hw->rxchan);
		printk("dma_rx_chnl release\n");
	}
	if (hw->txchan) {
		dma_release_channel(hw->txchan);
		printk("dma_tx_chnl release\n");
	}

#ifdef CONFIG_JZ_SPI_PIO_CE
	/* release chipselect gpio */
	for (i = 0; i < hw->pdata->num_chipselect; i++)
		gpio_free(hw->pdata->chipselect[i]);
#endif

	kfree(hw->g_jz_intr);
	kfree(hw);
	printk(KERN_INFO
	       "JZ SSI Controller for SPI channel %d driver removed\n",hw->chnl);

	return 0;
}

#ifdef CONFIG_PM
static int jz_spi_suspend(struct platform_device *pdev, pm_message_t msg)
{
	struct jz_spi *hw = platform_get_drvdata(pdev);
	unsigned long flags;

	spin_lock_irqsave(&hw->lock, flags);
	hw->state |= SUSPND;
	spin_unlock_irqrestore(&hw->lock, flags);

	while (hw->state & SPIBUSY)
		printk("Now spi is busy, waitting!\n");

	if(clk_is_enabled(hw->clk_gate)) {
		hw->clk_gate_flag = 1;
		clk_disable(hw->clk_gate);
	} else {
		hw->clk_gate_flag = 0;
	}

	if(clk_is_enabled(hw->clk)) {
		hw->clk_flag = 1;
		clk_disable(hw->clk);
	} else {
		hw->clk_flag = 0;
	}

	return 0;
}

static int jz_spi_resume(struct platform_device *pdev)
{
	struct jz_spi *hw = platform_get_drvdata(pdev);
	unsigned long	flags;

	if(hw->clk_flag == 1) {
		clk_enable(hw->clk);
	}
	if(hw->clk_gate_flag == 1) {
		clk_enable(hw->clk_gate);
	}

	spin_lock_irqsave(&hw->lock, flags);
	hw->state &= ~SUSPND;
	spin_unlock_irqrestore(&hw->lock, flags);

	return 0;
}

#else
#define jz_spi_suspend NULL
#define jz_spi_resume  NULL
#endif

MODULE_ALIAS("jz_ssi");			/* for platform bus hotplug */
static struct platform_driver jz_spidrv = {
	.remove		= __exit_p(jz_spi_remove),
	.suspend	= jz_spi_suspend,
	.resume		= jz_spi_resume,
	.driver		= {
		.name	= "jz-ssi",
		.owner	= THIS_MODULE,
	},
};

static int __init jz_spi_init(void)
{
		printk("jz_sfc_spi init\n");
        return platform_driver_probe(&jz_spidrv, jz_spi_probe);
}

static void __exit jz_spi_exit(void)
{
        platform_driver_unregister(&jz_spidrv);
		printk(KERN_INFO "JZ SSI Controller Module EXIT\n");

}

module_init(jz_spi_init);
module_exit(jz_spi_exit);

MODULE_DESCRIPTION("JZ SPI Driver");
MODULE_LICENSE("GPL");
