#define DEBUG
#include <linux/kernel.h>
#include <gpio.h>
#include <soc/gpio.h>

typedef struct gpio_pull_table {
	int gpio_num;
	enum gpio_function pull_status;
} gpio_pull_table_t;

typedef struct gpio_drive_strength_table {
	int gpio_num;
	gpio_drv_level_t drv_level;
} gpio_drive_strength_table_t;

static gpio_pull_table_t soc_gpio_pull_table[] = {
	{ GPIO_PB(28), GPIO_PULL_UP }, //TF cd
};

static gpio_drive_strength_table_t soc_gpio_drive_strength_table[] = {
	/* TF Card */
	{ GPIO_PB(0), DS_1_MODE }, //MSC0_D0
	{ GPIO_PB(1), DS_1_MODE }, //MSC0_D1
	{ GPIO_PB(2), DS_1_MODE }, //MSC0_D2
	{ GPIO_PB(3), DS_1_MODE }, //MSC0_D3
	{ GPIO_PB(4), DS_1_MODE }, //MSC0_CLK
	{ GPIO_PB(5), DS_1_MODE }, //MSC0_CMD
};

int gpio_drive_strength_init(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(soc_gpio_drive_strength_table); i++) {
		pr_debug("set gpio strength: %d-%d",
			 soc_gpio_drive_strength_table[i].gpio_num,
			 soc_gpio_drive_strength_table[i].drv_level);
		jz_gpio_set_drive_strength(soc_gpio_drive_strength_table[i].gpio_num,
					   soc_gpio_drive_strength_table[i].drv_level);
	}
	return 0;
}

int gpio_pull_init(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(soc_gpio_pull_table); i++) {
		pr_debug("set gpio pull: %d-%02x",
			 soc_gpio_pull_table[i].gpio_num,
			 soc_gpio_pull_table[i].pull_status);
		jz_gpio_set_func(soc_gpio_pull_table[i].gpio_num,
				 soc_gpio_pull_table[i].pull_status);
	}
	return 0;
}

void sfc_single_set(void)
{
	jz_gpio_set_func(GPIO_PA(25), GPIO_OUTPUT0);
	jz_gpio_set_func(GPIO_PA(26), GPIO_OUTPUT0);
}

int __init gpio_customized_init(void)
{
	gpio_drive_strength_init();
	gpio_pull_init();
	sfc_single_set();

	return 0;
}

//arch_initcall(gpio_customized_init);
