/*
 * SFC controller for SPI protocol, use FIFO and DMA;
 *
 * Copyright (c) 2015 Ingenic
 * Author: <xiaoyang.fu@ingenic.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include "sfc_flash.h"
#include "spinand.h"
#include "ingenic_sfc_common.h"
#include "ingenic_sfc_drv.h"
#include "./nand_device/nand_common.h"
#include "ingenic_sfc_nand_bbt.h"

/* Forward declarations */
static int32_t ingenic_sfcnand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);
static int32_t ingenic_sfcnand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);

static struct sfc_flash *flash_ioctl[2];
#define STATUS_SUSPND	(1<<0)
#define to_ingenic_sfc_nand(mtd_info) container_of(mtd_info, struct sfc_flash, mtd)

/*
 * below is the informtion about nand
 * that user should modify according to nand spec
 * */

static LIST_HEAD(nand_list);

void dump_flash_info(struct sfc_flash *flash)
{
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct ingenic_sfcnand_base_param *param = &nand_info->param;
	struct mtd_partition *partition = nand_info->partition.partition;
	uint8_t num_partition = nand_info->partition.num_partition;

	printk("id_manufactory = 0x%02x\n", nand_info->id_manufactory);
	printk("id_device = 0x%02x\n", nand_info->id_device);

	printk("pagesize = %d\n", param->pagesize);
	printk("blocksize = %d\n", param->blocksize);
	printk("oobsize = %d\n", param->oobsize);
	printk("flashsize = %d\n", param->flashsize);

	printk("tHOLD = %d\n", param->tHOLD);
	printk("tSETUP = %d\n", param->tSETUP);
	printk("tSHSL_R = %d\n", param->tSHSL_R);
	printk("tSHSL_W = %d\n", param->tSHSL_W);

	printk("ecc_max = %d\n", param->ecc_max);
	printk("need_quad = %d\n", param->need_quad);

	while(num_partition--) {
		printk("partition(%d) name=%s\n", num_partition, partition[num_partition].name);
		printk("partition(%d) size = 0x%llx\n", num_partition, partition[num_partition].size);
		printk("partition(%d) offset = 0x%llx\n", num_partition, partition[num_partition].offset);
		printk("partition(%d) mask_flags = 0x%x\n", num_partition, partition[num_partition].mask_flags);
	}
	return;
}

static int32_t ingenic_sfc_nand_read(struct sfc_flash *flash, int32_t pageaddr, int32_t columnaddr, u_char *buffer, size_t len)
{
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct ingenic_sfcnand_ops *ops = nand_info->ops;
	struct sfc_cdt_xfer xfer;
	int32_t ret = 0;

	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	if(nand_info->param.need_quad){
		xfer.cmd_index = NAND_QUAD_READ_TO_CACHE;
	}else{
		xfer.cmd_index = NAND_STANDARD_READ_TO_CACHE;
	}

	/* set addr */
	xfer.rowaddr = pageaddr;

	if(nand_info->param.plane_select){
		xfer.columnaddr = CONVERT_COL_ADDR(pageaddr, columnaddr);
	}else{
		xfer.columnaddr = columnaddr;
	}

	xfer.staaddr0 = SPINAND_ADDR_STATUS;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB0_TRAN_DIR_READ;
	xfer.config.ops_mode = OPS_MODE;
	xfer.config.buf = buffer;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev, "sfc_sync_cdt error ! %s %s %d\n", __FILE__,
			__func__, __LINE__);
		return -EIO;
	}

	/* get status to check nand ecc status */
	ret = ops->get_feature(flash, GET_ECC_STATUS);

	if(xfer.config.ops_mode == DMA_OPS)
		dma_cache_sync(NULL, (void *)xfer.config.buf, xfer.config.datalen, DMA_FROM_DEVICE);

	return ret;
}


static int32_t ingenic_sfc_nand_write(struct sfc_flash *flash, u_char *buffer, uint32_t pageaddr, uint32_t columnaddr, size_t len)
{
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct ingenic_sfcnand_ops *ops = nand_info->ops;
	struct sfc_cdt_xfer xfer;
	int32_t ret = 0;

	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	if(nand_info->param.need_quad){
		xfer.cmd_index = NAND_QUAD_WRITE_ENABLE;
	}else{
		xfer.cmd_index = NAND_STANDARD_WRITE_ENABLE;
	}

	/* set addr */
	xfer.rowaddr = pageaddr;

	if(nand_info->param.plane_select){
		xfer.columnaddr = CONVERT_COL_ADDR(pageaddr, columnaddr);
	}else{
		xfer.columnaddr = columnaddr;
	}

	xfer.staaddr0 = SPINAND_ADDR_STATUS;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB0_TRAN_DIR_WRITE;
	xfer.config.ops_mode = OPS_MODE;
	xfer.config.buf = buffer;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	/* get status to be sure nand write completed */
	ret = ops->get_feature(flash, GET_WRITE_STATUS);

	return  ret;
}

static int32_t ingenic_sfc_nand_erase_blk(struct sfc_flash *flash, uint32_t pageaddr)
{
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct ingenic_sfcnand_ops *ops = nand_info->ops;
	struct sfc_cdt_xfer xfer;
	int32_t ret = 0;

	memset(&xfer, 0, sizeof(xfer));

	/* set index */
	xfer.cmd_index = NAND_ERASE_WRITE_ENABLE;

	/* set addr */
	xfer.rowaddr = pageaddr;
	xfer.staaddr0 = SPINAND_ADDR_STATUS;

	/* set transfer config */
	xfer.dataen = DISABLE;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev, "sfc_sync_cdt error ! %s %s %d\n", __FILE__,
			__func__, __LINE__);
		return -EIO;
	}

	/* get status to be sure nand write completed */
	ret = ops->get_feature(flash, GET_ERASE_STATUS);
	if(ret){
		dev_err(flash->dev,
			"Erase error, get state error ! %s %s %d \n", __FILE__,
			__func__, __LINE__);
	}

	return ret;
}

