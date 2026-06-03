/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_GRETAP_H_
#define _NPU_GRETAP_H_

#include "npu/npu_params.h"

#if defined(CONFIG_MTK_NPU_GRETAP)
int mtk_npu_gretap_decap_param_setup(struct sk_buff *skb, struct npu_params *params);
int mtk_npu_gretap_init(void);
void mtk_npu_gretap_deinit(void);
#else /* !defined(CONFIG_MTK_NPU_GRETAP) */
static inline int mtk_npu_gretap_decap_param_setup(struct sk_buff *skb,
						    struct npu_params *params)
{
	return -ENODEV;
}

static inline int mtk_npu_gretap_init(void)
{
	return 0;
}

static inline void mtk_npu_gretap_deinit(void)
{
}
#endif /* defined(CONFIG_MTK_NPU_GRETAP) */
#endif /* _NPU_GRETAP_H_ */
