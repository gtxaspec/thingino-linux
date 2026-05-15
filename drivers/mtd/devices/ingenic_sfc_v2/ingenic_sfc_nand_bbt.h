/*
 * SFC controller for SPI NAND - Bad Block Table Management Header
 *
 * Copyright (c) 2015 Ingenic
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __INGENIC_SFC_NAND_BBT_H__
#define __INGENIC_SFC_NAND_BBT_H__

#include <linux/mtd/mtd.h>
#include "sfc_flash.h"

/* ========================================================================
 * SPI NAND Bad Block Table (BBT) Management
 * Independent implementation for SPI NAND, not relying on raw NAND drivers
 * ======================================================================== */

#define SPI_NAND_BBT_MAGIC "SfcB" /* BBT magic pattern */
#define SPI_NAND_BBT_VERSION 0x20250912 /* BBT version */
#define SPI_NAND_BBT_BLOCKS 4 /* Reserve last 4 blocks for BBT */
#define SPI_NAND_BBT_PATTERN_LEN 4 /* Length of BBT pattern */

/* SPI NAND specific configuration */
struct sfc_nand_config {
	u32 erasesize; /* Erase block size */
	u32 writesize; /* Page size */
	u32 oobsize; /* OOB size */
	u32 chipsize; /* Total chip size */
	u8 badblockpos; /* Bad block marker position in OOB */
	u8 badblockbits; /* Number of bits for bad block marker */
	bool scan_lastpage; /* Scan last page for bad block */
	bool scan_2ndpage; /* Scan 2nd page for bad block */
};

/* BBT info structure */
struct sfc_nand_bbt_info {
	u8 *bbt; /* Bad block table in memory */
	int bbt_size; /* Size of BBT in bytes */
	int bbt_blocks; /* Number of blocks for BBT */
	int total_blocks; /* Total number of blocks */
	struct sfc_nand_config config; /* SPI NAND configuration */
};

/* Bad block status in BBT (2 bits per block) */
#define SPI_NAND_BBT_BLOCK_GOOD 0x00
#define SPI_NAND_BBT_BLOCK_WORN 0x01
#define SPI_NAND_BBT_BLOCK_FACTORY_BAD 0x02
#define SPI_NAND_BBT_BLOCK_RESERVED 0x03

/* Function pointer types for OOB operations */
typedef int32_t (*ingenic_sfcnand_read_oob_fn)(struct mtd_info *mtd,
					       loff_t from,
					       struct mtd_oob_ops *ops);
typedef int32_t (*ingenic_sfcnand_write_oob_fn)(struct mtd_info *mtd, loff_t to,
						struct mtd_oob_ops *ops);

/* BBT management functions */
struct sfc_nand_bbt_info *sfc_nand_get_bbt_info(struct sfc_flash *flash);
void sfc_nand_set_bbt_info(struct sfc_flash *flash,
			   struct sfc_nand_bbt_info *bbt_info);
int sfc_nand_bbt_get_entry(struct sfc_flash *flash, int block);
void sfc_nand_bbt_set_entry(struct sfc_flash *flash, int block, int value);

/* BBT scanning functions */
int sfc_nand_scan_block_bad(struct mtd_info *mtd, int block,
			    ingenic_sfcnand_read_oob_fn read_oob);
int sfc_nand_memory_bbt_scan(struct mtd_info *mtd,
			     ingenic_sfcnand_read_oob_fn read_oob);
int ingenic_sfcnand_scan_bbt(struct mtd_info *mtd,
			     ingenic_sfcnand_read_oob_fn read_oob);

/* Block management functions */
int ingenic_sfcnand_block_bad(struct mtd_info *mtd, loff_t ofs,
			      ingenic_sfcnand_read_oob_fn read_oob);
int ingenic_sfcnand_block_markbad(struct mtd_info *mtd, loff_t ofs,
				  ingenic_sfcnand_write_oob_fn write_oob);

/* BBT cleanup function */
void ingenic_sfcnand_bbt_cleanup(struct mtd_info *mtd);

#endif /* __INGENIC_SFC_NAND_BBT_H__ */