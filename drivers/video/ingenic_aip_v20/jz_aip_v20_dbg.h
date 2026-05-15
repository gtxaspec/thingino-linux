/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#ifndef _JZ_AIP_V20_DBG_H_
#define _JZ_AIP_V20_DBG_H_

extern uint32_t jz_aip_v20_debug;

extern __printf(2, 3)
void jz_aip_v20_ut_debug_printk(const char *function_name,
				const char *format, ...);
extern __printf(1, 2)
void jz_aip_v20_err(const char *format, ...);

#define JZ_AIP_V20_NAME "AIP2.0"

#define JZ_AIP_V20_INFO(fmt, ...)                              \
	printk(KERN_INFO "[" JZ_AIP_V20_NAME "] " fmt, ##__VA_ARGS__)

/**
 * Debug output.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */
#define JZ_AIP_V20_DEBUG(fmt, args...)                                     \
        do {                                                               \
                if (unlikely(jz_aip_v20_debug))                            \
                        jz_aip_v20_ut_debug_printk(__func__, fmt, ##args); \
        } while (0)

/**
 * Error output.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */
#define JZ_AIP_V20_ERROR(fmt, ...)                             \
        jz_aip_v20_err(fmt, ##__VA_ARGS__)

#endif
