/* drivers/i2c/busses/i2c-v12-jz.c
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *      http://www.ingenic.com
 *      Sun Jiwei<jwsun@ingenic.cn>
 *
 * I2C adapter driver for the Ingenic I2C controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <asm/errno.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <linux/export.h>
#include <linux/gfp.h>
#include <linux/printk.h>
#include <linux/spinlock_types.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>
#include <linux/dma-direction.h>
#include <linux/types.h>
#include <mach/jzdma.h>
#include <mach/jz_i2cdma.h>
#include <linux/dmaengine.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define	I2C_CTRL		(0x00)
#define	I2C_TAR     		(0x04)
#define	I2C_SAR     		(0x08)
#define	I2C_DC      		(0x10)
#define	I2C_SHCNT		(0x14)
#define	I2C_SLCNT		(0x18)
#define	I2C_FHCNT		(0x1C)
#define	I2C_FLCNT		(0x20)
#define	I2C_INTST		(0x2C)
#define	I2C_INTM		(0x30)
#define I2C_RXTL		(0x38)
#define I2C_TXTL		(0x3c)
#define	I2C_CINTR		(0x40)
#define	I2C_CRXUF		(0x44)
#define	I2C_CRXOF		(0x48)
#define	I2C_CTXOF		(0x4C)
#define	I2C_CRXREQ		(0x50)
#define	I2C_CTXABRT		(0x54)
#define	I2C_CRXDONE		(0x58)
#define	I2C_CACT		(0x5C)
#define	I2C_CSTP		(0x60)
#define	I2C_CSTT		(0x64)
#define	I2C_CGC    		(0x68)
#define	I2C_ENB     		(0x6C)
#define	I2C_STA     		(0x70)
#define I2C_TXFLR		(0x74)
#define I2C_RXFLR		(0x78)
#define I2C_SDAHD		(0x7C)
#define	I2C_TXABRT		(0x80)
#define I2C_DMACR            	(0x88)
#define I2C_DMATDLR          	(0x8c)
#define I2C_DMARDLR     	(0x90)
#define	I2C_SDASU		(0x94)
#define	I2C_ACKGC		(0x98)
#define	I2C_ENSTA		(0x9C)
#define I2C_FLT			(0xA0)

/* I2C Control Register (I2C_CTRL) */
#define I2C_CTRL_SLVDIS		(1 << 6)	/* after reset slave is disabled */
#define I2C_CTRL_REST		(1 << 5)
#define I2C_CTRL_MATP		(1 << 4)	/* 1: 10bit address 0: 7bit addressing */
#define I2C_CTRL_SATP		(1 << 3)	/* 1: 10bit address 0: 7bit address */
#define I2C_CTRL_SPDF		(2 << 1)	/* fast mode 400kbps */
#define I2C_CTRL_SPDS		(1 << 1)	/* standard mode 100kbps */
#define I2C_CTRL_MD		(1 << 0)	/* master enabled */

/* I2C Status Register (I2C_STA) */
#define I2C_STA_SLVACT		(1 << 6)	/* Slave FSM is not in IDLE state */
#define I2C_STA_MSTACT		(1 << 5)	/* Master FSM is not in IDLE state */
#define I2C_STA_RFF		(1 << 4)	/* RFIFO if full */
#define I2C_STA_RFNE		(1 << 3)	/* RFIFO is not empty */
#define I2C_STA_TFE		(1 << 2)	/* TFIFO is empty */
#define I2C_STA_TFNF		(1 << 1)	/* TFIFO is not full  */
#define I2C_STA_ACT		(1 << 0)	/* I2C Activity Status */

/* i2c interrupt status (I2C_INTST) */
#define I2C_INTST_IGC           (1 << 11)
#define I2C_INTST_ISTT          (1 << 10)
#define I2C_INTST_ISTP          (1 << 9)
#define I2C_INTST_IACT          (1 << 8)
#define I2C_INTST_RXDN          (1 << 7)
#define I2C_INTST_TXABT         (1 << 6)
#define I2C_INTST_RDREQ         (1 << 5)
#define I2C_INTST_TXEMP         (1 << 4)
#define I2C_INTST_TXOF          (1 << 3)
#define I2C_INTST_RXFL          (1 << 2)
#define I2C_INTST_RXOF          (1 << 1)
#define I2C_INTST_RXUF          (1 << 0)

/* i2c interrupt mask status (I2C_INTM) */
#define I2C_INTM_MIGC		(1 << 11)
#define I2C_INTM_MISTT		(1 << 10)
#define I2C_INTM_MISTP		(1 << 9)
#define I2C_INTM_MIACT		(1 << 8)
#define I2C_INTM_MRXDN		(1 << 7)
#define I2C_INTM_MTXABT		(1 << 6)
#define I2C_INTM_MRDREQ		(1 << 5)
#define I2C_INTM_MTXEMP		(1 << 4)
#define I2C_INTM_MTXOF		(1 << 3)
#define I2C_INTM_MRXFL		(1 << 2)
#define I2C_INTM_MRXOF		(1 << 1)
#define I2C_INTM_MRXUF		(1 << 0)

#define I2C_DC_REST		(1 << 10)
#define I2C_DC_STP		(1 << 9)
#define I2C_DC_READ    		(1 << 8)

#define I2C_ENB_I2CENB		(1 << 0)	/* Enable the i2c */

#define I2C_FIFO_LEN		(CONFIG_I2C_FIFO_LEN)
#define TX_LEVEL		(I2C_FIFO_LEN / 2)
#define RX_LEVEL		(I2C_FIFO_LEN / 2 - 1)
#define TIMEOUT			0xff
#define DEBUG_INFO		2
#define DEBUG_WARN		1

/*
 *  msg_end_type: The bus control which need to be send at end of transfer.
 *  @MSG_END_STOP: Send stop pulse at end of transfer.
 *  @MSG_END_REPEAT_START: Send repeat start at end of transfer.
 */
enum msg_end_type {
	MSG_END_STOP,
	MSG_END_CONTINUE,
	MSG_END_REPEAT_START,
};

/* I2C Transmit Abort Status Register (I2C_TXABRT) */
static const char *abrt_src[] = {
	"I2C_TXABRT_ABRT_7B_ADDR_NOACK",
	"I2C_TXABRT_ABRT_10ADDR1_NOACK",
	"I2C_TXABRT_ABRT_10ADDR2_NOACK",
	"I2C_TXABRT_ABRT_XDATA_NOACK",
	"I2C_TXABRT_ABRT_GCALL_NOACK",
	"I2C_TXABRT_ABRT_GCALL_READ",
	"I2C_TXABRT_ABRT_HS_ACKD",
	"I2C_TXABRT_SBYTE_ACKDET",
	"I2C_TXABRT_ABRT_HS_NORSTRT",
	"I2C_TXABRT_SBYTE_NORSTRT",
	"I2C_TXABRT_ABRT_10B_RD_NORSTRT",
	"I2C_TXABRT_ABRT_MASTER_DIS",
	"I2C_TXABRT_ARB_LOST",
	"I2C_TXABRT_SLVFLUSH_TXFIFO",
	"I2C_TXABRT_SLV_ARBLOST",
	"I2C_TXABRT_SLVRD_INTX",
};

#undef DMADBG
// #define DMADBG
#ifdef DMADBG
#define DMAC_BASE        0xb3420000 /*!< PDMA register base. */
#define IRQ_INTC0_PDMA   (10)

#define DMAC_DSAR(n)	(0x00 + (n) * 0x20) /* DMA source address */
#define DMAC_DTAR(n)	(0x04 + (n) * 0x20) /* DMA target address */
#define DMAC_DTCR(n)	(0x08 + (n) * 0x20) /* DMA transfer count */
#define DMAC_DRSR(n)	(0x0c + (n) * 0x20) /* DMA request source */
#define DMAC_DCCSR(n)	(0x10 + (n) * 0x20) /* DMA control/status */
#define DMAC_DCMD(n)	(0x14 + (n) * 0x20) /* DMA command */
#define DMAC_DDA(n)		(0x18 + (n) * 0x20) /* DMA descriptor address */
#define DMAC_DSD(n)		(0x1c + (n) * 0x20)

#define DMAC	0x1000
#define DIRQP	0x1004
#define DDR		0x1008
#define DDRS	0x100C
#endif

