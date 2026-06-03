/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _NPU_MEM_TEST_H_
#define _NPU_MEM_TEST_H_

#include <linux/platform_device.h>
#include <linux/types.h>

#if defined(CONFIG_MTK_NPU_MEM_TEST)
int mtk_npu_mem_test(void);
#else /* !defined(CONFIG_MTK_NPU_MEM_TEST) */
static inline int mtk_npu_mem_test(void)
{
	return 0;
}
#endif /* defined(CONFIG_MTK_NPU_MEM_TEST) */
#endif /* _NPU_MEM_TEST_H_ */
