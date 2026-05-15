#ifndef __LINUX_SPI_JZ_H
#define __LINUX_SPI_JZ_H

// #include "soc/sfc.h"
#include <mach/jzdma.h>

/*************************************************************************
 * SSI (Synchronous Serial Interface)
 *************************************************************************/
/* n = 0, 1 (SSI0, SSI1) */
#define	SSI_DR			0x000
#define	SSI_CR0			0x004
#define	SSI_CR1			0x008
#define	SSI_SR			0x00C
#define	SSI_ITR			0x010
#define	SSI_ICR			0x014
#define	SSI_GR			0x018
#define	SSI_RCNT		0x01C

/* SSI Data Register (SSI_DR) */
#define	DR_GPC_BIT		0
#define	DR_GPC_MASK		(0x1ff << SSI_DR_GPC_BIT)

/* SSI Control Register 0 (SSI_CR0) */
#define CR0_TENDIAN_BIT		18
#define CR0_TENDIAN_MASK	(3 << CR0_TENDIAN_BIT)
#define CR0_RENDIAN_BIT		16
#define CR0_RENDIAN_MASK	(3 << CR0_RENDIAN_BIT)
#define CR0_SSIE		(1 << 15)
#define CR0_TIE			(1 << 14)
#define CR0_RIE			(1 << 13)
#define CR0_TEIE		(1 << 12)
#define CR0_REIE		(1 << 11)
#define CR0_LOOP		(1 << 10)
#define CR0_RFINE		(1 << 9)
#define CR0_RFINC		(1 << 8)
#define CR0_EACLRUN		(1 << 7) /* hardware auto clear underrun when TxFifo no empty */
#define CR0_FSEL		(1 << 6)
#define CR0_VRCNT		(1 << 4)
#define CR0_TFMODE		(1 << 3)
#define CR0_TFLUSH		(1 << 2)
#define CR0_RFLUSH		(1 << 1)
#define CR0_DISREV		(1 << 0)

/* SSI Control Register 1 (SSI_CR1) */
#define CR1_FRMHL_BIT		30
#define CR1_FRMHL_MASK		(0x3 << CR1_FRMHL_BIT)
#define CR1_FRMHL_CELOW_CE2LOW	(0 << CR1_FRMHL_BIT) /* SSI_CE_ is low valid and SSI_CE2_ is low valid */
#define CR1_FRMHL_CEHIGH_CE2LOW	(1 << CR1_FRMHL_BIT) /* SSI_CE_ is high valid and SSI_CE2_ is low valid */
#define CR1_FRMHL_CELOW_CE2HIGH	(2 << CR1_FRMHL_BIT) /* SSI_CE_ is low valid  and SSI_CE2_ is high valid */
#define CR1_FRMHL_CEHIGH_CE2HIGH	(3 << CR1_FRMHL_BIT) /* SSI_CE_ is high valid and SSI_CE2_ is high valid */
#define CR1_TFVCK_BIT		28
#define CR1_TFVCK_MASK		(0x3 << CR1_TFVCK_BIT)
  #define CR1_TFVCK_0		  (0 << CR1_TFVCK_BIT)
  #define CR1_TFVCK_1		  (1 << CR1_TFVCK_BIT)
  #define CR1_TFVCK_2		  (2 << CR1_TFVCK_BIT)
  #define CR1_TFVCK_3		  (3 << CR1_TFVCK_BIT)
#define CR1_TCKFI_BIT		26
#define CR1_TCKFI_MASK		(0x3 << CR1_TCKFI_BIT)
  #define CR1_TCKFI_0		  (0 << CR1_TCKFI_BIT)
  #define CR1_TCKFI_1		  (1 << CR1_TCKFI_BIT)
  #define CR1_TCKFI_2		  (2 << CR1_TCKFI_BIT)
  #define CR1_TCKFI_3		  (3 << CR1_TCKFI_BIT)
#define CR1_ITFRM		(1 << 24)
#define CR1_UNFIN		(1 << 23)
#define CR1_FMAT_BIT		20
#define CR1_FMAT_MASK		(0x3 << CR1_FMAT_BIT)
  #define CR1_FMAT_SPI		  (0 << CR1_FMAT_BIT) /* Motorola¡¯s SPI format */
  #define CR1_FMAT_SSP		  (1 << CR1_FMAT_BIT) /* TI's SSP format */
  #define CR1_FMAT_MW1		  (2 << CR1_FMAT_BIT) /* National Microwire 1 format */
  #define CR1_FMAT_MW2		  (3 << CR1_FMAT_BIT) /* National Microwire 2 format */
#define CR1_TTRG_BIT		16 /* SSI1 TX trigger */
#define CR1_TTRG_MASK		(0xf << CR1_TTRG_BIT)
#define CR1_MCOM_BIT		12
#define CR1_MCOM_MASK		(0xf << CR1_MCOM_BIT)
//  #define CR1_MCOM_BIT(NO)	  (##NO## << CR1_MCOM_BIT) /* N-bit command selected */
#define CR1_RTRG_BIT		8 /* SSI RX trigger */
#define CR1_RTRG_MASK		(0xf << CR1_RTRG_BIT)
#define CR1_FLEN_BIT		3
#define CR1_FLEN_MASK		(0x1f << CR1_FLEN_BIT)
  #define CR1_FLEN_2BIT		  (0x0 << CR1_FLEN_BIT)
