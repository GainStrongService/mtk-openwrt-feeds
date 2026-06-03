/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NF_HNAT_STATISTIC_H_
#define _NPU_NF_HNAT_STATISTIC_H_

#if defined(CONFIG_MTK_NPU_STATISTIC)
int mtk_npu_nf_hnat_statistic_init(void);
void mtk_npu_nf_hnat_statistic_deinit(void);
#else /* !defined(CONFIG_MTK_NPU_STATISTIC) */
int mtk_npu_nf_hnat_statistic_init(void)
{
	return 0;
}

void mtk_npu_nf_hnat_statistic_deinit(void)
{
}
#endif /* defined(CONFIG_MTK_NPU_STATISTIC) */
#endif /* _NPU_NF_HNAT_STATISTIC_H_ */
