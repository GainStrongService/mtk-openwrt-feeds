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
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/version.h>

#include "npu/debugfs.h"
#include "npu/firmware.h"
#include "npu/hwspinlock.h"
#include "npu/internal.h"
#include "npu/logger.h"
#include "npu/mbox.h"
#include "npu/mcu.h"
#include "npu/mem-test.h"
#include "npu/misc.h"
#include "npu/netsys.h"
#include "npu/net-core.h"
#include "npu/net-event.h"
#include "npu/npu-ocd.h"
#include "npu/procfs.h"
#include "npu/ser.h"
#include "npu/sysfs.h"
#include "npu/sysfs-net.h"
#include "npu/tdma.h"
#include "npu/trm.h"
#include "npu/trm-fs.h"
#include "npu/tunnel.h"
#include "npu/thermal.h"
#include "npu/wdt.h"
#include "npu/seq_gen.h"

#define EFUSE_NPU_POWER_OFF		(0xD08)

struct npu_init_data {
	int (*plat_net_dev_init)(struct platform_device *pdev);
};

struct npu npu;

static int mtk_npu_post_init(struct platform_device *pdev)
{
	int ret = 0;

	/* TODO */
	ret = mtk_npu_logger_init(pdev);
	if (ret) {
		NPU_ERR("logger init failed: %d\n", ret);
		return ret;
	}

	/* kick core */
	ret = mtk_npu_mcu_bring_up(pdev);
	if (ret) {
		NPU_ERR("mcu post init failed: %d\n", ret);
		goto err_logger_deinit;
	}

	ret = mtk_npu_tnl_offload_post_init(pdev);
	if (ret) {
		NPU_ERR("tnl offload post init failed: %d\n", ret);
		goto err_logger_deinit;
	}

	ret = mtk_npu_netevent_register(pdev);
	if (ret) {
		NPU_ERR("netevent register fail: %d\n", ret);
		goto err_mcu_tear_down;
	}

	/* create sysfs file */
	ret = mtk_npu_sysfs_init(pdev);
	if (ret) {
		NPU_ERR("sysfs init failed: %d\n", ret);
		goto err_netevent_unregister;
	}

	ret = mtk_npu_sysfs_net_init(pdev);
	if (ret) {
		NPU_ERR("sysfs net init failed: %d\n", ret);
		goto err_sysfs_deinit;
	}

	ret = mtk_npu_ser_init(pdev);
	if (ret) {
		NPU_ERR("ser init failed: %d\n", ret);
		goto err_sysfs_net_deinit;
	}

	ret = mtk_npu_wdt_init(pdev);
	if (ret) {
		NPU_ERR("wdt init failed: %d\n", ret);
		goto err_ser_deinit;
	}

	ret = mtk_npu_thermal_init(pdev);
	if (ret) {
		NPU_ERR("thermal init failed: %d\n", ret);
		goto err_wdt_deinit;
	}

	ret = mtk_npu_procfs_init(pdev);
	if (ret) {
		NPU_ERR("npu procfs init failed: %d\n", ret);
		goto err_thermal_deinit;
	}

	ret = mtk_npu_debugfs_init(pdev);
	if (ret) {
		NPU_ERR("npu debugfs init failed: %d\n", ret);
		goto err_procfs_deinit;
	}

	ret = mtk_npu_trm_fs_init();
	if (ret) {
		NPU_ERR("trm init failed: %d\n", ret);
		goto err_debugfs_deinit;
	}

	return ret;

err_debugfs_deinit:
	mtk_npu_debugfs_deinit(pdev);

err_procfs_deinit:
	mtk_npu_procfs_deinit(pdev);

err_thermal_deinit:
	mtk_npu_thermal_deinit(pdev);

err_wdt_deinit:
	mtk_npu_wdt_deinit(pdev);

err_ser_deinit:
	mtk_npu_ser_deinit(pdev);

err_sysfs_net_deinit:
	mtk_npu_sysfs_net_deinit(pdev);

err_sysfs_deinit:
	mtk_npu_sysfs_deinit(pdev);

err_netevent_unregister:
	mtk_npu_netevent_unregister(pdev);

err_mcu_tear_down:
	mtk_npu_mcu_tear_down(pdev);

err_logger_deinit:
	mtk_npu_logger_deinit(pdev);

	return ret;
}

