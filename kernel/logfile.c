/**
    NVT logfile function
    This file will handle NVT logfile function
    @file logfile.c
    @ingroup
    @note
    Copyright Novatek Microelectronics Corp. 2021. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/kexec.h>
#include <linux/profile.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/capability.h>
#include <linux/compiler.h>
#include <linux/uaccess.h>
#include <mach/nvt-io.h>
#include <linux/dma-buf.h>
#include <linux/sched/clock.h>
#include <plat/hardware.h>
#include <linux/module.h>

extern struct dma_buf *logfile_dmabuf;

#define MAKEFOURCC(ch0, ch1, ch2, ch3)	((u32)(u8)(ch0) | ((u32)(u8)(ch1) << 8) | ((u32)(u8)(ch2) << 16) | ((u32)(u8)(ch3) << 24 ))
#define LOGFILE_INTERFACE_VER			0x19112808
#define LOGFILE_SYS_ERROR_KEY			MAKEFOURCC('S','Y','S','E')
#define PRINT_CTRL_CHAR_LEN				7
#define NVT_TIMER_TM0_CNT				0x108

enum log_flags {
	LOG_ROLLBACK = 1,	/* log already rollback */
};

typedef struct {
	unsigned int        InterfaceVer;      ///< the Interface version of logfile ring buffer
	unsigned int        BufferStartAddr;   ///< the buffer start address for store log msg
	unsigned int        BufferSize;        ///< the total buffer size for store log msg
	unsigned int        DataIn;            ///< the log msg input offset
	unsigned int        DataOut;           ///< the log msg output offset
	unsigned int        BufferMapAddr;     ///< the ioremap buffer start address for store log msg
	unsigned int        SysErr;            ///< the kernel panic or some system error
	unsigned int        flags;             ///< the log flags
	unsigned int        reserved[8];       ///< reserved
} LOGFILE_RINGBUF_HEAD;

static LOGFILE_RINGBUF_HEAD *g_logfile_buf = NULL;
static u64 g_timer0_offset = 0;

static int logfile_init(void)
{
	unsigned long buf_addr, buf_size;

	if (!logfile_dmabuf)
		return -1;

	/*
	 * Get buffer information from shared buffer object and
	 * map a page of the buffer object into kernel address space.
	 */
	buf_size = (unsigned long)logfile_dmabuf->size;
	buf_addr = (unsigned long)dma_buf_kmap(logfile_dmabuf, buf_size/PAGE_SIZE);
	g_logfile_buf = (LOGFILE_RINGBUF_HEAD *)buf_addr;
	g_logfile_buf->BufferMapAddr = (unsigned int)g_logfile_buf + sizeof(LOGFILE_RINGBUF_HEAD);

	/*
	 * Add interface version check and init log msg input/output offset
	 * here to store log msg before userspace application ready.
	 */
	g_logfile_buf->InterfaceVer = LOGFILE_INTERFACE_VER;
	g_logfile_buf->BufferSize = buf_size - sizeof(LOGFILE_RINGBUF_HEAD);
	g_logfile_buf->DataIn = 0;
	g_logfile_buf->DataOut = 0;

	/* Read nvt timer0 for log time string. */
	g_timer0_offset = nvt_readl(NVT_TIMER_BASE_VIRT + NVT_TIMER_TM0_CNT);
	g_timer0_offset *= 1000;

	return 0;
}

static size_t logfile_print_time(u64 ts, char *buf)
{
	unsigned long rem_nsec;

	rem_nsec = do_div(ts, 1000000000);
	return sprintf(buf, "[%5lu.%06lu] ",
		       (unsigned long)ts, rem_nsec / 1000);
}

static int logfile_skip_head(const char *s)
{
	char *in = (char*)s;

	if (strncmp(s,"\033[0;3",5) == 0 || strncmp(s,"\033[1;3",5) == 0) {
		in+= 6;
		if (*in == 'm')
			return 7;
	}
	return 0;
}