static int ingenic_sfcnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	uint32_t addr = (uint32_t)instr->addr;
	uint32_t end;
	int32_t ret;

	if(addr % mtd->erasesize) {
		dev_err(flash->dev, "ERROR:%s line %d eraseaddr no align\n", __func__,__LINE__);
		return -EINVAL;
	}
	end = addr + instr->len;
	mutex_lock(&flash->lock);
	while (addr < end) {
		if((ret = ingenic_sfc_nand_erase_blk(flash, addr / mtd->writesize))) {
			dev_err(flash->dev, "spi nand erase error blk id  %d !\n",addr / mtd->erasesize);
			goto erase_exit;
		}
		addr += mtd->erasesize;
	}

erase_exit:
	mutex_unlock(&flash->lock);
	return ret;
}

static int ingenic_sfcnand_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	uint32_t pagesize = mtd->writesize;
	uint32_t pageaddr;
	uint32_t columnaddr;
	uint32_t rlen;
	int32_t ret = -1, reterr = 0, ret_eccvalue = 0;
#ifdef CONFIG_SOC_PRJ007
	int32_t i = 0, cnt = 0;
	struct sfc *sfc = flash->sfc;
	struct sfc_desc *desc = sfc->desc;
#endif

	mutex_lock(&flash->lock);
	while(len) {
		pageaddr = (uint32_t)from / pagesize;
		columnaddr = (uint32_t)from % pagesize;
		rlen = min_t(uint32_t, len, pagesize - columnaddr);

		/* create DMA Descriptors */
		ret = create_sfc_desc(flash, buf, rlen);
		if(ret < 0){
			dev_err(flash->dev, "%s create descriptors error. -%d\n", __func__, ret);
			return ret;
		}
#ifdef CONFIG_SOC_PRJ007
		else if(ret >= 0){
			cnt = ret;
			for(i=0; i<=cnt; i++){
				rlen = desc[0].tran_len;
				pageaddr = (uint32_t)from / pagesize;
				columnaddr = (uint32_t)from % pagesize;
				ret = ingenic_sfc_nand_read(flash, pageaddr, columnaddr, buf, rlen);
				if(ret < 0) {
					dev_err(flash->dev, "%s %s %d: ingenic_sfc_nand_read error, ret = %d, \
						pageaddr = %u, columnaddr = %u, rlen = %u\n",
						__FILE__, __func__, __LINE__,
						ret, pageaddr, columnaddr, rlen);
					reterr = ret;
					if(ret == -EIO)
						break;
				} else if (ret > 0) {
					dev_dbg(flash->dev, "%s %s %d: ingenic_sfc_nand_read, ecc value = %d, \
						    pageaddr = %u, columnaddr = %u, rlen = %u\n",
						__FILE__, __func__, __LINE__,
						ret, pageaddr, columnaddr, rlen);
					ret_eccvalue = ret;
				}

				len -= rlen;
				from += rlen;
				buf += rlen;
				*retlen += rlen;
				desc[0].next_des_addr = 0;
				desc[0].mem_addr = desc[i+1].mem_addr;
				desc[0].tran_len = desc[i+1].tran_len;
				desc[0].link = 0;
			}
			if(ret == EIO)
				break;
		}
#else
		/* DMA Descriptors read */
		ret = ingenic_sfc_nand_read(flash, pageaddr, columnaddr, buf, rlen);
		if(ret < 0) {
			dev_err(flash->dev, "%s %s %d: ingenic_sfc_nand_read error, ret = %d, \
				pageaddr = %u, columnaddr = %u, rlen = %u\n",
					__FILE__, __func__, __LINE__,
					ret, pageaddr, columnaddr, rlen);
			reterr = ret;
			if(ret == -EIO)
				break;
		} else if (ret > 0) {
			dev_dbg(flash->dev, "%s %s %d: ingenic_sfc_nand_read, ecc value = %d, \
				    pageaddr = %u, columnaddr = %u, rlen = %u\n",
					    __FILE__, __func__, __LINE__,
					    ret, pageaddr, columnaddr, rlen);
			ret_eccvalue = ret;
		}

		len -= rlen;
		from += rlen;
		buf += rlen;
		*retlen += rlen;
#endif
	}
	mutex_unlock(&flash->lock);
	return reterr ? reterr : (ret_eccvalue ? ret_eccvalue : ret);
}

#if 0
static int ingenic_sfcnand_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	uint32_t pagesize = mtd->writesize;
	uint32_t pageaddr;
	uint32_t columnaddr;
	uint32_t wlen;
	int32_t ret = -1;
#ifdef CONFIG_SOC_PRJ007
	int32_t i=0, cnt=0;
	struct sfc *sfc = flash->sfc;
	struct sfc_desc *desc = sfc->desc;
