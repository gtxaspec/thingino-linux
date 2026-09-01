/*
 * drivers/net/phy/na51055.c
 *
 * Driver for Novatek PHYs
 *
 * Copyright (c) 2019 Novatek Microelectronics Corp.
 *
 */
#include <linux/phy.h>
#include <linux/module.h>
//#include <linux/gpio.h>
//#include <nvt-gpio.h>
//#include <nvt-gpio.h>
#include <plat/top.h>
#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#endif

#include "na51055_reg.h"
static int bplug = 0;
static int g_enable_e_loopback = 0;
static int g_enable_i_loopback = 0;
static int g_nvt_phy_sysfs_created = 0;

static enum phy_state na510xx_phy_state = PHY_DOWN;

static void __iomem *NVT_PHY_BASE = 0;

static ssize_t loopback_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "external loopback mode is %s\n"
			"internal loopback mode is %s\n",
			(g_enable_e_loopback) ? "enable" : "disable",
			(g_enable_i_loopback) ? "enable" : "disable");
}

static ssize_t loopback_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t len)
{
	struct phy_device *phydev = to_phy_device(dev);
	int enable_lm = 0, loopback_mode = 0, ret, val;

	ret = sscanf(buf, "%d %d", &loopback_mode, &enable_lm);
	printk("ret = %d, loopback = %d, enable_lm = %d\n", ret, loopback_mode, enable_lm);

	if ((ret != 2) || ((loopback_mode >> 1) != 0) || ((enable_lm >> 1) != 0)) {
		printk("invalid loopback_mode : %d (internal = 0, external = 1)"
			"or enable value : %d (disable = 0, enable = 1)\n",
			loopback_mode, enable_lm);
		return -EINVAL;
	}

	if (loopback_mode == 1) {
		// external loppback mode
		if (enable_lm == 1) {
			phy_sw_reset_enable();
			iowrite32(0x3c, (void *)(0xFD2B3800 + 0x04));
			iowrite32(0x80, (void *)(0xFD2B3800 + 0x74));
			mdelay(10);
			phy_sw_reset_disable();
			g_enable_e_loopback = 1;
		} else {
			phy_sw_reset_enable();
			iowrite32(0x00, (void *)(0xFD2B3800 + 0x04));
			mdelay(10);
			phy_sw_reset_disable();
			g_enable_e_loopback = 0;
		}
	} else {
		//internal loopback mode
		if (enable_lm == 1) {
			val = phy_read(phydev, MII_BMCR);
			if (val < 0)
				return -EIO;

			val |= BMCR_LOOPBACK;

			ret = phy_write(phydev, MII_BMCR, val);
			if (ret < 0)
				return -EIO;
			g_enable_i_loopback = 1;
		} else {
			val = phy_read(phydev, MII_BMCR);
			if (val < 0)
				return -EIO;

			val &= ~BMCR_LOOPBACK;

			ret = phy_write(phydev, MII_BMCR, val);
			if (ret < 0)
				return -EIO;
			g_enable_i_loopback = 0;
		}
	}
	return len;
}

static DEVICE_ATTR_RW(loopback);

static int nvt_phy_sysfs_init(struct device *dev)
{
	return sysfs_create_file(&dev->kobj, &dev_attr_loopback.attr);
}

static void nvt_phy_sysfs_remove(struct device *dev)
{
	sysfs_remove_file(&dev->kobj, &dev_attr_loopback.attr);
}

/**
 * phy_poll_reset - Safely wait until a PHY reset has properly completed
 * @phydev: The PHY device to poll
 *
 * Description: According to IEEE 802.3, Section 2, Subsection 22.2.4.1.1, as
 *   published in 2008, a PHY reset may take up to 0.5 seconds.  The MII BMCR
 *   register must be polled until the BMCR_RESET bit clears.
 *
 *   Furthermore, any attempts to write to PHY registers may have no effect
 *   or even generate MDIO bus errors until this is complete.
 *
 *   Some PHYs (such as the Marvell 88E1111) don't entirely conform to the
 *   standard and do not fully reset after the BMCR_RESET bit is set, and may
 *   even *REQUIRE* a soft-reset to properly restart autonegotiation.  In an
 *   effort to support such broken PHYs, this function is separate from the
 *   standard phy_init_hw() which will zero all the other bits in the BMCR
 *   and reapply all driver-specific and board-specific fixups.
 */
