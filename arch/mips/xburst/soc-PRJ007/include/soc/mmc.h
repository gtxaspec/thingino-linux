#ifndef __MMC_H
#define __MMC_H
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

extern int jzmmc_manual_detect(int index, int on);
#endif