#endif

	mutex_lock(&flash->lock);
	while(len) {
		pageaddr = (uint32_t)to / pagesize;
		columnaddr = (uint32_t)to % pagesize;
		wlen = min_t(uint32_t, pagesize - columnaddr, len);
		/* create DMA Descriptors */
		ret = create_sfc_desc(flash, (unsigned char *)buf, wlen);
		if(ret < 0){
			dev_err(flash->dev, "%s create descriptors error. -%d\n", __func__, ret);
			return ret;
		}
#ifdef CONFIG_SOC_PRJ007
		else if(ret >= 0){
			cnt = ret;
			for(i=0; i<=cnt; i++){
				wlen = desc[0].tran_len;
				pageaddr = (uint32_t)to / pagesize;
				columnaddr = (uint32_t)to % pagesize;
				/* DMA Descriptors write */
				if((ret = ingenic_sfc_nand_write(flash, (u_char *)buf, pageaddr, columnaddr, wlen))) {
					dev_err(flash->dev, "%s %s %d : spi nand write fail, ret = %d, \
						pageaddr = %u, columnaddr = %u, wlen = %u\n",
					__FILE__, __func__, __LINE__, ret,
					pageaddr, columnaddr, wlen);
					break;
				}
				*retlen += wlen;
				len -= wlen;
				to += wlen;
				buf += wlen;
				desc[0].next_des_addr = 0;
				desc[0].mem_addr = desc[i+1].mem_addr;
				desc[0].tran_len = desc[i+1].tran_len;
				desc[0].link = 0;
			}
			if(ret)
				break;
		}
#else
		/* DMA Descriptors write */
		if((ret = ingenic_sfc_nand_write(flash, (u_char *)buf, pageaddr, columnaddr, wlen))) {
			dev_err(flash->dev, "%s %s %d : spi nand write fail, ret = %d, \
				pageaddr = %u, columnaddr = %u, wlen = %u\n",
				__FILE__, __func__, __LINE__, ret,
				pageaddr, columnaddr, wlen);
			break;
		}
		*retlen += wlen;
		len -= wlen;
		to += wlen;
		buf += wlen;
#endif
	}
	mutex_unlock(&flash->lock);
	return ret;
}
#endif

typedef enum {
    NAND_OP_READ,
    NAND_OP_WRITE
} nand_operation_t;

static int32_t
ingenic_sfcnand_operate_segment(struct sfc_flash *flash, uint32_t pageaddr,
				uint32_t columnaddr, u_char *buf, uint32_t len,
				const char *segment_type, nand_operation_t op)
{
	int32_t ret;

	ret = create_sfc_desc(flash, buf, len);
	if (ret < 0) {
		dev_err(flash->dev,
			"%s: failed to create %s segment DMA descriptor, error: %d\n",
			__func__, segment_type, ret);
		return ret;
	}

	if (op == NAND_OP_READ) {
		ret = ingenic_sfc_nand_read(flash, pageaddr, columnaddr, buf,
					    len);
	} else {
		ret = ingenic_sfc_nand_write(flash, buf, pageaddr, columnaddr,
					     len);
	}

	if (ret < 0)
	{
		const char *op_str = (op == NAND_OP_READ) ? "read" : "write";
		dev_err(flash->dev,
			"%s: failed to %s %s segment, error: %d, page: %u, column: %u, length: %u\n",
			__func__, op_str, segment_type, ret, pageaddr,
			columnaddr, len);
	}
	else if (ret > 0 && op == NAND_OP_READ)
	{
		dev_dbg(flash->dev,
			"%s: successfully read %s segment, ECC value: %d, page: %u, column: %u, length: %u\n",
			__func__, segment_type, ret, pageaddr, columnaddr, len);
	}

	return ret;
}

static int32_t ingenic_sfcnand_read_oob(struct mtd_info *mtd, loff_t from,
					struct mtd_oob_ops *ops)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	const uint32_t pagesize = mtd->writesize;
	uint32_t pageaddr, columnaddr, rlen;
	int32_t ret = 0, reterr = 0, ret_eccvalue = 0;
	u_char *datbuf = ops->datbuf, *oobbuf = ops->oobbuf;
	size_t datlen = ops->len, ooblen = ops->ooblen;
	loff_t dataddr = from, oobaddr = from;

	ops->retlen = 0;
	ops->oobretlen = 0;

	mutex_lock(&flash->lock);

	while (datlen > 0 && datbuf != NULL) {
		pageaddr = (uint32_t)dataddr / pagesize;
		columnaddr = (uint32_t)dataddr % pagesize;
		rlen = min_t(uint32_t, datlen, pagesize - columnaddr);

		ret = ingenic_sfcnand_operate_segment(flash, pageaddr,
						      columnaddr, datbuf, rlen,
						      "DATA", NAND_OP_READ);
		if (ret < 0) {
			reterr = ret;
			if (ret == -EIO)
				goto read_oob_exit;
		} else if (ret > 0) {
			ret_eccvalue = ret;
		}

		datlen -= rlen;
		dataddr += rlen;
		datbuf += rlen;
		ops->retlen += rlen;
	}

	if (ooblen > 0 && oobbuf != NULL) {
		pageaddr = (uint32_t)from / pagesize;
		columnaddr = pagesize + ops->ooboffs;
		rlen = ooblen;

		ret = ingenic_sfcnand_operate_segment(flash, pageaddr,
						      columnaddr, oobbuf, rlen,
						      "OOB", NAND_OP_READ);
		if (ret < 0) {
			reterr = ret;
			if (ret == -EIO)
				goto read_oob_exit;
		} else if (ret > 0) {
			ret_eccvalue = ret;
		}

		ooblen -= rlen;
		oobaddr += rlen;
		oobbuf += rlen;
		ops->oobretlen += rlen;
	}

read_oob_exit:
	mutex_unlock(&flash->lock);

	return reterr ? reterr : (ret_eccvalue ? ret_eccvalue : ret);
}

static int ingenic_sfcnand_write_oob(struct mtd_info *mtd, loff_t to,
				     struct mtd_oob_ops *ops)
{
	struct sfc_flash *flash = to_ingenic_sfc_nand(mtd);
	const uint32_t pagesize = mtd->writesize;
	uint32_t pageaddr, columnaddr, wlen;
	int32_t ret = 0, reterr = 0;
	u_char *datbuf = ops->datbuf, *oobbuf = ops->oobbuf;
	size_t datlen = ops->len, ooblen = ops->ooblen;
	loff_t dataddr = to;

