#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <soc/cache.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <soc/ddr.h>
#include "clk.h"
static DEFINE_SPINLOCK(cpm_div_lock);
struct clk_selectors {
	unsigned int route[4];
};
enum {
	SELECTOR_A = 0,
	SELECTOR_B,
	SELECTOR_C,
	SELECTOR_D
};
const struct clk_selectors selector[] = {
#define CLK(X)	CLK_ID_##X
/*
 *		   bit31,bit30
 *			0	, 0		  STOP
 *			0	, 1		  SCLKA
 *			1	, 0		  MPLL
 *			1	, 1		  INVALID
 */
	[SELECTOR_A].route = {CLK(STOP),CLK(SCLKA),CLK(MPLL),CLK(INVALID)},
/*
 *		   bit31,bit30
 *			0	, x		  SCLKA
 *			0	, x		  MPLL
 *			1	, x		  VPLL
 *			1	, x		  VPLL
 */
	[SELECTOR_B].route	= {CLK(SCLKA),CLK(MPLL),CLK(VPLL),CLK(VPLL)},
/*
 *		   bit31,bit30
 *			0	, 0		  SCLKA
 *			0	, 1		  MPLL
 *			1	, 0		  VPLL
 *			1	, 1		  INVALID
 */
	[SELECTOR_C].route = {CLK(SCLKA) ,CLK(MPLL),CLK(VPLL),CLK(INVALID)},
/*
 *		   bit31,bit30
 *			0	, 0		  SCLKA
 *			0	, 1		  MPLL
 *			1	, 0		  VPLL
 *			1	, 1		  EXT1
 */
	[SELECTOR_D].route = {CLK(SCLKA),CLK(MPLL),CLK(VPLL),CLK(EXT1)},
#undef CLK
};

#define IS_DIV_CLK(x) (x&CLK_FLG_DIV)

struct div_clk {
	/* off: reg offset. ce_busy_stop: CE offset  + 1 is busy. coe : coe for div .div: div bit width */
	/* ext: extal/pll sel bit. sels: {select} */
	int off,ce_busy_stop,coe,div,sel,cache;
};
static struct div_clk div_clks[] = {
	[DIV_DDR] =		{ CPM_DDRCDR,	27, 1, 4, SELECTOR_A},
	[DIV_VPU] =		{ CPM_VPUCDR,	27, 1, 4, SELECTOR_B},
	[DIV_RSA] =		{ CPM_RSACDR,	27, 1, 4, SELECTOR_B},
	[DIV_GMAC] =	{ CPM_MACCDR,	27, 1, 8, SELECTOR_B},
#ifdef CONFIG_ZERATUL
	[DIV_SFC] =		{ CPM_SFC,		27, 4, 8, SELECTOR_C},
	[DIV_SFC1] =	{ CPM_SFC1,		27, 4, 8, SELECTOR_C},
#else
	[DIV_SFC] =		{ CPM_SFC,		27, 2, 8, SELECTOR_C},
	[DIV_SFC1] =	{ CPM_SFC1,		27, 2, 8, SELECTOR_C},
#endif
	[DIV_PWM] =		{ CPM_PWM,		27, 1, 8, SELECTOR_C},
	[DIV_SSI] =		{ CPM_SSI,		27, 1, 8, SELECTOR_C},
	[DIV_SSI_SLV] = { CPM_SSI_SLV,	27, 1, 8, SELECTOR_C},
	[DIV_LCD] =		{ CPM_LPCDR,	27, 1, 8, SELECTOR_C},
	[DIV_MSC0] =	{ CPM_MSC0CDR,	27, 4, 8, SELECTOR_D},
	[DIV_MSC1] =	{ CPM_MSC1CDR,	27, 4, 8, SELECTOR_D},
	[DIV_LDC] =		{ CPM_LDC,		27, 1, 8, SELECTOR_C},
	[DIV_I2S_SPK] =		{ CPM_I2SSPKCDR, 29, 0, 20, SELECTOR_B}, /*TODO:check*/
	[DIV_I2S_MIC] =		{ CPM_I2SMICCDR, 29, 0, 20, SELECTOR_B}, /*TODO:check*/
	[DIV_CIM] =		{ CPM_CIMCDR,	27, 1, 8, SELECTOR_B},
	[DIV_ISP] =		{ CPM_ISPMCDR,	27, 1, 4, SELECTOR_C},
	[DIV_ISPS] =	{ CPM_ISPSCDR,	27, 1, 4, SELECTOR_C},
	[DIV_ISPA] =	{ CPM_ISPACDR,	27, 1, 4, SELECTOR_C},
	[DIV_TNPU] =	{ CPM_TNPU,		27, 1, 4, SELECTOR_C},
};

