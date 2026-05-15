#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>

static int sz18201_config_init(struct phy_device *phydev)
{
	int ret, reg;
	phy_write(phydev, MII_BMCR, 0x3100);
	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

	phy_write(phydev, 0x1e, 0x50);
	reg = phy_read(phydev, 0x1f);

	reg &= ~0x40; // bit6, 0:25M
 // reg |= 0x40;  // bit6, 1:50M
	phy_write(phydev, 0x1f, reg);

	phy_write(phydev, 0x1e, 0x4000);
	reg = phy_read(phydev, 0x1f);
	// reg = 0x13;
	reg |= 0x10;
	phy_write(phydev, 0x1f, reg);
#if 1 // Drive
	phy_write(phydev, 0x1e, 0x2012);
	phy_write(phydev, 0x1f, 0x6f0);
	phy_write(phydev, 0x1e, 0x2056);
	phy_write(phydev, 0x1f, 0xc000);
	phy_write(phydev, 0x1e, 0x4001);
	phy_write(phydev, 0x1f, 0x10);
#endif
#if 1 // restart Auto Negotiation
	phy_write(phydev, 0x4, 0x1e1);
	phy_write(phydev, 0x0, 0x3100);
	phy_write(phydev, 0x0, 0x3300);
#endif

	/* config leds */
	phy_write(phydev, 0x1e, 0x40c0); // LED 0
	reg = phy_read(phydev, 0x1f);
	reg &= 0xffe0;
	reg |= 0x1000;
	phy_write(phydev, 0x1f, reg);

	phy_write(phydev, 0x1e, 0x40c3); // LED 2
	reg = phy_read(phydev, 0x1f);
	reg &= 0xfcff;
	reg |= 0x10;
	phy_write(phydev, 0x1f, reg);

	return 0;
}

static struct phy_driver motorcomm_driver[] = {
{
	.phy_id      = 0x00000128,
	.phy_id_mask = 0x0fffffff,
	.name = "sz18201",
	.features = PHY_BASIC_FEATURES,
	.soft_reset	= genphy_no_soft_reset,
	.config_init = sz18201_config_init,
	// .config_init = genphy_config_init,
	.config_aneg = genphy_config_aneg,
	.aneg_done	 = genphy_aneg_done,
	.read_status = genphy_read_status,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.driver		= { .owner = THIS_MODULE,},
}
};

module_phy_driver(motorcomm_driver);

static struct mdio_device_id __maybe_unused motorcomm_tbl[] = {
	{ 0x00000128, 0x0fffffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, motorcomm_tbl);

MODULE_DESCRIPTION("Motorcomm SZ18201/RPC8201F/YT8512 PHY drivers");
MODULE_LICENSE("GPL");
