/*********************************************************
 * File Name   : flash_operate.c
 * Author      : Jasmine
 * Mail        : jian.dong@ingenic.com
 * Created Time: 2021-09-13 17:12
 ********************************************************/

#include <linux/kernel.h>
#include "usb_update.h"

struct erase_info {
	struct mtd_info *mtd;
	uint64_t addr;
	uint64_t len;
	uint64_t fail_addr;
	u_long time;
	u_long retries;
	unsigned dev;
	unsigned cell;
	void (*callback) (struct erase_info *self);
	u_long priv;
	u_char state;
	struct erase_info *next;
};

extern int recovery_norflash_erase(struct erase_info *instr);
extern int recovery_norflash_read(loff_t from, size_t len, unsigned char *buf);
extern int recovery_norflash_write(loff_t to, size_t len, const unsigned char *buf);

int read_flash(void *buf, int offset, int size)
{
	recovery_norflash_read((loff_t)offset, (size_t)size, (unsigned char *)buf);
	return 0;
}

int erase_flash(int offset, int size)
{
	struct erase_info instr;
	instr.addr = (unsigned long long)offset;
	instr.len = (unsigned long long)size;
	recovery_norflash_erase(&instr);
	printk("###########erase_flash addr=%llu,len=%llu#######\n",instr.addr,instr.len);

	return 0;
}

int write_flash(void *buf, int deviation, int size)
{
	int offset = 0;
	offset = deviation;
	recovery_norflash_write((loff_t)offset, (size_t)size, (unsigned char *)buf);
	printk("##########write_flash offset=%d,size=%d############\n",offset,size);
	return 0;
}

