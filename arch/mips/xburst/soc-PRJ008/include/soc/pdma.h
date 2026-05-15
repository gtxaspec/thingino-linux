/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2021 by nick shen <xianghui.shen@ingneic.com>
 */
#ifndef __ASM_MACH_INGENIC_PDMA_H__
#define __ASM_MACH_INGENIC_PDMA_H__

#include <dt-bindings/dma/ingenic-pdma.h>
#define INGENIC_DMA_REQ_AUTO 0xff
#define INGENIC_DMA_CHAN_CNT 16

#ifdef CONFIG_AIC_DMA_MODE
# define AIC_DMA_CHANNELS 3
#else
# define AIC_DMA_CHANNELS 0
#endif

#ifdef CONFIG_AUTO_DMA_MODE
# define AUTO_DMA_CHANNELS 1
#else
# define AUTO_DMA_CHANNELS 0
#endif

#ifdef CONFIG_SADC_DMA_MODE
# define SADC_DMA_CHANNELS 1
#else
# define SADC_DMA_CHANNELS 0
#endif

#ifdef CONFIG_UART2_DMA_MODE
# define UART2_DMA_CHANNELS 2
#else
# define UART2_DMA_CHANNELS 0
#endif

#ifdef CONFIG_UART1_DMA_MODE
# define UART1_DMA_CHANNELS 2
#else
# define UART1_DMA_CHANNELS 0
#endif

#ifdef CONFIG_UART0_DMA_MODE
# define UART0_DMA_CHANNELS 2
#else
# define UART0_DMA_CHANNELS 0
#endif

#ifdef CONFIG_SSI0_DMA_MODE
# define SSI0_DMA_CHANNELS 2
#else
# define SSI0_DMA_CHANNELS 0
#endif

#ifdef CONFIG_SSLV_DMA_MODE
# define SSLV_DMA_CHANNELS 2
#else
# define SSLV_DMA_CHANNELS 0
#endif

#ifdef CONFIG_AES0_DMA_MODE
# define AES0_DMA_CHANNELS 1
#else
# define AES0_DMA_CHANNELS 0
#endif

#ifdef CONFIG_AES1_DMA_MODE
# define AES1_DMA_CHANNELS 1
#else
# define AES1_DMA_CHANNELS 0
#endif

#ifdef CONFIG_HASH_DMA_MODE
# define HASH_DMA_CHANNELS 1
#else
# define HASH_DMA_CHANNELS 0
#endif

#ifdef CONFIG_I2C0_DMA_MODE
# define I2C0_DMA_CHANNELS 2
#else
# define I2C0_DMA_CHANNELS 0
#endif

#ifdef CONFIG_I2C1_DMA_MODE
# define I2C1_DMA_CHANNELS 2
#else
# define I2C1_DMA_CHANNELS 0
#endif

#ifdef CONFIG_I2C2_DMA_MODE
# define I2C2_DMA_CHANNELS 2
#else
# define I2C2_DMA_CHANNELS 0
#endif

#define PDMA_MAPS_COUNT ( \
		0 + \
		AIC_DMA_CHANNELS + \
		AUTO_DMA_CHANNELS + \
		SADC_DMA_CHANNELS + \
		UART2_DMA_CHANNELS + \
		UART1_DMA_CHANNELS + \
		UART0_DMA_CHANNELS + \
		SSI0_DMA_CHANNELS + \
		SSLV_DMA_CHANNELS + \
		AES0_DMA_CHANNELS + \
		AES1_DMA_CHANNELS + \
		HASH_DMA_CHANNELS + \
		I2C0_DMA_CHANNELS + \
		I2C1_DMA_CHANNELS + \
		I2C2_DMA_CHANNELS \
		)
#ifndef Static_assert
#define Static_assert(cond, msg) _Static_assert(cond, msg)
#endif
Static_assert(PDMA_MAPS_COUNT <= INGENIC_DMA_CHAN_CNT, "DMA channel count exceeded!");

unsigned int pdma_maps[INGENIC_DMA_CHAN_CNT] = {
#ifdef CONFIG_AIC_DMA_MODE
	INGENIC_DMA_REQ_AEC,
	INGENIC_DMA_REQ_I2S0_TX,
	INGENIC_DMA_REQ_I2S0_RX,
#endif

#ifdef CONFIG_AUTO_DMA_MODE
	INGENIC_DMA_REQ_AUTO_TX,
#endif

#ifdef CONFIG_SADC_DMA_MODE
	INGENIC_DMA_REQ_SADC_RX,
#endif

#ifdef CONFIG_UART2_DMA_MODE
	INGENIC_DMA_REQ_UART2_TX,
	INGENIC_DMA_REQ_UART2_RX,
#endif

#ifdef CONFIG_UART1_DMA_MODE
	INGENIC_DMA_REQ_UART1_TX,
	INGENIC_DMA_REQ_UART1_RX,
#endif

#ifdef CONFIG_UART0_DMA_MODE
	INGENIC_DMA_REQ_UART0_TX,
	INGENIC_DMA_REQ_UART0_RX,
#endif

#ifdef CONFIG_SSI0_DMA_MODE
	INGENIC_DMA_REQ_SSI0_TX,
	INGENIC_DMA_REQ_SSI0_RX,
#endif

#ifdef CONFIG_SSLV_DMA_MODE
	INGENIC_DMA_REQ_SLV_TX,
	INGENIC_DMA_REQ_SLV_RX,
#endif

#ifdef CONFIG_AES0_DMA_MODE
	INGENIC_DMA_REQ_AES0_TX,
#endif

#ifdef CONFIG_AES1_DMA_MODE
	INGENIC_DMA_REQ_AES1_TX,
#endif

#ifdef CONFIG_HASH_DMA_MODE
	INGENIC_DMA_REQ_HASH_TX,
#endif

#ifdef CONFIG_I2C0_DMA_MODE
	INGENIC_DMA_REQ_I2C0_TX,
	INGENIC_DMA_REQ_I2C0_RX,
#endif

#ifdef CONFIG_I2C1_DMA_MODE
	INGENIC_DMA_REQ_I2C1_TX,
	INGENIC_DMA_REQ_I2C1_RX,
#endif

#ifdef CONFIG_I2C2_DMA_MODE
	INGENIC_DMA_REQ_I2C2_TX,
	INGENIC_DMA_REQ_I2C2_RX,
#endif
};

#endif
