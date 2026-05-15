#
# linux/drivers/mtd/devices/ingenic_sfc_v2/Makefile
#
ifdef CONFIG_ZERATUL
ifeq ($(CONFIG_INGENIC_SFC_V2),m)
	obj-m += jz_sfc.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NORFLASH) += ingenic_sfc_drv.o ingenic_sfc_common.o ingenic_sfc_nor.o ingenic_sfc_ops.o

	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += ingenic_sfc_drv.o ingenic_sfc_common.o ingenic_sfc_nand.o ingenic_sfc_ops.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/nand_common.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/ato_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/dosilicon_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/esmt_mid2c_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/foresee_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/gd_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/micron_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/mxic_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/winbond_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/xtx_mid0b_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/xtx_nand.o
	jz_sfc-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += nand_device/zetta_nand.o
else
	obj-$(CONFIG_MTD_INGENIC_SFC_V2_NORFLASH) += ingenic_sfc_common.o ingenic_sfc_drv.o ingenic_sfc_nor.o ingenic_sfc_ops.o
	obj-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += ingenic_sfc_common.o ingenic_sfc_drv.o ingenic_sfc_nand.o nand_device/
endif
else
	obj-$(CONFIG_MTD_INGENIC_SFC_V2_NORFLASH) += ingenic_sfc_common.o ingenic_sfc_drv.o ingenic_sfc_nor.o ingenic_sfc_ops.o
	obj-$(CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH) += ingenic_sfc_common.o ingenic_sfc_drv.o ingenic_sfc_nand.o nand_device/
endif