#define CR1_PHA			(1 << 1)
#define CR1_POL			(1 << 0)

/* SSI Status Register (SSI_SR) */
#define SR_TFIFONUM_BIT		16
#define SR_TFIFONUM_MASK	(0xff << SR_TFIFONUM_BIT)
#define SR_RFIFONUM_BIT		8
#define SR_RFIFONUM_MASK	(0xff << SR_RFIFONUM_BIT)
#define SR_END			(1 << 7)
#define SR_BUSY			(1 << 6)
#define SR_TFF			(1 << 5)
#define SR_RFE			(1 << 4)
#define SR_TFHE			(1 << 3)
#define SR_RFHF			(1 << 2)
#define SR_UNDR			(1 << 1)
#define SR_OVER			(1 << 0)

/* SSI Interval Time Control Register (SSI_ITR) */
#define	ITR_CNTCLK		(1 << 15)
#define ITR_IVLTM_BIT		0
#define ITR_IVLTM_MASK		(0x7fff << ITR_IVLTM_BIT)



#define R_MODE			0x1
#define W_MODE			0x2
#define RW_MODE			(R_MODE | W_MODE)

#define R_DMA			0x4
#define W_DMA			0x8
#define RW_DMA			(R_DMA |W_DMA)

#define SPI_DMA_ACK		0x1

#define SPI_DMA_ERROR  		-3
#define SPI_CPU_ERROR		-4

#define SPI_COMPLETE		5

#define JZ_SSI_MAX_FIFO_ENTRIES 	128
#define JZ_SSI_DMA_BURST_LENGTH 	16

#define FIFO_W8			8
#define FIFO_W16		16
#define FIFO_W32		32

#define SPI_BITS_8		8
#define SPI_BITS_16		16
#define SPI_BITS_32		32

#define SPI_8BITS		1
#define SPI_16BITS		2
#define SPI_32BITS		4


/* tx rx threshold from 0x0 to 0xF */
#define SSI_FULL_THRESHOLD		0xF
#define SSI_TX_FIFO_THRESHOLD		0x1
#define SSI_RX_FIFO_THRESHOLD		(SSI_FULL_THRESHOLD - SSI_TX_FIFO_THRESHOLD)
#define SSI_SAFE_THRESHOLD		0x1

#define CPU_ONCE_BLOCK_ENTRIES 		((SSI_FULL_THRESHOLD-SSI_TX_FIFO_THRESHOLD)*8)

#define MAX_SSI_INTR		10000

#define MAX_SSICDR			63
#define MAX_CGV				255

#define SSI_DMA_FASTNESS_CHNL 	 0   // SSI controller [n] FASTNESS when probe();

#define JZ_NEW_CODE_TYPE

#define BUFFER_SIZE	PAGE_SIZE

#define CONFIG_DMA_ENGINE 1

#define SUSPND    (1<<0)
#define SPIBUSY   (1<<1)
#define RXBUSY    (1<<2)
#define TXBUSY    (1<<3)

struct jz_spi {
	/* bitbang has to be first */
	struct spi_bitbang	bitbang;
	struct clk		*clk_gate;
	unsigned int            clk_gate_flag;
	struct clk		*clk;
	unsigned int		clk_flag;
	unsigned int		set_clk_flag;
	struct completion	done;
	struct completion	done_rx;
	struct completion	done_tx_dma;
	struct completion	done_rx_dma;

	spinlock_t		lock;
	spinlock_t		txrx_lock;

	unsigned int		state;

	u8			chnl;
	u8			rw_mode;
	u8			spi_mode;
	u8			use_dma;
	u8			is_first;

	u8			bits_per_word;		/*8 or 16 (or 32)*/
	u8			transfer_unit_size;	/* 1 or 2 (or 4) */
	u8			tx_trigger;		/* 0-128 */
	u8			rx_trigger;		/* 0-128 */
	u8			dma_tx_unit;		/* 1 or 2 or 4 or 16 or 32*/
	u8			dma_rx_unit;		/* 1 or 2 or 4 or 16 or 32*/
	u8			txfifo_width;
	u8			rxfifo_width;
	u32			fifodepth;

	/* data buffers */
	const u8		*tx;
	u8			*rx;

	/* temp buffers */
	void			*buffer;
	dma_addr_t		buffer_dma;

	void __iomem		*iomem;
	unsigned long		phys;
	int			 irq;
	u32			 len;
	u32			 rlen;	  /* receive len */
	u32			 count;   /* sent count */
	enum jzdma_type     	 dma_type;
	u32			 dma_flag;

	void			(*set_cs)(struct jz_spi_info *spi, u8 cs, unsigned int pol);

