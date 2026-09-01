/**
    NVT pll preset function
    This file will preset PLL value
    @file       pll_preset.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/clk-provider.h>
#include <plat/nvt-sramctl.h>
#include <plat/top.h>
#include <plat/efuse_protected.h>

static void sp_clk_sel(void)
{
	struct device_node *node;
	struct clk *clk, *pll_clk;
	char *special_clk_parent[] = {"fix480m", "pll4", "pll5", "pll6"};
	u32 value = 0;
	
	node = of_find_node_by_path("/sp_clk@0");
	if (node) {
		
		if (!of_property_read_u32_array(node, "sp_clksel_config", &value, 1)) {
				pll_clk = clk_get(NULL, special_clk_parent[value]);
				//pr_err("sp %s %d\n", __func__, value);
				clk = clk_get(NULL, "sp_clk");
				if ((!IS_ERR(clk)) && (!IS_ERR(pll_clk)))
					clk_set_parent(clk, pll_clk);
				else
					pr_err("%s get clock err\n", __func__);
		}
		
		if (!of_property_read_u32_array(node, "sp2_clksel_config", &value, 1)) {
				pll_clk = clk_get(NULL, special_clk_parent[value]);
				//pr_err("sp2 %s %d\n", __func__, value);
				clk = clk_get(NULL, "sp2_clk");
				if ((!IS_ERR(clk)) && (!IS_ERR(pll_clk)))
					clk_set_parent(clk, pll_clk);
				else
					pr_err("%s get clock err\n", __func__);					
		}			

	}
}


static void disable_cnn(void)
{
	struct clk* turnoff_clk;

	turnoff_clk = clk_get(NULL, "f0d60000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		clk_set_phase(turnoff_clk, 1);
		nvt_enable_sram_shutdown(CNN2_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get CNN2 Clock error\n", __func__);

	turnoff_clk = clk_get(NULL, "f0cb0000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		nvt_enable_sram_shutdown(CNN_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get CNN Clock error\n", __func__);

	turnoff_clk = clk_get(NULL, "f0d50000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		nvt_enable_sram_shutdown(NUE2_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get NUE2 Clock error\n", __func__);

	turnoff_clk = clk_get(NULL, "f0c60000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		nvt_enable_sram_shutdown(NUE_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get NUE Clock error\n", __func__);
}

static void check_cpu_freq(void)
{
	if (nvt_get_chip_id() == CHIP_NA51084) {
		struct clk* pll8_clk; 
		unsigned long cpu_clk;

		pll8_clk = clk_get(NULL, "pll8");
		cpu_clk = clk_get_rate(pll8_clk);
		cpu_clk *= 8;

		//1. cpu_clk > 1.35GHz set as 1.35GHz
		if (cpu_clk > (UINT32)1350000000) {
			if (efuse_check_available_extend(EFUSE_ABILITY_CPU_PLL_FREQ , 1350000000) == TRUE) {
				pr_err("cpu clock exceeds limitation with %ld, reset to 1.35 GHz\n", cpu_clk);
				clk_set_rate(pll8_clk, 1350000000 / 8);
			} else {
				if (efuse_check_available_extend(EFUSE_ABILITY_CPU_PLL_FREQ , 1200000000) == TRUE) {
					pr_err("cpu clock exceeds limitation with %ld, reset to 1.2 GHz\n", cpu_clk);
					clk_set_rate(pll8_clk, 1200000000 / 8);
				} else {
					pr_err("cpu clock exceeds limitation with %ld, reset to 1 GHz\n", cpu_clk);
					clk_set_rate(pll8_clk, 1000000000 / 8);
				}
			} 
			//2. 1.2GHz < cpu_clk <= 1.35GHz (only overclock can usage)									
		} else if (cpu_clk <= (UINT32)1350000000 && cpu_clk > 1200000000) {
			if (efuse_check_available_extend(EFUSE_ABILITY_CPU_PLL_FREQ , cpu_clk) == FALSE) {
				if (efuse_check_available_extend(EFUSE_ABILITY_CPU_PLL_FREQ , 1200000000) == TRUE) {
					pr_err("cpu clock exceeds limitation with %ld, reset to 1.2 GHz\n", cpu_clk);
					clk_set_rate(pll8_clk, 1200000000 / 8);
				} else {
					pr_err("cpu clock exceeds limitation with %ld, reset to 1 GHz\n", cpu_clk);
					clk_set_rate(pll8_clk, 1000000000 / 8);
				}
			} else {
				// over clock > 1.2G is available
				//pr_info("over clock > 1.2G OK %ldHz\r\n", cpu_clk);				
			}
		} else if (cpu_clk <= (UINT32)1200000000 && cpu_clk > 1000000000) {
			if (efuse_check_available_extend(EFUSE_ABILITY_CPU_PLL_FREQ , cpu_clk) == FALSE) {
				pr_err("cpu clock exceeds limitation with %ld, reset to 1 GHz\n", cpu_clk);
				clk_set_rate(pll8_clk, 1000000000 / 8);				
			} else {
				// over clock > 1.05G is available
				//pr_info("over clock > 1.05G OK %ldHz\r\n", cpu_clk);
			}
		}
		
//		if (efuse_check_available_extend(EFUSE_ABILITY_CPU_PLL_FREQ , cpu_clk) == FALSE) {
//			pr_err("cpu clock exceeds limitation with %ld, reset to 1.05 GHz\n", cpu_clk);
//			clk_set_rate(pll8_clk, 1050000000 / 8);
//		}
	}
}

static int __init nvt_preset_pll(void)
{
	struct device_node *node, *pll;
	struct clk* pll_clk;
	char pll_name[10] = {};
	u32 value[3] = {};

	node = of_find_node_by_path("/pll_preset@0");
	if (node) {
		for_each_child_of_node(node, pll) {
			if (!of_property_read_u32_array(pll, "pll_config", value, 3)) {
				if (value[1]) {
					if (value[0] == 24)
						snprintf(pll_name, 10, "pllf320");
					else
						snprintf(pll_name, 10, "pll%d", value[0]);

					pll_clk = clk_get(NULL, pll_name);
					if (!IS_ERR(pll_clk))
						clk_set_rate(pll_clk, value[1]);
					else
						pr_err("*** %s Get PLL%d Clock error\n", __func__, value[0]);
				}

				if (value[2]) {
					if (value[0] == 24)
						snprintf(pll_name, 10, "pllf320");
					else
						snprintf(pll_name, 10, "pll%d", value[0]);

					pll_clk = clk_get(NULL, pll_name);
					if (!IS_ERR(pll_clk)) {
						if (!__clk_is_enabled(pll_clk))
							clk_prepare_enable(pll_clk);
					} else
						pr_err("*** %s Get PLL%d Clock error\n", __func__, value[0]);
				}
			}
		}
	} else
		pr_err("*** %s not get dts node ***\n", __func__);

	disable_cnn();

	check_cpu_freq();

	sp_clk_sel();

	return 0;
}

arch_initcall_sync(nvt_preset_pll);
