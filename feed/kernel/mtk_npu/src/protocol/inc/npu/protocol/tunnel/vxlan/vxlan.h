/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#ifndef _NPU_VXLAN_H_
#define _NPU_VXLAN_H_

#include "npu/protocol/tunnel/vxlan/vxlan_params.h"

#if defined(CONFIG_MTK_NPU_VXLAN)
int mtk_npu_vxlan_init(void);
void mtk_npu_vxlan_deinit(void);
#else /* !defined(CONFIG_MTK_NPU_VXLAN) */
static inline int mtk_npu_vxlan_init(void)
{
	return 0;
}

static inline void mtk_npu_vxlan_deinit(void)
{
}
#endif /* defined(CONFIG_MTK_NPU_VXLAN) */
#endif /* _NPU_VXLAN_H_ */
