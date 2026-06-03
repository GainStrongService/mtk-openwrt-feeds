/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_MCAST_SYSFS_
#define _NPU_MCAST_SYSFS_

#if defined(CONFIG_SYSFS)
int mtk_npu_mcast_sysfs_init(void);
void mtk_npu_mcast_sysfs_deinit(void);
#else /* !defined(CONFIG_SYSFS) */
static inline int mtk_npu_mcast_sysfs_init(void)
{
	return 0;
}

static inline void mtk_npu_mcast_sysfs_deinit(void)
{
}
#endif /* defined(CONFIG_SYSFS) */
#endif /* _NPU_MCAST_SYSFS_ */