static unsigned long div_get_rate(struct clk *clk)
{
	unsigned long x;
	unsigned long flags;
	unsigned int reg_val = 0;
	int no = CLK_DIV_NO(clk->flags);

	if (!(clk->parent)) {
		printk("%s, parent is invalid, please check if the PLL exists!\n", clk->name);
		clk->flags &= ~CLK_FLG_ENABLE;
		return 0;
	}
	if (!(strcmp(clk->name, "div_i2s_spk")) || !(strcmp(clk->name, "div_i2s_mic"))) {
		int m = 0, n = 0;
		reg_val = cpm_inl(div_clks[no].off);
		n = reg_val & 0xfffff;
		m = (reg_val >> 20) & 0x1ff;

		/*printk(KERN_DEBUG"%s, parent = %ld, rate = %ld, m = %d, n = %d, reg val = 0x%08x\n",
				__func__, clk->parent->rate, clk->rate, m, n, cpm_inl(div_clks[no].off));*/
		return (clk->parent->rate * m) / n;
	}

	if(clk->parent == get_clk_from_id(CLK_ID_EXT1))
		return clk->parent->rate;
	if(div_clks[no].div == 0)
		return clk_get_rate(clk->parent);
	spin_lock_irqsave(&cpm_div_lock,flags);
	if(!(strcmp(clk->name, "div_sfc0")) || !(strcmp(clk->name, "div_sfc1")))
	{
		reg_val = cpm_inl(div_clks[no].off) >> 8;
		reg_val &= 0x3;
		if (reg_val == 0x2)
			div_clks[no].coe = 4;
		else
			div_clks[no].coe = 2;
	}
	x = cpm_inl(div_clks[no].off);
	x &= (1 << div_clks[no].div) - 1;
	x = (x + 1) * div_clks[no].coe;

	spin_unlock_irqrestore(&cpm_div_lock,flags);
	if (!(strcmp(clk->name, "div_ddr")))
		return clk->parent->rate / x * 2;
	else
		return clk->parent->rate / x;
}
static int div_enable(struct clk *clk,int on)
{
	int no = CLK_DIV_NO(clk->flags);
	int reg_val;
	int ce,stop,busy;
	int prev_on;
	unsigned int mask;
	unsigned long flags;

	spin_lock_irqsave(&cpm_div_lock,flags);

	if (!(strcmp(clk->name, "div_i2s_spk")) || !(strcmp(clk->name, "div_i2s_mic"))) {
		reg_val = cpm_inl(div_clks[no].off);
		reg_val |= (1 << 29);
		cpm_outl(reg_val,div_clks[no].off);
		printk(KERN_DEBUG"%s,%s reg val = 0x%08x\n",
				__func__, clk->name, cpm_inl(div_clks[no].off));
		goto div_enable_finish;
	}

	reg_val = cpm_inl(div_clks[no].off);
	stop = div_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	prev_on = !(reg_val & (1 << stop));
	mask = (1 << div_clks[no].div) - 1;
	if(prev_on && on)
		goto div_enable_finish;
	if((!prev_on) && (!on))
		goto div_enable_finish;
	if(on){
		if(div_clks[no].cache && ((div_clks[no].cache & mask) != (reg_val & mask))) {
			unsigned int x = div_clks[no].cache;
			x = (x & ~(0x1 << stop)) | (0x1 << ce);
			cpm_outl(x,div_clks[no].off);
#ifndef CONFIG_FPGA_TEST
			while(cpm_test_bit(busy,div_clks[no].off)) {
				//printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
			}
#endif
			cpm_clear_bit(ce, div_clks[no].off);
			x &= (1 << div_clks[no].div) - 1;
			x = (x + 1) * div_clks[no].coe;
			clk->rate = clk->parent->rate / x;
			div_clks[no].cache = 0;
		} else {
			reg_val |= (1 << ce);
			reg_val &= ~(1 << stop);
			cpm_outl(reg_val,div_clks[no].off);
			cpm_clear_bit(ce,div_clks[no].off);
		}
	} else {
		reg_val |= (1 << ce);
		reg_val |= ( 1<< stop);
		cpm_outl(reg_val,div_clks[no].off);
		cpm_clear_bit(ce,div_clks[no].off);
	}

div_enable_finish:
	spin_unlock_irqrestore(&cpm_div_lock,flags);
	return 0;
}

