/*
 * SFC controller for SPI NAND - Bad Block Table Management
 *
 * Copyright (c) 2015 Ingenic
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include "sfc_flash.h"
#include "ingenic_sfc_nand_bbt.h"

#define to_ingenic_sfc_nand(mtd_info)                                          \
	container_of(mtd_info, struct sfc_flash, mtd)

/* Global BBT info storage - one per SPI NAND device */
static struct sfc_nand_bbt_info *g_sfc_nand_bbt_info[2] = { NULL, NULL };

/* Get BBT info from sfc_flash structure */
struct sfc_nand_bbt_info *sfc_nand_get_bbt_info(struct sfc_flash *flash)
{
	/* Use flash->sfc->sfc_index as identifier */
	int index = flash->sfc->sfc_index & 0x1;
	return g_sfc_nand_bbt_info[index];
}

/* Set BBT info for sfc_flash structure */
void sfc_nand_set_bbt_info(struct sfc_flash *flash,
			   struct sfc_nand_bbt_info *bbt_info)
{
	/* Use flash->sfc->sfc_index as identifier */
	int index = flash->sfc->sfc_index & 0x1;
	g_sfc_nand_bbt_info[index] = bbt_info;
}

/**
 * sfc_nand_bbt_get_entry - Get BBT entry for a specific block
 * @flash: SPI NAND flash structure
 * @block: Block number
 */
int sfc_nand_bbt_get_entry(struct sfc_flash *flash, int block)
{
	struct sfc_nand_bbt_info *bbt_info = sfc_nand_get_bbt_info(flash);
	int byte = block >> 2;
	int shift = (block & 0x03) << 1;

	if (!bbt_info || !bbt_info->bbt)
		return SPI_NAND_BBT_BLOCK_GOOD;

	return (bbt_info->bbt[byte] >> shift) & 0x03;
}

/**
 * sfc_nand_bbt_set_entry - Set BBT entry for a specific block
 * @flash: SPI NAND flash structure
 * @block: Block number
 * @value: BBT entry value
 */
void sfc_nand_bbt_set_entry(struct sfc_flash *flash, int block, int value)
{
	struct sfc_nand_bbt_info *bbt_info = sfc_nand_get_bbt_info(flash);
	int byte = block >> 2;
	int shift = (block & 0x03) << 1;

	if (!bbt_info || !bbt_info->bbt)
		return;

	bbt_info->bbt[byte] &= ~(0x03 << shift);
	bbt_info->bbt[byte] |= (value & 0x03) << shift;
}

static int badblk_check(int len, unsigned char *buf)
{
	int j;
	unsigned char *check_buf = buf;

	for (j = 0; j < len; j++) {
		if (check_buf[j] != 0xff) {
			return 1;
		}
	}
	return 0;
}

/**
 * sfc_nand_scan_block_bad - Scan a block for bad block marker
 * @mtd: MTD device structure
 * @block: Block number to scan
 * @read_oob: Function pointer to read OOB data
 */
int sfc_nand_scan_block_bad(struct mtd_info *mtd, int block,
			    ingenic_sfcnand_read_oob_fn read_oob)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	struct sfc_nand_bbt_info *bbt_info = sfc_nand_get_bbt_info(flash);
	struct mtd_oob_ops ops;
	u8 check_buf[2] = { 0xFF, 0xFF };
	loff_t ofs = (loff_t)block * (loff_t)mtd->erasesize;
	int check_len = 1; /* Default to 1 byte for SPI NAND */
	int ret;

	if (!bbt_info)
		return -EINVAL;

	memset(&ops, 0, sizeof(ops));
	ops.oobbuf = check_buf;
	ops.ooblen = check_len;
	ops.ooboffs = 0;
	ops.mode = MTD_OPS_RAW;

	/* Check first page */
	ret = read_oob(mtd, ofs, &ops);
	if (ret < 0)
		return ret;
	if (check_buf[0] != 0xFF)
		printk("sfc_nand_scan_block_bad: block = %d, ret = %d, check_buf = %02x %02x\n",
		       block, ret, check_buf[0], check_buf[1]);

	if (badblk_check(check_len, check_buf))
		return 1;

	/* Check last page if required */
	if (bbt_info->config.scan_lastpage) {
		ofs += mtd->erasesize - mtd->writesize;
		ret = read_oob(mtd, ofs, &ops);
		if (ret < 0)
			return ret;

		if (badblk_check(check_len, check_buf))
			return 1;
	}

	return 0;
}

/**
 * sfc_nand_memory_bbt_scan - Scan flash and build BBT in memory
 * @mtd: MTD device structure
 * @read_oob: Function pointer to read OOB data
 */