	/* functions to deal with different size buffers */
	u32 (*get_rx) (struct jz_spi *);
	u32 (*get_tx) (struct jz_spi *);
	int (*txrx_bufs)(struct spi_device *spi, struct spi_transfer *t);
	irqreturn_t (*irq_callback)(struct jz_spi *);

#ifdef CONFIG_DMA_ENGINE
	struct dma_chan 	*txchan;
	struct dma_chan 	*rxchan;
	struct scatterlist  	*sg_rx;	/* I/O scatter list */
	struct scatterlist  	*sg_tx;	/* I/O scatter list */
#endif

	unsigned long src_clk;
	unsigned long spi_clk;

	struct jz_intr_cnt *g_jz_intr;

	struct resource		*ioarea;
	struct spi_master	*master;
	struct device		*dev;
	struct jz_spi_info *pdata;
//	struct spi_board_info *pdata;
};


struct jz_intr_cnt{
	int dma_tx_cnt;
	int dma_rx_cnt;
	int ssi_intr_cnt;
	int max_ssi_intr;
	int ssi_txi;
	int ssi_rxi;
	int ssi_eti;
	int ssi_eri;
	int ssi_rlen;
	int dma_tx_err;
	int dma_tx_end;
	int dma_rx_err;
	int dma_rx_end;
};
/* the max number of spi devices */
#define MAX_SPI_DEVICES				10
#define MAX_SPI_HOST				2

#define JZ_SPI_ID_INVALID(ssi_id) ( ((ssi_id) < 0) || ((ssi_id) > (MAX_SPI_HOST - 1)) )

#define MAX_SPI_CHIPSELECT_NUM 		MAX_GPIO_NUM


static inline void spi_writel(struct jz_spi *spi,
			      unsigned short offset, u32 value)
{
	writel(value, spi->iomem + offset);
}

static inline u32 spi_readl(struct jz_spi *spi,
				      unsigned short offset)
{
	return readl(spi->iomem + offset);
}