static int div_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long x,tmp;
	int i,no = CLK_DIV_NO(clk->flags);
	int ce,stop,busy;

	unsigned int reg_val, mask;
	unsigned long flags;
	unsigned long long m, n;
	unsigned long long m_mul;
	unsigned long long tmp_value;
	unsigned long long tmp_rate = (unsigned long long)rate;
	unsigned char sig = 0;
	unsigned long long max = 0xfffff;
	unsigned long long min = 1;

    if (!(strcmp(clk->name, "div_i2s_spk")) || !(strcmp(clk->name, "div_i2s_mic"))) {
		for(m=1;m<=0x1ff;m++)
		{
			m_mul = clk->parent->rate * m;
			max = 0xfffff;
			min = 1;
			while(max >= min){
				n = (max + min) >> 1;
				tmp_value = (unsigned long long)(n*tmp_rate);
				if( m_mul == tmp_value )
				{
					sig = 1;
					break;
				}else if (tmp_value > m_mul)
					max = n - 1;
				else
					min = n + 1;
			}
			if( sig )
			{
				clk->rate = rate;
				break;
			}
		}

		reg_val = cpm_inl(div_clks[no].off) & 0xf0000000;
		reg_val |= (m << 20) | (n << 0);
		cpm_outl(reg_val,div_clks[no].off);
		printk(KERN_DEBUG"%s, parent = %ld, rate = %ld, n = %lld, reg val = 0x%08x\n",
				__func__, clk->parent->rate, clk->rate, n, cpm_inl(div_clks[no].off));
		return 0;
	}
	spin_lock_irqsave(&cpm_div_lock,flags);
	if(!(strcmp(clk->name, "div_sfc0")) || !(strcmp(clk->name, "div_sfc1")))
	{
		reg_val = cpm_inl(div_clks[no].off) >> 8;
		reg_val &= 0x3;
		if (reg_val == 0x2)
			div_clks[no].coe = 4;
		else
			div_clks[no].coe = 2;
	}
	mask = (1 << div_clks[no].div) - 1;
	tmp = clk->parent->rate / div_clks[no].coe;
	for (i = 1; i <= mask+1; i++) {
		if ((tmp / i) <= rate)
			break;
	}
	i--;
	if(i > mask)
		i = mask;
	reg_val = cpm_inl(div_clks[no].off);
	x = reg_val & ~mask;
	x |= i;
	stop = div_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	if(x & (1 << stop)) {
		if (div_clks[no].cache) {
			div_clks[no].cache &= ~mask;
			div_clks[no].cache |= i;
		} else {
			div_clks[no].cache = x;
		}
		clk->rate = tmp  / (i + 1);
	}
	else if((mask & reg_val) != i){

		x = (x & ~(0x1 << stop)) | (0x1 << ce);
		cpm_outl(x, div_clks[no].off);
#ifndef CONFIG_FPGA_TEST
		while(cpm_test_bit(busy,div_clks[no].off));
            //printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
#endif
		x &= ~(1 << ce);
		cpm_outl(x, div_clks[no].off);
		div_clks[no].cache = 0;
		clk->rate = tmp  / (i + 1);
	}
	spin_unlock_irqrestore(&cpm_div_lock,flags);
	return 0;
}

static struct clk * div_get_parent(struct clk *clk)
{
	unsigned int no,div,idx,pidx;

