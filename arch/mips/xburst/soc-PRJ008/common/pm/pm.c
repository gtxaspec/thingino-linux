#include <linux/init.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <soc/cache.h>
#include <soc/base.h>
#include <asm/io.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include "pm.h"
#include "pm_fastboot.h"
#if defined CONFIG_KP_AXP
#include <linux/regulator/machine.h>
#endif

static unsigned long pm_suspend_start_time;
uint8_t *mem_test_rgn = NULL;

static int soc_pm_fastboot(void)
{
	int ret = 0;
	/* soc_pm_set_hwcr_cycles(1); */
	/* soc_pm_set_hwfcr_cycles(1); */
	setting_debug_mode(0);//1 open debug,0 close debug
	get_compilation_time();
	ret = soc_pm_fastboot_config();
	if(ret < 0){
		pr_err("%s config fastboot error\n",__func__);
		return ret;
	}
	load_func_to_rtc_ram();

	sys_save();
	load_func_to_oram();
	return 0;
}

static void goto_sleep(unsigned int sleep_addr)
{
	mb();
	save_goto(sleep_addr);
	mb();
}

int t33_pm_enter(suspend_state_t state)
{
	unsigned int sleep_addr = 0;
	unsigned int ret =0;

	ret = soc_pm_fastboot();
	if(ret < 0){
		return ret;
	}
	sleep_addr = FASTBOOT_SLEEP_CODE_ADDR;

	{
		long nj = jiffies - pm_suspend_start_time;
		unsigned msec;

		msec = jiffies_to_msecs(abs(nj));
		pr_info("PM: %s completion time %d.%03d seconds\n", "Suspend",
			msec / 1000, msec % 1000);
	}

	goto_sleep((unsigned int )sleep_addr);

	soc_pm_wakeup_fastboot();
	return 0;
}

static int t33_pm_begin(suspend_state_t state)
{
	printk("t33 suspend begin\n");
	pm_suspend_start_time = jiffies;
	return 0;
}

static void t33_pm_end(void)
{
	printk("t33 pm end!\n");
}
#if defined CONFIG_KP_AXP
static int t33_suspend_prepare(void)
{
	return regulator_suspend_prepare(PM_SUSPEND_MEM);
}
static void t33_suspend_finish(void)
{
	if (regulator_suspend_finish())
		pr_err("%s: Suspend finish failed\n", __func__);
}
#endif

static int ingenic_pm_valid(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_ON:
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;

	default:
		return 0;
	}
}
static const struct platform_suspend_ops t33_pm_ops = {
	.valid		= ingenic_pm_valid,
	.begin		= t33_pm_begin,
	.enter		= t33_pm_enter,
	.end		= t33_pm_end,
#if defined CONFIG_KP_AXP
	.prepare = t33_suspend_prepare,
	.finish = t33_suspend_finish,
#endif
};


int bc_idx = 0;
static int __init suspend_console_setup(char *str)
{

	char buf[32];
	char *s;

	strncpy(buf, str, sizeof(buf) - 1);

	for (s = buf; *s; s++)
		if (isdigit(*s) || *s == ',')
			break;

	bc_idx = simple_strtoul(s, NULL, 10);

	return 0;
}

__setup("console=", suspend_console_setup);

/*
 * Initialize suspend interface
 */
static int __init pm_init(void)
{
#ifdef CONFIG_PM
	int ret = 0;
	suspend_set_ops(&t33_pm_ops);
	mem_test_rgn = kmalloc(4096,GFP_KERNEL);
	if(mem_test_rgn == NULL){
		printk("pm init mem test failed\n");
	}
	ret = ingenic_fastboot_init();
	ret += ingenic_zboost_init();
	if(ret != 0){
		printk("pm init failed\n");
	}
#endif /* ifdef CONFIG_PM */
	return 0;
}
static void __exit pm_deinit(void)
{
#ifdef CONFIG_PM
	if(mem_test_rgn)
		kfree(mem_test_rgn);
	ingenic_fastboot_deinit();
	ingenic_zboost_exit();
#endif /* ifdef CONFIG_PM */
}
late_initcall(pm_init);
module_exit(pm_deinit);
