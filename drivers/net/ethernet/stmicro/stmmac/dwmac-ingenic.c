/*
 * dwmac-ingenic.c - Ingenic SOC DWMAC specific glue layer
 *
 * Copyright (C) 2021 Ingenic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/stmmac.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/of_net.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_net.h>
#include "stmmac_platform.h"


#define INGENIC_ETH_SEL_MASK		(0x7)
#define INGENIC_ETH_SEL_RMII		(0x4)

#define INGENIC_ETH_SPEED_MASK		(0x3<<29)
#define INGENIC_ETH_SPEED_10M		(0x2<<29)
#define INGENIC_ETH_SPEED_100M		(0x3<<29)

struct ingenic_gmac_data {
	int interface;
	int clk_enabled;
	unsigned int *cpm_phyc_reg;
	/*hw reset*/
	u32 reset_ms;
	int reset_gpio;
	u32 reset_lvl;
	struct clk *clk_gate;
	struct clk *macphy_clk;
	unsigned int macphy_rate;
	unsigned int macphy_type;
	unsigned int mac_max_speed;
};

#if 1
static int ingenic_gmac_phy_hwrst(struct device *dev, bool init,struct ingenic_gmac_data  *gmac)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int ret = 0;

	if (IS_ERR_OR_NULL(gmac)) {
		dev_err(dev, "gmac is null\n");
		return -1;
	}

	gmac->reset_gpio = of_get_named_gpio_flags(np, "ingenic,rst-gpio", 0, &flags);
	if (gmac->reset_gpio < 0){
		ret = gmac->reset_gpio;
		dev_err(dev, "ingenic mac-hw-rst gpio request failed errno(%d)\n", ret);
		return -1;
	}
	ret = devm_gpio_request(dev, gmac->reset_gpio, "mac-hw-rst");
	if (ret) {
		gmac->reset_gpio = ret;
		dev_err(dev, "ingenic mac-hw-rst gpio request failed errno(%d)\n", ret);
		goto out;
	}

	gmac->reset_lvl = flags & OF_GPIO_ACTIVE_LOW ? 0 : 1;

	if (of_property_read_u32(np, "ingenic,rst-ms", &gmac->reset_ms) < 0) {
		gmac->reset_ms = 10;
	}

	if (!init)
		goto hw_reset;
hw_reset:
	gpio_direction_output(gmac->reset_gpio, gmac->reset_lvl);
	if (in_atomic())
		mdelay(gmac->reset_ms);
	else
		msleep(gmac->reset_ms);
	gpio_direction_output(gmac->reset_gpio, !gmac->reset_lvl);
	mdelay(80);// need delay any time to read phy reg.

out:
	gpio_free(gmac->reset_gpio);

	return ret;
}
#endif

static int ingenic_gmac_parse_data(struct ingenic_gmac_data *gmac, struct device *dev)
{
	int ret = 0;
	unsigned int reg = 0;

#ifndef CONFIG_FPGA_TEST
	char clk_gate[16] = {0};
	char clk_phy[16] = {0};

	memcpy(clk_gate,"gate_gmac", strlen("gate_gmac"));
	gmac->clk_gate = devm_clk_get(dev, clk_gate);
	if (IS_ERR(gmac->clk_gate)) {
		dev_err(dev, "Could not get clock gate\n");
		return PTR_ERR(gmac->clk_gate);
	}

	memcpy(clk_phy,"div_macphy", strlen("div_macphy"));
	gmac->macphy_clk = devm_clk_get(dev, clk_phy);
	if (IS_ERR(gmac->macphy_clk)) {
		dev_err(dev, "Could not get macphy_clk clock\n");
		return PTR_ERR(gmac->macphy_clk);
	}

#endif
	ret = of_property_read_u32(dev->of_node, "macphy-rate", &gmac->macphy_rate);
	if (ret < 0) {
		gmac->macphy_rate = 25000000;
		dev_err(dev, "Could not get macphy_rate, error!!!\n");
	}

	ret = of_property_read_u32(dev->of_node, "max-speed", &gmac->mac_max_speed);
	if (ret < 0) {
		dev_err(dev, "Could not get mac max-speed, error!!!\n");
	}

	ret = of_property_read_u32(dev->of_node, "ingenic,mode-reg", &reg);
	if (ret < 0)
		return ret;
	gmac->cpm_phyc_reg = ioremap(reg, 4);
	printk("%s[%d]: cpm_phyc_reg = 0x%08x\n",__func__,__LINE__,reg);

	return ret;
}

