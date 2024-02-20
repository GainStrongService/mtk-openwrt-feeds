/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _TOPS_MISC_H_
#define _TOPS_MISC_H_

#include <linux/platform_device.h>

enum misc_cmd_type {
	MISC_CMD_TYPE_NULL,
	MISC_CMD_TYPE_SET_PPE_NUM,

	__MISC_CMD_TYPE_MAX,
};

int mtk_tops_misc_set_ppe_num(void);
int mtk_tops_misc_init(struct platform_device *pdev);
void mtk_tops_misc_deinit(struct platform_device *pdev);
#endif /* _TOPS_MISC_H_ */