static inline void set_frmhl(struct jz_spi *spi, unsigned int frmhl)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp = (tmp & ~CR1_FRMHL_MASK) | frmhl;
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void set_spi_clock_phase(struct jz_spi *spi, unsigned int cpha)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp = (tmp & ~CR1_PHA) | (cpha ? CR1_PHA : 0);
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void set_spi_clock_polarity(struct jz_spi *spi,
					  unsigned int cpol)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp = (tmp & ~CR1_POL) | (cpol ? CR1_POL : 0);
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void set_tx_msb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_TENDIAN_MASK;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void set_tx_lsb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= (tmp & ~CR0_TENDIAN_MASK) | (0x2 << CR0_TENDIAN_BIT);
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void set_rx_msb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_RENDIAN_MASK;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void set_rx_lsb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= (tmp & ~CR0_RENDIAN_MASK) | (0x2 << CR0_RENDIAN_BIT);
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void enable_loopback(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_LOOP;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_loopback(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_LOOP;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void transmit_data(struct jz_spi *spi, u32 value)
{
	spi_writel(spi, SSI_DR, value);
}

static inline void set_frame_length(struct jz_spi *spi, u32 len)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp = (tmp & ~CR1_FLEN_MASK) | (((len) - 2) << CR1_FLEN_BIT);
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void set_tx_trigger(struct jz_spi *spi, u32 val)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp = (tmp & ~CR1_TTRG_MASK) | ((val)/8) << CR1_TTRG_BIT;
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void set_rx_trigger(struct jz_spi *spi, u32 val)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp = (tmp & ~CR1_RTRG_MASK) | ((val)/8) << CR1_RTRG_BIT;
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void enable_txfifo_half_empty_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_TIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_txfifo_half_empty_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_TIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void enable_rxfifo_half_full_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_RIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_rxfifo_half_full_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_RIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void enable_tx_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_TIE | CR0_TEIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_tx_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~(CR0_TIE | CR0_TEIE);
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void enable_rx_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_RIE | CR0_REIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_rx_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~(CR0_RIE | CR0_REIE);
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void enable_tx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_TEIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_tx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_TEIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void enable_rx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_REIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_rx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_REIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void underrun_auto_clear(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_EACLRUN;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void clear_errors(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_SR);
	tmp &= ~(SR_UNDR | SR_OVER);
	spi_writel(spi, SSI_SR, tmp);
}

static inline void set_spi_format(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp &= ~CR1_FMAT_MASK;
	tmp |= CR1_FMAT_SPI;
	tmp &= ~(CR1_TFVCK_MASK | CR1_TCKFI_MASK);
	tmp |= (CR1_TFVCK_0 | CR1_TCKFI_0);
//	tmp |= (CR1_TFVCK_1 | CR1_TCKFI_1);
//	tmp |= (CR1_TFVCK_2 | CR1_TCKFI_2);
//	tmp |= (CR1_TFVCK_3 | CR1_TCKFI_3);
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void enable_receive(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_DISREV;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void disable_receive(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_DISREV;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void flush_fifo(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_TFLUSH | CR0_RFLUSH;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void finish_transmit(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp &= ~CR1_UNFIN;
	spi_writel(spi, SSI_CR1, tmp);
}

static inline void start_transmit(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR1);
	tmp |= CR1_UNFIN;
	spi_writel(spi, SSI_CR1, tmp);
}

static inline int rxfifo_empty(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_RFE;
}

static inline int ssi_busy(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_BUSY;
}

static inline void ssi_disable(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_SSIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void ssi_enable(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_SSIE;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline u32 get_rxfifo_count(struct jz_spi *spi)
{
	return (spi_readl(spi, SSI_SR) & SR_RFIFONUM_MASK) >> SR_RFIFONUM_BIT;
}

static inline void flush_txfifo(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_TFLUSH;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void flush_rxfifo(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_RFLUSH;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline int ssi_underrun(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_UNDR;
}

static inline int ssi_overrun(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_OVER;
}

static inline int ssi_transfer_end(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_END;
}

static inline int tx_error_intr(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_CR0) & CR0_TEIE;
}

static inline int rx_error_intr(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_CR0) & CR0_REIE;
}

static inline int rxfifo_half_full(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_RFHF;
}

static inline int txfifo_half_empty(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_TFHE;
}

static inline int txfifo_half_empty_intr(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_CR0) & CR0_TIE;
}

static inline int rxfifo_half_full_intr(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_CR0) & CR0_RIE;
}

static inline void select_ce(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp &= ~CR0_FSEL;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void select_ce2(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SSI_CR0);
	tmp |= CR0_FSEL;
	spi_writel(spi, SSI_CR0, tmp);
}

static inline void dump_spi_reg(struct jz_spi *spi)
{
//	printk("SSI_DR	:%08x\n", spi_readl(spi, SSI_DR	));
	printk("SSI_CR0	:%08x\n", spi_readl(spi, SSI_CR0 ));
	printk("SSI_CR1	:%08x\n", spi_readl(spi, SSI_CR1 ));
	printk("SSI_SR	:%08x\n", spi_readl(spi, SSI_SR	));
	printk("SSI_ITR	:%08x\n", spi_readl(spi, SSI_ITR ));
	printk("SSI_ICR	:%08x\n", spi_readl(spi, SSI_ICR ));
	printk("SSI_GR	:%08x\n", spi_readl(spi, SSI_GR	));
	printk("SSI_RCNT:%08x\n", spi_readl(spi, SSI_RCNT));
}

#if 1
// 用sfc1 替换ssi
#if 1
#define SFC_GLB0					(0x0000)
#define SFC_DEV_CONF			(0x0004)
#define SFC_DEV_STA_EXP			(0x0008)
#define SFC_DEV_STA_RT			(0x000c)
#define SFC_DEV_STA_MSK			(0x0010)
#define SFC_TRAN_CONF(n)		(0x0014 + (n * 4))
#define SFC_DEV_ADDR(n)			(0x0030 + (n * 4))
#define SFC_DEV_ADDR_PLUS(n)	(0x0048 + (n * 4))
#define SFC_TRAN_LEN			(0x002c)
#define SFC_MEM_ADDR			(0x0060)
#define SFC_TRIG				(0x0064)
#define SFC_SR					(0x0068)
#define SFC_SCR					(0x006c)
#define SFC_INTC				(0x0070)
#define SFC_FSM					(0x0074)
#define SFC_CGE					(0x0078)
#define SFC_CMD_IDX				(0x007c)
#define SFC_COL_ADDR			(0x0080)
#define SFC_ROW_ADDR			(0x0084)
#define SFC_STA_ADDR0			(0x0088)
#define SFC_STA_ADDR1			(0x008c)
#define SFC_DES_ADDR			(0x0090)
#define SFC_GLB1				(0x0094)
#define SFC_DEV1_STA_RT			(0x0098)
#define	SFC_TRAN_CONF1(n)		(0x009c + (n * 4))
#define SFC_LUT					(0x0800)
#define SFC_DR					(0x1000)


//For SFC_GLB
#define GLB_POLL_TIME_OFFSET	(16)
#define GLB_POLL_TIME_MSK       (0xffff << GLB_POLL_TIME_OFFSET)
#define GLB_DES_EN				(1 << 15)
#define GLB_CDT_EN				(1 << 14)
#define	GLB_TRAN_DIR		    (1 << 13)
#define GLB_TRAN_DIR_OFFSET     (13)
#define GLB_TRAN_DIR_WRITE      (1)
#define GLB_TRAN_DIR_READ       (0)
#define	GLB_THRESHOLD_OFFSET	(7)
#define GLB_THRESHOLD_MSK		(0x3f << GLB_THRESHOLD_OFFSET)
#define GLB_OP_MODE			    (1 << 6)
#define GLB_OP_OFF				(6)
#define SLAVE_MODE				(0x0)
#define DMA_MODE				(0x1)
#define GLB_PHASE_NUM_OFFSET	(3)
#define GLB_PHASE_NUM_MSK		(0x7 << GLB_PHASE_NUM_OFFSET)
#define GLB_WP_EN			    (1 << 2)
#define GLB_BURST_MD_OFFSET		(0)
#define GLB_BURST_MD_MSK		(0x3 << GLB_BURST_MD_OFFSET)

//For SFC_DEV_CONF
#define DEV_CONF_STA_ENDIAN			(31) //T40 add
#define STA_ENDIAN_LSB				(0 << DEV_CONF_STA_ENDIAN)
#define STA_ENDIAN_MSB				(1 << DEV_CONF_STA_ENDIAN)
#define	DEV_CONF_SMP_DELAY_OFFSET	(16)
#define	DEV_CONF_SMP_DELAY_MSK		(0x1f << DEV_CONF_SMP_DELAY_OFFSET)
#define DEV_CONF_SMP_DELAY_0		(0)
#define DEV_CONF_SMP_DELAY_45		(1)
#define DEV_CONF_SMP_DELAY_90		(2)
#define DEV_CONF_SMP_DELAY_135		(3)
#define DEV_CONF_SMP_DELAY_180		(4)
#define DEV_CONF_SMP_DELAY_225		(5)
#define DEV_CONF_SMP_DELAY_270		(6)
#define DEV_CONF_SMP_DELAY_315		(7)
#define DEV_CONF_SMP_DELAY_1		(8)
#define DEV_CONF_SMP_DELAY_2		(16)
#define DEV_CONF_CMD_TYPE		    (1 << 15)
#define DEV_CONF_STA_TYPE_OFFSET	(13)
#define DEV_CONF_STA_TYPE_MSK		(1 << DEV_CONF_STA_TYPE_OFFSET)
#define DEV_CONF_THOLD_OFFSET		(11)
#define	DEV_CONF_THOLD_MSK		    (0x3 << DEV_CONF_THOLD_OFFSET)
#define DEV_CONF_TSETUP_OFFSET		(9)
#define DEV_CONF_TSETUP_MSK		    (0x0 << DEV_CONF_TSETUP_OFFSET)
#define DEV_CONF_TSH_OFFSET		    (5)
#define DEV_CONF_TSH_MSK			(0xf << DEV_CONF_TSH_OFFSET)
#define DEV_CONF_CPHA			    (1 << 4)
#define DEV_CONF_CPOL			    (1 << 3)
#define DEV_CONF_CEDL			    (1 << 2)
#define DEV_CONF_HOLDDL			    (1 << 1)
#define DEV_CONF_WPDL			    (1 << 0)

//For SFC_TRAN_CONF0
#define	TRAN_CONF0_CLK_OFFSET			(29)
#define	TRAN_CONF0_CLK_MSK				(0x7 << TRAN_CONF0_CLK_OFFSET)
#define TRAN_CONF0_CLK_MODE_SSS     	(0)
#define TRAN_CONF0_CLK_MODE_SSD     	(1)
#define TRAN_CONF0_CLK_MODE_SDS     	(2)
#define TRAN_CONF0_CLK_MODE_SDD     	(3)
#define TRAN_CONF0_CLK_MODE_DSS     	(4)
#define TRAN_CONF0_CLK_MODE_DSD     	(5)
#define TRAN_CONF0_CLK_MODE_DDS     	(6)
#define TRAN_CONF0_CLK_MODE_DDD     	(7)
#define	TRAN_CONF0_ADDR_WIDTH_OFFSET	(26)
#define	TRAN_CONF0_ADDR_WIDTH_MSK		(0x7 << TRAN_CONF0_ADDR_WIDTH_OFFSET)
#define	TRAN_CONF0_POLL_EN         		(1 << 25)
#define TRAN_CONF0_POLL_OFFSET			(25)
#define TRAN_CONF0_CMDEN_OFFSET			(24)
#define	TRAN_CONF0_CMD_EN         		(1 << 24)
#define	TRAN_CONF0_PHASE_FORMAT         (1 << 23)
#define TRAN_CONF0_FMAT_OFFSET			(23)
#define TRAN_CONF0_DMYBITS_OFFSET		(17)
#define TRAN_CONF0_DMYBITS_MSK			(0x3f << TRAN_CONF0_DMYBITS_OFFSET)
#define TRAN_CONF0_DATEEN_OFFSET		(16)
#define TRAN_CONF0_DATEEN				(1 << 16)
#define	TRAN_CONF0_CMD_OFFSET			(0)
#define	TRAN_CONF0_CMD_MSK				(0xffff << TRAN_CONF0_CMD_OFFSET)

/* For SFC_TRAN_CONF1 */
#define TRAN_CONF1_DATA_ENDIAN			(1 << 18)
#define TRAN_CONF1_DATA_ENDIAN_OFFSET	(18)
#define TRAN_CONF1_DATA_ENDIAN_LSB		(0)
#define TRAN_CONF1_DATA_ENDIAN_MSB		(1)
#define TRAN_CONF1_WORD_UNIT_OFFSET		(16)
#define TRAN_CONF1_WORD_UNIT_MSK		(3 << 16)
#define TRAN_CONF1_TRAN_MODE_OFFSET		(4)
#define TRAN_CONF1_TRAN_MODE_MSK		(0xf << TRAN_CONF1_TRAN_MODE_OFFSET)
#define TRAN_CONF1_SPI_STANDARD			(0x0)
#define TRAN_CONF1_SPI_DUAL				(0x1)
#define TRAN_CONF1_SPI_QUAD				(0x5)
#define TRAN_CONF1_SPI_IO_QUAD			(0x6)
#define TRAN_CONF1_SPI_OCTAL			(0x9)
#define TRAN_CONF1_SPI_IO_OCTAL			(0xa)
#define TRAN_CONF1_SPI_FULL_OCTAL		(0xb)

//For SFC_TRIG
#define TRIG_FLUSH			(1 << 2)
#define TRIG_STOP			(1 << 1)
#define TRIG_START			(1 << 0)

//For SFC_SR
#define FIFONUM_OFFSET	(16)
#define FIFONUM_MSK		(0x7f << FIFONUM_OFFSET)
#define BUSY_OFFSET		(5)
#define BUSY_MSK		(0x3 << BUSY_OFFSET)
#define END			    (1 << 4)
#define TRAN_REQ		(1 << 3)
#define RECE_REQ		(1 << 2)
#define OVER			(1 << 1)
#define UNDER			(1 << 0)

//For SFC_SCR
#define	CLR_END			(1 << 4)
#define CLR_TREQ		(1 << 3)
#define CLR_RREQ		(1 << 2)
#define CLR_OVER		(1 << 1)
#define CLR_UNDR		(1 << 0)

//For SFC_INTC
#define	MASK_END		(1 << 4)
#define MASK_TREQ		(1 << 3)
#define MASK_RREQ		(1 << 2)
#define MASK_OVER		(1 << 1)
#define MASK_UNDR		(1 << 0)

//SFC_CMD_IDX
#define CMD_IDX_MSK                     (0x3f)
#define LUT_DATAEN_MSK                  (0x1 << 31)
#define LUT_DATAEN_OFF                  (31)
#define LUT_DIR_MSK                     (0x1 << 30)
#define LUT_DIR_OFF                     (30)

/* For SFC_GLB */
#define GLB1_DQS_EN				(1 << 2)
#define GLB1_CHIP_SEL_OFFSET	(0)
#define GLB1_CHIP_SEL_MSK		(0x3)
#define GLB1_CHIP_SEL_0			(0)
#define GLB1_CHIP_SEL_1			(1)
#define GLB1_CHIP_SEL_01		(2)


//For SFC_FSM
#define FSM_AHB_OFFSET		(16)
#define FSM_AHB_MSK		    (0xf << FSM_AHB_OFFSET)
#define FSM_SPI_OFFSET		(11)
#define FSM_SPI_MSK		    (0x1f << FSM_SPI_OFFSET)
#define FSM_CLK_OFFSET		(6)
#define FSM_CLK_MSK		    (0xf << FSM_CLK_OFFSET)
#define FSM_DMAC_OFFSET		(3)
#define FSM_DMAC_MSK		(0x7 << FSM_DMAC_OFFSET)
#define FSM_RMC_OFFSET		(0)
#define FSM_RMC_MSK		    (0x7 << FSM_RMC_OFFSET)

//For SFC_CGE
#define CG_EN				(1 << 0)
#define SFC_FIFO_LEN		(63)

#define N_MAX				6

#define ENABLE				1
#define DISABLE				0

#define COM_CMD				1	// common cmd
#define POLL_CMD			2	// the cmd will poll the status of flash,ext: read status

#define DMA_OPS				1
#define CPU_OPS				0

#define TM_STD_SPI			0
#define TM_DI_DO_SPI		1
#define TM_DIO_SPI			2
#define TM_FULL_DIO_SPI		3
#define TM_QI_QO_SPI		5
#define TM_QIO_SPI			6
#define	TM_FULL_QIO_SPI		7
#define TM_OCTAL_SPT		9
#define TM_OCTAL_IO_SPI		10
#define TM_OCTAL_FULL_SPI	11

#define DEFAULT_ADDRSIZE	3

#define THRESHOLD			(1)

#define DEF_ADDR_LEN    	3
#define DEF_TCHSH       	5
#define DEF_TSLCH       	5
#define DEF_TSHSL_R     	20
#define DEF_TSHSL_W     	50
#endif

static inline void sfc_select_ce_low(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp &= ~DEV_CONF_CEDL;
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_select_ce_high(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp |= DEV_CONF_CEDL;
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_set_transferdir(struct jz_spi *spi, u32 value)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_GLB0);
	if(value & W_MODE){
		tmp |= GLB_TRAN_DIR;
	} else {
		tmp &= ~GLB_TRAN_DIR;
	}

	spi_writel(spi, SFC_GLB0, tmp);
}

static inline void sfc_transmit_data(struct jz_spi *spi, u32 value)
{
	printk("sfc dr value:%#x\n",value);
	spi_writel(spi, SFC_DR, value);
}

static inline void sfc_set_frame_length(struct jz_spi *spi, u32 len)
{
	u32 tmp;
	//对比普通外设来说，phase只需要配置1即可，flash需要配置4个，先按外设来调 按1个来
	tmp = spi_readl(spi, SFC_GLB0);
	tmp &= ~((1<<4) | (1<<5));
	tmp |= (1<<3);
	spi_writel(spi, SFC_GLB0,tmp);

	tmp = spi_readl(spi, SFC_TRAN_CONF1(0));
	switch(len) {
		case FIFO_W8:
			tmp &= ~(0x3 << 16);
			break;
		case FIFO_W16:
			tmp = (tmp & ~(0x3 << 16)) | (0x1 << 16);
			break;
		case FIFO_W32:
			tmp = (tmp & ~(0x3 << 16)) | (0x1 << 17);
			break;
		default:
			break;
	}
	spi_writel(spi, SFC_TRAN_CONF1(0), tmp);
}

static inline void sfc_set_tran_conf1(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi,SFC_TRAN_CONF1(0));
	// tmp = 0x1 << 17;
	tmp = 0x0 << 17;
	spi_writel(spi, SFC_TRAN_CONF1(0), tmp);
}

static inline void sfc_set_cmd_en(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_TRAN_CONF(0));
	tmp |= TRAN_CONF0_CMD_EN;
	spi_writel(spi, SFC_TRAN_CONF(0), tmp);
}

static inline void sfc_set_poll_en(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_TRAN_CONF(0));
	tmp |= TRAN_CONF0_POLL_EN;
	spi_writel(spi, SFC_TRAN_CONF(0), tmp);
}

static inline void sfc_set_cmd(struct jz_spi *spi,u32 value)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_TRAN_CONF(0));
	tmp &= ~0xffff;
	tmp |= (value & 0xffff);
	spi_writel(spi, SFC_TRAN_CONF(0), tmp);
}

