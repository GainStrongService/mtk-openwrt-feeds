/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_SYSFS_NET_H_
#define _NPU_SYSFS_NET_H_

#include <linux/platform_device.h>

#if defined(CONFIG_SYSFS)
int mtk_npu_sysfs_net_init(struct platform_device *pdev);
void mtk_npu_sysfs_net_deinit(struct platform_device *pdev);
#else /* !defined(CONFIG_SYSFS) */
static inline int mtk_npu_sysfs_net_init(struct platform_device *pdev)
{
	return 0;
}

static inline void mtk_npu_sysfs_net_deinit(struct platform_device *pdev)
{
}
#endif /* defined(CONFIG_SYSFS) */
#endif /* _NPU_SYSFS_NET_H_ */
