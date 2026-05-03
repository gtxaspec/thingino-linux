/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * This header provides clock numbers for the ingenic,t31-cgu DT binding.
 *
 * They are roughly ordered as:
 *   - external clocks
 *   - PLLs
 *   - muxes/dividers in the order they appear in the T31 programmers manual
 *   - gates in order of their bit in the CLKGR* registers
 */

#ifndef __DT_BINDINGS_CLOCK_T31_CGU_H__
#define __DT_BINDINGS_CLOCK_T31_CGU_H__

#define T31_CLK_EXCLK		0
#define T31_CLK_RTCLK		1
#define T31_CLK_APLL		2
#define T31_CLK_MPLL		3
#define T31_CLK_VPLL		4
#define T31_CLK_OTGPHY		5
#define T31_CLK_SCLKA		6
#define T31_CLK_CPUMUX		7
#define T31_CLK_CPU		8
#define T31_CLK_L2CACHE		9
#define T31_CLK_AHB0		10
#define T31_CLK_AHB2		11
#define T31_CLK_PCLK		12
#define T31_CLK_DDR		13
#define T31_CLK_MAC		14
#define T31_CLK_LCD		15
#define T31_CLK_MSCMUX		16
#define T31_CLK_MSC0		17
#define T31_CLK_MSC1		18
#define T31_CLK_SSIPLL		19
#define T31_CLK_ISP		20
#define T31_CLK_CIM		21
#define T31_CLK_RSA		22
#define T31_CLK_EL150		23
#define T31_CLK_NEMC		24
#define T31_CLK_EFUSE		25
#define T31_CLK_OTG		26
#define T31_CLK_SSI0		27
#define T31_CLK_SSI1		28
#define T31_CLK_SMB0		29
#define T31_CLK_SMB1		30
#define T31_CLK_UART0		31
#define T31_CLK_UART1		32
#define T31_CLK_UART2		33
#define T31_CLK_DMIC		34
#define T31_CLK_AIC		35
#define T31_CLK_SFC		36
#define T31_CLK_PDMA		37
#define T31_CLK_TCU		38
#define T31_CLK_DES		39
#define T31_CLK_HASH		40
#define T31_CLK_MIPI_CSI	41
#define T31_CLK_RISCV		42
#define T31_CLK_SADC		43
#define T31_CLK_SLV		44
#define T31_CLK_AHB1		45
#define T31_CLK_AES		46
#define T31_CLK_GMAC		47
#define T31_CLK_IPU		48
#define T31_CLK_DTRNG		49
#define T31_CLK_OST		50
#define T31_CLK_EXCLK_DIV512	51
#define T31_CLK_RTC		52
#define T31_CLK_USBPHY		53

#endif /* __DT_BINDINGS_CLOCK_T31_CGU_H__ */
