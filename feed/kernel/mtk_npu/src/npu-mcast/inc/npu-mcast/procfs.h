/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_MCAST_PROCFS_H_
#define _NPU_MCAST_PROCFS_H_

#if defined(CONFIG_PROC_FS)
int mtk_npu_mcast_procfs_init(void);
void mtk_npu_mcast_procfs_deinit(void);
#else /* !defined(CONFIG_PROC_FS) */
static inline int mtk_npu_mcast_procfs_init(void)
{
	return 0;
}

static inline void mtk_npu_mcast_procfs_deinit(void)
{
}
#endif /* defined(CONFIG_PROC_FS) */
#endif /* _NPU_MCAST_PROCFS_H_ */