static inline void sfc_set_tx_msb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp |= (0x1 << 31);
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_set_tx_lsb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp &= ~(0x1 << 31);
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_set_rx_msb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp |= (0x1 << 31);
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_set_rx_lsb(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp &= ~(0x1 << 31);
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_set_smp_delay(struct jz_spi *spi,u32 value)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp |= (value & 0x1f) << 16;
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_disable_tx_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= MASK_TREQ;//0x1f;
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_enable_tx_intr(struct jz_spi *spi)
{

	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~MASK_TREQ;//~(0x1f);//bit0~4 clear 0
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_enable_rx_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~MASK_RREQ;//~(0x1f);
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_disable_rx_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= MASK_RREQ;//0x1f;
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_disable_all_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= 0x1f;//0x1f;
	// tmp |= 0xf;//0x1f;
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_enable_all_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~0x1f;//0x1f;
	// tmp |= 0xf;//0x1f;
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_start_transmit(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_TRIG);
	tmp |= TRIG_START;
	spi_writel(spi, SFC_TRIG, tmp);
}

static inline void sfc_finish_transmit(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_TRIG);
	tmp |= TRIG_STOP;
	spi_writel(spi, SFC_TRIG, tmp);
}

static inline void sfc_flush_fifo(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_TRIG);
	tmp |= TRIG_FLUSH;
	spi_writel(spi, SFC_TRIG, tmp);
}

