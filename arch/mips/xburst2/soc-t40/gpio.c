#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <soc/base.h>
#include <soc/gpio.h>

#define GPIO_PORT_OFF    0x1000
#define GPIO_SHADOW_OFF  0x7000

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

//Shadow Register Group PZ  (Reset Value 0x00000000)
#define PZINTS    0x14  //W Shadow Interrupt Set Register
#define PZINTC    0x18  //W Shadow Interrupt Clear Register
#define PZMSKS    0x24  //W Shadow Interrupt Mask Set Register
#define PZMSKC    0x28  //W Shadow Interrupt Mask Clear Register
#define PZPAT1S   0x34  //W Shadow Interrupt pattern 1 Set Register
#define PZPAT1C   0x38  //W Shadow Interrupt pattern 1 Clear Register
#define PZPAT0S   0x44  //W Shadow Interrupt pattern 0 Set Register
#define PZPAT0C   0x48  //W Shadow Interrupt pattern 0 Clear Register
#define PZGID2LD  0xF0  //W PORT Group ID to be load from PZ Register

#define SHADOW 5

static const unsigned long gpiobase[] = {
    [0] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 0 * GPIO_PORT_OFF),
    [1] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 1 * GPIO_PORT_OFF),
    [2] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 2 * GPIO_PORT_OFF),
    [3] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 3 * GPIO_PORT_OFF),
    [4] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 4 * GPIO_PORT_OFF),

    [SHADOW] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + GPIO_SHADOW_OFF),
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
    if ((func == GPIO_INT_LO) || (func == GPIO_INT_HI) || (func == GPIO_INT_FE) || (func == GPIO_INT_RE))
    {
        gpio_write(port, PZINTS, pins);
        gpio_write(port, PZMSKC, pins);
        switch (func)
        {
        case GPIO_INT_LO: // Low Level trigger interrupt
            gpio_write(port, PZPAT1C, pins);
            gpio_write(port, PZPAT0C, pins);
            break;
        case GPIO_INT_HI: // High Level trigger interrupt
            gpio_write(port, PZPAT1C, pins);
            gpio_write(port, PZPAT0S, pins);
            break;
        case GPIO_INT_FE: // Fall Edge trigger interrupt
            gpio_write(port, PZPAT1S, pins);
            gpio_write(port, PZPAT0C, pins);
            break;
        case GPIO_INT_RE: // Rise Edge trigger interrupt
            gpio_write(port, PZPAT1S, pins);
            gpio_write(port, PZPAT0S, pins);
            break;
        default:
            break;
        }
    }
    else
    {
        gpio_write(port, PZINTC, pins);
        if ((func == GPIO_OUTPUT0) || (func == GPIO_OUTPUT1) || (func == GPIO_INPUT))
        {
            gpio_write(port, PZMSKS, pins);
            switch (func)
            {
            case GPIO_OUTPUT0:
                gpio_write(port, PZPAT1C, pins);
                gpio_write(port, PZPAT0C, pins);
                break;
            case GPIO_OUTPUT1:
                gpio_write(port, PZPAT1C, pins);
                gpio_write(port, PZPAT0S, pins);
                break;
            case GPIO_INPUT:
                gpio_write(port, PZPAT1S, pins);
                break;
            default:
                break;
            }
        }
        else if ((func == GPIO_FUNC_0) || (func == GPIO_FUNC_1) || (func == GPIO_FUNC_2) || (func == GPIO_FUNC_3))
        {
            gpio_write(port, PZMSKC, pins);
            switch (func)
            {
            case GPIO_FUNC_0:
                gpio_write(port, PZPAT1C, pins);
                gpio_write(port, PZPAT0C, pins);
                break;
            case GPIO_FUNC_1:
                gpio_write(port, PZPAT1C, pins);
                gpio_write(port, PZPAT0S, pins);
                break;
            case GPIO_FUNC_2:
                gpio_write(port, PZPAT1S, pins);
                gpio_write(port, PZPAT0C, pins);
                break;
            case GPIO_FUNC_3:
                gpio_write(port, PZPAT1S, pins);
                gpio_write(port, PZPAT0S, pins);
                break;
            default:
                break;
            }
        }
    }
    /* configure PzGID2LD to specify which port group to load */
    gpio_write(SHADOW, PZGID2LD, port);
}

unsigned long ingenic_pinctrl_lock(int port);
void ingenic_pinctrl_unlock(int port, unsigned long flags);

int jzgpio_set_func(int port, enum gpio_function func, unsigned long pins)
{
    unsigned long flags;

    if (port < 0 || port > 4) {
        printk(KERN_ERR "gpio: invalid gpio port for T40: %d\n", port);
        return -EINVAL;
    }

    flags = ingenic_pinctrl_lock(port);

    hal_gpio_port_set_func(port, pins, func);

    ingenic_pinctrl_unlock(port, flags);

    return 0;
}
EXPORT_SYMBOL(jzgpio_set_func);
