// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <pce/netsys.h>

#include "npu-nf_hnat/internal.h"
#include "npu-nf_hnat/netsys.h"

#define PPE_DEFAULT_ENTRY_SIZE			(0x400)

struct netsys_hw {
	void __iomem *base;
	u32 ppe_num;
};

static struct netsys_hw netsys;

static inline u32 netsys_read(u32 reg)
{
	return readl(netsys.base + reg);
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

u32 mtk_npu_nf_hnat_netsys_ppe_get_num(void)
{
	return netsys.ppe_num;
}

u32 mtk_npu_nf_hnat_netsys_ppe_get_max_entry_num(u32 ppe_id)
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

static void mtk_npu_nf_hnat_netsys_base_deinit(void)
{
	iounmap(netsys.base);
}

static int mtk_npu_nf_hnat_netsys_base_init(void)
{
	struct device_node *fe_mem = NULL;
	struct resource res;
	int ret = 0;

	fe_mem = of_parse_phandle(npu_nf_hnat.dev->of_node, "fe_mem", 0);
	if (!fe_mem) {
		NPU_NOTICE("can not find fe_mem node\n");
		return -ENODEV;
	}

	if (of_address_to_resource(fe_mem, 0, &res)) {
		ret = -ENXIO;
		goto out;
	}

	netsys.base = ioremap(res.start, resource_size(&res));
	if (!netsys.base) {
		ret = -ENOMEM;
		goto out;
	}

out:
	of_node_put(eth);

	return ret;
}

static int mtk_npu_nf_hnat_netsys_ppe_num_init(void)
{
	struct device_node *hnat = NULL;
	u32 val = 0;
	int ret = 0;

	hnat = of_parse_phandle(npu_nf_hnat.dev->of_node, "hnat", 0);
	if (!hnat) {
		TOPS_ERR("can not find hnat node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(hnat, "mtketh-ppe-num", &val);
	if (ret)
		netsys.ppe_num = 1;
	else
		netsys.ppe_num = val;

	of_node_put(hnat);

	return 0;
}

int mtk_npu_nf_hnat_netsys_init(void)
{
	int ret;

	ret = mtk_npu_nf_hnat_netsys_base_init();
	if (ret)
		return ret;

	ret = mtk_npu_nf_hnat_netsys_ppe_num_init();
	if (ret) {
		mtk_npu_nf_hnat_netsys_base_deinit();
		return ret;
	}

	return ret;
}

void mtk_npu_nf_hnat_netsys_deinit(void)
{
	mtk_npu_nf_hnat_netsys_base_deinit();
}
