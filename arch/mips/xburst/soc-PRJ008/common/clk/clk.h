#ifndef _CLK_H_
#define _CLK_H_
#include <linux/init.h>
#include <linux/kernel.h>
struct clk;
struct cpm_pwc;
struct clk_ops {
	int (*enable)(struct clk *,int);
	struct clk* (*get_parent)(struct clk *);
	int (*set_parent)(struct clk *,struct clk *);
	unsigned long (*get_rate)(struct clk *);
	int (*set_rate)(struct clk *,unsigned long);
	int (*set_round_rate)(struct clk *,unsigned long);
};
struct clk {
	const char *name;
	unsigned long rate;
	struct clk *parent;
	unsigned long flags;
#define CLK_FLG_NOALLOC	BIT(0)
#define CLK_FLG_ENABLE	BIT(1)
#define CLK_GATE_BIT(flg)	((flg) >> 24)
#define CLK_FLG_GATE	BIT(2)
#define CLK_CPCCR_NO(flg)	(((flg) >> 24) & 0xff)
#define CLK_FLG_CPCCR	BIT(3)
#define CLK_DIV_NO(flg)		(((flg) >> 24) & 0xff)
#define CLK_FLG_DIV	BIT(4)
#define CLK_PLL_NO(flg)		(((flg) >> 24) & 0xff)
#define CLK_FLG_PLL	BIT(5)
#define CLK_PWC_NO(flg)		(((flg) >> 24) & 0xff)
#define CLK_FLG_PWC	BIT(6)
#define CLK_PARENT(flg)		(((flg) >> 16) & 0xff)
#define CLK_RELATIVE(flg)	(((flg) >> 16) & 0xff)
#define CLK_FLG_PARENT	BIT(7)
#define CLK_FLG_RELATIVE BIT(8)
	struct clk_ops *ops;
	int count;
	int init_state;
	struct clk *source;
	struct clk *child;
	unsigned int CLK_ID;
};

enum {
	CLK_ID_EXT	   = 0,
	CLK_ID_EXT0,
#define CLK_NAME_EXT0		"ext0"
	CLK_ID_EXT1,
#define CLK_NAME_EXT1		"ext1"
	CLK_ID_OTGPHY,
#define CLK_NAME_OTGPHY		"otg_phy"

