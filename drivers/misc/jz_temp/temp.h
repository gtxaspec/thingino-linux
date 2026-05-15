#ifndef __TSENSOR_H__
#define __TSENSOR_H__

#define ADENA	0x00
#define ADCFG	0x04
#define ADCTRL	0x08
#define ADSTATE	0x0c
#define ADATA0	0x10
#define ADATA1	0x0014
#define ADCLK	0x20
#define ADSTB	0x24
#define ADRETM	0x28

#define TEMP_IOC_MAGIC 'T'
#define TSENSOR_GET_TEMP _IOWR(TEMP_IOC_MAGIC,1,int)

#define TEMP_DIV_NUM 5
#define EINVALTEMP (0xffff)
#define EFUSETSENSOR	(0xb3540000+0x218)
// #define TSENSOR_DBUG

struct stemp_operation {
	struct miscdevice temp_dev;
	struct resource *res;
	const struct mfd_cell *cell;
	void __iomem *iomem;
	struct clk *clk;
	struct device *dev;
	char name[16];
	int state;
	struct proc_dir_entry *proc;
};

int tsensor_value[] = {
	2539,2509,2478,2447,2415,
	2383,2351,2319,2287,2255,
	2223,2190,2158,2125,2092,
	2059,2025,1991,1957,1924,
	1890,1855,1822,1787,1753,
	1719,1685,1650,1615,1581,
	1546,1510,1475,1439,
};

int temprange[] = {
	-40, -35, -30, -25, -20,
	-15, -10, -5 , 0  , 5  ,
	10 , 15 , 20 , 25 , 30 ,
	35 , 40 , 45 , 50 , 55 ,
	60 , 65 , 70 , 75 , 80 ,
	85 , 90 , 95 , 100, 105,
	110, 115, 120, 125,
};

#ifdef CONFIG_TS_AUTO_REFI
#define CONFIG_AUTOREFI_DELAY 30000 //30S
// #define TS_DEBUG
#ifdef TS_DEBUG
#define ts_debug(fmt, args...) printk("[TSensor] %s %d -> " fmt, __FILE__, __LINE__, ##args)
#else
#define ts_debug(fmt, args...)
#endif

struct ts_AutoRefiInfo {
	struct timer_list tsensor_timer;
	struct workqueue_struct *wq;
	struct work_struct wrk;

	unsigned int trefi;
	unsigned int Trefi7800;
	unsigned int Trefi3900;
	unsigned int Trefi2920;

	struct stemp_operation *tsensor_stemp_ope;
};

struct ts_AutoRefiInfo tsPI;

#define SRWEN   (0x328)
#define SRPCEN  (0x320)
#define RTR     (0x064)

#define tswritel(addr,value)  (*(volatile unsigned int*)(0xb3012000 + addr) = value)
#define tsreadl(addr)         (*(volatile unsigned int*)(0xb3012000 + addr))
#endif

#endif /*__ingenic_SPI_DEV_H__*/
