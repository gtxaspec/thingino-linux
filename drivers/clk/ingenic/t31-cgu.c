// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T31 SoC CGU driver
 *
 * Copyright (C) 2026 Alfonso Gamboa <gtxent@gmail.com>
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>

#include <dt-bindings/clock/ingenic,t31-cgu.h>

#include "cgu.h"
#include "pm.h"

/* CGU register offsets */
#define CGU_REG_CPCCR		0x00
#define CGU_REG_CPPCR		0x0c
#define CGU_REG_APLL		0x10
#define CGU_REG_MPLL		0x14
#define CGU_REG_CLKGR0		0x20
#define CGU_REG_OPCR		0x24
#define CGU_REG_CLKGR1		0x28
#define CGU_REG_DDRCDR		0x2c
#define CGU_REG_EL150CDR	0x30
#define CGU_REG_USBPCR		0x3c
#define CGU_REG_USBRDT		0x40
#define CGU_REG_USBVBFIL	0x44
#define CGU_REG_USBPCR1		0x48
#define CGU_REG_RSACDR		0x4c
#define CGU_REG_MACCDR		0x54
#define CGU_REG_LPCDR		0x64
#define CGU_REG_MSC0CDR		0x68
#define CGU_REG_SSICDR		0x74
#define CGU_REG_CIMCDR		0x7c
#define CGU_REG_ISPCDR		0x80
#define CGU_REG_MSC1CDR		0xa4
#define CGU_REG_VPLL		0xe0
#define CGU_REG_MACPHYC		0xe8

/* bits within the OPCR register */
#define OPCR_GATE_USBPHYCLK	BIT(23)
#define OPCR_SPENDN0		BIT(7)

/* bits within the USBPCR register */
#define USBPCR_SIDDQ		BIT(21)
#define USBPCR_OTG_DISABLE	BIT(20)

static struct ingenic_cgu *cgu;

static int t31_usb_phy_enable(struct clk_hw *hw)
{
	return 0;
}

static void t31_usb_phy_disable(struct clk_hw *hw)
{
	void __iomem *reg_opcr	= cgu->base + CGU_REG_OPCR;
	void __iomem *reg_usbpcr = cgu->base + CGU_REG_USBPCR;

	writel((readl(reg_opcr) & ~OPCR_SPENDN0) | OPCR_GATE_USBPHYCLK,
	       reg_opcr);
	writel(readl(reg_usbpcr) | USBPCR_OTG_DISABLE | USBPCR_SIDDQ,
	       reg_usbpcr);
}

static int t31_usb_phy_is_enabled(struct clk_hw *hw)
{
	void __iomem *reg_opcr	= cgu->base + CGU_REG_OPCR;
	void __iomem *reg_usbpcr = cgu->base + CGU_REG_USBPCR;

	return (readl(reg_opcr) & OPCR_SPENDN0) &&
	       !(readl(reg_usbpcr) & USBPCR_SIDDQ) &&
	       !(readl(reg_usbpcr) & USBPCR_OTG_DISABLE);
}

static const struct clk_ops t31_otg_phy_ops = {
	.enable		= t31_usb_phy_enable,
	.disable	= t31_usb_phy_disable,
	.is_enabled	= t31_usb_phy_is_enabled,
};

/*
 * T31 PLL OD encoding.
 *
 * T31 PLLs have two output divider fields: OD1 (bits 13:11) and OD0
 * (bits 10:8). The effective output divider is OD1 * OD0. In practice,
 * the bootloader always sets OD0 = 1, so we model only OD1 here.
 *
 * The register value maps directly to the divider value (1-based):
 * The table maps divider index to register value. The CGU framework
 * searches for the register value in the table; the matching index
 * is the divider. Index 0 is unused (divide-by-0 is invalid).
 */
static const s8 t31_pll_od_encoding[8] = {
	-1, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
};

