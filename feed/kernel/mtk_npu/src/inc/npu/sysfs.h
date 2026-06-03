/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_SYSFS_H_
#define _NPU_SYSFS_H_

#include <linux/platform_device.h>

#define NPU_DEV_ATTR(_p, _n, _m, _show, _store)			\
	struct device_attribute dev_attr_ ## _p ## _ ## _n = __ATTR(_n, _m, _show, _store)

#define NPU_DEV_ATTR_RO(_pre, _name)						\
	NPU_DEV_ATTR(_pre, _name, 0444, _pre ## _ ## _name ## _show, NULL)

#define NPU_DEV_ATTR_WO(_pre, _name)						\
	NPU_DEV_ATTR(_pre, _name, 0200, NULL, _pre ## _ ## _name ## _store)

#define NPU_DEV_ATTR_RW(_pre, _name)						\
	NPU_DEV_ATTR(_pre, _name, 0644, _pre ## _ ## _name ## _show, _pre ## _ ## _name ## _store)

#if defined(CONFIG_SYSFS)
extern struct kset *npu_kset;

int mtk_npu_sysfs_init(struct platform_device *pdev);
void mtk_npu_sysfs_deinit(struct platform_device *pdev);
#else /* !defined(CONFIG_SYSFS) */
static inline int mtk_npu_sysfs_init(struct platform_device *pdev)
{
	return 0;
}

void mtk_npu_sysfs_deinit(struct platform_device *pdev)
{
}
#endif /* defined(CONFIG_SYSFS) */
#endif /* _NPU_SYSFS_H_ */
