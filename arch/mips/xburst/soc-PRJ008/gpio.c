#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <soc/base.h>
#include <soc/gpio.h>
#include <linux/gpio.h>
#include <asm-generic/gpio.h>

#define GPIO_PORT_OFF    0x1000

#define PXPIN      0x00   /* PIN Level Register */
#define PXINT      0x10   /* Port Interrupt Register */
#define PXINTS     0x14   /* Port Interrupt Set Register */
#define PXINTC     0x18   /* Port Interrupt Clear Register */
#define PXMSK      0x20   /* Port Interrupt Mask Reg */
#define PXMSKS     0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC     0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1     0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S    0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C    0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0     0x40   /* Port Pattern 0 Register */
#define PXPAT0S    0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C    0x48   /* Port Pattern 0 Clear Register */
#define PXFLG      0x50   /* Port Flag Register */
#define PXFLGC     0x58   /* Port Flag clear Register */
#define PXPU       0x110   /* Port PULL-UP State Register */
#define PXPUS      0x114   /* Port PULL-UP State Set Register */
#define PXPUC      0x118   /* Port PULL-UP State Clear Register */
#define PXPD       0x120   /* Port PULL-DOWN State Register */
#define PXPDS      0x124   /* Port PULL-DOWN State Set Register */
#define PXPDC      0x128   /* Port PULL-DOWN State Clear Register */
#define PXPDRVL		0x130	/* port Drive Strength Low bit Register */
#define PXPDRVLS	0x134	/* port Drive Strength Low bit Set Register */
#define PXPDRVLC	0x138	/* port Drive Strength Low bit Clear Register */
#define PXPDRVH		0x140	/* port Drive Strength High bit Register */
#define PXPDRVHS	0x144	/* port Drive Strength High bit Set Register */
#define PXPDRVHC	0x148	/* port Drive Strength High bit Clear Register */
#define PXPSLW		0x150	/* port Slew Rate Register */
#define PXPSLWS	    0x154	/* port Slew Rate Set Register */
#define PXPSLWC	    0x158	/* port Slew Rate Clear Register */
#define PXPSMT		0x160	/* port Schmitt Trigger Register */
#define PXPSMTS	    0x164	/* port Schmitt Trigger Set Register */
#define PXPSMTC	    0x168	/* port Schmitt Trigger Clear Register */
/*shadow reg*/
#define PXSHD       0x1d0
#define PXSHDS      0X1d4
#define PXSHDC      0X1d8
#define PXUPD       0X1e4

#ifdef CONFIG_INGENIC_GPIO_FUNC_DEBUG
extern struct ingenic_gpio_chip *ingenic_get_gpio_chip(struct gpio_chip *chip);
extern unsigned int ingenic_get_used_pins_bitmap(struct gpio_chip *chip);
extern void ingenic_set_used_pins_bitmap(struct gpio_chip *chip, unsigned int bitmap);
#endif

static const unsigned long gpiobase[] = {
	[0] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 0 * GPIO_PORT_OFF),
	[1] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 1 * GPIO_PORT_OFF),
	[2] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 2 * GPIO_PORT_OFF),
};

#define GPIO_ADDR(port, reg) ((volatile unsigned long *)(gpiobase[port] + reg))

static inline void gpio_write(int port, unsigned int reg, int val)
{
	*GPIO_ADDR(port, reg) = val;
}

static inline unsigned int gpio_read(int port, unsigned int reg)
{
	return *GPIO_ADDR(port, reg);
}

