/*
 * Platform device support for Jz4780 SoC.
 *
 * Copyright 2007, <zpzhong@ingenic.cn>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/i2c-gpio.h>

#include <gpio.h>

#include <soc/gpio.h>
#include <soc/base.h>
#include <soc/irq.h>

#include <mach/platform.h>
#include <mach/jzdma.h>
#include <mach/jzsnd.h>

#include <mach/txx-funcs.h>

#include <linux/mfd/jz_tcu.h>

//#include <mach/jznand.h>

/* device IO define array */
struct jz_gpio_func_def platform_devio_array[] = {
#if 0
#ifdef CONFIG_MMC_SDHCI_MMC0_PB_4BIT
	MSC0_PORTB,
#endif
#ifdef CONFIG_MMC_SDHCI_MMC1_PA_4BIT
	MSC1_PORTA,
#endif
#ifdef CONFIG_MMC_SDHCI_MMC1_PB_4BIT
	MSC1_PORTB,
#endif
#ifdef CONFIG_MMC_SDHCI_MMC1_PC_4BIT
	MSC1_PORTC,
#endif
#endif
#ifdef CONFIG_I2C0_PA12_PA13
	I2C0_PORTA,
#endif
#ifdef CONFIG_I2C1_PA14_PA15
	I2C1_PORTA,
#endif
#ifdef CONFIG_I2C1_PB01_PB02
	I2C1_PORTB,
#endif
#ifdef CONFIG_I2C1_PC06_PC07
	I2C1_PORTC,
#endif
#ifdef CONFIG_I2C2_PB25_PB26
	I2C2_PORTB,
#endif
#ifdef CONFIG_I2C2_PA10_PA11
	I2C2_PORTA_1,
#endif
#ifdef CONFIG_I2C2_PA18_PA19
	I2C2_PORTA_2,
#endif
#ifdef CONFIG_I2C2_PC08_PC09
	I2C2_PORTC,
#endif

#ifdef CONFIG_SERIAL_UART0_PB
	UART0_PORTB,
#endif
#ifdef CONFIG_SERIAL_UART0_PC
	UART0_PORTC,
#endif
#ifdef CONFIG_SERIAL_UART1
	UART1_PORTB,
#endif
#ifdef CONFIG_SERIAL_UART2_PB
	UART2_PORTB,
#endif
#ifdef CONFIG_SERIAL_UART2_PA
	UART2_PORTA,
#endif
#ifdef CONFIG_JZ_PWM_GPIO_B17
	PWM_PORTB_BIT17,
#endif
#ifdef CONFIG_JZ_PWM_GPIO_B18
	PWM_PORTB_BIT18,
#endif
#ifdef CONFIG_JZ_PWM_GPIO_C8
	PWM_PORTC_BIT8,
#endif
#ifdef CONFIG_JZ_PWM_GPIO_C9
	PWM_PORTC_BIT9,
#endif

#ifdef	CONFIG_SOUND_JZ_SPDIF_V12
	I2S_PORTDE,
#endif

#ifdef CONFIG_USB_DWC2_DRVVBUS_FUNCTION_PIN
	OTG_DRVVUS,
#endif

#ifdef CONFIG_JZ_SPI0_PC
	SSI0_PORTC,
#endif

#ifdef CONFIG_JZ_SFC1_SSI_PC
	SFC1_PORTC_1,
#endif

#ifdef CONFIG_JZ_SPI0_PB
	SSI0_PORTB,
#endif

/* The board of PRJ008 CYGNET uses spi1 and spislave to loop test */
#ifdef CONFIG_JZ_SPI_SPISLV_LOOPTEST
	SSI1_PORTB_2,
#endif

#ifdef CONFIG_JZ_SPISLV_PA
	SSISLV_PORTA,
#endif
#ifdef CONFIG_JZ_SPISLV_PC
	SSISLV_PORTC,
#endif
#ifdef CONFIG_JZ_SPISLV_PC_1
	SSISLV_PORTC_1,
#endif

#ifdef CONFIG_JZ_SFC0_PA
#ifdef CONFIG_BOARD_VELA
	SFC0_SINGLE_PORTA,
#else
	SFC0_PORTA,
#endif
#endif

#ifdef CONFIG_JZ_SFC1_PC1
	SFC1_PORTC_1,
#endif

#ifdef CONFIG_JZ_SFC1_PC2
	SFC1_PORTC_2,
#endif

#ifdef CONFIG_JZ_MAC
	GMAC_PORTB,
#endif
#ifdef CONFIG_FPGA_SLCD_TRULY_240_240
        DPU_PORTD_SLCD_8BIT,
#endif

};

int platform_devio_array_size = ARRAY_SIZE(platform_devio_array);

int jz_device_register(struct platform_device *pdev, void *pdata)
{
	pdev->dev.platform_data = pdata;

	return platform_device_register(pdev);
}

