/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _NPU_THERMAL_H_
#define _NPU_THERMAL_H_

#include <linux/platform_device.h>

#include "npu/internal.h"

#if defined(CONFIG_MTK_NPU_THERMAL)
int mtk_npu_thermal_apply_state(void);
int mtk_npu_thermal_init(struct platform_device *pdev);
void mtk_npu_thermal_deinit(struct platform_device *pdev);
#else /* !defined(CONFIG_MTK_NPU_THERMAL) */
static inline int mtk_npu_thermal_apply_state(void)
{
	return 0;
}

static inline int mtk_npu_thermal_init(struct platform_device *pdev)
{
	return 0;
}
static inline void mtk_npu_thermal_deinit(struct platform_device *pdev)
{
}
#endif /* defined(CONFIG_MTK_NPU_THERMAL) */
#endif /* _NPU_THERMAL_H_ */
