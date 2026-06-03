/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 */

#ifndef _NPU_DPDK_PROCFS_H_
#define _NPU_DPDK_PROCFS_H_

#if defined(CONFIG_PROC_FS)
int mtk_npu_dpdk_procfs_init(void);
void mtk_npu_dpdk_procfs_deinit(void);
#else /* !defined(CONFIG_PROC_FS) */
static inline int mtk_npu_dpdk_procfs_init(void)
{
	return 0;
}
static inline void mtk_npu_dpdk_procfs_deinit(void)
{
}
#endif /* defined(CONFIG_PROC_FS) */
#endif /* _NPU_DPDK_PROCFS_H_ */
