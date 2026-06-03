/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _NPU_LOGGER_H_
#define _NPU_LOGGER_H_

#include "npu/logger-fw.h"

enum logger_state {
	LOGGER_STATE_NOT_RUNNING,
	LOGGER_STATE_FW_DEFAULT_RUNNING,
	LOGGER_STATE_RUNNING,		/* both firmware and driver are running */
};

#if defined(CONFIG_MTK_NPU_LOGGER)
void mtk_npu_logger_enable(void);
void mtk_npu_logger_disable(void);
bool mtk_npu_logger_is_running(void);
void mtk_npu_logger_running_sync(void);
int mtk_npu_logger_init(struct platform_device *pdev);
void mtk_npu_logger_deinit(struct platform_device *pdev);
#else /* !defined(CONFIG_MTK_NPU_LOGGER) */
static inline void mtk_npu_logger_enable(void)
{
}

static inline void mtk_npu_logger_disable(void)
{
}

static inline bool mtk_npu_logger_is_running(void)
{
	return false;
}

static inline void mtk_npu_logger_running_sync(void)
{
}

static inline int mtk_npu_logger_init(struct platform_device *pdev)
{
	return 0;
}

static inline void mtk_npu_logger_deinit(struct platform_device *pdev)
{
}
#endif /* defined(CONFIG_MTK_NPU_LOGGER) */
#endif /* _NPU_LOGGER_H_ */
