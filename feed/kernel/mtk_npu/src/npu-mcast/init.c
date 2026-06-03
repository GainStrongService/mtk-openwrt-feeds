// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/io.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "npu-mcast/internal.h"
#include "npu-mcast/mcast.h"
#include "npu-mcast/procfs.h"
#include "npu-mcast/sysfs.h"

struct npu_mcast_dev nmcast;

static int mtk_npu_multicast_device_init(void)
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

	nmcast.base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!nmcast.base)
		return -ENOMEM;

	nmcast.dev = &pdev->dev;

out:
	of_node_put(node);

	return ret;
}

static int __init mtk_npu_multicast_init(void)
{
	int ret;

	ret = mtk_npu_multicast_device_init();
	if (ret) {
		pr_notice("find npu device for multicast failed: %d\n", ret);
		return ret;
	}

	/*
	 * TODO: should implement a multicast deinitialization function
	 * to undone everything that is submitted to the MCU.
	 */
	ret = mtk_npu_mcast_init();
	if (ret) {
		NPU_NOTICE("multicast init failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_mcast_sysfs_init();
	if (ret) {
		NPU_NOTICE("multicast sysfs init failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_mcast_procfs_init();
	if (ret) {
		NPU_NOTICE("multicast procfs init failed: %d\n", ret);
		goto err_sysfs_deinit;
	}

	return ret;

err_sysfs_deinit:
	mtk_npu_mcast_sysfs_deinit();

	return ret;
}

static void __exit mtk_npu_multicast_exit(void)
{
	mtk_npu_mcast_procfs_deinit();
	mtk_npu_mcast_sysfs_deinit();
}

module_init(mtk_npu_multicast_init);
module_exit(mtk_npu_multicast_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek Network Processor Unit with IPv4/IPv6 Multicast");
MODULE_AUTHOR("Ren-Ting Wang <ren-ting.wang@mediatek.com>");