	CLK_ID_PLL,
	CLK_ID_APLL,
#define CLK_NAME_APLL		"apll"
	CLK_ID_MPLL,
#define CLK_NAME_MPLL		"mpll"
	CLK_ID_SCLKA,
#define CLK_NAME_SCLKA		"sclka"
	/**********************************************************************************/
	CLK_ID_CPPCR,
	CLK_ID_CCLK,
#define CLK_NAME_CCLK		"cpuclk"
	CLK_ID_L2CLK,
#define CLK_NAME_L2CLK		"l2clk"
	CLK_ID_H0CLK,
#define CLK_NAME_H0CLK		"h0clk"
	CLK_ID_H2CLK,
#define CLK_NAME_H2CLK		"h2clk"
	CLK_ID_PCLK,
#define CLK_NAME_PCLK		"pclk"
	/**********************************************************************************/
	CLK_ID_DIV,
	CLK_ID_DIV_DDR,
#define CLK_NAME_DIV_DDR	"div_ddr"
	CLK_ID_DIV_VPU,
#define CLK_NAME_DIV_VPU	"div_vpu"
	CLK_ID_DIV_RSA,
#define CLK_NAME_DIV_RSA	"div_rsa"
	CLK_ID_DIV_GMAC,
#define CLK_NAME_DIV_GMAC	"div_gmac"
	CLK_ID_DIV_SFC,
#define CLK_NAME_DIV_SFC	"div_sfc0"
	CLK_ID_DIV_SFC1,
#define CLK_NAME_DIV_SFC1	"div_sfc1"
	CLK_ID_DIV_PWM,
#define CLK_NAME_DIV_PWM	"div_pwm"
	CLK_ID_DIV_SSI,
#define CLK_NAME_DIV_SSI	"div_ssi"
	CLK_ID_DIV_SSI_SLV,
#define CLK_NAME_DIV_SSI_SLV	"div_ssi_slv"
	CLK_ID_DIV_LCD,
#define CLK_NAME_DIV_LCD	"div_lcd"
	CLK_ID_DIV_MSC0,
#define CLK_NAME_DIV_MSC0	"div_msc0"
	CLK_ID_DIV_MSC1,
#define CLK_NAME_DIV_MSC1	"div_msc1"
	CLK_ID_DIV_LDC,
#define CLK_NAME_DIV_LDC	"div_ldc"
	CLK_ID_DIV_I2S_SPK,
#define CLK_NAME_DIV_I2S_SPK	"div_i2s_spk"
	CLK_ID_DIV_I2S_MIC,
#define CLK_NAME_DIV_I2S_MIC	"div_i2s_mic"
	CLK_ID_DIV_CIM,
#define CLK_NAME_DIV_CIM	"div_cim"
	CLK_ID_DIV_ISP,
#define CLK_NAME_DIV_ISP	"div_isp"
	CLK_ID_DIV_ISPA,
#define CLK_NAME_DIV_ISPA	"div_ispa"
	CLK_ID_DIV_TNPU,
#define CLK_NAME_DIV_TNPU	"div_tnpu"
	CLK_ID_DIV_RISCV,
#define CLK_NAME_DIV_RISCV	"div_riscv"

/**********************************************************************************/
	CLK_ID_DEVICES,
	CLK_ID_DDR,
#define CLK_NAME_DDR		"gate_ddr"
	CLK_ID_TCU,
#define CLK_NAME_TCU		"gate_tcu"
	CLK_ID_RTC,
#define CLK_NAME_RTC		"gate_rtc"
	CLK_ID_DES,
#define CLK_NAME_DES		"gate_des"
	CLK_ID_MIPI_CSI,
#define CLK_NAME_MIPI_CSI	"gate_csi"
	CLK_ID_LCD,
#define CLK_NAME_LCD		"gate_lcd"
	CLK_ID_ISP,
#define CLK_NAME_ISP		"gate_isp"
	CLK_ID_PDMA,
#define CLK_NAME_PDMA		"gate_pdma"
	CLK_ID_SFC,
#define CLK_NAME_SFC		"gate_sfc0"
	CLK_ID_UART2,
#define CLK_NAME_UART2		"gate_uart2"
	CLK_ID_UART1,
#define CLK_NAME_UART1		"gate_uart1"
	CLK_ID_UART0,
#define CLK_NAME_UART0		"gate_uart0"
	CLK_ID_SADC,
#define CLK_NAME_SADC		"gate_sadc"
	CLK_ID_DMIC,
#define CLK_NAME_DMIC		"gate_dmic"
	CLK_ID_AIC,
#define CLK_NAME_AIC		"gate_aic"
	CLK_ID_HASH,
#define CLK_NAME_HASH		"gate_hash"
	CLK_ID_I2C2,
#define CLK_NAME_I2C2		"gate_i2c2"
	CLK_ID_I2C1,
#define CLK_NAME_I2C1		"gate_i2c1"
	CLK_ID_I2C0,
#define CLK_NAME_I2C0		"gate_i2c0"
	CLK_ID_SSI,
#define CLK_NAME_SSI		"gate_ssi"
	CLK_ID_SSI_SLV,
#define CLK_NAME_SSI_SLV	"gate_ssi_slv"
	CLK_ID_MSC1,
#define CLK_NAME_MSC1		"gate_msc1"
	CLK_ID_MSC0,
#define CLK_NAME_MSC0		"gate_msc0"
	CLK_ID_OTG,
#define CLK_NAME_OTG		"gate_otg1"
	CLK_ID_EFUSE,
#define CLK_NAME_EFUSE		"gate_efuse"
	CLK_ID_CPU,
#define CLK_NAME_CPU		"gate_cpu"
	CLK_ID_APB0,
#define CLK_NAME_APB0		"gate_apb0"
	CLK_ID_SYS_OST,
#define CLK_NAME_SYS_OST	"sys_ost"
	CLK_ID_AHB0,
#define CLK_NAME_AHB0		"gate_ahb0"
	CLK_ID_RISCV,
#define CLK_NAME_RISCV		"gate_riscv"
	CLK_ID_AES,
#define CLK_NAME_AES		"gate_aes"
	CLK_ID_RSA,
#define CLK_NAME_RSA		"gate_rsa"
	CLK_ID_AHB1,
#define CLK_NAME_AHB1		"gate_ahb1"
	CLK_ID_GMAC,
#define CLK_NAME_GMAC		"gate_gmac"
	CLK_ID_IPU,
#define CLK_NAME_IPU		"gate_ipu"
	CLK_ID_DTRNG,
#define CLK_NAME_DTRNG		"gate_dtrng"
	CLK_ID_VPU,
#define CLK_NAME_VPU		"gate_vpu"
	CLK_ID_MONITOR,
#define CLK_NAME_MONITOR	"gate_monitor"
	CLK_ID_LDC,
#define CLK_NAME_LDC		"gate_ldc"
	CLK_ID_IVDC,
#define CLK_NAME_IVDC		"gate_ivdc"
	CLK_ID_LZMA0,
#define CLK_NAME_LZMA0		"gate_lzma0"
	CLK_ID_LZMA1,
#define CLK_NAME_LZMA1		"gate_lzma1"
	CLK_ID_ODVP,
#define CLK_NAME_ODVP		"gate_odvp"
	CLK_ID_I2D,
#define CLK_NAME_I2D		"gate_i2d"
	CLK_ID_PWM,
#define CLK_NAME_PWM		"gate_pwm"
	CLK_ID_TNPU,
#define CLK_NAME_TNPU		"gate_tnpu"
	CLK_ID_SFC1,
#define CLK_NAME_SFC1		"gate_sfc1"