void logfile_save_str(const char *s, size_t count, int kernel_space)
{
	const char          *instr;
	char                timeStr[40];
	unsigned int        timeLen = 0, strLen;
	unsigned int        DataOut, DataIn;
	unsigned int        instrLen, remainLen, DataSize;
	LOGFILE_RINGBUF_HEAD *pbuf;
	u64                 ts_nsec;
	int                 isAddTimeStrNext = 0;
	static int          isAddTimeStr = 1;
	char                temp_start_str[PRINT_CTRL_CHAR_LEN];
	char                temp_end_str[1];
	int                 skip_len;

	if (g_logfile_buf == NULL)
		if (logfile_init() < 0)
			return;

	pbuf = g_logfile_buf;

	/* Check interface version. */
	if (pbuf->InterfaceVer != LOGFILE_INTERFACE_VER)
		return;

	ts_nsec = local_clock() + g_timer0_offset;

	DataOut = pbuf->DataOut;
	DataIn = pbuf->DataIn;
	if (DataIn >= DataOut)
		DataSize = DataIn - DataOut;
	else
		DataSize = pbuf->BufferSize + DataIn - DataOut;

	instrLen = count;
	instr = s;
	if (count >= PRINT_CTRL_CHAR_LEN) {
		if (kernel_space == 1)
			memcpy((void *)temp_start_str, instr, PRINT_CTRL_CHAR_LEN);
		else
			copy_from_user((void *)temp_start_str, instr, PRINT_CTRL_CHAR_LEN);

		skip_len = logfile_skip_head(temp_start_str);
		instrLen -= skip_len;
		instr += skip_len;
	}

	if (instrLen < 1 || instrLen > pbuf->BufferSize)
		return;

	if (kernel_space == 1)
		memcpy((void *)temp_end_str, instr+instrLen-1, 1);
	else
		copy_from_user((void *)temp_end_str, instr+instrLen-1, 1);

	/* Check new line and need to add time string at next line. */
	if (temp_end_str[0] == '\n')
		isAddTimeStrNext = 1;

	/* Add log time string. */
	if (isAddTimeStr) {
		logfile_print_time(ts_nsec, timeStr);
		timeLen = strlen(timeStr);
	}

	strLen = instrLen + timeLen;
	if (DataIn + strLen < pbuf->BufferSize) {
		if (isAddTimeStr) {
			memcpy((void *)(DataIn + pbuf->BufferMapAddr), timeStr, timeLen);
			if (kernel_space == 1)
				memcpy((void *)(DataIn + pbuf->BufferMapAddr + timeLen), instr, strLen - timeLen);
			else
				copy_from_user((void *)(DataIn + pbuf->BufferMapAddr + timeLen), instr, strLen - timeLen);
		} else {
			if (kernel_space == 1)
				memcpy((void *)(DataIn + pbuf->BufferMapAddr), instr, strLen);
			else
				copy_from_user((void *)(DataIn + pbuf->BufferMapAddr), instr, strLen);
		}
		DataIn += strLen;
	} else {
		remainLen = pbuf->BufferSize - DataIn;
		if (isAddTimeStr) {
			if (remainLen >= timeLen) {
				memcpy((void *)(DataIn + pbuf->BufferMapAddr), timeStr, timeLen);
				DataIn += timeLen;
				remainLen -= timeLen;
				if (kernel_space == 1)
					memcpy((void *)DataIn + pbuf->BufferMapAddr, instr, remainLen);
				else
					copy_from_user((void *)DataIn + pbuf->BufferMapAddr, instr, remainLen);
				/* Rollback log. */
				DataIn = 0;
				if (kernel_space == 1)
					memcpy((void *)DataIn + pbuf->BufferMapAddr, instr+remainLen, instrLen - remainLen);
				else
					copy_from_user((void *)DataIn + pbuf->BufferMapAddr, instr+remainLen, instrLen - remainLen);
				DataIn += (instrLen - remainLen);
			} else {
				memcpy((void *)(DataIn + pbuf->BufferMapAddr), timeStr, remainLen);
				/* Rollback log. */
				DataIn = 0;
				memcpy((void *)DataIn + pbuf->BufferMapAddr, timeStr+remainLen, timeLen - remainLen);
				DataIn += (timeLen - remainLen);
				if (kernel_space == 1)
					memcpy((void *)(DataIn + pbuf->BufferMapAddr + timeLen), instr, strLen - timeLen);
				else
					copy_from_user((void *)(DataIn + pbuf->BufferMapAddr + timeLen), instr, strLen - timeLen);
				DataIn += instrLen;
			}
		} else {
			if (kernel_space == 1)
				memcpy((void *)DataIn + pbuf->BufferMapAddr, instr, remainLen);
			else
				copy_from_user((void *)DataIn + pbuf->BufferMapAddr, instr, remainLen);
			/* Rollback log. */
			DataIn = 0;
			if (kernel_space == 1)
				memcpy((void *)DataIn + pbuf->BufferMapAddr, instr+remainLen, instrLen - remainLen);
			else
				copy_from_user((void *)DataIn + pbuf->BufferMapAddr, instr+remainLen, instrLen - remainLen);
			DataIn += (instrLen - remainLen);
		}
		pbuf->flags |= LOG_ROLLBACK;
	}
	pbuf->DataIn = DataIn;

	isAddTimeStr = isAddTimeStrNext;
}


void logfile_save_syserr(void)
{
	LOGFILE_RINGBUF_HEAD *pbuf;

	if (g_logfile_buf == NULL)
		if (logfile_init() < 0)
			return;

	pbuf = g_logfile_buf;

	/* Add sys error key. */
	pbuf->SysErr = LOGFILE_SYS_ERROR_KEY;
}

MODULE_AUTHOR("Novatek Microelectronics Corp.");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("logfile function for NVT");
MODULE_VERSION("1.00.000");