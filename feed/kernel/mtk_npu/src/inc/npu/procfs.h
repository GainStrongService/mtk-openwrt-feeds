/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_PROCFS_H_
#define _NPU_PROCFS_H_

#include <linux/platform_device.h>
#include <linux/proc_fs.h>

#if defined(CONFIG_PROC_FS)
extern struct proc_dir_entry *npu_proc_root;

int mtk_npu_procfs_init(struct platform_device *pdev);
void mtk_npu_procfs_deinit(struct platform_device *pdev);
#else /* !defined(CONFIG_PROC_FS) */
static inline int mtk_npu_procfs_init(struct platform_device *pdev)
{
	return 0;
};

static inline void mtk_npu_procfs_deinit(struct platform_device *pdev)
{
}
#endif /* defined(CONFIG_PROC_FS) */
#endif /* _NPU_PROCFS_H_ */