	ops->retlen = 0;
	ops->oobretlen = 0;

	mutex_lock(&flash->lock);

	while (datlen > 0 && datbuf != NULL) {
		pageaddr = (uint32_t)dataddr / pagesize;
		columnaddr = (uint32_t)dataddr % pagesize;
		wlen = min_t(uint32_t, datlen, pagesize - columnaddr);

		ret = ingenic_sfcnand_operate_segment(flash, pageaddr,
						      columnaddr, datbuf, wlen,
						      "data", NAND_OP_WRITE);
		if (ret < 0) {
			reterr = ret;
			if (ret == -EIO)
				goto write_oob_exit;
		}

		datlen -= wlen;
		dataddr += wlen;
		datbuf += wlen;
		ops->retlen += wlen;
	}

	if (oobbuf != NULL && ooblen > 0) {
		pageaddr = (uint32_t)to / pagesize;
		columnaddr = pagesize + ops->ooboffs;
		wlen = ooblen;
		// printk("write oob pageaddr = 0x%08x columnaddr = 0x%08x ooblen = %d\n", pageaddr, columnaddr, ooblen);

		ret = ingenic_sfcnand_operate_segment(flash, pageaddr,
						      columnaddr, oobbuf, wlen,
						      "OOB", NAND_OP_WRITE);
		if (ret < 0) {
			reterr = ret;
			if (ret == -EIO)
				goto write_oob_exit;
		}

		ooblen -= wlen;
		oobbuf += wlen;
		ops->oobretlen += wlen;
	}

write_oob_exit:
	mutex_unlock(&flash->lock);

	return reterr ? reterr : ret;
}

static int ingenic_sfc_nand_set_feature(struct sfc_flash *flash, uint8_t addr, uint32_t val)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set index */
	xfer.cmd_index = NAND_SET_FEATURE;

	/* set addr */
	xfer.staaddr0 = addr;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = 1;
	xfer.config.data_dir = GLB0_TRAN_DIR_WRITE;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = (uint8_t *)&val;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	return 0;
}

static int ingenic_sfc_nand_get_feature(struct sfc_flash *flash, uint8_t addr, uint8_t *val)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set index */
	xfer.cmd_index = NAND_GET_FEATURE;

	/* set addr */
	xfer.staaddr0 = addr;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = 1;
	xfer.config.data_dir = GLB0_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = (uint8_t *)val;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	return 0;
}

static int32_t ingenic_sfc_nand_dev_init(struct sfc_flash *flash)
{
	int32_t ret;
	/*release protect*/
	uint8_t feature = 0;
	if((ret = ingenic_sfc_nand_set_feature(flash, SPINAND_ADDR_PROTECT, feature)))
		goto exit;

	if((ret = ingenic_sfc_nand_get_feature(flash, SPINAND_ADDR_FEATURE, &feature)))
		goto exit;

	feature |= (1 << 4) | (1 << 3) | (1 << 0);
	if((ret = ingenic_sfc_nand_set_feature(flash, SPINAND_ADDR_FEATURE, feature)))
		goto exit;

	return 0;
exit:
	return ret;
}


/* BBT functions wrapper - call the BBT module functions */
static int ingenic_sfcnand_block_bad_wrapper(struct mtd_info *mtd, loff_t ofs)
{
	return ingenic_sfcnand_block_bad(mtd, ofs, ingenic_sfcnand_read_oob);
}

static int ingenic_sfcnand_block_markbad_wrapper(struct mtd_info *mtd, loff_t ofs)
{
	return ingenic_sfcnand_block_markbad(mtd, ofs, ingenic_sfcnand_write_oob);
}

static int32_t __init ingenic_sfc_nand_try_id(struct sfc_flash *flash)
{
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct ingenic_sfcnand_device *nand_device;
	struct sfc_cdt_xfer xfer;
	uint8_t id_buf[2] = {0};
	unsigned short index[2] = {NAND_TRY_ID, NAND_TRY_ID_DMY};
	uint8_t i = 0;

	for(i = 0; i < 2; i++){
		memset(&xfer, 0, sizeof(xfer));

		/* set index */
		xfer.cmd_index = index[i];

		/* set addr */
		xfer.rowaddr = 0;

		/* set transfer config */
		xfer.dataen = ENABLE;
		xfer.config.datalen = sizeof(id_buf);
		xfer.config.data_dir = GLB0_TRAN_DIR_READ;
		xfer.config.ops_mode = CPU_OPS;
		xfer.config.buf = id_buf;

		if(sfc_sync_cdt(flash->sfc, &xfer)) {
			dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
			return -EIO;
		}

		list_for_each_entry(nand_device, &nand_list, list) {
			if(nand_device->id_manufactory == id_buf[0]) {
				nand_info->id_manufactory = id_buf[0];
				nand_info->id_device = id_buf[1];
				break;
			}
		}

		if(nand_info->id_manufactory && nand_info->id_device)
			    break;
	}

	if(!nand_info->id_manufactory && !nand_info->id_device) {
		dev_err(flash->dev, " ERROR!: don`t support this nand manufactory, please add nand driver.\n");
		return -ENODEV;
	} else {
		struct device_id_struct *device_id = nand_device->id_device_list;
		int32_t id_count = nand_device->id_device_count;
		while(id_count--) {
			if(device_id->id_device == nand_info->id_device) {
			/* notice :base_param and partition param should read from nand */
				nand_info->param = *device_id->param;
				break;
			}
			device_id++;
		}
		if(id_count < 0) {
			dev_err(flash->dev, "ERROR: do support this device, id_manufactory = 0x%02x, id_device = 0x%02x\n", nand_info->id_manufactory, nand_info->id_device);
			return -ENODEV;
		}
		dev_info(flash->dev, "Found Supported nand device, id = 0x%02x%02x,name = %s\n", nand_info->id_manufactory, nand_info->id_device, device_id->name);
	}


	/* fill manufactory special operation and cdt params */
	nand_info->ops = &nand_device->ops;
	nand_info->cdt_params = nand_info->ops->get_cdt_params(flash, nand_info->id_device);

	if (!nand_info->ops->get_feature) {
		if (!nand_info->ops->deal_ecc_status) {
			dev_err(flash->dev,"ERROR:xxx_nand.c \"get_feature()\" and \"deal_ecc_status()\" not define.\n");
			return -ENODEV;
		} else {
			nand_info->ops->get_feature = nand_common_get_feature;
			printk("use nand common get feature interface!\n");
		}
	} else {
		printk("use nand private get feature interface!\n");
	}

	return 0;
}