static int mtk_npu_probe(struct platform_device *pdev)
{
	const struct npu_init_data *init_data = of_device_get_match_data(&pdev->dev);
	struct resource *res;
	int ret = 0;

	npu.dev = &pdev->dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "npu-base");
	if (!res) {
		/* for legacy NPU name, should be removed after openwrt-21.02 deprecated */
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-base");
		if (!res)
			return -ENXIO;
	}

	npu.base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!npu.base)
		return -ENOMEM;

	ret = mtk_npu_mbox_init(pdev);
	if (ret) {
		NPU_ERR("mailbox init failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_fw_init(pdev);
	if (ret) {
		NPU_ERR("firmware init failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_trm_init(pdev);
	if (ret) {
		NPU_ERR("trm init failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_mcu_init(pdev);
	if (ret) {
		NPU_ERR("mcu init failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_netsys_init(pdev);
	if (ret) {
		NPU_ERR("netsys init failed: %d\n", ret);
		goto err_mcu_deinit;
	}

	ret = mtk_npu_net_core_init(pdev);
	if (ret) {
		NPU_ERR("npu net core init failed: %d\n", ret);
		goto err_netsys_deinit;
	}

	ret = init_data->plat_net_dev_init(pdev);
	if (ret) {
		NPU_ERR("platform npu net device init failed: %d\n", ret);
		goto err_net_core_deinit;
	}

	ret = mtk_npu_tnl_offload_init(pdev);
	if (ret) {
		NPU_ERR("tunnel table init failed: %d\n", ret);
		goto err_net_core_deinit;
	}

	ret = mtk_npu_misc_init(pdev);
	if (ret) {
		NPU_ERR("npu misc init failed: %d\n", ret);
		goto err_tnl_offload_deinit;
	}

	ret = mtk_npu_post_init(pdev);
	if (ret)
		goto err_misc_deinit;

	NPU_INFO("init done\n");
	return ret;

err_misc_deinit:
	mtk_npu_misc_deinit(pdev);

err_tnl_offload_deinit:
	mtk_npu_tnl_offload_deinit(pdev);

err_net_core_deinit:
	mtk_npu_net_core_deinit(pdev);

err_netsys_deinit:
	mtk_npu_netsys_deinit(pdev);

err_mcu_deinit:
	mtk_npu_mcu_deinit(pdev);

	return ret;
}

#if KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE
static int mtk_npu_remove(struct platform_device *pdev)
#else /* KERNEL_VERSION(6, 11, 0) <= LINUX_VERSION_CODE */
static void mtk_npu_remove(struct platform_device *pdev)
#endif /* KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE */
{
	mtk_npu_trm_fs_deinit();

	mtk_npu_debugfs_deinit(pdev);

	mtk_npu_procfs_deinit(pdev);

	mtk_npu_thermal_deinit(pdev);

	mtk_npu_wdt_deinit(pdev);

	mtk_npu_ser_deinit(pdev);

	mtk_npu_sysfs_net_deinit(pdev);

	mtk_npu_sysfs_deinit(pdev);

	mtk_npu_netevent_unregister(pdev);

	mtk_npu_mcu_tear_down(pdev);

	mtk_npu_logger_deinit(pdev);

	mtk_npu_misc_deinit(pdev);

	mtk_npu_tnl_offload_deinit(pdev);

	mtk_npu_netsys_deinit(pdev);

	mtk_npu_mcu_deinit(pdev);

	mtk_npu_trm_deinit(pdev);

#if KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE
	return 0;
#endif /* KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE */
}

static struct npu_init_data mt7988_npu_init_data = {
	.plat_net_dev_init = mtk_npu_tdma_init,
};

static const struct of_device_id npu_match[] = {
	{ .compatible = "mediatek,npu", .data = &mt7988_npu_init_data },
	{ .compatible = "mediatek,tops", .data = &mt7988_npu_init_data },
	{ },
};
MODULE_DEVICE_TABLE(of, npu_match);

static struct platform_driver mtk_npu_driver = {
	.probe = mtk_npu_probe,
	.remove = mtk_npu_remove,
	.driver = {
		.name = "mediatek,npu",
		.owner = THIS_MODULE,
		.of_match_table = npu_match,
	},
};

static int __init mtk_npu_hw_disabled(void)
{
	struct device_node *efuse_node;
	struct resource res;
	void __iomem *efuse_base;
	int ret = 0;

	efuse_node = of_find_compatible_node(NULL, NULL, "mediatek,efuse");
	if (!efuse_node)
		return -ENODEV;

	if (of_address_to_resource(efuse_node, 0, &res)) {
		ret = -ENXIO;
		goto out;
	}

	/* since we are not probed yet, we cannot use devm_* API */
	efuse_base = ioremap(res.start, resource_size(&res));
	if (!efuse_base) {
		ret = -ENOMEM;
		goto out;
	}

	if (readl(efuse_base + EFUSE_NPU_POWER_OFF))
		ret = -ENODEV;

	iounmap(efuse_base);

out:
	of_node_put(efuse_node);

	return ret;
}

static int __init mtk_npu_init(void)
{
	int ret;

	ret = mtk_npu_hw_disabled();
	if (ret) {
		pr_notice("NPU is not supported in this hardware configuration\n");
		return -ENODEV;
	}

	mtk_npu_ocd_init();

	return platform_driver_register(&mtk_npu_driver);
}

static void __exit mtk_npu_exit(void)
{
	platform_driver_unregister(&mtk_npu_driver);

	mtk_npu_ocd_exit();
}

module_init(mtk_npu_init);
module_exit(mtk_npu_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek NPU Driver");
MODULE_AUTHOR("Ren-Ting Wang <ren-ting.wang@mediatek.com>");