static inline void sfc_enable_receive(struct jz_spi *spi)
{
	u32 tmp;
	// tmp = spi_readl(spi, SFC_GLB0);
	// tmp &= ~GLB_TRAN_DIR;
	// spi_writel(spi, SFC_GLB0, tmp);

	tmp = spi_readl(spi, SFC_TRAN_CONF(0));
	tmp |= 1 << 16;
	spi_writel(spi, SFC_TRAN_CONF(0), tmp);

}

static inline void sfc_set_transfer_len(struct jz_spi *spi,u32 val)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_TRAN_LEN);
	tmp &= ~0xffffffff;
	tmp |= val;
	spi_writel(spi, SFC_TRAN_LEN, tmp);
}


static inline void sfc_clear_errors(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_SCR);
	tmp |= (CLR_UNDR | CLR_OVER | CLR_END);
	spi_writel(spi, SFC_SCR, tmp);
}

static inline void sfc_clear_treq(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_SCR);
	tmp |= CLR_TREQ;
	spi_writel(spi, SFC_SCR, tmp);
}

static inline void sfc_clear_rreq(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_SCR);
	tmp |= CLR_RREQ;
	spi_writel(spi, SFC_SCR, tmp);
}

static inline void sfc_set_tx_trigger(struct jz_spi *spi, u32 val)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_GLB0);
	tmp = (tmp & ~GLB_THRESHOLD_MSK) | ((val)/8) << GLB_THRESHOLD_OFFSET;
	spi_writel(spi, SFC_GLB0, tmp);
}

