#ifndef __SDJCI_JZ_H
#define __SDHCI_JZ_H
typedef unsigned int mmc_pm_flag_t;

enum cd_types {
	JZ_SDHCI_CD_INTERNAL,	/* use mmc internal CD line */
	JZ_SDHCI_CD_GPIO,		/* use external gpio pin for CD line */
	JZ_SDHCI_CD_NONE,		/* no CD line, use polling to detect card */
	JZ_SDHCI_CD_PERMANENT,	/* no CD line, card permanently wired to host */
	JZ_SDHCI_CD_NON_INITIAL,	/* no initialize */
};

enum tx_sels {
	TUNING_TX_SEL_90,
	TUNING_TX_SEL_135,
	TUNING_TX_SEL_180,
	TUNING_TX_SEL_270,
};

enum rx_sels {
	TUNING_RX_SEL_0,
	TUNING_RX_SEL_45,
	TUNING_RX_SEL_90,
	TUNING_RX_SEL_135,
	TUNING_RX_SEL_180,
	TUNING_RX_SEL_225,
	TUNING_RX_SEL_270,
	TUNING_RX_SEL_315,
};

/**
 * struct jz_sdhci_platdata() - Platform device data for ingenic SDHCI
 * @max_width: The maximum number of data bits supported.
 * @host_caps: Standard MMC host capabilities bit field.
 * @host_caps2: The second standard MMC host capabilities bit field.
 * @cd_type: Type of Card Detection method (see cd_types enum above)
 * @ext_cd_gpio: gpio pin used for external CD line, valid only if
 *       cd_type == JZ_SDHCI_CD_GPIO
 * @ext_cd_gpio_invert: invert values for external CD gpio line
 * @cfg_gpio: Configure the GPIO for a specific card bit-width
 *
 * Initialisation data specific to either the machine or the platform
 * for the device driver to use or call-back when configuring gpio or
 * card speed information.
 */
struct jz_sdhci_platdata {
	unsigned int    max_width;
	unsigned int    host_caps;
	unsigned int    host_caps2;
	unsigned int    pm_caps;
	mmc_pm_flag_t   pm_flags;
	enum cd_types   cd_type;
	enum tx_sels	tx_sel;
	enum rx_sels	rx_sel;

	unsigned int	pio_mode;
	unsigned int	enable_autocmd12;

	int		ext_cd_gpio;
	bool	ext_cd_gpio_invert;
	int		ext_pwr_gpio;
	bool	ext_pwr_gpio_invert;
	int		voltage_switch_gpio;
	bool	voltage_switch_gpio_invert;

	void    (*cfg_gpio)(struct platform_device *dev, int width);
	int     (*private_init)(void);
};



#define INGENIC_MMC_CARD_PRESENT	0
#define INGENIC_MMC_CARD_NEED_INIT	1
#define INGENIC_MMC_USE_PIO		2
/**
 * struct sdhci_jz - JZ SDHCI instance
 * @host: The SDHCI host created
 * @pdev: The platform device we where created from.
 * @ioarea: The resource created when we claimed the IO area.
 * @pdata: The platform data for this controller.
 */
struct sdhci_jz {
	struct device		*dev;
	int id;
	struct sdhci_host   *host;
	struct platform_device  *pdev;
	struct resource     *ioarea;
	struct jz_sdhci_platdata *pdata;

	struct list_head	list;

	struct clk		*clk;
	struct clk		*clk_gate;
	struct clk		*pll;
	unsigned long max_freq[2];
	unsigned long			flags;

	int         ext_cd_irq;
	int         ext_cd_gpio;
};

int jzmmc_manual_detect(int index, int on);
#endif //__SDJCI_JZ_H