static int32_t __init nand_partition_param_copy(struct sfc_flash *flash, struct ingenic_sfcnand_burner_param *burn_param) {
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	int i = 0, count = 5, ret;
	size_t retlen = 0;

	/* partition param copy */
	nand_info->partition.num_partition = burn_param->partition_num;

	burn_param->partition = kzalloc(nand_info->partition.num_partition * sizeof(struct ingenic_sfcnand_partition), GFP_KERNEL);
	if (IS_ERR_OR_NULL(burn_param->partition)) {
	    	dev_err(flash->dev, "alloc partition space failed!\n");
			return -ENOMEM;
	}

	nand_info->partition.partition = kzalloc(nand_info->partition.num_partition * sizeof(struct mtd_partition), GFP_KERNEL);
	if (IS_ERR_OR_NULL(nand_info->partition.partition)) {
	    	dev_err(flash->dev, "alloc partition space failed!\n");
		kfree(burn_param->partition);
		return -ENOMEM;
	}

partition_retry_read:
	ret = ingenic_sfcnand_read(&flash->mtd, flash->param_offset + sizeof(*burn_param) - sizeof(burn_param->partition),
		nand_info->partition.num_partition * sizeof(struct ingenic_sfcnand_partition),
		&retlen, (u_char *)burn_param->partition);

	if((ret < 0) && count--)
		goto partition_retry_read;
	if(count < 0) {
		dev_err(flash->dev, "read nand partition failed!\n");
		kfree(burn_param->partition);
		kfree(nand_info->partition.partition);
		return -EIO;
	}

	for(i = 0; i < burn_param->partition_num; i++) {
		nand_info->partition.partition[i].name = burn_param->partition[i].name;
		nand_info->partition.partition[i].size = burn_param->partition[i].size;
		nand_info->partition.partition[i].offset = burn_param->partition[i].offset;
		nand_info->partition.partition[i].mask_flags = burn_param->partition[i].mask_flags;
	}
	return 0;
}

static struct ingenic_sfcnand_burner_param *burn_param;
static int32_t __init flash_part_from_chip(struct sfc_flash *flash) {

	int32_t ret = 0, retlen = 0, count = 5;

	burn_param = kzalloc(sizeof(struct ingenic_sfcnand_burner_param), GFP_KERNEL);
	if(IS_ERR_OR_NULL(burn_param)) {
		dev_err(flash->dev, "alloc burn_param space error!\n");
		return -ENOMEM;
	}

	count = 5;
param_retry_read:
	ret = ingenic_sfcnand_read(&flash->mtd, flash->param_offset,
		sizeof(struct ingenic_sfcnand_burner_param), &retlen, (u_char *)burn_param);
	if((ret < 0) && count--)
		goto param_retry_read;
	if(count < 0) {
		dev_err(flash->dev, "read nand base param failed!\n");
		ret = -EIO;
		goto failed;
	}

	if(burn_param->magic_num != SPINAND_MAGIC_NUM) {
		dev_info(flash->dev, "NOTICE: this flash haven`t param, magic_num:%x\n", burn_param->magic_num);
		ret = -EINVAL;
		goto failed;
	}

	if(nand_partition_param_copy(flash, burn_param)) {
		ret = -ENOMEM;
		goto failed;
	}

	return 0;
failed:
	kfree(burn_param);
	return ret;

}

static int32_t __init flash_part_from_board(struct sfc_flash *flash, struct ingenic_sfc_info *board_info) {
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct ingenic_sfcnand_partition *flash_partition = board_info->flash_partition;
	int8_t i = 0;

	nand_info->partition.num_partition = board_info->num_partition;
	nand_info->partition.partition = kzalloc(nand_info->partition.num_partition * sizeof(struct mtd_partition), GFP_KERNEL);
	if (IS_ERR_OR_NULL(nand_info->partition.partition)) {
		dev_err(flash->dev, "alloc partition space failed!\n");
		nand_info->partition.num_partition = 0;
		return -ENOMEM;
	}

	for(i = 0; i < board_info->num_partition; i++) {
		nand_info->partition.partition[i].name = flash_partition[i].name;
		nand_info->partition.partition[i].size = flash_partition[i].size;
		nand_info->partition.partition[i].offset = flash_partition[i].offset;
		nand_info->partition.partition[i].mask_flags = flash_partition[i].mask_flags;
	}
	return 0;
}

static int32_t __init ingenic_sfcnand_partition(struct sfc_flash *flash, struct ingenic_sfc_info *board_info) {
	int32_t ret = 0;
	if(!board_info || !board_info->use_board_info) {
		if((ret = flash_part_from_chip(flash)))
			dev_info(flash->dev, "read partition from flash failed!\n");
	} else {
		if((ret = flash_part_from_board(flash, board_info)))
			dev_err(flash->dev, "copy partition from board failed!\n");
	}
	return ret;
}