	no = CLK_DIV_NO(clk->flags);
	if(!(strcmp(clk->name, "div_i2s_mic"))){
		/* the operation uses for t32, because that the bits[30~31] of 0x10000084 is readonly */
		div = *(volatile unsigned int*)(0xb0000070); // the register is i2s speaker pll.
	}else{
		div = cpm_inl(div_clks[no].off);
	}
	idx = div >> 30;
	pidx = selector[div_clks[no].sel].route[idx];
	if (pidx == CLK_ID_STOP || pidx == CLK_ID_INVALID)
		return NULL;

	return get_clk_from_id(pidx);
}

static int div_set_parent(struct clk *clk, struct clk *parent)
{
	int i,tmp;
	int no = CLK_DIV_NO(clk->flags);
	unsigned int reg_val,div,mask;
	int ce,stop,busy;
	unsigned long flags;
	stop = div_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	mask = (1 << div_clks[no].div) - 1;

	for(i = 0;i < 4;i++) {
		if(selector[div_clks[no].sel].route[i] == get_clk_id(parent)){
			break;
		}
	}
	if(i >= 4)
		return -EINVAL;
	spin_lock_irqsave(&cpm_div_lock,flags);
	div = cpm_inl(div_clks[no].off);
	reg_val = div;
	div &= ~(3 << 30);
	div |= i << 30;

	tmp = parent->rate / div_clks[no].coe;
	for (i = 1; i <= mask+1; i++) {
		if ((tmp / i) <= clk->rate)
			break;
	}
	i--;
	mask = (1 << div_clks[no].div) - 1;
	div = (div & ~(0x1 << stop)) | (0x1 << ce);
	div = div & ~mask;
	div |= i;

	if(reg_val & (1 << stop))
		div_clks[no].cache = div;
	else if((mask & reg_val) != i){
		cpm_outl(div, div_clks[no].off);
#ifndef CONFIG_FPGA_TEST
		while(cpm_test_bit(busy,div_clks[no].off));
			//printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
#endif
		div &= ~(1 << ce);
		cpm_outl(div, div_clks[no].off);
		div_clks[no].cache = 0;
	}

	clk->parent = parent;

	spin_unlock_irqrestore(&cpm_div_lock,flags);
	return 0;
}

static int div_is_enabled(struct clk *clk) {
	static	unsigned long clkgr[2]={0};
	int no = CLK_DIV_NO(clk->flags);
	int stop;
	clkgr[0] = cpm_inl(CPM_CLKGR);
	clkgr[1] = cpm_inl(CPM_CLKGR1);
	stop = div_clks[no].ce_busy_stop;
	if(no == DIV_ISPS || no == DIV_ISPA)
		no = DIV_ISP;
	if(no == DIV_CIM)
		return !(cpm_inl(div_clks[no].off) & (1 << stop));
	if(clkgr[no / 32] & (1 << (no % 32))) {
		return 0;
	}else {
		return !(cpm_inl(div_clks[no].off) & (1 << stop));
	}
}
static struct clk_ops clk_div_ops = {
	.enable	= div_enable,
	.get_rate = div_get_rate,
	.set_rate = div_set_rate,
	.get_parent = div_get_parent,
	.set_parent = div_set_parent,
};

#define CONVERT_DDR_RATE (50 * 1000 * 1000)
#define UP_DIV 1
#define DOWN_DIV 2
#define REG32(x) *(volatile unsigned int *)(x)

#define OFF_TDR			(0x00)
#define OFF_LCR			(0x0C)
#define OFF_LSR			(0x14)

#define LSR_TDRQ		(1 << 5)
#define LSR_TEMT		(1 << 6)

#define U1_IOBASE (UART1_IOBASE + 0xa0000000)
#define TCSM_PCHAR(x)							\
	*((volatile unsigned int*)(U1_IOBASE+OFF_TDR)) = x;		\
	while ((*((volatile unsigned int*)(U1_IOBASE + OFF_LSR)) & (LSR_TDRQ | LSR_TEMT)) != (LSR_TDRQ | LSR_TEMT))

