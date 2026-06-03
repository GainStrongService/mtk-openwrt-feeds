/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NF_HNAT_DEBUGFS_H_
#define _NPU_NF_HNAT_DEBUGFS_H_

#if defined(CONFIG_DEBUG_FS)
int mtk_npu_nf_hnat_debugfs_init(void);
void mtk_npu_nf_hnat_debugfs_deinit(void);
#else /* !defined(CONFIG_DEBUG_FS) */
static inline int mtk_npu_nf_hnat_debugfs_init(void)
{
	return 0;
}

static inline void mtk_npu_nf_hnat_debugfs_deinit(void)
{
}
#endif /* defined(CONFIG_DEBUG_FS) */
#endif /* _NPU_NF_HNAT_DEBUGFS_H_ */
