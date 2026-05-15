#include <linux/delay.h>
#include <linux/hw_random.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/io.h>
#include <linux/crypto.h>
#include <linux/random.h>
#include <linux/interrupt.h>
#include <crypto/scatterwalk.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <soc/base.h>
#include <soc/irq.h>


#define DTRNG_TIMEOUT_MS 500

#define DTRNG_CFG (0x00)
#define DTRNG_RANDOMNUM (0x04)
#define DTRNG_STAT (0x08)

#define DTRNG_EN (0x1 << 0)
#define DTRNG_DIS ~(0x1 << 0)

struct ingenic_rng {
	void __iomem *base;
	struct hwrng rng;
	struct clk *clk;
};

static int jz_rng_init(struct hwrng *rng)
{
	struct ingenic_rng *data =
			container_of(rng, struct ingenic_rng, rng);
	printk("jz_rng_init,base:%p\n",data->base);
	return 0;
}

static void jz_rng_cleanup(struct hwrng *rng)
{
	unsigned int trng_cfg;
	struct ingenic_rng *prng =container_of(rng, struct ingenic_rng, rng);

	trng_cfg = readl(prng->base + DTRNG_CFG);
	trng_cfg &= DTRNG_DIS;
	writel(trng_cfg, prng->base + DTRNG_CFG);
	return ;
}

static int jz_rng_read(struct hwrng *rng,void *buf, size_t max, bool wait)
{
	unsigned long value = 0;
	unsigned long timeout = jiffies + msecs_to_jiffies(DTRNG_TIMEOUT_MS);
	struct ingenic_rng *prng =container_of(rng, struct ingenic_rng, rng);

	value = readl(prng->base + DTRNG_CFG);
	value |= DTRNG_EN;

	writel(value,prng->base + DTRNG_CFG);

	// wait dtrng ready
	while (time_before(jiffies, timeout)) {
		msleep(1);
		if(readl(prng->base + DTRNG_STAT) & 0x1){
			break;
		} else {
			printk("[%s]warning, status is err\n",__FUNCTION__);
		}
	}

	value = readl(prng->base + DTRNG_RANDOMNUM);

	*(u32 *)buf = value;

	return sizeof(u32);
}

static int jz_dtrng_probe(struct platform_device *pdev)
{
	struct ingenic_rng *rng;
	struct resource *res;
	int ret;

	rng = devm_kzalloc(&pdev->dev, sizeof(*rng), GFP_KERNEL);
	if (!rng)
		return -ENOMEM;

	platform_set_drvdata(pdev, rng);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	rng->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rng->base))
		return PTR_ERR(rng->base);

	rng->rng.name = pdev->name;
	rng->rng.init = jz_rng_init;
	rng->rng.cleanup = jz_rng_cleanup;
	rng->rng.read = jz_rng_read;

	ret = hwrng_register(&rng->rng);
	if (ret) {
		dev_err(&pdev->dev, "failed to register hwrng\n");
		return ret;
	}

	rng->clk = clk_get(&pdev->dev, "gate_dtrng");
	if (IS_ERR(rng->clk)) {
		dev_err(&pdev->dev, "dtrng clk get failed!\n");
		ret = -EINVAL;
		return -1;
	}
	clk_enable(rng->clk);

	return 0;
}

static int jz_dtrng_remove(struct platform_device *pdev)
{
	struct ingenic_rng *rng = platform_get_drvdata(pdev);
	hwrng_register(&rng->rng);
	clk_disable(rng->clk);
	clk_put(rng->clk);
	kfree(rng);

	return 0;
}
static void platform_dtrng_release(struct device *dev)
{
	return ;
}

static struct resource jz_dtrng_resources[] = {
	[0] = {
		.start	= DTRNG_IOBASE,
		.end	= DTRNG_IOBASE + 0x20,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DTRNG,
		.end	= IRQ_DTRNG,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device jz_dtrng_device = {
	.name = "jz-dtrng",
	.id = 0,
	.resource = jz_dtrng_resources,
	.dev = {
		.release = platform_dtrng_release,
	},
	.num_resources = ARRAY_SIZE(jz_dtrng_resources),
};

static struct platform_driver jz_dtrng_driver = {
	.probe	= jz_dtrng_probe,
	.remove	= jz_dtrng_remove,
	.driver	= {
		.name	= "jz-dtrng",
		.owner	= THIS_MODULE,
	},
};

static int __init jz_dtrng_mod_init(void)
{
	int ret = 0;

	ret = platform_device_register(&jz_dtrng_device);
	if(ret){
		printk("Failed to insmod dtrng device!!!\n");
		return ret;
	}
	return platform_driver_register(&jz_dtrng_driver);
}

static void __exit jz_dtrng_mod_exit(void)
{
	platform_device_unregister(&jz_dtrng_device);
	platform_driver_unregister(&jz_dtrng_driver);
}

module_init(jz_dtrng_mod_init);
module_exit(jz_dtrng_mod_exit);

MODULE_DESCRIPTION("Ingenic DTRNG hw support.");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jwang@ingenic.com");