/* I2C standard mode high count register(I2CSHCNT) */
#define I2CSHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C standard mode low count register(I2CSLCNT) */
#define I2CSLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))
/* I2C fast mode high count register(I2CFHCNT) */
#define I2CFHCNT_ADJUST(n)      (((n) - 8) < 6 ? 6 : ((n) - 8))
/* I2C fast mode low count register(I2CFLCNT) */
#define I2CFLCNT_ADJUST(n)      (((n) - 1) < 8 ? 8 : ((n) - 1))

#define I2C_CHAIN_NUM_MAX (32)

struct i2c_jz;

struct i2c_dma_node {
	struct i2c_jz *i2c;
	struct list_head entry;
	uint16_t index;
	uint32_t work_mode;
	// i2c_dma_param_t i2cdma_param;
	void (*i2c_dma_callback)(void *arg,int ret);
	void *param;
	unsigned long slaver_addr;
	unsigned long dma_phy;
	int trans_len;	//real trans len
	int real_Len;	//alloc/free mem
	struct dma_chan *chan;
	struct dma_async_tx_descriptor *desc_tx;
	struct dma_async_tx_descriptor *desc_rx;
	struct dma_slave_config dma_config_tx;
	struct dma_slave_config dma_config_rx;
};

struct i2c_jz {
	void __iomem *iomem;
	int irq;
	struct clk *clk;
	struct i2c_adapter adap;

	enum msg_end_type w_end_type;
	enum msg_end_type r_end_type;
	unsigned char *rbuf;
	unsigned char *wbuf;
	unsigned int rd_len;
	int len;

#define DMAI2CBUFSIZE (4*1024)
	u32 dma_type_tx;
	u32 dma_type_rx;
	dma_addr_t base_phy;
	void *dma_vir;
	dma_addr_t dma_phy;
	struct dma_chan *chan;
	struct dma_async_tx_descriptor *desc_tx;
	struct dma_async_tx_descriptor *desc_rx;
	struct dma_slave_config dma_config_tx;
	struct dma_slave_config dma_config_rx;
	struct completion dma_tx_done_comp;
	struct completion dma_rx_done_comp;
	struct completion complete;
	struct completion notify_pio_comp;
	struct list_head buf_list;
	struct list_head queues_list;
	uint32_t pio_wait;
	uint32_t isdma_mode;
	uint32_t i2c_busy_status;
	spinlock_t slock;

	int32_t free_offset;
	int32_t busy_offset;

	int state;
	int debug;
	unsigned int rate;
	unsigned int speed;
};

static int i2c_dma_commit(struct i2c_jz *i2c);
static int i2c_irq_dmawrite_done(struct i2c_jz *i2c,int ret);
void i2cdma_free(struct i2c_jz *i2c,size_t len);

#define I2C_DEBUG
static inline unsigned short i2c_readl(struct i2c_jz *i2c,
		unsigned short offset);
#ifdef I2C_DEBUG

