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
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/page.h>
#include "sfc_flash.h"
#include "spinor.h"
#include "ingenic_sfc_common.h"
#include "ingenic_sfc_drv.h"

//#define DEBUG_CLONER_PARAMS

static struct sfc_flash *flash_ioctl[2];
#define STATUS_SUSPND	(1<<0)

static struct spi_nor_cmd_info read_standard_table = {
	.cmd = SPINOR_OP_READ,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info read_quad_table = {
	.cmd = SPINOR_OP_READ_1_1_4,
	.dummy_byte = 8,
	.addr_nbyte = 3,
	.transfer_mode = 5,
};
static struct spi_nor_cmd_info write_standard_table = {
	.cmd = SPINOR_OP_PP,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info write_quad_table = {
	.cmd = SPINOR_OP_QPP,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 5,
};
static struct spi_nor_cmd_info sector_erase_table_4k = {
	.cmd = SPINOR_OP_BE_4K,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info sector_erase_table_32k = {
	.cmd = SPINOR_OP_BE_32K,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};

static struct spi_nor_cmd_info sector_erase_table_64k = {
	.cmd = SPINOR_OP_SE,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info wr_en_table = {
	.cmd = SPINOR_OP_WREN,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info en4byte_table = {
	.cmd = SPINOR_OP_EN4B,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info set_gb_lock_table = {
	.cmd = SPINOR_GB_LOCK,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info set_gb_unlock_table = {
	.cmd = SPINOR_GB_UNLOCK,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info set_ib_lock_table = {
	.cmd = SPINOR_IB_LOCK,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info set_ib_unlock_table = {
	.cmd = SPINOR_IB_UNLOCK,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info rb_lock_table = {
	.cmd = SPINOR_RB_LOCK,
	.dummy_byte = 0,
	.addr_nbyte = 3,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info set_status_table = {
	.cmd = SPINOR_OP_WRSR,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info set_status1_table = {
	.cmd = SPINOR_OP_WRSR_1,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info set_status2_table = {
	.cmd = SPINOR_OP_WRSR_2,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_st_info quad_set_table = {
	.cmd = SPINOR_OP_WRSR_1,
	.bit_shift = 1,
	.mask = 1,
	.val = 1,
	.len = 1,
	.dummy = 0,
};
static struct spi_nor_st_info quad_get_table = {
	.cmd = SPINOR_OP_RDSR_1,
	.bit_shift = 1,
	.mask = 1,
	.val = 1,
	.len = 1,
	.dummy = 0,
};
static struct spi_nor_st_info busy_table = {
	.cmd = SPINOR_OP_RDSR,
	.bit_shift = 0,
	.mask = 1,
	.val = 0,
	.len = 1,
	.dummy = 0,
};
static struct spi_nor_cmd_info enotp_table = {
	.cmd = SPINOR_OP_ENOTP,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};
static struct spi_nor_cmd_info exotp_table = {
	.cmd = SPINOR_OP_EXOTP,
	.dummy_byte = 0,
	.addr_nbyte = 0,
	.transfer_mode = 0,
};

static struct burner_params *burner_params;
struct sfc_flash *to_ingenic_spi_norflash(struct mtd_info *mtd_info)
{
	return container_of(mtd_info, struct sfc_flash, mtd);
}

#ifdef CONFIG_JZ_SFC_NOR_RESET
int32_t sfc_nor_reset(struct sfc_flash *flash)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	xfer.cmd_index = NOR_RESET_ENABLE;

	/* set addr */
	xfer.rowaddr = 0;
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = DISABLE;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	udelay(100);
	return 0;
}
#endif

static int sfc_nor_read_status(struct sfc_flash *flash,unsigned int addr, unsigned char *buf, size_t len)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	if(addr == SPINOR_OP_RDSR)
		xfer.cmd_index = NOR_GET_STATUS;
	else if(addr == SPINOR_OP_RDSR_1)
		xfer.cmd_index = NOR_GET_STATUS_1;
	else if(addr == SPINOR_OP_RDSR_2)
		xfer.cmd_index = NOR_GET_STATUS_2;
	else
		dev_err(flash->dev,"sfc read status index error ! %s %s %d\n",__FILE__,__func__,__LINE__);


	/* set addr */
	xfer.rowaddr = 0;
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB0_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = buf;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}
	return 0;
}

static int sfc_nor_write_status(struct sfc_flash *flash,unsigned char addr, unsigned char *buf, size_t len)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	if(addr == SPINOR_OP_WRSR)
		xfer.cmd_index = NOR_STATUS_SET_ENABLE;
	else if(addr == SPINOR_OP_WRSR_1)
		xfer.cmd_index = NOR_STATUS_1_SET_ENABLE;
	else if(addr == SPINOR_OP_WRSR_2)
		xfer.cmd_index = NOR_STATUS_2_SET_ENABLE;
	else
		dev_err(flash->dev,"sfc write status index error ! %s %s %d\n",__FILE__,__func__,__LINE__);

	/* set addr */
	xfer.rowaddr = 0;
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB0_TRAN_DIR_WRITE;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = (uint8_t *)buf;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	if(xfer.config.ops_mode == DMA_OPS)
		dma_cache_sync(NULL, (void *)xfer.config.buf, xfer.config.datalen, DMA_FROM_DEVICE);
	return 0;
}

int sfc_nor_read_id(struct sfc_flash *flash)
{
	struct sfc_cdt_xfer xfer;
	unsigned char buf[3];
	unsigned int chip_id = 0;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	xfer.cmd_index = NOR_READ_ID;

	/* set addr */
	xfer.rowaddr = 0;
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = 3;
	xfer.config.data_dir = GLB0_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = buf;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	if(xfer.config.ops_mode == DMA_OPS)
		dma_cache_sync(NULL, (void *)xfer.config.buf, xfer.config.datalen, DMA_FROM_DEVICE);

	chip_id = ((buf[0] & 0xff) << 16) | ((buf[1] & 0xff) << 8) | (buf[2] & 0xff);

	return chip_id;
}

static int sfc_do_lock(struct sfc_flash *flash, uint32_t addr, uint8_t flag)
{
	struct sfc_cdt_xfer xfer;

	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	xfer.cmd_index =  flag ? NOR_IB_LOCK_ENABLE : NOR_IB_UNLOCK_ENABLE;
	/* set addr */
	xfer.rowaddr = addr;

	/* set transfer config */
	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	return 0;

}

static unsigned int sfc_do_read(struct sfc_flash *flash, unsigned int addr, unsigned char *buf, size_t len)
{
	struct spinor_flashinfo *nor_info = flash->flash_info;
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	if (nor_info->quad_succeed) {
		xfer.cmd_index = NOR_READ_QUAD;
	} else {
		xfer.cmd_index = NOR_READ_STANDARD;
	}

	/* set addr */
	xfer.columnaddr = 0;
	xfer.rowaddr = addr;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB0_TRAN_DIR_READ;
	xfer.config.ops_mode = OPS_MODE;
	xfer.config.buf = buf;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	if(xfer.config.ops_mode == DMA_OPS)
		dma_cache_sync(NULL, (void *)buf, len, DMA_FROM_DEVICE);

	return len;
}

static unsigned  int sfc_do_write(struct sfc_flash *flash, unsigned int addr, const unsigned char *buf, size_t len)
{
	struct spinor_flashinfo *nor_info = flash->flash_info;
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	if (nor_info->quad_succeed) {
		xfer.cmd_index = NOR_WRITE_QUAD_ENABLE;
	} else {
		xfer.cmd_index = NOR_WRITE_STANDARD_ENABLE;
	}

	/* set addr */
	xfer.columnaddr = 0;
	xfer.rowaddr = addr;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB0_TRAN_DIR_WRITE;
	xfer.config.ops_mode = OPS_MODE;
	xfer.config.buf = (uint8_t *)buf;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	return len;
}

static int sfc_do_erase(struct sfc_flash *flash, uint32_t addr, uint32_t flash_erase_size)
{
	struct sfc_cdt_xfer xfer;

	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	switch (flash_erase_size) {
		case 0x1000:
			xfer.cmd_index = NOR_ERASE_WRITE_ENABLE_4K;
			break;
		case 0x8000:
			xfer.cmd_index = NOR_ERASE_WRITE_ENABLE_32K;
			break;
		case 0x10000:
			xfer.cmd_index = NOR_ERASE_WRITE_ENABLE_64K;
			break;
	}

	/* set addr */
	xfer.rowaddr = addr;

	/* set transfer config */
	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	return 0;
}

static int sfc_read(struct sfc_flash *flash, loff_t from, size_t len, unsigned char *buf)
{
	int32_t ret = -1;
#ifdef CONFIG_SOC_PRJ007
	int32_t retlen = 0;
	uint32_t i = 0, cnt = 0;
	struct sfc *sfc = flash->sfc;
	struct sfc_desc *desc = sfc->desc;
#endif

	//memset(flash->sfc->desc, 0, sizeof(struct sfc_desc) * DESC_MAX_NUM);  //1024 bytes space

	/* create DMA Descriptors */
	ret = create_sfc_desc(flash, buf, len);
	if(ret < 0){
		dev_err(flash->dev, "%s create descriptors error. -%d\n", __func__, ret);
		return ret;
	}
#ifdef CONFIG_SOC_PRJ007
	else if(ret>0){
		cnt = ret;
		for (i=0;i<=cnt;i++){
			len = desc[0].tran_len;
			ret = sfc_do_read(flash, (unsigned int)from, buf, len);
			buf += desc[0].tran_len;
			from += desc[0].tran_len;
			desc[0].next_des_addr = 0;
			desc[0].mem_addr = desc[i+1].mem_addr;
			desc[0].tran_len = desc[i+1].tran_len;
			desc[0].link = 0;
			retlen += ret;
		}
		return retlen;
	}
#endif
	//dump_desc(flash->sfc, ret);

	/* DMA Descriptors read */
	ret = sfc_do_read(flash, (unsigned int)from, buf, len);

	return ret;
}

static int sfc_write(struct sfc_flash *flash, loff_t to, size_t len, const unsigned char *buf)
{
	int32_t ret = -1;
#ifdef CONFIG_SOC_PRJ007
	uint32_t retlen = 0;
	uint32_t i = 0, cnt = 0;
	struct sfc *sfc = flash->sfc;
	struct sfc_desc *desc = sfc->desc;
#endif

	//memset(flash->sfc->desc, 0, sizeof(struct sfc_desc) * DESC_MAX_NUM);  //1024 bytes space

	/* create DMA Descriptors */
	ret = create_sfc_desc(flash, (unsigned char *)buf, len);
	if(ret < 0){
		dev_err(flash->dev, "%s create descriptors error. -%d\n", __func__, ret);
		return ret;
	}
#ifdef CONFIG_SOC_PRJ007
	else if(ret>0){
		cnt = ret;
		for (i=0;i<=cnt;i++){
			len = desc[0].tran_len;
			ret = sfc_do_write(flash, (unsigned int)to, buf, len);
			buf += desc[0].tran_len;
			to += desc[0].tran_len;
			desc[0].next_des_addr = 0;
			desc[0].mem_addr = desc[i+1].mem_addr;
			desc[0].tran_len = desc[i+1].tran_len;
			desc[0].link = 0;
			retlen += ret;
		}
		return retlen;
	}
#endif
	//dump_desc(flash->sfc, ret);

	/* DMA Descriptors write */
	ret = sfc_do_write(flash, (unsigned int)to, buf, len);

	return ret;
}

static int ingenic_spi_norflash_read_lock_status(struct sfc_flash *flash, uint32_t addr, uint8_t *buf)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	xfer.cmd_index = NOR_RB_LOCK;

	/* set addr */
	xfer.rowaddr = addr;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = 1;
	xfer.config.data_dir = GLB0_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = buf;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}
	return 0;
}

static int ingenic_spi_norflash_lock(struct sfc_flash *flash, uint32_t offset_addr, uint32_t len, uint8_t flag)
{
	struct sfc_cdt_xfer xfer;
	uint32_t addr, end;
	int32_t ret;
	memset(&xfer, 0, sizeof(xfer));

	mutex_lock(&flash->lock);

	addr = (offset_addr & (0x10000 - 1));
	if (addr) {
		dev_err(flash->dev, "%s lock/unlock addr no align\n", __func__);
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}
	end = (len & (0x10000 - 1));
	if (end) {
		dev_err(flash->dev,"%s lock/unlock size no align\n", __func__);
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}
	addr = (uint32_t)offset_addr;
	end = addr + (uint32_t)len;

	if(len != flash->mtd.size) {
		while(addr < end)
		{
			ret = sfc_do_lock(flash, addr, flag);
			if (ret) {
				dev_err(flash->dev,"lock/unlock error !\n");
				mutex_unlock(&flash->lock);
				return ret;
			}
			if(addr < 0x10000 || (addr >= flash->mtd.size - 0x10000 && addr < flash->mtd.size))
				addr += 0x1000;
			else
				addr += 0x10000;
		}
	} else {
		xfer.cmd_index = flag ? NOR_GB_LOCK_ENABLE : NOR_GB_UNLOCK_ENABLE;
		if(sfc_sync_cdt(flash->sfc, &xfer)) {
			dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
			mutex_unlock(&flash->lock);
			return -EIO;
		}
	}
	mutex_unlock(&flash->lock);

	return 0;
}

static int ingenic_spi_norflash_read(struct mtd_info *mtd, loff_t from, size_t len,size_t *retlen, unsigned char *buf)
{
	struct sfc_flash *flash = to_ingenic_spi_norflash(mtd);

	mutex_lock(&flash->lock);
	*retlen = sfc_read(flash, from, len, buf);
	mutex_unlock(&flash->lock);

	return 0;
}

static int ingenic_spi_norflash_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const unsigned char *buf)
{
	u32 page_offset, actual_len;
	struct sfc_flash *flash = to_ingenic_spi_norflash(mtd);
	struct spinor_flashinfo *nor_info = flash->flash_info;
	struct spi_nor_info *spi_nor_info = nor_info->nor_flash_info;
	int ret;

	mutex_lock(&flash->lock);

	page_offset = to & (spi_nor_info->page_size - 1);
	/* do all the bytes fit onto one page? */
	if (page_offset + len <= spi_nor_info->page_size) {
		ret = sfc_write(flash, (unsigned int)to, len, buf);
		*retlen = ret;
	} else {
		u32 i;

		/* the size of data remaining on the first page */
		actual_len = spi_nor_info->page_size - page_offset;
		ret = sfc_write(flash, (unsigned int)to, actual_len, buf);
		*retlen += ret;

		/* write everything in flash->page_size chunks */
		for (i = actual_len; i < len; i += mtd->writesize) {
			actual_len = len - i;
			if (actual_len >= mtd->writesize)
				actual_len = mtd->writesize;

			ret = sfc_write(flash, (unsigned int)to + i, actual_len, buf + i);
			*retlen += ret;
		}
	}
	mutex_unlock(&flash->lock);
	return 0;
}

static int ingenic_spi_norflash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct sfc_flash *flash = to_ingenic_spi_norflash(mtd);
	struct spinor_flashinfo *nor_info = flash->flash_info;
	struct spi_nor_info *spi_nor_info = nor_info->nor_flash_info;
	uint32_t erase_size;
	uint32_t addr, end;
	int ret;

#ifndef CONFIG_ZERATUL
	erase_size = mtd->erasesize;
#else
	erase_size = instr->mtd->erasesize;
#endif
	mutex_lock(&flash->lock);

	addr = (instr->addr & (erase_size - 1));
	if (addr) {
		dev_err(flash->dev, "%s eraseaddr no align\n", __func__);
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}
	end = (instr->len & (erase_size - 1));
	if (end) {
		dev_err(flash->dev,"%s erasesize no align\n", __func__);
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}
	addr = (uint32_t)instr->addr;
	end = addr + (uint32_t)instr->len;

	while (addr < end) {
		ret = sfc_do_erase(flash, addr, erase_size);
		if (ret) {
			dev_err(flash->dev,"erase error !\n");
			mutex_unlock(&flash->lock);
			instr->state = MTD_ERASE_FAILED;
			return ret;
		}
		addr += erase_size;
	}
	mutex_unlock(&flash->lock);
	instr->state = MTD_ERASE_DONE;

	mtd_erase_callback(instr);
	return 0;
}
#if 0
static int32_t ingenic_spi_norflash_read_params(struct sfc_flash *flash, loff_t from, size_t len, uint8_t *buf)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	xfer.cmd_index = NOR_READ_STANDARD;

	xfer.columnaddr = 0;
	xfer.rowaddr = from;

	xfer.dataen =  ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB0_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = buf;

	if(sfc_sync_cdt(flash->sfc, &xfer)) {
		dev_err(flash->dev,"sfc_sync_cdt error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	return 0;
}
#endif
#ifdef DEBUG_CLONER_PARAMS
static void dump_cloner_params(struct burner_params *params)
{
	struct spi_nor_info *spi_nor_info;

	spi_nor_info = &params->spi_nor_info;

	printk("name=%s\n", spi_nor_info->name);
	printk("id=0x%x\n", spi_nor_info->id);

	printk("read_standard->cmd=0x%x\n",		spi_nor_info->read_standard.cmd);
	printk("read_standard->dummy=0x%x\n",		spi_nor_info->read_standard.dummy_byte);
	printk("read_standard->addr_nbyte=0x%x\n",	spi_nor_info->read_standard.addr_nbyte);
	printk("read_standard->transfer_mode=0x%x\n",	spi_nor_info->read_standard.transfer_mode);

	printk("read_quad->cmd=0x%x\n",			spi_nor_info->read_quad.cmd);
	printk("read_quad->dummy=0x%x\n",		spi_nor_info->read_quad.dummy_byte);
	printk("read_quad->addr_nbyte=0x%x\n",		spi_nor_info->read_quad.addr_nbyte);
	printk("read_quad->transfer_mode=0x%x\n",	spi_nor_info->read_quad.transfer_mode);

	printk("write_standard->cmd=0x%x\n",		spi_nor_info->write_standard.cmd);
	printk("write_standard->dummy=0x%x\n",		spi_nor_info->write_standard.dummy_byte);
	printk("write_standard->addr_nbyte=0x%x\n",	spi_nor_info->write_standard.addr_nbyte);
	printk("write_standard->transfer_mode=0x%x\n",	spi_nor_info->write_standard.transfer_mode);

	printk("write_quad->cmd=0x%x\n",		spi_nor_info->write_quad.cmd);
	printk("write_quad->dummy=0x%x\n",		spi_nor_info->write_quad.dummy_byte);
	printk("write_quad->addr_nbyte=0x%x\n",		spi_nor_info->write_quad.addr_nbyte);
	printk("write_quad->transfer_mode=0x%x\n",	spi_nor_info->write_quad.transfer_mode);

	printk("sector_erase->cmd=0x%x\n",		spi_nor_info->sector_erase.cmd);
	printk("sector_erase->dummy=0x%x\n",		spi_nor_info->sector_erase.dummy_byte);
	printk("sector_erase->addr_nbyte=0x%x\n",	spi_nor_info->sector_erase.addr_nbyte);
	printk("sector_erase->transfer_mode=0x%x\n",	spi_nor_info->sector_erase.transfer_mode);

	printk("wr_en->cmd=0x%x\n",		spi_nor_info->wr_en.cmd);
	printk("wr_en->dummy=0x%x\n",		spi_nor_info->wr_en.dummy_byte);
	printk("wr_en->addr_nbyte=0x%x\n",	spi_nor_info->wr_en.addr_nbyte);
	printk("wr_en->transfer_mode=0x%x\n",	spi_nor_info->wr_en.transfer_mode);

	printk("en4byte->cmd=0x%x\n",		spi_nor_info->en4byte.cmd);
	printk("en4byte->dummy=0x%x\n",		spi_nor_info->en4byte.dummy_byte);
	printk("en4byte->addr_nbyte=0x%x\n",	spi_nor_info->en4byte.addr_nbyte);
	printk("en4byte->transfer_mode=0x%x\n",	spi_nor_info->en4byte.transfer_mode);

	printk("quad_set->cmd=0x%x\n",		spi_nor_info->quad_set.cmd);
	printk("quad_set->bit_shift=0x%x\n",		spi_nor_info->quad_set.bit_shift);
	printk("quad_set->mask=0x%x\n",		spi_nor_info->quad_set.mask);
	printk("quad_set->val=0x%x\n",		spi_nor_info->quad_set.val);
	printk("quad_set->len=0x%x\n",		spi_nor_info->quad_set.len);
	printk("quad_set->dummy=0x%x\n",	spi_nor_info->quad_set.dummy);

	printk("quad_get->cmd=0x%x\n",		spi_nor_info->quad_get.cmd);
	printk("quad_get->bit_shift=0x%x\n",		spi_nor_info->quad_get.bit_shift);
	printk("quad_get->mask=0x%x\n",		spi_nor_info->quad_get.mask);
	printk("quad_get->val=0x%x\n",		spi_nor_info->quad_get.val);
	printk("quad_get->len=0x%x\n",		spi_nor_info->quad_get.len);
	printk("quad_get->dummy=0x%x\n",	spi_nor_info->quad_get.dummy);

	printk("busy->cmd=0x%x\n",		spi_nor_info->busy.cmd);
	printk("busy->bit_shift=0x%x\n",		spi_nor_info->busy.bit_shift);
	printk("busy->mask=0x%x\n",		spi_nor_info->busy.mask);
	printk("busy->val=0x%x\n",		spi_nor_info->busy.val);
	printk("busy->len=0x%x\n",		spi_nor_info->busy.len);
	printk("busy->dummy=0x%x\n",		spi_nor_info->busy.dummy);

	printk("quad_ops_mode=%d\n",	spi_nor_info->quad_ops_mode);
	printk("addr_ops_mode=%d\n",	spi_nor_info->addr_ops_mode);

	printk("tCHSH=%d\n",	spi_nor_info->tCHSH);
	printk("tSLCH=%d\n",	spi_nor_info->tSLCH);
	printk("tSHSL_RD=%d\n", spi_nor_info->tSHSL_RD);
	printk("tSHSL_WR=%d\n", spi_nor_info->tSHSL_WR);

	printk("chip_size=%d\n",	spi_nor_info->chip_size);
	printk("page_size=%d\n",	spi_nor_info->page_size);
	printk("erase_size=%d\n",	spi_nor_info->erase_size);

	printk("chip_erase_cmd=0x%x\n",	spi_nor_info->chip_erase_cmd);
}
#endif

static int ingenic_spi_norflash_get_params(struct sfc_flash *flash)
{
	int i = 0;
	int32_t err = 0;
	unsigned int chip_id;
	struct spinor_flashinfo *nor_info = flash->flash_info;
	struct ingenic_sfc_info *ingenic_sfc_info = flash->pdata_params;

	burner_params = kzalloc(sizeof(struct burner_params), GFP_KERNEL);
	if (!burner_params) {
		dev_err(flash->dev, "Failed to alloc mem for params\n");
		err = -ENOMEM;
		goto err_params;
	}

	chip_id = sfc_nor_read_id(flash);
	burner_params->spi_nor_info_size = ARRAY_SIZE(spi_nor_info_table);

	for(i = 0; i < burner_params->spi_nor_info_size; i++)
	{
		if(chip_id == spi_nor_info_table[i].id)
		{
			if(spi_nor_info_table[i].addr_len == 4)
			{
				read_standard_table.addr_nbyte = 4;
				read_quad_table.addr_nbyte = 4;
				write_standard_table.addr_nbyte = 4;
				write_quad_table.addr_nbyte = 4;
				sector_erase_table_4k.addr_nbyte =4;
				sector_erase_table_32k.addr_nbyte =4;
				sector_erase_table_64k.addr_nbyte =4;
				set_ib_lock_table.addr_nbyte =4;
				set_ib_unlock_table.addr_nbyte =4;
				rb_lock_table.addr_nbyte =4;
			}
			quad_set_table.cmd = spi_nor_info_table[i].quad_set.cmd;
			quad_set_table.bit_shift = spi_nor_info_table[i].quad_set.bit_shift;
			quad_get_table.cmd = spi_nor_info_table[i].quad_get.cmd;
			quad_get_table.bit_shift = spi_nor_info_table[i].quad_get.bit_shift;
			spi_nor_info_table[i].read_standard		= read_standard_table;
			spi_nor_info_table[i].read_quad			= read_quad_table;
			spi_nor_info_table[i].write_standard	= write_standard_table;
			spi_nor_info_table[i].write_quad		= write_quad_table;
			spi_nor_info_table[i].sector_erase_4k		= sector_erase_table_4k;
			spi_nor_info_table[i].sector_erase_32k	= sector_erase_table_32k;
			spi_nor_info_table[i].sector_erase_64k	= sector_erase_table_64k;
			spi_nor_info_table[i].wr_en				= wr_en_table;
			spi_nor_info_table[i].en4byte			= en4byte_table;
			spi_nor_info_table[i].status_set		= set_status_table;
			spi_nor_info_table[i].status1_set		= set_status1_table;
			spi_nor_info_table[i].status2_set		= set_status2_table;
			spi_nor_info_table[i].set_gb_lock		= set_gb_lock_table;
			spi_nor_info_table[i].set_gb_unlock		= set_gb_unlock_table;
			spi_nor_info_table[i].set_ib_lock		= set_ib_lock_table;
			spi_nor_info_table[i].set_ib_unlock		= set_ib_unlock_table;
			spi_nor_info_table[i].get_rb_lock		= rb_lock_table;
			spi_nor_info_table[i].enotp				= enotp_table;
			spi_nor_info_table[i].exotp				= exotp_table;
			spi_nor_info_table[i].quad_set			= quad_set_table;
			spi_nor_info_table[i].quad_get			= quad_get_table;
			spi_nor_info_table[i].busy				= busy_table;
			burner_params->spi_nor_info				= spi_nor_info_table[i];
			dev_info(flash->dev, "Found Supported nor device, BE = %dK, id = %x, name = %s\n", spi_nor_info_table[i].erase_size/1024, chip_id, spi_nor_info_table[i].name);
			break;
		}
	}

	if(i == burner_params->spi_nor_info_size)
	{
		if((chip_id != 0)&&(chip_id != 0xff))
		{
			printk("ingenic: Unsupported ID %x\n", chip_id);
			return EINVAL;
		}else{
			printk("read or write error! Unsupported ID %x\n", chip_id);
			return EINVAL;
		}
	}

#if 0
	ret = jz_spi_norflash_read_params(flash, SPIFLASH_PARAMER_OFFSET, sizeof(struct burner_params), (uint8_t *)burner_params);
	if (ret) {
		dev_err(flash->dev, "Failed to read params (burned by Burner)\n");
		err = -EINVAL;
		goto err_read_params;
	}
#endif

	//add crc check for params
	nor_info->nor_flash_info = NULL;
	nor_info->nor_flash_info = &burner_params->spi_nor_info;
#if 0
	nor_info->norflash_partitions = &burner_params->norflash_partitions;
	nor_info->nor_pri_data = &burner_params->nor_pri_data;
#endif
	if (ingenic_sfc_info && ingenic_sfc_info->use_board_info) {
		printk("use_board_info is set!!!\n");
		if (ingenic_sfc_info->flash_param)
			nor_info->nor_flash_info = ingenic_sfc_info->flash_param;
		if (ingenic_sfc_info->flash_partition) {
			nor_info->norflash_partitions->num_partition_info = ingenic_sfc_info->num_partition;
			memcpy(nor_info->norflash_partitions->nor_partition, ingenic_sfc_info->flash_partition, sizeof(struct nor_partition) * ingenic_sfc_info->num_partition);
		}
	}

	if ((!nor_info->nor_flash_info)) {
		printk("WARNING : cannot get nor flash params !!!\n");
		err = -EINVAL;
		goto err_read_params;
	}

#ifdef DEBUG_CLONER_PARAMS
	dump_cloner_params(burner_params);
#endif
	return 0;
err_read_params:
	kfree(burner_params);
err_params:
	return err;
}
static ssize_t sfc_nor_partition_offset_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf,"0x%x\n", SPIFLASH_PARAMER_OFFSET + sizeof(int) * 2 + sizeof(struct spi_nor_info));
}

static DEVICE_ATTR(sfc_nor_partition_offset, S_IRUGO | S_IWUSR,
		sfc_nor_partition_offset_show,
		NULL);

static ssize_t sfc_nor_params_offset_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf,"0x%x\n",SPIFLASH_PARAMER_OFFSET);
}

static DEVICE_ATTR(sfc_nor_params_offset, S_IRUGO | S_IWUSR,
		sfc_nor_params_offset_show,
		NULL);

/*add your attr in here*/
static struct attribute *sfc_norflash_info_attributes[] = {
	&dev_attr_sfc_nor_partition_offset.attr,
	&dev_attr_sfc_nor_params_offset.attr,
	NULL
};

static const struct attribute_group sfc_norflash_info_attr_group = {
	.attrs = sfc_norflash_info_attributes
};

/*
 *MK_CMD(cdt, cmd, LINK, ADDRMODE, DATA_EN)
 *MK_ST(cdt, st, LINK, ADDRMODE, ADDR_WIDTH, POLL_EN, DATA_EN, TRAN_MODE)
 */
static void params_to_cdt(struct spi_nor_info *params, struct sfc_cdt *cdt)
{
	/* 4.nor singleRead */
	MK_CMD(cdt[NOR_READ_STANDARD], params->read_standard, 0, ROW_ADDR, ENABLE);

	/* 5.nor quadRead */
	MK_CMD(cdt[NOR_READ_QUAD], params->read_quad, 0, ROW_ADDR, ENABLE);

	/* 6. nor writeStandard */
	MK_CMD(cdt[NOR_WRITE_STANDARD_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_WRITE_STANDARD], params->write_standard, 1, ROW_ADDR, ENABLE);
	MK_ST(cdt[NOR_WRITE_STANDARD_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 7. nor writeQuad */
	MK_CMD(cdt[NOR_WRITE_QUAD_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_WRITE_QUAD], params->write_quad, 1, ROW_ADDR, ENABLE);
	MK_ST(cdt[NOR_WRITE_QUAD_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 8. nor erase */

	MK_CMD(cdt[NOR_ERASE_WRITE_ENABLE_4K], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_ERASE_4K], params->sector_erase_4k, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NOR_ERASE_FINISH_4K], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	MK_CMD(cdt[NOR_ERASE_WRITE_ENABLE_32K], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_ERASE_32K], params->sector_erase_32k, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NOR_ERASE_FINISH_32K], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	MK_CMD(cdt[NOR_ERASE_WRITE_ENABLE_64K], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_ERASE_64K], params->sector_erase_64k, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NOR_ERASE_FINISH_64K], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 9. quad mode */
	if(params->quad_ops_mode){
		if(!params->otp_ops_mode){
			MK_CMD(cdt[NOR_QUAD_SET_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
			MK_ST(cdt[NOR_QUAD_SET], params->quad_set, 1, DEFAULT_ADDRMODE, 0, DISABLE, ENABLE, TM_STD_SPI);  //disable poll, enable data

			MK_ST(cdt[NOR_QUAD_FINISH], params->busy, 1, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

			MK_ST(cdt[NOR_QUAD_GET], params->quad_get, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

		} else {
			MK_CMD(cdt[NOR_ENTER_OTP], params->enotp, 1, DEFAULT_ADDRMODE, DISABLE);

			MK_CMD(cdt[NOR_OTP_QUAD_SET_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
			MK_ST(cdt[NOR_OTP_QUAD_SET], params->quad_set, 1, DEFAULT_ADDRMODE, 0, DISABLE, ENABLE, TM_STD_SPI);  //disable poll, enable data

			MK_ST(cdt[NOR_OTP_QUAD_FINISH], params->busy, 1, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

			MK_ST(cdt[NOR_OTP_QUAD_GET], params->quad_get, 1, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

			MK_CMD(cdt[NOR_EXIT_OTP], params->exotp, 0, DEFAULT_ADDRMODE, DISABLE);
		}
	}

	/* 10. nor write ENABLE */
	MK_CMD(cdt[NOR_WRITE_ENABLE], params->wr_en, 0, DEFAULT_ADDRMODE, DISABLE);

	/* 11. entry 4byte mode */
	MK_CMD(cdt[NOR_EN_4BYTE], params->en4byte, 0, DEFAULT_ADDRMODE, DISABLE);

	/* 13. write status */
	MK_CMD(cdt[NOR_STATUS_SET_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_STATUS_SET], params->status_set, 1, DEFAULT_ADDRMODE, ENABLE);
	MK_ST(cdt[NOR_STATUS_SET_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 14. write status 1*/
	MK_CMD(cdt[NOR_STATUS_1_SET_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_STATUS_1_SET], params->status1_set, 1, DEFAULT_ADDRMODE, ENABLE);
	MK_ST(cdt[NOR_STATUS_1_SET_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 15. write status 2*/
	MK_CMD(cdt[NOR_STATUS_2_SET_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_STATUS_2_SET], params->status2_set, 1, DEFAULT_ADDRMODE, ENABLE);
	MK_ST(cdt[NOR_STATUS_2_SET_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 16. set Global Block Lock */
	MK_CMD(cdt[NOR_GB_LOCK_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_GB_LOCK], params->set_gb_lock, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_ST(cdt[NOR_GB_LOCK_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 17. set Global Block unLock */
	MK_CMD(cdt[NOR_GB_UNLOCK_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_GB_UNLOCK], params->set_gb_unlock, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_ST(cdt[NOR_GB_UNLOCK_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 18. set Individual Block Lock */
	MK_CMD(cdt[NOR_IB_LOCK_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_IB_LOCK], params->set_ib_lock, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NOR_IB_LOCK_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 19. set Individual Block unLock */
	MK_CMD(cdt[NOR_IB_UNLOCK_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_IB_UNLOCK], params->set_ib_unlock, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NOR_IB_UNLOCK_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 20. Read Block Lock status */
	MK_CMD(cdt[NOR_RB_LOCK], params->get_rb_lock, 0, ROW_ADDR, ENABLE);
}

static void nor_create_cdt_table(struct sfc_flash *flash, uint32_t flag)
{
	struct spinor_flashinfo *nor_info = flash->flash_info;
	struct spi_nor_info *nor_flash_info;
	struct sfc_cdt sfc_cdt[INDEX_MAX_NUM];

	memset(sfc_cdt, 0, sizeof(sfc_cdt));

	/* 1.nor reset */
	sfc_cdt[NOR_RESET_ENABLE].link = CMD_LINK(1, DEFAULT_ADDRMODE, TM_STD_SPI);
	sfc_cdt[NOR_RESET_ENABLE].xfer = CMD_XFER(0, DISABLE, 0, DISABLE, SPINOR_OP_RSTEN);
	sfc_cdt[NOR_RESET_ENABLE].staExp = 0;
	sfc_cdt[NOR_RESET_ENABLE].staMsk = 0;

	sfc_cdt[NOR_RESET].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	sfc_cdt[NOR_RESET].xfer = CMD_XFER(0, DISABLE, 0, DISABLE, SPINOR_OP_RST);
	sfc_cdt[NOR_RESET].staExp = 0;
	sfc_cdt[NOR_RESET].staMsk = 0;


	/* 2.nor read id */
	sfc_cdt[NOR_READ_ID].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	sfc_cdt[NOR_READ_ID].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDID);
	sfc_cdt[NOR_READ_ID].staExp = 0;
	sfc_cdt[NOR_READ_ID].staMsk = 0;


	/* 3. nor get status */
	sfc_cdt[NOR_GET_STATUS].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	sfc_cdt[NOR_GET_STATUS].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDSR);
	sfc_cdt[NOR_GET_STATUS].staExp = 0;
	sfc_cdt[NOR_GET_STATUS].staMsk = 0;

	sfc_cdt[NOR_GET_STATUS_1].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	sfc_cdt[NOR_GET_STATUS_1].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDSR_1);
	sfc_cdt[NOR_GET_STATUS_1].staExp = 0;
	sfc_cdt[NOR_GET_STATUS_1].staMsk = 0;

	sfc_cdt[NOR_GET_STATUS_2].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	sfc_cdt[NOR_GET_STATUS_2].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDSR_2);
	sfc_cdt[NOR_GET_STATUS_2].staExp = 0;
	sfc_cdt[NOR_GET_STATUS_2].staMsk = 0;

	if(flag == DEFAULT_CDT){
		/* 4.nor singleRead */
		sfc_cdt[NOR_READ_STANDARD].link = CMD_LINK(0, ROW_ADDR, TM_STD_SPI);
		sfc_cdt[NOR_READ_STANDARD].xfer = CMD_XFER(DEFAULT_ADDRSIZE, DISABLE, 0, ENABLE, SPINOR_OP_READ);
		sfc_cdt[NOR_READ_STANDARD].staExp = 0;
		sfc_cdt[NOR_READ_STANDARD].staMsk = 0;

		/* first create cdt table */
		write_cdt(flash->sfc, sfc_cdt, NOR_RESET_ENABLE, NOR_READ_STANDARD);
	}


	if(flag == UPDATE_CDT){
		nor_flash_info = nor_info->nor_flash_info;
		params_to_cdt(nor_flash_info, sfc_cdt);

		/* second create cdt table */
		write_cdt(flash->sfc, sfc_cdt, NOR_READ_STANDARD, NOR_MAX_INDEX - 1);
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

#ifdef CONFIG_ZERATUL
static struct mtd_erase_region_info erase_regions[] = {
	/* uboot blocks */
	{
		.offset	 = 0,
		.erasesize	= ERASE_SIZE,
		.numblocks	= 0,
	},

	/* tag blocks */
	{
		.offset	 = CONFIG_TAG_OFFSET,
		.erasesize	= CONFIG_ERASE_SIZE_4K,
		.numblocks	= 0,
	},

	/* main block */
	{
		.offset	 = CONFIG_KERNEL_OFFSET,
		.erasesize	= ERASE_SIZE,
		.numblocks	= 0,
	}
};
#endif

static int ingenic_sfc_open(struct inode *inode, struct file *filp)
{
	unsigned int major = imajor(inode);
	unsigned int minor = iminor(inode);

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
	unsigned char w_cmd[6] = {0x05, 0x35, 0x15, 0x01, 0x31, 0x11};
	unsigned char sr[3] = {0};
	static unsigned char *data_buf;
	struct sfc_transfer trans_config;
	struct spi_nor_lock_info spi_nor_lock;
	unsigned char tmp, ret, i;

	if(_IOC_TYPE(cmd) != SFC_IOTC_CHECK) {
		printk("ioctl cmd error !\n");
		return -EINVAL;
	}

	switch(cmd) {
	case SFC_READ_SR:
	case SFC_READ_SR1:
	case SFC_READ_SR2:
		mutex_lock(&flash->lock);
		for(i=0;i<3;i++) {
			sfc_nor_read_status(flash,w_cmd[i],&sr[i],1);
		}
		printk(" nor sr0 = 0x%02x, sr1 = 0x%02x, sr2 = 0x%02x\n", sr[0],sr[1],sr[2]);
		mutex_unlock(&flash->lock);
		ret = copy_to_user((unsigned char __user *)arg, \
			&sr,sizeof(sr));
		break;
	case SFC_WRITE_SR:
		get_user(tmp, (uint32_t *)arg);
		mutex_lock(&flash->lock);
		ret = sfc_nor_write_status(flash,w_cmd[3],&tmp,1);
		if(ret < 0) {
			printk("write status error ! %s %s %d\n",__FILE__,__func__,__LINE__);
			return -EINVAL;
		}
		mutex_unlock(&flash->lock);
		break;
	case SFC_WRITE_SR1:
		get_user(tmp, (uint32_t *)arg);
		mutex_lock(&flash->lock);
		ret = sfc_nor_write_status(flash,w_cmd[4],&tmp,1);
		if(ret < 0) {
			printk("write status 1 error ! %s %s %d\n",__FILE__,__func__,__LINE__);
			return -EINVAL;
		}
		mutex_unlock(&flash->lock);
		break;
	case SFC_WRITE_SR2:
		get_user(tmp, (uint32_t *)arg);
		mutex_lock(&flash->lock);
		ret = sfc_nor_write_status(flash,w_cmd[5],&tmp,1);
		if(ret < 0) {
			printk("write status 2 error ! %s %s %d\n",__FILE__,__func__,__LINE__);
			return -EINVAL;
		}
		mutex_unlock(&flash->lock);
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
				mutex_lock(&flash->lock);
				trans_config.retlen = sfc_read(flash, trans_config.addr, trans_config.len, data_buf);
				mutex_unlock(&flash->lock);
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
	case SFC_SET_BLOCK_LOCK:
		ret = copy_from_user(&spi_nor_lock, \
			(struct spi_nor_lock_info __user *)arg,sizeof(spi_nor_lock));
		if(ret)
			return -EINVAL;

		ret = ingenic_spi_norflash_lock(flash, spi_nor_lock.addr, spi_nor_lock.len, spi_nor_lock.flag);
		if(ret < 0) {
			printk("set lock error! %s %s %d\n",__FILE__,__func__,__LINE__);
			return -EINVAL;
		}
		break;
	case SFC_READ_BLOCK_LOCK_STATUS:
		ret = copy_from_user(&spi_nor_lock, \
			(struct spi_nor_lock_info __user *)arg,sizeof(spi_nor_lock));
		if(ret)
			return -EINVAL;

		mutex_lock(&flash->lock);
		ret = ingenic_spi_norflash_read_lock_status(flash, spi_nor_lock.addr,&spi_nor_lock.status);
		if(ret < 0) {
			printk("set lock error! %s %s %d\n",__FILE__,__func__,__LINE__);
			mutex_unlock(&flash->lock);
			return -EINVAL;
		}
		mutex_unlock(&flash->lock);

		ret = copy_to_user((struct spi_nor_lock_info __user *)arg, \
			&spi_nor_lock,sizeof(spi_nor_lock));
		if(ret)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#define LOCK_PART_OFFSET 0x0
#define LOCK_PART_SIZE 0x1000000
static int ingenic_sfc_nor_lock(struct sfc_flash *flash)
{
	int ret = 0;
	uint8_t value = 0;

	mutex_lock(&flash->lock);
	//set falsh wps
	sfc_nor_read_status(flash,SPINOR_OP_RDSR_2,&value,1);
	if(!(value & SR3_WPS))
	{
		value |= SR3_WPS;
		ret = sfc_nor_write_status(flash,SPINOR_OP_WRSR_2,&value,1);
		if(ret < 0) {
			printk("write status3 error ! %s %s %d\n",__FILE__,__func__,__LINE__);
			mutex_unlock(&flash->lock);
			return -EINVAL;
		}
	}
	mutex_unlock(&flash->lock);
	if(flash->mtd.sfc_index == 0) {
		//flash0 unlock part
		ret = ingenic_spi_norflash_lock(flash,LOCK_PART_OFFSET,LOCK_PART_SIZE,0);
		if(ret < 0) {
				printk("set lock error! %s %s %d\n",__FILE__,__func__,__LINE__);
				return -EINVAL;
		}
	} else {
		//flash1 unlock part
		ret = ingenic_spi_norflash_lock(flash,LOCK_PART_OFFSET,LOCK_PART_SIZE,0);
		if(ret < 0) {
				printk("set lock error! %s %s %d\n",__FILE__,__func__,__LINE__);
				return -EINVAL;
		}
	}
	return 0;
}

static const struct file_operations ingenic_sfc_ops = {
       .owner = THIS_MODULE,
       .open = ingenic_sfc_open,
       .release = ingenic_sfc_release,
       .unlocked_ioctl = ingenic_sfc_ioctl,
};

int ingenic_sfc_nor_probe(struct sfc_flash *flash)
{
	int err = 0,ret = 0,sfc_index = -1;
	struct spinor_flashinfo *nor_info;
	dev_t devid;
	int tchsh;
	int tslch;
	int tshsl_rd;
	int tshsl_wr;
	char mtdname[16],sfcdev_name[16];

	nor_info = kzalloc(sizeof(*nor_info), GFP_KERNEL);
	if(!nor_info) {
		dev_err(flash->dev, "alloc nor_info failed!\n");
		return -ENOMEM;
	}
	flash->flash_info = nor_info;

	set_flash_timing(flash->sfc, DEF_TCHSH, DEF_TSLCH, DEF_TSHSL_R, DEF_TSHSL_W);

	/* request DMA Descriptor space */
	ret = request_sfc_desc(flash);
	if(ret){
		dev_err(flash->dev, "Failure to request DMA descriptor space!\n");
		ret = -ENOMEM;
		goto err_sfc_desc_request;
	}

	/* try creating default CDT table */
	flash->create_cdt_table = nor_create_cdt_table;
	flash->create_cdt_table(flash, DEFAULT_CDT);

	/* Reset nor flash once in the kernel, off by default */
#ifdef CONFIG_JZ_SFC_NOR_RESET
	pr_debug("Reset the nor flash\n");
	ret = sfc_nor_reset(flash);
	if(ret) {
		dev_warn(flash->dev, "Failed to reset nor flash, Try to go on\n");
	}
#endif

	ret = ingenic_spi_norflash_get_params(flash);
	if (ret) {
		ret = -ENODEV;
		dev_err(flash->dev, "Failed to match correct nor flash device!\n");
		goto err_match_device;
	}

	/* Update to private CDT table */
	flash->create_cdt_table(flash, UPDATE_CDT);

	sfc_index  = flash->id;
	sprintf(mtdname, "sfc%d_nor", sfc_index );
	flash->mtd.name     = mtdname;
	flash->mtd.sfc_index= sfc_index ;
	flash->mtd.owner    = THIS_MODULE;
	flash->mtd.type     = MTD_NORFLASH;
	flash->mtd.flags    = MTD_CAP_NORFLASH;
	flash->mtd.erasesize    = nor_info->nor_flash_info->erase_size;
	flash->mtd.writesize    = nor_info->nor_flash_info->page_size;
	flash->mtd.size     = nor_info->nor_flash_info->chip_size;
	flash->mtd._erase   = ingenic_spi_norflash_erase;
	flash->mtd._read    = ingenic_spi_norflash_read;
	flash->mtd._write   = ingenic_spi_norflash_write;
#ifdef CONFIG_ZERATUL
	flash->mtd.numeraseregions = ARRAY_SIZE(erase_regions);
	flash->mtd.eraseregions = erase_regions;
#endif

	tchsh = nor_info->nor_flash_info->tCHSH;
	tslch = nor_info->nor_flash_info->tSLCH;
	tshsl_rd = nor_info->nor_flash_info->tSHSL_RD;
	tshsl_wr = nor_info->nor_flash_info->tSHSL_WR;
	set_flash_timing(flash->sfc, tchsh, tslch, tshsl_rd, tshsl_wr);

	sfc_nor_get_special_ops(flash);

#ifdef CONFIG_SPI_STANDARD_MODE
	nor_info->quad_succeed = 0;
	pr_debug("nor flash now use standard mode!\n");
#else
	ret = nor_info->nor_flash_ops->set_quad_mode(flash);
	if (ret < 0) {
		nor_info->quad_succeed = 0;
		pr_debug("set quad mode error !\n");
	} else {
		nor_info->quad_succeed = 1;
		pr_debug("nor flash quad mode is set, now use quad mode!\n");
	}
#endif

	/* if nor flash size is greater than 16M, use 4byte mode */
	if(flash->mtd.size > NOR_SIZE_16M) {
		if (nor_info->nor_flash_ops->set_4byte_mode) {
			nor_info->nor_flash_ops->set_4byte_mode(flash);
		}
		pr_debug("nor flash 4bytes mode is set, now use 4bytes mode!\n");
	} else {
		pr_debug("nor flash now use 3bytes mode!\n");
	}

	ret = mtd_device_parse_register(&flash->mtd, NULL, NULL, NULL, 0);
	if (ret) {
		ret = -ENODEV;
		dev_err(flash->dev, "Failed to parse register!\n");
		goto err_parse_register;
	}

	flash->attr_group = &sfc_norflash_info_attr_group;
	ret = sysfs_create_group(&flash->dev->kobj, flash->attr_group);
	if (err) {
		dev_err(flash->dev, "failed to register sysfs\n");
		ret = -EIO;
		goto err_create_group;
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
		goto err_create_group;
	}

	flash->device = device_create(flash->class, NULL, devid, NULL, sfcdev_name);
	if (IS_ERR(flash->device)){
		class_destroy(flash->class);
		unregister_chrdev(flash->major,sfcdev_name);
		ret = -EBUSY;
		goto err_create_group;
	}

	pr_debug("SPI NOR MTD LOAD OK\n");
	return 0;

err_create_group:
	mtd_device_unregister(&flash->mtd);
err_parse_register:
	kfree(burner_params);
err_match_device:
err_sfc_desc_request:
	dma_free_coherent(flash->dev, sizeof(struct sfc_desc) * DESC_MAX_NUM, flash->sfc->desc, flash->sfc->desc_pyaddr);
	kfree(nor_info);
	return ret;

}

int ingenic_sfc_nor_suspend(struct sfc_flash *flash)
{
	return 0;
}

int ingenic_sfc_nor_resume(struct sfc_flash *flash)
{
	int ret = 0;
	struct spinor_flashinfo *nor_info = flash->flash_info;
	int tchsh,tslch,tshsl_rd,tshsl_wr;

	set_flash_timing(flash->sfc, DEF_TCHSH, DEF_TSLCH, DEF_TSHSL_R, DEF_TSHSL_W);

	flash->create_cdt_table(flash, DEFAULT_CDT);
#ifdef CONFIG_JZ_SFC_NOR_RESET
	ret = sfc_nor_reset(flash);
	if(ret) {
		dev_warn(flash->dev, "Failed to reset nor flash, Try to go on\n");
	}
#endif

	flash->create_cdt_table(flash, UPDATE_CDT);

	tchsh = nor_info->nor_flash_info->tCHSH;
	tslch = nor_info->nor_flash_info->tSLCH;
	tshsl_rd = nor_info->nor_flash_info->tSHSL_RD;
	tshsl_wr = nor_info->nor_flash_info->tSHSL_WR;
	set_flash_timing(flash->sfc, tchsh, tslch, tshsl_rd, tshsl_wr);

#ifdef CONFIG_SPI_STANDARD_MODE
	nor_info->quad_succeed = 0;
	pr_debug("nor flash now use standard mode!\n");
#else
	ret = nor_info->nor_flash_ops->set_quad_mode(flash);
	if (ret < 0) {
		nor_info->quad_succeed = 0;
		pr_debug("set quad mode error !\n");
	} else {
		nor_info->quad_succeed = 1;
		pr_debug("nor flash quad mode is set, now use quad mode!\n");
	}
#endif

	/* if nor flash size is greater than 16M, use 4byte mode */
	if(flash->mtd.size > NOR_SIZE_16M) {
		if (nor_info->nor_flash_ops->set_4byte_mode) {
			nor_info->nor_flash_ops->set_4byte_mode(flash);
		}
	}
	return 0;
}