	CLK_ID_STOP,
	CLK_ID_INVALID,
};


/* please check clk_gate */
enum {
	DIV_NEMC,
	DIV_EFUSE,
	DIV_OTG,
	DIV_MSC0,
	DIV_MSC1,
	DIV_SSI,
	DIV_I2C0,
	DIV_I2C1,
	DIV_I2S_SPK,
	DIV_I2S_MIC,
	DIV_SADC,
	DIV_UART0,
	DIV_UART1,
	DIV_UART2,
	DIV_I2C2,
	DIV_HASH,
	DIV_MONITOR,
	DIV_SFC,
	DIV_PDMA,
	DIV_ISP,
	DIV_LCD,
	DIV_MIPI_CSI,
	DIV_RISCV,
	DIV_RSA,
	DIV_DES,
	DIV_RTC,
	DIV_TCU,
	DIV_DDR,
	DIV_VPU,
	DIV_DTRNG,
	DIV_IPU,
	DIV_LDC,
	DIV_GMAC,
	DIV_AES,
	DIV_AHB1,
	DIV_IVDC,
	DIV_LZMA0,
	DIV_ODVP,
	DIV_AHB0,
	DIV_SYS_OST,
	DIV_I2D,
	DIV_PWM,
	DIV_APB0,
	DIV_TNPU,
	DIV_CPU,
	DIV_SFC1,
	DIV_SSI_SLV,
	DIV_LZMA1,
	DIV_ISPA,
	DIV_CIM,
};
/**
 * struct ingenic_clk_reg_sleep: register dump of clock controller registers.
 * @offset: clock register offset from the controller base address.
 * @value: the value to be register at offset.
 */
struct ingenic_clk_reg_sleep {
	unsigned int offset;
	unsigned int value;
	unsigned int ce;
	unsigned int busy;
	unsigned int stop;
	unsigned int flags;
};
#define REG_SLEEP(o, c, b, s, f)   \
{				\
	.offset = o,		\
	.ce = c,		\
	.busy = b,		\
	.stop = s,		\
	.flags = f,		\
}

enum {
	CDIV = 0,L2CDIV,H0DIV,H2DIV,PDIV,SCLKA,
};
struct	freq_udelay_jiffy {
	unsigned int  max_num;
	unsigned int  cpufreq;
	unsigned int  udelay_val;
	unsigned int  loops_per_jiffy;
};

int get_clk_sources_size(void);
struct clk *get_clk_from_id(int clk_id);
int get_clk_id(struct clk *clk);
//typedef int (*DUMP_CALLBACK)(char *, const char *, ...);
//int dump_out_clk(char *str);

void __init init_div_clk(struct clk *clk);
void __init init_cpccr_clk(struct clk *clk);
void __init init_ext_pll(struct clk *clk);
void __init init_gate_clk(struct clk *clk);
void __init cpm_pwc_init(void);
void __init init_pwc_clk(struct clk *clk);
void update_gate_clk(struct clk *clk);
int cpm_pwc_enable_ctrl(struct clk *clk,int on);
void cpm_pwc_suspend(void);
void cpm_pwc_resume(void);
#endif /* _CLK_H_ */