static inline void sfc_set_rx_trigger(struct jz_spi *spi, u32 val)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_GLB0);
	tmp = (tmp & ~GLB_THRESHOLD_MSK) | ((val)/8) << GLB_THRESHOLD_OFFSET;
	spi_writel(spi, SFC_GLB0, tmp);
}

static inline void sfc_set_wp_en(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_GLB0);
	tmp |= 0x1 << 2;
	spi_writel(spi, SFC_GLB0, tmp);
}

static inline void sfc_set_glb1(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_GLB1);
	tmp = 0;
	spi_writel(spi, SFC_GLB1, tmp);
}

static inline void sfc_set_sta_exp(struct jz_spi *spi,u32 value)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_STA_EXP);
	tmp |= value;
	spi_writel(spi, SFC_DEV_STA_EXP, tmp);
}

static inline void sfc_set_sta_mask(struct jz_spi *spi,u32 value)
{
	u32 tmp;
	tmp = spi_readl(spi,SFC_DEV_STA_MSK);
	tmp |= value;
	spi_writel(spi, SFC_DEV_STA_MSK, tmp);
}


static inline void sfc_enable_tx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~(MASK_TREQ );
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_enable_rx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~(MASK_RREQ);
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_disable_tx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= (MASK_TREQ );
	spi_writel(spi, SFC_INTC, tmp);
}