static struct resource jz_pdma_res[] = {
	[0] = {
	       .flags = IORESOURCE_MEM,
	       .start = PDMA_IOBASE,
	       .end = PDMA_IOBASE + 0x10000 - 1,
	       },
	[1] = {
	       .name = "irq",
	       .flags = IORESOURCE_IRQ,
	       .start = IRQ_PDMA,
	       },
	[2] = {
	       .name = "pdmam",
	       .flags = IORESOURCE_IRQ,
	       .start = IRQ_PDMAM,
	       },
	[3] = {
	       .name = "mcu",
	       .flags = IORESOURCE_IRQ,
	       .start = IRQ_MCU,
	       },
};

static struct jzdma_platform_data jzdma_pdata = {
	.irq_base = IRQ_MCU_BASE,
	.irq_end = IRQ_MCU_END,
	.map = {
#ifdef CONFIG_AUTO_DMA_MODE
		JZDMA_REQ_AUTO,
#endif

#ifdef CONFIG_AIC_DMA_MODE
		JZDMA_REQ_AEC,
		JZDMA_REQ_I2S0,
		JZDMA_REQ_I2S0,
#endif

#ifdef CONFIG_SSLV_DMA_MODE
		JZDMA_REQ_SSLV,
		JZDMA_REQ_SSLV,
#endif

#ifdef CONFIG_UART1_DMA_MODE
		JZDMA_REQ_UART1,
		JZDMA_REQ_UART1,
#endif

#ifdef CONFIG_UART0_DMA_MODE
		JZDMA_REQ_UART0,
		JZDMA_REQ_UART0,
#endif

#ifdef CONFIG_UART2_DMA_MODE
		JZDMA_REQ_UART2,
		JZDMA_REQ_UART2,
#endif

#ifdef CONFIG_SSI0_DMA_MODE
		JZDMA_REQ_SSI0,
		JZDMA_REQ_SSI0,
#endif

#ifdef CONFIG_I2C0_DMA_MODE
		JZDMA_REQ_I2C0,
		JZDMA_REQ_I2C0,
#endif

#ifdef CONFIG_I2C1_DMA_MODE
		JZDMA_REQ_I2C1,
		JZDMA_REQ_I2C1,
#endif

#ifdef CONFIG_I2C2_DMA_MODE
		JZDMA_REQ_I2C2,
		JZDMA_REQ_I2C2,
#endif

#ifdef CONFIG_AES0_DMA_MODE
		JZDMA_REQ_AES0,
#endif

#ifdef CONFIG_AES1_DMA_MODE
		JZDMA_REQ_AES1,
#endif

#ifdef CONFIG_HASH_DMA_MODE
		JZDMA_REQ_HASH,
#endif

#ifdef CONFIG_SADC_DMA_MODE
		JZDMA_REQ_SADC,
#endif
		},
};

struct platform_device jz_pdma_device = {
	.name = "jz-dma",
	.id = -1,
	.dev = {
		.platform_data = &jzdma_pdata,
		},
	.resource = jz_pdma_res,
	.num_resources = ARRAY_SIZE(jz_pdma_res),
};

#ifdef CONFIG_CRYPTO_DEV_INGENIC_SHA
static struct resource jz_hash_resources[] = {
	[0] = {
		.start  = HASH_IOBASE,
		.end    = HASH_IOBASE + 0x20,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_HASH,
		.end    = IRQ_HASH,
		.flags  = IORESOURCE_IRQ,
	},
};
struct platform_device jz_hash_device = {
	.name   = "ingenic-crypto-hash",
	.id = 0,
	.resource   = jz_hash_resources,
	.num_resources  = ARRAY_SIZE(jz_hash_resources),
};
#endif

