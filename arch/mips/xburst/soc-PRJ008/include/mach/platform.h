/*
 *  Copyright (C) 2010 Ingenic Semiconductor Inc.
 *
 *   In this file, here are some macro/device/function to
 * to help the board special file to organize resources
 * on the chip.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

/* devio define list */

/*******************************************************************************************************************/
#define UART0_PORTB							\
	{ .name = "uart0-pB", .port = GPIO_PORT_B, .func = GPIO_FUNC_2, .pins = 0xF << 2, }
#define UART0_PORTC							\
	{ .name = "uart0-pC", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 0x3 << 8, }
#define UART1_PORTB							\
	{ .name = "uart1", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 23, }
#define UART2_PORTA							\
	{ .name = "uart2-pA", .port = GPIO_PORT_A, .func = GPIO_FUNC_0, .pins = 0x3 << 21, }
#define UART2_PORTB							\
	{ .name = "uart2-pB", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x3 << 29, }
/*******************************************************************************************************************/

#define MSC0_PORTB							\
	{ .name = "msc0-pB",		.port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = (0x3f<<0), }
#define MSC1_PORTA							\
	{ .name = "msc1-pA",		.port = GPIO_PORT_A, .func = GPIO_FUNC_2, .pins = (0x7<<20 | 0x1<<17 | 0x3<<14), }
#define MSC1_PORTB							\
	{ .name = "msc1-pB",		.port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = (0x3f<<8), }
#define MSC1_PORTC							\
	{ .name = "msc1-pC",		.port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3f<<2), }

/*******************************************************************************************************************/
/*****************************************************************************************************************/

#define SSI0_PORTC							\
	{ .name = "ssi0-pc",	       .port = GPIO_PORT_C, .func = GPIO_FUNC_2, .pins = (0x3f << 2), }
#define SSI0_PORTB							\
	{ .name = "ssi1-pb",	       .port = GPIO_PORT_B, .func = GPIO_FUNC_2, .pins = (0xf << 25), }
#define SSISLV_PORTA                         \
	{ .name = "ssi-slave-pa",    .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = (0x3f << 17), }
#define SSISLV_PORTC							\
	{ .name = "ssi-slave-pc",      .port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = (0x3f << 11), }
#define SSISLV_PORTC_1                         \
	{ .name = "ssi-slave-pc_1",    .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3f << 17), }
/* SFC : the function of hold and reset not use, hardware pull up */
#define SFC0_SINGLE_PORTA							\
	{ .name = "sfc",		.port = GPIO_PORT_A, .func = GPIO_FUNC_0, .pins = (0x33 << 23), }
#define SFC0_PORTA							\
	{ .name = "sfc",		.port = GPIO_PORT_A, .func = GPIO_FUNC_0, .pins = (0x3f << 23), }
#define SFC1_PORTC_1							\
	{ .name = "sfc",		.port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = (0x3f << 2), }
#define SFC1_PORTC_2							\
	{ .name = "sfc",		.port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = (0x3f << 11), }

/*****************************************************************************************************************/