static inline void sfc_disable_rx_error_intr(struct jz_spi *spi)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= (MASK_RREQ);
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_enable_txfifo_half_empty_intr(struct jz_spi *spi)
{
	//半中断的意思？ 难道是数据到达fifo的一半产生中断？
	//半中断的半并不是一半的含义，而是当fifo空间大于阈值时触发rx中断，当fifo中填充的空间大于阈值时，触发tx中断，等同于sfc的req寄存器
	// sfc只有full 中断，无empty中断
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~(MASK_TREQ);
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_enable_fifo_half_empty_intr(struct jz_spi *spi)
{
	// tx rx共用fifo，只用fifo表示，empty表示rreq
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~(MASK_TREQ);
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_disable_fifo_half_empty_intr(struct jz_spi *spi)
{
	// tx rx共用fifo，只用fifo表示，empty表示rreq
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= MASK_TREQ;
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_enable_fifo_half_full_intr(struct jz_spi *spi)
{
	// tx rx共用fifo，只用fifo表示，full表示r req
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp &= ~(MASK_RREQ);
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_disable_fifo_half_full_intr(struct jz_spi *spi)
{
	// tx rx共用fifo，只用fifo表示，half表示rreq
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= MASK_RREQ;
	spi_writel(spi, SFC_INTC, tmp);
}

static inline void sfc_disable_end_intr(struct jz_spi *spi)
{
	// tx rx共用fifo，只用fifo表示，half表示rreq
	u32 tmp;
	tmp = spi_readl(spi, SFC_INTC);
	tmp |= MASK_END;
	spi_writel(spi, SFC_INTC, tmp);
}


static inline int sfc_rxfifo_half_full(struct jz_spi *spi)
{
	return spi_readl(spi, SFC_SR) & SR_RFHF;
}

static inline int sfc_txfifo_half_empty(struct jz_spi *spi)
{
	return spi_readl(spi, SFC_SR) & SR_TFHE;
}

// static inline int sfc_clr_txfifo(struct jz_spi *spi)
// {
// 	return spi_readl(spi, SFC_SR) & SR_TFHE;
// }

static inline u32 sfc_get_txrxfifo_count(struct jz_spi *spi)
{
	return (spi_readl(spi, SFC_SR) & (0x7f << FIFONUM_OFFSET)) >> FIFONUM_OFFSET;
}

static inline int sfc_ssi_underrun(struct jz_spi *spi)
{
	return spi_readl(spi, SFC_SR) & MASK_UNDR;
}

static inline int sfc_ssi_end(struct jz_spi *spi)
{
	return spi_readl(spi, SFC_SR) & MASK_END;
}

static inline int sfc_ssi_fifo(struct jz_spi *spi)
{
	return spi_readl(spi, SFC_SR) & (0x1<<FIFONUM_OFFSET);
}

static inline int sfc_ssi_overrun(struct jz_spi *spi)
{
	return spi_readl(spi, SFC_SR) & MASK_OVER;
}

static inline int sfc_rxfifo_empty(struct jz_spi *spi)
{
	return spi_readl(spi, SSI_SR) & SR_RFE;
}

static inline void sfc_set_spi_clock_phase(struct jz_spi *spi, unsigned int cpha)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp = (tmp & ~DEV_CONF_CPHA) | (cpha ? DEV_CONF_CPHA : 0);
	spi_writel(spi, SFC_DEV_CONF, tmp);
}

static inline void sfc_set_spi_clock_polarity(struct jz_spi *spi,
					  unsigned int cpol)
{
	u32 tmp;
	tmp = spi_readl(spi, SFC_DEV_CONF);
	tmp = (tmp & ~DEV_CONF_CPOL) | (cpol ? DEV_CONF_CPOL : 0);
	spi_writel(spi, SFC_DEV_CONF, tmp);
}


// static inline int sfc_tx_error_intr(struct jz_spi *spi)
// {
// 	return spi_readl(spi, SSI_CR0) & CR0_TEIE;
// }

#endif

#endif /* __LINUX_SPI_JZ_H */
