/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_MCAST_INTERNAL_H_
#define _NPU_MCAST_INTERNAL_H_

#include <linux/device.h>

struct npu_mcast_dev {
	struct device *dev;
	void __iomem *base;
};

extern struct npu_mcast_dev nmcast;

#define NPU_DBG(fmt, ...)		dev_dbg(nmcast.dev, "mcast: " fmt, ##__VA_ARGS__)
#define NPU_INFO(fmt, ...)		dev_info(nmcast.dev, "mcast: " fmt, ##__VA_ARGS__)
#define NPU_NOTICE(fmt, ...)		dev_notice(nmcast.dev, "mcast: " fmt, ##__VA_ARGS__)
#define NPU_WARN(fmt, ...)		dev_warn(nmcast.dev, "mcast: " fmt, ##__VA_ARGS__)
#define NPU_ERR(fmt, ...)		dev_err(nmcast.dev, "mcast: " fmt, ##__VA_ARGS__)
#endif /* _NPU_MCAST_INTERNAL_H_ */