/* AES controller*/
static struct resource jz_aes_resources[] = {
	[0] = {
		.start  = AES_IOBASE,
		.end    = AES_IOBASE + 0x28,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_AES,
		.end    = IRQ_AES,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device jz_aes_device = {
	.name   = "jz-aes",
	.id = 0,
	.resource   = jz_aes_resources,
	.num_resources  = ARRAY_SIZE(jz_aes_resources),
};

#ifdef CONFIG_JZ_WDT
/* WDT controller*/
static struct resource jz_wdt_resources[] = {
	[0] = {
		.start  = WDT_IOBASE,
		.end    = WDT_IOBASE + 0xC,
		.flags  = IORESOURCE_MEM,
	},
};

struct platform_device jz_wdt_device = {
	.name   = "jz-wdt",
	.id = 0,
	.resource   = jz_wdt_resources,
	.num_resources  = ARRAY_SIZE(jz_wdt_resources),
};
#endif

/* DES controller*/
static struct resource jz_des_resources[] = {
	[0] = {
		.start  = DES_IOBASE,
		.end    = DES_IOBASE + 0x38,
		.flags  = IORESOURCE_MEM,
	},
};

struct platform_device jz_des_device = {
	.name   = "jz-des",
	.id = 0,
	.resource   = jz_des_resources,
	.num_resources  = ARRAY_SIZE(jz_des_resources),
};

/* ADC controller*/
static struct resource jz_adc_resources[] = {
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

struct platform_device jz_adc_device = {
	.name   = "jz-adc",
	.id = -1,
	.num_resources  = ARRAY_SIZE(jz_adc_resources),
	.resource   = jz_adc_resources,
};

/* MSC ( msc controller v1.2) */
static u64 jz_msc_dmamask = ~(u32) 0;

#define DEF_MSC(NO)							\
	static struct resource jz_msc##NO##_resources[] = {		\
		{							\
			.start          = MSC##NO##_IOBASE,		\
			.end            = MSC##NO##_IOBASE + 0x1000 - 1, \
			.flags          = IORESOURCE_MEM,		\
		},							\
		{							\
			.start          = IRQ_MSC##NO,			\
			.end            = IRQ_MSC##NO,			\
			.flags          = IORESOURCE_IRQ,		\
		},							\
	};								\
	struct platform_device jz_msc##NO##_device = {                  \
		.name = "jz-sdhci",					\
		.id = NO,						\
		.dev = {						\
			.dma_mask               = &jz_msc_dmamask,	\
			.coherent_dma_mask      = 0xffffffff,		\
		},							\
		.resource       = jz_msc##NO##_resources,               \
		.num_resources  = ARRAY_SIZE(jz_msc##NO##_resources),	\
	};
DEF_MSC(0);
DEF_MSC(1);

/*vpu irq*/
#ifdef CONFIG_JZ_VPU_IRQ_TEST
static struct resource jz_vpu_irq_resources[] = {			\
	[0] = {								\
		.start          = HELIX_IOBASE_UNIT(0),			\
		.end            = HELIX_IOBASE_UNIT(0) + 0x10000 - 1,	\
		.flags          = IORESOURCE_MEM,			\
	},								\
	[1] = {								\
		.start          = RADIX_IOBASE_UNIT(0),			\
		.end            = RADIX_IOBASE_UNIT(0) + 0xf0000 - 1,	\
		.flags          = IORESOURCE_MEM,			\
	},								\
	[2] = {								\
		.start          = IRQ_HELIX0,			\
		.end            = IRQ_HELIX0,			\
		.flags          = IORESOURCE_IRQ,			\
	},								\
};									\

struct platform_device jz_vpu_irq_device = {					\
	.name = "jz-vpu-irq",							\
	.id = 0,								\
	.dev = {								\
		.coherent_dma_mask      = 0xffffffff,				\
	},									\
	.num_resources  = ARRAY_SIZE(jz_vpu_irq_resources),			\
	.resource       = jz_vpu_irq_resources,				\
};
#elif defined(CONFIG_SOC_VPU) && (defined(CONFIG_VPU_HELIX) || defined(CONFIG_VPU_RADIX))
static u64 jz_vpu_dmamask = ~(u64)0;
#define DEF_VPU(UNAME, LNAME, NO)								\
	static struct resource jz_vpu_##LNAME##NO##_resources[] = {			\
		[0] = {								\
			.start          = UNAME##_IOBASE_UNIT(NO),			\
			.end            = UNAME##_IOBASE_UNIT(NO) + 0xf0000 - 1,	\
			.flags          = IORESOURCE_MEM,			\
		},								\
		[1] = {								\
			.start          = IRQ_##UNAME##NO,			\
			.end            = IRQ_##UNAME##NO,			\
			.flags          = IORESOURCE_IRQ,			\
		},								\
	};									\
struct platform_device jz_vpu_##LNAME##NO##_device = {					\
	.name = #LNAME,							\
	.id = NO,								\
	.dev = {								\
		.dma_mask               = &jz_vpu_dmamask,			\
		.coherent_dma_mask      = 0xffffffff,				\
	},									\
	.num_resources  = ARRAY_SIZE(jz_vpu_##LNAME##NO##_resources),			\
	.resource       = jz_vpu_##LNAME##NO##_resources,				\
};

#ifdef CONFIG_VPU_HELIX
#if (CONFIG_VPU_HELIX_NUM >= 1)
DEF_VPU(HELIX, helix, 0);
#endif
#endif
#ifdef CONFIG_VPU_RADIX
#if (CONFIG_VPU_RADIX_NUM >= 1)
DEF_VPU(RADIX, radix, 0);
#endif
#endif
#endif

/* ipu */
static u64 jz_ipu_dmamask = ~(u64) 0;
static struct resource jz_ipu_resources[] = {
	[0] = {
		.start = IPU_IOBASE,
		.end = IPU_IOBASE + 0x8000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_IPU,
		.end = IRQ_IPU,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device jz_ipu_device = {
	.name = "jz-ipu",
	.id = 0,
	.dev = {
		.dma_mask = &jz_ipu_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = ARRAY_SIZE(jz_ipu_resources),
	.resource = jz_ipu_resources,
};

/* i2d */
static u64 jz_i2d_dmamask = ~(u64) 0;
static struct resource jz_i2d_resources[] = {
	[0] = {
		.start = I2D_IOBASE,
		.end = I2D_IOBASE + 0x8000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2D,
		.end = IRQ_I2D,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device jz_i2d_device = {
	.name = "jz-i2d",
	.id = 0,
	.dev = {
		.dma_mask = &jz_i2d_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = ARRAY_SIZE(jz_i2d_resources),
	.resource = jz_i2d_resources,
};

/*DWC OTG*/
static struct resource jz_dwc_otg_resources[] = {
	[0] = {
		.start = OTG_IOBASE,
		.end = OTG_IOBASE + 0x40000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.flags = IORESOURCE_IRQ,
		.start = IRQ_OTG,
		.end = IRQ_OTG,
	},
};

struct platform_device jz_dwc_otg_device = {
	.name = "jz-dwc2",
	.id = -1,
	.num_resources = ARRAY_SIZE(jz_dwc_otg_resources),
	.resource = jz_dwc_otg_resources,
};

/* UART ( uart controller) */
static struct resource jz_uart0_resources[] = {
	[0] = {
		.start = UART0_IOBASE,
		.end = UART0_IOBASE + 0x1000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UART0,
		.end = IRQ_UART0,
		.flags = IORESOURCE_IRQ,
	},
#ifdef CONFIG_SERIAL_UART0_DMA
	[2] = {
		.start = JZDMA_REQ_UART0,
		.flags = IORESOURCE_DMA,
	},
#endif
};

struct platform_device jz_uart0_device = {
	.name = "jz-uart",
	.id = 0,
	.num_resources = ARRAY_SIZE(jz_uart0_resources),
	.resource = jz_uart0_resources,
};

static struct resource jz_uart1_resources[] = {
	[0] = {
		.start = UART1_IOBASE,
		.end = UART1_IOBASE + 0x1000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UART1,
		.end = IRQ_UART1,
		.flags = IORESOURCE_IRQ,
	},
#ifdef CONFIG_SERIAL_UART1_DMA
	[2] = {
		.start = JZDMA_REQ_UART1,
		.flags = IORESOURCE_DMA,
	},
#endif
};

struct platform_device jz_uart1_device = {
	.name = "jz-uart",
	.id = 1,
	.num_resources = ARRAY_SIZE(jz_uart1_resources),
	.resource = jz_uart1_resources,
};

static struct resource jz_uart2_resources[] = {
	[0] = {
		.start = UART2_IOBASE,
		.end = UART2_IOBASE + 0x1000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UART2,
		.end = IRQ_UART2,
		.flags = IORESOURCE_IRQ,
	},
#ifdef CONFIG_SERIAL_UART2_DMA
	[2] = {
		.start = JZDMA_REQ_UART2,
		.flags = IORESOURCE_DMA,
	},
#endif
};

struct platform_device jz_uart2_device = {
	.name = "jz-uart",
	.id = 2,
	.num_resources = ARRAY_SIZE(jz_uart2_resources),
	.resource = jz_uart2_resources,
};

#if (defined(CONFIG_I2C0_V12_JZ) || defined(CONFIG_I2C1_V12_JZ) || defined(CONFIG_I2C2_V12_JZ))
static u64 jz_i2c_dmamask = ~(u32) 0;

#define DEF_I2C(NO)							\
	static struct resource jz_i2c##NO##_resources[] = {		\
		[0] = {							\
			.start          = I2C##NO##_IOBASE,		\
			.end            = I2C##NO##_IOBASE + 0x1000 - 1, \
			.flags          = IORESOURCE_MEM,		\
		},							\
		[1] = {							\
			.start          = IRQ_I2C##NO,			\
			.end            = IRQ_I2C##NO,			\
			.flags          = IORESOURCE_IRQ,		\
		},							\
		[2] = {							\
			.start          = JZDMA_REQ_I2C##NO,		\
			.flags          = IORESOURCE_DMA,		\
		},							\
		[3] = {							\
			.start          = CONFIG_I2C##NO##_SPEED,	\
			.flags          = IORESOURCE_BUS,		\
		},							\
	};								\
	struct platform_device jz_i2c##NO##_device = {                  \
		.name = "jz-i2c",					\
		.id = NO,						\
		.dev = {						\
			.dma_mask               = &jz_i2c_dmamask,	\
			.coherent_dma_mask      = 0xffffffff,		\
		},							\
		.num_resources  = ARRAY_SIZE(jz_i2c##NO##_resources),	\
		.resource       = jz_i2c##NO##_resources,		\
	};
#ifdef CONFIG_I2C0_V12_JZ
DEF_I2C(0);
#endif
#ifdef CONFIG_I2C1_V12_JZ
DEF_I2C(1);
#endif
#ifdef CONFIG_I2C2_V12_JZ
DEF_I2C(2);
#endif
#endif

#if defined(CONFIG_SPI1_PIO_ONLY) || defined(CONFIG_SPI0_PIO_ONLY) || defined(CONFIG_JZ_SFC1_SSI)
#define DEF_PIO_SSI(NO)							\
	static struct resource jz_ssi##NO##_resources[] = {		\
		[0] = {							\
			.flags	       = IORESOURCE_MEM,		\
			.start	       = SSI##NO##_IOBASE,		\
			.end	       = SSI##NO##_IOBASE + 0x1000 - 1,	\
		},							\
		[1] = {							\
			.flags	       = IORESOURCE_IRQ,		\
			.start	       = IRQ_SSI##NO,			\
			.end	       = IRQ_SSI##NO,			\
		},							\
	};								\
	struct platform_device jz_ssi##NO##_device = {			\
		.name = "jz-ssi",					\
		.id = NO,						\
		.resource       = jz_ssi##NO##_resources,		\
		.num_resources  = ARRAY_SIZE(jz_ssi##NO##_resources),	\
	};
#define DEF_PIO_SFC1_SSI(NO)							\
	static struct resource jz_ssi##NO##_resources[] = {		\
		[0] = {							\
			.flags	       = IORESOURCE_MEM,		\
			.start	       = SFC1_IOBASE,		\
			.end	       = SFC1_IOBASE + 0x1000 - 1,	\
		},							\
		[1] = {							\
			.flags	       = IORESOURCE_IRQ,		\
			.start	       = IRQ_SFC1/*IRQ_SSI##NO*/,			\
			.end	       = IRQ_SFC1/*IRQ_SSI##NO*/,			\
		},							\
	};								\
	struct platform_device jz_ssi##NO##_device = {			\
		.name = "jz-ssi",					\
		.id = NO,						\
		.resource       = jz_ssi##NO##_resources,		\
		.num_resources  = ARRAY_SIZE(jz_ssi##NO##_resources),	\
	};
#ifdef CONFIG_JZ_SPI1
DEF_PIO_SSI(1);
#endif
#ifdef CONFIG_JZ_SPI0
DEF_PIO_SSI(0);
#endif
#if defined (CONFIG_JZ_SFC1_SSI) && !defined(CONFIG_SFC1)
DEF_PIO_SFC1_SSI(0);
#endif
#else
#if defined(CONFIG_JZ_SPI0) || defined(CONFIG_JZ_SPI1)
static u64 jz_ssi_dmamask =  ~(u32)0;
#endif
#define DEF_SSI(NO)							\
	static struct resource jz_ssi##NO##_resources[] = {		\
		[0] = {							\
			.flags	       = IORESOURCE_MEM,		\
			.start	       = SSI##NO##_IOBASE,		\
			.end	       = SSI##NO##_IOBASE + 0x1000 - 1,	\
		},							\
		[1] = {							\
			.flags	       = IORESOURCE_IRQ,		\
			.start	       = IRQ_SSI##NO,			\
			.end	       = IRQ_SSI##NO,			\
		},							\
		[2] = {							\
			.flags	       = IORESOURCE_DMA,		\
			.start	       = JZDMA_REQ_SSI##NO,		\
		},							\
	};								\
	struct platform_device jz_ssi##NO##_device = {			\
		.name = "jz-ssi",					\
		.id = NO,						\
		.dev = {						\
			.dma_mask	       = &jz_ssi_dmamask,	\
			.coherent_dma_mask	= 0xffffffff,		\
		},							\
		.resource       = jz_ssi##NO##_resources,		\
		.num_resources  = ARRAY_SIZE(jz_ssi##NO##_resources),	\
	};
#ifdef CONFIG_JZ_SPI1
DEF_SSI(1);
#endif
#ifdef CONFIG_JZ_SPI0
DEF_SSI(0);
#endif
#ifdef CONFIG_JZ_SPI_SPISLV_LOOPTEST
static u64 jz_ssi_dmamask =  ~(u32)0;
DEF_SSI(1);
#endif
#endif

#ifdef CONFIG_JZ_SPI_SLAVE
static u64 jz_ssi_slv_dmamask =  ~(u32)0;
static struct resource jz_spi_slave_resources[] = {
	{
		.flags = IORESOURCE_MEM,
		.start = SSISLV_IOBASE,
		.end   = SSISLV_IOBASE + 0x1000 - 1,
	},
	{
		.flags = IORESOURCE_IRQ,
		.start = IRQ_SSISLV,
		.end   = IRQ_SSISLV,
	},
	{
		.flags = IORESOURCE_DMA,
		.start = JZDMA_REQ_SSLV_TX,
		.end   = JZDMA_REQ_SSLV_RX,
	},
};

struct platform_device jz_spi_slave_device = {
	.name = "jz_spi_slave",
	.id = -1,
	.dev = {
		.dma_mask	       = &jz_ssi_slv_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.resource = jz_spi_slave_resources,
	.num_resources = ARRAY_SIZE(jz_spi_slave_resources),
};
#endif

#ifdef CONFIG_RTC_DRV_JZ
static struct resource jz_rtc_resource[] = {
	[0] = {
		.start = RTC_IOBASE,
		.end   = RTC_IOBASE + 0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_RTC,
		.end   = IRQ_RTC,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device jz_rtc_device = {
	.name             = "jz-rtc",
	.id               = 0,
	.num_resources    = ARRAY_SIZE(jz_rtc_resource),
	.resource         = jz_rtc_resource,
};
#endif

#ifdef CONFIG_JZ_EFUSE_V13
/* efuse */
struct platform_device jz_efuse_device = {
       .name = "jz-efuse-v13",
};
#endif

#if defined(CONFIG_INGENIC_SFC_V2) || defined(CONFIG_INGENIC_SFC_V2_MODULE)
#ifdef CONFIG_SFC0
static struct resource jz_sfc0_resources[] = {
	[0] = {
		.start = SFC0_IOBASE,
		.end = SFC0_IOBASE + 0x1000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SFC0,
		.end = IRQ_SFC0,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = CONFIG_SFC0_SPEED,
		.flags = IORESOURCE_BUS,
	},
};
struct platform_device jz_sfc0_device = {
	.name = "jz-sfc",
	.id = 0,
	.num_resources = ARRAY_SIZE(jz_sfc0_resources),
	.resource = jz_sfc0_resources,
};
#endif
#ifdef CONFIG_SFC1
static struct resource jz_sfc1_resources[] = {
	[0] = {
		.start = SFC1_IOBASE,
		.end = SFC1_IOBASE + 0x1000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SFC1,
		.end = IRQ_SFC1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = CONFIG_SFC1_SPEED,
		.flags = IORESOURCE_BUS,
	},
};
struct platform_device jz_sfc1_device = {
	.name = "jz-sfc",
	.id = 1,
	.num_resources = ARRAY_SIZE(jz_sfc1_resources),
	.resource = jz_sfc1_resources,
};
#endif
#endif

#ifdef CONFIG_PWM_INGENIC
static struct resource jz_pwm_resources[] = {
	{
		.flags = IORESOURCE_MEM,
		.start = PWM_IOBASE,
		.end   = PWM_IOBASE + 0x1000 - 1,
	},
};
struct platform_device jz_pwm_device = {
	.name = "ingenic,PRJ008-pwm",
	.id = -1,
	.dev = {
		.platform_data = NULL,
	},
	.resource = jz_pwm_resources,
	.num_resources = ARRAY_SIZE(jz_pwm_resources),
};
#endif

#ifdef CONFIG_MFD_JZ_TCU
static struct resource jz_tcu_resources[] = {
	{
		.flags = IORESOURCE_MEM,
		.start = TCU_IOBASE,
		.end   = TCU_IOBASE + 0x1000 - 1,
	},
	{
		.flags = IORESOURCE_IRQ,
		.start = IRQ_TCU0,
		.end   = IRQ_TCU0,
	},
	{
		.flags  = IORESOURCE_IRQ,
		.start  = IRQ_TCU_BASE,
		.end    = IRQ_TCU_BASE,
	},
};

struct platform_device jz_tcu_device = {
	.name = "jz-tcu",
	.id = -1,
	.dev = {
		.platform_data = NULL,
	},
	.resource = jz_tcu_resources,
	.num_resources = ARRAY_SIZE(jz_tcu_resources),
};
#endif

unsigned long rmem_base = 0;
EXPORT_SYMBOL(rmem_base);

unsigned long rmem_size = 0;
EXPORT_SYMBOL(rmem_size);
unsigned long ispmem_base = 0;
EXPORT_SYMBOL(ispmem_base);

unsigned long ispmem_size = 0;
EXPORT_SYMBOL(ispmem_size);

static int __init ispmem_parse(char *str)
{
	char *retptr;

	ispmem_size = memparse(str, &retptr);
	if(ispmem_size < 0) {
		ispmem_size = 0;
	}

	if (*retptr == '@')
		ispmem_base = memparse(retptr + 1, NULL);

	if(ispmem_base < 0) {
		printk("## no ispmem! ##\n");
	}
	return 1;
}

static int __init rmem_parse(char *str)
{
	char *retptr;

	rmem_size = memparse(str, &retptr);
	if(rmem_size < 0) {
		rmem_size = 0;
	}

	if (*retptr == '@')
		rmem_base = memparse(retptr + 1, NULL);

	if(rmem_base < 0) {
		printk("## no rmem! ##\n");
	}
	return 1;
}
__setup("rmem=", rmem_parse);
__setup("ispmem=", ispmem_parse);

static void get_isp_priv_mem(unsigned int *phyaddr, unsigned int *size)
{
	*phyaddr = ispmem_base;
	*size = ispmem_size;
}

#ifndef CONFIG_PROC_FS
#error NOT config procfs
#endif

static struct task_struct *mythread_run(int (*threadfn)(void *data),
					   void *data, const char namefmt[])
{
	return kthread_run(threadfn, data, namefmt);
}

static void *mykmalloc(size_t s, gfp_t gfp)
{
	return kmalloc(s, gfp);
}

static void mykfree(void *p)
{
	return kfree(p);
}

static int myseq_printf(struct seq_file *m, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int r = 0;
	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	r = seq_printf(m, "%pV", &vaf);
	va_end(args);
	return r;
}

static long my_copy_from_user(void *to, const void __user *from, long size)
{
	return copy_from_user(to, from, size);
}

static long my_copy_to_user(void __user *to, const void *from, long size)
{
	return copy_to_user(to, from, size);
}


static int my_wait_event_interruptible(wait_queue_head_t *q, int (*state)(void *data), void *data)
{
	return wait_event_interruptible((*q), state(data));
}

static void my_wake_up_all(wait_queue_head_t *q)
{
	wake_up_all(q);
}

static void my_wake_up(wait_queue_head_t *q)
{
	wake_up(q);
}

static void my_init_waitqueue_head(wait_queue_head_t *q)
{
	init_waitqueue_head(q);
}

static struct resource * private_request_mem_region(resource_size_t start, resource_size_t n,
				   const char *name)
{
	return request_mem_region(start, n, name);
}

static void private_release_mem_region(resource_size_t start, resource_size_t n)
{
	release_mem_region(start, n);
}

static inline void __iomem * private_ioremap(phys_t offset, unsigned long size)
{
	return ioremap(offset, size);
}

static void private_spin_lock_irqsave(spinlock_t *lock, unsigned long *flags)
{
	raw_spin_lock_irqsave(spinlock_check(lock), *flags);
}

static void private_spin_lock_init(spinlock_t *lock)
{
	spin_lock_init(lock);
}

static void private_raw_mutex_init(struct mutex *lock, const char *name, struct lock_class_key *key)
{
	__mutex_init(lock, name, key);
}

static int private_request_module(bool wait, const char *fmt, ...)
{
	int ret = 0;
	struct va_format vaf;
	va_list args;
	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	ret =  __request_module(true,"%pV", &vaf);
	va_end(args);
	return ret;
}

static struct sk_buff *private_nlmsg_new(size_t payload, gfp_t flags)
{
	return nlmsg_new(payload, flags);
}

static struct nlmsghdr *private_nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq,
					 int type, int payload, int flags)
{
	return nlmsg_put(skb, portid, seq, type, payload, flags);
}


static struct sock *private_netlink_kernel_create(struct net *net, int unit, struct netlink_kernel_cfg *cfg)
{
	return netlink_kernel_create(net, unit, cfg);
}


static mm_segment_t private_get_fs(void)
{
	return get_fs();
}

static void private_set_fs(mm_segment_t val)
{
	set_fs(val);
}

extern struct net init_net;
static struct net *private_get_init_net(void)
{
	return &init_net;
}

static struct jz_driver_common_interfaces private_funcs = {
	.flags_0 = (unsigned int)&(printk),
	/* platform interface */
	.priv_platform_driver_register = platform_driver_register,
	.priv_platform_driver_unregister = platform_driver_unregister,
	.priv_platform_set_drvdata = platform_set_drvdata,
	.priv_platform_get_drvdata = platform_get_drvdata,
	.priv_platform_device_register = platform_device_register,
	.priv_platform_device_unregister = platform_device_unregister,
	.priv_platform_get_resource = platform_get_resource,
	.priv_dev_set_drvdata = dev_set_drvdata,
	.priv_dev_get_drvdata = dev_get_drvdata,
	.priv_platform_get_irq = platform_get_irq,
	.priv_request_mem_region = private_request_mem_region,
	.priv_release_mem_region = private_release_mem_region,
	.priv_ioremap = private_ioremap,
	.priv_iounmap = iounmap,

	/* interrupt interface */
	.priv_request_threaded_irq = request_threaded_irq,
	.priv_enable_irq = enable_irq,
	.priv_disable_irq = disable_irq,
	.priv_free_irq = free_irq,

	/* lock and mutex interface */
	.priv_spin_unlock_irqrestore = spin_unlock_irqrestore,
	.priv_mutex_lock = mutex_lock,
	.priv_mutex_unlock = mutex_unlock,
	.priv_spin_lock_irqsave = private_spin_lock_irqsave,
	.priv_spin_lock_init = private_spin_lock_init,
	.priv_raw_mutex_init = private_raw_mutex_init,

	/* clock interfaces */
	.priv_clk_get = clk_get,
	.priv_clk_enable = clk_enable,
	.priv_clk_is_enabled = clk_is_enabled,
	.priv_clk_disable = clk_disable,
	.priv_clk_get_rate = clk_get_rate,
	.priv_clk_put = clk_put,
	.priv_clk_set_rate = clk_set_rate,

	/* i2c interfaces */
	.priv_i2c_get_adapter = i2c_get_adapter,
	.priv_i2c_put_adapter = i2c_put_adapter,
	.priv_i2c_transfer = i2c_transfer,
	.priv_i2c_register_driver = i2c_register_driver,
	.priv_i2c_del_driver = i2c_del_driver,

	.priv_i2c_new_device = i2c_new_device,
	.priv_i2c_get_clientdata = i2c_get_clientdata,
	.priv_i2c_set_clientdata = i2c_set_clientdata,
	.priv_i2c_unregister_device = i2c_unregister_device,

	/* gpio interfaces */
	.priv_gpio_request = gpio_request,
	.priv_gpio_free = gpio_free,
	.priv_gpio_direction_output = gpio_direction_output,
	.priv_gpio_direction_input = gpio_direction_input,
	.priv_gpio_set_debounce = gpio_set_debounce,
	.priv_jzgpio_set_func = jzgpio_set_func,
	.priv_jzgpio_ctrl_pull = jzgpio_ctrl_pull,

	/* system interface */
	.priv_msleep = msleep,
	.priv_capable = capable,
	.priv_sched_clock = sched_clock,
	.priv_try_module_get = try_module_get,
	.priv_request_module = private_request_module,
	.priv_module_put = module_put,

	/* wait */
	.priv_init_completion = init_completion,
	.priv_complete = complete,
	.priv_wait_for_completion_interruptible = wait_for_completion_interruptible,
	.priv_wait_event_interruptible = my_wait_event_interruptible,
	.priv_wake_up_all = my_wake_up_all,
	.priv_wake_up = my_wake_up,
	.priv_init_waitqueue_head = my_init_waitqueue_head,
	.priv_wait_for_completion_timeout = wait_for_completion_timeout,

	/* misc */
	.priv_misc_register = misc_register,
	.priv_misc_deregister = misc_deregister,
	.priv_proc_create_data = proc_create_data,
	/* proc */
	.priv_seq_read = seq_read,
	.priv_seq_lseek = seq_lseek,
	.priv_single_release = single_release,
	.priv_single_open_size = single_open_size,
	.priv_jz_proc_mkdir = jz_proc_mkdir,
	.priv_proc_remove = proc_remove,
	.priv_seq_printf = myseq_printf,
	.priv_simple_strtoull = simple_strtoull,

	/* kthread */
	.priv_kthread_should_stop = kthread_should_stop,
	.priv_kthread_run = mythread_run,
	.priv_kthread_stop = kthread_stop,

	.priv_kmalloc = mykmalloc,
	.priv_kfree = mykfree,
	.priv_copy_from_user = my_copy_from_user,
	.priv_copy_to_user = my_copy_to_user,

	/* netlink */
	.priv_nlmsg_new = private_nlmsg_new,
	.priv_nlmsg_put = private_nlmsg_put,
	.priv_netlink_unicast = netlink_unicast,
	.priv_netlink_kernel_create = private_netlink_kernel_create,
	.priv_sock_release = sock_release,

	/* filp */
	.priv_filp_open = filp_open,
	.priv_filp_close = filp_close,
	.priv_get_fs = private_get_fs,
	.priv_set_fs = private_set_fs,
	.priv_vfs_read = vfs_read,
	.priv_vfs_write = vfs_write,
	.priv_vfs_llseek = vfs_llseek,
	.priv_dma_cache_sync = dma_cache_sync,

	.priv_getrawmonotonic = getrawmonotonic,
	.priv_get_init_net = private_get_init_net,
	/* isp driver interface */
	.get_isp_priv_mem = get_isp_priv_mem,
	.flags_1 = (unsigned int)&(printk),
};

void *get_driver_common_interfaces(void)
{
	return (void *)&private_funcs;
}

EXPORT_SYMBOL(get_driver_common_interfaces);
