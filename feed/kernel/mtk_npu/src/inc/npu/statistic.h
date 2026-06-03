/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_STATISTIC_H_
#define _NPU_STATISTIC_H_

#include <linux/platform_device.h>

#if defined(CONFIG_MTK_NPU_STATISTIC)
int mtk_npu_statistic_init(struct platform_device *pdev);
void mtk_npu_statistic_deinit(struct platform_device *pdev);
#else /* !defined(CONFIG_MTK_NPU_STATISTIC) */
int mtk_npu_statistic_init(struct platform_device *pdev)
{
	return 0;
}

void mtk_npu_statistic_deinit(struct platform_device *pdev)
{
}
#endif /* defined(CONFIG_MTK_NPU_STATISTIC) */
#endif /* _NPU_STATISTIC_H_ */