int ingenic_sfcnand_register(struct ingenic_sfcnand_device *flash) {
	list_add_tail(&flash->list, &nand_list);
	return 0;
}
EXPORT_SYMBOL_GPL(ingenic_sfcnand_register);

/*
 *MK_CMD(cdt, cmd, LINK, ADDRMODE, DATA_EN)
 *MK_ST(cdt, st, LINK, ADDRMODE, ADDR_WIDTH, POLL_EN, DATA_EN, TRAN_MODE)
 */
static void params_to_cdt(cdt_params_t *params, struct sfc_cdt *cdt)
{
	/* 6. nand standard read */
	MK_CMD(cdt[NAND_STANDARD_READ_TO_CACHE], params->r_to_cache, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NAND_STANDARD_READ_GET_FEATURE], params->oip, 1, STA_ADDR0, 1, ENABLE, DISABLE, TM_STD_SPI);
	MK_CMD(cdt[NAND_STANDARD_READ_FROM_CACHE], params->standard_r, 0, COL_ADDR, ENABLE);

	/* 7. nand quad read */
	MK_CMD(cdt[NAND_QUAD_READ_TO_CACHE], params->r_to_cache, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NAND_QUAD_READ_GET_FEATURE], params->oip, 1, STA_ADDR0, 1, ENABLE, DISABLE, TM_STD_SPI);
	MK_CMD(cdt[NAND_QUAD_READ_FROM_CACHE], params->quad_r, 0, COL_ADDR, ENABLE);

	/* 8. nand standard write */
	MK_CMD(cdt[NAND_STANDARD_WRITE_ENABLE], params->w_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NAND_STANDARD_WRITE_TO_CACHE], params->standard_w_cache, 1, COL_ADDR, ENABLE);
	MK_CMD(cdt[NAND_STANDARD_WRITE_EXEC], params->w_exec, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NAND_STANDARD_WRITE_GET_FEATURE], params->oip, 0, STA_ADDR0, 1, ENABLE, DISABLE, TM_STD_SPI);

	/* 9. nand quad write */
	MK_CMD(cdt[NAND_QUAD_WRITE_ENABLE], params->w_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NAND_QUAD_WRITE_TO_CACHE], params->quad_w_cache, 1, COL_ADDR, ENABLE);
	MK_CMD(cdt[NAND_QUAD_WRITE_EXEC], params->w_exec, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NAND_QUAD_WRITE_GET_FEATURE], params->oip, 0, STA_ADDR0, 1, ENABLE, DISABLE, TM_STD_SPI);

	/* 10. block erase */
	MK_CMD(cdt[NAND_ERASE_WRITE_ENABLE], params->w_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NAND_BLOCK_ERASE], params->b_erase, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NAND_ERASE_GET_FEATURE], params->oip, 0, STA_ADDR0, 1, ENABLE, DISABLE, TM_STD_SPI);

	/* 11. ecc status read */
	MK_CMD(cdt[NAND_ECC_STATUS_READ], params->ecc_r, 0, DEFAULT_ADDRMODE, ENABLE);

}

static void nand_create_cdt_table(struct sfc_flash *flash, uint32_t flag)
{
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	cdt_params_t *cdt_params;
	struct sfc_cdt sfc_cdt[INDEX_MAX_NUM];

	memset(sfc_cdt, 0, sizeof(sfc_cdt));
	if(flag == DEFAULT_CDT)
	{
		/* 1. reset */
		sfc_cdt[NAND_RESET].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
		sfc_cdt[NAND_RESET].xfer = CMD_XFER(0, DISABLE, 0, DISABLE, SPINAND_CMD_RESET);
		sfc_cdt[NAND_RESET].staExp = 0;
		sfc_cdt[NAND_RESET].staMsk = 0;

		/* 2. try id */
		sfc_cdt[NAND_TRY_ID].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
		sfc_cdt[NAND_TRY_ID].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINAND_CMD_RDID);
		sfc_cdt[NAND_TRY_ID].staExp = 0;
		sfc_cdt[NAND_TRY_ID].staMsk = 0;

		/* 3. try id with dummy */
		/*
		 * There are some NAND flash, try ID operation requires 8-bit dummy value to be all 0,
		 * so use 1 byte address instead of dummy here.
		 */
		sfc_cdt[NAND_TRY_ID_DMY].link = CMD_LINK(0, ROW_ADDR, TM_STD_SPI);
		sfc_cdt[NAND_TRY_ID_DMY].xfer = CMD_XFER(1, DISABLE, 0, ENABLE, SPINAND_CMD_RDID);
		sfc_cdt[NAND_TRY_ID_DMY].staExp = 0;
		sfc_cdt[NAND_TRY_ID_DMY].staMsk = 0;

		/* 4. set feature */
		sfc_cdt[NAND_SET_FEATURE].link = CMD_LINK(0, STA_ADDR0, TM_STD_SPI);
		sfc_cdt[NAND_SET_FEATURE].xfer = CMD_XFER(1, DISABLE, 0, ENABLE, SPINAND_CMD_SET_FEATURE);
		sfc_cdt[NAND_SET_FEATURE].staExp = 0;
		sfc_cdt[NAND_SET_FEATURE].staMsk = 0;

		/* 5. get feature */
		sfc_cdt[NAND_GET_FEATURE].link = CMD_LINK(0, STA_ADDR0, TM_STD_SPI);
		sfc_cdt[NAND_GET_FEATURE].xfer = CMD_XFER(1, DISABLE, 0, ENABLE, SPINAND_CMD_GET_FEATURE);
		sfc_cdt[NAND_GET_FEATURE].staExp = 0;
		sfc_cdt[NAND_GET_FEATURE].staMsk = 0;

		/* first create cdt table */
		write_cdt(flash->sfc, sfc_cdt, NAND_RESET, NAND_GET_FEATURE);
	}

	if(flag == UPDATE_CDT){
		cdt_params = nand_info->cdt_params;
		params_to_cdt(cdt_params, sfc_cdt);

		/* second create cdt table */
		write_cdt(flash->sfc, sfc_cdt, NAND_STANDARD_READ_TO_CACHE, NAND_ECC_STATUS_READ);
	}
	//dump_cdt(flash->sfc);
}