#define PRINT_REG_WITH_ID(reg_name, id)					\
	dev_info(&(i2c->adap.dev),"--"#reg_name "    	0x%08x\n",i2c_readl(id,reg_name))

static void i2c_jz_dump_regs(struct i2c_jz *i2c)
{
	struct i2c_jz *i2c_id = i2c;

	// PRINT_REG_WITH_ID(I2C_CTRL, i2c_id);
	// PRINT_REG_WITH_ID(I2C_INTM, i2c_id);
	// PRINT_REG_WITH_ID(I2C_RXTL, i2c_id);
	// PRINT_REG_WITH_ID(I2C_TXTL, i2c_id);
	// PRINT_REG_WITH_ID(0x78, i2c_id);
	// return;

	PRINT_REG_WITH_ID(I2C_CTRL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TAR, i2c_id);
	PRINT_REG_WITH_ID(I2C_SAR, i2c_id);
	//      PRINT_REG_WITH_ID(I2C_DC, i2c_id);
	PRINT_REG_WITH_ID(I2C_SHCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_SLCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_FHCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_FLCNT, i2c_id);
	PRINT_REG_WITH_ID(I2C_INTST, i2c_id);
	PRINT_REG_WITH_ID(I2C_INTM, i2c_id);
	PRINT_REG_WITH_ID(I2C_RXTL, i2c_id);
	PRINT_REG_WITH_ID(I2C_TXTL, i2c_id);
	PRINT_REG_WITH_ID(I2C_CINTR, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXUF, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXOF, i2c_id);
	PRINT_REG_WITH_ID(I2C_CTXOF, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXREQ, i2c_id);
	PRINT_REG_WITH_ID(I2C_CTXABRT, i2c_id);
	PRINT_REG_WITH_ID(I2C_CRXDONE, i2c_id);
	PRINT_REG_WITH_ID(I2C_CACT, i2c_id);
	PRINT_REG_WITH_ID(I2C_CSTP, i2c_id);
	PRINT_REG_WITH_ID(I2C_CSTT, i2c_id);
	PRINT_REG_WITH_ID(I2C_CGC, i2c_id);
	PRINT_REG_WITH_ID(I2C_ENB, i2c_id);
	PRINT_REG_WITH_ID(I2C_STA, i2c_id);
	/*debug trans & recive fifo count */
	PRINT_REG_WITH_ID(0x74, i2c_id);
	PRINT_REG_WITH_ID(0x78, i2c_id);

	PRINT_REG_WITH_ID(I2C_TXABRT, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMACR, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMATDLR, i2c_id);
	PRINT_REG_WITH_ID(I2C_DMARDLR, i2c_id);
	PRINT_REG_WITH_ID(I2C_SDASU, i2c_id);
	PRINT_REG_WITH_ID(I2C_ACKGC, i2c_id);
	PRINT_REG_WITH_ID(I2C_ENSTA, i2c_id);
	PRINT_REG_WITH_ID(I2C_SDAHD, i2c_id);
}
#endif

static inline unsigned short i2c_readl(struct i2c_jz *i2c,
		unsigned short offset)
{
	return readl(i2c->iomem + offset);
}

static inline void i2c_writel(struct i2c_jz *i2c, unsigned short offset,
		unsigned short value)
{
	writel(value, i2c->iomem + offset);
}

static int i2c_jz_disable(struct i2c_jz *i2c)
{
	int timeout = TIMEOUT;
	i2c_writel(i2c, I2C_ENB, 0);
	while ((i2c_readl(i2c, I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))
		msleep(1);

	if (timeout)
		return 0;

	printk("enable i2c%d failed\n", i2c->adap.nr);
	return -ETIMEDOUT;
}

static int i2c_jz_enable(struct i2c_jz *i2c)
{
	int timeout = TIMEOUT;

	i2c_writel(i2c, I2C_ENB, 1);
	while (!(i2c_readl(i2c, I2C_ENSTA) & I2C_ENB_I2CENB) && (--timeout > 0))
		msleep(1);

	if (timeout)
		return 0;

	printk("enable i2c%d failed\n", i2c->adap.nr);
	return -ETIMEDOUT;
}

static void i2c_jz_reset(struct i2c_jz *i2c)
{
	i2c_readl(i2c, I2C_CTXABRT);
	i2c_readl(i2c, I2C_INTST);

	i2c_jz_disable(i2c);
	udelay(10);
	i2c_jz_enable(i2c);
}

/* function: send read command
 * return:	0, successful
 *		1, txfifo valid entry is more than receive fifo, before send read command,
 *			must be read.
 *		2, txfifo count is 0 or rxfifo count is 0.
 * */
static inline unsigned int i2c_send_rcmd(struct i2c_jz *i2c)
{
	unsigned int tx_count, rx_count, count, tx_valid, rx_valid;

	tx_valid = i2c_readl(i2c, I2C_TXFLR);
	rx_valid = i2c_readl(i2c, I2C_RXFLR);
	tx_count = I2C_FIFO_LEN - tx_valid;
	rx_count = I2C_FIFO_LEN - rx_valid;

#ifdef CONFIG_I2C_DEBUG_INFO
	if(i2c->debug > DEBUG_INFO)
		dev_info(&(i2c->adap.dev),
				"%s, tx_valid = %d, rx_valid = %d,"
				" tx_count = %d, rx_count = %d\n",
				__func__, tx_valid, rx_valid, tx_count, rx_count);
#endif

	if (tx_valid > rx_count) {
		dev_warn(&(i2c->adap.dev),
				"\n\n###Warrning: I2C transfer fifo valid entry is more receive fifo, "
				"before send read cmd, please read data from "
				"the read fifo.\n\n");
		return 1;
	}

	if (!tx_count || !rx_count) {
		dev_warn(&(i2c->adap.dev),
				"\n\n###Warrning: I2C receive fifo or transfer fifo is full, "
				"before send read cmd, please read data from "
				"the read fifo or wait some time.\n\n");
		return 2;
	}

	count = min3(i2c->rd_len, tx_count, rx_count);

#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_INFO)
		dev_info(&(i2c->adap.dev),
				"%s, Before send read cmd, "
				"need_send = %d, left_send = %d\n",
				__func__, count ,i2c->rd_len);
#endif

	i2c->rd_len -= count;

	if (!i2c->rd_len) {
		while (count > 1) {
			i2c_writel(i2c, I2C_DC, I2C_DC_READ);
			count--;
		}
		if (i2c->r_end_type == MSG_END_STOP) {
			i2c_writel(i2c, I2C_DC, I2C_DC_READ | I2C_DC_STP);
		} else {
			i2c_writel(i2c, I2C_DC, I2C_DC_READ);
		}
	} else {
		while (count > 0) {
			i2c_writel(i2c, I2C_DC, I2C_DC_READ);
			count--;
		}
	}

#ifdef CONFIG_I2C_DEBUG_INFO
	if(i2c->debug > DEBUG_INFO)
		dev_info(&(i2c->adap.dev),
				"%s, After send read cmd, "
				"left_send = %d\n",
				__func__, i2c->rd_len);
#endif
	return 0;
}

#ifdef CONFIG_I2C_DEBUG_INFO
static ssize_t enable_debug(struct device *dev, struct device_attribute *attr,
		                        const char *buf, size_t size)
{
	int rc;
	unsigned long   enable;
	struct i2c_jz *i2c = (struct i2c_jz *)dev_get_drvdata(dev);

	rc = strict_strtoul(buf, 0, &enable);
	if (rc)
		return rc;

	if ((enable >= 0) && (enable <= DEBUG_INFO + 1))
		i2c->debug = enable;
	else
		goto err;

	return size;
err:
	pr_err("Please input correct number(enable >= 0 && enable <= 5)"
			" to disable or enable debug info print\n");
	return -EAGAIN;
}

static struct device_attribute attributes[] = {
	__ATTR(debug_info, 0666, NULL, enable_debug),
};

static int create_debug_sysfs_interface(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto err;
	return 0;

err:
	for( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	return -1;
}
#endif

int clear_irq[] = {
	I2C_CRXUF,
	I2C_CRXOF,
	0,
	I2C_CTXOF,
	0,
	I2C_CRXREQ,
	0,//I2C_CTXABRT,
	I2C_CRXDONE,
	I2C_CACT,
	I2C_CSTP,
	I2C_CSTT,
	I2C_CGC,
};

static irqreturn_t i2c_jz_irq(int irqno, void *dev_id)
{
	int ret = 0;
	int i = 0;
	unsigned short tmp, intst, intmsk;
	struct i2c_jz *i2c = dev_id;

	intst = i2c_readl(i2c, I2C_INTST);
	intmsk = i2c_readl(i2c, I2C_INTM);

	for(i = 0; i < 12;i++){
		if(intst & (0x1 << i)){
			i2c_readl(i2c, clear_irq[i]);
			// printk("i:%d,irq:%#x\n",i,clear_irq[i]);
		}
	}
	// printk("intst:%#x,intmsk:%#x\n",intst,intmsk);
#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_INFO)
		dev_info(&(i2c->adap.dev), "--I2C irq register INTST:0x%08x\n", intst);
#endif

	if ((intst & I2C_INTST_TXABT) && (intmsk & I2C_INTM_MTXABT)) {
		dev_err(&(i2c->adap.dev),
				"%s %d, I2C transfer error, ABORT interrupt\n",
				__func__, __LINE__);

		if(i2c->isdma_mode){
			ret = -EINIT;
			// ret = i2c_irq_dmawrite_done(i2c,ret);
		}else{
			printk("%s[%d]: \n",__func__,__LINE__);
		}
		goto END_TRSF_IRQ_HND;
	}

	if ((intst & I2C_INTST_ISTP) && (intmsk & I2C_INTM_MISTP)) {
		// i2c_readl(i2c, I2C_CSTP);	/* clear STP bit */
#ifdef CONFIG_I2C_DEBUG_INFO
		if (i2c->debug > DEBUG_INFO) {
			dev_info(&(i2c->adap.dev),
					"%s, Now stop condition has occurred,"
					"and left data length is %d\n",
					__func__, i2c->len);
		}
#endif

		if (i2c->len == 0)
			goto END_TRSF_IRQ_HND;
	}

	if ((intmsk & I2C_INTM_MTXEMP) && (intst & I2C_INTST_TXEMP)) {
		if(i2c->isdma_mode){
			ret = 0;
			// ret = i2c_irq_dmawrite_done(i2c,0);
			goto END_TRSF_IRQ_HND;
		} else {
			if (!i2c->len) {
				if (i2c->w_end_type == MSG_END_REPEAT_START) {
					goto END_TRSF_IRQ_HND;
				} else {
					tmp = i2c_readl(i2c, I2C_INTM);
					tmp &= ~I2C_INTM_MTXEMP;
					i2c_writel(i2c, I2C_INTM, tmp);
				}
			} else {
				while ((i2c->len > 0) &&
						(i2c_readl(i2c, I2C_STA) & I2C_STA_TFNF)) {
					tmp = *i2c->wbuf++;
					if (i2c->len == 1) {
						if (i2c->w_end_type == MSG_END_STOP)
							tmp |= I2C_DC_STP;
					}

					i2c_writel(i2c, I2C_DC, tmp);
					i2c->len -= 1;
				}

				if (i2c->len == 0) {
					i2c_writel(i2c, I2C_TXTL, 0);
				}
			}
		}
	}

	if ((intst & I2C_INTST_RXFL) && (intmsk & I2C_INTM_MRXFL)) {
#ifdef CONFIG_I2C_DEBUG_INFO
		if (i2c->debug >= DEBUG_INFO) {
			dev_info(&(i2c->adap.dev),
					"%s, Before read I2C_DC, "
					"left_send_cmd = %d, left_read_cnt = %d,"
					" rx_valid = %d, tx_valid = %d\n",
					__func__, i2c->rd_len, i2c->len,
					i2c_readl(i2c, I2C_RXFLR), i2c_readl(i2c, I2C_TXFLR));
		}
#endif

		while ((i2c_readl(i2c, I2C_STA) & I2C_STA_RFNE) &&
				(i2c->len > 0)) {
			tmp = i2c_readl(i2c, I2C_DC) & 0xff;
			*i2c->rbuf++ = tmp;
			i2c->len--;
		}

#ifdef CONFIG_I2C_DEBUG_INFO
		if (i2c->debug >= DEBUG_INFO)
			dev_info(&(i2c->adap.dev),
					"%s, After read I2C_DC, "
					"left_read_cnt = %d,"
					" rx_valid = %d, tx_valid = %d\n",
					__func__, i2c->len,
					i2c_readl(i2c, I2C_RXFLR), i2c_readl(i2c, I2C_TXFLR));
#endif

		if (i2c->len == 0) {
			goto END_RECE_IRQ_HND;
		}

		if (i2c->len <= I2C_FIFO_LEN) {
			i2c_writel(i2c, I2C_RXTL, i2c->len - 1);
		}

		if (i2c_send_rcmd(i2c)) {
			dev_err(&(i2c->adap.dev),
					"%s %d, I2C controller has BUG,"
					" RXFLR or TXFLR can not clear\n",
					__func__, __LINE__);
			BUG();
		}

#ifdef CONFIG_I2C_DEBUG_INFO
		if (i2c->debug > DEBUG_INFO)
			dev_info(&(i2c->adap.dev),
					"%s, After send read command, "
					"left_send_cmd = %d, "
					"left_read_cnt = %d\n",
					__func__, i2c->rd_len, i2c->len);
#endif
	}

	if ((intst & I2C_INTST_RXOF) && (intmsk & I2C_INTM_MRXOF)) {
		dev_err(&(i2c->adap.dev),
				"%s %d, I2C transfer error, RXFIFO over full\n",
				__func__, __LINE__);
		// i2c_readl(i2c, I2C_CRXOF);	/* clear RXOF bit */
	}

	if ((intst & I2C_INTST_TXOF) && (intmsk & I2C_INTM_MTXOF)) {
		dev_err(&(i2c->adap.dev),
				"%s %d, I2C transfer error, TXFIFO over full\n",
				__func__, __LINE__);
		// i2c_readl(i2c, I2C_CTXOF);	/* clear TXOF bit */
		goto END_TRSF_IRQ_HND;
	}

	return IRQ_HANDLED;
END_RECE_IRQ_HND:
END_TRSF_IRQ_HND:

	i2c_writel(i2c, I2C_INTM, 0);
	if(i2c->isdma_mode){
//		i2c_writel(i2c, I2C_INTM, I2C_INTM_MTXABT);
		ret = i2c_irq_dmawrite_done(i2c,ret);
	}else {
//		i2c_writel(i2c, I2C_INTM, 0);
		complete(&i2c->complete);
	}
	return IRQ_HANDLED;
}

static void txabrt(struct i2c_jz *i2c, int src)
{
	int i;

	dev_err(&(i2c->adap.dev), "--I2C txabrt:\n");
	for (i = 0; i < 16; i++) {
		if (src & (0x1 << i))
			dev_info(&(i2c->adap.dev), "--I2C TXABRT[%d]=%s\n", i,
					abrt_src[i]);
	}
}

static inline int xfer_read(struct i2c_jz *i2c, unsigned char *buf, int len,
		enum msg_end_type end_type)
{
	int ret = 0;
	long timeout;
	unsigned short tmp;
	unsigned int wait_complete_timeout_ms;

	wait_complete_timeout_ms =
		len * 1000 * 9 * 2 / i2c->rate + CONFIG_I2C_JZV12_WAIT_MS;

#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_WARN)
		dev_info(&(i2c->adap.dev),
				"%s, Begin read msg, want to read length is %d\n",
				__func__, len);

    memset(buf, 0, len);
#endif

	i2c->rd_len = len;
	i2c->len = len;
	i2c->rbuf = buf;
	i2c->r_end_type = end_type;

	i2c_readl(i2c, I2C_CSTP);	/* clear STP bit */
	i2c_readl(i2c, I2C_CTXOF);	/* clear TXOF bit */
	i2c_readl(i2c, I2C_CTXABRT);	/* clear TXABRT bit */

	if (len <= I2C_FIFO_LEN) {
		i2c_writel(i2c, I2C_RXTL, len - 1);
	} else {
		i2c_writel(i2c, I2C_RXTL, RX_LEVEL);
	}

	while (i2c_readl(i2c, I2C_STA) & I2C_STA_RFNE) {
		i2c_readl(i2c, I2C_DC);
	}
	if (i2c_send_rcmd(i2c))
		BUG();

	tmp = I2C_INTM_MRXFL | I2C_INTM_MTXABT;
	if (end_type == MSG_END_STOP)
		tmp |= I2C_INTM_MISTP;
	i2c_writel(i2c, I2C_INTM, tmp);

	timeout = wait_for_completion_timeout(&i2c->complete,
			msecs_to_jiffies
			(wait_complete_timeout_ms));
	if (!timeout) {
		dev_err(&(i2c->adap.dev), "--I2C irq read timeout\n");
#ifdef I2C_DEBUG
		i2c_jz_dump_regs(i2c);
#endif
		ret = -ETIMEDOUT;
	}

	tmp = i2c_readl(i2c, I2C_TXABRT);
	if (tmp) {
		txabrt(i2c, tmp);
		if (tmp > 0x1 && tmp < 0x10)
			ret = -ENXIO;
		else
			ret = -EIO;
		// ABRT_GCALL_READ
		if (tmp & (1 << 5)) {
			ret = -EAGAIN;
		}
		i2c_readl(i2c, I2C_CTXABRT);
	}
	if (ret < 0)
		i2c_jz_reset(i2c);

#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_WARN)
		dev_info(&(i2c->adap.dev),
				"%s, Reading msg over\n", __func__);
#endif

	return ret;
}

static inline int xfer_write(struct i2c_jz *i2c, unsigned char *buf, int len,
		enum msg_end_type end_type)
{
	int ret = 0;
	long timeout = TIMEOUT;
	unsigned short reg_tmp;
	unsigned int wait_complete_timeout_ms;

	wait_complete_timeout_ms =
		len * 1000 * 9 * 2 / i2c->rate + CONFIG_I2C_JZV12_WAIT_MS;

#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_WARN)
		dev_info(&(i2c->adap.dev),
				"%s, Begin write msg, want to write length is %d\n",
				__func__, len);
#endif

	i2c->wbuf = buf;
	i2c->len = len;

	i2c_writel(i2c, I2C_TXTL, TX_LEVEL);

	i2c_readl(i2c, I2C_CSTP);	/* clear STP bit */
	i2c_readl(i2c, I2C_CTXOF);	/* clear TXOF bit */
	i2c_readl(i2c, I2C_CTXABRT);	/* clear TXABRT bit */

	i2c->w_end_type = end_type;
	while ((i2c_readl(i2c, I2C_STA) & I2C_STA_TFNF) && (i2c->len > 0)) {
		reg_tmp = *i2c->wbuf++;
		if (i2c->len == 1) {
			if (end_type == MSG_END_STOP) {
				reg_tmp |= I2C_DC_STP;
			}
		}
		i2c_writel(i2c, I2C_DC, reg_tmp);

		i2c->len -= 1;
	}

	if (i2c->len == 0) {
		i2c_writel(i2c, I2C_TXTL, 0);
	}
#if 0
	while (!(i2c_readl(i2c, I2C_STA) & I2C_STA_TFE) && (--timeout > 0)){
		udelay(150);
	}
#endif
	reg_tmp = I2C_INTM_MTXEMP | I2C_INTM_MTXABT | I2C_INTM_MTXOF;
	if (end_type == MSG_END_STOP)
		reg_tmp |= I2C_INTM_MISTP;

	i2c_writel(i2c, I2C_INTM, reg_tmp);

	timeout = wait_for_completion_timeout(&i2c->complete,
			msecs_to_jiffies
			(wait_complete_timeout_ms));

	if (!timeout) {
		dev_err(&(i2c->adap.dev), "--I2C pio write wait timeout\n");
#ifdef I2C_DEBUG
		i2c_jz_dump_regs(i2c);
#endif
		ret = -ETIMEDOUT;
	}

	reg_tmp = i2c_readl(i2c, I2C_TXABRT);
	if (reg_tmp) {
		txabrt(i2c, reg_tmp);
		if (reg_tmp > 0x1 && reg_tmp < 0x10)
			ret = -ENXIO;
		else
			ret = -EIO;
		//after I2C_TXABRT_ABRT_XDATA_NOACK error,this required core to resend
		if (reg_tmp & 8) {
			ret = -EAGAIN;
		}
		i2c_readl(i2c, I2C_CTXABRT);
	}

	if (ret < 0)
		i2c_jz_reset(i2c);

#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_WARN)
		dev_info(&(i2c->adap.dev),
				"%s, Write msg over\n", __func__);
#endif
	return ret;
}

static int i2c_disable_clk(struct i2c_jz *i2c)
{
	int timeout = 10;
	int tmp = i2c_readl(i2c, I2C_STA);

	while ((tmp & I2C_STA_MSTACT) && (--timeout > 0)) {
		udelay(90);
		tmp = i2c_readl(i2c, I2C_STA);
	}
	if (timeout > 0) {
		clk_disable(i2c->clk);
		return 0;
	} else {
		dev_err(&(i2c->adap.dev),
				"--I2C disable clk timeout, I2C_STA = 0x%x\n", tmp);
		i2c_jz_reset(i2c);
		clk_disable(i2c->clk);
		return -ETIMEDOUT;
	}
}

static int i2c_irq_dmawrite_done(struct i2c_jz *i2c,int result)
{
	// unsigned long flags;
	int ret = 0;
	struct i2c_dma_node *chain ;

	chain = list_first_entry_or_null(&i2c->queues_list, struct i2c_dma_node, entry);
	if(!chain) {
		// printk("[%s]error! i2c dma write chain list is null\n",__FUNCTION__);
		return -EINPROGRESS;
	}

#ifdef DMADBG
	if(result < 0){
		struct jzdma_channel *chnnel;
		unsigned int dma_src = 0;
		chnnel = to_jzdma_chan(i2c->chan);

		//读取dma的寄存器
		dma_src = readl((void *)0xb3420000 + 0x20 * (chnnel->id));
		dmaengine_device_control(i2c->chan, DMA_TERMINATE_ALL, 0);
		dev_err(&(i2c->adap.dev),"dma_offset = %d\n", dma_src - chain->dma_phy);
	}
#endif
	list_del(&chain->entry);
	list_add_tail(&chain->entry, &i2c->buf_list);

	i2cdma_free(i2c,chain->real_Len);

	if(chain->i2c_dma_callback){
		chain->i2c_dma_callback(chain->param,result);
	}

	/*这里处理完，表示一个chain处理完，那么因为在commit中使能了clk，因此这里处理完就立即关闭clk，否则一直关闭不掉
	这里关闭也是安全的*/
	i2c_readl(i2c, I2C_CTXABRT); /* clear */
	i2c_writel(i2c, I2C_DMACR, 0);
//	i2c_writel(i2c, I2C_ENB, 0);
	clk_disable(i2c->clk);

	if(i2c_dma_commit(i2c) != 0)
	{
		//没有pdma任务，清理状态
		// i2c_writel(i2c, I2C_DMACR, 0);
		i2c->i2c_busy_status = 0;
		i2c->isdma_mode = 0;
		if(i2c->pio_wait){
			// printk("dma_write_done_callback notify_pio_comp......\n");
			complete(&i2c->notify_pio_comp);
		}
		// printk("after disable i2cclk,i2csta:%#x,i2cdmasta:%#x\n",i2c_readl(i2c,I2C_STA),i2c_readl(i2c,I2C_DMACR));
		ret = -EINPROGRESS;
	}

	return ret;
}
static void dma_write_done_callback(void *data)
{
	struct i2c_dma_node *chain = (struct i2c_dma_node *)data;
	struct i2c_jz *i2c = chain->i2c;

	//set I2C_INTM_MTXEMP 1，when i2cdma end, i2c_jz_irq will call
	i2c_writel(i2c, I2C_INTM, i2c_readl(i2c, I2C_INTM) | I2C_INTM_MTXEMP);
	// printk("i2c callback,now i2c handle should run\n");
	return ;
}

static int i2c_dma_config(struct i2c_dma_node *chain,int offset)
{
	int ret;
	struct i2c_jz *i2c;
	i2c = chain->i2c;

//	dma_sync_single_for_device(NULL, (dma_addr_t)chain->dma_phy, chain->trans_len, DMA_FROM_DEVICE);
	//tx config
	chain->dma_config_tx.direction = DMA_MEM_TO_DEV;
	chain->dma_config_tx.src_addr = (dma_addr_t)chain->dma_phy;//(dma_addr_t)chain->dma_phy + offset;
	chain->dma_config_tx.dst_addr = (dma_addr_t)(i2c->base_phy + I2C_DC);
	chain->dma_config_tx.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	chain->dma_config_tx.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	chain->dma_config_tx.src_maxburst = 2; //对应tsz 2byte
	chain->dma_config_tx.dst_maxburst = 2;
	// printk("tx (srcaddr,dstaddr)=(%#x,%#x)\n",chain->dma_config_tx.src_addr,chain->dma_config_tx.dst_addr);
	ret = dmaengine_slave_config(i2c->chan,&chain->dma_config_tx);
	if(ret < 0){
		printk("i2c dmaengine_slave_config err\n");
		return -1;
	}

	//submit
	chain->desc_tx= dmaengine_prep_slave_single(i2c->chan,chain->dma_config_tx.src_addr,chain->trans_len,DMA_MEM_TO_DEV,DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
	if(!chain->desc_tx){
		printk("dmaengine_prep_slave_single err\n");
		return -1;
	}

	chain->desc_tx->callback = dma_write_done_callback;
	chain->desc_tx->callback_param = chain;
	if(dmaengine_submit(chain->desc_tx) < 0) {
		printk("DMA rx Request error\n");
		return -1;
	}

	return 0;
}

static int i2c_jz_xfer(struct i2c_adapter *adap, struct i2c_msg *msg, int count)
{
	int i, ret = 0;
	unsigned long flags;
	int i2cdmaret = 0;
	struct i2c_jz *i2c = adap->algo_data;

    //printk("%s[%d]: \n",__func__,__LINE__);
#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_WARN) {
		printk("\n\n\n");
		dev_info(&(i2c->adap.dev),
				"%s, Begin master xfer, want to transfer msg count is %d\n",
				__func__, count);
	}
#endif

	spin_lock_irqsave(&i2c->slock,flags);
	if(i2c->i2c_busy_status) {
		//case 1. i2c busy,add to list
		i2c->pio_wait = 1;
		spin_unlock_irqrestore(&i2c->slock,flags);
		// printk("i2c_jz_xfer i2c is busy,wait notify_pio_comp......\n");
		if(wait_for_completion_timeout(&i2c->notify_pio_comp, 1000) <= 0){
			printk("i2c_jz_xfer wait i2c dma done out over time!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}

		spin_lock_irqsave(&i2c->slock,flags);
		i2c->pio_wait = 0;
		INIT_COMPLETION(i2c->notify_pio_comp);
	}

	i2c->i2c_busy_status = 1 ;
	spin_unlock_irqrestore(&i2c->slock,flags);

	clk_enable(i2c->clk);
	i2c_writel(i2c, I2C_TAR, msg->addr);

	for (i = 0; i < count; i++, msg++) {
		enum msg_end_type end_type = MSG_END_STOP;
		if (i < (count - 1)) {
			if (msg[i + 1].flags & I2C_M_NOSTART) {
				end_type = MSG_END_CONTINUE;	/* have no STOP and START */
			} else {
				end_type = MSG_END_REPEAT_START;	/* have no STOP but have RESTART */
			}
		}

		INIT_COMPLETION(i2c->complete);

#ifdef CONFIG_I2C_DEBUG_INFO
		if (i2c->debug > DEBUG_WARN)
			dev_info(&(i2c->adap.dev),
					"%s, Now transfer msg: %d\n", __func__, i);
#endif
		if (msg->flags & I2C_M_RD) {
			ret = xfer_read(i2c, msg->buf, msg->len, end_type);
		} else {
			ret = xfer_write(i2c, msg->buf, msg->len, end_type);
		}
		if (ret < 0) {
			clk_disable(i2c->clk);
			goto ERR;
		}
	}

	if (i2c_disable_clk(i2c)) {
		ret = -ETIMEDOUT;
		goto ERR;
	}

#ifdef CONFIG_I2C_DEBUG_INFO
	if (i2c->debug > DEBUG_WARN)
		dev_info(&(i2c->adap.dev),
				"%s, Transfer msg over\n\n\n", __func__);
#endif

ERR:
	i2cdmaret = i2c_dma_commit(i2c);
	spin_lock_irqsave(&i2c->slock,flags);

	if(i2cdmaret < 0)
	{
		i2c->i2c_busy_status = 0;
	}
	spin_unlock_irqrestore(&i2c->slock,flags);
	// if(ret != 0){
	// 	printk(".......i2c_jz_xfer ret:%d,i2cdmaret:%d...............................\n",ret,i2cdmaret);
	// }
	return ret ? : i;
}

static u32 i2c_jz_functionality(struct i2c_adapter *adap)
{
	unsigned int ret;
	ret = I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_10BIT_ADDR;

	return ret;
}

static const struct i2c_algorithm i2c_jz_algorithm = {
	.master_xfer = i2c_jz_xfer,
	.functionality = i2c_jz_functionality,
};

static int i2c_set_speed(struct i2c_jz *i2c, int rate)
{
	/*ns */
	long dev_clk = clk_get_rate(i2c->clk);
	long cnt_high = 0;	/* HIGH period count of the SCL clock */
	long cnt_low = 0;	/* LOW period count of the SCL clock */
	long setup_time = 0;
	long hold_time = 0;
	unsigned short tmp;

	i2c->rate = rate;
	if (i2c_jz_disable(i2c))
		dev_info(&(i2c->adap.dev), "i2c not disable\n");
	if (rate <= 100000) {
		tmp = 0x43 | (1 << 5);	/* standard speed mode */
		i2c_writel(i2c, I2C_CTRL, tmp);
	} else {
		tmp = 0x45 | (1 << 5);	/* fast speed mode */
		i2c_writel(i2c, I2C_CTRL, tmp);
	}

	/*         high
	 *         ____     ____      ____      ____
	 *  clk __|  | |___|    |____|    |____|    |___
	 *           | | |
	 *           | | |
	 *           |_|_|     _________      ____
	 * data    __/ | |\___/         \____/    \____
	 *    setup->| |<|
	 *           ->| |<-hold
	 */

	//setup_time = (10 000 000/(rate*4)) + 1;
	setup_time = (dev_clk / (rate * 4));
	if (setup_time > 1)
		setup_time -= 1;
	//hold_time =  (10000000/(rate*4)) - 1;
	hold_time = (dev_clk / (rate * 4));

	/*         high
	 *         ____     ____
	 *  clk __|    |___|    |____
	 *              low
	 *        |<--period--->|
	 *
	 */
	cnt_high = dev_clk / (rate * 2);
	cnt_low = dev_clk / (rate * 2);

	/*dev_info(&(i2c->adap.dev), "set:%ld  hold:%ld dev=%ld h=%ld l=%ld\n",
			setup_time, hold_time, dev_clk, cnt_high, cnt_low);*/
	if (setup_time > 255)
		setup_time = 255;
	if (setup_time <= 0)
		setup_time = 1;
	if (hold_time > 0xFFFF)
		hold_time = 0xFFFF;

	if (rate <= 100000) {
		i2c_writel(i2c, I2C_SHCNT, I2CSHCNT_ADJUST(cnt_high));
		i2c_writel(i2c, I2C_SLCNT, I2CSLCNT_ADJUST(cnt_low));
	} else {
		i2c_writel(i2c, I2C_FHCNT, I2CFHCNT_ADJUST(cnt_high));
		i2c_writel(i2c, I2C_FLCNT, I2CFLCNT_ADJUST(cnt_low));
	}

	i2c_writel(i2c, I2C_SDASU, setup_time & 0xff);
	i2c_writel(i2c, I2C_SDAHD, hold_time);

	return 0;
}

static bool dma_txchan_filter(struct dma_chan *chan, void *filter_param)
{
	struct i2c_jz *i2c = (struct i2c_jz*)filter_param;

	// printk("tx dma_chan:%d,type:%#x,dma_type:%#x\n",chan->chan_id,(int)chan->private,i2c->dma_type_tx);
	if(i2c->dma_type_tx == (int)chan->private){
		// printk("dma_chan:%d,type:%#x,dma_type:%#x\n",chan->chan_id,(int)chan->private,ingss->dma_type);
		return 1;
	} else {
		return 0;
	}
}

static int i2c_dma_init(struct i2c_jz *i2c)
{
	dma_cap_mask_t mask_tx;
	if(!i2c){
		printk("err param is null\n");
		return -1;
	}

	switch(i2c->adap.nr){
		case 0:
			i2c->dma_type_tx = JZDMA_REQ_I2C0_TX;
			i2c->dma_type_rx = JZDMA_REQ_I2C0_RX;
			break;
		case 1:
			i2c->dma_type_tx = JZDMA_REQ_I2C1_TX;
			i2c->dma_type_rx = JZDMA_REQ_I2C1_RX;
			break;
		case 2:
			i2c->dma_type_tx = JZDMA_REQ_I2C2_TX;
			i2c->dma_type_rx = JZDMA_REQ_I2C2_RX;
			break;
		default:
			return -1;
	}

	dma_cap_zero(mask_tx);
	dma_cap_set(DMA_SLAVE,mask_tx);
	i2c->chan = dma_request_channel(mask_tx,dma_txchan_filter,(void *)i2c);
	if(!i2c->chan) {
		printk("Request DMA TX channel error!\n");
		return -1;
	}

	return 0;
}
static int32_t dam_data_conver(void *src_buf, void *dst_buf, int32_t buf_num, i2c_dma_param_t param)
{
	int i,j;
	uint8_t *src = (uint8_t *)src_buf;
	uint16_t *dst = (uint16_t *)dst_buf;
	const int32_t unit_bytes = param.unit_size;
	unsigned int i2cmode = param.i2cmode;

	for ( i = 0; i < buf_num; i++) {
		uint8_t *cur_src = src + i * unit_bytes;
		for (j = 0; j < unit_bytes; j++) {
			if(i2cmode == 0){
				dst[j] = (j == unit_bytes - 1)
					? (cur_src[j] | I2C_DC_STP)
					: cur_src[j];
			} else {
				dst[j] = (j == unit_bytes - 2)
					? (cur_src[j] | I2C_DC_REST)
					: cur_src[j];
			}
		}
		dst += unit_bytes;
	}

	return buf_num * unit_bytes *sizeof(uint16_t);
}



static int param_check(i2c_dma_param_t *param)
{
	if(param->unit_size > 8 || param->unit_size <= 0){
		return -EAGAIN;
	}

	if(param->i2cmode < 0 || param->i2cmode > 1){
		printk("check err,i2cmode now only support 0(stop mode) 1(restart mode),i2cmode:%d\n",param->i2cmode);
		return -EAGAIN;
	}
	if(!param->buf){
		printk("check err,buf is null\n");
		return -EAGAIN;
	}

	if(param->unit_num*param->unit_size*sizeof(uint16_t) > DMAI2CBUFSIZE){
		printk("check err,buflen out over range\n");
		return -EAGAIN;
	}

	return 0;
}

static int i2c_dma_commit(struct i2c_jz *i2c)
{
	struct i2c_dma_node *chain;
	unsigned long flags;
	int ret = 0;
	static int debug = 0;

	spin_lock_irqsave(&i2c->slock,flags);
	chain = list_first_entry_or_null(&i2c->queues_list, struct i2c_dma_node, entry);
	if(!chain) {
		spin_unlock_irqrestore(&i2c->slock,flags);
		// printk("[%s]error! i2c dma write chain list is null\n",__FUNCTION__);
		return -2;
	}

	i2c->isdma_mode = 1;
	spin_unlock_irqrestore(&i2c->slock,flags);

	clk_enable(i2c->clk);
	i2c_readl(i2c, I2C_CTXABRT); /* clear */
	i2c_writel(i2c, I2C_TAR, chain->slaver_addr);
	i2c_writel(i2c, I2C_DMATDLR, TX_LEVEL);
//	i2c_writel(i2c, I2C_DMATDLR, 0);
	i2c_writel(i2c, I2C_INTM, I2C_INTM_MTXABT);
	if(debug){
		i2c_jz_dump_regs(i2c);
		debug = 0;
	}
	ret = i2c_dma_config(chain, 0);
	i2c_writel(i2c, I2C_DMACR, 0x1<<1);
	dma_async_issue_pending(i2c->chan);
//	i2c_writel(i2c, I2C_ENB, 1);

	return ret;
}

void *i2cdma_alloc(struct i2c_jz *i2c,int unit_num,int unit_size,int *real_len)
{
	void *alloc_ptr = NULL;
	int remain;
	int remain_head,remain_end;
	int len = unit_num * unit_size;

	if (!len) {
		return NULL;
	}

	// CASE 1: 空闲内存是连续的一段 (free_offset < busy_offset)
	if (i2c->free_offset < i2c->busy_offset) {
		remain = i2c->busy_offset - i2c->free_offset;
		if(remain < unit_size){
			*real_len = 0;
			return NULL;
		}
		if (len <= remain) {
			alloc_ptr = (char *)i2c->dma_vir + i2c->free_offset;
			i2c->free_offset += len;
			*real_len = len;
		} else {
			alloc_ptr = (char *)i2c->dma_vir + i2c->free_offset;
			i2c->free_offset += remain;
			*real_len = remain;
		}
	}
	// CASE 2: 空闲内存分两段 [0, busy_offset] 和 [free_offset, end]
	else {
		remain_head = i2c->busy_offset - 0;
		remain_end = DMAI2CBUFSIZE - i2c->free_offset;

		if(remain_end >= len){
			alloc_ptr = (char *)i2c->dma_vir + i2c->free_offset;
			i2c->free_offset += len;
			*real_len = len;
		} else if (remain_head >=len ){
			alloc_ptr = (char *)i2c->dma_vir;
			i2c->free_offset = len;
			*real_len = len;
		} else if(remain_end >= remain_head) {
			if(remain_end < unit_size){
				*real_len = 0;
				return NULL;
			}
			alloc_ptr = (char *)i2c->dma_vir + i2c->free_offset;
			i2c->free_offset += remain_end;
			*real_len = remain_end;
		} else {
			if(remain_head < unit_size){
				*real_len = 0;
				return NULL;
			}
			alloc_ptr = (char *)i2c->dma_vir ;
			i2c->free_offset = remain_head;
			*real_len = remain_head;
		}
	}

	return alloc_ptr;
}

void i2cdma_free(struct i2c_jz *i2c,size_t len) {
	if (len == 0) {
		return;
	}

	if(DMAI2CBUFSIZE - i2c->busy_offset >= len){
		i2c->busy_offset = (i2c->busy_offset + len) % DMAI2CBUFSIZE;
	} else {
		i2c->busy_offset = len;
	}

	return ;
}


static unsigned long i2c_dma_vir_to_phy(struct i2c_jz *i2c,void *vir){
	return i2c->dma_phy + ((unsigned long)vir - (unsigned long)i2c->dma_vir);
}

int i2c_dma_write_transfer(struct i2c_adapter *adapter,i2c_dma_param_t *param)
{
	unsigned long flags;
	int trans_unit_num;
	int real_len;
	struct i2c_dma_node *chain;
	uint32_t dma_tran_count = 0;
	void *dstbuf = NULL;
	struct i2c_jz *i2c = NULL;
	i2c_dma_param_t stparam;

	if(!adapter || !param){
		return -EAGAIN;
	}

	i2c = container_of(adapter, struct i2c_jz, adap);
	if(!i2c){
		printk("container_of err\n");
		return -EAGAIN;
	}

	if(param_check(param) < 0){
		return -EAGAIN;
	}

	memcpy(&stparam, param, sizeof(i2c_dma_param_t));

	// printk("param.buflen:%d,unit_num:%d,unit_size:%d",stparam.buf_len,stparam.unit_num,stparam.unit_size);

	spin_lock_irqsave(&i2c->slock,flags);

	chain = list_first_entry_or_null(&i2c->buf_list, struct i2c_dma_node, entry);
	if(!chain) {
		spin_unlock_irqrestore(&i2c->slock,flags);
		printk("error! i2c dma write chain list is null\n");
		return -EBUSY;
	}

	dstbuf = i2cdma_alloc(i2c,stparam.unit_num,stparam.unit_size * sizeof(uint16_t) ,&real_len);
	if(!dstbuf){
		printk("err, i2cdma now is no buf to use\n");
		spin_unlock_irqrestore(&i2c->slock,flags);
		return -ENOMEM;
	}

	trans_unit_num = real_len / (stparam.unit_size *sizeof(uint16_t)); //实际可以处理的内存大小
	dma_tran_count = dam_data_conver(stparam.buf,dstbuf, trans_unit_num, stparam);

	// printk("(vir:%#lx,phy:%#lx),real_len:%d,dma_tran_count:%d\n",(unsigned long)dstbuf,i2c_dma_vir_to_phy(i2c,dstbuf),
	chain->slaver_addr = stparam.slaver_addr;
	chain->dma_phy = i2c_dma_vir_to_phy(i2c,dstbuf);
	chain->real_Len = real_len;
	chain->trans_len = dma_tran_count;
	chain->i2c_dma_callback = stparam.i2c_dma_callback;
	chain->param = stparam.param;

	list_del(&chain->entry);
	list_add_tail(&chain->entry, &i2c->queues_list);

	// case 1 i2cbusy return
	if(i2c->i2c_busy_status || i2c->pio_wait){
		spin_unlock_irqrestore(&i2c->slock,flags);
		return trans_unit_num;
	}

	i2c->i2c_busy_status = 1;
	spin_unlock_irqrestore(&i2c->slock,flags);

	//case 2  i2c idle,commit dma task
	if(i2c_dma_commit(i2c) < 0) {
		return -EINVAL;
	}

#ifdef DMADBG
	struct jzdma_channel *chnnel;
	chnnel = to_jzdma_chan(i2c->chan);

	//读取dma的寄存器
	printk("tx channel:%d\n",chnnel->id);
	printk("0x00 DMAC_DSAR %#x\n",readl((void *)DMAC_BASE + DMAC_DSAR(chnnel->id)));
	printk("0x04 DMAC_DTAR %#x\n",readl((void *)DMAC_BASE + DMAC_DTAR(chnnel->id)));
	printk("0x08 DMAC_DTCR %#x\n",readl((void *)DMAC_BASE + DMAC_DTCR(chnnel->id)));
	printk("0x0c DMAC_DRSR %#x\n",readl((void *)DMAC_BASE + DMAC_DRSR(chnnel->id)));
	printk("0x10 DMAC_DCCSR %#x\n",readl((void *)DMAC_BASE + DMAC_DCCSR(chnnel->id)));
	printk("0x14 DMAC_DCMD %#x\n",readl((void *)DMAC_BASE + DMAC_DCMD(chnnel->id)));
	printk("0x18 DMAC_DDA %#x\n",readl((void *)DMAC_BASE + DMAC_DDA(chnnel->id)));
	printk("0x1c DMAC_DSD %#x\n",readl((void *)DMAC_BASE + DMAC_DSD(chnnel->id)));
	printk("0x1000 DMAC %#x\n",readl((void *)DMAC_BASE + DMAC));
	printk("0x1004 DIRQP %#x\n",readl((void *)DMAC_BASE + DIRQP));
	printk("0x1008 DDB %#x\n",readl((void *)DMAC_BASE + DDR));
	printk("0x100c DDBS %#x\n",readl((void *)DMAC_BASE + DDRS));

	int i = 0;
	// for(i = 0; i < dma_tran_count;i++)
	{
		printk("tx dma_vir[%d]=%#lx,slv_addr:%#lx,buflen:%d,trslen:%d\n",i,chain->dma_phy,\
												stparam.slaver_addr,stparam.buf_len,chain->trans_len);
	}

	i2c_jz_dump_regs(i2c);
#endif

	return trans_unit_num;
}
EXPORT_SYMBOL(i2c_dma_write_transfer);


static int i2c_jz_probe(struct platform_device *dev)
{
	int ret = 0;
	int i;
	struct i2c_jz *i2c;
	struct resource *res;
	void *chain_buf;
	struct i2c_dma_node *chain;
	unsigned int reg_tmp;

	i2c = kzalloc(sizeof(struct i2c_jz), GFP_KERNEL);
	if (!i2c) {
		printk("Error: Now we can not malloc memory for I2C!\n");
		ret = -ENOMEM;
		goto ERR0;
	}

	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &i2c_jz_algorithm;
	i2c->adap.retries = 5;
	i2c->adap.timeout = 5;
	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &dev->dev;
	i2c->adap.nr = dev->id;
	sprintf(i2c->adap.name, "gate_i2c%u", dev->id);

	i2c->clk = clk_get(&dev->dev, i2c->adap.name);
	if (!i2c->clk) {
		printk("Error: Now we can not get i2c%d clock!\n", dev->id);
		ret = -ENODEV;
		goto clk_failed;
	}

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	i2c->base_phy = res->start;
	i2c->iomem = ioremap(res->start, resource_size(res));
	if (!i2c->iomem) {
		printk("Error: Now we can remap IO for I2C%d!\n", dev->id);
		ret = -ENOMEM;
		goto io_failed;
	}

	ret = i2c_dma_init(i2c);
	if(ret < 0){
		ret = -ENODEV;
		goto io_failed;
	}
	i2c->dma_vir = dma_alloc_coherent(&dev->dev, DMAI2CBUFSIZE,
		&i2c->dma_phy, GFP_KERNEL | GFP_DMA);
	if(!i2c->dma_vir){
		printk("no memm dma_alloc_coherent err \n");
		return -ENOMEM;
	}

	i2c->free_offset = 0;
	i2c->busy_offset = DMAI2CBUFSIZE;

	i2c->irq = platform_get_irq(dev, 0);
	ret =
		request_irq(i2c->irq, i2c_jz_irq, IRQF_DISABLED,
				dev_name(&dev->dev), i2c);
	if (ret) {
		printk("Error: Now we can request irq for I2C%d!\n", dev->id);
		ret = -ENODEV;
		goto irq_failed;
	}

	/* I2C0 already enable in SPL */
	if(dev->id == 0){
#ifndef CONFIG_I2C0_BOOT_DEFAULT
		clk_enable(i2c->clk);
#endif
	}else if(dev->id == 1){
#ifndef CONFIG_I2C1_BOOT_DEFAULT
		clk_enable(i2c->clk);
#endif
	}else{
		clk_enable(i2c->clk);
	}

	/* I2C0 already enable in SPL */
	if(dev->id == 0){
		res = platform_get_resource(dev, IORESOURCE_BUS, 0);
		i2c->speed = res->start;
#ifndef CONFIG_I2C0_BOOT_DEFAULT
		i2c_set_speed(i2c, i2c->speed * 1000);
		reg_tmp = i2c_readl(i2c, I2C_CTRL);
#if defined(CONFIG_I2C0_NON_RESTART_MODE) \
		||defined(CONFIG_I2C1_NON_RESTART_MODE) \
		||defined(CONFIG_I2C2_NON_RESTART_MODE) \
		||defined(CONFIG_I2C3_NON_RESTART_MODE) \
		||defined(CONFIG_I2C4_NON_RESTART_MODE)
		reg_tmp &= ~I2C_CTRL_REST;
#else
		reg_tmp |= I2C_CTRL_REST;
#endif
		i2c_writel(i2c, I2C_CTRL, reg_tmp);
		i2c_writel(i2c, I2C_INTM, 0x0);
#endif
	}else if(dev->id == 1){
		res = platform_get_resource(dev, IORESOURCE_BUS, 0);
		i2c->speed = res->start;
#ifndef CONFIG_I2C1_BOOT_DEFAULT
		i2c_set_speed(i2c, i2c->speed * 1000);
		reg_tmp = i2c_readl(i2c, I2C_CTRL);
#if defined(CONFIG_I2C0_NON_RESTART_MODE) \
		||defined(CONFIG_I2C1_NON_RESTART_MODE) \
		||defined(CONFIG_I2C2_NON_RESTART_MODE) \
		||defined(CONFIG_I2C3_NON_RESTART_MODE) \
		||defined(CONFIG_I2C4_NON_RESTART_MODE)
		reg_tmp &= ~I2C_CTRL_REST;
#else
		reg_tmp |= I2C_CTRL_REST;
#endif
		i2c_writel(i2c, I2C_CTRL, reg_tmp);
		i2c_writel(i2c, I2C_INTM, 0x0);
#endif
	}else{
		res = platform_get_resource(dev, IORESOURCE_BUS, 0);
		i2c->speed = res->start;
		i2c_set_speed(i2c, i2c->speed * 1000);
#if 0
		reg_tmp = i2c_readl(i2c, I2C_DC);
		reg_tmp &= ~I2C_DC_STP;
		i2c_writel(i2c, I2C_DC, reg_tmp);
#endif
		reg_tmp = i2c_readl(i2c, I2C_CTRL);
#if defined(CONFIG_I2C0_NON_RESTART_MODE) \
		||defined(CONFIG_I2C1_NON_RESTART_MODE) \
		||defined(CONFIG_I2C2_NON_RESTART_MODE) \
		||defined(CONFIG_I2C3_NON_RESTART_MODE) \
		||defined(CONFIG_I2C4_NON_RESTART_MODE)
		reg_tmp &= ~I2C_CTRL_REST;
#else
		reg_tmp |= I2C_CTRL_REST;
#endif
		i2c_writel(i2c, I2C_CTRL, reg_tmp);

		// for jgao WHY?
		//  i2c_writel(i2c, I2C_FLT, 0xF);      /*set filter*/

		i2c_writel(i2c, I2C_INTM, 0x0);
	}

	spin_lock_init(&i2c->slock);
	init_completion(&i2c->dma_rx_done_comp);
	init_completion(&i2c->dma_tx_done_comp);
	init_completion(&i2c->complete);
	init_completion(&i2c->notify_pio_comp);

	INIT_LIST_HEAD(&i2c->buf_list);
	INIT_LIST_HEAD(&i2c->queues_list);


	chain_buf = kmalloc(sizeof(struct i2c_dma_node) * I2C_CHAIN_NUM_MAX,GFP_KERNEL);
	if(!chain_buf) {
		printk("%s %d, chain_buf alloc failed\n",__func__,__LINE__);
	}

	for(i = 0; i < I2C_CHAIN_NUM_MAX; i++) {
		chain = (struct i2c_dma_node *)chain_buf + i;
		chain->index = i;
		chain->work_mode = 0;
		chain->i2c = i2c;
		list_add_tail(&chain->entry, &i2c->buf_list);
	}

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&(i2c->adap.dev), KERN_INFO "I2C: Failed to add bus\n");
		goto adapt_failed;
	}

	platform_set_drvdata(dev, i2c);

	/* I2C0 already enable in SPL */
	if(dev->id == 0){
#ifndef CONFIG_I2C0_BOOT_DEFAULT
		i2c_jz_enable(i2c);
		clk_disable(i2c->clk);
		dev_info(&(i2c->adap.dev), "i2c%d register ok, speed is %d\n", dev->id, i2c->speed * 1000);
#else
		dev_info(&(i2c->adap.dev), "i2c%d skip clk init.\n", dev->id);
#endif
	}else if(dev->id == 1){
#ifndef CONFIG_I2C1_BOOT_DEFAULT
		i2c_jz_enable(i2c);
		clk_disable(i2c->clk);
		dev_info(&(i2c->adap.dev), "i2c%d register ok, speed is %d\n", dev->id, i2c->speed * 1000);
#else
		dev_info(&(i2c->adap.dev), "i2c%d skip clk init.\n", dev->id);
#endif
	}else{
		i2c_jz_enable(i2c);
		clk_disable(i2c->clk);
		dev_info(&(i2c->adap.dev), "i2c%d register ok, speed is %d\n", dev->id, i2c->speed * 1000);
	}

#ifdef CONFIG_I2C_DEBUG_INFO
	ret = create_debug_sysfs_interface(&dev->dev);
	if (ret < 0)
		dev_err(&i2c->adap.dev, "create debug sysfs interface failed\n");
#endif

	return 0;

adapt_failed:
	free_irq(i2c->irq, i2c);
irq_failed:
	iounmap(i2c->iomem);
io_failed:
	clk_put(i2c->clk);
clk_failed:
	kfree(i2c);
ERR0:
	return ret;
}

static int i2c_jz_remove(struct platform_device *dev)
{
	struct i2c_jz *i2c = platform_get_drvdata(dev);

	free_irq(i2c->irq, i2c);
	iounmap(i2c->iomem);
	clk_put(i2c->clk);
	i2c_del_adapter(&i2c->adap);
	kfree(i2c);

	return 0;
}
#ifdef CONFIG_PM_SLEEP
static int ingenic_i2c_suspend(struct device *dev)
{
	return 0;
}

static int ingenic_i2c_resume(struct device *dev)
{
	unsigned int reg_tmp;
	int ret = 0;
	struct i2c_jz *i2c = dev_get_drvdata(dev);//// dev->p->driver_data;
	printk("%s:i2c=%p\n", __func__, i2c);
	ret = clk_prepare_enable(i2c->clk);
	if(ret){
		dev_err(&(i2c->adap.dev),
			"%s %d, I2C enable clk error\n",
			__func__, __LINE__);
	}
	i2c_set_speed(i2c, i2c->speed * 1000);
	reg_tmp = i2c_readl(i2c, I2C_CTRL);
#if defined(CONFIG_I2C0_NON_RESTART_MODE) \
	||defined(CONFIG_I2C1_NON_RESTART_MODE) \
	||defined(CONFIG_I2C2_NON_RESTART_MODE) \
	||defined(CONFIG_I2C3_NON_RESTART_MODE) \
	||defined(CONFIG_I2C4_NON_RESTART_MODE)
	reg_tmp &= ~I2C_CTRL_REST;
#else
	reg_tmp |= I2C_CTRL_REST;
#endif
	i2c_writel(i2c, I2C_CTRL, reg_tmp);
	i2c_writel(i2c, I2C_INTM, 0x0);
	i2c_jz_enable(i2c);
	clk_disable_unprepare(i2c->clk);

	return 0;
}
static SIMPLE_DEV_PM_OPS(ingenic_i2c_pm_ops, ingenic_i2c_suspend, ingenic_i2c_resume);
#endif

static struct platform_driver i2c_jz_driver = {
	.probe = i2c_jz_probe,
	.remove = i2c_jz_remove,
	.driver = {
		.name = "jz-i2c",
#ifdef CONFIG_PM_SLEEP
		.pm = &ingenic_i2c_pm_ops,
#endif
	},
};

static int __init i2c_jz_init(void)
{
	return platform_driver_register(&i2c_jz_driver);
}

subsys_initcall(i2c_jz_init);

static void __exit i2c_jz_exit(void)
{
	platform_driver_unregister(&i2c_jz_driver);
}

module_exit(i2c_jz_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("i2c driver for JZ SoCs");
