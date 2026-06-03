/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */
#ifndef _NPU_DEBUGFS_H_
#define _NPU_DEBUGFS_H_

#include <linux/debugfs.h>
#include <linux/platform_device.h>

#if defined(CONFIG_DEBUG_FS)
extern struct dentry *npu_debugfs_root;

int mtk_npu_debugfs_init(struct platform_device *pdev);
void mtk_npu_debugfs_deinit(struct platform_device *pdev);
#else /* !defined(CONFIG_DEBUG_FS) */
static inline mtk_npu_debugfs_init(struct platform_device *pdev)
{
	return 0;
}

static inline void mtk_npu_debugfs_deinit(struct platform_device *pdev)
{
}
#endif /* defined(CONFIG_DEBUG_FS) */
#endif /* _NPU_DEBUGFS_H_ */
