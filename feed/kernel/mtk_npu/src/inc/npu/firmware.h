/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_FW_H_
#define _NPU_FW_H_

#include <linux/platform_device.h>
#include <linux/time.h>

enum npu_role_type {
	NPU_ROLE_TYPE_MGMT,
	NPU_ROLE_TYPE_CLUSTER,

	__NPU_ROLE_TYPE_MAX,
};

u64 mtk_npu_fw_get_git_commit_id(enum npu_role_type rtype);
void mtk_npu_fw_get_built_date(enum npu_role_type rtype, struct tm *tm);
u32 mtk_npu_fw_attr_get_num(enum npu_role_type rtype);
const char *mtk_npu_fw_attr_get_property(enum npu_role_type rtype, u32 idx);
const char *mtk_npu_fw_attr_get_value(enum npu_role_type rtype,
				       const char *property);

int mtk_npu_fw_bring_up_default_cores(void);
void mtk_npu_fw_clean_up(void);
int mtk_npu_fw_init(struct platform_device *pdev);
#endif /* _NPU_FW_H_ */