int sfc_nand_memory_bbt_scan(struct mtd_info *mtd,
			     ingenic_sfcnand_read_oob_fn read_oob)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	struct sfc_nand_bbt_info *bbt_info;
	int numblocks;
	int block;
	int ret;

	dev_info(flash->dev, "Scanning device for bad blocks\n");

	/* Allocate BBT info structure */
	bbt_info = kzalloc(sizeof(*bbt_info), GFP_KERNEL);
	if (!bbt_info)
		return -ENOMEM;

	/* Initialize SPI NAND configuration */
	bbt_info->config.erasesize = mtd->erasesize;
	bbt_info->config.writesize = mtd->writesize;
	bbt_info->config.oobsize = mtd->oobsize;
	bbt_info->config.chipsize = mtd->size;
	bbt_info->config.badblockpos = 0; /* First byte of OOB */
	bbt_info->config.badblockbits = 8;
	bbt_info->config.scan_lastpage = false; /* Can be configured */
	bbt_info->config.scan_2ndpage = false; /* Can be configured */

	/* Calculate number of blocks */
	numblocks = (int)div_u64(mtd->size, mtd->erasesize);
	bbt_info->total_blocks = numblocks;

	/* Calculate BBT size (2 bits per block) */
	bbt_info->bbt_size = (numblocks + 3) / 4;
	bbt_info->bbt = kzalloc(bbt_info->bbt_size, GFP_KERNEL);
	if (!bbt_info->bbt) {
		kfree(bbt_info);
		return -ENOMEM;
	}

	/* Store BBT info in global storage */
	sfc_nand_set_bbt_info(flash, bbt_info);

	/* Scan all blocks */
	for (block = 0; block < numblocks; block++) {
		ret = sfc_nand_scan_block_bad(mtd, block, read_oob);
		if (ret < 0) {
			dev_err(flash->dev, "Failed to scan block %d\n", block);
			/* Mark as bad on error */
			sfc_nand_bbt_set_entry(flash, block,
					       SPI_NAND_BBT_BLOCK_FACTORY_BAD);
		} else if (ret > 0) {
			dev_info(flash->dev, "Bad block found at 0x%08llx\n",
				 (loff_t)block * (loff_t)mtd->erasesize);
			sfc_nand_bbt_set_entry(flash, block,
					       SPI_NAND_BBT_BLOCK_FACTORY_BAD);
			mtd->ecc_stats.badblocks++;
		}
	}

	dev_info(flash->dev, "Bad block scan completed, found %d bad blocks\n",
		 mtd->ecc_stats.badblocks);

	return 0;
}

/**
 * ingenic_sfcnand_scan_bbt - Main BBT scanning function
 * @mtd: MTD device structure
 * @read_oob: Function pointer to read OOB data
 */
int ingenic_sfcnand_scan_bbt(struct mtd_info *mtd,
			     ingenic_sfcnand_read_oob_fn read_oob)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	int ret;

	/* For now, we only support memory-based BBT */
	/* TODO: Add flash-based BBT support if needed */
	ret = sfc_nand_memory_bbt_scan(mtd, read_oob);
	if (ret) {
		dev_err(flash->dev, "Failed to scan BBT: %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * ingenic_sfcnand_block_bad - Check if a block is bad
 * @mtd: MTD device structure
 * @ofs: Offset to check
 * @read_oob: Function pointer to read OOB data
 */
int ingenic_sfcnand_block_bad(struct mtd_info *mtd, loff_t ofs,
			      ingenic_sfcnand_read_oob_fn read_oob)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	struct sfc_nand_bbt_info *bbt_info = sfc_nand_get_bbt_info(flash);
	int block = (int)div_u64(ofs, mtd->erasesize);
	int ret;

	/* If we have a BBT, use it */
	if (bbt_info && bbt_info->bbt) {
		ret = sfc_nand_bbt_get_entry(flash, block);
		return (ret != SPI_NAND_BBT_BLOCK_GOOD) ? 1 : 0;
	}

	/* Otherwise, scan the block directly */
	return sfc_nand_scan_block_bad(mtd, block, read_oob);
}

/**
 * ingenic_sfcnand_block_markbad - Mark a block as bad
 * @mtd: MTD device structure
 * @ofs: Offset of bad block
 * @write_oob: Function pointer to write OOB data
 */
int ingenic_sfcnand_block_markbad(struct mtd_info *mtd, loff_t ofs,
				  ingenic_sfcnand_write_oob_fn write_oob)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	struct sfc_nand_bbt_info *bbt_info = sfc_nand_get_bbt_info(flash);
	struct mtd_oob_ops ops;
	int block = (int)div_u64(ofs, mtd->erasesize);
	u8 buf[2] = { 0x00, 0x00 };
	int ret = 0;
	loff_t wr_ofs = ofs;

	if (!bbt_info)
		return -EINVAL;

	/* Update BBT in memory */
	if (bbt_info->bbt) {
		sfc_nand_bbt_set_entry(flash, block, SPI_NAND_BBT_BLOCK_WORN);
	}

	/* Write bad block marker to OOB */
	memset(&ops, 0, sizeof(ops));
	ops.datbuf = NULL;
	ops.oobbuf = buf;
	ops.ooboffs = bbt_info->config.badblockpos;
	ops.len = ops.ooblen = 1; /* SPI NAND typically uses 1 byte */
	ops.mode = MTD_OPS_PLACE_OOB;

	ret = write_oob(mtd, wr_ofs, &ops);
	if (ret) {
		dev_err(flash->dev,
			"Failed to write bad block marker at 0x%llx\n", wr_ofs);
		return ret;
	}

	/* Update bad block count */
	mtd->ecc_stats.badblocks++;

	dev_info(flash->dev, "Block at 0x%08llx marked as bad\n", ofs);

	return 0;
}

/**
 * ingenic_sfcnand_bbt_cleanup - Clean up BBT resources
 * @mtd: MTD device structure
 *
 * This function frees all allocated memory for the BBT, including
 * the BBT table itself and the BBT info structure.
 */
void ingenic_sfcnand_bbt_cleanup(struct mtd_info *mtd)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	struct sfc_nand_bbt_info *bbt_info = sfc_nand_get_bbt_info(flash);

	if (bbt_info) {
		/* Free BBT table memory */
		if (bbt_info->bbt) {
			kfree(bbt_info->bbt);
			bbt_info->bbt = NULL;
		}

		/* Free BBT info structure */
		kfree(bbt_info);

		/* Clear global storage */
		sfc_nand_set_bbt_info(flash, NULL);

		dev_info(flash->dev, "BBT resources cleaned up\n");
	}
}