/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NF_HNAT_H_
#define _NPU_NF_HNAT_H_

#include <linux/device.h>

struct npu_nf_hnat_dev {
	struct device *dev;
	void __iomem *base;
};

extern struct npu_nf_hnat_dev npu_nf_hnat;

#define NPU_DBG(fmt, ...)		dev_dbg(npu_nf_hnat.dev, "nf_hnat: " fmt, ##__VA_ARGS__)
#define NPU_INFO(fmt, ...)		dev_info(npu_nf_hnat.dev, "nf_hnat: " fmt, ##__VA_ARGS__)
#define NPU_NOTICE(fmt, ...)		dev_notice(npu_nf_hnat.dev, "nf_hnat: " fmt, ##__VA_ARGS__)
#define NPU_WARN(fmt, ...)		dev_warn(npu_nf_hnat.dev, "nf_hnat: " fmt, ##__VA_ARGS__)
#define NPU_ERR(fmt, ...)		dev_err(npu_nf_hnat.dev, "nf_hnat: " fmt, ##__VA_ARGS__)
#endif /* _NPU_NF_HNAT_H_ */