static inline void serial_put_hex(unsigned int x) {
	int i;
	unsigned int d;
	for(i = 7;i >= 0;i--) {
		d = (x	>> (i * 4)) & 0xf;
		if(d < 10) d += '0';
		else d += 'A' - 10;
		TCSM_PCHAR(d);
	}
}
static int ddr_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int ddr_div = cpm_inl(CPM_DDRCDR);
	unsigned int div = ddr_div & 0xf;
	unsigned int up = 0;
	unsigned int ret = 0;
	unsigned long flags,fail;

    if(rate > CONVERT_DDR_RATE){
		if(clk->rate <= CONVERT_DDR_RATE) {
			up = UP_DIV;
			div >>= 1;
			clk->rate = 100 * 1000 * 1000;
		}
	}else{
		if(clk->rate > CONVERT_DDR_RATE) {
			up = DOWN_DIV;
			div <<= 1;
			clk->rate = 50 * 1000 * 1000;
		}
	}
	spin_lock_irqsave(&cpm_div_lock,flags);
	cache_prefetch(DDRCLAB1,200);
	fast_iob();
DDRCLAB1:
	REG32(0xb3012068) = 0;

	ddr_div |= (1 << 29) | (1 << 26);
	ddr_div &= ~(1 << 24);
	ddr_div &= ~0xf;
	TCSM_PCHAR('1');
	cpm_outl(ddr_div,CPM_DDRCDR);
	TCSM_PCHAR('a');
	ddr_div |= (1 << 25);
	ddr_div |= div;
	TCSM_PCHAR('2');
	cpm_outl(ddr_div,CPM_DDRCDR);
	fail = 0;
	TCSM_PCHAR('3');
	while(cpm_inl(CPM_DDRCDR) & 1 << 28) {
		serial_put_hex(cpm_inl(CPM_DDRCDR));
		TCSM_PCHAR('\r');
		TCSM_PCHAR('\n');
	}
	TCSM_PCHAR('4');
	cpm_clear_bit(25,CPM_DDRCDR);
	REG32(0xb3012068) = 0xf0000100;
		fail = REG32(0xa0000000);
	if(cpm_inl(CPM_DDRCDR) & 1 << 24) {
		printk("fail!\n");
	}else
		printk("ok!\n");

	//printk("REG32(0xb301206c) = %x\n",REG32(0xb301206c));

	spin_unlock_irqrestore(&cpm_div_lock,flags);
	return ret;

}
static struct clk_ops clk_ddr_ops = {
	.enable	= div_enable,
	.get_rate = div_get_rate,
	.set_rate = ddr_set_rate,
	.get_parent = div_get_parent,
	.set_parent = div_set_parent,

};
void __init init_div_clk(struct clk *clk)
{
	int no;
	int id;
#ifdef CONFIG_ZERATUL
	unsigned int reg_val;
#endif
	//printk("REG32(0xb3012088) = %x\n",REG32(0xb3012088));


	if (clk->flags & CLK_FLG_PARENT) {
		id = CLK_PARENT(clk->flags);
		clk->parent = get_clk_from_id(id);
	} else {
		clk->parent = div_get_parent(clk);
	}
	no = CLK_DIV_NO(clk->flags);
	div_clks[no].cache = 0;
	clk->rate = div_get_rate(clk);
	if(div_is_enabled(clk)) {
		clk->flags |= CLK_FLG_ENABLE;
	}

#ifdef CONFIG_ZERATUL
	if(no == DIV_SFC || no == DIV_SFC1){
		reg_val = cpm_inl(div_clks[no].off);
		reg_val &= ~(0x3 << 8);
		if(div_clks[no].coe == 4)
			reg_val |= (0x2 << 8);
		else if(div_clks[no].coe == 2)
			reg_val |= (0x1 << 8);
		cpm_outl(reg_val, div_clks[no].off);
	}
#endif
	if(no == DIV_DDR){
		clk->ops = &clk_ddr_ops;
		if(ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP){	/*TODO:zm*/
					/**
					 * DDR request cpm to stop clk
					 * (0x9 << 28) DDR_CLKSTP_CFG (0x13012068)
					 * CPM response ddr stop clk request (1 << 26) (0x1000002c)
					 */
					cpm_set_bit(26,CPM_DDRCDR);
					REG32(0xb3012068) |= 0x9 << 28;
				}
		REG32(0xb3012088) |= 4 << 16;
	}
	else
		clk->ops = &clk_div_ops;

}
