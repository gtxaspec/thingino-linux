#ifndef __SDHCI_INGENIC_H__
#define __SDHCI_INGENIC_H__

#include <soc/mmc.h>
#include "ingenic_mmc_reg.h"
enum cd_types {
	SDHCI_INGENIC_CD_INTERNAL,	/* use mmc internal CD line */
	SDHCI_INGENIC_CD_GPIO,		/* use external gpio pin for CD line */
	SDHCI_INGENIC_CD_NONE,		/* no CD line, use polling to detect card */
	SDHCI_INGENIC_CD_PERMANENT,	/* no CD line, card permanently wired to host */
	SDHCI_INGENIC_CD_NON_INITIAL,   /* no CD line & no initialize */
};

#define CPM_MSC0_CLK_R     (0xB0000068)
#define CPM_MSC1_CLK_R     (0xB00000A4)
#define MSC_CDR            (0xff << 0)
#define MSC_CLK_EXCLK_EN   (0x1 << 21)
#define MSC_CLK_STOP       (0x1 << 27)
#define MSC_CLK_BUSY       (0x1 << 28)
#define MSC_CLK_CE         (0x1 << 29)
#define MSC_CLK_CS(x)      (x << 30)

#define LOW_ENABLE			0
#define HIGH_ENABLE			1
struct ingenic_mmc_pin {
	short				num;
	short 				enable_level;
};

struct card_gpio {
	struct ingenic_mmc_pin 		wp;
	struct ingenic_mmc_pin 		cd;
	struct ingenic_mmc_pin 		pwr;
	struct ingenic_mmc_pin 		rst;
};

/**
 * struct sdhci_ingenic_platdata() - Platform device data for ingenic SDHCI
 * @max_width: The maximum number of data bits supported.
 * @host_caps: Standard MMC host capabilities bit field.
 * @host_caps2: The second standard MMC host capabilities bit field.
 * @sdr_v18: External gpio pin switch voltage from 3.3V to 1.8V
 * @cd_type: Type of Card Detection method (see cd_types enum above)
 * @enable_cpm_rx_tunning: Manual adjustment cpm rx tunting
 * @enable_cpm_tx_tunning: Manual adjustment cpm tx tunting
 * @ext_cd_gpio_invert: invert values for external CD gpio line
 * @cfg_gpio: Configure the GPIO for a specific card bit-width
 *
 * Initialisation data specific to either the machine or the platform
 * for the device driver to use or call-back when configuring gpio or
 * card speed information.
 */
struct sdhci_ingenic_pdata {
	const char *gpio_group_name;
	unsigned int    host_caps;
	unsigned int    host_caps2;
	unsigned short	sdio_clk;
	int		sdr_v18;
	unsigned int	poc_v18;
	unsigned int    pm_caps;
	enum cd_types   cd_type;

	unsigned int	pio_mode;
	unsigned int	enable_autocmd12;

	struct card_gpio *gpio;
	int	enable_cpm_rx_tuning;
	int	enable_cpm_tx_tuning;
	int driver_strength;
	int gpio_pull_flag;
	bool        ext_cd_gpio_invert;

	void    (*cfg_gpio)(struct platform_device *dev, int width);
	int     (*private_init)(void);
};


#define INGENIC_MMC_CARD_PRESENT	0
#define INGENIC_MMC_CARD_NEED_INIT	1
#define INGENIC_MMC_USE_PIO		2
/**
 * struct sdhci_ingenic - INGENIC SDHCI instance
 * @host: The SDHCI host created
 * @pdev: The platform device we where created from.
 * @ioarea: The resource created when we claimed the IO area.
 * @pdata: The platform data for this controller.
 */
struct sdhci_ingenic {
	struct device			*dev;
	struct sdhci_host		*host;
	struct platform_device		*pdev;
	struct device_node		*node;
	struct sdhci_ingenic_pdata	*pdata;
	struct list_head		list;
	struct clk			*clk_cgu;
	struct clk			*clk_gate;
	struct clk			*clk_mux;
	struct clk			*clk_pll;
	int				cur_clk;
	int				ext_cd_irq;
	int				ext_cd_gpio;
	unsigned long			clk_rates;
	unsigned long			flags;
};

int ingenic_sdio_wlan_init(struct device *dev, int index);
int ingenic_sdio_wlan_deinit(struct device *dev);
int ingenic_mmc_clk_ctrl(int index, int on);
#endif	/* __SDHCI_INGENIC_H__ */
