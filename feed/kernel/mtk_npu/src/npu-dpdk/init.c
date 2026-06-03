// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "npu/net-core.h"
#include "npu-dpdk/cdev.h"
#include "npu-dpdk/internal.h"
#include "npu-dpdk/mac-filter.h"
#include "npu-dpdk/npu-dpdk-cmd.h"
#include "npu-dpdk/procfs.h"
#include "npu-dpdk/sysfs.h"

struct npu_dpdk_dev ndpdk;

static int mtk_npu_dpdk_device_init(void)
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
		if (!res) {
			ret = -ENXIO;
			goto out;
		}
	}

	ndpdk.base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!ndpdk.base) {
		ret = -ENOMEM;
		goto out;
	}

	ndpdk.dev = &pdev->dev;

out:
	of_node_put(node);

	return ret;
}

static int mtk_npu_dpdk_enable(bool en)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_DPDK,
		.sub_type = NPU_DPDK_NET_CMD_SET_EN,
		.arg[0] = en,
	};

	mtk_npu_net_send_cmd_all_no_wait(&cmd);

	return 0;
}

static int __init mtk_npu_dpdk_init(void)
{
	int ret;

	ret = mtk_npu_dpdk_device_init();
	if (ret) {
		pr_notice("find npu device for dpdk failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_dpdk_cdev_init();
	if (ret) {
		NPU_NOTICE("dpdk character device init failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_dpdk_enable(true);
	if (ret) {
		NPU_NOTICE("dpdk cmd set failed: %d\n", ret);
		goto cdev_deinit;
	}

	ret = mtk_npu_dpdk_mac_filter_init();
	if (ret) {
		NPU_NOTICE("dpdk mac filter init failed: %d\n", ret);
		goto dpdk_disable;
	}

	ret = mtk_npu_dpdk_procfs_init();
	if (ret) {
		NPU_NOTICE("dpdk procfs init failed: %d\n", ret);
		goto mac_filter_deinit;
	}

	ret = mtk_npu_dpdk_sysfs_init();
	if (ret) {
		NPU_NOTICE("dpdk sysfs init failed: %d\n", ret);
		goto dpdk_procfs_deinit;
	}

	return ret;

dpdk_procfs_deinit:
	mtk_npu_dpdk_procfs_deinit();

mac_filter_deinit:
	mtk_npu_dpdk_mac_filter_deinit();

dpdk_disable:
	mtk_npu_dpdk_enable(false);

cdev_deinit:
	mtk_npu_dpdk_cdev_deinit();

	return ret;
}

static void __exit mtk_npu_dpdk_exit(void)
{
	mtk_npu_dpdk_sysfs_deinit();

	mtk_npu_dpdk_procfs_deinit();

	mtk_npu_dpdk_mac_filter_deinit();

	mtk_npu_dpdk_enable(false);

	mtk_npu_dpdk_cdev_deinit();
}

module_init(mtk_npu_dpdk_init);
module_exit(mtk_npu_dpdk_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek Network Processor Unit with DPDK function");
MODULE_AUTHOR("Ren-Ting Wang <ren-ting.wang@mediatek.com>");
