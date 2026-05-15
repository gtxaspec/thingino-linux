#
# linux/drivers/mtd/devices/Makefile
#

obj-$(CONFIG_MTD_DOCG3)		+= docg3.o
obj-$(CONFIG_MTD_SLRAM)		+= slram.o
obj-$(CONFIG_MTD_PHRAM)		+= phram.o
obj-$(CONFIG_MTD_PMC551)	+= pmc551.o
obj-$(CONFIG_MTD_MS02NV)	+= ms02-nv.o
obj-$(CONFIG_MTD_MTDRAM)	+= mtdram.o
obj-$(CONFIG_MTD_LART)		+= lart.o
obj-$(CONFIG_MTD_BLOCK2MTD)	+= block2mtd.o
obj-$(CONFIG_MTD_DATAFLASH)	+= mtd_dataflash.o
obj-$(CONFIG_MTD_M25P80)	+= m25p80.o
obj-$(CONFIG_MTD_NAND_OMAP_BCH)	+= elm.o
obj-$(CONFIG_MTD_SPEAR_SMI)	+= spear_smi.o
obj-$(CONFIG_MTD_SST25L)	+= sst25l.o
obj-$(CONFIG_MTD_BCM47XXSFLASH)	+= bcm47xxsflash.o
obj-$(CONFIG_MTD_JZ_SPI_NORFLASH)	+= jz_spi_norflash.o

ifeq ($(CONFIG_MTD_JZ_SFC),m)
ifeq ($(CONFIG_MTD_JZ_SFC_NOR),m)
	obj-m += jz_sfc.o
	jz_sfc-y += jz_sfc_nor.o
else
	obj-m += jz_sfc.o
	jz_sfc-y += jz_sfc_common.o jz_sfc_nand.o
	jz_sfc-y += nand_device/nand_common.o
	jz_sfc-y += nand_device/ato_nand.o
	jz_sfc-y += nand_device/fm25s_nand.o
	jz_sfc-y += nand_device/gd_nand.o
	jz_sfc-y += nand_device/mxic_nand.o
	jz_sfc-y += nand_device/winbond_nand.o
	jz_sfc-y += nand_device/xtx_mid0b_nand.o
	jz_sfc-y += nand_device/xtx_nand.o
	jz_sfc-y += nand_device/alliance_nand.o
	jz_sfc-y += nand_device/dosilicon_nand.o
	jz_sfc-y += nand_device/foresee_nand.o
	jz_sfc-y += nand_device/sky_nand.o
	jz_sfc-y += nand_device/wodposit_nand.o
	jz_sfc-y += nand_device/xtx_mid2c_nand.o
endif
else
	obj-$(CONFIG_MTD_JZ_SFC_NOR)	+= jz_sfc_nor.o
	obj-$(CONFIG_MTD_JZ_SFC_NAND)       += jz_sfc_common.o jz_sfc_nand.o nand_device/
endif

CFLAGS_docg3.o			+= -I$(src)