static int ingenic_gmac_init(struct platform_device *pdev, void *priv)
{
	int ret = 0;
	struct ingenic_gmac_data *gmac = priv;

	if (!gmac) {
		dev_err(&pdev->dev, "gmac is NULL in ingenic_gmac_init\n");
		return -EINVAL;
	}

	if (!gmac->cpm_phyc_reg) {
		dev_err(&pdev->dev, "cpm_phyc_reg is NULL\n");
		return -EINVAL;
	}

	*gmac->cpm_phyc_reg &= ~(INGENIC_ETH_SEL_MASK | INGENIC_ETH_SPEED_MASK);
	switch (gmac->interface) {
	case PHY_INTERFACE_MODE_MII:
		break;
	case PHY_INTERFACE_MODE_RMII:
		*gmac->cpm_phyc_reg |= INGENIC_ETH_SEL_RMII;
		pr_debug("INGENIC ETH init : PHY_INTERFACE_MODE_RMII\n");
		break;
	case PHY_INTERFACE_MODE_GMII:
		break;
	case PHY_INTERFACE_MODE_RGMII:
		break;
	default:
		pr_warn("INGENIC ETH init :  Do not manage %d interface\n",
			 gmac->interface);
		return -EINVAL;
	}

	switch (gmac->mac_max_speed) {
	case 10:
		*gmac->cpm_phyc_reg |= INGENIC_ETH_SPEED_10M;
		break;
	case 100:
		*gmac->cpm_phyc_reg |= INGENIC_ETH_SPEED_100M;
		break;
	default:
		pr_warn("INGENIC ETH Speed init:  Do not manage %d interface\n",
			 gmac->mac_max_speed);
		return -EINVAL;
	}
	printk("%s[%d]: mac max speed %d\n",__func__,__LINE__, gmac->mac_max_speed);

#ifndef CONFIG_FPGA_TEST
	clk_set_rate(gmac->macphy_clk, gmac->macphy_rate);
	ret = clk_prepare_enable(gmac->macphy_clk);
	if (ret) {
		printk("%s[%d]: rmii ret %d\n",__func__,__LINE__,ret);
	}
	ret = clk_prepare_enable(gmac->clk_gate);
	if (ret) {
		printk("%s[%d]: rmii ret %d\n",__func__,__LINE__,ret);
	}
#endif

	gmac->clk_enabled = 1;
	return ret;
}

static void ingenic_gmac_exit(struct platform_device *pdev, void *priv)
{
	struct ingenic_gmac_data *gmac = priv;

	if(gmac->clk_enabled){
#ifndef CONFIG_FPGA_TEST
		clk_disable_unprepare(gmac->macphy_clk);
		clk_disable_unprepare(gmac->clk_gate);
#endif
	}
	gmac->clk_enabled = 0;

}

static int ingenic_gmac_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	struct ingenic_gmac_data *gmac;
	struct device *dev = &pdev->dev;
	int ret = 0;
	/* printk("############### %s[%d]: start\n",__func__,__LINE__); */

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	if (pdev->dev.of_node) {
		plat_dat = stmmac_probe_config_dt(pdev, stmmac_res.mac);
		if (IS_ERR(plat_dat)) {
			dev_err(&pdev->dev, "dt configuration failed\n");
			return PTR_ERR(plat_dat);
		}
	} else {
		plat_dat = dev_get_platdata(&pdev->dev);
		if (!plat_dat) {
			dev_err(&pdev->dev, "no platform data provided\n");
			return  -EINVAL;
		}

		/* Set default value for multicast hash bins */
		plat_dat->multicast_filter_bins = HASH_TABLE_SIZE;

		/* Set default value for unicast filter entries */
		plat_dat->unicast_filter_entries = 1;
	}


	gmac = devm_kzalloc(dev, sizeof(*gmac), GFP_KERNEL);
	if (!gmac) {
		ret = -ENOMEM;
		goto err_remove_config_dt;
	}

	gmac->interface = plat_dat->interface;
	ret = ingenic_gmac_parse_data(gmac, &pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "Unable to parse OF data\n");
		goto err_remove_config_dt;
	}
#if 1
	//reset phy
	if (ingenic_gmac_phy_hwrst(&pdev->dev, true, gmac)) {
		ret = -ENODEV;
		goto err_remove_config_dt;
	}
#endif


	/*
	 * platform data specifying hardware features and callbacks.
	 * hardware features were copied from a1 drivers.
	 * */
	plat_dat->tx_coe = true;
	plat_dat->rx_coe = true;
	/* plat_dat->has_xgmac = true; */
	/* plat_dat->tso_en = true; */
	/* plat_dat->pmt = false; */
	plat_dat->max_speed = 100;
	plat_dat->has_gmac = true;
	plat_dat->bsp_priv = gmac;
	plat_dat->init = ingenic_gmac_init;
	plat_dat->exit = ingenic_gmac_exit;

	/* Custom initialisation (if needed) */
	if (plat_dat->init) {
		ret = plat_dat->init(pdev, plat_dat->bsp_priv);
		if (ret) {
			dev_err(&pdev->dev, "stmmac driver init failed\n");
			goto err_remove_config_dt;
		}
	}

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret) {
		dev_err(&pdev->dev, "stmmac_dvr_probe failed\n");
		goto err_gmac_exit;
	}
	/*printk("############### %s[%d] end\n",__func__,__LINE__);*/

	return 0;

err_gmac_exit:
	if (plat_dat->exit)
		plat_dat->exit(pdev, plat_dat->bsp_priv);
err_remove_config_dt:
	if (pdev->dev.of_node)
		stmmac_remove_config_dt(pdev, plat_dat);

	return ret;
}

static const struct of_device_id ingenic_dwmac_match[] = {
	{ .compatible = "ingenic,t31-gmac" },
	{ .compatible = "ingenic,PRJ007-gmac" },
	{ .compatible = "ingenic,PRJ008-gmac" },
	{ }
};
MODULE_DEVICE_TABLE(of, ingenic_dwmac_match);

static struct platform_driver ingenic_dwmac_driver = {
	.probe  = ingenic_gmac_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		.name           = "ingenic-dwmac",
		.pm		= &stmmac_pltfr_pm_ops,
		.of_match_table = ingenic_dwmac_match,
	},
};
module_platform_driver(ingenic_dwmac_driver);

MODULE_AUTHOR("ywhan <keven.ywhan@ingenic.com>");
MODULE_DESCRIPTION("Ingenic DWMAC specific glue layer");
MODULE_LICENSE("GPL");
