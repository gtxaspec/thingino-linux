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
	int ret, reg;
	phy_write(phydev, MII_BMCR, 0x3100);
	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

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
	.soft_reset	= genphy_no_soft_reset,
	.config_init = jl1101_config_init,
	// .config_init = genphy_config_init,
	.config_aneg = genphy_config_aneg,
	.aneg_done	 = genphy_aneg_done,
	.read_status = genphy_read_status,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.driver		= { .owner = THIS_MODULE,},
}
};

module_phy_driver(jlsemi_driver);

static struct mdio_device_id __maybe_unused jlsemi_tbl[] = {
	{ 0x937c4023, 0x0ffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, jlsemi_tbl);

MODULE_DESCRIPTION("JLSemi JL1101/JL1111 PHY drivers");
MODULE_LICENSE("GPL");