static int request_sfc_desc(struct sfc_flash *flash)
{
	struct sfc *sfc = flash->sfc;
	sfc->desc = (struct sfc_desc *)dma_alloc_coherent(flash->dev, sizeof(struct sfc_desc) * DESC_MAX_NUM, &sfc->desc_pyaddr, GFP_KERNEL);
	if(flash->sfc->desc == NULL){
		return -ENOMEM;
	}
	sfc->desc_max_num = DESC_MAX_NUM;

	return 0;
}

static int ingenic_sfc_open(struct inode *inode, struct file *filp)
{
	unsigned int major = imajor(inode);

	if(major == INGENIC_SFC_MAJOR)
		filp->private_data = flash_ioctl[0];
	else if(major == INGENIC_SFC_MAJOR - 1)
		filp->private_data = flash_ioctl[1];
	else
		printk("sfc device major error!\n");
	return 0;
}

static int ingenic_sfc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long ingenic_sfc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct sfc_flash *flash = file->private_data;
	static unsigned char *data_buf, *buf;
	struct sfc_transfer trans_config;
	uint32_t len,rlen,retlen = 0,addr;
	int ret;

	if(_IOC_TYPE(cmd) != SFC_IOTC_CHECK) {
		printk("ioctl cmd error !\n");
		return -EINVAL;
	}

	switch(cmd) {
	case SFC_READ_SR:
	case SFC_READ_SR1:
	case SFC_READ_SR2:
	case SFC_WRITE_SR:
	case SFC_WRITE_SR1:
	case SFC_WRITE_SR2:
		printk("nand not support ioctl read/write status register!\n");
		break;
	case SFC_DO_READ:
		ret = copy_from_user(&trans_config, \
			(struct transfer __user *)arg,sizeof(trans_config));
		if(ret)
			return -EINVAL;

		switch(trans_config.status) {
			case MALLOC_READ:
				data_buf = kmalloc(trans_config.len,GFP_KERNEL);
				if(!data_buf) {
					printk("alloc buf failed! %s %s %d\n",__FILE__,__func__,__LINE__);
					kfree(data_buf);
					return -ENOMEM;
				}
			case BUF_READ:
				addr = trans_config.addr;
				len = trans_config.len;
				buf = data_buf;
				while(len){
					ret = ingenic_sfcnand_block_bad_wrapper(&flash->mtd, addr);
					if(ret) {
						addr += flash->mtd.erasesize;
						printk("sfc nand bad block ,ret = %d, addr = 0x%x\n",ret,addr);
						continue;
					}
					rlen = len < flash->mtd.erasesize ? len : flash->mtd.erasesize;
					/* printk("len = 0x%x,addr = 0x%x,buf = 0x%p,rlen = 0x%x\n",len, addr, buf, rlen); */
					ret = ingenic_sfcnand_read(&flash->mtd, addr, rlen, &retlen, buf);
					if (ret < 0)
						return ret;

					buf += rlen;
					addr += rlen;
					len -= rlen;
					trans_config.retlen += retlen;
				}
				break;
			case FREE_BUF:
				kfree(data_buf);
				return 0;
			default:
				printk("trans_config status error ,status = %d,%s %s %d\n",trans_config.status,__FILE__,__func__,__LINE__);
				return -EINVAL;
		}

		trans_config.d_vir_buf_addr = (uint32_t)data_buf;
		trans_config.d_phy_buf_addr = (uint32_t)virt_to_phys(data_buf);
		ret = copy_to_user((struct transfer __user *)arg, \
			&trans_config,sizeof(trans_config));
		if(ret)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations ingenic_sfc_ops = {
       .owner = THIS_MODULE,
       .open = ingenic_sfc_open,
       .release = ingenic_sfc_release,
       .unlocked_ioctl = ingenic_sfc_ioctl,
};

int ingenic_sfc_nand_probe(struct sfc_flash *flash)
{
	const char *ingenic_probe_types[] = {"cmdlinepart",NULL};
	struct ingenic_sfc_info *board_info = NULL;
	dev_t devid;
	struct ingenic_sfcnand_flashinfo *nand_info;
	int32_t ret = 0, sfc_index = -1;
	int chip_param = 1;
	char mtdname[16], sfcdev_name[16];

	nand_info = kzalloc(sizeof(struct ingenic_sfcnand_flashinfo), GFP_KERNEL);
	if(IS_ERR_OR_NULL(nand_info)) {
		return -ENOMEM;
	}

	flash->flash_info = nand_info;
#define THOLD	5
#define TSETUP	5
#define TSHSL_R	    20
#define TSHSL_W	    50

	set_flash_timing(flash->sfc, THOLD, TSETUP, TSHSL_R, TSHSL_W);

	/* request DMA Descriptor space */
	ret = request_sfc_desc(flash);
	if(ret){
		dev_err(flash->dev, "Failure to request DMA descriptor space!\n");
		ret = -ENOMEM;
		dma_free_coherent(flash->dev, sizeof(struct sfc_desc) * DESC_MAX_NUM, flash->sfc->desc, flash->sfc->desc_pyaddr);
		goto free_base;
	}

	/* Try creating default CDT table */
	flash->create_cdt_table = nand_create_cdt_table;
	flash->create_cdt_table(flash, DEFAULT_CDT);

#ifdef CONFIG_ZERATUL
	nand_flash_init(flash);
#endif

	if((ret = ingenic_sfc_nand_dev_init(flash))) {
		dev_err(flash->dev, "nand device init failed!\n");
		goto free_base;
	}

	if((ret = ingenic_sfc_nand_try_id(flash))) {
		dev_err(flash->dev, "try device id failed\n");
		goto free_base;
	}

	/* Update to private CDT table */
	flash->create_cdt_table(flash, UPDATE_CDT);

	set_flash_timing(flash->sfc, nand_info->param.tHOLD,
			nand_info->param.tSETUP, nand_info->param.tSHSL_R, nand_info->param.tSHSL_W);
	sfc_index  = of_alias_get_id(flash->dev->of_node, "sfc");
	sprintf(mtdname, "sfc%d_nand", sfc_index );
	flash->sfc->sfc_index= sfc_index;
	flash->mtd.name = mtdname;
	flash->mtd.owner = THIS_MODULE;
	flash->mtd.type = MTD_NANDFLASH;
	flash->mtd.flags |= MTD_CAP_NANDFLASH;
	flash->mtd.erasesize = nand_info->param.blocksize;
	flash->mtd.writesize = nand_info->param.pagesize;
	flash->mtd.size = nand_info->param.flashsize;
	flash->mtd.oobsize = nand_info->param.oobsize;
	flash->mtd.writebufsize = flash->mtd.writesize;
	flash->mtd.bitflip_threshold = flash->mtd.ecc_strength = nand_info->param.ecc_max - 1;

	flash->mtd.priv = NULL;
	flash->mtd._erase = ingenic_sfcnand_erase;
	flash->mtd._read_oob = ingenic_sfcnand_read_oob;
	flash->mtd._write_oob = ingenic_sfcnand_write_oob;
	flash->mtd._block_isbad = ingenic_sfcnand_block_bad_wrapper;
	flash->mtd._block_markbad = ingenic_sfcnand_block_markbad_wrapper;

	/* Scan BBT using our own implementation */
	ret = ingenic_sfcnand_scan_bbt(&flash->mtd, ingenic_sfcnand_read_oob);
	if (ret) {
		dev_err(flash->dev, "Failed to scan BBT: %d\n", ret);
		goto free_base;
	}

	if((ret = ingenic_sfcnand_partition(flash, board_info))) {
		if(ret == -EINVAL)
		{
		    chip_param = 0;
			dev_info(flash->dev, "read mtdparts!\n");
		}else{
			dev_err(flash->dev, "read flash partition failed!\n");
			goto free_base;
		}
	}else{
		flash->mtd.name = "chip_param";
	}

	/*	dump_flash_info(flash);*/
	if(chip_param)
		ret = mtd_device_parse_register(&flash->mtd, ingenic_probe_types, NULL, nand_info->partition.partition, nand_info->partition.num_partition);
	else
		ret = mtd_device_parse_register(&flash->mtd, NULL, NULL, NULL, 0);
	if (ret) {
		kfree(nand_info->partition.partition);
		if(!board_info->use_board_info) {
			kfree(burn_param->partition);
			kfree(burn_param);
		}
		ret = -ENODEV;
		goto free_base;
	}

	flash_ioctl[sfc_index] = flash;
	flash->major = INGENIC_SFC_MAJOR - sfc_index;
	sprintf(sfcdev_name, "ingenic_sfc%d", sfc_index );
	devid = MKDEV(flash->major, 0);
	register_chrdev_region(devid, 1, sfcdev_name);

	flash->cdev.owner = THIS_MODULE;
	cdev_init(&flash->cdev, &ingenic_sfc_ops);
	cdev_add(&flash->cdev, devid, 1);

	flash->class = class_create(THIS_MODULE, sfcdev_name);
	if (IS_ERR(flash->class)) {
		unregister_chrdev(flash->major,sfcdev_name);
		ret = -EBUSY;
		goto free_base;
	}

	flash->device = device_create(flash->class, NULL, devid, NULL, sfcdev_name);
	if (IS_ERR(flash->device)){
		class_destroy(flash->class);
		unregister_chrdev(flash->major,sfcdev_name);
		ret = -EBUSY;
		goto free_base;
	}

	dev_info(flash->dev,"SPI NAND MTD LOAD OK\n");
	return 0;

free_base:
	/* Clean up BBT resources */
	ingenic_sfcnand_bbt_cleanup(&flash->mtd);
	kfree(nand_info);
	return ret;
}

int ingenic_sfc_nand_suspend(struct sfc_flash *flash)
{
	return 0;
}

int ingenic_sfc_nand_resume(struct sfc_flash *flash)
{
	struct ingenic_sfcnand_flashinfo *nand_info;
	int32_t ret;

	nand_info = flash->flash_info;

	set_flash_timing(flash->sfc, THOLD, TSETUP, TSHSL_R, TSHSL_W);
	flash->create_cdt_table(flash, DEFAULT_CDT);

	if((ret = ingenic_sfc_nand_dev_init(flash))) {
		dev_err(flash->dev, "nand device init failed!\n");
		goto free_base;
	}
	flash->create_cdt_table(flash, UPDATE_CDT);

	set_flash_timing(flash->sfc, nand_info->param.tHOLD,
			nand_info->param.tSETUP, nand_info->param.tSHSL_R, nand_info->param.tSHSL_W);

	return 0;

free_base:
	kfree(nand_info);
	return ret;
}
