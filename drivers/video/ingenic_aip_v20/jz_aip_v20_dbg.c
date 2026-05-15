/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "jz_aip_v20_dbg.h"

#ifdef CONFIG_AIP_V20_DEBUG
	uint32_t jz_aip_v20_debug = 1;
#else
	uint32_t jz_aip_v20_debug = 0;
#endif


void jz_aip_v20_err(const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);

	vaf.fmt = format;
	vaf.va = &args;

	printk(KERN_ERR "[" JZ_AIP_V20_NAME ":%ps] *ERROR* %pV",
	       __builtin_return_address(0), &vaf);

	va_end(args);
}

void jz_aip_v20_ut_debug_printk(const char *function_name, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);
	vaf.fmt = format;
	vaf.va = &args;

	printk(KERN_DEBUG "[" JZ_AIP_V20_NAME ":%s] %pV", function_name, &vaf);

	va_end(args);
}