#define I2C0_PORTA							\
	{ .name = "i2c0-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_0, .pins = 0x3 << 12, }
#define I2C1_PORTA							\
	{ .name = "i2c1-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_0, .pins = 0x3 << 14, }
#define I2C1_PORTB							\
	{ .name = "i2c1-pb", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x3 << 0, }
#define I2C1_PORTC							\
	{ .name = "i2c1-pc", .port = GPIO_PORT_C, .func = GPIO_FUNC_3, .pins = 0x3 << 6, }
#define I2C2_PORTB							\
	{ .name = "i2c2-pb", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 25, }
#define I2C2_PORTC							\
	{ .name = "i2c2-pc", .port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = 0x3 << 8, }
#define I2C2_PORTA_1							\
	{ .name = "i2c2-pa-1", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x3 << 10, }
#define I2C2_PORTA_2							\
	{ .name = "i2c2-pa-2", .port = GPIO_PORT_A, .func = GPIO_FUNC_2, .pins = 0x3 << 18, }
/*******************************************************************************************************************/

#define MII_PORTBDF							\
	{ .name = "mii-0", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x00000010, }, \
	{ .name = "mii-1", .port = GPIO_PORT_D, .func = GPIO_FUNC_1, .pins = 0x3c000000, }, \
	{ .name = "mii-2", .port = GPIO_PORT_F, .func = GPIO_FUNC_0, .pins = 0x0000fff0, }

/*******************************************************************************************************************/

#define OTG_DRVVUS							\
	{ .name = "otg-drvvbus", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 1 << 27, }

/*******************************************************************************************************************/

#define DVP_PORTA_LOW_10BIT							\
	{ .name = "dvp-pa-10bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x000343ff, }

#define DVP_PORTA_HIGH_10BIT							\
	{ .name = "dvp-pa-10bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x00034ffc, }

#define DVP_PORTA_12BIT							\
	{ .name = "dvp-pa-12bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x00034fff, }

/*******************************************************************************************************************/

/*******************************************************************************************************************/
#define DPU_PORTB_SLCD_8BIT                                                     \
        { .name = "dpu_slcd_8bit",  .port = GPIO_PORT_B, .func = GPIO_FUNC_3, .pins = (0x7<<25 | 0x3ff<<6)}
#define DPU_PORTC_SLCD_8BIT                                                     \
        { .name = "dpu_slcd_8bit",  .port = GPIO_PORT_C, .func = GPIO_FUNC_2, .pins = (0x1fff<<17)}
/*******************************************************************************************************************/
#define GMAC_PORTB							\
	{ .name = "gmac_pb",	.port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x0001efc0}

/*******************************************************************************************************************/

/* JZ SoC on Chip devices list */
extern struct platform_device jz_adc_device;

#if defined(CONFIG_AVPU) && defined(CONFIG_AVPU_DRIVER)
extern struct platform_device jz_avpu_irq_device;
#else
#ifdef CONFIG_JZ_VPU_IRQ_TEST
extern struct platform_device jz_vpu_irq_device;
#elif defined(CONFIG_SOC_VPU)
#ifdef CONFIG_VPU_HELIX
#if (CONFIG_VPU_HELIX_NUM >= 1)
extern struct platform_device jz_vpu_helix0_device;
#endif
#endif
#ifdef CONFIG_VPU_RADIX
#if (CONFIG_VPU_RADIX_NUM >= 1)
extern struct platform_device jz_vpu_radix0_device;
#endif
#endif
#endif
#endif

extern struct platform_device jz_uart0_device;
extern struct platform_device jz_uart1_device;
extern struct platform_device jz_uart2_device;

extern struct platform_device jz_ssi1_device;
extern struct platform_device jz_ssi0_device;

extern struct platform_device jz_i2c0_device;
extern struct platform_device jz_i2c1_device;
extern struct platform_device jz_i2c2_device;

extern struct platform_device jz_sfc0_device;
extern struct platform_device jz_sfc1_device;
extern struct platform_device jz_msc0_device;
extern struct platform_device jz_msc1_device;
extern struct platform_device jz_ipu_device;
extern struct platform_device jz_i2d_device;
extern struct platform_device jz_pdma_device;

extern struct platform_device jz_dwc_otg_device;

extern struct platform_device jz_mixer0_device;
extern struct platform_device jz_mixer1_device;
extern struct platform_device jz_mixer2_device;
extern struct platform_device jz_mixer3_device;
extern struct platform_device jz_rtc_device;
extern struct platform_device jz_efuse_device;
extern struct platform_device jz_pwm_device;
extern struct platform_device jz_tcu_device;

int jz_device_register(struct platform_device *pdev, void *pdata);
void *get_driver_common_interfaces(void);

#endif
/* __PLATFORM_H__ */