static int phy_poll_reset(struct phy_device *phydev)
{
	/* Poll until the reset bit clears (50ms per retry == 0.6 sec) */
	unsigned int retries = 12;
	int ret;

	do {
		msleep(50);
		ret = phy_read(phydev, MII_BMCR);
		if (ret < 0)
			return ret;
	} while (ret & BMCR_RESET && --retries);
	if (ret & BMCR_RESET)
		return -ETIMEDOUT;

	/* Some chips (smsc911x) may still need up to another 1ms after the
	 * BMCR_RESET bit is cleared before they are usable.
	 */
	msleep(1);
	return 0;
}

/**
 * genphy_soft_reset - software reset the PHY via BMCR_RESET bit
 * @phydev: target phy_device struct
 *
 * Description: Perform a software PHY reset using the standard
 * BMCR_RESET bit and poll for the reset bit to be cleared.
 *
 * Returns: 0 on success, < 0 on failure
 */
static int nvt_soft_reset(struct phy_device *phydev)
{
	int val;
	int ret;

   	val = phy_read(phydev, MII_BMCR);
	if (val < 0)
		return val;

    val |= BMCR_RESET;

	ret = phy_write(phydev, MII_BMCR, val);
	if (ret < 0)
		return ret;

	return phy_poll_reset(phydev);
}

static int nvt_config_aneg(struct phy_device *phydev)
{
	int ret;

	if (phydev->autoneg) {
		iowrite32(0xE0, (void *)(0xFD2B3800 + 0x380));
	} else {
		iowrite32(0xA0, (void *)(0xFD2B3800 + 0x380));
	}
	ret = genphy_config_aneg(phydev);
	if (ret) return ret;

	if (phydev->autoneg) {
		ret = genphy_restart_aneg(phydev);
	}

	return ret;
}

static void eth_phy_poweron(void)
{
	unsigned long reg;

	reg = ioread32((void*)(0xFD2B3800 + 0xF8));
	iowrite32(reg | (1<<7), (void *)(0xFD2B3800 + 0xF8));
	udelay(20);
	reg = ioread32((void*)(0xFD2B3800 + 0xC8));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0xC8));
	udelay(200);
	reg = ioread32((void*)(0xFD2B3800 + 0xC8));
	iowrite32(reg & (~(1<<1)), (void *)(0xFD2B3800 + 0xC8));
	udelay(250);
	reg = ioread32((void*)(0xFD2B3800 + 0x2E8));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0x2E8));
	reg = ioread32((void*)(0xFD2B3800 + 0xCC));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0xCC));
	reg = ioread32((void*)(0xFD2B3800 + 0xDC));
	iowrite32(reg | (1<<0), (void *)(0xFD2B3800 + 0xDC));
	reg = ioread32((void*)(0xFD2B3800 + 0x9C));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0x9C));
}

static int nvt_resume(struct phy_device *phydev)
{
	int ret = 0;
	int inv_led = 0;
	PIN_GROUP_CONFIG pinmux_config[1] = {0};
	struct device *dev = &phydev->mdio.dev;

#ifdef CONFIG_OF
	struct device_node* of_node = of_find_node_by_path("/phy@f02b3800");
#endif

	na510xx_phy_state = phydev->state;

#ifdef CONFIG_OF
	if (of_node) {
		printk("%s: find node\r\n", __func__);
		if(!of_property_read_u32(of_node, "led-inv", &inv_led)) {
			printk("%s: led-inv found\r\n", __func__);
		}
	}
#endif
	printk("%s: led-inv = %d\r\n", __func__, inv_led);

	pinmux_config[0].pin_function = PIN_FUNC_ETH;
	ret = nvt_pinmux_capture(pinmux_config, 1);
	if (ret) {
		printk("%s: pinmux not found\r\n", __func__);
	}
	printk("%s: pinmux 0x%x\r\n", __func__, (int)pinmux_config[0].config);

	eth_phy_poweron();

	if (nvt_get_chip_id() == CHIP_NA51055) {
		/* NA51055 */
		set_best_setting_t28();
	} else {
		/* NA51084 / NA51089 */
		set_best_setting_t22();
	}
	phy_sw_reset_enable();
	set_break_link_timer();
#if defined(CONFIG_NVT_IVOT_PLAT_NA51055)
	if ((pinmux_config[0].config&PIN_ETH_CFG_LED_1ST_ONLY) ||
			(pinmux_config[0].config&PIN_ETH_CFG_LED_2ND_ONLY)) {
		set_one_led_link_act();
	} else {
		set_two_leds_link_act();
	}
#endif
#if defined(CONFIG_NVT_IVOT_PLAT_NA51089)
        if ((pinmux_config[0].config&PIN_ETH_CFG_LINKLED_ONLY) ||
                        (pinmux_config[0].config&PIN_ETH_CFG_ACTLED_ONLY)) {
                set_one_led_link_act();
        } else {
                set_two_leds_link_act();
        }
#endif

	if (inv_led) {
		set_led_inv();
	}
	mdelay(10);
	phy_sw_reset_disable();
	genphy_config_aneg(phydev);

	if (!g_nvt_phy_sysfs_created) {
		ret = nvt_phy_sysfs_init(dev);
		if (ret) {
			printk("%s: sysfs init fail ret = %d\r\n", __func__, ret);
		}
		g_nvt_phy_sysfs_created = 1;
	}

	return 0;
}

