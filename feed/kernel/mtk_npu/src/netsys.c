// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <pce/netsys.h>

#include "npu/internal.h"
#include "npu/mcu.h"
#include "npu/netsys.h"
#include "npu/tdma.h"
#include "npu/trm.h"

/* Netsys dump length */
#define FE_BASE_LEN				(0x2900)

#define PPE_DEFAULT_ENTRY_SIZE			(0x400)

static int netsys_trm_hw_dump(void *dst, u32 ofs, u32 len);

struct netsys_hw {
	struct mtk_eth *eth;
	void __iomem *base;
	u32 ppe_num;
	bool dsa_mode;
};

static struct netsys_hw netsys;

static struct trm_config netsys_trm_configs[] = {
	{
		TRM_CFG_EN("netsys-fe",
			   FE_BASE, FE_BASE_LEN,
			   0x0, FE_BASE_LEN,
			   0)
	},
};

static struct trm_hw_config netsys_trm_hw_cfg = {
	.trm_cfgs = netsys_trm_configs,
	.cfg_len = ARRAY_SIZE(netsys_trm_configs),
	.trm_hw_dump = netsys_trm_hw_dump,
};

static inline void netsys_write(u32 reg, u32 val)
{
	writel(val, netsys.base + reg);
}

static inline void netsys_set(u32 reg, u32 mask)
{
	setbits(netsys.base + reg, mask);
}

static inline void netsys_clr(u32 reg, u32 mask)
{
	clrbits(netsys.base + reg, mask);
}

static inline void netsys_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(netsys.base + reg, mask, val);
}

static inline u32 netsys_read(u32 reg)
{
	return readl(netsys.base + reg);
}

static int netsys_trm_hw_dump(void *dst, u32 start_addr, u32 len)
{
	u32 ofs;

	if (unlikely(!dst))
		return -ENODEV;

	for (ofs = 0; len > 0; len -= 0x4, ofs += 0x4)
		writel(netsys_read(start_addr + ofs), dst + ofs);

	return 0;
}

static inline void ppe_rmw(enum pse_port ppe, u32 reg, u32 mask, u32 val)
{
	if (ppe == PSE_PORT_PPE0)
		netsys_rmw(PPE0_BASE + reg, mask, val);
	else if (ppe == PSE_PORT_PPE1)
		netsys_rmw(PPE1_BASE + reg, mask, val);
	else if (ppe == PSE_PORT_PPE2)
		netsys_rmw(PPE2_BASE + reg, mask, val);
}

static inline u32 ppe_read(enum pse_port ppe, u32 reg)
{
	if (ppe == PSE_PORT_PPE0)
		return netsys_read(PPE0_BASE + reg);
	else if (ppe == PSE_PORT_PPE1)
		return netsys_read(PPE1_BASE + reg);
	else if (ppe == PSE_PORT_PPE2)
		return netsys_read(PPE2_BASE + reg);

	return 0;
}

struct mtk_eth *mtk_npu_netsys_get_eth(void)
{
	return netsys.eth;
}
EXPORT_SYMBOL(mtk_npu_netsys_get_eth);

u32 mtk_npu_netsys_ppe_get_num(void)
{
	return netsys.ppe_num;
}
EXPORT_SYMBOL(mtk_npu_netsys_ppe_get_num);

u32 mtk_npu_netsys_ppe_get_max_entry_num(u32 ppe_id)
{
	u32 tbl_entry_num;
	enum pse_port ppe;

	if (ppe_id == 0)
		ppe = PSE_PORT_PPE0;
	else if (ppe_id == 1)
		ppe = PSE_PORT_PPE1;
	else if (ppe_id == 2)
		ppe = PSE_PORT_PPE2;
	else
		return PPE_DEFAULT_ENTRY_SIZE << 5; /* max entry count */

	tbl_entry_num = ppe_read(ppe, PPE_TBL_CFG);
	if (tbl_entry_num > 5)
		return PPE_DEFAULT_ENTRY_SIZE << 5;

	return PPE_DEFAULT_ENTRY_SIZE << tbl_entry_num;
}
EXPORT_SYMBOL(mtk_npu_netsys_ppe_get_max_entry_num);

bool mtk_npu_netsys_is_dsa_mode(void)
{
	return netsys.dsa_mode;
}

static int mtk_npu_netsys_base_init(struct platform_device *pdev)
{
	struct device_node *fe_mem = NULL;
	struct platform_device *eth_pdev;
	struct resource res;
	int ret = 0;

	fe_mem = of_parse_phandle(pdev->dev.of_node, "fe_mem", 0);
	if (!fe_mem) {
		NPU_ERR("can not find fe_mem node\n");
		return -ENODEV;
	}

	eth_pdev = of_find_device_by_node(fe_mem);
	if (!eth_pdev) {
		ret = -ENODEV;
		goto out;
	}

	if (!eth_pdev->dev.driver) {
		ret = -EFAULT;
		goto out;
	}

	if (of_address_to_resource(fe_mem, 0, &res)) {
		ret = -ENXIO;
		goto out;
	}

	netsys.base = devm_ioremap(&pdev->dev, res.start, resource_size(&res));
	if (!netsys.base) {
		ret = -ENOMEM;
		goto out;
	}

	netsys.eth = platform_get_drvdata(eth_pdev);

out:
	of_node_put(fe_mem);

	return ret;
}

static int mtk_npu_netsys_ppe_num_init(struct platform_device *pdev)
{
	struct device_node *hnat = NULL;
	u32 val = 0;
	int ret = 0;

	/* Todo: nf_hnat and flowblock has different way to get ppe_num
	 * Maybe we could seperate ppe_num_init in npu-nf_hnat.ko and
	 * npu-flow.ko
	 */
	netsys.ppe_num = 3;

	hnat = of_parse_phandle(pdev->dev.of_node, "hnat", 0);
	if (!hnat) {
		NPU_ERR("can not find hnat node, use default ppe_num\n");
		return 0;
	}

	ret = of_property_read_u32(hnat, "mtketh-ppe-num", &val);
	if (ret)
		netsys.ppe_num = 1;
	else
		netsys.ppe_num = val;

	of_node_put(hnat);

	return 0;
}

static void mtk_npu_netsys_dsa_mode_init(struct platform_device *pdev)
{
	struct device_node *node;

	node = of_find_node_by_name(NULL, "switch");
	if (node)
		netsys.dsa_mode = true;
}

int mtk_npu_netsys_init(struct platform_device *pdev)
{
	int ret;

	ret = mtk_npu_netsys_base_init(pdev);
	if (ret)
		return ret;

	ret = mtk_npu_netsys_ppe_num_init(pdev);
	if (ret)
		return ret;

	mtk_npu_netsys_dsa_mode_init(pdev);

	ret = mtk_trm_hw_config_register(TRM_NETSYS, &netsys_trm_hw_cfg);
	if (ret)
		return ret;

	return ret;
}

void mtk_npu_netsys_deinit(struct platform_device *pdev)
{
	mtk_trm_hw_config_unregister(TRM_NETSYS, &netsys_trm_hw_cfg);
}
