// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: James Liao <jamesjj.liao@mediatek.com>
 */

#define pr_fmt(fmt) "[clk-bringup] " fmt

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>

static const struct of_device_id bring_up_id_table[] = {
	{ .compatible = "mediatek,clk-bring-up",},
	{ .compatible = "mediatek,mt8163-bring-up",},
	{ .compatible = "mediatek,mt8173-bring-up",},
	{ },
};
MODULE_DEVICE_TABLE(of, bring_up_id_table);

static int bring_up_probe(struct platform_device *pdev)
{
	const int NR_CLKS = 400;
	char clk_name_buf[16];
	struct clk *clk;
	int i, r;

	for (i = 0; i < NR_CLKS; i++) {
		sprintf(clk_name_buf, "%d", i);

		clk = devm_clk_get(&pdev->dev, clk_name_buf);
		if (!IS_ERR(clk)) {
			r = clk_prepare_enable(clk);
			if (r)
				pr_debug("clk_prepare_enable(%s): %d\n",
					__clk_get_name(clk), r);
		}
	}

	return 0;
}

static int bring_up_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver bring_up = {
	.probe		= bring_up_probe,
	.remove		= bring_up_remove,
	.driver		= {
		.name	= "bring_up",
		.owner	= THIS_MODULE,
		.of_match_table = bring_up_id_table,
	},
};

module_platform_driver(bring_up);