static int nvt_suspend(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	bplug = 0;

	if (g_nvt_phy_sysfs_created) {
		nvt_phy_sysfs_remove(dev);
		g_nvt_phy_sysfs_created = 0;
	}
	printk("%s, suspend\n", __func__);
	return 0;
//	return genphy_suspend(phydev);
}

static void nvt_link_change_notify(struct phy_device *phydev)
{
	int status, phy_link;
	int ret;
#ifdef NVT_PHY_AN_FAIL_10M_TO_100M
	static unsigned long crc_count = 0, crc_count_old = 0;
	static unsigned long reg_old, tx_reg_old;
	unsigned long reg = 0, tx_reg = 0;
#endif

	status = phy_read(phydev, MII_BMSR);
	if (status < 0)
		return;

	if ((status & BMSR_LSTATUS) == 0)
		phy_link = 0;
	else
		phy_link = 1;

#ifdef NVT_PHY_AN_FAIL_10M_TO_100M
	if (phy_link) {
		tx_reg = ioread32((void*)(0xFD2B3800 + 0x204));
		reg = ioread32((void*)(0xFD2B3800 + 0x210));
		if (reg != reg_old)
			crc_count += reg;

		reg_old = reg;
		if ((crc_count - crc_count_old > 100) && (tx_reg == tx_reg_old)) {
			ret = nvt_soft_reset(phydev);
			if (ret < 0)
				printk("reset fail\r\n");
		}
		crc_count_old = crc_count;
		tx_reg_old = tx_reg;
	}
#endif

	if (((na510xx_phy_state!=PHY_HALTED) && (phydev->state == PHY_HALTED)) ||
		((phydev->state == PHY_CHANGELINK) && (!phy_link))) {
		printk("remove\r\n");
		bplug = 0;
		eq_reset_enable();
		msleep(50);
		eq_reset_disable();
	} else if ((bplug==0) && (phydev->state == PHY_RUNNING)) {
		printk("plug\r\n");
		ret = phy_write(phydev, 0x1F, 0xC00);
		if (ret < 0) {
			printk("phy write fail, ret = %x\r\n", ret);
			return;
		}

		bplug = 1;
	}
	na510xx_phy_state = phydev->state;
}


static struct phy_driver novatek_drv[] = {
	{
		.phy_id             = 0x00000001,
		.name               = "Novatek Fast Ethernet Phy",
		.phy_id_mask        = 0x001fffff,
		.features           = PHY_BASIC_FEATURES,
		.soft_reset         = nvt_soft_reset,
		.config_init        = genphy_config_init,
		.config_aneg        = nvt_config_aneg,
		.read_status        = genphy_read_status,
		.suspend            = nvt_suspend,
		.resume	            = nvt_resume,
		.link_change_notify	= nvt_link_change_notify,
	},
};

module_phy_driver(novatek_drv);

static struct mdio_device_id __maybe_unused novatek_tbl[] = {
	{ 0x00000001, 0x001fffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, novatek_tbl);

MODULE_DESCRIPTION("Novatek Ethernet PHY Driver");
MODULE_VERSION("1.00.007");
MODULE_LICENSE("GPL");