static void hal_gpio_port_set_func(int port, unsigned int pins, enum gpio_function func)
{
#ifdef CONFIG_INGENIC_GPIO_FUNC_DEBUG
	struct gpio_chip *gc;
	unsigned int offset;
	unsigned gpio_base;

	if (!(func & 0x100) && !(func & 0x80)) {
		gpio_base = port * 32;
		gc = gpio_to_chip(gpio_base);
		if (IS_ERR(gc)) {
			pr_err("Cannot find GPIO chip for port %d\n", port);
			return;
		}

		for (offset = 0; offset < 32; offset++) {
			if (!(pins & (1 << offset)))
				continue;
			if (ingenic_get_used_pins_bitmap(gc) & (1 << offset)) {
				printk(KERN_DEBUG "gpio functions has redefinition: port %d pin %d\n", port, offset);
			}
		}
		ingenic_set_used_pins_bitmap(gc, ingenic_get_used_pins_bitmap(gc) | pins);
	}
#endif

	/* func option */
	if((func == GPIO_INT_LO)||(func == GPIO_INT_HI)||(func == GPIO_INT_FE)||(func == GPIO_INT_RE))
	{
		gpio_write(port, PXSHDS, 1);
		switch (func)
		{
		case GPIO_INT_LO : //Low Level trigger interrupt
			gpio_write(port, PXINTS, pins);
			gpio_write(port, PXMSKS, pins);
			gpio_write(port, PXPAT1C, pins);
			gpio_write(port, PXPAT0C, pins);
			break;
		case GPIO_INT_HI : //High Level trigger interrupt
			gpio_write(port, PXINTS, pins);
			gpio_write(port, PXMSKS, pins);
			gpio_write(port, PXPAT1C, pins);
			gpio_write(port, PXPAT0S, pins);
			break;
		case GPIO_INT_FE : //Fall Edge trigger interrupt
			gpio_write(port, PXINTS, pins);
			gpio_write(port, PXMSKS, pins);
			gpio_write(port, PXPAT1S, pins);
			gpio_write(port, PXPAT0C, pins);
			break;
		case GPIO_INT_RE : //Rise Edge trigger interrupt
			gpio_write(port, PXINTS, pins);
			gpio_write(port, PXMSKS, pins);
			gpio_write(port, PXPAT1S, pins);
			gpio_write(port, PXPAT0S, pins);
			break;
		default : break;
		}
		gpio_write(port, PXUPD, 1);
		gpio_write(port, PXSHDC, 1);
	}
	else if((func == GPIO_OUTPUT0)||(func == GPIO_OUTPUT1)||(func == GPIO_INPUT))
	{
		gpio_write(port, PXSHDS, 1);
		switch (func)
		{
		case GPIO_OUTPUT0 :
			gpio_write(port, PXINTC, pins);
			gpio_write(port, PXMSKS, pins);
			gpio_write(port, PXPAT1C, pins);
			gpio_write(port, PXPAT0C, pins);
			break;
		case GPIO_OUTPUT1 :
			gpio_write(port, PXINTC, pins);
			gpio_write(port, PXMSKS, pins);
			gpio_write(port, PXPAT1C, pins);
			gpio_write(port, PXPAT0S, pins);
			break;
		case GPIO_INPUT :
			gpio_write(port, PXINTC, pins);
			gpio_write(port, PXMSKS, pins);
			gpio_write(port, PXPAT1S, pins);
			break;
		default : break;
		}
		gpio_write(port, PXUPD, 1);
		gpio_write(port, PXSHDC, 1);
	}
	else if((func == GPIO_FUNC_0)||(func == GPIO_FUNC_1)||(func == GPIO_FUNC_2)||(func == GPIO_FUNC_3))
	{
		gpio_write(port, PXSHDS, 1);
		switch (func)
		{
		case GPIO_FUNC_0 :
			gpio_write(port, PXINTC, pins);
			gpio_write(port, PXMSKC, pins);
			gpio_write(port, PXPAT1C, pins);
			gpio_write(port, PXPAT0C, pins);
			break;
		case GPIO_FUNC_1 :
			gpio_write(port, PXINTC, pins);
			gpio_write(port, PXMSKC, pins);
			gpio_write(port, PXPAT1C, pins);
			gpio_write(port, PXPAT0S, pins);
			break;
		case GPIO_FUNC_2 :
			gpio_write(port, PXINTC, pins);
			gpio_write(port, PXMSKC, pins);
			gpio_write(port, PXPAT1S, pins);
			gpio_write(port, PXPAT0C, pins);
			break;
		case GPIO_FUNC_3 :
			gpio_write(port, PXINTC, pins);
			gpio_write(port, PXMSKC, pins);
			gpio_write(port, PXPAT1S, pins);
			gpio_write(port, PXPAT0S, pins);
			break;
		default : break;
		}
		gpio_write(port, PXUPD, 1);
		gpio_write(port, PXSHDC, 1);
	}
	else if (func & 0x80)
	{
		int pull = (func >> 5) & 0x3;
		if (pull == 0) { // no pull
			gpio_write(port, PXPUC, pins);
			gpio_write(port, PXPDC, pins);
		}
		if (pull == 1) { // pull up
			gpio_write(port, PXPDC, pins);
			gpio_write(port, PXPUS, pins);
		}
		if (pull == 2) { // pull down
			gpio_write(port, PXPUC, pins);
			gpio_write(port, PXPDS, pins);
		}
	}
	else if (func & 0x100)
	{
		int drive_strength = func;
		switch (drive_strength) {
			case GPIO_DRIVE_STRENGTH_MODE_1:
				gpio_write(port, PXPDRVHC, BIT(pins));
				gpio_write(port, PXPDRVLC, BIT(pins));
				if(port == 0 && (pins >=12 && pins <=22)) {
					pr_debug("now drive_strength use mode 1,typical values is 3.0mA\n");
				}else {
					pr_debug("now drive_strength use mode 1,typical values is 9.8mA\n");
				}
				break;
			case GPIO_DRIVE_STRENGTH_MODE_2:
				gpio_write(port, PXPDRVHC, BIT(pins));
				gpio_write(port, PXPDRVLS, BIT(pins));
				if(port == 0 && (pins >=12 && pins <=22)) {
					pr_debug("now drive_strength use mode 2,typical values is 6.1mA\n");
				}else {
					pr_debug("now drive_strength use mode 2,typical values is 19.7mA\n");
				}
				break;
			case GPIO_DRIVE_STRENGTH_MODE_3:
				gpio_write(port, PXPDRVHS, BIT(pins));
				gpio_write(port, PXPDRVLC, BIT(pins));
				if(port == 0 && (pins >=12 && pins <=22)) {
					pr_debug("now drive_strength use mode 3,typical values is 9.2mA\n");
				}else {
					pr_debug("now drive_strength use mode 3,typical values is 29.7mA\n");
				}
				break;
			case GPIO_DRIVE_STRENGTH_MODE_4:
				gpio_write(port, PXPDRVHS, BIT(pins));
				gpio_write(port, PXPDRVLS, BIT(pins));
				if(port == 0 && (pins >=12 && pins <=22)) {
					pr_debug("now drive_strength use mode 4,typical values is 12.2mA\n");
				}else {
					pr_debug("now drive_strength use mode 4,typical values is 39.5mA\n");
				}
				break;
			default:
				printk("Error: Unknown drive strength mode,so we use the default mode 1\n");
				gpio_write(port, PXPDRVHC, BIT(pins));
				gpio_write(port, PXPDRVLC, BIT(pins));
				break;
		}
	}
}

unsigned long ingenic_pinctrl_lock(int port);
unsigned long ingenic_pinctrl_unlock(int port, unsigned long flags);

int jzgpio_set_func(int port, enum gpio_function func, unsigned long pins)
{
	unsigned long flags;

	if (port < 0 || port > 3) {
        printk(KERN_ERR "gpio: invalid gpio port for PRJ008: %d\n", port);
		return -EINVAL;
	}

	flags = ingenic_pinctrl_lock(port);

	hal_gpio_port_set_func(port, pins, func);

	ingenic_pinctrl_unlock(port, flags);

	return 0;
}
EXPORT_SYMBOL(jzgpio_set_func);