static const struct ingenic_cgu_clk_info t31_cgu_clocks[] = {

	/* External clocks */

	[T31_CLK_EXCLK] = { "ext", CGU_CLK_EXT },
	[T31_CLK_RTCLK] = { "rtc", CGU_CLK_EXT },

	/* PLLs */

	[T31_CLK_APLL] = {
		"apll", CGU_CLK_PLL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.pll = {
			.reg = CGU_REG_APLL,
			.rate_multiplier = 1,
			.m_shift = 20,
			.m_bits = 12,
			.m_offset = 0,
			.n_shift = 14,
			.n_bits = 6,
			.n_offset = 0,
			.od_shift = 11,
			.od_bits = 3,
			.od_max = 8,
			.od_encoding = t31_pll_od_encoding,
			.bypass_reg = CGU_REG_APLL,
			.bypass_bit = -1,
			.enable_bit = 0,
			.stable_bit = 3,
		},
	},

	[T31_CLK_MPLL] = {
		"mpll", CGU_CLK_PLL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.pll = {
			.reg = CGU_REG_MPLL,
			.rate_multiplier = 1,
			.m_shift = 20,
			.m_bits = 12,
			.m_offset = 0,
			.n_shift = 14,
			.n_bits = 6,
			.n_offset = 0,
			.od_shift = 11,
			.od_bits = 3,
			.od_max = 8,
			.od_encoding = t31_pll_od_encoding,
			.bypass_reg = CGU_REG_MPLL,
			.bypass_bit = -1,
			.enable_bit = 0,
			.stable_bit = 3,
		},
	},

	[T31_CLK_VPLL] = {
		"vpll", CGU_CLK_PLL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.pll = {
			.reg = CGU_REG_VPLL,
			.rate_multiplier = 1,
			.m_shift = 20,
			.m_bits = 12,
			.m_offset = 0,
			.n_shift = 14,
			.n_bits = 6,
			.n_offset = 0,
			.od_shift = 11,
			.od_bits = 3,
			.od_max = 8,
			.od_encoding = t31_pll_od_encoding,
			.bypass_reg = CGU_REG_VPLL,
			.bypass_bit = -1,
			.enable_bit = 0,
			.stable_bit = 3,
		},
	},

	/* Custom (SoC-specific) OTG PHY */

	[T31_CLK_OTGPHY] = {
		"otg_phy", CGU_CLK_CUSTOM,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.custom = { &t31_otg_phy_ops },
	},

	/* Muxes & dividers */

	[T31_CLK_SCLKA] = {
		"sclk_a", CGU_CLK_MUX,
		.parents = { -1, T31_CLK_EXCLK, T31_CLK_APLL, -1 },
		.mux = { CGU_REG_CPCCR, 30, 2 },
	},

	[T31_CLK_CPUMUX] = {
		"cpu_mux", CGU_CLK_MUX,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_CPCCR, 28, 2 },
	},

	[T31_CLK_CPU] = {
		"cpu", CGU_CLK_DIV | CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_CPUMUX, -1, -1, -1 },
		.div = { CGU_REG_CPCCR, 0, 1, 4, 22, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 15 },
	},

	[T31_CLK_L2CACHE] = {
		"l2cache", CGU_CLK_DIV,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_CPUMUX, -1, -1, -1 },
		.div = { CGU_REG_CPCCR, 4, 1, 4, 22, -1, -1 },
	},

	[T31_CLK_AHB0] = {
		"ahb0", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_CPCCR, 26, 2 },
		.div = { CGU_REG_CPCCR, 8, 1, 4, 21, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 10 },
	},

	[T31_CLK_AHB2] = {
		"ahb2", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_CPCCR, 24, 2 },
		.div = { CGU_REG_CPCCR, 12, 1, 4, 20, -1, -1 },
	},

	[T31_CLK_PCLK] = {
		"pclk", CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.div = { CGU_REG_CPCCR, 16, 1, 4, 20, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 14 },
	},

	[T31_CLK_DDR] = {
		"ddr", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_DDRCDR, 30, 2 },
		.div = { CGU_REG_DDRCDR, 0, 1, 4, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 31 },
	},

	[T31_CLK_MAC] = {
		"mac", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_MACCDR, 30, 2 },
		.div = { CGU_REG_MACCDR, 0, 1, 8, 29, 28, 27 },
	},

	[T31_CLK_LCD] = {
		"lcd", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_LPCDR, 30, 2 },
		.div = { CGU_REG_LPCDR, 0, 1, 8, 28, 27, 26 },
		.gate = { CGU_REG_CLKGR0, 24 },
	},

	[T31_CLK_MSCMUX] = {
		"msc_mux", CGU_CLK_MUX,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_MSC0CDR, 30, 2 },
	},

	[T31_CLK_MSC0] = {
		"msc0", CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_MSCMUX, -1, -1, -1 },
		.div = { CGU_REG_MSC0CDR, 0, 4, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 4 },
	},

	[T31_CLK_MSC1] = {
		"msc1", CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_MSCMUX, -1, -1, -1 },
		.div = { CGU_REG_MSC1CDR, 0, 4, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 5 },
	},

	[T31_CLK_SSIPLL] = {
		"ssi_pll", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_SSICDR, 30, 2 },
		.div = { CGU_REG_SSICDR, 0, 1, 8, 28, 27, 26 },
	},

	[T31_CLK_ISP] = {
		"isp", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_ISPCDR, 30, 2 },
		.div = { CGU_REG_ISPCDR, 0, 1, 4, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 23 },
	},

	[T31_CLK_CIM] = {
		"cim", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_CIMCDR, 30, 2 },
		.div = { CGU_REG_CIMCDR, 0, 1, 8, -1, -1, -1 },
	},

	[T31_CLK_RSA] = {
		"rsa", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_RSACDR, 30, 2 },
		.div = { CGU_REG_RSACDR, 0, 1, 4, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 27 },
	},

	[T31_CLK_EL150] = {
		"el150", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_EL150CDR, 30, 2 },
		.div = { CGU_REG_EL150CDR, 0, 1, 4, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 0 },
	},

	/* Gate-only clocks */

	[T31_CLK_NEMC] = {
		"nemc", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 0 },
	},

	[T31_CLK_EFUSE] = {
		"efuse", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 1 },
	},

	[T31_CLK_OTG] = {
		"otg", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 3 },
	},

	[T31_CLK_SSI0] = {
		"ssi0", CGU_CLK_GATE,
		.parents = { T31_CLK_SSIPLL, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 6 },
	},

	[T31_CLK_SMB0] = {
		"smb0", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 7 },
	},

	[T31_CLK_SMB1] = {
		"smb1", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 8 },
	},

	[T31_CLK_AIC] = {
		"aic", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 11 },
	},

	[T31_CLK_DMIC] = {
		"dmic", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 12 },
	},

	[T31_CLK_SADC] = {
		"sadc", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 13 },
	},

	[T31_CLK_UART0] = {
		"uart0", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 14 },
	},

	[T31_CLK_UART1] = {
		"uart1", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 15 },
	},

	[T31_CLK_UART2] = {
		"uart2", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 16 },
	},

	[T31_CLK_SLV] = {
		"slv", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 17 },
	},

	[T31_CLK_HASH] = {
		"hash", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 18 },
	},

	[T31_CLK_SSI1] = {
		"ssi1", CGU_CLK_GATE,
		.parents = { T31_CLK_SSIPLL, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 19 },
	},

	[T31_CLK_SFC] = {
		"sfc", CGU_CLK_GATE,
		.parents = { T31_CLK_SSIPLL, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 20 },
	},

	[T31_CLK_PDMA] = {
		"pdma", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 21 },
	},

	[T31_CLK_MIPI_CSI] = {
		"mipi_csi", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 25 },
	},

	[T31_CLK_RISCV] = {
		"riscv", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 26 },
	},

	[T31_CLK_DES] = {
		"des", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 28 },
	},

	[T31_CLK_TCU] = {
		"tcu_gate", CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 30 },
	},

	[T31_CLK_DTRNG] = {
		"dtrng", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 1 },
	},

	[T31_CLK_IPU] = {
		"ipu", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 2 },
	},

	[T31_CLK_GMAC] = {
		"gmac", CGU_CLK_GATE,
		.parents = { T31_CLK_MAC, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 4 },
	},

	[T31_CLK_AES] = {
		"aes", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 5 },
	},

	[T31_CLK_AHB1] = {
		"ahb1", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 6 },
	},

	[T31_CLK_OST] = {
		"ost_gate", CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 11 },
	},

	[T31_CLK_EXCLK_DIV512] = {
		"exclk_div512", CGU_CLK_FIXDIV,
		.parents = { T31_CLK_EXCLK },
		.fixdiv = { 512 },
	},

	[T31_CLK_RTC] = {
		"rtc_ercs", CGU_CLK_MUX | CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK_DIV512, T31_CLK_RTCLK, -1, -1 },
		.mux = { CGU_REG_OPCR, 2, 1 },
		.gate = { CGU_REG_CLKGR0, 29 },
	},

	[T31_CLK_USBPHY] = {
		"usb_phy", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_OPCR, 23, .clear_to_gate = true },
	},
};

static void __init t31_cgu_init(struct device_node *np)
{
	int retval;

	cgu = ingenic_cgu_new(t31_cgu_clocks,
			      ARRAY_SIZE(t31_cgu_clocks), np);
	if (!cgu) {
		pr_err("%s: failed to initialise CGU\n", __func__);
		return;
	}

	retval = ingenic_cgu_register_clocks(cgu);
	if (retval) {
		pr_err("%s: failed to register CGU clocks\n", __func__);
		return;
	}

	ingenic_cgu_register_syscore(cgu);
}
/*
 * CGU has some children devices, this is useful for probing children devices
 * in the case where the device node is compatible with "simple-mfd".
 */
CLK_OF_DECLARE_DRIVER(t31_cgu, "ingenic,t31-cgu", t31_cgu_init);
