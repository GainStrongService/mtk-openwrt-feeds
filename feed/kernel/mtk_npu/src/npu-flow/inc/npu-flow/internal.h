/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Chris Chou <chris.chou@mediatek.com>
 */

#ifndef _NPU_FLOW_H_
#define _NPU_FLOW_H_

#include <linux/device.h>

struct npu_flow_dev {
	struct device *dev;
	void __iomem *base;
};

extern struct npu_flow_dev npu_flow;

#define NPU_DBG(fmt, ...)		dev_dbg(npu_flow.dev, "flow: " fmt, ##__VA_ARGS__)
#define NPU_INFO(fmt, ...)		dev_info(npu_flow.dev, "flow: " fmt, ##__VA_ARGS__)
#define NPU_NOTICE(fmt, ...)		dev_notice(npu_flow.dev, "flow: " fmt, ##__VA_ARGS__)
#define NPU_WARN(fmt, ...)		dev_warn(npu_flow.dev, "flow: " fmt, ##__VA_ARGS__)
#define NPU_ERR(fmt, ...)		dev_err(npu_flow.dev, "flow: " fmt, ##__VA_ARGS__)
#endif /* _NPU_FLOW_H_ */
