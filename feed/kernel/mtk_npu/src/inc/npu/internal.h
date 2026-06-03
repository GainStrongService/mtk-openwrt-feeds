/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_INTERNAL_H_
#define _NPU_INTERNAL_H_

#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/io.h>

struct npu {
	struct device *dev;
	void __iomem *base;
};

extern struct npu npu;

#define NPU_DBG(fmt, ...)		dev_dbg(npu.dev, fmt, ##__VA_ARGS__)
#define NPU_INFO(fmt, ...)		dev_info(npu.dev, fmt, ##__VA_ARGS__)
#define NPU_NOTICE(fmt, ...)		dev_notice(npu.dev, fmt, ##__VA_ARGS__)
#define NPU_WARN(fmt, ...)		dev_warn(npu.dev, fmt, ##__VA_ARGS__)
#define NPU_ERR(fmt, ...)		dev_err(npu.dev, fmt, ##__VA_ARGS__)

/* npu 32 bits read/write */
#define setbits(addr, set)		writel(readl(addr) | (set), (addr))
#define clrbits(addr, clr)		writel(readl(addr) & ~(clr), (addr))
#define clrsetbits(addr, clr, set)	writel((readl(addr) & ~(clr)) | (set), (addr))
#endif /* _NPU_INTERNAL_H_ */
