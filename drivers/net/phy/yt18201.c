#include <linux/module.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/mdio.h>
#include <linux/marvell_phy.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/sfp.h>
#include <linux/netdevice.h>

static int sz18201_config_init(struct phy_device *phydev)
{
	int reg;

	__ETHTOOL_DECLARE_LINK_MODE_MASK(supported) = { 0, };

	printk("######### phy_id = 0x%x\n",phydev->phy_id);

	/* For now, I'll claim that the generic driver supports all possible port types */
	linkmode_set_bit(ETHTOOL_LINK_MODE_TP_BIT, supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_MII_BIT, supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_AUI_BIT, supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_FIBRE_BIT, supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_BNC_BIT, supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_Pause_BIT, supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_Asym_Pause_BIT, supported);

	phy_write(phydev, MII_BMCR, 0x3100);

	/* Do we support autonegotiation? */
	reg = phy_read(phydev, MII_BMSR);
	if (reg < 0)
		return reg;

	if (reg & BMSR_ANEGCAPABLE)
		linkmode_set_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, supported);

	if (reg & BMSR_100FULL)
		linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, supported);
	if (reg & BMSR_100HALF)
		linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT, supported);
	if (reg & BMSR_10FULL)
		linkmode_set_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT, supported);
	if (reg & BMSR_10HALF)
		linkmode_set_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT, supported);
	if (reg & BMSR_ESTATEN) {
		reg = phy_read(phydev, MII_ESTATUS);
		if (reg < 0)
			return reg;

		if (reg & ESTATUS_1000_TFULL)
			linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, supported);
		if (reg & ESTATUS_1000_THALF)
			linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT, supported);
	}

	linkmode_copy(phydev->supported, supported);
	linkmode_copy(phydev->advertising, supported);

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
		.phy_id      	= 0x00000128,
		.phy_id_mask 	= 0x0fffffff,
		.name 		= "sz18201",
		.features 	= PHY_BASIC_FEATURES,
		.config_init 	= sz18201_config_init,
		.config_aneg 	= genphy_config_aneg,
		.read_status 	= genphy_read_status,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
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
