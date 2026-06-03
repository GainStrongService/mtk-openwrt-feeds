// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "npu-nf_hnat/internal.h"
#include "npu-nf_hnat/netsys.h"
#include "npu-nf_hnat/net-event.h"
#include "npu-nf_hnat/statistic.h"
#include "npu-nf_hnat/tnl-offload.h"

struct npu_nf_hnat_dev npu_nf_hnat;

static int mtk_npu_nf_hnat_device_init(void)
{
	struct platform_device *pdev;
	struct device_node *node;
	struct resource *res;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,npu");
	if (!node) {
		/* for legacy NPU name, should be removed after openwrt-21.02 deprecated */
		node = of_find_compatible_node(NULL, NULL, "mediatek,tops");
		if (!node) {
			pr_notice("Cannot find NPU device node\n");
			return -ENODEV;
		}
	}

	pdev = of_find_device_by_node(node);
	if (!pdev) {
		pr_notice("Cannot find NPU platform device\n");
		ret = -ENODEV;
		goto out;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "npu-base");
	if (!res) {
		/* for legacy NPU name, should be removed after openwrt-21.02 deprecated */
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-base");
		if (!res)
			return -ENXIO;
	}

	npu_nf_hnat.base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!npu_nf_hnat.base)
		return -ENOMEM;

	npu_nf_hnat.dev = &pdev->dev;

out:
	of_node_put(node);

	return ret;
}

static int __init mtk_npu_nf_hnat_init(void)
{
	int ret = 0;

	ret = mtk_npu_nf_hnat_device_init();
	if (ret)
		return ret;

	ret = mtk_npu_nf_hnat_tnl_offload_init();
	if (ret)
		return ret;

	ret = mtk_npu_nf_hnat_net_event_init();
	if (ret)
		goto err_tnl_offload_deinit;

	ret = mtk_npu_nf_hnat_statistic_init();
	if (ret)
		goto err_net_event_deinit;

	return ret;

err_net_event_deinit:
	mtk_npu_nf_hnat_net_event_deinit();

err_tnl_offload_deinit:
	mtk_npu_nf_hnat_tnl_offload_deinit();

	return ret;
}

static void __exit mtk_npu_nf_hnat_exit(void)
{
	mtk_npu_nf_hnat_statistic_deinit();

	mtk_npu_nf_hnat_net_event_deinit();

	mtk_npu_nf_hnat_tnl_offload_deinit();
}

module_init(mtk_npu_nf_hnat_init);
module_exit(mtk_npu_nf_hnat_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek NPU Tunnel Offload Module with HNAT");
MODULE_AUTHOR("Ren-Ting Wang <ren-ting.wang@mediatek.com>");
