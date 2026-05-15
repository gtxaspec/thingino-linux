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
	int reg;
	u32 features;

	phy_write(phydev, MII_BMCR, 0x3100);

	/* For now, I'll claim that the generic driver supports all possible port types */
	features = (SUPPORTED_TP | SUPPORTED_MII | SUPPORTED_AUI | SUPPORTED_FIBRE |
			SUPPORTED_BNC | SUPPORTED_Pause | SUPPORTED_Asym_Pause);

	/* Do we support autonegotiation? */
	reg = phy_read(phydev, MII_BMSR);

	if (reg < 0)
		return reg;

	if (reg & BMSR_ANEGCAPABLE)
		features |= SUPPORTED_Autoneg;

	if (reg & BMSR_100FULL)
		features |= SUPPORTED_100baseT_Full;
	if (reg & BMSR_100HALF)
		features |= SUPPORTED_100baseT_Half;
	if (reg & BMSR_10FULL)
		features |= SUPPORTED_10baseT_Full;
	if (reg & BMSR_10HALF)
		features |= SUPPORTED_10baseT_Half;

	if (reg & BMSR_ESTATEN) {
		reg = phy_read(phydev, MII_ESTATUS);

		if (reg < 0)
			return reg;

		if (reg & ESTATUS_1000_TFULL)
			features |= SUPPORTED_1000baseT_Full;
		if (reg & ESTATUS_1000_THALF)
			features |= SUPPORTED_1000baseT_Half;
	}
	phydev->supported = features;
	phydev->advertising = features;


	phy_write(phydev, 0x1e, 0x50);
	reg = phy_read(phydev, 0x1f);
	/* bit6, 1:50M 0:25M */
	reg &= ~0x40;
	// reg |= 0x40;
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
	.config_init = sz18201_config_init,
	.config_aneg = genphy_config_aneg,
	.read_status = genphy_read_status,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.driver		= { .owner = THIS_MODULE,},
}
};

static int __init motorcomm_init(void)
{
	return phy_drivers_register(motorcomm_driver, ARRAY_SIZE(motorcomm_driver));
}

static void __exit motorcomm_exit(void)
{
	phy_drivers_unregister(motorcomm_driver, ARRAY_SIZE(motorcomm_driver));
}

module_init(motorcomm_init);
module_exit(motorcomm_exit);

static struct mdio_device_id __maybe_unused motorcomm_tbl[] = {
	{ 0x00000128, 0x0fffffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, motorcomm_tbl);

MODULE_DESCRIPTION("Motorcomm SZ18201/RPC8201F/YT8512 PHY drivers");
MODULE_LICENSE("GPL");
