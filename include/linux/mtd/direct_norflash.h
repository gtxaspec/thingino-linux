/*********************************************************
 * File Name   : usb_update.h
 * Author      : Jasmine
 * Mail        : jian.dong@ingenic.com
 * Created Time: 2021-09-13 16:44
 ********************************************************/

#ifndef __MTD_DIRECT_NORFLASH_H
#define __MTD_DIRECT_NORFLASH_H

#ifdef CONFIG_SFC_NOR_INTERFACE_USE
int direct_read_norflash(void *buf, int offset, int size);
int direct_erase_norflash(int offset, int size);
int direct_write_norflash(void *buf, int deviation, int size);
#endif


#endif

