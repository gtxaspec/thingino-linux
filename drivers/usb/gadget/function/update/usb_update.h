/*********************************************************
 * File Name   : usb_update.h
 * Author      : Jasmine
 * Mail        : jian.dong@ingenic.com
 * Created Time: 2021-09-13 16:44
 ********************************************************/

#ifndef __USB_UPDATE_H
#define __USB_UPDATE_H

#include <linux/mtd/direct_norflash.h>

#define NORFALSH_RESET_SIGNATURE		(0xffffffff)
#define UPDATE_SIGNATURE		        (0x55504454)
#define LOG_SIGNATURE		        	(0x55503343)
#define NORFLASH_OFFSET				(16*1024*1024 - 32*1024)
#define LOGDATA_OFFSET				(32*1024 + 4064*1024 + 8192*1024 + 256 * 1024 + 2560*1024)
#define HEAD_INFO_RESERVED			128

/*
 *int read_flash(void *buf, int offset, int size);
 *int erase_flash(int offset, int size);
 *int write_flash(void *buf, int deviation, int size);
 */

int buffer_md5sum_0(char *data_buf, unsigned char md5[16], int size_len);

#endif

