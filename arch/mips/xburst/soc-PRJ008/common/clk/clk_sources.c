/*
 * JZSOC Clock and Power Manager
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include "clk.h"
/*
 *	   31 ... 24   GATE_ID or CPCCR_ID or DIV_ID or PLL_ID or DIV_ID.
 *	   23 ... 16   PARENR_ID or RELATIVE_ID.
 *	   16 ... 0    some FLG.
 */

static struct clk clk_srcs[] = {
#define GATE(x)  (((x)<<24) | CLK_FLG_GATE)
#define CPCCR(x) (((x)<<24) | CLK_FLG_CPCCR)
#define DIV(no)  (((no)<<24) | CLK_FLG_DIV)
#define PLL(no)  (((no)<<24) | CLK_FLG_PLL)
#define PWC(no)  (((no)<<24) | CLK_FLG_PWC)
#define PARENT(P)  (((CLK_ID_##P)<<16) | CLK_FLG_PARENT)
#define RELATIVE(P)  (((CLK_ID_##P)<<16) | CLK_FLG_RELATIVE)
#define DEF_CLK(N,FLAG)						\
	[CLK_ID_##N] = { .name = CLK_NAME_##N, .flags = FLAG, }

	DEF_CLK(EXT0,		CLK_FLG_NOALLOC),
	DEF_CLK(EXT1,		CLK_FLG_NOALLOC),
	DEF_CLK(OTGPHY,		CLK_FLG_NOALLOC),

	DEF_CLK(APLL,		PLL(CPM_CPAPCR)),
	DEF_CLK(MPLL,		PLL(CPM_CPMPCR)),

	DEF_CLK(SCLKA,		CPCCR(SCLKA)),
	DEF_CLK(CCLK,		CPCCR(CDIV)),
	DEF_CLK(L2CLK,		CPCCR(L2CDIV)),
	DEF_CLK(H0CLK,		CPCCR(H0DIV)),
	DEF_CLK(H2CLK,		CPCCR(H2DIV)),
	DEF_CLK(PCLK,		CPCCR(PDIV)),

	DEF_CLK(EFUSE,		GATE(1) | PARENT(H2CLK)),
	DEF_CLK(OTG,		GATE(2) | PARENT(H2CLK)),
	DEF_CLK(MSC0,		GATE(3) | PARENT(DIV_MSC0)),
	DEF_CLK(MSC1,		GATE(4) | PARENT(DIV_MSC1)),
	DEF_CLK(SSI,		GATE(5) | PARENT(DIV_SSI)),
	DEF_CLK(I2C0,		GATE(6) | PARENT(PCLK)),
	DEF_CLK(I2C1,		GATE(7) | PARENT(PCLK)),
	DEF_CLK(AIC,		GATE(8) | PARENT(PCLK)),
	DEF_CLK(DMIC,		GATE(9) | PARENT(H2CLK)),
	DEF_CLK(SADC,		GATE(10) | PARENT(PCLK)),
	DEF_CLK(UART0,		GATE(11) | PARENT(EXT1)),
	DEF_CLK(UART1,		GATE(12) | PARENT(EXT1)),
	DEF_CLK(UART2,		GATE(13) | PARENT(EXT1)),
	DEF_CLK(I2C2,		GATE(14) | PARENT(PCLK)),
	DEF_CLK(HASH,		GATE(15) | PARENT(H2CLK)),
	DEF_CLK(MONITOR,	GATE(16)),
	DEF_CLK(SFC,		GATE(17) | PARENT(DIV_SFC)),
	DEF_CLK(PDMA,		GATE(18)),
	DEF_CLK(ISP,		GATE(19) | PARENT(DIV_ISP)),
	DEF_CLK(LCD,		GATE(20) | PARENT(DIV_LCD)),
	DEF_CLK(MIPI_CSI,	GATE(21) | PARENT(PCLK)),
	DEF_CLK(RISCV,		GATE(22) | PARENT(DIV_RISCV)),
	DEF_CLK(RSA,		GATE(23) | PARENT(DIV_RSA)),
	DEF_CLK(DES,		GATE(24) | PARENT(PCLK)),
	DEF_CLK(RTC,		GATE(25) | PARENT(PCLK)),
	DEF_CLK(TCU,		GATE(26) | PARENT(PCLK)),
	DEF_CLK(DDR,		GATE(27) | PARENT(DIV_DDR)),
	DEF_CLK(VPU,		GATE(28) | PARENT(DIV_VPU)),
	DEF_CLK(DTRNG,		GATE(29) | PARENT(PCLK)),
	DEF_CLK(IPU,		GATE(30) | PARENT(H0CLK)),
	DEF_CLK(LDC,		GATE(31) | PARENT(DIV_LDC)),
	DEF_CLK(GMAC,		GATE(32 + 0) | PARENT(DIV_GMAC)),
	DEF_CLK(AES,		GATE(32 + 1) | PARENT(H2CLK)),
	DEF_CLK(AHB1,		GATE(32 + 2) | PARENT(DIV_ISPA)),
	DEF_CLK(IVDC,		GATE(32 + 3) | PARENT(DIV_ISPA)),
	DEF_CLK(LZMA0,		GATE(32 + 4) | PARENT(DIV_TNPU)),
	DEF_CLK(ODVP,		GATE(32 + 5) | PARENT(H0CLK)),
	DEF_CLK(AHB0,		GATE(32 + 6) | PARENT(H0CLK)),
	DEF_CLK(SYS_OST,	GATE(32 + 7)),
	DEF_CLK(I2D,		GATE(32 + 8) | PARENT(H0CLK)),
	DEF_CLK(PWM,		GATE(32 + 9) | PARENT(DIV_PWM)),
	DEF_CLK(APB0,		GATE(32 + 10) | PARENT(PCLK)),
	DEF_CLK(TNPU,		GATE(32 + 11) | PARENT(DIV_TNPU)),
	DEF_CLK(CPU,		GATE(32 + 12) | PARENT(SCLKA)),
	DEF_CLK(SFC1,		GATE(32 + 13) | PARENT(DIV_SFC1)),
	DEF_CLK(SSI_SLV,	GATE(32 + 14) | PARENT(DIV_SSI_SLV)),
	DEF_CLK(LZMA1,		GATE(32 + 15) | PARENT(DIV_TNPU)),

	DEF_CLK(DIV_DDR,	DIV(DIV_DDR)),
	DEF_CLK(DIV_VPU,	DIV(DIV_VPU)),
	DEF_CLK(DIV_RSA,	DIV(DIV_RSA)),
	DEF_CLK(DIV_GMAC,	DIV(DIV_GMAC)),
	DEF_CLK(DIV_SFC,	DIV(DIV_SFC)),
	DEF_CLK(DIV_SFC1,	DIV(DIV_SFC1)),
	DEF_CLK(DIV_PWM,	DIV(DIV_PWM)),
	DEF_CLK(DIV_SSI,	DIV(DIV_SSI)),
	DEF_CLK(DIV_SSI_SLV,DIV(DIV_SSI_SLV)),
	DEF_CLK(DIV_LCD,	DIV(DIV_LCD)),
	DEF_CLK(DIV_MSC0,	DIV(DIV_MSC0)),
	DEF_CLK(DIV_MSC1,	DIV(DIV_MSC1)),
	DEF_CLK(DIV_LDC,	DIV(DIV_LDC)),
	DEF_CLK(DIV_I2S_SPK,DIV(DIV_I2S_SPK)),
	DEF_CLK(DIV_I2S_MIC,DIV(DIV_I2S_MIC)),
	DEF_CLK(DIV_CIM,	DIV(DIV_CIM)),
	DEF_CLK(DIV_ISP,	DIV(DIV_ISP)),
	DEF_CLK(DIV_ISPA,	DIV(DIV_ISPA)),
	DEF_CLK(DIV_TNPU,	DIV(DIV_TNPU)),
	DEF_CLK(DIV_RISCV,	DIV(DIV_RISCV)),

#undef GATE
#undef CPCCR
#undef DIV
#undef PWC
#undef PARENT
#undef DEF_CLK
#undef RELATIVE
};
int get_clk_sources_size(void){
	return ARRAY_SIZE(clk_srcs);
}
struct clk *get_clk_from_id(int clk_id)
{
	return &clk_srcs[clk_id];
}
int get_clk_id(struct clk *clk)
{
	return (clk - &clk_srcs[0]);
}
