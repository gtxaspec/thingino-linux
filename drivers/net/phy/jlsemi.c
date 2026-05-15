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

static int jl1101_config_init(struct phy_device *phydev)
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

#if 1 // MDI
	phy_write(phydev, 31, 130); // page 130
	reg = phy_read(phydev, 22);
	reg = reg | (1 << 3);		// enable MDI Amplitude 1 Bit Adjust
	phy_write(phydev, 22, reg); // page 130
#endif
	phy_write(phydev, 31, 7); // page 7
	reg = phy_read(phydev, 19);
	reg &= ~(0x3 << 4);
	reg |= (0x1 << 4);
	phy_write(phydev, 19, reg); // LED config

	phy_write(phydev, 31, 0); // page 0

	return 0;
}

static struct phy_driver jlsemi_driver[] = {
{
	.phy_id      = 0x937c4023,
	.phy_id_mask = 0x0ffffff0,
	.name = "jl1101",
	.features = PHY_BASIC_FEATURES,
	.config_init = jl1101_config_init,
	.config_aneg = genphy_config_aneg,
	.read_status = genphy_read_status,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.driver		= { .owner = THIS_MODULE,},
}
};

static int __init jlsemi_init(void)
{
	return phy_drivers_register(jlsemi_driver, ARRAY_SIZE(jlsemi_driver));
}

static void __exit jlsemi_exit(void)
{
	phy_drivers_unregister(jlsemi_driver, ARRAY_SIZE(jlsemi_driver));
}

module_init(jlsemi_init);
module_exit(jlsemi_exit);

static struct mdio_device_id __maybe_unused jlsemi_tbl[] = {
	{ 0x937c4023, 0x0ffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, jlsemi_tbl);

MODULE_DESCRIPTION("JLSemi JL1101/JL1111 PHY drivers");
MODULE_LICENSE("GPL");
