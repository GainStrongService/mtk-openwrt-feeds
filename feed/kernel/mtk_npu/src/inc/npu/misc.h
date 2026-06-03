/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _NPU_MISC_H_
#define _NPU_MISC_H_

#include <linux/platform_device.h>

#include "npu/npu.h"

#define DFS_SCALE_MIN			(1)
#define DFS_SCALE_MAX			(32)

enum misc_cmd_type {
	MISC_CMD_TYPE_NULL,
	MISC_CMD_TYPE_SET_DFS_SCALE,
	MISC_CMD_TYPE_SET_PPE_NUM,
	MISC_CMD_TYPE_SET_FW_LOG_LEVEL,
	MISC_CMD_TYPE_GET_FW_LOG_LEVEL,

	__MISC_CMD_TYPE_MAX,
};

enum npu_fw_log_level {
	NPU_FW_LOG_LEVEL_NONE,
	NPU_FW_LOG_LEVEL_EMERG,
	NPU_FW_LOG_LEVEL_ALERT,
	NPU_FW_LOG_LEVEL_CRIT,
	NPU_FW_LOG_LEVEL_ERROR,
	NPU_FW_LOG_LEVEL_WARN,
	NPU_FW_LOG_LEVEL_NOTICE,
	NPU_FW_LOG_LEVEL_INFO,
	NPU_FW_LOG_LEVEL_DEBUG,

	__NPU_FW_LOG_LEVEL_MAX,
};

int mtk_npu_misc_set_dfs_scale(u8 dfs_scale);
int mtk_npu_misc_set_ppe_num(void);
int mtk_npu_misc_set_fw_log_level(enum npu_fw_log_level level);
enum npu_fw_log_level mtk_npu_misc_get_fw_log_level(enum core_id core);
int mtk_npu_misc_init(struct platform_device *pdev);
void mtk_npu_misc_deinit(struct platform_device *pdev);
#endif /* _NPU_MISC_H_ */
