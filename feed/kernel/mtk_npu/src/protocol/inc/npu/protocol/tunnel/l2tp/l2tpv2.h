/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_L2TP_V2_H_
#define _NPU_L2TP_V2_H_

#include "npu/protocol/tunnel/l2tp/l2tp_params.h"

#if defined(CONFIG_MTK_NPU_L2TP_V2)
int mtk_npu_l2tpv2_init(void);
void mtk_npu_l2tpv2_deinit(void);
#else /* !defined(CONFIG_MTK_NPU_L2TP_V2) */
static inline int mtk_npu_l2tpv2_init(void)
{
	return 0;
}

static inline void mtk_npu_l2tpv2_deinit(void)
{
}
#endif /* defined(CONFIG_MTK_NPU_L2TP_V2) */
#endif /* _NPU_L2TP_V2_H_ */
