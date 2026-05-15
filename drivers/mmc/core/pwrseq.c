// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 2014 Linaro Ltd
 *
 * Author: Ulf Hansson <ulf.hansson@linaro.org>
 *
 *  MMC power sequence management
 */
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <soc/gpio.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>

#include "pwrseq.h"

static DEFINE_MUTEX(pwrseq_list_mutex);
static LIST_HEAD(pwrseq_list);

int mmc_pwrseq_alloc(struct mmc_host *host)
{
	struct device_node *np;
	struct mmc_pwrseq *p;

	np = of_parse_phandle(host->parent->of_node, "mmc-pwrseq", 0);
	if (!np)
		return 0;

	mutex_lock(&pwrseq_list_mutex);
	list_for_each_entry(p, &pwrseq_list, pwrseq_node) {
		if (p->dev->of_node == np) {
			if (!try_module_get(p->owner))
				dev_err(host->parent,
					"increasing module refcount failed\n");
			else
				host->pwrseq = p;

			break;
		}
	}

	of_node_put(np);
	mutex_unlock(&pwrseq_list_mutex);

	if (!host->pwrseq)
		return -EPROBE_DEFER;

	dev_info(host->parent, "allocated mmc-pwrseq\n");

	return 0;
}

void mmc_pwrseq_pre_power_on(struct mmc_host *host)
{
	struct mmc_pwrseq *pwrseq = host->pwrseq;

	if (pwrseq && pwrseq->ops->pre_power_on)
		pwrseq->ops->pre_power_on(host);
}

static int mmc_pwrseq_select_pinctrl_state(struct device *dev, const char *state)
{
    struct pinctrl *p;
    struct pinctrl_state *pinctrl_state;
    int ret = 0;

    if (!dev)
        return -EINVAL;

    // 获取设备的pinctrl
    p = devm_pinctrl_get(dev);
    if (IS_ERR(p)) {
        dev_err(dev, "no pinctrl handle\n");
        return -1;
    }

    // 查找对应的状态
    pinctrl_state = pinctrl_lookup_state(p, state);
    if (IS_ERR(pinctrl_state)) {
        dev_err(dev, "no '%s' pinctrl state\n", state);
        devm_pinctrl_put(p);
        return -1;
    }

    // 切换状态
    ret = pinctrl_select_state(p, pinctrl_state);
    if (ret)
        dev_err(dev, "failed to select state '%s': %d\n", state, ret);

    // 释放pinctrl引用
    devm_pinctrl_put(p);

    return ret;
}

void mmc_pwrseq_post_power_on(struct mmc_host *host)
{
	int ret = 0;
	struct mmc_pwrseq *pwrseq = host->pwrseq;
	struct device *dev = host->parent;

	if (pwrseq && pwrseq->ops->post_power_on)
		pwrseq->ops->post_power_on(host);
	mdelay(50);
	ret = mmc_pwrseq_select_pinctrl_state(dev, "active");
	if (ret)
		dev_err(dev, "Failed to power on msc%d!: %d\n", host->index, ret);
}

void mmc_pwrseq_power_off(struct mmc_host *host)
{
	int ret = 0;
	struct mmc_pwrseq *pwrseq = host->pwrseq;
	struct device *dev = host->parent;

	ret = mmc_pwrseq_select_pinctrl_state(dev, "default");
	if (ret)
		dev_err(dev, "Failed to power off msc%d: %d\n", host->index, ret);

	if (pwrseq && pwrseq->ops->power_off)
		pwrseq->ops->power_off(host);
}

void mmc_pwrseq_reset(struct mmc_host *host)
{
	struct mmc_pwrseq *pwrseq = host->pwrseq;

	if (pwrseq && pwrseq->ops->reset)
		pwrseq->ops->reset(host);
}

void mmc_pwrseq_free(struct mmc_host *host)
{
	struct mmc_pwrseq *pwrseq = host->pwrseq;

	if (pwrseq) {
		module_put(pwrseq->owner);
		host->pwrseq = NULL;
	}
}

int mmc_pwrseq_register(struct mmc_pwrseq *pwrseq)
{
	if (!pwrseq || !pwrseq->ops || !pwrseq->dev)
		return -EINVAL;

	mutex_lock(&pwrseq_list_mutex);
	list_add(&pwrseq->pwrseq_node, &pwrseq_list);
	mutex_unlock(&pwrseq_list_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(mmc_pwrseq_register);

void mmc_pwrseq_unregister(struct mmc_pwrseq *pwrseq)
{
	if (pwrseq) {
		mutex_lock(&pwrseq_list_mutex);
		list_del(&pwrseq->pwrseq_node);
		mutex_unlock(&pwrseq_list_mutex);
	}
}
EXPORT_SYMBOL_GPL(mmc_pwrseq_unregister);
