#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <soc/base.h>
#include <soc/gpio.h>

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
#define PXDSLS	   0x134   /* Port DRIVE_STRENGTH pin 0-15 Set Register */
#define PXDSLC     0x138   /* Port DRIVE_STRENGTH pin 0-15 Clear Register */
/*shadow reg*/
#define PXSHD       0x1d0
#define PXSHDS      0X1d4
#define PXSHDC      0X1d8
#define PXUPD       0X1e4

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
	/* only support PX 0-15*/
	else if (func & 0x100)
	{
		int drive_strength = func & 0x3;
		/* GPIO_DRIVE_STRENGTH_2 */
		if (drive_strength == 0) {
			gpio_write(port, PXDSLC, BIT(pins * 2));
			gpio_write(port, PXDSLC, BIT(pins * 2 + 1));
		}
		/* GPIO_DRIVE_STRENGTH_4 */
		if (drive_strength == 1) {
			gpio_write(port, PXDSLS, BIT(pins * 2));
			gpio_write(port, PXDSLC, BIT(pins * 2 + 1));
		}
		/* GPIO_DRIVE_STRENGTH_8 */
		if (drive_strength == 2) {
			gpio_write(port, PXDSLC, BIT(pins * 2));
			gpio_write(port, PXDSLS, BIT(pins * 2 + 1));
		}
		/* GPIO_DRIVE_STRENGTH_12 */
		if (drive_strength == 3) {
			gpio_write(port, PXDSLS, BIT(pins * 2));
			gpio_write(port, PXDSLS, BIT(pins * 2 + 1));
		}
	}
}

unsigned long ingenic_pinctrl_lock(int port);
void ingenic_pinctrl_unlock(int port, unsigned long flags);

int jzgpio_set_func(int port, enum gpio_function func, unsigned long pins)
{
    unsigned long flags;

    if (port < 0 || port > 3) {
        printk(KERN_ERR "gpio: invalid gpio port for PRJ007: %d\n", port);
        return -EINVAL;
    }

    flags = ingenic_pinctrl_lock(port);

    hal_gpio_port_set_func(port, pins, func);

    ingenic_pinctrl_unlock(port, flags);

    return 0;
}
EXPORT_SYMBOL(jzgpio_set_func);
